#include "Turn/RTReactionLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"

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

int32 URTReactionLibrary::FindInterceptableHit(int32 SelfId, int32 InterceptRange,
	const TArray<FRTHexAttackHit>& Hits, const TArray<FRTHexAttackIntent>& Intents,
	const TArray<FRTHexCombatUnit>& Units, const URTHexMapAsset* Map,
	const TSet<int32>& ExcludedHits)
{
	if (!Map || !Units.IsValidIndex(SelfId) || !Units[SelfId].bAlive)
	{
		return INDEX_NONE; // fail-closed: senza mappa la traiettoria non e' verificabile
	}
	const FRTHexCombatUnit& Self = Units[SelfId];

	for (int32 h = 0; h < Hits.Num(); ++h)
	{
		if (ExcludedHits.Contains(h))
		{
			continue; // gia' intercettato da un alleato in questo stesso Blast
		}
		const FRTHexAttackHit& Hit = Hits[h];

		// Un alleato DIVERSO da chi intercetta: interporsi davanti a se stessi non e' un'interposizione.
		if (Hit.TargetId == SelfId || !Units.IsValidIndex(Hit.TargetId)) { continue; }
		const FRTHexCombatUnit& Victim = Units[Hit.TargetId];
		if (Victim.TeamId != Self.TeamId) { continue; }

		// Solo attacchi DIRETTI: un'area non ha una traiettoria in cui mettersi in mezzo.
		if (!Intents.IsValidIndex(Hit.IntentIndex)
			|| Intents[Hit.IntentIndex].Shape != ERTAbilityShape::Single)
		{
			continue;
		}

		// L'alleato dev'essere abbastanza vicino da poterlo coprire.
		if (URTHexLibrary::HexDistance(Self.Cell, Victim.Cell) > InterceptRange) { continue; }

		// La traiettoria deve arrivare a CHI intercetta: il colpo cambia bersaglio, non percorso.
		if (!Units.IsValidIndex(Hit.AttackerId)) { continue; }
		if (!URTHexVisionLibrary::HasLineOfSight(Map, Units[Hit.AttackerId].Cell, Self.Cell)) { continue; }

		return h;
	}
	return INDEX_NONE;
}
