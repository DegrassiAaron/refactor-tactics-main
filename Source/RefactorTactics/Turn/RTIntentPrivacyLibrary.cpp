#include "Turn/RTIntentPrivacyLibrary.h"

TArray<FRTIntentView> URTIntentPrivacyLibrary::FilterForTeam(int32 ObserverTeamId,
	const TArray<FRTPlannedIntent>& Intents)
{
	TArray<FRTIntentView> Views;
	Views.Reserve(Intents.Num());

	for (const FRTPlannedIntent& Intent : Intents)
	{
		if (!Intent.bAlive)
		{
			continue;
		}

		const bool bIsAlly = (Intent.TeamId == ObserverTeamId);
		if (!bIsAlly && !Intent.bRevealed)
		{
			continue; // avversario non rivelato: nessuna voce, non una voce vuota
		}

		FRTIntentView View;
		View.OwnerCell = Intent.OwnerCell;
		View.bIsAlly = bIsAlly;
		View.bMoving = Intent.bMoving;
		View.PlannedCell = Intent.PlannedCell;
		View.ActionName = Intent.ActionName;
		View.bHasTarget = Intent.bHasTarget;
		View.TargetCell = Intent.TargetCell;
		View.PlannedPath = Intent.PlannedPath; // movimento: `Reveal` lo espone
		View.bDashing = Intent.bDashing;
		View.DashCell = Intent.DashCell;

		// Reazione e waypoint si copiano SOLO per un alleato. Non c'e' un ramo che li copia e poi li cancella:
		// per chiunque altro restano semplicemente non valorizzati, rivelato o meno.
		if (bIsAlly)
		{
			View.ReactionName = Intent.ReactionName;
			View.PlannedWaypoints = Intent.PlannedWaypoints;
		}

		Views.Add(View);
	}

	return Views;
}
