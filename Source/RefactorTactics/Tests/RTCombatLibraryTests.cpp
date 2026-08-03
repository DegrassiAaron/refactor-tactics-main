#include "Misc/AutomationTest.h"
#include "Combat/RTCombatLibrary.h"
#include "Turn/RTTurnLog.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAbilityUsableTest,
	"RefactorTactics.Combat.AbilityUsableByCooldownAndEnergy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAbilityUsableTest::RunTest(const FString&)
{
	TestTrue(TEXT("pronta e senza costo"), URTCombatLibrary::IsAbilityUsable(0, 50, 0));
	TestFalse(TEXT("in ricarica"), URTCombatLibrary::IsAbilityUsable(2, 50, 0));
	TestTrue(TEXT("energia sufficiente"), URTCombatLibrary::IsAbilityUsable(0, 100, 100));
	TestFalse(TEXT("energia insufficiente"), URTCombatLibrary::IsAbilityUsable(0, 99, 100));
	TestFalse(TEXT("in ricarica anche con energia"), URTCombatLibrary::IsAbilityUsable(1, 100, 100));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIntentVisibilityTest,
	"RefactorTactics.Combat.IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIntentVisibilityTest::RunTest(const FString&)
{
	// Alleati (stessa squadra): il piano e' sempre visibile, rivelato o no.
	TestTrue(TEXT("alleato vede il piano"), URTCombatLibrary::IsIntentVisibleTo(0, 0, false));
	TestTrue(TEXT("alleato vede anche se rivelato"), URTCombatLibrary::IsIntentVisibleTo(0, 0, true));
	// Avversari (squadra diversa): visibile solo se il proprietario e' rivelato (invariante #6).
	TestFalse(TEXT("nemico NON vede il piano privato"), URTCombatLibrary::IsIntentVisibleTo(1, 0, false));
	TestTrue(TEXT("nemico vede il piano rivelato"), URTCombatLibrary::IsIntentVisibleTo(1, 0, true));
	TestFalse(TEXT("nemico (altra squadra) non rivelato"), URTCombatLibrary::IsIntentVisibleTo(0, 1, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEffectiveAttackPowerTest,
	"RefactorTactics.Combat.EffectiveAttackPowerWithTerrainBonus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEffectiveAttackPowerTest::RunTest(const FString&)
{
	TestEqual(TEXT("altura +10 -> 40"), URTCombatLibrary::EffectiveAttackPower(30, 10), 40);
	TestEqual(TEXT("nessun bonus -> base"), URTCombatLibrary::EffectiveAttackPower(30, 0), 30);
	TestEqual(TEXT("malus enorme -> clamp 0"), URTCombatLibrary::EffectiveAttackPower(10, -30), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNewlyDefeatedTest,
	"RefactorTactics.Combat.NewlyDefeatedDetectsFreshDeaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNewlyDefeatedTest::RunTest(const FString&)
{
	// Una sola unita' passa da viva a morta -> il suo indice.
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 100, 100 }, { 100, 0 });
		TestEqual(TEXT("una morte, indice 1"), D.Num(), 1);
		TestTrue(TEXT("indice corretto"), D.Num() == 1 && D[0] == 1);
	}
	// Chi era gia' morto PRIMA non e' "appena eliminato".
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 0, 100 }, { 0, 100 });
		TestEqual(TEXT("gia' morta -> nessuna nuova morte"), D.Num(), 0);
	}
	// HP negativi contano come morte.
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 100 }, { -5 });
		TestTrue(TEXT("HP negativi -> morta"), D.Num() == 1 && D[0] == 0);
	}
	// Morti multiple, indici in ordine.
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 50, 50, 50 }, { 0, 50, 0 });
		TestTrue(TEXT("due morti agli indici 0 e 2"), D.Num() == 2 && D[0] == 0 && D[1] == 2);
	}
	// Lunghezze diverse: itera fino al minimo, senza crash.
	{
		const TArray<int32> D = URTCombatLibrary::NewlyDefeated({ 100, 100, 100 }, { 0 });
		TestTrue(TEXT("solo l'indice 0 confrontabile"), D.Num() == 1 && D[0] == 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnockbackTest,
	"RefactorTactics.Combat.KnockbackPushesTargetAwayBlockedByObstaclesAndEdges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnockbackTest::RunTest(const FString&)
{
	const TArray<FRTGridCoord> NoBlock;
	// Spinta in spazio aperto lungo +Y di 2 celle: (5,6) -> (5,8).
	TestTrue(TEXT("spinta verticale libera"),
		URTCombatLibrary::KnockbackDestination(FRTGridCoord(5, 5), FRTGridCoord(5, 6), 2, NoBlock, 10, 10) == FRTGridCoord(5, 8));
	// Spinta orizzontale +X di 2: (6,5) -> (8,5).
	TestTrue(TEXT("spinta orizzontale libera"),
		URTCombatLibrary::KnockbackDestination(FRTGridCoord(5, 5), FRTGridCoord(6, 5), 2, NoBlock, 10, 10) == FRTGridCoord(8, 5));
	// Ostacolo in (5,8): la spinta si ferma prima, a (5,7).
	{
		const TArray<FRTGridCoord> Block = { FRTGridCoord(5, 8) };
		TestTrue(TEXT("si ferma prima dell'ostacolo"),
			URTCombatLibrary::KnockbackDestination(FRTGridCoord(5, 5), FRTGridCoord(5, 6), 3, Block, 10, 10) == FRTGridCoord(5, 7));
	}
	// Bordo della griglia: da (0,8) spinta +Y su 10x10 si ferma a (0,9).
	TestTrue(TEXT("si ferma al bordo"),
		URTCombatLibrary::KnockbackDestination(FRTGridCoord(0, 7), FRTGridCoord(0, 8), 5, NoBlock, 10, 10) == FRTGridCoord(0, 9));
	// Distanza 0 -> resta.
	TestTrue(TEXT("distanza 0 -> fermo"),
		URTCombatLibrary::KnockbackDestination(FRTGridCoord(5, 5), FRTGridCoord(5, 6), 0, NoBlock, 10, 10) == FRTGridCoord(5, 6));
	// Attaccante sulla stessa cella del bersaglio -> nessuna direzione, resta.
	TestTrue(TEXT("stessa cella -> fermo"),
		URTCombatLibrary::KnockbackDestination(FRTGridCoord(5, 5), FRTGridCoord(5, 5), 2, NoBlock, 10, 10) == FRTGridCoord(5, 5));
	// Direzione = asse col delta maggiore: dx=2,dy=1 -> spinta lungo +X: (7,6) -> (9,6).
	TestTrue(TEXT("spinge lungo l'asse dominante (X)"),
		URTCombatLibrary::KnockbackDestination(FRTGridCoord(5, 5), FRTGridCoord(7, 6), 2, NoBlock, 10, 10) == FRTGridCoord(9, 6));
	// Preserva il layer del bersaglio (spinta orizzontale sullo stesso piano).
	{
		const FRTGridCoord Dest = URTCombatLibrary::KnockbackDestination(FRTGridCoord(3, 4, 1), FRTGridCoord(4, 4, 1), 1, NoBlock, 10, 10);
		TestTrue(TEXT("mantiene il layer"), Dest == FRTGridCoord(5, 4, 1));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTClassifyCombatOutcomeTest,
	"RefactorTactics.Combat.ClassifyCombatOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTClassifyCombatOutcomeTest::RunTest(const FString&)
{
	// ShieldAbsorbed: HP invariati (lo scudo ha assorbito tutto).
	TestTrue(TEXT("shield"), URTCombatLibrary::ClassifyCombatOutcome(100, 100, 0) == ERTCombatOutcome::ShieldAbsorbed);
	// Hit: HP calano, nessun bonus, non letale.
	TestTrue(TEXT("hit"), URTCombatLibrary::ClassifyCombatOutcome(100, 70, 0) == ERTCombatOutcome::Hit);
	// TerrainBonus: HP calano, bonus altura > 0, non letale.
	TestTrue(TEXT("terrain"), URTCombatLibrary::ClassifyCombatOutcome(100, 55, 15) == ERTCombatOutcome::TerrainBonus);
	// Lethal: HP a 0 (priorita' su tutto).
	TestTrue(TEXT("lethal"), URTCombatLibrary::ClassifyCombatOutcome(30, 0, 0) == ERTCombatOutcome::Lethal);
	// Lethal ha priorita' sul bonus altura.
	TestTrue(TEXT("lethal>terrain"), URTCombatLibrary::ClassifyCombatOutcome(30, 0, 15) == ERTCombatOutcome::Lethal);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
