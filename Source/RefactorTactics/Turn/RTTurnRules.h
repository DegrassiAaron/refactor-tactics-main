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

/** Esito della partita in base alle unita' vive per squadra. */
UENUM(BlueprintType)
enum class ERTMatchOutcome : uint8
{
	InProgress,
	Team0Wins,
	Team1Wins,
	Draw
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

	/**
	 * Esito dato il numero di unita' vive per squadra.
	 * Una squadra senza unita' vive perde; se entrambe sono a zero e' pareggio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Turn")
	static ERTMatchOutcome EvaluateOutcome(int32 Team0Alive, int32 Team1Alive);
};
