#include "Misc/AutomationTest.h"
#include "Bot/RTBotLibrary.h"
#include "Core/RTTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTBotContext BotCtx(const TArray<FRTGridCoord>& Enemies, const TArray<int32>& Ranges, int32 Kite)
	{
		FRTBotContext C;
		C.Enemies = Enemies;
		C.EnemyRanges = Ranges;
		C.KiteStandoff = Kite;
		return C;
	}

	FRTBotPlan AttackPlan(const FRTGridCoord& Dest, int32 Damage, int32 TargetHP)
	{
		FRTBotPlan P;
		P.DestCell = Dest;
		P.bHasAttack = true;
		P.AttackDamage = Damage;
		P.TargetHealth = TargetHP;
		return P;
	}

	FRTBotPlan MovePlan(const FRTGridCoord& Dest)
	{
		FRTBotPlan P;
		P.DestCell = Dest;
		return P;
	}
}

// Un attacco che uccide (danno >= HP bersaglio) vale piu' di uno che non uccide (focus-fire).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotScoreKillTest,
	"RefactorTactics.Bot.ScorePrefersKill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotScoreKillTest::RunTest(const FString&)
{
	const FRTBotContext Ctx = BotCtx({}, {}, 0);
	const FRTBotPlan Kill   = AttackPlan(FRTGridCoord(0, 0), 50, 40);
	const FRTBotPlan NoKill = AttackPlan(FRTGridCoord(0, 0), 50, 100);
	TestTrue(TEXT("kill > non-kill"),
		URTBotLibrary::ScorePlan(Kill, Ctx) > URTBotLibrary::ScorePlan(NoKill, Ctx));
	return true;
}

// A parita' d'altro, piu' danno = punteggio maggiore.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotScoreDamageTest,
	"RefactorTactics.Bot.ScorePrefersMoreDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotScoreDamageTest::RunTest(const FString&)
{
	const FRTBotContext Ctx = BotCtx({}, {}, 0);
	const FRTBotPlan Hi = AttackPlan(FRTGridCoord(0, 0), 60, 100);
	const FRTBotPlan Lo = AttackPlan(FRTGridCoord(0, 0), 30, 100);
	TestTrue(TEXT("piu' danno meglio"),
		URTBotLibrary::ScorePlan(Hi, Ctx) > URTBotLibrary::ScorePlan(Lo, Ctx));
	return true;
}

// Una cella minacciata (nemico entro gittata) vale meno di una sicura.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotScoreThreatTest,
	"RefactorTactics.Bot.ScorePenalizesThreatenedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotScoreThreatTest::RunTest(const FString&)
{
	const FRTBotContext Ctx = BotCtx({ FRTGridCoord(5, 5) }, { 3 }, 0); // nemico gittata 3
	const FRTBotPlan Safe   = MovePlan(FRTGridCoord(0, 0)); // dist 10 > 3 -> sicura
	const FRTBotPlan Danger = MovePlan(FRTGridCoord(5, 7)); // dist 2 <= 3 -> minacciata
	TestTrue(TEXT("sicura > minacciata"),
		URTBotLibrary::ScorePlan(Safe, Ctx) > URTBotLibrary::ScorePlan(Danger, Ctx));
	return true;
}

// Kiter (standoff > 0): preferisce la cella oltre la distanza di sicurezza.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotScoreKiteTest,
	"RefactorTactics.Bot.KiterPrefersStandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotScoreKiteTest::RunTest(const FString&)
{
	const FRTBotContext Ctx = BotCtx({ FRTGridCoord(0, 0) }, { 1 }, 4); // kiter, standoff 4
	const FRTBotPlan Far   = MovePlan(FRTGridCoord(5, 0)); // dist 5 >= 4 -> ok
	const FRTBotPlan Close = MovePlan(FRTGridCoord(2, 0)); // dist 2 < 4 -> violazione
	TestTrue(TEXT("kiter: lontano > vicino"),
		URTBotLibrary::ScorePlan(Far, Ctx) > URTBotLibrary::ScorePlan(Close, Ctx));
	return true;
}

// Mischia (standoff 0): preferisce avvicinarsi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotScoreApproachTest,
	"RefactorTactics.Bot.MeleePrefersClosing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotScoreApproachTest::RunTest(const FString&)
{
	const FRTBotContext Ctx = BotCtx({ FRTGridCoord(0, 0) }, { 1 }, 0); // mischia
	const FRTBotPlan Near = MovePlan(FRTGridCoord(2, 0)); // dist 2
	const FRTBotPlan Far  = MovePlan(FRTGridCoord(6, 0)); // dist 6
	TestTrue(TEXT("mischia: vicino > lontano"),
		URTBotLibrary::ScorePlan(Near, Ctx) > URTBotLibrary::ScorePlan(Far, Ctx));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
