#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
// Il test sul turno intero passa da un TURNO vero: lo stato `Exposed` sull'`ARTUnit` deve arrivare al pass del Blast
// e da li' al colpo del piano. Misurarlo sul piano puro non lo proverebbe.
#include "Ability/RTHeroCatalogLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * E53 CP 53.3b — `Status.Exposed` ENTRA nella formula come moltiplicatore 1,2 in `Mod` ([D-589] (3), [D-595] (2)).
 *
 * `Danno = max(0, floor((Base + Spalle) × Mod × 100/(100+DEF)) − Copertura − Guard)`: qui si prova che il pass del
 * Blast (`URTHexCombatLibrary::ApplyExposedToFirstHit`) ricalcoli il colpo RACCOLTO con `Mod` 120 — prima della DEF e
 * della copertura, sul solo primo colpo che fa danno — e che il breakdown lo racconti fra le spalle e la DEF. I numeri
 * sono l'esempio 7 della tabella di controllo di [D-595]: `PrecisionAttack` 240 contro DEF 25 su un bersaglio
 * `Exposed` → `floor(288 × 100/125) = 230`, non 192.
 *
 * ⚠️ **Perche' non basta `Combat.Formula.ExposedMultipliesTheBase`**: quello prova la funzione pura con un 120 scritto a
 * mano. Questi provano che il pass la CHIAMI con il 120 del catalogo, sul colpo giusto, nell'ordine giusto rispetto a
 * DEF e copertura, e che lo stato dell'`ARTUnit` arrivi fin li' in un turno vero. `Status.Exposed.FirstDirectHitOnly`
 * (in `RTCombatResolverTests.cpp`) pinna il «primo» e l'ordine; qui si pinna la composizione con il resto della formula.
 */

namespace
{
	/** Arena piena di raggio N. Nome distinto per file: la unity build condivide la translation unit. */
	URTHexMapAsset* MakeExpMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	void ExpSetLowCover(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	FRTHexCombatUnit ExpUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing,
		int32 Defense = 0, int32 RearBonus = 0)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		U.Facing = Facing;
		U.Defense = Defense;
		U.RearBonus = RearBonus;
		return U;
	}

	FRTHexAttackIntent ExpIntent(const FRTCellId& AimCell, int32 AttackerId, int32 TargetId, int32 Power)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.TargetCell = AimCell;
		I.Shape = ERTAbilityShape::Single;
		I.RangeCells = 5;
		I.AreaRadius = 0;
		I.Power = Power;
		I.bCountsAsAttack = true;
		return I;
	}

	/** L'ordine canonico del Blast non e' in gioco qui: l'identita' sull'array del piano, che `CollectHexAttacks` ordina. */
	TArray<int32> ExpIdentityOrder(const FRTHexBlastPlan& Plan)
	{
		TArray<int32> Order;
		for (int32 h = 0; h < Plan.Hits.Num(); ++h) { Order.Add(h); }
		return Order;
	}

	const FRTHexAttackHit* ExpHitFrom(const FRTHexBlastPlan& Plan, int32 AttackerId)
	{
		for (const FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.AttackerId == AttackerId) { return &Hit; }
		}
		return nullptr;
	}

	ARTUnit* ExpSpawnUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * HP persi dal difensore in UN turno intero, cambiando SOLO la DEF iniettata e lo stato `Exposed` messo a mano
	 * prima del lock-in (come il Blast lo trova dopo uno Sprint del turno precedente, [D-116]).
	 *
	 * Attaccante a OVEST del difensore, che GUARDA a ovest: colpo frontale, nessuna copertura (arena piena). ⚠️ Lo scudo
	 * NON e' zero: a inizio turno `ARTUnit::RechargeBaseShield` arma `URTCombatLibrary::BaseShield`, che assorbe prima
	 * degli HP. Il chiamante ricompone il colpo pieno `P` sommando lo scudo agli HP persi del caso base.
	 *
	 * `-1` = montaggio fallito, gia' segnalato da `Test`.
	 */
	int32 ExpTurnDamageTaken(FAutomationTestBase& Test, int32 DefenderDefense, bool bExposed)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), World)) { return -1; }

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakeExpMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();

		ARTUnit* Attaccante = ExpSpawnUnit(World, /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
		ARTUnit* Difensore = ExpSpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0));

		if (!Test.TestNotNull(TEXT("mappa"), MapActor) || !Test.TestNotNull(TEXT("turn manager"), TM)
			|| !Test.TestNotNull(TEXT("attaccante"), Attaccante) || !Test.TestNotNull(TEXT("difensore"), Difensore))
		{
			RTWorldFixtures::DestroyWorld(World);
			return -1;
		}

		Difensore->Facing = ERTHexDirection::W; // guarda l'attaccante: nessun bonus alle spalle in gioco
		Difensore->Defense = DefenderDefense;   // INIETTATA: sovrascrive il 15 di Ivrin (CP 53.3c): qui si misura il pass
		Difensore->PlannedAbilityIndex = INDEX_NONE;
		if (bExposed)
		{
			Difensore->ApplyStatus(TAG_Status_Exposed, /*Turni*/ 2);
		}

		Attaccante->PlannedAbilityIndex = 0; // attacco base (`Hero.Ivrin.PulseShot`)
		Attaccante->PlannedAttackTarget = Difensore;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}

		const int32 Taken = Difensore->MaxHealth - Difensore->Health;
		RTWorldFixtures::DestroyWorld(World);
		return Taken;
	}
}

/**
 * Esempio 7 di [D-595]: il colpo raccolto con DEF 25 vale 192; il pass di `Exposed` lo porta a 230, cioe'
 * `floor(240 × 120/100 × 100/125)`. Tre coppie sullo stesso piano, perche' il 240 da solo non distingue l'ordine:
 * - 240 contro DEF 25: 230 (l'esempio della decisione; `floor(192 × 1,2)` darebbe 230 anche moltiplicando il mitigato);
 * - 111 contro DEF 25: **106**, non 105 — `floor(111 × 1,2 × 100/125) = floor(106,56)` contro
 *   `floor(floor(111 × 100/125) × 1,2) = floor(88 × 1,2) = 105`: un solo arrotondamento, dopo il prodotto ([D-595] (1)),
 *   e il moltiplicatore PRIMA della DEF;
 * - 240 contro DEF 25 in copertura bassa frontale (100): **130** = 230 − 100, non `floor((192 − 100) × 1,2) = 110`:
 *   il moltiplicatore sta PRIMA della copertura, che resta piatta e si sottrae dopo ([D-589] (3)).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatExposedMultipliesBeforeDefenseTest,
	"RefactorTactics.Combat.ExposedMultipliesBeforeDefenseAndCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatExposedMultipliesBeforeDefenseTest::RunTest(const FString&)
{
	const FRTCellId DifA(0, 0, 0), DifB(0, 3, 0), DifC(0, -3, 0);
	URTHexMapAsset* Map = MakeExpMap(4);
	ExpSetLowCover(Map, DifC, ERTHexDirection::W); // muretto sul lato da cui arriva l'attaccante 5

	TArray<FRTHexCombatUnit> Units;
	Units.Add(ExpUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
	Units.Add(ExpUnit(1, 1, DifA, ERTHexDirection::W, /*Defense*/ 25)); // guarda l'attaccante 0: frontale
	Units.Add(ExpUnit(2, 0, FRTCellId(-1, 3, 0), ERTHexDirection::E));
	Units.Add(ExpUnit(3, 1, DifB, ERTHexDirection::W, /*Defense*/ 25));
	Units.Add(ExpUnit(4, 0, FRTCellId(-1, -3, 0), ERTHexDirection::E));
	Units.Add(ExpUnit(5, 1, DifC, ERTHexDirection::W, /*Defense*/ 25)); // guarda il muretto: la copertura tiene
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(ExpIntent(DifA, /*Attacker*/ 0, /*Target*/ 1, /*Power*/ 240));
	Intents.Add(ExpIntent(DifB, /*Attacker*/ 2, /*Target*/ 3, /*Power*/ 111));
	Intents.Add(ExpIntent(DifC, /*Attacker*/ 4, /*Target*/ 5, /*Power*/ 240));

	FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	if (!TestEqual(TEXT("tre colpi raccolti"), Plan.Hits.Num(), 3)) { return false; }
	const FRTHexAttackHit* A = ExpHitFrom(Plan, 0);
	const FRTHexAttackHit* B = ExpHitFrom(Plan, 2);
	const FRTHexAttackHit* C = ExpHitFrom(Plan, 4);
	if (!TestNotNull(TEXT("colpo 0"), A) || !TestNotNull(TEXT("colpo 2"), B) || !TestNotNull(TEXT("colpo 4"), C)) { return false; }
	TestEqual(TEXT("premessa A: raccolto con la sola DEF, 240 → 192"), A->Power, 192);
	TestEqual(TEXT("premessa B: 111 → 88"), B->Power, 88);
	TestEqual(TEXT("premessa C: 192 − 100 di copertura = 92"), C->Power, 92);
	TestEqual(TEXT("premessa C: la copertura ha morso"), C->CoverReduction, URTCombatLibrary::LowCoverDamageReduction);

	const TArray<bool> Esposto = { false, true, false, true, false, true };
	const TArray<int32> Moltiplicati = URTHexCombatLibrary::ApplyExposedToFirstHit(Plan.Hits, ExpIdentityOrder(Plan), Esposto);
	A = ExpHitFrom(Plan, 0);
	B = ExpHitFrom(Plan, 2);
	C = ExpHitFrom(Plan, 4);
	TestEqual(TEXT("A — D-595 esempio 7: 240 Exposed contro DEF 25 → 230"), A->Power, 230);
	TestEqual(TEXT("A — il mitigato e' il nuovo valore"), A->MitigatedPower, 230);
	TestEqual(TEXT("A — e il colpo dichiara il Mod 120"), A->ModPercentApplied, URTCombatLibrary::ExposedModPercent);
	TestEqual(TEXT("A — la DEF letta non cambia"), A->DefenseApplied, 25);
	TestEqual(TEXT("B — un solo floor dopo il prodotto: 106, non 105"), B->Power, 106);
	TestEqual(TEXT("C — prima della copertura: 230 − 100 = 130, non floor(92 × 1,2) = 110"), C->Power, 130);
	TestEqual(TEXT("C — il mitigato e' 230 e la copertura resta 100"), C->MitigatedPower, 230);
	TestEqual(TEXT("C — la copertura non e' stata toccata"), C->CoverReduction, URTCombatLibrary::LowCoverDamageReduction);
	TestEqual(TEXT("tre colpi moltiplicati, uno per bersaglio"), Moltiplicati.Num(), 3);

	// Il valore che il pass scrive e' quello della funzione pura con il 120 del catalogo: nessuna seconda formula.
	TestEqual(TEXT("A coincide con MitigateDirectDamage(240, 0, 120, 25)"), A->Power,
		URTCombatLibrary::MitigateDirectDamage(240, 0, URTCombatLibrary::ExposedModPercent, 25));
	TestEqual(TEXT("B coincide con MitigateDirectDamage(111, 0, 120, 25)"), B->Power,
		URTCombatLibrary::MitigateDirectDamage(111, 0, URTCombatLibrary::ExposedModPercent, 25));
	return true;
}

/**
 * Il breakdown racconta il `Mod` fra le spalle e la DEF, come la formula: Catalog 240 → RearBonus +60 → 300 →
 * Mod ×120 → 360 → Defense → 288. E' `MitigateDirectDamage(240, 60, 120, 25) = 288` di `Combat.Formula.*`, letto voce per voce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBreakdownRecordsModBetweenRearAndDefenseTest,
	"RefactorTactics.Damage.BreakdownRecordsModBetweenRearAndDefense",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBreakdownRecordsModBetweenRearAndDefenseTest::RunTest(const FString&)
{
	const FRTCellId Difensore(0, 0, 0);
	TArray<FRTHexCombatUnit> Units;
	Units.Add(ExpUnit(0, 0, FRTCellId(1, 0, 0), ERTHexDirection::W, /*Defense*/ 0, /*RearBonus*/ 60)); // a EST, alle spalle
	Units.Add(ExpUnit(1, 1, Difensore, ERTHexDirection::W, /*Defense*/ 25));                           // guarda a ovest
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(ExpIntent(Difensore, 0, 1, 240));

	FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeExpMap(4));
	if (!TestEqual(TEXT("un colpo raccolto"), Plan.Hits.Num(), 1)) { return false; }
	TestEqual(TEXT("premessa: alle spalle con DEF, (240 + 60) × 100/125 = 240"), Plan.Hits[0].Power, 240);

	const TArray<bool> Esposto = { false, true };
	URTHexCombatLibrary::ApplyExposedToFirstHit(Plan.Hits, ExpIdentityOrder(Plan), Esposto);
	TestEqual(TEXT("(240 + 60) × 1,2 × 100/125 = 288"), Plan.Hits[0].Power, 288);

	const TArray<FRTAttack> Attacks = URTHexCombatLibrary::ToAttacks(Plan);
	if (!TestEqual(TEXT("un attacco"), Attacks.Num(), 1)) { return false; }
	const TArray<FRTDamageStageEntry>& S = Attacks[0].Breakdown;
	if (!TestEqual(TEXT("quattro voci: Catalog, RearBonus, Mod, Defense"), S.Num(), 4)) { return false; }
	TestTrue(TEXT("[0] catalogo"), S[0].Stage == ERTDamageStage::Catalog);
	TestEqual(TEXT("[0] 240"), S[0].After, 240);
	TestTrue(TEXT("[1] spalle"), S[1].Stage == ERTDamageStage::RearBonus);
	TestEqual(TEXT("[1] 240 → 300"), S[1].After, 300);
	TestTrue(TEXT("[2] Mod"), S[2].Stage == ERTDamageStage::Mod);
	TestTrue(TEXT("[2] operazione Multiply"), S[2].Operation == ERTDamageOp::Multiply);
	TestEqual(TEXT("[2] operando 120"), S[2].Operand, URTCombatLibrary::ExposedModPercent);
	TestEqual(TEXT("[2] prima 300"), S[2].Before, 300);
	TestEqual(TEXT("[2] dopo 360"), S[2].After, 360);
	TestTrue(TEXT("[2] provenienza D-595"), S[2].SourceId == URTCombatLibrary::ExposedModSource);
	TestTrue(TEXT("[3] difesa"), S[3].Stage == ERTDamageStage::Defense);
	TestEqual(TEXT("[3] prima 360"), S[3].Before, 360);
	TestEqual(TEXT("[3] dopo 288: il valore reale, con un solo floor"), S[3].After, 288);
	TestEqual(TEXT("e l'attacco porta 288"), Attacks[0].Power, 288);
	return true;
}

/**
 * Lo stato dell'`ARTUnit` arriva al pass del Blast in un turno vero e il vecchio `+50` non c'e' piu':
 * HP persi = `floor(P × 1,2 × 100/(100+DEF)) − BaseShield`, dove `P` e' il colpo pieno ricomposto dal caso base.
 *
 * ⚠️ **Questo test prova il CABLAGGIO, non l'ordine fra `Mod` e DEF**: con il `PulseShot` da 210 del banco, Mod
 * prima o dopo la DEF 25 fa 201 in entrambi i versi. L'ordine lo pinna il caso B (111 → 106, non 105) del test sul
 * piano, dove il numero e' scelto apposta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatExposedMultipliesInAFullTurnTest,
	"RefactorTactics.Combat.ExposedMultipliesBeforeDefenseInAFullTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatExposedMultipliesInAFullTurnTest::RunTest(const FString&)
{
	const int32 Nudo = ExpTurnDamageTaken(*this, 0, false);
	const int32 Esposto = ExpTurnDamageTaken(*this, 0, true);
	const int32 EspostoConDEF = ExpTurnDamageTaken(*this, 25, true);
	if (Nudo < 0 || Esposto < 0 || EspostoConDEF < 0) { return false; }

	if (!TestTrue(TEXT("premessa: l'attacco base fa danno"), Nudo > 0)) { return false; }
	TestTrue(TEXT("Exposed fa perdere piu' HP"), Esposto > Nudo);

	const int32 ColpoPieno = Nudo + URTCombatLibrary::BaseShield;
	TestEqual(TEXT("floor(P × 1,2) − S"), Esposto,
		URTCombatLibrary::MitigateDirectDamage(ColpoPieno, 0, URTCombatLibrary::ExposedModPercent, 0) - URTCombatLibrary::BaseShield);
	TestEqual(TEXT("con DEF 25: floor(P × 1,2 × 100/125) − S (il cablaggio; l'ordine lo pinna il caso 111 → 106)"), EspostoConDEF,
		URTCombatLibrary::MitigateDirectDamage(ColpoPieno, 0, URTCombatLibrary::ExposedModPercent, 25) - URTCombatLibrary::BaseShield);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
