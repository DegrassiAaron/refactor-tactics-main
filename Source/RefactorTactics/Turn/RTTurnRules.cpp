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
