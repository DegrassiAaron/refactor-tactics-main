#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "RTGridLibrary.generated.h"

/**
 * Utility pure per la griglia logica, indipendenti dal rendering e dal mondo.
 * Le conversioni prendono origine e dimensione cella espliciti cosi' restano testabili senza un Actor.
 *
 * ATTENZIONE (CP 6.1): questa libreria applica geometria QUADRATA (distanza di Manhattan, 4 vicini, linee e
 * coni su assi ortogonali) a coordinate ora di tipo FRTCellId, che e' ASSIALE. Finche' il flusso di partita
 * non e' migrato a URTHexLibrary/URTHexPathLibrary/URTHexVisionLibrary (CP 6.2-6.4) le due cose convivono:
 * qui e' cambiato il TIPO, non le REGOLE. Non usare queste funzioni per logica esagonale nuova — usare le
 * librerie hex. Rimozione pianificata in M7 (dismissione del quadrato).
 */
UCLASS()
class REFACTORTACTICS_API URTGridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Centro-mondo (X,Y) della cella; Z = Origin.Z. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static FVector CellToWorld(const FRTCellId& Cell, const FVector& Origin, float CellSize);

	/**
	 * Come CellToWorld ma con offset verticale del pivot: Z = CellToWorld.Z + ZOffset + Layer*LayerHeight.
	 * ZOffset 0 = pivot ai piedi (personaggi UE); ~90 = centro del cilindro segnaposto (retrocompat).
	 * Puro/testabile: la posizione visiva dell'unita' non dipende dal tipo di mesh.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static FVector CellToWorldElevated(const FRTCellId& Cell, const FVector& Origin, float CellSize,
		float ZOffset, float LayerHeight);

	/**
	 * Yaw (gradi) per orientare un attore da From verso To sul piano XY (facing planare, Z ignorata).
	 * Convenzione UE: +X = 0, +Y = 90, -X = +/-180, -Y = -90. Direzione nulla -> 0. Puro/testabile.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static float DirectionYaw(const FVector& From, const FVector& To);

	/** Cella che contiene la posizione mondo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static FRTCellId WorldToCell(const FVector& World, const FVector& Origin, float CellSize);

	/** Vero se la cella e' dentro la griglia [0,Width) x [0,Height) (ignora il Layer). Pura. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static bool IsInBounds(const FRTCellId& Cell, int32 Width, int32 Height);

	/** Vero se la cella e' dentro una griglia Width x Height con origine logica (0,0). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static bool IsInsideGrid(const FRTCellId& Cell, int32 Width, int32 Height);

	/** Distanza di Manhattan tra due celle. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static int32 ManhattanDistance(const FRTCellId& A, const FRTCellId& B);

	/** Vero se To e' raggiungibile da From entro Range passi (distanza di Manhattan <= Range). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static bool IsWithinRange(const FRTCellId& From, const FRTCellId& To, int32 Range);

	/**
	 * Vero se la linea di tiro tra From e To non attraversa alcuna cella bloccante.
	 * From e To non bloccano mai (l'attaccante e il bersaglio non si coprono da soli).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static bool HasLineOfSight(const FRTCellId& From, const FRTCellId& To, const TArray<FRTCellId>& Blockers);

	/** Tutte le celle entro Radius passi (Manhattan) da Center, incluso Center. Radius 0 = solo Center. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTCellId> CellsInRadius(const FRTCellId& Center, int32 Radius);

	/** Celle attraversate dalla linea From->To (From escluso, To incluso). Usata per gli attacchi lineari. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTCellId> CellsInLine(const FRTCellId& From, const FRTCellId& To);

	/**
	 * Cono (a 45°, allineato agli assi) da From nella direzione di Target, lungo Range celle:
	 * a distanza d dall'origine il cono si allarga di d celle per lato. Usato per gli attacchi a ventaglio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTCellId> CellsInCone(const FRTCellId& From, const FRTCellId& Target, int32 Range);

	/**
	 * Celle raggiungibili da From entro MoveRange passi ortogonali (BFS, costo 1/passo, niente diagonali),
	 * senza attraversare celle bloccanti (Blockers) e restando dentro la griglia. Include From.
	 * A differenza di IsWithinRange (Manhattan), rispetta gli ostacoli: una cella "vicina" ma dietro un
	 * muro non è raggiungibile. Ordine celle stabile (deterministico). Base del path finding (FR-PATH-01).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTCellId> ReachableCells(const FRTCellId& From, int32 MoveRange,
		const TArray<FRTCellId>& Blockers, int32 Width, int32 Height);

	/**
	 * Percorso ortogonale minimo da From a To (BFS, costo uniforme), aggirando i Blockers e restando
	 * dentro la griglia. Ritorna la sequenza di celle da From a To inclusi; vuoto se To è irraggiungibile,
	 * bloccato o fuori griglia. Deterministico. Usato per la preview del movimento (FR-PATH-05).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTCellId> FindPath(const FRTCellId& From, const FRTCellId& To,
		const TArray<FRTCellId>& Blockers, int32 Width, int32 Height);

	// --- Pathfinding pesato (PF.3): costo per cella dato da CellCost (assente = 1; RT_BLOCKED_COST = impassabile).
	//     Il costo di un percorso e' la somma dei costi di ENTRATA delle celle (From costa 0).
	//     Non UFUNCTION: la TMap con chiave USTRUCT non e' esposta a Blueprint; uso interno + test.

	/**
	 * Celle raggiungibili da From con costo accumulato <= CostBudget (Dijkstra), aggirando le celle
	 * impassabili e restando dentro la griglia. Include From. Ordine celle stabile (FR-PATH-07).
	 */
	static TArray<FRTCellId> ReachableCellsByCost(const FRTCellId& From, int32 CostBudget,
		const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height);

	/**
	 * Percorso a COSTO minimo da From a To (Dijkstra): fra due rotte preferisce la più economica,
	 * anche se più lunga in celle. Ritorna From..To inclusi; vuoto se irraggiungibile/bloccato/fuori
	 * griglia. Deterministico (FR-PATH-06). Euristica: non necessaria su griglia MVP (Dijkstra).
	 */
	static TArray<FRTCellId> FindPathByCost(const FRTCellId& From, const FRTCellId& To,
		const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height);

	/**
	 * Costo totale di un percorso (somma dei costi di ENTRATA, From escluso), o -1 se NON valido.
	 * Uno step e' valido se: celle ortogonalmente adiacenti (costo = costo della cella), OPPURE esiste
	 * un arco From->To in Edges (costo = costo dell'arco). Nessuna delle due -> invalido. Cella
	 * impassabile -> invalido. Path vuoto o di 1 cella = costo 0 (fermo). Edges vuoti = validazione 2D.
	 */
	static int32 PathCost(const TArray<FRTCellId>& Path, const TMap<FRTCellId, int32>& CellCost,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());

	/**
	 * Percorso composito: concatena gli auto-route (FindPathByCost) da Start attraverso i Waypoints in
	 * ordine. Ritorna Start..ultimoWaypoint; vuoto se un tratto e' irraggiungibile. Base della path
	 * composita a waypoint (FR-PATH-09). Nessun waypoint = solo [Start].
	 */
	static TArray<FRTCellId> BuildCompositePath(const FRTCellId& Start,
		const TArray<FRTCellId>& Waypoints, const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());

	// --- Pathfinding a GRAFO (PF.4): oltre ai 4 vicini ortogonali (stesso layer), attraversa gli archi
	//     espliciti (Edges: scale/portali/cross-layer). Generalizza le versioni ByCost al multilivello.

	/** Come ReachableCellsByCost ma includendo gli archi di traversata (multilivello). */
	static TArray<FRTCellId> ReachableCellsByGraph(const FRTCellId& From, int32 CostBudget,
		const TMap<FRTCellId, int32>& CellCost, const TArray<FRTTraversalEdge>& Edges, int32 Width, int32 Height);

	/** Come FindPathByCost ma includendo gli archi di traversata (percorso a costo minimo su grafo). */
	static TArray<FRTCellId> FindPathByGraph(const FRTCellId& From, const FRTCellId& To,
		const TMap<FRTCellId, int32>& CellCost, const TArray<FRTTraversalEdge>& Edges, int32 Width, int32 Height);
};
