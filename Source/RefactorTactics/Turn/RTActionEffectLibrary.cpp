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

		case ERTActionEffect::Damage:
		case ERTActionEffect::Heal:
		case ERTActionEffect::Shield:
		case ERTActionEffect::Push:
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
