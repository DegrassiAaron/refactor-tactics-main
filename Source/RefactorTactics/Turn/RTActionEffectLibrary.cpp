#include "Turn/RTActionEffectLibrary.h"

TArray<FRTActionEvent> URTActionEffectLibrary::ProduceEvents(const FRTActionInstance& Instance)
{
	TArray<FRTActionEvent> Events;

	FRTActionEvent Event;
	Event.Kind = Instance.Def.Effect;
	Event.SourceUnitId = Instance.SourceUnitId;
	Event.TargetUnitId = Instance.TargetUnitId;
	Event.Cell = Instance.TargetCell;

	// Un caso per ogni effetto: niente `default`, cosi' aggiungerne uno senza tradurlo non compila.
	switch (Instance.Def.Effect)
	{
	case ERTActionEffect::None:
		return Events; // l'azione occupa lo slot e basta (es. attesa)

	case ERTActionEffect::Damage:
	case ERTActionEffect::Heal:
	case ERTActionEffect::Shield:
	case ERTActionEffect::Push:
		if (Instance.Def.EffectAmount <= 0 || Instance.TargetUnitId == INDEX_NONE)
		{
			return Events; // entita' non positiva o nessun bersaglio: nessun evento da applicare
		}
		Event.Amount = Instance.Def.EffectAmount;
		break;

	case ERTActionEffect::Status:
		if (!Instance.Def.StatusToApply.IsValid() || Instance.Def.StatusDuration <= 0
			|| Instance.TargetUnitId == INDEX_NONE)
		{
			return Events; // uno stato senza tag o senza durata applicherebbe "qualcosa" di indefinito
		}
		Event.StatusTag = Instance.Def.StatusToApply;
		Event.Amount = Instance.Def.StatusDuration; // la durata viaggia in Amount: interi soltanto
		break;
	}

	Events.Add(Event);
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
