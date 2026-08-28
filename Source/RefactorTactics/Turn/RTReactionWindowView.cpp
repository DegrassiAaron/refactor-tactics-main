#include "Turn/RTReactionWindowView.h"

FRTReactionWindowView URTReactionWindowLibrary::FilterWindowForTeam(int32 ObserverTeamId, int32 OwnerTeamId,
	const FRTReactionOpportunity& Opportunity, float WindowSeconds)
{
	FRTReactionWindowView View;

	// 🔴 Il ritorno anticipato E' la regola di privacy: chi non e' della squadra del proprietario riceve la
	// struttura ai default, cioe' niente. Popolarla e lasciare a un `bVisible` il compito di nasconderla
	// spedirebbe comunque il dato — e in rete (M10) sarebbe sul filo.
	if (ObserverTeamId != OwnerTeamId)
	{
		return View;
	}

	// Nessun boundary, niente finestra: la cardinalita' e' la sola regola che lo decide (ADR-0004 §2), e
	// resta dove gia' vive. Chiederla qui con un `AllowedResponses.Num() >= 2` scritto a mano sarebbe la
	// seconda regola che `RequiresDecisionBoundary` esiste per non far nascere.
	if (!URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity))
	{
		return View;
	}

	View.bOpen = true;
	View.Key = Opportunity.Key;

	// Un tempo negativo non e' un tempo, ed e' la stessa scelta di `SetPlanningSeconds`: il chiamante
	// sbagliato produce «nessuna attesa», non un countdown che conta all'indietro dal nulla.
	View.WindowSeconds = FMath::Max(0.f, WindowSeconds);

	View.SafeResponse = URTReactionOpportunityLibrary::SafeResponse(Opportunity);

	for (const FString& Response : Opportunity.AllowedResponses)
	{
		// Il discriminante e' il FORMATO, non la posizione nell'elenco: `FireResponseTarget` e' l'inversa di
		// `FireResponse` e vive accanto a lei. Una risposta senza bersaglio — la scelta sicura, o una
		// maneuver del `Brace` — non e' un bottone di bersaglio e non entra qui.
		const int32 TargetUnitId = URTReactionOpportunityLibrary::FireResponseTarget(Response);
		if (TargetUnitId == INDEX_NONE)
		{
			continue;
		}

		FRTReactionWindowTargetView Target;
		Target.UnitId = TargetUnitId;
		Target.Response = Response;
		View.Targets.Add(Target);
	}

	return View;
}
