#include "Turn/RTReactionLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Turn/RTActionEffectLibrary.h"
#include "Turn/RTActionQueue.h"

namespace
{
	/**
	 * Chi subisce l'effetto di una reazione: chi reagisce, o chi ha innescato. Switch senza `default`, cosi'
	 * un effetto nuovo non puo' entrare nell'enum senza che qualcuno decida dove punta quando e' una reazione.
	 */
	bool ReactionEffectTargetsTriggerer(ERTActionEffect Effect)
	{
		switch (Effect)
		{
		case ERTActionEffect::Damage:
		case ERTActionEffect::Push:
		case ERTActionEffect::Pull:
		case ERTActionEffect::Status:
			return true; // reagire CONTRO chi ha colpito

		case ERTActionEffect::None:
		case ERTActionEffect::Heal:
		case ERTActionEffect::Shield:
		case ERTActionEffect::DamageReduction:
		// `SelfReposition` e `CancelDisplacement` sono difensivi per DEFINIZIONE: il primo sposta chi
		// reagisce, il secondo annulla lo spostamento che sta per subire. Puntarli a chi ha innescato
		// significherebbe spostare l'attaccante, che e' `Push` — un effetto che esiste gia' ed e' un'altra cosa.
		case ERTActionEffect::SelfReposition:
		case ERTActionEffect::CancelDisplacement:
		// Le due che non colpiscono un'unita': una reazione non le dichiara oggi, ma senza i loro `case` lo
		// switch non era esaustivo e il commento qui sopra prometteva una garanzia che non dava.
		case ERTActionEffect::DamageStructure:
		case ERTActionEffect::SetDoorState:
			return false; // difendere SE STESSI
		}
		return false;
	}
}

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

ERTReactionPassPoint URTReactionLibrary::PassPointFor(ERTReactionTrigger Trigger)
{
	// Senza `default`: e' l'unica forma che rende impossibile aggiungere un trigger e dimenticarsi di dire
	// dove viene valutato. Con un `default` il trigger nuovo compilerebbe, non scatterebbe mai, e il suo test
	// sul catalogo resterebbe verde — il modo esatto in cui i tre moduli di `#505` sono rimasti fermi.
	switch (Trigger)
	{
	case ERTReactionTrigger::None:
		return ERTReactionPassPoint::Never;
	case ERTReactionTrigger::HitByDirectAttack:
		return ERTReactionPassPoint::BlastHits;
	case ERTReactionTrigger::AllyHitByDirectAttack:
		return ERTReactionPassPoint::BlastIntercept;
	case ERTReactionTrigger::AboutToBeDisplaced:
		return ERTReactionPassPoint::BlastDisplacement;
	}
	return ERTReactionPassPoint::Never;
}

TArray<FRTActionEvent> URTReactionLibrary::BuildReactionEvents(const FRTActionDef& Def, int32 SelfId,
	int32 TriggeredBy)
{
	TArray<FRTActionEvent> Events;
	if (SelfId == INDEX_NONE)
	{
		return Events;
	}

	// Un effetto per volta, nell'ORDINE dichiarato: la traduzione (entita' non positiva, stato senza tag,
	// bersaglio assente) resta quella di `URTActionEffectLibrary::ProduceEvents`, non una seconda copia che
	// puo' divergere. Il prezzo e' un'istanza per effetto: una reazione ne dichiara al piu' una manciata.
	for (const FRTActionEffectSpec& Spec : Def.Effects)
	{
		const int32 TargetId = ReactionEffectTargetsTriggerer(Spec.Effect) ? TriggeredBy : SelfId;
		if (TargetId == INDEX_NONE)
		{
			continue; // nessun attaccante identificato: l'effetto offensivo non si applica a caso
		}

		FRTActionInstance Instance;
		Instance.Def = Def;
		Instance.Def.Effects = { Spec };
		Instance.SourceUnitId = SelfId;
		Instance.TargetUnitId = TargetId;
		Instance.EventSequence = Events.Num();
		Events.Append(URTActionEffectLibrary::ProduceEvents(Instance));
	}

	return Events;
}
