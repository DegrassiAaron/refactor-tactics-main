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
