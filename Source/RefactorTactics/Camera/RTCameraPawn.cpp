#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
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

void ARTCameraPawn::FocusOn(const FVector& WorldLocation)
{
	// Solo X/Y: la quota del pawn resta la sua (il braccio ci pensa da se'). Lo zoom corrente non si tocca,
	// cosi' il comando inquadra senza sorprendere chi si era gia' regolato la distanza.
	SetActorLocation(FVector(WorldLocation.X, WorldLocation.Y, GetActorLocation().Z));
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
	// Centra sulla mappa ESAGONALE del livello (se presente) e ripristina lo zoom di default. La mappa non e'
	// per forza centrata sull'origine: il centro viene dal bounding box delle sue celle.
	if (const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld()))
	{
		FVector Origin; float HexSize; float LayerHeight;
		if (const URTHexMapAsset* Map = HexMap->GetHexContext(Origin, HexSize, LayerHeight))
		{
			SetActorLocation(URTHexLibrary::AxialToWorld(Map->GetCenterCell(), Origin, HexSize, LayerHeight));
		}
		else
		{
			SetActorLocation(Origin);
		}
	}
	if (SpringArm)
	{
		SpringArm->TargetArmLength = DefaultArmLength;
	}
}
