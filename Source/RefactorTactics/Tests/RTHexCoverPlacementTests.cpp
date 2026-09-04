#include "Misc/AutomationTest.h"
#include "Map/RTHexCoverPlacementLibrary.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchSetupLibrary.h"
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

	// I nomi sono quelli del catalogo (`D-307`): `Small` 2, `Medium` 3, `Large` 4 settori contigui.
	// ⚠️ Fino al 2026-09-01 il profilo a 3 si chiamava qui `Small`: era scritto prima che le taglie
	// avessero un nome canonico, e con `D-307` quel valore e' il `Medium`. Le due misure che il test gia'
	// faceva non cambiano — cambia quale taglia le porta —, e la terza (`Small` a 2) e' nuova.
	FRTFootprintProfile Small;
	Small.MinContiguousWedges = 2;
	FRTFootprintProfile Medium;
	Medium.MinContiguousWedges = 3;
	FRTFootprintProfile Large;
	Large.MinContiguousWedges = 4;

	TestTrue(TEXT("il piccolo entra nei due gruppi da tre"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Split, Small));
	TestTrue(TEXT("e il medio pure, che ne chiede esattamente tre"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Split, Medium));
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
 * 🔴 **IL BLOCCO CHE E' CADUTO, e il test che lo misurava e' rimasto a misurarne l'assenza.**
 *
 * Fino al 2026-08-31 questo test si chiamava `CentreContactRuleStillCollapsesTheWholeCell` e asseriva
 * `Mask.Sectors == 0xFFF`: un muro passante per il centro accendeva **dodici** settori su dodici e la cella
 * diventava inagibile. La sua intestazione dichiarava anche perche' non si correggeva li' — *«"il contatto in
 * un solo punto e' invasione?" e' `MSE-4`, una decisione aperta con due uscite scritte: sceglierne una qui
 * significherebbe deciderla per inerzia dentro una PR che parla d'altro»* — e prometteva: *«il giorno in cui
 * `MSE-4` si chiude a favore dell'intersezione di lunghezza non nulla, questo test diventa rosso, e chi lo
 * legge trova qui l'altra meta' del lavoro gia' scritta»*.
 *
 * ✅ **Quel giorno e' arrivato con `#1826`**, e il test e' stato **riscritto invece che cancellato**: il caso
 * resta misurato, con l'esito nuovo. Cancellarlo avrebbe tolto l'unica misura della pipeline vera —
 * grammatica → polilinea → `ComputeMask` — lasciando gli altri test di questo file su maschere scritte a mano.
 *
 * 🔑 **La riga che segue e' la piu' importante del file**: dice che il numero e' cambiato **e perche'**. Un
 * `0xC3` senza storia, fra sei mesi, sembrerebbe una costante arbitraria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementRealGeometryTest,
	"RefactorTactics.CoverPlacement.CentreContactNoLongerCollapsesTheWholeCell",
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

	// LA MISURA, esatta: quattro bit, non dodici. `0xC3` e' `{0, 1, 6, 7}` — i due settori a destra e i due
	// a sinistra del diametro `Deg0`, che va dal punto medio del lato `W` a quello del lato `E` e giace sul
	// confine fra `0` e `1` da una parte e fra `6` e `7` dall'altra.
	TestEqual(TEXT("MSE-4: il muro occupa i settori che attraversa, non tutti"),
		Mask.Sectors, 0xC3);
	TestFalse(TEXT("e il centro non e' bloccato: nessun footprint chiuso lo contiene"), Mask.bCoreBlocked);

	// ⚠️ **Il contatto lungo un segmento resta occupato**, ed e' il motivo per cui i settori sono quattro e
	// non due: il diametro giace sul confine fra due settori per ciascun semipiano, e li invade entrambi.
	// La regola nuova toglie il contatto PUNTUALE, non quello lungo un tratto.

	TArray<FRTPlacementRegion> Regions;
	URTHexCoverPlacementLibrary::ComputeFreeRegions(Mask, Regions);
	TestEqual(TEXT("due regioni di posa, una per semipiano"), Regions.Num(), 2);
	TestTrue(TEXT("e la cella torna calpestabile"),
		URTHexCoverPlacementLibrary::HasLegalPlacement(Mask, FRTFootprintProfile()));

	// 🔑 **La controprova che il difetto era a monte**, e la riga viene dal test precedente: la libreria di
	// posa rispondeva GIA' bene alla maschera giusta. Ora la pipeline vera produce quella maschera, quindi
	// le due devono coincidere — se divergessero, il difetto si sarebbe solo spostato.
	const FRTOccupancyMask Expected = MaskOf({ 0, 1, 6, 7 }, /*bCore*/ false);
	TestEqual(TEXT("la pipeline vera produce ora la maschera che la posa gia' sapeva leggere"),
		Mask.Sectors, Expected.Sectors);

	return true;
}

/**
 * IL DIAMETRO SU TUTTI E SEI GLI ASSI — perche' `Deg0` da solo non e' una regola.
 *
 * ⚠️ **Un solo asse non distingue una regola da una coincidenza.** `Deg0` punta al punto medio di un lato;
 * tre dei sei assi puntano invece a un VERTICE, dove la geometria dei triangoli attorno e' diversa. Se la
 * regola valesse solo per gli assi «comodi», il difetto sopravvivrebbe sulla meta' dei muri che un designer
 * puo' disegnare.
 *
 * 🔑 **E i quattro settori non si scrivono a mano**: si derivano da `AxisBoundaryIndex`, l'unico posto in
 * cui la corrispondenza fra un asse e i dodici confini e' definita — la stessa da cui `AxisHalfPlanes`
 * ricava i propri semipiani. Una tabella di costanti qui sarebbe la copia che diverge, e questo test non
 * misurerebbe piu' la regola ma la propria costante.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementDiameterOnEveryAxisTest,
	"RefactorTactics.CoverPlacement.DiameterOccupiesFourSectorsOnEveryAxis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementDiameterOnEveryAxisTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.0f;

	for (int32 AxisIndex = 0; AxisIndex < RT_TacticalAxisCount; ++AxisIndex)
	{
		const ERTTacticalAxis Axis = static_cast<ERTTacticalAxis>(AxisIndex);

		TArray<FRTOccupancyPolyline> Geometry;
		Geometry.Add(URTGeometryGrammarLibrary::ToPolyline(Diameter(Axis), HexSize));
		const FRTOccupancyMask Mask = URTHexOccupancyLibrary::ComputeMask(Geometry, HexSize);

		int32 Occupied = 0;
		for (int32 Sector = 0; Sector < RT_OccupancySectorCount; ++Sector)
		{
			if ((Mask.Sectors & (1 << Sector)) != 0) { ++Occupied; }
		}
		TestEqual(*FString::Printf(TEXT("asse %d: quattro settori occupati"), AxisIndex), Occupied, 4);

		TArray<FRTPlacementRegion> Regions;
		URTHexCoverPlacementLibrary::ComputeFreeRegions(Mask, Regions);
		TestEqual(*FString::Printf(TEXT("asse %d: due regioni libere"), AxisIndex), Regions.Num(), 2);

		// 🔑 **QUALI quattro, e non solo quanti.** Un conteggio da solo passerebbe anche se i settori fossero
		// quelli sbagliati. Il diametro buca il perimetro nei due confini opposti `b` e `b + 6`, e un
		// confine separa due settori: gli occupati sono percio' i due a cavallo di ciascuno.
		//
		// ⛔ La corrispondenza asse -> confine non si riscrive qui: `AxisBoundaryIndex` e' l'unico posto in
		// cui e' definita, ed e' la stessa da cui `AxisHalfPlanes` deriva i propri semipiani.
		const int32 Boundary = URTGeometryGrammarLibrary::AxisBoundaryIndex(Axis);
		int32 Straddling = 0;
		for (const int32 Step : { -1, 0, 5, 6 })
		{
			const int32 Wedge = ((Boundary + Step) % RT_OccupancySectorCount + RT_OccupancySectorCount)
				% RT_OccupancySectorCount;
			Straddling |= (1 << Wedge);
		}
		TestEqual(*FString::Printf(TEXT("asse %d: sono i quattro settori a cavallo dei due confini"),
			AxisIndex), Mask.Sectors, Straddling);

		// E i due semipiani di `AxisHalfPlanes` restano complementari: e' la proprieta' su cui
		// `ComputeFreeRegions` conta per dare una regione per parte.
		int32 MaskA = 0, MaskB = 0;
		URTHexCoverPlacementLibrary::AxisHalfPlanes(Axis, MaskA, MaskB);
		TestEqual(*FString::Printf(TEXT("asse %d: i due semipiani non si sovrappongono"), AxisIndex),
			MaskA & MaskB, 0);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// 13 · La traversata autorata — `E23.7`, `D-308`, `#1828`
// ─────────────────────────────────────────────────────────────────────────────────────────────────

/**
 * UN MURO SCAVALCABILE RENDE VALIDO CIÒ CHE SENZA ERA `Blocked`, **e solo quello**.
 *
 * 🔑 È il terzo valore che l'enum aspettava, e questo test è il suo produttore misurato: fino a `D-308` la
 * scavalcabilità non esisteva come dato, quindi `AuthoredTraversal` sarebbe stata un'etichetta che nessun
 * ramo emette — il difetto che `RTHexCoverPlacementLibrary.h` §6 dichiarava di voler evitare.
 *
 * Le due maschere sono ciò che il chiamante costruisce: `Mask` con tutti i muri, `Traversable` senza quelli
 * che un autore ha marcato superabili.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementAuthoredTraversalTest,
	"RefactorTactics.CoverPlacement.AuthoredTraversalUnblocksOnlyWhatItSeparates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementAuthoredTraversalTest::RunTest(const FString&)
{
	// Un diametro `Deg0`: occupa `{0,1,6,7}` e divide la cella in due semipiani — `{2..5}` e `{8..11}`.
	const FRTOccupancyMask Divided = MaskOf({ 0, 1, 6, 7 }, /*bCore*/ true);
	const FRTOccupancyMask Empty;

	// Senza traversata: la vecchia risposta, invariata.
	TestEqual(TEXT("senza traversata resta Blocked"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversal(Divided, 3, 9),
		ERTIntraCellTraversal::Blocked);

	// Con il muro dichiarato scavalcabile: `Traversable` è la cella senza quel muro, cioè vuota.
	TestEqual(TEXT("con il muro scavalcabile la transizione è autorata"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversalWithAuthored(Divided, Empty, 3, 9),
		ERTIntraCellTraversal::AuthoredTraversal);

	// ⚠️ **E NON diventa `SameRegion`**: là non si attraversa niente e non si paga; qui si scavalca, e
	// `D-308` fissa un costo. Schiacciare i due valori renderebbe gratuito ciò che ha un prezzo.
	TestNotEqual(TEXT("e non si confonde con SameRegion"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversalWithAuthored(Divided, Empty, 3, 9),
		ERTIntraCellTraversal::SameRegion);

	// Due settori già nella stessa regione restano `SameRegion` anche con muri scavalcabili in giro: non si
	// paga un muro a cui si passa accanto.
	TestEqual(TEXT("chi non attraversa non paga"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversalWithAuthored(Divided, Empty, 3, 4),
		ERTIntraCellTraversal::SameRegion);

	// 🔴 **L'AC 5: una traversata autorata non collega due regioni ATTRAVERSO geometria bloccante.** Qui il
	// muro scavalcabile è uno solo dei due che separano, e toglierlo non basta: `Traversable` resta divisa
	// dall'altro, e la risposta torna `Blocked`. Il modello non sa esprimere il caso vietato.
	const FRTOccupancyMask StillDivided = MaskOf({ 0, 1, 6, 7 }, /*bCore*/ true);
	TestEqual(TEXT("se resta un muro non scavalcabile, è Blocked"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversalWithAuthored(Divided, StillDivided, 3, 9),
		ERTIntraCellTraversal::Blocked);

	// Un settore occupato non ha posa: nessuna traversata lo rende raggiungibile.
	TestEqual(TEXT("un settore occupato resta Blocked"),
		URTHexCoverPlacementLibrary::ClassifyIntraCellTraversalWithAuthored(Divided, Empty, 0, 3),
		ERTIntraCellTraversal::Blocked);
	return true;
}

/**
 * LA SCAVALCABILITÀ ENTRA NELL'HASH, e non si deduce dall'altezza.
 *
 * ⛔ La seconda metà è la clausola di `D-308`: `Low`/`High` sono vocabolario di **mitigazione** (`D-271`) e
 * non devono diventare per inerzia un vocabolario di **traversabilità**. Un muretto non è scavalcabile
 * perché è basso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverPlacementTraversableHashTest,
	"RefactorTactics.HexMap.TraversableWallEntersHashAndIsNotDerivedFromHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverPlacementTraversableHashTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 2);
	if (!TestNotNull(TEXT("arena"), Map)) { return false; }

	FRTHexInteriorWall Wall(FRTCellId(0, 0, 0), Diameter(ERTTacticalAxis::Deg0));
	Map->InteriorWalls.Add(Wall);
	const uint32 Solid = Map->ComputeHash();

	Map->InteriorWalls[0].bTraversable = true;
	TestNotEqual(TEXT("renderlo scavalcabile cambia l'hash"), Map->ComputeHash(), Solid);

	// ⛔ Abbassarlo NON lo rende scavalcabile: sono due campi indipendenti, ed è la clausola di `D-308`.
	Map->InteriorWalls[0].bTraversable = false;
	Map->InteriorWalls[0].Segment.WallType = ERTHexCoverType::Low;
	TestFalse(TEXT("un muretto non è scavalcabile per il fatto di essere basso"),
		Map->InteriorWalls[0].bTraversable);

	// E il formato dichiara la versione che porta il campo.
	TestEqual(TEXT("il formato è v15"), URTHexMapAsset::CurrentFormatVersion, 15);
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
