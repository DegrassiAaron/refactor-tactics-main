// Il perimetro di una regione (OVL-02, #1942): estrazione PURA, senza PIE.
//
// La DoD di #1942 chiede esattamente questo — «gli edge di perimetro si estraggono con un test headless,
// funzione pura» — e chiede tre casi nominati con tre asserzioni distinte: concava, buco, disconnessa. Che
// il perimetro si LEGGA a distanza tattica e' un giudizio visivo e resta al PIE; qui si conta.

#include "Misc/AutomationTest.h"
#include "Map/RTRegionBoundary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTMapVisuals.h"
#include "Algo/Reverse.h"
#include "UObject/UObjectArray.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Il disco di raggio `R` attorno all'origine, su un piano: la regione compatta di riferimento. */
	TSet<FRTCellId> Disc(const int32 Radius, const int32 Layer = 0)
	{
		TSet<FRTCellId> Cells;
		for (int32 X = -Radius; X <= Radius; ++X)
		{
			for (int32 Y = -Radius; Y <= Radius; ++Y)
			{
				if (FMath::Abs(X + Y) <= Radius)
				{
					Cells.Add(FRTCellId(X, Y, Layer));
				}
			}
		}
		return Cells;
	}

	/** Quanti edge di perimetro toccano una certa cella. */
	int32 EdgesOn(const TArray<FRTBoundaryEdge>& Edges, const FRTCellId& Cell)
	{
		int32 N = 0;
		for (const FRTBoundaryEdge& E : Edges) { if (E.Cell == Cell) { ++N; } }
		return N;
	}
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// Una cella sola: sei lati, tutti di perimetro
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundarySingleCellTest,
	"RefactorTactics.RegionBoundary.SingleCellHasSixEdges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundarySingleCellTest::RunTest(const FString&)
{
	const TSet<FRTCellId> One = { FRTCellId(0, 0, 0) };
	const TArray<FRTBoundaryEdge> Edges = URTRegionBoundaryLibrary::ExtractBoundaryEdges(One);

	// Il caso degenere che fissa la convenzione: nessun vicino appartiene alla regione, quindi tutti e sei i
	// lati guardano fuori.
	TestEqual(TEXT("una cella isolata ha sei lati di perimetro"), Edges.Num(), 6);

	// E ogni lato compare UNA volta sola: sei direzioni distinte, non sei copie della stessa.
	TSet<uint8> Dirs;
	for (const FRTBoundaryEdge& E : Edges) { Dirs.Add(static_cast<uint8>(E.Dir)); }
	TestEqual(TEXT("le sei direzioni sono distinte"), Dirs.Num(), 6);

	TestTrue(TEXT("una regione vuota non ha perimetro"),
		URTRegionBoundaryLibrary::ExtractBoundaryEdges(TSet<FRTCellId>()).Num() == 0);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// Gli edge INTERNI non producono ribbon — contati, non guardati
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryNoInteriorEdgesTest,
	"RefactorTactics.RegionBoundary.InteriorEdgesProduceNoRibbon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryNoInteriorEdgesTest::RunTest(const FString&)
{
	// 🔑 **La casella della DoD: «provato contando gli edge, non guardando».**
	//
	// Il disco di raggio 1 ha 7 celle. Se ogni lato di ogni cella producesse ribbon sarebbero 42 — ed e'
	// precisamente il difetto che rende il perimetro illeggibile. Il perimetro vero e' la corona di 6 celle
	// con 3 lati esposti ciascuna: 18.
	const TSet<FRTCellId> R1 = Disc(1);
	TestEqual(TEXT("il disco di raggio 1 ha sette celle"), R1.Num(), 7);

	const TArray<FRTBoundaryEdge> Edges = URTRegionBoundaryLibrary::ExtractBoundaryEdges(R1);
	TestEqual(TEXT("il perimetro e' 18, non 42"), Edges.Num(), 18);

	// La cella centrale e' circondata: zero lati di perimetro. E' l'asserzione piu' diretta sul difetto.
	TestEqual(TEXT("la cella centrale non contribuisce NESSUN edge"), EdgesOn(Edges, FRTCellId(0, 0, 0)), 0);

	// E ogni cella della corona ne contribuisce esattamente tre.
	for (const FRTCellId& Cell : R1)
	{
		if (Cell == FRTCellId(0, 0, 0)) { continue; }
		TestEqual(TEXT("ogni cella della corona espone tre lati"), EdgesOn(Edges, Cell), 3);
	}

	// Controprova sulla definizione: ogni edge restituito ha davvero il vicino FUORI dalla regione.
	for (const FRTBoundaryEdge& E : Edges)
	{
		TestFalse(TEXT("il vicino di un edge di perimetro non appartiene alla regione"),
			R1.Contains(URTHexLibrary::Neighbor(E.Cell, E.Dir)));
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// Concava
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryConcaveTest,
	"RefactorTactics.RegionBoundary.ConcaveRegion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryConcaveTest::RunTest(const FString&)
{
	// Una «C»: dal disco di raggio 1 si toglie una cella della corona. La rientranza e' concava, e i lati
	// che la guardano sono perimetro tanto quanto quelli esterni.
	TSet<FRTCellId> C = Disc(1);
	const FRTCellId Removed(1, 0, 0);
	TestTrue(TEXT("la cella da togliere apparteneva alla corona"), C.Contains(Removed));
	C.Remove(Removed);

	const TArray<FRTBoundaryEdge> Edges = URTRegionBoundaryLibrary::ExtractBoundaryEdges(C);

	// Togliendo una cella della corona: si perdono i suoi 3 lati esposti, e i 3 vicini che la toccavano
	// scoprono un lato ciascuno. 18 - 3 + 3 = 18.
	TestEqual(TEXT("il perimetro della concava resta 18"), Edges.Num(), 18);

	// 🔑 **La rientranza e' davvero perimetro**: il centro, che nella regione piena non contribuiva nulla,
	// ora espone il lato che guarda la cella tolta.
	TestEqual(TEXT("il centro ora espone UN lato: quello verso la rientranza"),
		EdgesOn(Edges, FRTCellId(0, 0, 0)), 1);

	// E resta connessa: una C e' una componente sola.
	TestEqual(TEXT("la concava e' una componente sola"),
		URTRegionBoundaryLibrary::ConnectedComponents(C).Num(), 1);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// Buco
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryHoleTest,
	"RefactorTactics.RegionBoundary.RegionWithHole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryHoleTest::RunTest(const FString&)
{
	// La ciambella: disco di raggio 2 senza il centro. Il buco e' interno e circondato — il caso in cui un
	// perimetro «solo esterno» sbaglierebbe in silenzio.
	TSet<FRTCellId> Donut = Disc(2);
	Donut.Remove(FRTCellId(0, 0, 0));

	const TArray<FRTBoundaryEdge> Edges = URTRegionBoundaryLibrary::ExtractBoundaryEdges(Donut);

	// 🔑 Il perimetro e' DUE anelli, non uno: l'esterno (12 celle da 3 lati = 36... la corona di raggio 2)
	// piu' i sei lati che affacciano sul buco.
	int32 EdgesFacingHole = 0;
	for (const FRTBoundaryEdge& E : Edges)
	{
		if (URTHexLibrary::Neighbor(E.Cell, E.Dir) == FRTCellId(0, 0, 0)) { ++EdgesFacingHole; }
	}
	TestEqual(TEXT("il buco e' circondato da SEI lati di perimetro"), EdgesFacingHole, 6);

	// I sei che affacciano sul buco appartengono alle sei celle dell'anello interno, uno ciascuna.
	TSet<FRTCellId> HoleRing;
	for (const FRTBoundaryEdge& E : Edges)
	{
		if (URTHexLibrary::Neighbor(E.Cell, E.Dir) == FRTCellId(0, 0, 0)) { HoleRing.Add(E.Cell); }
	}
	TestEqual(TEXT("sei celle distinte affacciano sul buco"), HoleRing.Num(), 6);

	// ⛔ E il buco NON spezza la regione: la ciambella resta una componente sola.
	TestEqual(TEXT("una ciambella e' una componente sola"),
		URTRegionBoundaryLibrary::ConnectedComponents(Donut).Num(), 1);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// Disconnessa: un perimetro per componente
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryDisconnectedTest,
	"RefactorTactics.RegionBoundary.DisconnectedRegionHasOnePerimeterEach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryDisconnectedTest::RunTest(const FString&)
{
	// Due isole lontane sullo stesso piano.
	TSet<FRTCellId> Two = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };   // isola A: due celle adiacenti
	Two.Add(FRTCellId(10, 0, 0));                                        // isola B: una cella sola

	const TArray<TArray<FRTCellId>> Components = URTRegionBoundaryLibrary::ConnectedComponents(Two);
	TestEqual(TEXT("due isole, due componenti"), Components.Num(), 2);

	// 🔑 **Un perimetro CIASCUNA, e non un mucchio solo.** Estratti per componente, i conti tornano: due
	// celle adiacenti espongono 10 lati (12 meno i 2 dell'edge condiviso), una cella sola ne espone 6.
	int32 TotalePerComponente = 0;
	for (const TArray<FRTCellId>& Component : Components)
	{
		const TArray<FRTBoundaryEdge> Edges = URTRegionBoundaryLibrary::ExtractBoundaryEdges(Component);
		if (Component.Num() == 1)
		{
			TestEqual(TEXT("l'isola da una cella ha sei lati"), Edges.Num(), 6);
		}
		else
		{
			TestEqual(TEXT("l'isola da due celle ha dieci lati, non dodici"), Edges.Num(), 10);
		}
		TotalePerComponente += Edges.Num();
	}

	// E la somma coincide con l'estrazione fatta su tutta la regione: separare in componenti non crea ne'
	// perde perimetro.
	TestEqual(TEXT("la somma dei perimetri e' il perimetro dell'insieme"),
		TotalePerComponente, URTRegionBoundaryLibrary::ExtractBoundaryEdges(Two).Num());
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// Multilayer: nessuna fusione, e nessuna tenda che scende
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryMultilayerTest,
	"RefactorTactics.RegionBoundary.SameXYOnDifferentLayersDoNotMerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryMultilayerTest::RunTest(const FString&)
{
	// La casella della DoD: «due regioni su Layer diversi con lo stesso X/Y producono DUE perimetri distinti
	// e nessuna fusione».
	TSet<FRTCellId> TwoFloors;
	TwoFloors.Add(FRTCellId(0, 0, 0));
	TwoFloors.Add(FRTCellId(0, 0, 1)); // stesso X/Y, piano diverso

	TestEqual(TEXT("stesso X/Y su piani diversi sono due celle diverse"), TwoFloors.Num(), 2);

	const TArray<TArray<FRTCellId>> Components = URTRegionBoundaryLibrary::ConnectedComponents(TwoFloors);
	TestEqual(TEXT("due piani, due componenti: nessuna fusione"), Components.Num(), 2);

	const TArray<FRTBoundaryEdge> Edges = URTRegionBoundaryLibrary::ExtractBoundaryEdges(TwoFloors);
	TestEqual(TEXT("sei lati per piano, dodici in tutto"), Edges.Num(), 12);

	// ⛔ **Nessuna tenda verticale verso il piano sottostante**: ogni edge nomina una cella della regione,
	// quindi il suo `Layer` e' quello di quella cella e non puo' appartenere a un altro piano. Non c'e' un
	// controllo che lo impedisce — non c'e' modo di esprimerlo.
	int32 SulPianoZero = 0;
	int32 SulPianoUno = 0;
	for (const FRTBoundaryEdge& E : Edges)
	{
		if (E.Cell.Layer == 0) { ++SulPianoZero; }
		if (E.Cell.Layer == 1) { ++SulPianoUno; }
	}
	TestEqual(TEXT("sei edge restano sul piano 0"), SulPianoZero, 6);
	TestEqual(TEXT("sei edge restano sul piano 1"), SulPianoUno, 6);

	// E un vicinato non attraversa mai un piano: e' la riga da cui discende tutto il resto.
	for (const ERTHexDirection Dir : { ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
			ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE })
	{
		TestEqual(TEXT("Neighbor conserva il Layer"),
			URTHexLibrary::Neighbor(FRTCellId(0, 0, 3), Dir).Layer, 3);
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// L'ordine e' stabile, e non e' quello del TSet
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryStableOrderTest,
	"RefactorTactics.RegionBoundary.OrderIsStableAcrossInsertionOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryStableOrderTest::RunTest(const FString&)
{
	// 🔴 **Il difetto che questo test intercetta e' invisibile finche' non morde.** Iterare un `TSet` da' un
	// ordine che dipende dall'hash e dalla storia degli inserimenti: la stessa regione costruita in ordine
	// diverso darebbe due uscite diverse, e un consumatore che si fidasse dell'ordine funzionerebbe finche'
	// qualcuno non riordina un `for`.
	TArray<FRTCellId> Ascending;
	for (int32 X = -1; X <= 1; ++X)
	{
		for (int32 Y = -1; Y <= 1; ++Y)
		{
			if (FMath::Abs(X + Y) <= 1) { Ascending.Add(FRTCellId(X, Y, 0)); }
		}
	}
	TArray<FRTCellId> Descending = Ascending;
	Algo::Reverse(Descending);

	const TArray<FRTBoundaryEdge> A = URTRegionBoundaryLibrary::ExtractBoundaryEdges(Ascending);
	const TArray<FRTBoundaryEdge> B = URTRegionBoundaryLibrary::ExtractBoundaryEdges(Descending);

	TestEqual(TEXT("stesso numero di edge"), A.Num(), B.Num());
	bool bIdentical = A.Num() == B.Num();
	for (int32 I = 0; bIdentical && I < A.Num(); ++I)
	{
		bIdentical = A[I] == B[I];
	}
	TestTrue(TEXT("l'ordine di inserimento non cambia l'uscita"), bIdentical);

	// E l'ordine dichiarato e' davvero quello: Layer, poi X, poi Y, poi Dir.
	for (int32 I = 1; I < A.Num(); ++I)
	{
		const bool bOrdered = URTRegionBoundaryLibrary::CellLess(A[I - 1].Cell, A[I].Cell)
			|| (A[I - 1].Cell == A[I].Cell
				&& static_cast<uint8>(A[I - 1].Dir) < static_cast<uint8>(A[I].Dir));
		TestTrue(TEXT("l'uscita e' ordinata per Layer, X, Y, Dir"), bOrdered);
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// L'estrazione non crea nulla: nessun Actor, nessun component
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryNoActorPerEdgeTest,
	"RefactorTactics.RegionBoundary.ExtractionAllocatesNoObject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryNoActorPerEdgeTest::RunTest(const FString&)
{
	// La casella «nessun Actor e nessun component per edge» si misura di solito sul delta di actor in un
	// mondo. Qui l'affermazione e' piu' forte e non serve un mondo: l'estrazione e' una funzione pura che
	// restituisce `FRTBoundaryEdge`, che e' una `USTRUCT` di valore. Non c'e' una `UObject` da contare.
	//
	// Il delta di OGGETTI lo prova comunque, ed e' l'oracolo che cade se qualcuno ci mettesse dentro una
	// `NewObject` o uno spawn.
	const int32 ObjectsPrima = GUObjectArray.GetObjectArrayNum();

	// Una regione grande: se esistesse un oggetto per edge, il salto sarebbe di centinaia.
	const TSet<FRTCellId> Big = Disc(6);
	const TArray<FRTBoundaryEdge> Edges = URTRegionBoundaryLibrary::ExtractBoundaryEdges(Big);
	TestTrue(TEXT("la regione grande ha un perimetro non banale"), Edges.Num() >= 36);

	const int32 ObjectsDopo = GUObjectArray.GetObjectArrayNum();
	TestEqual(TEXT("estrarre un perimetro non alloca NESSUNA UObject"), ObjectsDopo, ObjectsPrima);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────
// L'altezza della ribbon e' derivata, non scritta
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBoundaryRibbonHeightTest,
	"RefactorTactics.RegionBoundary.RibbonHeightIsDerived",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBoundaryRibbonHeightTest::RunTest(const FString&)
{
	// Gli `static_assert` in `RTMapVisuals.h` gia' fermano la build se la frazione esce dalla banda della
	// spec. Questo test aggiunge cio' che uno `static_assert` non dice: che l'altezza **segua** `H` invece di
	// essere un numero che ci somiglia.
	TestEqual(TEXT("l'altezza e' la frazione di H, non un centimetro scritto"),
		RTBoundaryRibbonHeight, RTBoundaryRibbonHeightInH * RTCellLayerHeightRef);

	TestTrue(TEXT("resta nella baseline 30-35 cm della spec v0.2"),
		RTBoundaryRibbonHeight >= 30.f && RTBoundaryRibbonHeight <= 35.f);

	TestTrue(TEXT("la ribbon torreggia sulla pila di lettura invece di infilarcisi"),
		RTBoundaryRibbonHeight > (2.5f) * 4.f); // 2.5 = RTLiftPreview - RTCellTopZ

	TestEqual(TEXT("nasce sulla superficie del Layer, non sotto"), RTBoundaryRibbonBaseZ, RTCellTopZ);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
