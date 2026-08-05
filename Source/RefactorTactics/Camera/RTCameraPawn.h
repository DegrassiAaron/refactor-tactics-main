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

	/** Riporta la camera al centro della griglia e ripristina lo zoom di default (DefaultArmLength). */
	void RecenterView();

	/**
	 * Centra la camera su un punto del mondo (tipicamente un'unita'), mantenendo la quota e lo zoom correnti:
	 * lo zoom orbita attorno alla posizione di questo pawn, quindi senza spostarlo ci si avvicina al centro
	 * della mappa invece che al personaggio.
	 */
	void FocusOn(const FVector& WorldLocation);

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

	/** Distanza (arm length) iniziale all'avvio: piu' piccola = piu' vicino alla mappa. Tra Min e Max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float DefaultArmLength = 800.f;

	/** Zoom minimo (piu' piccolo = primo piano piu' ravvicinato sui personaggi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float MinArmLength = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera")
	float MaxArmLength = 4000.f;

	/**
	 * Inclinazione del braccio in gradi (negativa = guarda verso il basso). -55 e' una vista a volo d'uccello;
	 * valori meno ripidi (-40, -35) danno un'inquadratura piu' laterale, in cui i personaggi si leggono meglio.
	 * Era una costante nel costruttore: esposta qui per poterla tarare in editor senza ricompilare.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Camera",
		meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float CameraPitch = -40.f;

	/** Applica distanza (clampata tra Min e Max) e inclinazione correnti al braccio. */
	void ApplyViewSettings();

#if WITH_EDITOR
	/** Rende immediate le modifiche di distanza/inclinazione fatte nel Details, anche durante il PIE. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
