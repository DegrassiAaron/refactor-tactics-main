// Anteprima di pianificazione: cio' che il giocatore DEVE vedere prima di premere Spazio.
//
// Questi test coprono il WIRING (le celle giuste arrivano all'actor mappa), non il disegno: che a schermo si
// veda un esagono colorato resta al PIE, per costruzione. Il calcolo delle forme e' gia' coperto altrove
// (`HexCombat.Shape*`): qui si verifica che l'anteprima usi QUELLO, e non un secondo calcolo che potrebbe
// divergere dall'esito (invariante #1: la presentazione non decide, riceve).

#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Ability/RTActionData.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: nella unity build condividono la translation unit.
	UWorld* MakePreviewWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPreviewWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Mappa piatta di raggio 3 sul layer 0: basta a contenere ogni forma provata qui. */
	URTHexMapAsset* MakePreviewMap(UObject* Outer)
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(Outer, 3);
		return Map;
	}

	ARTHexMapActor* SpawnPreviewMapActor(UWorld* World)
	{
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		if (Actor)
		{
			Actor->MapAsset = MakePreviewMap(Actor);
		}
		return Actor;
	}
}

/**
 * L'anteprima dell'area riceve ESATTAMENTE le celle che il combat colpirebbe. Se qui e in `HexHitCells`
 * uscissero due insiemi diversi, il giocatore vedrebbe una zona e ne subirebbe un'altra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewHitCellsMatchCombatTest,
	"RefactorTactics.Preview.HitCellsMatchCombatShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewHitCellsMatchCombatTest::RunTest(const FString&)
{
	UWorld* World = MakePreviewWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* HexMap = SpawnPreviewMapActor(World);
	if (!TestNotNull(TEXT("actor mappa"), HexMap)) { DestroyPreviewWorld(World); return false; }

	const FRTCellId From(0, 0, 0);
	const FRTCellId Target(2, 0, 0);

	// Area raggio 1 attorno al bersaglio: la forma la decide il combat, non l'anteprima.
	const TArray<FRTCellId> Expected =
		URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Area, From, Target, /*RangeCells=*/ 4, /*AreaRadius=*/ 1);
	TestTrue(TEXT("la forma di riferimento non e' vuota (altrimenti il test non proverebbe nulla)"),
		Expected.Num() > 0);

	HexMap->SetPreviewHitCells(Expected, /*AllyCells=*/ TArray<FRTCellId>());

	TestEqual(TEXT("l'anteprima ha lo stesso numero di celle del combat"),
		HexMap->NumPreviewHitCells(), Expected.Num());
	for (const FRTCellId& Cell : Expected)
	{
		TestTrue(TEXT("ogni cella colpita dal combat e' nell'anteprima"), HexMap->IsPreviewHitCell(Cell));
	}
	// Una cella fuori dall'area non deve comparire: l'anteprima non deve allargare la minaccia.
	TestFalse(TEXT("una cella lontana NON e' nell'anteprima"), HexMap->IsPreviewHitCell(FRTCellId(-3, 0, 0)));

	DestroyPreviewWorld(World);
	return true;
}

/**
 * Il fuoco amico si vede PRIMA del lock-in. E' la meta' che nessun test di danno puo' coprire: che il danno
 * arrivi all'alleato e' gia' verificato da `Actions.AoE.FriendlyFire`; qui si verifica che il giocatore lo
 * sappia mentre puo' ancora cambiare idea.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewAllyInAreaIsFlaggedTest,
	"RefactorTactics.Preview.AllyInAreaIsFlagged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewAllyInAreaIsFlaggedTest::RunTest(const FString&)
{
	UWorld* World = MakePreviewWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* HexMap = SpawnPreviewMapActor(World);
	if (!TestNotNull(TEXT("actor mappa"), HexMap)) { DestroyPreviewWorld(World); return false; }

	const FRTCellId Target(2, 0, 0);
	const FRTCellId AllyCell(2, -1, 0); // vicina al bersaglio: dentro l'area di raggio 1

	const TArray<FRTCellId> Hit =
		URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Area, FRTCellId(0, 0, 0), Target, 4, 1);
	TestTrue(TEXT("la cella dell'alleato e' davvero dentro l'area"), Hit.Contains(AllyCell));

	TArray<FRTCellId> Allies;
	Allies.Add(AllyCell);
	HexMap->SetPreviewHitCells(Hit, Allies);

	TestEqual(TEXT("un solo alleato segnalato"), HexMap->NumPreviewAllyHitCells(), 1);
	TestTrue(TEXT("la cella dell'alleato e' marcata come fuoco amico"), HexMap->IsPreviewAllyHitCell(AllyCell));
	TestTrue(TEXT("resta comunque fra le celle colpite"), HexMap->IsPreviewHitCell(AllyCell));
	TestFalse(TEXT("la cella del bersaglio NON e' fuoco amico"), HexMap->IsPreviewAllyHitCell(Target));

	DestroyPreviewWorld(World);
	return true;
}

/**
 * Annullare il piano spegne l'anteprima. Un'anteprima che sopravvive al piano che la giustificava e' peggio
 * di nessuna anteprima: mostra una minaccia che non esiste piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewClearedWhenPlanIsCancelledTest,
	"RefactorTactics.Preview.ClearedWhenPlanIsCancelled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewClearedWhenPlanIsCancelledTest::RunTest(const FString&)
{
	UWorld* World = MakePreviewWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* HexMap = SpawnPreviewMapActor(World);
	if (!TestNotNull(TEXT("actor mappa"), HexMap)) { DestroyPreviewWorld(World); return false; }

	const TArray<FRTCellId> Hit =
		URTHexCombatLibrary::HexHitCells(ERTAbilityShape::Area, FRTCellId(0, 0, 0), FRTCellId(2, 0, 0), 4, 1);
	TArray<FRTCellId> Allies;
	Allies.Add(FRTCellId(2, -1, 0));
	HexMap->SetPreviewHitCells(Hit, Allies);
	HexMap->SetPreviewReachableCells(Hit);
	TestTrue(TEXT("precondizione: l'anteprima e' accesa"), HexMap->NumPreviewHitCells() > 0);

	HexMap->SetPreviewHitCells(TArray<FRTCellId>(), TArray<FRTCellId>());
	HexMap->SetPreviewReachableCells(TArray<FRTCellId>());

	TestEqual(TEXT("nessuna cella colpita dopo l'annullamento"), HexMap->NumPreviewHitCells(), 0);
	TestEqual(TEXT("nessun fuoco amico residuo"), HexMap->NumPreviewAllyHitCells(), 0);
	TestEqual(TEXT("nessuna cella raggiungibile residua"), HexMap->NumPreviewReachableCells(), 0);

	DestroyPreviewWorld(World);
	return true;
}

/**
 * L'anteprima delle celle raggiungibili riceve quello che le viene passato, senza filtrarlo. Il budget e i
 * blocchi sono gia' applicati da `ReachableCells` (coperto da `HexSim.ReachableRespectsTerrainCost`): se
 * l'anteprima li riapplicasse, sarebbe un secondo calcolo che puo' divergere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPreviewReachableCellsArePassedThroughTest,
	"RefactorTactics.Preview.ReachableCellsArePassedThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPreviewReachableCellsArePassedThroughTest::RunTest(const FString&)
{
	UWorld* World = MakePreviewWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }
	ARTHexMapActor* HexMap = SpawnPreviewMapActor(World);
	if (!TestNotNull(TEXT("actor mappa"), HexMap)) { DestroyPreviewWorld(World); return false; }

	TArray<FRTCellId> Reachable;
	Reachable.Add(FRTCellId(1, 0, 0));
	Reachable.Add(FRTCellId(0, 1, 0));
	Reachable.Add(FRTCellId(-1, 1, 0));
	HexMap->SetPreviewReachableCells(Reachable);

	TestEqual(TEXT("tutte le celle raggiungibili arrivano all'anteprima"),
		HexMap->NumPreviewReachableCells(), Reachable.Num());
	for (const FRTCellId& Cell : Reachable)
	{
		TestTrue(TEXT("la cella raggiungibile e' nell'anteprima"), HexMap->IsPreviewReachableCell(Cell));
	}
	TestFalse(TEXT("una cella non passata NON compare"), HexMap->IsPreviewReachableCell(FRTCellId(3, 0, 0)));

	DestroyPreviewWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
