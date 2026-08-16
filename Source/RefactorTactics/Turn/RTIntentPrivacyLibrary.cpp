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

			// 🔴 Qui c'era un `if (!Intent.ReactionName.IsEmpty()) View.ReactionCertainty = Uncertain;`, e la
			// sua esistenza era il difetto: faceva sembrare CALCOLATO un livello che e' costante, e lasciava
			// l'altro ramo — nessuna reazione — al default `Confirmed`. Cioe' lo stesso valore diceva «non
			// c'e' nessuna reazione» e «la reazione e' certa», e la seconda lettura e' quella che un widget
			// fa senza pensarci.
			// Il default e' ora `Uncertain` (vedi `FRTIntentView::ReactionCertainty`), quindi il ramo sarebbe
			// un no-op che assegna cio' che c'e' gia': toglierlo lascia UNA sola affermazione nel codice —
			// una reazione armata e' incerta, sempre — invece di due che si contraddicono ai bordi.
			// **Se una reazione potra' essere `Predicted`, il calcolo torna qui**, e allora sara' un calcolo.
		}

		Views.Add(View);
	}

	return Views;
}
