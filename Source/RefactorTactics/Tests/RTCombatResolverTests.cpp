#include "Misc/AutomationTest.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTCombatLibrary.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAttackSingleTest,
	"RefactorTactics.Combat.SingleAttackAppliesDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAttackSingleTest::RunTest(const FString&)
{
	// U0 (100/0) colpisce U1 (100/20) con 30 -> scudo 20 assorbe, 10 agli HP: U1 = 90/0.
	const TArray<FRTUnitCombatState> Units = { {100, 0}, {100, 20} };
	const TArray<FRTAttack> Attacks = { FRTAttack(1, 30) };
	const TArray<FRTUnitCombatState> Out = URTCombatResolver::ResolveAttacks(Units, Attacks);
	TestEqual(TEXT("U0 intatta"), Out[0].Health, 100);
	TestEqual(TEXT("U1 HP 90"), Out[1].Health, 90);
	TestEqual(TEXT("U1 scudo 0"), Out[1].Shield, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAttackFocusFireTest,
	"RefactorTactics.Combat.FocusFireSumsDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAttackFocusFireTest::RunTest(const FString&)
{
	// U0 e U1 colpiscono entrambe U2 (100/20) con 30 ciascuna -> 60 danni: 20 scudo + 40 HP = 60.
	const TArray<FRTUnitCombatState> Units = { {100, 0}, {100, 0}, {100, 20} };
	const TArray<FRTAttack> Attacks = { FRTAttack(2, 30), FRTAttack(2, 30) };
	const TArray<FRTUnitCombatState> Out = URTCombatResolver::ResolveAttacks(Units, Attacks);
	TestEqual(TEXT("U2 HP 60"), Out[2].Health, 60);
	TestEqual(TEXT("U2 scudo 0"), Out[2].Shield, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAttackMutualTest,
	"RefactorTactics.Combat.MutualAttackUsesInitialState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAttackMutualTest::RunTest(const FString&)
{
	// U0 (20/0) e U1 (20/0) si colpiscono con 30: entrambe muoiono, perche' il danno
	// e' calcolato sullo stato iniziale (nessuna delle due "salta" il colpo morendo prima).
	const TArray<FRTUnitCombatState> Units = { {20, 0}, {20, 0} };
	const TArray<FRTAttack> Attacks = { FRTAttack(1, 30), FRTAttack(0, 30) };
	const TArray<FRTUnitCombatState> Out = URTCombatResolver::ResolveAttacks(Units, Attacks);
	TestEqual(TEXT("U0 morta"), Out[0].Health, 0);
	TestEqual(TEXT("U1 morta"), Out[1].Health, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAttackOrderIndependentTest,
	"RefactorTactics.Combat.AttackResolutionIsOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAttackOrderIndependentTest::RunTest(const FString&)
{
	const TArray<FRTUnitCombatState> Units = { {100, 0}, {100, 0}, {100, 20} };
	const TArray<FRTAttack> Forward = { FRTAttack(2, 30), FRTAttack(2, 30), FRTAttack(0, 50) };
	TArray<FRTAttack> Backward = Forward;
	Algo::Reverse(Backward);

	const TArray<FRTUnitCombatState> A = URTCombatResolver::ResolveAttacks(Units, Forward);
	const TArray<FRTUnitCombatState> B = URTCombatResolver::ResolveAttacks(Units, Backward);

	bool bSame = (A.Num() == B.Num());
	for (int32 i = 0; i < A.Num() && bSame; ++i)
	{
		bSame = (A[i].Health == B[i].Health && A[i].Shield == B[i].Shield);
	}
	TestTrue(TEXT("stesso esito invertendo l'ordine degli attacchi"), bSame);
	TestEqual(TEXT("U0 HP 50"), A[0].Health, 50);
	TestEqual(TEXT("U2 HP 60"), A[2].Health, 60);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTExposedFirstHitTest,
	"RefactorTactics.Status.Exposed.FirstDirectHitOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTExposedFirstHitTest::RunTest(const FString&)
{
	// U1 e' Exposed (ha scattato allo scoperto), U2 no. Tre colpi in arrivo: due su U1, uno su U2.
	TArray<int32> Delta;
	Delta.Init(0, 3);
	Delta[1] = URTCombatLibrary::ExposedFirstHitBonus;

	const TArray<FRTAttack> Attacks = { FRTAttack(1, 20), FRTAttack(1, 10), FRTAttack(2, 20) };
	const TArray<FRTAttack> Boosted = URTCombatResolver::ApplyFirstHitDelta(Attacks, Delta);

	if (!TestEqual(TEXT("nessun colpo si perde per strada"), Boosted.Num(), 3)) { return false; }
	TestEqual(TEXT("il primo colpo su chi e' esposto prende +5"), Boosted[0].Power, 25);
	TestEqual(TEXT("il secondo colpo sullo stesso bersaglio NON lo prende: vale una volta sola"), Boosted[1].Power, 10);
	TestEqual(TEXT("chi non e' esposto incassa il danno nominale"), Boosted[2].Power, 20);

	// Ordine-indipendenza (invariante #3): il TOTALE per bersaglio non cambia se i colpi arrivano in un altro
	// ordine — cambia solo a quale colpo e' attribuito il bonus, e i danni si sommano.
	TArray<FRTAttack> Reversed = Attacks;
	Algo::Reverse(Reversed);
	const TArray<FRTAttack> BoostedRev = URTCombatResolver::ApplyFirstHitDelta(Reversed, Delta);
	int32 SumDirect = 0, SumReversed = 0;
	for (const FRTAttack& A : Boosted)    { if (A.TargetIndex == 1) { SumDirect += A.Power; } }
	for (const FRTAttack& A : BoostedRev) { if (A.TargetIndex == 1) { SumReversed += A.Power; } }
	TestEqual(TEXT("danno totale sull'esposto: 20 + 10 + 5"), SumDirect, 35);
	TestEqual(TEXT("invertendo l'ordine dei colpi il totale e' identico"), SumReversed, SumDirect);

	// Nessuno esposto: gli attacchi restano quelli dichiarati (nessun ritocco silenzioso).
	TArray<int32> NoDelta;
	NoDelta.Init(0, 3);
	const TArray<FRTAttack> Untouched = URTCombatResolver::ApplyFirstHitDelta(Attacks, NoDelta);
	TestEqual(TEXT("senza stato il primo colpo non cambia"), Untouched[0].Power, 20);

	// Un delta negativo (la Guardia di CP 4.4) puo' annullare un colpo, non curare il bersaglio.
	TArray<int32> Guarded;
	Guarded.Init(0, 3);
	Guarded[1] = -50;
	const TArray<FRTAttack> Blocked = URTCombatResolver::ApplyFirstHitDelta(Attacks, Guarded);
	TestEqual(TEXT("un delta piu' grande del colpo lo azzera, non lo inverte"), Blocked[0].Power, 0);

	// Bersaglio fuori dall'array dei delta: nessun crash e nessuna modifica.
	TArray<int32> Short;
	Short.Init(0, 1);
	const TArray<FRTAttack> Safe = URTCombatResolver::ApplyFirstHitDelta(Attacks, Short);
	TestEqual(TEXT("indice fuori dai delta: colpo invariato"), Safe[0].Power, 20);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
