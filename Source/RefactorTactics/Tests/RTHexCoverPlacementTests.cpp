#include "Misc/AutomationTest.h"
#include "Map/RTHexCoverPlacementLibrary.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * LA MATRICE MINIMA del Decision Record *«Cover Placement & Intra-Hex Geometry»* (2026-08-30), nella parte
 * che vive in una funzione pura. Il resto — intento di movimento con copertura scelta, replay, PIE,
 * packaged — non e' qui perche' non e' ancora implementato, e un test che lo simulasse mentirebbe.
 *
 * ⚠️ **Due asserzioni di questo file esistono per NEGARE una regola precedente**, e vanno lette insieme al
 * codice che superano: `CenterCrossingWall...` nega *«geometria che tocca il centro => cella bloccata»*
 * (`D-179` punto 3) e `TwoObstacleGroups...` nega *«sei settori occupati => cella bloccata»*
 * (`FRTOccupancyThresholds::BlockedFrom`). Sono le due scorciatoie che il Decision Record dichiara superate.
 */

namespace
{
	/** Maschera dai settori elencati. Espressa per settori NOMINATI, non per conteggio: qui conta QUALI. */
	FRTOccupancyMask MaskOf(std::initializer_list<int32> Wedges, bool bCore = false)
	{
		FRTOccupancyMask M;
		for (int32 W : Wedges) { M.Sectors |= (1 << W); }
		M.bCoreBlocked = bCore;
		return M;
	}

	int32 MaskFromWedges(std::initializer_list<int32> Wedges)
	{
		int32 Bits = 0;
		for (int32 W : Wedges) { Bits |= (1 << W); }
		return Bits;
	}

	/** Una cella con una copertura su ciascuno dei bordi elencati. */
	FRTHexCellData CellWithCovers(std::initializer_list<ERTHexDirection> Edges)
	{
		FRTHexCellData Cell(FRTCellId(0, 0, 0));
		for (ERTHexDirection Edge : Edges)
		{
			Cell.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low));
		}
		return Cell;
	}

	/** Il muro continuo: diametro sull'asse, passante per il centro (`Offset == 0`). */
	FRTGeometrySegment Diameter(ERTTacticalAxis Axis)
	{
		FRTGeometrySegment S;
		S.Axis = Axis;
		S.Offset = 0;
		S.AlongStart = -RT_GeometryQuanta;
		S.AlongEnd = RT_GeometryQuanta;
		S.WallType = ERTHexCoverType::High;
		return S;
	}

	/** Il raggio centro -> punto notevole: meta' diametro, e non separa la cella. */
	FRTGeometrySegment CenterRay(ERTTacticalAxis Axis)
	{
		FRTGeometrySegment S = Diameter(Axis);
		S.AlongStart = 0;
		return S;
	}

	int32 CountOptionsWithSide(const TArray<FRTCoverOption>& Options, ERTCoverSide Side)
	{
		int32 N = 0;
		for (const FRTCoverOption& O : Options) { if (O.Side == Side) { ++N; } }
		return N;
	}
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 1 · Cella vuota
// ─────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementEmptyCellTest,
	"RefactorTactics.CoverPlacement.EmptyCellIsStandableWithNoCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementEmptyCellTest::RunTest(const FString&)
{
	const FRTOccupancyMask Empty;

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Empty, Regions);

	TestEqual(TEXT("una sola regione"), Regions.Num(), 1);
	TestEqual(TEXT("copre tutti i dodici settori"), Regions.Num() == 1 ? Regions[0].Size : -1,
		RT_OccupancySectorCount);
	TestEqual(TEXT("FirstWedge convenzionale a zero"), Regions.Num() == 1 ? Regions[0].FirstWedge : -1, 0);

	TestTrue(TEXT("calpestabile"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Empty, FRTFootprintProfile()));

	TArray<FRTCoverOption> Options;
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(FRTHexCellData(FRTCellId(0, 0, 0)),
		TArray<FRTGeometrySegment>(), Empty, Options);
	TestEqual(TEXT("nessuna copertura"), Options.Num(), 0);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 2-3 · Rocce, e rocce piu' albero: due gruppi disgiunti, due opzioni indipendenti
// ─────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementRocksTest,
	"RefactorTactics.CoverPlacement.RocksKeepCellStandableAndExposeAdjacentCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementRocksTest::RunTest(const FString&)
{
	// Rocce sui settori 1, 2, 3: il caso 2 della matrice minima.
	const FRTOccupancyMask Rocks = MaskOf({ 1, 2, 3 });

	TestTrue(TEXT("resta calpestabile"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Rocks, FRTFootprintProfile()));

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Rocks, Regions);
	TestEqual(TEXT("una sola regione libera"), Regions.Num(), 1);
	TestEqual(TEXT("nove settori liberi in fila"), Regions.Num() == 1 ? Regions[0].Size : -1, 9);

	// Il bordo NE copre i settori 2 e 3, che le rocce occupano: chi sta accanto ci si ripara lo stesso.
	TArray<FRTCoverOption> Options;
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(CellWithCovers({ ERTHexDirection::NE }),
		TArray<FRTGeometrySegment>(), Rocks, Options);
	TestEqual(TEXT("una opzione di copertura"), Options.Num(), 1);
	TestTrue(TEXT("e' del bordo, senza faccia"), Options.Num() == 1
		&& Options[0].Source.Kind == ERTCoverSourceKind::Edge && Options[0].Side == ERTCoverSide::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementTwoGroupsTest,
	"RefactorTactics.CoverPlacement.TwoObstacleGroupsExposeIndependentOptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementTwoGroupsTest::RunTest(const FString&)
{
	// Rocce su 1,2,3 e albero su 7,8,9: il caso 3 della matrice minima.
	const FRTOccupancyMask Both = MaskOf({ 1, 2, 3, 7, 8, 9 });

	// 🔴 La regola SUPERATA, tenuta qui come contrasto misurato e non come ricordo: con sei settori
	// occupati la soglia dichiara `Blocked`, cioe' «non si attraversa».
	TestTrue(TEXT("la vecchia soglia direbbe Blocked"),
		URTHexOccupancyLibrary::Classify(Both, FRTOccupancyThresholds()) == ERTCellOccupancy::Blocked);

	// La regola nuova guarda la FORMA dello spazio libero, non il conteggio.
	TestTrue(TEXT("ma una posa legale esiste"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Both, FRTFootprintProfile()));

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Both, Regions);
	TestEqual(TEXT("due regioni disgiunte"), Regions.Num(), 2);
	TestEqual(TEXT("la prima comincia al settore 4"), Regions.Num() == 2 ? Regions[0].FirstWedge : -1, 4);
	TestEqual(TEXT("la seconda al settore 10"), Regions.Num() == 2 ? Regions[1].FirstWedge : -1, 10);
	TestTrue(TEXT("tre settori ciascuna"),
		Regions.Num() == 2 && Regions[0].Size == 3 && Regions[1].Size == 3);

	// Una copertura per gruppo: NE tocca le rocce (settori 2-3), SW tocca l'albero (settori 8-9).
	TArray<FRTCoverOption> Options;
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(
		CellWithCovers({ ERTHexDirection::NE, ERTHexDirection::SW }),
		TArray<FRTGeometrySegment>(), Both, Options);

	TestEqual(TEXT("due opzioni, una per gruppo"), Options.Num(), 2);
	if (Options.Num() == 2)
	{
		TestTrue(TEXT("le due opzioni si usano da regioni diverse"),
			(Options[0].AccessMask & Options[1].AccessMask) == 0);
		TestTrue(TEXT("chi sta al settore 5 usa solo la prima"),
			URTHexCoverPlacementLibrary::IsOptionReachableFromWedge(Both, Options[0], 5)
			&& !URTHexCoverPlacementLibrary::IsOptionReachableFromWedge(Both, Options[1], 5));
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 4-5 · Muri larghi e muri centrali: la regola del centro non blocca piu' da sola
// ─────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementWideWallTest,
	"RefactorTactics.CoverPlacement.WideWallLeavesCellStandable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementWideWallTest::RunTest(const FString&)
{
	// Un muro che sottende 120 gradi: quattro settori consecutivi.
	const FRTOccupancyMask Wall = MaskOf({ 0, 1, 2, 3 });

	TestTrue(TEXT("calpestabile"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Wall, FRTFootprintProfile()));

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Wall, Regions);
	TestEqual(TEXT("una regione da otto"), Regions.Num() == 1 ? Regions[0].Size : -1, 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementCenterTest,
	"RefactorTactics.CoverPlacement.CenterCrossingWallNoLongerBlocksTheCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementCenterTest::RunTest(const FString&)
{
	// Un diametro sull'asse `Deg0`: attraversa il centro e occupa i quattro settori dei due passaggi.
	const FRTOccupancyMask Crossing = MaskOf({ 0, 1, 6, 7 }, /*bCore*/ true);

	// 🔴 La regola SUPERATA: il centro occupato bloccava la cella da solo, per chiunque.
	TestTrue(TEXT("la vecchia regola direbbe Blocked"),
		URTHexOccupancyLibrary::Classify(Crossing, FRTOccupancyThresholds()) == ERTCellOccupancy::Blocked);

	// La regola nuova: il centro e' un REQUISITO di profilo, non un divieto universale.
	TestTrue(TEXT("con il profilo di default si sta"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Crossing, FRTFootprintProfile()));

	FRTFootprintProfile NeedsCore;
	NeedsCore.bRequiresFreeCore = true;
	TestFalse(TEXT("un profilo che pretende il centro no"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Crossing, NeedsCore));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementFootprintTest,
	"RefactorTactics.CoverPlacement.FootprintDecidesStandabilityNotTheCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementFootprintTest::RunTest(const FString&)
{
	// Sei settori liberi, ma in due gruppi da tre: lo stesso CONTEGGIO di sei in fila, spazio diverso.
	const FRTOccupancyMask Split = MaskOf({ 1, 2, 3, 7, 8, 9 });
	const FRTOccupancyMask InLine = MaskOf({ 0, 1, 2, 3, 4, 5 });

	FRTFootprintProfile Small;
	Small.MinContiguousWedges = 3;
	FRTFootprintProfile Large;
	Large.MinContiguousWedges = 4;

	TestTrue(TEXT("il piccolo entra nei due gruppi da tre"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Split, Small));
	TestFalse(TEXT("il grande no"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Split, Large));
	TestTrue(TEXT("ma entra nei sei in fila, a parita' di conteggio"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(InLine, Large));
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 6-8 · Due facce, e la traversata che non e' un teletrasporto
// ─────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementContinuousWallTest,
	"RefactorTactics.CoverPlacement.ContinuousWallSeparatesSidesAndRejectsTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementContinuousWallTest::RunTest(const FString&)
{
	const FRTOccupancyMask Crossing = MaskOf({ 0, 1, 6, 7 }, /*bCore*/ true);

	TArray<FRTGeometrySegment> Segments;
	Segments.Add(Diameter(ERTTacticalAxis::Deg0));

	TArray<FRTCoverOption> Options;
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(FRTHexCellData(FRTCellId(0, 0, 0)), Segments,
		Crossing, Options);

	TestEqual(TEXT("due opzioni, una per faccia"), Options.Num(), 2);
	TestEqual(TEXT("una faccia A"), CountOptionsWithSide(Options, ERTCoverSide::A), 1);
	TestEqual(TEXT("una faccia B"), CountOptionsWithSide(Options, ERTCoverSide::B), 1);
	if (Options.Num() == 2)
	{
		TestTrue(TEXT("stessa sorgente, facce diverse"), Options[0].Source == Options[1].Source);
		TestTrue(TEXT("accessi disgiunti"), (Options[0].AccessMask & Options[1].AccessMask) == 0);
	}

	// 🔑 Il muro continuo separa: da un lato non si arriva all'altro restando nella cella.
	TestTrue(TEXT("SideA -> SideB rifiutata"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversal(Crossing, 3, 9)
			== ERTIntraCellTraversal::Blocked);
	TestTrue(TEXT("dentro lo stesso lato si passa"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversal(Crossing, 2, 5)
			== ERTIntraCellTraversal::SameRegion);

	// E l'opzione dell'altra faccia non e' raggiungibile: la scelta non teletrasporta.
	for (const FRTCoverOption& Option : Options)
	{
		const bool bReachable =
			URTHexCoverPlacementLibrary::IsOptionReachableFromWedge(Crossing, Option, 3);
		TestEqual(TEXT("dal settore 3 e' raggiungibile solo la faccia A"),
			bReachable, Option.Side == ERTCoverSide::A);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementCenterRayTest,
	"RefactorTactics.CoverPlacement.CenterToVertexWallExposesBothSidesInOneRegion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementCenterRayTest::RunTest(const FString&)
{
	// Il raggio centro -> vertice attraversa un solo confine: occupa due settori e non separa la cella.
	const FRTOccupancyMask Ray = MaskOf({ 1, 2 }, /*bCore*/ true);

	TArray<FRTGeometrySegment> Segments;
	Segments.Add(CenterRay(ERTTacticalAxis::Deg30));

	TArray<FRTCoverOption> Options;
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(FRTHexCellData(FRTCellId(0, 0, 0)), Segments,
		Ray, Options);

	TestEqual(TEXT("due facce, come per il muro continuo"), Options.Num(), 2);
	TestEqual(TEXT("una faccia A"), CountOptionsWithSide(Options, ERTCoverSide::A), 1);
	TestEqual(TEXT("una faccia B"), CountOptionsWithSide(Options, ERTCoverSide::B), 1);

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Ray, Regions);
	TestEqual(TEXT("ma la regione libera e' UNA"), Regions.Num(), 1);

	// 🔑 La differenza col muro continuo: qui si passa girando attorno all'estremo, ed e' un percorso reale.
	TestTrue(TEXT("da una faccia all'altra si passa"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversal(Ray, 4, 10)
			== ERTIntraCellTraversal::SameRegion);
	for (const FRTCoverOption& Option : Options)
	{
		TestTrue(TEXT("entrambe le facce sono raggiungibili dal settore 4"),
			URTHexCoverPlacementLibrary::IsOptionReachableFromWedge(Ray, Option, 4));
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 9 · Occupancy: piu' opzioni non fanno piu' posti
// ─────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementCapacityTest,
	"RefactorTactics.CoverPlacement.CoverOptionsDoNotIncreaseCellCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementCapacityTest::RunTest(const FString&)
{
	// La cella contesa espone davvero due opzioni distinte: senza questo, il test sotto proverebbe una
	// contesa fra due unita' che NON avevano fra cui scegliere, cioe' un caso piu' debole di quello voluto.
	const FRTOccupancyMask Crossing = MaskOf({ 0, 1, 6, 7 }, /*bCore*/ true);
	TArray<FRTGeometrySegment> Segments;
	Segments.Add(Diameter(ERTTacticalAxis::Deg0));

	TArray<FRTCoverOption> Options;
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(FRTHexCellData(FRTCellId(1, 0, 0)), Segments,
		Crossing, Options);
	TestEqual(TEXT("la destinazione espone due opzioni"), Options.Num(), 2);

	// E il resolver AUTOREVOLE le ignora, perche' la cella e' una: due unita' che la puntano nello stesso
	// micro-step si fermano entrambe, qualunque faccia avessero in mente.
	TArray<TArray<FRTCellId>> Paths;
	Paths.Add({ FRTCellId(0, 0), FRTCellId(1, 0) });
	Paths.Add({ FRTCellId(2, 0), FRTCellId(1, 0) });

	const TArray<FRTHexMoveResult> R = URTHexSimLibrary::ResolveHexPaths(Paths);
	TestEqual(TEXT("un risultato per richiesta"), R.Num(), 2);
	TestTrue(TEXT("nessuna delle due entra"), R.Num() == 2
		&& R[0].Final == FRTCellId(0, 0) && R[1].Final == FRTCellId(2, 0));
	TestTrue(TEXT("e il motivo e' la contesa, non l'ordine"), R.Num() == 2
		&& R[0].Outcome == ERTMoveOutcome::BlockedContested
		&& R[1].Outcome == ERTMoveOutcome::BlockedContested);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 10 · Determinismo e identita' stabile
// ─────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementStableIdTest,
	"RefactorTactics.CoverPlacement.SourceIdIsStableAcrossCollectionReordering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementStableIdTest::RunTest(const FString&)
{
	const FRTOccupancyMask Empty;

	FRTGeometrySegment First = Diameter(ERTTacticalAxis::Deg0);
	FRTGeometrySegment Second = CenterRay(ERTTacticalAxis::Deg60);

	TArray<FRTGeometrySegment> Forward;
	Forward.Add(First);
	Forward.Add(Second);

	TArray<FRTGeometrySegment> Backward;
	Backward.Add(Second);
	Backward.Add(First);

	// Lo stesso segmento, percorso al contrario, e' lo STESSO segmento: `FRTGeometrySegment::operator==` lo
	// dice gia', e l'identita' della sorgente deve dirlo con lui — altrimenti un rieditaggio che scambia
	// gli estremi produrrebbe una chiave nuova per un muro che non e' cambiato.
	FRTGeometrySegment Reversed = First;
	Swap(Reversed.AlongStart, Reversed.AlongEnd);
	TArray<FRTGeometrySegment> WithReversed;
	WithReversed.Add(Reversed);

	TArray<FRTCoverOption> A, B, C;
	const FRTHexCellData Cell(FRTCellId(0, 0, 0));
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(Cell, Forward, Empty, A);
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(Cell, Backward, Empty, B);
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(Cell, WithReversed, Empty, C);

	TestEqual(TEXT("stesso numero di opzioni"), A.Num(), B.Num());
	TestTrue(TEXT("le chiavi sono le stesse, in ordine scambiato"), A.Num() == 4 && B.Num() == 4
		&& A[0].Source == B[2].Source && A[2].Source == B[0].Source);
	TestTrue(TEXT("il segmento invertito ha la stessa chiave"),
		C.Num() > 0 && A.Num() > 0 && C[0].Source == A[0].Source);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementRegionsCanonicalTest,
	"RefactorTactics.CoverPlacement.RegionsAreCanonicalAndCircular",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementRegionsCanonicalTest::RunTest(const FString&)
{
	// La contiguita' scavalca lo zero: 11 e 0 sono adiacenti, e una regione puo' passarci sopra.
	const FRTOccupancyMask Mask = MaskOf({ 4, 5 });

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Mask, Regions);

	TestEqual(TEXT("una regione sola, che scavalca lo zero"), Regions.Num(), 1);
	TestEqual(TEXT("comincia al settore 6"), Regions.Num() == 1 ? Regions[0].FirstWedge : -1, 6);
	TestEqual(TEXT("dieci settori"), Regions.Num() == 1 ? Regions[0].Size : -1, 10);
	TestEqual(TEXT("e la maschera li elenca tutti"), Regions.Num() == 1 ? Regions[0].WedgeMask : -1,
		MaskFromWedges({ 6, 7, 8, 9, 10, 11, 0, 1, 2, 3 }));

	// Cella interamente occupata: nessuna posa, nessuna opzione, e nessun crash.
	FRTOccupancyMask Full;
	Full.Sectors = 0xFFF;
	TArray<FRTPlacementRegion> None;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Full, None);
	TestEqual(TEXT("nessuna regione"), None.Num(), 0);
	TestFalse(TEXT("non calpestabile"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Full, FRTFootprintProfile()));
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 11 · La copertura scelta non spegne il resto della geometria
// ─────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementIndependenceTest,
	"RefactorTactics.CoverPlacement.SelectingOneSourceDoesNotSuppressTheOthers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementIndependenceTest::RunTest(const FString&)
{
	const FRTOccupancyMask Rocks = MaskOf({ 1, 2, 3 });

	FRTHexCellData Cell = CellWithCovers({ ERTHexDirection::NE });
	Cell.bBlocksLineOfSight = true;   // l'albero che ferma la vista, e che nessuno ha «scelto»

	TArray<FRTGeometrySegment> Segments;
	Segments.Add(CenterRay(ERTTacticalAxis::Deg60));

	TArray<FRTCoverOption> Options;
	URTHexCoverPlacementLibrary::EnumerateCoverOptions(Cell, Segments, Rocks, Options);

	// Il bordo E il segmento sono entrambi enumerati: sceglierne uno e' una scelta di chi gioca, non un
	// filtro applicato qui. Nessuna funzione di questo file puo' rendere l'altro intangibile, perche'
	// nessuna scrive.
	bool bHasEdge = false;
	bool bHasSegment = false;
	for (const FRTCoverOption& O : Options)
	{
		bHasEdge = bHasEdge || O.Source.Kind == ERTCoverSourceKind::Edge;
		bHasSegment = bHasSegment || O.Source.Kind == ERTCoverSourceKind::InteriorSegment;
	}
	TestTrue(TEXT("il bordo e' enumerato"), bHasEdge);
	TestTrue(TEXT("il segmento anche"), bHasSegment);

	// E il dato di blocco vista resta quello che era: l'enumerazione non lo legge e non lo tocca.
	TestTrue(TEXT("bBlocksLineOfSight invariato"), Cell.bBlocksLineOfSight);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 12 · Il ponte con la geometria vera: la maschera che il diametro produce davvero
// ─────────────────────────────────────────────────────────────────────────────────────────────────

/**
 * 🔴 **IL BLOCCO MISURATO, non un test che gira a vuoto.**
 *
 * Gli altri test di questo file lavorano su maschere scritte a mano, che sono l'ingresso legittimo di una
 * libreria pura. Questo le confronta con la pipeline vera — grammatica -> polilinea -> `ComputeMask` — e la
 * risposta e' che **oggi non combaciano**, per una ragione precisa e gia' registrata.
 *
 * Un segmento passante per il centro tocca il centro. Il centro e' il vertice comune di **tutti e dodici**
 * i triangoli di settore, e la regola d'intersezione di `ComputeMask` e' dichiaratamente conservativa —
 * *«un settore e' occupato se la geometria lo INTERSECA»*, contatto puntuale compreso. Ne segue che ogni
 * muro centrale accende dodici bit su dodici, e con zero settori liberi non esiste posa: **la regola che il
 * Decision Record dichiara superata sopravvive qui, dentro il produttore della maschera.**
 *
 * ⚠️ **Non si corregge in questo commit, e non e' prudenza.** «Il contatto in un solo punto e' invasione?»
 * e' `MSE-4` in `docs/OPEN_DECISIONS.md` e [#717](https://github.com/DegrassiAaron/refactor-tactics-main/issues/717),
 * una decisione aperta con due uscite scritte. Sceglierne una qui significherebbe deciderla per inerzia
 * dentro una PR che parla d'altro — che e' esattamente cio' che il Decision Record vieta.
 *
 * ✅ **Cio' che questo test compra**: il giorno in cui `MSE-4` si chiude a favore dell'intersezione di
 * lunghezza non nulla, questo test diventa **rosso**, e chi lo legge trova qui l'altra meta' del lavoro
 * gia' scritta. Un blocco che nessun test misura si riscopre da capo sei mesi dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementRealGeometryTest,
	"RefactorTactics.CoverPlacement.CentreContactRuleStillCollapsesTheWholeCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementRealGeometryTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.0f;

	const FRTGeometrySegment Segment = Diameter(ERTTacticalAxis::Deg0);
	TestTrue(TEXT("il diametro appartiene alla grammatica"),
		URTGeometryGrammarLibrary::ValidateSegment(Segment) == ERTGeometryViolation::None);

	TArray<FRTOccupancyPolyline> Geometry;
	Geometry.Add(URTGeometryGrammarLibrary::ToPolyline(Segment, HexSize));
	const FRTOccupancyMask Mask = URTHexOccupancyLibrary::ComputeMask(Geometry, HexSize);

	// La misura, esatta: dodici bit su dodici da un muro che ne attraversa quattro.
	TestEqual(TEXT("MSE-4: il contatto nel centro accende tutti i settori"), Mask.Sectors, 0xFFF);

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Mask, Regions);
	TestEqual(TEXT("quindi nessuna regione di posa"), Regions.Num(), 0);
	TestFalse(TEXT("e la cella resta non calpestabile, come nella regola superata"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Mask, FRTFootprintProfile()));

	// ⚠️ La libreria di posa NON e' complice: data la maschera che `MSE-4` produrrebbe con l'altra uscita,
	// risponde gia' come il Decision Record chiede. Il difetto e' a monte, e questa riga lo dimostra invece
	// di lasciarlo dedurre.
	const FRTOccupancyMask WithoutPointContact = MaskOf({ 0, 1, 6, 7 }, /*bCore*/ true);
	TArray<FRTPlacementRegion> Would;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(WithoutPointContact, Would);
	TestEqual(TEXT("con l'altra uscita di MSE-4 sarebbero due regioni"), Would.Num(), 2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
