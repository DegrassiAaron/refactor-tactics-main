#include "Misc/AutomationTest.h"
#include "Bot/RTBotLibrary.h"
#include "Grid/RTGridLibrary.h"
#include "Core/RTTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotStepAwayTest,
	"RefactorTactics.Bot.StepAwayRetreatsFromThreatWithinGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotStepAwayTest::RunTest(const FString&)
{
	// Minaccia sopra (stessa colonna): si ritira lungo -Y aumentando la distanza di MoveRange.
	{
		const FRTGridCoord From(5, 5), Threat(5, 9);
		const FRTGridCoord Dest = URTBotLibrary::StepAway(From, Threat, 3, 10, 10);
		TestEqual(TEXT("distanza aumentata di 3"),
			URTGridLibrary::ManhattanDistance(Dest, Threat),
			URTGridLibrary::ManhattanDistance(From, Threat) + 3);
		TestTrue(TEXT("dentro la griglia"), URTGridLibrary::IsInsideGrid(Dest, 10, 10));
	}
	// Minaccia in diagonale: ritirata diagonale, distanza aumentata di MoveRange.
	{
		const FRTGridCoord From(5, 5), Threat(8, 8);
		const FRTGridCoord Dest = URTBotLibrary::StepAway(From, Threat, 2, 10, 10);
		TestEqual(TEXT("diagonale: +2 distanza"),
			URTGridLibrary::ManhattanDistance(Dest, Threat),
			URTGridLibrary::ManhattanDistance(From, Threat) + 2);
		TestTrue(TEXT("dentro la griglia"), URTGridLibrary::IsInsideGrid(Dest, 10, 10));
	}
	// Contro il bordo: la ritirata viene limitata alla griglia (nessuna cella negativa).
	{
		const FRTGridCoord From(1, 5), Threat(9, 5);
		const FRTGridCoord Dest = URTBotLibrary::StepAway(From, Threat, 3, 10, 10);
		TestTrue(TEXT("clamp al bordo -> (0,5)"), Dest == FRTGridCoord(0, 5));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
