#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "HAL/FileManager.h"
#include "Ability/RTCatalogLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Misc/Paths.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Turn/RTTurnLogLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N sul layer 0, tutte le celle a MoveCost 1. */
	URTHexMapAsset* MakeSimMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
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

	/** Voce di log con la cella di partenza indicata (la chiave stabile dell'unita' nel turno). */
	const FRTTurnLogEntry* EntryFromCell(const TArray<FRTTurnLogEntry>& Log, const FRTCellId& Src)
	{
		return Log.FindByPredicate([&Src](const FRTTurnLogEntry& E)
		{
			return E.SrcCell.X == Src.X && E.SrcCell.Y == Src.Y && E.SrcCell.Layer == Src.Layer;
		});
	}

	/** Tre intenti che coprono i tre esiti: A bloccata da B ferma, B ferma, C libera di muoversi. */
	TArray<TArray<FRTCellId>> SamplePaths()
	{
		TArray<TArray<FRTCellId>> Paths;
		Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) });
		Paths.Add({ FRTCellId(1, 0) });
		Paths.Add({ FRTCellId(0, 1), FRTCellId(1, 1) });
		return Paths;
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

/**
 * 🔴 **La sovrapposizione si REGISTRA nello snapshot, invece di sparire** (`#1970`).
 *
 * `MakeSnapshot` dichiarava che le sovrapposizioni erano *«un errore strutturale, segnalato da
 * `ValidateSnapshot`»* — e in partita `ValidateSnapshot` non lo chiamava nessuno: cinque test e un report
 * di debug su richiesta esplicita. L'invariante era dichiarata e non la guardava nessuno.
 *
 * ⚠️ **Si asserisce il DATO e non il log.** Il log lo emette il resolver autoritativo, e testarlo vorrebbe
 * `AddExpectedError`, che in questo repository conta le occorrenze **esatte**: un test cosi' si romperebbe
 * ogni volta che un'altra feature attraversa lo stesso percorso. Il campo si asserisce senza accoppiare
 * test indipendenti.
 *
 * ⛔ E si asserisce anche cio' che NON cambia: `Occupancy` resta identica, perche' questa fetta rende la
 * condizione visibile e non la risolve diversamente. Senza questa meta', un domani «migliorare» la
 * risoluzione del conflitto cambierebbe l'esito di partite esistenti senza far cadere niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSnapshotRecordsOverlapsTest,
	"RefactorTactics.HexSim.SnapshotRecordsOverlaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSnapshotRecordsOverlapsTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(2);
	const FRTCellId Contesa(0, 0);

	// --- 1. Due unita' VIVE sulla stessa cella: una voce, con i tre campi giusti -----------------------
	{
		TArray<FRTHexSimUnit> Due;
		Due.Add(FRTHexSimUnit(1, Contesa, 2));
		Due.Add(FRTHexSimUnit(2, Contesa, 2));
		const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Due);

		if (!TestEqual(TEXT("una sovrapposizione registrata"), Snap.Overlaps.Num(), 1)) { return false; }
		TestEqual(TEXT("sulla cella contesa"), Snap.Overlaps[0].Cell, Contesa);
		TestEqual(TEXT("scartata l'unita' con id maggiore"), Snap.Overlaps[0].DiscardedUnitId, 2);
		TestEqual(TEXT("la occupa quella con id minore"), Snap.Overlaps[0].KeptUnitId, 1);

		// ⛔ L'esito NON cambia: e' il punto in cui questa fetta si distingue da una che "corregge".
		const int32* Occupante = Snap.Occupancy.Find(Contesa);
		if (!TestNotNull(TEXT("la cella resta occupata"), Occupante)) { return false; }
		TestEqual(TEXT("e vince ancora l'UnitId minore"), *Occupante, 1);
		TestEqual(TEXT("una sola cella occupata"), Snap.Occupancy.Num(), 1);
	}

	// --- 2. Una viva e una MORTA: nessuna sovrapposizione, perche' un cadavere non occupa --------------
	{
		TArray<FRTHexSimUnit> VivaEMorta;
		VivaEMorta.Add(FRTHexSimUnit(1, Contesa, 2));
		VivaEMorta.Add(FRTHexSimUnit(2, Contesa, 2, /*bInAlive=*/ false));
		const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, VivaEMorta);

		TestEqual(TEXT("nessun falso positivo su un'unita' morta"), Snap.Overlaps.Num(), 0);
		TestEqual(TEXT("e la viva occupa"), Snap.Occupancy.FindRef(Contesa), 1);
	}

	// --- 3. TRE vive sulla stessa cella: due sovrapposizioni, entrambe verso il vincitore --------------
	{
		TArray<FRTHexSimUnit> Tre;
		Tre.Add(FRTHexSimUnit(1, Contesa, 2));
		Tre.Add(FRTHexSimUnit(2, Contesa, 2));
		Tre.Add(FRTHexSimUnit(3, Contesa, 2));
		const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Tre);

		if (!TestEqual(TEXT("due sovrapposizioni"), Snap.Overlaps.Num(), 2)) { return false; }
		for (const FRTHexOverlap& O : Snap.Overlaps)
		{
			TestEqual(TEXT("ognuna punta al medesimo vincitore"), O.KeptUnitId, 1);
		}
	}

	// --- 4. Uno snapshot sano non registra niente ------------------------------------------------------
	{
		TArray<FRTHexSimUnit> Sano;
		Sano.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
		Sano.Add(FRTHexSimUnit(2, FRTCellId(1, 0), 2));
		const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Sano);

		TestEqual(TEXT("nessuna sovrapposizione su uno snapshot sano"), Snap.Overlaps.Num(), 0);
		TestEqual(TEXT("ed entrambe occupano"), Snap.Occupancy.Num(), 2);
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

// ---------------------------------------------------------------------------------------------------------
// Fan RESIDUO (#877): dove si puo' ancora arrivare DOPO aver percorso i waypoint gia' pianificati.
//
// Il ventaglio verde diceva «dove posso arrivare questo turno, da dove sono» — informazione statica, che i
// waypoint non consumavano. Il giocatore legge «quanto mi resta». Questi test pinnano la seconda lettura.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableAfterPlanShrinksTest,
	"RefactorTactics.HexSim.ReachableAfterPlanShrinksFromPlannedTip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableAfterPlanShrinksTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(3);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	// Nessun waypoint: identico a `ReachableCells`, e il numero e' pinnato (3n^2+3n+1 con n=2). «Identico a
	// oggi» non falsifica nulla se «oggi» cambia; 19 si'.
	const TArray<FRTHexReachableCell> Full =
		URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 1, TArray<FRTCellId>());
	TestEqual(TEXT("senza waypoint: raggio pieno, 19 celle entro budget 2"), Full.Num(), 19);
	TestEqual(TEXT("senza waypoint coincide con ReachableCells"),
		Full.Num(), URTHexSimLibrary::ReachableCells(Snap, 1).Num());

	// Un waypoint a distanza 1: ne resta uno solo di MP, e il fan riparte DALLA PUNTA.
	TArray<FRTCellId> Waypoints;
	Waypoints.Add(FRTCellId(1, 0));
	const TArray<FRTHexReachableCell> After = URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 1, Waypoints);

	TestEqual(TEXT("un MP residuo: le 7 celle attorno alla punta"), After.Num(), 7);
	TestTrue(TEXT("la punta del percorso e' nel fan"), HasCell(After, FRTCellId(1, 0)));
	const FRTHexReachableCell* Tip =
		After.FindByPredicate([](const FRTHexReachableCell& C) { return C.Cell == FRTCellId(1, 0); });
	TestTrue(TEXT("la punta costa 0: e' la nuova partenza"), Tip != nullptr && Tip->Cost == 0);
	TestTrue(TEXT("si puo' tornare sulla cella di partenza"), HasCell(After, FRTCellId(0, 0)));

	// (-2,0) era dentro il raggio pieno (distanza 2) ed e' a 3 dalla punta: esce. E' il restringimento.
	TestTrue(TEXT("precondizione: (-2,0) era nel fan pieno"), HasCell(Full, FRTCellId(-2, 0)));
	TestFalse(TEXT("(-2,0) esce dal fan residuo"), HasCell(After, FRTCellId(-2, 0)));

	// Contenimento: il fan residuo non inventa celle che il budget pieno non copriva.
	bool bContained = true;
	for (const FRTHexReachableCell& C : After)
	{
		bContained &= HasCell(Full, C.Cell);
	}
	TestTrue(TEXT("il fan residuo e' contenuto in quello iniziale"), bContained);
	TestTrue(TEXT("ed e' strettamente piu' piccolo"), After.Num() < Full.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableAfterPlanUndoTest,
	"RefactorTactics.HexSim.ReachableAfterPlanUndoRestoresExactly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableAfterPlanUndoTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(3);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 3));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	TArray<FRTCellId> One;
	One.Add(FRTCellId(1, 0));
	const TArray<FRTHexReachableCell> AfterOne = URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 1, One);

	TArray<FRTCellId> Two = One;
	Two.Add(FRTCellId(2, 0));
	const TArray<FRTHexReachableCell> AfterTwo = URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 1, Two);
	TestTrue(TEXT("il secondo waypoint stringe ancora"), AfterTwo.Num() < AfterOne.Num());

	// L'undo toglie l'ultimo waypoint e ricalcola: deve tornare lo STESSO insieme, cella per cella. La
	// funzione e' pura, quindi qui l'invariante e' garantita per costruzione — il test la pinna perche' cada
	// il giorno in cui qualcuno ci mette una cache che non si invalida.
	const TArray<FRTHexReachableCell> Undone = URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 1, One);
	TestEqual(TEXT("l'undo riporta lo stesso numero di celle"), Undone.Num(), AfterOne.Num());
	bool bSame = true;
	for (int32 i = 0; i < Undone.Num() && i < AfterOne.Num(); ++i)
	{
		bSame &= (Undone[i].Cell == AfterOne[i].Cell) && (Undone[i].Cost == AfterOne[i].Cost);
	}
	TestTrue(TEXT("l'undo riporta esattamente le stesse celle, agli stessi costi"), bSame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableAfterPlanExhaustedTest,
	"RefactorTactics.HexSim.ReachableAfterPlanExhaustedKeepsArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableAfterPlanExhaustedTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(3);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	// Waypoint a distanza 2 con budget 2: residuo zero.
	TArray<FRTCellId> Waypoints;
	Waypoints.Add(FRTCellId(2, 0));
	const TArray<FRTHexReachableCell> After = URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 1, Waypoints);

	// Non vuoto e non sparito: la cella d'arrivo resta, a costo 0. Un fan vuoto direbbe «non puoi stare da
	// nessuna parte», che e' falso — ci sei gia'.
	TestEqual(TEXT("budget esaurito: una sola cella"), After.Num(), 1);
	TestTrue(TEXT("ed e' la cella d'arrivo"), HasCell(After, FRTCellId(2, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableAfterPlanInvalidTest,
	"RefactorTactics.HexSim.ReachableAfterPlanInvalidPlanFallsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableAfterPlanInvalidTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(3);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	// Waypoint a distanza 3 con budget 2: `BuildCompositeHexPath` rifiuta l'INTERO percorso. Non esiste una
	// punta da cui ripartire, quindi il fan torna quello pieno — come il piano, che e' «resto fermo».
	TArray<FRTCellId> Unreachable;
	Unreachable.Add(FRTCellId(3, 0));
	const TArray<FRTHexReachableCell> After = URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 1, Unreachable);
	TestEqual(TEXT("piano invalido: fan pieno dalla cella reale"), After.Num(), 19);
	TestTrue(TEXT("include la cella reale"), HasCell(After, FRTCellId(0, 0)));

	// Unita' sconosciuta: nessun fan, come `ReachableCells`.
	const TArray<FRTHexReachableCell> Ghost =
		URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 99, TArray<FRTCellId>());
	TestEqual(TEXT("unita' fuori snapshot: fan vuoto"), Ghost.Num(), 0);
	return true;
}

/**
 * L'INVARIANTE che impedisce alla zona mostrata di divergere da quella subita: una cella e' nel verde se e
 * solo se, cliccandola, la pianificazione la accetta come waypoint.
 *
 * ⚠️ L'oracolo e' `BuildCompositeHexPath`, non `ClassifyWaypointCell`: quest'ultima **non guarda il budget**
 * (restituisce `Ok` per ogni cella libera della mappa, anche a cinquanta esagoni), quindi un confronto con lei
 * sarebbe vero per costruzione in un verso e falso per sempre nell'altro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReachableAfterPlanMatchesAcceptanceTest,
	"RefactorTactics.HexSim.ReachableAfterPlanMatchesWaypointAcceptance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReachableAfterPlanMatchesAcceptanceTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(3);

	FRTHexCellData Wall(FRTCellId(0, 1));
	Wall.bBlocksMovement = true;
	M->AddOrUpdateCell(Wall);

	FRTHexCellData Mud(FRTCellId(1, -1));
	Mud.MoveCost = 2;
	M->AddOrUpdateCell(Mud);
	M->SortCells();

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 3));
	Units.Add(FRTHexSimUnit(2, FRTCellId(-1, 0), 0)); // un'altra unita': la sua cella non e' un waypoint valido
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	TArray<FRTCellId> Waypoints;
	Waypoints.Add(FRTCellId(1, 0));

	const TArray<FRTHexReachableCell> Fan = URTHexSimLibrary::ReachableCellsAfterPlan(Snap, 1, Waypoints);
	TestTrue(TEXT("il fan non e' vuoto (altrimenti il test non proverebbe nulla)"), Fan.Num() > 1);

	int32 Divergenze = 0;
	for (const FRTHexCellData& Cell : M->Cells)
	{
		TArray<FRTCellId> Probe = Waypoints;
		Probe.Add(Cell.Id);
		const bool bAccettata =
			URTHexSimLibrary::BuildCompositeHexPath(Snap, 1, Probe).Status == ERTHexPathStatus::Success;
		const bool bNelFan = HasCell(Fan, Cell.Id);
		if (bAccettata != bNelFan)
		{
			++Divergenze;
			AddError(FString::Printf(TEXT("(%d,%d,L%d): nel fan=%s, accettata=%s"),
				Cell.Id.X, Cell.Id.Y, Cell.Id.Layer,
				bNelFan ? TEXT("si") : TEXT("no"), bAccettata ? TEXT("si") : TEXT("no")));
		}
	}
	TestEqual(TEXT("nessuna cella della mappa diverge fra fan e accettazione"), Divergenze, 0);
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

/**
 * Lo scambio diretto BLOCCA (#1922, `D-295` ← `AUTHOR-MOVE-001`).
 *
 * 🔄 Questo test si chiamava `ResolveSwapAllowed` e asseriva l'opposto: era una caratterizzazione fedele
 * del resolver, non una regola voluta. `StepHexMovement` bloccava per due sole condizioni — destinazione
 * contesa e unita' ferma — e in uno scambio non si verifica nessuna delle due: le celle sono distinte e
 * nessuna delle due unita' e' ferma.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimSwapTest,
	"RefactorTactics.HexSim.ResolveSwapBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimSwapTest::RunTest(const FString&)
{
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(1, 0), FRTCellId(0, 0) });

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths);
	if (!TestEqual(TEXT("due risultati"), R.Num(), 2)) { return false; }
	TestTrue(TEXT("nessuna delle due si muove"),
		R[0].Final == FRTCellId(0, 0) && R[1].Final == FRTCellId(1, 0));
	TestTrue(TEXT("reason = ciclo, per entrambe"),
		R[0].Outcome == ERTMoveOutcome::BlockedByCycle && R[1].Outcome == ERTMoveOutcome::BlockedByCycle);
	TestEqual(TEXT("nessuna cella attraversata"), R[0].Entered.Num(), 0);
	return true;
}

/**
 * Il ciclo chiuso a tre BLOCCA. E' la forma generale di cui lo scambio e' il caso `n = 2`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimClosedCycleTest,
	"RefactorTactics.HexSim.ResolveClosedCycleBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimClosedCycleTest::RunTest(const FString&)
{
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(1, 0), FRTCellId(2, 0) });
	Paths.Add({ FRTCellId(2, 0), FRTCellId(0, 0) });

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths);
	if (!TestEqual(TEXT("tre risultati"), R.Num(), 3)) { return false; }
	TestTrue(TEXT("nessuna delle tre si muove"),
		R[0].Final == FRTCellId(0, 0) && R[1].Final == FRTCellId(1, 0) && R[2].Final == FRTCellId(2, 0));
	TestTrue(TEXT("reason = ciclo, per tutte e tre"),
		R[0].Outcome == ERTMoveOutcome::BlockedByCycle
		&& R[1].Outcome == ERTMoveOutcome::BlockedByCycle
		&& R[2].Outcome == ERTMoveOutcome::BlockedByCycle);
	return true;
}

/**
 * 🔴 Il convoy a coda libera AVANZA, e questo e' il test che distingue una regola corretta da una che
 * rompe il gioco.
 *
 * Convoy e ciclo hanno la STESSA forma — ogni unita' punta alla cella occupata da un'altra in movimento,
 * nessuna e' ferma — e differiscono SOLO per l'ultima cella della catena. La regola ovvia
 * (*«se il mio target e' la posizione di un altro mover, blocca»*) supera il test del ciclo e uccide questo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimFreeTailConvoyTest,
	"RefactorTactics.HexSim.ResolveFreeTailConvoyStillAdvances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimFreeTailConvoyTest::RunTest(const FString&)
{
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(1, 0), FRTCellId(2, 0) });
	Paths.Add({ FRTCellId(2, 0), FRTCellId(3, 0) }); // (3,0) e' LIBERA: la catena non si chiude

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths);
	if (!TestEqual(TEXT("tre risultati"), R.Num(), 3)) { return false; }
	TestTrue(TEXT("tutte e tre avanzano di una cella"),
		R[0].Final == FRTCellId(1, 0) && R[1].Final == FRTCellId(2, 0) && R[2].Final == FRTCellId(3, 0));
	TestTrue(TEXT("esito = mosse, per tutte e tre"),
		R[0].Outcome == ERTMoveOutcome::Moved
		&& R[1].Outcome == ERTMoveOutcome::Moved
		&& R[2].Outcome == ERTMoveOutcome::Moved);
	return true;
}

/**
 * Uno scambio in cui una delle due sta soltanto TRANSITANDO blocca comunque.
 *
 * ⚠️ E' il test che prova che la regola guarda i mover e non `bPassThrough`. Il resolver avanza a
 * micro-step — `Target[i] = Paths[i][Prog[i] + 1]` — quindi uno scambio puo' formarsi anche quando per
 * una delle due quella cella non e' la destinazione finale: qui A transita per `(1,0)` e vorrebbe finire in
 * `(2,0)`. Il flag `bPassThrough` governa il ramo dell'unita' FERMA e qui non c'entra: B e' in movimento.
 * Un'implementazione che lo consultasse passerebbe tutti gli altri test di questo file e sbaglierebbe qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimSwapWhilePassingThroughTest,
	"RefactorTactics.HexSim.ResolveSwapBlockedEvenWhenPassingThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimSwapWhilePassingThroughTest::RunTest(const FString&)
{
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) }); // A transita per (1,0)
	Paths.Add({ FRTCellId(1, 0), FRTCellId(0, 0) });                  // B va dove A si trova

	const TArray<bool> PassThrough = { true, false };
	const TArray<FRTHexMoveResult> R =
		URTHexSimLibrary::ResolveHexPaths(Paths, TArray<int32>(), TArray<bool>(), PassThrough);
	if (!TestEqual(TEXT("due risultati"), R.Num(), 2)) { return false; }
	TestTrue(TEXT("nessuna delle due si muove"),
		R[0].Final == FRTCellId(0, 0) && R[1].Final == FRTCellId(1, 0));
	TestTrue(TEXT("reason = ciclo, non bloccata da unita'"),
		R[0].Outcome == ERTMoveOutcome::BlockedByCycle && R[1].Outcome == ERTMoveOutcome::BlockedByCycle);
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

// ---------------------------------------------------------------------------------------------------------
// Collisioni con priorita' (CP 4.8): variante di ResolveHexPaths con precedenza e scontro frontale.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChargeBeatsMoveTest,
	"RefactorTactics.Actions.Charge.BeatsMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChargeBeatsMoveTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 (CP 4.8). Le priorita' sono quelle REALI del catalogo (numero PIU'
	// BASSO vince, stessa convenzione di URTActionQueueLibrary): `Action.Charge` (35) contro `Action.Move`
	// (50) sulla STESSA cella contesa. In partita le due fasi non si sovrappongono mai (Dash prima, Move
	// dopo il Blast): questo verifica il MECCANISMO condiviso, non uno scontro che avviene davvero fra loro.
	const int32 ChargePriority = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge")).Priority;
	const int32 MovePriority = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move")).Priority;
	if (!TestTrue(TEXT("Charge precede Move nel catalogo"), ChargePriority < MovePriority)) { return false; }

	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) }); // Charge
	Paths.Add({ FRTCellId(2, 0), FRTCellId(1, 0) }); // Move
	const TArray<int32> Priorities = { ChargePriority, MovePriority };
	const TArray<bool> bLinear = { true, false };

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths, Priorities, bLinear);
	TestTrue(TEXT("il Charge entra nella cella contesa"),
		R.Num() == 2 && R[0].Final == FRTCellId(1, 0) && R[0].Outcome == ERTMoveOutcome::Moved);
	TestTrue(TEXT("il Move resta indietro, per priorita' non per contesa a parita'"),
		R.Num() == 2 && R[1].Final == FRTCellId(2, 0) && R[1].Outcome == ERTMoveOutcome::BlockedByPriority);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimPriorityTieStillContestedTest,
	"RefactorTactics.HexSim.ResolvePriorityTieStillContested",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimPriorityTieStillContestedTest::RunTest(const FString&)
{
	// A PARITA' di priorita' (anche dichiarata esplicitamente, non solo "assente"), il comportamento resta
	// quello di base: "Charge prevale su Move", non "il primo dell'array vince".
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(2, 0), FRTCellId(1, 0) });
	const TArray<int32> Priorities = { 35, 35 };
	const TArray<bool> bLinear = { true, true };

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths, Priorities, bLinear);
	TestTrue(TEXT("entrambe ferme, nessuna vince per indice"),
		R.Num() == 2 && R[0].Final == FRTCellId(0, 0) && R[1].Final == FRTCellId(2, 0));
	TestTrue(TEXT("reason = contesa, non priorita'"),
		R.Num() == 2 && R[0].Outcome == ERTMoveOutcome::BlockedContested && R[1].Outcome == ERTMoveOutcome::BlockedContested);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimHeadOnBlocksLinearSwapTest,
	"RefactorTactics.HexSim.ResolveHeadOnBlocksLinearSwap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimHeadOnBlocksLinearSwapTest::RunTest(const FString&)
{
	// Contrasto diretto con ResolveSwapBlocked: la' lo scambio blocca come CICLO (`BlockedByCycle`), qui
	// due mobilita' LINEARI (`Action.Charge` e affini) che si scambierebbero la cella si fermano l'una davanti
	// all'altra invece di attraversarsi — e' lo scontro frontale di due cariche opposte (CP 4.8).
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(1, 0), FRTCellId(0, 0) });
	const TArray<int32> Priorities = { 35, 35 };
	const TArray<bool> bLinear = { true, true };

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths, Priorities, bLinear);
	TestTrue(TEXT("nessuna attraversa l'altra"),
		R.Num() == 2 && R[0].Final == FRTCellId(0, 0) && R[1].Final == FRTCellId(1, 0));
	TestTrue(TEXT("reason = scontro frontale"),
		R.Num() == 2 && R[0].Outcome == ERTMoveOutcome::BlockedByImpact && R[1].Outcome == ERTMoveOutcome::BlockedByImpact);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCollisionsNoPlayerIdBiasTest,
	"RefactorTactics.Actions.Collisions.NoPlayerIdBias",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCollisionsNoPlayerIdBiasTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 (CP 4.8). Stesso scontro di FRTChargeBeatsMoveTest, con i due
	// contendenti scambiati di POSIZIONE nell'array (permutazione degli "ID"): chi vince deve dipendere dalla
	// priorita' dichiarata, mai dall'indice/ordine di inserimento.
	const int32 ChargePriority = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge")).Priority;
	const int32 MovePriority = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move")).Priority;

	TArray<TArray<FRTCellId>> PathsOrderA = {
		{ FRTCellId(0, 0), FRTCellId(1, 0) }, // Charge in posizione 0
		{ FRTCellId(2, 0), FRTCellId(1, 0) }  // Move in posizione 1
	};
	const TArray<FRTHexMoveResult> ResultA = URTHexSimLibrary::ResolveHexPaths(
		PathsOrderA, { ChargePriority, MovePriority }, { true, false });

	TArray<TArray<FRTCellId>> PathsOrderB = {
		{ FRTCellId(2, 0), FRTCellId(1, 0) }, // Move in posizione 0
		{ FRTCellId(0, 0), FRTCellId(1, 0) }  // Charge in posizione 1
	};
	const TArray<FRTHexMoveResult> ResultB = URTHexSimLibrary::ResolveHexPaths(
		PathsOrderB, { MovePriority, ChargePriority }, { false, true });

	if (!TestEqual(TEXT("un risultato per contendente, in entrambi gli ordini"), ResultA.Num(), 2)
		|| !TestEqual(TEXT("un risultato per contendente, in entrambi gli ordini"), ResultB.Num(), 2))
	{
		return false;
	}

	// Ordine A: il Charge e' in posizione 0. Ordine B: il Charge e' in posizione 1. L'esito letto PER RUOLO
	// (non per indice) deve coincidere.
	TestTrue(TEXT("il Charge vince in entrambi gli ordini"),
		ResultA[0].Final == FRTCellId(1, 0) && ResultB[1].Final == FRTCellId(1, 0));
	TestTrue(TEXT("il Move perde in entrambi gli ordini"),
		ResultA[1].Final == FRTCellId(2, 0) && ResultB[0].Final == FRTCellId(2, 0));
	TestTrue(TEXT("stesso reason code per ruolo, a prescindere dall'indice"),
		ResultA[0].Outcome == ResultB[1].Outcome && ResultA[1].Outcome == ResultB[0].Outcome);
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

// ---------------------------------------------------------------------------------------------------------
// TurnLog e replay (H6.3)
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimMoveLogCauseTest,
	"RefactorTactics.HexSim.MoveLogCarriesCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimMoveLogCauseTest::RunTest(const FString&)
{
	const TArray<TArray<FRTCellId>> Paths = SamplePaths();
	const TArray<FRTHexMoveResult> Results = URTHexSimLibrary::ResolveHexPaths(Paths);

	// La CAUSA di uno spostamento (#307): il log diceva CHE l'unita' si e' spostata e con quale esito, non
	// PERCHE'. `ActionId` esiste gia' nella voce ed e' gia' serializzato e gia' nell'hash: valorizzarlo non
	// tocca il formato, quindi il corpus golden di #178 puo' nascere senza attendere una migrazione.
	const TArray<FRTTurnLogEntry> Log =
		URTHexSimLibrary::BuildMoveLog(Paths, Results, /*CauseActionId*/ TEXT("Action.Move"));

	TestTrue(TEXT("almeno una voce, o il ciclo non verifica nulla"), Log.Num() > 0);
	for (const FRTTurnLogEntry& Entry : Log)
	{
		TestEqual(TEXT("ogni voce di movimento dichiara la propria causa"),
			Entry.ActionId, FName(TEXT("Action.Move")));
	}

	// Senza causa dichiarata la voce resta com'era: le tracce gia' scritte non cambiano significato.
	const TArray<FRTTurnLogEntry> Legacy = URTHexSimLibrary::BuildMoveLog(Paths, Results);
	TestTrue(TEXT("senza causa l'ActionId resta vuoto"), Legacy.Num() > 0 && Legacy[0].ActionId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimBuildMoveLogTest,
	"RefactorTactics.HexSim.BuildMoveLogEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimBuildMoveLogTest::RunTest(const FString&)
{
	const TArray<TArray<FRTCellId>> Paths = SamplePaths();
	const TArray<FRTHexMoveResult> Results = URTHexSimLibrary::ResolveHexPaths(Paths);
	const TArray<FRTTurnLogEntry> Log = URTHexSimLibrary::BuildMoveLog(Paths, Results);

	TestEqual(TEXT("una voce per unita'"), Log.Num(), 3);

	// La coordinata di log conserva le assiali (q, r, Layer) senza reinterpretazioni: BuildMoveLog emette
	// una voce per unita' nell'ordine dell'input, quindi Log[0] appartiene a Paths[0].
	if (Paths.Num() > 0 && Paths[0].Num() > 0 && Log.Num() > 0)
	{
		TestTrue(TEXT("la cella di partenza entra nel log invariata"), Log[0].SrcCell == Paths[0][0]);
	}

	if (const FRTTurnLogEntry* Blocked = EntryFromCell(Log, FRTCellId(0, 0)))
	{
		TestTrue(TEXT("A: fase e categoria di movimento"),
			Blocked->Phase == ERTMatchPhase::Move && Blocked->Category == ERTLogCategory::Move);
		TestEqual(TEXT("A: reason = bloccata da unita'"),
			Blocked->Outcome, static_cast<uint8>(ERTMoveOutcome::BlockedByUnit));
		TestTrue(TEXT("A: destinazione = cella di partenza (non si e' mossa)"),
			Blocked->TgtCell.X == 0 && Blocked->TgtCell.Y == 0);
		TestEqual(TEXT("A: zero celle percorse"), Blocked->Amount, 0);
	}
	else
	{
		AddError(TEXT("voce mancante per l'unita' bloccata"));
	}

	if (const FRTTurnLogEntry* Still = EntryFromCell(Log, FRTCellId(1, 0)))
	{
		TestEqual(TEXT("B: reason = ferma"), Still->Outcome, static_cast<uint8>(ERTMoveOutcome::Stayed));
		TestEqual(TEXT("B: zero celle percorse"), Still->Amount, 0);
	}
	else
	{
		AddError(TEXT("voce mancante per l'unita' ferma"));
	}

	if (const FRTTurnLogEntry* Moved = EntryFromCell(Log, FRTCellId(0, 1)))
	{
		TestEqual(TEXT("C: reason = mossa"), Moved->Outcome, static_cast<uint8>(ERTMoveOutcome::Moved));
		TestTrue(TEXT("C: destinazione raggiunta"), Moved->TgtCell.X == 1 && Moved->TgtCell.Y == 1);
		TestEqual(TEXT("C: una cella percorsa"), Moved->Amount, 1);
	}
	else
	{
		AddError(TEXT("voce mancante per l'unita' mossa"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimMoveLogPermutationTest,
	"RefactorTactics.HexSim.MoveLogPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimMoveLogPermutationTest::RunTest(const FString&)
{
	const TArray<TArray<FRTCellId>> Paths = SamplePaths();

	TArray<TArray<FRTCellId>> Shuffled;
	Shuffled.Add(Paths[2]); Shuffled.Add(Paths[0]); Shuffled.Add(Paths[1]);

	const TArray<FRTTurnLogEntry> LogA = URTHexSimLibrary::BuildMoveLog(Paths, URTHexSimLibrary::ResolveHexPaths(Paths));
	const TArray<FRTTurnLogEntry> LogB = URTHexSimLibrary::BuildMoveLog(Shuffled, URTHexSimLibrary::ResolveHexPaths(Shuffled));

	TestEqual(TEXT("una voce per unita'"), LogA.Num(), 3); // senza questa, due log vuoti darebbero lo stesso hash
	TestEqual(TEXT("stesso numero di voci"), LogA.Num(), LogB.Num());
	TestEqual(TEXT("hash indipendente dall'ordine delle unita'"),
		URTTurnLogLibrary::HashTurnLog(LogB), URTTurnLogLibrary::HashTurnLog(LogA));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimReplayDivergenceTest,
	"RefactorTactics.HexSim.ReplayDivergenceZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimReplayDivergenceTest::RunTest(const FString&)
{
	const TArray<TArray<FRTCellId>> Paths = SamplePaths();

	// Due esecuzioni della STESSA risoluzione -> stesso log -> stesso hash.
	const TArray<FRTTurnLogEntry> Run1 = URTHexSimLibrary::BuildMoveLog(Paths, URTHexSimLibrary::ResolveHexPaths(Paths));
	const TArray<FRTTurnLogEntry> Run2 = URTHexSimLibrary::BuildMoveLog(Paths, URTHexSimLibrary::ResolveHexPaths(Paths));
	const uint32 Expected = URTTurnLogLibrary::HashTurnLog(Run1);
	TestEqual(TEXT("nessuna divergenza fra due esecuzioni"), URTTurnLogLibrary::HashTurnLog(Run2), Expected);

	// Traccia persistita e ricaricata: hash preservato e topologia esagonale dichiarata nel formato.
	const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("turnlog_hex_replay.rttl"));
	TestTrue(TEXT("salvataggio riuscito"),
		URTTurnLogLibrary::SaveTurnLogToFile(Path, Run1, ERTLogTopology::Hex));

	TArray<FRTTurnLogEntry> Restored;
	ERTLogTopology Topology = ERTLogTopology::Square;
	TestTrue(TEXT("caricamento riuscito"), URTTurnLogLibrary::LoadTurnLogFromFile(Path, Restored, &Topology));
	TestTrue(TEXT("topologia esagonale conservata nel file"), Topology == ERTLogTopology::Hex);
	TestEqual(TEXT("hash preservato dal round-trip su file"), URTTurnLogLibrary::HashTurnLog(Restored), Expected);
	IFileManager::Get().Delete(*Path);

	// Un intento diverso deve produrre una traccia diversa (l'hash non e' cieco ai cambiamenti).
	TArray<TArray<FRTCellId>> Different = Paths;
	Different[2] = { FRTCellId(0, 1), FRTCellId(0, 2) };
	const TArray<FRTTurnLogEntry> Other =
		URTHexSimLibrary::BuildMoveLog(Different, URTHexSimLibrary::ResolveHexPaths(Different));
	TestNotEqual(TEXT("intento diverso -> hash diverso"), URTTurnLogLibrary::HashTurnLog(Other), Expected);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 6.3 — percorso composito a waypoint (pianificazione del giocatore)
// ---------------------------------------------------------------------------------------------------------

/**
 * Il giocatore clicca piu' celle: il percorso deve passare DA OGNI waypoint nell'ordine dato, non prendere la
 * scorciatoia. Caso discriminante: (0,0) -> (2,0) -> (2,-2) costa 4, mentre la diagonale diretta
 * (0,0) -> (2,-2) costerebbe 2. Se i waypoint venissero ignorati il costo sarebbe 2.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCompositePathTest,
	"RefactorTactics.HexSim.CompositePathFollowsWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCompositePathTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSimMap(3);
	const FRTCellId Start(0, 0, 0);
	const FRTCellId Via(2, 0, 0);
	const FRTCellId Goal(2, -2, 0);

	// Premessa del test: la scorciatoia esiste ed e' piu' corta. Se cade, il caso non discrimina piu'.
	TestEqual(TEXT("premessa: distanza diretta 2"), URTHexLibrary::HexDistance(Start, Goal), 2);

	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, /*MoveBudget=*/ 5) });
	const FRTHexPathResult R = URTHexSimLibrary::BuildCompositeHexPath(Snap, 7, { Via, Goal });

	TestTrue(TEXT("percorso trovato"), R.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("costo = somma dei tratti (2+2), non la scorciatoia"), R.TotalCost, 4);
	TestTrue(TEXT("passa dal waypoint intermedio"), PathContains(R, Via));
	if (R.Path.Num() > 0)
	{
		TestTrue(TEXT("parte dalla cella dell'unita'"), R.Path[0] == Start);
		TestTrue(TEXT("finisce sull'ultimo waypoint"), R.Path.Last() == Goal);
	}
	TestEqual(TEXT("celle totali = 1 + 2 + 2"), R.Path.Num(), 5);
	return true;
}

/**
 * Il budget si spende in modo CUMULATIVO sui tratti. Caso discriminante: budget 3, due tratti da 2 (totale 4).
 * Ogni tratto preso da solo entrerebbe nel budget: un'implementazione che passasse il budget pieno a ogni
 * tratto accetterebbe il percorso. Deve rifiutarlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCompositeBudgetTest,
	"RefactorTactics.HexSim.CompositePathBudgetIsCumulative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCompositeBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSimMap(3);
	const FRTCellId Start(0, 0, 0);
	const FRTCellId Via(2, 0, 0);
	const FRTCellId Goal(2, -2, 0);

	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, /*MoveBudget=*/ 3) });

	// Premessa: ciascun tratto da solo entra in 3 (costa 2).
	const FRTHexPathResult SingleLeg = URTHexSimLibrary::BuildCompositeHexPath(Snap, 7, { Via });
	TestTrue(TEXT("premessa: un solo tratto entra nel budget"), SingleLeg.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("premessa: un tratto costa 2"), SingleLeg.TotalCost, 2);

	// I due tratti insieme costano 4 > 3 -> rifiuto dell'intero percorso.
	const FRTHexPathResult Both = URTHexSimLibrary::BuildCompositeHexPath(Snap, 7, { Via, Goal });
	TestTrue(TEXT("due tratti oltre il budget -> rifiutato"), Both.Status != ERTHexPathStatus::Success);
	TestEqual(TEXT("percorso rifiutato -> nessuna cella"), Both.Path.Num(), 0);
	return true;
}

/** Cella oltre il budget in un colpo solo: rifiuto, non troncamento. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCompositeOutOfBudgetTest,
	"RefactorTactics.HexSim.CompositePathRejectsCellOutOfBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCompositeOutOfBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSimMap(3);
	const FRTCellId Start(0, 0, 0);
	const FRTCellId TooFar(3, 0, 0); // distanza 3, budget 2

	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, /*MoveBudget=*/ 2) });
	const FRTHexPathResult R = URTHexSimLibrary::BuildCompositeHexPath(Snap, 7, { TooFar });

	TestTrue(TEXT("fuori budget -> rifiutato"), R.Status != ERTHexPathStatus::Success);
	TestEqual(TEXT("nessun percorso parziale"), R.Path.Num(), 0);
	return true;
}

/** Waypoint su una cella occupata da un'ALTRA unita': rifiuto (una unita' non blocca se stessa). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCompositeOccupiedTest,
	"RefactorTactics.HexSim.CompositePathRejectsOccupiedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCompositeOccupiedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSimMap(3);
	const FRTCellId Start(0, 0, 0);
	const FRTCellId Occupied(2, 0, 0);

	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, {
		FRTHexSimUnit(7, Start, /*MoveBudget=*/ 5),
		FRTHexSimUnit(8, Occupied, /*MoveBudget=*/ 0)
	});

	const FRTHexPathResult R = URTHexSimLibrary::BuildCompositeHexPath(Snap, 7, { Occupied });
	TestTrue(TEXT("cella occupata -> rifiutata"), R.Status != ERTHexPathStatus::Success);

	// La stessa cella, libera, sarebbe raggiungibile: e' l'occupazione a rifiutarla, non la geometria.
	const FRTHexSnapshot Free = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, /*MoveBudget=*/ 5) });
	TestTrue(TEXT("controprova: libera e' raggiungibile"),
		URTHexSimLibrary::BuildCompositeHexPath(Free, 7, { Occupied }).Status == ERTHexPathStatus::Success);
	return true;
}

/** Nessun waypoint = piano "resto fermo": la sola cella di partenza, costo 0. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCompositeEmptyTest,
	"RefactorTactics.HexSim.CompositePathEmptyWaypointsStays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCompositeEmptyTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSimMap(2);
	const FRTCellId Start(1, 0, 0);

	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, /*MoveBudget=*/ 4) });
	const FRTHexPathResult R = URTHexSimLibrary::BuildCompositeHexPath(Snap, 7, {});

	TestTrue(TEXT("nessun waypoint -> Success"), R.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("solo la cella di partenza"), R.Path.Num(), 1);
	TestEqual(TEXT("costo 0"), R.TotalCost, 0);
	if (R.Path.Num() == 1)
	{
		TestTrue(TEXT("e' la cella dell'unita'"), R.Path[0] == Start);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveNoGlobalRecomputeTest,
	"RefactorTactics.Actions.Move.NoGlobalRecompute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveNoGlobalRecomputeTest::RunTest(const FString&)
{
	// Il percorso si calcola UNA VOLTA, in pianificazione; la risoluzione lo fa solo avanzare a micro-step.
	// Se durante la resolution ci fosse un ricalcolo globale (un A* per micro-step), un'unita' bloccata a meta'
	// strada aggirerebbe l'ostacolo e arriverebbe comunque a destinazione: qui deve invece FERMARSI.
	//
	// L'aggiramento esiste eccome sulla mappa (dalla riga r=-1 si arriva a (3,0) senza toccare (2,0)): e'
	// proprio la strada che un ricalcolo troverebbe.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(3, 0) }); // A: dritto verso est
	Paths.Add({ FRTCellId(2, 0) });                                                    // B: ferma, sulla strada

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths);
	if (!TestEqual(TEXT("un esito per unita'"), R.Num(), 2)) { return false; }

	TestTrue(TEXT("A si ferma davanti a chi occupa la cella"), R[0].Final == FRTCellId(1, 0));
	TestTrue(TEXT("A NON raggiunge la destinazione aggirando l'ostacolo"), !(R[0].Final == FRTCellId(3, 0)));
	TestTrue(TEXT("il motivo registrato e' l'unita' che blocca"), R[0].Outcome == ERTMoveOutcome::BlockedByUnit);
	TestTrue(TEXT("B non si muove"), R[1].Final == FRTCellId(2, 0));

	// Proprieta' generale, non solo questo caso: le celle attraversate sono sempre un PREFISSO del percorso
	// dichiarato. Un ricalcolo produrrebbe celle che nel piano non c'erano.
	bool bPrefix = R[0].Entered.Num() < Paths[0].Num();
	for (int32 i = 0; i < R[0].Entered.Num() && bPrefix; ++i)
	{
		bPrefix = (R[0].Entered[i] == Paths[0][i + 1]); // Entered esclude la cella di partenza
	}
	TestTrue(TEXT("le celle attraversate sono un prefisso del percorso pianificato"), bPrefix);
	return true;
}


/**
 * Il motivo per cui un waypoint viene rifiutato va DETTO, non accorpato. Il messaggio
 * "oltre il budget, bloccata o occupata" mette tre difetti diversi nella stessa frase: chi gioca clicca una
 * cella libera, legge "bloccata" e crede a un difetto del gioco. E' la stessa lezione di
 * Combat.HexTargetingReasonDistinguishesRangeFromCover.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexWaypointReasonTest,
	"RefactorTactics.HexSim.WaypointRejectionSaysWhich",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexWaypointReasonTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSimMap(3);

	// Una cella che blocca il movimento e una occupata da un'altra unita'.
	FRTHexCellData Wall(FRTCellId(1, 0, 0));
	Wall.bBlocksMovement = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();

	const FRTCellId Mine(0, 0, 0);
	const FRTCellId Other(2, 0, 0);
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, {
		FRTHexSimUnit(7, Mine,  /*MoveBudget=*/ 4),
		FRTHexSimUnit(8, Other, /*MoveBudget=*/ 0)
	});

	TestTrue(TEXT("cella fuori dalla mappa -> NotOnMap"),
		URTHexSimLibrary::ClassifyWaypointCell(Snap, 7, FRTCellId(9, 9, 0)) == ERTHexWaypointReason::NotOnMap);
	TestTrue(TEXT("cella che blocca il movimento -> BlocksMovement"),
		URTHexSimLibrary::ClassifyWaypointCell(Snap, 7, FRTCellId(1, 0, 0)) == ERTHexWaypointReason::BlocksMovement);
	TestTrue(TEXT("cella occupata da un'altra unita' -> Occupied"),
		URTHexSimLibrary::ClassifyWaypointCell(Snap, 7, Other) == ERTHexWaypointReason::Occupied);
	TestTrue(TEXT("la propria cella non e' 'occupata'"),
		URTHexSimLibrary::ClassifyWaypointCell(Snap, 7, Mine) == ERTHexWaypointReason::Ok);

	// Cella libera e percorribile: la cella non ha nulla che non va. Se il percorso composito fallisce comunque,
	// il motivo e' il BUDGET — ed e' cosi' che il chiamante distingue i due casi.
	TestTrue(TEXT("cella libera -> Ok (un eventuale rifiuto e' questione di budget)"),
		URTHexSimLibrary::ClassifyWaypointCell(Snap, 7, FRTCellId(0, 2, 0)) == ERTHexWaypointReason::Ok);
	return true;
}

/**
 * CP 4.5 / issue #46 — lo scatto e' un movimento LINEARE lungo una delle sei direzioni (catalogo v0.1 §3.2):
 * non aggira gli ostacoli e non gira gli angoli. Finora usava l'A* sul grafo, quindi faceva entrambe le cose —
 * e in PIE si vedeva un bot "arrampicarsi" sulla piattaforma passando da una transizione.
 *
 * Regola scelta: o si arriva sulla cella richiesta in linea retta, o lo scatto NON avviene. Nessuno scatto a
 * meta' verso una cella che il giocatore non ha scelto (stessa disciplina di BuildCompositeHexPath).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLinearDashTest,
	"RefactorTactics.HexSim.DashIsLinear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLinearDashTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSimMap(3);

	// Ostacolo sulla direzione +q, a un passo dalla partenza.
	FRTHexCellData Wall(FRTCellId(1, 0, 0));
	Wall.bBlocksMovement = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();

	const FRTCellId Start(0, 0, 0);
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, /*Budget=*/ 3) });

	// Lo scatto ha UNA sola implementazione (issue #140): la stessa che esegue la fase Dash.
	auto Dash = [Map](const FRTHexSnapshot& S, const FRTCellId& From, const FRTCellId& Goal, int32 MaxCells)
	{
		return URTMovementActionLibrary::ResolveLinearMove(
			Map, From, Goal, MaxCells, ERTMovementStyle::LinearDash, S.Occupancy, {});
	};

	// 1. In linea retta e libera: si scatta.
	{
		const FRTLinearMoveResult R = Dash(Snap, Start, FRTCellId(2, -2, 0), 3);
		TestTrue(TEXT("linea libera -> arriva sulla cella richiesta"), R.Final == FRTCellId(2, -2, 0));
		TestEqual(TEXT("due passi in linea"), R.Entered.Num(), 2);
		TestTrue(TEXT("esito dichiarato: completato"), R.Stop == ERTLinearStop::Completed);
	}

	// 2. Cella allineata ma con un ostacolo sulla traiettoria: RIFIUTATO (non lo aggira).
	//    Sul grafo sarebbe raggiungibile girandoci attorno: e' il caso che discrimina la regola.
	{
		const FRTCellId Beyond(2, 0, 0);
		TestTrue(TEXT("premessa: sul grafo la cella oltre l'ostacolo sarebbe raggiungibile"),
			URTHexSimLibrary::FindPathForUnit(Snap, 7, Beyond).Status == ERTHexPathStatus::Success);
		const FRTLinearMoveResult R = Dash(Snap, Start, Beyond, 3);
		TestTrue(TEXT("ostacolo sulla linea -> si resta fermi"), R.Final == Start);
		TestTrue(TEXT("motivo dichiarato: terreno"), R.Stop == ERTLinearStop::BlockedByTerrain);
	}

	// 3. Cella NON allineata a nessuna delle sei direzioni: rifiutata, anche se vicina e raggiungibile.
	{
		const FRTCellId Offset(1, 1, 0);
		TestTrue(TEXT("premessa: sul grafo la cella non allineata sarebbe raggiungibile"),
			URTHexSimLibrary::FindPathForUnit(Snap, 7, Offset).Status == ERTHexPathStatus::Success);
		const FRTLinearMoveResult R = Dash(Snap, Start, Offset, 3);
		TestTrue(TEXT("cella non allineata -> nessuno scatto"), R.Final == Start);
		TestTrue(TEXT("motivo dichiarato: non allineata"), R.Stop == ERTLinearStop::NotAligned);
	}

	// 4. Oltre la portata: rifiutato. La portata di una mobilita' LINEARE si misura in CELLE, non in punti
	//    movimento — e' la distinzione del catalogo azioni («non riduce le mobilita' lineari»), ed e' cio' che
	//    la issue #140 ha allineato fra bot e resolver.
	{
		const FRTCellId Far(0, 3, 0);
		TestTrue(TEXT("con portata 3 la cella a 3 celle entra"), Dash(Snap, Start, Far, 3).Final == Far);
		TestTrue(TEXT("con portata 2 la stessa cella e' fuori"), Dash(Snap, Start, Far, 2).Final == Start);
	}

	// 5. Cella occupata da un'altra unita': non ci si puo' fermare sopra.
	{
		const FRTHexSnapshot Two = URTHexSimLibrary::MakeSnapshot(Map, {
			FRTHexSimUnit(7, Start, /*Budget=*/ 3),
			FRTHexSimUnit(8, FRTCellId(0, 2, 0), /*Budget=*/ 0)
		});
		const FRTLinearMoveResult R = Dash(Two, Start, FRTCellId(0, 2, 0), 3);
		TestTrue(TEXT("destinazione occupata -> ci si ferma prima"), R.Final == FRTCellId(0, 1, 0));
		TestTrue(TEXT("motivo dichiarato: unita'"), R.Stop == ERTLinearStop::BlockedByUnit);
	}

	// 6. Un layer diverso non e' mai "in linea": lo scatto non sale per una transizione.
	{
		Map->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 1, 1)));
		Map->AddTransition(FRTCellId(0, 1, 0), FRTCellId(0, 1, 1), /*Cost=*/ 1);
		Map->SortCells();
		const FRTHexSnapshot WithArc = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, 3) });
		TestTrue(TEXT("premessa: sul grafo la transizione porta al layer 1"),
			URTHexSimLibrary::FindPathForUnit(WithArc, 7, FRTCellId(0, 1, 1)).Status == ERTHexPathStatus::Success);
		TestTrue(TEXT("lo scatto non cambia layer"),
			Dash(WithArc, Start, FRTCellId(0, 1, 1), 3).Final == Start);
	}
	return true;
}

/**
 * La prova dell'invariante "chi genera candidate col grafo non deve proporre scatti illegali": su una mappa con
 * ostacoli esistono celle raggiungibili sul GRAFO che NON sono raggiungibili in linea, e il predicato usato per
 * filtrare le candidate del bot le scarta tutte.
 *
 * Questo test e' non-vacuo per costruzione: fallisce sia se il predicato accetta una cella non lineare, sia se lo
 * scenario non contiene piu' celle "solo di grafo" (cioe' se smettesse di discriminare).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexLinearFilterTest,
	"RefactorTactics.HexSim.LinearFilterDropsGraphOnlyCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexLinearFilterTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeSimMap(3);
	for (const FRTCellId& Id : { FRTCellId(1, 0, 0), FRTCellId(1, -1, 0), FRTCellId(0, 1, 0) })
	{
		FRTHexCellData Blocked(Id);
		Blocked.bBlocksMovement = true;
		Map->AddOrUpdateCell(Blocked);
	}
	Map->SortCells();

	const FRTCellId Start(0, 0, 0);
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, /*Budget=*/ 4) });

	auto Reachable = [Map](const FRTHexSnapshot& S, const FRTCellId& From, const FRTCellId& Goal, int32 MaxCells)
	{
		return URTMovementActionLibrary::IsLinearReachable(
			Map, From, Goal, MaxCells, ERTMovementStyle::LinearDash, S.Occupancy, {});
	};

	int32 GraphOnly = 0;
	for (const FRTHexReachableCell& Reach : URTHexSimLibrary::ReachableCells(Snap, 7))
	{
		if (!Reachable(Snap, Start, Reach.Cell, 4))
		{
			++GraphOnly; // raggiungibile camminando, non scattando: una candidata da scartare
			continue;
		}
		// Nessuna asserzione qui: `IsLinearReachable` E' definita come `ResolveLinearMove(...).Final ==
		// Target`, quindi confrontare le due sarebbe `if (!P) continue; assert(P)`. Cio' che il test prova
		// sta sotto: che il filtro scarti davvero qualcosa, e il caso puntuale in fondo.
	}

	// Il caso deve esistere, altrimenti il filtro non sta filtrando nulla e il test non prova niente.
	AddInfo(FString::Printf(TEXT("celle raggiungibili solo sul grafo: %d"), GraphOnly));
	TestTrue(TEXT("lo scenario contiene celle raggiungibili sul grafo ma NON in linea"), GraphOnly > 0);

	// Un caso puntuale, indipendente dall'enumerazione: oltre l'ostacolo in direzione +q. Serve una portata
	// piu' ampia, perche' il giro attorno al gruppo di ostacoli costa 5 passi mentre in linea ne basterebbero 2.
	{
		const FRTCellId Beyond(2, 0, 0);
		const FRTHexSnapshot Wide = URTHexSimLibrary::MakeSnapshot(Map, { FRTHexSimUnit(7, Start, /*Budget=*/ 6) });
		TestTrue(TEXT("premessa: (2,0) e' raggiungibile camminando (aggirando gli ostacoli)"),
			URTHexSimLibrary::FindPathForUnit(Wide, 7, Beyond).Status == ERTHexPathStatus::Success);
		TestFalse(TEXT("(2,0) NON e' raggiungibile scattando (ostacolo sulla linea)"),
			Reachable(Wide, Start, Beyond, 6));
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 14.2 — il resolver a microstep e quello in blocco sono lo stesso codice
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Le configurazioni che hanno gia' un test dedicato in questo file, piu' i casi degeneri. */
	TArray<TArray<TArray<FRTCellId>>> StepperCases()
	{
		TArray<TArray<TArray<FRTCellId>>> Cases;

		// Nessuna unita': il caso che rompe i cicli scritti male.
		Cases.Add({});

		// Una sola unita' che non si muove (path di una cella).
		Cases.Add({ { FRTCellId(0, 0) } });

		// Corsa libera, nessuna interazione.
		Cases.Add({ { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) } });

		// Destinazione contesa: entrambe restano.
		Cases.Add({
			{ FRTCellId(0, 0), FRTCellId(1, 0) },
			{ FRTCellId(2, 0), FRTCellId(1, 0) } });

		// Scambio diretto fra due non-lineari: consentito.
		Cases.Add({
			{ FRTCellId(0, 0), FRTCellId(1, 0) },
			{ FRTCellId(1, 0), FRTCellId(0, 0) } });

		// Coda: chi sta davanti si ferma per contesa, chi segue lo trova fermo al microstep dopo.
		Cases.Add({
			{ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) },
			{ FRTCellId(1, 0), FRTCellId(2, 0) },
			{ FRTCellId(3, 0), FRTCellId(2, 0) } });

		// Percorsi di lunghezza diversa: chi finisce prima diventa un ostacolo fermo per chi continua.
		Cases.Add({
			{ FRTCellId(0, 0), FRTCellId(1, 0) },
			{ FRTCellId(-2, 0), FRTCellId(-1, 0), FRTCellId(0, 0), FRTCellId(1, 0) } });

		return Cases;
	}

	bool SameResults(const TArray<FRTHexMoveResult>& A, const TArray<FRTHexMoveResult>& B)
	{
		if (A.Num() != B.Num()) { return false; }
		for (int32 i = 0; i < A.Num(); ++i)
		{
			if (A[i].Final != B[i].Final || A[i].Outcome != B[i].Outcome) { return false; }
			if (A[i].Entered.Num() != B[i].Entered.Num()) { return false; }
			for (int32 k = 0; k < A[i].Entered.Num(); ++k)
			{
				if (A[i].Entered[k] != B[i].Entered[k]) { return false; }
			}
		}
		return true;
	}
}

/**
 * CP 14.2 — **nessun comportamento cambia**: guidare i microstep a mano produce esattamente cio' che produce
 * `ResolveHexPaths`, che di quei microstep e' il wrapper.
 *
 * Il test ha valore anche sapendo che condividono il codice: e' il gate che cadrebbe il giorno in cui qualcuno
 * ne scrivesse una seconda copia "solo per il caso interattivo" — che e' il modo in cui i due algoritmi di
 * collisione nascono davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimStepperMatchesBatchTest,
	"RefactorTactics.Movement.StepperMatchesBatchResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimStepperMatchesBatchTest::RunTest(const FString&)
{
	const TArray<TArray<TArray<FRTCellId>>> Cases = StepperCases();
	for (int32 c = 0; c < Cases.Num(); ++c)
	{
		const TArray<TArray<FRTCellId>>& Paths = Cases[c];

		const TArray<FRTHexMoveResult> Batch = URTHexSimLibrary::ResolveHexPaths(Paths);

		FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths);
		int32 Guard = 0;
		while (URTHexSimLibrary::ResolveNextHexMicroStep(State) && Guard++ < 64) {}
		TestTrue(FString::Printf(TEXT("caso %d: la risoluzione termina"), c), State.bFinished);

		TestTrue(FString::Printf(TEXT("caso %d: stepper == batch"), c), SameResults(State.Results, Batch));
	}

	// Il caso con priorita' e mobilita' lineari passa dallo stesso confronto: il ramo di CP 4.8 e' il piu'
	// facile da dimenticare in un refactoring del ciclo.
	{
		TArray<TArray<FRTCellId>> Paths;
		Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
		Paths.Add({ FRTCellId(2, 0), FRTCellId(1, 0) });
		const TArray<int32> Priorities = { 50, 10 };
		const TArray<bool> Linear = { false, true };

		const TArray<FRTHexMoveResult> Batch = URTHexSimLibrary::ResolveHexPaths(Paths, Priorities, Linear);

		FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths, Priorities, Linear);
		const TArray<FRTHexMoveResult> Stepped = URTHexSimLibrary::FinishHexMovement(State);

		TestTrue(TEXT("con priorita': stepper == batch"), SameResults(Stepped, Batch));
		TestTrue(TEXT("e la priorita' piu' bassa ha vinto la cella"),
			Batch.Num() == 2 && Batch[1].Final == FRTCellId(1, 0));
	}

	return true;
}

/**
 * CP 14.2 — il punto fisso resta monotono: permutare le richieste non cambia l'esito di nessuna.
 *
 * E' l'invariante #3 letto sul resolver a passi. Un microstep che dipendesse dall'ordine dell'array
 * produrrebbe partite diverse a parita' di piani, e la divergenza si vedrebbe solo nel replay.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimStepperPermutationTest,
	"RefactorTactics.Movement.StepperIsDeterministicUnderPermutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimStepperPermutationTest::RunTest(const FString&)
{
	// Tre unita' che interagiscono: A e C contendono la cella di mezzo, B la attraversa in coda.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) });   // A
	Paths.Add({ FRTCellId(-1, 0), FRTCellId(0, 0), FRTCellId(1, 0) });  // B
	Paths.Add({ FRTCellId(2, 0), FRTCellId(1, 0) });                    // C

	FRTMovementResolutionState Direct = URTHexSimLibrary::BeginHexMovement(Paths);
	const TArray<FRTHexMoveResult> Reference = URTHexSimLibrary::FinishHexMovement(Direct);

	// Le sei permutazioni di tre elementi, riportate all'ordine originale prima del confronto.
	const int32 Perms[6][3] = { {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0} };
	for (int32 p = 0; p < 6; ++p)
	{
		TArray<TArray<FRTCellId>> Shuffled;
		for (int32 k = 0; k < 3; ++k) { Shuffled.Add(Paths[Perms[p][k]]); }

		FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Shuffled);
		const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::FinishHexMovement(State);

		for (int32 k = 0; k < 3; ++k)
		{
			const int32 Original = Perms[p][k];
			TestTrue(FString::Printf(TEXT("perm %d: unita' %d finisce dove deve"), p, Original),
				Out[k].Final == Reference[Original].Final);
			TestTrue(FString::Printf(TEXT("perm %d: unita' %d con lo stesso motivo"), p, Original),
				Out[k].Outcome == Reference[Original].Outcome);
		}
	}

	return true;
}

/**
 * CP 14.2 — lo stato SOPRAVVIVE al chiamante: e' la ragione per cui `FRTMovementResolutionState` copia gli
 * input invece di referenziarli.
 *
 * Senza la copia questo test leggerebbe memoria morta, ed e' esattamente cio' che accadrebbe a una finestra di
 * reazione aperta fra due microstep — il caso d'uso per cui il checkpoint esiste.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSimStepperOwnsInputTest,
	"RefactorTactics.Movement.StepperOwnsItsInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSimStepperOwnsInputTest::RunTest(const FString&)
{
	FRTMovementResolutionState State;
	{
		TArray<TArray<FRTCellId>> Ephemeral;
		Ephemeral.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) });
		State = URTHexSimLibrary::BeginHexMovement(Ephemeral);
		URTHexSimLibrary::ResolveNextHexMicroStep(State);
	}   // gli input escono di scope QUI, a risoluzione iniziata

	const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::FinishHexMovement(State);
	TestTrue(TEXT("il movimento si completa dopo la morte dell'input"),
		Out.Num() == 1 && Out[0].Final == FRTCellId(2, 0));
	TestTrue(TEXT("con l'esito giusto"), Out.Num() == 1 && Out[0].Outcome == ERTMoveOutcome::Moved);
	TestTrue(TEXT("e ha richiesto piu' di un microstep"), State.MicroStepIndex >= 2);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// I due invarianti che il resolver applicava senza che nessun test li nominasse (#2000, D-305)
//
// `spec-tassonomia-movimento.md` §2.0 li dichiara dal 2026-08-31. Fino ad allora erano veri per abitudine
// d'implementazione: `Movement.StepperMatchesBatchResolver` li esercitava senza asserirli, quindi una
// riscrittura del ciclo che li avesse rotti sarebbe rimasta verde finche' i due percorsi restavano d'accordo
// FRA LORO.
// ---------------------------------------------------------------------------------------------------------

/**
 * `MaxGraphTransitionsPerUnitPerMicroStep = 1` — un microstep avanza di UN nodo del percorso, mai due.
 *
 * 🔑 **Il percorso cambia LAYER a meta'**, e non e' un dettaglio decorativo: `(1,0,0)` e `(1,0,1)` **non sono
 * adiacenti sull'esagono** — `FRTCellId::operator==` confronta il `Layer`, e celle su layer diversi si
 * raggiungono solo per arco esplicito. Un ciclo che ragionasse per adiacenza esagonale invece che per nodi
 * del percorso qui sbaglierebbe; uno che "compattasse" i passi non-adiacenti pure. E' la ragione per cui la
 * regola si enuncia su una TRANSIZIONE DEL GRAFO e non su un esagono vicino: cosi' vale gia' per rampe,
 * scale, ponti, tunnel e porte, senza riscritture.
 *
 * ⚠️ **L'asserzione forte e' l'uguaglianza, non una disuguaglianza**: `Pos == Paths[k]` dopo `k` microstep
 * esclude in un colpo solo l'avanzamento doppio e quello nullo. Un `Pos != Paths[k+1]` sarebbe passato anche
 * per un resolver fermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementOneTransitionPerMicroStepTest,
	"RefactorTactics.Movement.OneTransitionMax_PerMicroStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementOneTransitionPerMicroStepTest::RunTest(const FString&)
{
	TArray<TArray<FRTCellId>> Paths;
	// Cinque nodi, quattro archi, e il terzo arco e' un cambio di layer.
	Paths.Add({ FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(1, 0, 1), FRTCellId(2, 0, 1), FRTCellId(3, 0, 1) });
	// Una seconda unita' lontana e piu' lenta: il tetto e' PER UNITA', non una proprieta' del caso a uno.
	Paths.Add({ FRTCellId(0, 5, 0), FRTCellId(1, 5, 0), FRTCellId(2, 5, 0) });

	const int32 ArcsA = Paths[0].Num() - 1;
	const int32 ArcsB = Paths[1].Num() - 1;

	FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths);

	TestTrue(TEXT("le due unita' partono dai propri nodi zero"),
		State.Pos.Num() == 2 && State.Pos[0] == Paths[0][0] && State.Pos[1] == Paths[1][0]);

	for (int32 k = 1; k <= ArcsA; ++k)
	{
		const bool bMoved = URTHexSimLibrary::ResolveNextHexMicroStep(State);
		TestTrue(FString::Printf(TEXT("microstep %d: qualcuno si e' mosso"), k), bMoved);

		// Il cuore del test: dopo k microstep si e' esattamente al k-esimo nodo.
		TestTrue(FString::Printf(TEXT("microstep %d: A e' al nodo %d %s, non oltre"),
				k, k, *Paths[0][k].ToString()),
			State.Pos[0] == Paths[0][k]);
		TestTrue(FString::Printf(TEXT("microstep %d: e il progresso di A vale %d"), k, k),
			State.Prog[0] == k);

		// B ha un percorso piu' corto: avanza finche' ne ha, poi resta ferma. Mai due archi in un colpo.
		const int32 ExpectedB = FMath::Min(k, ArcsB);
		TestTrue(FString::Printf(TEXT("microstep %d: B e' al nodo %d, non oltre"), k, ExpectedB),
			State.Pos[1] == Paths[1][ExpectedB]);
	}

	TestTrue(TEXT("A e' arrivata in fondo"), State.Pos[0] == Paths[0][ArcsA]);

	// Il conto totale chiude il cerchio: quattro archi, quattro microstep che muovono. Se uno solo ne avesse
	// consumati due, saremmo arrivati prima e questo numero sarebbe diverso.
	const bool bMovedAgain = URTHexSimLibrary::ResolveNextHexMicroStep(State);
	TestFalse(TEXT("dopo l'ultimo arco nessun microstep muove piu'"), bMovedAgain);
	TestTrue(TEXT("la risoluzione e' finita"), State.bFinished);

	const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::FinishHexMovement(State);
	TestTrue(TEXT("A ha percorso tutti e quattro gli archi, uno per microstep"),
		Out.Num() == 2 && Out[0].Entered.Num() == ArcsA);
	TestTrue(TEXT("e l'ultimo nodo entrato e' quello su layer 1"),
		Out.Num() == 2 && Out[0].Final == FRTCellId(3, 0, 1));

	return true;
}

/**
 * `auto-reroute: mai` — il resolver percorre il PIANO che ha ricevuto; non ne cerca un altro.
 *
 * La matrice di `spec-tassonomia-movimento.md` §2 porta questa riga su tutte e quattro le famiglie. Il client
 * pianifica, l'autorita' valida e risolve **quel** piano: se una transizione diventa invalida durante la
 * risoluzione, il residuo si interrompe.
 *
 * ⚠️ **Il punto (3) e' cio' che rende il test non vacuo, e va letto prima del punto (4).** Senza dimostrare
 * che una via attorno ESISTE ed e' dentro il budget, «non ha deviato» sarebbe vero anche di una scena in cui
 * deviare era impossibile — e il test passerebbe per assenza di alternative invece che per assenza di
 * reroute. E' lo stesso difetto che `Oracoli che non discriminano` descrive: un'asserzione che non puo'
 * fallire per la ragione che dichiara.
 *
 * 🔎 Nota su cosa NON prova: `ResolveHexPaths` non riceve la mappa, quindi non potrebbe deviare nemmeno
 * volendo — ed e' precisamente la garanzia. Questo test pinna che la separazione resti: il giorno in cui
 * qualcuno passasse lo snapshot al resolver «per gestire meglio i blocchi», il punto (4) diventerebbe rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementBlockedPathNoRerouteTest,
	"RefactorTactics.Movement.BlockedPath_DoesNotAutoReroute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementBlockedPathNoRerouteTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeSimMap(3);

	// (1) In pianificazione la via diretta e' libera: A pianifica (0,0) -> (1,0) -> (2,0).
	TArray<FRTHexSimUnit> AtPlanning;
	AtPlanning.Add(FRTHexSimUnit(1, FRTCellId(0, 0), 6));
	const FRTHexSnapshot Before = URTHexSimLibrary::MakeSnapshot(M, AtPlanning);

	const FRTHexPathResult Planned = URTHexSimLibrary::FindPathForUnit(Before, 1, FRTCellId(2, 0));
	TestTrue(TEXT("(1) il piano esiste"), Planned.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("(1) ed e' la via diretta, costo 2"), Planned.TotalCost, 2);
	TestTrue(TEXT("(1) passando per la cella intermedia"), PathContains(Planned, FRTCellId(1, 0)));

	// (2) Dopo il lock, un'altra unita' occupa la cella intermedia. Il piano di A e' ora invalido a meta'.
	TArray<FRTHexSimUnit> AtResolution = AtPlanning;
	AtResolution.Add(FRTHexSimUnit(2, FRTCellId(1, 0), 0));
	const FRTHexSnapshot After = URTHexSimLibrary::MakeSnapshot(M, AtResolution);

	// (3) LA VIA ATTORNO ESISTE DAVVERO, ed e' dentro il budget di A. Senza questa verifica il punto (4)
	//     non discriminerebbe fra «non ha deviato» e «non poteva deviare».
	const FRTHexPathResult Around = URTHexSimLibrary::FindPathForUnit(After, 1, FRTCellId(2, 0));
	TestTrue(TEXT("(3) una via attorno esiste"), Around.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("(3) costa 3 — una deviazione, non la via diretta"), Around.TotalCost, 3);
	TestFalse(TEXT("(3) e non passa dalla cella occupata"), PathContains(Around, FRTCellId(1, 0)));
	TestTrue(TEXT("(3) ed e' dentro il budget 6 di A"), Around.TotalCost <= 6);

	// (4) Il resolver esegue il PIANO. A si ferma; non prende la via attorno.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add(Planned.Path);              // A: il piano pianificato al punto (1)
	Paths.Add({ FRTCellId(1, 0) });       // B: ferma sulla cella intermedia
	const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::ResolveHexPaths(Paths);

	TestTrue(TEXT("(4) A resta dov'era"), Out.Num() == 2 && Out[0].Final == FRTCellId(0, 0));
	TestTrue(TEXT("(4) A NON raggiunge la destinazione per un'altra via"),
		Out.Num() == 2 && Out[0].Final != FRTCellId(2, 0));
	TestTrue(TEXT("(4) e il motivo e' l'unita' ferma, non la topologia"),
		Out.Num() == 2 && Out[0].Outcome == ERTMoveOutcome::BlockedByUnit);
	TestTrue(TEXT("(4) A non e' entrata in nessuna cella"), Out.Num() == 2 && Out[0].Entered.Num() == 0);

	// (5) Nessuna cella della deviazione e' stata percorsa: e' la forma diretta di «non ha ripianificato».
	for (const FRTCellId& C : Around.Path)
	{
		if (C == FRTCellId(0, 0))
		{
			continue; // la partenza appartiene a entrambe le vie
		}
		TestFalse(FString::Printf(TEXT("(5) A non ha percorso %s della deviazione"), *C.ToString()),
			Out[0].Entered.Contains(C));
	}

	return true;
}



// ---------------------------------------------------------------------------------------------------------
// #2100 — un muro interno divide la cella, e il grafo se ne accorge
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Una mappa piatta col corridoio assiale murato tranne il centro: per passare si DEVE attraversare. */
	URTHexMapAsset* CorridorMap(float HexSize = 150.f)
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 2);
		Map->HexSize = HexSize;
		for (const FRTCellId& Walled : { FRTCellId(0, -2), FRTCellId(0, -1), FRTCellId(0, 1), FRTCellId(0, 2) })
		{
			if (FRTHexCellData* D = const_cast<FRTHexCellData*>(Map->FindCell(Walled)))
			{
				D->bBlocksMovement = true;
			}
		}
		return Map;
	}

	/** Il diametro fra due punti medi opposti dentro `Cell`: il muro che divide la cella in due. */
	void AddDiameter(URTHexMapAsset* Map, const FRTCellId& Cell, ERTTacticalAxis Axis)
	{
		FRTGeometrySegment Wall;
		Wall.Axis = Axis;
		Wall.Offset = 0;
		Wall.AlongStart = -RT_GeometryQuanta;
		Wall.AlongEnd = RT_GeometryQuanta;
		Map->InteriorWalls.Add(FRTHexInteriorWall(Cell, Wall));
	}
}

/**
 * UN MURO CONTINUO FERMA CHI VUOLE ATTRAVERSARE LA CELLA.
 *
 * 🔴 **La regola che esisteva e non applicava nessuno** (#2100). `spec-cover-placement-intra-hex.md` §6
 * dice che *«stesso `CellId` non significa passaggio libero»*, e `ERTIntraCellTraversal` lo sapeva
 * rispondere da tredici test — con **zero** chiamanti di produzione. Il difetto non si vedeva perche' il
 * bake produceva coperture di bordo al posto dei muri interni, e *quelle* fermavano il passo: per la
 * ragione sbagliata (#2085).
 *
 * ⚠️ **Il corridoio non e' decorazione.** Senza le celle murate ai lati, l'unita' aggirerebbe e il test
 * sarebbe verde anche con la regola spenta: proverebbe l'esistenza di una strada, non il divieto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPathContinuousWallBlocksCrossingTest,
	"RefactorTactics.Path.ContinuousWallBlocksTheCrossing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPathContinuousWallBlocksCrossingTest::RunTest(const FString&)
{
	// CONTROPROVA PRIMA: senza il muro, il corridoio si percorre. Se questa fallisse, il test che segue
	// sarebbe verde per la ragione sbagliata.
	{
		URTHexMapAsset* Clean = CorridorMap();
		const FRTHexPathResult Through =
			URTHexPathLibrary::FindPath(Clean, FRTCellId(-1, 0), FRTCellId(1, 0));
		TestTrue(TEXT("senza muro il corridoio si attraversa"),
			Through.Status == ERTHexPathStatus::Success);
		TestTrue(TEXT("e passa per il centro"), Through.Path.Contains(FRTCellId(0, 0)));
	}

	// Il muro: un diametro su `Deg90`, perpendicolare all'asse del corridoio.
	URTHexMapAsset* Map = CorridorMap();
	AddDiameter(Map, FRTCellId(0, 0), ERTTacticalAxis::Deg90);

	const FRTHexPathResult Blocked =
		URTHexPathLibrary::FindPath(Map, FRTCellId(-1, 0), FRTCellId(1, 0));

	TestTrue(TEXT("il muro continuo nega la traversata"), Blocked.Status != ERTHexPathStatus::Success);

	// E la primitiva lo dice direttamente, senza passare dal pathfinding.
	TestFalse(TEXT("CanTransitCell rifiuta le due sponde"),
		URTHexPathLibrary::CanTransitCell(Map, FRTCellId(0, 0), ERTHexDirection::W, ERTHexDirection::E));

	return true;
}

/**
 * MA CI SI PUO' ANCORA ENTRARE, e muoversi dentro la stessa regione.
 *
 * 🔑 **E' la meta' che impedisce la correzione grossolana.** Rendere la cella semplicemente
 * impraticabile passerebbe il test qui sopra e sarebbe sbagliato: il muro divide, non chiude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPathSameRegionStepStillAllowedTest,
	"RefactorTactics.Path.SameRegionStepIsStillAllowed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPathSameRegionStepStillAllowedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = CorridorMap();
	AddDiameter(Map, FRTCellId(0, 0), ERTTacticalAxis::Deg90);

	// Entrare nella cella divisa resta lecito: la meta e' la cella, e una posa c'e'.
	const FRTHexPathResult Into = URTHexPathLibrary::FindPath(Map, FRTCellId(-1, 0), FRTCellId(0, 0));
	TestTrue(TEXT("nella cella divisa ci si entra"), Into.Status == ERTHexPathStatus::Success);

	// E due lati dalla STESSA parte del muro si parlano: non e' un divieto globale.
	TestTrue(TEXT("due lati della stessa regione restano collegati"),
		URTHexPathLibrary::CanTransitCell(Map, FRTCellId(0, 0), ERTHexDirection::W, ERTHexDirection::NW)
		|| URTHexPathLibrary::CanTransitCell(Map, FRTCellId(0, 0), ERTHexDirection::W, ERTHexDirection::SW));

	// CONTROPROVA sul sentinella: una cella SENZA geometria non distingue i lati, mai.
	TestTrue(TEXT("una cella pulita si attraversa da qualunque lato"),
		URTHexPathLibrary::CanTransitCell(Map, FRTCellId(1, 0), ERTHexDirection::W, ERTHexDirection::E));
	TestFalse(TEXT("e non porta geometria interna"),
		URTHexPathLibrary::CellHasInteriorGeometry(Map, FRTCellId(1, 0)));

	return true;
}

namespace
{
	/** Il piano di un'unita': quante celle ha chiesto il giocatore, e se il terreno voleva portarla oltre. */
	FRTPlannedMovement PianoDelGiocatore(int32 PlannedLength, bool bSlideRequested)
	{
		FRTPlannedMovement Plan;
		Plan.PlannedLength = PlannedLength;
		Plan.bSlideRequested = bSlideRequested;
		return Plan;
	}

	/** Risolve in blocco e restituisce l'esito dell'unita' indicata. */
	ERTMoveOutcome EsitoConPiano(const TArray<TArray<FRTCellId>>& Paths,
		const TArray<FRTPlannedMovement>& Planned, int32 UnitId)
	{
		FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths, TArray<int32>(),
			TArray<bool>(), TArray<bool>(), Planned);
		const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::FinishHexMovement(State);
		return Out.IsValidIndex(UnitId) ? Out[UnitId].Outcome : ERTMoveOutcome::Stayed;
	}
}

/**
 * CHI ARRIVA A DESTINAZIONE MA NON RIESCE A SCIVOLARE NON E' «FERMATO» — `#2314`.
 *
 * 🔴 **Il difetto era una causa scritta, precisa e falsa.** Lo scivolamento allunga il percorso PRIMA del
 * resolver, che considera l'ultima cella la destinazione: se quella cella e' impedita, l'unita' — arrivata
 * **esattamente dove il giocatore l'aveva mandata** — compariva nel replay come *«fermo: cella occupata»*,
 * con un `MoveBlocked`/`Important` nel feed. Il piano aveva funzionato, e il gioco diceva di no.
 *
 * ⚠️ **La contro-prova senza `Planned` non e' decorativa**: e' cio' che rende il test non vacuo. Lo stesso
 * identico stato, senza dichiarare quanto fosse pianificato, deve ancora produrre `BlockedByUnit` — cioe'
 * il comportamento di prima. Se passasse comunque `SlideBlocked`, il test starebbe misurando una costante.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementSlideBlockedByUnitTest,
	"RefactorTactics.Movement.SlideBlockedByUnitKeepsThePlannedArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementSlideBlockedByUnitTest::RunTest(const FString&)
{
	// L'unita' 0 ha chiesto (0,0) -> (1,0): DUE celle. Il ghiaccio ha aggiunto (2,0), dove sta ferma
	// l'unita' 1. Il percorso che entra nel resolver ne ha quindi tre, e solo due sono del giocatore.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) });
	Paths.Add({ FRTCellId(2, 0) }); // ferma: e' lei a impedire lo scivolamento

	const TArray<FRTPlannedMovement> Planned = {
		PianoDelGiocatore(/*PlannedLength*/ 2, /*bSlideRequested*/ true),
		PianoDelGiocatore(1, false)
	};

	TestEqual(TEXT("arrivata dove voleva, scivolamento impedito: SlideBlocked"),
		static_cast<int32>(EsitoConPiano(Paths, Planned, 0)),
		static_cast<int32>(ERTMoveOutcome::SlideBlocked));

	// E ci arriva davvero: senza questa riga «SlideBlocked» potrebbe essere scritto a un'unita' rimasta al palo.
	{
		FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths, TArray<int32>(),
			TArray<bool>(), TArray<bool>(), Planned);
		const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::FinishHexMovement(State);
		TestTrue(TEXT("e la destinazione pianificata e' stata raggiunta"),
			Out.IsValidIndex(0) && Out[0].Final == FRTCellId(1, 0));
	}

	// CONTRO-PROVA: lo stesso stato senza il piano dichiarato torna al comportamento di prima di `#2314`.
	TestEqual(TEXT("senza `Planned` l'esito resta quello di prima: BlockedByUnit"),
		static_cast<int32>(EsitoConPiano(Paths, TArray<FRTPlannedMovement>(), 0)),
		static_cast<int32>(ERTMoveOutcome::BlockedByUnit));

	return true;
}

/**
 * CHI SI FERMA PRIMA DELLA DESTINAZIONE PIANIFICATA CONSERVA IL SUO MOTIVO VERO — `#2314`.
 *
 * ⚠️ **E' la precedenza piu' importante del contratto.** `SlideBlocked` dice «il tuo piano ha funzionato»:
 * scriverlo a chi si e' fermata a meta' strada sarebbe la stessa bugia di prima, rovesciata. Il motivo
 * dell'arresto e' cio' che il giocatore ha VISTO, e resta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementStoppedBeforePlannedKeepsReasonTest,
	"RefactorTactics.Movement.StoppedBeforePlannedDestinationKeepsItsReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementStoppedBeforePlannedKeepsReasonTest::RunTest(const FString&)
{
	// Piano di TRE celle, piu' una di scivolamento. L'unita' 1 e' ferma sulla seconda tappa del piano:
	// l'unita' 0 si ferma a un terzo, ben prima di poter parlare di scivolamento.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(3, 0) });
	Paths.Add({ FRTCellId(2, 0) });

	const TArray<FRTPlannedMovement> Planned = {
		PianoDelGiocatore(/*PlannedLength*/ 3, /*bSlideRequested*/ true),
		PianoDelGiocatore(1, false)
	};

	TestEqual(TEXT("fermata prima della destinazione pianificata: resta BlockedByUnit"),
		static_cast<int32>(EsitoConPiano(Paths, Planned, 0)),
		static_cast<int32>(ERTMoveOutcome::BlockedByUnit));
	return true;
}

/**
 * UNO SCIVOLAMENTO AVVENUTO ANCHE SOLO IN PARTE E' `Slid`, NON `SlideBlocked` — `#2314`.
 *
 * 🔑 **Il confine ha conseguenze a valle.** `D-319` lega `Status.Unbalanced` all'essere stati spostati
 * dall'ambiente: classificare come «non scivolata» un'unita' portata via di almeno una cella le toglierebbe
 * lo stato, e con esso il seguito che quella decisione descrive.
 *
 * ⚠️ **Il caso E' raggiungibile in partita, e lo e' diventato mentre questa PR era aperta.** Quando fu
 * scritto non lo era — `ApplyIceSliding` allungava di UNA cella sola — ma `#2253` ha reso `SlideCells` un
 * contatore e [D-319] gli somma `FRTHexSimUnit::ExtraSlideCells`, che vale `1` per chi e' gia'
 * `Status.Unbalanced`: uno sbilanciato che scivola di due celle e trova la seconda occupata percorre la
 * prima e si ferma. Questo test misura comunque il CONTRATTO del resolver — costruisce il piano a mano,
 * quindi non dipende da quei valori — e il flusso di produzione e' coperto da
 * `Terrain.Ice.SlideBlockedInMatch`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementPartialSlideIsStillASlideTest,
	"RefactorTactics.Movement.PartialSlideIsStillASlide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementPartialSlideIsStillASlideTest::RunTest(const FString&)
{
	// Piano di DUE celle, estensione ambientale di DUE. L'unita' 1 e' ferma sulla seconda cella di
	// scivolamento: la prima viene percorsa, la seconda no.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(3, 0) });
	Paths.Add({ FRTCellId(3, 0) });

	const TArray<FRTPlannedMovement> Planned = {
		PianoDelGiocatore(/*PlannedLength*/ 2, /*bSlideRequested*/ true),
		PianoDelGiocatore(1, false)
	};

	FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths, TArray<int32>(),
		TArray<bool>(), TArray<bool>(), Planned);
	const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::FinishHexMovement(State);

	// ANTI-VACUITA': se l'unita' non avesse percorso NESSUNA cella di scivolamento, `Slid` sarebbe l'esito
	// giusto per la ragione sbagliata. Il progresso si misura, non si assume.
	if (!TestTrue(TEXT("premessa: una cella di scivolamento e' stata davvero percorsa"),
		Out.IsValidIndex(0) && Out[0].Final == FRTCellId(2, 0)))
	{
		return false;
	}
	TestEqual(TEXT("scivolata di una cella e poi fermata: e' comunque uno scivolamento"),
		static_cast<int32>(Out[0].Outcome), static_cast<int32>(ERTMoveOutcome::Slid));
	return true;
}

/**
 * UN PERCORSO CHE RIVISITA UNA CELLA NON INGANNA IL RESOLVER — `#2314`.
 *
 * 🔴 **E' il caso che ha affossato l'approccio di `#2290`, e viveva anche QUI.** La classificazione
 * confrontava la cella FINALE con l'ultima del percorso; ma `BuildCompositeHexPath` concatena i segmenti A*
 * fra waypoint **senza deduplicare**, quindi `{A, B, C, B}` e' un percorso producibile. Un'unita' fermata a
 * un terzo di strada, ferma su `B`, soddisfaceva l'uguaglianza e veniva registrata come **arrivata** — con
 * `Moved` prima di `#2314`, e con `SlideBlocked` se qualcuno reintroducesse quel criterio dopo.
 *
 * 🔑 **Posizione finale e quantita' di percorso eseguito sono concetti diversi**, e questo test e' il punto
 * in cui la differenza si misura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementRevisitedCellIsNotArrivalTest,
	"RefactorTactics.Movement.RevisitedCellIsNotProofOfArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementRevisitedCellIsNotArrivalTest::RunTest(const FString&)
{
	// `{A, B, C, B}`: il piano torna sui propri passi, e la destinazione COINCIDE con la prima tappa.
	// L'unita' 1 e' ferma su `C`, quindi l'unita' 0 si ferma su `B` dopo un solo passo — nella cella che e'
	// anche la sua destinazione pianificata.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(2, 0) });

	// Il terreno chiedeva uno scivolamento dalla destinazione pianificata: cosi' un criterio posizionale non
	// sbaglierebbe di poco — scriverebbe proprio `SlideBlocked`, l'esito che questa issue introduce.
	const TArray<FRTPlannedMovement> Planned = {
		PianoDelGiocatore(/*PlannedLength*/ 4, /*bSlideRequested*/ true),
		PianoDelGiocatore(1, false)
	};

	FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths, TArray<int32>(),
		TArray<bool>(), TArray<bool>(), Planned);
	const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::FinishHexMovement(State);

	// La premessa E' il test: senza la coincidenza fra cella finale e destinazione pianificata, un criterio
	// posizionale non verrebbe nemmeno esercitato e l'asserzione sotto passerebbe per caso.
	if (!TestTrue(TEXT("premessa: si ferma nella cella che e' anche la destinazione pianificata"),
		Out.IsValidIndex(0) && Out[0].Final == FRTCellId(1, 0) && Out[0].Final == Paths[0].Last()))
	{
		return false;
	}
	TestEqual(TEXT("un terzo di percorso non e' un arrivo: resta BlockedByUnit"),
		static_cast<int32>(Out[0].Outcome), static_cast<int32>(ERTMoveOutcome::BlockedByUnit));
	return true;
}

/**
 * IL PIANO PER-UNITA' NON ROMPE L'INDIPENDENZA DALL'ORDINE — `#2314`.
 *
 * Il contratto di `FinalizeHexMovementOutcomes` e' dichiarato: *«dipende solo dai dati per-unita' dello
 * stato, quindi e' indipendente dall'ordine»*. `FRTPlannedMovement` e' un dato dello stato quanto `Paths`,
 * quindi non lo viola — **ma va verificato, non assunto**: e' precisamente il genere di aggiunta che
 * introdurrebbe una dipendenza dall'ordine se qualcuno la tenesse in una variabile globale del resolver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementPlannedLengthIsOrderIndependentTest,
	"RefactorTactics.Movement.PlannedLengthOutcomesAreOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementPlannedLengthIsOrderIndependentTest::RunTest(const FString&)
{
	// Tre unita' con tre piani diversi: una scivola e viene impedita, una si ferma prima, una sta ferma e
	// impedisce entrambe. Gli esiti sono i tre rami della classificazione, non uno solo.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) });                  // 0: arriva, non scivola
	Paths.Add({ FRTCellId(0, -1), FRTCellId(1, -1), FRTCellId(1, 0), FRTCellId(2, 0) }); // 1: si ferma prima
	Paths.Add({ FRTCellId(2, 0) });                                                    // 2: ferma

	TArray<FRTPlannedMovement> Planned;
	Planned.Add(PianoDelGiocatore(2, true));
	Planned.Add(PianoDelGiocatore(3, true));
	Planned.Add(PianoDelGiocatore(1, false));

	TArray<ERTMoveOutcome> Reference;
	for (int32 i = 0; i < 3; ++i) { Reference.Add(EsitoConPiano(Paths, Planned, i)); }

	// ANTI-VACUITA': se tutti e tre gli esiti fossero uguali, qualunque permutazione li conserverebbe.
	if (!TestTrue(TEXT("premessa: i tre esiti sono distinti"),
		Reference[0] != Reference[1] && Reference[1] != Reference[2] && Reference[0] != Reference[2]))
	{
		return false;
	}
	TestEqual(TEXT("premessa: il primo e' proprio SlideBlocked"),
		static_cast<int32>(Reference[0]), static_cast<int32>(ERTMoveOutcome::SlideBlocked));

	const int32 Perms[6][3] = { {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0} };
	for (int32 p = 0; p < 6; ++p)
	{
		// `Paths` e `Planned` si permutano INSIEME: l'indice e' l'identita' dell'unita' per tutta la
		// risoluzione, e disaccoppiarli misurerebbe un altro difetto.
		TArray<TArray<FRTCellId>> ShuffledPaths;
		TArray<FRTPlannedMovement> ShuffledPlanned;
		for (int32 k = 0; k < 3; ++k)
		{
			ShuffledPaths.Add(Paths[Perms[p][k]]);
			ShuffledPlanned.Add(Planned[Perms[p][k]]);
		}

		for (int32 k = 0; k < 3; ++k)
		{
			TestEqual(FString::Printf(TEXT("perm %d: l'unita' %d ha lo stesso esito"), p, Perms[p][k]),
				static_cast<int32>(EsitoConPiano(ShuffledPaths, ShuffledPlanned, k)),
				static_cast<int32>(Reference[Perms[p][k]]));
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
