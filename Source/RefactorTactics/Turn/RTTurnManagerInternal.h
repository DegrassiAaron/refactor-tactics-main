#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"
#include "Perception/RTPerceptionLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Turn/RTFacingLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapAsset.h"

/**
 * Gli helper che PIU' FASI della risoluzione condividono (`#2679`).
 *
 * 🔴 **Perche' esistono qui e non piu' in un namespace anonimo.** Vivevano in due blocchi senza nome dentro
 * `RTTurnManager.cpp`, e quella collocazione ha smesso di reggere quando la risoluzione del movimento e' stata
 * spostata in `RTTurnManager_Movement.cpp`: `FinishMovementResolution` e `ResolveReactionBoundary` non
 * potevano seguirla, perche' un namespace anonimo e' visibile a **una** unita' di traduzione e loro ne
 * usavano quattro. Il risultato era che il file da alleggerire non poteva essere alleggerito proprio nei
 * punti che pesavano.
 *
 * ⚠️ **Sono condivisi DA PRIMA, e non da adesso**: `BuildRouteObserverTeams` e `FreezeRouteVerdicts` servono
 * gia' a `ResolveDash` e alla risoluzione del movimento; `BoundaryFacing` a tre punti diversi. Questo header
 * non li rende condivisi — **dichiara** una condivisione che c'era gia' e che il namespace anonimo teneva
 * implicita.
 *
 * ⛔ **Non e' un'API pubblica del turno.** Nessuno fuori dai `RTTurnManager*.cpp` deve includerlo: cio' che
 * il resto del progetto puo' usare sta nelle librerie (`URTHexSimLibrary`, `URTPerceptionLibrary`), che sono
 * pure e testate senza mondo. Qui vivono dettagli di come UNA classe risolve il proprio turno.
 *
 * 🔑 **`inline` e non `static`**: le funzioni sono usate da piu' unita' di traduzione, e `static` ne
 * produrrebbe una copia per file — con l'effetto, silenzioso, che due fasi potrebbero divergere se qualcuno
 * ne modificasse una sola.
 */
namespace RTTurnManagerInternal
{
/** Gli osservatori di una squadra, alle posizioni con cui si decide il verdetto della traccia. */
struct FRTRouteObserverTeam
{
	int32 TeamId = INDEX_NONE;
	TArray<FRTPerceiver> Observers;
};

/**
 * Un gruppo di osservatori per ogni squadra VIVA, dalle posizioni correnti degli attori.
 *
 * ⚠️ **Chiamata PRIMA di `PlaceOnCell`**, quindi le celle sono quelle di inizio fase: e' il campione
 * dichiarato come limite in [D-223] — `TeamKnowledgeState` ha due sole assegnazioni per turno, entrambe
 * per fase, e un campione per micro-step non esiste. Risponde bene quando a nascondere e' il movimento
 * del NEMICO, sbaglia quando e' quello dell'osservatore.
 *
 * Costruita UNA volta per risoluzione e non per rotta: `VisibleCells` ricostruisce l'area visibile a
 * ogni chiamata, e rifarla per ogni cella di ogni percorso sarebbe lo stesso lavoro moltiplicato.
 *
 * L'ordine e' quello dei `TeamId` crescenti, come `RefreshTeamKnowledgeForPlanning`: l'ordine di un
 * `TSet` dipende dall'hash, e qui si itera (invariante #3).
 */
inline TArray<FRTRouteObserverTeam> BuildRouteObserverTeams(const TArray<ARTUnit*>& Units)
{
	TSet<int32> Teams;
	for (const ARTUnit* U : Units)
	{
		if (IsValid(U) && U->IsAlive()) { Teams.Add(U->TeamId); }
	}
	TArray<int32> SortedTeams = Teams.Array();
	SortedTeams.Sort();

	TArray<FRTRouteObserverTeam> Out;
	Out.Reserve(SortedTeams.Num());
	for (int32 TeamId : SortedTeams)
	{
		FRTRouteObserverTeam Entry;
		Entry.TeamId = TeamId;
		for (const ARTUnit* U : Units)
		{
			// Un cadavere non vede: stessa guardia di `RefreshTeamKnowledgeForPlanning`.
			if (!IsValid(U) || !U->IsAlive() || U->TeamId != TeamId) { continue; }
			FRTPerceiver P;
			P.Cell = U->Cell;
			P.Facing = U->Facing;
			P.VisionRange = U->VisionRange;
			Entry.Observers.Add(P);
		}
		Out.Add(MoveTemp(Entry));
	}
	return Out;
}

/**
 * Chi puo' vedere disegnata UNA cella percorsa da un soggetto di `SubjectTeamId` ([D-223]).
 *
 * 🔴 **La squadra del soggetto vede sempre la propria traccia**, e non passa dalla percezione: sai dove
 * si e' mosso il tuo anche se in quel momento nessun compagno lo guardava. E' lo stesso ramo che
 * `ARTHUD::ShouldDrawUnitOverlay` risolve con `bIsOwnTeam` e che `ClassifyTarget` risolve con
 * `TargetTeamId == Knowledge.TeamId`: senza, la traccia di un proprio esploratore sparirebbe a chi lo ha
 * mandato avanti, che non e' conoscenza parziale ma amnesia.
 *
 * ⚠️ **Il predicato e' `TeamAwarenessOfCell`, quello che la produzione usa gia'** nel ciclo a micro-step
 * del Move: riscrivere qui un confronto su `VisibleCells` sarebbe la terza via che [D-223] vieta — due
 * riletture della stessa regola, libere di divergere.
 *
 * Fail-closed per costruzione: una squadra assente da `Teams` non riceve il bit, e senza mappa il
 * verdetto e' `NoOne()` — la traccia sparisce, mai il contrario.
 */
inline FRTKnowledgeVerdict FreezeRouteCellVerdict(const URTHexMapAsset* Map,
	const TArray<FRTRouteObserverTeam>& Teams, int32 SubjectTeamId, const FRTCellId& Cell)
{
	FRTKnowledgeVerdict Verdict;
	for (const FRTRouteObserverTeam& Team : Teams)
	{
		if (Team.TeamId == SubjectTeamId
			|| URTPerceptionLibrary::TeamAwarenessOfCell(Map, Team.Observers, Cell) == ERTAwareness::Detected)
		{
			Verdict.AllowTeam(Team.TeamId);
		}
	}
	return Verdict;
}

/** I verdetti di una rotta intera, uno per cella e nello stesso ordine. */
inline void FreezeRouteVerdicts(const URTHexMapAsset* Map, const TArray<FRTRouteObserverTeam>& Teams,
	int32 SubjectTeamId, const TArray<FRTCellId>& Cells, TArray<FRTKnowledgeVerdict>& Out)
{
	Out.Reset();
	Out.Reserve(Cells.Num());
	for (const FRTCellId& Cell : Cells)
	{
		Out.Add(FreezeRouteCellVerdict(Map, Teams, SubjectTeamId, Cell));
	}
}

/** La riduzione che la copertura applica a un colpo deciso a un decision boundary.
 *
 *  🔴 **Un colpo di boundary e' un TIRO NORMALE** ([#888], 2026-08-25): usa lo stesso
 *  `EffectiveCoverReduction` del Blast, quindi eredita copertura **e** facing — chi viene preso fuori
 *  dall'arco frontale perde il beneficio del muro. Il brief dice che chi arma *«spara con la propria
 *  arma»*: se l'arma e' la stessa lo sono anche le regole del tiro, e il counterplay del difensore
 *  resta la **rotta**.
 *
 *  ⚠️ La cella del bersaglio e' quella del **micro-step corrente**, ed e' deterministica: il resolver
 *  non muove in continuo, quindi non esiste l'ambiguita' «cella lasciata o raggiunta» che rendeva la
 *  domanda difficile finche' la si guardava a parole.
 *
 *  🔴 **ENTRAMBE le celle arrivano dal chiamante, e non e' stile** (`#2142`). `EffectiveCoverReduction`
 *  legge tre ingressi — `Attacker.Cell`, `Target.Cell`, `Target.Facing` — quindi la posizione di **chi
 *  spara** conta quanto quella di chi incassa. La firma precedente prendeva la cella del bersaglio come
 *  parametro e quella dell'attaccante dall'Actor, e quell'asimmetria non era neutra: **invitava** il
 *  difetto che l'ha resa necessaria. L'Overwatch passava `Target->Cell` — la cella di **partenza del
 *  turno**, perche' `PlaceOnCell` gira dopo il ciclo dei micro-step — mentre trenta righe sopra il
 *  proprio watcher veniva costruito da `State.Pos[OwnerIdx]`: due letture della stessa posizione nello
 *  stesso micro-step, con esiti diversi. Con entrambe le celle esplicite quel sito non e' piu'
 *  scrivibile per distrazione.
 *
 *  Gli Actor restano nella firma per cio' che la posizione non porta: squadra, orientamento, vitalita'.
 *
 *  `Shape::Single`: un colpo di boundary ha un bersaglio solo. */
/**
 * **ADR-0008 §2 — il facing entra come PARAMETRO, come gia' la cella.** Fino a `#2131` questa funzione
 * leggeva `Target->Facing`, cioe' l'orientamento d'INGRESSO nella fase: dentro il ciclo dei micro-step
 * l'attore non e' ancora stato ne' spostato ne' riorientato. E' la stessa forma del difetto che `#2142`
 * ha corretto sulle CELLE — *«le due letture erano divergenti nello stesso micro-step»* — un anello piu'
 * in la': la cella veniva dal boundary e il facing dall'inizio del turno.
 *
 * Chi chiama dal boundary passa `FacingAtMicroStep`; la predittiva, che risolve **fuori** dal ciclo,
 * passa il facing dell'attore ed e' invariata.
 */
/**
 * **ADR-0008 §2** — il facing che un'unita' IN MOVIMENTO ha al decision boundary corrente: la direzione
 * dell'ultimo passo compiuto, derivata dal prefisso della rotta gia' percorsa.
 *
 * `FacingAtMoveStart` e' l'orientamento d'ingresso nella fase, cioe' `ARTUnit::Facing` finche' il ciclo
 * dei micro-step gira: `RecordFacingChange(DerivedFromMove)` scrive dopo l'uscita, e il pivot dichiarato
 * dopo ancora. Passare `Unit->Facing` e' quindi corretto **solo da dentro il ciclo**.
 *
 * Fail-closed su uno stato incoerente: senza rotta non c'e' niente da derivare e vale il facing d'ingresso.
 */
inline ERTHexDirection BoundaryFacing(const FRTMovementResolutionState& State, int32 UnitIdx,
	ERTHexDirection FacingAtMoveStart)
{
	if (!State.Paths.IsValidIndex(UnitIdx) || State.Paths[UnitIdx].Num() == 0
		|| !State.Results.IsValidIndex(UnitIdx))
	{
		return FacingAtMoveStart;
	}
	return URTFacingLibrary::FacingAtMicroStep(State.Paths[UnitIdx][0], State.Results[UnitIdx].Entered,
		FacingAtMoveStart);
}

inline int32 BoundaryCoverReduction(const URTHexMapAsset* Map, const ARTUnit* Attacker, const FRTCellId& AttackerCell,
	ERTHexDirection AttackerFacing, const ARTUnit* Target, const FRTCellId& TargetCell,
	ERTHexDirection TargetFacing, bool& bOutFacingWasRead)
{
	// ⚠️ Inizializzato PRIMA di ogni uscita anticipata: un `false` non scritto lascerebbe al chiamante
	// il valore che aveva prima, e la voce nascerebbe o mancherebbe a seconda di cosa c'era in pila.
	bOutFacingWasRead = false;
	if (Map == nullptr || Attacker == nullptr || Target == nullptr) { return 0; }

	FRTHexCombatUnit A;
	A.UnitId = 0;
	A.TeamId = Attacker->TeamId;
	// Da DOVE il colpo parte: l'Overwatch passa `State.Pos[OwnerIdx]`, che e' la stessa cella con cui
	// `ResolveReactionBoundary` ha costruito la zona — un watcher spinto spara da dove e' finito, come
	// vuole [D-169]. La predittiva passa `Shooter->Cell`; vedi il suo sito per il limite che porta.
	A.Cell = AttackerCell;
	A.bAlive = Attacker->IsAlive();
	A.Facing = AttackerFacing;

	FRTHexCombatUnit T;
	T.UnitId = 1;
	T.TeamId = Target->TeamId;
	// La cella su cui il colpo e' deciso, passata dal chiamante: la predittiva usa la cella
	// BLOCCATA (`Armed.LockedCell`, quella su cui si e' scommesso) perche' al momento del danno
	// il troncamento del movimento non e' ancora avvenuto. L'Overwatch usa la cella del **micro-step**
	// (`State.Pos[TargetIdx]`), come ADR-0004 §*«Quale cella»* prescrive da sempre: e' la riga
	// *«Overwatch FIRE | la cella corrente | al suo micro-step e' gia' quella giusta»*, che questo
	// commento citava mentre il codice faceva l'opposto.
	T.Cell = TargetCell;
	T.bAlive = Target->IsAlive();
	T.Facing = TargetFacing;

	return URTHexCombatLibrary::EffectiveCoverReduction(Map, A, T, ERTAbilityShape::Single,
		bOutFacingWasRead);
}
}
