#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "RTGridLibrary.generated.h"

/**
 * Utility pure per la griglia logica, indipendenti dal rendering e dal mondo.
 * Le conversioni prendono origine e dimensione cella espliciti cosi' restano testabili senza un Actor.
 */
UCLASS()
class REFACTORTACTICS_API URTGridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Centro-mondo (X,Y) della cella; Z = Origin.Z. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static FVector CellToWorld(const FRTGridCoord& Cell, const FVector& Origin, float CellSize);

	/** Cella che contiene la posizione mondo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static FRTGridCoord WorldToCell(const FVector& World, const FVector& Origin, float CellSize);

	/** Vero se la cella e' dentro una griglia Width x Height con origine logica (0,0). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static bool IsInsideGrid(const FRTGridCoord& Cell, int32 Width, int32 Height);

	/** Distanza di Manhattan tra due celle. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static int32 ManhattanDistance(const FRTGridCoord& A, const FRTGridCoord& B);

	/** Vero se To e' raggiungibile da From entro Range passi (distanza di Manhattan <= Range). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static bool IsWithinRange(const FRTGridCoord& From, const FRTGridCoord& To, int32 Range);

	/**
	 * Vero se la linea di tiro tra From e To non attraversa alcuna cella bloccante.
	 * From e To non bloccano mai (l'attaccante e il bersaglio non si coprono da soli).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static bool HasLineOfSight(const FRTGridCoord& From, const FRTGridCoord& To, const TArray<FRTGridCoord>& Blockers);

	/** Tutte le celle entro Radius passi (Manhattan) da Center, incluso Center. Radius 0 = solo Center. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTGridCoord> CellsInRadius(const FRTGridCoord& Center, int32 Radius);

	/** Celle attraversate dalla linea From->To (From escluso, To incluso). Usata per gli attacchi lineari. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTGridCoord> CellsInLine(const FRTGridCoord& From, const FRTGridCoord& To);

	/**
	 * Cono (a 45°, allineato agli assi) da From nella direzione di Target, lungo Range celle:
	 * a distanza d dall'origine il cono si allarga di d celle per lato. Usato per gli attacchi a ventaglio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTGridCoord> CellsInCone(const FRTGridCoord& From, const FRTGridCoord& Target, int32 Range);

	/**
	 * Celle raggiungibili da From entro MoveRange passi ortogonali (BFS, costo 1/passo, niente diagonali),
	 * senza attraversare celle bloccanti (Blockers) e restando dentro la griglia. Include From.
	 * A differenza di IsWithinRange (Manhattan), rispetta gli ostacoli: una cella "vicina" ma dietro un
	 * muro non è raggiungibile. Ordine celle stabile (deterministico). Base del path finding (FR-PATH-01).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTGridCoord> ReachableCells(const FRTGridCoord& From, int32 MoveRange,
		const TArray<FRTGridCoord>& Blockers, int32 Width, int32 Height);

	/**
	 * Percorso ortogonale minimo da From a To (BFS, costo uniforme), aggirando i Blockers e restando
	 * dentro la griglia. Ritorna la sequenza di celle da From a To inclusi; vuoto se To è irraggiungibile,
	 * bloccato o fuori griglia. Deterministico. Usato per la preview del movimento (FR-PATH-05).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static TArray<FRTGridCoord> FindPath(const FRTGridCoord& From, const FRTGridCoord& To,
		const TArray<FRTGridCoord>& Blockers, int32 Width, int32 Height);

	// --- Pathfinding pesato (PF.3): costo per cella dato da CellCost (assente = 1; RT_BLOCKED_COST = impassabile).
	//     Il costo di un percorso e' la somma dei costi di ENTRATA delle celle (From costa 0).
	//     Non UFUNCTION: la TMap con chiave USTRUCT non e' esposta a Blueprint; uso interno + test.

	/**
	 * Celle raggiungibili da From con costo accumulato <= CostBudget (Dijkstra), aggirando le celle
	 * impassabili e restando dentro la griglia. Include From. Ordine celle stabile (FR-PATH-07).
	 */
	static TArray<FRTGridCoord> ReachableCellsByCost(const FRTGridCoord& From, int32 CostBudget,
		const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height);

	/**
	 * Percorso a COSTO minimo da From a To (Dijkstra): fra due rotte preferisce la più economica,
	 * anche se più lunga in celle. Ritorna From..To inclusi; vuoto se irraggiungibile/bloccato/fuori
	 * griglia. Deterministico (FR-PATH-06). Euristica: non necessaria su griglia MVP (Dijkstra).
	 */
	static TArray<FRTGridCoord> FindPathByCost(const FRTGridCoord& From, const FRTGridCoord& To,
		const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height);

	/**
	 * Costo totale di un percorso (somma dei costi di ENTRATA, From escluso), o -1 se NON valido.
	 * Uno step e' valido se: celle ortogonalmente adiacenti (costo = costo della cella), OPPURE esiste
	 * un arco From->To in Edges (costo = costo dell'arco). Nessuna delle due -> invalido. Cella
	 * impassabile -> invalido. Path vuoto o di 1 cella = costo 0 (fermo). Edges vuoti = validazione 2D.
	 */
	static int32 PathCost(const TArray<FRTGridCoord>& Path, const TMap<FRTGridCoord, int32>& CellCost,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());

	/**
	 * Percorso composito: concatena gli auto-route (FindPathByCost) da Start attraverso i Waypoints in
	 * ordine. Ritorna Start..ultimoWaypoint; vuoto se un tratto e' irraggiungibile. Base della path
	 * composita a waypoint (FR-PATH-09). Nessun waypoint = solo [Start].
	 */
	static TArray<FRTGridCoord> BuildCompositePath(const FRTGridCoord& Start,
		const TArray<FRTGridCoord>& Waypoints, const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());

	// --- Pathfinding a GRAFO (PF.4): oltre ai 4 vicini ortogonali (stesso layer), attraversa gli archi
	//     espliciti (Edges: scale/portali/cross-layer). Generalizza le versioni ByCost al multilivello.

	/** Come ReachableCellsByCost ma includendo gli archi di traversata (multilivello). */
	static TArray<FRTGridCoord> ReachableCellsByGraph(const FRTGridCoord& From, int32 CostBudget,
		const TMap<FRTGridCoord, int32>& CellCost, const TArray<FRTTraversalEdge>& Edges, int32 Width, int32 Height);

	/** Come FindPathByCost ma includendo gli archi di traversata (percorso a costo minimo su grafo). */
	static TArray<FRTGridCoord> FindPathByGraph(const FRTGridCoord& From, const FRTGridCoord& To,
		const TMap<FRTGridCoord, int32>& CellCost, const TArray<FRTTraversalEdge>& Edges, int32 Width, int32 Height);
};
