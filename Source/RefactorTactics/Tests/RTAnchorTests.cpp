#include "Misc/AutomationTest.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCoverLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Il prefisso `Anchor` non e' decorativo: la unity build fonde i namespace anonimi di piu' `.cpp` nella
	// stessa translation unit, e un nome generico collide con l'omonimo di un altro file di test (`#1530`).
	constexpr float AnchorTestHexSize = 100.0f;

	/** Tolleranza per i confronti CONTRO IL MONDO. La grammatica e' intera; il mondo, che la verifica, no. */
	constexpr double AnchorWorldEpsilon = 0.01;

	/** Il centro-mondo di una cella, in 2D: il piano basta, gli anchor non hanno quota. */
	FVector2D AnchorCellCenter(const FRTCellId& Cell)
	{
		const FVector C = URTHexLibrary::AxialToWorld(Cell, FVector::ZeroVector, AnchorTestHexSize, 0.0f);
		return FVector2D(C.X, C.Y);
	}

	/** Dove cade un anchor NEL MONDO, passando dalla sua cella. E' l'oracolo indipendente. */
	FVector2D AnchorWorld(const FRTAnchorRef& Ref)
	{
		return AnchorCellCenter(Ref.Cell)
			+ URTGeometryGrammarLibrary::AnchorLocal(Ref, AnchorTestHexSize);
	}
}

/**
 * LA PALETTE ESISTE, ED E' DI TREDICI — `#1893`, `GEO-5` di `D-288`.
 *
 * ⚠️ Il numero non e' un'opinione: e' il centro piu' i dodici confini di settore che
 * `URTHexOccupancyLibrary::SectorBoundaryPoints` gia' produce. Se un giorno la lattice cambiasse, questo
 * test cade qui invece che a schermo tre giorni dopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorPaletteIsThirteenTest,
	"RefactorTactics.Anchor.PaletteIsThirteen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorPaletteIsThirteenTest::RunTest(const FString&)
{
	const FRTCellId Cell(3, -1, 0);

	TArray<FRTAnchorRef> Anchors;
	URTGeometryGrammarLibrary::AnchorsOfCell(Cell, Anchors);

	TestEqual(TEXT("la palette ha tredici anchor"), Anchors.Num(), RT_AnchorsPerCell);

	int32 Centers = 0, Vertices = 0, EdgeMids = 0;
	for (const FRTAnchorRef& A : Anchors)
	{
		TestTrue(TEXT("ogni anchor appartiene alla cella chiesta"), A.Cell == Cell);
		switch (A.Kind)
		{
		case ERTAnchorKind::Center:  ++Centers;  TestEqual(TEXT("il centro ha indice zero"), A.Index, 0); break;
		case ERTAnchorKind::Vertex:  ++Vertices; break;
		case ERTAnchorKind::EdgeMid: ++EdgeMids; break;
		}
		if (A.Kind != ERTAnchorKind::Center)
		{
			TestTrue(TEXT("gli indici stanno fra 0 e 5"), A.Index >= 0 && A.Index <= 5);
		}
	}

	TestEqual(TEXT("un centro"), Centers, 1);
	TestEqual(TEXT("sei vertici"), Vertices, 6);
	TestEqual(TEXT("sei punti medi"), EdgeMids, 6);

	// E sono tutti DISTINTI: un elenco di tredici copie dello stesso anchor passerebbe i conteggi sopra.
	TSet<FString> Distinct;
	for (const FRTAnchorRef& A : Anchors)
	{
		Distinct.Add(A.ToString());
	}
	TestEqual(TEXT("tredici anchor distinti"), Distinct.Num(), RT_AnchorsPerCell);

	return true;
}

/**
 * LE POSIZIONI NON SONO NUOVE: sono i confini di settore che il repository gia' produce.
 *
 * Il vertice `k` e' il confine `2k`, il punto medio del lato `j` e' il confine `2j + 1`, il centro e'
 * l'origine. Questo test e' cio' che impedisce di ricalcolare i coseni una seconda volta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorLocalMatchesSectorBoundariesTest,
	"RefactorTactics.Anchor.LocalMatchesSectorBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorLocalMatchesSectorBoundariesTest::RunTest(const FString&)
{
	TArray<FVector2D> Boundary;
	URTHexOccupancyLibrary::SectorBoundaryPoints(AnchorTestHexSize, Boundary);
	TestEqual(TEXT("dodici confini di settore"), Boundary.Num(), 12);
	if (Boundary.Num() != 12)
	{
		return false;
	}

	const FRTCellId Cell(0, 0, 0);

	const FVector2D Center = URTGeometryGrammarLibrary::AnchorLocal(
		FRTAnchorRef(Cell, ERTAnchorKind::Center), AnchorTestHexSize);
	TestTrue(TEXT("il centro e' l'origine locale"), Center.Equals(FVector2D::ZeroVector, AnchorWorldEpsilon));

	for (int32 K = 0; K < 6; ++K)
	{
		const FVector2D V = URTGeometryGrammarLibrary::AnchorLocal(
			FRTAnchorRef(Cell, ERTAnchorKind::Vertex, K), AnchorTestHexSize);
		if (!TestTrue(FString::Printf(TEXT("il vertice %d e' il confine %d"), K, 2 * K),
			V.Equals(Boundary[2 * K], AnchorWorldEpsilon)))
		{
			return false;
		}

		const FVector2D E = URTGeometryGrammarLibrary::AnchorLocal(
			FRTAnchorRef(Cell, ERTAnchorKind::EdgeMid, K), AnchorTestHexSize);
		if (!TestTrue(FString::Printf(TEXT("il punto medio %d e' il confine %d"), K, 2 * K + 1),
			E.Equals(Boundary[2 * K + 1], AnchorWorldEpsilon)))
		{
			return false;
		}

		// ⚠️ SECONDO ORACOLO, e indipendente dal primo: `EdgeMidpointWorld` esiste da prima di questa issue
		// e per un altro scopo. Se `EdgeMid K` fosse il punto medio del lato SBAGLIATO, i confini di settore
		// non se ne accorgerebbero — sono dodici punti su un cerchio, e sbagliare il verso li permuta senza
		// spostarli. Questo lega l'indice del punto medio alla DIREZIONE, che e' cio' che la
		// canonicalizzazione poi usa.
		const FVector EdgeW = URTHexLibrary::EdgeMidpointWorld(
			Cell, URTHexLibrary::DirectionForEdgeIndex(K), FVector::ZeroVector, AnchorTestHexSize, 0.0f);
		if (!TestTrue(FString::Printf(TEXT("il punto medio %d e' il lato in direzione %d"), K,
				static_cast<int32>(URTHexLibrary::DirectionForEdgeIndex(K))),
			E.Equals(FVector2D(EdgeW.X, EdgeW.Y), AnchorWorldEpsilon)))
		{
			return false;
		}
	}

	return true;
}

/**
 * IL CUORE — `GEO-5`: LO STESSO PUNTO, NOMINATO DA DUE CELLE, HA UNA CHIAVE SOLA.
 *
 * 🔑 **L'oracolo e' IL MONDO, e la funzione non lo guarda.** La canonicalizzazione e' combinatoria — non
 * misura distanze, per non reintrodurre l'epsilon che la grammatica esiste per evitare — quindi il test
 * deve verificarla con qualcosa di indipendente: due riferimenti che cadono nello stesso punto-mondo
 * DEVONO avere la stessa chiave, e due che cadono in punti diversi NON devono averla. E' la stessa
 * disciplina di `RefactorTactics.Hex.EdgeIndexMatchesNeighbourDirection`, che deriva dal mondo la
 * corrispondenza fra indice di bordo e direzione.
 *
 * Il giro copre un intorno completo, non una coppia scelta bene: sei celle attorno all'origine, tutti e
 * tredici gli anchor di ciascuna.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorCanonicalKeyAgreesWithTheWorldTest,
	"RefactorTactics.Anchor.CanonicalKeyAgreesWithTheWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorCanonicalKeyAgreesWithTheWorldTest::RunTest(const FString&)
{
	// L'origine e i suoi sei vicini: ogni bordo e ogni vertice dell'origine e' condiviso da qualcuno del giro.
	TArray<FRTCellId> Cells;
	Cells.Add(FRTCellId(0, 0, 0));
	Cells.Append(URTHexLibrary::Neighbors(FRTCellId(0, 0, 0)));
	TestEqual(TEXT("sette celle nell'intorno"), Cells.Num(), 7);

	// Tutti gli anchor di tutte le celle, con la loro posizione-mondo e la loro chiave.
	TArray<FRTAnchorRef> All;
	for (const FRTCellId& C : Cells)
	{
		TArray<FRTAnchorRef> Some;
		URTGeometryGrammarLibrary::AnchorsOfCell(C, Some);
		All.Append(Some);
	}
	TestEqual(TEXT("sette celle per tredici anchor"), All.Num(), 7 * RT_AnchorsPerCell);
	if (All.Num() == 0)
	{
		return false;
	}

	int32 Coincident = 0;
	for (int32 I = 0; I < All.Num(); ++I)
	{
		for (int32 J = I + 1; J < All.Num(); ++J)
		{
			const bool bSamePoint = AnchorWorld(All[I]).Equals(AnchorWorld(All[J]), AnchorWorldEpsilon);
			const bool bSameKey =
				URTGeometryGrammarLibrary::CanonicalAnchor(All[I])
				== URTGeometryGrammarLibrary::CanonicalAnchor(All[J]);

			if (bSamePoint)
			{
				++Coincident;
			}

			if (bSamePoint != bSameKey)
			{
				AddError(FString::Printf(
					TEXT("%s e %s: stesso punto=%d, stessa chiave=%d (canoniche: %s e %s)"),
					*All[I].ToString(), *All[J].ToString(), bSamePoint ? 1 : 0, bSameKey ? 1 : 0,
					*URTGeometryGrammarLibrary::CanonicalAnchor(All[I]).ToString(),
					*URTGeometryGrammarLibrary::CanonicalAnchor(All[J]).ToString()));
				return false;
			}
		}
	}

	// ⚠️ La controprova: senza di questa il test passerebbe con una funzione che non trova MAI due anchor
	// coincidenti, cioe' con la canonicalizzazione spenta. L'intorno di sette celle ne ha, e sono tanti.
	TestTrue(FString::Printf(TEXT("l'intorno contiene coincidenze da risolvere (trovate %d)"), Coincident),
		Coincident > 20);

	return true;
}

/**
 * LA CHIAVE E' IDEMPOTENTE E TOTALE — la proprieta' che, non essendo persistita, sostituisce la persistenza.
 *
 * ⚠️ `GEO-5` sceglie di NON serializzare il riferimento; il prezzo dichiarato e' che la funzione dev'essere
 * stabile per sempre. Un test che la fissa e' cio' che rende quel prezzo pagato invece che sperato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorCanonicalIsIdempotentAndTotalTest,
	"RefactorTactics.Anchor.CanonicalIsIdempotentAndTotal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorCanonicalIsIdempotentAndTotalTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Cells = {
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(-2, 3, 0), FRTCellId(5, -4, 0),
		FRTCellId(0, 0, 1), FRTCellId(-1, -1, 2)
	};

	for (const FRTCellId& C : Cells)
	{
		TArray<FRTAnchorRef> Anchors;
		URTGeometryGrammarLibrary::AnchorsOfCell(C, Anchors);
		if (!TestEqual(TEXT("la palette e' totale su ogni cella"), Anchors.Num(), RT_AnchorsPerCell))
		{
			return false;
		}

		for (const FRTAnchorRef& A : Anchors)
		{
			const FRTAnchorRef Once = URTGeometryGrammarLibrary::CanonicalAnchor(A);
			const FRTAnchorRef Twice = URTGeometryGrammarLibrary::CanonicalAnchor(Once);
			if (!TestTrue(FString::Printf(TEXT("idempotente su %s"), *A.ToString()), Once == Twice))
			{
				return false;
			}

			// Il rappresentante e' un anchor VERO, non una coordinata inventata: sta sullo stesso Layer e
			// il suo indice e' nella palette.
			TestEqual(TEXT("il rappresentante resta sul suo Layer"), Once.Cell.Layer, A.Cell.Layer);
			TestTrue(TEXT("il rappresentante ha un indice legale"),
				Once.Kind == ERTAnchorKind::Center ? Once.Index == 0 : (Once.Index >= 0 && Once.Index <= 5));
			TestTrue(TEXT("il genere non cambia: un vertice resta un vertice"), Once.Kind == A.Kind);
		}
	}

	// Due celle diverse hanno centri diversi: il centro non e' condiviso con nessuno.
	const FRTAnchorRef C1(FRTCellId(0, 0, 0), ERTAnchorKind::Center);
	const FRTAnchorRef C2(FRTCellId(1, 0, 0), ERTAnchorKind::Center);
	TestTrue(TEXT("i centri di due celle non collidono"),
		URTGeometryGrammarLibrary::CanonicalAnchor(C1) != URTGeometryGrammarLibrary::CanonicalAnchor(C2));

	// E un centro resta se stesso: appartiene a una cella sola, quindi non ha nessun altro con cui accordarsi.
	TestTrue(TEXT("il centro e' il rappresentante di se stesso"),
		URTGeometryGrammarLibrary::CanonicalAnchor(C1) == C1);

	return true;
}

/**
 * DA UNA COPPIA DI ANCHOR AL SEGMENTO — la via che non passa dal mouse.
 *
 * `C→V` e `C→E` sono due delle configurazioni che il documento del 2026-08-31 chiede esplicitamente, e qui
 * si costruiscono senza inventare un secondo modo di dire un muro: il risultato deve coincidere con quello
 * che `SnapToGrammar` produce per il gesto equivalente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorSegmentBetweenAnchorsTest,
	"RefactorTactics.Anchor.SegmentBetweenAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorSegmentBetweenAnchorsTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);
	const FRTAnchorRef Center(Cell, ERTAnchorKind::Center);

	// `C→E`: il centro e il punto medio di un lato. Sempre esprimibile — e' un asse tattico per definizione.
	for (int32 J = 0; J < 6; ++J)
	{
		const FRTAnchorRef E(Cell, ERTAnchorKind::EdgeMid, J);
		FRTGeometrySegment S;
		if (!TestTrue(FString::Printf(TEXT("C verso il punto medio %d e' esprimibile"), J),
			URTGeometryGrammarLibrary::SegmentBetweenAnchors(Center, E, AnchorTestHexSize, S)))
		{
			return false;
		}
		TestTrue(TEXT("il segmento prodotto e' in grammatica"),
			URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::None);

		// Gli estremi cadono DAVVERO sui due anchor chiesti: e' l'unica formulazione misurabile di
		// «il segmento e' quello che ho chiesto».
		const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(S, AnchorTestHexSize);
		if (TestEqual(TEXT("due estremi"), Line.Points.Num(), 2))
		{
			const FVector2D A0 = URTGeometryGrammarLibrary::AnchorLocal(Center, AnchorTestHexSize);
			const FVector2D A1 = URTGeometryGrammarLibrary::AnchorLocal(E, AnchorTestHexSize);
			const bool bForward = Line.Points[0].Equals(A0, AnchorWorldEpsilon)
				&& Line.Points[1].Equals(A1, AnchorWorldEpsilon);
			const bool bBackward = Line.Points[0].Equals(A1, AnchorWorldEpsilon)
				&& Line.Points[1].Equals(A0, AnchorWorldEpsilon);
			TestTrue(FString::Printf(TEXT("gli estremi sono i due anchor chiesti (lato %d)"), J),
				bForward || bBackward);
		}
	}

	// `C→V`: il centro e un vertice. Anche questa e' un asse tattico.
	for (int32 K = 0; K < 6; ++K)
	{
		const FRTAnchorRef V(Cell, ERTAnchorKind::Vertex, K);
		FRTGeometrySegment S;
		if (!TestTrue(FString::Printf(TEXT("C verso il vertice %d e' esprimibile"), K),
			URTGeometryGrammarLibrary::SegmentBetweenAnchors(Center, V, AnchorTestHexSize, S)))
		{
			return false;
		}
		TestTrue(TEXT("il segmento prodotto e' in grammatica"),
			URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::None);
	}

	// L'ORDINE DI AUTHORING NON CONTA: e' l'invariante che il documento chiede, e qui e' gratis perche'
	// `operator==` tratta gli estremi come coppia non ordinata. Il test lo FISSA, cosi' resta gratis.
	{
		const FRTAnchorRef V(Cell, ERTAnchorKind::Vertex, 1);
		FRTGeometrySegment Forward, Backward;
		const bool bF = URTGeometryGrammarLibrary::SegmentBetweenAnchors(Center, V, AnchorTestHexSize, Forward);
		const bool bB = URTGeometryGrammarLibrary::SegmentBetweenAnchors(V, Center, AnchorTestHexSize, Backward);
		TestTrue(TEXT("esprimibile in entrambi i versi"), bF && bB);
		if (bF && bB)
		{
			TestTrue(TEXT("e produce lo STESSO segmento"), Forward == Backward);
		}
	}

	// UN SEGMENTO DEGENERE NON E' UN SEGMENTO: due volte lo stesso anchor non produce niente.
	{
		FRTGeometrySegment S;
		TestFalse(TEXT("un anchor con se stesso non e' un segmento"),
			URTGeometryGrammarLibrary::SegmentBetweenAnchors(Center, Center, AnchorTestHexSize, S));
	}

	return true;
}

/**
 * LE VENTIQUATTRO COPPIE CHE NESSUN ASSE PORTA — `GEO-8`.
 *
 * `RefactorTactics.Geometry.NotablePairsOnTacticalAxes` misura che delle 78 coppie di punti notevoli 54
 * stanno su un asse tattico e 24 no. Quel test lo misura sui PUNTI; questo fissa la stessa proprieta'
 * sull'API che l'authoring usera', perche' e' li' che il rifiuto deve diventare visibile invece di
 * produrre un altro muro in silenzio.
 *
 * ⚠️ Le 24 sono tutte e sole le **vertice ↔ punto-medio non adiacente**: un vertice appartiene a due lati,
 * quindi dei sei punti medi ne ha due adiacenti e quattro no — `6 × 4 = 24`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorInexpressiblePairsAreRefusedTest,
	"RefactorTactics.Anchor.InexpressiblePairsAreRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorInexpressiblePairsAreRefusedTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);

	TArray<FRTAnchorRef> Anchors;
	URTGeometryGrammarLibrary::AnchorsOfCell(Cell, Anchors);
	if (!TestEqual(TEXT("tredici anchor"), Anchors.Num(), RT_AnchorsPerCell))
	{
		return false;
	}

	int32 Expressible = 0, Refused = 0;
	for (int32 I = 0; I < Anchors.Num(); ++I)
	{
		for (int32 J = I + 1; J < Anchors.Num(); ++J)
		{
			FRTGeometrySegment S;
			if (URTGeometryGrammarLibrary::SegmentBetweenAnchors(Anchors[I], Anchors[J], AnchorTestHexSize, S))
			{
				++Expressible;
				TestTrue(TEXT("cio' che si esprime e' in grammatica"),
					URTGeometryGrammarLibrary::ValidateSegment(S) == ERTGeometryViolation::None);
			}
			else
			{
				++Refused;
			}
		}
	}

	TestEqual(TEXT("coppie totali"), Expressible + Refused, 78);
	TestEqual(TEXT("coppie esprimibili"), Expressible, 54);
	TestEqual(TEXT("coppie rifiutate"), Refused, 24);

	// E le rifiutate sono quelle previste, non ventiquattro a caso: un vertice contro un punto medio che
	// non gli appartiene.
	for (int32 K = 0; K < 6; ++K)
	{
		const FRTAnchorRef V(Cell, ERTAnchorKind::Vertex, K);
		for (int32 J = 0; J < 6; ++J)
		{
			// Il vertice `K` appartiene ai lati `K-1` e `K`.
			const bool bAdjacent = (J == K) || (J == ((K + 5) % 6));
			const FRTAnchorRef E(Cell, ERTAnchorKind::EdgeMid, J);
			FRTGeometrySegment S;
			const bool bOk = URTGeometryGrammarLibrary::SegmentBetweenAnchors(V, E, AnchorTestHexSize, S);
			if (!TestEqual(FString::Printf(TEXT("V%d verso E%d: adiacente=%d"), K, J, bAdjacent ? 1 : 0),
				bOk, bAdjacent))
			{
				return false;
			}
		}
	}

	return true;
}


/**
 * IL BORDO CONDIVISO SCRITTO DUE VOLTE — `GEO-7` di `D-288`.
 *
 * `URTGeometryBakeLibrary::BakeCell` non scrive mai la seconda faccia, e il test
 * `RefactorTactics.GeometryBake.RebakeTouchesOnlyTheInvestedRegion` lo protegge: *«la seconda scrittura non
 * aggiunge nulla al gioco e aggiunge un elemento all'array che entra in `ComputeHash`»*.
 *
 * 🔴 **Ma l'autore puo' scriverla lo stesso** — ridisegnando dalla cella opposta, o aggiungendo la
 * copertura dal pannello Details — e nessuna regola se ne accorge: quella esistente, *«coperture
 * sovrapposte sul bordo»*, guarda DENTRO una cella e non FRA due. Il risultato e' una mappa che si gioca
 * identica a un'altra e ne differisce per hash, che e' il falso positivo contro `replay divergence = 0`
 * che l'intestazione di quel test nomina.
 *
 * E' un **Warning** e non un Error, con il precedente di forma gia' nel file: *«Warning: transizione
 * ridondante tra celle gia' adiacenti»*. La seconda faccia e' legale — `CoverBetween` la risolve tenendo la
 * piu' alta — ed e' ridondante, non illegale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorRedundantSharedFaceIsWarnedTest,
	"RefactorTactics.Anchor.RedundantSharedFaceIsWarned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorRedundantSharedFaceIsWarnedTest::RunTest(const FString&)
{
	const FRTCellId Origin(0, 0, 0);
	const ERTHexDirection Dir = ERTHexDirection::E;
	const FRTCellId Far = URTHexLibrary::Neighbor(Origin, Dir);
	const ERTHexDirection Back = URTHexLibrary::OppositeDirection(Dir);

	auto CountRedundant = [](URTHexMapAsset* Map)
	{
		int32 N = 0;
		for (const FString& Line : Map->ValidateMap())
		{
			if (Line.Contains(TEXT("ridondante")) && Line.Contains(TEXT("condiviso")))
			{
				++N;
			}
		}
		return N;
	};

	auto MakePair = [&Origin, &Far](URTHexMapAsset*& Map)
	{
		Map = NewObject<URTHexMapAsset>();
		FRTHexCellData A; A.Id = Origin; Map->AddOrUpdateCell(A);
		FRTHexCellData B; B.Id = Far;    Map->AddOrUpdateCell(B);
	};

	auto CoverOn = [](URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, ERTHexCoverType Type)
	{
		FRTHexCellData Cell = *Map->FindCell(Id);
		Cell.Covers.Add(FRTHexCover(Edge, Type, 50));
		Map->AddOrUpdateCell(Cell);
	};

	// 1. UNA faccia sola: e' il caso normale, e non deve segnalare niente.
	{
		URTHexMapAsset* Map = nullptr; MakePair(Map);
		CoverOn(Map, Origin, Dir, ERTHexCoverType::High);
		TestEqual(TEXT("una faccia sola non e' ridondante"), CountRedundant(Map), 0);
		TestEqual(TEXT("e la mappa resta valida"), Map->ValidateMap().Num(), 0);
	}

	// 2. LE DUE facce dello stesso bordo condiviso: una segnalazione, e una sola.
	{
		URTHexMapAsset* Map = nullptr; MakePair(Map);
		CoverOn(Map, Origin, Dir, ERTHexCoverType::High);
		CoverOn(Map, Far, Back, ERTHexCoverType::High);
		TestEqual(TEXT("le due facce dello stesso bordo sono una segnalazione sola"), CountRedundant(Map), 1);
	}

	// 3. ⚠️ La stessa cosa con TIPI DIVERSI: resta una segnalazione sola e non diventa un errore. Non e' un
	//    conflitto da risolvere qui — `CoverBetween` tiene gia' la piu' alta, deterministicamente.
	{
		URTHexMapAsset* Map = nullptr; MakePair(Map);
		CoverOn(Map, Origin, Dir, ERTHexCoverType::Low);
		CoverOn(Map, Far, Back, ERTHexCoverType::High);
		TestEqual(TEXT("tipi diversi sulle due facce: sempre una segnalazione"), CountRedundant(Map), 1);
	}

	// 4. LA CONTROPROVA che rende il test discriminante: due coperture che NON sono lo stesso bordo. Senza,
	//    una regola che segnala qualunque coppia di coperture passerebbe i tre casi sopra.
	{
		URTHexMapAsset* Map = nullptr; MakePair(Map);
		CoverOn(Map, Origin, ERTHexDirection::NE, ERTHexCoverType::High);
		CoverOn(Map, Far, ERTHexDirection::NE, ERTHexCoverType::High);
		TestEqual(TEXT("due bordi diversi non sono ridondanti"), CountRedundant(Map), 0);
	}

	// 5. E il vicino che NON esiste nella mappa non produce falsi positivi: la faccia opposta non c'e'.
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		FRTHexCellData A; A.Id = Origin; Map->AddOrUpdateCell(A);
		FRTHexCellData Cell = *Map->FindCell(Origin);
		Cell.Covers.Add(FRTHexCover(Dir, ERTHexCoverType::High, 50));
		Map->AddOrUpdateCell(Cell);
		TestEqual(TEXT("senza la cella vicina non c'e' ridondanza"), CountRedundant(Map), 0);
	}

	// 6. IL FATTO CHE MOTIVA LA REGOLA: le due mappe si giocano identiche e hashano diverso. Se un giorno
	//    l'hash canonicalizzasse il bordo, questa asserzione cade e la regola va ripensata — ed e' bene che
	//    cada qui.
	{
		URTHexMapAsset* One = nullptr; MakePair(One);
		CoverOn(One, Origin, Dir, ERTHexCoverType::High);

		URTHexMapAsset* Two = nullptr; MakePair(Two);
		CoverOn(Two, Origin, Dir, ERTHexCoverType::High);
		CoverOn(Two, Far, Back, ERTHexCoverType::High);

		TestEqual(TEXT("la barriera fra le due celle e' la stessa"),
			static_cast<int32>(URTHexCoverLibrary::CoverBetween(One, Origin, Far)),
			static_cast<int32>(URTHexCoverLibrary::CoverBetween(Two, Origin, Far)));
		TestNotEqual(TEXT("ma l'hash no: e' il difetto che la segnalazione rende visibile"),
			One->ComputeHash(), Two->ComputeHash());
	}

	return true;
}


/**
 * I DUE RAMI CHE LA PRIMA STESURA AVEVA SCRITTO SENZA PROVARLI — trovati rileggendo il diff.
 *
 * ⚠️ Non sono casi esotici: sono le due porte da cui entra un dato costruito a mano, ed erano l'unico
 * codice di questa issue che nessun test faceva cadere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorRefusesWhatItCannotSayTest,
	"RefactorTactics.Anchor.RefusesWhatItCannotSay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorRefusesWhatItCannotSayTest::RunTest(const FString&)
{
	// 1. DUE CELLE DIVERSE non hanno un sistema di coordinate comune in cui dire un segmento: la grammatica
	//    e' definita per cella, e un muro lungo e' piu' segmenti — uno per cella, via
	//    `URTHexLibrary::SplitSegmentAcrossCells`. Chiederlo qui deve fallire, non produrre un segmento
	//    misurato in due sistemi diversi.
	{
		const FRTAnchorRef Here(FRTCellId(0, 0, 0), ERTAnchorKind::Center);
		const FRTAnchorRef There(FRTCellId(1, 0, 0), ERTAnchorKind::EdgeMid, 3);
		FRTGeometrySegment S;
		TestFalse(TEXT("due anchor di celle diverse non fanno un segmento"),
			URTGeometryGrammarLibrary::SegmentBetweenAnchors(Here, There, AnchorTestHexSize, S));
	}

	// 2. Anche due celle sullo stesso posto ma su LAYER diversi: non sono adiacenti per definizione, e la
	//    geometria di un piano non descrive l'altro.
	{
		const FRTAnchorRef Ground(FRTCellId(0, 0, 0), ERTAnchorKind::Center);
		const FRTAnchorRef Above(FRTCellId(0, 0, 1), ERTAnchorKind::Vertex, 0);
		FRTGeometrySegment S;
		TestFalse(TEXT("due anchor su layer diversi non fanno un segmento"),
			URTGeometryGrammarLibrary::SegmentBetweenAnchors(Ground, Above, AnchorTestHexSize, S));
	}

	// 3. UN CENTRO CON UN INDICE SPORCO: il centro e' uno solo, quindi il suo indice non significa niente.
	//    Il costruttore lo azzera, ma un `FRTAnchorRef` riempito campo per campo — da una deserializzazione,
	//    o da chi lo costruisce a mano — puo' portarselo dietro. La chiave canonica DEVE normalizzarlo,
	//    altrimenti due nomi dello stesso centro non si riconoscerebbero.
	{
		FRTAnchorRef Dirty;
		Dirty.Cell = FRTCellId(2, -2, 0);
		Dirty.Kind = ERTAnchorKind::Center;
		Dirty.Index = 4; // non passa dal costruttore: e' proprio il caso da coprire

		const FRTAnchorRef Clean(FRTCellId(2, -2, 0), ERTAnchorKind::Center);
		TestTrue(TEXT("il centro sporco e quello pulito hanno la stessa chiave"),
			URTGeometryGrammarLibrary::CanonicalAnchor(Dirty)
				== URTGeometryGrammarLibrary::CanonicalAnchor(Clean));
		TestEqual(TEXT("e la chiave porta indice zero"),
			URTGeometryGrammarLibrary::CanonicalAnchor(Dirty).Index, 0);
	}

	// 4. E un indice fuori intervallo su un vertice non fa uscire dall'array dei confini: si riduce, come
	//    fa gia' `DirectionForEdgeIndex` con il suo modulo positivo.
	{
		FRTAnchorRef OutOfRange;
		OutOfRange.Cell = FRTCellId(0, 0, 0);
		OutOfRange.Kind = ERTAnchorKind::Vertex;
		OutOfRange.Index = 7; // = 1

		const FRTAnchorRef Wrapped(FRTCellId(0, 0, 0), ERTAnchorKind::Vertex, 1);
		TestTrue(TEXT("un indice fuori intervallo si riduce invece di sfondare"),
			URTGeometryGrammarLibrary::AnchorLocal(OutOfRange, AnchorTestHexSize)
				.Equals(URTGeometryGrammarLibrary::AnchorLocal(Wrapped, AnchorTestHexSize), AnchorWorldEpsilon));
	}

	return true;
}

/**
 * 🔴 IL PIN DEL DIFETTO — `#1895`, e serve a FISSARE O SMENTIRE una deduzione.
 *
 * Il corpo dell'issue dichiara, e lo dichiara onestamente come non provato: *«`SnapToGrammar` prova tutti e
 * sei gli assi e tiene l'asse che sbaglia meno; per una delle ventiquattro coppie non ha un ramo di rifiuto
 * — proietta sull'asse meno sbagliato e produce un muro legale e DIVERSO da quello chiesto. Chi disegna vede
 * una linea comparire e non ha modo di sapere che non e' la sua.»*
 *
 * ⚠️ **Nessun test esistente lo guardava.** `InexpressiblePairsAreRefused` misura le 78 coppie su
 * `SegmentBetweenAnchors`, che le ventiquattro le **rifiuta** per costruzione — la verifica di fedelta' di
 * `GEO-8`. Ma il tool d'authoring chiama `SnapToGrammar` (`RTHexGeometryTool.cpp:119, 247, 326`), e su
 * quella funzione non c'era alcuna asserzione per le coppie inesprimibili.
 *
 * 🔑 **L'asserzione e' la sola che vale in entrambi i casi**: qualunque cosa `SnapToGrammar` faccia, non deve
 * MAI restituire il muro chiesto — perche' quel muro non esiste nella grammatica. Se rifiuta, bene; se
 * produce, deve produrre altro. Il test conta i due esiti e li **scrive nel log**, cosi' che il giorno in cui
 * la proporzione cambiasse si veda, invece di scoprirlo da un ghost che disegna la cosa sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorSnapNeverInventsTheInexpressibleTest,
	"RefactorTactics.Anchor.SnapNeverInventsTheInexpressible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorSnapNeverInventsTheInexpressibleTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);

	TArray<FRTAnchorRef> Anchors;
	URTGeometryGrammarLibrary::AnchorsOfCell(Cell, Anchors);

	int32 Inexpressible = 0, Refused = 0, ProducedSomethingElse = 0, ProducedWhatWasAsked = 0;

	for (int32 I = 0; I < Anchors.Num(); ++I)
	{
		for (int32 J = I + 1; J < Anchors.Num(); ++J)
		{
			// Solo le coppie che la grammatica NON porta: sulle altre `SnapToGrammar` deve riuscire, ed e'
			// gia' coperto da `SnapAlwaysProducesAWallFromTheAlphabet`.
			FRTGeometrySegment Faithful;
			if (URTGeometryGrammarLibrary::SegmentBetweenAnchors(Anchors[I], Anchors[J], AnchorTestHexSize,
				Faithful))
			{
				continue;
			}
			++Inexpressible;

			const FVector2D PA = URTGeometryGrammarLibrary::AnchorLocal(Anchors[I], AnchorTestHexSize);
			const FVector2D PB = URTGeometryGrammarLibrary::AnchorLocal(Anchors[J], AnchorTestHexSize);

			FRTGeometrySegment Snapped;
			if (!URTGeometryGrammarLibrary::SnapToGrammar(PA, PB, AnchorTestHexSize, Snapped))
			{
				++Refused;
				continue;
			}

			// Ha prodotto qualcosa: gli estremi coincidono con i due anchor chiesti? Stessa tolleranza
			// relativa di `SegmentBetweenAnchors` — la grammatica e' esatta sui punti notevoli, quindi lo
			// scarto ammesso e' quello di macchina e non un margine di comodo.
			const FRTOccupancyPolyline Line =
				URTGeometryGrammarLibrary::ToPolyline(Snapped, AnchorTestHexSize);
			const double Tolerance = static_cast<double>(AnchorTestHexSize) * 1e-4;
			const bool bAsAsked = Line.Points.Num() == 2
				&& ((Line.Points[0].Equals(PA, Tolerance) && Line.Points[1].Equals(PB, Tolerance))
					|| (Line.Points[0].Equals(PB, Tolerance) && Line.Points[1].Equals(PA, Tolerance)));

			if (bAsAsked)
			{
				++ProducedWhatWasAsked;
				AddError(FString::Printf(
					TEXT("%s -> %s: la grammatica non porta questa coppia, ma lo snap l'ha prodotta"),
					*Anchors[I].ToString(), *Anchors[J].ToString()));
			}
			else
			{
				++ProducedSomethingElse;
			}
		}
	}

	TestEqual(TEXT("le coppie inesprimibili sono ventiquattro"), Inexpressible, 24);

	// L'INVARIANTE, e vale qualunque sia la ripartizione fra i due esiti leciti.
	TestEqual(TEXT("nessuna coppia inesprimibile viene prodotta come chiesta"), ProducedWhatWasAsked, 0);
	TestEqual(TEXT("e i due esiti leciti coprono tutte e ventiquattro"),
		Refused + ProducedSomethingElse, Inexpressible);

	// LA MISURA che l'issue chiedeva: quale dei due comportamenti ha davvero `SnapToGrammar`.
	AddInfo(FString::Printf(
		TEXT("su %d coppie inesprimibili: %d rifiutate, %d producono un muro DIVERSO da quello chiesto"),
		Inexpressible, Refused, ProducedSomethingElse));

	if (ProducedSomethingElse > 0)
	{
		AddInfo(TEXT("∴ la deduzione di #1895 e' CONFERMATA: lo snap non fallisce, riesce con altro — ed e' ")
			TEXT("la ragione per cui il gesto d'authoring deve passare da SegmentBetweenAnchors"));
	}

	return true;
}

/**
 * `NearestAnchor` E' TOTALE, E SUI TREDICI PUNTI E' L'IDENTITA' — `#1895`.
 *
 * 🔑 **L'identita' e' la proprieta' che rende la funzione utilizzabile dal gesto.** Se chiesto l'anchor piu'
 * vicino al punto di un anchor si ottenesse un altro anchor, l'aggancio dichiarato dal ghost sarebbe una
 * bugia proprio nel caso in cui l'autore ha mirato bene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorNearestIsTotalAndIdempotentTest,
	"RefactorTactics.Anchor.NearestIsTotalAndIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorNearestIsTotalAndIdempotentTest::RunTest(const FString&)
{
	const FRTCellId Cell(2, -1, 0);

	TArray<FRTAnchorRef> Anchors;
	URTGeometryGrammarLibrary::AnchorsOfCell(Cell, Anchors);

	for (const FRTAnchorRef& Ref : Anchors)
	{
		const FVector2D Local = URTGeometryGrammarLibrary::AnchorLocal(Ref, AnchorTestHexSize);
		const FRTAnchorRef Found = URTGeometryGrammarLibrary::NearestAnchor(Cell, Local, AnchorTestHexSize);
		TestTrue(*FString::Printf(TEXT("%s si aggancia a se stesso"), *Ref.ToString()), Found == Ref);
	}

	// Un punto QUALUNQUE ottiene comunque un anchor: la palette e' chiusa, quindi non esiste il caso
	// «nessuno». Il centro della cella e' l'unico punto la cui risposta e' ovvia, e serve da controllo.
	TestTrue(TEXT("il centro geometrico da' il centro"),
		URTGeometryGrammarLibrary::NearestAnchor(Cell, FVector2D::ZeroVector, AnchorTestHexSize)
			== FRTAnchorRef(Cell, ERTAnchorKind::Center));

	// ⚠️ E un punto LONTANISSIMO non fa fallire la funzione: non c'e' soglia, per scelta dichiarata
	// nell'header — una soglia sarebbe un numero da tarare, non una proprieta' della geometria.
	const FRTAnchorRef Far = URTGeometryGrammarLibrary::NearestAnchor(
		Cell, FVector2D(10000.0, -10000.0), AnchorTestHexSize);
	TestTrue(TEXT("anche un punto lontano ottiene un anchor della cella chiesta"), Far.Cell == Cell);

	// Un punto vicino a un vertice, ma non esattamente su di esso, aggancia QUEL vertice: e' il caso reale
	// del gesto, dove il mouse non cade mai esatto.
	const FRTAnchorRef V2(Cell, ERTAnchorKind::Vertex, 2);
	const FVector2D Nudged =
		URTGeometryGrammarLibrary::AnchorLocal(V2, AnchorTestHexSize) + FVector2D(3.0, -2.0);
	TestTrue(TEXT("un punto vicino a un vertice aggancia quel vertice"),
		URTGeometryGrammarLibrary::NearestAnchor(Cell, Nudged, AnchorTestHexSize) == V2);

	return true;
}

/**
 * `ExplainPair` DICE PERCHE', E NON PUO' DIVERGERE DA CHI PRODUCE — `#1895`.
 *
 * 🔴 **Il quarto controllo e' l'intera ragione per cui questa funzione puo' esistere senza essere una
 * seconda verita'.** `ExplainPair` e `SegmentBetweenAnchors` rispondono a due domande diverse sulla stessa
 * proprieta'; il giorno in cui una cambiasse senza l'altra, il ghost direbbe «si puo'» su una coppia che
 * non produce muro, o il contrario. L'equivalenza su tutte e 78 le coppie e' cio' che lo impedisce, e va
 * asserita invece che ricordata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnchorExplainPairAgreesWithProductionTest,
	"RefactorTactics.Anchor.ExplainPairAgreesWithProduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnchorExplainPairAgreesWithProductionTest::RunTest(const FString&)
{
	const FRTCellId Cell(0, 0, 0);

	TArray<FRTAnchorRef> Anchors;
	URTGeometryGrammarLibrary::AnchorsOfCell(Cell, Anchors);

	int32 Expressible = 0, NoAxis = 0;
	for (int32 I = 0; I < Anchors.Num(); ++I)
	{
		for (int32 J = I + 1; J < Anchors.Num(); ++J)
		{
			const ERTAnchorPairRefusal Why =
				URTGeometryGrammarLibrary::ExplainPair(Anchors[I], Anchors[J], AnchorTestHexSize);

			FRTGeometrySegment S;
			const bool bProduced = URTGeometryGrammarLibrary::SegmentBetweenAnchors(
				Anchors[I], Anchors[J], AnchorTestHexSize, S);

			// L'EQUIVALENZA, coppia per coppia.
			TestEqual(*FString::Printf(TEXT("%s -> %s: spiegazione e produzione concordano"),
				*Anchors[I].ToString(), *Anchors[J].ToString()),
				Why == ERTAnchorPairRefusal::None, bProduced);

			if (Why == ERTAnchorPairRefusal::None) { ++Expressible; }
			else if (Why == ERTAnchorPairRefusal::NoTacticalAxis) { ++NoAxis; }
		}
	}

	TestEqual(TEXT("cinquantaquattro si esprimono"), Expressible, 54);
	TestEqual(TEXT("e ventiquattro non stanno su nessun asse"), NoAxis, 24);

	// I TRE RIFIUTI STRUTTURALI, ciascuno con il proprio codice: senza, il ghost direbbe «non sta su un
	// asse» a chi ha semplicemente trascinato su un'altra cella, mandandolo a correggere la cosa sbagliata.
	{
		const FRTAnchorRef Here(FRTCellId(0, 0, 0), ERTAnchorKind::Center);
		const FRTAnchorRef Elsewhere(FRTCellId(1, 0, 0), ERTAnchorKind::Center);
		TestEqual(TEXT("due celle diverse"),
			URTGeometryGrammarLibrary::ExplainPair(Here, Elsewhere, AnchorTestHexSize),
			ERTAnchorPairRefusal::DifferentCell);

		const FRTAnchorRef Above(FRTCellId(0, 0, 1), ERTAnchorKind::Vertex, 0);
		TestEqual(TEXT("due layer diversi"),
			URTGeometryGrammarLibrary::ExplainPair(Here, Above, AnchorTestHexSize),
			ERTAnchorPairRefusal::DifferentLayer);

		TestEqual(TEXT("lo stesso anchor due volte"),
			URTGeometryGrammarLibrary::ExplainPair(Here, Here, AnchorTestHexSize),
			ERTAnchorPairRefusal::SameAnchor);

		// ⚠️ E un centro con indice SPORCO resta lo stesso centro: il confronto passa dalla forma canonica,
		// non dai campi grezzi. Senza, un `FRTAnchorRef` costruito a mano darebbe due punti dove ce n'e' uno.
		FRTAnchorRef Dirty;
		Dirty.Cell = FRTCellId(0, 0, 0);
		Dirty.Kind = ERTAnchorKind::Center;
		Dirty.Index = 5;
		TestEqual(TEXT("un centro con indice sporco e' sempre lo stesso centro"),
			URTGeometryGrammarLibrary::ExplainPair(Here, Dirty, AnchorTestHexSize),
			ERTAnchorPairRefusal::SameAnchor);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
