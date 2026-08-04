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

// RingLocalZ: offset Z locale che porta un anello (figlio della mesh) al piano della cella.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRingLocalZTest,
	"RefactorTactics.Unit.RingLocalZ",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRingLocalZTest::RunTest(const FString&)
{
	// Cilindro segnaposto: pivot al centro (offset 90) + scala Z 1.8 -> compensa a -49 (a terra).
	TestEqual(TEXT("cilindro (90, 1.8) -> -49"), ARTUnit::RingLocalZ(90.f, 1.8f), -49.f);
	// Skeletal: pivot ai piedi (offset 0) -> +1 (a livello base della mesh).
	TestEqual(TEXT("skeletal (0, 1.8) -> 1"), ARTUnit::RingLocalZ(0.f, 1.8f), 1.f);
	// Guardia div-by-zero: scala Z 0 -> fallback 1 (nessuna divisione).
	TestEqual(TEXT("scala 0 -> 1 (guardia)"), ARTUnit::RingLocalZ(90.f, 0.f), 1.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
