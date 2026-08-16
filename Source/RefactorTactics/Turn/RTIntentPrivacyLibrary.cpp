#include "Turn/RTIntentPrivacyLibrary.h"

#include "Combat/RTCombatLibrary.h"

ERTIntentCertainty URTIntentPrivacyLibrary::ClassifyPlan(const FRTPlannedIntent& Intent)
{
	// L'ordine dei rami e' la regola, non una comodita': muoversi VINCE su avere un bersaglio. Un'unita' che
	// spara mentre corre non e' piu' certa di una che corre e basta — le celle che attraversa restano
	// contendibili, e il resolver puo' troncarle la rotta prima che arrivi a tiro.
	//
	// ⚠️ `bMoving` e `bDashing` sono flag DISTINTI: guardare solo il primo lascerebbe uno scatto
	// «confermato», che e' il caso in cui la rotta e' piu' lunga e piu' facile da interrompere.
	if (Intent.bMoving || Intent.bDashing)
	{
		return ERTIntentCertainty::Uncertain;
	}

	// Ferma, ma punta qualcosa: vale nello snapshot corrente. Non promette l'esito — promette che adesso il
	// collegamento e' valido.
	if (Intent.bHasTarget)
	{
		return ERTIntentCertainty::Predicted;
	}

	return ERTIntentCertainty::Confirmed;
}

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

		// La regola di visibilita' sta in UN posto solo (#507). Prima era riscritta qui inline — identica, ma
		// separata dalla funzione che il test copre: `IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed` era
		// verde su una `IsIntentVisibleTo` che nessuno chiamava, quindi non avrebbe visto una regressione
		// proprio di questa riga, che e' il percorso vero (la HUD passa di qui).
		if (!URTCombatLibrary::IsIntentVisibleTo(ObserverTeamId, Intent.TeamId, Intent.bRevealed))
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
		View.DashStyle = Intent.DashStyle;
		View.Facing = Intent.Facing; // posa attuale: si vede guardando l'unita', non e' un piano

		// Il livello di certezza (CP 11.2) si calcola da QUESTO intento e da nient'altro: `Intents` contiene
		// anche i piani nemici, e derivarne il livello sarebbe un canale laterale — vedi `ERTIntentCertainty`.
		View.Certainty = ClassifyPlan(Intent);

		// Reazione, waypoint e rotazione DICHIARATA si copiano SOLO per un alleato. Non c'e' un ramo che li copia
		// e poi li cancella: per chiunque altro restano semplicemente non valorizzati, rivelato o meno.
		if (bIsAlly)
		{
			View.ReactionName = Intent.ReactionName;
			View.PlannedWaypoints = Intent.PlannedWaypoints;
			View.bDeclaresRotation = Intent.bDeclaresRotation;
			View.DeclaredFacing = Intent.DeclaredFacing;

			// Il livello della reazione non si copia: `ReactionCertainty` e' uscito dal DTO il 2026-08-16 —
			// la ragione sta sulla struttura, dove la cerca chi consuma.
		}

		Views.Add(View);
	}

	return Views;
}
