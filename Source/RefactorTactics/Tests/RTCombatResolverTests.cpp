#include "Misc/AutomationTest.h"
#include "Combat/RTCombatResolver.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
