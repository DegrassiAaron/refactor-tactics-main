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

// ChooseBestPlan: fra piu' candidate sceglie quella a punteggio massimo (qui l'attacco letale domina).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotChoosePicksMaxTest,
	"RefactorTactics.Bot.ChooseBestPlanPicksHighestScore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotChoosePicksMaxTest::RunTest(const FString&)
{
	FRTBotContext Ctx = BotCtx({}, {}, 0);
	Ctx.Origin = FRTGridCoord(0, 0);
	TArray<FRTBotPlan> Cands;
	Cands.Add(MovePlan(FRTGridCoord(1, 0)));             // solo movimento (score 0)
	Cands.Add(AttackPlan(FRTGridCoord(2, 0), 50, 40));   // uccide -> +WKill: domina
	const FRTBotPlan Best = URTBotLibrary::ChooseBestPlan(Cands, Ctx);
	TestTrue(TEXT("vince la candidata con attacco letale"),
		Best.bHasAttack && Best.DestCell == FRTGridCoord(2, 0));
	return true;
}

// ChooseBestPlan: a parita' di score, preferisce la mossa minima da Origin (tie-break assoluto).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotChooseTieBreakTest,
	"RefactorTactics.Bot.ChooseBestPlanTieBreakPrefersMinimalMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotChooseTieBreakTest::RunTest(const FString&)
{
	FRTBotContext Ctx = BotCtx({ FRTGridCoord(0, 10) }, { 0 }, 0); // mischia; nemico gittata 0 (non minaccia)
	Ctx.Origin = FRTGridCoord(0, 0);
	TArray<FRTBotPlan> Cands;
	Cands.Add(MovePlan(FRTGridCoord(6, 10))); // dist dal nemico 6, mossa da Origin 16
	Cands.Add(MovePlan(FRTGridCoord(0, 4)));  // dist dal nemico 6, mossa da Origin 4 -> vince
	const FRTBotPlan Best = URTBotLibrary::ChooseBestPlan(Cands, Ctx);
	TestTrue(TEXT("a parita' di score, mossa minima da Origin"),
		Best.DestCell == FRTGridCoord(0, 4));
	return true;
}

// ChooseBestPlan: permutare l'ordine delle candidate non cambia la scelta (determinismo, invariante #4).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotChoosePermInvariantTest,
	"RefactorTactics.Bot.ChooseBestPlanIsPermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotChoosePermInvariantTest::RunTest(const FString&)
{
	FRTBotContext Ctx = BotCtx({ FRTGridCoord(0, 10) }, { 0 }, 0);
	Ctx.Origin = FRTGridCoord(0, 0);
	const FRTBotPlan A = MovePlan(FRTGridCoord(6, 10)); // mossa 16
	const FRTBotPlan B = MovePlan(FRTGridCoord(0, 4));  // mossa 4 -> atteso in entrambi gli ordini
	TArray<FRTBotPlan> Fwd; Fwd.Add(A); Fwd.Add(B);
	TArray<FRTBotPlan> Rev; Rev.Add(B); Rev.Add(A);
	const FRTBotPlan R1 = URTBotLibrary::ChooseBestPlan(Fwd, Ctx);
	const FRTBotPlan R2 = URTBotLibrary::ChooseBestPlan(Rev, Ctx);
	TestTrue(TEXT("permutare l'input non cambia la scelta"),
		R1.DestCell == FRTGridCoord(0, 4) && R2.DestCell == FRTGridCoord(0, 4));
	return true;
}

// ChooseBestPlan: senza candidate, ripiega sul "resta a Origin".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotChooseEmptyTest,
	"RefactorTactics.Bot.ChooseBestPlanEmptyFallsBackToOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotChooseEmptyTest::RunTest(const FString&)
{
	FRTBotContext Ctx = BotCtx({}, {}, 0);
	Ctx.Origin = FRTGridCoord(4, 2);
	const TArray<FRTBotPlan> None;
	const FRTBotPlan Best = URTBotLibrary::ChooseBestPlan(None, Ctx);
	TestTrue(TEXT("nessuna candidata -> resta a Origin"),
		Best.DestCell == FRTGridCoord(4, 2));
	return true;
}

// AttackIsBetter: preferisce l'AttackScore maggiore (un kill batte un non-kill, coord irrilevanti).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAttackBetterScoreTest,
	"RefactorTactics.Bot.AttackIsBetterPrefersHigherScore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAttackBetterScoreTest::RunTest(const FString&)
{
	// A = kill (dmg 50 >= hp 40); B = non-kill (dmg 50 < hp 100). A è migliore anche con coord "peggiori".
	TestTrue(TEXT("kill > non-kill"),
		URTBotLibrary::AttackIsBetter(50, 40, FRTGridCoord(9, 9), 50, 100, FRTGridCoord(0, 0)));
	return true;
}

// AttackIsBetter: a parità di AttackScore, vince il bersaglio con coord minori, in modo ANTISIMMETRICO
// (tie-break assoluto -> scelta indipendente dall'ordine di iterazione, invariante #4).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAttackBetterTieBreakTest,
	"RefactorTactics.Bot.AttackIsBetterTieBreakByCoord",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAttackBetterTieBreakTest::RunTest(const FString&)
{
	// Stesso danno/HP -> stesso AttackScore; decide la coord del bersaglio.
	const bool LowBeatsHigh = URTBotLibrary::AttackIsBetter(30, 100, FRTGridCoord(1, 0), 30, 100, FRTGridCoord(2, 0));
	const bool HighBeatsLow = URTBotLibrary::AttackIsBetter(30, 100, FRTGridCoord(2, 0), 30, 100, FRTGridCoord(1, 0));
	TestTrue(TEXT("coord minore preferita, antisimmetrico"), LowBeatsHigh && !HighBeatsLow);
	return true;
}

// Elevazione: a parità d'altro (nessun nemico), una cella in quota più alta è preferita.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotScoreElevationTest,
	"RefactorTactics.Bot.ScorePrefersHigherElevation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotScoreElevationTest::RunTest(const FString&)
{
	const FRTBotContext Ctx = BotCtx({}, {}, 0); // nessun nemico: isola il fattore quota
	const FRTBotPlan High = MovePlan(FRTGridCoord(0, 0, 1)); // Layer 1
	const FRTBotPlan Low  = MovePlan(FRTGridCoord(0, 0, 0)); // Layer 0
	TestTrue(TEXT("quota alta > quota bassa"),
		URTBotLibrary::ScorePlan(High, Ctx) > URTBotLibrary::ScorePlan(Low, Ctx));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
