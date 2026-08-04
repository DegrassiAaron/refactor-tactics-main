#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Grid/RTGridActor.h"
#include "Kismet/GameplayStatics.h"

ARTCameraPawn::ARTCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = DefaultArmLength; // partenza piu' vicina (tunabile via DefaultArmLength)
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
	if (SpringArm)
	{
		SpringArm->TargetArmLength = DefaultArmLength; // applica il default (anche se modificato in editor)
	}
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

void ARTCameraPawn::RecenterView()
{
	// Centra sul centro della griglia (se presente) e ripristina lo zoom di default.
	if (const ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass())))
	{
		const FVector Center = Grid->GetActorLocation() +
			FVector(Grid->Width * Grid->CellSize * 0.5f, Grid->Height * Grid->CellSize * 0.5f, 0.f);
		SetActorLocation(Center);
	}
	if (SpringArm)
	{
		SpringArm->TargetArmLength = DefaultArmLength;
	}
}
