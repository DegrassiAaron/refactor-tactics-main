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
	 * Priorità di un attacco per la scelta del bersaglio del bot.
	 * Un colpo che uccide (Damage >= TargetHealth) ha priorità massima, e tra i kill si preferisce
	 * il bersaglio più debole; tra i non-kill si preferisce il danno maggiore.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static int32 AttackScore(int32 Damage, int32 TargetHealth);
};
