#include "UI/RTHudViewModel.h"

#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchFormatData.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // l'autorita' su quale slot consuma un'azione
#include "Core/RTGameplayTags.h"      // TAG_Status_Reveal: cosa rende un piano visibile all'avversario
#include "Turn/RTIntentPrivacyLibrary.h"

FRTMatchHeaderView URTHudViewModel::BuildMatchHeader(const ARTTurnManager* TurnManager)
{
	FRTMatchHeaderView View;
	if (!TurnManager)
	{
		// Vista neutra, non «zero»: un widget costruito prima del manager deve poter mostrare un trattino.
		return View;
	}

	View.Round = TurnManager->GetTurnNumber();
	// Il limite viene dal FORMATO, mai da una costante: e' la voce del DoD di CP 11.1 che esiste per
	// impedire a un widget di stampare `12`. Qui non c'e' proprio un posto dove scriverlo a mano.
	View.RoundLimit = TurnManager->GetMatchRules().RoundLimit;
	View.Phase = TurnManager->GetPhase();
	View.bResolving = TurnManager->IsResolving();

	// Il timer risponde solo dentro il Planning, e **solo se un timer esiste davvero**. Fuori resta negativo
	// — «non si applica» — invece di 0, che un widget leggerebbe come «scaduto adesso».
	//
	// ⚠️ La seconda guardia non e' difensiva, chiude un caso REALE. `GetPlanningTimeRemaining()` restituisce
	// `0.f` in due situazioni diverse — timer scaduto e timer **mai impostato** — perche' clampa a zero il
	// risultato di `GetTimerRemaining` su un handle che puo' non esistere; e il timer si imposta solo
	// `if (PlanningSeconds > 0.f)`. Un Planning senza orologio non e' un caso di laboratorio:
	// `RTScenarioSession` chiama `SetPlanningSeconds(0.f)` per le run headless. Senza questa riga un
	// `WBP_RT_TurnHeader` mostrerebbe `0:00` lampeggiante per tutta una partita che di proposito non ha
	// tempo — cioe' il contratto dichiarato in `FRTMatchHeaderView` sarebbe falso proprio dove serve.
	if (View.Phase == ERTMatchPhase::Planning && !View.bResolving && TurnManager->GetPlanningSeconds() > 0.f)
	{
		View.PlanningSecondsRemaining = TurnManager->GetPlanningTimeRemaining();
	}

	return View;
}

FRTUnitCardView URTHudViewModel::BuildUnitCard(const ARTUnit* Unit, int32 PlayerTeamId)
{
	FRTUnitCardView Card;
	if (!Unit)
	{
		return Card;
	}

	Card.HeroId = Unit->HeroId;
	Card.Health = Unit->Health;
	Card.MaxHealth = Unit->MaxHealth;
	Card.Shield = Unit->Shield;
	Card.Energy = Unit->Energy;
	Card.MaxEnergy = Unit->MaxEnergy;
	Card.bIsAlly = (Unit->TeamId == PlayerTeamId);
	Card.bAlive = Unit->IsAlive();

	return Card;
}

namespace
{
	/** Riempie uno slot con l'azione che lo occupa, prendendone identita' e nome dal kit. */
	void FillSlotFromAbility(FRTPlannedSlotView& Slot, const ARTUnit& Unit, int32 AbilityIndex)
	{
		const URTActionData* Action = Unit.GetAbility(AbilityIndex);
		if (!Action)
		{
			return; // indice non valido: lo slot resta libero invece di risultare occupato da nulla
		}
		Slot.bOccupied = true;
		Slot.ActionId = Action->Def.ActionId;
		Slot.DisplayName = Action->DisplayName;
	}
}

FRTUnitSlotsView URTHudViewModel::BuildUnitSlots(const ARTUnit* Unit)
{
	FRTUnitSlotsView Slots;
	if (!Unit)
	{
		return Slots;
	}

	// --- movimento ---------------------------------------------------------------------------------------
	// Due modi di occuparlo, e uno solo dei due ha un nome. Un percorso e' `PlannedWaypoints`: occupa lo slot
	// e non e' un'azione scelta, quindi resta senza `ActionId` (vedi il commento su `FRTPlannedSlotView`).
	if (Unit->PlannedWaypoints.Num() > 0)
	{
		Slots.Movement.bOccupied = true;
	}
	if (Unit->PlannedDashAbility != INDEX_NONE)
	{
		// Lo scatto NOMINA lo slot anche se un percorso lo aveva gia' segnato occupato: fra i due e' l'unico
		// che il giocatore ha scelto per nome, ed e' l'informazione utile. Che i due possano coesistere non lo
		// decide la HUD — se e' un piano illegale, lo dice `ValidateActionSlots`.
		FillSlotFromAbility(Slots.Movement, *Unit, Unit->PlannedDashAbility);
	}

	// --- azione principale -------------------------------------------------------------------------------
	if (Unit->PlannedAbilityIndex != INDEX_NONE)
	{
		FillSlotFromAbility(Slots.Main, *Unit, Unit->PlannedAbilityIndex);

		// ⚠️ `Action.Sprint` dichiara `MovementAndMain`: un'azione sola, DUE slot. Senza questa riga il
		// pannello mostrerebbe il movimento libero a chi ha appena speso tutto il turno per correre.
		// Chi risponde e' il catalogo, non un `==` scritto qui: e' lo stesso predicato del validatore.
		if (const URTActionData* Main = Unit->GetAbility(Unit->PlannedAbilityIndex))
		{
			if (URTCatalogLibrary::TakesMovementSlot(Main->Def))
			{
				FillSlotFromAbility(Slots.Movement, *Unit, Unit->PlannedAbilityIndex);
			}
		}
	}

	// --- reazione ----------------------------------------------------------------------------------------
	// Slot indipendente (CP 5.1): un eroe puo' muoversi, agire E tenere una reazione pronta nello stesso turno.
	if (Unit->PlannedReactionAbility != INDEX_NONE)
	{
		FillSlotFromAbility(Slots.Reaction, *Unit, Unit->PlannedReactionAbility);
	}

	return Slots;
}

TArray<FRTAbilityCooldownView> URTHudViewModel::BuildAbilityCooldowns(const ARTUnit* Unit)
{
	TArray<FRTAbilityCooldownView> Cooldowns;
	if (!Unit)
	{
		return Cooldowns;
	}

	for (int32 Index = 0; Index < Unit->NumAbilities(); ++Index)
	{
		const URTActionData* Action = Unit->GetAbility(Index);
		if (!Action)
		{
			continue;
		}

		FRTAbilityCooldownView View;
		View.ActionId = Action->Def.ActionId;
		View.DisplayName = Action->DisplayName;
		View.AbilityIndex = Index;
		View.Slot = Action->Def.Slot;

		// Il numero si LEGGE dal simulatore. `FMath::Max(0, ...)` non e' difensivo per abitudine: la vista
		// dichiara «mai negativo» nel proprio contratto, e un contratto che dipende dal fatto che nessuno
		// scriva mai un valore negativo altrove non e' un contratto.
		View.TurnsRemaining = FMath::Max(0, Unit->GetAbilityCooldown(Index));

		// Usabile ADESSO e' un'altra domanda: `CanUseAbility` guarda ricarica **ed** energia.
		View.bUsableNow = Unit->CanUseAbility(Index);

		Cooldowns.Add(View);
	}

	return Cooldowns;
}

TArray<FRTUnitCardView> URTHudViewModel::BuildTeamRoster(const TArray<ARTUnit*>& Units, int32 PlayerTeamId)
{
	TArray<FRTUnitCardView> Roster;
	for (const ARTUnit* Unit : Units)
	{
		// Le morte restano: `bAlive` lo dice, e un elenco che si accorcia fa perdere il conto della squadra.
		if (Unit && Unit->TeamId == PlayerTeamId)
		{
			Roster.Add(BuildUnitCard(Unit, PlayerTeamId));
		}
	}
	return Roster;
}

TArray<FRTPlannedIntent> URTHudViewModel::BuildAuthoritativeIntents(const TArray<AActor*>& Actors)
{
	TArray<FRTPlannedIntent> Authoritative;
	Authoritative.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		const ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive()) { continue; }

		const URTActionData* Planned = Unit->GetAbility(Unit->PlannedAbilityIndex);
		const URTActionData* Reaction = Unit->GetAbility(Unit->PlannedReactionAbility);

		FRTPlannedIntent Intent;
		Intent.OwnerCell = Unit->Cell;
		Intent.TeamId = Unit->TeamId;
		Intent.bAlive = true;
		Intent.bRevealed = Unit->HasStatus(TAG_Status_Reveal);
		Intent.bMoving = (Unit->PlannedCell != Unit->Cell);
		Intent.PlannedCell = Unit->PlannedCell;
		Intent.ActionName = Planned ? Planned->DisplayName : FText::GetEmpty();
		Intent.bHasTarget = (Unit->PlannedAttackTarget != nullptr && Unit->PlannedAttackTarget->IsAlive());
		Intent.TargetCell = Intent.bHasTarget ? Unit->PlannedAttackTarget->Cell : Unit->Cell;
		Intent.ReactionName = Reaction ? Reaction->DisplayName : FText::GetEmpty();
		Intent.PlannedPath = Unit->PlannedPath;
		Intent.PlannedWaypoints = Unit->PlannedWaypoints;
		Intent.bDashing = (Unit->PlannedDashAbility != INDEX_NONE);
		Intent.DashCell = Unit->PlannedDashCell;
		// Rotazione dichiarata (D-020, #291): finora `bDeclaresRotation` e `DeclaredFacing` restavano ai
		// default perche' nessuno li valorizzava, e `FilterForTeam` filtrava un campo sempre vuoto.
		Intent.bDeclaresRotation = Unit->bDeclaresPlannedFacing;
		Intent.DeclaredFacing = Unit->PlannedFacing;
		if (const URTActionData* DashAb = Unit->GetAbility(Unit->PlannedDashAbility))
		{
			Intent.DashStyle = DashAb->Def.MovementStyle;
		}
		Authoritative.Add(Intent);
	}
	return Authoritative;
}
