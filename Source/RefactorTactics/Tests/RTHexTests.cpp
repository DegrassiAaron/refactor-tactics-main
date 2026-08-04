#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
