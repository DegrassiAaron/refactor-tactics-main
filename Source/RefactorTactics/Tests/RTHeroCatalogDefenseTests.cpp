// E53 CP 53.3c (#4013): i VALORI di DEF, RES e bonus alle spalle entrano nel catalogo eroi ([D-595] (3), [D-599]),
// e la tabella di controllo di CP 53.1 smette di essere un foglio: qui si misura sul catalogo REALE, con le unita'
// del piano costruite da `URTHeroData` e con un turno intero su `ARTUnit` configurate dal catalogo, senza iniettare
// nulla. I file gemelli: `RTDefenseMitigationTests.cpp` prova il CHIAMANTE con valori iniettati (CP 53.2c),
// `RTHeroDefenseFieldsTests.cpp` pinna i numeri del catalogo campo per campo (CP 53.2b → 53.3c).
//
// ⚠️ Perche' non basta `Combat.Formula.*` ne' `RTDefenseMitigationTests.cpp`: quelli provano la funzione e il
// chiamante con numeri scelti a mano. Questi provano che i numeri che il GIOCO applica siano quelli del catalogo —
// un catalogo che dichiarasse 15 e un'unita' che ne copiasse 0 lascerebbe entrambi verdi.

#include "Misc/AutomationTest.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Arena piena di raggio N. Nome distinto per file: la unity build condivide la translation unit. */
	URTHexMapAsset* MakeCatDefMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	/** Unita' del piano costruita DAL CATALOGO: DEF e bonus alle spalle sono quelli di `URTHeroData`, non un numero scelto qui. */
	FRTHexCombatUnit CatDefUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing, const URTHeroData* Hero)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		U.Facing = Facing;
		U.Defense = Hero ? Hero->Defense : 0;
		U.RearBonus = Hero ? Hero->RearBonus : 0;
		return U;
	}

	FRTHexAttackIntent CatDefIntent(const FRTCellId& AimCell, int32 AttackerId, int32 TargetId, int32 Power)
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

	const FRTHexAttackHit* CatDefHitOn(const FRTHexBlastPlan& Plan, int32 TargetId)
	{
		for (const FRTHexAttackHit& Hit : Plan.Hits)
		{
			if (Hit.TargetId == TargetId) { return &Hit; }
		}
		return nullptr;
	}

	/** Il danno dell'attacco base di un eroe, LETTO dal catalogo: `Actions[0]` e' l'attacco base per i quattro (ADR-0007). */
	int32 CatBasicAttackDamage(const URTHeroData* Hero)
	{
		if (!Hero || Hero->Actions.Num() == 0 || !Hero->Actions[0]) { return 0; }
		return URTCatalogLibrary::FirstDamage(Hero->Actions[0]->Def);
	}

	ARTUnit* CatDefSpawnUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, URTHeroData* Hero)
	{
		if (!World || !Hero) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi
		U->ConfigureFromHeroData(Hero); // DEF, RES e spalle arrivano da qui, e da nessun'altra parte
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * HP persi dal difensore in UN turno intero fra due eroi del catalogo, SENZA iniettare nulla. L'attaccante sta a
	 * OVEST del difensore e tira l'attacco base; il difensore guarda a ovest (colpo frontale) o a est (`bFromBehind`:
	 * il colpo arriva dal lato posteriore, [D-126], e il bonus alle spalle dell'attaccante scatta). Arena piena:
	 * nessuna copertura, nessuno stato, nessuna reazione armata (`PlannedAbilityIndex` del difensore a `INDEX_NONE`).
	 *
	 * ⚠️ Lo scudo NON e' zero: a inizio turno `ARTUnit::RechargeBaseShield` arma `URTCombatLibrary::BaseShield`, che
	 * assorbe prima degli HP. Il chiamante lo somma per confrontare col colpo mitigato.
	 *
	 * `-1` = montaggio fallito, gia' segnalato da `Test`.
	 */
	int32 CatTurnDamageTaken(FAutomationTestBase& Test, URTHeroData* AttackerHero, URTHeroData* DefenderHero, bool bFromBehind)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), World)) { return -1; }

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakeCatDefMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();

		ARTUnit* Attaccante = CatDefSpawnUnit(World, /*TeamId=*/ 0, FRTCellId(-1, 0, 0), AttackerHero);
		ARTUnit* Difensore = CatDefSpawnUnit(World, /*TeamId=*/ 1, FRTCellId(0, 0, 0), DefenderHero);

		if (!Test.TestNotNull(TEXT("mappa"), MapActor) || !Test.TestNotNull(TEXT("turn manager"), TM)
			|| !Test.TestNotNull(TEXT("attaccante"), Attaccante) || !Test.TestNotNull(TEXT("difensore"), Difensore))
		{
			RTWorldFixtures::DestroyWorld(World);
			return -1;
		}

		Difensore->Facing = bFromBehind ? ERTHexDirection::E : ERTHexDirection::W;
		Difensore->PlannedAbilityIndex = INDEX_NONE;

		Attaccante->Facing = ERTHexDirection::E; // guarda il bersaglio
		Attaccante->PlannedAbilityIndex = 0;     // attacco base
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

	struct FCatTableRow
	{
		const TCHAR* Label;
		int32 Power;      // letto dal catalogo, verificato contro la colonna «×10» della tabella
		int32 AtDef15;    // le tre colonne della tabella di CP 53.1 §3 che il roster di oggi esercita
		int32 AtDef25;
		int32 AtDef45;
	};
}

/**
 * La tabella di controllo di CP 53.1 §3 sul catalogo REALE: per ogni eroe del roster (DEF 15, 25, 45 dal CP 53.3c) e
 * per ogni colpo diretto del catalogo, il colpo frontale raccolto da `CollectHexAttacks` vale la cella della tabella
 * ([D-595] (1): per difetto, una volta sola). Le potenze NON sono ricopiate: si leggono dal catalogo azioni e dal kit
 * di Ivrin, e il test cade se il catalogo e la tabella divergono — in entrambe le direzioni.
 *
 * `Action.Counter` della tabella non e' qui: e' una reazione e il suo danno viaggia su un altro tracciato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroesControlTableHoldsOnTheRealCatalogTest,
	"RefactorTactics.Heroes.ControlTableHoldsOnTheRealCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroesControlTableHoldsOnTheRealCatalogTest::RunTest(const FString&)
{
	URTHeroData* Ivrin = URTHeroCatalogLibrary::MakeIvrin();
	const TArray<FCatTableRow> Rows = {
		{ TEXT("Action.Mortar"),          URTCatalogLibrary::FirstDamage(URTCatalogLibrary::FindCoreAction(TEXT("Action.Mortar"))),          104, 96, 82 },
		{ TEXT("Action.CircularAoE"),     URTCatalogLibrary::FirstDamage(URTCatalogLibrary::FindCoreAction(TEXT("Action.CircularAoE"))),     156, 144, 124 },
		{ TEXT("Action.Charge"),          URTCatalogLibrary::FirstDamage(URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"))),          173, 160, 137 },
		{ TEXT("Action.LineAttack"),      URTCatalogLibrary::FirstDamage(URTCatalogLibrary::FindCoreAction(TEXT("Action.LineAttack"))),      191, 176, 151 },
		{ TEXT("Action.PrecisionAttack"), URTCatalogLibrary::FirstDamage(URTCatalogLibrary::FindCoreAction(TEXT("Action.PrecisionAttack"))), 208, 192, 165 },
		{ TEXT("Action.HeavyAttack"),     URTCatalogLibrary::FirstDamage(URTCatalogLibrary::FindCoreAction(TEXT("Action.HeavyAttack"))),     304, 280, 241 },
		{ TEXT("Hero.Ivrin.PulseShot"),   CatBasicAttackDamage(Ivrin),                                                                       182, 168, 144 },
	};
	// La colonna «×10» della tabella: se il catalogo si muove, la tabella e' da riscrivere, non da ignorare.
	const TMap<FString, int32> TablePower = {
		{ TEXT("Action.Mortar"), 120 }, { TEXT("Action.CircularAoE"), 180 }, { TEXT("Action.Charge"), 200 },
		{ TEXT("Action.LineAttack"), 220 }, { TEXT("Action.PrecisionAttack"), 240 }, { TEXT("Action.HeavyAttack"), 350 },
		{ TEXT("Hero.Ivrin.PulseShot"), 210 },
	};

	const FRTCellId Defender(0, 0, 0);
	const FRTCellId Attacker(-1, 0, 0);
	URTHexMapAsset* Map = MakeCatDefMap(3);

	for (URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		const FString Who = Hero->HeroId.ToString();
		int32 Column = -1;
		switch (Hero->Defense)
		{
			case 15: Column = 0; break;
			case 25: Column = 1; break;
			case 45: Column = 2; break;
			default: break;
		}
		if (!TestTrue(*FString::Printf(TEXT("%s: la DEF %d e' una colonna della tabella (15, 25, 45)"), *Who, Hero->Defense), Column >= 0))
		{
			continue;
		}

		for (const FCatTableRow& Row : Rows)
		{
			if (!TestEqual(*FString::Printf(TEXT("%s: il catalogo dichiara la potenza della tabella"), Row.Label), Row.Power, TablePower[Row.Label]))
			{
				continue;
			}
			TArray<FRTHexAttackIntent> Intents;
			Intents.Add(CatDefIntent(Defender, 0, 1, Row.Power));
			TArray<FRTHexCombatUnit> Units;
			Units.Add(CatDefUnit(0, 0, Attacker, ERTHexDirection::E, Ivrin));
			Units.Add(CatDefUnit(1, 1, Defender, ERTHexDirection::W, Hero)); // guarda l'attaccante: frontale, nessun bonus

			const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
			const FRTHexAttackHit* Hit = CatDefHitOn(Plan, 1);
			if (!TestNotNull(*FString::Printf(TEXT("%s contro %s: il colpo c'e'"), Row.Label, *Who), Hit)) { continue; }

			const int32 Expected = Column == 0 ? Row.AtDef15 : Column == 1 ? Row.AtDef25 : Row.AtDef45;
			TestEqual(*FString::Printf(TEXT("%s (%d) contro %s (DEF %d): la cella della tabella, %d"), Row.Label, Row.Power, *Who, Hero->Defense, Expected),
				Hit->Power, Expected);
			TestEqual(*FString::Printf(TEXT("%s contro %s: la DEF letta e' quella del catalogo"), Row.Label, *Who), Hit->DefenseApplied, Hero->Defense);
			TestEqual(*FString::Printf(TEXT("%s contro %s: frontale, nessun bonus"), Row.Label, *Who), Hit->RearBonusApplied, 0);
		}
	}
	return true;
}

/**
 * Il turno intero sul catalogo, senza iniezioni: Ivrin tira `PulseShot` (210) a ciascuno dei quattro di fronte e gli HP
 * persi sono `floor(210 × 100/(100+DEF)) − scudo base`: Aevik e Ivrin (DEF 15) 182 − 50 = 132, Muiren (DEF 25)
 * 168 − 50 = 118, Branth (DEF 45) 144 − 50 = 94. Poi l'esempio di [D-599] alla lettera: il colpo alle spalle di
 * Kallari (Ivrin, spalle 90) su Muriel (Muiren, DEF 25) vale `floor(300 × 100/125) = 240` contro 168 di fronte —
 * qui al netto dello scudo, 190 contro 118 — e quello del tank (Branth, spalle 30) si somma alla SUA base.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatCatalogDefenseAndRearBonusInAFullTurnTest,
	"RefactorTactics.Combat.CatalogDefenseAndRearBonusInAFullTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatCatalogDefenseAndRearBonusInAFullTurnTest::RunTest(const FString&)
{
	const int32 Shield = URTCombatLibrary::BaseShield;

	// Frontale: i quattro bersagli, la DEF del catalogo di ciascuno.
	{
		const int32 PulseShot = CatBasicAttackDamage(URTHeroCatalogLibrary::MakeIvrin());
		if (!TestEqual(TEXT("premessa: PulseShot vale 210 nel catalogo"), PulseShot, 210)) { return false; }

		struct FCase { const TCHAR* Who; URTHeroData* (*Make)(); int32 Def; int32 HpLost; };
		const TArray<FCase> Cases = {
			{ TEXT("Aevik"),  &URTHeroCatalogLibrary::MakeAevik,  15, 132 },
			{ TEXT("Muiren"), &URTHeroCatalogLibrary::MakeMuiren, 25, 118 },
			{ TEXT("Branth"), &URTHeroCatalogLibrary::MakeBranth, 45, 94 },
			{ TEXT("Ivrin"),  &URTHeroCatalogLibrary::MakeIvrin,  15, 132 },
		};
		for (const FCase& C : Cases)
		{
			URTHeroData* Defender = C.Make();
			TestEqual(*FString::Printf(TEXT("%s: la DEF del catalogo"), C.Who), Defender->Defense, C.Def);
			const int32 Taken = CatTurnDamageTaken(*this, URTHeroCatalogLibrary::MakeIvrin(), Defender, /*bFromBehind*/ false);
			if (Taken < 0) { return false; }
			TestEqual(*FString::Printf(TEXT("PulseShot 210 di fronte a %s (DEF %d): floor(210 × 100/%d) − %d = %d HP"), C.Who, C.Def, 100 + C.Def, Shield, C.HpLost),
				Taken, C.HpLost);
			TestEqual(*FString::Printf(TEXT("%s: e' la formula sul colpo del catalogo"), C.Who),
				Taken, URTCombatLibrary::MitigateDirectDamage(PulseShot, 0, 100, Defender->Defense) - Shield);
		}
	}

	// Alle spalle: l'esempio di D-599 (Kallari su Muriel), al netto dello scudo.
	{
		URTHeroData* Ivrin = URTHeroCatalogLibrary::MakeIvrin();
		TestEqual(TEXT("Ivrin porta la fascia flanker, 90"), Ivrin->RearBonus, 90);
		const int32 Behind = CatTurnDamageTaken(*this, Ivrin, URTHeroCatalogLibrary::MakeMuiren(), /*bFromBehind*/ true);
		const int32 Front = CatTurnDamageTaken(*this, URTHeroCatalogLibrary::MakeIvrin(), URTHeroCatalogLibrary::MakeMuiren(), /*bFromBehind*/ false);
		if (Behind < 0 || Front < 0) { return false; }
		TestEqual(TEXT("D-599: (210 + 90) contro DEF 25 = 240, meno lo scudo 50: 190 HP"), Behind, 240 - Shield);
		TestEqual(TEXT("D-599: di fronte 210 contro DEF 25 = 168, meno lo scudo: 118 HP"), Front, 168 - Shield);
	}

	// Il tank: Branth (spalle 30) alle spalle di Muiren con il SUO attacco base; il bonus e' dell'attaccante e si
	// somma alla sua base, non a quella di Ivrin.
	{
		URTHeroData* Branth = URTHeroCatalogLibrary::MakeBranth();
		TestEqual(TEXT("Branth porta la fascia tank, 30"), Branth->RearBonus, 30);
		const int32 ImpactShot = CatBasicAttackDamage(Branth);
		if (!TestTrue(TEXT("premessa: l'attacco base di Branth fa danno"), ImpactShot > Shield)) { return false; }
		const int32 Behind = CatTurnDamageTaken(*this, Branth, URTHeroCatalogLibrary::MakeMuiren(), /*bFromBehind*/ true);
		const int32 Front = CatTurnDamageTaken(*this, URTHeroCatalogLibrary::MakeBranth(), URTHeroCatalogLibrary::MakeMuiren(), /*bFromBehind*/ false);
		if (Behind < 0 || Front < 0) { return false; }
		TestEqual(*FString::Printf(TEXT("Branth alle spalle di Muiren: floor((%d + 30) × 100/125) − %d"), ImpactShot, Shield),
			Behind, URTCombatLibrary::MitigateDirectDamage(ImpactShot, 30, 100, 25) - Shield);
		TestEqual(*FString::Printf(TEXT("Branth di fronte a Muiren: floor(%d × 100/125) − %d"), ImpactShot, Shield),
			Front, URTCombatLibrary::MitigateDirectDamage(ImpactShot, 0, 100, 25) - Shield);
		TestTrue(TEXT("il colpo alle spalle del tank vale piu' di quello di fronte, ma meno di quello del flanker con la stessa base"),
			Behind > Front && URTCombatLibrary::MitigateDirectDamage(ImpactShot, 30, 100, 25) < URTCombatLibrary::MitigateDirectDamage(ImpactShot, 90, 100, 25));
	}
	return true;
}

/**
 * [D-602] (E53 CP 53.3c): copertura bassa 60, Guardia 100, Brace 60 — la taratura con la DEF in mano. I due default di
 * `GuardReduction` (`URTHeroData` e `ARTUnit`) sono LETTERALI, perche' quegli header non includono `Combat/`: questo
 * test e' cio' che impedisce loro di divergere dalla costante in silenzio. Ogni eroe del roster porta il default.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatFlatReductionsMatchD602Test,
	"RefactorTactics.Combat.FlatReductionsMatchD602",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatFlatReductionsMatchD602Test::RunTest(const FString&)
{
	TestEqual(TEXT("copertura bassa 60"), URTCombatLibrary::LowCoverDamageReduction, 60);
	TestEqual(TEXT("Guardia 100"), URTCombatLibrary::GuardReductionDefault, 100);
	TestEqual(TEXT("Brace 60"), URTCombatLibrary::BraceDamageReduction, 60);
	TestEqual(TEXT("il default di URTHeroData e' la costante"),
		GetDefault<URTHeroData>()->GuardReduction, URTCombatLibrary::GuardReductionDefault);
	TestEqual(TEXT("il default di ARTUnit e' la costante"),
		GetDefault<ARTUnit>()->GuardReduction, URTCombatLibrary::GuardReductionDefault);
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		TestEqual(*FString::Printf(TEXT("%s porta la Guardia di default"), *Hero->HeroId.ToString()),
			Hero->GuardReduction, URTCombatLibrary::GuardReductionDefault);
	}
	// La Guardia toglie piu' del Brace sull'arco frontale (D-408): la taratura conserva l'ordine.
	TestTrue(TEXT("Guardia > Brace"), URTCombatLibrary::GuardReductionDefault > URTCombatLibrary::BraceDamageReduction);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
