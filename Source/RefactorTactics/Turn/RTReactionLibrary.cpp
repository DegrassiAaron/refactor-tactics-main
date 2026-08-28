#include "Turn/RTReactionLibrary.h"
#include "Core/RTGameplayTags.h" // TAG_Status_Root/Slow: la lista degli stati di controllo (CP 7.5)
#include "Map/RTHexLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h" // GraphNeighbors: uno scarto passa dal grafo, non dai vicini geometrici
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
		case ERTActionEffect::CancelStatus:
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
	case ERTReactionTrigger::AboutToReceiveControl:
		return ERTReactionPassPoint::BlastStatus;
	case ERTReactionTrigger::CellBecameHazardous:
		return ERTReactionPassPoint::CleanupSurfaceBirth;
	}
	return ERTReactionPassPoint::Never;
}

const TArray<FGameplayTag>& URTReactionLibrary::ControlStatusesBySeverity()
{
	// `static` e non ricostruito a ogni chiamata: e' consultato una volta per stato in arrivo, dentro il
	// Blast. L'ordine E' il contratto — vedi il commento nell'header.
	static const TArray<FGameplayTag> Controls = { TAG_Status_Root, TAG_Status_Slow };
	return Controls;
}

int32 URTReactionLibrary::ControlSeverityRank(const FGameplayTag& Tag)
{
	return ControlStatusesBySeverity().IndexOfByKey(Tag);
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

FRTCellId URTReactionLibrary::FindSidestepCell(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& PushFrom, ERTHexDirection Facing, const TArray<FRTCellId>& Occupied)
{
	if (!Map)
	{
		return From; // fail-closed: senza mappa non si sa dove si puo' andare, come `FindEscapeCell`
	}

	// Le due celle DELLA LINEA, che sono esattamente cio' da cui si esce: quella verso cui la spinta manda —
	// `PushFrom -> From` prolungata — e quella da cui la spinta arriva. Si ricavano dalla direzione della
	// spinta invece che dalla geometria a mano: `DirectionTowards` e' la stessa funzione che il resolver usa
	// per orientare, quindi «sulla linea» significa qui cio' che significa altrove.
	ERTHexDirection PushDir;
	if (!URTHexLibrary::DirectionTowards(PushFrom, From, PushDir))
	{
		// Attaccante e bersaglio sulla stessa cella assiale: non c'e' una linea da cui uscire. Non e' un
		// errore — e' il caso in cui `HexKnockbackDestination` stesso non produce spostamento.
		return From;
	}
	const FRTCellId Ahead  = URTHexLibrary::Neighbor(From, PushDir);
	const FRTCellId Behind = URTHexLibrary::Neighbor(From, URTHexLibrary::OppositeDirection(PushDir));

	// Le celle davvero raggiungibili: il GRAFO, non i sei vicini geometrici. Un dislivello senza arco o un
	// muro non sono uno scarto solo perche' la cella e' adiacente sulla griglia — stessa disciplina di
	// `URTTerrainLibrary::FindEscapeCell`, e per la stessa ragione.
	TArray<FRTCellId> Reachable;
	for (const TPair<FRTCellId, int32>& Step : URTHexPathLibrary::GraphNeighbors(Map, From))
	{
		Reachable.Add(Step.Key);
	}

	auto IsSidestep = [&Reachable, &Occupied, &Ahead, &Behind](const FRTCellId& Candidate)
	{
		// ⚠️ **L'esclusione della linea viene PRIMA della raggiungibilita'**, e l'ordine e' la regola: se
		// `Ahead` fosse raggiungibile e libera, un controllo scritto al contrario la offrirebbe come scarto —
		// cioe' riprodurrebbe esattamente la risposta dominata che questa funzione esiste per togliere.
		if (Candidate == Ahead || Candidate == Behind) { return false; }
		return Reachable.Contains(Candidate) && !Occupied.Contains(Candidate);
	};

	// 1) Dove sta guardando: e' la scelta che il giocatore puo' prevedere senza conoscere l'ordine interno
	//    delle direzioni ([D-104], e il precedente di `FindEscapeCell`).
	const FRTCellId Faced = URTHexLibrary::Neighbor(From, Facing);
	if (IsSidestep(Faced))
	{
		return Faced;
	}

	// 2) Ripiego nell'ordine canonico delle direzioni (E, NE, NW, W, SW, SE): arbitrario per il giocatore ma
	//    DICHIARATO e stabile, quindi due situazioni identiche danno lo stesso scarto.
	for (const FRTCellId& Candidate : URTHexLibrary::Neighbors(From))
	{
		if (IsSidestep(Candidate))
		{
			return Candidate;
		}
	}

	// 3) Nessuna cella fuori dalla linea: chi chiama ripiega su `Hold Ground`. ⚠️ E' l'opposto di
	//    `EmergencyDash`, che in questo caso si SPRECA: li' una reazione si consuma, qui `Hold Ground` non e'
	//    una risorsa — chi sceglie di scartare non deve finire meno protetto di chi non ha scelto affatto.
	return From;
}
