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

#endif // WITH_DEV_AUTOMATION_TESTS
