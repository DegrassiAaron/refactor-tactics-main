#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il ponte fra i **due spazi di id** di uno scenario (`#1625`).
 *
 * 🔴 **Sono davvero due, e il repository ha gia' pagato per averli confusi**: `RTScenarioSession.cpp`
 * avverte che *«`OwnerUnitId` NON e' lo `StableUnitId` … l'indice nell'array di risoluzione»*. Qui la coppia
 * e' un'altra ma la trappola e' la stessa:
 *
 *  - uno **scenario** nomina le unita' con una `FString` — *«cio' che intent, decisioni e assertion
 *    nominano; non e' un indice»*;
 *  - il **TurnLog** porta un `int32 StableUnitId`, che `ARTTurnManager::EnsureMatchRoster` assegna
 *    **dopo** aver ordinato il roster per `TeamId`, `Cell` e nome dell'actor.
 *
 * ⛔ **La corrispondenza NON e' l'ordine di dichiarazione**, e questo test esiste per renderlo impossibile
 * da credere: lo scenario che usa dichiara le unita' in un ordine **opposto** a quello che il roster
 * produce. Un ponte costruito sull'indice d'authoring — la soluzione ovvia — le scambierebbe, e a valle
 * la preview muoverebbe l'unita' sbagliata **senza un errore**.
 */
namespace
{
	/**
	 * Due unita' della **stessa squadra**, dichiarate in ordine inverso rispetto a `Cell.X`.
	 *
	 * 🔑 **La stessa squadra e' obbligatoria**: `MatchRosterLess` confronta prima `TeamId`, quindi con due
	 * squadre diverse l'ordine del roster seguirebbe quello e il caso non morderebbe. Qui il primo criterio
	 * pareggia e decide `Cell.X` — che e' esattamente il verso opposto alla dichiarazione.
	 *
	 *   dichiarazione : "zulu" (+2)  poi  "alfa" (-2)
	 *   roster (Cell.X): "alfa" (-2) poi  "zulu" (+2)   ->  alfa = 1, zulu = 2
	 *
	 * Un ponte per indice darebbe `zulu = 1`, e questo test lo vede.
	 */
	const TCHAR* IdBridgeJson = TEXT(R"JSON(
	{
	  "scenarioId": "Harness.IdBridgeProbe",
	  "version": 1,
	  "mapRadius": 3,
	  "units": [
	    { "id": "zulu", "hero": "Hero.Gadget", "team": 0, "cell": [2, 0, 0] },
	    { "id": "alfa", "hero": "Hero.Wraith", "team": 0, "cell": [-2, 0, 0] },
	    { "id": "nemico", "hero": "Hero.Riktor", "team": 1, "cell": [0, 2, 0] }
	  ],
	  "turns": [ { "intents": [ { "unit": "alfa", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [ { "type": "UnitAtCell", "unit": "alfa", "cell": [-1, 0, 0] } ]
	}
	)JSON");

	UWorld* MakeIdBridgeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyIdBridgeWorld(UWorld* World)
	{
		if (!World) { return; }
		if (GEngine) { GEngine->DestroyWorldContext(World); }
		World->DestroyWorld(false);
	}
}

/**
 * **Il risultato porta fuori la corrispondenza vera, non l'ordine di dichiarazione.**
 *
 * ⚠️ Senza questo ponte la traccia di uno scenario non e' collegabile all'authoring: il TurnLog dice
 * *«l'unita' 1 si e' mossa»* e nessuno sa **quale** unita' dello scenario sia. E' il prerequisito perche' la
 * preview d'editor possa muovere il marcatore giusto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioIdBridgeTest,
	"RefactorTactics.Scenario.ResultCarriesTheAuthoringIdOfEachUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioIdBridgeTest::RunTest(const FString&)
{
	FRTTestScenario Scenario;
	FString Error;
	if (!TestTrue(TEXT("lo scenario si carica"),
		URTScenarioLoader::LoadFromString(IdBridgeJson, Scenario, Error)))
	{
		AddError(Error);
		return false;
	}

	UWorld* World = MakeIdBridgeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);
	DestroyIdBridgeWorld(World);

	// --- Il ponte c'e', e nomina tutte le unita' ------------------------------------------------------
	// ⚠️ Anti-vacuita': una mappa vuota renderebbe verdi per assenza tutte le asserzioni qui sotto.
	if (!TestEqual(TEXT("il risultato porta un'identita' per unita'"),
		Result.ScenarioIdByUnitId.Num(), 3))
	{
		AddError(FString::Printf(TEXT("mappa: %d voci"), Result.ScenarioIdByUnitId.Num()));
		return false;
	}

	// --- L'ordine del ROSTER, non quello di dichiarazione ---------------------------------------------
	//
	// 🔴 E' la meta' che un ponte per indice fallisce: `alfa` e' dichiarata SECONDA e ordinata PRIMA.
	const FString* Primo = Result.ScenarioIdByUnitId.Find(1);
	const FString* Secondo = Result.ScenarioIdByUnitId.Find(2);
	if (!TestNotNull(TEXT("esiste l'unita' con StableUnitId 1"), Primo)
		|| !TestNotNull(TEXT("esiste l'unita' con StableUnitId 2"), Secondo))
	{
		return false;
	}

	TestEqual(TEXT("l'id 1 e' 'alfa', che e' dichiarata SECONDA ma sta piu' a sinistra"),
		*Primo, FString(TEXT("alfa")));
	TestEqual(TEXT("l'id 2 e' 'zulu', dichiarata PRIMA"), *Secondo, FString(TEXT("zulu")));

	// La premessa dell'anti-vacuita', resa esplicita: se l'ordine di dichiarazione coincidesse con quello
	// del roster, questo test non distinguerebbe le due implementazioni.
	TestNotEqual(TEXT("dichiarazione e roster danno ordini DIVERSI: il test puo' distinguerli"),
		*Primo, FString(TEXT("zulu")));

	// --- Lo `0` resta libero, ed e' [D-063] -----------------------------------------------------------
	TestNull(TEXT("nessuna unita' ha id 0: quello significa «nessuna unita' dichiarata»"),
		Result.ScenarioIdByUnitId.Find(0));

	// ⛔ **Non si verifica che il TurnLog nomini questi id**, e vale dirlo invece di ometterlo:
	// `FRTTestResult` porta le tracce **serializzate** (`FRTTurnTrace::Bytes`), non le voci, e
	// deserializzarle qui aggiungerebbe un passo senza aggiungere prova — il soggetto di questo test e'
	// il ponte fra i due spazi di id, non il contenuto del log. Che i due si incontrino davvero lo
	// misurera' il consumatore, quando la preview leggera' la traccia.

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
