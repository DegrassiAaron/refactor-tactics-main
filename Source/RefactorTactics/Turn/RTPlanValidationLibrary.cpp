#include "Turn/RTPlanValidationLibrary.h"

namespace
{
	/** Rifiuto con motivo e colpevole: un solo punto di costruzione, cosi' nessun ramo dimentica un campo. */
	FRTPlanValidation Reject(ERTActionInvalidReason Reason, const FName& ActionId)
	{
		FRTPlanValidation Result;
		Result.bLegal = false;
		Result.Reason = Reason;
		Result.OffendingActionId = ActionId;
		return Result;
	}
}

int32 URTPlanValidationLibrary::SlotWidth(ERTActionSlot Slot)
{
	switch (Slot)
	{
	case ERTActionSlot::None:
		return 0;
	case ERTActionSlot::MovementAndMain:
		return 2;
	default:
		return 1;
	}
}

FRTPlanValidation URTPlanValidationLibrary::ValidatePlan(const FRTHexSimUnit& Unit,
	const TArray<FRTPlannedAction>& Plan)
{
	// Ordine canonico: chi occupa PIU' slot si colloca per primo, a parita' si va per `ActionId`. Serve a
	// rendere il verdetto indipendente dall'ordine di composizione — e a dare il colpevole giusto: davanti a
	// `Sprint` + attacco il rifiutato e' l'attacco, non lo `Sprint` che occupa entrambi gli slot.
	TArray<FRTPlannedAction> Ordered = Plan;
	Ordered.Sort([](const FRTPlannedAction& A, const FRTPlannedAction& B)
	{
		const int32 WidthA = SlotWidth(A.Def.Slot);
		const int32 WidthB = SlotWidth(B.Def.Slot);
		if (WidthA != WidthB)
		{
			return WidthA > WidthB;
		}
		return A.Def.ActionId.Compare(B.Def.ActionId) < 0;
	});

	// 1. Intrinseco: un'abilita' in cooldown non e' pianificabile nemmeno con tutti gli slot liberi.
	for (const FRTPlannedAction& Entry : Ordered)
	{
		if (Entry.CooldownRemaining > 0)
		{
			return Reject(ERTActionInvalidReason::OnCooldown, Entry.Def.ActionId);
		}
	}

	// 2. Somma di piano: i Movement Point sono interi, e spendere ESATTAMENTE il budget e' legale.
	int32 TotalCostMP = 0;
	for (const FRTPlannedAction& Entry : Ordered)
	{
		TotalCostMP += Entry.Def.CostMP;
	}
	if (TotalCostMP > Unit.MoveBudget)
	{
		// Il colpevole e' la voce piu' cara del piano: e' quella che, tolta, lo renderebbe pianificabile.
		const FRTPlannedAction* Costliest = nullptr;
		for (const FRTPlannedAction& Entry : Ordered)
		{
			if (Costliest == nullptr || Entry.Def.CostMP > Costliest->Def.CostMP)
			{
				Costliest = &Entry;
			}
		}
		return Reject(ERTActionInvalidReason::InsufficientMovementPoints,
			Costliest ? Costliest->Def.ActionId : NAME_None);
	}

	// 3. Combinazione: l'occupazione degli slot. `None` non toglie niente (`Action.Wait`).
	bool bMovementTaken = false;
	bool bMainTaken = false;
	bool bReactionTaken = false;
	for (const FRTPlannedAction& Entry : Ordered)
	{
		const bool bWantsMovement = Entry.Def.Slot == ERTActionSlot::Movement
			|| Entry.Def.Slot == ERTActionSlot::MovementAndMain;
		const bool bWantsMain = Entry.Def.Slot == ERTActionSlot::Main
			|| Entry.Def.Slot == ERTActionSlot::MovementAndMain;
		const bool bWantsReaction = Entry.Def.Slot == ERTActionSlot::Reaction;

		if ((bWantsMovement && bMovementTaken) || (bWantsMain && bMainTaken)
			|| (bWantsReaction && bReactionTaken))
		{
			return Reject(ERTActionInvalidReason::SlotOccupied, Entry.Def.ActionId);
		}

		bMovementTaken |= bWantsMovement;
		bMainTaken |= bWantsMain;
		bReactionTaken |= bWantsReaction;
	}

	return FRTPlanValidation();
}
