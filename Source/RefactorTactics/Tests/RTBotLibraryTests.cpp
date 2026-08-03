#include "Misc/AutomationTest.h"
#include "Bot/RTBotLibrary.h"
#include "Grid/RTGridLibrary.h"
#include "Core/RTTypes.h"
#include "Terrain/RTTerrainTypes.h" // RT_BLOCKED_COST

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Cost map dove le celle indicate sono impassabili (per i test del bot pesato).
	TMap<FRTGridCoord, int32> BlockedCost(const TArray<FRTGridCoord>& Blockers)
	{
		TMap<FRTGridCoord, int32> Cost;
		for (const FRTGridCoord& C : Blockers) { Cost.Add(C, RT_BLOCKED_COST); }
		return Cost;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStepFarTest,
	"RefactorTactics.Bot.StepTowardConsumesFullRangeWhenFar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotStepFarTest::RunTest(const FString&)
{
	// (0,0) -> (0,10), range 4: si avvicina di 4 -> (0,4).
	const FRTGridCoord Dest = URTBotLibrary::StepToward(FRTGridCoord(0, 0), FRTGridCoord(0, 10), 4);
	TestTrue(TEXT("arriva a (0,4)"), Dest == FRTGridCoord(0, 4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStopsAdjacentTest,
	"RefactorTactics.Bot.StepTowardStopsAdjacentToTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotStopsAdjacentTest::RunTest(const FString&)
{
	// (0,0) -> (0,3), range 4: si ferma a distanza 1 -> (0,2), non sul bersaglio.
	const FRTGridCoord Dest = URTBotLibrary::StepToward(FRTGridCoord(0, 0), FRTGridCoord(0, 3), 4);
	TestTrue(TEXT("si ferma a (0,2)"), Dest == FRTGridCoord(0, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAlreadyAdjacentTest,
	"RefactorTactics.Bot.StepTowardStaysWhenAlreadyAdjacent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAlreadyAdjacentTest::RunTest(const FString&)
{
	TestTrue(TEXT("adiacente -> fermo"), URTBotLibrary::StepToward(FRTGridCoord(0, 0), FRTGridCoord(0, 1), 4) == FRTGridCoord(0, 0));
	TestTrue(TEXT("stessa cella -> fermo"), URTBotLibrary::StepToward(FRTGridCoord(5, 5), FRTGridCoord(5, 5), 4) == FRTGridCoord(5, 5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotDiagonalTest,
	"RefactorTactics.Bot.StepTowardReducesDistanceDiagonally",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotDiagonalTest::RunTest(const FString&)
{
	// (7,7) -> (2,2), range 4: la distanza deve calare esattamente di 4 (da 10 a 6),
	// e la destinazione deve restare dentro una griglia 10x10.
	const FRTGridCoord From(7, 7), Target(2, 2);
	const FRTGridCoord Dest = URTBotLibrary::StepToward(From, Target, 4);
	TestEqual(TEXT("distanza ridotta di 4"),
		URTGridLibrary::ManhattanDistance(Dest, Target),
		URTGridLibrary::ManhattanDistance(From, Target) - 4);
	TestTrue(TEXT("dentro la griglia"), URTGridLibrary::IsInsideGrid(Dest, 10, 10));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAttackScoreTest,
	"RefactorTactics.Bot.AttackScorePrefersKillsThenDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAttackScoreTest::RunTest(const FString&)
{
	// Un kill batte sempre un non-kill.
	TestTrue(TEXT("kill > non-kill"),
		URTBotLibrary::AttackScore(50, 40) > URTBotLibrary::AttackScore(50, 100));
	// Tra i kill, si preferisce il bersaglio più debole.
	TestTrue(TEXT("kill sul più debole"),
		URTBotLibrary::AttackScore(50, 30) > URTBotLibrary::AttackScore(50, 40));
	// Tra i non-kill, si preferisce più danno.
	TestTrue(TEXT("non-kill: più danno"),
		URTBotLibrary::AttackScore(50, 100) > URTBotLibrary::AttackScore(30, 100));
	// Danno esattamente pari agli HP è un kill.
	TestTrue(TEXT("danno == HP è kill"),
		URTBotLibrary::AttackScore(40, 40) > URTBotLibrary::AttackScore(39, 40));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotApproachTest,
	"RefactorTactics.Bot.BestApproachCellAvoidsBlockersAndApproaches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotApproachTest::RunTest(const FString&)
{
	const TMap<FRTGridCoord, int32> NoCost;
	// Percorso libero: (0,0) -> (0,9), range 3 -> cella piu' vicina raggiungibile (0,3), distanza 6.
	{
		const FRTGridCoord Dest = URTBotLibrary::BestApproachCell(FRTGridCoord(0, 0), FRTGridCoord(0, 9), 3, NoCost, 10, 10);
		TestEqual(TEXT("distanza al bersaglio 6"), URTGridLibrary::ManhattanDistance(Dest, FRTGridCoord(0, 9)), 6);
	}
	// Ostacolo 2x2 centrale: il bot non deve MAI finire su una copertura, e deve avvicinarsi.
	{
		const TArray<FRTGridCoord> Blockers = { FRTGridCoord(4,4), FRTGridCoord(5,4), FRTGridCoord(4,5), FRTGridCoord(5,5) };
		const FRTGridCoord From(7, 5), Target(2, 4);
		const FRTGridCoord Dest = URTBotLibrary::BestApproachCell(From, Target, 3, BlockedCost(Blockers), 10, 10);
		TestFalse(TEXT("non su una copertura"), Blockers.Contains(Dest));
		TestTrue(TEXT("dentro la griglia"), URTGridLibrary::IsInsideGrid(Dest, 10, 10));
		TestTrue(TEXT("entro la portata"), URTGridLibrary::ManhattanDistance(From, Dest) <= 3);
		TestTrue(TEXT("si e' avvicinato"),
			URTGridLibrary::ManhattanDistance(Dest, Target) < URTGridLibrary::ManhattanDistance(From, Target));
		TestTrue(TEXT("non sul bersaglio"), Dest != Target);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotKiteTest,
	"RefactorTactics.Bot.BestKiteCellMaximizesDistanceAvoidingWallsAndCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotKiteTest::RunTest(const FString&)
{
	const TMap<FRTGridCoord, int32> NoCost;
	// Spazio aperto: massimizza la distanza -> la aumenta esattamente di MoveRange.
	{
		const FRTGridCoord From(5, 5), Threat(5, 6);
		const FRTGridCoord Dest = URTBotLibrary::BestKiteCell(From, Threat, 3, NoCost, 10, 10);
		TestEqual(TEXT("distanza massimizzata (+3)"),
			URTGridLibrary::ManhattanDistance(Dest, Threat),
			URTGridLibrary::ManhattanDistance(From, Threat) + 3);
	}
	// All'angolo: non puo' allontanarsi in linea retta -> fuga laterale che AUMENTA comunque la distanza.
	{
		const FRTGridCoord From(0, 0), Threat(1, 1);
		const FRTGridCoord Dest = URTBotLibrary::BestKiteCell(From, Threat, 3, NoCost, 10, 10);
		TestTrue(TEXT("dentro la griglia"), URTGridLibrary::IsInsideGrid(Dest, 10, 10));
		TestTrue(TEXT("si e' allontanato dall'angolo"),
			URTGridLibrary::ManhattanDistance(Dest, Threat) > URTGridLibrary::ManhattanDistance(From, Threat));
	}
	// Non fugge mai su una copertura.
	{
		const TArray<FRTGridCoord> Blockers = { FRTGridCoord(4,4), FRTGridCoord(5,4), FRTGridCoord(4,5), FRTGridCoord(5,5) };
		const FRTGridCoord From(3, 4), Threat(1, 4);
		const FRTGridCoord Dest = URTBotLibrary::BestKiteCell(From, Threat, 3, BlockedCost(Blockers), 10, 10);
		TestFalse(TEXT("non su copertura"), Blockers.Contains(Dest));
		TestTrue(TEXT("entro la portata"), URTGridLibrary::ManhattanDistance(From, Dest) <= 3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotApproachReachableTest,
	"RefactorTactics.Bot.BestApproachCellPicksOnlyReachableCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotApproachReachableTest::RunTest(const FString&)
{
	// Muro verticale x=5 (y 0..8), varco solo in (5,9). Bot a (4,4), bersaglio (6,4) oltre il muro.
	// In Manhattan (6,3)/(6,5) sono "vicini" al bersaglio (dist 1) ma NON raggiungibili in 3 passi
	// (il muro va aggirato dal varco): la cella scelta dal bot deve restare nell'insieme raggiungibile.
	TArray<FRTGridCoord> Wall;
	for (int32 Y = 0; Y <= 8; ++Y) { Wall.Add(FRTGridCoord(5, Y)); }
	const FRTGridCoord From(4, 4), Target(6, 4);
	const int32 Range = 3;

	const FRTGridCoord Dest = URTBotLibrary::BestApproachCell(From, Target, Range, BlockedCost(Wall), 10, 10);
	const TArray<FRTGridCoord> Reachable = URTGridLibrary::ReachableCellsByCost(From, Range, BlockedCost(Wall), 10, 10);

	TestTrue(TEXT("la cella scelta e' realmente raggiungibile"), Reachable.Contains(Dest));
	TestFalse(TEXT("non sceglie una cella oltre il muro"), Dest.X > 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotAvoidsHazardTest,
	"RefactorTactics.Bot.BestApproachCellAvoidsHazardCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotAvoidsHazardTest::RunTest(const FString&)
{
	// (2,3) e' un hazard (impassabile per il bot). Bot (2,0) verso (2,5), budget-costo 3:
	// senza hazard sceglierebbe (2,3) [dist 2]; con l'hazard deve scegliere (2,2) [dist 3].
	TMap<FRTGridCoord, int32> Cost;
	Cost.Add(FRTGridCoord(2, 3), RT_BLOCKED_COST);
	const FRTGridCoord Dest = URTBotLibrary::BestApproachCell(FRTGridCoord(2, 0), FRTGridCoord(2, 5), 3, Cost, 10, 10);

	TestFalse(TEXT("non finisce sull'hazard"), Dest == FRTGridCoord(2, 3));
	TestTrue(TEXT("ripiega su (2,2)"), Dest == FRTGridCoord(2, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotUsesRampTest,
	"RefactorTactics.Bot.BestApproachCellUsesRampToBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotUsesRampTest::RunTest(const FString&)
{
	// Coperture a terra (4,4)(5,4) bloccano l'avvicinamento diretto; il bersaglio (5,4,1) e' sul ponte.
	// Con la rampa (2,4,0)->(3,4,1) il bot puo' salire e avvicinarsi in quota; senza, resta a terra.
	TMap<FRTGridCoord, int32> Cost;
	Cost.Add(FRTGridCoord(4, 4, 0), RT_BLOCKED_COST);
	Cost.Add(FRTGridCoord(5, 4, 0), RT_BLOCKED_COST);
	const TArray<FRTTraversalEdge> Ramp = {
		FRTTraversalEdge(FRTGridCoord(2, 4, 0), FRTGridCoord(3, 4, 1), 2),
		FRTTraversalEdge(FRTGridCoord(3, 4, 1), FRTGridCoord(2, 4, 0), 2) };
	const FRTGridCoord From(2, 4, 0), Target(5, 4, 1);

	const FRTGridCoord WithRamp = URTBotLibrary::BestApproachCell(From, Target, 4, Cost, 10, 10, Ramp);
	TestEqual(TEXT("con la rampa: sale sul ponte (layer 1)"), WithRamp.Layer, 1);

	const FRTGridCoord NoRamp = URTBotLibrary::BestApproachCell(From, Target, 4, Cost, 10, 10);
	TestEqual(TEXT("senza rampa: resta a terra (layer 0)"), NoRamp.Layer, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotFiringCellClimbsBridgeTest,
	"RefactorTactics.Bot.BestFiringCellClimbsBridgeForElevatedShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotFiringCellClimbsBridgeTest::RunTest(const FString&)
{
	// Bersaglio a TERRA (6,4,0). Coperture a terra (4,4,0)(5,4,0) bloccano il tiro dal suolo lungo y=4.
	// Dal ponte (layer 1) la LOS di elevazione passa SOPRA le coperture -> il bot deve salire per sparare.
	TMap<FRTGridCoord, int32> Cost;
	Cost.Add(FRTGridCoord(4, 4, 0), RT_BLOCKED_COST);
	Cost.Add(FRTGridCoord(5, 4, 0), RT_BLOCKED_COST);
	const TArray<FRTTraversalEdge> Ramp = {
		FRTTraversalEdge(FRTGridCoord(2, 4, 0), FRTGridCoord(3, 4, 1), 2),
		FRTTraversalEdge(FRTGridCoord(3, 4, 1), FRTGridCoord(2, 4, 0), 2) };
	const TArray<FRTGridCoord> Blockers = { FRTGridCoord(4, 4, 0), FRTGridCoord(5, 4, 0) };
	const FRTGridCoord From(2, 4, 0), Target(6, 4, 0);
	const int32 MoveRange = 4, AttackRange = 5;

	// Da terra il bot NON ha LOS sul bersaglio (coperture in mezzo).
	TestFalse(TEXT("nessun tiro dal suolo"), URTGridLibrary::HasLineOfSight(From, Target, Blockers));

	// Con la rampa: sale sul ponte e trova una posizione di tiro elevata.
	const FRTGridCoord Fire = URTBotLibrary::BestFiringCell(From, Target, MoveRange, Cost, 10, 10, Blockers, AttackRange, Ramp);
	TestEqual(TEXT("sale sul ponte (layer 1) per il tiro"), Fire.Layer, 1);
	TestTrue(TEXT("da lì ha davvero LOS + gittata"),
		URTGridLibrary::IsWithinRange(Fire, Target, AttackRange) && URTGridLibrary::HasLineOfSight(Fire, Target, Blockers));

	// Senza rampa: nessuna posizione di tiro elevata raggiungibile -> non sale (resta a terra).
	const FRTGridCoord NoRamp = URTBotLibrary::BestFiringCell(From, Target, MoveRange, Cost, 10, 10, Blockers, AttackRange);
	TestEqual(TEXT("senza rampa: non sale (layer 0)"), NoRamp.Layer, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotFiringCellNoneReachableTest,
	"RefactorTactics.Bot.BestFiringCellReturnsFromWhenNoShotReachable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotFiringCellNoneReachableTest::RunTest(const FString&)
{
	// Bersaglio lontano e fuori gittata da ogni cella raggiungibile -> ritorna From (poi si avvicina).
	const TMap<FRTGridCoord, int32> NoCost;
	const TArray<FRTGridCoord> NoBlockers;
	const FRTGridCoord From(0, 0), Target(9, 9);
	const FRTGridCoord Fire = URTBotLibrary::BestFiringCell(From, Target, 2, NoCost, 10, 10, NoBlockers, 2);
	TestTrue(TEXT("nessun tiro raggiungibile -> resta a From"), Fire == From);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
