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

namespace
{
	// Confronto profondo di un FRTPathResult atteso.
	bool PathResultEquals(const FRTPathResult& R, const FRTGridCoord& Final, const TArray<FRTGridCoord>& Entered)
	{
		if (!(R.Final == Final) || R.Entered.Num() != Entered.Num()) { return false; }
		for (int32 i = 0; i < Entered.Num(); ++i) { if (!(R.Entered[i] == Entered[i])) { return false; } }
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolvePathsBehaviorsTest,
	"RefactorTactics.Turn.ResolvePathsMicrostepBehaviors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolvePathsBehaviorsTest::RunTest(const FString&)
{
	// (1) Percorso libero: entra in tutte le celle.
	{
		const TArray<TArray<FRTGridCoord>> P = { { FRTGridCoord(0,0), FRTGridCoord(1,0), FRTGridCoord(2,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("libero: (2,0) attraversando (1,0)(2,0)"),
			PathResultEquals(R[0], FRTGridCoord(2,0), { FRTGridCoord(1,0), FRTGridCoord(2,0) }));
	}
	// (2) Destinazione contesa: entrambi fermi.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(2,0), FRTGridCoord(1,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("contesa: u0 fermo"), PathResultEquals(R[0], FRTGridCoord(0,0), {}));
		TestTrue(TEXT("contesa: u1 fermo"), PathResultEquals(R[1], FRTGridCoord(2,0), {}));
	}
	// (3) Scambio diretto: consentito.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(1,0), FRTGridCoord(0,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("scambio: u0 -> (1,0)"), PathResultEquals(R[0], FRTGridCoord(1,0), { FRTGridCoord(1,0) }));
		TestTrue(TEXT("scambio: u1 -> (0,0)"), PathResultEquals(R[1], FRTGridCoord(0,0), { FRTGridCoord(0,0) }));
	}
	// (4) Bloccata da unita' ferma.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(1,0) } }; // u1 non si muove
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("bloccata: u0 fermo"), PathResultEquals(R[0], FRTGridCoord(0,0), {}));
		TestTrue(TEXT("ferma: u1 resta"), PathResultEquals(R[1], FRTGridCoord(1,0), {}));
	}
	// (5) Treno: u1 libera (1,0) mentre u0 vi entra -> entrambi avanzano (correttezza sincrona).
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(1,0), FRTGridCoord(2,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("treno: u0 -> (1,0)"), PathResultEquals(R[0], FRTGridCoord(1,0), { FRTGridCoord(1,0) }));
		TestTrue(TEXT("treno: u1 -> (2,0)"), PathResultEquals(R[1], FRTGridCoord(2,0), { FRTGridCoord(2,0) }));
	}
	// (6) Troncamento: u0 avanza poi si blocca quando u1 si ferma davanti.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0), FRTGridCoord(2,0) },
			{ FRTGridCoord(3,0), FRTGridCoord(2,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("troncamento: u0 si ferma a (1,0)"), PathResultEquals(R[0], FRTGridCoord(1,0), { FRTGridCoord(1,0) }));
		TestTrue(TEXT("troncamento: u1 a (2,0)"), PathResultEquals(R[1], FRTGridCoord(2,0), { FRTGridCoord(2,0) }));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolvePathsOrderIndependenceTest,
	"RefactorTactics.Turn.ResolvePathsOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolvePathsOrderIndependenceTest::RunTest(const FString&)
{
	// Mix di treno + contesa; permutando l'ordine, il risultato di ogni unita' non cambia.
	const TArray<TArray<FRTGridCoord>> P = {
		{ FRTGridCoord(0,0), FRTGridCoord(1,0) },   // A: treno con B
		{ FRTGridCoord(1,0), FRTGridCoord(2,0) },   // B
		{ FRTGridCoord(4,0), FRTGridCoord(3,0) },   // C: contesa con D su (3,0)
		{ FRTGridCoord(2,0), FRTGridCoord(3,0) } }; // D
	TArray<TArray<FRTGridCoord>> Rev = P; Algo::Reverse(Rev);

	const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
	const TArray<FRTPathResult> RR = URTMovementResolver::ResolvePaths(Rev);

	// RR e' l'esito con input invertito: RR[N-1-i] deve corrispondere a R[i].
	const int32 N = P.Num();
	bool bMatch = (R.Num() == N && RR.Num() == N);
	for (int32 i = 0; i < N && bMatch; ++i)
	{
		bMatch = PathResultEquals(RR[N - 1 - i], R[i].Final, R[i].Entered);
	}
	TestTrue(TEXT("risultato indipendente dall'ordine delle richieste"), bMatch);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
