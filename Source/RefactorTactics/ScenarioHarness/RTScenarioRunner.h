#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "Ability/RTWorkbenchVariant.h" // il default della variante e' la baseline
#include "RTScenarioRunner.generated.h"

class UWorld;

/**
 * Esecutore degli scenari di test automatici.
 *
 * PRINCIPIO NON NEGOZIABILE: il runner entra dagli **stessi ingressi del giocatore** — scrive i piani sulle
 * unita' (`PlannedCell`/`PlannedPath`, come fa il controller dopo un click) e chiama `LockInAndResolve()`.
 * Non chiama `SetActorLocation`, non applica danni a mano, non conosce il resolver. Se lo facesse, un test
 * verde non direbbe nulla sul codice che gira davvero in partita.
 *
 * Il turn manager e il resolver **non sanno** di essere sotto test: nessun ramo `if (IsTest)` nel gameplay.
 * L'unico appiglio e' `ARTTurnManager::PlanBotsForTest()`, che esisteva gia' per i test d'integrazione.
 */
UCLASS()
class REFACTORTACTICS_API URTScenarioRunner : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Esegue lo scenario nel mondo dato e ritorna l'esito.
	 *
	 * Il mondo viene popolato dal runner (mappa esagonale generata, unita' dal catalogo eroi, turn manager):
	 * passare un mondo gia' pieno di unita' produce risultati non riproducibili, quindi il runner si aspetta
	 * un mondo **vuoto** — quello di un test di automazione o una PIE appena avviata su sandbox.
	 *
	 * Non lancia mai: ogni fallimento diventa `Outcome`, perche' un harness che crasha non sa dire perche'.
	 */
	static FRTTestResult Run(UWorld* World, const FRTTestScenario& Scenario);

	/**
	 * Una singola esecuzione, senza varianti. E' cio' che `Run` chiama — una volta per scenario normale, una
	 * per variante.
	 *
	 * @param bTearDownAfter rimuove unita' e turn manager dal mondo alla fine. Vero solo fra due varianti, che
	 *        condividono il mondo: nel caso normale il mondo lo possiede il chiamante, e ripulirlo qui gli
	 *        toglierebbe da sotto i piedi gli actor su cui potrebbe voler guardare.
	 *
	 * @param Variant variante sperimentale dello Skill Workbench, applicata al kit dopo l'equipaggiamento
	 *        e prima del primo turno. **Il default e' la baseline**, quindi ogni chiamante esistente
	 *        continua a eseguire esattamente cio' che eseguiva: la variante e' un ingresso in piu', non un
	 *        comportamento nuovo di chi non la passa.
	 */
	static FRTTestResult RunSingle(UWorld* World, const FRTTestScenario& Scenario, bool bTearDownAfter,
		const FRTWorkbenchVariant& Variant = FRTWorkbenchVariant());

	/**
	 * Carica lo scenario per ID (`Movement.Basic`), lo esegue e ne **scrive il report**.
	 * E' il punto d'ingresso della console e dell'auto-run: un solo posto dove «eseguire uno scenario»
	 * significa anche «lasciarne traccia leggibile».
	 *
	 * @param OutReportDirectory cartella della run appena scritta (vuota se la scrittura e' fallita).
	 */
	static FRTTestResult RunById(UWorld* World, const FString& ScenarioId, FString& OutReportDirectory);

	/** ID di tutti gli scenari versionati sotto `Scenarios/`, in ordine alfabetico. */
	static TArray<FString> ListScenarioIds();

	/**
	 * Numero massimo di tick di risoluzione per turno: tetto di sicurezza, non una regola di gioco.
	 *
	 * ⚠️ **I due consumatori lo spendono in valute diverse, e il cambio del 2026-09-03 riguarda solo uno.**
	 *  - `URTScenarioRunner::RunSingle` avanza a passo fisso di `0,05 s`, quindi `900` valgono **45 s** di
	 *    tempo simulato: abbondante, e insensibile alla velocita' di playback.
	 *  - `FRTScenarioSession::Tick` incrementa una volta per **FRAME** renderizzato (`bPumpTurnManager =
	 *    false`), quindi a 60 fps `900` valgono **15 s** di tempo reale — ed e' questo il vincolo stretto.
	 *
	 * 🔑 **Perche' non e' piu' `400`.** Con `PlaybackCellsPerSecond` a `1.44` (`#1878`) un round 2v2 pieno
	 * costa circa **8,5 s**: a 60 fps sono ~**513 frame**, cioe' oltre il vecchio tetto di `400` (6,7 s).
	 * La sessione live sarebbe abortita con *«il turno N non ha finito di risolvere entro 400 passi»* — la
	 * stessa diagnosi che `Scenarios/AutoBattle/ArenaV01.json` documenta gia' per il frame rate scatenato.
	 *
	 * ⛔ **La suite non avrebbe visto il difetto**: gira dal percorso a passo fisso, che aveva 20 s di
	 * budget e ne usava 3. Il tetto e' stato ricalcolato sul consumatore stretto, non su quello comodo.
	 */
	static constexpr int32 MaxResolveTicks = 900;

	/**
	 * Tetto di turni che un FILE non puo' superare: uno scenario non deve poter girare all'infinito.
	 *
	 * 🔴 **Questo commento diceva «oltre il quale il runner si ferma dichiarando `Error`», ed era falso**:
	 * ne' `Run` ne' `RunSingle` hanno mai letto questa costante — `git grep MaxTurnsHardCap -- Source` dava un
	 * solo risultato, la sua stessa dichiarazione. Il consumatore esiste da `#957` ed e'
	 * `URTScenarioLoader::Validate`, che rifiuta un `maxTurns` piu' grande: e' un tetto sul FORMATO, non un
	 * comportamento del runner. La differenza conta per chi costruisce uno scenario **in memoria** senza
	 * passare dal loader — i test lo fanno — perche' li' nessuno lo applica.
	 */
	static constexpr int32 MaxTurnsHardCap = 100;

	/**
	 * Quante volte al massimo un file puo' chiedere di rigiocare lo stesso scenario (`repeatCount`).
	 *
	 * Stessa natura di `MaxTurnsHardCap` e stessa ragione: senza, `"repeatCount": 100000` passerebbe la
	 * validazione e farebbe girare centomila partite complete dentro `EveryShippedScenarioRuns` — un blocco
	 * senza diagnostica, in un harness il cui principio dichiarato e' che un test appeso non deve somigliare a
	 * un test lento. Cento ripetizioni sono gia' due ordini di grandezza oltre il corpus di determinismo, che
	 * ne usa due.
	 */
	static constexpr int32 MaxRepeatCount = 100;
};
