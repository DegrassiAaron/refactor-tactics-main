#include "Misc/AutomationTest.h"
#include "Terrain/RTTerrainData.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Core/RTGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainCostsFromCatalogTest,
	"RefactorTactics.Terrain.CostsFromCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainCostsFromCatalogTest::RunTest(const FString&)
{
	const TArray<FRTTerrainDef> Catalog = URTTerrainLibrary::GetTerrainCatalog();
	TestEqual(TEXT("8 terreni nel catalogo"), Catalog.Num(), 8);

	auto FindCost = [&Catalog](ERTHexSurface Surface) -> int32
	{
		for (const FRTTerrainDef& Def : Catalog)
		{
			if (Def.Surface == Surface) { return Def.MoveCost; }
		}
		return -1;
	};

	TestEqual(TEXT("Floor costo 1"), FindCost(ERTHexSurface::Floor), 1);
	TestEqual(TEXT("Rough costo 2"), FindCost(ERTHexSurface::Rough), 2);
	TestEqual(TEXT("ShallowWater costo 2"), FindCost(ERTHexSurface::ShallowWater), 2);
	TestEqual(TEXT("Fire costo 2"), FindCost(ERTHexSurface::Fire), 2);
	TestEqual(TEXT("Conductive costo 1"), FindCost(ERTHexSurface::Conductive), 1);
	TestEqual(TEXT("Smoke costo 1"), FindCost(ERTHexSurface::Smoke), 1);
	TestEqual(TEXT("Ice costo 1"), FindCost(ERTHexSurface::Ice), 1);
	TestEqual(TEXT("HighGround costo 1"), FindCost(ERTHexSurface::HighGround), 1);

	const FRTTerrainDef Rough = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Rough);
	TestTrue(TEXT("Rough blocca Dash/Charge"), Rough.bBlocksDashCharge);

	const FRTTerrainDef Smoke = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Smoke);
	TestEqual(TEXT("Smoke limita il targeting a 2"), Smoke.MaxTargetingRangeThrough, 2);

	const FRTTerrainDef Conductive = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Conductive);
	TestTrue(TEXT("Conductive conduce elettricita'"), Conductive.bConductsElectricity);
	for (const FRTActionEffectSpec& Effect : Conductive.OnEnterEffects)
	{
		TestFalse(TEXT("Conductive non applica Wet"), Effect.Effect == ERTActionEffect::Status && Effect.StatusTag == TAG_Status_Wet);
	}

	const FRTTerrainDef ShallowWater = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::ShallowWater);
	TestTrue(TEXT("ShallowWater conduce elettricita'"), ShallowWater.bConductsElectricity);
	bool bShallowWaterAppliesWet = false;
	for (const FRTActionEffectSpec& Effect : ShallowWater.OnEnterEffects)
	{
		if (Effect.Effect == ERTActionEffect::Status && Effect.StatusTag == TAG_Status_Wet)
		{
			bShallowWaterAppliesWet = true;
		}
	}
	TestTrue(TEXT("ShallowWater applica Wet"), bShallowWaterAppliesWet);

	const FRTTerrainDef Fire = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Fire);
	bool bFireDealsDamage = false;
	bool bFireAppliesBurning = false;
	for (const FRTActionEffectSpec& Effect : Fire.OnEnterEffects)
	{
		if (Effect.Effect == ERTActionEffect::Damage && Effect.Amount == 10)
		{
			bFireDealsDamage = true;
		}
		if (Effect.Effect == ERTActionEffect::Status && Effect.StatusTag == TAG_Status_Burning && Effect.StatusDuration == 2)
		{
			bFireAppliesBurning = true;
		}
	}
	TestTrue(TEXT("Fire infligge 10 danni all'ingresso"), bFireDealsDamage);
	TestTrue(TEXT("Fire applica Burning per 2 turni"), bFireAppliesBurning);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainValidateCatalogTest,
	"RefactorTactics.Terrain.ValidateCatalog.NoDuplicatesNoNegativeCosts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainValidateCatalogTest::RunTest(const FString&)
{
	const TArray<FString> Errors = URTTerrainLibrary::ValidateTerrainCatalog();
	TestEqual(TEXT("catalogo spedito valido"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainValidateCatalogEntriesTest,
	"RefactorTactics.Terrain.ValidateCatalogEntries.CatchesDuplicatesAndNegativeCosts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainValidateCatalogEntriesTest::RunTest(const FString&)
{
	TArray<FRTTerrainDef> Broken;
	FRTTerrainDef A; A.Surface = ERTHexSurface::Floor; A.MoveCost = -1;
	FRTTerrainDef B; B.Surface = ERTHexSurface::Floor; // duplicato di A
	Broken.Add(A);
	Broken.Add(B);

	const TArray<FString> Errors = URTTerrainLibrary::ValidateCatalogEntries(Broken);
	TestTrue(TEXT("almeno 2 errori (duplicato + costo negativo)"), Errors.Num() >= 2);
	return true;
}

#endif
