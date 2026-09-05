#pragma once

#include "CoreMinimal.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "Turn/RTMatchStateHash.h" // FRTUnitStateDigest: lo stato d'ingresso del diff (#1630)

/**
 * IL DIFF FRA DUE STATI, dichiarato qui perche' sia VERIFICABILE — `#1630`.
 *
 * 🔑 Le funzioni vivono nel `.cpp`, ma la dichiarazione sta in un header per una ragione sola: un
 * calcolo che nessun test puo' chiamare si verifica solo attraverso una run intera, e allora fallisce
 * insieme a tutto il resto senza dire quale meta' e' rotta.
 */
namespace RTScenarioStateDiff
{
	/** I digest delle unita' vive, ORDINATI per `UnitId` — l'iterazione di una `TMap` non e' garantita. */
	REFACTORTACTICS_API TArray<FRTUnitStateDigest> Snapshot(
		const TMap<FString, TWeakObjectPtr<ARTUnit>>& UnitsById);

	/** Il diff fra due elenchi: campi cambiati, comparse e sparizioni, in ordine di `UnitId`. */
	REFACTORTACTICS_API TArray<FRTUnitStateDiff> Build(const TArray<FRTUnitStateDigest>& Before,
		const TArray<FRTUnitStateDigest>& After);
}
#include "Ability/RTWorkbenchVariant.h" // per valore: la sessione la possiede, non la osserva

class UWorld;
class ARTUnit;
class ARTTurnManager;
class URTHexMapAsset;
// Forward e non `#include`: il decisore la riceve per riferimento const, e tirare dentro
// `RTReactionOpportunityTypes.h` qui la farebbe ricompilare a ogni consumatore della sessione.
struct FRTReactionOpportunity;

/**
 * Esecuzione di uno scenario come **macchina a stati**, avanzabile un passo alla volta.
 *
 * Esiste per una ragione misurata: risolvendo tutti i turni dentro `BeginPlay`, lo scenario finiva *prima
 * del primo fotogramma* e non si vedeva giocare. Con la sessione, chi guarda puo' farla avanzare **un passo
 * per frame** e vedere il playback scorrere; chi verifica in automatico la fa girare in un ciclo stretto e
 * ottiene lo stesso identico esito.
 *
 * Le due strade guidano la **stessa** sessione, non due copie della stessa logica: se divergessero, un test
 * verde non direbbe piu' niente su quel che si vede a schermo.
 *
 * Il turn manager e il resolver restano ignari: la sessione entra dagli stessi ingressi del giocatore
 * (piani sulle unita', `LockInAndResolve`).
 */
class REFACTORTACTICS_API FRTScenarioSession
{
public:
	/**
	 * 🔴 **Sbinda il decisore, e deve esistere perche' `TearDown()` non basta.**
	 *
	 * `URTScenarioRunner::Run` esegue gli scenari non-variante con `RunSingle(..., bTearDownAfter=false)` e
	 * `ARTGameMode` non chiama `TearDown` affatto: la sessione e' una locale che muore al `return`, mentre
	 * `BindRaw(this, ...)` ha lasciato un puntatore GREZZO dentro un delegate posseduto dall'ATTORE, che
	 * sopravvive. Chi tenesse vivo il mondo — o ne eseguisse un secondo scenario — chiamerebbe attraverso
	 * `this` gia' distrutto, oppure troverebbe `IsBound()` vero e prenderebbe il ramo `test-override`
	 * ignorando in silenzio le `decisions` del secondo scenario.
	 *
	 * Il distruttore chiude il ciclo dove si chiude davvero: la vita dell'oggetto.
	 */
	~FRTScenarioSession();

	/**
	 * Pausa in secondi **prima** di risolvere ogni turno: il tempo per guardare dove sono le unita' prima che
	 * si muovano. 0 = nessuna pausa (e' il valore delle esecuzioni headless, dove non c'e' nessuno a guardare).
	 */
	float TurnPauseSeconds = 0.f;

	/**
	 * La variante sperimentale dello Skill Workbench, applicata al kit delle unita' **dopo**
	 * l'equipaggiamento e **prima** del primo turno. Vuota = baseline, ed e' il default.
	 *
	 * ⚠️ **Vive qui e non nel formato scenario**, per decisione ([`#1982`], strada C): `FRTScenarioVariant`
	 * e' un canary di EQUITA' — chiede che le varianti producano lo stesso TurnLog per dimostrare che un
	 * ingresso *non* ha avuto effetto — e questa serve l'opposto. Non essendo serializzata, non puo'
	 * raggiungere un dato di produzione nemmeno per errore.
	 *
	 * ⚠️ **Dopo il loadout, non prima**: `EquipLoadout` e' la configurazione di PRODUZIONE — la stessa che
	 * `FRTMatchBootstrapper` monta in partita, varianti d'arma comprese. L'esperimento sta sopra di essa,
	 * altrimenti il loadout lo sovrascriverebbe e il designer vedrebbe la variante sparire senza un motivo
	 * visibile.
	 *
	 * Campo pubblico come `TurnPauseSeconds`: si imposta prima di `Start`, e chi non lo tocca esegue la
	 * baseline.
	 */
	FRTWorkbenchVariant WorkbenchVariant;

	/**
	 * Allestisce il mondo: arena, unita' dal catalogo, turn manager.
	 *
	 * @return false se lo scenario non e' eseguibile — l'esito e' gia' un `Error` con il motivo in
	 *         `GetResult()`. Non lancia mai: un harness che crasha non sa dire perche'.
	 */
	bool Start(UWorld* World, const FRTTestScenario& Scenario);

	/**
	 * Avanza di un passo.
	 *
	 * @param DeltaSeconds tempo trascorso: scandisce le pause e, in modalita' pompata, il playback.
	 * @param bPumpTurnManager **true** quando nessuno sta ticcando il mondo (test headless: il turn manager va
	 *        fatto avanzare a mano); **false** in gioco, dove il mondo lo ticca gia' e pomparlo lo farebbe
	 *        correre al doppio della velocita'.
	 */
	void Step(float DeltaSeconds, bool bPumpTurnManager);

	/** Vero quando non c'e' piu' niente da fare: assertion valutate ed esito definitivo. */
	bool IsFinished() const { return State == EState::Finished; }

	/**
	 * Questo nome di capability esiste nel vocabolario — disponibile o no?
	 *
	 * Esposto perche' un refuso va colto **senza eseguire lo scenario**. Eseguendolo si vede solo il primo
	 * turno che il runner raggiunge: `RT_Showcase_Relay_v01` chiedeva `Facing` al turno 4 e nessuno se n'e'
	 * accorto per mesi, perche' bastava un turno precedente `Blocked` a fermare la corsa prima. Il controllo
	 * statico non ha quel punto cieco — `RefactorTactics.Scenario.ShippedScenariosRequireKnownCapabilities`.
	 *
	 * ⚠️ Un `false` NON e' un'attesa: e' un errore di scrittura dello scenario. Chi lo chiama tratti i due
	 * casi in modo diverso, o rimette insieme cio' che questa funzione serve a separare.
	 */
	static bool IsKnownCapability(const FString& Capability);

	/**
	 * Il gioco sa fare questa cosa **oggi**?
	 *
	 * ⚠️ Non e' `IsKnownCapability`, ed e' la distinzione che l'intero vocabolario esiste per fare: un nome
	 * **noto** puo' essere indisponibile — vale `Blocked`, ed e' un'attesa legittima — mentre un nome
	 * **ignoto** e' un refuso e vale `Error`. Chiedere «e' noto?» dove serve «e' disponibile?» rimette
	 * insieme i due insiemi che `KnownUnavailableCapabilities()` serve a separare.
	 */
	static bool IsAvailableCapability(const FString& Capability);

	const FRTTestResult& GetResult() const { return Result; }

	/** Turno corrente (1-based) mentre gira, per la diagnostica a schermo. 0 = non ancora partito. */
	int32 GetCurrentTurn() const { return TurnIndex + 1; }

	/**
	 * Rimuove dal mondo cio' che questa sessione ha messo: le unita' schierate e il turn manager.
	 *
	 * Serve alle VARIANTI, che rigiocano lo stesso allestimento nello stesso mondo. Senza, la seconda variante
	 * troverebbe le unita' della prima e — molto peggio — un turn manager con il numero di turno e la Team
	 * Knowledge gia' scritti: un canary sull'informazione confronterebbe due partite contaminate proprio
	 * sull'informazione. Il chiamante verifica che il mondo sia tornato vuoto invece di fidarsi.
	 *
	 * La MAPPA resta: `BuildScenarioArena` riusa l'actor e ne riscrive l'asset, quindi distruggerla
	 * costringerebbe a ricostruire le istanze senza nessun guadagno.
	 */
	void TearDown();

	/**
	 * Il marchio che porta ogni unita' spawnata da uno scenario (`#2223`).
	 *
	 * 🔴 **Serve perche' in PIE non si puo' ricreare il mondo.** Fuori dal PIE ogni corsa costruisce un
	 * `UWorld` temporaneo e parte pulita per costruzione; in PIE il mondo e' quello della sessione, e senza
	 * questo marchio lo scenario successivo si sommerebbe al precedente — che e' esattamente il difetto
	 * osservato lanciando scenari uno dopo l'altro su `L_DevSandbox`.
	 *
	 * ⛔ **E' un marchio e non un conteggio, e la differenza e' tutta qui**: in PIE lo scenario gira DENTRO
	 * una partita, e le `ARTUnit` in campo non sono tutte sue. Sgomberare *«tutte le unita' del mondo»*
	 * distruggerebbe la partita che lo ospita. Si toglie solo cio' che uno scenario ha messo.
	 *
	 * ⚠️ Vive in `AActor::Tags`, non in un campo di `ARTUnit`: e' una marcatura dell'HARNESS su un tipo di
	 * gioco, e non deve entrare nel contratto di quel tipo ne' in cio' che il gioco serializza.
	 */
	static const FName SpawnedByScenarioTag;

	/**
	 * Toglie dal mondo le unita' marcate da una corsa precedente. Restituisce quante ne ha tolte.
	 *
	 * ⚠️ **Chiamata all'inizio, non alla fine.** Un teardown al termine non scatterebbe mai su uno scenario
	 * interrotto a meta' — ed e' proprio quello che lascia il campo sporco. Sgomberare all'ingresso rende la
	 * corsa pulita *qualunque cosa* sia successa alla precedente.
	 */
	static int32 ClearScenarioSpawnedUnits(UWorld* InWorld);

private:
	enum class EState : uint8
	{
		NotStarted,
		/** In attesa prima di risolvere il turno: e' la finestra in cui si guarda il campo. */
		PauseBeforeTurn,
		/** Il turn manager sta risolvendo: qui scorre il playback, ed e' cio' che si vede. */
		Resolving,
		Finished
	};

	/** Scrive i piani del turno sulle unita', come farebbe il giocatore, e chiude la pianificazione. */
	void BeginTurn();

	/**
	 * Traduce gli `intents` del turno corrente in piani sulle unita': azione, bersaglio, facing, percorso.
	 *
	 * E' la parte piu' lunga di `BeginTurn` e sta a parte perche' fa una cosa sola — scrivere piani — mentre
	 * cio' che la circonda ne fa altre: verificare le capability richieste, azzerare i piani del turno prima,
	 * far decidere i bot. Passa dal percorso REALE come farebbe il giocatore: uno scenario che scrivesse
	 * direttamente lo stato salterebbe le regole che dovrebbe verificare.
	 *
	 * Il turn manager arriva per RIFERIMENTO e non come puntatore letto dal membro: quando questa funzione
	 * viene chiamata `BeginTurn` ha gia' verificato che esista, e un riferimento dice che il controllo e'
	 * stato fatto — un puntatore riaprirebbe la domanda in un punto dove la risposta e' gia' nota.
	 */
	void ApplyScenarioIntents(ARTTurnManager& TurnManagerRef);

	/**
	 * Seleziona in PIE l'unita' dichiarata da `PreviewUnit`, cosi' l'ANTEPRIMA del suo attacco compare da
	 * sola durante la pausa prima del primo turno.
	 *
	 * Scrive in anticipo il solo piano d'attacco del turno 1 per quell'unita': `BeginTurn` riscrivera' gli
	 * stessi valori un istante dopo, quindi e' idempotente e non anticipa nessuna decisione di gioco.
	 * Senza player controller — cioe' headless — non fa nulla, ed e' la ragione per cui l'esito logico degli
	 * scenari non puo' dipendere da questo campo.
	 */
	void ApplyPreviewSelection();

	/** Azzera i piani, calcola l'hash dello stato e valuta le assertion. */
	void Finish();

	EState State = EState::NotStarted;
	FRTTestScenario Scenario;
	FRTTestResult Result;

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<ARTTurnManager> TurnManager;
	TObjectPtr<URTHexMapAsset> Map;
	TMap<FString, TWeakObjectPtr<ARTUnit>> UnitsById;

	/**
	 * Lo stato delle unita' COME ERANO all'avvio, per il diff di `#1630`.
	 *
	 * ⚠️ Si cattura alla fine di `Start()`, quando l'allestimento e' finito e nessun turno e' ancora
	 * girato: un istante prima le unita' non esistono, uno dopo il primo turno le ha gia' toccate.
	 */
	TArray<FRTUnitStateDigest> InitialUnitStates;

	int32 TurnIndex = 0;
	float PauseElapsed = 0.f;

	/**
	 * Le decisioni del turno corrente e, in parallelo, quali sono gia' state consumate.
	 *
	 * Due array e non un cursore per unita': `DecideScriptedResponse` scandisce dall'inizio e prende la
	 * prima non consumata che nomina il proprietario della finestra. L'ordine di dichiarazione E' l'ordine
	 * di consumo, ed e' cio' che `ScriptedDecisionsAreConsumedInOrder` pinna.
	 */
	TArray<FRTScenarioDecision> PendingDecisions;
	TArray<bool> PendingConsumed;

	/**
	 * Le decisioni applicate in questo turno, in forma leggibile e con gli id di SCENARIO.
	 *
	 * Serve al messaggio del rifiuto: il token che il gioco riceve e' `FIRE:<indice di risoluzione>`, e un
	 * referto che lo riportasse non nominerebbe il bersaglio che qualcuno ha scritto nello scenario —
	 * manderebbe a rileggere il turno intero per capire quale risposta sia stata rifiutata.
	 */
	TArray<FString> AppliedDecisionDescs;

	/**
	 * Chi risponde alle finestre in questa esecuzione: `scenario`, `test-override`, `none`. Deciso una volta
	 * al bind, e copiato nel referto (task 7).
	 */
	FString DecisionSource = TEXT("none");

	/** Il decisore scriptato: risponde con la coda del turno, stringa vuota se nulla combacia. */
	FString DecideScriptedResponse(const FRTReactionOpportunity& Opportunity, int32 OwnerUnitId);

	/** Sbinda il PROPRIO decisore, se e solo se e' questa sessione ad averlo legato. Idempotente. */
	void UnbindOwnDecider();

	/**
	 * Le unita' nell'ordine di risoluzione, catturate alla PRIMA finestra del turno e riusate per tutte le
	 * successive.
	 *
	 * ⚠️ Ricostruirlo a ogni finestra sarebbe sbagliato, non solo costoso: `MakeCurrentSnapshot` filtra
	 * `IsAlive()`, mentre il resolver costruisce il proprio array **una volta sola** per l'intera
	 * risoluzione. Un `FIRE` che uccide un bersaglio lo toglierebbe dallo snapshot successivo e sposterebbe
	 * di uno tutti gli indici a valle: la finestra dopo mapperebbe `OwnerUnitId` sull'unita' sbagliata.
	 */
	TArray<ARTUnit*> RuntimeUnitsForTurn;

	/**
	 * Lo scenario schiera almeno un'unita' guidata dal bot, quindi ogni turno passa dal pianificatore del gioco
	 * prima di risolvere.
	 *
	 * Si tiene qui invece di riscorrere `Scenario.Units` a ogni turno per una ragione che non e' la velocita':
	 * il flag dice cosa e' stato SPAWNATO, e uno scenario il cui unico bot fosse stato scartato in fase di
	 * spawn non chiamerebbe un pianificatore che non ha nessuno da pianificare.
	 */
	bool bHasBotUnits = false;

	/**
	 * Perche' la sessione si e' fermata prima della fine, se si e' fermata: nomina la **capability mancante**
	 * e il turno che la chiedeva. Vuoto = lo scenario e' arrivato in fondo.
	 */
	FString BlockedBy;

	/**
	 * Errore di SCRITTURA dello scenario scoperto durante l'esecuzione — tipicamente un'abilita' che l'eroe
	 * non possiede. Vuoto = nessuno.
	 *
	 * Esiste separato da `BlockedBy` perche' ha precedenza su tutto: uno scenario scritto male non produce un
	 * verdetto sul gioco. Se cadesse come `Fail`, il report direbbe «regressione» per un errore di battitura.
	 */
	FString ErroredBy;

	/** Vedi `FRTTestResult::Notes`: cio' che e' successo e che spiega un risultato altrimenti muto. */
	TArray<FString> Notes;

	/**
	 * TurnLog di TUTTO lo scenario, nell'ordine in cui e' stato scritto.
	 *
	 * Accumulato qui e non letto dal `TurnManager` a fine corsa perche' `ARTTurnManager::TurnLog` viene
	 * AZZERATO a ogni `LockInAndResolve`: a scenario finito conterrebbe il solo ultimo turno, e un'assertion
	 * su tre turni sarebbe verde o rossa per il motivo sbagliato. Le voci si appendono quando il turno ha
	 * finito di risolvere, cioe' nell'unico istante in cui il log di quel turno e' completo e non ancora
	 * sostituito.
	 *
	 * ORDINE DI SCRITTURA, non forma canonica: la forma canonica — quella serializzata e quella che entra in
	 * `URTTurnLogLibrary::HashTurnLog` — e' ordinata, quindi l'hash e' invariante per permutazione e non sa
	 * niente delle sequenze. `LogEventOrder` deve leggere questa.
	 */
	TArray<FRTTurnLogEntry> ScenarioLog;

	/** Tetto di sicurezza sulla risoluzione di UN turno: fallire e' meglio che girare all'infinito. */
	int32 ResolveTicks = 0;
};
