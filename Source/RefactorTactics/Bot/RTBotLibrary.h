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
	 * Cella verso cui ritirarsi allontanandosi da Threat (kiting), restando entro MoveRange passi
	 * e dentro la griglia [0,Width) x [0,Height). Ritirata diagonale quando possibile; se From coincide
	 * con Threat, si ritira in una direzione deterministica (+X).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static FRTGridCoord StepAway(const FRTGridCoord& From, const FRTGridCoord& Threat, int32 MoveRange, int32 Width, int32 Height);

	/**
	 * Cella di avvicinamento a Target raggiungibile entro MoveRange passi, dentro la griglia e
	 * NON su una copertura (Blockers): sceglie quella piu' vicina al bersaglio (a parita', la mossa
	 * minima). Evita in un solo turno gli ostacoli; non si sovrappone al bersaglio. Deterministica.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static FRTGridCoord BestApproachCell(const FRTGridCoord& From, const FRTGridCoord& Target, int32 MoveRange,
		const TArray<FRTGridCoord>& Blockers, int32 Width, int32 Height);

	/**
	 * Priorità di un attacco per la scelta del bersaglio del bot.
	 * Un colpo che uccide (Damage >= TargetHealth) ha priorità massima, e tra i kill si preferisce
	 * il bersaglio più debole; tra i non-kill si preferisce il danno maggiore.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static int32 AttackScore(int32 Damage, int32 TargetHealth);
};
