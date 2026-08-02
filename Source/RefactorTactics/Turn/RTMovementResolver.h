#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "RTMovementResolver.generated.h"

/**
 * Richiesta di movimento di un'unita' nel turno: cella attuale -> cella desiderata.
 * To == From significa che l'unita' resta ferma.
 */
USTRUCT(BlueprintType)
struct FRTMoveRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Turn")
	FRTGridCoord From;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Turn")
	FRTGridCoord To;

	FRTMoveRequest() = default;
	FRTMoveRequest(const FRTGridCoord& InFrom, const FRTGridCoord& InTo) : From(InFrom), To(InTo) {}
};

/**
 * Risoluzione del movimento simultaneo. Regole:
 * - destinazione contesa (2+ unita' verso la stessa cella) -> tutte restano ferme;
 * - scambio diretto (A verso la cella di B e B verso quella di A) -> consentito;
 * - cella occupata da un'unita' che resta ferma -> movimento bloccato (l'unita' resta).
 * Il risultato NON dipende dall'ordine delle richieste (funzione pura, punto fisso monotono).
 */
UCLASS()
class REFACTORTACTICS_API URTMovementResolver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Ritorna la cella finale di ogni richiesta, nello stesso ordine dell'input. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	static TArray<FRTGridCoord> ResolveMoves(const TArray<FRTMoveRequest>& Requests);
};
