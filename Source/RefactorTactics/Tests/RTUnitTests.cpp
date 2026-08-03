#include "Misc/AutomationTest.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTeamColorForTest,
	"RefactorTactics.Unit.TeamColorFor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTeamColorForTest::RunTest(const FString&)
{
	const FLinearColor A(0.10f, 0.40f, 1.00f); // team 0 (blu)
	const FLinearColor B(1.00f, 0.20f, 0.15f); // team 1 (rosso)
	TestTrue(TEXT("team 0 -> A"), ARTUnit::TeamColorFor(0, A, B) == A);
	TestTrue(TEXT("team 1 -> B"), ARTUnit::TeamColorFor(1, A, B) == B);
	TestTrue(TEXT("default (>1) -> B"), ARTUnit::TeamColorFor(5, A, B) == B);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
