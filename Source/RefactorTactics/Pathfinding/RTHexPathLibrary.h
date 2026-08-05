#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Pathfinding/RTHexPath.h"
#include "RTHexPathLibrary.generated.h"

class URTHexMapAsset;

/**
 * Pathfinding esagonale AUTOREVOLE sul grafo tattico della mappa (A* deterministico, costi interi). Non usa la
 * NavMesh. Le celle su layer diversi si collegano SOLO tramite transizioni esplicite (archi). Nessuna dipendenza
 * dall'ordine di iterazione di TSet/TMap: il tie-break usa l'ID della cella (ordinamento stabile).
 */
UCLASS()
class REFACTORTACTICS_API URTHexPathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Vicini raggiungibili dalla cella nel grafo tattico: i 6 vicini orizzontali presenti e non bloccati
	 * (costo = MoveCost della destinazione) piu' le transizioni esplicite uscenti (costo = costo dell'arco).
	 * Ordine deterministico (direzioni E..SE, poi archi nell'ordine dell'asset).
	 */
	static TArray<TPair<FRTCellId, int32>> GraphNeighbors(const URTHexMapAsset* Map, const FRTCellId& Cell);

	/**
	 * A* deterministico da Start a Goal sul grafo della mappa. MaxCost > 0 limita il costo (0 = illimitato);
	 * MaxNodes limita i nodi espansi (guardia). Euristica = distanza esagonale (ammissibile per archi locali).
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Hex")
	static FRTHexPathResult FindPath(const URTHexMapAsset* Map, const FRTCellId& Start, const FRTCellId& Goal,
		int32 MaxCost = 0, int32 MaxNodes = 100000);

	/**
	 * Come FindPath, ma tratta le celle in Blocked come non percorribili (ostacoli DINAMICI, es. unita' occupanti:
	 * non appartengono all'asset mappa). Blocked == nullptr equivale a FindPath. Goal bloccato -> NoPath.
	 */
	static FRTHexPathResult FindPathAvoiding(const URTHexMapAsset* Map, const FRTCellId& Start, const FRTCellId& Goal,
		const TSet<FRTCellId>* Blocked, int32 MaxCost = 0, int32 MaxNodes = 100000);
};
