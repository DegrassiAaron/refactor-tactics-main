#include "Misc/AutomationTest.h"
#include "Terrain/RTTerrainData.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Combat/RTHexCombatLibrary.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainShallowWaterAppliesWetTest,
	"RefactorTactics.Terrain.ShallowWater.AppliesWet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainShallowWaterAppliesWetTest::RunTest(const FString&)
{
	// Verifica sul CATALOGO: e' li' che l'acqua bassa dichiara `Status.Wet`, ed e' da li' che
	// ApplyTerrainOnEnterEffects lo pesca senza sapere di quale terreno si tratti.
	// ATTENZIONE (aperto): la durata dichiarata e' 0 e sia ARTUnit::ApplyStatus sia URTActionEffectLibrary
	// scartano gli stati con durata <= 0 (`Action.Sprint` usa 1 per "fino al Cleanup"). Finche' resta 0, Wet
	// e Obscured sono INERTI in partita: il catalogo li dichiara ma nessuna unita' li riceve davvero.
	const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(ERTHexSurface::ShallowWater);
	bool bAppliesWet = false;
	for (const FRTActionEffectSpec& Effect : Def.OnEnterEffects)
	{
		if (Effect.Effect == ERTActionEffect::Status && Effect.StatusTag == TAG_Status_Wet) { bAppliesWet = true; }
	}
	TestTrue(TEXT("ShallowWater applica Status.Wet"), bAppliesWet);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainRoughBlocksDashTest,
	"RefactorTactics.Terrain.Rough.BlocksDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainRoughBlocksDashTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	FRTHexCellData Blocker(FRTCellId(1, 0, 0));
	Blocker.Surface = ERTHexSurface::Rough;
	Map->AddOrUpdateCell(Blocker);
	Map->SortCells();

	FRTHexSimUnit Unit;
	Unit.UnitId = 0;
	Unit.Cell = FRTCellId(0, 0, 0);
	Unit.MoveBudget = 10;

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.Units.Add(Unit);

	const TArray<FRTCellId> Path = URTHexSimLibrary::LinearDashPath(Snapshot, /*UnitId=*/ 0, FRTCellId(2, 0, 0));
	TestEqual(TEXT("scatto rifiutato: Rough blocca Dash"), Path.Num(), 0);

	// Il movimento NORMALE, invece, attraversa Rough (costa di piu', non e' bloccato).
	const FRTHexPathResult Normal = URTHexSimLibrary::FindPathForUnit(Snapshot, /*UnitId=*/ 0, FRTCellId(2, 0, 0));
	TestTrue(TEXT("il Move normale attraversa Rough"), Normal.Status == ERTHexPathStatus::Success);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainIceSlidesWithBudgetTest,
	"RefactorTactics.Terrain.Ice.SlidesWithSufficientBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainIceSlidesWithBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	FRTHexCellData IceCell(FRTCellId(1, 0, 0));
	IceCell.Surface = ERTHexSurface::Ice;
	Map->AddOrUpdateCell(IceCell);
	Map->SortCells();

	FRTHexSimUnit Unit;
	Unit.UnitId = 0;
	Unit.Cell = FRTCellId(0, 0, 0);
	Unit.MoveBudget = 5; // 1 per entrare sul ghiaccio (costo 1), 4 residui: >= 2, scivola

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.Units.Add(Unit);

	const TArray<FRTCellId> Path = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };
	const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path);

	TestEqual(TEXT("il percorso si estende di una cella"), Extended.Num(), 3);
	TestTrue(TEXT("la cella extra e' nella direzione d'ingresso"), Extended.Last() == FRTCellId(2, 0, 0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainIceBlockedCellStopsSlidingTest,
	"RefactorTactics.Terrain.Ice.BlockedCellStopsSliding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainIceBlockedCellStopsSlidingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	FRTHexCellData IceCell(FRTCellId(1, 0, 0));
	IceCell.Surface = ERTHexSurface::Ice;
	Map->AddOrUpdateCell(IceCell);
	FRTHexCellData Wall(FRTCellId(2, 0, 0));
	Wall.bBlocksMovement = true;
	Map->AddOrUpdateCell(Wall);
	Map->SortCells();

	FRTHexSimUnit Unit;
	Unit.UnitId = 0;
	Unit.Cell = FRTCellId(0, 0, 0);
	Unit.MoveBudget = 5;

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.Units.Add(Unit);

	const TArray<FRTCellId> Path = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };
	const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path);

	TestEqual(TEXT("nessuno scivolamento: la cella successiva blocca il movimento"), Extended.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainSmokeLimitsTargetingTest,
	"RefactorTactics.Terrain.Smoke.LimitsTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainSmokeLimitsTargetingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 5))
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	FRTHexCellData Smoke(FRTCellId(2, 0, 0));
	Smoke.Surface = ERTHexSurface::Smoke;
	Map->AddOrUpdateCell(Smoke);
	Map->SortCells();

	TArray<FRTHexCombatUnit> Units;
	FRTHexCombatUnit Attacker; Attacker.UnitId = 0; Attacker.TeamId = 0; Attacker.Cell = FRTCellId(0, 0, 0);
	FRTHexCombatUnit Target;   Target.UnitId = 1;   Target.TeamId = 1;   Target.Cell = FRTCellId(4, 0, 0);
	Units.Add(Attacker);
	Units.Add(Target);

	FRTHexAttackIntent Intent;
	Intent.AttackerId = 0;
	Intent.TargetId = 1;
	Intent.RangeCells = 6; // la portata dichiarata basterebbe, ma la linea attraversa il Fumo a q=2
	Intent.Power = 10;

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(Intent);

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	TestEqual(TEXT("intento scartato: oltre il cap di targeting del Fumo (2 celle)"), Plan.Hits.Num(), 0);

	return true;
}

#endif
