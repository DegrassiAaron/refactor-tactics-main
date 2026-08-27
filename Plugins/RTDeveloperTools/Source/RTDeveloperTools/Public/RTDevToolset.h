#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "ToolsetRegistry/ToolsetDefinition.h"

#include "RTDevToolset.generated.h"

class URTHexMapAsset;

// ---------------------------------------------------------------------------------------------------------
// Tipi di RITORNO dei tool.
//
// Sono USTRUCT e non JSON costruito a mano: lo schema di output lo genera la reflection, quindi non esiste
// un secondo posto in cui il formato possa divergere dal codice. La regola per aggiungere un campo e' una
// sola — deve esistere un dato che lo produce. Un campo che il MapState non possiede si mente da solo.
// ---------------------------------------------------------------------------------------------------------

/** Un vicino raggiungibile nel grafo tattico, con il costo dell'arco che ci porta. */
USTRUCT(BlueprintType)
struct FRTDevNeighbor
{
	GENERATED_BODY()

	/** Axial hex coordinate of the neighbour cell (X = q, Y = r) plus its layer. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FRTCellId Cell;

	/** Integer cost of traversing the arc that reaches this neighbour. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 Cost = 0;
};

/** Stato del progetto e della mappa tattica caricata nell'Editor. */
USTRUCT(BlueprintType)
struct FRTDevProjectStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FString ProjectName;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FString EngineVersion;

	/** Name of the level currently open in the editor. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FString LevelName;

	/** True when the editor world contains an ARTHexMapActor with a hex map asset assigned. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	bool bHasTacticalMap = false;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FString MapAssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 NumCells = 0;

	/** Map graph revision. Rises on every structural edit; caches and paths are only comparable at equal revision. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 GraphRevision = 0;
};

/** Fotografia della mappa tattica attiva. */
USTRUCT(BlueprintType)
struct FRTDevMapReport
{
	GENERATED_BODY()

	/** False when no hex map is loaded in the editor world; every other field is then meaningless. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	bool bLoaded = false;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FString LevelName;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 NumCells = 0;

	/** Explicit transitions (stairs, ramps, bridges, tunnels). Cells on different layers connect ONLY through these. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 NumTransitions = 0;

	/** Layers that actually contain cells. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	TArray<int32> Layers;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 GraphRevision = 0;

	/** Serialized asset format version. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 FormatVersion = 0;

	/** Deterministic content hash of the map asset. Two identical maps hash identically. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int64 ContentHash = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FRTCellId CenterCell;
};

/** Stato reale di una cella del grafo tattico. */
USTRUCT(BlueprintType)
struct FRTDevCellReport
{
	GENERATED_BODY()

	/** The cell that was queried, echoed back. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FRTCellId Cell;

	/** True when the cell exists in the active map. When false every field below is a default, not a measurement. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	bool bExists = false;

	/** Vertical offset used for rendering. Logic uses Layer plus explicit transitions, not this. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 Height = 0;

	/** Surface kind: Floor, ShallowWater, Rough, Fire, Conductive, Ice, Void, Smoke, HighGround. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FString Surface;

	/** Base terrain movement cost (integer: the pathfinder never uses floats). */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 MoveCost = 0;

	/** Extra traversal cost caused by geometry intruding into the cell. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 OccupancySurcharge = 0;

	/** MoveCost + OccupancySurcharge: the cost the pathfinder actually pays to enter this cell. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 TotalMoveCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	bool bBlocksMovement = false;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	bool bBlocksLineOfSight = false;

	/** Number of edges of this cell carrying cover. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 NumCovers = 0;

	/** Number of edges of this cell carrying a door. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 NumDoors = 0;

	/** Map graph revision at query time. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 GraphRevision = 0;

	/**
	 * Cells actually reachable from here in the tactical graph, with their arc cost: the six planar
	 * neighbours that exist and are not blocked, plus outgoing explicit transitions. Deterministic order.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	TArray<FRTDevNeighbor> Neighbors;
};

/** Esito di una query di pathfinding sul grafo tattico. */
USTRUCT(BlueprintType)
struct FRTDevPathReport
{
	GENERATED_BODY()

	/** True only when Status is Success. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	bool bSuccess = false;

	/** Success, NoPath, StartInvalid, GoalInvalid, or NodeLimit. Reported verbatim from the pathfinder. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	FString Status;

	/** Ordered cells from start to goal, both included. Empty unless Status is Success. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	TArray<FRTCellId> Path;

	/** Total integer cost of the path. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 TotalCost = 0;

	/** Nodes expanded by A*. Diagnostic: it measures the search, not the path. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 NodesVisited = 0;

	/** Map graph revision at query time. The same query at the same revision must return the same ordered path. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 GraphRevision = 0;

	/**
	 * Wall-clock time of the C++ pathfinder alone, in milliseconds. It excludes MCP transport and JSON
	 * serialization: comparing it against the project's path-query target is only meaningful this way.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	double PathfinderMilliseconds = 0.0;
};

/** Esito della validazione della mappa attiva. */
USTRUCT(BlueprintType)
struct FRTDevValidationReport
{
	GENERATED_BODY()

	/** False when no hex map is loaded; bValid is then meaningless rather than true. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	bool bLoaded = false;

	/** True when the map validator reported no issues. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	bool bValid = false;

	/** One entry per problem found by the authoritative map validator. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	TArray<FString> Issues;

	/** Cells the map actor last computed as unreachable. Empty if the actor never ran the analysis. */
	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	TArray<FRTCellId> UnreachableCells;

	UPROPERTY(BlueprintReadOnly, Category = "RTDevTools")
	int32 GraphRevision = 0;
};

// ---------------------------------------------------------------------------------------------------------

/**
 * Toolset MCP di RefactorTactics: ispezione EDITOR-ONLY della mappa tattica e del pathfinding.
 *
 * Ogni tool e' una FACADE. Non c'e' una riga di gameplay qui: la mappa la risponde `URTHexMapAsset`, il
 * percorso lo calcola `URTHexPathLibrary` — lo stesso A* che gioca la partita — e la validazione la fa
 * `URTHexMapAsset::ValidateMap`. Se un dato non esiste di la', questo toolset non lo espone.
 *
 * ⚠️ **Confine, e non e' una formalita'**: qui si ISPEZIONA. Nessun tool muta lo stato di gioco, nessuno
 * legge intenti di squadra, nessuno esegue Python, console command o shell. Un tool che scrivesse
 * romperebbe l'unica ragione per cui questo bridge puo' esistere accanto a un simulatore deterministico.
 *
 * Il canale d'errore e' `UKismetSystemLibrary::RaiseScriptError`, come nei toolset dell'engine: chi chiama
 * riceve un errore esplicito, non una struct vuota che sembra una misura. Fa eccezione il pathfinding, che
 * ha gia' il suo vocabolario di esiti (`ERTHexPathStatus`) e lo riporta invece di duplicarlo.
 *
 * I doc-comment sono in inglese di proposito: NON sono commenti, sono la descrizione che il registry
 * consegna al client MCP. I commenti implementativi restano in italiano come nel resto del repository.
 */
UCLASS(BlueprintType)
class URTDevToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:

	/**
	 * Reports the RefactorTactics project state: project name, engine version, the level currently open in
	 * the editor, and whether a tactical hex map is loaded (with its asset path, cell count and graph
	 * revision). Call this first to check the bridge is talking to a live editor with a map.
	 */
	UFUNCTION(meta = (AICallable), Category = "RTDevTools")
	static FRTDevProjectStatus ProjectStatus();

	/**
	 * Describes the tactical hex map currently loaded in the editor world: asset path, cell and transition
	 * counts, populated layers, graph revision, serialized format version, deterministic content hash and
	 * centre cell. Raises an error if no hex map actor is present.
	 */
	UFUNCTION(meta = (AICallable), Category = "RTDevTools")
	static FRTDevMapReport GetCurrentMap();

	/**
	 * Inspects one cell of the active tactical map and returns its real authored state plus the cells
	 * reachable from it in the movement graph.
	 *
	 * Coordinates are AXIAL hex coordinates, not cartesian: X is q, Y is r, and the third cube coordinate
	 * is derived as -X-Y. Layer separates stacked floors; cells on different layers are never adjacent, they
	 * connect only through explicit transitions, which appear in the neighbour list when present.
	 *
	 * Raises an error if the cell does not exist in the map, rather than returning an empty report.
	 *
	 * @param X      Axial q coordinate.
	 * @param Y      Axial r coordinate.
	 * @param Layer  Floor index. Defaults to 0.
	 */
	UFUNCTION(meta = (AICallable), Category = "RTDevTools")
	static FRTDevCellReport DumpCell(int32 X, int32 Y, int32 Layer = 0);

	/**
	 * Runs the authoritative RefactorTactics A* on the active tactical map and returns the ordered path,
	 * its integer total cost, the number of expanded nodes and the time spent inside the pathfinder.
	 *
	 * This is the same deterministic pathfinder the game uses: integer costs, stable tie-break, no
	 * dependency on hash iteration order. The same query at the same graph revision returns the same
	 * ordered path. Coordinates are axial (X = q, Y = r) with an explicit layer.
	 *
	 * An unreachable or non-existent endpoint is NOT an error: it comes back as a Status of NoPath,
	 * StartInvalid or GoalInvalid with bSuccess false.
	 *
	 * @param Start     Starting cell.
	 * @param Goal      Destination cell.
	 * @param MaxCost   Stop searching above this total cost. 0 means unlimited.
	 * @param MaxNodes  Safety limit on expanded nodes; exceeding it returns Status NodeLimit.
	 */
	UFUNCTION(meta = (AICallable), Category = "RTDevTools")
	static FRTDevPathReport FindPath(FRTCellId Start, FRTCellId Goal, int32 MaxCost = 0, int32 MaxNodes = 100000);

	/**
	 * Validates the active tactical map with the project's own map validator and returns the list of
	 * problems found, plus any cells the map actor computed as unreachable. An empty issue list means the
	 * validator found nothing, not that validation was skipped.
	 */
	UFUNCTION(meta = (AICallable), Category = "RTDevTools")
	static FRTDevValidationReport ValidateTacticalMap();

	// -----------------------------------------------------------------------------------------------------
	// Varianti che RICEVONO la mappa invece di risolverla dal world dell'Editor.
	//
	// Non sono `UFUNCTION`: il registry non le vede, quindi non sono un secondo modo di chiamare i tool ne'
	// una superficie MCP in piu'. Esistono perche' il calcolo di un report sia verificabile su una mappa
	// costruita in memoria, senza dipendere da quale livello e' aperto — e quel calcolo e' l'unica parte di
	// questi tool che possa sbagliare in silenzio. Ogni tool qui sopra e' «risolvi la mappa» + una di queste.
	//
	// ⚠️ Sollevano l'errore di script quanto i tool, quindi un test che le chiama deve costruirsi uno stack
	// frame (`UE::ToolsetRegistry::FToolCallExceptionHandler::CaptureErrorsIn`) o non vedra' nulla.
	// -----------------------------------------------------------------------------------------------------

	/** `GetCurrentMap` su una mappa data. `LevelName` e' informativo e non influenza nulla. */
	static FRTDevMapReport DescribeMap(const URTHexMapAsset* Map, const FString& LevelName);

	/** `DumpCell` su una mappa data. */
	static FRTDevCellReport DumpCellOnMap(const URTHexMapAsset* Map, const FRTCellId& Cell);

	/** `FindPath` su una mappa data. */
	static FRTDevPathReport FindPathOnMap(const URTHexMapAsset* Map, const FRTCellId& Start,
		const FRTCellId& Goal, int32 MaxCost = 0, int32 MaxNodes = 100000);

	/** `ValidateTacticalMap` su una mappa data; le celle irraggiungibili le calcola l'actor, non l'asset. */
	static FRTDevValidationReport ValidateMapAsset(const URTHexMapAsset* Map,
		const TArray<FRTCellId>& UnreachableCells);
};
