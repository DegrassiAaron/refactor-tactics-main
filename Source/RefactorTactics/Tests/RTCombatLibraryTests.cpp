#include "Misc/AutomationTest.h"
#include "Combat/RTCombatLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamagePartlyAbsorbedTest,
	"RefactorTactics.Combat.DamagePartlyAbsorbedByShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamagePartlyAbsorbedTest::RunTest(const FString&)
{
	// 30 danni, scudo 20 -> scudo assorbe 20, 10 agli HP: 100 -> 90.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(30, 20, 100);
	TestEqual(TEXT("scudo consumato"), R.Shield, 0);
	TestEqual(TEXT("HP 90"), R.Health, 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageFullyAbsorbedTest,
	"RefactorTactics.Combat.DamageFullyAbsorbedByShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageFullyAbsorbedTest::RunTest(const FString&)
{
	// 15 danni, scudo 20 -> scudo assorbe tutto (resta 5), HP intatti.
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(15, 20, 100);
	TestEqual(TEXT("scudo residuo 5"), R.Shield, 5);
	TestEqual(TEXT("HP 100"), R.Health, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageExceedsAllTest,
	"RefactorTactics.Combat.DamageExceedingHealthClampsToZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageExceedsAllTest::RunTest(const FString&)
{
	// Danno enorme -> scudo e HP a 0 (nessun valore negativo).
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(200, 20, 100);
	TestEqual(TEXT("scudo 0"), R.Shield, 0);
	TestEqual(TEXT("HP 0"), R.Health, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageNoShieldTest,
	"RefactorTactics.Combat.DamageWithoutShieldHitsHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageNoShieldTest::RunTest(const FString&)
{
	const FRTDamageResult R = URTCombatLibrary::ApplyDamage(30, 0, 100);
	TestEqual(TEXT("scudo 0"), R.Shield, 0);
	TestEqual(TEXT("HP 70"), R.Health, 70);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGainEnergyTest,
	"RefactorTactics.Combat.GainEnergyClampsToMax",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGainEnergyTest::RunTest(const FString&)
{
	TestEqual(TEXT("25+25 = 50"), URTCombatLibrary::GainEnergy(25, 25, 100), 50);
	TestEqual(TEXT("clamp al massimo"), URTCombatLibrary::GainEnergy(90, 25, 100), 100);
	TestEqual(TEXT("gia' al massimo resta"), URTCombatLibrary::GainEnergy(100, 25, 100), 100);
	TestEqual(TEXT("nessun guadagno"), URTCombatLibrary::GainEnergy(40, 0, 100), 40);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUltimateReadyTest,
	"RefactorTactics.Combat.UltimateReadyAtFullEnergy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUltimateReadyTest::RunTest(const FString&)
{
	TestTrue(TEXT("energia piena -> pronto"), URTCombatLibrary::IsUltimateReady(100, 100));
	TestFalse(TEXT("energia parziale -> non pronto"), URTCombatLibrary::IsUltimateReady(99, 100));
	TestFalse(TEXT("energia zero -> non pronto"), URTCombatLibrary::IsUltimateReady(0, 100));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEffectiveMoveRangeTest,
	"RefactorTactics.Combat.EffectiveMoveRangeWithStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEffectiveMoveRangeTest::RunTest(const FString&)
{
	TestEqual(TEXT("nessuno status -> base"), URTCombatLibrary::EffectiveMoveRange(4, false, false), 4);
	TestEqual(TEXT("root -> 0"), URTCombatLibrary::EffectiveMoveRange(4, true, false), 0);
	TestEqual(TEXT("slow -> meta'"), URTCombatLibrary::EffectiveMoveRange(4, false, true), 2);
	TestEqual(TEXT("slow su dispari arrotonda per difetto"), URTCombatLibrary::EffectiveMoveRange(5, false, true), 2);
	TestEqual(TEXT("root prevale su slow"), URTCombatLibrary::EffectiveMoveRange(4, true, true), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
