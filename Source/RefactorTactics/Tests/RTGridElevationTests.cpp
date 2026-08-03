#include "Misc/AutomationTest.h"
#include "Grid/RTGridLibrary.h"
#include "Core/RTTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

// La posizione-mondo di un'unita' su una cella deriva da CellToWorld piu' un offset verticale ESPLICITO
// (VisualZOffset): 0 = pivot appoggiato a terra (personaggi UE), ~90 = centro del cilindro (retrocompat).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellToWorldElevatedOffsetTest,
	"RefactorTactics.Grid.CellToWorldElevatedAppliesZOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellToWorldElevatedOffsetTest::RunTest(const FString&)
{
	const FVector Origin(0.f, 0.f, 0.f);
	const float CellSize = 200.f;
	const FRTGridCoord Cell(2, 3); // Layer 0

	const FVector Base = URTGridLibrary::CellToWorld(Cell, Origin, CellSize);
	const FVector Elevated = URTGridLibrary::CellToWorldElevated(Cell, Origin, CellSize, /*ZOffset=*/90.f, /*LayerHeight=*/0.f);

	TestTrue(TEXT("X invariato rispetto a CellToWorld"), FMath::IsNearlyEqual(Elevated.X, Base.X));
	TestTrue(TEXT("Y invariato rispetto a CellToWorld"), FMath::IsNearlyEqual(Elevated.Y, Base.Y));
	TestTrue(TEXT("Z = base + ZOffset"), FMath::IsNearlyEqual(Elevated.Z, Base.Z + 90.0));
	return true;
}

// Con ZOffset 0 il personaggio e' appoggiato a terra: Z uguale alla base della cella.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellToWorldElevatedGroundedTest,
	"RefactorTactics.Grid.CellToWorldElevatedGroundedAtZeroOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellToWorldElevatedGroundedTest::RunTest(const FString&)
{
	const FVector Origin(0.f, 0.f, 0.f);
	const float CellSize = 200.f;
	const FRTGridCoord Cell(1, 1);

	const FVector Base = URTGridLibrary::CellToWorld(Cell, Origin, CellSize);
	const FVector Grounded = URTGridLibrary::CellToWorldElevated(Cell, Origin, CellSize, /*ZOffset=*/0.f, /*LayerHeight=*/0.f);

	// Con offset 0 e layer 0 la posizione coincide ESATTAMENTE con la base della cella (X,Y,Z),
	// non solo sulla Z: cosi' il test discrimina uno stub che ignora X/Y (es. ZeroVector).
	TestTrue(TEXT("a terra: X == base"), FMath::IsNearlyEqual(Grounded.X, Base.X));
	TestTrue(TEXT("a terra: Y == base"), FMath::IsNearlyEqual(Grounded.Y, Base.Y));
	TestTrue(TEXT("a terra: Z == base"), FMath::IsNearlyEqual(Grounded.Z, Base.Z));
	return true;
}

// L'elevazione per layer si somma all'offset: layer 1 sta 1*LayerHeight sopra layer 0.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCellToWorldElevatedLayerTest,
	"RefactorTactics.Grid.CellToWorldElevatedAddsLayerHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCellToWorldElevatedLayerTest::RunTest(const FString&)
{
	const FVector Origin(0.f, 0.f, 0.f);
	const float CellSize = 200.f;
	const float LayerHeight = 200.f;
	FRTGridCoord Ground(4, 4); Ground.Layer = 0;
	FRTGridCoord Upper(4, 4); Upper.Layer = 1;

	const FVector G = URTGridLibrary::CellToWorldElevated(Ground, Origin, CellSize, /*ZOffset=*/50.f, LayerHeight);
	const FVector U = URTGridLibrary::CellToWorldElevated(Upper, Origin, CellSize, /*ZOffset=*/50.f, LayerHeight);

	TestTrue(TEXT("layer 1 sta 1*LayerHeight sopra layer 0"), FMath::IsNearlyEqual(U.Z - G.Z, static_cast<double>(LayerHeight)));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
