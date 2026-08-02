#include "Misc/AutomationTest.h"
#include "Core/RTTypes.h"
#include "Grid/RTGridLibrary.h"
#include "Terrain/RTTerrainTypes.h" // RT_BLOCKED_COST

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridReachableCellsTest,
	"RefactorTactics.Grid.ReachableCellsRespectsObstacles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridReachableCellsTest::RunTest(const FString&)
{
	const TArray<FRTGridCoord> NoBlockers;
	// (a) Percorso libero al centro: coincide con il rombo di Manhattan (raggio 2 -> 13 celle).
	{
		const TArray<FRTGridCoord> R = URTGridLibrary::ReachableCells(FRTGridCoord(5, 5), 2, NoBlockers, 10, 10);
		TestEqual(TEXT("rombo raggio 2 = 13 celle"), R.Num(), 13);
		TestTrue(TEXT("include se stessa"), R.Contains(FRTGridCoord(5, 5)));
		TestTrue(TEXT("(7,5) raggiungibile"), R.Contains(FRTGridCoord(7, 5)));
		TestFalse(TEXT("(5,8) oltre il raggio"), R.Contains(FRTGridCoord(5, 8)));
	}
	// (b) Ostacolo 2x2: una cella "vicina" (Manhattan 3) ma dietro l'ostacolo NON e' raggiungibile.
	{
		const TArray<FRTGridCoord> L = { FRTGridCoord(4,4), FRTGridCoord(5,4), FRTGridCoord(4,5), FRTGridCoord(5,5) };
		const TArray<FRTGridCoord> R = URTGridLibrary::ReachableCells(FRTGridCoord(3, 4), 3, L, 10, 10);
		TestFalse(TEXT("(6,4) dietro l'ostacolo: path reale > 3"), R.Contains(FRTGridCoord(6, 4)));
		TestTrue(TEXT("(3,7) libero: raggiungibile"), R.Contains(FRTGridCoord(3, 7)));
		TestFalse(TEXT("mai su una cella bloccante"), R.Contains(FRTGridCoord(4, 4)));
	}
	// (c) Muro che sigilla: colonna x=2 su tutta l'altezza -> x>=3 irraggiungibile a qualunque range.
	{
		TArray<FRTGridCoord> Wall;
		for (int32 Y = 0; Y < 10; ++Y) { Wall.Add(FRTGridCoord(2, Y)); }
		const TArray<FRTGridCoord> R = URTGridLibrary::ReachableCells(FRTGridCoord(0, 0), 20, Wall, 10, 10);
		TestTrue(TEXT("(1,9) di qua dal muro"), R.Contains(FRTGridCoord(1, 9)));
		TestFalse(TEXT("(5,5) oltre il muro sigillato"), R.Contains(FRTGridCoord(5, 5)));
	}
	// (d) Determinismo: due chiamate producono lo stesso insieme.
	{
		const TArray<FRTGridCoord> L = { FRTGridCoord(4,4), FRTGridCoord(5,4), FRTGridCoord(4,5), FRTGridCoord(5,5) };
		const TArray<FRTGridCoord> R1 = URTGridLibrary::ReachableCells(FRTGridCoord(3, 4), 3, L, 10, 10);
		const TArray<FRTGridCoord> R2 = URTGridLibrary::ReachableCells(FRTGridCoord(3, 4), 3, L, 10, 10);
		TestEqual(TEXT("stessa cardinalita'"), R1.Num(), R2.Num());
		bool bSame = true;
		for (const FRTGridCoord& C : R1) { if (!R2.Contains(C)) { bSame = false; break; } }
		TestTrue(TEXT("stesso insieme"), bSame);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridFindPathTest,
	"RefactorTactics.Grid.FindPathMinimalAroundObstacles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridFindPathTest::RunTest(const FString&)
{
	// (f) Percorso minimo che aggira l'ostacolo 2x2: 5 passi (6 celle) verso nord.
	{
		const TArray<FRTGridCoord> L = { FRTGridCoord(4,4), FRTGridCoord(5,4), FRTGridCoord(4,5), FRTGridCoord(5,5) };
		const TArray<FRTGridCoord> P = URTGridLibrary::FindPath(FRTGridCoord(3, 4), FRTGridCoord(6, 4), L, 10, 10);
		TestEqual(TEXT("6 celle (5 passi minimi)"), P.Num(), 6);
		if (P.Num() > 0)
		{
			TestTrue(TEXT("parte da From"), P[0] == FRTGridCoord(3, 4));
			TestTrue(TEXT("arriva a To"), P.Last() == FRTGridCoord(6, 4));
		}
		for (const FRTGridCoord& C : P)
		{
			TestFalse(TEXT("nessuna cella bloccante nel path"), L.Contains(C));
		}
		// Celle consecutive adiacenti (un solo passo ortogonale).
		bool bContiguous = true;
		for (int32 i = 1; i < P.Num(); ++i)
		{
			if (URTGridLibrary::ManhattanDistance(P[i - 1], P[i]) != 1) { bContiguous = false; break; }
		}
		TestTrue(TEXT("percorso contiguo"), bContiguous);
	}
	// (g) Irraggiungibile: muro sigillante -> percorso vuoto.
	{
		TArray<FRTGridCoord> Wall;
		for (int32 Y = 0; Y < 10; ++Y) { Wall.Add(FRTGridCoord(2, Y)); }
		const TArray<FRTGridCoord> P = URTGridLibrary::FindPath(FRTGridCoord(0, 0), FRTGridCoord(5, 5), Wall, 10, 10);
		TestEqual(TEXT("nessun percorso -> vuoto"), P.Num(), 0);
	}
	// (trivial) From == To -> solo la cella di partenza.
	{
		const TArray<FRTGridCoord> None;
		const TArray<FRTGridCoord> P = URTGridLibrary::FindPath(FRTGridCoord(5, 5), FRTGridCoord(5, 5), None, 10, 10);
		TestEqual(TEXT("From==To -> 1 cella"), P.Num(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridWeightedPathTest,
	"RefactorTactics.Grid.FindPathByCostPrefersCheaperRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridWeightedPathTest::RunTest(const FString&)
{
	// Colonna "fango" costo 3 su (5,4)(5,5)(5,6); tutto il resto costo 1 (assente dalla mappa).
	TMap<FRTGridCoord, int32> Cost;
	Cost.Add(FRTGridCoord(5, 4), 3);
	Cost.Add(FRTGridCoord(5, 5), 3);
	Cost.Add(FRTGridCoord(5, 6), 3);

	// (f) Percorso a costo minimo (5,3)->(5,7): la retta attraverso il fango costa 3+3+3+1=10;
	//     il giro laterale costa 6 -> deve preferire il giro (evita il fango).
	{
		const TArray<FRTGridCoord> P = URTGridLibrary::FindPathByCost(FRTGridCoord(5, 3), FRTGridCoord(5, 7), Cost, 10, 10);
		TestTrue(TEXT("percorso non vuoto"), P.Num() > 0);
		if (P.Num() > 0)
		{
			TestTrue(TEXT("parte da From"), P[0] == FRTGridCoord(5, 3));
			TestTrue(TEXT("arriva a To"), P.Last() == FRTGridCoord(5, 7));
		}
		TestFalse(TEXT("evita il fango centrale (5,5)"), P.Contains(FRTGridCoord(5, 5)));
		// Costo totale del percorso scelto = 6 (sei celle da 1, oltre a From).
		int32 Total = 0;
		for (int32 i = 1; i < P.Num(); ++i) { Total += (Cost.Contains(P[i]) ? Cost[P[i]] : 1); }
		TestEqual(TEXT("costo totale minimo 6"), Total, 6);
	}
	// (g) Reachability per budget di costo: da (5,3) con budget 4.
	{
		const TArray<FRTGridCoord> R = URTGridLibrary::ReachableCellsByCost(FRTGridCoord(5, 3), 4, Cost, 10, 10);
		TestTrue(TEXT("include se stessa"), R.Contains(FRTGridCoord(5, 3)));
		TestTrue(TEXT("(2,3) a costo 3 raggiungibile"), R.Contains(FRTGridCoord(2, 3)));
		TestTrue(TEXT("(5,4) fango costo 3 raggiungibile"), R.Contains(FRTGridCoord(5, 4)));
		TestFalse(TEXT("(5,5) costo 6 oltre budget"), R.Contains(FRTGridCoord(5, 5)));
		TestFalse(TEXT("(5,7) troppo caro/lontano"), R.Contains(FRTGridCoord(5, 7)));
	}
	// (h) Celle impassabili (costo negativo): muro che sigilla la colonna x=2.
	{
		TMap<FRTGridCoord, int32> Wall;
		for (int32 Y = 0; Y < 10; ++Y) { Wall.Add(FRTGridCoord(2, Y), RT_BLOCKED_COST); }
		const TArray<FRTGridCoord> R = URTGridLibrary::ReachableCellsByCost(FRTGridCoord(0, 0), 20, Wall, 10, 10);
		TestTrue(TEXT("(1,9) di qua dal muro"), R.Contains(FRTGridCoord(1, 9)));
		TestFalse(TEXT("(5,5) oltre il muro"), R.Contains(FRTGridCoord(5, 5)));
		const TArray<FRTGridCoord> P = URTGridLibrary::FindPathByCost(FRTGridCoord(0, 0), FRTGridCoord(5, 5), Wall, 10, 10);
		TestEqual(TEXT("nessun percorso oltre il muro"), P.Num(), 0);
	}
	// (i) From == To -> solo la cella di partenza.
	{
		const TMap<FRTGridCoord, int32> None;
		const TArray<FRTGridCoord> P = URTGridLibrary::FindPathByCost(FRTGridCoord(5, 5), FRTGridCoord(5, 5), None, 10, 10);
		TestEqual(TEXT("From==To -> 1 cella"), P.Num(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridPathCostTest,
	"RefactorTactics.Grid.PathCostValidatesAndSums",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridPathCostTest::RunTest(const FString&)
{
	TMap<FRTGridCoord, int32> Cost;
	Cost.Add(FRTGridCoord(1, 0), 3); // fango
	Cost.Add(FRTGridCoord(2, 0), RT_BLOCKED_COST); // muro

	// Fermo (0/1 cella) -> costo 0.
	{
		TestEqual(TEXT("vuoto -> 0"), URTGridLibrary::PathCost({}, Cost), 0);
		TestEqual(TEXT("una cella -> 0"), URTGridLibrary::PathCost({ FRTGridCoord(0,0) }, Cost), 0);
	}
	// Percorso libero: 3 celle da 1 -> costo 3 (From escluso: 3 entrate).
	{
		const TArray<FRTGridCoord> P = { FRTGridCoord(0,1), FRTGridCoord(0,2), FRTGridCoord(0,3), FRTGridCoord(0,4) };
		TestEqual(TEXT("libero: costo 3"), URTGridLibrary::PathCost(P, Cost), 3);
	}
	// Attraverso il fango: (0,0)->(1,0) entra nel fango (costo 3).
	{
		const TArray<FRTGridCoord> P = { FRTGridCoord(0,0), FRTGridCoord(1,0) };
		TestEqual(TEXT("fango: costo 3"), URTGridLibrary::PathCost(P, Cost), 3);
	}
	// Non contiguo (salto) -> -1.
	{
		const TArray<FRTGridCoord> P = { FRTGridCoord(0,0), FRTGridCoord(0,2) };
		TestEqual(TEXT("salto -> invalido"), URTGridLibrary::PathCost(P, Cost), -1);
	}
	// Attraversa un muro -> -1.
	{
		const TArray<FRTGridCoord> P = { FRTGridCoord(1,0), FRTGridCoord(2,0) };
		TestEqual(TEXT("muro -> invalido"), URTGridLibrary::PathCost(P, Cost), -1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridCompositePathTest,
	"RefactorTactics.Grid.BuildCompositePathThroughWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridCompositePathTest::RunTest(const FString&)
{
	const TMap<FRTGridCoord, int32> NoCost;
	// Nessun waypoint -> solo Start.
	{
		const TArray<FRTGridCoord> P = URTGridLibrary::BuildCompositePath(FRTGridCoord(0, 0), {}, NoCost, 10, 10);
		TestEqual(TEXT("nessun waypoint -> [Start]"), P.Num(), 1);
	}
	// Un waypoint -> auto-route a quello.
	{
		const TArray<FRTGridCoord> P = URTGridLibrary::BuildCompositePath(FRTGridCoord(0, 0), { FRTGridCoord(0, 3) }, NoCost, 10, 10);
		TestEqual(TEXT("un waypoint: 4 celle"), P.Num(), 4);
		TestTrue(TEXT("parte da Start"), P[0] == FRTGridCoord(0, 0));
		TestTrue(TEXT("arriva al waypoint"), P.Last() == FRTGridCoord(0, 3));
	}
	// Due waypoint: forza il passaggio per (3,0) prima di (3,3) -> contiene (3,0), contiguo, no duplicati alla giunzione.
	{
		const TArray<FRTGridCoord> P = URTGridLibrary::BuildCompositePath(FRTGridCoord(0, 0),
			{ FRTGridCoord(3, 0), FRTGridCoord(3, 3) }, NoCost, 10, 10);
		TestTrue(TEXT("parte da Start"), P[0] == FRTGridCoord(0, 0));
		TestTrue(TEXT("passa per il waypoint (3,0)"), P.Contains(FRTGridCoord(3, 0)));
		TestTrue(TEXT("arriva a (3,3)"), P.Last() == FRTGridCoord(3, 3));
		bool bContig = true;
		for (int32 i = 1; i < P.Num(); ++i) { if (URTGridLibrary::ManhattanDistance(P[i - 1], P[i]) != 1) { bContig = false; break; } }
		TestTrue(TEXT("contiguo (no duplicati alla giunzione)"), bContig);
		TestEqual(TEXT("lunghezza 7 celle (3+3 passi)"), P.Num(), 7);
	}
	// Waypoint irraggiungibile (muro sigillante) -> vuoto.
	{
		TMap<FRTGridCoord, int32> Wall;
		for (int32 Y = 0; Y < 10; ++Y) { Wall.Add(FRTGridCoord(2, Y), RT_BLOCKED_COST); }
		const TArray<FRTGridCoord> P = URTGridLibrary::BuildCompositePath(FRTGridCoord(0, 0), { FRTGridCoord(5, 5) }, Wall, 10, 10);
		TestEqual(TEXT("irraggiungibile -> vuoto"), P.Num(), 0);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
