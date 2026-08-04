#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnLog.h"
#include "RTTurnLogLibrary.generated.h"

/**
 * Ordinamento e hash del TurnLog per l'osservabilita'/replay. Pura, deterministica, SOLO interi
 * (invariante #4: niente float). L'hash e' usato per la verifica di replay ("replay divergence = 0"),
 * mai per la logica di gioco.
 */
UCLASS()
class REFACTORTACTICS_API URTTurnLogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Ordine TOTALE deterministico fra due voci: Phase -> Category -> SrcCell(X,Y,Layer) ->
	 * TgtCell(X,Y,Layer) -> Outcome -> Amount. Vero se A precede B. Con un ordine totale, riordinare
	 * un insieme di voci da' sempre la stessa sequenza, indipendentemente dall'ordine d'inserimento.
	 */
	static bool EntryLess(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B);

	/** Ordina il TurnLog in place con EntryLess (ordine totale deterministico). */
	static void SortTurnLog(TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Hash intero (FNV-1a sui campi interi) del TurnLog, PERMUTAZIONE-INVARIANTE (ordina con EntryLess
	 * prima di mescolare). Deterministico, solo interi. Uso: verifica di replay, mai logica di gioco.
	 */
	static uint32 HashTurnLog(const TArray<FRTTurnLogEntry>& Entries);
};
