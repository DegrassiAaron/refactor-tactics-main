#include "Turn/RTPlanPreview.h"

#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"  // BuildCompositeHexPath: lo STESSO A* del resolver
#include "Turn/RTFacingLibrary.h"  // FacingFromPath: la derivazione pura, senza TurnLog

namespace
{
	/** L'unita' nello snapshot, o `nullptr`. Identita' per indice, come ovunque nel simulatore. */
	const FRTHexSimUnit* UnitOf(const FRTHexSnapshot& Snapshot, int32 UnitId)
	{
		for (const FRTHexSimUnit& U : Snapshot.Units)
		{
			if (U.UnitId == UnitId)
			{
				return &U;
			}
		}
		return nullptr;
	}

	/**
	 * Il facing che guarda da `From` a `To`, derivato dalla regola e non inventato.
	 *
	 * ⚠️ **`FacingFromPath` e non `ReadFacingForConsumer`**: la seconda scrive una voce di TurnLog, cioe'
	 * dichiara che il RESOLVER ha letto quel facing. Un'anteprima che la chiamasse inciderebbe nel registro
	 * canonico una lettura mai avvenuta in partita. Vedi l'header.
	 */
	ERTHexDirection FacingVerso(const FRTCellId& From, const FRTCellId& To, ERTHexDirection Corrente)
	{
		if (From == To)
		{
			return Corrente;
		}
		const TArray<FRTCellId> Tratto = { From, To };
		return URTFacingLibrary::FacingFromPath(Tratto, Corrente);
	}
}

FRTPlanPreview URTPlanPreviewLibrary::MakePlanPreview(const FRTHexSnapshot& Snapshot,
	const FRTPlanPreviewInput& Plan, const TArray<FRTHexCombatUnit>& CombatUnits)
{
	FRTPlanPreview Out;

	const FRTHexSimUnit* Unit = UnitOf(Snapshot, Plan.UnitId);
	if (!Unit || !Unit->bAlive)
	{
		// Nessuna unita', nessuna timeline. ⛔ Non si restituisce una lista di fasi vuote: «non c'e' un piano»
		// e «il piano non fa niente» sono due cose diverse, e una lista vuota le distingue da sola.
		return Out;
	}

	const FRTCellId CellaCorrente = Unit->Cell;
	const ERTHexDirection FacingCorrente = Unit->Facing;

	// Lo stato che ogni fase consegna alla successiva. Le fasi risolvono in ordine, quindi la fase N vede
	// cio' che la N-1 ha lasciato: leggere sempre lo snapshot iniziale mostrerebbe quattro fasi che partono
	// tutte dallo stesso posto, cioe' l'errore che questo checkpoint esiste per togliere.
	FRTCellId CellaCorrenteDiFase = CellaCorrente;
	ERTHexDirection FacingCorrenteDiFase = FacingCorrente;

	// ── PREP ────────────────────────────────────────────────────────────────────────────────────────────
	//
	// Non sposta e non bersaglia: per la definizione di `ERTIntentCertainty::Confirmed` — *«niente puo'
	// cambiarlo: nessun movimento, nessun bersaglio»* — e' l'unica fase che puo' essere certa.
	if (Plan.bReactionArmed || !Plan.PrepActionId.IsNone())
	{
		FRTPhasePreviewEntry Prep;
		Prep.Phase = ERTResolutionPhase::Preparation;
		Prep.UnitId = Plan.UnitId;
		Prep.ActionId = Plan.PrepActionId;
		Prep.PreviewOrigin = CellaCorrenteDiFase;
		Prep.PreviewDestination = CellaCorrenteDiFase;
		Prep.Facing = FacingCorrenteDiFase;
		Prep.FacingSource = ERTPreviewFacingSource::Authoritative;
		Prep.PoseId = TEXT("Pose.Prep");
		Prep.Certainty = ERTIntentCertainty::Confirmed;
		Out.Phases.Add(Prep);
	}

	// ── DASH ────────────────────────────────────────────────────────────────────────────────────────────
	//
	// ⚠️ La voce si mostra anche quando lo scatto **non** si applica, e il livello di certezza e' cio' che
	// porta la differenza: nasconderla direbbe al giocatore che non ha pianificato uno scatto, mentre lo ha
	// pianificato e potrebbe non arrivare.
	if (Plan.bDashPlanned)
	{
		FRTPhasePreviewEntry Dash;
		Dash.Phase = ERTResolutionPhase::FastMovement;
		Dash.UnitId = Plan.UnitId;
		Dash.ActionId = Plan.DashActionId;
		Dash.PreviewOrigin = CellaCorrenteDiFase;
		Dash.PreviewDestination = Plan.PlannedDashCell;
		Dash.PreviewPath = { CellaCorrenteDiFase, Plan.PlannedDashCell };
		Dash.Facing = FacingVerso(CellaCorrenteDiFase, Plan.PlannedDashCell, FacingCorrenteDiFase);
		Dash.FacingSource = ERTPreviewFacingSource::DerivedFromPath;
		Dash.PoseId = TEXT("Pose.Dash");
		// «Muoversi basta»: le celle del percorso sono contendibili e il resolver puo' troncare la rotta.
		Dash.Certainty = ERTIntentCertainty::Uncertain;
		Out.Phases.Add(Dash);

		// 🔴 **Solo se si applica DAVVERO.** `bDashResolves` e' la stessa domanda che `ResolveDash` si pone,
		// e propagare una cella d'arrivo che lo scatto non raggiunge sposterebbe anche l'origine del Blast.
		if (Plan.bDashResolves)
		{
			CellaCorrenteDiFase = Plan.PlannedDashCell;
			FacingCorrenteDiFase = Dash.Facing;
		}
	}

	// ── BLAST ───────────────────────────────────────────────────────────────────────────────────────────
	//
	// ⛔ L'origine e l'area NON si ricalcolano qui: le produce `MakeBlastPreview`, che a sua volta chiama
	// `HexHitCells` — la forma canonica del combat. Una seconda derivazione sarebbe la seconda autorita' che
	// questo checkpoint vieta espressamente.
	const FRTBlastPreview Blast = URTHexCombatLibrary::MakeBlastPreview(Plan.Blast, CombatUnits);
	if (Plan.Blast.bHasAction)
	{
		FRTPhasePreviewEntry Colpo;
		Colpo.Phase = ERTResolutionPhase::Attack;
		Colpo.UnitId = Plan.UnitId;
		Colpo.ActionId = Plan.BlastActionId;
		Colpo.PreviewOrigin = Blast.Origin;
		Colpo.PreviewDestination = Blast.Origin; // il Blast non sposta chi lo esegue
		Colpo.AffectedCells = Blast.HitCells;
		Colpo.AllyCells = Blast.AllyCells;
		Colpo.TargetRefusal = Plan.BlastTargetRefusal;
		Colpo.PoseId = TEXT("Pose.Blast");

		// Cio' che il piano DICHIARA di bersagliare, distinto da cio' che l'azione toccherebbe.
		FRTCellId CellaMira;
		bool bMiraNota = false;
		if (Plan.Blast.bTargetsCell)
		{
			CellaMira = Plan.Blast.TargetCell;
			bMiraNota = true;
		}
		else if (CombatUnits.IsValidIndex(Plan.Blast.TargetId))
		{
			CellaMira = CombatUnits[Plan.Blast.TargetId].Cell;
			bMiraNota = true;
		}
		if (bMiraNota)
		{
			Colpo.TargetCells.Add(CellaMira);
			// Il facing verso il bersaglio e' la stessa derivazione che il resolver registra come
			// `TargetingReoriented` (`FacingAfterPrepActionTargeting`): un'azione con bersaglio orienta chi
			// la esegue. Qui si PREVEDE quella rotazione, non se ne inventa un'altra.
			Colpo.Facing = FacingVerso(Blast.Origin, CellaMira, FacingCorrenteDiFase);
			Colpo.FacingSource = ERTPreviewFacingSource::DerivedFromPath;
		}
		else
		{
			Colpo.Facing = FacingCorrenteDiFase;
			Colpo.FacingSource = Plan.bDashResolves
				? ERTPreviewFacingSource::InheritedFromPreviousPhase
				: ERTPreviewFacingSource::Authoritative;
		}

		// 🔑 **Il livello segue le definizioni dell'enum, non un giudizio di questa funzione.**
		//  - bersaglio rifiutato o nessuna cella colpita: il piano non produrra' niente → `Uncertain`;
		//  - origine che viene da uno scatto: dipende da un movimento, e «muoversi basta» → `Uncertain`;
		//  - altrimenti: c'e' un bersaglio e l'unita' non si sposta → `Predicted`.
		const bool bBersaglioUtile =
			Plan.BlastTargetRefusal == ERTTargetRefusal::None && Blast.HitCells.Num() > 0;
		Colpo.Certainty = (!bBersaglioUtile || Blast.bOriginFromPlannedDash)
			? ERTIntentCertainty::Uncertain
			: ERTIntentCertainty::Predicted;

		Out.Phases.Add(Colpo);
		FacingCorrenteDiFase = Colpo.Facing;
	}

	// ── MOVE ────────────────────────────────────────────────────────────────────────────────────────────
	//
	// 🔴 **Il percorso lo costruisce `BuildCompositeHexPath`, ed e' la parita' che `CP 11.5` chiede.** Non e'
	// una funzione «equivalente»: e' letteralmente quella che riempie `ARTUnit::PlannedPath`
	// (`RTPlayerController.cpp:1830`), e `ResolveMovement` consuma quel percorso quando parte ancora dalla
	// cella dell'unita'. Preview e resolver leggono la stessa uscita della stessa funzione.
	//
	// ⚠️ **E il Move parte da DOPO lo scatto, non dalla cella di adesso.** `ResolveMovement` prende uno
	// snapshot FRESCO (`MakeCurrentSnapshot`, `RTTurnManager_Movement.cpp:109`), cioe' vede l'unita' gia'
	// scattata; e riusa `PlannedPath` solo se quel percorso parte ancora dalla cella corrente — dopo uno
	// scatto non ci parte piu', e il resolver **ricalcola** verso `PlannedCell`. Ecco perche' la certezza di
	// questa fase scende quando lo scatto si applica: cio' che si disegna e' il percorso dichiarato, e il
	// resolver ne percorrera' un altro.
	if (Plan.PlannedWaypoints.Num() > 0)
	{
		FRTHexSnapshot SnapshotDiFase = Snapshot;
		if (Plan.bDashResolves)
		{
			for (FRTHexSimUnit& U : SnapshotDiFase.Units)
			{
				if (U.UnitId == Plan.UnitId)
				{
					U.Cell = CellaCorrenteDiFase;
					break;
				}
			}
		}

		const FRTHexPathResult Percorso =
			URTHexSimLibrary::BuildCompositeHexPath(SnapshotDiFase, Plan.UnitId, Plan.PlannedWaypoints);

		FRTPhasePreviewEntry Move;
		Move.Phase = ERTResolutionPhase::NormalMovement;
		Move.UnitId = Plan.UnitId;
		Move.ActionId = Plan.MoveActionId;
		Move.PreviewOrigin = CellaCorrenteDiFase;
		Move.PreviewPath = Percorso.Path;
		// ⚠️ Un percorso RIFIUTATO torna vuoto (vedi il contratto di `BuildCompositeHexPath`), e allora la
		// destinazione e' l'origine: il piano non porta l'unita' da nessuna parte, e dirlo e' l'esito giusto.
		Move.PreviewDestination = Percorso.Path.Num() > 0 ? Percorso.Path.Last() : CellaCorrenteDiFase;
		Move.Facing = Percorso.Path.Num() >= 2
			? URTFacingLibrary::FacingFromPath(Percorso.Path, FacingCorrenteDiFase)
			: FacingCorrenteDiFase;
		Move.FacingSource = Percorso.Path.Num() >= 2
			? ERTPreviewFacingSource::DerivedFromPath
			: ERTPreviewFacingSource::InheritedFromPreviousPhase;
		Move.PoseId = TEXT("Pose.Move");
		Move.Certainty = ERTIntentCertainty::Uncertain;
		Out.Phases.Add(Move);
	}

	// ── LA REAZIONE, che fase non e' ────────────────────────────────────────────────────────────────────
	//
	// ⛔ Fuori da `Phases` di proposito: vedi `FRTReactionPreview`. Il livello e' `Uncertain` per definizione
	// dell'enum — *«vale **sempre** per una reazione armata, che per definizione attende un trigger che non
	// decidiamo noi»* — quindi non e' una scelta di questa funzione ed e' scritto altrove.
	if (Plan.bReactionArmed)
	{
		Out.Reaction.bArmed = true;
		Out.Reaction.UnitId = Plan.UnitId;
		Out.Reaction.ReactionProfileId = Plan.ReactionProfileId;
		// La cella di FINE Dash: la reazione e' armata in Prep e il Move risolve dopo il Blast, quindi
		// sorveglia da dove l'unita' si trovera' quando un innesco potra' scattare.
		Out.Reaction.WatchOrigin = Plan.bDashResolves ? Plan.PlannedDashCell : CellaCorrente;
		Out.Reaction.Facing = FacingCorrenteDiFase;
		Out.Reaction.Certainty = ERTIntentCertainty::Uncertain;
	}

	return Out;
}
