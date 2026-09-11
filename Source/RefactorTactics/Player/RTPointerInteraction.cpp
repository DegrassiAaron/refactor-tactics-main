#include "Player/RTPointerInteraction.h"
#include "Unit/RTUnit.h"

ERTPointerTargetKind URTPointerLibrary::TargetKindForAction(const FRTActionDef& Def, bool bSelfTarget,
	ERTAbilityShape Shape)
{
	// Un supporto su se stessi non ha niente da puntare: si pianifica alla pressione del tasto.
	if (bSelfTarget)
	{
		return ERTPointerTargetKind::None;
	}

	// Il BORDO prima della forma, e l'ordine conta: `Action.CreateCover` dichiara `Shape::Single` e agisce
	// comunque su un bordo. Chiedere prima la forma classificherebbe come `Unit` un'azione che il resolver
	// rifiuta con `CoverRejected` se il piano non dichiara il lato.
	if (Def.StructureOp != ERTStructureOp::None)
	{
		return ERTPointerTargetKind::Edge;
	}

	// Un'AREA si centra su una CELLA, che puo' essere vuota. E' la riga che rende `PlannedAttackCell`
	// raggiungibile dal giocatore invece che dal solo harness.
	if (Shape == ERTAbilityShape::Area)
	{
		return ERTPointerTargetKind::Cell;
	}

	return ERTPointerTargetKind::Unit;
}

FRTPointerTarget URTPointerLibrary::ResolveTarget(ERTPointerContext Context, ERTPointerTargetKind Kind,
	const FRTPointerCandidates& C)
{
	FRTPointerTarget T;

	// `C.bGhost` non compare in nessun ramo, e **questa assenza e' la regola**: un ghost e' un fuoco della UI,
	// mai un bersaglio di gioco. Non serve un ramo che lo scarti — serve non guardarlo mai, e dove copre
	// qualcosa vince cio' che sta sotto. Il campo esiste perche' l'hover lo usa per il focus di fase.

	switch (Context)
	{
	case ERTPointerContext::IdleSelection:
	case ERTPointerContext::Planning:
		// unita' > elemento logico > cella. Nello stato neutro un'unita' si ISPEZIONA o si seleziona: chi
		// decide quale dei due e' il controller, che sa di che squadra e'. Qui si dice solo *su cosa*.
		if (C.Unit)
		{
			T.Kind = ERTPointerTargetKind::Unit;
			T.Unit = C.Unit;
			if (C.bHasCell) { T.Cell = C.Cell; }
			return T;
		}
		if (C.bMapElement)
		{
			T.Kind = ERTPointerTargetKind::Object;
			if (C.bHasCell) { T.Cell = C.Cell; }
			return T;
		}
		if (C.bHasCell)
		{
			T.Kind = ERTPointerTargetKind::Cell;
			T.Cell = C.Cell;
		}
		return T;

	case ERTPointerContext::Pathing:
		// §4.1 conseguenza 1 — **una porta non deve impedire di indicare la cella oltre la porta.**
		// `bMapElement` non compare: in `Pathing` la mesh di porta, ponte, hazard e objective e' annotazione.
		// Se quel percorso non si puo' fare lo rifiuta la topologia, con un reason; non un collider davanti.
		if (C.Unit)
		{
			T.Kind = ERTPointerTargetKind::Unit;
			T.Unit = C.Unit;
			if (C.bHasCell) { T.Cell = C.Cell; }
			return T;
		}
		if (C.bHasCell)
		{
			T.Kind = ERTPointerTargetKind::Cell;
			T.Cell = C.Cell;
		}
		return T;

	case ERTPointerContext::Targeting:
		switch (Kind)
		{
		case ERTPointerTargetKind::Unit:
			// La cella NON e' un ripiego: un'azione che vuole un'unita' non bersaglia il terreno perche' il
			// giocatore ha mancato. Meglio nessun target e un reason che un bersaglio inventato.
			if (C.Unit)
			{
				T.Kind = ERTPointerTargetKind::Unit;
				T.Unit = C.Unit;
				if (C.bHasCell) { T.Cell = C.Cell; }
			}
			return T;

		case ERTPointerTargetKind::Cell:
			// §4.1 conseguenza 2 — **un'unita' sopra una cella non deve impedire di bersagliare quella cella.**
			// `C.Unit` non compare, ed e' il punto: e' il caso che rende un'area utilizzabile, perche' la si
			// centra su chi la occupa. Il chiamante deve riempire `Cell` anche quando il raggio ha colpito
			// un'unita': la cella si ricava dal punto colpito, non dall'actor.
			if (C.bHasCell)
			{
				T.Kind = ERTPointerTargetKind::Cell;
				T.Cell = C.Cell;
			}
			return T;

		case ERTPointerTargetKind::Edge:
			// Un bordo senza la sua cella non identifica niente: sono i due dati insieme a fare un lato.
			if (C.bHasEdge && C.bHasCell)
			{
				T.Kind = ERTPointerTargetKind::Edge;
				T.Cell = C.Cell;
				T.Edge = C.Edge;
			}
			return T;

		case ERTPointerTargetKind::Object:
			if (C.bMapElement)
			{
				T.Kind = ERTPointerTargetKind::Object;
				if (C.bHasCell) { T.Cell = C.Cell; }
			}
			return T;

		default:
			return T;
		}

	case ERTPointerContext::Facing:
		// Il settore di rotazione vince su tutto il resto: unita', ghost e annotazioni sono trasparenti.
		if (C.bHasEdge)
		{
			T.Kind = ERTPointerTargetKind::Edge;
			T.Edge = C.Edge;
			if (C.bHasCell) { T.Cell = C.Cell; }
		}
		return T;

	case ERTPointerContext::ResolutionPlayback:
	case ERTPointerContext::ReactionWindow:
	case ERTPointerContext::Modal:
		// Il mondo e' sola lettura. Durante una `ReactionWindow` le opzioni sono quelle che l'opportunity
		// dichiara — gia' sanificate (ADR-0004) — e non passano da qui.
		return T;

	default:
		return T;
	}
}

ERTPointerBackStep URTPointerLibrary::ResolveBack(ERTPointerContext Context, bool bInspectorPinned,
	int32 WaypointCount, bool bPhaseFocusPinned)
{
	// L'ordine e' TOTALE, e l'elenco e' quello di §5.5. Scritto come cascata di `return` e non come `switch`
	// sul contesto perche' la priorita' attraversa i contesti: un inspector pinnato si chiude prima di uscire
	// da un targeting, qualunque sia il contesto.

	if (Context == ERTPointerContext::ReactionWindow)
	{
		return ERTPointerBackStep::ReactionFallback;
	}
	if (Context == ERTPointerContext::Modal)
	{
		return ERTPointerBackStep::Modal;
	}
	if (bInspectorPinned)
	{
		return ERTPointerBackStep::Inspector;
	}
	if (Context == ERTPointerContext::Targeting || Context == ERTPointerContext::Facing)
	{
		return ERTPointerBackStep::Declaration;
	}
	if (Context == ERTPointerContext::Pathing)
	{
		return WaypointCount > 0 ? ERTPointerBackStep::Waypoint : ERTPointerBackStep::Pathing;
	}
	if (bPhaseFocusPinned)
	{
		return ERTPointerBackStep::PhaseFocus;
	}

	// ⚠️ Durante `ResolutionPlayback` si arriva qui, ed e' giusto: §5.3 dice `NoOp`. Nessun input cambia un
	// piano gia' consegnato.
	return ERTPointerBackStep::None;
}

ERTPointerOutcome URTPointerLibrary::ResolveOutcome(ERTPointerContext Context, bool bHitUnit, bool bCommandable,
	bool bObserved)
{
	// Nessuna unita' sotto il cursore: questa funzione non ha niente da dire. Le celle, i bordi e gli oggetti
	// logici sono di `ResolveTarget`, che ha i candidati in mano; rispondere qualcosa di diverso da `NoOp`
	// significherebbe avere due funzioni che decidono sullo stesso hit.
	if (!bHitUnit)
	{
		return ERTPointerOutcome::NoOp;
	}

	// 🔴 **I contesti in cui l'input di gioco non arriva rispondono `Blocked`, non `NoOp`.** Il controller
	// esce prima, su `IsGameplayInputBlocked`, quindi in produzione questi rami non si raggiungono — ma la
	// matrice §5 dev'essere **totale**, e un buco qui sarebbe un esito non dichiarato invece di un esito
	// dichiarato irraggiungibile. ⚠️ `Blocked` e non `NoOp` perche' e' un rifiuto: la DoD di `#705` chiede
	// che *«ogni rifiuto porti un reason code»*, e un `NoOp` non ne ha uno da portare.
	if (Context == ERTPointerContext::Modal
		|| Context == ERTPointerContext::ResolutionPlayback
		|| Context == ERTPointerContext::ReactionWindow)
	{
		return ERTPointerOutcome::Blocked;
	}

	// ⛔ **Il velo decide PRIMA della squadra, e risponde `NoOp` — NON `Blocked`.**
	//
	// 🔴 L'ordine conta: guardando prima `bCommandable`, un'avversaria nascosta produrrebbe `Inspect`, e la
	// sequenza stessa direbbe che li' c'e' qualcosa.
	//
	// 🔴 **E l'esito conta quanto l'ordine.** Una prima stesura rispondeva `Blocked`, citando la DoD di
	// `#705` — *«ogni rifiuto porta un reason code»* — e si contraddiceva con il proprio commento accanto,
	// che prometteva un comportamento *«indistinguibile da una cella vuota»*. Una cella vuota da' `NoOp`:
	// `Blocked` sarebbe stato **distinguibile**, e un reason code su un'unita' che non dovresti sapere
	// esistere e' esattamente il canale che questa riga esiste per chiudere. Cio' che il velo nasconde non
	// e' un rifiuto — e' un nulla, e va risposto come tale ([D-225]).
	//
	// ⚠️ `bObserved` non vincola le unita' comandabili: le proprie si vedono sempre, e chiedere al velo di
	// autorizzarle introdurrebbe un modo di perdere il comando delle proprie unita'.
	if (!bCommandable && !bObserved)
	{
		return ERTPointerOutcome::NoOp;
	}

	// 🔑 **Un'unita' comandabile si seleziona SEMPRE, `Targeting` incluso**, e non e' una semplificazione:
	// e' il comportamento di oggi. Il ramo che bersaglia (`RTPlayerController.cpp`) pretende
	// `ClickedUnit->TeamId != SelectedUnit->TeamId`, quindi cliccare una propria unita' mentre si mira la
	// **ri-seleziona** invece di bersagliarla. ⌫ Una prima stesura di questa funzione metteva il ramo
	// `Targeting` per primo e rispondeva `Confirm` anche per le proprie: avrebbe fatto bersagliare le
	// compagne, che e' un cambio di gameplay travestito da riordino.
	if (bCommandable)
	{
		return ERTPointerOutcome::Select;
	}

	// Non comandabile. In `Targeting` il click e' la conferma del bersaglio, ed e' la parte della decisione
	// del 2026-09-11 che dice **cosa NON cambia**: la lettura «larga» — `LMB` non bersaglia mai — e' stata
	// scartata perche' toglierebbe il gesto a ogni abilita' a bersaglio singolo.
	// ⚠️ `Confirm` NON significa «il bersaglio e' legale»: portata, linea di tiro e slot restano di chi
	// chiama. Significa «questo click e' una conferma di bersaglio», che e' l'unica cosa che il contesto sa.
	//
	// 🔑 Fuori da `Targeting`, `Inspect`. **`Inspect` e `Select` sono esiti DIVERSI e non due nomi della
	// stessa cosa**: selezionare significa comandare, e cio' che segue la selezione — il pannello degli slot,
	// il dock delle azioni — leggerebbe di un'avversaria il piano e il kit. L'esito separato esiste perche'
	// il soggetto ispezionato sia un'altra cosa dal soggetto comandato.
	return Context == ERTPointerContext::Targeting ? ERTPointerOutcome::Confirm : ERTPointerOutcome::Inspect;
}
