#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Combat/RTCombatLibrary.h" // LowCoverDamageReduction: il bonus direzionale si confronta col catalogo
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

	/**
	 * Contesto con attacco AD AREA di raggio 1 e fuoco amico attivo — il caso di `Flux.Overload` (#213).
	 * Gittata nemica 0: nessuna minaccia sulla cella, cosi' il punteggio isola il collaterale.
	 */
	FRTHexBotContext MakeAreaCtx(const FRTCellId& Origin, const FRTCellId& Enemy, int32 Damage)
	{
		FRTHexBotContext Ctx = MakeCtx(Origin, Enemy, /*EnemyRange*/ 0, /*EnemyHealth*/ 100);
		Ctx.AttackDamage = Damage;
		Ctx.AttackShape = ERTAbilityShape::Area;
		Ctx.AttackAreaRadius = 1;
		Ctx.bAttackFriendlyFire = true;
		return Ctx;
	}

	/** Piano con attacco AD AREA raggio 1 e fuoco amico: la forma viaggia col piano, non col contesto. */
	FRTHexBotPlan MakeAreaPlan(const FRTCellId& Dest, int32 Damage, int32 TargetHealth)
	{
		FRTHexBotPlan P;
		P.DestCell = Dest;
		P.bHasAttack = true;
		P.TargetIndex = 0;
		P.AttackDamage = Damage;
		P.TargetHealth = TargetHealth;
		P.Shape = ERTAbilityShape::Area;
		P.AreaRadius = 1;
		P.bFriendlyFire = true;
		return P;
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
// Forma dell'attacco e collaterale (#213)
//
// ScorePlan valutava ogni attacco come se colpisse una cella sola: non contava i nemici in piu' presi da
// un'area, ne' gli alleati che ci finiscono dentro. Con il fuoco amico attivo di default (2026-08-08) la
// seconda meta' dell'omissione e' diventata dannosa, non solo conservativa.
//
// Il modello segue il resolver, non un'idea del resolver: `CollectHexAttacks` esclude SEMPRE l'attaccante
// (`u == Intent.AttackerId`) e colpisce un alleato solo se l'azione dichiara `bFriendlyFire`.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotAreaExtraEnemiesTest,
	"RefactorTactics.HexBot.ScoreAreaCountsExtraEnemies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotAreaExtraEnemiesTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);

	// Un solo nemico nell'area.
	const FRTHexBotContext One = MakeAreaCtx(Origin, Target, /*damage*/ 30);

	// Un secondo nemico ADIACENTE al bersaglio: entra nell'esagono di raggio 1, ma non cambia MinDist
	// (2 contro 3) ne' la minaccia (gittata 0), quindi l'unica differenza di punteggio e' il collaterale.
	FRTHexBotContext Two = One;
	Two.Enemies.Add(FRTCellId(3, 0));
	Two.EnemyRanges.Add(0);
	Two.EnemyHealth.Add(100);

	const int32 ScoreOne = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), One);
	const int32 ScoreTwo = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), Two);

	TestTrue(TEXT("l'area che prende due nemici vale piu' di quella che ne prende uno"), ScoreTwo > ScoreOne);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotAreaPenalizesAllyTest,
	"RefactorTactics.HexBot.ScoreAreaPenalizesAlly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotAreaPenalizesAllyTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);

	const FRTHexBotContext Clean = MakeAreaCtx(Origin, Target, /*damage*/ 30);

	FRTHexBotContext WithAlly = Clean;
	WithAlly.Allies.Add(FRTCellId(3, 0)); // adiacente al bersaglio: dentro l'area
	WithAlly.AllyHealth.Add(100);

	const int32 ScoreClean = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), Clean);
	const int32 ScoreAlly = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), WithAlly);

	TestTrue(TEXT("prendere il compagno nell'area abbassa il punteggio"), ScoreAlly < ScoreClean);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotAllyPenaltyScalesTest,
	"RefactorTactics.HexBot.ScoreAllyPenaltyScalesWithDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotAllyPenaltyScalesTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);
	const FRTCellId Ally(3, 0);

	auto PenaltyForDamage = [&](int32 Damage)
	{
		const FRTHexBotContext Clean = MakeAreaCtx(Origin, Target, Damage);
		FRTHexBotContext WithAlly = Clean;
		WithAlly.Allies.Add(Ally);
		WithAlly.AllyHealth.Add(100);

		return URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, Damage, 100), Clean)
			- URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, Damage, 100), WithAlly);
	};

	// La decisione di design e' PENALITA' PROPORZIONALE, non veto: ferire il compagno per prendere due
	// nemici resta una scelta legittima, ma costa in proporzione a quanto lo si ferisce.
	TestTrue(TEXT("la penalita' e' proporzionale al danno, non una costante"),
		PenaltyForDamage(40) > PenaltyForDamage(20));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotNoFriendlyFireTest,
	"RefactorTactics.HexBot.ScoreIgnoresAllyWithoutFriendlyFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotNoFriendlyFireTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);

	const FRTHexBotContext Clean = MakeAreaCtx(Origin, Target, /*damage*/ 30);

	FRTHexBotContext NoFire = Clean;
	NoFire.bAttackFriendlyFire = false;
	NoFire.Allies.Add(FRTCellId(3, 0));
	NoFire.AllyHealth.Add(100);

	FRTHexBotPlan NoFirePlan = MakeAreaPlan(Origin, 30, 100);
	NoFirePlan.bFriendlyFire = false; // l'azione non colpisce gli alleati: il piano lo porta con se'

	const int32 ScoreClean = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), Clean);
	const int32 ScoreNoFire = URTHexBotLibrary::ScorePlan(M, NoFirePlan, NoFire);

	// Il resolver non tocca l'alleato: penalizzarlo qui renderebbe il bot timido su un pericolo che non esiste.
	TestEqual(TEXT("senza fuoco amico l'alleato nell'area non entra nel punteggio"), ScoreNoFire, ScoreClean);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotSingleShapeTest,
	"RefactorTactics.HexBot.ScoreSingleShapeIgnoresNeighbours",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotSingleShapeTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);

	// Forma Single: il vicino del bersaglio non viene colpito, ne' se nemico ne' se alleato.
	FRTHexBotContext One = MakeCtx(Origin, Target, /*range*/ 0, /*hp*/ 100);
	One.AttackDamage = 30;

	FRTHexBotContext Two = One;
	Two.Enemies.Add(FRTCellId(3, 0));
	Two.EnemyRanges.Add(0);
	Two.EnemyHealth.Add(100);

	const int32 ScoreOne = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 30, 100), One);
	const int32 ScoreTwo = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 30, 100), Two);

	TestEqual(TEXT("Single non guadagna nulla dai vicini del bersaglio"), ScoreTwo, ScoreOne);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotCandidateShapeTest,
	"RefactorTactics.HexBot.CandidatesCarryShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotCandidateShapeTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 1));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTHexBotContext Ctx = MakeAreaCtx(FRTCellId(0, 0), FRTCellId(1, 0), /*damage*/ 18);
	Ctx.AttackRange = 3;

	const TArray<FRTHexBotPlan> Candidates = URTHexBotLibrary::BuildCandidates(Snap, 1, Ctx);

	// Il primo anello del cablaggio: la forma passa dal contesto AL PIANO. Se restasse nel contesto,
	// ChooseBestPlan — che confronta in una lista sola le candidate di abilita' diverse — la applicherebbe
	// a tutte.
	int32 Attacks = 0;
	for (const FRTHexBotPlan& P : Candidates)
	{
		if (!P.bHasAttack)
		{
			continue;
		}
		++Attacks;
		TestEqual(TEXT("la candidata porta la forma dell'azione"), P.Shape, ERTAbilityShape::Area);
		TestEqual(TEXT("la candidata porta il raggio dell'area"), P.AreaRadius, 1);
		TestEqual(TEXT("la candidata porta la gittata"), P.RangeCells, 3);
		TestTrue(TEXT("la candidata porta il fuoco amico"), P.bFriendlyFire);
	}
	TestTrue(TEXT("almeno una candidata con attacco, o il ciclo non verifica nulla"), Attacks > 0);
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

// ---------------------------------------------------------------------------------------------------------
// Fuga (kiting in panico): la guardia che il quadrato risolve con BestKiteCell
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotKiteCellTest,
	"RefactorTactics.HexBot.KiteCellMaximizesDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotKiteCellTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	const FRTCellId Threat(2, 0);
	const FRTCellId Flee = URTHexBotLibrary::BestKiteCell(Snap, /*UnitId*/ 1, Threat);

	TestTrue(TEXT("si allontana dalla minaccia"),
		URTHexLibrary::HexDistance(Flee, Threat) > URTHexLibrary::HexDistance(FRTCellId(0, 0), Threat));
	TestTrue(TEXT("resta entro il budget"), URTHexLibrary::HexDistance(Flee, FRTCellId(0, 0)) <= 2);
	TestTrue(TEXT("massimizza la distanza raggiungibile"), URTHexLibrary::HexDistance(Flee, Threat) == 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotKiteCellLegalTest,
	"RefactorTactics.HexBot.KiteCellStaysLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotKiteCellLegalTest::RunTest(const FString&)
{
	// Mappa STRETTA: la fuga non deve proporre celle inesistenti, bloccate od occupate.
	URTHexMapAsset* M = MakeBotMap(1); // solo l'origine e i suoi 6 vicini

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 3));
	Units.Add(FRTHexSimUnit(2, FRTCellId(-1, 0), /*budget*/ 0)); // alleato fermo sulla via di fuga
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	const FRTCellId Flee = URTHexBotLibrary::BestKiteCell(Snap, /*UnitId*/ 1, FRTCellId(1, 0));

	TestTrue(TEXT("la cella di fuga esiste nella mappa"), M->ContainsCell(Flee));
	TestFalse(TEXT("non fugge dentro un'altra unita'"), Flee == FRTCellId(-1, 0));

	// Unita' immobile: non c'e' fuga possibile, resta dov'e' (nessuna mossa illegale, nessun crash).
	TArray<FRTHexSimUnit> Stuck;
	Stuck.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 0));
	const FRTHexSnapshot StuckSnap = URTHexSimLibrary::MakeSnapshot(M, Stuck);
	TestTrue(TEXT("senza budget resta dov'e'"),
		URTHexBotLibrary::BestKiteCell(StuckSnap, 1, FRTCellId(1, 0)) == FRTCellId(0, 0));
	return true;
}


// ---------------------------------------------------------------------------------------------------------
// CP 13.5 / ADR-0005 — l'ORIENTAMENTO nel punteggio delle candidate.
//
// I tre test qui sotto isolano il facing tenendo TUTTO il resto identico: due contesti che differiscono per
// il solo orientamento del bersaglio, e lo stesso piano. E' l'unico allestimento in cui una differenza di
// punteggio non possa venire da altro — distanza, minaccia e danno sono gli stessi per costruzione.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Copertura bassa sul bordo indicato: la stessa forma che usano i test di `RTHexCoverTests`. */
	void SetBotLowCover(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Bot in `Origin`, un solo nemico in `Enemy` con l'orientamento dato. Nessuna minaccia: gittata 0. */
	FRTHexBotContext MakeFacingCtx(const FRTCellId& Origin, const FRTCellId& Enemy, ERTHexDirection EnemyFacing)
	{
		FRTHexBotContext Ctx;
		Ctx.Origin = Origin;
		Ctx.SelfFacing = ERTHexDirection::E;
		Ctx.Enemies.Add(Enemy);
		Ctx.EnemyRanges.Add(0);
		Ctx.EnemyHealth.Add(1000); // alto: nessun colpo letale, cosi' `WKill` non entra nel confronto
		Ctx.EnemyFacings.Add(EnemyFacing);
		Ctx.AttackRange = 4;
		Ctx.AttackDamage = 20;
		return Ctx;
	}

	/** Attacco singolo sul nemico 0, sferrato restando fermi in `Origin`. */
	FRTHexBotPlan MakeFacingPlan(const FRTCellId& Origin)
	{
		FRTHexBotPlan Plan;
		Plan.DestCell = Origin;
		Plan.FromCell = Origin; // fermo: il facing non deriva dal movimento
		Plan.bHasAttack = true;
		Plan.TargetIndex = 0;
		Plan.AttackDamage = 20;
		Plan.TargetHealth = 1000;
		Plan.Shape = ERTAbilityShape::Single;
		Plan.RangeCells = 4;
		return Plan;
	}
}

/**
 * Il bot preferisce colpire il lato SCOPERTO: fra due bersagli identici, quello che gli offre il fianco vale
 * di piu', perche' la sua copertura non lo protegge da li' (CP 16.2).
 *
 * Il nemico sta a EST del bot e ha una copertura sul bordo OVEST, cioe' quella rivolta al bot. Se guarda a
 * ovest il colpo e' frontale e la copertura tiene; se guarda a est ha voltato le spalle e la copertura non
 * vale piu' niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotExposedRearArcTest,
	"RefactorTactics.HexBot.ConsidersExposedRearArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotExposedRearArcTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Enemy(2, 0);
	SetBotLowCover(M, Enemy, ERTHexDirection::W); // il lato da cui il bot spara

	const int32 Facing = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::W)); // guarda il bot: coperto
	const int32 Exposed = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::E)); // gli volta le spalle: scoperto

	TestTrue(TEXT("colpire il bersaglio che offre il fianco vale piu' che colpirlo di fronte"), Exposed > Facing);
	return true;
}

/**
 * Il termine direzionale vale ESATTAMENTE il danno che la direzione aggiunge: `WDamage x riduzione scavalcata`.
 *
 * E' il test che impedisce al termine di diventare un peso libero. Un peso nuovo, in questo modello, si tara
 * contro una scala che `#149` ha gia' misurato come rotta — e la lezione di quella issue e' che non esiste un
 * valore che funzioni. Qui non c'e' niente da tarare: il numero viene dal catalogo di combattimento, e se
 * qualcuno lo sostituisse con una costante questo test cadrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotRearBonusValueTest,
	"RefactorTactics.HexBot.RearBonusMatchesBypassedReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotRearBonusValueTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Enemy(2, 0);
	SetBotLowCover(M, Enemy, ERTHexDirection::W);

	const FRTHexBotContext Covered = MakeFacingCtx(Origin, Enemy, ERTHexDirection::W);
	const int32 Facing = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin), Covered);
	const int32 Exposed = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::E));

	TestEqual(TEXT("il bonus e' WDamage x la riduzione annullata, non un numero suo"),
		Exposed - Facing, Covered.WDamage * URTCombatLibrary::LowCoverDamageReduction);
	return true;
}

/**
 * Senza una protezione da scavalcare, l'orientamento non muove il punteggio di un punto.
 *
 * ⚠️ **Asserisce uno ZERO che oggi e' la norma**, e per questo va letto insieme al suo motivo:
 * `RearHitBypassedCover` ANNULLA una riduzione, e dove non c'e' riduzione non c'e' niente da annullare. Il
 * giorno in cui il bot guadagnasse una protezione propria da mettere in gioco, questo test diventerebbe rosso
 * e chiederebbe di essere promosso — che e' esattamente il suo mestiere. Un residuo asserito non sparisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotExposureZeroTest,
	"RefactorTactics.HexBot.ExposureIsZeroWithoutProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotExposureZeroTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4); // arena liscia: nessuna copertura da nessuna parte
	const FRTCellId Origin(0, 0);
	const FRTCellId Enemy(2, 0);

	const int32 Facing = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::W));
	const int32 Exposed = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::E));

	TestEqual(TEXT("senza copertura l'orientamento non cambia il punteggio"), Exposed, Facing);

	// E il verso difensivo, dallo stesso allestimento: un nemico che minaccia davvero la cella, ma nessuna
	// copertura da perdere. La penalita' resta quella di sempre, `WThreat`, senza aggiunte direzionali.
	FRTHexBotContext Threat = MakeFacingCtx(Origin, Enemy, ERTHexDirection::W);
	Threat.EnemyRanges[0] = 4; // ora ci arriva
	FRTHexBotContext ThreatExposed = Threat;
	ThreatExposed.EnemyFacings[0] = ERTHexDirection::E;

	TestEqual(TEXT("nemmeno sul lato difensivo, finche' non c'e' copertura da perdere"),
		URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin), ThreatExposed),
		URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin), Threat));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
