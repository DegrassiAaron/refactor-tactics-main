#include "Camera/RTCameraPawn.h"
#include "RefactorTactics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h" // squadra del giocatore da inquadrare all'avvio
#include "Unit/RTUnit.h"
#include "EngineUtils.h"               // TActorIterator
#include "TimerManager.h"
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
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
}

bool ARTCameraPawn::FrameOwnTeam()
{
	const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(GetWorld());
	if (!HexMap)
	{
		return false;
	}

	// La squadra da inquadrare e' quella del giocatore; senza controller si assume la 0 (demo).
	int32 TeamId = 0;
	if (const ARTPlayerController* PC = Cast<ARTPlayerController>(GetController()))
	{
		TeamId = PC->PlayerTeamId;
	}

	TArray<FRTCellId> Cells;
	for (TActorIterator<ARTUnit> It(GetWorld()); It; ++It)
	{
		if (It->TeamId == TeamId && It->IsAlive())
		{
			Cells.Add(It->Cell);
		}
	}
	if (Cells.Num() == 0)
	{
		return false; // unita' non ancora nel mondo: il chiamante riprova
	}

	FVector Origin; float HexSize; float LayerHeight;
	HexMap->GetHexContext(Origin, HexSize, LayerHeight);
	FocusOn(URTHexLibrary::CellsCentroidWorld(Cells, Origin, HexSize, LayerHeight));
	if (SpringArm)
	{
		SpringArm->TargetArmLength = FMath::Clamp(MatchStartArmLength, MinArmLength, MaxArmLength);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Camera sulla squadra %d (%d unita', arm=%.0f)"),
		TeamId, Cells.Num(), SpringArm ? SpringArm->TargetArmLength : -1.f);
	return true;
}

void ARTCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	ApplyViewSettings(); // applica i valori correnti (anche se modificati in editor)

	// Si parte guardando le proprie unita', non il centro della mappa. Le unita' possono non esistere ancora:
	// l'ordine di BeginPlay fra actor non e' garantito, quindi si riprova al tick successivo — una volta sola —
	// e solo se anche quello fallisce si ripiega sull'inquadratura d'insieme. E' presentazione: il ritardo di un
	// frame non si vede e non tocca la simulazione.
	if (!FrameOwnTeam())
	{
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (!FrameOwnTeam())
			{
				RecenterView();
			}
		}));
	}

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
	// Sul piano, ma RELATIVO a dove la camera guarda: `Axis.Y` e' «avanti sullo schermo», `Axis.X` e' «a
	// destra sullo schermo». Con yaw 0 il risultato e' identico a prima (nessuna inquadratura esistente
	// cambia comportamento); appena si ruota, e' cio' che impedisce a W di spostare la vista di lato.
	//
	// L'inclinazione non entra: si scorre sul piano della mappa, non lungo la direzione di sguardo, altrimenti
	// scorrere avvicinerebbe anche il terreno.
	const FVector Forward = FRotator(0.f, CameraYaw, 0.f).RotateVector(FVector::ForwardVector);
	const FVector Right   = FRotator(0.f, CameraYaw, 0.f).RotateVector(FVector::RightVector);
	const FVector Delta   = (Forward * Axis.Y + Right * Axis.X) * PanSpeed;
	AddActorWorldOffset(Delta);
}

void ARTCameraPawn::AddYaw(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}
	// Normalizzata subito: ruotando a lungo nella stessa direzione il valore crescerebbe senza fine, e il
	// campo in editor diventerebbe illeggibile pur descrivendo la stessa vista.
	CameraYaw = FRotator::ClampAxis(CameraYaw + FMath::Sign(AxisValue) * YawStep);

	// SOLO la rotazione, non `ApplyViewSettings()`: quella riporta anche il braccio a `DefaultArmLength`, e
	// ruotare avrebbe annullato lo zoom a ogni pressione. Sono due cose diverse che vivevano nella stessa
	// funzione perche' finora nessuno cambiava l'inquadratura senza volerla anche ripristinare.
	if (SpringArm)
	{
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	}
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
	// Anche la ROTAZIONE torna a zero. `Home` e' il tasto del «riportami a un'inquadratura che conosco»: se
	// riportasse la posizione ma lasciasse la vista girata, chi si e' perso ruotando resterebbe perso.
	CameraYaw = 0.f;
	if (SpringArm)
	{
		SpringArm->TargetArmLength = DefaultArmLength;
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));
	}
}
