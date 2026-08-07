#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Test/RTTestScenario.h"
#include "Test/RTTestResult.h"
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

	/** Numero massimo di tick di risoluzione per turno: tetto di sicurezza, non una regola di gioco. */
	static constexpr int32 MaxResolveTicks = 400;

	/** Tetto di turni oltre il quale il runner si ferma dichiarando `Error`: uno scenario non deve poter girare all'infinito. */
	static constexpr int32 MaxTurnsHardCap = 100;
};
