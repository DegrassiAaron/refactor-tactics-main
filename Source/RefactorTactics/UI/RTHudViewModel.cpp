#include "UI/RTHudViewModel.h"

#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchFormatData.h"
#include "Unit/RTUnit.h"

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

	// Il timer risponde solo dentro il Planning. Fuori resta negativo — «non si applica» — invece di 0,
	// che un widget leggerebbe come «scaduto adesso».
	if (View.Phase == ERTMatchPhase::Planning && !View.bResolving)
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
