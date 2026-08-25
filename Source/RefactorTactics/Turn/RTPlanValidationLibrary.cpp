#include "Turn/RTPlanValidationLibrary.h"

#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Unit/RTUnit.h"

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

// `Unit` resta nella FIRMA e non e' letto dal corpo: con [D-190] l'unica cosa che il validatore leggeva
// dallo snapshot — `MoveBudget` — e' uscita insieme al ramo dei Movement Point. La firma non si accorcia
// perche' e' il contratto dichiarato di CP 38.2 (*«una funzione pura sullo snapshot + piano»*) e perche' il
// lavoro che segue ne ha bisogno: il bot valida contro lo stato, e CP 38.3 legge il profilo di movimento
// dell'unita'. Il nome e' commentato via invece che lasciato morto, cosi' chi legge vede che l'assenza di
// letture e' voluta e non una dimenticanza.
TArray<FRTPlannedAction> URTPlanValidationLibrary::MakePlanFor(const ARTUnit* Unit)
{
	TArray<FRTPlannedAction> Plan;
	if (!Unit)
	{
		return Plan;
	}

	// Una voce per ogni abilita' pianificata. Lo `Slot` arriva dal catalogo insieme al resto del `Def`:
	// leggerlo qui, invece di dedurlo dal CAMPO in cui l'abilita' e' scritta, e' cio' che tiene questo
	// adattatore ignaro dei kit — `PlannedDashAbility` non significa "slot movimento", significa "l'azione
	// che questa unita' ha messo nel suo scatto", e quale slot occupi lo dice lei.
	auto AddAbility = [&Plan, Unit](int32 Index)
	{
		const URTActionData* Ability = Unit->GetAbility(Index);
		if (!Ability)
		{
			return;
		}
		FRTPlannedAction Entry;
		Entry.Def = Ability->Def;
		// Il cooldown residuo vive sull'Actor, in un array privato parallelo ad `Abilities`: `GetAbilityCooldown`
		// e' l'unico accesso, ed e' la ragione per cui `FRTPlannedAction` lo porta come dato invece di andarselo
		// a prendere (vedi il commento della struct).
		Entry.CooldownRemaining = Unit->GetAbilityCooldown(Index);
		Plan.Add(Entry);
	};

	AddAbility(Unit->PlannedDashAbility);
	AddAbility(Unit->PlannedAbilityIndex);
	AddAbility(Unit->PlannedReactionAbility);

	// Il movimento normale: una destinazione diversa dalla cella attuale, oppure un percorso a waypoint.
	// Le due condizioni non sono ridondanti — chi posa waypoint scrive entrambi, ma il bot pianifica
	// destinazioni e non percorsi (`ARTTurnManager::PlanBots`), quindi guardare il solo `PlannedPath`
	// renderebbe invisibile ogni movimento del bot.
	const bool bMoves = Unit->PlannedCell != Unit->Cell || Unit->PlannedPath.Num() > 1;
	if (bMoves)
	{
		const FRTActionDef Move = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move"));
		if (!Move.ActionId.IsNone())
		{
			FRTPlannedAction Entry;
			Entry.Def = Move;
			Plan.Add(Entry);
		}
	}

	return Plan;
}

FRTPlanValidation URTPlanValidationLibrary::ValidatePlan(const FRTHexSimUnit& /*Unit*/,
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

	// 2. Combinazione: l'occupazione degli slot. `None` non toglie niente (`Action.Wait`).
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
