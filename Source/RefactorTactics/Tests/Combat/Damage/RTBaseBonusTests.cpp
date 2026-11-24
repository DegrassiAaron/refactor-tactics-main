#include "Misc/AutomationTest.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/Cover/RTHexCoverLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Misc/ScopeExit.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Log/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * #4189, fetta 1 di #4188 - il marchio (+60) e il +80 di `Hero.Gadget.LinearDischarge` contro un bersaglio `Wet` entrano
 * nel Base, PRIMA della difesa ([D-634] (35)): `max(0, floor((Base + bonus + Spalle) x Mod x 100/(100 + difesa)) - Copertura)`,
 * con un solo arrotondamento ([D-595] (1)). Fino al #4189 si sommavano al colpo gia' mitigato.
 *
 * Il banco e' quello del piano: LinearDischarge (240, senza elemento, quindi la DEF) di Gadget contro Riktor (DEF 45 nel
 * catalogo), frontale, senza copertura, scudo a zero. Con DEF 45:
 *   - nessun bonus            floor(240 x 100/145)             = 165;
 *   - bagnato                 floor(320 x 100/145)             = 220 (prima 165 + 80 = 245);
 *   - marchiato               floor(300 x 100/145)             = 206 (prima 165 + 60 = 225);
 *   - marchiato ed esposto    floor(300 x 120 x 100/(100 x 145)) = 248 (prima 198 + 60 = 258): il bonus entra una volta,
 *     nel Base, e il `Mod` 120 lo moltiplica (Review Focus 2 del piano);
 *   - i tre insieme           floor(380 x 120 x 100/(100 x 145)) = 314 (prima 198 + 60 + 80 = 338).
 *
 * Chi prende il bonus NON cambia (Ruling 4 del #4189): il marchio resta sul primo colpo alleato che fa danno nell'ordine
 * canonico, il +80 resta della sola LinearDischarge. Cambia solo il punto in cui il bonus entra nella formula.
 */

namespace
{
	/** Nomi distinti per file: la unity build condivide la translation unit. */
	constexpr int32 BbTargetDefense = 45;
	constexpr int32 BbDischargePower = 240;

	URTHexMapAsset* MakeBbMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	void BbSetLowCover(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	FRTHexCombatUnit BbUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing, int32 Defense = 0)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		U.Facing = Facing;
		U.Defense = Defense;
		return U;
	}

	/** Un colpo singolo a portata 5, mirato sulla cella del bersaglio ([D-415]), senza elemento. */
	FRTHexAttackIntent BbIntent(const FRTCellId& AimCell, int32 AttackerId, int32 TargetId, int32 Power)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.TargetCell = AimCell;
		I.Shape = ERTAbilityShape::Single;
		I.RangeCells = 5;
		I.Power = Power;
		I.bCountsAsAttack = true;
		return I;
	}

	/** Un'azione del catalogo core montata sull'unita', con il `Power` legacy azzerato: il danno lo dicono gli effetti. */
	int32 BbMountCoreAction(ARTUnit* Unit, const TCHAR* ActionId, int32 RangeCells)
	{
		if (!Unit) { return INDEX_NONE; }
		URTActionData* Action = NewObject<URTActionData>(Unit);
		Action->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Action->Def.RangeCells = RangeCells;
		Action->RangeCells = RangeCells;
		Action->Shape = ERTAbilityShape::Single;
		Action->Power = 0; // il default legacy (30) farebbe di una spinta un colpo che fa danno
		return Unit->Abilities.Add(Action);
	}

	/** Le voci `Combat` del turno prodotte da `ActionId`, e la somma del loro `Amount`. */
	int32 BbCombatEntries(const ARTTurnManager* TM, const FName& ActionId, int32& OutAmount)
	{
		int32 N = 0;
		OutAmount = 0;
		if (!TM) { return N; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Combat && E.ActionId == ActionId)
			{
				++N;
				OutAmount += E.Amount;
			}
		}
		return N;
	}

	int32 BbStatusEntries(const ARTTurnManager* TM, const FGameplayTag& Tag, ERTStatusOutcome Outcome)
	{
		int32 N = 0;
		if (!TM) { return N; }
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Status && E.ActionId == Tag.GetTagName()
				&& E.Outcome == static_cast<uint8>(Outcome))
			{
				++N;
			}
		}
		return N;
	}

	FRTHexAttackHit* BbHitFrom(FRTHexBlastPlan& Plan, int32 AttackerId)
	{
		for (FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.AttackerId == AttackerId) { return &Hit; }
		}
		return nullptr;
	}

	ARTUnit* BbSpawnUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World || !Hero) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi
		U->ConfigureFromHeroData(Hero); // DEF dal catalogo: Riktor 45
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->SetPlannedCell(Cell); // fermo: qui conta il Blast, non il movimento
		return U;
	}

	/**
	 * HP persi da Riktor in UN turno vero, colpito da UNA LinearDischarge di Gadget, con gli stati messi a mano prima del
	 * lock-in (come il Blast li trova dopo un turno precedente). Gadget a ovest, Riktor guarda a ovest: frontale, nessun
	 * bonus alle spalle; arena piena, nessuna copertura; scudo a zero, cosi' gli HP persi sono il colpo.
	 *
	 * `-1` = montaggio fallito, gia' segnalato da `Test`.
	 */
	int32 BbDischargeTurn(FAutomationTestBase& Test, bool bWet, bool bMarked, bool bExposed)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), World)) { return -1; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakeBbMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		ARTUnit* Gadget = BbSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-1, 0, 0));
		ARTUnit* Riktor = BbSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
		if (!Test.TestNotNull(TEXT("mappa"), MapActor) || !Test.TestNotNull(TEXT("turn manager"), TM)
			|| !Test.TestNotNull(TEXT("Gadget"), Gadget) || !Test.TestNotNull(TEXT("Riktor"), Riktor)
			|| !Test.TestNotNull(TEXT("la LinearDischarge di Gadget"), Gadget->GetAbility(1)))
		{
			return -1;
		}

		// Premesse sul catalogo: se si muovono, i numeri dei test sono da riscrivere, non da ignorare.
		const URTActionData* Discharge = Gadget->GetAbility(1);
		if (!Test.TestEqual(TEXT("premessa: l'azione 1 di Gadget e' LinearDischarge"), Discharge->Def.ActionId,
				FName(TEXT("Hero.Gadget.LinearDischarge")))
			|| !Test.TestEqual(TEXT("premessa: LinearDischarge vale 240"), URTCatalogLibrary::FirstDamage(Discharge->Def),
				BbDischargePower)
			|| !Test.TestEqual(TEXT("premessa: LinearDischarge e' senza elemento, quindi legge la DEF"),
				Discharge->Def.ElementId, FName(NAME_None))
			|| !Test.TestEqual(TEXT("premessa: Riktor ha DEF 45"), Riktor->Defense, BbTargetDefense)
			|| !Test.TestTrue(TEXT("premessa: Riktor regge il colpo piu' grosso del banco"), Riktor->Health > 400))
		{
			return -1;
		}

		Riktor->Facing = ERTHexDirection::W; // guarda Gadget: frontale
		Riktor->ClearPlannedMainAction();
		Riktor->Shield = 0;
		if (bWet) { Riktor->ApplyStatus(TAG_Status_Wet, /*Turni*/ 2); }
		if (bMarked) { Riktor->ApplyMarkedBy(Gadget->TeamId, /*Turni*/ 2); } // marchio della squadra di chi spara
		if (bExposed) { Riktor->ApplyStatus(TAG_Status_Exposed, /*Turni*/ 2); }
		const int32 StartHealth = Riktor->Health;

		Gadget->Facing = ERTHexDirection::E;
		Gadget->SetPlannedMainAction(1);
		Gadget->DeclareAttackOnUnit(Riktor);

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		if (!Test.TestFalse(TEXT("premessa: il turno si e' risolto"), TM->IsResolving())) { return -1; }
		return StartHealth - Riktor->Health;
	}
}

/**
 * Il +80 contro `Wet` entra nel Base: floor((240 + 80) x 100/145) = 220, non 165 + 80 = 245. Il caso asciutto nello
 * stesso banco (165) e' il controllo: dice che la DEF morde, quindi che 220 e 245 non sono uguali per caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseBonusWetEntersTheBaseInATurnTest,
	"RefactorTactics.Damage.BaseBonus.WetEntersTheBaseInATurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseBonusWetEntersTheBaseInATurnTest::RunTest(const FString&)
{
	const int32 Asciutto = BbDischargeTurn(*this, /*bWet*/ false, /*bMarked*/ false, /*bExposed*/ false);
	const int32 Bagnato = BbDischargeTurn(*this, /*bWet*/ true, /*bMarked*/ false, /*bExposed*/ false);
	if (Asciutto < 0 || Bagnato < 0) { return false; }

	TestEqual(TEXT("controllo: asciutto, floor(240 x 100/145) = 165"), Asciutto,
		URTCombatLibrary::MitigateDirectDamage(BbDischargePower, 0, 100, BbTargetDefense));
	TestEqual(TEXT("bagnato: floor((240 + 80) x 100/145) = 220, prima della DEF ([D-634] (35))"), Bagnato, 220);
	TestEqual(TEXT("e' la formula con il bonus nel Base"), Bagnato, URTCombatLibrary::MitigateDirectDamage(
		BbDischargePower + URTCombatLibrary::GadgetWetDischargeBonus, 0, 100, BbTargetDefense));
	return true;
}

/** Il marchio entra nel Base: floor((240 + 60) x 100/145) = 206, non 165 + 60 = 225. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseBonusMarkEntersTheBaseInATurnTest,
	"RefactorTactics.Damage.BaseBonus.MarkEntersTheBaseInATurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseBonusMarkEntersTheBaseInATurnTest::RunTest(const FString&)
{
	const int32 Marchiato = BbDischargeTurn(*this, /*bWet*/ false, /*bMarked*/ true, /*bExposed*/ false);
	if (Marchiato < 0) { return false; }

	TestEqual(TEXT("marchiato: floor((240 + 60) x 100/145) = 206, prima della DEF ([D-634] (35))"), Marchiato, 206);
	TestEqual(TEXT("e' la formula con il bonus nel Base"), Marchiato, URTCombatLibrary::MitigateDirectDamage(
		BbDischargePower + URTCombatLibrary::MarkedFirstHitBonus, 0, 100, BbTargetDefense));
	return true;
}

/**
 * Review Focus 2 del piano: marchio ed `Exposed` sullo stesso colpo. Il bonus entra UNA volta, nel Base, e il `Mod` 120
 * moltiplica anche lui: floor((240 + 60) x 120 x 100/(100 x 145)) = 248. Il colpo esposto senza marchio vale 198, quindi
 * il bonus sommato dopo darebbe 258, e contato due volte (nel Base e dopo) 308.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseBonusMarkAndExposedCountTheBonusOnceTest,
	"RefactorTactics.Damage.BaseBonus.MarkAndExposedCountTheBonusOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseBonusMarkAndExposedCountTheBonusOnceTest::RunTest(const FString&)
{
	const int32 Esposto = BbDischargeTurn(*this, /*bWet*/ false, /*bMarked*/ false, /*bExposed*/ true);
	const int32 Entrambi = BbDischargeTurn(*this, /*bWet*/ false, /*bMarked*/ true, /*bExposed*/ true);
	if (Esposto < 0 || Entrambi < 0) { return false; }

	TestEqual(TEXT("controllo: esposto, floor(240 x 120 x 100/(100 x 145)) = 198"), Esposto,
		URTCombatLibrary::MitigateDirectDamage(BbDischargePower, 0, URTCombatLibrary::ExposedModPercent, BbTargetDefense));
	TestEqual(TEXT("marchiato ed esposto: floor((240 + 60) x 120 x 100/(100 x 145)) = 248"), Entrambi, 248);
	TestEqual(TEXT("e' la formula con il bonus nel Base e il Mod 120"), Entrambi, URTCombatLibrary::MitigateDirectDamage(
		BbDischargePower + URTCombatLibrary::MarkedFirstHitBonus, 0, URTCombatLibrary::ExposedModPercent, BbTargetDefense));
	return true;
}

/**
 * I tre insieme sulla stessa LinearDischarge: marchio, bagnato ed esposto. I due bonus entrano nel Base insieme e c'e'
 * un solo arrotondamento: floor((240 + 60 + 80) x 120 x 100/(100 x 145)) = 314 (prima 198 + 60 + 80 = 338).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseBonusMarkWetAndExposedOnOneHitTest,
	"RefactorTactics.Damage.BaseBonus.MarkWetAndExposedOnOneHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseBonusMarkWetAndExposedOnOneHitTest::RunTest(const FString&)
{
	const int32 Tutti = BbDischargeTurn(*this, /*bWet*/ true, /*bMarked*/ true, /*bExposed*/ true);
	if (Tutti < 0) { return false; }

	TestEqual(TEXT("marchio, bagnato ed esposto: floor(380 x 120 x 100/(100 x 145)) = 314"), Tutti, 314);
	TestEqual(TEXT("e' la formula con i due bonus nel Base"), Tutti, URTCombatLibrary::MitigateDirectDamage(
		BbDischargePower + URTCombatLibrary::MarkedFirstHitBonus + URTCombatLibrary::GadgetWetDischargeBonus, 0,
		URTCombatLibrary::ExposedModPercent, BbTargetDefense));
	return true;
}

/**
 * La guardia del marchio che il Ruling 4 del #4189 conserva: il marchio va al primo colpo alleato che FA danno, giudicato
 * PRIMA del bonus (`Hit.Power <= 0` in `RTTurnManager.cpp`, `MarkSpentOn`). Riktor (DEF 45, scudo a zero) e' marchiato
 * dalla squadra 0 prima del Blast; Ivrin lo spinge (`Action.Push`, priorita' 40, nessun danno: un colpo con `Power` 0) e
 * poi Gadget lo scarica (LinearDischarge, 55). Nell'ordine canonico la spinta viene prima.
 *
 * Con la guardia il +60 va alla scarica: floor(300 x 100/145) = 206, la spinta resta a 0. Senza, il ricalcolo nel Base
 * darebbe alla spinta floor(60 x 100/145) = 41 e la scarica resterebbe a 165 — una spinta che fa danno.
 *
 * ⚠️ **Gli HP persi NON distinguono i due casi, e per questo si leggono le voci per colpo.** 41 + 165 = 206 =
 * floor(300 x 100/145): con DEF 45 la somma dei due floor coincide con il floor della somma. Il test legge l'`Amount` delle
 * voci `Combat` del TurnLog, una per colpo, con l'`ActionId` che le distingue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseBonusMarkSkipsAZeroPowerHitTest,
	"RefactorTactics.Damage.BaseBonus.MarkSkipsAZeroPowerHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseBonusMarkSkipsAZeroPowerHitTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
	if (MapActor) { MapActor->MapAsset = MakeBbMap(6); }
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	ARTUnit* Gadget = BbSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-1, 0, 0));
	ARTUnit* Ivrin = BbSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, -1, 0));  // adiacente
	ARTUnit* Riktor = BbSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("mappa"), MapActor) || !TestNotNull(TEXT("turn manager"), TM)
		|| !TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("Ivrin"), Ivrin) || !TestNotNull(TEXT("Riktor"), Riktor)
		|| !TestNotNull(TEXT("la LinearDischarge di Gadget"), Gadget->GetAbility(1)))
	{
		return false;
	}

	const int32 PushIdx = BbMountCoreAction(Ivrin, TEXT("Action.Push"), /*Range*/ 1);
	const FRTActionDef& PushDef = Ivrin->Abilities[PushIdx]->Def;
	const FRTActionDef& DischargeDef = Gadget->GetAbility(1)->Def;
	if (!TestEqual(TEXT("premessa: la spinta del catalogo"), PushDef.ActionId, FName(TEXT("Action.Push")))
		|| !TestEqual(TEXT("premessa: la spinta non dichiara danno"), URTCatalogLibrary::FirstDamage(PushDef), 0)
		|| !TestTrue(TEXT("premessa: la spinta viene prima della scarica nell'ordine canonico"),
			PushDef.Priority < DischargeDef.Priority)
		|| !TestEqual(TEXT("premessa: la scarica vale 240"), URTCatalogLibrary::FirstDamage(DischargeDef), BbDischargePower)
		|| !TestEqual(TEXT("premessa: Riktor ha DEF 45"), Riktor->Defense, BbTargetDefense))
	{
		return false;
	}

	Riktor->Facing = ERTHexDirection::W; // guarda Gadget: la scarica e' frontale
	Riktor->ClearPlannedMainAction();
	Riktor->Shield = 0;
	Riktor->ApplyMarkedBy(Gadget->TeamId, /*Turni*/ 2); // marchio della squadra 0, gia' in essere quando il Blast comincia
	const int32 StartHealth = Riktor->Health;

	Ivrin->SetPlannedMainAction(PushIdx);
	Ivrin->DeclareAttackOnUnit(Riktor);
	Gadget->Facing = ERTHexDirection::E;
	Gadget->SetPlannedMainAction(1);
	Gadget->DeclareAttackOnUnit(Riktor);

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
	{
		TM->Tick(0.05f);
	}
	if (!TestFalse(TEXT("premessa: il turno si e' risolto"), TM->IsResolving())) { return false; }

	int32 PushAmount = -1, DischargeAmount = -1;
	const int32 PushEntries = BbCombatEntries(TM, FName(TEXT("Action.Push")), PushAmount);
	const int32 DischargeEntries = BbCombatEntries(TM, FName(TEXT("Hero.Gadget.LinearDischarge")), DischargeAmount);
	// Senza la voce della spinta il colpo a `Power` 0 non sarebbe mai esistito, e la guardia non sarebbe in gioco.
	if (!TestEqual(TEXT("premessa: la spinta ha raggiunto Riktor come colpo"), PushEntries, 1)
		|| !TestEqual(TEXT("premessa: la scarica ha raggiunto Riktor"), DischargeEntries, 1))
	{
		return false;
	}

	TestEqual(TEXT("il +60 va alla scarica, il primo colpo che fa danno: floor(300 x 100/145) = 206"), DischargeAmount,
		URTCombatLibrary::MitigateDirectDamage(BbDischargePower + URTCombatLibrary::MarkedFirstHitBonus, 0, 100, BbTargetDefense));
	TestEqual(TEXT("la spinta resta senza danno: il marchio non l'ha presa (sarebbe 41)"), PushAmount, 0);
	TestEqual(TEXT("HP persi: 206"), StartHealth - Riktor->Health, 206);
	TestEqual(TEXT("il marchio e' incassato una volta"),
		BbStatusEntries(TM, TAG_Status_Marked, ERTStatusOutcome::Spent), 1);
	TestFalse(TEXT("e non c'e' piu'"), Riktor->HasStatus(TAG_Status_Marked));
	return true;
}

/**
 * La funzione sul piano puro, `URTHexCombatLibrary::AddBonusToHitBase`, su tre colpi raccolti da 240 contro DEF 45:
 *   - A: +60 poi +80 sullo stesso colpo. 206, poi floor(380 x 100/145) = **262**: i bonus si sommano nel Base e c'e' un
 *     solo arrotondamento. Due delta calcolati entrambi dal nominale dell'intento darebbero 165 + 41 + 55 = 261;
 *   - B: `Exposed` prima, poi +60: floor(300 x 120 x 100/(100 x 145)) = 248 — il `Mod` che il colpo porta moltiplica
 *     anche il bonus;
 *   - C: in copertura bassa frontale (60, [D-602]): 105, poi floor(300 x 100/145) - 60 = 146. La copertura resta piatta e
 *     sottratta una volta, dopo la difesa ([D-589] (3)).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseBonusRecomputesTheCollectedHitTest,
	"RefactorTactics.Damage.BaseBonus.RecomputesTheCollectedHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseBonusRecomputesTheCollectedHitTest::RunTest(const FString&)
{
	const FRTCellId DifA(0, 0, 0), DifB(0, 3, 0), DifC(0, -3, 0);
	URTHexMapAsset* Map = MakeBbMap(4);
	BbSetLowCover(Map, DifC, ERTHexDirection::W); // muretto sul lato da cui arriva l'attaccante 4

	TArray<FRTHexCombatUnit> Units;
	Units.Add(BbUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
	Units.Add(BbUnit(1, 1, DifA, ERTHexDirection::W, BbTargetDefense)); // guarda l'attaccante: frontale
	Units.Add(BbUnit(2, 0, FRTCellId(-1, 3, 0), ERTHexDirection::E));
	Units.Add(BbUnit(3, 1, DifB, ERTHexDirection::W, BbTargetDefense));
	Units.Add(BbUnit(4, 0, FRTCellId(-1, -3, 0), ERTHexDirection::E));
	Units.Add(BbUnit(5, 1, DifC, ERTHexDirection::W, BbTargetDefense)); // guarda il muretto: la copertura tiene
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(BbIntent(DifA, /*Attacker*/ 0, /*Target*/ 1, BbDischargePower));
	Intents.Add(BbIntent(DifB, /*Attacker*/ 2, /*Target*/ 3, BbDischargePower));
	Intents.Add(BbIntent(DifC, /*Attacker*/ 4, /*Target*/ 5, BbDischargePower));

	FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	if (!TestEqual(TEXT("tre colpi raccolti"), Plan.Hits.Num(), 3)) { return false; }
	FRTHexAttackHit* A = BbHitFrom(Plan, 0);
	FRTHexAttackHit* B = BbHitFrom(Plan, 2);
	FRTHexAttackHit* C = BbHitFrom(Plan, 4);
	if (!TestNotNull(TEXT("colpo A"), A) || !TestNotNull(TEXT("colpo B"), B) || !TestNotNull(TEXT("colpo C"), C))
	{
		return false;
	}
	if (!TestEqual(TEXT("premessa: A raccolto a floor(240 x 100/145) = 165"), A->Power, 165)
		|| !TestEqual(TEXT("premessa: B raccolto a 165"), B->Power, 165)
		|| !TestEqual(TEXT("premessa: C raccolto a 165 - 60 = 105"), C->Power, 105)
		|| !TestEqual(TEXT("premessa: la copertura di C morde per 60"), C->CoverReduction, URTCombatLibrary::LowCoverDamageReduction))
	{
		return false;
	}

	// A: marchio, poi +80 sullo stesso colpo.
	URTHexCombatLibrary::AddBonusToHitBase(*A, URTCombatLibrary::MarkedFirstHitBonus);
	TestEqual(TEXT("A con il marchio: floor(300 x 100/145) = 206"), A->Power, 206);
	TestEqual(TEXT("A: il mitigato e' il ricalcolato"), A->MitigatedPower, 206);
	TestEqual(TEXT("A: il Base porta il bonus"), A->NominalPower, BbDischargePower + URTCombatLibrary::MarkedFirstHitBonus);
	URTHexCombatLibrary::AddBonusToHitBase(*A, URTCombatLibrary::GadgetWetDischargeBonus);
	TestEqual(TEXT("A con marchio e +80: floor(380 x 100/145) = 262, un solo arrotondamento"), A->Power, 262);
	TestNotEqual(TEXT("A: non due delta dal nominale dell'intento (165 + 41 + 55 = 261)"), A->Power, 261);
	TestEqual(TEXT("A: il Base porta entrambi i bonus"), A->NominalPower, 380);
	TestEqual(TEXT("A: nessun Mod"), A->ModPercentApplied, 100);

	// B: Exposed sul piano, poi il marchio.
	TArray<bool> bExposed;
	bExposed.Init(false, Units.Num());
	bExposed[3] = true;
	TArray<int32> Order;
	for (int32 h = 0; h < Plan.Hits.Num(); ++h) { Order.Add(h); }
	URTHexCombatLibrary::ApplyExposedToFirstHit(Plan.Hits, Order, bExposed);
	if (!TestEqual(TEXT("premessa: B esposto, floor(240 x 120 x 100/(100 x 145)) = 198"), B->Power, 198)) { return false; }
	URTHexCombatLibrary::AddBonusToHitBase(*B, URTCombatLibrary::MarkedFirstHitBonus);
	TestEqual(TEXT("B esposto e marchiato: floor(300 x 120 x 100/(100 x 145)) = 248"), B->Power, 248);
	TestEqual(TEXT("B: il Mod resta 120"), B->ModPercentApplied, URTCombatLibrary::ExposedModPercent);
	TestEqual(TEXT("B: il mitigato e' il ricalcolato"), B->MitigatedPower, 248);

	// C: in copertura.
	URTHexCombatLibrary::AddBonusToHitBase(*C, URTCombatLibrary::MarkedFirstHitBonus);
	TestEqual(TEXT("C in copertura e marchiato: floor(300 x 100/145) - 60 = 146"), C->Power, 146);
	TestEqual(TEXT("C: il mitigato e' prima della copertura"), C->MitigatedPower, 206);
	return true;
}

/**
 * Il +80 contro `Wet` non ha la guardia `Power <= 0` del marchio (Ruling 4 del #4189), quindi raggiunge anche un colpo
 * che la copertura aveva AZZERATO. La copertura si paga una volta, dopo la difesa: 50 contro DEF 0 in copertura bassa
 * (60) e' 0, e con il +80 nel Base e' 130 - 60 = **70**, non 0 + 80 = 80 (il bonus sommato dopo, come fino al #4189, o
 * la sola differenza 130 - 50 sommata al colpo azzerato). Lo stesso colpo contro DEF 45: floor(50 x 100/145) = 34, a
 * zero; con il +80 floor(130 x 100/145) - 60 = 89 - 60 = **29**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseBonusZeroedHitPaysTheCoverOnceTest,
	"RefactorTactics.Damage.BaseBonus.ZeroedHitPaysTheCoverOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseBonusZeroedHitPaysTheCoverOnceTest::RunTest(const FString&)
{
	const FRTCellId DifA(0, 0, 0), DifB(0, 3, 0);
	URTHexMapAsset* Map = MakeBbMap(4);
	BbSetLowCover(Map, DifA, ERTHexDirection::W);
	BbSetLowCover(Map, DifB, ERTHexDirection::W);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(BbUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
	Units.Add(BbUnit(1, 1, DifA, ERTHexDirection::W, /*Defense*/ 0));
	Units.Add(BbUnit(2, 0, FRTCellId(-1, 3, 0), ERTHexDirection::E));
	Units.Add(BbUnit(3, 1, DifB, ERTHexDirection::W, BbTargetDefense));
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(BbIntent(DifA, /*Attacker*/ 0, /*Target*/ 1, /*Power*/ 50));
	Intents.Add(BbIntent(DifB, /*Attacker*/ 2, /*Target*/ 3, /*Power*/ 50));

	FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	if (!TestEqual(TEXT("due colpi raccolti"), Plan.Hits.Num(), 2)) { return false; }
	FRTHexAttackHit* A = BbHitFrom(Plan, 0);
	FRTHexAttackHit* B = BbHitFrom(Plan, 2);
	if (!TestNotNull(TEXT("colpo A"), A) || !TestNotNull(TEXT("colpo B"), B)) { return false; }
	if (!TestEqual(TEXT("premessa: A azzerato dalla copertura (50 - 60)"), A->Power, 0)
		|| !TestEqual(TEXT("premessa: B azzerato dalla copertura (34 - 60)"), B->Power, 0)
		|| !TestEqual(TEXT("premessa: la copertura di A e' 60"), A->CoverReduction, URTCombatLibrary::LowCoverDamageReduction)
		|| !TestEqual(TEXT("premessa: la copertura di B e' 60"), B->CoverReduction, URTCombatLibrary::LowCoverDamageReduction))
	{
		return false;
	}

	URTHexCombatLibrary::AddBonusToHitBase(*A, URTCombatLibrary::GadgetWetDischargeBonus);
	URTHexCombatLibrary::AddBonusToHitBase(*B, URTCombatLibrary::GadgetWetDischargeBonus);
	TestEqual(TEXT("A, DEF 0: 130 - 60 = 70, la copertura una volta sola"), A->Power, 70);
	TestEqual(TEXT("B, DEF 45: floor(130 x 100/145) - 60 = 29"), B->Power, 29);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
