#include "Misc/AutomationTest.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapAsset.h"
#include "Misc/ScopeExit.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Log/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * #4189, fetta 1 di #4188 - lo scudo BASE ([D-224], 50) ferma solo la parte `Direct` del danno di un bersaglio: un colpo
 * elementale lo attraversa e paga solo la RES ([D-634] (5), «Solo il Direct, come oggi»).
 *
 * L'ordine e' quello del design (§5 punto 4): il temporaneo assorbe per primo, da qualunque parte, e delle due consuma
 * prima l'elementale; la base assorbe solo cio' che resta del `Direct`. Il banco distingue tre modelli:
 *   (a) quello scelto: temporaneo sull'elementale, poi sul `Direct`; base sul solo `Direct`;
 *   (b) temporaneo sul `Direct` per primo: la base resta senza niente da fermare e l'elementale passa intero;
 *   (c) quello di prima del #4189: tutto `Direct`, e la base ferma anche l'elementale.
 * Il caso misto del piano (base 50, temporaneo 30, 40 elementali e 60 `Direct`) separa (a) da (b), 20 contro 40 HP, ma non
 * da (c), che fa 20 anche lui. Per questo accanto c'e' un caso che separa tutti e tre: base 50, temporaneo 20, 100
 * elementali e 30 `Direct` fanno 80 HP in (a), 100 in (b), 60 in (c).
 *
 * I primi casi stanno sul resolver (`URTCombatResolver::ResolveAttacks`), che e' il punto in cui il turno somma i colpi; gli
 * ultimi due giocano un turno vero, con Gadget e Riktor del catalogo.
 */

namespace
{
	/** Nomi distinti per file: la unity build condivide la translation unit. */
	FName BsElectric() { return FName(TEXT("Element.Electric")); }

	/** Un colpo gia' pronto per il resolver; `NAME_None` = `Direct`. */
	FRTAttack BsAttack(int32 TargetIndex, int32 Power, const FName& ElementId)
	{
		FRTAttack A(TargetIndex, Power);
		A.ElementId = ElementId;
		return A;
	}

	/** Un bersaglio con lo scudo base del gioco e il temporaneo dato: `Shield` e' il totale, come su `ARTUnit`. */
	FRTUnitCombatState BsTarget(int32 Temporary, int32 Health = 1000)
	{
		return FRTUnitCombatState(Health, URTCombatLibrary::BaseShield + Temporary, Temporary);
	}

	/**
	 * La formula di `ApplyDamage` com'era prima del #4189, copiata qui: e' l'oracolo che dice se un chiamante a una sorgente
	 * sola ha ancora lo stesso esito. Non chiama niente del codice di produzione, di proposito.
	 */
	FRTDamageResult BsApplyDamageBefore4189(int32 Damage, ERTDamageSource Source, int32 Shield, int32 TemporaryShield,
		int32 Health)
	{
		const int32 SafeDamage = FMath::Max(0, Damage);
		const int32 SafeShield = FMath::Max(0, Shield);
		const int32 SafeTemp = FMath::Clamp(TemporaryShield, 0, SafeShield);
		const int32 SafeBase = SafeShield - SafeTemp;
		const int32 AbsorbedByTemp = FMath::Min(SafeDamage, SafeTemp);
		int32 Remaining = SafeDamage - AbsorbedByTemp;
		const int32 NewTemp = SafeTemp - AbsorbedByTemp;
		const int32 AbsorbedByBase = (Source == ERTDamageSource::Direct) ? FMath::Min(Remaining, SafeBase) : 0;
		Remaining -= AbsorbedByBase;
		const int32 NewBase = SafeBase - AbsorbedByBase;
		return FRTDamageResult(FMath::Max(0, Health - Remaining), NewBase + NewTemp, NewTemp);
	}

	const FRTDamageStageEntry* BsStage(const FRTDamageBreakdown& B, ERTDamageStage Stage)
	{
		for (const FRTDamageStageEntry& E : B.Stages)
		{
			if (E.Stage == Stage) { return &E; }
		}
		return nullptr;
	}

	ARTUnit* BsSpawnUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World || !Hero) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi
		U->ConfigureFromHeroData(Hero); // DEF e RES dal catalogo
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->SetPlannedCell(Cell); // fermo: qui conta il Blast, non il movimento
		return U;
	}

	/** Un tiratore del turno: dove sta, e l'elemento scritto sul `Def` della sua ArcPulse (`NAME_None` = com'e' oggi). */
	struct FBsShooter
	{
		FRTCellId Cell;
		FName ElementId;
	};

	/** Cio' che il turno ha prodotto sul bersaglio. */
	struct FBsTurnOutcome
	{
		TArray<FRTHexAttackHit> Hits;
		int32 HpLost = -1;
		int32 CombatEntries = 0;
		int32 CombatAmount = 0;
	};

	/**
	 * UN turno vero: uno o piu' Gadget (squadra 0) tirano `Hero.Gadget.ArcPulse` (220) a Riktor (squadra 1, DEF 45, RES 35
	 * nel catalogo, 1200 HP) in (0,0), che guarda a ovest. I tiratori stanno nell'arco frontale, quindi niente bonus alle
	 * spalle; l'arena e' piena, quindi niente copertura. Lo scudo di Riktor e' quello che il costruttore gli da' — la base,
	 * 50 — piu' il temporaneo dato. Contro di lui ArcPulse vale floor(220 x 100/145) = 151 senza elemento e
	 * floor(220 x 100/135) = 162 col fulmine.
	 *
	 * `false` = montaggio fallito, gia' segnalato da `Test`.
	 */
	bool BsArcPulseTurn(FAutomationTestBase& Test, const TArray<FBsShooter>& Shooters, int32 Temporary, FBsTurnOutcome& Out)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		ARTUnit* Riktor = BsSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
		if (!Test.TestNotNull(TEXT("mappa"), MapActor) || !Test.TestNotNull(TEXT("turn manager"), TM)
			|| !Test.TestNotNull(TEXT("Riktor"), Riktor))
		{
			return false;
		}

		// Premesse sul catalogo: se si muovono, i numeri dei test sono da riscrivere, non da ignorare.
		if (!Test.TestEqual(TEXT("premessa: Riktor ha DEF 45"), Riktor->Defense, 45)
			|| !Test.TestEqual(TEXT("premessa: Riktor ha RES 35"), Riktor->ResistanceDefault, 35)
			|| !Test.TestEqual(TEXT("premessa: Riktor non dichiara RES per elemento"), Riktor->ElementResistance.Num(), 0)
			|| !Test.TestEqual(TEXT("premessa: Riktor porta lo scudo base"), Riktor->Shield, URTCombatLibrary::BaseShield)
			|| !Test.TestEqual(TEXT("premessa: Riktor senza temporaneo"), Riktor->GetTemporaryShield(), 0)
			|| !Test.TestTrue(TEXT("premessa: Riktor regge i colpi del banco"), Riktor->Health > 400))
		{
			return false;
		}

		Riktor->Facing = ERTHexDirection::W; // guarda i tiratori: frontale
		Riktor->ClearPlannedMainAction();
		Riktor->AddTemporaryShield(Temporary);

		for (const FBsShooter& S : Shooters)
		{
			ARTUnit* Gadget = BsSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), S.Cell);
			if (!Test.TestNotNull(TEXT("Gadget"), Gadget) || !Test.TestNotNull(TEXT("l'ArcPulse di Gadget"), Gadget->GetAbility(0)))
			{
				return false;
			}
			URTActionData* ArcPulse = Gadget->GetAbility(0);
			if (!Test.TestEqual(TEXT("premessa: l'azione 0 di Gadget e' ArcPulse"), ArcPulse->Def.ActionId,
					FName(TEXT("Hero.Gadget.ArcPulse")))
				|| !Test.TestEqual(TEXT("premessa: ArcPulse vale 220"), URTCatalogLibrary::FirstDamage(ArcPulse->Def), 220)
				|| !Test.TestEqual(TEXT("premessa: ArcPulse e' senza elemento nel catalogo"), ArcPulse->Def.ElementId,
					FName(NAME_None)))
			{
				return false;
			}
			ArcPulse->Def.ElementId = S.ElementId; // l'elemento sul `Def` di QUESTO Gadget: nessuna azione di oggi ne ha uno
			Gadget->Facing = ERTHexDirection::E;
			Gadget->SetPlannedMainAction(0);
			Gadget->DeclareAttackOnUnit(Riktor);
		}

		const int32 HpBefore = Riktor->Health;
		TM->bRecordBlastPlanForTest = true;
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}

		Out.Hits = TM->RecordedBlastHitsForTest();
		Out.HpLost = HpBefore - Riktor->Health;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Phase == ERTMatchPhase::Blast && E.Category == ERTLogCategory::Combat
				&& E.ActionId == FName(TEXT("Hero.Gadget.ArcPulse")))
			{
				++Out.CombatEntries;
				Out.CombatAmount += E.Amount;
			}
		}
		return true;
	}
}

/**
 * Passo 1 del piano: un bersaglio con lo scudo base (50) e nessun temporaneo prende 100 elementali e perde 100 HP; 100
 * `Direct` gliene tolgono 50. Sul resolver, che e' dove il turno somma i colpi, e su `ApplyDamageParts` da sola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseShieldElementalHitGoesThroughTest,
	"RefactorTactics.Damage.BaseShield.ElementalHitGoesThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseShieldElementalHitGoesThroughTest::RunTest(const FString&)
{
	if (!TestEqual(TEXT("premessa: lo scudo base vale 50"), URTCombatLibrary::BaseShield, 50)) { return false; }

	{
		const TArray<FRTUnitCombatState> R = URTCombatResolver::ResolveAttacks({ BsTarget(0) }, { BsAttack(0, 100, BsElectric()) });
		TestEqual(TEXT("100 elementali: HP persi 100"), 1000 - R[0].Health, 100);
		TestEqual(TEXT("100 elementali: lo scudo base resta intero"), R[0].Shield, URTCombatLibrary::BaseShield);
		TestEqual(TEXT("100 elementali: nessun temporaneo"), R[0].TemporaryShield, 0);
	}
	{
		const TArray<FRTUnitCombatState> R = URTCombatResolver::ResolveAttacks({ BsTarget(0) }, { BsAttack(0, 100, NAME_None) });
		TestEqual(TEXT("100 Direct: HP persi 50"), 1000 - R[0].Health, 50);
		TestEqual(TEXT("100 Direct: lo scudo base e' consumato"), R[0].Shield, 0);
	}
	{
		// Un elementale piu' piccolo della base: la base non ne toglie niente, nemmeno quando potrebbe coprirlo tutto.
		const TArray<FRTUnitCombatState> R = URTCombatResolver::ResolveAttacks({ BsTarget(0) }, { BsAttack(0, 30, BsElectric()) });
		TestEqual(TEXT("30 elementali: HP persi 30"), 1000 - R[0].Health, 30);
		TestEqual(TEXT("30 elementali: lo scudo base resta intero"), R[0].Shield, URTCombatLibrary::BaseShield);
	}

	// La funzione da sola: `DirectDamage`, poi `NonDirectDamage`.
	{
		const FRTDamageResult R = URTCombatLibrary::ApplyDamageParts(0, 100, URTCombatLibrary::BaseShield, 0, 1000);
		TestEqual(TEXT("ApplyDamageParts(0, 100): HP 900"), R.Health, 900);
		TestEqual(TEXT("ApplyDamageParts(0, 100): scudo 50"), R.Shield, URTCombatLibrary::BaseShield);
	}
	{
		const FRTDamageResult R = URTCombatLibrary::ApplyDamageParts(100, 0, URTCombatLibrary::BaseShield, 0, 1000);
		TestEqual(TEXT("ApplyDamageParts(100, 0): HP 950"), R.Health, 950);
		TestEqual(TEXT("ApplyDamageParts(100, 0): scudo 0"), R.Shield, 0);
	}
	return true;
}

/**
 * Passo 2 del piano, il caso misto (Review Focus 3): base 50, temporaneo 30, 40 elementali e 60 `Direct` sullo stesso
 * bersaglio. Il temporaneo assorbe 30 dei 40 elementali (10 passano), la base 50 dei 60 `Direct` (10 passano): HP persi
 * 20, scudo a 0. Col temporaneo speso prima sul `Direct` (modello (b)) sarebbero 40 HP e 20 di base rimasta.
 *
 * Il secondo caso separa anche il modello di prima (c): base 50, temporaneo 20, 100 elementali e 30 `Direct`. Il temporaneo
 * prende 20 dei 100 elementali (80 passano), la base i 30 `Direct` e ne avanza 20: HP persi 80 (b: 100, c: 60).
 *
 * Ogni caso nei due ordini dell'array: la somma per bersaglio non dipende da quale colpo arrivi prima (invariante #3).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseShieldTemporaryTakesTheElementalPartFirstTest,
	"RefactorTactics.Damage.BaseShield.TemporaryTakesTheElementalPartFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseShieldTemporaryTakesTheElementalPartFirstTest::RunTest(const FString&)
{
	struct FCase
	{
		const TCHAR* Label;
		int32 Temporary;
		int32 Elemental;
		int32 Direct;
		int32 ExpectedHpLost;
		int32 ExpectedShield;
	};
	const TArray<FCase> Cases = {
		{ TEXT("piano: base 50, temp 30, 40 E + 60 D"), 30, 40, 60, 20, 0 },
		{ TEXT("tre modelli: base 50, temp 20, 100 E + 30 D"), 20, 100, 30, 80, 20 },
	};
	for (const FCase& C : Cases)
	{
		const TArray<TArray<FRTAttack>> Orders = {
			{ BsAttack(0, C.Elemental, BsElectric()), BsAttack(0, C.Direct, NAME_None) },
			{ BsAttack(0, C.Direct, NAME_None), BsAttack(0, C.Elemental, BsElectric()) },
		};
		for (int32 o = 0; o < Orders.Num(); ++o)
		{
			const TArray<FRTUnitCombatState> R = URTCombatResolver::ResolveAttacks({ BsTarget(C.Temporary) }, Orders[o]);
			TestEqual(*FString::Printf(TEXT("%s, ordine %d: HP persi %d"), C.Label, o, C.ExpectedHpLost),
				1000 - R[0].Health, C.ExpectedHpLost);
			TestEqual(*FString::Printf(TEXT("%s, ordine %d: scudo %d"), C.Label, o, C.ExpectedShield),
				R[0].Shield, C.ExpectedShield);
			TestEqual(*FString::Printf(TEXT("%s, ordine %d: il temporaneo e' consumato"), C.Label, o),
				R[0].TemporaryShield, 0);
		}

		const FRTDamageResult P = URTCombatLibrary::ApplyDamageParts(C.Direct, C.Elemental,
			URTCombatLibrary::BaseShield + C.Temporary, C.Temporary, 1000);
		TestEqual(*FString::Printf(TEXT("%s: ApplyDamageParts, HP persi %d"), C.Label, C.ExpectedHpLost),
			1000 - P.Health, C.ExpectedHpLost);
		TestEqual(*FString::Printf(TEXT("%s: ApplyDamageParts, scudo %d"), C.Label, C.ExpectedShield),
			P.Shield, C.ExpectedShield);
	}
	return true;
}

/**
 * I chiamanti a una sorgente sola — caduta, terreno, `Burning`, propagazione (`Environmental`), colpo predittivo e Overwatch
 * (`Direct`) — passano da `ApplyDamage`, che dal #4189 e' il caso a una parte di `ApplyDamageParts`. Qui si confronta, su
 * una griglia, con la formula di prima copiata nel test: stesso HP, stesso scudo, stesso temporaneo. E il resolver con soli
 * colpi senza elemento da' cio' che dava, cioe' `ApplyDamage` della somma con `Direct`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseShieldSingleSourceKeepsTodaysArithmeticTest,
	"RefactorTactics.Damage.BaseShield.SingleSourceKeepsTodaysArithmetic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseShieldSingleSourceKeepsTodaysArithmeticTest::RunTest(const FString&)
{
	const TArray<int32> Damages = { -5, 0, 1, 8, 29, 30, 49, 50, 51, 80, 81, 130, 400 };
	const TArray<int32> Shields = { 0, 20, 50, 80 };
	const TArray<int32> Temps = { -3, 0, 10, 30, 50, 120 }; // 120 > ogni scudo: il clamp
	const TArray<int32> Healths = { 1, 60, 1000 };
	const TArray<ERTDamageSource> Sources = { ERTDamageSource::Direct, ERTDamageSource::Environmental };

	int32 Checked = 0, Mismatches = 0;
	for (const ERTDamageSource Source : Sources)
	for (const int32 Damage : Damages)
	for (const int32 Shield : Shields)
	for (const int32 Temp : Temps)
	for (const int32 Health : Healths)
	{
		const FRTDamageResult Now = URTCombatLibrary::ApplyDamage(Damage, Source, Shield, Temp, Health);
		const FRTDamageResult Before = BsApplyDamageBefore4189(Damage, Source, Shield, Temp, Health);
		++Checked;
		if (Now.Health != Before.Health || Now.Shield != Before.Shield || Now.TemporaryShield != Before.TemporaryShield)
		{
			++Mismatches;
			AddError(FString::Printf(TEXT("ApplyDamage(%d, %s, scudo %d, temp %d, HP %d): ora (%d, %d, %d), prima (%d, %d, %d)"),
				Damage, Source == ERTDamageSource::Direct ? TEXT("Direct") : TEXT("Environmental"), Shield, Temp, Health,
				Now.Health, Now.Shield, Now.TemporaryShield, Before.Health, Before.Shield, Before.TemporaryShield));
		}
	}
	TestEqual(TEXT("la griglia e' stata percorsa tutta"), Checked, 2 * 13 * 4 * 6 * 3);
	TestEqual(TEXT("nessuna differenza dalla formula di prima"), Mismatches, 0);

	// Controllo che l'oracolo distingua le sorgenti: con la base piena, `Direct` e `Environmental` divergono.
	TestNotEqual(TEXT("controllo: l'oracolo separa Direct da Environmental"),
		BsApplyDamageBefore4189(30, ERTDamageSource::Direct, 50, 0, 1000).Health,
		BsApplyDamageBefore4189(30, ERTDamageSource::Environmental, 50, 0, 1000).Health);

	// Il resolver con soli colpi senza elemento: due bersagli, tre colpi, la somma per bersaglio con `Direct`.
	const TArray<FRTUnitCombatState> Units = { BsTarget(25, 500), BsTarget(0, 40) };
	const TArray<FRTAttack> Attacks = { BsAttack(0, 30, NAME_None), BsAttack(1, 70, NAME_None), BsAttack(0, 45, NAME_None) };
	const TArray<FRTUnitCombatState> R = URTCombatResolver::ResolveAttacks(Units, Attacks);
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		const int32 Sum = (i == 0) ? 75 : 70;
		const FRTDamageResult Expected = BsApplyDamageBefore4189(Sum, ERTDamageSource::Direct, Units[i].Shield,
			Units[i].TemporaryShield, Units[i].Health);
		TestEqual(*FString::Printf(TEXT("resolver, bersaglio %d: HP come prima"), i), R[i].Health, Expected.Health);
		TestEqual(*FString::Printf(TEXT("resolver, bersaglio %d: scudo come prima"), i), R[i].Shield, Expected.Shield);
		TestEqual(*FString::Printf(TEXT("resolver, bersaglio %d: temporaneo come prima"), i), R[i].TemporaryShield,
			Expected.TemporaryShield);
	}
	return true;
}

/**
 * L'elemento arriva dal colpo del piano al colpo del resolver: `ToAttacks` lo copia, e i tre delta della composizione di
 * produzione (`ApplyEligibleHitDelta(ApplyAbsorptionPool(ApplyDamageDelta(ToAttacks(Plan))))`, `RTTurnManager.cpp`) lo
 * conservano. Senza questo passaggio il resolver vedrebbe solo colpi `Direct`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseShieldToAttacksCarriesTheElementTest,
	"RefactorTactics.Damage.BaseShield.ToAttacksCarriesTheElement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseShieldToAttacksCarriesTheElementTest::RunTest(const FString&)
{
	FRTHexBlastPlan Plan;
	FRTHexAttackHit Elemental(/*Attacker*/ 0, /*Target*/ 1, /*Power*/ 120, /*Intent*/ 0);
	Elemental.ElementId = BsElectric();
	Plan.Hits.Add(Elemental);
	Plan.Hits.Add(FRTHexAttackHit(/*Attacker*/ 2, /*Target*/ 1, /*Power*/ 90, /*Intent*/ 1));

	const TArray<FRTAttack> Raw = URTHexCombatLibrary::ToAttacks(Plan);
	if (!TestEqual(TEXT("due colpi"), Raw.Num(), 2)) { return false; }
	TestEqual(TEXT("ToAttacks: il colpo elementale porta l'elemento"), Raw[0].ElementId, BsElectric());
	TestEqual(TEXT("ToAttacks: il colpo senza elemento resta Direct"), Raw[1].ElementId, FName(NAME_None));

	// La composizione di produzione, con un delta, un pool e una Guardia che mordono tutti e tre.
	const TArray<int32> Delta = { 0, -10, 0 };
	const TArray<int32> Pool = { 0, 15, 0 };
	const TArray<int32> Guard = { 0, -20, 0 };
	const TArray<bool> All = { true, true };
	const TArray<FRTAttack> Composed = URTCombatResolver::ApplyEligibleHitDelta(
		URTCombatResolver::ApplyAbsorptionPool(URTCombatResolver::ApplyDamageDelta(Raw, Delta), Pool, All,
			URTCombatLibrary::ReactionReductionPoolSource),
		Guard, All, URTCombatLibrary::GuardPerHitSource);
	if (!TestEqual(TEXT("la composizione conserva i colpi"), Composed.Num(), 2)) { return false; }
	TestEqual(TEXT("premessa: i delta hanno morso il colpo elementale"), Composed[0].Power, 120 - 10 - 15 - 20);
	TestEqual(TEXT("dopo i delta: l'elementale porta ancora l'elemento"), Composed[0].ElementId, BsElectric());
	TestEqual(TEXT("dopo i delta: l'altro resta Direct"), Composed[1].ElementId, FName(NAME_None));
	return true;
}

/**
 * Il breakdown resta coerente con la scomposizione: la voce `ShieldAbsorption` porta `Before` = la somma dei colpi, `After` =
 * gli HP persi davvero (l'`Amount` che il TurnLog ripartisce) e `Operand` = quanto hanno assorbito temporaneo e base insieme.
 * Con un elementale da solo lo scudo base non assorbe niente e la voce lo dice: operando 0.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseShieldBreakdownFollowsTheSplitTest,
	"RefactorTactics.Damage.BaseShield.BreakdownFollowsTheSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseShieldBreakdownFollowsTheSplitTest::RunTest(const FString&)
{
	{
		TMap<int32, FRTDamageBreakdown> ByTarget;
		const TArray<FRTUnitCombatState> R = URTCombatResolver::ResolveAttacksWithBreakdown({ BsTarget(30) },
			{ BsAttack(0, 40, BsElectric()), BsAttack(0, 60, NAME_None) }, ByTarget);
		const FRTDamageBreakdown* B = ByTarget.Find(0);
		if (!TestNotNull(TEXT("misto: il breakdown c'e'"), B)) { return false; }
		const FRTDamageStageEntry* Sum = BsStage(*B, ERTDamageStage::TargetSum);
		const FRTDamageStageEntry* Shield = BsStage(*B, ERTDamageStage::ShieldAbsorption);
		if (!TestNotNull(TEXT("misto: c'e' la somma"), Sum) || !TestNotNull(TEXT("misto: c'e' l'assorbimento"), Shield))
		{
			return false;
		}
		TestEqual(TEXT("misto: la somma conta due colpi"), Sum->Operand, 2);
		TestEqual(TEXT("misto: la somma vale 100"), Sum->After, 100);
		TestEqual(TEXT("misto: l'assorbimento parte da 100"), Shield->Before, 100);
		TestEqual(TEXT("misto: l'assorbimento arriva agli HP persi"), Shield->After, 1000 - R[0].Health);
		TestEqual(TEXT("misto: gli HP persi sono 20"), Shield->After, 20);
		TestEqual(TEXT("misto: temporaneo e base hanno assorbito 80"), Shield->Operand, 80);
		TestEqual(TEXT("misto: l'ultima voce e' l'assorbimento"), static_cast<int32>(B->Stages.Last().Stage),
			static_cast<int32>(ERTDamageStage::ShieldAbsorption));
	}
	{
		TMap<int32, FRTDamageBreakdown> ByTarget;
		const TArray<FRTUnitCombatState> R = URTCombatResolver::ResolveAttacksWithBreakdown({ BsTarget(0) },
			{ BsAttack(0, 100, BsElectric()) }, ByTarget);
		const FRTDamageBreakdown* B = ByTarget.Find(0);
		const FRTDamageStageEntry* Shield = B ? BsStage(*B, ERTDamageStage::ShieldAbsorption) : nullptr;
		if (!TestNotNull(TEXT("elementale: c'e' l'assorbimento"), Shield)) { return false; }
		TestEqual(TEXT("elementale: parte da 100"), Shield->Before, 100);
		TestEqual(TEXT("elementale: arriva a 100, gli HP persi"), Shield->After, 1000 - R[0].Health);
		TestEqual(TEXT("elementale: lo scudo base non ha assorbito niente"), Shield->Operand, 0);
	}
	return true;
}

/**
 * Un colpo elementale del Blast incontra davvero lo scudo base, in un turno vero: ArcPulse col fulmine contro Riktor vale
 * 162 (la RES 35) e gli toglie 162 HP — la base (50) non lo ferma. Il controllo e' la stessa ArcPulse senza elemento:
 * 151 (la DEF 45) meno la base, 101. La voce del TurnLog dice gli stessi HP.
 *
 * E' il test che prova il tragitto intero: l'elemento dall'intento al colpo (Task 1), dal colpo a `FRTAttack` (`ToAttacks`),
 * attraverso i delta del turno, fino alla somma per bersaglio. Se si perdesse in un punto qualunque, il colpo tornerebbe
 * `Direct` e gli HP persi sarebbero 112.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseShieldElementalHitIgnoresTheBaseInATurnTest,
	"RefactorTactics.Damage.BaseShield.ElementalHitIgnoresTheBaseInATurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseShieldElementalHitIgnoresTheBaseInATurnTest::RunTest(const FString&)
{
	FBsTurnOutcome Fulmine;
	if (!BsArcPulseTurn(*this, { { FRTCellId(-1, 0, 0), BsElectric() } }, /*Temporary*/ 0, Fulmine)) { return false; }
	if (TestEqual(TEXT("fulmine: un colpo"), Fulmine.Hits.Num(), 1))
	{
		TestEqual(TEXT("premessa: il colpo e' elementale"), Fulmine.Hits[0].ElementId, BsElectric());
		TestEqual(TEXT("premessa: floor(220 x 100/135) = 162"), Fulmine.Hits[0].Power, 162);
		TestEqual(TEXT("premessa: frontale, nessun bonus"), Fulmine.Hits[0].RearBonusApplied, 0);
	}
	TestEqual(TEXT("fulmine: HP persi 162, lo scudo base non lo ferma"), Fulmine.HpLost, 162);
	TestEqual(TEXT("fulmine: una voce di TurnLog"), Fulmine.CombatEntries, 1);
	TestEqual(TEXT("fulmine: la voce dice 162"), Fulmine.CombatAmount, 162);

	FBsTurnOutcome Oggi;
	if (!BsArcPulseTurn(*this, { { FRTCellId(-1, 0, 0), NAME_None } }, /*Temporary*/ 0, Oggi)) { return false; }
	if (TestEqual(TEXT("controllo: un colpo"), Oggi.Hits.Num(), 1))
	{
		TestEqual(TEXT("controllo: floor(220 x 100/145) = 151"), Oggi.Hits[0].Power, 151);
	}
	TestEqual(TEXT("controllo senza elemento: 151 meno la base"), Oggi.HpLost, 151 - URTCombatLibrary::BaseShield);
	TestEqual(TEXT("controllo: la voce dice gli stessi HP"), Oggi.CombatAmount, Oggi.HpLost);
	return true;
}

/**
 * Il caso misto in un turno vero: due Gadget, uno col fulmine (162) e uno senza elemento (151), sullo stesso Riktor con un
 * temporaneo di 200. Il temporaneo prende prima i 162 elementali, poi 38 dei 151 `Direct`; la base 50 dei 113 rimasti: HP
 * persi 63. Col temporaneo speso prima sul `Direct` (modello (b)) la base non fermerebbe niente: 113.
 *
 * Il TurnLog ripartisce il netto fra i due colpi in proporzione a `Power` ([D-438]): la somma delle voci e' gli HP persi.
 * Come si divida fra i due e' la regola di [D-438], non di questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageBaseShieldMixedTargetInATurnTest,
	"RefactorTactics.Damage.BaseShield.MixedTargetInATurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageBaseShieldMixedTargetInATurnTest::RunTest(const FString&)
{
	FBsTurnOutcome Misto;
	// Ovest e sud-ovest di Riktor, che guarda a ovest: entrambi nell'arco frontale.
	if (!BsArcPulseTurn(*this, { { FRTCellId(-1, 0, 0), BsElectric() }, { FRTCellId(-1, 1, 0), NAME_None } },
			/*Temporary*/ 200, Misto))
	{
		return false;
	}
	if (!TestEqual(TEXT("due colpi"), Misto.Hits.Num(), 2)) { return false; }
	int32 Elemental = 0, Direct = 0;
	for (const FRTHexAttackHit& Hit : Misto.Hits)
	{
		TestEqual(TEXT("premessa: frontale, nessun bonus"), Hit.RearBonusApplied, 0);
		(Hit.ElementId.IsNone() ? Direct : Elemental) += Hit.Power;
	}
	TestEqual(TEXT("premessa: la parte elementale vale 162"), Elemental, 162);
	TestEqual(TEXT("premessa: la parte Direct vale 151"), Direct, 151);

	TestEqual(TEXT("HP persi 63: temporaneo prima sull'elementale, base sul Direct"), Misto.HpLost, 63);
	TestEqual(TEXT("due voci di TurnLog"), Misto.CombatEntries, 2);
	TestEqual(TEXT("la somma delle voci e' gli HP persi"), Misto.CombatAmount, Misto.HpLost);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
