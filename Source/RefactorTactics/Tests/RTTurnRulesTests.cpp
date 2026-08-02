#include "Misc/AutomationTest.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPhaseCycleTest,
	"RefactorTactics.Turn.PhaseCycleReturnsToPlanning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPhaseCycleTest::RunTest(const FString&)
{
	ERTMatchPhase P = ERTMatchPhase::Planning;
	P = URTTurnRules::NextPhase(P); TestTrue(TEXT("-> Prep"), P == ERTMatchPhase::Prep);
	P = URTTurnRules::NextPhase(P); TestTrue(TEXT("-> Dash"), P == ERTMatchPhase::Dash);
	P = URTTurnRules::NextPhase(P); TestTrue(TEXT("-> Blast"), P == ERTMatchPhase::Blast);
	P = URTTurnRules::NextPhase(P); TestTrue(TEXT("-> Move"), P == ERTMatchPhase::Move);
	P = URTTurnRules::NextPhase(P); TestTrue(TEXT("-> Cleanup"), P == ERTMatchPhase::Cleanup);
	P = URTTurnRules::NextPhase(P); TestTrue(TEXT("-> Planning"), P == ERTMatchPhase::Planning);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPhaseEndedAbsorbingTest,
	"RefactorTactics.Turn.MatchEndedIsAbsorbing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPhaseEndedAbsorbingTest::RunTest(const FString&)
{
	TestTrue(TEXT("MatchEnded resta MatchEnded"),
		URTTurnRules::NextPhase(ERTMatchPhase::MatchEnded) == ERTMatchPhase::MatchEnded);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
