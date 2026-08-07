#include "Turn/RTReactionLibrary.h"

bool URTReactionLibrary::EvaluateReactionTrigger(ERTReactionTrigger Trigger, int32 SelfId,
	const TArray<FRTHexAttackHit>& Hits, const TArray<FRTHexAttackIntent>& Intents)
{
	// "Il trigger e' scattato" e "chi l'ha fatto scattare" sono la stessa domanda posta due volte: tenerle in
	// una funzione sola evita che le due risposte possano divergere (una reazione attiva senza attaccante, o
	// viceversa) quando arriveranno altri trigger.
	return FindTriggeringAttacker(Trigger, SelfId, Hits, Intents) != INDEX_NONE;
}

int32 URTReactionLibrary::FindTriggeringAttacker(ERTReactionTrigger Trigger, int32 SelfId,
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
				return Hit.AttackerId;
			}
		}
		return INDEX_NONE;

	case ERTReactionTrigger::None:
	default:
		return INDEX_NONE;
	}
}
