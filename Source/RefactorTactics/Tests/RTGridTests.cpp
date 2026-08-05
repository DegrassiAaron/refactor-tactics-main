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
	const FVector World = URTGridLibrary::CellToWorld(FRTCellId(2, 3), RTTestOrigin, RTTestCellSize);
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
	const FRTCellId Cell = URTGridLibrary::WorldToCell(FVector(510.0, 690.0, 0.0), RTTestOrigin, RTTestCellSize);
	TestTrue(TEXT("(510,690) -> cella (2,3)"), Cell == FRTCellId(2, 3));
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
			const FRTCellId Cell(X, Y);
			const FVector World = URTGridLibrary::CellToWorld(Cell, RTTestOrigin, RTTestCellSize);
			const FRTCellId Back = URTGridLibrary::WorldToCell(World, RTTestOrigin, RTTestCellSize);
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
	TestTrue(TEXT("(0,0) dentro 10x10"), URTGridLibrary::IsInsideGrid(FRTCellId(0, 0), 10, 10));
	TestTrue(TEXT("(9,9) dentro 10x10"), URTGridLibrary::IsInsideGrid(FRTCellId(9, 9), 10, 10));
	TestFalse(TEXT("(-1,0) fuori"), URTGridLibrary::IsInsideGrid(FRTCellId(-1, 0), 10, 10));
	TestFalse(TEXT("(10,0) fuori"), URTGridLibrary::IsInsideGrid(FRTCellId(10, 0), 10, 10));
	TestFalse(TEXT("(0,10) fuori"), URTGridLibrary::IsInsideGrid(FRTCellId(0, 10), 10, 10));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridManhattanTest,
	"RefactorTactics.Grid.ManhattanDistanceSumsAbsoluteDeltas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridManhattanTest::RunTest(const FString&)
{
	TestEqual(TEXT("(1,1)->(4,5) = 7"), URTGridLibrary::ManhattanDistance(FRTCellId(1, 1), FRTCellId(4, 5)), 7);
	TestEqual(TEXT("stessa cella = 0"), URTGridLibrary::ManhattanDistance(FRTCellId(3, 3), FRTCellId(3, 3)), 0);
	TestEqual(TEXT("delta negativi in valore assoluto"), URTGridLibrary::ManhattanDistance(FRTCellId(5, 5), FRTCellId(1, 2)), 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridWithinRangeTest,
	"RefactorTactics.Grid.IsWithinRangeChecksManhattan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridWithinRangeTest::RunTest(const FString&)
{
	const FRTCellId From(2, 2);
	TestTrue(TEXT("stessa cella entro range 4"), URTGridLibrary::IsWithinRange(From, FRTCellId(2, 2), 4));
	TestTrue(TEXT("distanza 4 entro range 4"), URTGridLibrary::IsWithinRange(From, FRTCellId(2, 6), 4));
	TestTrue(TEXT("distanza 3 (diagonale) entro range 4"), URTGridLibrary::IsWithinRange(From, FRTCellId(4, 3), 4));
	TestFalse(TEXT("distanza 5 fuori range 4"), URTGridLibrary::IsWithinRange(From, FRTCellId(2, 7), 4));
	TestFalse(TEXT("distanza 8 fuori range 4"), URTGridLibrary::IsWithinRange(From, FRTCellId(6, 6), 4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridLineOfSightTest,
	"RefactorTactics.Grid.LineOfSightBlockedByObstacles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridLineOfSightTest::RunTest(const FString&)
{
	// Linea verticale (0,0)->(0,4): un ostacolo a (0,2) blocca; uno a (1,2) no.
	TestFalse(TEXT("ostacolo sulla linea verticale blocca"),
		URTGridLibrary::HasLineOfSight(FRTCellId(0, 0), FRTCellId(0, 4), { FRTCellId(0, 2) }));
	TestTrue(TEXT("ostacolo fuori linea non blocca"),
		URTGridLibrary::HasLineOfSight(FRTCellId(0, 0), FRTCellId(0, 4), { FRTCellId(1, 2) }));

	// Linea orizzontale (0,0)->(4,0): ostacolo a (2,0) blocca.
	TestFalse(TEXT("ostacolo sulla linea orizzontale blocca"),
		URTGridLibrary::HasLineOfSight(FRTCellId(0, 0), FRTCellId(4, 0), { FRTCellId(2, 0) }));

	// Diagonale (0,0)->(4,4): ostacolo a (2,2) blocca.
	TestFalse(TEXT("ostacolo sulla diagonale blocca"),
		URTGridLibrary::HasLineOfSight(FRTCellId(0, 0), FRTCellId(4, 4), { FRTCellId(2, 2) }));

	// Nessun ostacolo -> libera; From/To non bloccano.
	TestTrue(TEXT("nessun ostacolo -> libera"),
		URTGridLibrary::HasLineOfSight(FRTCellId(0, 0), FRTCellId(4, 0), {}));
	TestTrue(TEXT("ostacolo su From/To non conta"),
		URTGridLibrary::HasLineOfSight(FRTCellId(0, 0), FRTCellId(0, 4), { FRTCellId(0, 0), FRTCellId(0, 4) }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridCellsInRadiusTest,
	"RefactorTactics.Grid.CellsInRadiusFormsDiamond",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridCellsInRadiusTest::RunTest(const FString&)
{
	const FRTCellId C(5, 5);

	const TArray<FRTCellId> R0 = URTGridLibrary::CellsInRadius(C, 0);
	TestEqual(TEXT("radius 0 = 1 cella"), R0.Num(), 1);
	TestTrue(TEXT("radius 0 = centro"), R0.Contains(C));

	const TArray<FRTCellId> R1 = URTGridLibrary::CellsInRadius(C, 1);
	TestEqual(TEXT("radius 1 = 5 celle"), R1.Num(), 5);
	TestTrue(TEXT("include centro"), R1.Contains(C));
	TestTrue(TEXT("include (4,5)"), R1.Contains(FRTCellId(4, 5)));
	TestTrue(TEXT("include (6,5)"), R1.Contains(FRTCellId(6, 5)));
	TestTrue(TEXT("include (5,4)"), R1.Contains(FRTCellId(5, 4)));
	TestTrue(TEXT("include (5,6)"), R1.Contains(FRTCellId(5, 6)));
	TestFalse(TEXT("esclude la diagonale (4,4)"), R1.Contains(FRTCellId(4, 4)));

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
	const TArray<FRTCellId> V = URTGridLibrary::CellsInLine(FRTCellId(0, 0), FRTCellId(0, 3));
	TestEqual(TEXT("verticale: 3 celle"), V.Num(), 3);
	TestFalse(TEXT("From escluso"), V.Contains(FRTCellId(0, 0)));
	TestTrue(TEXT("include (0,1)"), V.Contains(FRTCellId(0, 1)));
	TestTrue(TEXT("include (0,2)"), V.Contains(FRTCellId(0, 2)));
	TestTrue(TEXT("include To (0,3)"), V.Contains(FRTCellId(0, 3)));

	// Orizzontale (0,0)->(3,0).
	const TArray<FRTCellId> H = URTGridLibrary::CellsInLine(FRTCellId(0, 0), FRTCellId(3, 0));
	TestEqual(TEXT("orizzontale: 3 celle"), H.Num(), 3);
	TestTrue(TEXT("include (2,0)"), H.Contains(FRTCellId(2, 0)));

	// Adiacente: solo il bersaglio.
	const TArray<FRTCellId> A = URTGridLibrary::CellsInLine(FRTCellId(0, 0), FRTCellId(0, 1));
	TestEqual(TEXT("adiacente: 1 cella"), A.Num(), 1);
	TestTrue(TEXT("è il bersaglio"), A.Contains(FRTCellId(0, 1)));

	// Diagonale: include il bersaglio.
	TestTrue(TEXT("diagonale include To"),
		URTGridLibrary::CellsInLine(FRTCellId(0, 0), FRTCellId(3, 3)).Contains(FRTCellId(3, 3)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridCellsInConeTest,
	"RefactorTactics.Grid.CellsInConeExpandsTowardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridCellsInConeTest::RunTest(const FString&)
{
	// Cono verso +X, range 3: si allarga d celle per lato -> 3 + 5 + 7 = 15 celle.
	const FRTCellId O(0, 0);
	const TArray<FRTCellId> East = URTGridLibrary::CellsInCone(O, FRTCellId(5, 0), 3);
	TestEqual(TEXT("cono est: 15 celle"), East.Num(), 15);
	TestFalse(TEXT("origine esclusa"), East.Contains(O));
	TestTrue(TEXT("punta (1,0)"), East.Contains(FRTCellId(1, 0)));
	TestTrue(TEXT("bordo (3,3)"), East.Contains(FRTCellId(3, 3)));
	TestTrue(TEXT("bordo (3,-3)"), East.Contains(FRTCellId(3, -3)));
	TestFalse(TEXT("fuori apertura (1,2)"), East.Contains(FRTCellId(1, 2)));
	TestFalse(TEXT("oltre il range (4,0)"), East.Contains(FRTCellId(4, 0)));

	// Cono verso +Y (asse dominante Y), range 2: 3 + 5 = 8 celle.
	const TArray<FRTCellId> North = URTGridLibrary::CellsInCone(O, FRTCellId(0, 3), 2);
	TestEqual(TEXT("cono nord: 8 celle"), North.Num(), 8);
	TestTrue(TEXT("punta (0,1)"), North.Contains(FRTCellId(0, 1)));
	TestTrue(TEXT("bordo (2,2)"), North.Contains(FRTCellId(2, 2)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridReachableCellsTest,
	"RefactorTactics.Grid.ReachableCellsRespectsObstacles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridReachableCellsTest::RunTest(const FString&)
{
	const TArray<FRTCellId> NoBlockers;
	// (a) Percorso libero al centro: coincide con il rombo di Manhattan (raggio 2 -> 13 celle).
	{
		const TArray<FRTCellId> R = URTGridLibrary::ReachableCells(FRTCellId(5, 5), 2, NoBlockers, 10, 10);
		TestEqual(TEXT("rombo raggio 2 = 13 celle"), R.Num(), 13);
		TestTrue(TEXT("include se stessa"), R.Contains(FRTCellId(5, 5)));
		TestTrue(TEXT("(7,5) raggiungibile"), R.Contains(FRTCellId(7, 5)));
		TestFalse(TEXT("(5,8) oltre il raggio"), R.Contains(FRTCellId(5, 8)));
	}
	// (b) Ostacolo 2x2: una cella "vicina" (Manhattan 3) ma dietro l'ostacolo NON e' raggiungibile.
	{
		const TArray<FRTCellId> L = { FRTCellId(4,4), FRTCellId(5,4), FRTCellId(4,5), FRTCellId(5,5) };
		const TArray<FRTCellId> R = URTGridLibrary::ReachableCells(FRTCellId(3, 4), 3, L, 10, 10);
		TestFalse(TEXT("(6,4) dietro l'ostacolo: path reale > 3"), R.Contains(FRTCellId(6, 4)));
		TestTrue(TEXT("(3,7) libero: raggiungibile"), R.Contains(FRTCellId(3, 7)));
		TestFalse(TEXT("mai su una cella bloccante"), R.Contains(FRTCellId(4, 4)));
	}
	// (c) Muro che sigilla: colonna x=2 su tutta l'altezza -> x>=3 irraggiungibile a qualunque range.
	{
		TArray<FRTCellId> Wall;
		for (int32 Y = 0; Y < 10; ++Y) { Wall.Add(FRTCellId(2, Y)); }
		const TArray<FRTCellId> R = URTGridLibrary::ReachableCells(FRTCellId(0, 0), 20, Wall, 10, 10);
		TestTrue(TEXT("(1,9) di qua dal muro"), R.Contains(FRTCellId(1, 9)));
		TestFalse(TEXT("(5,5) oltre il muro sigillato"), R.Contains(FRTCellId(5, 5)));
	}
	// (d) Determinismo: due chiamate producono lo stesso insieme.
	{
		const TArray<FRTCellId> L = { FRTCellId(4,4), FRTCellId(5,4), FRTCellId(4,5), FRTCellId(5,5) };
		const TArray<FRTCellId> R1 = URTGridLibrary::ReachableCells(FRTCellId(3, 4), 3, L, 10, 10);
		const TArray<FRTCellId> R2 = URTGridLibrary::ReachableCells(FRTCellId(3, 4), 3, L, 10, 10);
		TestEqual(TEXT("stessa cardinalita'"), R1.Num(), R2.Num());
		bool bSame = true;
		for (const FRTCellId& C : R1) { if (!R2.Contains(C)) { bSame = false; break; } }
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
		const TArray<FRTCellId> L = { FRTCellId(4,4), FRTCellId(5,4), FRTCellId(4,5), FRTCellId(5,5) };
		const TArray<FRTCellId> P = URTGridLibrary::FindPath(FRTCellId(3, 4), FRTCellId(6, 4), L, 10, 10);
		TestEqual(TEXT("6 celle (5 passi minimi)"), P.Num(), 6);
		if (P.Num() > 0)
		{
			TestTrue(TEXT("parte da From"), P[0] == FRTCellId(3, 4));
			TestTrue(TEXT("arriva a To"), P.Last() == FRTCellId(6, 4));
		}
		for (const FRTCellId& C : P)
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
		TArray<FRTCellId> Wall;
		for (int32 Y = 0; Y < 10; ++Y) { Wall.Add(FRTCellId(2, Y)); }
		const TArray<FRTCellId> P = URTGridLibrary::FindPath(FRTCellId(0, 0), FRTCellId(5, 5), Wall, 10, 10);
		TestEqual(TEXT("nessun percorso -> vuoto"), P.Num(), 0);
	}
	// (trivial) From == To -> solo la cella di partenza.
	{
		const TArray<FRTCellId> None;
		const TArray<FRTCellId> P = URTGridLibrary::FindPath(FRTCellId(5, 5), FRTCellId(5, 5), None, 10, 10);
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
	TMap<FRTCellId, int32> Cost;
	Cost.Add(FRTCellId(5, 4), 3);
	Cost.Add(FRTCellId(5, 5), 3);
	Cost.Add(FRTCellId(5, 6), 3);

	// (f) Percorso a costo minimo (5,3)->(5,7): la retta attraverso il fango costa 3+3+3+1=10;
	//     il giro laterale costa 6 -> deve preferire il giro (evita il fango).
	{
		const TArray<FRTCellId> P = URTGridLibrary::FindPathByCost(FRTCellId(5, 3), FRTCellId(5, 7), Cost, 10, 10);
		TestTrue(TEXT("percorso non vuoto"), P.Num() > 0);
		if (P.Num() > 0)
		{
			TestTrue(TEXT("parte da From"), P[0] == FRTCellId(5, 3));
			TestTrue(TEXT("arriva a To"), P.Last() == FRTCellId(5, 7));
		}
		TestFalse(TEXT("evita il fango centrale (5,5)"), P.Contains(FRTCellId(5, 5)));
		// Costo totale del percorso scelto = 6 (sei celle da 1, oltre a From).
		int32 Total = 0;
		for (int32 i = 1; i < P.Num(); ++i) { Total += (Cost.Contains(P[i]) ? Cost[P[i]] : 1); }
		TestEqual(TEXT("costo totale minimo 6"), Total, 6);
	}
	// (g) Reachability per budget di costo: da (5,3) con budget 4.
	{
		const TArray<FRTCellId> R = URTGridLibrary::ReachableCellsByCost(FRTCellId(5, 3), 4, Cost, 10, 10);
		TestTrue(TEXT("include se stessa"), R.Contains(FRTCellId(5, 3)));
		TestTrue(TEXT("(2,3) a costo 3 raggiungibile"), R.Contains(FRTCellId(2, 3)));
		TestTrue(TEXT("(5,4) fango costo 3 raggiungibile"), R.Contains(FRTCellId(5, 4)));
		TestFalse(TEXT("(5,5) costo 6 oltre budget"), R.Contains(FRTCellId(5, 5)));
		TestFalse(TEXT("(5,7) troppo caro/lontano"), R.Contains(FRTCellId(5, 7)));
	}
	// (h) Celle impassabili (costo negativo): muro che sigilla la colonna x=2.
	{
		TMap<FRTCellId, int32> Wall;
		for (int32 Y = 0; Y < 10; ++Y) { Wall.Add(FRTCellId(2, Y), RT_BLOCKED_COST); }
		const TArray<FRTCellId> R = URTGridLibrary::ReachableCellsByCost(FRTCellId(0, 0), 20, Wall, 10, 10);
		TestTrue(TEXT("(1,9) di qua dal muro"), R.Contains(FRTCellId(1, 9)));
		TestFalse(TEXT("(5,5) oltre il muro"), R.Contains(FRTCellId(5, 5)));
		const TArray<FRTCellId> P = URTGridLibrary::FindPathByCost(FRTCellId(0, 0), FRTCellId(5, 5), Wall, 10, 10);
		TestEqual(TEXT("nessun percorso oltre il muro"), P.Num(), 0);
	}
	// (i) From == To -> solo la cella di partenza.
	{
		const TMap<FRTCellId, int32> None;
		const TArray<FRTCellId> P = URTGridLibrary::FindPathByCost(FRTCellId(5, 5), FRTCellId(5, 5), None, 10, 10);
		TestEqual(TEXT("From==To -> 1 cella"), P.Num(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridPathCostTest,
	"RefactorTactics.Grid.PathCostValidatesAndSums",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridPathCostTest::RunTest(const FString&)
{
	TMap<FRTCellId, int32> Cost;
	Cost.Add(FRTCellId(1, 0), 3); // fango
	Cost.Add(FRTCellId(2, 0), RT_BLOCKED_COST); // muro

	// Fermo (0/1 cella) -> costo 0.
	{
		TestEqual(TEXT("vuoto -> 0"), URTGridLibrary::PathCost({}, Cost), 0);
		TestEqual(TEXT("una cella -> 0"), URTGridLibrary::PathCost({ FRTCellId(0,0) }, Cost), 0);
	}
	// Percorso libero: 3 celle da 1 -> costo 3 (From escluso: 3 entrate).
	{
		const TArray<FRTCellId> P = { FRTCellId(0,1), FRTCellId(0,2), FRTCellId(0,3), FRTCellId(0,4) };
		TestEqual(TEXT("libero: costo 3"), URTGridLibrary::PathCost(P, Cost), 3);
	}
	// Attraverso il fango: (0,0)->(1,0) entra nel fango (costo 3).
	{
		const TArray<FRTCellId> P = { FRTCellId(0,0), FRTCellId(1,0) };
		TestEqual(TEXT("fango: costo 3"), URTGridLibrary::PathCost(P, Cost), 3);
	}
	// Non contiguo (salto) -> -1.
	{
		const TArray<FRTCellId> P = { FRTCellId(0,0), FRTCellId(0,2) };
		TestEqual(TEXT("salto -> invalido"), URTGridLibrary::PathCost(P, Cost), -1);
	}
	// Attraversa un muro -> -1.
	{
		const TArray<FRTCellId> P = { FRTCellId(1,0), FRTCellId(2,0) };
		TestEqual(TEXT("muro -> invalido"), URTGridLibrary::PathCost(P, Cost), -1);
	}
	// Passo via ARCO (rampa cross-layer): valido col relativo edge (costo dell'arco), invalido senza.
	{
		const TArray<FRTCellId> Ramp = { FRTCellId(2,4,0), FRTCellId(3,4,1) };
		const TArray<FRTTraversalEdge> Edges = { FRTTraversalEdge(FRTCellId(2,4,0), FRTCellId(3,4,1), 2) };
		TestEqual(TEXT("arco: costo 2"), URTGridLibrary::PathCost(Ramp, Cost, Edges), 2);
		TestEqual(TEXT("senza arco: invalido"), URTGridLibrary::PathCost(Ramp, Cost), -1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridCompositePathTest,
	"RefactorTactics.Grid.BuildCompositePathThroughWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridCompositePathTest::RunTest(const FString&)
{
	const TMap<FRTCellId, int32> NoCost;
	// Nessun waypoint -> solo Start.
	{
		const TArray<FRTCellId> P = URTGridLibrary::BuildCompositePath(FRTCellId(0, 0), {}, NoCost, 10, 10);
		TestEqual(TEXT("nessun waypoint -> [Start]"), P.Num(), 1);
	}
	// Un waypoint -> auto-route a quello.
	{
		const TArray<FRTCellId> P = URTGridLibrary::BuildCompositePath(FRTCellId(0, 0), { FRTCellId(0, 3) }, NoCost, 10, 10);
		TestEqual(TEXT("un waypoint: 4 celle"), P.Num(), 4);
		TestTrue(TEXT("parte da Start"), P[0] == FRTCellId(0, 0));
		TestTrue(TEXT("arriva al waypoint"), P.Last() == FRTCellId(0, 3));
	}
	// Due waypoint: forza il passaggio per (3,0) prima di (3,3) -> contiene (3,0), contiguo, no duplicati alla giunzione.
	{
		const TArray<FRTCellId> P = URTGridLibrary::BuildCompositePath(FRTCellId(0, 0),
			{ FRTCellId(3, 0), FRTCellId(3, 3) }, NoCost, 10, 10);
		TestTrue(TEXT("parte da Start"), P[0] == FRTCellId(0, 0));
		TestTrue(TEXT("passa per il waypoint (3,0)"), P.Contains(FRTCellId(3, 0)));
		TestTrue(TEXT("arriva a (3,3)"), P.Last() == FRTCellId(3, 3));
		bool bContig = true;
		for (int32 i = 1; i < P.Num(); ++i) { if (URTGridLibrary::ManhattanDistance(P[i - 1], P[i]) != 1) { bContig = false; break; } }
		TestTrue(TEXT("contiguo (no duplicati alla giunzione)"), bContig);
		TestEqual(TEXT("lunghezza 7 celle (3+3 passi)"), P.Num(), 7);
	}
	// Waypoint irraggiungibile (muro sigillante) -> vuoto.
	{
		TMap<FRTCellId, int32> Wall;
		for (int32 Y = 0; Y < 10; ++Y) { Wall.Add(FRTCellId(2, Y), RT_BLOCKED_COST); }
		const TArray<FRTCellId> P = URTGridLibrary::BuildCompositePath(FRTCellId(0, 0), { FRTCellId(5, 5) }, Wall, 10, 10);
		TestEqual(TEXT("irraggiungibile -> vuoto"), P.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellIdLayerTest,
	"RefactorTactics.Grid.GridCoordDistinguishesLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellIdLayerTest::RunTest(const FString&)
{
	// Retro-compatibilita': il costruttore 2-arg ha Layer 0.
	TestEqual(TEXT("2-arg -> Layer 0"), FRTCellId(3, 4).Layer, 0);
	// Stesse X,Y ma Layer diverso: NON sono la stessa cella.
	TestTrue(TEXT("layer diverso -> diverso"), FRTCellId(3, 4, 0) != FRTCellId(3, 4, 1));
	// Stesse X,Y,Layer: uguali.
	TestTrue(TEXT("stesso layer -> uguale"), FRTCellId(3, 4, 1) == FRTCellId(3, 4, 1));
	// Chiavi distinte in una TMap (hash consapevole del layer).
	{
		TMap<FRTCellId, int32> M;
		M.Add(FRTCellId(3, 4, 0), 10);
		M.Add(FRTCellId(3, 4, 1), 20);
		TestEqual(TEXT("due chiavi distinte"), M.Num(), 2);
		TestEqual(TEXT("layer 1 -> 20"), M.FindRef(FRTCellId(3, 4, 1)), 20);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridGraphPathTest,
	"RefactorTactics.Grid.FindPathByGraphUsesEdgesAndLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridGraphPathTest::RunTest(const FString&)
{
	const TMap<FRTCellId, int32> NoCost;

	// (a) Portale sullo stesso layer: (0,0)->(9,9) a costo 1 -> il percorso lo usa (2 celle, non 19).
	{
		const TArray<FRTTraversalEdge> Edges = { FRTTraversalEdge(FRTCellId(0, 0, 0), FRTCellId(9, 9, 0), 1) };
		const TArray<FRTCellId> P = URTGridLibrary::FindPathByGraph(FRTCellId(0, 0, 0), FRTCellId(9, 9, 0), NoCost, Edges, 10, 10);
		TestEqual(TEXT("portale: 2 celle"), P.Num(), 2);
		if (P.Num() == 2)
		{
			TestTrue(TEXT("arriva via portale"), P.Last() == FRTCellId(9, 9, 0));
		}
	}
	// (b) Scala tra layer 0 e 1: (5,5,0)->(5,5,1) costo 2. Path (5,4,0)->(5,5,0)->[scala]->(5,5,1).
	{
		const TArray<FRTTraversalEdge> Edges = { FRTTraversalEdge(FRTCellId(5, 5, 0), FRTCellId(5, 5, 1), 2) };
		const TArray<FRTCellId> P = URTGridLibrary::FindPathByGraph(FRTCellId(5, 4, 0), FRTCellId(5, 5, 1), NoCost, Edges, 10, 10);
		TestTrue(TEXT("arriva al layer 1"), P.Num() > 0 && P.Last() == FRTCellId(5, 5, 1));
		TestTrue(TEXT("passa dalla scala"), P.Contains(FRTCellId(5, 5, 0)) && P.Contains(FRTCellId(5, 5, 1)));
	}
	// (c) Reachability cross-layer entro budget di costo: 1 passo + 2 scala = 3.
	{
		const TArray<FRTTraversalEdge> Edges = { FRTTraversalEdge(FRTCellId(5, 5, 0), FRTCellId(5, 5, 1), 2) };
		const TArray<FRTCellId> R3 = URTGridLibrary::ReachableCellsByGraph(FRTCellId(5, 4, 0), 3, NoCost, Edges, 10, 10);
		TestTrue(TEXT("(5,5,1) raggiungibile con budget 3"), R3.Contains(FRTCellId(5, 5, 1)));
		const TArray<FRTCellId> R2 = URTGridLibrary::ReachableCellsByGraph(FRTCellId(5, 4, 0), 2, NoCost, Edges, 10, 10);
		TestFalse(TEXT("(5,5,1) NON raggiungibile con budget 2"), R2.Contains(FRTCellId(5, 5, 1)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridElevationLOSTest,
	"RefactorTactics.Grid.LineOfSightElevationRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridElevationLOSTest::RunTest(const FString&)
{
	// (a) 2D (retro-compat): copertura a terra sulla linea -> blocca.
	{
		const TArray<FRTCellId> B = { FRTCellId(2, 0, 0) };
		TestFalse(TEXT("2D: copertura blocca"), URTGridLibrary::HasLineOfSight(FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), B));
	}
	// (b) Dal PONTE (layer 1) verso terra: la copertura BASSA (layer 0) NON blocca -> si spara sopra.
	{
		const TArray<FRTCellId> B = { FRTCellId(2, 2, 0) };
		TestTrue(TEXT("ponte spara sopra la copertura bassa"), URTGridLibrary::HasLineOfSight(FRTCellId(2, 4, 1), FRTCellId(2, 0, 0), B));
	}
	// (c) Da TERRA (layer 0) verso il ponte: la copertura a terra sulla linea BLOCCA (stesso layer).
	{
		const TArray<FRTCellId> B = { FRTCellId(2, 2, 0) };
		TestFalse(TEXT("terra bloccata dalla copertura a terra"), URTGridLibrary::HasLineOfSight(FRTCellId(2, 0, 0), FRTCellId(2, 4, 1), B));
	}
	// (d) Sul ponte: un blocker sul ponte (stesso layer) blocca.
	{
		const TArray<FRTCellId> B = { FRTCellId(5, 4, 1) };
		TestFalse(TEXT("ponte: blocker sul ponte blocca"), URTGridLibrary::HasLineOfSight(FRTCellId(3, 4, 1), FRTCellId(7, 4, 1), B));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGridIsInBoundsTest,
	"RefactorTactics.Grid.IsInBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGridIsInBoundsTest::RunTest(const FString&)
{
	TestTrue(TEXT("(0,0) dentro 10x10"), URTGridLibrary::IsInBounds(FRTCellId(0, 0), 10, 10));
	TestTrue(TEXT("(9,9) dentro 10x10"), URTGridLibrary::IsInBounds(FRTCellId(9, 9), 10, 10));
	TestFalse(TEXT("(-1,0) fuori"), URTGridLibrary::IsInBounds(FRTCellId(-1, 0), 10, 10));
	TestFalse(TEXT("(10,0) fuori (X = Width)"), URTGridLibrary::IsInBounds(FRTCellId(10, 0), 10, 10));
	TestFalse(TEXT("(0,10) fuori (Y = Height)"), URTGridLibrary::IsInBounds(FRTCellId(0, 10), 10, 10));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
