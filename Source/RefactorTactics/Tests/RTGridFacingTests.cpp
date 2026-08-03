#include "Misc/AutomationTest.h"
#include "Grid/RTGridLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

// DirectionYaw: yaw (gradi) per guardare da From verso To sul piano XY.
// Convenzione UE: forward +X = 0, +Y = 90, -X = +/-180, -Y = -90. La quota Z e' ignorata (facing planare).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDirectionYawFacesTargetTest,
	"RefactorTactics.Grid.DirectionYawFacesTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDirectionYawFacesTargetTest::RunTest(const FString&)
{
	const FVector O(0.f, 0.f, 0.f);
	TestTrue(TEXT("+X -> 0"),        FMath::IsNearlyEqual(URTGridLibrary::DirectionYaw(O, FVector(100.f, 0.f, 0.f)),   0.f,   0.01f));
	TestTrue(TEXT("+Y -> 90"),       FMath::IsNearlyEqual(URTGridLibrary::DirectionYaw(O, FVector(0.f, 100.f, 0.f)),  90.f,   0.01f));
	TestTrue(TEXT("-Y -> -90"),      FMath::IsNearlyEqual(URTGridLibrary::DirectionYaw(O, FVector(0.f, -100.f, 0.f)), -90.f,  0.01f));
	TestTrue(TEXT("-X -> +/-180"),   FMath::IsNearlyEqual(FMath::Abs(URTGridLibrary::DirectionYaw(O, FVector(-100.f, 0.f, 0.f))), 180.f, 0.01f));
	TestTrue(TEXT("ignora Z: +X in quota -> 0"), FMath::IsNearlyEqual(URTGridLibrary::DirectionYaw(O, FVector(100.f, 0.f, 500.f)), 0.f, 0.01f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
