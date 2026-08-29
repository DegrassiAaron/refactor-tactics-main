#include "Turn/RTTurnRules.h"

ERTMatchPhase URTTurnRules::NextPhase(ERTMatchPhase Phase)
{
	switch (Phase)
	{
	case ERTMatchPhase::Planning: return ERTMatchPhase::Prep;
	case ERTMatchPhase::Prep:     return ERTMatchPhase::Dash;
	case ERTMatchPhase::Dash:     return ERTMatchPhase::Blast;
	case ERTMatchPhase::Blast:    return ERTMatchPhase::Move;
	case ERTMatchPhase::Move:     return ERTMatchPhase::Cleanup;
	case ERTMatchPhase::Cleanup:  return ERTMatchPhase::Planning;
	case ERTMatchPhase::MatchEnded:
	default:                      return ERTMatchPhase::MatchEnded;
	}
}

ERTMatchOutcome URTTurnRules::EvaluateOutcome(int32 Team0Alive, int32 Team1Alive)
{
	const bool bTeam0Dead = Team0Alive <= 0;
	const bool bTeam1Dead = Team1Alive <= 0;

	if (bTeam0Dead && bTeam1Dead) { return ERTMatchOutcome::Draw; }
	if (bTeam1Dead)               { return ERTMatchOutcome::Team0Wins; }
	if (bTeam0Dead)               { return ERTMatchOutcome::Team1Wins; }
	return ERTMatchOutcome::InProgress;
}

namespace
{
	/** Confronto di due punteggi: pareggio DICHIARATO se pari, mai la prima squadra per posizione. */
	ERTMatchOutcome CompareScores(int32 Team0Score, int32 Team1Score)
	{
		if (Team0Score > Team1Score) { return ERTMatchOutcome::Team0Wins; }
		if (Team1Score > Team0Score) { return ERTMatchOutcome::Team1Wins; }
		return ERTMatchOutcome::Draw;
	}
}

ERTObjectiveOutcome URTTurnRules::ResolveObjectiveControl(int32 Team0Present, int32 Team1Present)
{
	// Presenze negative non arrivano da nessun chiamante — sono conteggi — ma la regola resta TOTALE invece
	// di avere un buco: un valore assurdo si comporta come «nessuno», non come un controllo.
	const int32 Team0 = FMath::Max(0, Team0Present);
	const int32 Team1 = FMath::Max(0, Team1Present);

	if (Team0 == 0 && Team1 == 0)
	{
		return ERTObjectiveOutcome::Unclaimed;
	}
	// Il pareggio si dichiara PRIMA dei due controlli, come `CompareScores` sopra: scritto dopo, un `>` e un
	// `>=` che divergono farebbero vincere la squadra 0 per posizione nell'array.
	if (Team0 == Team1)
	{
		return ERTObjectiveOutcome::Contested;
	}
	return Team0 > Team1 ? ERTObjectiveOutcome::Team0Scores : ERTObjectiveOutcome::Team1Scores;
}

FRTMatchResult URTTurnRules::EvaluateMatchEnd(const FRTMatchState& State, const FRTMatchRules& Rules)
{
	// 1. Eliminazione: e' la via che il vertical slice ha da sempre, e resta la prima.
	const ERTMatchOutcome ByElimination = EvaluateOutcome(State.Team0Alive, State.Team1Alive);
	if (ByElimination != ERTMatchOutcome::InProgress)
	{
		return FRTMatchResult(ByElimination, ERTMatchEndReason::Elimination);
	}

	// 2. Obiettivo: soglia positiva raggiunta da almeno una squadra. Se la toccano entrambe nello stesso
	// Cleanup decide il punteggio piu' alto, e la parita' resta un pareggio.
	if (Rules.ScoreToWin > 0
		&& (State.Team0Score >= Rules.ScoreToWin || State.Team1Score >= Rules.ScoreToWin))
	{
		return FRTMatchResult(CompareScores(State.Team0Score, State.Team1Score), ERTMatchEndReason::Objective);
	}

	// 3. Scadenza dei round. Un limite non positivo non arriva dal validator, ma la regola resta totale:
	// senza limite non si scade, invece di scadere al primo round.
	if (Rules.RoundLimit > 0 && State.RoundNumber >= Rules.RoundLimit)
	{
		return FRTMatchResult(CompareScores(State.Team0Score, State.Team1Score), ERTMatchEndReason::RoundLimit);
	}

	return FRTMatchResult(ERTMatchOutcome::InProgress, ERTMatchEndReason::None);
}

FString URTTurnRules::DescribeOutcome(ERTMatchOutcome Outcome)
{
	switch (Outcome)
	{
	case ERTMatchOutcome::Team0Wins: return TEXT("Vince il team 0 (blu)");
	case ERTMatchOutcome::Team1Wins: return TEXT("Vince il team 1 (rosso)");
	case ERTMatchOutcome::Draw:      return TEXT("Pareggio");
	case ERTMatchOutcome::InProgress:
	default:                         return TEXT("Partita in corso");
	}
}

FString URTTurnRules::DescribeEndReason(ERTMatchEndReason Reason)
{
	switch (Reason)
	{
	case ERTMatchEndReason::Elimination: return TEXT("per eliminazione");
	case ERTMatchEndReason::Objective:   return TEXT("per obiettivo");
	case ERTMatchEndReason::RoundLimit:  return TEXT("allo scadere dei round");
	case ERTMatchEndReason::None:
	default:                             return TEXT("nessuna via");
	}
}
