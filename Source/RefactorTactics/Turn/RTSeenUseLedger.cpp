#include "Turn/RTSeenUseLedger.h"

#include "Turn/RTTurnLogLibrary.h"

bool FRTSeenUseLedger::IsUseByItsSubject(const FRTTurnLogEntry& Entry)
{
	// Senza soggetto, azione o turno la voce non dice chi ha fatto che cosa e quando: non si inventa.
	if (Entry.UnitId == 0 || Entry.ActionId.IsNone() || Entry.TurnNumber <= 0)
	{
		return false;
	}

	// Chi subisce non ha agito: e' il caso che P4 chiede di escludere per nome. Vale anche per il danno ambientale e
	// per l'azione cancellata da un `Interrupt`, che portano in `UnitId` chi li ha SUBITI.
	if (URTTurnLogLibrary::IsSubjectTheSufferer(Entry))
	{
		return false;
	}

	if (URTTurnLogLibrary::IsDamageInflictedByActor(Entry))
	{
		return true;
	}

	// La cura non e' danno, ma ha un agente: `ApplyPlannedHeals` la scrive a nome di chi cura.
	return Entry.Category == ERTLogCategory::Combat
		&& Entry.Outcome == static_cast<uint8>(ERTCombatOutcome::Healed);
}

void FRTSeenUseLedger::Observe(const FRTTurnLogEntry& Entry, TConstArrayView<int32> ObserverTeamIds)
{
	if (!IsUseByItsSubject(Entry))
	{
		return;
	}

	for (const int32 TeamId : ObserverTeamIds)
	{
		// Il verdetto e' stato congelato quando la voce e' nata, contro l'unita' che l'ha scritta (`AppendLogEntry`):
		// una squadra che non vedeva quell'unita' in quell'istante non legge la voce, e non ne deduce niente.
		if (!Entry.Verdict.AllowsTeam(TeamId))
		{
			continue;
		}

		FRow* Existing = Rows.FindByPredicate([&](const FRow& Row)
		{
			return Row.TeamId == TeamId && Row.UnitId == Entry.UnitId && Row.ActionId == Entry.ActionId;
		});
		if (Existing != nullptr)
		{
			// Il massimo e non l'ultima scritta: una voce di un turno precedente non fa tornare indietro la stima.
			Existing->Turn = FMath::Max(Existing->Turn, Entry.TurnNumber);
			continue;
		}

		FRow& Added = Rows.AddDefaulted_GetRef();
		Added.TeamId = TeamId;
		Added.UnitId = Entry.UnitId;
		Added.ActionId = Entry.ActionId;
		Added.Turn = Entry.TurnNumber;
	}
}

int32 FRTSeenUseLedger::LastSeenTurn(int32 ObserverTeamId, int32 UnitId, FName ActionId) const
{
	const FRow* Found = Rows.FindByPredicate([&](const FRow& Row)
	{
		return Row.TeamId == ObserverTeamId && Row.UnitId == UnitId && Row.ActionId == ActionId;
	});
	return Found != nullptr ? Found->Turn : 0;
}
