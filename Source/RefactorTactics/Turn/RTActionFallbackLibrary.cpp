#include "Turn/RTActionFallbackLibrary.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

ERTActionInvalidReason URTActionFallbackLibrary::ValidateInstance(const FRTActionInstance& Instance,
	const TArray<FRTHexCombatUnit>& Units, const URTHexMapAsset* Map)
{
	// Un'azione senza bersaglio-unita' non ha nulla da validare qui: il movimento lo giudica il resolver dei
	// percorsi, il supporto su se stessi non puo' "perdere il bersaglio".
	if (Instance.TargetUnitId == INDEX_NONE)
	{
		return ERTActionInvalidReason::None;
	}

	if (!Units.IsValidIndex(Instance.SourceUnitId) || !Units.IsValidIndex(Instance.TargetUnitId))
	{
		return ERTActionInvalidReason::TargetGone;
	}

	const FRTHexCombatUnit& Source = Units[Instance.SourceUnitId];
	const FRTHexCombatUnit& Target = Units[Instance.TargetUnitId];

	// Su se stessi (scudi, stance) non si valuta ne' squadra ne' portata: si e' sempre a portata di se'.
	if (Instance.SourceUnitId == Instance.TargetUnitId)
	{
		return ERTActionInvalidReason::None;
	}

	if (!Target.bAlive)
	{
		return ERTActionInvalidReason::TargetDead;
	}
	if (Target.TeamId == Source.TeamId)
	{
		return ERTActionInvalidReason::TargetFriendly;
	}

	// Portata PRIMA della linea di tiro: sono due difetti diversi da correggere per chi gioca, e attribuire
	// ogni rifiuto alla copertura renderebbe il log una bugia (stessa disciplina di ClassifyHexTargeting).
	// La portata e' quella EFFETTIVA: il terreno (Fumo) la cappa. Il controllo NON passa da ClassifyHexTargeting
	// (qui serve distinguere anche NoMap da OutOfRange con quest'ordine), quindi la regola si chiama diretta —
	// stessa funzione condivisa, nessuna seconda copia della logica. Con `Map == nullptr` il cap e' un no-op e
	// l'ordine dei motivi resta quello di prima.
	const int32 EffectiveRange = URTTerrainLibrary::EffectiveTargetingRange(Map, Source.Cell, Target.Cell,
		Instance.Def.RangeCells);
	if (URTHexLibrary::HexDistance(Source.Cell, Target.Cell) > EffectiveRange)
	{
		return ERTActionInvalidReason::OutOfRange;
	}

	// FAIL-CLOSED: senza mappa autorevole non si valida la traiettoria, quindi non si colpisce. Il motivo resta
	// distinto da una copertura: e' un difetto del livello, non un esito di gioco.
	if (Map == nullptr)
	{
		return ERTActionInvalidReason::NoMap;
	}
	if (URTCombatLibrary::ClassifyHexTargeting(Map, Source.Cell, Target.Cell, Instance.Def.RangeCells)
		== ERTHexTargetReason::NoLineOfSight)
	{
		return ERTActionInvalidReason::NoLineOfSight;
	}

	return ERTActionInvalidReason::None;
}

FRTFallbackResult URTActionFallbackLibrary::ApplyFallback(const FRTActionInstance& Instance,
	ERTActionInvalidReason Reason)
{
	FRTFallbackResult Result;
	Result.Instance = Instance;
	Result.Applied = Instance.Def.Fallback;
	Result.bProducesEffects = false;

	// Azione ancora valida: nessun fallback da applicare, si esegue quella pianificata.
	if (Reason == ERTActionInvalidReason::None)
	{
		Result.bProducesEffects = true;
		return Result;
	}

	switch (Instance.Def.Fallback)
	{
	case ERTActionFallback::Stop:
		// Regola del movimento: si procede fino all'ultima cella valida. L'istanza resta intatta — a fermarla
		// e' il resolver dei percorsi, che avanza a micro-step e non ricalcola nulla.
		Result.bProducesEffects = true;
		break;

	case ERTActionFallback::AttackCell:
	{
		// L'area parte comunque: si perde il bersaglio, si tiene la CELLA pianificata. Dove hai puntato, li'
		// colpisci — ma solo se il problema era il BERSAGLIO. Se era la geometria (fuori portata, traiettoria
		// bloccata, nessuna mappa) la cella ha lo stesso difetto, e colpirla vorrebbe dire sparare attraverso
		// il muro o oltre la propria portata. `TargetGone` e' escluso perche' senza il bersaglio non si sa
		// nemmeno DOVE fosse: non c'e' una cella su cui ripiegare.
		const bool bCellStillUsable = (Reason == ERTActionInvalidReason::TargetDead
			|| Reason == ERTActionInvalidReason::TargetFriendly);
		if (!bCellStillUsable)
		{
			Result.Instance.Def.Effects.Reset();
			Result.Instance.TargetUnitId = INDEX_NONE;
			Result.Applied = ERTActionFallback::Cancel;
			break;
		}
		Result.Instance.TargetUnitId = INDEX_NONE;
		Result.bProducesEffects = true;
		break;
	}

	case ERTActionFallback::AttackTarget:
		// «Segue il bersaglio, se ancora valido»: qui il bersaglio NON e' valido (siamo nel fallback perche'
		// la validazione e' fallita), quindi non c'e' nessuno da seguire e l'azione si annulla. Seguire
		// qualcun altro sarebbe targeting automatico.
		Result.Instance.Def.Effects.Reset();
		Result.Instance.TargetUnitId = INDEX_NONE;
		Result.Applied = ERTActionFallback::Cancel;
		break;

	case ERTActionFallback::Wait:
	{
		// Sostituisce l'azione con `Action.Wait`: presa dal catalogo, non inventata qui. Occupa lo slot, non
		// produce effetti, e resta osservabile nel TurnLog.
		FRTActionDef WaitDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Wait"));
		if (WaitDef.ActionId.IsNone())
		{
			WaitDef = Instance.Def;
			WaitDef.Effects.Reset(); // catalogo incompleto: meglio un'azione vuota che una che agisce a caso
		}
		Result.Instance.Def = WaitDef;
		Result.Instance.TargetUnitId = INDEX_NONE;
		break;
	}

	case ERTActionFallback::BasicAttack:
		// Non praticabile in v0.1: «il bersaglio valido piu' vicino» e' targeting automatico, che il catalogo
		// §17 elenca fra gli errori da evitare (esiti poco leggibili). Degrada a Cancel e lo DICHIARA, cosi'
		// il log dice cosa e' successo davvero invece di far sparire l'azione.
		Result.Instance.Def.Effects.Reset();
		Result.Instance.TargetUnitId = INDEX_NONE;
		Result.Applied = ERTActionFallback::Cancel;
		break;

	case ERTActionFallback::Cancel:
		// Attacchi diretti e cure: non si esegue nulla. Nessun effetto, nessun bersaglio di ripiego.
		Result.Instance.Def.Effects.Reset();
		Result.Instance.TargetUnitId = INDEX_NONE;
		break;
	}

	// Nessun ramo qui sopra guarda le altre unita': e' voluto. Un fallback che SCEGLIE un bersaglio sarebbe
	// targeting automatico, e la firma senza il contesto delle unita' rende la regola impossibile da violare
	// per distrazione — non solo sconsigliata a parole.
	return Result;
}

ERTFallbackOutcome URTActionFallbackLibrary::ToLogOutcome(ERTActionFallback Fallback)
{
	// Un caso per valore, nessun `default`: aggiungere un fallback senza dargli un esito di log non compila.
	switch (Fallback)
	{
	case ERTActionFallback::Stop:         return ERTFallbackOutcome::Stopped;
	case ERTActionFallback::Wait:         return ERTFallbackOutcome::Waited;
	case ERTActionFallback::AttackCell:   return ERTFallbackOutcome::AttackedCell;
	case ERTActionFallback::AttackTarget: return ERTFallbackOutcome::Cancelled; // nessun bersaglio da seguire
	case ERTActionFallback::BasicAttack:  return ERTFallbackOutcome::Cancelled; // niente targeting automatico
	case ERTActionFallback::Cancel:       return ERTFallbackOutcome::Cancelled;
	}
	return ERTFallbackOutcome::Cancelled;
}
