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
 * `#879`: la risoluzione del click e' **pura e qui**, non nel modulo Editor dove nessun test la raggiunge.
 *
 * Il chiamante decide se il colpo VALE — componente giusto, layer giusto — e passa la risposta come
 * `bHasValidHit`. Questo test tiene ferma la conseguenza di quella decisione: con il colpo si usa il colpo,
 * senza si usa il piano, e i due danno celle DIVERSE. Se non fossero diverse il test passerebbe anche
 * scambiando il ramo, che e' esattamente la mutazione da cui difende.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexResolveRayValidatedHitTest,
	"RefactorTactics.Hex.ResolveRayToCellOnLayerUsesValidatedHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexResolveRayValidatedHitTest::RunTest(const FString&)
{
	const FVector Origin = FVector::ZeroVector;
	const float HexSize = 100.f;
	const float LayerHeight = 250.f;
	const int32 ActiveLayer = 1;

	// Raggio verticale che colpisce il piano del layer attivo esattamente sul centro di (0,0,1).
	const FVector RayOrigin(0.0, 0.0, 1000.0);
	const FVector RayDir(0.0, 0.0, -1.0);
	const FRTCellId PlaneCell(0, 0, ActiveLayer);
	const FRTCellId HitCell(2, 1, ActiveLayer);
	const FVector HitPoint = URTHexLibrary::AxialToWorld(HitCell, Origin, HexSize, LayerHeight);

	// Le due celle DEVONO essere diverse, o il resto del test non proverebbe niente.
	TestTrue(TEXT("il caso e' discriminante: cella del colpo != cella del piano"), HitCell != PlaneCell);

	const FRTCellId WithHit = URTHexLibrary::ResolveRayToCellOnLayer(RayOrigin, RayDir,
		Origin, HexSize, LayerHeight, ActiveLayer, /*bHasValidHit=*/ true, HitPoint);
	TestTrue(FString::Printf(TEXT("colpo valido -> cella del colpo (%s)"), *WithHit.ToString()),
		WithHit == HitCell);

	// Stesso raggio, stesso HitPoint, colpo NON valido: il punto d'impatto va ignorato del tutto.
	const FRTCellId WithoutHit = URTHexLibrary::ResolveRayToCellOnLayer(RayOrigin, RayDir,
		Origin, HexSize, LayerHeight, ActiveLayer, /*bHasValidHit=*/ false, HitPoint);
	TestTrue(FString::Printf(TEXT("colpo scartato -> cella del piano (%s)"), *WithoutHit.ToString()),
		WithoutHit == PlaneCell);

	return true;
}

/**
 * `#879`: il piano su cui si risolve e' quello del layer **attivo**, non quello dedotto dalla quota del
 * raggio o del colpo. E' la ragione per cui il chiamante scarta i colpi di un altro piano: proiettarli
 * sposterebbe la cella in orizzontale di circa `LayerHeight` con la camera obliqua del viewport.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexResolveRayActivePlaneTest,
	"RefactorTactics.Hex.ResolveRayToCellOnLayerFallsBackToActivePlane",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexResolveRayActivePlaneTest::RunTest(const FString&)
{
	const FVector Origin(1000.0, -500.0, 200.0);
	const float HexSize = 100.f;
	const float LayerHeight = 250.f;

	// Raggio OBLIQUO: la quota a cui incontra il piano cambia il punto X/Y, quindi il layer attivo non
	// e' solo un'etichetta sul risultato — sposta davvero la cella. Un raggio verticale non lo direbbe.
	//
	// La pendenza non e' arbitraria: con `dX = 2*dZ` un piano in piu' sposta il punto di `2*LayerHeight`
	// = 500 unita', contro una larghezza di cella pointy-top di `sqrt(3)*HexSize` ~ 173. Il margine e'
	// ~2.9 celle, quindi l'asserzione «la cella cambia» non dipende da dove cadono gli arrotondamenti.
	// Con una pendenza piu' ripida (`dX = dZ/2`, 125 unita') il passo sarebbe MINORE di una cella e il
	// test fallirebbe a intermittenza per una ragione che non ha niente a che vedere con la regola.
	const FVector RayOrigin(1000.0, -500.0, 200.0 + 2000.0);
	const FVector RayDir = FVector(2.0, 0.0, -1.0).GetSafeNormal();

	FRTCellId Previous;
	for (int32 Layer = 0; Layer <= 3; ++Layer)
	{
		const FRTCellId Cell = URTHexLibrary::ResolveRayToCellOnLayer(RayOrigin, RayDir,
			Origin, HexSize, LayerHeight, Layer, /*bHasValidHit=*/ false, FVector::ZeroVector);

		TestEqual(FString::Printf(TEXT("layer %d: la cella sta sul piano attivo"), Layer), Cell.Layer, Layer);

		if (Layer > 0)
		{
			// Salendo di un piano il raggio obliquo incontra il piano piu' vicino alla camera: la coppia
			// assiale cambia. Se restasse identica, il piano non starebbe entrando nel calcolo.
			TestTrue(FString::Printf(TEXT("layer %d: la coppia assiale si sposta col piano"), Layer),
				Cell.X != Previous.X || Cell.Y != Previous.Y);
		}
		Previous = Cell;
	}

	// `LayerHeight` 0: tutti i piani coincidono, quindi la coppia assiale non puo' dipendere dal layer.
	{
		const FRTCellId A = URTHexLibrary::ResolveRayToCellOnLayer(RayOrigin, RayDir,
			Origin, HexSize, 0.f, 0, /*bHasValidHit=*/ false, FVector::ZeroVector);
		const FRTCellId B = URTHexLibrary::ResolveRayToCellOnLayer(RayOrigin, RayDir,
			Origin, HexSize, 0.f, 2, /*bHasValidHit=*/ false, FVector::ZeroVector);
		TestTrue(TEXT("LayerHeight 0: stessa coppia assiale su piani diversi"), A.X == B.X && A.Y == B.Y);
		TestEqual(TEXT("LayerHeight 0: il layer resta quello dichiarato"), B.Layer, 2);
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
 * Ingombro-mondo di un insieme di celle: cio' che serve per dire «fammi vedere TUTTO» (`#623`, seduta `U21`).
 *
 * Distinto dal centroide qui sopra, e non ne e' un'estensione: il centroide risponde *dove guardare*, questo
 * *quanto largo*. Una camera che inquadrasse il centroide con una distanza fissa taglierebbe le mappe grandi
 * e sprecherebbe schermo su quelle piccole.
 *
 * ⚠️ Il valore atteso e' ricalcolato QUI da `FMath::Sqrt(3.0)` invece di riusare la costante del modulo:
 * un test che prende il numero dalla stessa fonte dell'implementazione verifica che due copie siano uguali,
 * non che il numero sia giusto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCellsBoundsTest,
	"RefactorTactics.Hex.CellsBoundsWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCellsBoundsTest::RunTest(const FString&)
{
	const FVector Origin(500.0, -200.0, 30.0);
	const float HexSize = 100.f;
	const float LayerHeight = 250.f;

	// Semi-estensione di un esagono pointy-top di circumraggio `HexSize`, dai suoi vertici: i sei angoli
	// stanno a -30 + 60k gradi, quindi |cos| massimo e' sqrt(3)/2 e |sin| massimo e' 1. La punta e' su Y.
	const double HalfX = static_cast<double>(HexSize) * FMath::Sqrt(3.0) * 0.5;
	const double HalfY = static_cast<double>(HexSize);

	// Nessuna cella: box NON VALIDO. Non e' un dettaglio: un box degenere sull'origine sarebbe un'inquadratura
	// plausibile e sbagliata su una mappa vuota, ed e' il difetto che `rt.Arena.Check` esiste per denunciare.
	// ⚠️ `TArray<FRTCellId>{}` esplicito e non `{}`: con due overload la lista vuota e' ambigua, e il
	// compilatore lo dice. Entrambi devono rispondere «non valido», quindi si verificano tutti e due.
	TestFalse(TEXT("insieme vuoto (id) -> box non valido, non un punto inventato"),
		URTHexLibrary::CellsBoundsWorld(TArray<FRTCellId>{}, Origin, HexSize, LayerHeight).IsValid != 0);
	TestFalse(TEXT("insieme vuoto (celle) -> box non valido"),
		URTHexLibrary::CellsBoundsWorld(TArray<FRTHexCellData>{}, Origin, HexSize, LayerHeight).IsValid != 0);

	// Una cella: il suo INGOMBRO, non il suo centro. E' la parte che una implementazione ingenua sbaglia —
	// prendere solo i centri taglia mezza cella su ogni bordo della mappa.
	const FRTCellId Single(2, -1, 0);
	const FVector Centre = URTHexLibrary::AxialToWorld(Single, Origin, HexSize, LayerHeight);
	const FBox One = URTHexLibrary::CellsBoundsWorld(TArray<FRTCellId>{ Single }, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("una cella -> box valido"), One.IsValid != 0);
	TestTrue(TEXT("una cella -> min = centro - semi-estensione"),
		One.Min.Equals(FVector(Centre.X - HalfX, Centre.Y - HalfY, Centre.Z), 0.01));
	TestTrue(TEXT("una cella -> max = centro + semi-estensione"),
		One.Max.Equals(FVector(Centre.X + HalfX, Centre.Y + HalfY, Centre.Z), 0.01));

	// Celle LONTANE DALL'ORIGINE: il box le segue invece di restare attorno a (0,0). E' il caso che
	// `PIE-MAPED-FRAME` chiede di allestire, e qui lo si pinna headless.
	const FRTCellId Far(30, -12, 0);
	const FVector FarCentre = URTHexLibrary::AxialToWorld(Far, Origin, HexSize, LayerHeight);
	const FBox FarBox = URTHexLibrary::CellsBoundsWorld(TArray<FRTCellId>{ Far }, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("cella lontana -> il box la contiene"), FarBox.IsInsideOrOn(FarCentre));
	TestFalse(TEXT("cella lontana -> il box NON contiene l'origine"), FarBox.IsInsideOrOn(Origin));

	// Due celle sullo stesso layer: il box copre entrambi gli ingombri.
	const FRTCellId A(0, 0, 0);
	const FRTCellId B(6, 0, 0);
	const FBox Two = URTHexLibrary::CellsBoundsWorld(TArray<FRTCellId>{ A, B }, Origin, HexSize, LayerHeight);
	const FVector WA = URTHexLibrary::AxialToWorld(A, Origin, HexSize, LayerHeight);
	const FVector WB = URTHexLibrary::AxialToWorld(B, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("due celle -> min in X copre la piu' a sinistra"),
		FMath::IsNearlyEqual(Two.Min.X, FMath::Min(WA.X, WB.X) - HalfX, 0.01));
	TestTrue(TEXT("due celle -> max in X copre la piu' a destra"),
		FMath::IsNearlyEqual(Two.Max.X, FMath::Max(WA.X, WB.X) + HalfX, 0.01));

	// MULTILIVELLO: e' il DoD di `#623` e il criterio di `PIE-MAPED-FRAME`. Il box deve coprire i layer
	// diversi da quello attivo, o il comando mostra solo il piano su cui si sta lavorando.
	const FRTCellId Low(0, 0, 0);
	const FRTCellId High(0, 0, 3);
	const FBox Stack = URTHexLibrary::CellsBoundsWorld(TArray<FRTCellId>{ Low, High }, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("multilivello -> min Z sul layer piu' basso"),
		FMath::IsNearlyEqual(Stack.Min.Z, Origin.Z, 0.01));
	TestTrue(TEXT("multilivello -> max Z sul layer piu' alto"),
		FMath::IsNearlyEqual(Stack.Max.Z, Origin.Z + 3.0 * static_cast<double>(LayerHeight), 0.01));

	// L'ordine dell'input non cambia il risultato: min/max sono una piega commutativa.
	const FBox Forward = URTHexLibrary::CellsBoundsWorld(TArray<FRTCellId>{ A, B, High }, Origin, HexSize, LayerHeight);
	const FBox Reversed = URTHexLibrary::CellsBoundsWorld(TArray<FRTCellId>{ High, B, A }, Origin, HexSize, LayerHeight);
	TestTrue(TEXT("ordine irrilevante"),
		Forward.Min.Equals(Reversed.Min, 0.01) && Forward.Max.Equals(Reversed.Max, 0.01));

	// QUOTA D'AUTORE: l'overload su `FRTHexCellData` deve alzare il box, perche' `RebuildInstances` alza
	// la cella nel render. Trovato in code review, ed e' un difetto **latente**: al 2026-08-17 nessun
	// produttore scrive `Height` — l'unica assegnazione in `Source/` sta in `RTHexDoorTests.cpp`.
	FRTHexCellData Raised(FRTCellId(0, 0, 0));
	Raised.Height = 600;
	const FBox WithHeight = URTHexLibrary::CellsBoundsWorld(TArray<FRTHexCellData>{ Raised },
		Origin, HexSize, LayerHeight);
	TestTrue(TEXT("quota d'autore -> il box sale con la cella"),
		FMath::IsNearlyEqual(WithHeight.Max.Z, Origin.Z + 600.0, 0.01));

	// E con quota zero i due overload devono coincidere: se divergessero, il piu' usato dei due
	// racconterebbe una mappa diversa dall'altro.
	FRTHexCellData Flat(FRTCellId(2, -1, 0));
	const FBox ViaData = URTHexLibrary::CellsBoundsWorld(TArray<FRTHexCellData>{ Flat },
		Origin, HexSize, LayerHeight);
	const FBox ViaId = URTHexLibrary::CellsBoundsWorld(TArray<FRTCellId>{ Flat.Id },
		Origin, HexSize, LayerHeight);
	TestTrue(TEXT("quota zero -> i due overload coincidono"),
		ViaData.Min.Equals(ViaId.Min, 0.01) && ViaData.Max.Equals(ViaId.Max, 0.01));
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
 * Lo stesso bordo fisico ha lo stesso centro, visto dalle DUE celle che lo condividono.
 *
 * E' l'invariante che rende la primitiva utilizzabile: coperture e porte si dichiarano **per cella**
 * (`FRTHexCover::Edge` e' «visto DALLA cella»), quindi lo stesso muretto puo' essere descritto da una parte o
 * dall'altra. Se i due punti non coincidessero, la stessa copertura apparirebbe in due posti diversi a
 * seconda di chi la dichiara — e nessuno saprebbe quale dei due e' il bordo vero.
 *
 * Verifica anche che il punto stia sul bordo e non da qualche altra parte: la distanza dal centro della cella
 * dev'essere l'apotema (sqrt(3)/2 volte la dimensione), non il raggio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexEdgeMidpointTest,
	"RefactorTactics.Hex.EdgeMidpointIsSharedByBothCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexEdgeMidpointTest::RunTest(const FString&)
{
	const FVector Origin(1000.0, -500.0, 250.0); // origine NON banale: un bug che ignora l'origine si vede
	constexpr float HexSize = 100.f;
	constexpr float LayerHeight = 250.f;
	const FRTCellId Cell(2, -1, 0);

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		const ERTHexDirection Back = URTHexLibrary::OppositeDirection(Dir);
		const FRTCellId Other = URTHexLibrary::Neighbor(Cell, Dir);

		// 1. Lo stesso bordo dalle due parti.
		const FVector FromHere = URTHexLibrary::EdgeMidpointWorld(Cell, Dir, Origin, HexSize, LayerHeight);
		const FVector FromThere = URTHexLibrary::EdgeMidpointWorld(Other, Back, Origin, HexSize, LayerHeight);
		TestTrue(*FString::Printf(TEXT("direzione %d: stesso bordo dalle due celle"), D),
			FromHere.Equals(FromThere, 0.01));

		// 2. Il punto sta sul bordo: distanza dal centro = apotema, non raggio.
		const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);
		const double Apothem = FMath::Sqrt(3.0) / 2.0 * static_cast<double>(HexSize);
		TestTrue(*FString::Printf(TEXT("direzione %d: il punto e' sul bordo (apotema)"), D),
			FMath::IsNearlyEqual(FVector::Dist(Center, FromHere), Apothem, 0.01));

		// 3. L'opposto dell'opposto e' se stesso: la tabella delle direzioni non e' scritta a mano.
		TestTrue(TEXT("opposto involutivo"), URTHexLibrary::OppositeDirection(Back) == Dir);

		// 4. Guardato dalle due parti, il pannello e' ruotato di 180 gradi.
		const float YawHere = URTHexLibrary::EdgeRotation(Cell, Dir).Yaw;
		const float YawThere = URTHexLibrary::EdgeRotation(Other, Back).Yaw;
		const float Delta = FMath::Abs(FRotator::NormalizeAxis(YawHere - YawThere));
		TestTrue(*FString::Printf(TEXT("direzione %d: yaw opposto (delta %.1f)"), D, Delta),
			FMath::IsNearlyEqual(Delta, 180.f, 0.5f));
	}

	// Il bordo segue il LAYER: su un piano diverso il punto sale di LayerHeight, o le coperture della
	// piattaforma finirebbero disegnate a terra.
	const FVector Ground = URTHexLibrary::EdgeMidpointWorld(FRTCellId(0, 0, 0), ERTHexDirection::E,
		Origin, HexSize, LayerHeight);
	const FVector Upper = URTHexLibrary::EdgeMidpointWorld(FRTCellId(0, 0, 1), ERTHexDirection::E,
		Origin, HexSize, LayerHeight);
	TestTrue(TEXT("il bordo sale di un piano"),
		FMath::IsNearlyEqual(Upper.Z - Ground.Z, static_cast<double>(LayerHeight), 0.01));
	return true;
}


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

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.
#endif // WITH_DEV_AUTOMATION_TESTS
