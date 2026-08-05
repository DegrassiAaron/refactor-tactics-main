#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnLog.h"
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
	FRTCellId From;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Turn")
	FRTCellId To;

	FRTMoveRequest() = default;
	FRTMoveRequest(const FRTCellId& InFrom, const FRTCellId& InTo) : From(InFrom), To(InTo) {}
};

/**
 * Risoluzione del movimento simultaneo. Regole:
 * - destinazione contesa (2+ unita' verso la stessa cella) -> tutte restano ferme;
 * - scambio diretto (A verso la cella di B e B verso quella di A) -> consentito;
 * - cella occupata da un'unita' che resta ferma -> movimento bloccato (l'unita' resta).
 * Il risultato NON dipende dall'ordine delle richieste (funzione pura, punto fisso monotono).
 */
/**
 * Esito del movimento di un'unita' lungo un path: cella finale + celle effettivamente attraversate
 * (per il cross-damage del terreno). Entered esclude la cella di partenza.
 */
USTRUCT(BlueprintType)
struct FRTPathResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Turn")
	FRTCellId Final;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Turn")
	TArray<FRTCellId> Entered;

	/** Perche' il movimento e' finito cosi' (default: nessun movimento pianificato). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Turn")
	ERTMoveOutcome Outcome = ERTMoveOutcome::Stayed;
};

UCLASS()
class REFACTORTACTICS_API URTMovementResolver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Ritorna la cella finale di ogni richiesta, nello stesso ordine dell'input. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	static TArray<FRTCellId> ResolveMoves(const TArray<FRTMoveRequest>& Requests);

	/**
	 * Esegue path multi-cella simultanei con algoritmo a MICROSTEP sincroni (§7 spec-terreni):
	 * a ogni microstep tutte le unita' avanzano di 1 cella lungo il proprio path; si risolvono
	 * le collisioni del microstep (destinazione contesa -> contendenti fermi da li'; cella occupata
	 * da un'unita' ferma -> bloccata; scambio diretto -> consentito), si ripete finche' nessuno avanza.
	 * Ogni path e' From..To (From = elemento 0). Il risultato NON dipende dall'ordine delle richieste.
	 */
	static TArray<FRTPathResult> ResolvePaths(const TArray<TArray<FRTCellId>>& Paths);
};
