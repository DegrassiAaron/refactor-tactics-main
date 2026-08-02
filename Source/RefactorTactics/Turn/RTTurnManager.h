#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Turn/RTTurnRules.h"
#include "RTTurnManager.generated.h"

/**
 * Orchestratore del turno: tiene fase e numero di turno e, al lock-in, avanza le fasi
 * applicando il movimento simultaneo (URTMovementResolver) nella fase Move.
 */
UCLASS()
class REFACTORTACTICS_API ARTTurnManager : public AActor
{
	GENERATED_BODY()

public:
	ARTTurnManager();

	/** Chiude la pianificazione e risolve il turno; il movimento si applica nella fase Move. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Turn")
	void LockInAndResolve();

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	ERTMatchPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	int32 GetTurnNumber() const { return TurnNumber; }

protected:
	void ResolveMovement();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	int32 TurnNumber = 1;
};
