#include "Misc/AutomationTest.h"
#include "Turn/RTMovementResolver.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogMoveOutcomesTest,
	"RefactorTactics.TurnLog.MoveOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogMoveOutcomesTest::RunTest(const FString&)
{
	// Stayed: path < 2 celle.
	{
		const TArray<TArray<FRTGridCoord>> P = { { FRTGridCoord(0,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("stayed"), R[0].Outcome == ERTMoveOutcome::Stayed);
	}
	// Moved: percorso libero fino in fondo.
	{
		const TArray<TArray<FRTGridCoord>> P = { { FRTGridCoord(0,0), FRTGridCoord(1,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("moved"), R[0].Outcome == ERTMoveOutcome::Moved);
	}
	// BlockedContested: due verso la stessa cella.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(2,0), FRTGridCoord(1,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("u0 contested"), R[0].Outcome == ERTMoveOutcome::BlockedContested);
		TestTrue(TEXT("u1 contested"), R[1].Outcome == ERTMoveOutcome::BlockedContested);
	}
	// BlockedByUnit: u1 ferma su (1,0), u0 prova a entrarci.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(1,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("u0 blocked-by-unit"), R[0].Outcome == ERTMoveOutcome::BlockedByUnit);
		TestTrue(TEXT("u1 stayed"), R[1].Outcome == ERTMoveOutcome::Stayed);
	}
	// Scambio -> Moved per entrambi.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(1,0), FRTGridCoord(0,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("swap u0 moved"), R[0].Outcome == ERTMoveOutcome::Moved);
		TestTrue(TEXT("swap u1 moved"), R[1].Outcome == ERTMoveOutcome::Moved);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogMoveOrderInvariantTest,
	"RefactorTactics.TurnLog.MoveOutcomeOrderInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogMoveOrderInvariantTest::RunTest(const FString&)
{
	const TArray<TArray<FRTGridCoord>> P = {
		{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
		{ FRTGridCoord(2,0), FRTGridCoord(1,0) } };   // contesa con u0
	TArray<TArray<FRTGridCoord>> Rev = P; Algo::Reverse(Rev);
	const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
	const TArray<FRTPathResult> RR = URTMovementResolver::ResolvePaths(Rev);
	TestTrue(TEXT("outcome invariante"), R[0].Outcome == RR[1].Outcome && R[1].Outcome == RR[0].Outcome);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
