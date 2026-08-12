#include "Misc/AutomationTest.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Maschera con i primi `N` settori occupati. Quali siano non conta per la classificazione: conta quanti. */
	FRTOccupancyMask MaskWithSectors(int32 N, bool bCore = false)
	{
		FRTOccupancyMask M;
		M.Sectors = (N >= RT_OccupancySectorCount) ? 0xFFF : ((1 << N) - 1);
		M.bCoreBlocked = bCore;
		return M;
	}
}

/**
 * I CONFINI della classificazione, che sono l'unica parte falsificabile della tabella di soglia:
 * `0-3 Free`, `4-5 Constrained`, `6+ Blocked`. Un test che provasse 0 e 12 passerebbe con qualunque
 * soglia fra 1 e 11.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyThresholdBoundariesTest,
	"RefactorTactics.HexOccupancy.ThresholdBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyThresholdBoundariesTest::RunTest(const FString&)
{
	const FRTOccupancyThresholds T; // 4 / 6, la baseline

	TestTrue(TEXT("3 settori -> Free"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(3), T) == ERTCellOccupancy::Free);
	TestTrue(TEXT("4 settori -> Constrained (confine 3/4)"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(4), T) == ERTCellOccupancy::Constrained);
	TestTrue(TEXT("5 settori -> Constrained"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(5), T) == ERTCellOccupancy::Constrained);
	TestTrue(TEXT("6 settori -> Blocked (confine 5/6)"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(6), T) == ERTCellOccupancy::Blocked);

	// Estremi, per completezza: non sono i confini ma dicono che la funzione non e' costante.
	TestTrue(TEXT("0 settori -> Free"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(0), T) == ERTCellOccupancy::Free);
	TestTrue(TEXT("12 settori -> Blocked"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(12), T) == ERTCellOccupancy::Blocked);

	TestEqual(TEXT("il conteggio legge la maschera"), URTHexOccupancyLibrary::NumOccupiedSectors(MaskWithSectors(5)), 5);
	return true;
}

/**
 * Il centro occupato vince sul conteggio. Il caso che conta e' `0` settori e core occupato — un footprint
 * solido piu' grande della cella, i cui bordi cadono tutti fuori: senza questa regola sarebbe `Free`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyCoreBlockedTest,
	"RefactorTactics.HexOccupancy.CoreBlockedWinsOverSectorCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyCoreBlockedTest::RunTest(const FString&)
{
	const FRTOccupancyThresholds T;

	TestTrue(TEXT("core occupato, ZERO settori -> Blocked"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(0, /*bCore*/ true), T) == ERTCellOccupancy::Blocked);
	TestTrue(TEXT("core occupato, 3 settori (sarebbe Free) -> Blocked"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(3, /*bCore*/ true), T) == ERTCellOccupancy::Blocked);
	TestTrue(TEXT("core occupato, 4 settori (sarebbe Constrained) -> Blocked"),
		URTHexOccupancyLibrary::Classify(MaskWithSectors(4, /*bCore*/ true), T) == ERTCellOccupancy::Blocked);
	return true;
}

/**
 * La soglia e' davvero LETTA, non ricopiata in una costante. Senza questo test, `Classify` potrebbe ignorare
 * il parametro e la tabella di soglia sarebbe documentazione, non codice.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyThresholdsAreReadTest,
	"RefactorTactics.HexOccupancy.ChangingAThresholdChangesTheClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyThresholdsAreReadTest::RunTest(const FString&)
{
	const FRTOccupancyMask Fixture = MaskWithSectors(4);

	FRTOccupancyThresholds Baseline; // 4 / 6
	TestTrue(TEXT("con la baseline la fixture e' Constrained"),
		URTHexOccupancyLibrary::Classify(Fixture, Baseline) == ERTCellOccupancy::Constrained);

	FRTOccupancyThresholds Stricter;
	Stricter.ConstrainedFrom = 2;
	Stricter.BlockedFrom = 4;
	TestTrue(TEXT("alzando la severita' la STESSA fixture diventa Blocked"),
		URTHexOccupancyLibrary::Classify(Fixture, Stricter) == ERTCellOccupancy::Blocked);

	FRTOccupancyThresholds Looser;
	Looser.ConstrainedFrom = 5;
	Looser.BlockedFrom = 9;
	TestTrue(TEXT("abbassandola la STESSA fixture diventa Free"),
		URTHexOccupancyLibrary::Classify(Fixture, Looser) == ERTCellOccupancy::Free);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// La maschera dalla geometria.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	constexpr float TestHexSize = 100.f;

	/** Punto in coordinate locali di cella, dato un angolo in gradi e una frazione del raggio. */
	FVector2D PointAt(double AngleDegrees, double RadiusFraction)
	{
		const double Rad = FMath::DegreesToRadians(AngleDegrees);
		const double R = TestHexSize * RadiusFraction;
		return FVector2D(R * FMath::Cos(Rad), R * FMath::Sin(Rad));
	}

	FRTOccupancyPolyline OpenLine(const TArray<FVector2D>& Pts)
	{
		FRTOccupancyPolyline L;
		L.Points = Pts;
		L.bClosed = false;
		return L;
	}

	/** Quadrato chiuso centrato su `Centre`, mezzo-lato `Half`. */
	FRTOccupancyPolyline ClosedSquare(const FVector2D& Centre, double Half)
	{
		FRTOccupancyPolyline L;
		L.Points = {
			FVector2D(Centre.X - Half, Centre.Y - Half),
			FVector2D(Centre.X + Half, Centre.Y - Half),
			FVector2D(Centre.X + Half, Centre.Y + Half),
			FVector2D(Centre.X - Half, Centre.Y + Half)
		};
		L.bClosed = true;
		return L;
	}
}

/**
 * L'ancoraggio del settore 0. Senza questo test «deterministica» sarebbe vera per costruzione: due
 * implementazioni entrambe corrette ma ancorate diversamente darebbero maschere diverse dalla stessa
 * geometria, e la maschera e' un intero che prima o poi finisce in un hash.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancySectorAnchorTest,
	"RefactorTactics.HexOccupancy.SectorZeroIsAnchoredToTheFirstVertex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancySectorAnchorTest::RunTest(const FString&)
{
	TArray<FVector2D> P;
	URTHexOccupancyLibrary::SectorBoundaryPoints(TestHexSize, P);

	TestEqual(TEXT("dodici punti di confine"), P.Num(), RT_OccupancySectorCount);
	if (P.Num() != RT_OccupancySectorCount)
	{
		return false;
	}

	// P[0] = primo VERTICE, a -30 gradi e a raggio pieno: lo stesso da cui URTHexLibrary costruisce il perimetro.
	const FVector2D ExpectedV0 = PointAt(-30.0, 1.0);
	TestTrue(TEXT("P[0] e' il primo vertice (-30 gradi, raggio pieno)"), (P[0] - ExpectedV0).Size() < 0.01);

	// P[1] = punto medio del lato E, a 0 gradi e a raggio ridotto di cos(30).
	const FVector2D ExpectedM0 = PointAt(0.0, FMath::Cos(FMath::DegreesToRadians(30.0)));
	TestTrue(TEXT("P[1] e' il punto medio del lato E (0 gradi)"), (P[1] - ExpectedM0).Size() < 0.01);

	// P[2] = secondo vertice, a +30: ne segue che la direzione E copre esattamente i settori 0 e 1.
	const FVector2D ExpectedV1 = PointAt(30.0, 1.0);
	TestTrue(TEXT("P[2] e' il secondo vertice (+30 gradi)"), (P[2] - ExpectedV1).Size() < 0.01);
	return true;
}

/** Un segmento interamente dentro un settore accende quel settore e nessun altro. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancySingleSegmentTest,
	"RefactorTactics.HexOccupancy.SingleSegmentOccupiesOnlyItsSector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancySingleSegmentTest::RunTest(const FString&)
{
	// Il settore 0 copre gli angoli [-30, 0). Entrambi gli estremi ci stanno dentro con margine.
	const TArray<FRTOccupancyPolyline> Geometry = {
		OpenLine({ PointAt(-20.0, 0.3), PointAt(-10.0, 0.6) })
	};

	const FRTOccupancyMask M = URTHexOccupancyLibrary::ComputeMask(Geometry, TestHexSize);
	TestEqual(TEXT("solo il settore 0"), M.Sectors, 1 << 0);
	TestEqual(TEXT("un settore occupato"), URTHexOccupancyLibrary::NumOccupiedSectors(M), 1);
	TestFalse(TEXT("il centro resta libero"), M.bCoreBlocked);
	return true;
}

/** L'ORDINE dell'ingresso non cambia l'uscita: e' la voce di DoD sulla determinismo, e non si deduce. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyOrderIndependenceTest,
	"RefactorTactics.HexOccupancy.MaskIsIndependentOfInputOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyOrderIndependenceTest::RunTest(const FString&)
{
	const FRTOccupancyPolyline A = OpenLine({ PointAt(-20.0, 0.3), PointAt(-10.0, 0.6) });
	const FRTOccupancyPolyline B = OpenLine({ PointAt(100.0, 0.3), PointAt(110.0, 0.6) });
	const FRTOccupancyPolyline C = OpenLine({ PointAt(200.0, 0.3), PointAt(210.0, 0.6) });

	const FRTOccupancyMask Forward = URTHexOccupancyLibrary::ComputeMask({ A, B, C }, TestHexSize);
	const FRTOccupancyMask Shuffled = URTHexOccupancyLibrary::ComputeMask({ C, A, B }, TestHexSize);
	const FRTOccupancyMask Reversed = URTHexOccupancyLibrary::ComputeMask({ C, B, A }, TestHexSize);

	TestEqual(TEXT("stessa maschera con ordine mescolato"), Shuffled.Sectors, Forward.Sectors);
	TestEqual(TEXT("stessa maschera con ordine invertito"), Reversed.Sectors, Forward.Sectors);
	TestTrue(TEXT("e la maschera non e' vuota, altrimenti il test non direbbe nulla"), Forward.Sectors != 0);
	return true;
}

/** Fixture «angolo»: una polilinea di tre punti accende tutti i settori che attraversa, non solo gli estremi. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyCornerTest,
	"RefactorTactics.HexOccupancy.CornerSpansEverySectorItCrosses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyCornerTest::RunTest(const FString&)
{
	// -20 sta nel settore 0 ([-30,0)), 10 nel settore 1 ([0,30)), 40 nel settore 2 ([30,60)).
	const TArray<FRTOccupancyPolyline> Geometry = {
		OpenLine({ PointAt(-20.0, 0.5), PointAt(10.0, 0.5), PointAt(40.0, 0.5) })
	};

	const FRTOccupancyMask M = URTHexOccupancyLibrary::ComputeMask(Geometry, TestHexSize);
	TestEqual(TEXT("settori 0, 1 e 2"), M.Sectors, (1 << 0) | (1 << 1) | (1 << 2));
	TestFalse(TEXT("il centro resta libero"), M.bCoreBlocked);
	return true;
}

/** Fixture «footprint solido»: un contorno chiuso che contiene il centro lo blocca. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancySolidFootprintTest,
	"RefactorTactics.HexOccupancy.SolidFootprintAroundTheCentreBlocksTheCore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancySolidFootprintTest::RunTest(const FString&)
{
	const TArray<FRTOccupancyPolyline> Geometry = { ClosedSquare(FVector2D::ZeroVector, 20.0) };

	const FRTOccupancyMask M = URTHexOccupancyLibrary::ComputeMask(Geometry, TestHexSize);
	TestTrue(TEXT("il centro e' bloccato"), M.bCoreBlocked);
	TestEqual(TEXT("un contorno attorno al centro attraversa tutti i settori"), M.Sectors, 0xFFF);

	// E la classificazione ne segue, che e' il punto: `CoreBlocked` non e' un'informazione che muore qui.
	TestTrue(TEXT("classificata Blocked"),
		URTHexOccupancyLibrary::Classify(M, FRTOccupancyThresholds()) == ERTCellOccupancy::Blocked);
	return true;
}

/**
 * Fixture «footprint void»: un contorno chiuso che NON contiene il centro occupa i suoi settori e lascia il
 * centro libero. E' il gemello di controllo del test sopra — senza, «il centro e' bloccato» passerebbe anche
 * con un'implementazione che lo mette sempre a vero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyVoidFootprintTest,
	"RefactorTactics.HexOccupancy.VoidFootprintLeavesTheCoreFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyVoidFootprintTest::RunTest(const FString&)
{
	const TArray<FRTOccupancyPolyline> Geometry = { ClosedSquare(PointAt(-15.0, 0.6), 5.0) };

	const FRTOccupancyMask M = URTHexOccupancyLibrary::ComputeMask(Geometry, TestHexSize);
	TestFalse(TEXT("il centro NON e' bloccato"), M.bCoreBlocked);
	TestEqual(TEXT("occupa il solo settore 0"), M.Sectors, 1 << 0);
	return true;
}

/**
 * L'esempio canonico di §14 del sorgente, alla lettera. Vale come test perche' pinna insieme tre cose che
 * altrimenti driftano: la larghezza della maschera, il conteggio, e la classificazione che ne esce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyGoldenExampleTest,
	"RefactorTactics.HexOccupancy.GoldenExampleFromTheSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyGoldenExampleTest::RunTest(const FString&)
{
	// Occupied Sectors: 5 / 12 · Mask: 001111000100 · Core: Free · Classification: Constrained
	FRTOccupancyMask M;
	M.Sectors = 0b001111000100;
	M.bCoreBlocked = false;

	TestEqual(TEXT("la maschera dell'esempio vale 964"), M.Sectors, 964);
	TestEqual(TEXT("cinque settori su dodici"), URTHexOccupancyLibrary::NumOccupiedSectors(M), 5);
	TestTrue(TEXT("classificata Constrained"),
		URTHexOccupancyLibrary::Classify(M, FRTOccupancyThresholds()) == ERTCellOccupancy::Constrained);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Il CONSUMATORE: senza, `Constrained` sarebbe indistinguibile da `Free` per chiunque legga.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Mappa piena di raggio N, tutte le celle a costo 1 e senza sovrapprezzo. */
	URTHexMapAsset* MakeFlatMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	void SetSurcharge(URTHexMapAsset* M, const FRTCellId& Id, int32 Surcharge)
	{
		if (const FRTHexCellData* Existing = M->FindCell(Id))
		{
			FRTHexCellData Updated = *Existing;
			Updated.OccupancySurcharge = Surcharge;
			M->AddOrUpdateCell(Updated);
		}
	}
}

/** Il sovrapprezzo si LEGGE dalle soglie, e `Blocked` non ne paga uno: non si attraversa affatto. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancySurchargeTest,
	"RefactorTactics.HexOccupancy.SurchargeIsReadFromTheThresholds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancySurchargeTest::RunTest(const FString&)
{
	FRTOccupancyThresholds T;
	TestEqual(TEXT("Free non paga nulla"), URTHexOccupancyLibrary::Surcharge(ERTCellOccupancy::Free, T), 0);
	TestEqual(TEXT("Constrained paga il sovrapprezzo di catalogo"),
		URTHexOccupancyLibrary::Surcharge(ERTCellOccupancy::Constrained, T), 1);
	TestEqual(TEXT("Blocked non paga: non si attraversa"),
		URTHexOccupancyLibrary::Surcharge(ERTCellOccupancy::Blocked, T), 0);

	T.ConstrainedSurcharge = 4;
	TestEqual(TEXT("cambiare il catalogo cambia il sovrapprezzo"),
		URTHexOccupancyLibrary::Surcharge(ERTCellOccupancy::Constrained, T), 4);
	return true;
}

/**
 * L'EFFETTO OSSERVABILE su `FindPath`. Il sovrapprezzo sta sulla cella di DESTINAZIONE, cosi' ogni percorso lo
 * paga: se stesse su una cella intermedia, un costo piu' alto sarebbe indistinguibile da una deviazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyPathCostTest,
	"RefactorTactics.HexOccupancy.ConstrainedCellCostsMoreInFindPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyPathCostTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeFlatMap(3);
	const FRTCellId Start(0, 0);
	const FRTCellId Goal(2, 0);

	const FRTHexPathResult Before = URTHexPathLibrary::FindPath(M, Start, Goal);
	TestTrue(TEXT("il percorso esiste"), Before.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("due passi, costo 2"), Before.TotalCost, 2);

	SetSurcharge(M, Goal, 1);

	const FRTHexPathResult After = URTHexPathLibrary::FindPath(M, Start, Goal);
	TestTrue(TEXT("il percorso esiste ancora"), After.Status == ERTHexPathStatus::Success);
	TestEqual(TEXT("con la cella stretta costa 3"), After.TotalCost, 3);
	return true;
}

/** L'EFFETTO OSSERVABILE su `ReachableCells`: una cella stretta esce dal budget. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyReachableTest,
	"RefactorTactics.HexOccupancy.ConstrainedCellShrinksReachableCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyReachableTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeFlatMap(3);
	const FRTCellId Narrow(2, 0);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*MoveBudget=*/ 2));

	auto ReachesNarrow = [&](const URTHexMapAsset* Map)
	{
		const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(Map, Units);
		for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Snap, 1))
		{
			if (R.Cell == Narrow) { return true; }
		}
		return false;
	};

	TestTrue(TEXT("col budget 2 la cella a distanza 2 si raggiunge"), ReachesNarrow(M));

	SetSurcharge(M, Narrow, 1);
	TestFalse(TEXT("resa stretta, lo stesso budget non ci arriva piu'"), ReachesNarrow(M));

	// Il gemello di controllo: la cella PRIMA resta raggiungibile, cioe' non si e' rotto il movimento in
	// generale — si e' pagato di piu' solo dove la geometria stringe.
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);
	bool bReachesMidpoint = false;
	for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Snap, 1))
	{
		if (R.Cell == FRTCellId(1, 0)) { bReachesMidpoint = true; }
	}
	TestTrue(TEXT("la cella intermedia resta raggiungibile"), bReachesMidpoint);
	return true;
}

/**
 * Il sovrapprezzo entra nell'HASH di stato. Se non ci entrasse, due mappe che si giocano diversamente
 * avrebbero lo stesso hash e il KPI `replay divergence = 0` sarebbe verde per il motivo sbagliato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyHashTest,
	"RefactorTactics.HexOccupancy.SurchargeEntersTheMatchStateHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyHashTest::RunTest(const FString&)
{
	URTHexMapAsset* Plain = MakeFlatMap(2);
	URTHexMapAsset* Narrow = MakeFlatMap(2);
	TestEqual(TEXT("due mappe identiche hanno lo stesso hash"), Plain->ComputeHash(), Narrow->ComputeHash());

	SetSurcharge(Narrow, FRTCellId(1, 0), 1);
	TestNotEqual(TEXT("il sovrapprezzo cambia l'hash"), Plain->ComputeHash(), Narrow->ComputeHash());
	return true;
}

/** Un asset scritto PRIMA del campo si rilegge senza perdere nulla, e nasce senza sovrapprezzo. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancyMigrationTest,
	"RefactorTactics.HexOccupancy.MigrationToV7DefaultsTheSurchargeToZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancyMigrationTest::RunTest(const FString&)
{
	URTHexMapAsset* Legacy = NewObject<URTHexMapAsset>();
	FRTHexCellData Cell(FRTCellId(0, 0));
	Cell.MoveCost = 3;
	Cell.Height = 2;
	Legacy->AddOrUpdateCell(Cell);
	Legacy->AddOrUpdateCell(FRTHexCellData(FRTCellId(1, 0)));
	Legacy->FormatVersion = 6;

	Legacy->MigrateToCurrentFormat();

	// Pinnato di proposito: un bump di formato deve far cadere un test, non passare inosservato.
	TestEqual(TEXT("la versione corrente e' la 7"), URTHexMapAsset::CurrentFormatVersion, 7);
	TestEqual(TEXT("versione portata alla corrente"), Legacy->FormatVersion,
		URTHexMapAsset::CurrentFormatVersion);
	TestEqual(TEXT("nessuna cella persa"), Legacy->NumCells(), 2);

	const FRTHexCellData* Migrated = Legacy->FindCell(FRTCellId(0, 0));
	TestTrue(TEXT("la cella esiste ancora"), Migrated != nullptr);
	if (Migrated)
	{
		TestEqual(TEXT("MoveCost preservato"), Migrated->MoveCost, 3);
		TestEqual(TEXT("Height preservata"), Migrated->Height, 2);
		TestEqual(TEXT("nessun sovrapprezzo inventato"), Migrated->OccupancySurcharge, 0);
	}
	TestEqual(TEXT("mappa migrata valida"), Legacy->ValidateMap().Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Il guasto che ha deciso DOVE vive il sovrapprezzo, verificato sul codice vero e non su una simulazione.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	UWorld* MakeOccupancyWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyOccupancyWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnOccupancyUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		U->Cell = Cell;
		U->FinishSpawning(FTransform::Identity);
		return U;
	}

	void PlayOneOccupancyTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

/**
 * ⚠️ **Questo test e' la ragione per cui il sovrapprezzo NON vive dentro `MoveCost`.**
 *
 * `ApplyDynamicSurface` riscrive `MoveCost` dal catalogo della nuova superficie, e `TickDynamicSurfaces` lo
 * riscrive di nuovo al ripristino. Se il sovrapprezzo fosse dentro quel campo, un corridoio stretto su cui
 * un'abilita' mettesse dell'acqua tornerebbe al costo del pavimento e resterebbe largo per il resto della
 * partita — senza che nulla protesti.
 *
 * Il ciclo si esercita col codice VERO (`ApplyDynamicSurfaceForTest` chiama la produzione), non simulando in
 * un test cio' che il turno fa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOccupancySurvivesSurfaceCycleTest,
	"RefactorTactics.HexOccupancy.SurchargeSurvivesADynamicSurfaceCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOccupancySurvivesSurfaceCycleTest::RunTest(const FString&)
{
	UWorld* World = MakeOccupancyWorld();
	if (!World) { return false; }

	URTHexMapAsset* Map = MakeFlatMap(4);
	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	MapActor->MapAsset = Map;

	ARTUnit* A = SpawnOccupancyUnit(World, 0, URTHeroCatalogLibrary::MakeVektor(), FRTCellId(-3, 1));
	SpawnOccupancyUnit(World, 1, URTHeroCatalogLibrary::MakeBastion(), FRTCellId(3, -1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !A) { DestroyOccupancyWorld(World); return false; }

	PlayOneOccupancyTurn(TM);

	// Un corridoio stretto: la geometria l'ha reso `Constrained` e la cottura ci ha messo il sovrapprezzo.
	const FRTCellId Narrow(1, 0, 0);
	SetSurcharge(Map, Narrow, 1);
	const int32 CostBefore = Map->FindCell(Narrow) ? Map->FindCell(Narrow)->MoveCost : -1;

	// Durata 1: il Cleanup del turno seguente la riporta com'era.
	const bool bApplied = TM->ApplyDynamicSurfaceForTest(Map, Narrow, ERTHexSurface::ShallowWater, /*Turns=*/ 1,
		FName(TEXT("Action.Ignite")), A);
	if (!TestTrue(TEXT("la superficie e' stata trasformata"), bApplied))
	{
		DestroyOccupancyWorld(World); return false;
	}

	const FRTHexCellData* Wet = Map->FindCell(Narrow);
	TestTrue(TEXT("la cella esiste dopo il cambio"), Wet != nullptr);
	if (Wet)
	{
		TestEqual(TEXT("il cambio di superficie NON tocca il sovrapprezzo"), Wet->OccupancySurcharge, 1);
	}

	PlayOneOccupancyTurn(TM); // il Cleanup fa scadere la superficie e la ripristina

	const FRTHexCellData* Restored = Map->FindCell(Narrow);
	TestTrue(TEXT("la cella esiste dopo il ripristino"), Restored != nullptr);
	if (Restored)
	{
		TestEqual(TEXT("la superficie e' tornata quella di prima"),
			static_cast<int32>(Restored->Surface), static_cast<int32>(ERTHexSurface::Floor));
		TestEqual(TEXT("anche il MoveCost e' tornato quello di prima"), Restored->MoveCost, CostBefore);
		TestEqual(TEXT("e il sovrapprezzo e' SOPRAVVISSUTO al ciclo completo"), Restored->OccupancySurcharge, 1);
	}

	DestroyOccupancyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
