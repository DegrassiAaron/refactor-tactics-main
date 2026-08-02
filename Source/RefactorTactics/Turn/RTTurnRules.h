#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTTurnRules.generated.h"

/** Fasi di un turno (risoluzione a fasi con priorita' fissa). */
UENUM(BlueprintType)
enum class ERTMatchPhase : uint8
{
	Planning,
	Prep,
	Dash,
	Blast,
	Move,
	Cleanup,
	MatchEnded
};

UCLASS()
class REFACTORTACTICS_API URTTurnRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Fase successiva nel ciclo Planning -> Prep -> Dash -> Blast -> Move -> Cleanup -> Planning.
	 * MatchEnded e' assorbente (resta MatchEnded).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	static ERTMatchPhase NextPhase(ERTMatchPhase Phase);
};
