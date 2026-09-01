// Copyright RefactorTactics. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTBreakdownTests
{
	/** Tre unita' identiche: 100 HP, nessuno scudo se non dichiarato. */
	TArray<FRTUnitCombatState> MakeUnits()
	{
		return { FRTUnitCombatState(100, 0, 0), FRTUnitCombatState(100, 0, 0), FRTUnitCombatState(100, 0, 0) };
	}

	FRTAttack MakeAttack(int32 Target, int32 Power, int32 Attacker)
	{
		FRTAttack A(Target, Power, Attacker);
		// Il colpo entra nella catena come lo produrrebbe `ToAttacks`: uno stadio `Catalog` e basta.
		A.Breakdown.Emplace(ERTDamageStage::Catalog, FName(TEXT("intent")), ERTDamageOp::Add, Power, 0, Power);
		return A;
	}

	const FRTDamageStageEntry* Find(const FRTDamageBreakdown& B, ERTDamageStage Stage)
	{
		return B.Stages.FindByPredicate([Stage](const FRTDamageStageEntry& E) { return E.Stage == Stage; });
	}
}

/**
 * L'ULTIMO `After` DEL BREAKDOWN E' IL DANNO CHE GLI HP HANNO SUBITO — `#1951`.
 *
 * 🔑 **E' cio' che rende il breakdown una LETTURA e non un secondo calcolo.** Se l'ultimo stadio dicesse
 * un numero diverso da quello che la partita ha applicato, il registro spiegherebbe un danno che non e'
 * avvenuto — ed e' il modo in cui una diagnostica diventa peggio di nessuna diagnostica.
 *
 * ⚠️ Il valore confrontato e' il danno EFFETTIVO, che e' la definizione di `FRTTurnLogEntry::Amount`. Il
 * confronto con la voce di log vera vive in uno scenario; qui si pinna l'identita' alla fonte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownMatchesDamageDealtTest,
	"RefactorTactics.Damage.BreakdownFinalValueMatchesDamageDealt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownMatchesDamageDealtTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	// Uno scudo base da 5 sul bersaglio: 22 nominali -> 17 sugli HP, la catena gia' misurata da `D-224`.
	TArray<FRTUnitCombatState> Units = MakeUnits();
	Units[1] = FRTUnitCombatState(120, 5, 0);

	const TArray<FRTAttack> Attacks = { MakeAttack(1, 22, 0) };

	TMap<int32, FRTDamageBreakdown> ByTarget;
	const TArray<FRTUnitCombatState> After = URTCombatResolver::ResolveAttacksWithBreakdown(Units, Attacks, ByTarget);

	const int32 HpLost = Units[1].Health - After[1].Health;
	TestEqual(TEXT("lo scudo base ha fermato 5 dei 22"), HpLost, 17);

	const FRTDamageBreakdown* B = ByTarget.Find(1);
	if (!TestNotNull(TEXT("il bersaglio ha un registro"), B)) { return false; }
	if (!TestTrue(TEXT("il registro non e' vuoto"), B->Stages.Num() > 0)) { return false; }

	TestEqual(TEXT("l'ultimo After e' il danno subito davvero"), B->Stages.Last().After, HpLost);
	TestEqual(TEXT("e l'ultimo stadio e' l'assorbimento dello scudo"),
		static_cast<int32>(B->Stages.Last().Stage), static_cast<int32>(ERTDamageStage::ShieldAbsorption));

	// ⛔ La controprova che il registro non sia una costante: senza scudo l'ultimo `After` cambia.
	const TArray<FRTUnitCombatState> Bare = MakeUnits();
	TMap<int32, FRTDamageBreakdown> BareByTarget;
	URTCombatResolver::ResolveAttacksWithBreakdown(Bare, Attacks, BareByTarget);
	TestEqual(TEXT("senza scudo passano tutti e 22"), BareByTarget[1].Stages.Last().After, 22);

	return true;
}

/**
 * PERMUTARE I COLPI NON CAMBIA LA SEQUENZA DEGLI STADI — `#1951`.
 *
 * 🔴 **La versione precedente di `ResolveAttacks` iterava una `TMap`**, e per gli stati era indifferente:
 * ogni bersaglio si risolve dal proprio stato iniziale. Per un REGISTRO non lo e' — un elenco che cambia
 * ordine fra due esecuzioni non e' verificabile, ed e' un vincolo dichiarato dalla issue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownOrderIsStableTest,
	"RefactorTactics.Damage.BreakdownOrderIsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownOrderIsStableTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	const TArray<FRTUnitCombatState> Units = MakeUnits();
	const TArray<FRTAttack> Straight = { MakeAttack(1, 10, 0), MakeAttack(2, 7, 0), MakeAttack(1, 4, 0) };
	const TArray<FRTAttack> Swapped  = { MakeAttack(2, 7, 0), MakeAttack(1, 10, 0), MakeAttack(1, 4, 0) };

	TMap<int32, FRTDamageBreakdown> A, B;
	URTCombatResolver::ResolveAttacksWithBreakdown(Units, Straight, A);
	URTCombatResolver::ResolveAttacksWithBreakdown(Units, Swapped, B);

	const int32 Targets[] = { 1, 2 };
	for (const int32 Target : Targets)
	{
		const FRTDamageBreakdown* L = A.Find(Target);
		const FRTDamageBreakdown* R = B.Find(Target);
		if (!TestNotNull(TEXT("registro a sinistra"), L) || !TestNotNull(TEXT("registro a destra"), R))
		{
			return false;
		}
		if (!TestEqual(FString::Printf(TEXT("bersaglio %d: stesso numero di stadi"), Target),
			L->Stages.Num(), R->Stages.Num()))
		{
			continue;
		}
		for (int32 i = 0; i < L->Stages.Num(); ++i)
		{
			TestEqual(FString::Printf(TEXT("bersaglio %d, stadio %d"), Target, i),
				static_cast<int32>(L->Stages[i].Stage), static_cast<int32>(R->Stages[i].Stage));
			TestEqual(FString::Printf(TEXT("bersaglio %d, stadio %d: After"), Target, i),
				L->Stages[i].After, R->Stages[i].After);
		}
	}
	return true;
}

/**
 * UNO STADIO CHE NON SI APPLICA NON COMPARE — `#1951`.
 *
 * 🔑 **Un elenco che contiene tutto non spiega niente.** La differenza fra «la Guardia non ha assorbito» e
 * «la Guardia non c'era» si legge solo se il secondo caso lascia il registro muto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownOmitsStagesThatDidNotApplyTest,
	"RefactorTactics.Damage.BreakdownOmitsStagesThatDidNotApply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownOmitsStagesThatDidNotApplyTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	const TArray<FRTUnitCombatState> Units = MakeUnits();

	// Un colpo solo, nessuna mitigazione: niente somma per bersaglio, niente pool, niente delta.
	TMap<int32, FRTDamageBreakdown> ByTarget;
	URTCombatResolver::ResolveAttacksWithBreakdown(Units, { MakeAttack(1, 9, 0) }, ByTarget);

	const FRTDamageBreakdown& B = ByTarget[1];
	TestNull(TEXT("nessuna somma per bersaglio con un colpo solo"), Find(B, ERTDamageStage::TargetSum));
	TestNull(TEXT("nessun pool: la Guardia non c'era"), Find(B, ERTDamageStage::AbsorptionPool));
	TestNull(TEXT("nessun delta di primo colpo"), Find(B, ERTDamageStage::FirstHitDelta));

	// ✅ La controprova: con DUE colpi la somma compare, quindi l'assenza sopra e' una scelta e non un buco.
	TMap<int32, FRTDamageBreakdown> Two;
	URTCombatResolver::ResolveAttacksWithBreakdown(Units, { MakeAttack(1, 9, 0), MakeAttack(1, 3, 0) }, Two);
	TestNotNull(TEXT("con due colpi la somma compare"), Find(Two[1], ERTDamageStage::TargetSum));

	return true;
}

/**
 * LA GUARDIA FRONTALE E QUELLA ALLE SPALLE RACCONTANO DUE STORIE PER LO STESSO DANNO NOMINALE — `#1951`.
 *
 * E' il caso che `D-292` + `D-206` decidono: solo l'arco frontale consuma il pool, e un colpo alle spalle
 * passa **intero** lasciando il budget intatto. Senza registro i due esiti si distinguono solo dagli HP.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBreakdownGuardTellsTwoStoriesTest,
	"RefactorTactics.Damage.BreakdownGuardFrontalAndBehindDiffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBreakdownGuardTellsTwoStoriesTest::RunTest(const FString&)
{
	using namespace RTBreakdownTests;

	const TArray<FRTUnitCombatState> Units = MakeUnits();
	const TArray<FRTAttack> Attacks = { MakeAttack(1, 12, 0) };
	const TArray<int32> Pool = { 0, 15, 0 };
	const TArray<bool> Frontally = { true };
	const TArray<bool> FromBehind = { false };

	TMap<int32, FRTDamageBreakdown> Frontal, Behind;
	URTCombatResolver::ResolveAttacksWithBreakdown(
		Units, URTCombatResolver::ApplyAbsorptionPool(Attacks, Pool, Frontally), Frontal);
	URTCombatResolver::ResolveAttacksWithBreakdown(
		Units, URTCombatResolver::ApplyAbsorptionPool(Attacks, Pool, FromBehind), Behind);

	const FRTDamageStageEntry* FrontPool = Find(Frontal[1], ERTDamageStage::AbsorptionPool);
	if (TestNotNull(TEXT("frontale: il pool ha morso"), FrontPool))
	{
		TestEqual(TEXT("e ha assorbito tutti e 12"), FrontPool->Operand, 12);
	}
	TestEqual(TEXT("frontale: niente arriva agli HP"), Frontal[1].Stages.Last().After, 0);

	TestNull(TEXT("alle spalle: il pool non compare affatto"), Find(Behind[1], ERTDamageStage::AbsorptionPool));
	TestEqual(TEXT("alle spalle: passano tutti e 12"), Behind[1].Stages.Last().After, 12);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
