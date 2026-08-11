#include "Turn/RTActionEffectLibrary.h"

TArray<FRTActionEvent> URTActionEffectLibrary::ProduceEvents(const FRTActionInstance& Instance)
{
	TArray<FRTActionEvent> Events;

	// Un'azione interrotta non produce NULLA: e' tutto o niente, non mezzo danno (catalogo §3, `HeavyAttack`).
	// Il controllo sta qui, prima della traduzione, cosi' vale per ogni azione senza che nessuna se ne occupi;
	// `bCanBeInterrupted == false` (Guard, Barrier, SuppressiveLine) rende il flag ininfluente per costruzione.
	if (Instance.bInterrupted && Instance.Def.bCanBeInterrupted)
	{
		return Events;
	}

	// Un'azione puo' dichiarare piu' effetti: si traducono nell'ordine dichiarato, che e' anche l'ordine in
	// cui il chiamante li applichera' (il danno prima della spinta, se l'azione li elenca cosi').
	for (const FRTActionEffectSpec& Spec : Instance.Def.Effects)
	{
		FRTActionEvent Event;
		Event.Kind = Spec.Effect;
		Event.SourceUnitId = Instance.SourceUnitId;
		Event.TargetUnitId = Instance.TargetUnitId;
		Event.Cell = Instance.TargetCell;

		// Un caso per ogni effetto: niente `default`, cosi' aggiungerne uno senza tradurlo non compila.
		switch (Spec.Effect)
		{
		case ERTActionEffect::None:
			continue; // dichiarato ma vuoto: l'azione occupa lo slot e basta

		case ERTActionEffect::SetDoorState:
			// Come `DamageStructure`: non e' un evento verso un'unita'. Lo raccoglie il Blast sul bordo, e un
			// evento con `TargetUnitId` valido lo farebbe applicare a chi sta dietro la porta.
			//
			// Questo `case` NON e' garantito dallo switch: il commento qui sopra promette che aggiungere un
			// effetto senza tradurlo non compili, ma non e' vero — passerebbe in silenzio producendo un evento
			// con `Amount` 0.
			continue;

		case ERTActionEffect::DamageStructure:
			// Non colpisce un'UNITA': la barriera non e' un bersaglio del registry degli effetti, e un evento
			// con `TargetUnitId` valido le farebbe applicare danno a chi sta dietro. Lo raccoglie il Blast sul
			// primo bordo coperto attraversato (`URTHexCombatLibrary::CollectHexAttacks`) e lo applica a fase
			// conclusa. Trovato da `Actions.HeavyAttack.NoEffectIfInterrupted`, che contava due eventi dove
			// l'azione ne ha uno solo verso il bersaglio.
			continue;

		case ERTActionEffect::Damage:
		case ERTActionEffect::Heal:
		case ERTActionEffect::Shield:
		case ERTActionEffect::Push:
		case ERTActionEffect::Pull:
		case ERTActionEffect::DamageReduction:
		// `SelfReposition` sta qui e non in un ramo suo: la traduzione e' identica — entita' positiva e un
		// bersaglio che esiste. Cambia CHI e' quel bersaglio, e lo decide il chiamante
		// (`BuildReactionEvents`), non questa funzione: e' la stessa divisione di responsabilita' che vale
		// gia' per `Damage` in una reazione, dove il bersaglio e' l'attaccante e non chi la usa.
		case ERTActionEffect::SelfReposition:
			if (Spec.Amount <= 0 || Instance.TargetUnitId == INDEX_NONE)
			{
				continue; // entita' non positiva o nessun bersaglio: nessun evento da applicare
			}
			Event.Amount = Spec.Amount;
			break;

		case ERTActionEffect::Status:
			if (!Spec.StatusTag.IsValid() || Spec.StatusDuration <= 0 || Instance.TargetUnitId == INDEX_NONE)
			{
				continue; // uno stato senza tag o senza durata applicherebbe "qualcosa" di indefinito
			}
			Event.StatusTag = Spec.StatusTag;
			Event.Amount = Spec.StatusDuration; // la durata viaggia in Amount: interi soltanto
			break;
		}

		Events.Add(Event);
	}

	return Events;
}

TArray<FRTActionEvent> URTActionEffectLibrary::ProduceEventsForAll(const TArray<FRTActionInstance>& Instances)
{
	TArray<FRTActionEvent> Events;
	for (const FRTActionInstance& Instance : Instances)
	{
		Events.Append(ProduceEvents(Instance));
	}
	return Events;
}
