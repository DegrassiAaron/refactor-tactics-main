#include "RTGameMode.h"
#include "Camera/RTCameraPawn.h"
#include "Player/RTPlayerController.h"
#include "Grid/RTGridActor.h"
#include "Unit/RTUnit.h"
#include "Turn/RTTurnManager.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"

ARTGameMode::ARTGameMode()
{
	DefaultPawnClass = ARTCameraPawn::StaticClass();
	PlayerControllerClass = ARTPlayerController::StaticClass();
}

void ARTGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Griglia: usa quella presente o ne crea una all'origine.
	ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	if (!Grid)
	{
		Grid = World->SpawnActor<ARTGridActor>(ARTGridActor::StaticClass(), FTransform::Identity);
	}

	// Luce direzionale (se assente) per rendere visibile la scena anche in un livello vuoto.
	if (!UGameplayStatics::GetActorOfClass(this, ADirectionalLight::StaticClass()))
	{
		if (ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>(
				ADirectionalLight::StaticClass(), FTransform(FRotator(-50.f, -30.f, 0.f))))
		{
			if (ULightComponent* LightComp = Light->GetLightComponent())
			{
				LightComp->SetMobility(EComponentMobility::Movable);
				LightComp->SetIntensity(6.f);
			}
		}
	}

	// Board 2v2 (solo se non ci sono gia' unita' nel livello).
	if (Grid && !UGameplayStatics::GetActorOfClass(this, ARTUnit::StaticClass()))
	{
		const FVector Origin = Grid->GetActorLocation();
		const float CellSize = Grid->CellSize;
		SpawnUnit(0, FRTGridCoord(2, 2), Origin, CellSize);
		SpawnUnit(0, FRTGridCoord(2, 4), Origin, CellSize);
		SpawnUnit(1, FRTGridCoord(7, 5), Origin, CellSize);
		SpawnUnit(1, FRTGridCoord(7, 7), Origin, CellSize);
		UE_LOG(LogRT, Log, TEXT("[RT] Demo barebone 2v2 avviata"));
	}

	// Orchestratore del turno.
	if (!UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()))
	{
		World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass(), FTransform::Identity);
	}
}

ARTUnit* ARTGameMode::SpawnUnit(int32 TeamId, const FRTGridCoord& Cell, const FVector& GridOrigin, float CellSize)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Deferred: imposto TeamId prima di BeginPlay, cosi' il colore-team viene applicato correttamente.
	ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
	if (Unit)
	{
		Unit->TeamId = TeamId;
		Unit->bIsBotControlled = (TeamId == 1); // team 1 giocato dal bot
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->PlaceOnCell(Cell, GridOrigin, CellSize);
	}
	return Unit;
}
