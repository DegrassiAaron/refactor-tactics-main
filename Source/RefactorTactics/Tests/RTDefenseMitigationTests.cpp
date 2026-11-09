#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
// Il test di integrazione passa da un TURNO vero: la DEF iniettata sull'`ARTUnit` deve arrivare allo snapshot
// del Blast (`RTTurnManager_Blast.cpp`) e da li' a `CollectHexAttacks`. Misurarla sul piano puro non lo proverebbe.
#include "Ability/RTHeroCatalogLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * E53 CP 53.2c — la DEF e il bonus alle spalle ENTRANO nel colpo ([D-589], [D-587], [D-595], [D-599]).
 *
 * `Danno = max(0, (Base + Spalle) × Mod × 100/(100+DEF) − Copertura − Guard)`: qui si prova il pezzo che
 * `CollectHexAttacks` possiede — spalle e DEF PRIMA della copertura — con valori INIETTATI sulle unita' del piano
 * e, in un caso, sull'`ARTUnit` di un turno intero. Il catalogo e' ancora a zero su tutti e quattro i campi
 * (CP 53.2b): nessun numero di gioco cambia con questo passo, e il controllo `ZeroDefenseAndRearLeaveHitsUntouched`
 * pinna proprio questo. I valori usati (210, 300, DEF 25, spalle 90) sono quelli della tabella di controllo di
 * [D-595] e degli esempi di [D-599]: `floor(300 × 100/125) = 240`, `floor(210 × 100/125) = 168`.
 *
 * ⚠️ **Perche' non basta `Combat.Formula.*`**: quei sette test provano la funzione pura. Questi provano che la
 * funzione sia CHIAMATA, con la DEF del bersaglio giusto e il bonus dell'attaccante giusto, dall'origine giusta,
 * nell'ordine giusto rispetto alla copertura. Un chiamante che passasse la DEF dell'attaccante, o che mitigasse
 * dopo la copertura, lascerebbe `Combat.Formula.*` verde.
 */

namespace
{
	/** Arena piena di raggio N. Nome distinto per file: la unity build condivide la translation unit. */
	URTHexMapAsset* MakeDefMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	void SetDefLowCover(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	FRTHexCombatUnit DefUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing,
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

	/** La mira si dichiara come CELLA ([D-415]); `TargetId` puo' essere `INDEX_NONE` per un'area su una cella vuota. */
	FRTHexAttackIntent DefIntent(const FRTCellId& AimCell, int32 AttackerId, int32 TargetId, int32 Power,
		ERTAbilityShape Shape = ERTAbilityShape::Single, int32 AreaRadius = 0)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.TargetCell = AimCell;
		I.Shape = Shape;
		I.RangeCells = 5;
		I.AreaRadius = AreaRadius;
		I.Power = Power;
		I.bCountsAsAttack = true;
		return I;
	}

	const FRTHexAttackHit* DefHitOn(const FRTHexBlastPlan& Plan, int32 TargetId)
	{
		for (const FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.TargetId == TargetId) { return &Hit; }
		}
		return nullptr;
	}

	int32 DefPowerOn(const FRTHexBlastPlan& Plan, int32 TargetId)
	{
		const FRTHexAttackHit* Hit = DefHitOn(Plan, TargetId);
		return Hit ? Hit->Power : -1;
	}

	ARTUnit* DefSpawnUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
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
	 * HP persi dal difensore in UN turno intero, cambiando SOLO la DEF iniettata su di lui (non dal catalogo:
	 * `ConfigureFromHeroData` l'ha appena messa a zero, e la si sovrascrive dopo).
	 *
	 * Attaccante a OVEST del difensore, che GUARDA a ovest: colpo frontale, nessuna copertura (arena piena),
	 * nessuno stato. ⚠️ Lo scudo NON e' zero: a inizio turno `ARTUnit::RechargeBaseShield` arma
	 * `URTCombatLibrary::BaseShield` su ogni unita' viva, e lo scudo assorbe prima degli HP. Quindi con DEF 0 gli
	 * HP persi sono `P − BaseShield`, e con DEF 25 sono `floor(P × 100/125) − BaseShield`: il chiamante ricompone
	 * `P` sommando lo scudo. (La prima stesura diceva «scudo a zero» e il test e' caduto per 11 contro 12:
	 * l'attacco base vale 20 in questo banco, 15 e 11 sono i due HP persi.)
	 *
	 * `-1` = montaggio fallito, gia' segnalato da `Test`.
	 */
	int32 DefTurnDamageTaken(FAutomationTestBase& Test, int32 DefenderDefense)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), World)) { return -1; }

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakeDefMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();

		ARTUnit* Attaccante = DefSpawnUnit(World, /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
		ARTUnit* Difensore = DefSpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0));

		if (!Test.TestNotNull(TEXT("mappa"), MapActor) || !Test.TestNotNull(TEXT("turn manager"), TM)
			|| !Test.TestNotNull(TEXT("attaccante"), Attaccante) || !Test.TestNotNull(TEXT("difensore"), Difensore))
		{
			RTWorldFixtures::DestroyWorld(World);
			return -1;
		}

		Difensore->Facing = ERTHexDirection::W; // guarda l'attaccante: nessun bonus alle spalle in gioco
		Difensore->Defense = DefenderDefense;   // INIETTATA: il catalogo la tiene a zero (CP 53.2b)
		Difensore->PlannedAbilityIndex = INDEX_NONE;

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatDefenseMitigatesBeforeCoverTest,
	"RefactorTactics.Combat.DefenseMitigatesBeforeCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatDefenseMitigatesBeforeCoverTest::RunTest(const FString&)
{
	// Attaccante a ovest, bersaglio in (2,0) che lo GUARDA (copertura e nessun bonus alle spalle), DEF 25, colpo 300.
	const FRTCellId Defender(2, 0, 0);
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(DefIntent(Defender, 0, 1, 300));

	TArray<FRTHexCombatUnit> Units;
	Units.Add(DefUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
	Units.Add(DefUnit(1, 1, Defender, ERTHexDirection::W, /*Defense*/ 25));

	// Senza copertura: la sola DEF. L'esempio 1 della tabella di D-595: 300 contro 25 fa 240.
	{
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeDefMap(3));
		TestEqual(TEXT("300 contro DEF 25, senza copertura: floor(300 × 100/125) = 240"), DefPowerOn(Plan, 1), 240);
		const FRTHexAttackHit* Hit = DefHitOn(Plan, 1);
		if (TestNotNull(TEXT("il colpo c'e'"), Hit))
		{
			TestEqual(TEXT("il colpo porta il valore mitigato"), Hit->MitigatedPower, 240);
			TestEqual(TEXT("e la DEF letta"), Hit->DefenseApplied, 25);
			TestEqual(TEXT("frontale: nessun bonus alle spalle"), Hit->RearBonusApplied, 0);
			TestEqual(TEXT("il nominale resta il colpo dichiarato"), Hit->NominalPower, 300);
		}
	}

	// Con copertura bassa sul bordo attraversato: PRIMA la difesa, POI la copertura piatta (D-589 (3)).
	// Se la copertura venisse sottratta prima, verrebbe floor(290 × 100/125) = 232, non 230.
	{
		URTHexMapAsset* Map = MakeDefMap(3);
		SetDefLowCover(Map, Defender, ERTHexDirection::W);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(TEXT("difesa prima, copertura dopo: 240 − copertura"),
			DefPowerOn(Plan, 1), 240 - URTCombatLibrary::LowCoverDamageReduction);
		TestNotEqual(TEXT("non e' la copertura prima della difesa"), DefPowerOn(Plan, 1),
			URTCombatLibrary::MitigateDirectDamage(300 - URTCombatLibrary::LowCoverDamageReduction, 0, 100, 25));
	}

	// Controllo positivo del banco: la stessa scena con DEF 0 fa cio' che faceva ieri.
	{
		TArray<FRTHexCombatUnit> Zero = Units;
		Zero[1].Defense = 0;
		URTHexMapAsset* Map = MakeDefMap(3);
		SetDefLowCover(Map, Defender, ERTHexDirection::W);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Zero, Intents, Map);
		TestEqual(TEXT("DEF 0: 300 − copertura, come prima di CP 53.2c"),
			DefPowerOn(Plan, 1), 300 - URTCombatLibrary::LowCoverDamageReduction);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatRearBonusIsTheAttackersAndAddsToBaseTest,
	"RefactorTactics.Combat.RearBonusIsTheAttackersAndAddsToBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatRearBonusIsTheAttackersAndAddsToBaseTest::RunTest(const FString&)
{
	// L'asserto che D-599 nomina. Attaccante a ovest con spalle 90 (la fascia di Kallari), bersaglio in (0,0)
	// con DEF 25 e un proprio bonus assurdo (999) che NON deve contare: il bonus e' di chi tira (D-587 (2)).
	const FRTCellId Defender(0, 0, 0);
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(DefIntent(Defender, 0, 1, 210)); // Mist Slash della tabella di D-599

	// Da' le spalle all'attaccante (guarda a E, l'attaccante e' a W): (210 + 90) × 100/125 = 240.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E, /*Defense*/ 0, /*RearBonus*/ 90));
		Units.Add(DefUnit(1, 1, Defender, ERTHexDirection::E, /*Defense*/ 25, /*RearBonus*/ 999));
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeDefMap(3));
		TestEqual(TEXT("alle spalle: (210 + 90) contro DEF 25 = 240"), DefPowerOn(Plan, 1), 240);
		const FRTHexAttackHit* Hit = DefHitOn(Plan, 1);
		if (TestNotNull(TEXT("il colpo c'e'"), Hit))
		{
			TestEqual(TEXT("il bonus applicato e' quello dell'attaccante, 90"), Hit->RearBonusApplied, 90);
		}
	}

	// Lo guarda: stesso colpo, stessa DEF, nessun bonus: 210 × 100/125 = 168.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E, 0, 90));
		Units.Add(DefUnit(1, 1, Defender, ERTHexDirection::W, 25, 999));
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeDefMap(3));
		TestEqual(TEXT("di fronte: 210 contro DEF 25 = 168"), DefPowerOn(Plan, 1), 168);
		const FRTHexAttackHit* Hit = DefHitOn(Plan, 1);
		if (TestNotNull(TEXT("il colpo c'e'"), Hit)) { TestEqual(TEXT("nessun bonus"), Hit->RearBonusApplied, 0); }
	}

	// Controfattuale: il bonus sul BERSAGLIO e zero sull'attaccante, alle spalle. Se la funzione leggesse il campo
	// sbagliato darebbe 240; deve dare 168.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E, 0, 0));
		Units.Add(DefUnit(1, 1, Defender, ERTHexDirection::E, 25, 90));
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeDefMap(3));
		TestEqual(TEXT("il bonus del bersaglio non e' di nessuno: 168"), DefPowerOn(Plan, 1), 168);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatZeroDefenseAndRearLeaveHitsUntouchedTest,
	"RefactorTactics.Combat.ZeroDefenseAndRearLeaveHitsUntouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatZeroDefenseAndRearLeaveHitsUntouchedTest::RunTest(const FString&)
{
	// Il CONTROLLO del passo: con i quattro campi a zero — il roster di oggi — colpo e breakdown sono quelli di
	// prima di CP 53.2c, voce per voce. E' la forma eseguibile del gate «golden byte-identici».
	const FRTCellId Defender(2, 0, 0);
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(DefIntent(Defender, 0, 1, 300));

	// Frontale con copertura: 300 − copertura, due stadi (catalogo, copertura).
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(DefUnit(1, 1, Defender, ERTHexDirection::W));
		URTHexMapAsset* Map = MakeDefMap(3);
		SetDefLowCover(Map, Defender, ERTHexDirection::W);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(TEXT("300 − copertura"), DefPowerOn(Plan, 1), 300 - URTCombatLibrary::LowCoverDamageReduction);
		const FRTHexAttackHit* Hit = DefHitOn(Plan, 1);
		if (TestNotNull(TEXT("il colpo c'e'"), Hit))
		{
			TestEqual(TEXT("mitigato == nominale"), Hit->MitigatedPower, Hit->NominalPower);
			TestEqual(TEXT("nessuna DEF letta"), Hit->DefenseApplied, 0);
			TestEqual(TEXT("nessun bonus"), Hit->RearBonusApplied, 0);
		}

		const TArray<FRTAttack> Attacks = URTHexCombatLibrary::ToAttacks(Plan);
		if (TestEqual(TEXT("un attacco"), Attacks.Num(), 1))
		{
			const TArray<FRTDamageStageEntry>& Stages = Attacks[0].Breakdown;
			TestEqual(TEXT("due stadi: catalogo e copertura, nessuno nuovo"), Stages.Num(), 2);
			for (const FRTDamageStageEntry& E : Stages)
			{
				TestTrue(TEXT("nessuno stadio RearBonus/Defense con i campi a zero"),
					E.Stage != ERTDamageStage::RearBonus && E.Stage != ERTDamageStage::Defense);
			}
			if (Stages.Num() == 2)
			{
				TestEqual(TEXT("la copertura parte dal nominale, come prima"), Stages[1].Before, 300);
				TestEqual(TEXT("e arriva al colpo"), Stages[1].After, 300 - URTCombatLibrary::LowCoverDamageReduction);
			}
		}
	}

	// Alle spalle senza bonus: la copertura cade (CP 16.2) e niente si aggiunge: 30 pieno.
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(DefUnit(1, 1, Defender, ERTHexDirection::E));
		URTHexMapAsset* Map = MakeDefMap(3);
		SetDefLowCover(Map, Defender, ERTHexDirection::W);
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		TestEqual(TEXT("alle spalle senza bonus: il colpo pieno, non di piu'"), DefPowerOn(Plan, 1), 300);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBreakdownRecordsRearAndDefenseBeforeCoverTest,
	"RefactorTactics.Damage.BreakdownRecordsRearAndDefenseBeforeCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBreakdownRecordsRearAndDefenseBeforeCoverTest::RunTest(const FString&)
{
	// Caso frontale con copertura: Catalog 300 → Defense (Mitigate 25) 240 → Cover 230. Tre stadi, in quest'ordine.
	{
		const FRTCellId Defender(2, 0, 0);
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(DefIntent(Defender, 0, 1, 300));
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
		Units.Add(DefUnit(1, 1, Defender, ERTHexDirection::W, /*Defense*/ 25));
		URTHexMapAsset* Map = MakeDefMap(3);
		SetDefLowCover(Map, Defender, ERTHexDirection::W);

		const TArray<FRTAttack> Attacks = URTHexCombatLibrary::ToAttacks(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map));
		if (!TestEqual(TEXT("un attacco"), Attacks.Num(), 1)) { return false; }
		const TArray<FRTDamageStageEntry>& S = Attacks[0].Breakdown;
		if (!TestEqual(TEXT("tre stadi: catalogo, difesa, copertura"), S.Num(), 3)) { return false; }

		TestTrue(TEXT("[0] catalogo"), S[0].Stage == ERTDamageStage::Catalog);
		TestEqual(TEXT("[0] 300"), S[0].After, 300);

		TestTrue(TEXT("[1] difesa"), S[1].Stage == ERTDamageStage::Defense);
		TestTrue(TEXT("[1] operazione Mitigate"), S[1].Operation == ERTDamageOp::Mitigate);
		TestEqual(TEXT("[1] operando = DEF"), S[1].Operand, 25);
		TestEqual(TEXT("[1] prima 300"), S[1].Before, 300);
		TestEqual(TEXT("[1] dopo 240"), S[1].After, 240);
		TestEqual(TEXT("[1] provenienza D-589"), S[1].SourceId, URTCombatLibrary::DefenseSource);

		TestTrue(TEXT("[2] copertura"), S[2].Stage == ERTDamageStage::Cover);
		TestEqual(TEXT("[2] parte dal mitigato, non dal nominale"), S[2].Before, 240);
		TestEqual(TEXT("[2] arriva al colpo"), S[2].After, 240 - URTCombatLibrary::LowCoverDamageReduction);
	}

	// Caso alle spalle: Catalog 210 → RearBonus (+90) 300 → Defense 240; la copertura non c'e' (alle spalle cade).
	{
		const FRTCellId Defender(0, 0, 0);
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(DefIntent(Defender, 0, 1, 210));
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E, 0, /*RearBonus*/ 90));
		Units.Add(DefUnit(1, 1, Defender, ERTHexDirection::E, /*Defense*/ 25));

		const TArray<FRTAttack> Attacks = URTHexCombatLibrary::ToAttacks(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeDefMap(3)));
		if (!TestEqual(TEXT("un attacco"), Attacks.Num(), 1)) { return false; }
		const TArray<FRTDamageStageEntry>& S = Attacks[0].Breakdown;
		if (!TestEqual(TEXT("tre stadi: catalogo, spalle, difesa"), S.Num(), 3)) { return false; }

		TestTrue(TEXT("[0] catalogo"), S[0].Stage == ERTDamageStage::Catalog);
		TestTrue(TEXT("[1] spalle"), S[1].Stage == ERTDamageStage::RearBonus);
		TestTrue(TEXT("[1] somma"), S[1].Operation == ERTDamageOp::Add);
		TestEqual(TEXT("[1] +90"), S[1].Operand, 90);
		TestEqual(TEXT("[1] 210 → 300"), S[1].After, 300);
		TestEqual(TEXT("[1] provenienza D-587"), S[1].SourceId, URTCombatLibrary::RearBonusSource);
		TestTrue(TEXT("[2] difesa dopo le spalle: il bonus passa dalla DEF (D-587 (3))"), S[2].Stage == ERTDamageStage::Defense);
		TestEqual(TEXT("[2] 300 → 240"), S[2].After, 240);
		TestEqual(TEXT("il valore finale del registro e' il colpo"), S[2].After, Attacks[0].Power);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatRedirectedHitUsesTheInterceptorsDefenseTest,
	"RefactorTactics.Combat.RedirectedHitUsesTheInterceptorsDefense",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatRedirectedHitUsesTheInterceptorsDefenseTest::RunTest(const FString&)
{
	// `Action.Intercept` (D-017) riassegna il colpo: la DEF e' TARGET-DEPENDENT come la copertura, quindi deve essere
	// quella dell'intercettore. Bersaglio originale senza DEF, intercettore in mezzo (1,0) con DEF 25 che guarda
	// l'attaccante: 300 → 240.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(DefUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
	Units.Add(DefUnit(1, 1, FRTCellId(2, 0, 0), ERTHexDirection::W));
	Units.Add(DefUnit(2, 1, FRTCellId(1, 0, 0), ERTHexDirection::W, /*Defense*/ 25));
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(DefIntent(FRTCellId(2, 0, 0), 0, 1, 300));
	URTHexMapAsset* Map = MakeDefMap(3);

	// Il colpo nasce sul bersaglio originale, dove non c'e' DEF. ⚠️ L'intercettore sta sulla linea di tiro: se la
	// geometria lo considerasse un ostacolo il piano non avrebbe colpi, e il test lo direbbe qui.
	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
	const FRTHexAttackHit* Original = DefHitOn(Plan, 1);
	if (!TestNotNull(TEXT("premessa: il colpo sul bersaglio originale esiste"), Original)) { return false; }
	TestEqual(TEXT("premessa: senza DEF il colpo e' pieno"), Original->Power, 300);

	const FRTHexAttackHit Redirected = URTHexCombatLibrary::RedirectHitTo(2, *Original, Units, Intents, Map);
	TestEqual(TEXT("riassegnato all'intercettore"), Redirected.TargetId, 2);
	TestEqual(TEXT("con la DEF dell'intercettore: 300 contro 25 = 240"), Redirected.Power, 240);
	TestEqual(TEXT("la DEF letta e' la sua"), Redirected.DefenseApplied, 25);
	TestEqual(TEXT("il mitigato e' coerente"), Redirected.MitigatedPower, 240);
	TestEqual(TEXT("di fronte: nessun bonus"), Redirected.RearBonusApplied, 0);
	TestEqual(TEXT("il nominale e' quello dell'intento"), Redirected.NominalPower, 300);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatAreaRearBonusUsesImpactCenterTest,
	"RefactorTactics.Combat.AreaRearBonusUsesImpactCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatAreaRearBonusUsesImpactCenterTest::RunTest(const FString&)
{
	// Stessa regola della Guardia (`AreaGuardIsBypassedWhenImpactCenterIsBehind`): per un'AREA conta il centro
	// d'impatto, non la cella del lanciatore. Lanciatore a (-2,0) con spalle 90, centro in (1,0), difensore in (0,0)
	// che GUARDA il lanciatore (W): il centro gli sta dietro, il bonus scatta. Se l'origine fosse il lanciatore
	// il colpo sarebbe frontale e il bonus no.
	const FRTCellId Lanciatore(-2, 0, 0);
	const FRTCellId Centro(1, 0, 0);
	const FRTCellId Difensore(0, 0, 0);
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(DefIntent(Centro, 0, INDEX_NONE, 210, ERTAbilityShape::Area, /*AreaRadius*/ 1));

	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, Lanciatore, ERTHexDirection::E, 0, /*RearBonus*/ 90));
		Units.Add(DefUnit(1, 1, Difensore, ERTHexDirection::W));
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeDefMap(3));
		if (!TestTrue(TEXT("l'area investe il difensore"), DefPowerOn(Plan, 1) >= 0)) { return false; }
		TestEqual(TEXT("centro alle spalle, lanciatore di fronte: il bonus scatta, 210 + 90"), DefPowerOn(Plan, 1), 300);
	}
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(DefUnit(0, 0, Lanciatore, ERTHexDirection::E, 0, 90));
		Units.Add(DefUnit(1, 1, Difensore, ERTHexDirection::E)); // guarda il centro: frontale
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeDefMap(3));
		TestEqual(TEXT("centro di fronte, lanciatore alle spalle: nessun bonus, 210"), DefPowerOn(Plan, 1), 210);
	}

	// La regola e' una funzione sola, e si legge: area → centro, altrimenti chi tira.
	TestTrue(TEXT("ImpactOriginOf: area → centro"),
		URTHexCombatLibrary::ImpactOriginOf(ERTAbilityShape::Area, Lanciatore, Centro) == Centro);
	TestTrue(TEXT("ImpactOriginOf: singolo → attaccante"),
		URTHexCombatLibrary::ImpactOriginOf(ERTAbilityShape::Single, Lanciatore, Centro) == Lanciatore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatInjectedDefenseReducesDamageInAFullTurnTest,
	"RefactorTactics.Combat.InjectedDefenseReducesDamageInAFullTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatInjectedDefenseReducesDamageInAFullTurnTest::RunTest(const FString&)
{
	// Il gate che il piano di E53 chiede per CP 53.2c: una DEF iniettata sul bersaglio (non dal catalogo) riduce
	// gli HP persi in un turno intero. Prova la plumbing `ARTUnit → snapshot del Blast → CollectHexAttacks`, che
	// nessun test sul piano puro puo' vedere.
	const int32 ConZero = DefTurnDamageTaken(*this, 0);
	const int32 ConDEF25 = DefTurnDamageTaken(*this, 25);
	if (ConZero < 0 || ConDEF25 < 0) { return false; }

	if (!TestTrue(TEXT("premessa: con DEF 0 l'attacco base fa danno"), ConZero > 0)) { return false; }
	TestTrue(TEXT("la DEF iniettata riduce gli HP persi"), ConDEF25 < ConZero);

	// Il colpo pieno e' HP persi + scudo base (assorbito prima degli HP); la DEF lo mitiga PRIMA dello scudo,
	// come prima della copertura: `floor((HP + S) × 100/125) − S`.
	const int32 ColpoPieno = ConZero + URTCombatLibrary::BaseShield;
	TestEqual(TEXT("ed e' la formula di D-589 sul colpo pieno, al netto dello scudo base: floor((HP + S) × 100/125) − S"),
		ConDEF25, URTCombatLibrary::MitigateDirectDamage(ColpoPieno, 0, 100, 25) - URTCombatLibrary::BaseShield);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
