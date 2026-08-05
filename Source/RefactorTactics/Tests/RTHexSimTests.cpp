#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N sul layer 0, tutte le celle a MoveCost 1. */
	URTHexMapAsset* MakeSimMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	bool HasCell(const TArray<FRTHexReachableCell>& Cells, const FRTCellId& Id)
	{
		return Cells.ContainsByPredicate([&Id](const FRTHexReachableCell& R) { return R.Cell == Id; });
	}

	bool PathContains(const FRTHexPathResult& Result, const FRTCellId& Id)
	{
		return Result.Path.Contains(Id);
	}
}

// ---------------------------------------------------------------------------------------------------------
// Snapshot e occupazione
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimSnapshotOccupancyTest,
	"RefactorTactics.HexSim.SnapshotCapturesOccupancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimSnapshotOccupancyTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(2);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	Units.Add(FRTHexSimUnit(2, FRTCellId(1, 0), 2, /*bAlive*/ false)); // eliminata: non occupa

	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	TestEqual(TEXT("hash catturato"), Snap.MapHash, M->ComputeHash());
	TestEqual(TEXT("revisione catturata"), Snap.Revision, M->Revision);
	TestEqual(TEXT("due unita' nello snapshot"), Snap.Units.Num(), 2);
	TestEqual(TEXT("una sola cella occupata"), Snap.Occupancy.Num(), 1);

	const int32* Occupant = Snap.Occupancy.Find(FRTCellId(0, 0));
	TestTrue(TEXT("(0,0) occupata dall'unita' 1"), Occupant != nullptr && *Occupant == 1);
	TestTrue(TEXT("la cella dell'unita' eliminata e' libera"), Snap.Occupancy.Find(FRTCellId(1, 0)) == nullptr);

	// IsCellFree: propria cella = libera per se stessi, occupata per gli altri.
	TestTrue(TEXT("cella vuota libera"), URTHexSimLibrary::IsCellFree(Snap, FRTCellId(0, 1), 1));
	TestTrue(TEXT("la propria cella non blocca se stessi"), URTHexSimLibrary::IsCellFree(Snap, FRTCellId(0, 0), 1));
	TestFalse(TEXT("cella occupata bloccata per gli altri"), URTHexSimLibrary::IsCellFree(Snap, FRTCellId(0, 0), 2));
	TestFalse(TEXT("cella fuori mappa non libera"), URTHexSimLibrary::IsCellFree(Snap, FRTCellId(9, 9), 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimSnapshotOrderTest,
	"RefactorTactics.HexSim.SnapshotOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimSnapshotOrderTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(2);

	TArray<FRTHexSimUnit> A;
	A.Add(FRTHexSimUnit(7, FRTCellId(1, 0), 1));
	A.Add(FRTHexSimUnit(3, FRTCellId(0, 1), 1));
	A.Add(FRTHexSimUnit(5, FRTCellId(0, 0), 1));

	TArray<FRTHexSimUnit> B;
	B.Add(A[2]); B.Add(A[0]); B.Add(A[1]); // stessa popolazione, ordine diverso

	const FRTHexSnapshot SA = URTHexSimLibrary::MakeSnapshot(M, A);
	const FRTHexSnapshot SB = URTHexSimLibrary::MakeSnapshot(M, B);

	TestEqual(TEXT("stesso numero di unita'"), SA.Units.Num(), SB.Units.Num());
	bool bSameOrder = SA.Units.Num() == SB.Units.Num();
	for (int32 i = 0; bSameOrder && i < SA.Units.Num(); ++i)
	{
		bSameOrder = SA.Units[i].UnitId == SB.Units[i].UnitId && SA.Units[i].Cell == SB.Units[i].Cell;
	}
	TestTrue(TEXT("ordine delle unita' identico (indipendente dall'input)"), bSameOrder);
	TestTrue(TEXT("unita' ordinate per UnitId crescente"),
		SA.Units.Num() == 3 && SA.Units[0].UnitId == 3 && SA.Units[1].UnitId == 5 && SA.Units[2].UnitId == 7);
	TestEqual(TEXT("stessa occupazione"), SA.Occupancy.Num(), SB.Occupancy.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimSnapshotStaleTest,
	"RefactorTactics.HexSim.SnapshotStaleAfterMapChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimSnapshotStaleTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(2);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	TestFalse(TEXT("snapshot fresco"), URTHexSimLibrary::IsSnapshotStale(Snap));

	// La mappa cambia DURANTE la fase: violazione dell'invariante -> lo snapshot va dichiarato stantio.
	FRTHexCellData Changed(FRTCellId(1, 0));
	Changed.MoveCost = 5;
	M->AddOrUpdateCell(Changed);

	TestTrue(TEXT("snapshot stantio dopo modifica della mappa"), URTHexSimLibrary::IsSnapshotStale(Snap));

	FRTHexSnapshot Orphan;
	TestTrue(TEXT("snapshot senza mappa e' stantio"), URTHexSimLibrary::IsSnapshotStale(Orphan));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimValidateTest,
	"RefactorTactics.HexSim.ValidateDetectsStructuralErrors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimValidateTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(2);

	{
		TArray<FRTHexSimUnit> Ok;
		Ok.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
		Ok.Add(FRTHexSimUnit(2, FRTCellId(1, 0), 2));
		TestEqual(TEXT("snapshot valido: nessun errore"),
			URTHexSimLibrary::ValidateSnapshot(URTHexSimLibrary::MakeSnapshot(M, Ok)).Num(), 0);
	}
	{
		TArray<FRTHexSimUnit> SameCell;
		SameCell.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
		SameCell.Add(FRTHexSimUnit(2, FRTCellId(0, 0), 2));
		TestTrue(TEXT("due unita' vive sulla stessa cella"),
			URTHexSimLibrary::ValidateSnapshot(URTHexSimLibrary::MakeSnapshot(M, SameCell)).Num() > 0);
	}
	{
		TArray<FRTHexSimUnit> OffMap;
		OffMap.Add(FRTHexSimUnit(1, FRTCellId(9, 9), 2));
		TestTrue(TEXT("unita' su cella assente dalla mappa"),
			URTHexSimLibrary::ValidateSnapshot(URTHexSimLibrary::MakeSnapshot(M, OffMap)).Num() > 0);
	}
	{
		TArray<FRTHexSimUnit> DupId;
		DupId.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
		DupId.Add(FRTHexSimUnit(1, FRTCellId(1, 0), 2));
		TestTrue(TEXT("UnitId duplicato"),
			URTHexSimLibrary::ValidateSnapshot(URTHexSimLibrary::MakeSnapshot(M, DupId)).Num() > 0);
	}
	{
		TArray<FRTHexSimUnit> NegBudget;
		NegBudget.Add(FRTHexSimUnit(1, FRTCellId(0, 0), -1));
		TestTrue(TEXT("budget negativo"),
			URTHexSimLibrary::ValidateSnapshot(URTHexSimLibrary::MakeSnapshot(M, NegBudget)).Num() > 0);
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Movement budget
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableBudgetTest,
	"RefactorTactics.HexSim.ReachableRespectsBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(3);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	const TArray<FRTHexReachableCell> R = URTHexSimLibrary::ReachableCells(Snap, 1);
	TestEqual(TEXT("19 celle entro budget 2 (3*2*3+1)"), R.Num(), 19);
	TestTrue(TEXT("include la partenza"), HasCell(R, FRTCellId(0, 0)));
	TestTrue(TEXT("include una cella a distanza 2"), HasCell(R, FRTCellId(2, 0)));
	TestFalse(TEXT("esclude una cella a distanza 3"), HasCell(R, FRTCellId(3, 0)));

	// Costo cumulato coerente con la distanza (costi tutti 1).
	const FRTHexReachableCell* Far = R.FindByPredicate([](const FRTHexReachableCell& C) { return C.Cell == FRTCellId(2, 0); });
	TestTrue(TEXT("costo 2 per la cella a distanza 2"), Far != nullptr && Far->Cost == 2);

	// Ordine deterministico (StableLess): nessuna dipendenza dall'ordine di TMap.
	bool bSorted = true;
	for (int32 i = 1; i < R.Num(); ++i)
	{
		bSorted &= URTHexLibrary::StableLess(R[i - 1].Cell, R[i].Cell);
	}
	TestTrue(TEXT("output ordinato in modo stabile"), bSorted);

	// Budget 0 -> solo la cella di partenza.
	TArray<FRTHexSimUnit> Still;
	Still.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 0));
	const TArray<FRTHexReachableCell> R0 = URTHexSimLibrary::ReachableCells(URTHexSimLibrary::MakeSnapshot(M, Still), 1);
	TestEqual(TEXT("budget 0 -> solo la partenza"), R0.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableCostTest,
	"RefactorTactics.HexSim.ReachableRespectsTerrainCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableCostTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(2);

	FRTHexCellData Costly(FRTCellId(1, 0));
	Costly.MoveCost = 3; // entrare qui costa 3
	M->AddOrUpdateCell(Costly);

	FRTHexCellData Wall(FRTCellId(0, 1));
	Wall.bBlocksMovement = true;
	M->AddOrUpdateCell(Wall);
	M->SortCells();

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	const TArray<FRTHexReachableCell> R2 = URTHexSimLibrary::ReachableCells(URTHexSimLibrary::MakeSnapshot(M, Units), 1);
	TestFalse(TEXT("cella a costo 3 fuori dal budget 2"), HasCell(R2, FRTCellId(1, 0)));
	TestFalse(TEXT("cella che blocca il movimento mai raggiungibile"), HasCell(R2, FRTCellId(0, 1)));

	TArray<FRTHexSimUnit> Rich;
	Rich.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 3));
	const TArray<FRTHexReachableCell> R3 = URTHexSimLibrary::ReachableCells(URTHexSimLibrary::MakeSnapshot(M, Rich), 1);
	TestTrue(TEXT("cella a costo 3 dentro il budget 3"), HasCell(R3, FRTCellId(1, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableOccupiedTest,
	"RefactorTactics.HexSim.ReachableExcludesOccupied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableOccupiedTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(2);

	// (1,0) e' l'UNICO passaggio a distanza 1 verso (2,0): occupandola, (2,0) esce dal budget 2.
	TArray<FRTHexSimUnit> Blocked;
	Blocked.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	Blocked.Add(FRTHexSimUnit(2, FRTCellId(1, 0), 0));
	const TArray<FRTHexReachableCell> RB = URTHexSimLibrary::ReachableCells(URTHexSimLibrary::MakeSnapshot(M, Blocked), 1);
	TestFalse(TEXT("cella occupata non raggiungibile"), HasCell(RB, FRTCellId(1, 0)));
	TestFalse(TEXT("cella dietro l'occupante fuori budget"), HasCell(RB, FRTCellId(2, 0)));

	// Senza occupante entrambe rientrano nel budget.
	TArray<FRTHexSimUnit> Free;
	Free.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	const TArray<FRTHexReachableCell> RF = URTHexSimLibrary::ReachableCells(URTHexSimLibrary::MakeSnapshot(M, Free), 1);
	TestTrue(TEXT("senza occupante (1,0) raggiungibile"), HasCell(RF, FRTCellId(1, 0)));
	TestTrue(TEXT("senza occupante (2,0) raggiungibile"), HasCell(RF, FRTCellId(2, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableTransitionTest,
	"RefactorTactics.HexSim.ReachableUsesTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableTransitionTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(1);
	M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1))); // cella isolata sul layer 1
	M->SortCells();
	M->AddTransition(FRTCellId(0, 0, 0), FRTCellId(0, 0, 1), 1, ERTHexTransitionKind::Stair, /*bBidirectional*/ true);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0, 0), 1));
	const TArray<FRTHexReachableCell> R = URTHexSimLibrary::ReachableCells(URTHexSimLibrary::MakeSnapshot(M, Units), 1);
	TestTrue(TEXT("il layer 1 si raggiunge tramite l'arco"), HasCell(R, FRTCellId(0, 0, 1)));

	TArray<FRTHexSimUnit> Still;
	Still.Add(FRTHexSimUnit(1, FRTCellId(0, 0, 0), 0));
	const TArray<FRTHexReachableCell> R0 = URTHexSimLibrary::ReachableCells(URTHexSimLibrary::MakeSnapshot(M, Still), 1);
	TestFalse(TEXT("senza budget l'arco non si percorre"), HasCell(R0, FRTCellId(0, 0, 1)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimPathBudgetTest,
	"RefactorTactics.HexSim.PathStopsAtBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimPathBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(5);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	const FRTHexPathResult Far = URTHexSimLibrary::FindPathForUnit(Snap, 1, FRTCellId(4, 0));
	TestTrue(TEXT("goal oltre il budget -> nessun percorso"), Far.Status != ERTHexPathStatus::Success);

	const FRTHexPathResult Near = URTHexSimLibrary::FindPathForUnit(Snap, 1, FRTCellId(2, 0));
	TestTrue(TEXT("goal entro il budget -> percorso"), Near.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("costo 2"), Near.TotalCost, 2);
	TestEqual(TEXT("3 celle (start + 2)"), Near.Path.Num(), 3);

	const FRTHexPathResult Unknown = URTHexSimLibrary::FindPathForUnit(Snap, 99, FRTCellId(1, 0));
	TestTrue(TEXT("unita' sconosciuta -> StartInvalid"), Unknown.Status == ERTHexPathStatus::StartInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimPathAvoidTest,
	"RefactorTactics.HexSim.PathAvoidsOccupiedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimPathAvoidTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(3);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 6));
	Units.Add(FRTHexSimUnit(2, FRTCellId(1, 0), 0)); // ostacolo vivo sul percorso diretto
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	const FRTHexPathResult Around = URTHexSimLibrary::FindPathForUnit(Snap, 1, FRTCellId(2, 0));
	TestTrue(TEXT("percorso trovato aggirando l'unita'"), Around.Status == ERTHexPathStatus::Success);
	TestFalse(TEXT("non attraversa la cella occupata"), PathContains(Around, FRTCellId(1, 0)));
	TestEqual(TEXT("costo 3 invece di 2 (deviazione)"), Around.TotalCost, 3);

	const FRTHexPathResult OntoUnit = URTHexSimLibrary::FindPathForUnit(Snap, 1, FRTCellId(1, 0));
	TestTrue(TEXT("goal occupato -> nessun percorso"), OntoUnit.Status != ERTHexPathStatus::Success);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Collisioni simultanee (microstep)
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimContestedTest,
	"RefactorTactics.HexSim.ResolveContestedDestination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimContestedTest::RunTest(const FString&)
{
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(2, 0), FRTCellId(1, 0) });

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths);
	TestEqual(TEXT("un risultato per richiesta"), R.Num(), 2);
	TestTrue(TEXT("A resta"), R.Num() == 2 && R[0].Final == FRTCellId(0, 0));
	TestTrue(TEXT("B resta"), R.Num() == 2 && R[1].Final == FRTCellId(2, 0));
	TestTrue(TEXT("reason = destinazione contesa"),
		R.Num() == 2 && R[0].Outcome == ERTMoveOutcome::BlockedContested && R[1].Outcome == ERTMoveOutcome::BlockedContested);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimSwapTest,
	"RefactorTactics.HexSim.ResolveSwapAllowed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimSwapTest::RunTest(const FString&)
{
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(1, 0), FRTCellId(0, 0) });

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths);
	TestTrue(TEXT("scambio diretto consentito"),
		R.Num() == 2 && R[0].Final == FRTCellId(1, 0) && R[1].Final == FRTCellId(0, 0));
	TestTrue(TEXT("entrambe risultano mosse"),
		R.Num() == 2 && R[0].Outcome == ERTMoveOutcome::Moved && R[1].Outcome == ERTMoveOutcome::Moved);
	TestTrue(TEXT("celle attraversate registrate"), R.Num() == 2 && R[0].Entered.Num() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimBlockedByStationaryTest,
	"RefactorTactics.HexSim.ResolveBlockedByStationary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimBlockedByStationaryTest::RunTest(const FString&)
{
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) }); // vuole passare
	Paths.Add({ FRTCellId(1, 0) });                                   // ferma sul passaggio

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths);
	TestTrue(TEXT("A bloccata alla partenza"), R.Num() == 2 && R[0].Final == FRTCellId(0, 0));
	TestTrue(TEXT("reason = bloccata da unita'"), R.Num() == 2 && R[0].Outcome == ERTMoveOutcome::BlockedByUnit);
	TestTrue(TEXT("B ferma"), R.Num() == 2 && R[1].Final == FRTCellId(1, 0) && R[1].Outcome == ERTMoveOutcome::Stayed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimOrderIndependenceTest,
	"RefactorTactics.HexSim.ResolveOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimOrderIndependenceTest::RunTest(const FString&)
{
	const TArray<FRTCellId> PA = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
	const TArray<FRTCellId> PB = { FRTCellId(3, 0), FRTCellId(2, 0) };
	const TArray<FRTCellId> PC = { FRTCellId(0, 1), FRTCellId(1, 1) };

	TArray<TArray<FRTCellId>> Order1; Order1.Add(PA); Order1.Add(PB); Order1.Add(PC);
	TArray<TArray<FRTCellId>> Order2; Order2.Add(PC); Order2.Add(PB); Order2.Add(PA);

	const TArray<FRTHexMoveResult> R1 = URTHexSimLibrary::ResolveHexPaths(Order1);
	const TArray<FRTHexMoveResult> R2 = URTHexSimLibrary::ResolveHexPaths(Order2);

	TestTrue(TEXT("A stesso esito nelle due permutazioni"),
		R1.Num() == 3 && R2.Num() == 3 && R1[0].Final == R2[2].Final && R1[0].Outcome == R2[2].Outcome);
	TestTrue(TEXT("B stesso esito"), R1.Num() == 3 && R2.Num() == 3 && R1[1].Final == R2[1].Final);
	TestTrue(TEXT("C stesso esito"), R1.Num() == 3 && R2.Num() == 3 && R1[2].Final == R2[0].Final);
	TestTrue(TEXT("C libera si muove"), R1.Num() == 3 && R1[2].Final == FRTCellId(1, 1));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
