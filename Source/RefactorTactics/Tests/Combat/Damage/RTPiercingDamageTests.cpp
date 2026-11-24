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
#include "Tests/Shared/RTAbilityFixtures.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * #4189, fetta 1 di #4188 - il danno PERFORANTE ([D-634] (47)): un colpo di un'azione `bPiercing` usa difesa 0 (ne' la DEF ne'
 * la RES dell'elemento) e non sottrae la copertura bassa; la Guard e lo scudo restano. Il bonus alle spalle, il marchio e il
 * +80 contro `Wet` entrano nel Base come per ogni colpo, e la copertura ALTA continua a fermare la linea di tiro.
 *
 * Il banco e' quello del piano: un colpo da 420 contro Riktor (DEF 45, RES 35, Guard 100, scudo base 50 nel catalogo), in
 * copertura bassa frontale (60, [D-602]).
 *   - senza il flag    floor(420 x 100/145) - 60 = 289 - 60 = 229;
 *   - perforante       420;
 *   - perforante e Guard    420 - 100 = 320 (il flag non salta la Guard);
 *   - perforante e scudo base    420 - 50 = 370: senza elemento e' un colpo `Direct`, e lo scudo base ferma il `Direct`
 *     ([D-634] (5)). Un perforante ELEMENTALE (la Sticky Bomb, in una fetta successiva) segue la regola dello scudo, non
 *     questa: non e' provato qui (Ruling 1 del #4189).
 *
 * ⚠️ **Perche' le ultime due colonne passano da un turno vero**: la Guard e lo scudo non stanno nel piano della raccolta ma nei
 * pass che la seguono (`ApplyEligibleHitDelta`, `ApplyDamageParts`), e un test sul solo `CollectHexAttacks` darebbe lo stesso
 * numero con e senza la Guard — verde su qualunque implementazione.
 */

namespace
{
	/** Nomi distinti per file: la unity build condivide la translation unit. */
	constexpr int32 PcDefense = 45;
	constexpr int32 PcPower = 420;
	constexpr int32 PcRearBonus = 30;

	URTHexMapAsset* MakePcMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	void PcSetCover(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, ERTHexCoverType Type)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, Type, FRTHexCover::DefaultIntegrity(Type)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	void PcSetLowCover(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		PcSetCover(Map, Id, Edge, ERTHexCoverType::Low);
	}

	FRTHexCombatUnit PcUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing, int32 Defense = 0,
		int32 Resistance = 0, int32 RearBonus = 0)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		U.Facing = Facing;
		U.Defense = Defense;
		U.ResistanceDefault = Resistance;
		U.RearBonus = RearBonus;
		return U;
	}

	/** Un colpo singolo a portata 5, mirato sulla cella del bersaglio ([D-415]). */
	FRTHexAttackIntent PcIntent(const FRTCellId& AimCell, int32 AttackerId, int32 TargetId, int32 Power, bool bPiercing,
		const FName& ElementId = NAME_None)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.TargetCell = AimCell;
		I.Shape = ERTAbilityShape::Single;
		I.RangeCells = 5;
		I.Power = Power;
		I.bCountsAsAttack = true;
		I.bPiercing = bPiercing;
		I.ElementId = ElementId;
		return I;
	}

	FRTHexAttackHit* PcHitFrom(FRTHexBlastPlan& Plan, int32 AttackerId)
	{
		for (FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.AttackerId == AttackerId) { return &Hit; }
		}
		return nullptr;
	}

	ARTUnit* PcSpawnUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World || !Hero) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi
		U->ConfigureFromHeroData(Hero); // DEF, RES e Guard dal catalogo
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->SetPlannedCell(Cell); // fermo: qui conta il Blast, non il movimento
		return U;
	}

	void PcRunTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Cio' che il turno vero cambia di caso in caso; il resto del banco e' fisso. */
	struct FPcTurnSpec
	{
		bool bPiercing = false;
		bool bLowCover = true;    // muretto basso sul lato ovest di Riktor, da cui arriva Gadget
		bool bGuarded = false;    // `Status.Guarded`, 100 per colpo frontale
		bool bBaseShield = false; // tiene lo scudo base (50): altrimenti Riktor ha lo scudo a zero
		bool bMarked = false;     // marchio della squadra di chi spara
		bool bExposed = false;
	};

	/**
	 * Gli HP persi, e basta. ⚠️ Lo scudo che resta NON si legge: a fine turno ogni unita' viva torna allo scudo base
	 * (`RechargeBaseShield`, [D-224]), quindi `Shield` dopo il Cleanup vale 50 con e senza assorbimento. L'assorbimento si vede
	 * negli HP: 420 senza scudo, 370 con.
	 */
	struct FPcTurnOutcome
	{
		int32 HpLost = -1;
	};

	/**
	 * UN turno vero: Gadget (squadra 0, a (-1,0), guarda a est) tira `Hero.Gadget.ArcPulse` a Riktor (squadra 1, a (0,0), guarda
	 * a ovest) con il danno portato a 420 sul `Def` — l'azione di prova — e il flag di `Spec`. Frontale, quindi niente bonus alle
	 * spalle; arena piena, con il muretto basso sul lato ovest di Riktor se `bLowCover`.
	 *
	 * `false` = montaggio fallito, gia' segnalato da `Test`: il chiamante non confronta niente.
	 */
	bool PcTurn(FAutomationTestBase& Test, const FPcTurnSpec& Spec, FPcTurnOutcome& Out)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		URTHexMapAsset* Map = MakePcMap(6);
		if (Spec.bLowCover) { PcSetLowCover(Map, FRTCellId(0, 0, 0), ERTHexDirection::W); }
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = Map; }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		ARTUnit* Gadget = PcSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-1, 0, 0));
		ARTUnit* Riktor = PcSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
		if (!Test.TestNotNull(TEXT("mappa"), MapActor) || !Test.TestNotNull(TEXT("turn manager"), TM)
			|| !Test.TestNotNull(TEXT("Gadget"), Gadget) || !Test.TestNotNull(TEXT("Riktor"), Riktor)
			|| !Test.TestNotNull(TEXT("l'ArcPulse di Gadget"), Gadget->GetAbility(0)))
		{
			return false;
		}

		// Premesse sul catalogo: se si muovono, i numeri dei test sono da riscrivere, non da ignorare.
		URTActionData* ArcPulse = Gadget->GetAbility(0);
		if (!Test.TestEqual(TEXT("premessa: l'azione 0 di Gadget e' ArcPulse"), ArcPulse->Def.ActionId,
				FName(TEXT("Hero.Gadget.ArcPulse")))
			|| !Test.TestFalse(TEXT("premessa: nessuna azione del catalogo e' perforante"), ArcPulse->Def.bPiercing)
			|| !Test.TestEqual(TEXT("premessa: ArcPulse e' senza elemento, quindi legge la DEF"), ArcPulse->Def.ElementId,
				FName(NAME_None))
			|| !Test.TestEqual(TEXT("premessa: Riktor ha DEF 45"), Riktor->Defense, PcDefense)
			|| !Test.TestEqual(TEXT("premessa: Riktor ha la Guard di catalogo"), Riktor->GuardReduction,
				URTCombatLibrary::GuardReductionDefault)
			|| !Test.TestEqual(TEXT("premessa: la Guard di catalogo vale 100"), URTCombatLibrary::GuardReductionDefault, 100)
			|| !Test.TestEqual(TEXT("premessa: Riktor porta lo scudo base"), Riktor->Shield, URTCombatLibrary::BaseShield)
			|| !Test.TestEqual(TEXT("premessa: Riktor senza temporaneo"), Riktor->GetTemporaryShield(), 0)
			|| !Test.TestTrue(TEXT("premessa: Riktor regge il colpo del banco"), Riktor->Health > PcPower + 200))
		{
			return false;
		}
		bool bDamageSet = false;
		for (FRTActionEffectSpec& Effect : ArcPulse->Def.Effects)
		{
			if (Effect.Effect == ERTActionEffect::Damage) { Effect.Amount = PcPower; bDamageSet = true; break; }
		}
		if (!Test.TestTrue(TEXT("premessa: ArcPulse dichiara un danno da portare a 420"), bDamageSet)) { return false; }
		ArcPulse->Def.bPiercing = Spec.bPiercing; // il flag sul `Def` di QUESTO Gadget: nessuna azione di oggi ne ha uno

		Riktor->Facing = ERTHexDirection::W; // guarda Gadget: frontale
		Riktor->ClearPlannedMainAction();
		if (!Spec.bBaseShield) { Riktor->Shield = 0; }
		if (Spec.bGuarded) { Riktor->ApplyStatus(TAG_Status_Guarded, 1); }
		if (Spec.bMarked) { Riktor->ApplyMarkedBy(Gadget->TeamId, /*Turni*/ 2); }
		if (Spec.bExposed) { Riktor->ApplyStatus(TAG_Status_Exposed, /*Turni*/ 2); }
		const int32 StartHealth = Riktor->Health;

		Gadget->Facing = ERTHexDirection::E;
		Gadget->SetPlannedMainAction(0);
		Gadget->DeclareAttackOnUnit(Riktor);

		PcRunTurn(TM);
		if (!Test.TestFalse(TEXT("premessa: il turno si e' risolto"), TM->IsResolving())) { return false; }

		Out.HpLost = StartHealth - Riktor->Health;
		return true;
	}
}

/**
 * Il caso del piano: 420 perforante contro DEF 45 in copertura bassa frontale da' 420. Il controllo e' lo stesso colpo senza il
 * flag, 229: dice che la DEF e la copertura mordono nel banco, quindi che 420 non e' 420 per caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingIgnoresDefenseAndLowCoverInATurnTest,
	"RefactorTactics.Damage.Piercing.IgnoresDefenseAndLowCoverInATurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingIgnoresDefenseAndLowCoverInATurnTest::RunTest(const FString&)
{
	FPcTurnSpec Perforante;
	Perforante.bPiercing = true;
	FPcTurnSpec Normale;
	FPcTurnOutcome P, N;
	if (!PcTurn(*this, Perforante, P) || !PcTurn(*this, Normale, N)) { return false; }

	TestEqual(TEXT("controllo: senza il flag, floor(420 x 100/145) - 60 = 289 - 60 = 229"), N.HpLost,
		URTCombatLibrary::MitigateDirectDamage(PcPower, 0, 100, PcDefense) - URTCombatLibrary::LowCoverDamageReduction);
	TestEqual(TEXT("controllo: e sono 229"), N.HpLost, 229);
	TestEqual(TEXT("perforante: 420, senza DEF e senza il muretto ([D-634] (47))"), P.HpLost, PcPower);
	return true;
}

/**
 * La Guard resta: 420 - 100 = 320. Il controllo senza flag, in Guard, e' 229 - 100 = 129; senza Guard il perforante e' 420. La
 * differenza fra 420 e 320 e' la Guard, e non e' una difesa che il flag salta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingGuardStillReducesTest,
	"RefactorTactics.Damage.Piercing.GuardStillReduces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingGuardStillReducesTest::RunTest(const FString&)
{
	FPcTurnSpec Perforante;
	Perforante.bPiercing = true;
	FPcTurnSpec PerforanteInGuard = Perforante;
	PerforanteInGuard.bGuarded = true;
	FPcTurnSpec NormaleInGuard;
	NormaleInGuard.bGuarded = true;
	FPcTurnOutcome P, PG, NG;
	if (!PcTurn(*this, Perforante, P) || !PcTurn(*this, PerforanteInGuard, PG) || !PcTurn(*this, NormaleInGuard, NG))
	{
		return false;
	}

	TestEqual(TEXT("controllo: perforante senza Guard, 420"), P.HpLost, PcPower);
	TestEqual(TEXT("perforante in Guard: 420 - 100 = 320, la Guard non e' saltata"), PG.HpLost,
		PcPower - URTCombatLibrary::GuardReductionDefault);
	TestEqual(TEXT("controllo: senza il flag, in Guard: 229 - 100 = 129"), NG.HpLost, 129);
	return true;
}

/**
 * Lo scudo base vale come per ogni colpo `Direct`: un perforante SENZA elemento lo e'. Con lo scudo base (50) e senza Guard i
 * 420 diventano 370 HP persi; con la Guard 420 - 100 = 320 e poi lo scudo, 270. Il confronto con `ApplyDamage` e' la
 * definizione di «come per ogni colpo `Direct`».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingBaseShieldStopsItAsADirectHitTest,
	"RefactorTactics.Damage.Piercing.BaseShieldStopsItAsADirectHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingBaseShieldStopsItAsADirectHitTest::RunTest(const FString&)
{
	FPcTurnSpec ConScudo;
	ConScudo.bPiercing = true;
	ConScudo.bBaseShield = true;
	FPcTurnSpec ConScudoInGuard = ConScudo;
	ConScudoInGuard.bGuarded = true;
	FPcTurnOutcome S, SG;
	if (!PcTurn(*this, ConScudo, S) || !PcTurn(*this, ConScudoInGuard, SG)) { return false; }

	const FRTDamageResult Atteso = URTCombatLibrary::ApplyDamage(PcPower, ERTDamageSource::Direct,
		URTCombatLibrary::BaseShield, 0, 10000);
	TestEqual(TEXT("perforante con lo scudo base: 420 - 50 = 370"), S.HpLost, 10000 - Atteso.Health);
	TestEqual(TEXT("e sono 370"), S.HpLost, PcPower - URTCombatLibrary::BaseShield);

	TestEqual(TEXT("perforante con scudo base e Guard: 420 - 100 - 50 = 270"), SG.HpLost,
		PcPower - URTCombatLibrary::GuardReductionDefault - URTCombatLibrary::BaseShield);
	return true;
}

/**
 * Il perforante resta tale quando un ricalcolo del piano lo riprende: `Exposed` (Mod 120) e il marchio (+60 nel Base) lo
 * ricalcolano da `NominalPower`, `RearBonusApplied`, `ModPercentApplied` e `DefenseApplied`, e sottraggono `CoverReduction`.
 * Con difesa 0 e copertura 0 registrate sul colpo, il risultato e' floor((420 + 60) x 120/100) = 576, senza DEF e senza il
 * muretto. Il controllo senza il flag: floor(480 x 120 x 100/(100 x 145)) - 60 = 397 - 60 = 337.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingMarkAndExposedKeepDefenseAndCoverAtZeroTest,
	"RefactorTactics.Damage.Piercing.MarkAndExposedKeepDefenseAndCoverAtZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingMarkAndExposedKeepDefenseAndCoverAtZeroTest::RunTest(const FString&)
{
	FPcTurnSpec Perforante;
	Perforante.bPiercing = true;
	Perforante.bMarked = true;
	Perforante.bExposed = true;
	FPcTurnSpec Normale = Perforante;
	Normale.bPiercing = false;
	FPcTurnOutcome P, N;
	if (!PcTurn(*this, Perforante, P) || !PcTurn(*this, Normale, N)) { return false; }

	TestEqual(TEXT("controllo: senza il flag, marchiato ed esposto: floor(480 x 120 x 100/(100 x 145)) - 60 = 337"), N.HpLost,
		URTCombatLibrary::MitigateDirectDamage(PcPower + URTCombatLibrary::MarkedFirstHitBonus, 0,
			URTCombatLibrary::ExposedModPercent, PcDefense) - URTCombatLibrary::LowCoverDamageReduction);
	TestEqual(TEXT("controllo: e sono 337"), N.HpLost, 337);
	TestEqual(TEXT("perforante, marchiato ed esposto: floor((420 + 60) x 120/100) = 576"), P.HpLost,
		(PcPower + URTCombatLibrary::MarkedFirstHitBonus) * URTCombatLibrary::ExposedModPercent / 100);
	TestEqual(TEXT("e sono 576"), P.HpLost, 576);
	return true;
}

/**
 * Lo stesso sul piano puro, dove si vede COSA il colpo ha registrato: un perforante raccolto porta `DefenseApplied` 0 e
 * `CoverReduction` 0, e dopo `AddBonusToHitBase` ed `ApplyExposedToFirstHit` li porta ancora (nessun ricalcolo li riscrive).
 * Tre colpi da 240 contro DEF 45 in copertura bassa frontale, uno per ricalcolo: marchio, `Exposed`, tutti e due.
 *   - A: +60, floor(300 x 100/100) = 300 (senza il flag: floor(300 x 100/145) - 60 = 146);
 *   - B: `Exposed`, floor(240 x 120/100) = 288 (senza il flag: floor(240 x 120 x 100/(100 x 145)) - 60 = 198 - 60 = 138);
 *   - C: `Exposed` poi +60, floor(300 x 120/100) = 360.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingRecomputesKeepZeroOnTheHitTest,
	"RefactorTactics.Damage.Piercing.RecomputesKeepZeroOnTheHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingRecomputesKeepZeroOnTheHitTest::RunTest(const FString&)
{
	const FRTCellId DifA(0, 0, 0), DifB(0, 3, 0), DifC(0, -3, 0);
	URTHexMapAsset* Map = MakePcMap(4);
	PcSetLowCover(Map, DifA, ERTHexDirection::W);
	PcSetLowCover(Map, DifB, ERTHexDirection::W);
	PcSetLowCover(Map, DifC, ERTHexDirection::W);

	TArray<FRTHexCombatUnit> Units;
	Units.Add(PcUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
	Units.Add(PcUnit(1, 1, DifA, ERTHexDirection::W, PcDefense));
	Units.Add(PcUnit(2, 0, FRTCellId(-1, 3, 0), ERTHexDirection::E));
	Units.Add(PcUnit(3, 1, DifB, ERTHexDirection::W, PcDefense));
	Units.Add(PcUnit(4, 0, FRTCellId(-1, -3, 0), ERTHexDirection::E));
	Units.Add(PcUnit(5, 1, DifC, ERTHexDirection::W, PcDefense));
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(PcIntent(DifA, /*Attacker*/ 0, /*Target*/ 1, 240, /*bPiercing*/ true));
	Intents.Add(PcIntent(DifB, /*Attacker*/ 2, /*Target*/ 3, 240, /*bPiercing*/ true));
	Intents.Add(PcIntent(DifC, /*Attacker*/ 4, /*Target*/ 5, 240, /*bPiercing*/ true));

	FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	if (!TestEqual(TEXT("tre colpi raccolti"), Plan.Hits.Num(), 3)) { return false; }
	FRTHexAttackHit* A = PcHitFrom(Plan, 0);
	FRTHexAttackHit* B = PcHitFrom(Plan, 2);
	FRTHexAttackHit* C = PcHitFrom(Plan, 4);
	if (!TestNotNull(TEXT("colpo A"), A) || !TestNotNull(TEXT("colpo B"), B) || !TestNotNull(TEXT("colpo C"), C))
	{
		return false;
	}
	for (const FRTHexAttackHit* Hit : { A, B, C })
	{
		if (!TestEqual(TEXT("premessa: raccolto a 240, senza DEF e senza copertura"), Hit->Power, 240)
			|| !TestEqual(TEXT("premessa: DefenseApplied 0"), Hit->DefenseApplied, 0)
			|| !TestEqual(TEXT("premessa: CoverReduction 0"), Hit->CoverReduction, 0))
		{
			return false;
		}
	}

	// A: il marchio.
	URTHexCombatLibrary::AddBonusToHitBase(*A, URTCombatLibrary::MarkedFirstHitBonus);
	TestEqual(TEXT("A con il marchio: 240 + 60 = 300, senza DEF e senza il muretto"), A->Power, 300);
	TestEqual(TEXT("A: il mitigato e' il ricalcolato, senza difesa"), A->MitigatedPower, 300);
	TestEqual(TEXT("A: DefenseApplied resta 0"), A->DefenseApplied, 0);
	TestEqual(TEXT("A: CoverReduction resta 0"), A->CoverReduction, 0);

	// B, C: Exposed sul piano (B e C esposti), poi il marchio su C.
	TArray<bool> bExposed;
	bExposed.Init(false, Units.Num());
	bExposed[3] = true;
	bExposed[5] = true;
	TArray<int32> Order;
	for (int32 h = 0; h < Plan.Hits.Num(); ++h) { Order.Add(h); }
	const TArray<int32> Boosted = URTHexCombatLibrary::ApplyExposedToFirstHit(Plan.Hits, Order, bExposed);
	TestEqual(TEXT("premessa: Exposed ha moltiplicato due colpi"), Boosted.Num(), 2);
	TestEqual(TEXT("B esposto: floor(240 x 120/100) = 288, senza DEF e senza il muretto"), B->Power, 288);
	TestEqual(TEXT("B: DefenseApplied resta 0"), B->DefenseApplied, 0);
	TestEqual(TEXT("B: CoverReduction resta 0"), B->CoverReduction, 0);
	URTHexCombatLibrary::AddBonusToHitBase(*C, URTCombatLibrary::MarkedFirstHitBonus);
	TestEqual(TEXT("C esposto e marchiato: floor(300 x 120/100) = 360"), C->Power, 360);
	TestEqual(TEXT("C: il mitigato e' il ricalcolato"), C->MitigatedPower, 360);
	TestEqual(TEXT("C: DefenseApplied resta 0"), C->DefenseApplied, 0);
	TestEqual(TEXT("C: CoverReduction resta 0"), C->CoverReduction, 0);
	return true;
}

/**
 * Cosa la raccolta registra su un perforante, contro lo stesso colpo senza il flag. Tutti da 420 contro DEF 45:
 *   - frontale in copertura bassa: Power 420, `MitigatedPower` 420, `DefenseApplied` 0, `CoverReduction` 0, nessun facing
 *     letto (senza il flag: 229, 289, 45, 60);
 *   - alle SPALLE in copertura bassa, con il bonus alle spalle di 30: Power 450, `CoverBypassedByFacing` 0 — una copertura
 *     che non si sottrae non e' una copertura scavalcata dalla direzione, e il bonus alle spalle resta nel Base (senza il
 *     flag: floor(450 x 100/145) = 310, `CoverBypassedByFacing` 60);
 *   - elementale (`Element.Electric`, RES 35): `DefenseApplied` 0 anche contro la RES, Power 420.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingCollectsDefenseAndCoverAtZeroTest,
	"RefactorTactics.Damage.Piercing.CollectsDefenseAndCoverAtZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingCollectsDefenseAndCoverAtZeroTest::RunTest(const FString&)
{
	const FName Electric(TEXT("Element.Electric"));
	const FRTCellId DifFrontale(0, 0, 0), DifSpalle(0, 3, 0), DifElettrico(0, -3, 0);
	URTHexMapAsset* Map = MakePcMap(4);
	PcSetLowCover(Map, DifFrontale, ERTHexDirection::W);
	PcSetLowCover(Map, DifSpalle, ERTHexDirection::W);
	PcSetLowCover(Map, DifElettrico, ERTHexDirection::W);

	for (const bool bPiercing : { true, false })
	{
		const TCHAR* Etichetta = bPiercing ? TEXT("perforante") : TEXT("senza il flag");
		TArray<FRTHexCombatUnit> Units;
		Units.Add(PcUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
		Units.Add(PcUnit(1, 1, DifFrontale, ERTHexDirection::W, PcDefense, /*RES*/ 35)); // guarda l'attaccante: la copertura tiene
		Units.Add(PcUnit(2, 0, FRTCellId(-1, 3, 0), ERTHexDirection::E, 0, 0, PcRearBonus));
		Units.Add(PcUnit(3, 1, DifSpalle, ERTHexDirection::E, PcDefense, 35)); // da' le spalle: la copertura non vale
		Units.Add(PcUnit(4, 0, FRTCellId(-1, -3, 0), ERTHexDirection::E));
		Units.Add(PcUnit(5, 1, DifElettrico, ERTHexDirection::W, PcDefense, 35));
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(PcIntent(DifFrontale, 0, 1, PcPower, bPiercing));
		Intents.Add(PcIntent(DifSpalle, 2, 3, PcPower, bPiercing));
		Intents.Add(PcIntent(DifElettrico, 4, 5, PcPower, bPiercing, Electric));

		FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		if (!TestEqual(TEXT("tre colpi raccolti"), Plan.Hits.Num(), 3)) { return false; }
		const FRTHexAttackHit* Frontale = PcHitFrom(Plan, 0);
		const FRTHexAttackHit* Spalle = PcHitFrom(Plan, 2);
		const FRTHexAttackHit* Elettrico = PcHitFrom(Plan, 4);
		if (!TestNotNull(TEXT("colpo frontale"), Frontale) || !TestNotNull(TEXT("colpo alle spalle"), Spalle)
			|| !TestNotNull(TEXT("colpo elettrico"), Elettrico))
		{
			return false;
		}

		const FString F = FString::Printf(TEXT("%s, frontale in copertura: "), Etichetta);
		const FString S = FString::Printf(TEXT("%s, alle spalle in copertura: "), Etichetta);
		const FString E = FString::Printf(TEXT("%s, elettrico (RES 35) in copertura: "), Etichetta);
		if (bPiercing)
		{
			TestEqual(F + TEXT("420, senza DEF e senza muretto"), Frontale->Power, PcPower);
			TestEqual(F + TEXT("il mitigato e' 420"), Frontale->MitigatedPower, PcPower);
			TestEqual(F + TEXT("DefenseApplied 0"), Frontale->DefenseApplied, 0);
			TestEqual(F + TEXT("CoverReduction 0"), Frontale->CoverReduction, 0);
			TestFalse(F + TEXT("il facing non e' stato letto: la copertura non si e' valutata"),
				URTHexCombatLibrary::CoverReadTargetFacing(*Frontale));

			TestEqual(S + TEXT("420 + 30 di spalle, senza DEF"), Spalle->Power, PcPower + PcRearBonus);
			TestEqual(S + TEXT("il bonus alle spalle resta nel Base"), Spalle->RearBonusApplied, PcRearBonus);
			TestEqual(S + TEXT("DefenseApplied 0"), Spalle->DefenseApplied, 0);
			TestEqual(S + TEXT("nessuna copertura scavalcata dalla direzione: non c'era niente da sottrarre"),
				Spalle->CoverBypassedByFacing, 0);

			TestEqual(E + TEXT("420, anche contro la RES"), Elettrico->Power, PcPower);
			TestEqual(E + TEXT("DefenseApplied 0"), Elettrico->DefenseApplied, 0);
			TestEqual(E + TEXT("l'elemento resta quello dell'azione"), Elettrico->ElementId, Electric);
		}
		else
		{
			TestEqual(F + TEXT("floor(420 x 100/145) - 60 = 229"), Frontale->Power, 229);
			TestEqual(F + TEXT("DefenseApplied 45"), Frontale->DefenseApplied, PcDefense);
			TestEqual(F + TEXT("CoverReduction 60"), Frontale->CoverReduction, URTCombatLibrary::LowCoverDamageReduction);

			TestEqual(S + TEXT("floor(450 x 100/145) = 310"), Spalle->Power, 310);
			TestEqual(S + TEXT("la direzione ha scavalcato 60 di copertura"), Spalle->CoverBypassedByFacing,
				URTCombatLibrary::LowCoverDamageReduction);

			TestEqual(E + TEXT("floor(420 x 100/135) - 60 = 251"), Elettrico->Power, 251);
			TestEqual(E + TEXT("DefenseApplied 35, la RES"), Elettrico->DefenseApplied, 35);
		}
	}
	return true;
}

/**
 * Un colpo intercettato cambia bersaglio e conserva cio' che l'azione e' (#4037, `Intercept`): `RedirectHitTo` ricalcola
 * difesa e copertura per l'intercettore dall'INTENTO, e per un perforante restano a zero. Due colpi da 420, ciascuno con il suo
 * intercettore (DEF 45, copertura bassa sul lato da cui arriva il colpo):
 *   - frontale: 420 (senza il flag floor(420 x 100/145) - 60 = 229, `DefenseApplied` 45);
 *   - alle spalle, con il bonus alle spalle di 30: 450 e `CoverBypassedByFacing` 0 (senza il flag floor(450 x 100/145) = 310 e
 *     60 di copertura scavalcata dalla direzione).
 *
 * ⚠️ `CoverReduction` del colpo redirezionato NON si asserisce: resta quello del bersaglio originale, difetto latente e
 * dichiarato (#4037, `RedirectHitTo`), che questo cambio non tocca. Il perforante lo eredita a zero perche' nasce a zero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingRedirectedHitStaysPiercingTest,
	"RefactorTactics.Damage.Piercing.RedirectedHitStaysPiercing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingRedirectedHitStaysPiercingTest::RunTest(const FString&)
{
	const FRTCellId FrontaleIntercettore(0, 0, 0), FrontaleOriginale(2, 0, 0);
	const FRTCellId SpalleIntercettore(0, 3, 0), SpalleOriginale(2, 3, 0);
	URTHexMapAsset* Map = MakePcMap(6);
	PcSetLowCover(Map, FrontaleIntercettore, ERTHexDirection::W);
	PcSetLowCover(Map, SpalleIntercettore, ERTHexDirection::W);

	for (const bool bPiercing : { true, false })
	{
		const TCHAR* Etichetta = bPiercing ? TEXT("perforante") : TEXT("senza il flag");
		TArray<FRTHexCombatUnit> Units;
		Units.Add(PcUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E, 0, 0, PcRearBonus));
		Units.Add(PcUnit(1, 1, FrontaleOriginale, ERTHexDirection::W));                   // il bersaglio mirato, senza DEF
		Units.Add(PcUnit(2, 1, FrontaleIntercettore, ERTHexDirection::W, PcDefense));     // si frappone: guarda il tiratore
		Units.Add(PcUnit(3, 0, FRTCellId(-1, 3, 0), ERTHexDirection::E, 0, 0, PcRearBonus));
		Units.Add(PcUnit(4, 1, SpalleOriginale, ERTHexDirection::W));
		Units.Add(PcUnit(5, 1, SpalleIntercettore, ERTHexDirection::E, PcDefense));       // si frappone: da' le spalle al tiratore
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(PcIntent(FrontaleOriginale, 0, 1, PcPower, bPiercing));
		Intents.Add(PcIntent(SpalleOriginale, 3, 4, PcPower, bPiercing));

		FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		const FRTHexAttackHit* Frontale = PcHitFrom(Plan, 0);
		const FRTHexAttackHit* Spalle = PcHitFrom(Plan, 3);
		if (!TestNotNull(*FString::Printf(TEXT("%s: il colpo frontale c'e'"), Etichetta), Frontale)
			|| !TestNotNull(*FString::Printf(TEXT("%s: il colpo alle spalle c'e'"), Etichetta), Spalle))
		{
			return false;
		}
		if (!TestEqual(*FString::Printf(TEXT("%s: premessa, sui bersagli mirati il colpo e' pieno (420)"), Etichetta),
				Frontale->Power, PcPower)
			|| !TestEqual(*FString::Printf(TEXT("%s: premessa, anche il secondo, perche' il bersaglio mirato lo guarda"),
				Etichetta), Spalle->Power, PcPower))
		{
			return false;
		}

		const FRTHexAttackHit RedirFrontale = URTHexCombatLibrary::RedirectHitTo(2, *Frontale, Units, Intents, Map);
		const FRTHexAttackHit RedirSpalle = URTHexCombatLibrary::RedirectHitTo(5, *Spalle, Units, Intents, Map);
		if (!TestEqual(*FString::Printf(TEXT("%s: premessa, il colpo frontale ha cambiato bersaglio"), Etichetta),
				RedirFrontale.TargetId, 2)
			|| !TestEqual(*FString::Printf(TEXT("%s: premessa, il colpo alle spalle ha cambiato bersaglio"), Etichetta),
				RedirSpalle.TargetId, 5))
		{
			return false;
		}

		if (bPiercing)
		{
			TestEqual(TEXT("perforante redirezionato, frontale: 420"), RedirFrontale.Power, PcPower);
			TestEqual(TEXT("perforante redirezionato, frontale: DefenseApplied 0"), RedirFrontale.DefenseApplied, 0);
			TestEqual(TEXT("perforante redirezionato, alle spalle: 420 + 30"), RedirSpalle.Power, PcPower + PcRearBonus);
			TestEqual(TEXT("perforante redirezionato, alle spalle: DefenseApplied 0"), RedirSpalle.DefenseApplied, 0);
			TestEqual(TEXT("perforante redirezionato, alle spalle: nessuna copertura scavalcata"),
				RedirSpalle.CoverBypassedByFacing, 0);
		}
		else
		{
			TestEqual(TEXT("senza il flag, redirezionato, frontale: floor(420 x 100/145) - 60 = 229"), RedirFrontale.Power, 229);
			TestEqual(TEXT("senza il flag, redirezionato, frontale: DefenseApplied 45"), RedirFrontale.DefenseApplied,
				PcDefense);
			TestEqual(TEXT("senza il flag, redirezionato, alle spalle: floor(450 x 100/145) = 310"), RedirSpalle.Power, 310);
			TestEqual(TEXT("senza il flag, redirezionato, alle spalle: la direzione ha scavalcato 60 di copertura"),
				RedirSpalle.CoverBypassedByFacing, URTCombatLibrary::LowCoverDamageReduction);
		}
	}
	return true;
}

/**
 * La copertura ALTA continua a fermare la linea di tiro: un colpo perforante non attraversa un muro ([D-634] (47) toglie la
 * sola sottrazione della copertura BASSA). Un perforante da 420 mirato a un bersaglio con un muro alto sul lato da cui
 * arriva non produce nessun colpo e finisce fra gli intenti bloccati; lo stesso con un muretto basso colpisce, a 420. Il
 * controllo senza flag col muro alto da' lo stesso esito (nessun colpo): il flag non cambia la linea, in nessuno dei due versi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingHighCoverStillBlocksTheLineTest,
	"RefactorTactics.Damage.Piercing.HighCoverStillBlocksTheLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingHighCoverStillBlocksTheLineTest::RunTest(const FString&)
{
	const FRTCellId Difensore(0, 0, 0);
	for (const bool bPiercing : { true, false })
	{
		const TCHAR* Etichetta = bPiercing ? TEXT("perforante") : TEXT("senza il flag");
		TArray<FRTHexCombatUnit> Units;
		Units.Add(PcUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
		Units.Add(PcUnit(1, 1, Difensore, ERTHexDirection::W, PcDefense));
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(PcIntent(Difensore, 0, 1, PcPower, bPiercing));

		URTHexMapAsset* MuroAlto = MakePcMap(4);
		PcSetCover(MuroAlto, Difensore, ERTHexDirection::W, ERTHexCoverType::High);
		const FRTHexBlastPlan Bloccato = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MuroAlto);
		TestEqual(*FString::Printf(TEXT("%s: con un muro alto sul lato del tiro, nessun colpo"), Etichetta),
			Bloccato.Hits.Num(), 0);
		TestTrue(*FString::Printf(TEXT("%s: l'intento e' bloccato dalla linea di tiro"), Etichetta),
			Bloccato.BlockedIntents.Contains(0));
	}

	// Controllo: lo stesso perforante con un muretto BASSO sul lato del tiro colpisce, e a 420 pieni.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(PcUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
		Units.Add(PcUnit(1, 1, Difensore, ERTHexDirection::W, PcDefense));
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(PcIntent(Difensore, 0, 1, PcPower, /*bPiercing*/ true));

		URTHexMapAsset* Muretto = MakePcMap(4);
		PcSetLowCover(Muretto, Difensore, ERTHexDirection::W);
		FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Muretto);
		if (TestEqual(TEXT("controllo: con un muretto basso il perforante colpisce"), Plan.Hits.Num(), 1))
		{
			TestEqual(TEXT("e a 420 pieni"), Plan.Hits[0].Power, PcPower);
		}
		TestEqual(TEXT("e non e' bloccato"), Plan.BlockedIntents.Num(), 0);
	}
	return true;
}

/**
 * Il flag dell'azione arriva all'intento, nei DUE punti in cui l'intento nasce (`CollectAttackIntents` e l'impatto di una
 * carica, `AppendChargeImpactIntents`): ogni campo dell'intento che viene dal `Def` e' stato dimenticato almeno una volta in
 * uno dei due (lo dicono i commenti accanto a `bFriendlyFire`, `LineOfSightPolicy` e `ElementId`). Lo stesso turno senza il
 * flag e' il controllo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingTravelsFromActionToIntentTest,
	"RefactorTactics.Damage.Piercing.TravelsFromActionToIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingTravelsFromActionToIntentTest::RunTest(const FString&)
{
	struct FOutcome { int32 Intents = 0; bool bIntentPiercing = false; };

	// Il primo punto: l'attacco base di Ivrin contro un'altra Ivrin, adiacente e frontale.
	const auto Turno = [this](bool bPiercing, FOutcome& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakePcMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		ARTUnit* Attaccante = PcSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-1, 0, 0));
		ARTUnit* Bersaglio = PcSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
		if (!TestNotNull(TEXT("mappa"), MapActor) || !TestNotNull(TEXT("turn manager"), TM)
			|| !TestNotNull(TEXT("attaccante"), Attaccante) || !TestNotNull(TEXT("bersaglio"), Bersaglio)
			|| !TestNotNull(TEXT("l'attacco base"), Attaccante->GetAbility(0)))
		{
			return false;
		}

		Attaccante->GetAbility(0)->Def.bPiercing = bPiercing;
		Bersaglio->Facing = ERTHexDirection::W;
		Bersaglio->ClearPlannedMainAction();
		Attaccante->SetPlannedMainAction(0);
		Attaccante->DeclareAttackOnUnit(Bersaglio);

		TM->bRecordBlastPlanForTest = true;
		PcRunTurn(TM);

		const TArray<FRTHexAttackIntent>& Intents = TM->RecordedBlastIntentsForTest();
		Out.Intents = Intents.Num();
		Out.bIntentPiercing = Intents.Num() == 1 && Intents[0].bPiercing;
		return true;
	};

	// Il secondo: Riktor carica dritto e incontra Ivrin a (2,0), come `Damage.Element.ChargeImpactCarriesTheChargeElement`.
	const auto Carica = [this](bool bPiercing, FOutcome& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakePcMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		ARTUnit* Caricatore = PcSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
		ARTUnit* Vittima = PcSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0, 0));
		if (!TestNotNull(TEXT("mappa"), MapActor) || !TestNotNull(TEXT("turn manager"), TM)
			|| !TestNotNull(TEXT("caricatore"), Caricatore) || !TestNotNull(TEXT("vittima"), Vittima))
		{
			return false;
		}

		const int32 Idx = RTAbilityFixtures::AddCoreAbility(Caricatore, TEXT("Action.Charge"));
		if (!TestNotNull(TEXT("la carica"), Caricatore->GetAbility(Idx))) { return false; }
		Caricatore->GetAbility(Idx)->Def.bPiercing = bPiercing;
		Vittima->ClearPlannedMainAction();
		Caricatore->SetPlannedDash(Idx, FRTCellId(3, 0, 0)); // dritto: incontra la vittima a (2,0) per strada

		TM->bRecordBlastPlanForTest = true;
		PcRunTurn(TM);

		const TArray<FRTHexAttackIntent>& Intents = TM->RecordedBlastIntentsForTest();
		Out.Intents = Intents.Num();
		Out.bIntentPiercing = Intents.Num() == 1 && Intents[0].bPiercing;
		return true;
	};

	FOutcome AttaccoPerforante, AttaccoNormale, CaricaPerforante, CaricaNormale;
	if (!Turno(true, AttaccoPerforante) || !Turno(false, AttaccoNormale)
		|| !Carica(true, CaricaPerforante) || !Carica(false, CaricaNormale))
	{
		return false;
	}

	if (!TestEqual(TEXT("premessa: un intento dall'attacco"), AttaccoPerforante.Intents, 1)
		|| !TestEqual(TEXT("premessa: un intento dall'impatto della carica"), CaricaPerforante.Intents, 1))
	{
		return false;
	}
	TestTrue(TEXT("l'intento dell'attacco porta il flag dell'azione"), AttaccoPerforante.bIntentPiercing);
	TestFalse(TEXT("controllo: senza il flag, l'intento dell'attacco non e' perforante"), AttaccoNormale.bIntentPiercing);
	TestTrue(TEXT("l'impatto della carica porta il flag della carica"), CaricaPerforante.bIntentPiercing);
	TestFalse(TEXT("controllo: senza il flag, l'impatto non e' perforante"), CaricaNormale.bIntentPiercing);
	return true;
}

/**
 * Un'azione d'eroe derivata da una riga core ne eredita il flag perforante: `MakeHeroActionFromCore` ricostruisce il `Def`
 * campo per campo, e ogni campo di comportamento che copia e' stato aggiunto dopo il proprio difetto (vedi
 * `HeroCatalog.DerivedFromCoreKeepsTheElement`). ⚠️ Nessuna riga core e' perforante oggi, quindi si deriva da un catalogo dato
 * (`MakeHeroActionFromCatalog`), copia del core con `Action.Charge` perforante. Il controllo e' la derivazione dal core vero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePiercingHeroDerivedFromCoreKeepsTheFlagTest,
	"RefactorTactics.Damage.Piercing.HeroDerivedFromCoreKeepsTheFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePiercingHeroDerivedFromCoreKeepsTheFlagTest::RunTest(const FString&)
{
	const FName Carica(TEXT("Action.Charge"));

	TArray<FRTActionDef> Catalogo = URTCatalogLibrary::GetCoreActionCatalog();
	FRTActionDef* Riga = Catalogo.FindByPredicate([&Carica](const FRTActionDef& D) { return D.ActionId == Carica; });
	if (!TestNotNull(TEXT("premessa: Action.Charge e' nel catalogo core"), Riga)) { return false; }
	if (!TestFalse(TEXT("premessa: il core non dichiara un'azione perforante"), Riga->bPiercing)) { return false; }
	Riga->bPiercing = true;

	const URTActionData* Derivata = URTHeroCatalogLibrary::MakeHeroActionFromCatalog(
		TEXT("Hero.Test.Perforante"), Catalogo, Carica, /*Cooldown*/ 2);
	if (TestNotNull(TEXT("la derivazione dal catalogo dato produce l'abilita'"), Derivata))
	{
		TestTrue(TEXT("e ne eredita il flag perforante"), Derivata->Def.bPiercing);
	}

	const URTActionData* DalCore = URTHeroCatalogLibrary::MakeHeroActionFromCore(TEXT("Hero.Test.Normale"), Carica, 2);
	if (TestNotNull(TEXT("controllo: la derivazione dal core produce l'abilita'"), DalCore))
	{
		TestFalse(TEXT("e non e' perforante"), DalCore->Def.bPiercing);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
