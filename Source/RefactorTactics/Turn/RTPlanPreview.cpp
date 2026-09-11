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
		Prep.Certainty = ERTIntentCertainty::Confirmed;
		Out.Phases.Add(Prep);
	}

	// ⚠️ **Lo scatto si applica solo se e' stato pianificato E risolve.** I due flag sono indipendenti
	// nell'ingresso, e leggerne uno solo — come faceva la prima stesura nel blocco della reazione — lascia
	// passare `bDashResolves` senza `bDashPlanned`, cioe' una cella d'arrivo che nessuna fase ha prodotto.
	const bool bScattoEffettivo = Plan.bDashPlanned && Plan.bDashResolves;

	// ── DASH ────────────────────────────────────────────────────────────────────────────────────────────
	//
	// ⚠️ La voce si mostra anche quando lo scatto **non** si applica, e il livello di certezza e' cio' che
	// porta la differenza: nasconderla direbbe al giocatore che non ha pianificato uno scatto, mentre lo ha
	// pianificato e potrebbe non arrivare.
	if (Plan.bDashPlanned)
	{
		// 🔴 **La rotta dello scatto la CALCOLA il resolver, e la prima stesura la inventava.**
		// Diceva `PreviewPath = { partenza, arrivo }` — due celle, una linea retta — mentre `ResolveDash`
		// fa `FindPathForUnit(Snapshot, i, PlannedDashCell).Path` (`RTTurnManager.cpp:5017`) e da quel
		// percorso ricava anche il facing. Su uno scatto che deve aggirare un ostacolo il ghost disegnava una
		// retta ATTRAVERSO l'ostacolo e dichiarava un orientamento che l'unita' non avrebbe mai avuto: lo
		// stesso difetto che la fase Move era stata scritta per evitare, sulla fase accanto.
		const FRTHexPathResult RottaScatto =
			URTHexSimLibrary::FindPathForUnit(Snapshot, Plan.UnitId, Plan.PlannedDashCell);

		FRTPhasePreviewEntry Dash;
		Dash.Phase = ERTResolutionPhase::FastMovement;
		Dash.UnitId = Plan.UnitId;
		Dash.ActionId = Plan.DashActionId;
		Dash.PreviewOrigin = CellaCorrenteDiFase;
		Dash.PreviewPath = RottaScatto.Path;
		// Una rotta RIFIUTATA torna vuota: allora lo scatto non porta da nessuna parte, e dirlo e' l'esito
		// giusto — come per il Move.
		Dash.PreviewDestination = RottaScatto.Path.Num() > 0 ? RottaScatto.Path.Last() : CellaCorrenteDiFase;
		Dash.Facing = RottaScatto.Path.Num() >= 2
			? URTFacingLibrary::FacingFromPath(RottaScatto.Path, FacingCorrenteDiFase)
			: FacingCorrenteDiFase;
		Dash.FacingSource = RottaScatto.Path.Num() >= 2
			? ERTPreviewFacingSource::DerivedFromPath
			: ERTPreviewFacingSource::InheritedFromPreviousPhase;
		// «Muoversi basta»: le celle del percorso sono contendibili e il resolver puo' troncare la rotta.
		Dash.Certainty = ERTIntentCertainty::Uncertain;
		Out.Phases.Add(Dash);

		// 🔴 **Solo se si applica DAVVERO.** `bDashResolves` e' la stessa domanda che `ResolveDash` si pone,
		// e propagare una cella d'arrivo che lo scatto non raggiunge sposterebbe anche l'origine del Blast.
		if (bScattoEffettivo)
		{
			CellaCorrenteDiFase = Dash.PreviewDestination;
			FacingCorrenteDiFase = Dash.Facing;
		}
	}

	// 🔑 **La cella e il facing DOPO lo scatto, in una sede sola.** La prima stesura ne aveva tre — la
	// catena di fase, i campi `Plan.Blast.*` che `MakeBlastPreview` consuma, e una terza derivazione dentro il
	// blocco della reazione — e con un ingresso incoerente rispondevano cose diverse alla stessa domanda.
	const FRTCellId CellaDopoScatto = CellaCorrenteDiFase;
	const ERTHexDirection FacingDopoScatto = FacingCorrenteDiFase;

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

		// Cio' che il piano DICHIARA di bersagliare, distinto da cio' che l'azione toccherebbe.
		//
		// ⛔ **Un bersaglio CADUTO non ha una cella dichiarata, e la prima stesura gliela dava.**
		// `MakeBlastPreview` esige `Units[TargetId].bAlive` e lo motiva: *«un bersaglio caduto non degrada
		// alla propria ultima cella — quello sarebbe il FALLBACK del resolver»*. Leggendo il solo
		// `IsValidIndex` la timeline segnava un bersaglio sulla cella di un cadavere, che l'area lasciava
		// giustamente vuota: due campi della stessa voce che si contraddicevano.
		FRTCellId CellaMira;
		bool bMiraSuUnitaViva = false;
		bool bMiraNota = false;
		if (Plan.Blast.bTargetsCell)
		{
			CellaMira = Plan.Blast.TargetCell;
			bMiraNota = true;
		}
		else if (CombatUnits.IsValidIndex(Plan.Blast.TargetId) && CombatUnits[Plan.Blast.TargetId].bAlive)
		{
			CellaMira = CombatUnits[Plan.Blast.TargetId].Cell;
			bMiraNota = true;
			bMiraSuUnitaViva = true;
		}
		if (bMiraNota)
		{
			Colpo.TargetCells.Add(CellaMira);
		}

		// 🔴 **La rotazione verso il bersaglio vale SOLO per un'unita' viva, e la prima stesura la
		// prevedeva anche per una cella.** Il resolver la guarda con
		// `if (Unit->IsAlive() && Target && Target->IsAlive() && Target != Unit)`
		// (`RTTurnManager_Blast.cpp:715`): un'azione bersagliata su una CELLA — un'area lasciata cadere su un
		// varco vuoto — non orienta chi la esegue. Prevederla comunque faceva leggere al giocatore una postura
		// di copertura direzionale che al momento del colpo non sarebbe esistita.
		if (bMiraSuUnitaViva)
		{
			// La stessa derivazione che il resolver registra come `TargetingReoriented`. Qui si PREVEDE
			// quella rotazione, non se ne inventa un'altra.
			Colpo.Facing = FacingVerso(Blast.Origin, CellaMira, FacingDopoScatto);
			Colpo.FacingSource = ERTPreviewFacingSource::DerivedFromPath;
		}
		else
		{
			Colpo.Facing = FacingDopoScatto;
			Colpo.FacingSource = bScattoEffettivo
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
		// 🔴 **Spostare l'unita' vuol dire spostare anche l'OCCUPAZIONE, e la prima stesura riscriveva
		// solo `Units[i].Cell`.** `BuildCompositeHexPath` non guarda `Units` per sapere cosa e' bloccato:
		// `BlockedCellsFor` itera `Snapshot.Occupancy` (`RTHexSimLibrary.cpp:34`). Con la sola cella riscritta
		// lo snapshot restava internamente incoerente — la cella lasciata continuava a risultare occupata dal
		// mover, e quella d'arrivo non risultava sua. Se un'altra unita' ne occupasse la destinazione, il
		// percorso partirebbe da un nodo bloccato e il Move collasserebbe a vuoto, **in silenzio**: un
		// percorso rifiutato si legge come «il piano non porta da nessuna parte».
		//
		// ⚠️ **E la copia si paga solo quando serve.** Senza scatto lo snapshot si usa com'e': copiarlo
		// duplicherebbe `Units`, `Occupancy`, `Overlaps` e la conoscenza di squadra a ogni waypoint aggiunto o
		// tolto, per poi non cambiarne niente.
		FRTHexSnapshot SnapshotDiFase;
		const FRTHexSnapshot* SnapshotPerIlMove = &Snapshot;
		if (bScattoEffettivo)
		{
			SnapshotDiFase = Snapshot;
			for (FRTHexSimUnit& U : SnapshotDiFase.Units)
			{
				if (U.UnitId == Plan.UnitId)
				{
					SnapshotDiFase.Occupancy.Remove(U.Cell);
					U.Cell = CellaDopoScatto;
					break;
				}
			}
			SnapshotDiFase.Occupancy.Add(CellaDopoScatto, Plan.UnitId);
			SnapshotPerIlMove = &SnapshotDiFase;
		}

		const FRTHexPathResult Percorso = URTHexSimLibrary::BuildCompositeHexPath(
			*SnapshotPerIlMove, Plan.UnitId, Plan.PlannedWaypoints);

		FRTPhasePreviewEntry Move;
		Move.Phase = ERTResolutionPhase::NormalMovement;
		Move.UnitId = Plan.UnitId;
		Move.ActionId = Plan.MoveActionId;
		Move.PreviewOrigin = CellaDopoScatto;
		Move.PreviewPath = Percorso.Path;
		// ⚠️ Un percorso RIFIUTATO torna vuoto (vedi il contratto di `BuildCompositeHexPath`), e allora la
		// destinazione e' l'origine: il piano non porta l'unita' da nessuna parte, e dirlo e' l'esito giusto.
		Move.PreviewDestination = Percorso.Path.Num() > 0 ? Percorso.Path.Last() : CellaDopoScatto;
		Move.Facing = Percorso.Path.Num() >= 2
			? URTFacingLibrary::FacingFromPath(Percorso.Path, FacingDopoScatto)
			: FacingDopoScatto;
		Move.FacingSource = Percorso.Path.Num() >= 2
			? ERTPreviewFacingSource::DerivedFromPath
			: ERTPreviewFacingSource::InheritedFromPreviousPhase;
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
		// ⚠️ **Cella E facing dello STESSO istante**, che e' la fine del Dash. La prima stesura prendeva la
		// cella dopo lo scatto e il facing dopo il BLAST: una reazione il cui innesco scatta durante il Dash —
		// o prima dell'attacco — veniva disegnata mentre guarda il bersaglio di un colpo non ancora partito.
		// L'argomento con cui `WatchOrigin` sceglie la cella del Dash sceglie anche il facing di quel momento.
		Out.Reaction.WatchOrigin = CellaDopoScatto;
		Out.Reaction.Facing = FacingDopoScatto;
		Out.Reaction.Certainty = ERTIntentCertainty::Uncertain;
	}

	return Out;
}
