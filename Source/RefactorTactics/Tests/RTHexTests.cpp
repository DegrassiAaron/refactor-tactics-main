#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"          // GetCellPrismMesh: la mesh generata della cella (#712 / U22)
#include "Map/RTHexMapAsset.h"          // HexSize dal CDO: la scala non si scrive in uu (#1992)
#include "Map/RTGeometryGrammar.h"      // SnapToGrammar: il ghost parte da qui
#include "Map/RTGeometryBake.h"         // EdgesTouchedBy: e finisce qui
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"        // FStaticMeshRenderData / FPositionVertexBuffer
#include "PhysicsEngine/BodySetup.h"
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
 * **Il contratto geometrico del marker di facing** (#1992, Epic #1990, `D-304`): sei test, uno per voce
 * `D007`, sulla trasformazione `Facing + BodyRadius + FaceHeight -> MarkerOrigin`.
 *
 * 🔑 **Perche' la funzione e' stata estratta.** Il fixture che la consuma e' un `.uasset`, e un Blueprint
 * non si esercita headless: senza una funzione pura questa issue non avrebbe **nessun** oracolo.
 *
 * ⚠️ **Cio' che questi sei NON provano: che il Blueprint la chiami.** Un fixture che calcolasse l'origine
 * per conto proprio — o che ignorasse `BodyRadius` — li lascerebbe tutti verdi. Quel legame e' verifica
 * d'editor (`D008.2` della issue), ed e' dichiarato **li'** invece di essere lasciato credere qui.
 *
 * ⛔ **I raggi e le quote usati sotto sono INGRESSI, non default canonici.** `GBX-5` — l'ingombro
 * dell'unita' rispetto alla cella — resta aperta e di #1094, e si chiude a `U25`. Le asserzioni sono
 * **relazioni** fra ingresso e uscita: cadono se la formula cambia, non se cambia la scala.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFacingMarkerOnSurfaceTest,
	"RefactorTactics.Hex.FacingMarkerOriginSitsOnBodySurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFacingMarkerOnSurfaceTest::RunTest(const FString&)
{
	// D007.1 — l'origine sta sulla SUPERFICIE del corpo: distanza IN PIANTA dal centro pari a BodyRadius.
	// In pianta e non in 3D, altrimenti l'asserzione includerebbe FaceHeight e misurerebbe due cose insieme.
	const FVector Center(1234.0, -567.0, 89.0); // centro non banale: uno a zero nasconderebbe un offset perso
	const float BodyRadius = 60.f;
	const float FaceHeight = 95.f;

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		const FVector O = URTHexLibrary::FacingMarkerOrigin(Dir, Center, BodyRadius, FaceHeight);
		const double PlanarDist = FVector::Dist2D(O, Center);
		TestTrue(*FString::Printf(TEXT("direzione %d: l'origine e' sulla superficie (%.3f atteso %.3f)"),
			D, PlanarDist, BodyRadius),
			FMath::IsNearlyEqual(PlanarDist, static_cast<double>(BodyRadius), 0.01));
	}
	return true;
}

/**
 * D007.2 — la direzione del marker e' quella VERSO IL VICINO, e il riferimento si ricava dal mondo.
 *
 * ⚠️ Il valore atteso NON viene da `EdgeRotation` (sarebbe tautologico) ne' da sei angoli in tabella: si
 * costruisce con `AxialToWorld` + `Neighbor`, cioe' per un'altra strada. E' la stessa disciplina di
 * `EdgeIndexMatchesNeighbourDirection`: se un giorno qualcuno riscrivesse `FacingRotation` come un `Atan2`
 * diviso in spicchi, questo test lo direbbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFacingMatchesNeighbourTest,
	"RefactorTactics.Hex.FacingRotationMatchesNeighbourDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFacingMatchesNeighbourTest::RunTest(const FString&)
{
	// Scala derivata dal CDO, non scritta in uu: un test che confrontasse 150 con 150 passerebbe anche
	// dopo un cambio di HexSize, cioe' esattamente quando dovrebbe fallire.
	const float HexSize = GetDefault<URTHexMapAsset>()->HexSize;
	const FVector Origin(0.0, 0.0, 0.0);
	constexpr float LayerHeight = 250.f;
	const FRTCellId Cell(3, -2, 0);
	const FVector Center = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		const FVector NeighbourCenter =
			URTHexLibrary::AxialToWorld(URTHexLibrary::Neighbor(Cell, Dir), Origin, HexSize, LayerHeight);
		const FVector Expected = (NeighbourCenter - Center).GetSafeNormal();
		const FVector Actual = URTHexLibrary::FacingRotation(Dir).Vector();
		TestTrue(*FString::Printf(TEXT("direzione %d: il marker guarda il vicino"), D),
			Actual.Equals(Expected, 1.e-3));
	}
	return true;
}

/**
 * D007.3 — `Up` e' il verso del MONDO: le sei origini sono complanari, alla quota `Center.Z + FaceHeight`.
 *
 * 🔴 **Il bug che questo test esiste per prendere non fallisce oggi.** Un'implementazione che ruotasse in
 * blocco l'offset `(BodyRadius, 0, FaceHeight)` darebbe lo **stesso** risultato finche' la rotazione e' di
 * solo yaw, e passerebbe D007.1 e D007.5. Comincerebbe a mentire il giorno in cui `EdgeRotation`
 * acquisisse pitch — terreno inclinato, multilivello — e il marker salirebbe di quota a seconda della
 * direzione. Costa una riga e fissa la verticale adesso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFacingMarkerWorldUpTest,
	"RefactorTactics.Hex.FacingMarkerOriginKeepsWorldUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFacingMarkerWorldUpTest::RunTest(const FString&)
{
	const FVector Center(-300.0, 720.0, 45.0);
	const float BodyRadius = 60.f;
	const float FaceHeight = 95.f;
	const double ExpectedZ = Center.Z + static_cast<double>(FaceHeight);

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		const FVector O = URTHexLibrary::FacingMarkerOrigin(Dir, Center, BodyRadius, FaceHeight);
		TestTrue(*FString::Printf(TEXT("direzione %d: quota %.3f, attesa %.3f"), D, O.Z, ExpectedZ),
			FMath::IsNearlyEqual(O.Z, ExpectedZ, 0.01));
	}
	return true;
}

/**
 * D007.4 — `FacingRotation` scarta la cella, e **questo va misurato**.
 *
 * ⚠️ E' l'asserzione che rende `FacingRotation` un alias legittimo invece di una seconda risposta. E'
 * lecito scartare la cella solo perche' `AxialToWorld` e' AFFINE in `(q,r)`: lo spostamento fra due centri
 * dipende dal solo delta assiale. Se quella funzione smettesse di esserlo — un'origine per layer, una
 * deformazione — la scorciatoia mentirebbe **in silenzio**, e nient'altro nel repository se ne accorgerebbe.
 *
 * Le celle sono sparse di proposito: origine, assi, diagonali, quadranti opposti e due layer diversi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFacingCellIndependentTest,
	"RefactorTactics.Hex.FacingRotationIsCellIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFacingCellIndependentTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Sparse = {
		FRTCellId(0, 0, 0), FRTCellId(7, 0, 0), FRTCellId(0, 7, 0), FRTCellId(-5, 3, 0),
		FRTCellId(4, -9, 0), FRTCellId(-11, -2, 0), FRTCellId(2, 2, 1), FRTCellId(-3, 6, 2)
	};

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		const FRotator Reference = URTHexLibrary::FacingRotation(Dir);
		for (const FRTCellId& Cell : Sparse)
		{
			TestTrue(*FString::Printf(TEXT("direzione %d da %s: stesso orientamento"), D, *Cell.ToString()),
				URTHexLibrary::EdgeRotation(Cell, Dir).Equals(Reference, 0.01f));
		}
	}
	return true;
}

/**
 * D007.5 — le sei origini sono DISTINTE fra loro.
 *
 * ⚠️ **Senza questa asserzione un bug che ignora `Facing` passerebbe**: sei chiamate che tornano lo stesso
 * punto soddisfano «sta sulla superficie» (D007.1) e «stessa quota» (D007.3) per tutte e sei.
 *
 * La soglia non e' un epsilon: due direzioni adiacenti distano 60 gradi, quindi su un raggio `R` i loro
 * punti distano `R` esatti. Confrontare con `R/2` separa un bug vero da un errore numerico senza dipendere
 * da quanto vale `R`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFacingSixDistinctTest,
	"RefactorTactics.Hex.FacingMarkerOriginsAreSixDistinctPoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFacingSixDistinctTest::RunTest(const FString&)
{
	const FVector Center(10.0, -20.0, 30.0);
	const float BodyRadius = 60.f;
	const float FaceHeight = 95.f;

	TArray<FVector> Origins;
	for (int32 D = 0; D < 6; ++D)
	{
		Origins.Add(URTHexLibrary::FacingMarkerOrigin(
			static_cast<ERTHexDirection>(D), Center, BodyRadius, FaceHeight));
	}

	const double MinSeparation = static_cast<double>(BodyRadius) * 0.5;
	for (int32 I = 0; I < Origins.Num(); ++I)
	{
		for (int32 J = I + 1; J < Origins.Num(); ++J)
		{
			const double Dist = FVector::Dist(Origins[I], Origins[J]);
			TestTrue(*FString::Printf(TEXT("direzioni %d e %d sono distinte (distano %.2f)"), I, J, Dist),
				Dist > MinSeparation);
		}
	}
	return true;
}

/**
 * D007.6 — `BodyRadius = 0` degenera al centro.
 *
 * E' il caso limite che dice se la formula e' **quella** o una che le somiglia: un'implementazione che
 * sommasse anche `MarkerLength`, o che partisse dall'apotema della cella invece che dal raggio del corpo,
 * qui non tornerebbe al centro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexFacingZeroRadiusTest,
	"RefactorTactics.Hex.FacingMarkerOriginDegeneratesAtZeroRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexFacingZeroRadiusTest::RunTest(const FString&)
{
	const FVector Center(500.0, 500.0, 12.0);
	const float FaceHeight = 95.f;
	const FVector Expected = Center + FVector(0.0, 0.0, static_cast<double>(FaceHeight));

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		const FVector O = URTHexLibrary::FacingMarkerOrigin(Dir, Center, /*BodyRadius=*/ 0.f, FaceHeight);
		TestTrue(*FString::Printf(TEXT("direzione %d: raggio nullo -> centro"), D), O.Equals(Expected, 0.01));
	}
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

	// ── CP 47.3 (#956): il SECONDO canale ──────────────────────────────────────────────────────────
	//
	// 🔴 **La guardia che mancava.** L'elenco qui sopra e' scritto a mano perche' l'enum non dichiara
	// `TEnumRange`, e finora nulla lo teneva allineato: una decima superficie sarebbe nata SCOPERTA da
	// entrambi i canali, senza che un solo test cadesse. E' il difetto che `AllOutcomes` ha gia' pagato per
	// `ERTStartupOutcome`, dove la lista a mano e l'enum avevano divergito.
	const UEnum* SurfaceEnum = StaticEnum<ERTHexSurface>();
	if (TestNotNull(TEXT("l'enum delle superfici e' riflesso"), SurfaceEnum))
	{
		// `NumEnums()` include il `_MAX` sintetico che UHT aggiunge: si sottrae.
		TestEqual(TEXT("l'elenco di questo test copre TUTTE le superfici dell'enum"),
			SurfaceEnum->NumEnums() - 1, All.Num());
	}

	// Luminanza Rec.601: e' la conversione con cui si guarda uno screenshot in scala di grigi, ed e' li' che
	// #956 ha misurato **7 coppie su 36** collassate — una board che si legge a colori e non si legge senza.
	auto Luma = [](const FColor& C)
	{
		return 0.299f * C.R + 0.587f * C.G + 0.114f * C.B;
	};

	// La soglia e' **derivata da quella del colore, non scelta**: su due grigi la somma per canale vale tre
	// volte la differenza di luminanza, quindi `60 / 3 = 20`. Un secondo numero arbitrario avrebbe reso il
	// gate piu' severo o piu' lasco senza che nessuno sapesse dire di quanto.
	const float LumaThreshold = 60.f / 3.f;

	for (int32 I = 0; I < All.Num(); ++I)
	{
		for (int32 J = I + 1; J < All.Num(); ++J)
		{
			// ⛔ **L'unica esenzione, dichiarata invece che scoperta a consuntivo** (criterio 2 di #956):
			// `Floor~Fire` collassa in entrambe le conversioni e nessuna delle due riceve un glifo. Per
			// quella coppia la regola del titolo — «colore E forma» — non e' rinviata: NON SI APPLICA.
			const bool bEsente =
				(All[I] == ERTHexSurface::Floor && All[J] == ERTHexSurface::Fire)
				|| (All[I] == ERTHexSurface::Fire && All[J] == ERTHexSurface::Floor);
			if (bEsente) { continue; }

			const float DLuma = FMath::Abs(Luma(URTHexLibrary::SurfaceColor(All[I]))
				- Luma(URTHexLibrary::SurfaceColor(All[J])));
			const bool bFormaSepara =
				URTHexLibrary::SurfaceRingCount(All[I]) != URTHexLibrary::SurfaceRingCount(All[J]);

			// **Colore OPPURE forma**, non «i quattro glifi sono diversi»: e' la regola di `D-146` applicata
			// alla coppia, e l'unica che dica qualcosa su una board vista in scala di grigi.
			TestTrue(*FString::Printf(
					TEXT("superfici %d e %d: separate in scala di grigi (dLuma %.1f, soglia %.1f) o dalla forma (%d vs %d)"),
					static_cast<int32>(All[I]), static_cast<int32>(All[J]), DLuma, LumaThreshold,
					URTHexLibrary::SurfaceRingCount(All[I]), URTHexLibrary::SurfaceRingCount(All[J])),
				DLuma >= LumaThreshold || bFormaSepara);
		}
	}

	return true;
}

/**
 * #712 / seduta `U22`: il pieno della cella e il contorno evidenziato devono nascere dalla STESSA
 * definizione di esagono.
 *
 * ⚠️ Il difetto che questo test chiude e' stato visto a schermo, non trovato qui: le celle si vedevano come
 * dischi perche' erano istanze di `/Engine/BasicShapes/Cylinder`, mentre il contorno veniva da `HexCorners`.
 * Due percorsi per la stessa forma, e nessuna asserzione che li tenesse insieme — quindi il bordo era un
 * esagono e il pieno un cerchio, per quanto entrambi fossero "giusti" ciascuno per conto suo.
 *
 * Il test non chiede che la mesh *sia* esagonale: chiede che i suoi vertici **coincidano** con quelli di
 * `HexCorners`. E' la differenza fra verificare una forma e verificare che due disegni non possano divergere.
 *
 * ⚠️ Sta in `RTHexTests.cpp` e non in `RTHexMapActorTests.cpp`, che sarebbe il posto naturale: quel file e'
 * nel `writable` di `content_editor`, e «il modulo e il suo test si toccano insieme» non autorizza a
 * prendersi un file che non si ha.
 */

/**
 * #956 / `D-183`: le corone del glifo nascono dagli STESSI vertici della cella.
 *
 * ⚠️ E' il test di `#712` applicato al secondo canale, e per la stessa ragione: due disegni della stessa
 * forma divergono appena qualcuno riscrive `cos(60k-30)` invece di chiedere a `HexCorners`. Li' il bordo
 * era un esagono e il pieno un cerchio; qui il glifo sarebbe ruotato rispetto alla cella che incide.
 *
 * Verifica anche i RAGGI, perche' e' li' che vive la leggibilita': gli anelli crescono verso l'interno dal
 * bordo del disco (`0,95`), e i raggi interni che ne risultano — `0,90 / 0,80 / 0,71 / 0,61` — sono i
 * numeri che `D-183` ha scelto guardando l'area, non lo spessore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlyphMeshMatchesHexCornersTest,
	"RefactorTactics.Hex.GlyphMeshMatchesHexCorners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlyphMeshMatchesHexCornersTest::RunTest(const FString&)
{
	constexpr float Radius = 50.f;
	constexpr float Outer = 0.95f;
	constexpr float Thickness = 0.0526f;
	constexpr float Gap = 0.0421f;

	// Mono-canale per scelta: nessun glifo, non un glifo vuoto.
	TestNull(TEXT("zero anelli non produce mesh"), ARTHexMapActor::GetCellGlyphMesh(0));

	for (int32 Rings = 1; Rings <= 4; ++Rings)
	{
		UStaticMesh* Mesh = ARTHexMapActor::GetCellGlyphMesh(Rings);
		if (!TestNotNull(*FString::Printf(TEXT("il glifo a %d anelli si costruisce"), Rings), Mesh))
		{
			continue;
		}

		// I raggi attesi: per ogni anello, esterno e interno.
		//
		// ⚠️ **Confronto con TOLLERANZA, non per chiave stringa.** La prima stesura usava `%.2f` come fa il
		// test del prisma, e cadeva su 4 vertici su 24: li' i raggi sono tondi (50), qui no (44,87), e un
		// valore che cade a meta' del centesimo arrotonda in modo diverso fra i due percorsi di calcolo. Il
		// difetto era nel metodo di verifica, non nella geometria.
		TArray<FVector2D> Expected;
		for (int32 I = 0; I < Rings; ++I)
		{
			const float RingOuter = Outer - I * (Thickness + Gap);
			for (const float R : { RingOuter, RingOuter - Thickness })
			{
				for (const FVector& C : URTHexLibrary::HexCorners(FVector::ZeroVector, Radius * R))
				{
					Expected.Add(FVector2D(C.X, C.Y));
				}
			}
		}

		const FStaticMeshRenderData* Render = Mesh->GetRenderData();
		if (!TestTrue(*FString::Printf(TEXT("il glifo a %d anelli ha render data"), Rings),
			Render != nullptr && Render->LODResources.Num() > 0))
		{
			continue;
		}

		const FPositionVertexBuffer& Buffer = Render->LODResources[0].VertexBuffers.PositionVertexBuffer;
		int32 Fuori = 0;
		for (uint32 V = 0; V < Buffer.GetNumVertices(); ++V)
		{
			const FVector3f P = Buffer.VertexPosition(V);
			bool bTrovato = false;
			for (const FVector2D& E : Expected)
			{
				// 0,05 uu su un raggio di 50: un millesimo. Separa l'errore di arrotondamento da un vertice
				// che sta su un raggio DIVERSO — i raggi adiacenti distano 2,6 uu, cinquanta volte tanto.
				if (FMath::Abs(P.X - E.X) < 0.05 && FMath::Abs(P.Y - E.Y) < 0.05) { bTrovato = true; break; }
			}
			if (!bTrovato) { ++Fuori; }
		}
		TestEqual(*FString::Printf(
			TEXT("glifo a %d anelli: ogni vertice sta su un raggio di HexCorners (fuori: %d su %u)"),
			Rings, Fuori, Buffer.GetNumVertices()), Fuori, 0);	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellPrismMatchesHexCornersTest,
	"RefactorTactics.Hex.CellPrismMatchesHexCorners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellPrismMatchesHexCornersTest::RunTest(const FString&)
{
	UStaticMesh* Mesh = ARTHexMapActor::GetCellPrismMesh();
	if (!TestNotNull(TEXT("il prisma della cella si costruisce"), Mesh))
	{
		return false;
	}

	// Le misure ereditate dal cilindro sostituito: `PlanarScale` divide per 50, e i lift di debug-line si
	// appoggiano a una mezza-altezza di 50. Se cambiano qui senza cambiare la', ogni quota si sposta in
	// silenzio — ed e' il motivo per cui il test le asserisce invece di leggerle dalla mesh.
	constexpr float ExpectedRadius = 50.f;
	constexpr float ExpectedHalfHeight = 50.f;

	const TArray<FVector> Expected = URTHexLibrary::HexCorners(FVector::ZeroVector, ExpectedRadius);
	TestEqual(TEXT("l'atteso ha sei vertici"), Expected.Num(), 6);

	auto KeyOf = [](double X, double Y) { return FString::Printf(TEXT("%.2f,%.2f"), X, Y); };

	TSet<FString> ExpectedKeys;
	for (const FVector& Corner : Expected)
	{
		ExpectedKeys.Add(KeyOf(Corner.X, Corner.Y));
	}

	// --- la mesh che si VEDE ---------------------------------------------------------------------------
	const FStaticMeshRenderData* Render = Mesh->GetRenderData();
	if (!TestTrue(TEXT("la mesh ha render data"), Render != nullptr && Render->LODResources.Num() > 0))
	{
		return false;
	}

	const FPositionVertexBuffer& Positions = Render->LODResources[0].VertexBuffers.PositionVertexBuffer;
	TestTrue(TEXT("la mesh ha vertici"), Positions.GetNumVertices() > 0);

	TSet<FString> MeshKeys;
	double MinZ = TNumericLimits<double>::Max();
	double MaxZ = TNumericLimits<double>::Lowest();
	for (uint32 Index = 0; Index < Positions.GetNumVertices(); ++Index)
	{
		const FVector3f P = Positions.VertexPosition(Index);
		MeshKeys.Add(KeyOf(P.X, P.Y));
		MinZ = FMath::Min(MinZ, static_cast<double>(P.Z));
		MaxZ = FMath::Max(MaxZ, static_cast<double>(P.Z));
	}

	// I vertici sono duplicati per faccia (ogni poligono ha le proprie istanze): conta quanti sono DISTINTI
	// in pianta, che e' l'unica cosa che descrive la forma vista dall'alto.
	TestEqual(TEXT("sei posizioni distinte in pianta"), MeshKeys.Num(), 6);
	TestTrue(TEXT("i vertici della mesh sono quelli di HexCorners"), MeshKeys.Difference(ExpectedKeys).IsEmpty());
	TestTrue(TEXT("faccia superiore a +50"), FMath::IsNearlyEqual(MaxZ, static_cast<double>(ExpectedHalfHeight), 0.01));
	TestTrue(TEXT("faccia inferiore a -50"), FMath::IsNearlyEqual(MinZ, static_cast<double>(-ExpectedHalfHeight), 0.01));

	// --- la forma che si CLICCA ------------------------------------------------------------------------
	// Il pick della cella e' un trace su collisione semplice e ricava la cella dall'indice dell'istanza: se
	// questa forma diverge da quella vista, si clicca una cella e se ne seleziona un'altra. Va asserita
	// separatamente proprio perche' nulla, nel codice, obbliga le due a coincidere.
	UBodySetup* Body = Mesh->GetBodySetup();
	if (!TestNotNull(TEXT("il prisma ha un body setup"), Body))
	{
		return false;
	}
	TestEqual(TEXT("una sola forma convessa"), Body->AggGeom.ConvexElems.Num(), 1);
	if (Body->AggGeom.ConvexElems.Num() != 1)
	{
		return false;
	}

	TSet<FString> HullKeys;
	for (const FVector& P : Body->AggGeom.ConvexElems[0].VertexData)
	{
		HullKeys.Add(KeyOf(P.X, P.Y));
	}
	TestEqual(TEXT("dodici vertici nello scafo convesso"), Body->AggGeom.ConvexElems[0].VertexData.Num(), 12);
	TestTrue(TEXT("la collisione ha la stessa pianta della mesh"), HullKeys.Difference(ExpectedKeys).IsEmpty());
	TestEqual(TEXT("e le stesse sei posizioni"), HullKeys.Num(), 6);

	return true;
}

/**
 * #712 / seduta `U22`: il ponte fra le due numerazioni dei bordi e' quello vero, e lo dice la geometria.
 *
 * 🔴 **La prima stesura di questo test fissava la convenzione SBAGLIATA.** Asseriva che un gesto sul lato
 * geometrico `k` producesse `ERTHexDirection(k)`, cioe' esattamente il `static_cast` che era il difetto:
 * i due sistemi girano in verso opposto e coincidono solo su `E` e `W`. Il test passava perche' ricopiava
 * l'errore invece di misurarlo — la stessa forma di cecita' dei sette test della cottura, che usavano solo
 * quelle due direzioni.
 *
 * Ora l'atteso viene dal **mondo** e non da una tabella: il bordo che guarda il vicino `D` e' quello il cui
 * punto medio giace nella direzione di `AxialToWorld(Neighbor(cell, D))`. Se le due numerazioni cambiassero
 * verso, questo test cadrebbe; se cambiassero **insieme e coerentemente**, resterebbe verde — che e'
 * esattamente cio' che deve fare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEdgeIndexMatchesNeighbourDirectionTest,
	"RefactorTactics.Hex.EdgeIndexMatchesNeighbourDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEdgeIndexMatchesNeighbourDirectionTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.f;
	const FRTCellId Cell{ 0, 0, 0 };
	const TArray<FVector> Corners = URTHexLibrary::HexCorners(FVector::ZeroVector, HexSize);
	if (!TestEqual(TEXT("sei vertici"), Corners.Num(), 6))
	{
		return false;
	}

	const FVector Here = URTHexLibrary::AxialToWorld(Cell, FVector::ZeroVector, HexSize, 0.f);

	for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(DirIndex);

		// Dove sta davvero il vicino, in coordinate-mondo.
		const FVector There = URTHexLibrary::AxialToWorld(
			URTHexLibrary::Neighbor(Cell, Dir), FVector::ZeroVector, HexSize, 0.f);
		const double NeighbourAngle = FMath::Atan2(There.Y - Here.Y, There.X - Here.X);

		// Il bordo geometrico che la libreria dice corrispondere a quella direzione.
		const int32 EdgeIndex = URTHexLibrary::EdgeIndexForDirection(Dir);
		if (!TestTrue(FString::Printf(TEXT("indice di bordo valido per %d"), DirIndex),
			Corners.IsValidIndex(EdgeIndex)))
		{
			continue;
		}

		// Il punto medio di quel bordo deve guardare nella stessa direzione del vicino.
		const FVector Mid = (Corners[EdgeIndex] + Corners[(EdgeIndex + 1) % 6]) * 0.5;
		const double MidAngle = FMath::Atan2(Mid.Y, Mid.X);
		const double Delta = FMath::Abs(FMath::UnwindRadians(MidAngle - NeighbourAngle));

		TestTrue(FString::Printf(
			TEXT("il bordo %d guarda il vicino %d (scarto %.2f gradi)"),
			EdgeIndex, DirIndex, FMath::RadiansToDegrees(Delta)), Delta < 0.01);

		// E il ponte deve essere invertibile: e' un rispecchiamento, quindi e' involutivo.
		TestEqual(FString::Printf(TEXT("il ponte per %d e' invertibile"), DirIndex),
			static_cast<int32>(URTHexLibrary::DirectionForEdgeIndex(EdgeIndex)), DirIndex);
	}

	return true;
}

/**
 * #712 / seduta `U22`: il pannello di un muro interno GIACE sul segmento che lo ha generato.
 *
 * 🔴 Non ci giaceva. La prima stesura orientava il pannello con lo yaw preso dall'angolo del muro, ma la
 * convenzione dei pannelli — quella che `EdgeRotation` segue per i bordi — mette lo **spessore sulla X** e
 * la **lunghezza sulla Y**. Il muro veniva quindi disegnato ruotato di un angolo retto rispetto al gesto, e
 * se n'e' accorto l'autore guardando lo schermo: *«i muri non seguono i vertici e il centro dell'esagono»*.
 *
 * ⚠️ E' la stessa forma di tutti gli altri difetti di questa seduta: due convenzioni che devono accordarsi
 * e nessuna asserzione che le tenga insieme. Il test lega il pannello al segmento invece di ricopiare
 * l'angolo atteso — un `TestEqual` su `+90` verificherebbe la formula contro se' stessa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteriorWallPanelFollowsTheSegmentTest,
	"RefactorTactics.HexMap.InteriorWallPanelFollowsTheSegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInteriorWallPanelFollowsTheSegmentTest::RunTest(const FString&)
{
	const FVector CellCentre(1000.0, -250.0, 40.0);
	constexpr float PanelHeight = 55.f;
	constexpr float PanelThickness = 0.10f;

	// Piu' giaciture, perche' un errore di 90 gradi su un caso solo puo' passare per caso.
	const TArray<double> Angles = { 0.0, 30.0, 60.0, 90.0, 137.0, 210.0 };
	for (const double Degrees : Angles)
	{
		const double Rad = FMath::DegreesToRadians(Degrees);
		const double Half = 45.0;
		const FVector2D A(-Half * FMath::Cos(Rad), -Half * FMath::Sin(Rad));
		const FVector2D B(+Half * FMath::Cos(Rad), +Half * FMath::Sin(Rad));

		const FTransform Panel = ARTHexMapActor::InteriorWallPanel(A, B, CellCentre, PanelHeight, PanelThickness);

		// 1. Il pannello e' CENTRATO sul segmento, in pianta.
		const FVector2D Mid = (A + B) * 0.5;
		TestTrue(FString::Printf(TEXT("a %.0f gradi il pannello e' centrato sul muro"), Degrees),
			FMath::IsNearlyEqual(Panel.GetLocation().X, CellCentre.X + Mid.X, 0.01)
			&& FMath::IsNearlyEqual(Panel.GetLocation().Y, CellCentre.Y + Mid.Y, 0.01));

		// 2. L'asse che PORTA LA LUNGHEZZA e' parallelo al muro. E' la riga che il difetto faceva fallire:
		//    con lo yaw lungo il muro questo asse risultava perpendicolare.
		const FVector AlongPanel = Panel.GetUnitAxis(EAxis::Y);
		const FVector2D AlongWall = (B - A).GetSafeNormal();
		const double Dot = FMath::Abs(AlongPanel.X * AlongWall.X + AlongPanel.Y * AlongWall.Y);
		TestTrue(FString::Printf(TEXT("a %.0f gradi la lunghezza del pannello segue il muro (|dot| %.3f)"),
			Degrees, Dot), Dot > 0.999);

		// 3. E lo SPESSORE gli e' perpendicolare, che e' l'altra meta' della stessa affermazione.
		const FVector Thick = Panel.GetUnitAxis(EAxis::X);
		const double DotThick = FMath::Abs(Thick.X * AlongWall.X + Thick.Y * AlongWall.Y);
		TestTrue(FString::Printf(TEXT("a %.0f gradi lo spessore e' perpendicolare (|dot| %.3f)"),
			Degrees, DotThick), DotThick < 0.001);

		// 4. La scala sulla Y rende il cubo lungo quanto il muro (il cubo engine e' 100 uu per lato).
		TestTrue(FString::Printf(TEXT("a %.0f gradi il pannello e' lungo quanto il muro"), Degrees),
			FMath::IsNearlyEqual(Panel.GetScale3D().Y * 100.0, FVector2D::Distance(A, B), 0.01));
	}

	return true;
}

/**
 * #712 / seduta `U22`: un muro lungo diventa UNA CATENA SENZA BUCHI, una porzione per cella.
 *
 * 🔴 L'autore: *«non si estende oltre il primo esagono»*. Il tool cuoceva solo la cella della pressione, e
 * dopo l'aggancio ai punti notevoli anche la geometria restava confinata li' — quei punti sono di quella
 * cella. La grammatica dei muri e' definita PER CELLA e non sa niente dei vicini: un muro lungo tre celle
 * e' quindi tre segmenti, e questo e' il taglio che li produce.
 *
 * ⚠️ La proprieta' che conta non e' «quante porzioni», e' la **continuita'**: la fine di una porzione deve
 * essere l'inizio della successiva, in coordinate-mondo. Un taglio che perde un pezzo fra due celle
 * produrrebbe un muro coi buchi, e il conteggio delle porzioni resterebbe giusto — e' il modo in cui un
 * test sul numero non vede il difetto che conta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSegmentSplitAcrossCellsIsContinuousTest,
	"RefactorTactics.Hex.SegmentSplitAcrossCellsIsContinuous",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSegmentSplitAcrossCellsIsContinuousTest::RunTest(const FString&)
{
	constexpr float HexSize = 100.f;
	const FVector Origin = FVector::ZeroVector;

	// --- 1. un gesto tutto dentro una cella resta una porzione sola ---------------------------------
	{
		TArray<URTHexLibrary::FRTCellSegment> Pieces;
		URTHexLibrary::SplitSegmentAcrossCells(
			FVector2D(-20.0, -10.0), FVector2D(20.0, 10.0), Origin, HexSize, 0, HexSize * 0.1f, Pieces);

		TestEqual(TEXT("dentro una cella: una porzione"), Pieces.Num(), 1);
		if (Pieces.Num() == 1)
		{
			TestTrue(TEXT("ed e' la cella d'origine"), Pieces[0].Cell == FRTCellId(0, 0, 0));
			TestTrue(TEXT("con gli estremi del gesto"),
				Pieces[0].LocalStart.Equals(FVector2D(-20.0, -10.0), 0.5f)
				&& Pieces[0].LocalEnd.Equals(FVector2D(20.0, 10.0), 0.5f));
		}
	}

	// --- 2. un gesto lungo attraversa piu' celle, e la catena non ha buchi --------------------------
	// Piu' giaciture, perche' un taglio puo' funzionare lungo un asse e perdere pezzi di traverso.
	const TArray<double> Directions = { 0.0, 23.0, 60.0, 91.0, 137.0 };
	for (const double Degrees : Directions)
	{
		const double Rad = FMath::DegreesToRadians(Degrees);
		const double Reach = static_cast<double>(HexSize) * 4.0;
		const FVector2D Start(-Reach * FMath::Cos(Rad), -Reach * FMath::Sin(Rad));
		const FVector2D End(Reach * FMath::Cos(Rad), Reach * FMath::Sin(Rad));

		TArray<URTHexLibrary::FRTCellSegment> Pieces;
		URTHexLibrary::SplitSegmentAcrossCells(Start, End, Origin, HexSize, 0, HexSize * 0.1f, Pieces);

		if (!TestTrue(FString::Printf(TEXT("a %.0f gradi il gesto lungo tocca piu' celle"), Degrees),
			Pieces.Num() >= 3))
		{
			continue;
		}

		// Le celle sono tutte diverse: una porzione per cella, non due.
		TSet<FString> Seen;
		FVector2D PreviousEndWorld = FVector2D::ZeroVector;
		bool bFirst = true;

		for (const URTHexLibrary::FRTCellSegment& Piece : Pieces)
		{
			const FString Key = FString::Printf(TEXT("%d,%d,%d"), Piece.Cell.X, Piece.Cell.Y, Piece.Cell.Layer);
			TestFalse(FString::Printf(TEXT("a %.0f gradi la cella %s compare una volta sola"), Degrees, *Key),
				Seen.Contains(Key));
			Seen.Add(Key);

			const FVector Centre = URTHexLibrary::AxialToWorld(Piece.Cell, Origin, HexSize, 0.f);
			const FVector2D CellCentre(Centre.X, Centre.Y);
			const FVector2D StartWorld = CellCentre + Piece.LocalStart;
			const FVector2D EndWorld = CellCentre + Piece.LocalEnd;

			// LA CONTINUITA': dove finisce una, comincia la successiva.
			if (!bFirst)
			{
				const double Gap = FVector2D::Distance(PreviousEndWorld, StartWorld);
				TestTrue(FString::Printf(
					TEXT("a %.0f gradi la catena non ha buchi fra le celle (scarto %.2f uu)"), Degrees, Gap),
					Gap < 0.5);
			}
			bFirst = false;
			PreviousEndWorld = EndWorld;

			// E ogni porzione sta DENTRO la propria cella: gli estremi non escono dal circumraggio.
			TestTrue(FString::Printf(TEXT("a %.0f gradi la porzione sta dentro la sua cella"), Degrees),
				Piece.LocalStart.Size() <= static_cast<double>(HexSize) + 0.5
				&& Piece.LocalEnd.Size() <= static_cast<double>(HexSize) + 0.5);
		}

		// E la catena copre il gesto da capo a fondo, invece di fermarsi alla prima cella.
		const FVector FirstCentre = URTHexLibrary::AxialToWorld(Pieces[0].Cell, Origin, HexSize, 0.f);
		const double Covered = FVector2D::Distance(
			FVector2D(FirstCentre.X, FirstCentre.Y) + Pieces[0].LocalStart, PreviousEndWorld);
		TestTrue(FString::Printf(TEXT("a %.0f gradi la catena copre il gesto (%.0f di %.0f uu)"),
			Degrees, Covered, 2.0 * Reach), Covered > 2.0 * Reach * 0.9);
	}

	return true;
}

// Chi aggiunge un test in fondo a questo file lo aggiunge PRIMA di questa riga: e' il difetto di #923,
// invisibile in Editor dove la guardia vale 1. Il controllo che lo dimostra e'
// `Build.bat RefactorTactics Win64 Shipping`, non la suite.
/**
 * 🔑 **L'ANCORA ASSOLUTA della bussola: `E` e' `+X`, e da li' `N` e' `-Y`.**
 *
 * 🔴 **Perche' non basta `FacingRotationMatchesNeighbourDirection`.** Quel test costruisce l'atteso con
 * `AxialToWorld` + `Neighbor` — un'altra strada, ma la **stessa origine**. Se qualcuno scambiasse `Wx` e
 * `Wy` dentro `AxialToWorld`, entrambi i lati ruoterebbero insieme e il test resterebbe **verde** mentre
 * l'intera mappa gira di 90 gradi sotto i piedi. La convenzione va dichiarata, non derivata: qui gli
 * angoli sono **letterali**, ed e' voluto — cambiarla deve costare la modifica di questa tabella.
 *
 * ⚠️ **Non e' un dettaglio da documento.** Nella seduta `U41` il verdetto sul facing e' stato
 * *«non so se e' corretto perche' non so qual e' il nord»*: senza questa ancora la domanda non ha
 * risposta nel repository. Owner in prosa: `spec-hex-geometry-authoring.md` §2.1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexCompassAnchorTest,
	"RefactorTactics.Hex.EastIsWorldPlusXAndTheSixYawsAreDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexCompassAnchorTest::RunTest(const FString&)
{
	// L'ancora, detta una volta e in chiaro.
	TestTrue(TEXT("E guarda world +X"),
		URTHexLibrary::FacingRotation(ERTHexDirection::E).Vector().Equals(FVector::XAxisVector, 1.e-3));

	// La tabella E' la convenzione: enum order E, NE, NW, W, SW, SE.
	const double DeclaredYaw[6] = { 0.0, -60.0, -120.0, 180.0, 120.0, 60.0 };
	for (int32 D = 0; D < 6; ++D)
	{
		const FVector Expected = FRotator(0.0, DeclaredYaw[D], 0.0).Vector();
		const FVector Actual = URTHexLibrary::FacingRotation(static_cast<ERTHexDirection>(D)).Vector();
		TestTrue(*FString::Printf(TEXT("direzione %d: yaw dichiarato %.0f"), D, DeclaredYaw[D]),
			Actual.Equals(Expected, 1.e-3));
	}

	// ⛔ E il corollario che la spec dichiara a parole: in un pointy-top **nessun lato guarda a nord**.
	// Se un giorno una direzione ci finisse sopra, la §2.1 sarebbe diventata falsa senza che nulla lo dica.
	const FVector North = -FVector::YAxisVector;
	for (int32 D = 0; D < 6; ++D)
	{
		TestFalse(*FString::Printf(TEXT("direzione %d non e' il nord (-Y)"), D),
			URTHexLibrary::FacingRotation(static_cast<ERTHexDirection>(D)).Vector().Equals(North, 1.e-3));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
