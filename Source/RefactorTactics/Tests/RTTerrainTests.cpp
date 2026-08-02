#include "Misc/AutomationTest.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Terrain/RTTerrainTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainDerivationTest,
	"RefactorTactics.Terrain.PropsDeriveCostBlockVision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainDerivationTest::RunTest(const FString&)
{
	// Terreno normale (default): costo 1, non blocca nulla.
	{
		const FRTTerrainProps Normal;
		TestEqual(TEXT("normale: costo 1"), URTTerrainLibrary::CellMoveCost(Normal), 1);
		TestFalse(TEXT("normale: non blocca il movimento"), URTTerrainLibrary::BlocksMovement(Normal));
		TestFalse(TEXT("normale: non blocca la vista"), URTTerrainLibrary::BlocksVision(Normal));
	}
	// Fango: ExtraMoveCost 1 -> costo 2.
	{
		FRTTerrainProps Fango; Fango.ExtraMoveCost = 1;
		TestEqual(TEXT("fango: costo 2"), URTTerrainLibrary::CellMoveCost(Fango), 2);
		TestFalse(TEXT("fango: non blocca il movimento"), URTTerrainLibrary::BlocksMovement(Fango));
	}
	// Muro: bloccante -> costo sentinella, blocca movimento (e prevale sul costo).
	{
		FRTTerrainProps Muro; Muro.bBlocksMovement = true; Muro.ExtraMoveCost = 3;
		TestEqual(TEXT("muro: costo bloccato"), URTTerrainLibrary::CellMoveCost(Muro), RT_BLOCKED_COST);
		TestTrue(TEXT("muro: blocca il movimento"), URTTerrainLibrary::BlocksMovement(Muro));
	}
	// Cespuglio: blocca solo la vista, attraversabile a costo normale.
	{
		FRTTerrainProps Cespuglio; Cespuglio.bBlocksVision = true;
		TestTrue(TEXT("cespuglio: blocca la vista"), URTTerrainLibrary::BlocksVision(Cespuglio));
		TestFalse(TEXT("cespuglio: non blocca il movimento"), URTTerrainLibrary::BlocksMovement(Cespuglio));
		TestEqual(TEXT("cespuglio: costo 1"), URTTerrainLibrary::CellMoveCost(Cespuglio), 1);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
