#include "Turn/RTReactionWindowView.h"

#include "Combat/RTCombatLibrary.h" // IsIntentVisibleTo: la visibilita' per squadra ha UN owner (#507)

FRTReactionWindowView URTReactionWindowLibrary::FilterWindowForTeam(int32 ObserverTeamId, int32 OwnerTeamId,
	const FRTReactionOpportunity& Opportunity, float WindowSeconds)
{
	FRTReactionWindowView View;

	// 🔴 Fail-closed sulle squadre non risolte, PRIMA della regola. `INDEX_NONE` e' il default dei team id in
	// tutto il progetto, e senza questa guardia due «non lo so» sarebbero risultati UGUALI: un osservatore
	// ignoto avrebbe ricevuto la finestra di un proprietario ignoto. Il filtro degli intenti non ha questo
	// buco perche' legge il team DAL DATO (`Intent.TeamId`); qui i due arrivano dal chiamante, quindi la
	// guardia va scritta.
	if (ObserverTeamId == INDEX_NONE || OwnerTeamId == INDEX_NONE)
	{
		return View;
	}

	// 🔴 La regola di visibilita' per squadra sta in UN posto solo (#507), e non si riscrive qui: un
	// `ObserverTeamId != OwnerTeamId` inline sarebbe identico oggi e separato dalla funzione che i test
	// coprono — e' il difetto che `RTIntentPrivacyLibrary` ha gia' rimosso da se'.
	//
	// `bOwnerRevealed = false` e' DICHIARATO e non un default subito: `Status.Reveal` mostra cosa un'unita'
	// sta per FARE, mai cosa e' pronta a PARARE. La DoD di CP 5.4 dice che la reazione non e' visibile ai
	// nemici, e «mai» include il caso rivelato — passare qui il flag di reveal aprirebbe la finestra a un
	// avversario esattamente nel turno in cui conta di piu'.
	if (!URTCombatLibrary::IsIntentVisibleTo(ObserverTeamId, OwnerTeamId, /*bOwnerRevealed=*/false))
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

	// Un tempo negativo non e' un tempo, ed e' la stessa scelta di `SetPlanningSeconds`. ⚠️ Qui il clamp
	// protegge la COPIA: la fonte si difende da sola con il `ClampMin` sul campo del `TurnManager`, perche'
	// un clamp che vive solo nel consumatore lascia passare il valore a ogni altro lettore futuro.
	View.WindowSeconds = FMath::Max(0.f, WindowSeconds);

	View.SafeResponse = URTReactionOpportunityLibrary::SafeResponse(Opportunity);

	View.Options.Reserve(Opportunity.AllowedResponses.Num());
	for (const FString& Response : Opportunity.AllowedResponses)
	{
		FRTReactionWindowOptionView Option;
		Option.Response = Response;

		// Il bersaglio si deriva dal FORMATO e non dalla posizione nell'elenco: `FireResponseTarget` e'
		// l'inversa di `FireResponse` e vive accanto a lei. Chi non e' un `FIRE:` resta senza bersaglio —
		// `HOLD`, `Hold Ground`, una maneuver del `Brace` — e **entra lo stesso**, perche' una risposta che
		// non si puo' scegliere e' una finestra che non si puo' chiudere.
		Option.TargetSnapshotIndex = URTReactionOpportunityLibrary::FireResponseTarget(Response);

		View.Options.Add(MoveTemp(Option));
	}

	return View;
}
