#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTPacing.h"
#include "RTPacingLibrary.generated.h"

/** Calcoli puri sulla telemetria di pacing (nessun Actor, nessun file, testabili headless). */
UCLASS()
class REFACTORTACTICS_API URTPacingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Percentile con metodo NEAREST-RANK su un array GIA' ORDINATO in modo crescente: nessuna
	 * interpolazione, quindi il risultato e' sempre un valore realmente osservato e il test si scrive a mano.
	 * Rango 1-based = ceil(Percentile/100 * N). Array vuoto -> 0.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static int32 PercentileNearestRank(const TArray<int32>& SortedValues, int32 Percentile);

	/**
	 * Sommario dei campioni. `CutoffWindowMs` e' la soglia che separa un TAGLIO (timeout con input piu'
	 * recente della soglia) da un'ATTESA A VUOTO: e' un parametro esplicito e non una costante sepolta,
	 * perche' e' una decisione di design ritarabile. Array vuoto -> sommario tutto a zero (fail-closed).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FRTPacingSummary SummarizeSamples(const TArray<FRTPacingSample>& Samples, int32 CutoffWindowMs);

	/** Intestazione del CSV: tredici colonne, nello stesso ordine di CsvRow. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FString CsvHeader();

	/** Una riga CSV: tutti interi con %d, quindi nessuna virgola decimale introdotta dal locale. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FString CsvRow(const FRTPacingSample& Sample);
};
