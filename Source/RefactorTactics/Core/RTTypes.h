#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTTypes.generated.h"

/**
 * Arco di traversata esplicito tra due celle (anche non adiacenti / su layer diversi):
 * scale, rampe, portali, ascensori, salti. Direzionale (per il bidirezionale servono due archi).
 * Rende la mappa un grafo (PF.4).
 */
USTRUCT(BlueprintType)
struct FRTTraversalEdge
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
	FRTCellId From;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
	FRTCellId To;

	/** Costo di attraversamento dell'arco (>= 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
	int32 Cost = 1;

	FRTTraversalEdge() = default;
	FRTTraversalEdge(const FRTCellId& InFrom, const FRTCellId& InTo, int32 InCost)
		: From(InFrom), To(InTo), Cost(InCost) {}
};

/**
 * [D-594] (1): il budget di Action Point di partenza di ogni unita', 6. Da E38 CP 38.11 ([D-613] (1), #4112) il budget e'
 * un numero dell'unita' per il turno, e questo e' il suo default (lettura D-613 (5): «il 6 resta il valore di partenza e
 * diventa il default»). Sta qui perche' lo leggono `ARTUnit`, `FRTHexSimUnit` e il validatore, senza includersi a vicenda.
 */
namespace RTActionPoints
{
	constexpr int32 DefaultBudget = 6;
}
