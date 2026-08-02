#include "Misc/AutomationTest.h"
#include "Turn/RTMovementResolver.h"
#include "Core/RTTypes.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTMoveRequest Move(int32 FromX, int32 FromY, int32 ToX, int32 ToY)
	{
		return FRTMoveRequest(FRTGridCoord(FromX, FromY), FRTGridCoord(ToX, ToY));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveSingleTest,
	"RefactorTactics.Turn.MoveIntoEmptyCellSucceeds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveSingleTest::RunTest(const FString&)
{
	const TArray<FRTMoveRequest> R = { Move(0, 0, 0, 1) };
	const TArray<FRTGridCoord> Out = URTMovementResolver::ResolveMoves(R);
	TestTrue(TEXT("si muove a (0,1)"), Out.Num() == 1 && Out[0] == FRTGridCoord(0, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveContestedTest,
	"RefactorTactics.Turn.ContestedDestinationBothStay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveContestedTest::RunTest(const FString&)
{
	// A e B vogliono entrambe la cella (1,1): restano ferme.
	const TArray<FRTMoveRequest> R = { Move(0, 0, 1, 1), Move(2, 2, 1, 1) };
	const TArray<FRTGridCoord> Out = URTMovementResolver::ResolveMoves(R);
	TestTrue(TEXT("A resta a (0,0)"), Out[0] == FRTGridCoord(0, 0));
	TestTrue(TEXT("B resta a (2,2)"), Out[1] == FRTGridCoord(2, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveSwapTest,
	"RefactorTactics.Turn.DirectSwapAllowed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveSwapTest::RunTest(const FString&)
{
	// A va nella cella di B e viceversa: scambio consentito.
	const TArray<FRTMoveRequest> R = { Move(0, 0, 0, 1), Move(0, 1, 0, 0) };
	const TArray<FRTGridCoord> Out = URTMovementResolver::ResolveMoves(R);
	TestTrue(TEXT("A -> (0,1)"), Out[0] == FRTGridCoord(0, 1));
	TestTrue(TEXT("B -> (0,0)"), Out[1] == FRTGridCoord(0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveBlockedTest,
	"RefactorTactics.Turn.MoveIntoStationaryOccupiedBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveBlockedTest::RunTest(const FString&)
{
	// B resta ferma su (0,1); A prova a entrarci: bloccata.
	const TArray<FRTMoveRequest> R = { Move(0, 0, 0, 1), Move(0, 1, 0, 1) };
	const TArray<FRTGridCoord> Out = URTMovementResolver::ResolveMoves(R);
	TestTrue(TEXT("A bloccata a (0,0)"), Out[0] == FRTGridCoord(0, 0));
	TestTrue(TEXT("B resta a (0,1)"), Out[1] == FRTGridCoord(0, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveVacatedTest,
	"RefactorTactics.Turn.VacatedCellCanBeEntered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveVacatedTest::RunTest(const FString&)
{
	// B lascia (0,1) andando a (0,2); A entra in (0,1) appena liberata.
	const TArray<FRTMoveRequest> R = { Move(0, 0, 0, 1), Move(0, 1, 0, 2) };
	const TArray<FRTGridCoord> Out = URTMovementResolver::ResolveMoves(R);
	TestTrue(TEXT("A -> (0,1)"), Out[0] == FRTGridCoord(0, 1));
	TestTrue(TEXT("B -> (0,2)"), Out[1] == FRTGridCoord(0, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveOrderIndependentTest,
	"RefactorTactics.Turn.ResolutionIsOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveOrderIndependentTest::RunTest(const FString&)
{
	// Mix: contesa (U0,U1 -> (1,1)) + scambio (U2<->U3). From tutte distinte.
	const TArray<FRTMoveRequest> Forward = {
		Move(0, 0, 1, 1), Move(2, 2, 1, 1), Move(5, 5, 5, 6), Move(5, 6, 5, 5)
	};
	TArray<FRTMoveRequest> Backward = Forward;
	Algo::Reverse(Backward);

	const TArray<FRTGridCoord> OutForward = URTMovementResolver::ResolveMoves(Forward);
	TArray<FRTGridCoord> OutBackward = URTMovementResolver::ResolveMoves(Backward);
	Algo::Reverse(OutBackward); // riallineo all'ordine di Forward

	bool bSame = (OutForward.Num() == OutBackward.Num());
	for (int32 i = 0; i < OutForward.Num() && bSame; ++i)
	{
		bSame = (OutForward[i] == OutBackward[i]);
	}
	TestTrue(TEXT("stesso risultato invertendo l'ordine"), bSame);

	// Esito atteso: contesa -> fermi; scambio -> consentito.
	TestTrue(TEXT("U0 resta a (0,0)"), OutForward[0] == FRTGridCoord(0, 0));
	TestTrue(TEXT("U1 resta a (2,2)"), OutForward[1] == FRTGridCoord(2, 2));
	TestTrue(TEXT("U2 -> (5,6)"), OutForward[2] == FRTGridCoord(5, 6));
	TestTrue(TEXT("U3 -> (5,5)"), OutForward[3] == FRTGridCoord(5, 5));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
