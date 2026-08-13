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
