#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Terrain/RTTerrainData.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Bot/RTHexBotLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTMovementActionLibrary.h"

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

	// Lo scivolamento e' un DATO del catalogo, non un `Surface == Ice` inciso nel resolver: solo il ghiaccio
	// lo dichiara, e chi aggiungesse un terreno scivoloso non deve toccare RTHexSimLibrary per farlo scivolare.
	for (const FRTTerrainDef& Def : Catalog)
	{
		if (Def.Surface == ERTHexSurface::Ice)
		{
			TestEqual(TEXT("Ice scivola di 1 cella"), Def.SlideCells, 1);
		}
		else
		{
			TestEqual(FString::Printf(TEXT("terreno %d non scivola"), static_cast<int32>(Def.Surface)),
				Def.SlideCells, 0);
		}
	}

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
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
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

	// Il predicato di raggiungibilita' lineare: dal consolidamento (#140) e' definito sopra la stessa
	// funzione che esegue lo scatto, ed e' quello con cui il bot filtra le proprie candidate. La prova che il
	// BOT lo usi davvero sta in `HexBotPlay.DashPlanIsExecutableOnCostlyTerrain`, non qui.
	TestFalse(TEXT("la raggiungibilita' lineare nega lo scatto attraverso Rough"),
		URTMovementActionLibrary::IsLinearReachable(Map, FRTCellId(0, 0, 0), FRTCellId(2, 0, 0),
			/*MaxCells=*/ 10, ERTMovementStyle::LinearDash, Snapshot.Occupancy, {}));

	// E il RESOLVER, che muove davvero l'unita' nella fase Dash, si ferma per lo stesso motivo dichiarato.
	for (const ERTMovementStyle Style : { ERTMovementStyle::LinearDash, ERTMovementStyle::LinearCharge })
	{
		const FRTLinearMoveResult Linear = URTMovementActionLibrary::ResolveLinearMove(
			Map, FRTCellId(0, 0, 0), FRTCellId(2, 0, 0), /*MaxCells=*/ 10, Style, {}, {});
		TestTrue(TEXT("il resolver si ferma prima del Rough"), Linear.Final == FRTCellId(0, 0, 0));
		TestTrue(TEXT("motivo dichiarato: terreno"), Linear.Stop == ERTLinearStop::BlockedByTerrain);
	}

	// Il SALTO invece scavalca: il terreno accidentato non ostacola chi ci passa sopra.
	const FRTLinearMoveResult Leap = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(2, 0, 0), /*MaxCells=*/ 10, ERTMovementStyle::LinearLeap, {}, {});
	TestTrue(TEXT("il salto scavalca il Rough"), Leap.Final == FRTCellId(2, 0, 0));

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
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
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
	const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path).Path;

	TestEqual(TEXT("il percorso si estende di una cella"), Extended.Num(), 3);
	TestTrue(TEXT("la cella extra e' nella direzione d'ingresso"), Extended.Last() == FRTCellId(2, 0, 0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainIceBlockedCellStopsSlidingTest,
	"RefactorTactics.Terrain.Ice.BlockedCellStopsSliding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainIceBlockedCellStopsSlidingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
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
	const TArray<FRTCellId> Extended = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path).Path;

	TestEqual(TEXT("nessuno scivolamento: la cella successiva blocca il movimento"), Extended.Num(), 2);

	return true;
}

/**
 * UN MURO SUL BORDO FERMA LO SCIVOLAMENTO QUANTO UNA CELLA CHE BLOCCA — `#2284`.
 *
 * 🔴 **Il difetto che questo test impedisce era invisibile al test gemello.**
 * `Terrain.Ice.BlockedCellStopsSliding` mette `bBlocksMovement` sulla CELLA, e passava. Ma muri e
 * coperture alte non stanno nella cella: stanno sul BORDO fra due celle
 * (`URTHexCoverLibrary::BlocksTraversal`), e `bBlocksMovement` non li vede. Un'unita' che finiva il Move
 * su una lastra addossata a un muro ci scivolava attraverso.
 *
 * Dentro `ResolveMovement` il danno era mascherato da `TruncatePathToTopology`, che a valle taglia il
 * passo — ma questa e' una funzione **pura e pubblica**, e ogni chiamante diretto riceveva il percorso
 * che attraversa il muro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainIceWallOnEdgeStopsSlidingTest,
	"RefactorTactics.Terrain.Ice.WallOnEdgeStopsSliding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainIceWallOnEdgeStopsSlidingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
	FRTHexCellData IceCell(FRTCellId(1, 0, 0));
	IceCell.Surface = ERTHexSurface::Ice;
	// Il muro sta sul BORDO verso (2,0), la cella di scivolamento. Quella cella resta percorribile in se'
	// — e' il passo per raggiungerla che non esiste.
	IceCell.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::High, 50));
	Map->AddOrUpdateCell(IceCell);
	Map->SortCells();

	FRTHexSimUnit Unit;
	Unit.UnitId = 0;
	Unit.Cell = FRTCellId(0, 0, 0);
	Unit.MoveBudget = 5;

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.Units.Add(Unit);

	// PREMESSA MISURATA: la cella di destinazione esiste e NON blocca il movimento. Senza questa riga il
	// test potrebbe passare per la ragione sbagliata — una cella assente — e non proverebbe niente sul bordo.
	const FRTHexCellData* Beyond = Map->FindCell(FRTCellId(2, 0, 0));
	if (!TestTrue(TEXT("premessa: la cella oltre il muro esiste e non blocca"),
		Beyond != nullptr && !Beyond->bBlocksMovement))
	{
		return false;
	}

	const TArray<FRTCellId> Path = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };
	const FRTIceSlideResult Slide = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path);

	TestEqual(TEXT("non si scivola attraverso un muro sul bordo"), Slide.Path.Num(), 2);

	// 🔴 **Il percorso invariato non basta a raccontare cosa e' successo** (`#2314`). E' la stessa risposta
	// che la funzione da' quando il terreno non chiede nulla — niente ghiaccio, budget finito, nessuna
	// direzione — e quei casi producono `Moved`, mentre questo deve produrre `SlideBlocked`. Senza questo
	// campo la distinzione muore qui, e a valle nessuno puo' ricostruirla: e' la ragione per cui `#2290`,
	// che ci provava dal chiamante, e' stata chiusa.
	TestTrue(TEXT("ma il terreno lo aveva CHIESTO: lo scivolamento e' impedito, non assente"),
		Slide.bSlideRequested);

	return true;
}

/**
 * UN MURO CHE IMPEDISCE LO SCIVOLAMENTO PRODUCE `SlideBlocked` — `#2314`, decisione d'autore D1.
 *
 * 🔑 **E' il caso che il fix di `#2284` aveva reso corretto e MUTO.** Da quando `ApplyIceSliding` verifica
 * la percorribilita' del passo, davanti a un muro non estende affatto il percorso: il resolver riceve un
 * percorso che il giocatore ha pianificato per intero, lo vede completato, e scriveva `Moved` — cioe'
 * *«arrivata, e il terreno non aveva altro da dire»*, che e' falso. Per il giocatore «non sono scivolato
 * perche' c'era un muro» e «perche' c'era qualcuno» sono lo **stesso** fatto, e `D-319` lo conferma: chi non
 * scivola non prende `Status.Unbalanced`, qualunque sia stata la causa.
 *
 * ⚠️ **Questo test cade se `ApplyIceSliding` perde `bSlideRequested`** — la mutazione prescritta dalla
 * issue — perche' quel campo e' l'unica differenza fra questo caso e un arrivo ordinario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainIceWallProducesSlideBlockedTest,
	"RefactorTactics.Terrain.Ice.WallBlockingSlideIsReportedAsSlideBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainIceWallProducesSlideBlockedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
	FRTHexCellData IceCell(FRTCellId(1, 0, 0));
	IceCell.Surface = ERTHexSurface::Ice;
	IceCell.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::High, 50)); // muro verso (2,0)
	Map->AddOrUpdateCell(IceCell);
	Map->SortCells();

	FRTHexSimUnit Unit;
	Unit.UnitId = 0;
	Unit.Cell = FRTCellId(0, 0, 0);
	Unit.MoveBudget = 5;

	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	Snapshot.Units.Add(Unit);

	// Lo stesso incastro di `ARTTurnManager::ResolveMovement`: si scivola, poi si risolve. La lunghezza
	// PRIMA dello scivolamento e' il piano del giocatore.
	const TArray<FRTCellId> Planned = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) };
	const FRTIceSlideResult Slide = URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Planned);

	// PREMESSA: il muro ha davvero impedito l'estensione. Se un giorno cadesse, il caso qui sotto sarebbe un
	// altro — uno scivolamento riuscito — e l'asserzione finale mentirebbe sul motivo per cui passa.
	if (!TestEqual(TEXT("premessa: il percorso non e' stato esteso"), Slide.Path.Num(), Planned.Num()))
	{
		return false;
	}

	FRTPlannedMovement Plan;
	Plan.PlannedLength = Planned.Num();
	Plan.bSlideRequested = Slide.bSlideRequested;

	FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement({ Slide.Path }, TArray<int32>(),
		TArray<bool>(), TArray<bool>(), { Plan });
	const TArray<FRTHexMoveResult> Out = URTHexSimLibrary::FinishHexMovement(State);

	if (!TestTrue(TEXT("premessa: l'unita' arriva dove il giocatore l'ha mandata"),
		Out.Num() == 1 && Out[0].Final == FRTCellId(1, 0, 0)))
	{
		return false;
	}
	TestEqual(TEXT("un muro che impedisce lo scivolamento produce SlideBlocked, non Moved"),
		static_cast<int32>(Out[0].Outcome), static_cast<int32>(ERTMoveOutcome::SlideBlocked));

	// CONTRO-PROVA sulla stessa mappa senza muro: il ghiaccio estende, l'unita' scivola, e l'esito e' `Slid`.
	// Senza questa meta' un `SlideBlocked` scritto a ogni arrivo su ghiaccio passerebbe il test sopra.
	{
		URTHexMapAsset* Open = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3);
		FRTHexCellData PlainIce(FRTCellId(1, 0, 0));
		PlainIce.Surface = ERTHexSurface::Ice;
		Open->AddOrUpdateCell(PlainIce);
		Open->SortCells();

		FRTHexSnapshot OpenSnapshot;
		OpenSnapshot.Map = Open;
		OpenSnapshot.Units.Add(Unit);

		const FRTIceSlideResult Free = URTHexSimLibrary::ApplyIceSliding(OpenSnapshot, /*UnitId=*/ 0, Planned);
		if (!TestEqual(TEXT("premessa della contro-prova: senza muro il percorso si estende"),
			Free.Path.Num(), Planned.Num() + 1))
		{
			return false;
		}

		FRTPlannedMovement FreePlan;
		FreePlan.PlannedLength = Planned.Num();
		FreePlan.bSlideRequested = Free.bSlideRequested;

		FRTMovementResolutionState FreeState = URTHexSimLibrary::BeginHexMovement({ Free.Path },
			TArray<int32>(), TArray<bool>(), TArray<bool>(), { FreePlan });
		const TArray<FRTHexMoveResult> FreeOut = URTHexSimLibrary::FinishHexMovement(FreeState);
		TestEqual(TEXT("senza muro lo scivolamento avviene: Slid"),
			static_cast<int32>(FreeOut[0].Outcome), static_cast<int32>(ERTMoveOutcome::Slid));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainSmokeLimitsTargetingTest,
	"RefactorTactics.Terrain.Smoke.LimitsTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainSmokeLimitsTargetingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 5);
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
	Intent.bCountsAsAttack = true; // intento d'attacco, e da [`INT-8`] va dichiarato

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(Intent);

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	TestEqual(TEXT("intento scartato: oltre il cap di targeting del Fumo (2 celle)"), Plan.Hits.Num(), 0);

	return true;
}

namespace
{
	/** Mappa piatta di raggio `Radius` con una sola cella dipinta `Surface`. */
	URTHexMapAsset* MakeMapWithSurface(int32 Radius, const FRTCellId& Painted, ERTHexSurface Surface)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			FRTHexCellData Data(Id);
			if (Id == Painted) { Data.Surface = Surface; }
			Map->AddOrUpdateCell(Data);
		}
		Map->SortCells();
		return Map;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainSmokeCapAgreesAcrossGatesTest,
	"RefactorTactics.Terrain.Smoke.CapAgreesAcrossGates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainSmokeCapAgreesAcrossGatesTest::RunTest(const FString&)
{
	// Il difetto che questo test chiude: il cap del Fumo viveva SOLO dentro CollectHexAttacks, mentre i tre
	// cancelli che decidono "il bersaglio e' a portata" — preview del giocatore (ClassifyHexTargeting),
	// validazione degli ordini (ValidateInstance) e proposta del bot (BuildCandidates) — usavano la portata
	// grezza. Risultato: azione accettata, slot speso, nessun effetto e nessuna riga di log.
	// L'asserzione e' quindi un ACCORDO fra i quattro, non il solo esito del resolver.
	const FRTCellId AttackerCell(0, 0, 0);
	const FRTCellId TargetCell(4, 0, 0); // distanza 4: dentro i 6 dichiarati, oltre il cap 2 del Fumo
	const FRTCellId SmokeCell(2, 0, 0);
	constexpr int32 DeclaredRange = 6;

	TArray<FRTHexCombatUnit> Units;
	FRTHexCombatUnit Attacker; Attacker.UnitId = 0; Attacker.TeamId = 0; Attacker.Cell = AttackerCell;
	FRTHexCombatUnit Target;   Target.UnitId = 1;   Target.TeamId = 1;   Target.Cell = TargetCell;
	Units.Add(Attacker);
	Units.Add(Target);

	FRTHexAttackIntent Intent;
	Intent.AttackerId = 0;
	Intent.TargetId = 1;
	Intent.RangeCells = DeclaredRange;
	Intent.Power = 10;
	Intent.bCountsAsAttack = true; // intento d'attacco, e da [`INT-8`] va dichiarato
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(Intent);

	FRTActionInstance Instance;
	Instance.SourceUnitId = 0;
	Instance.TargetUnitId = 1;
	Instance.Def.RangeCells = DeclaredRange;

	FRTHexSimUnit BotUnit;
	BotUnit.UnitId = 0;
	BotUnit.Cell = AttackerCell;
	BotUnit.MoveBudget = 0; // fermo: l'unica cella candidata e' la sua, cosi' si misura solo il targeting

	FRTHexBotContext BotCtx;
	BotCtx.Origin = AttackerCell;
	BotCtx.Enemies.Add(TargetCell);
	BotCtx.EnemyRanges.Add(DeclaredRange);
	BotCtx.EnemyHealth.Add(100);
	BotCtx.AttackRange = DeclaredRange;
	BotCtx.AttackDamage = 10;

	auto CountBotAttackPlans = [](const URTHexMapAsset* Map, const FRTHexSimUnit& Unit,
		const FRTHexBotContext& Ctx) -> int32
	{
		FRTHexSnapshot Snapshot;
		Snapshot.Map = Map;
		Snapshot.Units.Add(Unit);
		int32 Count = 0;
		for (const FRTHexBotPlan& Plan : URTHexBotLibrary::BuildCandidates(Snapshot, /*UnitId=*/ 0, Ctx))
		{
			if (Plan.bHasAttack) { ++Count; }
		}
		return Count;
	};

	// --- CONTROPROVA: senza Fumo i quattro cancelli dicono tutti "si colpisce". Senza questa meta', il test
	// passerebbe anche se il bersaglio fosse irraggiungibile per un motivo qualunque (geometria sbagliata).
	{
		URTHexMapAsset* Clear = MakeMapWithSurface(/*Radius=*/ 5, SmokeCell, ERTHexSurface::Floor);
		TestTrue(TEXT("senza Fumo: la preview accetta"),
			URTCombatLibrary::ClassifyHexTargeting(Clear, AttackerCell, TargetCell, DeclaredRange)
				== ERTHexTargetReason::Ok);
		TestTrue(TEXT("senza Fumo: l'ordine pianificato resta valido"),
			URTActionFallbackLibrary::ValidateInstance(Instance, Units, Clear) == ERTActionInvalidReason::None);
		TestEqual(TEXT("senza Fumo: il bot propone l'attacco"),
			CountBotAttackPlans(Clear, BotUnit, BotCtx), 1);
		TestEqual(TEXT("senza Fumo: il resolver colpisce"),
			URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Clear).Hits.Num(), 1);
	}

	// --- CON il Fumo sulla linea: tutti e quattro devono rifiutare, e per il motivo GIUSTO (portata, non
	// copertura: il Fumo non blocca la linea di vista, cappa la gittata).
	{
		URTHexMapAsset* Smoked = MakeMapWithSurface(/*Radius=*/ 5, SmokeCell, ERTHexSurface::Smoke);
		TestTrue(TEXT("col Fumo: la preview dice FUORI PORTATA (non 'coperto')"),
			URTCombatLibrary::ClassifyHexTargeting(Smoked, AttackerCell, TargetCell, DeclaredRange)
				== ERTHexTargetReason::OutOfRange);
		TestTrue(TEXT("col Fumo: l'ordine pianificato decade per portata"),
			URTActionFallbackLibrary::ValidateInstance(Instance, Units, Smoked)
				== ERTActionInvalidReason::OutOfRange);
		TestEqual(TEXT("col Fumo: il bot non propone l'attacco"),
			CountBotAttackPlans(Smoked, BotUnit, BotCtx), 0);
		TestEqual(TEXT("col Fumo: il resolver non colpisce"),
			URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Smoked).Hits.Num(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainSmokeCapsFromAttackerCellTest,
	"RefactorTactics.Terrain.Smoke.CapsWhenAttackerStandsInIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainSmokeCapsFromAttackerCellTest::RunTest(const FString&)
{
	// DoD canonico: «targeting max 2 celle **dentro o attraverso**». Gli estremi della linea CONTANO, quindi
	// sparare stando NEL fumo cappa la propria portata quanto sparare attraverso una nuvola davanti a se'.
	// Divergenza voluta da HasLineOfSight, che gli estremi li esclude (li' From/To sono chi guarda e cosa
	// guarda, non ostacoli): qui l'estremo e' proprio la nuvola in cui si e' immersi.
	const FRTCellId AttackerCell(0, 0, 0);
	URTHexMapAsset* Map = MakeMapWithSurface(/*Radius=*/ 5, AttackerCell, ERTHexSurface::Smoke);

	TestTrue(TEXT("dentro il Fumo: a 3 celle non si ingaggia, pur avendo portata 6"),
		URTCombatLibrary::ClassifyHexTargeting(Map, AttackerCell, FRTCellId(3, 0, 0), /*RangeCells=*/ 6)
			== ERTHexTargetReason::OutOfRange);

	// Il cap e' 2, non 0: dentro il fumo si spara ancora, ma corto. Ancora l'esito al valore ESATTO del
	// catalogo, cosi' un cap cambiato per sbaglio non passa inosservato.
	TestTrue(TEXT("dentro il Fumo: a 2 celle si ingaggia ancora"),
		URTCombatLibrary::ClassifyHexTargeting(Map, AttackerCell, FRTCellId(2, 0, 0), /*RangeCells=*/ 6)
			== ERTHexTargetReason::Ok);

	// Il cap non ALZA mai la portata: con un'abilita' da 1 cella, restare nel fumo non regala il secondo esagono.
	TestTrue(TEXT("il cap e' un minimo, non una portata garantita"),
		URTCombatLibrary::ClassifyHexTargeting(Map, AttackerCell, FRTCellId(2, 0, 0), /*RangeCells=*/ 1)
			== ERTHexTargetReason::OutOfRange);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainIceSlideBudgetBoundaryTest,
	"RefactorTactics.Terrain.Ice.SlideBudgetBoundaryIsExactlyTwo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainIceSlideBudgetBoundaryTest::RunTest(const FString&)
{
	// La soglia "residuo >= 2" e' una DECISIONE dell'autore (spec §5.2, confermata in review del Task 6): resta
	// piatta, senza guardare il costo reale della cella di scivolamento. Terrain.Ice.SlidesWithSufficientBudget
	// la esercita con residuo 4 — comodo, ma non direbbe nulla se la soglia diventasse 3 o 5. Qui si ancora il
	// confine ESATTO: 2 scivola, 1 no.
	URTHexMapAsset* Map = MakeMapWithSurface(/*Radius=*/ 3, FRTCellId(1, 0, 0), ERTHexSurface::Ice);
	const TArray<FRTCellId> Path = { FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) }; // costo 1: il ghiaccio costa 1

	auto SlideWithBudget = [Map, &Path](int32 MoveBudget) -> TArray<FRTCellId>
	{
		FRTHexSimUnit Unit;
		Unit.UnitId = 0;
		Unit.Cell = FRTCellId(0, 0, 0);
		Unit.MoveBudget = MoveBudget;

		FRTHexSnapshot Snapshot;
		Snapshot.Map = Map;
		Snapshot.Units.Add(Unit);
		return URTHexSimLibrary::ApplyIceSliding(Snapshot, /*UnitId=*/ 0, Path).Path;
	};

	// Budget 3 - costo 1 = residuo ESATTAMENTE 2: e' il minimo che scivola.
	const TArray<FRTCellId> AtThreshold = SlideWithBudget(3);
	TestEqual(TEXT("residuo esattamente 2: scivola"), AtThreshold.Num(), 3);
	TestTrue(TEXT("scivola nella direzione d'ingresso"), AtThreshold.Last() == FRTCellId(2, 0, 0));

	// Budget 2 - costo 1 = residuo 1: una unita' sotto la soglia, non scivola.
	TestEqual(TEXT("residuo 1: non scivola"), SlideWithBudget(2).Num(), 2);

	return true;
}

/**
 * Bot e resolver devono concordare su COSA e' raggiungibile con uno scatto.
 *
 * Il catalogo azioni distingue: `Action.Move`/`Action.Sprint` vanno a punti movimento, le mobilita'
 * LINEARI (Dash/Charge/Leap/Reposition) vanno a celle — «non riduce le mobilita' lineari, dichiarato fuori
 * scope v0.1». Finche' il bot ha filtrato le candidate col COSTO del terreno e il resolver le ha eseguite a
 * CELLE, i due strati hanno risposto in modo diverso sulla stessa mappa: su terreno a costo 2 il bot si
 * negava scatti perfettamente legali. Nessuna mossa illegale — l'invariante reggeva — ma un'opportunita'
 * persa in silenzio, che nessun test vedeva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTerrainDashReachAgreesTest,
	"RefactorTactics.Terrain.Dash.BotAndResolverAgreeOnCostlyTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTerrainDashReachAgreesTest::RunTest(const FString&)
{
	// Corridoio di acqua bassa (costo 2) lungo +q: attraversabile, ma caro.
	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	for (int32 Q = 1; Q <= 3; ++Q)
	{
		FRTHexCellData Water(FRTCellId(Q, 0, 0));
		Water.Surface = ERTHexSurface::ShallowWater;
		Water.MoveCost = 2; // il costo vive sulla cella, come lo dipinge il catalogo
		Map->AddOrUpdateCell(Water);
	}
	Map->SortCells();

	const FRTCellId Start(0, 0, 0);
	const FRTCellId Goal(3, 0, 0);   // tre celle di distanza
	const int32 DashRange = 3;       // portata dichiarata dall'azione: TRE CELLE

	// La portata di una mobilita' lineare si conta in CELLE: tre celle di acqua costano 6 punti movimento,
	// ma con portata 3 lo scatto ci arriva lo stesso. E' la regola del catalogo, ed e' cio' che il vecchio
	// filtro del bot — che sommava il costo — negava.
	const FRTLinearMoveResult Resolved = URTMovementActionLibrary::ResolveLinearMove(
		Map, Start, Goal, DashRange, ERTMovementStyle::LinearDash, {}, {});
	TestTrue(TEXT("tre celle care, portata tre: lo scatto arriva"), Resolved.Final == Goal);
	TestEqual(TEXT("le attraversa tutte e tre"), Resolved.Entered.Num(), 3);

	// Contro-prova sull'unita' di misura: se la portata fosse letta in punti movimento, 3 non basterebbe
	// (servirebbero 6) e questa asserzione cadrebbe insieme a quella sopra.
	TestTrue(TEXT("con portata 2 si ferma prima: e' un limite in celle, non in punti movimento"),
		URTMovementActionLibrary::ResolveLinearMove(
			Map, Start, Goal, /*MaxCells=*/ 2, ERTMovementStyle::LinearDash, {}, {}).Final == Start);

	// L'accordo bot<->resolver NON si verifica qui: `IsLinearReachable` e' definita come
	// `ResolveLinearMove(...).Final == Target`, quindi confrontarle sarebbe vero per costruzione. La prova
	// sta in `HexBotPlay.DashPlanIsExecutableOnCostlyTerrain`, che passa da `PlanBots`.
	return true;
}

#endif
