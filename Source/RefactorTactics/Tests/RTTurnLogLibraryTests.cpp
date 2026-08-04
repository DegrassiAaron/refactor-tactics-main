#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTTurnLogEntry MakeEntry(ERTMatchPhase Phase, ERTLogCategory Cat, uint8 Outcome,
		const FRTGridCoord& Src, const FRTGridCoord& Tgt, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.Phase = Phase;
		E.Category = Cat;
		E.Outcome = Outcome;
		E.SrcCell = Src;
		E.TgtCell = Tgt;
		E.Amount = Amount;
		return E;
	}
}

// EntryLess: ordine TOTALE -> distingue anche l'ultimo campo (Amount), antisimmetrico.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogEntryLessTest,
	"RefactorTactics.TurnLog.EntryLessTotalOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogEntryLessTest::RunTest(const FString&)
{
	const FRTGridCoord C(1, 1);
	const FRTTurnLogEntry A = MakeEntry(ERTMatchPhase::Move, ERTLogCategory::Move, 0, C, C, 5);
	const FRTTurnLogEntry B = MakeEntry(ERTMatchPhase::Move, ERTLogCategory::Move, 0, C, C, 10); // solo Amount diverso
	TestTrue(TEXT("A<B per Amount (ordine totale fino all'ultimo campo)"), URTTurnLogLibrary::EntryLess(A, B));
	TestFalse(TEXT("non B<A (antisimmetrico)"), URTTurnLogLibrary::EntryLess(B, A));
	return true;
}

// HashTurnLog: permutazione-invariante (stesse voci in ordine diverso -> stesso hash) e sensibile alle differenze.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogHashTest,
	"RefactorTactics.TurnLog.HashPermutationInvariantAndSensitive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogHashTest::RunTest(const FString&)
{
	const FRTGridCoord C1(1, 1), C2(2, 2), C3(3, 3);
	const FRTTurnLogEntry E1 = MakeEntry(ERTMatchPhase::Move,  ERTLogCategory::Move,   1, C1, C2, 3);
	const FRTTurnLogEntry E2 = MakeEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat, 2, C2, C3, 7);
	const FRTTurnLogEntry E3 = MakeEntry(ERTMatchPhase::Blast, ERTLogCategory::Combat, 2, C2, C3, 8); // Amount 8 != 7

	TArray<FRTTurnLogEntry> L12; L12.Add(E1); L12.Add(E2);
	TArray<FRTTurnLogEntry> L21; L21.Add(E2); L21.Add(E1); // stesse voci, ordine inverso
	TArray<FRTTurnLogEntry> L13; L13.Add(E1); L13.Add(E3); // una voce differisce (Amount)

	const uint32 H12 = URTTurnLogLibrary::HashTurnLog(L12);
	const uint32 H21 = URTTurnLogLibrary::HashTurnLog(L21);
	const uint32 H13 = URTTurnLogLibrary::HashTurnLog(L13);

	TestEqual(TEXT("permutazione-invariante: [E1,E2] == [E2,E1]"), H12, H21);
	TestNotEqual(TEXT("sensibile: [E1,E2] != [E1,E3]"), H12, H13);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
