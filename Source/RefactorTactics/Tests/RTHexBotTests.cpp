#include "Misc/AutomationTest.h"
#include "Bot/RTHexBotLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N sul layer 0. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeBotMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	void BlockBotSight(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.Id = Id;
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Contesto minimo: un nemico con gittata e HP dati. */
	FRTHexBotContext MakeCtx(const FRTCellId& Origin, const FRTCellId& Enemy, int32 EnemyRange, int32 EnemyHealth)
	{
		FRTHexBotContext Ctx;
		Ctx.Origin = Origin;
		Ctx.Enemies.Add(Enemy);
		Ctx.EnemyRanges.Add(EnemyRange);
		Ctx.EnemyHealth.Add(EnemyHealth);
		Ctx.AttackRange = 3;
		Ctx.AttackDamage = 30;
		return Ctx;
	}

	FRTHexBotPlan MakePlan(const FRTCellId& Dest, bool bAttack = false, int32 Damage = 0, int32 TargetHealth = 0)
	{
		FRTHexBotPlan P;
		P.DestCell = Dest;
		P.bHasAttack = bAttack;
		P.TargetIndex = bAttack ? 0 : INDEX_NONE;
		P.AttackDamage = Damage;
		P.TargetHealth = TargetHealth;
		return P;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Scoring
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotFocusFireTest,
	"RefactorTactics.HexBot.ScoreFocusFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotFocusFireTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Enemy(3, 0);
	const FRTHexBotContext Ctx = MakeCtx(Origin, Enemy, /*range*/ 0, /*hp*/ 100);

	const int32 NoAttack = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin), Ctx);
	const int32 Weak = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 20, 100), Ctx);
	const int32 Strong = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 40, 100), Ctx);
	const int32 Lethal = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 40, 35), Ctx);

	TestTrue(TEXT("attaccare batte non attaccare"), Weak > NoAttack);
	TestTrue(TEXT("piu' danno vale di piu'"), Strong > Weak);
	TestTrue(TEXT("il colpo letale domina"), Lethal > Strong);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotCoverTest,
	"RefactorTactics.HexBot.ScoreThreatRespectsCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotCoverTest::RunTest(const FString&)
{
	const FRTCellId Exposed(0, 0);
	const FRTCellId Enemy(3, 0);
	FRTHexBotContext Ctx = MakeCtx(Exposed, Enemy, /*range*/ 5, /*hp*/ 100);
	Ctx.KiteStandoff = 0;
	Ctx.WApproach = 0; // isola il contributo della minaccia

	URTHexMapAsset* Open = MakeBotMap(4);
	const int32 UnderFire = URTHexBotLibrary::ScorePlan(Open, MakePlan(Exposed), Ctx);

	URTHexMapAsset* Covered = MakeBotMap(4);
	BlockBotSight(Covered, FRTCellId(2, 0)); // muro fra nemico e cella
	const int32 BehindCover = URTHexBotLibrary::ScorePlan(Covered, MakePlan(Exposed), Ctx);

	TestTrue(TEXT("la cella sotto tiro e' penalizzata"), UnderFire < 0);
	TestEqual(TEXT("dietro copertura nessuna penalita' di minaccia"), BehindCover, 0);
	TestTrue(TEXT("la copertura migliora il punteggio"), BehindCover > UnderFire);

	// Fuori dalla gittata nemica non c'e' minaccia, anche senza copertura.
	FRTHexBotContext ShortRange = Ctx;
	ShortRange.EnemyRanges[0] = 1;
	TestEqual(TEXT("fuori gittata nessuna minaccia"),
		URTHexBotLibrary::ScorePlan(Open, MakePlan(Exposed), ShortRange), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotKiterVsMeleeTest,
	"RefactorTactics.HexBot.ScoreKiterVsMelee",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotKiterVsMeleeTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);
	const FRTCellId Enemy(4, 0);
	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), Enemy, /*range*/ 0, /*hp*/ 100);

	// Mischia: piu' vicino = meglio.
	Ctx.KiteStandoff = 0;
	const int32 MeleeNear = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(3, 0)), Ctx);
	const int32 MeleeFar = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(1, 0)), Ctx);
	TestTrue(TEXT("la mischia preferisce chiudere la distanza"), MeleeNear > MeleeFar);

	// Kiter con standoff 3: sotto la soglia viene penalizzato in proporzione.
	Ctx.KiteStandoff = 3;
	const int32 KiterAtStandoff = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(1, 0)), Ctx); // dist 3
	const int32 KiterTooClose = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(3, 0)), Ctx);   // dist 1
	TestTrue(TEXT("il kiter preferisce restare alla distanza di sicurezza"), KiterAtStandoff > KiterTooClose);
	TestEqual(TEXT("alla distanza di sicurezza nessuna penalita'"), KiterAtStandoff, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotElevationTest,
	"RefactorTactics.HexBot.ScoreElevationBonus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotElevationTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(3);
	FRTHexBotContext Ctx;
	Ctx.Origin = FRTCellId(0, 0);
	Ctx.WElevation = 20;

	const int32 Ground = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(1, 0, 0)), Ctx);
	const int32 High = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(1, 0, 2)), Ctx);
	TestTrue(TEXT("la quota alta vale di piu'"), High > Ground);
	TestEqual(TEXT("bonus proporzionale al layer"), High - Ground, 40);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Scelta del piano
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotOrderIndependenceTest,
	"RefactorTactics.HexBot.ChooseBestPlanOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotOrderIndependenceTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	FRTHexBotContext Ctx = MakeCtx(Origin, FRTCellId(3, 0), /*range*/ 2, /*hp*/ 100);

	TArray<FRTHexBotPlan> A;
	A.Add(MakePlan(FRTCellId(1, 0)));
	A.Add(MakePlan(FRTCellId(0, 1), true, 30, 25)); // letale
	A.Add(MakePlan(FRTCellId(1, -1)));

	TArray<FRTHexBotPlan> B;
	B.Add(A[2]); B.Add(A[0]); B.Add(A[1]);

	const FRTHexBotPlan BestA = URTHexBotLibrary::ChooseBestPlan(M, A, Ctx);
	const FRTHexBotPlan BestB = URTHexBotLibrary::ChooseBestPlan(M, B, Ctx);

	TestTrue(TEXT("stessa scelta indipendentemente dall'ordine"), BestA.DestCell == BestB.DestCell);
	TestTrue(TEXT("sceglie il colpo letale"), BestA.bHasAttack && BestA.DestCell == FRTCellId(0, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotTieBreakTest,
	"RefactorTactics.HexBot.ChooseBestPlanTieBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotTieBreakTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	FRTHexBotContext Ctx;
	Ctx.Origin = Origin; // nessun nemico: tutte le candidate valgono 0

	TArray<FRTHexBotPlan> Candidates;
	Candidates.Add(MakePlan(FRTCellId(2, 0)));
	Candidates.Add(MakePlan(Origin));
	Candidates.Add(MakePlan(FRTCellId(1, 0)));

	const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(M, Candidates, Ctx);
	TestTrue(TEXT("a parita' di punteggio resta fermo"), Best.DestCell == Origin);

	// Senza la candidata "resta", vince la mossa minima; a parita' di distanza, l'ordine stabile.
	TArray<FRTHexBotPlan> Moves;
	Moves.Add(MakePlan(FRTCellId(0, 1)));
	Moves.Add(MakePlan(FRTCellId(1, 0)));
	Moves.Add(MakePlan(FRTCellId(2, 0)));
	const FRTHexBotPlan Nearest = URTHexBotLibrary::ChooseBestPlan(M, Moves, Ctx);
	TestEqual(TEXT("mossa minima"), URTHexLibrary::HexDistance(Nearest.DestCell, Origin), 1);
	TestTrue(TEXT("tie-break stabile fra celle equidistanti"), Nearest.DestCell == FRTCellId(0, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotEmptyPlanTest,
	"RefactorTactics.HexBot.ChooseBestPlanEmptyStays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotEmptyPlanTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(2);
	FRTHexBotContext Ctx;
	Ctx.Origin = FRTCellId(1, -1);

	const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(M, TArray<FRTHexBotPlan>(), Ctx);
	TestTrue(TEXT("nessuna candidata -> resta all'origine"), Best.DestCell == Ctx.Origin);
	TestFalse(TEXT("nessun attacco"), Best.bHasAttack);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Pianificazione completa
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotKillingShotTest,
	"RefactorTactics.HexBot.PlanUnitTakesKillingShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotKillingShotTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);

	// Bot a (0,0) con budget 2 e gittata 2; nemico a (4,0) con 10 HP: da (2,0) e' colpibile e muore.
	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), FRTCellId(4, 0), /*range*/ 1, /*hp*/ 10);
	Ctx.AttackRange = 2;
	Ctx.AttackDamage = 30;

	const TArray<FRTHexBotPlan> Candidates = URTHexBotLibrary::BuildCandidates(Snap, 1, Ctx);
	TestTrue(TEXT("candidate generate"), Candidates.Num() > 0);
	TestTrue(TEXT("esiste una candidata con attacco"),
		Candidates.ContainsByPredicate([](const FRTHexBotPlan& P) { return P.bHasAttack; }));

	const FRTHexBotPlan Best = URTHexBotLibrary::PlanUnit(Snap, 1, Ctx);
	TestTrue(TEXT("pianifica l'attacco"), Best.bHasAttack);
	TestEqual(TEXT("bersaglio corretto"), Best.TargetIndex, 0);
	TestTrue(TEXT("si porta in gittata"), URTHexLibrary::HexDistance(Best.DestCell, FRTCellId(4, 0)) <= 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotBudgetTest,
	"RefactorTactics.HexBot.PlanUnitRespectsBudgetAndOccupancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 1));
	Units.Add(FRTHexSimUnit(2, FRTCellId(1, 0), /*budget*/ 0)); // alleato fermo davanti
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), FRTCellId(4, 0), /*range*/ 1, /*hp*/ 100);
	Ctx.KiteStandoff = 0; // mischia: vuole avvicinarsi il piu' possibile

	const TArray<FRTHexBotPlan> Candidates = URTHexBotLibrary::BuildCandidates(Snap, 1, Ctx);
	// Senza questa asserzione il ciclo sotto non verificherebbe nulla su una lista vuota.
	TestTrue(TEXT("candidate generate (origine + celle a distanza 1 libere)"), Candidates.Num() >= 6);
	for (const FRTHexBotPlan& P : Candidates)
	{
		TestTrue(TEXT("nessuna candidata oltre il budget"), URTHexLibrary::HexDistance(P.DestCell, FRTCellId(0, 0)) <= 1);
		TestTrue(TEXT("nessuna candidata sulla cella occupata"), !(P.DestCell == FRTCellId(1, 0)));
	}

	const FRTHexBotPlan Best = URTHexBotLibrary::PlanUnit(Snap, 1, Ctx);
	TestTrue(TEXT("destinazione legale"), !(Best.DestCell == FRTCellId(1, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotSeeksCoverTest,
	"RefactorTactics.HexBot.PlanUnitSeeksCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotSeeksCoverTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);
	BlockBotSight(M, FRTCellId(1, 1)); // muro che copre la cella (0,2) dal nemico a (3,-1)

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), FRTCellId(3, -1), /*range*/ 6, /*hp*/ 100);
	Ctx.AttackRange = 0;      // non puo' rispondere: conta solo il posizionamento
	Ctx.AttackDamage = 0;
	Ctx.KiteStandoff = 0;
	Ctx.WApproach = 0;        // isola il contributo della copertura
	Ctx.WElevation = 0;

	// L'origine e' esposta: esiste quindi una scelta migliore da fare (senza questa verifica il test
	// passerebbe anche con un pianificatore che non fa nulla).
	TestTrue(TEXT("l'origine e' sotto tiro"), URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(0, 0)), Ctx) < 0);

	const FRTHexBotPlan Best = URTHexBotLibrary::PlanUnit(Snap, 1, Ctx);
	TestFalse(TEXT("non pianifica attacchi senza gittata"), Best.bHasAttack);
	TestFalse(TEXT("si sposta dall'origine esposta"), Best.DestCell == FRTCellId(0, 0));
	TestEqual(TEXT("la cella scelta e' al riparo"),
		URTHexBotLibrary::ScorePlan(M, MakePlan(Best.DestCell), Ctx), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
