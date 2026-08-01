#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * Pawn camera tattica top-down/tre-quarti: braccio inclinato con zoom e pan sul piano.
 * Non usa fisica: si sposta con AddActorWorldOffset.
 */
UCLASS()
class REFACTORTACTICS_API ARTCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ARTCameraPawn();

	/** Sposta la camera sul piano mondo XY (input.X = destra, input.Y = avanti). */
	void AddPlanarMovement(const FVector2D& Axis);

	/** Zoom variando la lunghezza del braccio (valore positivo = allontana). */
	void AddZoom(float AxisValue);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float PanSpeed = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float ZoomStep = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float MinArmLength = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float MaxArmLength = 4000.f;
};
