#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

ARTCameraPawn::ARTCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = 1800.f;
	SpringArm->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f)); // Pitch giu' di 55 gradi.
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ARTCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogRT, Log, TEXT("[RT] CameraPawn BeginPlay (arm=%.0f)"), SpringArm ? SpringArm->TargetArmLength : -1.f);
}

void ARTCameraPawn::AddPlanarMovement(const FVector2D& Axis)
{
	// Sul piano mondo, indipendente dall'inclinazione della camera.
	const FVector Delta(Axis.Y * PanSpeed, Axis.X * PanSpeed, 0.f);
	AddActorWorldOffset(Delta);
}

void ARTCameraPawn::AddZoom(float AxisValue)
{
	if (!SpringArm)
	{
		return;
	}
	SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength + AxisValue * ZoomStep, MinArmLength, MaxArmLength);
}
