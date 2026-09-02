#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "RTScenarioAuthoring.generated.h"

class URTHexMapAsset;

/**
 * **La porta Blueprint dello Scenario Harness.** L'unica.
 *
 * `URTScenarioLoader`, `URTScenarioRunner` e `FRTScenarioSession` restano C++ e non sono raggiungibili da
 * Blueprint: questa classe li chiama per conto della UI e ne traduce gli esiti. Decisione registrata in
 * [ADR-0010](../../../docs/decisions/adr-0010-esposizione-blueprint-scenario-harness.md).
 *
 * ⚠️ **Cosa Blueprint NON puo' fare, per costruzione e non per disciplina:**
 *
 * - costruire un `FRTTestScenario`: le nove `USTRUCT` del formato non sono `BlueprintType`, quindi non
 *   esistono come pin;
 * - scrivere JSON: la serializzazione passa da `URTScenarioLoader::SaveToFile` e da nessun altro posto;
 * - decidere se uno scenario e' valido: lo decide `Validate`, e questa classe riporta la risposta;
 * - modificare il modello attraverso una vista: `FRTScenarioSummary` e `FRTScenarioUnitView` sono
 *   fotografie, e cambiarle non cambia niente.
 *
 * ⚠️ **E cosa questa classe non deve mai diventare**: un posto dove vive una regola di gioco. Traduce. Se si
 * trovasse a decidere un esito, un costo o una raggiungibilita', quella decisione sarebbe del runtime e
 * andrebbe spostata — e' il §3 di `spec-tactical-designer.md`.
 *
 * **Perche' un `UObject` e non un subsystem**: `#1115` colloca il Composer nel viewport dell'Editor, dove non
 * esiste una `GameInstance`. Un oggetto creato da factory funziona in Editor, in PIE e headless nei test, e
 * il widget lo tiene come variabile. Vedi ADR-0010 §4.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTScenarioAuthoring : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Crea un draft vuoto. Il chiamante ne diventa proprietario: un widget lo tiene come variabile, e piu'
	 * draft possono convivere senza che nulla sia globale.
	 *
	 * `Outer` esplicito perche' un oggetto d'authoring creato in Editor deve poter appartenere a chi lo usa;
	 * `nullptr` lo mette sotto il transient package, che e' il default sensato per uno strumento.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	static URTScenarioAuthoring* CreateScenarioDraft(UObject* Outer);

	/** Uno scenario nuovo. Non e' valido finche' non ha unita' e assertion, ed e' corretto cosi'. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	void NewScenario(const FString& ScenarioId, int32 MapRadius = 3);

	/**
	 * Apre uno scenario per **ID**, chiedendo all'indice dove vive. L'identita' e' dichiarata dal file: il
	 * percorso e' un dettaglio di storage e non si compone a mano.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult OpenById(const FString& ScenarioId, FString& OutError);

	/** Apre da un percorso esplicito, per i file che l'indice non copre. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult OpenFromFile(const FString& FilePath, FString& OutError);

	/** Chiude senza salvare. Le operazioni successive rispondono `NoScenarioOpen`. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	void Close();

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	bool IsOpen() const { return Draft.IsOpen(); }

	/** L'unico giudice della validita' e' `URTScenarioLoader::Validate`: qui se ne riporta la risposta. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult Validate(FString& OutError) const;

	/**
	 * Salva nel percorso dato. **Esplicito**: nessuna scrittura implicita a ogni modifica.
	 *
	 * Uno scenario invalido non tocca il disco, e i due esiti restano distinti — `Invalid` accusa lo
	 * scenario, `WriteFailed` accusa il disco.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SaveToFile(const FString& FilePath, FString& OutError);

	/** Salva dove lo scenario gia' viveva. `NotFound` se non e' mai stato su disco. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SaveInPlace(FString& OutError);

	// --- editing dell'initial state (#1115) -------------------------------------------------------------

	/**
	 * Schiera una unita' nuova. `Success`, oppure `Invalid` con `OutError` che nomina il problema: id gia'
	 * preso, eroe fuori catalogo, cella fuori arena, cella occupata, cella che blocca il movimento.
	 *
	 * ⚠️ La cella arriva come `FRTCellId` — il click sulla mappa lo traduce l'Editor, ma **quale** cella sia
	 * valida lo decide il runtime: qui si chiama la stessa regola che `Validate` applica allo scenario intero.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult AddUnit(const FString& UnitId, FName HeroId, int32 TeamId, FRTCellId Cell,
		ERTHexDirection Facing, FString& OutError);

	/** Sposta una unita' schierata. `NotFound` se l'id non esiste, `Invalid` se la cella non va bene. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult MoveUnit(const FString& UnitId, FRTCellId Cell, FString& OutError);

	/** Ritira una unita'. `NotFound` se non c'e'. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult RemoveUnit(const FString& UnitId, FString& OutError);

	/** Ruota una unita'. Il facing e' `ERTHexDirection`, mai un angolo libero. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetUnitFacing(const FString& UnitId, ERTHexDirection Facing, FString& OutError);

	// --- authoring dei turni (#1116) --------------------------------------------------------------------

	/** Aggiunge un turno vuoto in coda. `OutTurnIndex` e' l'indice del turno creato; il primo e' `0`. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult AddTurn(int32& OutTurnIndex, FString& OutError);

	/** Toglie un turno. Serve perche' `Validate` accetta un turno vuoto e il runner lo giocherebbe. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult RemoveTurn(int32 TurnIndex, FString& OutError);

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	int32 GetTurnCount() const { return Draft.NumTurns(); }

	/**
	 * Assegna un `Move` all'unita' in quel turno: le celle scelte dal viewport finiscono in
	 * `FRTScenarioIntent::Move`. Sostituisce l'intent che quell'unita' avesse gia' in quel turno.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetMoveIntent(int32 TurnIndex, const FString& UnitId,
		const TArray<FRTCellId>& Path, FString& OutError);

	/** Assegna un `Wait`: l'unita' e' nel turno e non fa nulla. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetWaitIntent(int32 TurnIndex, const FString& UnitId, FString& OutError);

	// ---------------------------------------------------------------------------------------------------
	// GLI SLOT DI COMBATTIMENTO — `#1626`
	//
	// 🔴 **Il Composer di `#1116` authorava Move e Wait**, e questo bastava per uno scenario di movimento.
	// Uno scenario di combattimento — che e' cio' per cui il Tactical Designer esiste — richiedeva di
	// editare il JSON a mano, cioe' esattamente il posto da cui `TD 0.2` doveva togliere il designer.
	//
	// ⚠️ **La UI si modella sui DUE SLOT, non su una lista di sette azioni.** Un selettore d'azione
	// renderebbe inesprimibile «muovi e attacca», che il doc header di `FRTScenarioIntent` dichiara essere
	// «la norma, non un caso limite». Ciascuna di queste scrive solo il proprio campo: chiamarle in
	// sequenza compone il piano. Il contratto e i limiti stanno su `FRTScenarioDraft`.
	// ---------------------------------------------------------------------------------------------------

	/** Slot movimento, forma mobilita'. `None` come abilita' svuota lo slot. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetDashIntent(int32 TurnIndex, const FString& UnitId, FName DashAbility,
		FRTCellId DashCell, FString& OutError);

	/** Slot principale senza bersaglio: le azioni che risolvono su chi le usa. `None` svuota lo slot. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetMainAction(int32 TurnIndex, const FString& UnitId, FName AbilityId,
		FString& OutError);

	/** Slot principale, bersaglio per unita'. Svuota la forma per cella. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetMainActionOnUnit(int32 TurnIndex, const FString& UnitId, FName AbilityId,
		const FString& TargetUnitId, FString& OutError);

	/** Slot principale, bersaglio per cella. Svuota la forma per unita'. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetMainActionOnCell(int32 TurnIndex, const FString& UnitId, FName AbilityId,
		FRTCellId TargetCell, FString& OutError);

	/** Modificatore: la rotazione finale dichiarata. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetFacingIntent(int32 TurnIndex, const FString& UnitId, bool bDeclare,
		ERTHexDirection Facing, FString& OutError);

	/** Modificatore: il bordo di copertura dichiarato. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetCoverEdgeIntent(int32 TurnIndex, const FString& UnitId, bool bDeclare,
		ERTHexDirection Edge, FString& OutError);

	/** Slot reattivo: reazione e condizione. `None` come reazione toglie anche la condizione. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult SetReactionIntent(int32 TurnIndex, const FString& UnitId, FName ReactionAbility,
		FName ConditionId, int32 ConditionParam, FString& OutError);

	/** Toglie l'intent di quell'unita' da quel turno. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult RemoveIntent(int32 TurnIndex, const FString& UnitId, FString& OutError);

	/** Aggiunge `UnitAtCell`: dove l'unita' deve trovarsi alla fine. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult AddExpectationUnitAtCell(const FString& UnitId, FRTCellId Cell, FString& OutError);

	/** Aggiunge `LogEventCount`: quante volte un evento del TurnLog deve comparire. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult AddExpectationLogEventCount(ERTLogCategory Category, uint8 Outcome, int32 Count,
		FString& OutError);

	/** Toglie l'assertion in quella posizione. L'indice e' quello che `ListExpectations` riporta. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult RemoveExpectation(int32 ExpectationIndex, FString& OutError);

	/**
	 * Gli intent di un turno, per mostrarli e per sapere quale togliere.
	 *
	 * ⚠️ Senza questa e la sorella, la facade poteva SCRIVERE un turno e non mostrarlo: `RemoveIntent` e
	 * `RemoveExpectation(indice)` chiedevano di nominare qualcosa che nessuna API sapeva elencare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	TArray<FRTScenarioIntentView> ListIntents(int32 TurnIndex) const;

	/** Le assertion dello scenario, con l'indice che `RemoveExpectation` accetta. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	TArray<FRTScenarioExpectationView> ListExpectations() const;

	/**
	 * Le celle raggiungibili dall'unita' nell'initial state, **chieste al servizio runtime**.
	 *
	 * ⚠️ Questa funzione non calcola niente: gira la domanda a `URTHexSimLibrary::ReachableCells`, che ha
	 * gia' applicato budget, blocchi, occupanti e archi. Un pathfinder nell'Editor mostrerebbe celle che il
	 * resolver poi non concede — ed e' esattamente cio' che `#1116` vieta.
	 *
	 * L'Editor la usa per colorare il viewport; l'esito lo decide comunque il resolver, all'esecuzione.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	TArray<FRTCellId> GetReachableCells(const FString& UnitId, FString& OutError);

	// --- esecuzione (#1117) -----------------------------------------------------------------------------

	/**
	 * `RUN`. Esegue lo scenario dal **percorso di gioco reale** e riempie il report.
	 *
	 * ⚠️ Costruisce un mondo TEMPORANEO per la corsa e lo smonta subito dopo. Serve perche' l'Editor non ne
	 * ha uno fuori dal PIE, ed e' anche cio' che rende ogni esecuzione indipendente: un mondo riusato
	 * conserverebbe unita' e turn manager della corsa precedente, e il secondo `RUN` misurerebbe il residuo
	 * del primo — un difetto che in questa stessa serie di PR e' gia' costato un test verde per la ragione
	 * sbagliata.
	 *
	 * `Success` significa che **l'esecuzione e' avvenuta**, non che lo scenario sia passato: l'esito del gioco
	 * sta in `OutReport.Outcome`, e sono due domande diverse. Un `FAIL` e' l'informazione piu' preziosa che
	 * questo strumento produce, e farlo apparire come un guasto dello strumento la butterebbe via.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult Run(FRTScenarioRunReport& OutReport, FString& OutError);

	/**
	 * `RESET`. Scarta il report e riporta lo scenario all'**initial state canonico**.
	 *
	 * ⚠️ Non e' un undo della partita: `Run` non ha modificato lo scenario, quindi non c'e' niente da
	 * annullare. Torna a cio' che il file DICHIARA, che e' un'altra cosa dallo stato precedente.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	ERTScenarioAuthoringResult Reset(bool& bOutDiscardedEdits, FString& OutError);

	/** L'esito dell'ultima esecuzione. `bHasRun` falso se non ce n'e' stata nessuna. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	FRTScenarioRunReport GetLastRunReport() const { return Draft.GetLastRunReport(); }

	/** Il TurnLog dell'ultima esecuzione, consultabile senza uscire dall'editor. */
	// `BlueprintCallable` e non `Pure`: un nodo puro viene rivalutato a ogni uso e a ogni frame, e questo
	// restituisce un array che il chiamante deve tenere, non un valore da leggere di continuo.
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	TArray<FRTScenarioLogEntryView> GetLastRunLog() const { return Draft.GetLastRunLog(); }

	/**
	 * Gli `HeroId` del roster, per popolare un menu senza scrivere i nomi a mano.
	 *
	 * `GetHeroIds()` e non `GetHeroRoster()`: il secondo istanzia quattro `URTHeroData` **con tutte le loro
	 * abilita'** a ogni chiamata, e una tendina che si riapre le pagherebbe tutte per leggere quattro nomi.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	static TArray<FName> ListHeroIds();

	/** Fotografia dell'intestazione. Modificarla non modifica lo scenario. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	FRTScenarioSummary GetSummary() const { return Draft.GetSummary(); }

	/** Le unita' schierate, nell'ordine del file. Fotografie: vedi `GetSummary`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	TArray<FRTScenarioUnitView> ListUnits() const { return Draft.ListUnits(); }

	/**
	 * L'ARENA dello scenario aperto: la **stessa** che il runner costruira' eseguendolo.
	 *
	 * 🔑 **Perche' passa da qui e non da `URTScenarioArenaLibrary` direttamente.** Quel builder prende un
	 * `FRTTestScenario`, cioe' il modello — che [ADR-0010] tiene `non-BlueprintType` e fuori dalla portata
	 * dell'editor apposta. Senza questa riga un consumatore d'editor avrebbe due sole strade: toccare il
	 * modello, oppure ricostruirsi l'arena da `GetSummary()` leggendo `Fixture` e `MapRadius` — e la seconda
	 * e' peggio della prima, perche' sembra innocua e **perde gli override di cella**, che il summary non
	 * porta. Una preview costruita cosi' mostrerebbe una mappa che il runner poi non usa.
	 *
	 * ⚠️ **Non e' una sessione e non e' cache**: costruisce e restituisce, come `GetSummary` fotografa.
	 * Chiamarla due volte da' due asset distinti, entrambi validi.
	 *
	 * @param Outer proprietario dell'asset. L'asset non deve sopravvivere a chi lo espone.
	 * @return `nullptr` se nessuno scenario e' aperto, o se la fixture/il raggio non producono un'arena.
	 *
	 * ⚠️ **NON e' una `UFUNCTION`, deliberatamente** — come `ARTTurnManager::KnowledgeForTeamPublic`, e per
	 * la stessa ragione. Esporla in Blueprint consegnerebbe l'`URTHexMapAsset` a un grafo, che potrebbe
	 * scriverci dentro: l'arena tornerebbe modificabile proprio dal lato che ADR-0010 tiene fuori dal
	 * modello. Il consumatore e' il modulo Editor, che e' C++ e non ne ha bisogno.
	 */
	URTHexMapAsset* BuildArena(UObject* Outer) const;

	/** Gli ID che l'indice conosce, filtrabili per tag. Vuoti = elenco completo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	static TArray<FString> ListScenarioIds(const FString& FilterTagA, const FString& FilterTagB);

	/** I tag esistenti nel corpus, per costruire un filtro senza scriverli a mano. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	static TArray<FString> ListScenarioTags();

	/** Frase leggibile per un esito, da mostrare accanto all'icona. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Scenario")
	static FText DescribeResult(ERTScenarioAuthoringResult Result);

	/**
	 * IL LOG FILTRATO PER CATEGORIA, e la sorgente resta intatta — `#1630`.
	 *
	 * 🔑 **Filtrare non cambia il dato**: la funzione e' pura e prende l'elenco per valore-const,
	 * quindi rimuovere un filtro fa ricomparire esattamente cio' che c'era. E' il criterio d'accettazione
	 * scritto come firma invece che come promessa — un filtro che mutasse la sorgente non potrebbe
	 * nemmeno compilare cosi'.
	 *
	 * Un insieme di categorie **vuoto** non filtra niente e restituisce tutto: «nessun filtro attivo» e
	 * «tutti i filtri spenti» sono lo stesso stato per chi guarda, e distinguerli darebbe una vista vuota
	 * a chi ha appena aperto il pannello.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Scenario")
	static TArray<FRTScenarioLogEntryView> FilterLogByCategory(
		const TArray<FRTScenarioLogEntryView>& Entries, const TArray<ERTLogCategory>& Categories);

	/** Il draft C++ sottostante. Per il codice C++ e i test: **non** e' esposto a Blueprint. */
	FRTScenarioDraft& GetDraft() { return Draft; }
	const FRTScenarioDraft& GetDraft() const { return Draft; }

private:
	/**
	 * La logica sta qui dentro, non in questa classe. La facade traduce e basta — e' la separazione che
	 * rende i test headless possibili senza costruire un `UObject`, la stessa di `FRTReplayViewModel`.
	 */
	FRTScenarioDraft Draft;
};
