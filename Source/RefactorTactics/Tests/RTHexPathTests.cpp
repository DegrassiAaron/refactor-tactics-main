#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Mappa esagonale piena di raggio N, tutte le celle a MoveCost 1.
	URTHexMapAsset* MakeMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPathSimpleTest,
	"RefactorTactics.HexPath.SimplePath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPathSimpleTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeMap(5);
	const FRTHexPathResult R = URTHexPathLibrary::FindPath(M, FRTCellId(0, 0), FRTCellId(3, 0));
	TestTrue(TEXT("successo"), R.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("4 celle (start+3)"), R.Path.Num(), 4);
	TestEqual(TEXT("costo 3"), R.TotalCost, 3);
	TestTrue(TEXT("parte da start"), R.Path.Num() > 0 && R.Path[0] == FRTCellId(0, 0));
	TestTrue(TEXT("arriva a goal"), R.Path.Num() > 0 && R.Path.Last() == FRTCellId(3, 0));

	// Start == Goal -> percorso di una cella, costo 0.
	const FRTHexPathResult Same = URTHexPathLibrary::FindPath(M, FRTCellId(1, 1), FRTCellId(1, 1));
	TestTrue(TEXT("same start/goal"), Same.Status == ERTHexPathStatus::Success && Same.Path.Num() == 1 && Same.TotalCost == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPathObstacleTest,
	"RefactorTactics.HexPath.AroundObstacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPathObstacleTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeMap(3);
	FRTHexCellData Block(FRTCellId(1, 0));
	Block.bBlocksMovement = true;
	M->AddOrUpdateCell(Block); // blocca la cella intermedia diretta

	const FRTHexPathResult R = URTHexPathLibrary::FindPath(M, FRTCellId(0, 0), FRTCellId(2, 0));
	TestTrue(TEXT("successo (aggira)"), R.Status == ERTHexPathStatus::Success);
	TestFalse(TEXT("non passa per l'ostacolo"), R.Path.Contains(FRTCellId(1, 0)));
	TestTrue(TEXT("arriva comunque"), R.Path.Num() > 0 && R.Path.Last() == FRTCellId(2, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPathLayerTest,
	"RefactorTactics.HexPath.CrossLayerNeedsTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPathLayerTest::RunTest(const FString&)
{
	URTHexMapAsset* M = NewObject<URTHexMapAsset>();
	M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 0)));
	M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1)));

	// Senza arco: layer diversi NON adiacenti -> nessun percorso.
	const FRTHexPathResult NoArc = URTHexPathLibrary::FindPath(M, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1));
	TestTrue(TEXT("no path senza transizione"), NoArc.Status == ERTHexPathStatus::NoPath);

	// Con transizione esplicita (costo 2): percorso via arco.
	M->Transitions.Add(FRTHexEdge(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 2));
	const FRTHexPathResult WithArc = URTHexPathLibrary::FindPath(M, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1));
	TestTrue(TEXT("path via arco"), WithArc.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("2 celle"), WithArc.Path.Num(), 2);
	TestEqual(TEXT("costo arco 2"), WithArc.TotalCost, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPathDeterministicTest,
	"RefactorTactics.HexPath.Deterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPathDeterministicTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeMap(4);
	const FRTHexPathResult A = URTHexPathLibrary::FindPath(M, FRTCellId(-3, 0), FRTCellId(3, 0));
	const FRTHexPathResult B = URTHexPathLibrary::FindPath(M, FRTCellId(-3, 0), FRTCellId(3, 0));
	TestEqual(TEXT("stessi nodi visitati"), A.NodesVisited, B.NodesVisited);
	TestEqual(TEXT("stesso costo"), A.TotalCost, B.TotalCost);
	bool bSame = (A.Path.Num() == B.Path.Num());
	for (int32 I = 0; I < A.Path.Num() && bSame; ++I) { bSame = (A.Path[I] == B.Path[I]); }
	TestTrue(TEXT("stesso percorso (deterministico)"), bSame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexPathInvalidTest,
	"RefactorTactics.HexPath.InvalidEndpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexPathInvalidTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeMap(2);
	TestTrue(TEXT("start invalido"),
		URTHexPathLibrary::FindPath(M, FRTCellId(99, 99), FRTCellId(0, 0)).Status == ERTHexPathStatus::StartInvalid);
	TestTrue(TEXT("goal invalido"),
		URTHexPathLibrary::FindPath(M, FRTCellId(0, 0), FRTCellId(99, 99)).Status == ERTHexPathStatus::GoalInvalid);
	TestTrue(TEXT("map nulla -> start invalido"),
		URTHexPathLibrary::FindPath(nullptr, FRTCellId(0, 0), FRTCellId(1, 0)).Status == ERTHexPathStatus::StartInvalid);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
