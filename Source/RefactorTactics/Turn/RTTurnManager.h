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

	/** Secondi rimanenti alla pianificazione (0 se scaduto/assente). Utile per una futura HUD. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	float GetPlanningTimeRemaining() const;

protected:
	virtual void BeginPlay() override;

	void PlanBots();
	void ResolveCombat();
	void ResolveMovement();
	void StartPlanningTimer();
	void OnPlanningTimeout();

	/** Durata della fase di pianificazione; allo scadere scatta il lock-in automatico. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	float PlanningSeconds = 30.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Turn")
	int32 TurnNumber = 1;

	FTimerHandle PlanningTimerHandle;
};
