#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "RTBotLibrary.generated.h"

/** Decisioni di base del bot (logica pura, indipendente dagli Actor). */
UCLASS()
class REFACTORTACTICS_API URTBotLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Cella verso cui muoversi per avvicinarsi a Target restando entro MoveRange passi.
	 * Avvicinamento greedy (prima X poi Y); si ferma a distanza 1 dal bersaglio
	 * (non si sovrappone) e non si muove se e' gia' adiacente o sulla stessa cella.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static FRTGridCoord StepToward(const FRTGridCoord& From, const FRTGridCoord& Target, int32 MoveRange);

	/**
	 * Cella di avvicinamento a Target raggiungibile entro il budget di COSTO (CellCost: pathfinding
	 * pesato, hazard/impassabili esclusi), dentro la griglia: sceglie quella piu' vicina al bersaglio
	 * (a parita', la mossa minima). Aggira ostacoli e terreno costoso; non si sovrappone al bersaglio.
	 */
	static FRTGridCoord BestApproachCell(const FRTGridCoord& From, const FRTGridCoord& Target, int32 MoveRange,
		const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height);

	/**
	 * Cella di fuga per il kiting: fra quelle raggiungibili entro il budget di costo (CellCost), dentro
	 * la griglia, sceglie quella che MASSIMIZZA la distanza dalla minaccia (a parita', la mossa minima).
	 * Aggira bordi, ostacoli e terreno pericoloso. Deterministica.
	 */
	static FRTGridCoord BestKiteCell(const FRTGridCoord& From, const FRTGridCoord& Threat, int32 MoveRange,
		const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height);

	/**
	 * Priorità di un attacco per la scelta del bersaglio del bot.
	 * Un colpo che uccide (Damage >= TargetHealth) ha priorità massima, e tra i kill si preferisce
	 * il bersaglio più debole; tra i non-kill si preferisce il danno maggiore.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static int32 AttackScore(int32 Damage, int32 TargetHealth);
};
