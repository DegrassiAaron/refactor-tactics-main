#include "Misc/AutomationTest.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTTurnLogEntry MakeEntry(ERTMatchPhase Phase, ERTLogCategory Cat, uint8 Outcome,
		const FRTCellId& Src, const FRTCellId& Tgt, int32 Amount)
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

// EntryLess: ordine TOTALE -> distingue anche l'ULTIMO campo della catena, antisimmetrico.
// L'ultimo non e' piu' `Amount` da un pezzo: dopo di lui vengono `ActionId`, poi `TurnNumber`,
// `GraphRevision` e `UnitId` (v6, D-063/D-067), e da ultimo `Priority` (v7, #79). La catena
// autorevole sta in `spec-turnlog.md` §6 — qui si cita, non si duplica, perche' un elenco copiato
// e' esattamente cio' che e' invecchiato tre volte.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogEntryLessTest,
	"RefactorTactics.TurnLog.EntryLessTotalOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogEntryLessTest::RunTest(const FString&)
{
	const FRTCellId C(1, 1);
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
	const FRTCellId C1(1, 1), C2(2, 2), C3(3, 3);
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

// ---------------------------------------------------------------------------------------------------------
// Descrizione leggibile: il reason code deve arrivare al giocatore, con le coordinate assiali
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogDescribeTest,
	"RefactorTactics.TurnLog.DescribeEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogDescribeTest::RunTest(const FString&)
{
	FRTTurnLogEntry Contested;
	Contested.Phase = ERTMatchPhase::Move;
	Contested.Category = ERTLogCategory::Move;
	Contested.Outcome = static_cast<uint8>(ERTMoveOutcome::BlockedContested);
	Contested.SrcCell = FRTCellId(1, -2, 0);
	Contested.TgtCell = FRTCellId(1, -2, 0);
	Contested.Amount = 0;

	const FString ContestedText = URTTurnLogLibrary::DescribeEntry(Contested);
	TestTrue(TEXT("la cella e' in coordinate assiali (q,r,L)"), ContestedText.Contains(TEXT("q=1")) && ContestedText.Contains(TEXT("r=-2")));
	TestTrue(TEXT("il motivo e' leggibile"), ContestedText.Contains(TEXT("contesa")));

	FRTTurnLogEntry NoLos;
	NoLos.Phase = ERTMatchPhase::Blast;
	NoLos.Category = ERTLogCategory::Combat;
	NoLos.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
	NoLos.SrcCell = FRTCellId(0, 0, 0);
	NoLos.TgtCell = FRTCellId(3, 0, 1);

	const FString NoLosText = URTTurnLogLibrary::DescribeEntry(NoLos);
	TestTrue(TEXT("compaiono attaccante e bersaglio"), NoLosText.Contains(TEXT("q=0")) && NoLosText.Contains(TEXT("q=3")));
	TestTrue(TEXT("il layer del bersaglio e' visibile"), NoLosText.Contains(TEXT("L=1")));
	TestTrue(TEXT("il motivo e' leggibile"), NoLosText.Contains(TEXT("linea di tiro")));

	FRTTurnLogEntry Lethal;
	Lethal.Phase = ERTMatchPhase::Blast;
	Lethal.Category = ERTLogCategory::Combat;
	Lethal.Outcome = static_cast<uint8>(ERTCombatOutcome::Lethal);
	Lethal.SrcCell = FRTCellId(0, 0, 0);
	Lethal.TgtCell = FRTCellId(1, 0, 0);
	Lethal.Amount = 40;

	const FString LethalText = URTTurnLogLibrary::DescribeEntry(Lethal);
	TestTrue(TEXT("il danno compare"), LethalText.Contains(TEXT("40")));
	TestTrue(TEXT("l'esito letale e' dichiarato"), LethalText.Contains(TEXT("elimin")));

	// Voci diverse devono leggersi diverse: una descrizione costante passerebbe le prove precedenti.
	TestNotEqual(TEXT("descrizioni distinte per esiti distinti"), ContestedText, NoLosText);
	TestNotEqual(TEXT("descrizioni distinte per danno diverso"), LethalText, NoLosText);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
