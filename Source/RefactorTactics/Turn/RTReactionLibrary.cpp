#include "Turn/RTReactionLibrary.h"

bool URTReactionLibrary::EvaluateReactionTrigger(ERTReactionTrigger Trigger, int32 SelfId,
	const TArray<FRTHexAttackHit>& Hits, const TArray<FRTHexAttackIntent>& Intents)
{
	switch (Trigger)
	{
	case ERTReactionTrigger::HitByDirectAttack:
		for (const FRTHexAttackHit& Hit : Hits)
		{
			if (Hit.TargetId != SelfId)
			{
				continue;
			}
			if (Intents.IsValidIndex(Hit.IntentIndex) && Intents[Hit.IntentIndex].Shape == ERTAbilityShape::Single)
			{
				return true;
			}
		}
		return false;

	case ERTReactionTrigger::None:
	default:
		return false;
	}
}
