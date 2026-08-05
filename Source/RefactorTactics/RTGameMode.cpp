#include "RTGameMode.h"
#include "Camera/RTCameraPawn.h"
#include "Player/RTPlayerController.h"
#include "UI/RTHUD.h"
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
	HUDClass = ARTHUD::StaticClass();
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

	// Orchestratore del turno: spawnato PRIMA delle unita' cosi' esiste gia' quando queste fanno BeginPlay
	// (i BP_Unit possono agganciarsi ai suoi delegate di playback senza attese). Sicuro: il TurnManager non
	// tocca le unita' al proprio BeginPlay (fa solo StartPlanningTimer); le raccoglie a ogni turno.
	if (!UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()))
	{
		World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass(), FTransform::Identity);
	}

	// Board 2v2 (solo se non ci sono gia' unita' nel livello).
	if (Grid && !UGameplayStatics::GetActorOfClass(this, ARTUnit::StaticClass()))
	{
		const FVector Origin = Grid->GetActorLocation();
		const float CellSize = Grid->CellSize;
		// Ogni squadra: un Ranger (fragile, lunga gittata) e un Guardian (tanky, corta gittata).
		SpawnUnit(0, FRTCellId(2, 2), /*bGuardian=*/ false, Origin, CellSize);
		SpawnUnit(0, FRTCellId(2, 4), /*bGuardian=*/ true,  Origin, CellSize);
		SpawnUnit(1, FRTCellId(7, 7), /*bGuardian=*/ false, Origin, CellSize);
		SpawnUnit(1, FRTCellId(7, 5), /*bGuardian=*/ true,  Origin, CellSize);
		UE_LOG(LogRT, Log, TEXT("[RT] Board 2v2 (Ranger + Guardian per squadra) avviata"));
	}
}

ARTUnit* ARTGameMode::SpawnUnit(int32 TeamId, const FRTCellId& Cell, bool bGuardian, const FVector& GridOrigin, float CellSize)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Classe per archetipo: se assegnata (BP_Unit con skeletal) usala, altrimenti fallback al cilindro C++.
	UClass* RangerCls = RangerUnitClass ? RangerUnitClass.Get() : ARTUnit::StaticClass();
	UClass* GuardianCls = GuardianUnitClass ? GuardianUnitClass.Get() : ARTUnit::StaticClass();
	UClass* UnitClass = bGuardian ? GuardianCls : RangerCls;

	// Deferred: imposto team e archetipo prima di BeginPlay, cosi' colore e statistiche sono corretti.
	ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(UnitClass, FTransform::Identity);
	if (Unit)
	{
		Unit->TeamId = TeamId;
		Unit->bIsBotControlled = (TeamId == 1); // team 1 giocato dal bot
		Unit->ConfigureAsArchetype(bGuardian ? ERTArchetype::Guardian : ERTArchetype::Ranger);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->PlaceOnCell(Cell, GridOrigin, CellSize, /*LayerHeight=*/ 0.f);
	}
	return Unit;
}
