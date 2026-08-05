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
	SpringArm->TargetArmLength = DefaultArmLength;
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.f, 0.f)); // inclinazione tunabile (era fissa a -55)
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ARTCameraPawn::ApplyViewSettings()
{
	if (!SpringArm)
	{
		return;
	}
	// La distanza iniziale deve stare nello stesso intervallo che lo zoom rispetta: un default fuori range
	// darebbe una partenza che il primo scroll "corregge" di scatto.
	SpringArm->TargetArmLength = FMath::Clamp(DefaultArmLength, MinArmLength, MaxArmLength);
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.f, 0.f));
}

void ARTCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	ApplyViewSettings(); // applica i valori correnti (anche se modificati in editor)
	UE_LOG(LogRT, Log, TEXT("[RT] CameraPawn BeginPlay (arm=%.0f, pitch=%.0f)"),
		SpringArm ? SpringArm->TargetArmLength : -1.f, CameraPitch);
}

#if WITH_EDITOR
void ARTCameraPawn::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyViewSettings(); // tarare distanza/inclinazione dal Details ha effetto subito, anche in PIE
}
#endif

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
