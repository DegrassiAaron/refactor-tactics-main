#pragma once

#include "CoreMinimal.h"
#include "Unit/RTUnit.h"

/**
 * L'ACCESSO RISERVATO AI TEST PER IL PIANO DELL'UNITA' (#4076, E50 fetta 2, tappa 2).
 *
 * I test scrivono il piano dai mutatori di `ARTUnit`, come il codice di gioco (#4068). Qui passano solo gli stati che
 * nessun mutatore produce: un percorso scritto senza ricavarne la cella d'arrivo, i waypoint azzerati senza il percorso,
 * l'abilita' dello scatto senza la sua cella, una mira a cella dichiarata a meta'. Sono gli stati che un test costruisce
 * per provare un resolver o una vista, e un mutatore li renderebbe coerenti, cambiando quello che il test prova.
 *
 * 🔑 **Ogni funzione scrive UN campo, e solo quello**: e' l'assegnazione che sostituisce, nient'altro. Un'operazione con
 * un significato (armare, dichiarare, ritirare) non sta qui: sta in `ARTUnit`, accanto alle altre.
 *
 * 🔑 **Perche' esiste**: quando lo struct `FRTUnitPlan` rendera' i campi privati ([D-608], tappa 3), cambieranno `ARTUnit`
 * e questo file, non i test che lo chiamano. E' l'unico file dei test che `Meta.PlanFieldsAreWrittenOnlyThroughTheUnit`
 * lascia fuori dal perimetro, per nome.
 *
 * Una `struct` con funzioni statiche e non un namespace: alla tappa 3 diventa `friend` di `ARTUnit` con una riga.
 */
struct FRTUnitPlanTestAccess
{
	/** Il percorso, cosi' com'e': la cella d'arrivo non si tocca (`SetPlannedRoute` la ricaverebbe). */
	static void SetPath(ARTUnit& Unit, const TArray<FRTCellId>& Path) { Unit.PlannedPath = Path; }
	static void ResetPath(ARTUnit& Unit) { Unit.PlannedPath.Reset(); }
	static void AddPathCell(ARTUnit& Unit, const FRTCellId& Cell) { Unit.PlannedPath.Add(Cell); }

	/** I waypoint, senza toccare il percorso (`ClearPlannedRoute` li azzererebbe insieme). */
	static void SetWaypoints(ARTUnit& Unit, const TArray<FRTCellId>& Waypoints) { Unit.PlannedWaypoints = Waypoints; }
	static void ResetWaypoints(ARTUnit& Unit) { Unit.PlannedWaypoints.Reset(); }

	/** Una sola meta' dello scatto (`SetPlannedDash` le scrive insieme). */
	static void SetDashAbility(ARTUnit& Unit, int32 AbilityIndex) { Unit.PlannedDashAbility = AbilityIndex; }
	static void SetDashCell(ARTUnit& Unit, const FRTCellId& Cell) { Unit.PlannedDashCell = Cell; }

	/** Una sola meta' della mira a cella (`DeclareAttackOnCell` ritira anche il bersaglio-unita'). */
	static void SetAttackCell(ARTUnit& Unit, const FRTCellId& Cell) { Unit.PlannedAttackCell = Cell; }
	static void SetAttackTargetsCell(ARTUnit& Unit, bool bTargetsCell) { Unit.bAttackTargetsCell = bTargetsCell; }

	/** Una sola meta' del bordo dichiarato (`DeclarePlannedCoverEdge` le scrive insieme). */
	static void SetCoverEdge(ARTUnit& Unit, ERTHexDirection Edge) { Unit.PlannedCoverEdge = Edge; }
	static void SetHasCoverEdge(ARTUnit& Unit, bool bHasEdge) { Unit.bHasPlannedCoverEdge = bHasEdge; }

	/** Il solo segno del verso dichiarato (`DeclarePlannedFacing` scrive anche il verso). */
	static void SetDeclaresFacing(ARTUnit& Unit, bool bDeclares) { Unit.bDeclaresPlannedFacing = bDeclares; }

	/** Il segno per il Back, da solo (`DeclarePlannedMainAction` lo scrive insieme all'indice). */
	static void SetWaypointsAllaDichiarazione(ARTUnit& Unit, int32 Count) { Unit.WaypointsAllaDichiarazione = Count; }
};
