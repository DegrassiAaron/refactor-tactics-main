#include "Turn/RTPlanValidationLibrary.h"

#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Unit/RTUnit.h"

namespace
{
	/** Rifiuto con motivo, colpevole e detentore dello slot: un solo punto di costruzione, cosi' nessun
	 *  ramo dimentica un campo. `Holder` resta `NAME_None` per i motivi che non sono conflitti di slot. */
	FRTPlanValidation Reject(ERTActionInvalidReason Reason, const FName& ActionId,
		const FName& Holder = NAME_None)
	{
		FRTPlanValidation Result;
		Result.bLegal = false;
		Result.Reason = Reason;
		Result.OffendingActionId = ActionId;
		Result.HolderActionId = Holder;
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
		// 🔴 Un'abilita' senza `ActionId` non viene dal catalogo: `ARTUnit::MakeAbility` popola i soli campi
		// mirror e lascia il `Def` vuoto, quindi il suo `Slot` e' il DEFAULT (`Main`) e non una dichiarazione.
		// Includerla darebbe al validatore un dato inventato — e un eventuale rifiuto nominerebbe
		// `NAME_None`, cioe' un verdetto che non dice quale azione ha sbagliato.
		//
		// ⚠️ **Il limite e' dichiarato**: un piano composto SOLO da abilita' non catalogate risulta vuoto,
		// quindi legale. E' il prezzo di non inventare, ed e' la stessa scelta fatta poche righe sotto per
		// `Action.Move`. Gli archetipi legacy (`EnsureDefaultAbilities`) sono gli unici a produrle: il roster
		// v0.1 passa tutto dal catalogo eroi. Il giorno in cui un consumatore reale ne incontrasse una, la
		// risposta e' dare un `Def` all'abilita', non indovinarne lo slot qui.
		if (Ability->Def.ActionId.IsNone())
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

	// Il movimento normale, con la regola in un posto solo: il PERCHE' delle due condizioni sta nel
	// docstring di `ARTUnit::HasPlannedNormalMove`, insieme alla regola che descrive.
	const bool bMoves = Unit->HasPlannedNormalMove();
	if (bMoves)
	{
		// `static const`, e non e' micro-ottimizzazione: `FindCoreAction` scorre `GetCoreActionCatalog()`, che
		// COSTRUISCE e restituisce un array di una trentina di `FRTActionDef` — ognuno con i propri `TArray`
		// annidati — a ogni invocazione. `MakePlanFor` e' il compositore dei consumatori in partita (la
		// preview della HUD a ogni click, il bot a ogni turno), quindi quell'allocazione cadrebbe una volta
		// per unita' che si muove. E' lo stesso rimedio, per la stessa causa, gia' applicato a
		// `GetReactionProfileCatalog` dopo una misura di code review.
		static const FRTActionDef Move = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move"));
		if (!Move.ActionId.IsNone())
		{
			FRTPlannedAction Entry;
			Entry.Def = Move;
			Plan.Add(Entry);
		}
	}

	return Plan;
}

// `Unit` resta nella FIRMA e non e' letto dal corpo: con [D-190] l'unica cosa che il validatore leggeva
// dallo snapshot — `MoveBudget` — e' uscita insieme al ramo dei Movement Point. La firma non si accorcia
// perche' e' il contratto dichiarato di CP 38.2 (*«una funzione pura sullo snapshot + piano»*) e perche' il
// lavoro che segue ne ha bisogno: il bot valida contro lo stato, e CP 38.3 legge il profilo di movimento
// dell'unita'. Il nome e' commentato via invece che lasciato morto, cosi' chi legge vede che l'assenza di
// letture e' voluta e non una dimenticanza.
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
	//
	// Gli slot portano CHI li ha presi, non un `bool`: chi comunica il rifiuto nomina entrambe le azioni in
	// conflitto, perche' l'ordine canonico di questa funzione non e' l'ordine con cui il resolver scarta.
	FName MovementHolder;
	FName MainHolder;
	FName ReactionHolder;
	for (const FRTPlannedAction& Entry : Ordered)
	{
		const bool bWantsMovement = Entry.Def.Slot == ERTActionSlot::Movement
			|| Entry.Def.Slot == ERTActionSlot::MovementAndMain;
		const bool bWantsMain = Entry.Def.Slot == ERTActionSlot::Main
			|| Entry.Def.Slot == ERTActionSlot::MovementAndMain;
		const bool bWantsReaction = Entry.Def.Slot == ERTActionSlot::Reaction;

		if (bWantsMovement && !MovementHolder.IsNone())
		{
			return Reject(ERTActionInvalidReason::SlotOccupied, Entry.Def.ActionId, MovementHolder);
		}
		if (bWantsMain && !MainHolder.IsNone())
		{
			return Reject(ERTActionInvalidReason::SlotOccupied, Entry.Def.ActionId, MainHolder);
		}
		if (bWantsReaction && !ReactionHolder.IsNone())
		{
			return Reject(ERTActionInvalidReason::SlotOccupied, Entry.Def.ActionId, ReactionHolder);
		}

		if (bWantsMovement) { MovementHolder = Entry.Def.ActionId; }
		if (bWantsMain)     { MainHolder = Entry.Def.ActionId; }
		if (bWantsReaction) { ReactionHolder = Entry.Def.ActionId; }
	}

	return FRTPlanValidation();
}
