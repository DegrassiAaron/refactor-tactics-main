#include "Turn/Actions/RTPlanValidationLibrary.h"
#include "Ability/RTMovementProfileLibrary.h"

#include "Ability/RTActionData.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Core/RTGameplayTags.h" // Unbalanced: [D-319] nega la CORSA, che dal 2026-09-16 e' una distanza
#include "Map/RTHexLibrary.h"    // HexDistance: i passi di chi dichiara la sola destinazione (il bot)
#include "Unit/RTUnit.h"

namespace
{
	/** Rifiuto con motivo, colpevole e detentore dello slot: un solo punto di costruzione, cosi' nessun
	 *  ramo dimentica un campo. `Holder` resta `NAME_None` per i motivi che non sono conflitti di slot. */
	FRTPlanValidation Reject(ERTActionInvalidReason Reason, const FRTPlannedAction& Offending,
		const FRTPlannedAction* Holder = nullptr)
	{
		FRTPlanValidation Result;
		Result.bLegal = false;
		Result.Reason = Reason;
		Result.OffendingActionId = Offending.Def.ActionId;
		Result.OffendingDeclarationIndex = Offending.DeclarationIndex;
		if (Holder)
		{
			Result.HolderActionId = Holder->Def.ActionId;
			Result.HolderDeclarationIndex = Holder->DeclarationIndex;
		}
		return Result;
	}

	/** L'ordine di dichiarazione come chiave: una voce non dichiarata (`INDEX_NONE`) viene dopo tutte le dichiarate. */
	int32 DeclarationKey(const FRTPlannedAction& Entry)
	{
		return Entry.DeclarationIndex == INDEX_NONE ? MAX_int32 : Entry.DeclarationIndex;
	}
}

int32 URTPlanValidationLibrary::ActionPointsSpent(const TArray<FRTPlannedAction>& Plan)
{
	int32 Spent = 0;
	for (const FRTPlannedAction& Entry : Plan)
	{
		Spent += FMath::Max(0, Entry.Def.ActionPointCost);
	}
	return Spent;
}

bool URTPlanValidationLibrary::EmptiesPlanAtLockIn(ERTActionInvalidReason Reason)
{
	return Reason == ERTActionInvalidReason::OnCooldown
		|| Reason == ERTActionInvalidReason::RepeatedWithCooldown
		|| Reason == ERTActionInvalidReason::OverBudget
		|| Reason == ERTActionInvalidReason::SecondDash; // [D-625] (4): come la copia vietata (lettura (c))
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
	// E38 CP 38.12: ogni voce porta il suo ordine di dichiarazione, la posizione della sua voce nella lista del piano.
	auto AddAbility = [&Plan, Unit](int32 Index, int32 DeclarationIndex)
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
		Entry.DeclarationIndex = DeclarationIndex;
		// [D-615] (6) (E38 CP 38.13): il cooldown della riga, non quello che la variante d'arma ha riscritto nel `Def`.
		Entry.CatalogCooldownTurns = Ability->CooldownTurnsBeforeVariant != INDEX_NONE
			? Ability->CooldownTurnsBeforeVariant
			: Ability->Def.CooldownTurns;
		Plan.Add(Entry);
	};

	// E38 CP 38.13: TUTTE le voci di ogni classe, non la prima. Le voci `Prep` sono le azioni di Prep in piu' della
	// principale; due voci di una classe chiusa le scrive solo l'accesso dei test, e il validatore le giudica lo stesso.
	// L'array si compone per classe (scatto, principale, Prep, reazione), e dentro la classe nell'ordine della lista.
	const FRTUnitPlan& Piano = Unit->GetPlan();
	for (const ERTPlanEntryClass Classe : { ERTPlanEntryClass::Dash, ERTPlanEntryClass::Main, ERTPlanEntryClass::Prep,
		ERTPlanEntryClass::Reaction })
	{
		for (int32 Posizione = 0; Posizione < Piano.GetEntries().Num(); ++Posizione)
		{
			const FRTPlanEntry& Voce = Piano.GetEntries()[Posizione];
			if (Voce.Class == Classe)
			{
				AddAbility(Voce.AbilityIndex, Posizione);
			}
		}
	}

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
		// 🔑 **Il profilo si DERIVA da quanto si e' pianificato** ([D-425]), e questo e' il punto unico in
		// cui la derivazione avviene. Tutto cio' che sta a valle — `ProfileForPlan`, i due budget dello
		// snapshot, la voce del TurnLog, la HUD — legge il risultato da qui invece di ricalcolarlo, che e'
		// la ragione per cui non ci sono due risposte alla domanda «con che andatura si muove».
		//
		// ⏱️ *Fino al 2026-09-15 il profilo era DICHIARATO*: il giocatore ciclava fra quattro etichette e
		// questa riga traduceva la sua scelta in un'azione. [D-425] ha rovesciato la derivazione — da
		// *profilo → azione → budget* a *distanza → banda* — dopo una sonda sul corpus golden che ha
		// mostrato che il selettore chiedeva al giocatore di dichiarare cio' che il suo percorso gia' diceva.
		//
		// ⚠️ **I passi si leggono dal PERCORSO quando c'e', e dalla distanza quando non c'e'.** Le due fonti
		// non sono equivalenti e nessuna copre l'altra: player e harness scrivono `PlannedPath`, il bot
		// dichiara la sola `PlannedCell` e il suo percorso nasce dentro la fase Move
		// (`RTTurnManager_Movement.cpp`). ⛔ Qui NON si ricostruisce il percorso: `MakePlanFor` e' il
		// compositore che la HUD chiama a ogni click, e un pathfinding per click sarebbe il costo che il
		// `static const` poche righe sopra esiste per evitare. La distanza esagonale e' un limite INFERIORE
		// ai passi reali — un percorso che aggira costa di piu', mai di meno — quindi puo' far leggere
		// `Move` dove il percorso reale dira' `Sprint`, e mai il contrario: sbaglia verso il prezzo piu'
		// alto per nessuno, non verso il piu' basso per qualcuno.
		//
		// ➕ **[D-629] (E38 CP 38.16): quando il piano porta il costo del percorso, la banda si legge da lui**, in Movement Point come il
		// tetto e i tratti di [D-628] (2): un percorso corto e caro e' uno Sprint. I passi restano la misura di chi non dichiara un costo
		// (il bot, che dichiara la sola cella d'arrivo), e la distanza quella di chi non ha un percorso: `ARTUnit::PlannedMoveMeasure`.
		const int32 PlannedSteps = Unit->PlannedMoveMeasure();

		// ⚠️ **`ReservedProfileForPlan(Plan)` legge il piano COSTRUITO FIN QUI**, cioe' le tre abilita'
		// aggiunte sopra e non la voce di movimento che stiamo per aggiungere. Non e' un ordine fragile: e'
		// l'unico che chiude il cerchio, perche' cio' che riserva lo slot movimento e' sempre un'ALTRA
		// azione del piano ([D-070]).
		// 🔑 **E le abilita' ci sono TUTTE, qualunque sia la loro posizione nella lista** (E38 CP 38.12, [D-615] (5)):
		// l'array si compone per classe, non per ordine di dichiarazione, quindi un Move dichiarato prima dell'Overwatch
		// riceve la stessa riserva `Withdraw`. Percorrere la lista in ordine di dichiarazione la perderebbe, e la
		// dichiarazione deciderebbe la risoluzione. Lo pinna `Plan.MovementBandIgnoresTheDeclaredSequence`.
		const FName Riservato = URTMovementProfileLibrary::ReservedProfileForPlan(Plan);
		const FRTMovementProfile Band = URTMovementProfileLibrary::ProfileForPlannedSteps(
			PlannedSteps,
			Unit->GetEffectiveMoveRange(),
			Unit->GetPlan().PlannedMovementProfileId(),
			Riservato,
			Unit->HasStatus(TAG_Status_Unbalanced));

		// L'azione che porta la banda, quando ce n'e' una: `Action.Sprint` e `Action.Withdraw` la nominano,
		// ed e' da loro che vengono i PREZZI della banda — `Status.Exposed` per due turni e nessuna reazione
		// ([D-116]). Chi si limita a camminare, e chi sguscia, porta `Action.Move`.
		//
		// ⛔ **`Sneak` non ha un'azione, e non deve averne una** ([D-425]): si dichiara, non si nomina. La
		// voce porta `Action.Move` con il profilo stampato sopra, ed e' il profilo — non l'azione — a dire
		// al resolver con che misura si muove.
		FRTActionDef Movement = URTMovementProfileLibrary::FindActionForProfile(Band.Id);
		if (Movement.ActionId.IsNone())
		{
			Movement = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move"));
		}
		// 🔑 **Il profilo viaggia come DATO della voce, non come identita' dell'azione.** E' cio' che rende
		// vera la clausola di [D-425] *«senza che nessuna azione la nomini»* senza toccare il formato: chi
		// legge il piano trova `MovementProfileId` dov'e' sempre stato, e il catalogo non cresce di una voce
		// per ogni andatura.
		Movement.MovementProfileId = Band.Id;

		if (!Movement.ActionId.IsNone())
		{
			FRTPlannedAction Entry;
			Entry.Def = Movement;
			// L'ordine della voce di movimento. Il bot dichiara la sola cella d'arrivo, che e' del piano e non di una
			// voce: senza la voce, il movimento e' l'ultimo dichiarato.
			const int32 Dichiarato = Piano.DeclarationIndexOf(ERTPlanEntryClass::Movement);
			Entry.DeclarationIndex = Dichiarato != INDEX_NONE ? Dichiarato : Piano.GetEntries().Num();
			Plan.Add(Entry);

			// E38 CP 38.16, [D-625] (1): oltre il tetto nudo il percorso si paga a tratti, e ogni tratto in piu' e' una voce `Action.Move`
			// di profilo `Move` (2 AP; la ripetizione che [D-614] ammette, perche' il Move non ha ricarica). Quanti, lo dice il costo del
			// percorso in Movement Point che il piano porta, letto quando si e' composto sulla mappa di chi pianifica ([D-628] (2) e la sua
			// lettura (b)): ⛔ qui non si rilegge, per la stessa ragione per cui qui non si ricostruisce il percorso.
			// ⚠️ La stessa dichiarazione della voce di movimento: e' lo stesso percorso, e l'ordine dei tratti e' la catena delle celle,
			// mai la dichiarazione ([D-615] (5)).
			const int32 TrattiInPiu = URTMovementProfileLibrary::ExtraMoveLegsForCost(Unit->GetPlan().PlannedPathCost(),
				Unit->GetEffectiveMoveRange(), Unit->GetPlan().PlannedMovementProfileId(), Riservato,
				Unit->HasStatus(TAG_Status_Unbalanced));
			if (TrattiInPiu > 0)
			{
				static const FRTActionDef Tratto = []()
				{
					FRTActionDef Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move"));
					Def.MovementProfileId = URTMovementProfileLibrary::ProfileMove;
					return Def;
				}();
				for (int32 k = 0; k < TrattiInPiu; ++k)
				{
					FRTPlannedAction Voce;
					Voce.Def = Tratto;
					Voce.DeclarationIndex = Entry.DeclarationIndex;
					Plan.Add(Voce);
				}
			}
		}
	}

	return Plan;
}

// `Unit` e' la firma di CP 38.2 (*«una funzione pura sullo snapshot + piano»*), e da E38 CP 38.11 il corpo la legge:
// `ActionPointBudget`, il budget dell'unita' per il turno ([D-613] (1)). E' l'unica lettura. Con [D-190] il validatore
// aveva smesso di leggere `MoveBudget`, uscito insieme al ramo dei Movement Point.
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
		if (A.Def.ActionId != B.Def.ActionId)
		{
			return A.Def.ActionId.Compare(B.Def.ActionId) < 0;
		}
		// E38 CP 38.12: l'ordine di dichiarazione in coda rende l'ordine totale anche su due copie della stessa
		// azione. Decide solo quale occorrenza si nomina, mai legalita' o motivo.
		return DeclarationKey(A) < DeclarationKey(B);
	});

	// 1. Intrinseco: un'abilita' in cooldown non e' pianificabile nemmeno con tutti gli slot liberi.
	for (const FRTPlannedAction& Entry : Ordered)
	{
		if (Entry.CooldownRemaining > 0)
		{
			return Reject(ERTActionInvalidReason::OnCooldown, Entry);
		}
	}

	// Le voci nell'ORDINE DI DICHIARAZIONE, per le due accuse che lo leggono (copia e budget). A parita' di dichiarazione
	// (le voci non dichiarate dei test) decide l'ordine canonico: lo stesso piano in due sequenze dell'array accusa la
	// stessa voce.
	TArray<FRTPlannedAction> Dichiarate = Ordered;
	Dichiarate.StableSort([](const FRTPlannedAction& A, const FRTPlannedAction& B)
	{
		return DeclarationKey(A) < DeclarationKey(B);
	});

	// 2. La regola della copia ([D-614]; E38 CP 38.13): due voci con lo stesso `ActionId` sono vietate quando la riga di
	// catalogo ha un cooldown, in qualunque fase ([D-615] (1)), e il cooldown e' quello della riga, prima della variante
	// d'arma ([D-615] (6)). Si accusa la prima voce che ripete l'azione di una dichiarata prima: la legalita' e' la
	// coppia, che nessun ordine cambia, quindi l'accusa e' resa, come per il budget. Precede il budget perche' la copia
	// vietata non deve pesare nella somma: l'avviso dice la causa vera.
	for (int32 Voce = 1; Voce < Dichiarate.Num(); ++Voce)
	{
		for (int32 Prima = 0; Prima < Voce; ++Prima)
		{
			const FRTPlannedAction& A = Dichiarate[Prima];
			const FRTPlannedAction& B = Dichiarate[Voce];
			if (A.Def.ActionId == B.Def.ActionId && (A.CatalogCooldownTurns > 0 || B.CatalogCooldownTurns > 0))
			{
				return Reject(ERTActionInvalidReason::RepeatedWithCooldown, B);
			}
		}
	}

	// 3. [D-594] (E38 CP 38.6): la somma dei costi AP non supera il budget. Precede lo slot perche' e' il vincolo che
	// ADR-0011 lascera' in piedi quando gli slot cadranno.
	// E38 CP 38.12: si accusa la prima voce, nell'ORDINE DI DICHIARAZIONE, la cui somma corrente supera il budget; le
	// voci dichiarate prima di lei ci stanno tutte. La legalita' e' la somma intera, che nessun ordine cambia, quindi
	// l'accusa e' resa.
	int32 Spent = 0;
	for (const FRTPlannedAction& Entry : Dichiarate)
	{
		Spent += FMath::Max(0, Entry.Def.ActionPointCost);
		if (Spent > Unit.ActionPointBudget) // [D-613] (1): il budget dell'unita' per il turno (E38 CP 38.11)
		{
			return Reject(ERTActionInvalidReason::OverBudget, Entry);
		}
	}

	// 4. [D-625] (4) (E38 CP 38.16): uno scatto per piano. Gli slot non decidono piu' la legalita' ([D-584] (1)), e `SlotOccupied` non ha
	// piu' produttori: scatto + Move, due principali, una mobilita' che costa tutto il turno passano o cadono sul budget qui sopra. Resta
	// la sola regola di classe che D-625 tiene oltre al budget: due scatti DIVERSI (la copia dello stesso scatto e' gia' caduta al passo 2).
	// Si accusa il secondo nell'ordine di dichiarazione, col primo come detentore, come per la copia: la legalita' e' la coppia.
	const FRTPlannedAction* PrimoScatto = nullptr;
	for (const FRTPlannedAction& Entry : Dichiarate)
	{
		if (!URTCatalogLibrary::IsFastMovement(Entry.Def))
		{
			continue;
		}
		if (PrimoScatto)
		{
			return Reject(ERTActionInvalidReason::SecondDash, Entry, PrimoScatto);
		}
		PrimoScatto = &Entry;
	}

	return FRTPlanValidation();
}
