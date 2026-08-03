#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTHexPath.generated.h"

/** Esito di una query di pathfinding esagonale. */
UENUM(BlueprintType)
enum class ERTHexPathStatus : uint8
{
	Success,      // percorso trovato
	NoPath,       // nessun percorso (grafo disconnesso / tutto bloccato)
	StartInvalid, // la cella di partenza non e' nella mappa
	GoalInvalid,  // la cella di arrivo non e' nella mappa
	NodeLimit     // superato il limite massimo di nodi (guardia anti-loop)
};

/**
 * Risultato di un pathfinding esagonale: stato, percorso ordinato (start..goal), costo totale intero,
 * numero di nodi espansi (diagnostica). Deterministico: nessuna dipendenza dall'ordine di TSet/TMap.
 */
USTRUCT(BlueprintType)
struct FRTHexPathResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTHexPathStatus Status = ERTHexPathStatus::NoPath;

	/** Celle dal partenza all'arrivo (incluse). Vuoto se Status != Success. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	TArray<FRTCellId> Path;

	/** Costo totale (intero) del percorso. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 TotalCost = 0;

	/** Nodi espansi dall'A* (diagnostica/performance). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 NodesVisited = 0;
};
