#include "Misc/AutomationTest.h"
#include "Core/RTTypes.h"
#include "Grid/RTGridLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr float RTTestCellSize = 200.0f;
	const FVector RTTestOrigin = FVector::ZeroVector;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridCellToWorldCenterTest,
	"RefactorTactics.Grid.CellToWorldReturnsCellCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridCellToWorldCenterTest::RunTest(const FString&)
{
	// Cella (2,3) con cella da 200 -> centro a ((2+0.5)*200, (3+0.5)*200) = (500, 700).
	const FVector World = URTGridLibrary::CellToWorld(FRTGridCoord(2, 3), RTTestOrigin, RTTestCellSize);
	TestEqual(TEXT("X = centro cella 2"), World.X, 500.0);
	TestEqual(TEXT("Y = centro cella 3"), World.Y, 700.0);
	TestEqual(TEXT("Z = Origin.Z"), World.Z, 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridWorldToCellTest,
	"RefactorTactics.Grid.WorldToCellFindsContainingCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridWorldToCellTest::RunTest(const FString&)
{
	// Un punto qualsiasi dentro la cella (2,3) deve mappare a (2,3).
	const FRTGridCoord Cell = URTGridLibrary::WorldToCell(FVector(510.0, 690.0, 0.0), RTTestOrigin, RTTestCellSize);
	TestTrue(TEXT("(510,690) -> cella (2,3)"), Cell == FRTGridCoord(2, 3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridRoundtripTest,
	"RefactorTactics.Grid.CellRoundtripHoldsForWholeGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridRoundtripTest::RunTest(const FString&)
{
	// DoD del checkpoint 1.3: WorldToCell(CellToWorld(c)) == c per ogni cella della griglia 10x10.
	for (int32 Y = 0; Y < 10; ++Y)
	{
		for (int32 X = 0; X < 10; ++X)
		{
			const FRTGridCoord Cell(X, Y);
			const FVector World = URTGridLibrary::CellToWorld(Cell, RTTestOrigin, RTTestCellSize);
			const FRTGridCoord Back = URTGridLibrary::WorldToCell(World, RTTestOrigin, RTTestCellSize);
			TestTrue(FString::Printf(TEXT("Roundtrip cella (%d,%d)"), X, Y), Back == Cell);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridInsideBoundsTest,
	"RefactorTactics.Grid.IsInsideGridRespectsBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridInsideBoundsTest::RunTest(const FString&)
{
	TestTrue(TEXT("(0,0) dentro 10x10"), URTGridLibrary::IsInsideGrid(FRTGridCoord(0, 0), 10, 10));
	TestTrue(TEXT("(9,9) dentro 10x10"), URTGridLibrary::IsInsideGrid(FRTGridCoord(9, 9), 10, 10));
	TestFalse(TEXT("(-1,0) fuori"), URTGridLibrary::IsInsideGrid(FRTGridCoord(-1, 0), 10, 10));
	TestFalse(TEXT("(10,0) fuori"), URTGridLibrary::IsInsideGrid(FRTGridCoord(10, 0), 10, 10));
	TestFalse(TEXT("(0,10) fuori"), URTGridLibrary::IsInsideGrid(FRTGridCoord(0, 10), 10, 10));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridManhattanTest,
	"RefactorTactics.Grid.ManhattanDistanceSumsAbsoluteDeltas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridManhattanTest::RunTest(const FString&)
{
	TestEqual(TEXT("(1,1)->(4,5) = 7"), URTGridLibrary::ManhattanDistance(FRTGridCoord(1, 1), FRTGridCoord(4, 5)), 7);
	TestEqual(TEXT("stessa cella = 0"), URTGridLibrary::ManhattanDistance(FRTGridCoord(3, 3), FRTGridCoord(3, 3)), 0);
	TestEqual(TEXT("delta negativi in valore assoluto"), URTGridLibrary::ManhattanDistance(FRTGridCoord(5, 5), FRTGridCoord(1, 2)), 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridWithinRangeTest,
	"RefactorTactics.Grid.IsWithinRangeChecksManhattan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridWithinRangeTest::RunTest(const FString&)
{
	const FRTGridCoord From(2, 2);
	TestTrue(TEXT("stessa cella entro range 4"), URTGridLibrary::IsWithinRange(From, FRTGridCoord(2, 2), 4));
	TestTrue(TEXT("distanza 4 entro range 4"), URTGridLibrary::IsWithinRange(From, FRTGridCoord(2, 6), 4));
	TestTrue(TEXT("distanza 3 (diagonale) entro range 4"), URTGridLibrary::IsWithinRange(From, FRTGridCoord(4, 3), 4));
	TestFalse(TEXT("distanza 5 fuori range 4"), URTGridLibrary::IsWithinRange(From, FRTGridCoord(2, 7), 4));
	TestFalse(TEXT("distanza 8 fuori range 4"), URTGridLibrary::IsWithinRange(From, FRTGridCoord(6, 6), 4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridLineOfSightTest,
	"RefactorTactics.Grid.LineOfSightBlockedByObstacles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridLineOfSightTest::RunTest(const FString&)
{
	// Linea verticale (0,0)->(0,4): un ostacolo a (0,2) blocca; uno a (1,2) no.
	TestFalse(TEXT("ostacolo sulla linea verticale blocca"),
		URTGridLibrary::HasLineOfSight(FRTGridCoord(0, 0), FRTGridCoord(0, 4), { FRTGridCoord(0, 2) }));
	TestTrue(TEXT("ostacolo fuori linea non blocca"),
		URTGridLibrary::HasLineOfSight(FRTGridCoord(0, 0), FRTGridCoord(0, 4), { FRTGridCoord(1, 2) }));

	// Linea orizzontale (0,0)->(4,0): ostacolo a (2,0) blocca.
	TestFalse(TEXT("ostacolo sulla linea orizzontale blocca"),
		URTGridLibrary::HasLineOfSight(FRTGridCoord(0, 0), FRTGridCoord(4, 0), { FRTGridCoord(2, 0) }));

	// Diagonale (0,0)->(4,4): ostacolo a (2,2) blocca.
	TestFalse(TEXT("ostacolo sulla diagonale blocca"),
		URTGridLibrary::HasLineOfSight(FRTGridCoord(0, 0), FRTGridCoord(4, 4), { FRTGridCoord(2, 2) }));

	// Nessun ostacolo -> libera; From/To non bloccano.
	TestTrue(TEXT("nessun ostacolo -> libera"),
		URTGridLibrary::HasLineOfSight(FRTGridCoord(0, 0), FRTGridCoord(4, 0), {}));
	TestTrue(TEXT("ostacolo su From/To non conta"),
		URTGridLibrary::HasLineOfSight(FRTGridCoord(0, 0), FRTGridCoord(0, 4), { FRTGridCoord(0, 0), FRTGridCoord(0, 4) }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridCellsInRadiusTest,
	"RefactorTactics.Grid.CellsInRadiusFormsDiamond",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridCellsInRadiusTest::RunTest(const FString&)
{
	const FRTGridCoord C(5, 5);

	const TArray<FRTGridCoord> R0 = URTGridLibrary::CellsInRadius(C, 0);
	TestEqual(TEXT("radius 0 = 1 cella"), R0.Num(), 1);
	TestTrue(TEXT("radius 0 = centro"), R0.Contains(C));

	const TArray<FRTGridCoord> R1 = URTGridLibrary::CellsInRadius(C, 1);
	TestEqual(TEXT("radius 1 = 5 celle"), R1.Num(), 5);
	TestTrue(TEXT("include centro"), R1.Contains(C));
	TestTrue(TEXT("include (4,5)"), R1.Contains(FRTGridCoord(4, 5)));
	TestTrue(TEXT("include (6,5)"), R1.Contains(FRTGridCoord(6, 5)));
	TestTrue(TEXT("include (5,4)"), R1.Contains(FRTGridCoord(5, 4)));
	TestTrue(TEXT("include (5,6)"), R1.Contains(FRTGridCoord(5, 6)));
	TestFalse(TEXT("esclude la diagonale (4,4)"), R1.Contains(FRTGridCoord(4, 4)));

	// Radius 2: diamante di 13 celle.
	TestEqual(TEXT("radius 2 = 13 celle"), URTGridLibrary::CellsInRadius(C, 2).Num(), 13);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridCellsInLineTest,
	"RefactorTactics.Grid.CellsInLineCoversTrajectory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridCellsInLineTest::RunTest(const FString&)
{
	// Verticale (0,0)->(0,3): {(0,1),(0,2),(0,3)}, From escluso.
	const TArray<FRTGridCoord> V = URTGridLibrary::CellsInLine(FRTGridCoord(0, 0), FRTGridCoord(0, 3));
	TestEqual(TEXT("verticale: 3 celle"), V.Num(), 3);
	TestFalse(TEXT("From escluso"), V.Contains(FRTGridCoord(0, 0)));
	TestTrue(TEXT("include (0,1)"), V.Contains(FRTGridCoord(0, 1)));
	TestTrue(TEXT("include (0,2)"), V.Contains(FRTGridCoord(0, 2)));
	TestTrue(TEXT("include To (0,3)"), V.Contains(FRTGridCoord(0, 3)));

	// Orizzontale (0,0)->(3,0).
	const TArray<FRTGridCoord> H = URTGridLibrary::CellsInLine(FRTGridCoord(0, 0), FRTGridCoord(3, 0));
	TestEqual(TEXT("orizzontale: 3 celle"), H.Num(), 3);
	TestTrue(TEXT("include (2,0)"), H.Contains(FRTGridCoord(2, 0)));

	// Adiacente: solo il bersaglio.
	const TArray<FRTGridCoord> A = URTGridLibrary::CellsInLine(FRTGridCoord(0, 0), FRTGridCoord(0, 1));
	TestEqual(TEXT("adiacente: 1 cella"), A.Num(), 1);
	TestTrue(TEXT("è il bersaglio"), A.Contains(FRTGridCoord(0, 1)));

	// Diagonale: include il bersaglio.
	TestTrue(TEXT("diagonale include To"),
		URTGridLibrary::CellsInLine(FRTGridCoord(0, 0), FRTGridCoord(3, 3)).Contains(FRTGridCoord(3, 3)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridCellsInConeTest,
	"RefactorTactics.Grid.CellsInConeExpandsTowardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridCellsInConeTest::RunTest(const FString&)
{
	// Cono verso +X, range 3: si allarga d celle per lato -> 3 + 5 + 7 = 15 celle.
	const FRTGridCoord O(0, 0);
	const TArray<FRTGridCoord> East = URTGridLibrary::CellsInCone(O, FRTGridCoord(5, 0), 3);
	TestEqual(TEXT("cono est: 15 celle"), East.Num(), 15);
	TestFalse(TEXT("origine esclusa"), East.Contains(O));
	TestTrue(TEXT("punta (1,0)"), East.Contains(FRTGridCoord(1, 0)));
	TestTrue(TEXT("bordo (3,3)"), East.Contains(FRTGridCoord(3, 3)));
	TestTrue(TEXT("bordo (3,-3)"), East.Contains(FRTGridCoord(3, -3)));
	TestFalse(TEXT("fuori apertura (1,2)"), East.Contains(FRTGridCoord(1, 2)));
	TestFalse(TEXT("oltre il range (4,0)"), East.Contains(FRTGridCoord(4, 0)));

	// Cono verso +Y (asse dominante Y), range 2: 3 + 5 = 8 celle.
	const TArray<FRTGridCoord> North = URTGridLibrary::CellsInCone(O, FRTGridCoord(0, 3), 2);
	TestEqual(TEXT("cono nord: 8 celle"), North.Num(), 8);
	TestTrue(TEXT("punta (0,1)"), North.Contains(FRTGridCoord(0, 1)));
	TestTrue(TEXT("bordo (2,2)"), North.Contains(FRTGridCoord(2, 2)));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
