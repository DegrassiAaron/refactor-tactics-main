#include "Misc/AutomationTest.h"
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Regole di prova: 12 round, vittoria per obiettivo a 3 punti. */
	FRTMatchRules MakeRules(int32 RoundLimit = 12, int32 ScoreToWin = 3)
	{
		FRTMatchRules Rules;
		Rules.FormatId = FName(TEXT("Format.Test2v2"));
		Rules.RoundLimit = RoundLimit;
		Rules.ScoreToWin = ScoreToWin;
		return Rules;
	}

	/** Stato di meta' partita: 2v2 in piedi, nessun punto, primo round. */
	FRTMatchState MakeState(int32 Team0Alive = 2, int32 Team1Alive = 2)
	{
		FRTMatchState State;
		State.Team0Alive = Team0Alive;
		State.Team1Alive = Team1Alive;
		State.RoundNumber = 1;
		return State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchEndsOnEliminationTest,
	"RefactorTactics.Match.EndsOnElimination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchEndsOnEliminationTest::RunTest(const FString&)
{
	const FRTMatchRules Rules = MakeRules();

	{
		FRTMatchState State = MakeState(1, 0);
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("squadra 1 azzerata -> vince la 0"), Result.Outcome == ERTMatchOutcome::Team0Wins);
		TestTrue(TEXT("per eliminazione"), Result.Reason == ERTMatchEndReason::Elimination);
	}
	{
		FRTMatchState State = MakeState(0, 2);
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("squadra 0 azzerata -> vince la 1"), Result.Outcome == ERTMatchOutcome::Team1Wins);
		TestTrue(TEXT("per eliminazione"), Result.Reason == ERTMatchEndReason::Elimination);
	}
	{
		FRTMatchState State = MakeState(0, 0);
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("entrambe azzerate -> pareggio"), Result.Outcome == ERTMatchOutcome::Draw);
		TestTrue(TEXT("sempre per eliminazione"), Result.Reason == ERTMatchEndReason::Elimination);
	}
	{
		FRTMatchState State = MakeState(2, 2);
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("2v2 in piedi -> in corso"), Result.Outcome == ERTMatchOutcome::InProgress);
		TestTrue(TEXT("nessuna via"), Result.Reason == ERTMatchEndReason::None);
	}
	{
		// L'eliminazione precede le altre vie (spec §12: la victory condition si valuta per prima). Una
		// squadra azzerata nello stesso Cleanup in cui l'altra tocca la soglia perde per eliminazione: se
		// vincesse l'obiettivo, l'esito dipenderebbe dall'ordine in cui il Cleanup guarda le due cose.
		FRTMatchState State = MakeState(2, 0);
		State.Team0Score = 3;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("vince la 0"), Result.Outcome == ERTMatchOutcome::Team0Wins);
		TestTrue(TEXT("e la via dichiarata e' l'eliminazione"), Result.Reason == ERTMatchEndReason::Elimination);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchEndsOnObjectiveTest,
	"RefactorTactics.Match.EndsOnObjective",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchEndsOnObjectiveTest::RunTest(const FString&)
{
	const FRTMatchRules Rules = MakeRules(12, 3);

	{
		FRTMatchState State = MakeState();
		State.Team0Score = 3;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("soglia raggiunta -> vince la 0"), Result.Outcome == ERTMatchOutcome::Team0Wins);
		TestTrue(TEXT("per obiettivo"), Result.Reason == ERTMatchEndReason::Objective);
	}
	{
		FRTMatchState State = MakeState();
		State.Team1Score = 5; // oltre la soglia: chiude comunque
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("soglia superata -> vince la 1"), Result.Outcome == ERTMatchOutcome::Team1Wins);
		TestTrue(TEXT("per obiettivo"), Result.Reason == ERTMatchEndReason::Objective);
	}
	{
		FRTMatchState State = MakeState();
		State.Team0Score = 2;
		State.Team1Score = 2;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("sotto soglia -> partita in corso"), Result.Outcome == ERTMatchOutcome::InProgress);
		TestTrue(TEXT("nessuna via"), Result.Reason == ERTMatchEndReason::None);
	}
	{
		// Entrambe alla soglia nello stesso Cleanup: vince chi ha di piu'. E' un caso raggiungibile — due
		// obiettivi possono completarsi nello stesso round — e senza questa regola l'esito dipenderebbe
		// dall'ordine di scansione delle squadre.
		FRTMatchState State = MakeState();
		State.Team0Score = 4;
		State.Team1Score = 3;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("entrambe a soglia -> vince chi ha di piu'"), Result.Outcome == ERTMatchOutcome::Team0Wins);
		TestTrue(TEXT("per obiettivo"), Result.Reason == ERTMatchEndReason::Objective);
	}
	{
		FRTMatchState State = MakeState();
		State.Team0Score = 3;
		State.Team1Score = 3;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("entrambe alla stessa soglia -> pareggio"), Result.Outcome == ERTMatchOutcome::Draw);
		TestTrue(TEXT("per obiettivo"), Result.Reason == ERTMatchEndReason::Objective);
	}
	{
		// ScoreToWin 0 = via disattivata: nessun punteggio chiude la partita, per quanto alto.
		const FRTMatchRules NoObjective = MakeRules(12, 0);
		FRTMatchState State = MakeState();
		State.Team0Score = 99;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, NoObjective);
		TestTrue(TEXT("senza soglia la partita continua"), Result.Outcome == ERTMatchOutcome::InProgress);
		TestTrue(TEXT("nessuna via"), Result.Reason == ERTMatchEndReason::None);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchEndsOnTurnLimitTest,
	"RefactorTactics.Match.EndsOnTurnLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchEndsOnTurnLimitTest::RunTest(const FString&)
{
	const FRTMatchRules Rules = MakeRules(12, 3);

	{
		FRTMatchState State = MakeState();
		State.RoundNumber = 11;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("un round prima del limite si gioca ancora"), Result.Outcome == ERTMatchOutcome::InProgress);
	}
	{
		// Il round 12 si gioca INTERO: la valutazione avviene nel Cleanup, quindi al limite la partita e'
		// finita dopo che il dodicesimo round ha prodotto i suoi effetti.
		FRTMatchState State = MakeState();
		State.RoundNumber = 12;
		State.Team0Score = 2;
		State.Team1Score = 1;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("al limite vince chi ha piu' progresso"), Result.Outcome == ERTMatchOutcome::Team0Wins);
		TestTrue(TEXT("per scadenza dei round"), Result.Reason == ERTMatchEndReason::RoundLimit);
	}
	{
		FRTMatchState State = MakeState();
		State.RoundNumber = 12;
		State.Team1Score = 1;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("vince la 1"), Result.Outcome == ERTMatchOutcome::Team1Wins);
		TestTrue(TEXT("per scadenza dei round"), Result.Reason == ERTMatchEndReason::RoundLimit);
	}
	{
		// Un limite diverso morde in un round diverso: e' la ragione per cui il FormatId deve stare nel
		// TurnLog — le due tracce sono identiche fino al round in cui il limite interviene.
		const FRTMatchRules ShortRules = MakeRules(10, 3);
		FRTMatchState State = MakeState();
		State.RoundNumber = 10;
		State.Team0Score = 1;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, ShortRules);
		TestTrue(TEXT("con RoundLimit 10 la partita finisce al round 10"),
			Result.Reason == ERTMatchEndReason::RoundLimit);

		const FRTMatchResult SameStateLongerFormat = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("lo stesso stato con RoundLimit 12 e' ancora in corso"),
			SameStateLongerFormat.Outcome == ERTMatchOutcome::InProgress);
	}
	{
		// L'eliminazione al round del limite resta eliminazione: la via dichiarata e' quella che ha deciso
		// davvero, non quella che sarebbe scattata comunque.
		FRTMatchState State = MakeState(2, 0);
		State.RoundNumber = 12;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("vince la 0"), Result.Outcome == ERTMatchOutcome::Team0Wins);
		TestTrue(TEXT("per eliminazione, non per scadenza"), Result.Reason == ERTMatchEndReason::Elimination);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchTieIsDeclaredTest,
	"RefactorTactics.Match.TieIsDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchTieIsDeclaredTest::RunTest(const FString&)
{
	const FRTMatchRules Rules = MakeRules(12, 3);

	{
		// Parita' allo scadere: pareggio DICHIARATO. Non si assegna la vittoria alla squadra 0 perche' e'
		// la prima nell'ordine, e non si prolunga: l'overtime e' fuori scope v0.1 (spec §12).
		FRTMatchState State = MakeState();
		State.RoundNumber = 12;
		State.Team0Score = 2;
		State.Team1Score = 2;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("punteggi pari -> pareggio"), Result.Outcome == ERTMatchOutcome::Draw);
		TestTrue(TEXT("per scadenza dei round"), Result.Reason == ERTMatchEndReason::RoundLimit);
	}
	{
		// Nessuno ha segnato: e' comunque un pareggio dichiarato, non una partita senza esito.
		FRTMatchState State = MakeState();
		State.RoundNumber = 12;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
		TestTrue(TEXT("zero a zero -> pareggio"), Result.Outcome == ERTMatchOutcome::Draw);
		TestTrue(TEXT("per scadenza dei round"), Result.Reason == ERTMatchEndReason::RoundLimit);
	}
	{
		// Simmetria: scambiare le due squadre scambia l'esito e non lo inventa. Un pareggio resta pareggio.
		FRTMatchState Mirrored = MakeState();
		Mirrored.RoundNumber = 12;
		Mirrored.Team0Score = 1;
		Mirrored.Team1Score = 4;
		const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(Mirrored, Rules);
		TestTrue(TEXT("vince la 1"), Result.Outcome == ERTMatchOutcome::Team1Wins);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchNoLimitNeverExpiresTest,
	"RefactorTactics.Match.NoRoundLimitNeverExpires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchNoLimitNeverExpiresTest::RunTest(const FString&)
{
	// La regola pura deve essere TOTALE: un limite non positivo non arriva mai dal validator, ma se
	// arrivasse non deve chiudere la partita al primo round.
	FRTMatchRules Rules = MakeRules(0, 0);
	FRTMatchState State = MakeState();
	State.RoundNumber = 99;

	const FRTMatchResult Result = URTTurnRules::EvaluateMatchEnd(State, Rules);
	TestTrue(TEXT("senza limite non si scade"), Result.Outcome == ERTMatchOutcome::InProgress);
	TestTrue(TEXT("nessuna via"), Result.Reason == ERTMatchEndReason::None);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
