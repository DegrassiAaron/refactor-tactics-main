#include "Algo/Reverse.h"
#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Perception/RTVisibilityBorder.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il CONFINE dell'unione visibile: da un insieme di celle ai suoi lati esposti (#1754, disegno assorbito da
 * #1715).
 *
 * Funzione pura e headless: nessun Actor, nessun `UWorld`, **nessuna mappa**. Questi test non montano
 * un'arena apposta — l'estrattore non la consulta, e passargliela darebbe l'impressione che la consulti.
 *
 * ⚠️ Il caso da non perdere e' `BorderOmitsInternalEdges`: e' l'unico che separa «bordo» da «tutti i lati di
 * tutte le celle», ed e' quello su cui la DoD chiede la mutazione verificata.
 */

namespace
{
	/** Quante volte questo lato compare nell'esito. Piu' di uno = posato due volte nello stesso punto. */
	int32 CountEdge(const TArray<FRTExposedEdge>& Edges, const FRTCellId& Cell, ERTHexDirection Dir)
	{
		int32 N = 0;
		for (const FRTExposedEdge& E : Edges)
		{
			if (E.Cell == Cell && E.Direction == Dir)
			{
				++N;
			}
		}
		return N;
	}

	bool HasEdgeOnCell(const TArray<FRTExposedEdge>& Edges, const FRTCellId& Cell)
	{
		for (const FRTExposedEdge& E : Edges)
		{
			if (E.Cell == Cell)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisibilityBorderOmitsInternalEdgesTest,
	"RefactorTactics.Visibility.BorderOmitsInternalEdges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisibilityBorderOmitsInternalEdgesTest::RunTest(const FString&)
{
	// Due celle ADIACENTI: il lato che condividono e' interno e non deve comparire da nessuna delle due.
	const FRTCellId A(0, 0, 0);
	const FRTCellId B = URTHexLibrary::Neighbor(A, ERTHexDirection::E);

	const TArray<FRTExposedEdge> Edges = URTVisibilityBorderLibrary::ExposedEdges({ A, B });

	// 6 + 6 = 12 lati in tutto, meno i DUE affacci sul lato condiviso: restano 10.
	TestEqual(TEXT("due celle adiacenti espongono dieci lati"), Edges.Num(), 10);

	TestEqual(TEXT("il lato condiviso non compare dalla prima cella"),
		CountEdge(Edges, A, ERTHexDirection::E), 0);
	TestEqual(TEXT("ne' dalla seconda"),
		CountEdge(Edges, B, URTHexLibrary::OppositeDirection(ERTHexDirection::E)), 0);

	// E i restanti ci sono davvero: senza questo, un estrattore che non emette NULLA passerebbe le righe
	// qui sopra: entrambe misurano un'assenza.
	TestEqual(TEXT("il lato opposto della prima cella e' esposto"),
		CountEdge(Edges, A, ERTHexDirection::W), 1);
	TestEqual(TEXT("e quello della seconda"),
		CountEdge(Edges, B, ERTHexDirection::E), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisibilityBorderCountsExposedEdgeOnceTest,
	"RefactorTactics.Visibility.BorderCountsExposedEdgeOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisibilityBorderCountsExposedEdgeOnceTest::RunTest(const FString&)
{
	// Una cella ISOLATA: sei lati, uno per direzione, nessuno ripetuto. E' anche il caso che dichiara che il
	// VUOTO oltre il bordo conta come «non visibile»: qui non c'e' nessuna mappa, e i sei lati escono lo
	// stesso.
	const FRTCellId Lone(2, -1, 0);
	const TArray<FRTExposedEdge> Edges = URTVisibilityBorderLibrary::ExposedEdges({ Lone });

	TestEqual(TEXT("una cella isolata espone sei lati"), Edges.Num(), 6);

	const ERTHexDirection All[] = {
		ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
		ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE
	};
	for (const ERTHexDirection Dir : All)
	{
		TestEqual(TEXT("ogni direzione esattamente una volta"), CountEdge(Edges, Lone, Dir), 1);
	}

	// Un duplicato nell'ingresso non moltiplica i lati: l'estrattore lavora su un insieme.
	const TArray<FRTExposedEdge> Doubled = URTVisibilityBorderLibrary::ExposedEdges({ Lone, Lone });
	TestEqual(TEXT("una cella ripetuta in ingresso non raddoppia il bordo"), Doubled.Num(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisibilityBorderHandlesDisconnectedRegionsTest,
	"RefactorTactics.Visibility.BorderHandlesDisconnectedRegions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisibilityBorderHandlesDisconnectedRegionsTest::RunTest(const FString&)
{
	// Due regioni LONTANE fra loro: e' il caso della fixture `VisionSplit` — una squadra spezzata in due
	// camere da un muro. Un contorno che le unisse affermerebbe che si vede anche in mezzo.
	const FRTCellId A(0, 0, 0);
	const FRTCellId B = URTHexLibrary::Neighbor(A, ERTHexDirection::E);
	const FRTCellId C(10, 0, 0);
	const FRTCellId D = URTHexLibrary::Neighbor(C, ERTHexDirection::E);

	const TArray<FRTExposedEdge> Edges = URTVisibilityBorderLibrary::ExposedEdges({ A, B, C, D });

	// Due perimetri completi e indipendenti: 10 + 10.
	TestEqual(TEXT("due regioni disgiunte danno due perimetri interi"), Edges.Num(), 20);

	TestTrue(TEXT("la prima regione ha il suo bordo"), HasEdgeOnCell(Edges, A) && HasEdgeOnCell(Edges, B));
	TestTrue(TEXT("e la seconda il proprio"), HasEdgeOnCell(Edges, C) && HasEdgeOnCell(Edges, D));

	// Nessuna cella INTERMEDIA finisce nell'esito: il bordo non riempie lo spazio fra le due camere.
	TestFalse(TEXT("lo spazio fra le due regioni non entra nel bordo"),
		HasEdgeOnCell(Edges, FRTCellId(5, 0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisibilityBorderIsLayerAwareTest,
	"RefactorTactics.Visibility.BorderIsLayerAware",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisibilityBorderIsLayerAwareTest::RunTest(const FString&)
{
	// Stessa X/Y, layer diverso: NON sono vicine — servono archi espliciti — quindi non si annullano a
	// vicenda. Fonderle mostrerebbe un solo perimetro dove ce ne sono due.
	const FRTCellId Ground(0, 0, 0);
	const FRTCellId Above(0, 0, 1);

	const TArray<FRTExposedEdge> Edges = URTVisibilityBorderLibrary::ExposedEdges({ Ground, Above });

	TestEqual(TEXT("due celle sovrapposte espongono dodici lati, sei per piano"), Edges.Num(), 12);

	for (const FRTExposedEdge& E : Edges)
	{
		TestTrue(TEXT("ogni lato dichiara il proprio piano"), E.Cell.Layer == 0 || E.Cell.Layer == 1);
	}

	// E il piano non si perde: un lato del layer 1 esiste, e non e' quello del layer 0 travestito.
	TestEqual(TEXT("il piano superiore ha il proprio lato est"),
		CountEdge(Edges, Above, ERTHexDirection::E), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisibilityBorderIsPermutationInvariantTest,
	"RefactorTactics.Visibility.BorderIsPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisibilityBorderIsPermutationInvariantTest::RunTest(const FString&)
{
	// L'invariante che `TeamVisibleCells` gia' garantisce a monte, e che sarebbe assurdo perdere qui: chi
	// osserva per primo non deve cambiare il disegno.
	const TArray<FRTCellId> Forward = {
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(1, -1, 0), FRTCellId(0, 1, 0)
	};
	TArray<FRTCellId> Backward = Forward;
	Algo::Reverse(Backward);

	const TArray<FRTExposedEdge> A = URTVisibilityBorderLibrary::ExposedEdges(Forward);
	const TArray<FRTExposedEdge> B = URTVisibilityBorderLibrary::ExposedEdges(Backward);

	TestEqual(TEXT("permutare l'ingresso non cambia il numero di lati"), A.Num(), B.Num());
	for (int32 i = 0; i < A.Num(); ++i)
	{
		// Elemento per elemento, nello stesso ordine: un esito «uguale come insieme ma in ordine diverso»
		// renderebbe instabile qualunque cosa lo consumi per indice.
		TestTrue(TEXT("e nemmeno il loro ordine"), A[i] == B[i]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisibilityBorderOfEmptySetIsEmptyTest,
	"RefactorTactics.Visibility.BorderOfEmptySetIsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisibilityBorderOfEmptySetIsEmptyTest::RunTest(const FString&)
{
	// Fail-closed, come `TeamVisibleCells` con zero osservatori: nessuna vista -> nessun confine. Mai «tutta
	// la mappa», che e' l'errore opposto e quello che rivelerebbe l'arena a chi non la conosce.
	TestEqual(TEXT("insieme vuoto, nessun lato"),
		URTVisibilityBorderLibrary::ExposedEdges({}).Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
