#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "RTScenarioDraft.generated.h"

/**
 * Esito di un'operazione d'authoring, come lo vede chi l'ha chiesta.
 *
 * Un `bool` costringerebbe la UI a indovinare **perche'** qualcosa non e' andato, e `#1115` chiede un errore
 * leggibile che nomini il problema. Il codice serve alla logica (un ramo, un'icona), la frase che lo
 * accompagna serve all'occhio: servono entrambi, e sono due cose diverse.
 */
UENUM(BlueprintType)
enum class ERTScenarioAuthoringResult : uint8
{
	/** Fatto. */
	Success,
	/** L'ID non esiste nell'indice, o il file che dichiara non si legge. */
	NotFound,
	/** Lo scenario non passa `URTScenarioLoader::Validate`. La frase dice quale campo. */
	Invalid,
	/** Validato ma non scritto: percorso non scrivibile, disco pieno, file in sola lettura. */
	WriteFailed,
	/** Si e' chiesto qualcosa a un draft che non ha nessuno scenario aperto. */
	NoScenarioOpen
};

/**
 * Vista di sola lettura sull'intestazione di uno scenario aperto.
 *
 * ⚠️ E' una **fotografia**, non il modello: modificarla non modifica niente. E' la proprieta' che rende
 * impossibile alla UI di diventare authority — non per disciplina di chi scrive il Blueprint, ma per
 * costruzione ([ADR-0010](../../../docs/decisions/adr-0010-esposizione-blueprint-scenario-harness.md)).
 */
USTRUCT(BlueprintType)
struct FRTScenarioSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	FString ScenarioId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	int32 Version = 0;

	/** I tag come sono scritti nel file: la forma canonica per i filtri la fa `URTScenarioIndex`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	TArray<FString> Tags;

	/** Vuota se lo scenario genera l'arena da `MapRadius` invece di partire da un allestimento. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	FString Fixture;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	int32 MapRadius = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	int32 UnitCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	int32 TurnCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	int32 ExpectationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	int32 VariantCount = 0;
};

/**
 * Vista di sola lettura su una unita' schierata. Vedi la nota su `FRTScenarioSummary`: e' una fotografia.
 *
 * Porta `FRTCellId` ed `ERTHexDirection`, cioe' il vocabolario del gioco — non una stringa e non un angolo
 * libero, come `#1115` richiede esplicitamente. Erano gia' `BlueprintType` prima di questo lavoro.
 */
USTRUCT(BlueprintType)
struct FRTScenarioUnitView
{
	GENERATED_BODY()

	/** Lo **Stable Unit ID**: e' cio' che intent, decisioni e assertion nominano. Non e' un indice. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	FName HeroId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	FRTCellId Cell = FRTCellId();

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	ERTHexDirection Facing = ERTHexDirection::E;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Scenario")
	bool bBotControlled = false;
};

/**
 * Lo scenario **in lavorazione**: apertura, creazione, validazione, salvataggio.
 *
 * C++ puro e senza `UObject`: la logica sta qui e la porta Blueprint (`URTScenarioAuthoring`) traduce e basta.
 * E' la stessa separazione di `FRTReplayViewModel` / `URTReplayViewerSubsystem`, e serve a una cosa concreta —
 * i test girano headless senza costruire niente.
 *
 * ⚠️ **Non e' un secondo modello di scenario.** Possiede un `FRTTestScenario` e non lo duplica: ogni operazione
 * finisce sul dato canonico, e `GetScenario()` restituisce quello vero. Un draft che tenesse una propria copia
 * dei campi sarebbe esattamente la seconda autorita' che
 * [ADR-0010](../../../docs/decisions/adr-0010-esposizione-blueprint-scenario-harness.md) vieta.
 */
USTRUCT()
struct REFACTORTACTICS_API FRTScenarioDraft
{
	GENERATED_BODY()

	/** Uno scenario nuovo, con l'identita' e l'arena dichiarate. Sostituisce quello eventualmente aperto. */
	void NewScenario(const FString& ScenarioId, int32 MapRadius);

	/**
	 * Apre uno scenario dall'indice per **ID**, non per percorso: l'identita' e' dichiarata dal file e la
	 * cartella e' un dettaglio di storage (`RTScenarioIndex.h`).
	 */
	ERTScenarioAuthoringResult OpenById(const FString& ScenarioId, FString& OutError);

	/** Apre uno scenario da un percorso esplicito. Per i file che l'indice non copre (test, cartelle nuove). */
	ERTScenarioAuthoringResult OpenFromFile(const FString& FilePath, FString& OutError);

	/** Chiude senza salvare. Dopo questa, `IsOpen()` e' falso e le operazioni rispondono `NoScenarioOpen`. */
	void Close();

	bool IsOpen() const { return bOpen; }

	/** `URTScenarioLoader::Validate` sullo scenario corrente. L'unico giudice della validita', e sta dove stava. */
	ERTScenarioAuthoringResult Validate(FString& OutError) const;

	/** Valida e scrive via `URTScenarioLoader::SaveToFile`. Uno scenario invalido non tocca il disco. */
	ERTScenarioAuthoringResult SaveToFile(const FString& FilePath, FString& OutError) const;

	/**
	 * Salva dove l'indice dice che questo scenario vive. Fallisce con `NotFound` se l'ID non e' ancora
	 * nell'indice — un file nuovo non ha ancora un posto, e inventarglielo qui significherebbe decidere una
	 * convenzione di cartelle che appartiene a chi le organizza.
	 */
	ERTScenarioAuthoringResult SaveInPlace(FString& OutError) const;

	// --- editing dell'initial state (#1115) -------------------------------------------------------------
	//
	// ⚠️ Nessuna di queste funzioni decide se una cella e' buona: lo chiedono a
	// `URTScenarioLoader::ValidateUnitPlacement`, che e' la stessa regola che `Validate` applica allo scenario
	// intero. Se una di loro si trovasse a confrontare una distanza o a leggere `bBlocksMovement` da se',
	// sarebbe una seconda copia della regola, e diverge dalla prima al primo campo aggiunto.
	//
	// Tutte falliscono **senza modificare niente**: uno scenario mezzo modificato e' peggio di uno non
	// modificato, perche' il secondo si nota.

	/**
	 * Schiera una unita' nuova. `Invalid` se l'id e' vuoto o gia' preso, se l'eroe non e' a catalogo, o se la
	 * cella e' fuori arena / occupata / bloccante — e `OutError` dice **quale** delle cose.
	 *
	 * Ritorna l'esito tipizzato come le sorelle, e non un `bool`: con un booleano il chiamante C++ non puo'
	 * distinguere «rifiutato dalle regole» da «nessuno scenario aperto», e la facade dovrebbe rifare la
	 * guardia — cioe' duplicare un controllo e il suo messaggio per poterli tradurre.
	 */
	ERTScenarioAuthoringResult AddUnit(const FString& UnitId, FName HeroId, int32 TeamId, const FRTCellId& Cell,
		ERTHexDirection Facing, FString& OutError);

	/**
	 * Sposta una unita' gia' schierata. `NotFound` se l'id non esiste, `Invalid` se il piazzamento non passa.
	 *
	 * ⚠️ **`Invalid` non significa necessariamente «cella sbagliata».** La rivalidazione riguarda l'unita'
	 * INTERA, quindi un difetto che l'unita' si portava dietro — un eroe uscito dal catalogo dopo un cambio di
	 * roster, un loadout diventato illegale — la blocca anche su una cella perfettamente buona, e `OutError`
	 * nomina quel campo invece della cella. E' la scelta prudente: applicare uno spostamento a una unita' che
	 * il gioco non saprebbe schierare produrrebbe uno scenario che si salva e non si esegue. Ma va letto
	 * l'errore, non dato per scontato che parli della destinazione.
	 */
	ERTScenarioAuthoringResult MoveUnit(const FString& UnitId, const FRTCellId& Cell, FString& OutError);

	/**
	 * Ritira una unita'. `NotFound` se non c'e'.
	 *
	 * ⚠️ Intent, decisioni e assertion che NOMINANO l'unita' restano: `OutError` li **conta** quando ce ne
	 * sono, cosi' chi ritira sa subito che lo scenario non e' piu' salvabile finche' non li sistema. Toglierli
	 * qui cancellerebbe in silenzio il lavoro di chi ha scritto quel turno; non dirlo lo lascerebbe scoprire
	 * al salvataggio, che con l'authoring dei turni ancora da fare (`#1116`) e' un vicolo cieco.
	 */
	ERTScenarioAuthoringResult RemoveUnit(const FString& UnitId, FString& OutError);

	/** Ruota una unita' schierata. Il facing e' `ERTHexDirection`, mai un angolo libero. */
	ERTScenarioAuthoringResult SetUnitFacing(const FString& UnitId, ERTHexDirection Facing, FString& OutError);

	/** Indice in `Units` dello Stable Unit ID, o `INDEX_NONE`. Gli id sono identita', non posizioni. */
	int32 IndexOfUnit(const FString& UnitId) const;

	// --- authoring dei turni (#1116) --------------------------------------------------------------------
	//
	// La fetta e' volutamente sottile: `Move` e `Wait`, due expectation. Un authoring di tutte le assertion
	// che il formato conosce sarebbe il «mega framework» che la spec vieta.

	/** Aggiunge un turno vuoto in coda e ne restituisce l'indice. Il primo turno e' `0`. */
	int32 AddTurn();

	int32 NumTurns() const { return Scenario.Turns.Num(); }

	/**
	 * Dichiara che l'unita' percorre `Path` in quel turno. Il percorso finisce in `FRTScenarioIntent::Move`,
	 * che e' il campo che il formato ha gia'.
	 *
	 * ⚠️ **Non verifica che il percorso sia percorribile**, ed e' voluto: la raggiungibilita' dipende dallo
	 * stato al momento del turno — dove sono finite le altre unita', cosa e' cambiato sulla mappa — e questo
	 * e' un dato, non una simulazione. Chi vuole sapere se una cella e' raggiungibile lo chiede a
	 * `GetReachableCells`, che gira il servizio runtime. Il giudizio vero lo da' il resolver, all'esecuzione.
	 *
	 * Sostituisce l'intent che quell'unita' avesse gia' in quel turno: un editor in cui la stessa unita'
	 * accumula due piani nello stesso turno e' un editor che mente.
	 */
	ERTScenarioAuthoringResult SetMoveIntent(int32 TurnIndex, const FString& UnitId,
		const TArray<FRTCellId>& Path, FString& OutError);

	/**
	 * Dichiara che l'unita' **non fa nulla** in quel turno.
	 *
	 * ⚠️ Si scrive come un intent che nomina l'unita' e nient'altro, non come `ability: "Action.Wait"`:
	 * `Action.Wait` risolve in `NormalMovement`, e il loader esige un bersaglio per ogni abilita' che non
	 * risolva su se' stessa — quindi quella forma verrebbe **rifiutata** in lettura. L'attesa e' l'assenza di
	 * un piano, e il formato la esprime cosi'.
	 */
	ERTScenarioAuthoringResult SetWaitIntent(int32 TurnIndex, const FString& UnitId, FString& OutError);

	/** Toglie l'intent di quell'unita' da quel turno. `NotFound` se non ce n'era uno. */
	ERTScenarioAuthoringResult RemoveIntent(int32 TurnIndex, const FString& UnitId, FString& OutError);

	/** Aggiunge `UnitAtCell`: dove l'unita' deve trovarsi alla fine. */
	ERTScenarioAuthoringResult AddExpectationUnitAtCell(const FString& UnitId, const FRTCellId& Cell,
		FString& OutError);

	/** Aggiunge `LogEventCount`: quante volte un evento del TurnLog deve comparire. */
	ERTScenarioAuthoringResult AddExpectationLogEventCount(ERTLogCategory Category, uint8 Outcome, int32 Count,
		FString& OutError);

	/** Toglie l'assertion in quella posizione. `NotFound` se l'indice non esiste. */
	ERTScenarioAuthoringResult RemoveExpectation(int32 ExpectationIndex, FString& OutError);

	// --- preview (#1116) ---------------------------------------------------------------------------------

	/**
	 * Le celle che l'unita' puo' raggiungere dalla sua posizione iniziale, **chieste al servizio runtime**.
	 *
	 * ⚠️ Questo metodo non contiene un algoritmo: costruisce l'arena con
	 * `URTScenarioArenaLibrary::BuildArena`, monta uno `FRTHexSnapshot` e gira la domanda a
	 * `URTHexSimLibrary::ReachableCells`, che ha gia' applicato budget, blocchi, occupanti e archi. Un
	 * pathfinder scritto qui sarebbe il secondo gioco che il §3 della spec vieta — e mostrerebbe celle che il
	 * resolver poi non concede.
	 *
	 * Il budget viene da `URTHeroData::MovePoints`, cioe' dalla stessa fonte da cui `ARTUnit` lo prende
	 * (`MoveRange = Hero->MovePoints`): non e' una stima, e' il valore.
	 *
	 * ⚠️ **E' l'anteprima del PRIMO turno.** Parte dall'initial state, non dallo stato dopo i turni gia'
	 * scritti: ricostruirlo richiederebbe eseguire lo scenario, che e' `RUN` (#1117) e non una preview.
	 *
	 * @param Outer proprietario dell'arena temporanea. `nullptr` -> transient package.
	 */
	TArray<FRTCellId> GetReachableCells(const FString& UnitId, UObject* Outer, FString& OutError) const;

	FRTScenarioSummary GetSummary() const;
	TArray<FRTScenarioUnitView> ListUnits() const;

	/** Il dato canonico. Lo usa la facade per parlare col runner; **non** viene esposto a Blueprint. */
	const FRTTestScenario& GetScenario() const { return Scenario; }
	FRTTestScenario& MutableScenario() { return Scenario; }

	/** Il percorso da cui si e' aperto, se si e' aperto da un file. Vuoto per uno scenario nuovo. */
	const FString& GetSourcePath() const { return SourcePath; }

private:
	FRTTestScenario Scenario;
	FString SourcePath;

	/**
	 * Distingue «nessuno scenario aperto» da «scenario aperto e vuoto». Senza questo flag un draft appena
	 * costruito e uno appena azzerato sarebbero indistinguibili, e la UI mostrerebbe un editor vuoto invece
	 * di un invito ad aprire qualcosa.
	 */
	bool bOpen = false;
};
