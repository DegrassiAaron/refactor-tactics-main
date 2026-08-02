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
