#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexCellData.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Terrain/RTTerrainData.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCellIdEqualityTest,
	"RefactorTactics.Hex.CellIdEqualityAndHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCellIdEqualityTest::RunTest(const FString&)
{
	const FRTCellId A(2, -1, 0);
	const FRTCellId B(2, -1, 0);
	const FRTCellId C(2, -1, 1); // stesso q,r ma layer diverso
	TestTrue(TEXT("uguali"), A == B);
	TestTrue(TEXT("hash uguale per uguali"), GetTypeHash(A) == GetTypeHash(B));
	TestTrue(TEXT("layer diverso -> diverse"), A != C);
	TestTrue(TEXT("coerenza cubica"), A.IsValid() && C.IsValid());
	TestTrue(TEXT("CubeZ = -q-r"), A.CubeZ() == -1); // -(2)-(-1) = -1

	// Usabile come chiave: due layer distinti con stessi q,r non collidono.
	TSet<FRTCellId> Set;
	Set.Add(A); Set.Add(C);
	TestEqual(TEXT("2 chiavi distinte"), Set.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexNeighborsTest,
	"RefactorTactics.Hex.SixDistinctNeighborsAtDistanceOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexNeighborsTest::RunTest(const FString&)
{
	const FRTCellId Center(0, 0, 0);
	const TArray<FRTCellId> N = URTHexLibrary::Neighbors(Center);
	TestEqual(TEXT("sei vicini"), N.Num(), 6);

	TSet<FRTCellId> Unique;
	for (const FRTCellId& Cell : N)
	{
		Unique.Add(Cell);
		TestEqual(TEXT("ogni vicino a distanza 1"), URTHexLibrary::HexDistance(Center, Cell), 1);
		TestEqual(TEXT("vicino sullo stesso layer"), Cell.Layer, 0);
	}
	TestEqual(TEXT("sei vicini distinti"), Unique.Num(), 6);

	// Coerenza enum -> Neighbor.
	TestTrue(TEXT("E = (+1,0)"), URTHexLibrary::Neighbor(Center, ERTHexDirection::E) == FRTCellId(1, 0, 0));
	TestTrue(TEXT("NW = (0,-1)"), URTHexLibrary::Neighbor(Center, ERTHexDirection::NW) == FRTCellId(0, -1, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexDistanceTest,
	"RefactorTactics.Hex.CubeDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexDistanceTest::RunTest(const FString&)
{
	TestEqual(TEXT("stessa cella = 0"), URTHexLibrary::HexDistance(FRTCellId(0, 0), FRTCellId(0, 0)), 0);
	TestEqual(TEXT("(0,0)->(3,0) = 3"), URTHexLibrary::HexDistance(FRTCellId(0, 0), FRTCellId(3, 0)), 3);
	TestEqual(TEXT("(0,0)->(0,3) = 3"), URTHexLibrary::HexDistance(FRTCellId(0, 0), FRTCellId(0, 3)), 3);
	TestEqual(TEXT("(0,0)->(2,-1) = 2"), URTHexLibrary::HexDistance(FRTCellId(0, 0), FRTCellId(2, -1)), 2);
	TestEqual(TEXT("(0,0)->(-2,1) = 2"), URTHexLibrary::HexDistance(FRTCellId(0, 0), FRTCellId(-2, 1)), 2);
	TestEqual(TEXT("(1,-3)->(4,-1) = ?"), URTHexLibrary::HexDistance(FRTCellId(1, -3), FRTCellId(4, -1)),
		(FMath::Abs(1 - 4) + FMath::Abs(-3 + 1) + FMath::Abs((-1 - (-3)) - (-4 - (-1)))) / 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexRoundTripTest,
	"RefactorTactics.Hex.AxialToWorldRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexRoundTripTest::RunTest(const FString&)
{
	const FVector Origin(1000.0, -500.0, 200.0);
	const float HexSize = 100.f;
	const float LayerHeight = 250.f;

	const TArray<FRTCellId> Cases = {
		FRTCellId(0, 0, 0), FRTCellId(2, 1, 0), FRTCellId(-1, 3, 0),
		FRTCellId(3, -2, 1), FRTCellId(-4, -2, 2), FRTCellId(5, -5, 0)
	};
	for (const FRTCellId& C : Cases)
	{
		const FVector World = URTHexLibrary::AxialToWorld(C, Origin, HexSize, LayerHeight);
		const FRTCellId Back = URTHexLibrary::WorldToAxial(World, Origin, HexSize, C.Layer);
		TestTrue(FString::Printf(TEXT("round-trip %s -> %s"), *C.ToString(), *Back.ToString()), Back == C);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexStableSortTest,
	"RefactorTactics.Hex.StableOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexStableSortTest::RunTest(const FString&)
{
	TArray<FRTCellId> Cells = {
		FRTCellId(1, 0, 1), FRTCellId(0, 5, 0), FRTCellId(0, -3, 0), FRTCellId(1, 0, 0), FRTCellId(-2, 2, 0)
	};
	Cells.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
	// Atteso: layer 0 prima (ordinati per X poi Y), poi layer 1.
	const TArray<FRTCellId> Expected = {
		FRTCellId(-2, 2, 0), FRTCellId(0, -3, 0), FRTCellId(0, 5, 0), FRTCellId(1, 0, 0), FRTCellId(1, 0, 1)
	};
	bool bSame = (Cells.Num() == Expected.Num());
	for (int32 I = 0; I < Cells.Num() && bSame; ++I) { bSame = (Cells[I] == Expected[I]); }
	TestTrue(TEXT("ordine stabile Layer->X->Y"), bSame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexAreaTest,
	"RefactorTactics.Hex.HexAreaRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexAreaTest::RunTest(const FString&)
{
	const FRTCellId Center(0, 0, 0);
	// Conteggio esagono pieno = 3N(N+1)+1: N=0->1, N=1->7, N=2->19.
	TestEqual(TEXT("raggio 0 = 1 cella"), URTHexLibrary::HexArea(Center, 0).Num(), 1);
	TestEqual(TEXT("raggio 1 = 7 celle"), URTHexLibrary::HexArea(Center, 1).Num(), 7);
	TestEqual(TEXT("raggio 2 = 19 celle"), URTHexLibrary::HexArea(Center, 2).Num(), 19);
	TestEqual(TEXT("raggio negativo = vuoto"), URTHexLibrary::HexArea(Center, -1).Num(), 0);

	// Tutte le celle sono entro il raggio e sul layer del centro; nessun duplicato.
	const TArray<FRTCellId> Area = URTHexLibrary::HexArea(FRTCellId(3, -1, 2), 2);
	TSet<FRTCellId> Unique;
	bool bWithin = true, bSameLayer = true;
	for (const FRTCellId& C : Area)
	{
		Unique.Add(C);
		bWithin = bWithin && URTHexLibrary::HexDistance(FRTCellId(3, -1, 2), C) <= 2;
		bSameLayer = bSameLayer && C.Layer == 2;
	}
	TestTrue(TEXT("tutte entro raggio"), bWithin);
	TestTrue(TEXT("tutte sul layer del centro"), bSameLayer);
	TestEqual(TEXT("nessun duplicato"), Unique.Num(), Area.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexWorldToLayerTest,
	"RefactorTactics.Hex.WorldToLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexWorldToLayerTest::RunTest(const FString&)
{
	const double OriginZ = 200.0;
	const float LayerH = 250.f;

	// LayerHeight <= 0 -> 0 (nessuna divisione).
	TestEqual(TEXT("LayerHeight 0 -> layer 0"), URTHexLibrary::WorldToLayer(9999.0, OriginZ, 0.f), 0);

	// Z al centro di un layer -> quel layer.
	TestEqual(TEXT("Z all'origine -> 0"), URTHexLibrary::WorldToLayer(OriginZ, OriginZ, LayerH), 0);
	TestEqual(TEXT("Z = origin + 2*LayerH -> 2"), URTHexLibrary::WorldToLayer(OriginZ + 2.0 * LayerH, OriginZ, LayerH), 2);
	TestEqual(TEXT("Z = origin - 1*LayerH -> -1"), URTHexLibrary::WorldToLayer(OriginZ - 1.0 * LayerH, OriginZ, LayerH), -1);

	// Tie-break floor(x+0.5): +1.5 -> 2 ; -0.5 -> 0.
	TestEqual(TEXT("Z = origin + 1.5*LayerH -> 2"), URTHexLibrary::WorldToLayer(OriginZ + 1.5 * LayerH, OriginZ, LayerH), 2);
	TestEqual(TEXT("Z = origin - 0.5*LayerH -> 0"), URTHexLibrary::WorldToLayer(OriginZ - 0.5 * LayerH, OriginZ, LayerH), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexDistanceRaySegTest,
	"RefactorTactics.Hex.DistanceRayToSegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexDistanceRaySegTest::RunTest(const FString&)
{
	// 1. Ray attraversa il segmento -> ~0.
	{
		const float D = URTHexLibrary::DistanceRayToSegment(FVector(1, 0, -10), FVector(0, 0, 1), FVector(0, 0, 0), FVector(2, 0, 0));
		TestTrue(TEXT("interseca -> 0"), FMath::IsNearlyZero(D, 1e-3f));
	}
	// 2. Ray parallelo al segmento, offset 5 in Y -> ~5.
	{
		const float D = URTHexLibrary::DistanceRayToSegment(FVector(0, 5, 0), FVector(1, 0, 0), FVector(0, 0, 0), FVector(10, 0, 0));
		TestTrue(TEXT("parallelo offset 5"), FMath::IsNearlyEqual(D, 5.f, 1e-3f));
	}
	// 3. Closest oltre l'estremo B -> sqrt(10^2 + 3^2) = sqrt(109).
	{
		const float D = URTHexLibrary::DistanceRayToSegment(FVector(20, 3, 0), FVector(0, 0, 1), FVector(0, 0, 0), FVector(10, 0, 0));
		TestTrue(TEXT("oltre estremo"), FMath::IsNearlyEqual(D, FMath::Sqrt(109.f), 1e-2f));
	}
	// 4. Segmento degenere (A==B): distanza punto-ray = 5.
	{
		const float D = URTHexLibrary::DistanceRayToSegment(FVector(0, 0, 5), FVector(0, 1, 0), FVector(0, 0, 0), FVector(0, 0, 0));
		TestTrue(TEXT("segmento degenere"), FMath::IsNearlyEqual(D, 5.f, 1e-3f));
	}
	return true;
}

/**
 * CP 6.3: un punto del mondo deve tornare la cella COMPLETA, layer incluso. Finora il layer si ricavava
 * separatamente (WorldToLayer) e poi si chiamava WorldToAxial passandolo a mano: chi lo compone e' costretto
 * a duplicare la sequenza (lo fa il tool Arch dell'editor, e servira' all'input di gioco).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexWorldToCellIdTest,
	"RefactorTactics.Hex.WorldToCellIdRoundTripAcrossLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexWorldToCellIdTest::RunTest(const FString&)
{
	const FVector Origin(1000.0, -500.0, 200.0);
	const float HexSize = 100.f;
	const float LayerHeight = 250.f;

	// Il centro di una cella deve tornare quella cella, layer compreso, su piu' layer.
	const TArray<FRTCellId> Cases = {
		FRTCellId(0, 0, 0), FRTCellId(2, 1, 0), FRTCellId(-1, 3, 1),
		FRTCellId(3, -2, 2), FRTCellId(-4, -2, -1)
	};
	for (const FRTCellId& C : Cases)
	{
		const FVector World = URTHexLibrary::AxialToWorld(C, Origin, HexSize, LayerHeight);
		const FRTCellId Back = URTHexLibrary::WorldToCellId(World, Origin, HexSize, LayerHeight);
		TestTrue(FString::Printf(TEXT("round-trip completo %s -> %s"), *C.ToString(), *Back.ToString()), Back == C);
	}

	// Coerente con le due funzioni che compone: stesso esito di WorldToLayer + WorldToAxial.
	{
		const FVector P(1234.0, -321.0, 200.0 + 1.2 * LayerHeight);
		const int32 ExpectedLayer = URTHexLibrary::WorldToLayer(P.Z, Origin.Z, LayerHeight);
		const FRTCellId Expected = URTHexLibrary::WorldToAxial(P, Origin, HexSize, ExpectedLayer);
		TestTrue(TEXT("equivale a WorldToLayer + WorldToAxial"),
			URTHexLibrary::WorldToCellId(P, Origin, HexSize, LayerHeight) == Expected);
	}

	// LayerHeight <= 0: nessuna divisione, tutto sul layer 0 (come WorldToLayer).
	{
		const FVector P(1000.0, -500.0, 9999.0);
		TestEqual(TEXT("LayerHeight 0 -> layer 0"),
			URTHexLibrary::WorldToCellId(P, Origin, HexSize, 0.f).Layer, 0);
	}
	return true;
}

/**
 * CP 6.3: i vertici dell'esagono servono sia al marker dell'editor sia all'anteprima in gioco. Una sola
 * definizione, cosi' i due disegni non divergono di orientamento (pointy-top, primo vertice a -30 gradi).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCornersTest,
	"RefactorTactics.Hex.HexCornersPointyTop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCornersTest::RunTest(const FString&)
{
	const FVector Center(300.0, -100.0, 50.0);
	const float Radius = 90.f;
	const TArray<FVector> Corners = URTHexLibrary::HexCorners(Center, Radius);

	TestEqual(TEXT("sei vertici"), Corners.Num(), 6);
	if (Corners.Num() != 6)
	{
		return false;
	}

	TSet<FString> Distinct;
	for (const FVector& C : Corners)
	{
		// Ogni vertice a distanza Radius dal centro, sullo stesso piano orizzontale.
		const double D = FVector2D(C.X - Center.X, C.Y - Center.Y).Size();
		TestTrue(FString::Printf(TEXT("vertice a distanza %.1f dal centro"), D),
			FMath::IsNearlyEqual(D, static_cast<double>(Radius), 0.01));
		TestTrue(TEXT("vertice complanare al centro"), FMath::IsNearlyEqual(C.Z, Center.Z, 0.01));
		Distinct.Add(FString::Printf(TEXT("%.2f,%.2f"), C.X, C.Y));
	}
	TestEqual(TEXT("sei vertici distinti"), Distinct.Num(), 6);

	// Orientamento pointy-top come il marker dell'editor: primo vertice a -30 gradi.
	const double Expected = -PI / 6.0;
	TestTrue(TEXT("primo vertice a -30 gradi"),
		FMath::IsNearlyEqual(Corners[0].X - Center.X, Radius * FMath::Cos(Expected), 0.01) &&
		FMath::IsNearlyEqual(Corners[0].Y - Center.Y, Radius * FMath::Sin(Expected), 0.01));
	return true;
}

/**
 * Punto da inquadrare per un gruppo di celle: la media dei loro centri. Serve alla camera per partire sulla
 * squadra del giocatore invece che sul centro della mappa. Nessuna decisione di gioco: sola geometria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCentroidTest,
	"RefactorTactics.Hex.CellsCentroidWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCentroidTest::RunTest(const FString&)
{
	const FVector Origin(500.0, -200.0, 30.0);
	const float HexSize = 100.f;
	const float LayerHeight = 250.f;

	// Nessuna cella: si ripiega sull'origine, non su un punto arbitrario.
	TestTrue(TEXT("insieme vuoto -> origine"),
		URTHexLibrary::CellsCentroidWorld({}, Origin, HexSize, LayerHeight).Equals(Origin, 0.01));

	// Una cella: esattamente il suo centro.
	const FRTCellId Single(2, -1, 0);
	TestTrue(TEXT("una cella -> il suo centro"),
		URTHexLibrary::CellsCentroidWorld({ Single }, Origin, HexSize, LayerHeight)
			.Equals(URTHexLibrary::AxialToWorld(Single, Origin, HexSize, LayerHeight), 0.01));

	// Due celle: il punto medio fra i due centri (e' cio' che rende l'inquadratura "sulla squadra").
	const FRTCellId A(0, 0, 0);
	const FRTCellId B(4, 0, 0);
	const FVector WA = URTHexLibrary::AxialToWorld(A, Origin, HexSize, LayerHeight);
	const FVector WB = URTHexLibrary::AxialToWorld(B, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("due celle -> punto medio"),
		URTHexLibrary::CellsCentroidWorld({ A, B }, Origin, HexSize, LayerHeight).Equals((WA + WB) * 0.5, 0.01));

	// L'ordine dell'input non cambia il risultato.
	TestTrue(TEXT("ordine irrilevante"),
		URTHexLibrary::CellsCentroidWorld({ B, A }, Origin, HexSize, LayerHeight)
			.Equals(URTHexLibrary::CellsCentroidWorld({ A, B }, Origin, HexSize, LayerHeight), 0.01));
	return true;
}


/**
 * Il rilievo che mostra il costo di movimento nasce dal CATALOGO, non da numeri incisi nella vista.
 *
 * E' l'invariante che tiene: ribilanciare `Rough` deve cambiare la mappa da sola. Se l'altezza fosse scritta
 * nella vista, il giorno del ribilanciamento il profilo resterebbe su un valore morto e racconterebbe un
 * costo che il gioco non applica piu' — una vista che mente, che e' peggio di una vista che manca.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexReliefFromCatalogTest,
	"RefactorTactics.Hex.ReliefHeightComesFromTerrainCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexReliefFromCatalogTest::RunTest(const FString&)
{
	// 1. Il pavimento sta a zero: il rilievo misura il SOVRAPPREZZO, non il costo assoluto. Una mappa tutta
	//    pavimento resta piatta, ed e' giusto — non c'e' niente da segnalare.
	TestEqual(TEXT("costo 1 -> nessun rilievo"), URTHexLibrary::ReliefHeightForCost(1), 0.f);

	// 2. Monotonia: piu' caro = piu' alto. E' l'unica cosa che rende il profilo leggibile a colpo d'occhio.
	TestTrue(TEXT("costo 2 > costo 1"),
		URTHexLibrary::ReliefHeightForCost(2) > URTHexLibrary::ReliefHeightForCost(1));
	TestTrue(TEXT("costo 3 > costo 2"),
		URTHexLibrary::ReliefHeightForCost(3) > URTHexLibrary::ReliefHeightForCost(2));

	// 3. Il legame col catalogo: un terreno che il catalogo dichiara piu' caro del pavimento DEVE produrre un
	//    rilievo, e uno che costa come il pavimento no. E' cio' che rende il profilo una lettura del costo
	//    reale invece di una decorazione — se domani `Rough` venisse ribilanciato a 1, questo cadrebbe, ed e'
	//    giusto: il rilievo racconterebbe un sovrapprezzo che non esiste piu'.
	//
	//    (Confrontare `ReliefHeightForCost(RoughCost)` con `ReliefHeightForCost(2)` sarebbe tautologico:
	//    `RoughCost` *e'* 2, quindi verificherebbe una cosa con se' stessa.)
	const int32 RoughCost = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Rough).MoveCost;
	const int32 FloorCost = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Floor).MoveCost;
	TestTrue(TEXT("il catalogo dichiara Rough piu' caro del pavimento"), RoughCost > FloorCost);
	TestTrue(TEXT("quindi Rough ha un rilievo"), URTHexLibrary::ReliefHeightForCost(RoughCost) > 0.f);
	TestEqual(TEXT("e il pavimento resta piatto"), URTHexLibrary::ReliefHeightForCost(FloorCost), 0.f);

	// 4. Un rilievo non deve MAI poter essere scambiato per un piano: e' il vincolo che tiene separati i due
	//    significati della quota in questa vista. Anche il terreno piu' caro del catalogo resta ben sotto.
	// L'elenco e' esplicito come in `SurfaceColorsAreDistinguishable`: l'enum non dichiara `TEnumRange`, e
	// aggiungerlo per un test cambierebbe un tipo di dominio per comodita' di verifica.
	const TArray<ERTHexSurface> AllSurfaces = {
		ERTHexSurface::Floor, ERTHexSurface::ShallowWater, ERTHexSurface::Rough, ERTHexSurface::Fire,
		ERTHexSurface::Conductive, ERTHexSurface::Ice, ERTHexSurface::Void,
		ERTHexSurface::Smoke, ERTHexSurface::HighGround
	};
	int32 MaxCost = 1;
	for (const ERTHexSurface S : AllSurfaces)
	{
		MaxCost = FMath::Max(MaxCost, URTTerrainLibrary::FindTerrainDef(S).MoveCost);
	}
	const float LayerHeightDefault = 250.f; // il default di URTHexMapAsset::LayerHeight
	TestTrue(TEXT("il rilievo piu' alto del catalogo resta ben sotto un piano"),
		URTHexLibrary::ReliefHeightForCost(MaxCost) < LayerHeightDefault * 0.25f);

	// 5. Costi non validi non producono buche: un asset editato a mano puo' contenerli, e una buca direbbe
	//    «qui si va piu' veloci», cosa che il gioco non prevede.
	TestEqual(TEXT("costo 0 -> piatto"), URTHexLibrary::ReliefHeightForCost(0), 0.f);
	TestEqual(TEXT("costo negativo -> piatto"), URTHexLibrary::ReliefHeightForCost(-5), 0.f);
	return true;
}

/**
 * Il bordo fra due celle e' UNO SOLO, e la libreria deve saperlo dire senza che nessun chiamante incida un
 * angolo.
 *
 * `MakeCoverYardArena` gia' avverte: «la direzione si CHIEDE alla libreria invece di scriverla a mano: se la
 * convenzione dei sei lati cambiasse, un valore inciso qui diventerebbe silenziosamente il bordo sbagliato».
 * Finora pero' non c'era nulla da chiedere per la GEOMETRIA di un lato — solo `AxialDirection`,
 * `DirectionTowards` e `HexCorners`, cioe' i vertici — e chi doveva disegnare su un bordo avrebbe scelto due
 * indici di `HexCorners` a occhio.
 *
 * Il test non verifica dei numeri: verifica le RELAZIONI che rendono la funzione derivata invece che incisa.
 * Se domani la convenzione dei sei lati cambiasse, questi vincoli reggerebbero e la geometria seguirebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexEdgeTransformTest,
	"RefactorTactics.Hex.EdgeTransformIsDerivedFromCellCenters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexEdgeTransformTest::RunTest(const FString&)
{
	const FVector Origin(1000.0, -500.0, 250.0); // non l'origine del mondo: un offset sbagliato si vedrebbe
	const float HexSize = 120.f;
	const float LayerHeight = 250.f;
	const FRTCellId Cell(3, -2, 1);

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		const FRTCellId Vicino = URTHexLibrary::Neighbor(Cell, Dir);
		const FVector CentroCella = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);
		const FVector CentroVicino = URTHexLibrary::AxialToWorld(Vicino, Origin, HexSize, LayerHeight);
		const FTransform Bordo = URTHexLibrary::EdgeTransform(Cell, Dir, Origin, HexSize, LayerHeight);

		// 1. Sta a META' STRADA fra i due centri. E' la definizione del §5 del brief, e l'unica che non
		//    richiede di sapere quali vertici dell'esagono delimitano quel lato.
		TestTrue(FString::Printf(TEXT("dir %d: il bordo e' il punto medio fra i due centri"), D),
			Bordo.GetLocation().Equals((CentroCella + CentroVicino) * 0.5, 0.01));

		// 2. Guarda VERSO il vicino: e' cio' che rende «da che lato sono coperto» una domanda con risposta.
		//    Un pannello ruotato di 90 gradi direbbe il bordo sbagliato pur stando nel posto giusto.
		const FVector Avanti = Bordo.GetRotation().GetForwardVector();
		const FVector VersoIlVicino = (CentroVicino - CentroCella).GetSafeNormal();
		TestTrue(FString::Printf(TEXT("dir %d: guarda verso il vicino"), D),
			Avanti.Equals(VersoIlVicino, 0.01));

		// 3. Resta sul PIANO della cella: un bordo che scivolasse di quota apparterrebbe a un altro layer, e
		//    su una mappa multilivello sarebbe indistinguibile da un errore di dato.
		TestTrue(FString::Printf(TEXT("dir %d: stessa quota del centro cella"), D),
			FMath::IsNearlyEqual(Bordo.GetLocation().Z, CentroCella.Z, 0.01));
	}

	// 4. La proprieta' che vale piu' delle altre tre: **il bordo E di una cella E' il bordo W del suo vicino**.
	//    Sono lo stesso muro visto da due stanze. Se le due chiamate dessero punti diversi, una copertura
	//    disegnata dalla cella A e la stessa vista da B starebbero in due posti, e la mappa mostrerebbe due
	//    ripari dove il dato ne ha uno.
	const FRTCellId Vicino = URTHexLibrary::Neighbor(Cell, ERTHexDirection::E);
	const FTransform DaQui = URTHexLibrary::EdgeTransform(Cell, ERTHexDirection::E, Origin, HexSize, LayerHeight);
	const FTransform DaLa = URTHexLibrary::EdgeTransform(Vicino, ERTHexDirection::W, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("il bordo E di una cella e' il bordo W del vicino: stesso punto"),
		DaQui.GetLocation().Equals(DaLa.GetLocation(), 0.01));

	// ...e i due lo guardano da lati OPPOSTI. Serve a chi disegna: un pannello per bordo, non due sovrapposti.
	TestTrue(TEXT("e lo guardano da versi opposti"),
		DaQui.GetRotation().GetForwardVector().Equals(-DaLa.GetRotation().GetForwardVector(), 0.01));

	// 5. Sei direzioni, sei bordi DISTINTI. Senza questo, un errore nella tabella delle direzioni che facesse
	//    collassare due lati passerebbe tutti i controlli qui sopra.
	TArray<FVector> Punti;
	for (int32 D = 0; D < 6; ++D)
	{
		Punti.Add(URTHexLibrary::EdgeTransform(Cell, static_cast<ERTHexDirection>(D), Origin, HexSize,
			LayerHeight).GetLocation());
	}
	int32 Coincidenti = 0;
	for (int32 A = 0; A < Punti.Num(); ++A)
	{
		for (int32 B = A + 1; B < Punti.Num(); ++B)
		{
			if (Punti[A].Equals(Punti[B], 0.01)) { ++Coincidenti; }
		}
	}
	TestEqual(TEXT("i sei bordi sono sei punti distinti"), Coincidenti, 0);

	return true;
}

/**
 * Le porte hanno QUATTRO stati e la vista ne deve dire quattro: se due collassassero nella stessa forma, la
 * mappa mostrerebbe una porta che non e' quella che il dato contiene.
 *
 * Il rischio e' concreto perche' la seconda distinzione — «chi la cambia» — e' portata dall'ingombro, che e'
 * un canale piu' debole dell'altezza. Un ritocco fatto per far star meglio i pannelli potrebbe avvicinarli
 * fino a renderli la stessa cosa **senza rompere nulla di visibile**: questo test e' cio' che lo impedisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexDoorProfilesTest,
	"RefactorTactics.Hex.DoorProfilesTellTheFourStatesApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexDoorProfilesTest::RunTest(const FString&)
{
	const TArray<ERTHexDoorState> Stati = {
		ERTHexDoorState::Open, ERTHexDoorState::Closed, ERTHexDoorState::Locked, ERTHexDoorState::Destroyed
	};

	// 1. Quattro stati, quattro profili distinti. Il confronto e' a coppie: basta che DUE coincidano perche'
	//    la mappa smetta di poter dire quale delle due sta guardando.
	int32 Coincidenti = 0;
	for (int32 A = 0; A < Stati.Num(); ++A)
	{
		for (int32 B = A + 1; B < Stati.Num(); ++B)
		{
			if (URTHexLibrary::DoorPanelProfile(Stati[A]).Equals(URTHexLibrary::DoorPanelProfile(Stati[B]), 0.001))
			{
				++Coincidenti;
			}
		}
	}
	TestEqual(TEXT("i quattro stati hanno quattro profili distinti"), Coincidenti, 0);

	// 2. La distinzione che conta di piu' e' anche la piu' forte: quella fra «ci si passa» e «non ci si passa»
	//    sta nell'ALTEZZA, ed e' netta. Chiuse e bloccate stanno sopra; aperte e sfondate sotto.
	const float Aperta = URTHexLibrary::DoorPanelProfile(ERTHexDoorState::Open).Z;
	const float Sfondata = URTHexLibrary::DoorPanelProfile(ERTHexDoorState::Destroyed).Z;
	const float Chiusa = URTHexLibrary::DoorPanelProfile(ERTHexDoorState::Closed).Z;
	const float Bloccata = URTHexLibrary::DoorPanelProfile(ERTHexDoorState::Locked).Z;
	TestTrue(TEXT("una porta chiusa e' molto piu' alta di una aperta"), Chiusa > Aperta * 4.f);
	TestTrue(TEXT("una bloccata e' alta come una chiusa: bloccano allo stesso modo"),
		FMath::IsNearlyEqual(Bloccata, Chiusa, 0.01f));
	TestTrue(TEXT("una sfondata e' bassa come una aperta: si passa in entrambe"),
		FMath::IsNearlyEqual(Sfondata, Aperta, 0.01f));

	// 3. ...e la seconda domanda e' portata dall'INGOMBRO, sull'asse che l'altezza lascia libero. E' cio' che
	//    rende i due assi indipendenti invece di due modi di dire la stessa cosa.
	TestTrue(TEXT("la bloccata e' piu' spessa della chiusa"),
		URTHexLibrary::DoorPanelProfile(ERTHexDoorState::Locked).X
			> URTHexLibrary::DoorPanelProfile(ERTHexDoorState::Closed).X);
	TestTrue(TEXT("la sfondata occupa meno lato della aperta: manca un pezzo"),
		URTHexLibrary::DoorPanelProfile(ERTHexDoorState::Destroyed).Y
			< URTHexLibrary::DoorPanelProfile(ERTHexDoorState::Open).Y);

	// 4. Le coperture: due tipi, due altezze, e quella ALTA nega l'attraversamento come un muro — quindi deve
	//    stare dalla parte dei blocchi, non da quella dei muretti. Il legame e' con la colonna di #552: sono
	//    la stessa regola detta su un bordo invece che su una cella, e devono somigliarsi.
	const float Bassa = URTHexLibrary::CoverPanelProfile(ERTHexCoverType::Low).Z;
	const float Alta = URTHexLibrary::CoverPanelProfile(ERTHexCoverType::High).Z;
	TestTrue(TEXT("la copertura alta e' piu' alta della bassa"), Alta > Bassa);
	TestTrue(TEXT("e sta vicino alla colonna che dice «non si passa»"),
		Alta > URTHexLibrary::MovementBlockerHeight * 0.5f);
	// La bassa invece si scavalca con lo sguardo: non deve poter essere scambiata per un blocco.
	TestTrue(TEXT("la bassa resta ben sotto quella colonna"),
		Bassa < URTHexLibrary::MovementBlockerHeight * 0.5f);

	return true;
}

/**
 * I due volumi che l'editor usa per le regole di blocco devono restare DISTINGUIBILI fra loro e dal rilievo
 * del costo, che occupa la stessa cella.
 *
 * E' la distinzione piu' fraintesa della mappa: una cella che blocca la vista **si attraversa**, ed e'
 * esattamente cio' che serve a una rotta coperta ma percorribile. Costruendo `L_HexArena` l'errore e' stato
 * commesso due volte, una anche scrivendo il layout da codice dopo averlo documentato come trappola.
 *
 * Con una sola mesh la differenza fra i due e' una PROPORZIONE: questi vincoli sono cio' che impedisce a un
 * ritocco di avvicinarli fino a renderli la stessa cosa. Il giudizio «si legge?» resta una verifica in
 * editor — qui si pinna solo che le proporzioni non collassino.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBlockerVolumesTest,
	"RefactorTactics.Hex.BlockerVolumesSeparateTheTwoRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBlockerVolumesTest::RunTest(const FString&)
{
	// 1. Di profilo: chi non si passa e' una COLONNA, chi non si vede attraverso e' una LASTRA.
	TestTrue(TEXT("la colonna del blocco e' piu' alta della lastra della vista"),
		URTHexLibrary::MovementBlockerHeight > URTHexLibrary::SightBlockerHeight);

	// 2. Dall'alto — che e' la vista di LAVORO, quella in cui si dipinge — la sola altezza non basta: le due
	//    regole devono differire anche di ingombro, o guardando a picco si somigliano.
	TestTrue(TEXT("la lastra della vista e' piu' larga della colonna del blocco"),
		URTHexLibrary::SightBlockerWidthScale > URTHexLibrary::MovementBlockerWidthScale);

	// 3. Il canale del COSTO non deve sparire sotto quello della REGOLA. La lastra e' larga, quindi circonda
	//    il rilievo: deve restare piu' bassa del rilievo piu' piccolo che possa esistere (sovrapprezzo di 1),
	//    altrimenti su una cella che blocca la vista il costo diventa invisibile.
	TestTrue(TEXT("la lastra non annega il rilievo di costo piu' piccolo"),
		URTHexLibrary::SightBlockerHeight < URTHexLibrary::ReliefHeightForCost(2));

	// 4. La colonna e' invece alta, quindi il rilievo non puo' starle sopra: le si mette DENTRO, e per vederlo
	//    la colonna deve essere piu' stretta del rilievo. I due canali convivono sulla stessa cella.
	TestTrue(TEXT("la colonna sta dentro il rilievo del costo, che resta visibile attorno"),
		URTHexLibrary::MovementBlockerWidthScale < URTHexLibrary::ReliefWidthScale);

	// 5. Nessuno dei due volumi copre il contorno colorato della superficie, che e' il canale del TIPO di
	//    terreno e l'unico che dice «fango» invece di «regola».
	const float SurfaceOutlineScale = 0.85f; // `DrawHexMarker(..., HexSize * 0.85f, SurfaceColor(...))`
	TestTrue(TEXT("la lastra resta dentro il contorno della superficie"),
		URTHexLibrary::SightBlockerWidthScale < SurfaceOutlineScale);

	// 6. Il vincolo che tiene separati i due significati della QUOTA, lo stesso che vale per il rilievo: un
	//    volume di regola non deve mai poter essere scambiato per il piano di sopra.
	const float LayerHeightDefault = 250.f; // il default di URTHexMapAsset::LayerHeight
	TestTrue(TEXT("nemmeno la colonna arriva al piano superiore"),
		URTHexLibrary::MovementBlockerHeight < LayerHeightDefault * 0.5f);

	// 7. La proprieta' che separa i due VOCABOLARI: «caro» e «impossibile» sono cose diverse, e la mappa non
	//    deve poterle confondere. Anche il terreno piu' caro del catalogo resta ben sotto la colonna — se un
	//    ribilanciamento lo portasse a superarla, un terreno costoso sembrerebbe un muro, e questo cade.
	const TArray<ERTHexSurface> AllSurfaces = {
		ERTHexSurface::Floor, ERTHexSurface::ShallowWater, ERTHexSurface::Rough, ERTHexSurface::Fire,
		ERTHexSurface::Conductive, ERTHexSurface::Ice, ERTHexSurface::Void,
		ERTHexSurface::Smoke, ERTHexSurface::HighGround
	};
	int32 MaxCost = 1;
	for (const ERTHexSurface S : AllSurfaces)
	{
		MaxCost = FMath::Max(MaxCost, URTTerrainLibrary::FindTerrainDef(S).MoveCost);
	}
	TestTrue(TEXT("il terreno piu' caro del catalogo non arriva all'altezza di un muro"),
		URTHexLibrary::ReliefHeightForCost(MaxCost) < URTHexLibrary::MovementBlockerHeight);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

/**
 * Leggibilita' tattica (pilastro di prodotto): l'overlay serve a far capire le regole della mappa, quindi due
 * superfici diverse NON possono avere lo stesso colore, e nessuna puo' somigliare al marcatore delle celle
 * bloccate. Sono le due condizioni che rendono l'overlay informativo invece di decorativo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexSurfaceColorTest,
	"RefactorTactics.Hex.SurfaceColorsAreDistinguishable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexSurfaceColorTest::RunTest(const FString&)
{
	const TArray<ERTHexSurface> All = {
		ERTHexSurface::Floor, ERTHexSurface::ShallowWater, ERTHexSurface::Rough, ERTHexSurface::Fire,
		ERTHexSurface::Conductive, ERTHexSurface::Ice, ERTHexSurface::Void,
		ERTHexSurface::Smoke, ERTHexSurface::HighGround
	};

	// Distanza percettiva grossolana: somma delle differenze per canale. Due colori troppo vicini a schermo
	// sono indistinguibili, e un overlay indistinguibile non aiuta a leggere la mappa.
	auto Distance = [](const FColor& A, const FColor& B)
	{
		return FMath::Abs(A.R - B.R) + FMath::Abs(A.G - B.G) + FMath::Abs(A.B - B.B);
	};

	for (int32 I = 0; I < All.Num(); ++I)
	{
		for (int32 J = I + 1; J < All.Num(); ++J)
		{
			const FColor CI = URTHexLibrary::SurfaceColor(All[I]);
			const FColor CJ = URTHexLibrary::SurfaceColor(All[J]);
			TestTrue(*FString::Printf(TEXT("superfici %d e %d hanno colori distinguibili (distanza %d)"),
					static_cast<int32>(All[I]), static_cast<int32>(All[J]), Distance(CI, CJ)),
				Distance(CI, CJ) >= 60);
		}
	}

	// Il rosso del marcatore "blocca il movimento" non deve confondersi con una superficie.
	for (ERTHexSurface S : All)
	{
		TestTrue(*FString::Printf(TEXT("la superficie %d non si confonde col marcatore di blocco"),
				static_cast<int32>(S)),
			Distance(URTHexLibrary::SurfaceColor(S), URTHexLibrary::BlockedCellColor()) >= 60);
	}
	return true;
}
