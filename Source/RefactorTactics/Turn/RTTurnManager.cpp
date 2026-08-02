#include "Turn/RTTurnManager.h"
#include "Turn/RTMovementResolver.h"
#include "Grid/RTGridActor.h"
#include "Grid/RTGridLibrary.h"
#include "Unit/RTUnit.h"
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARTTurnManager::ARTTurnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARTTurnManager::BeginPlay()
{
	Super::BeginPlay();
	StartPlanningTimer();
}

void ARTTurnManager::StartPlanningTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(PlanningTimerHandle);
	if (PlanningSeconds > 0.f)
	{
		World->GetTimerManager().SetTimer(PlanningTimerHandle, this, &ARTTurnManager::OnPlanningTimeout, PlanningSeconds, false);
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Pianificazione turno %d (%.0fs)"), TurnNumber, PlanningSeconds);
}

void ARTTurnManager::OnPlanningTimeout()
{
	UE_LOG(LogRT, Log, TEXT("[RT] Timer scaduto -> lock-in automatico"));
	LockInAndResolve();
}

float ARTTurnManager::GetPlanningTimeRemaining() const
{
	if (const UWorld* World = GetWorld())
	{
		const float Remaining = World->GetTimerManager().GetTimerRemaining(PlanningTimerHandle);
		return Remaining > 0.f ? Remaining : 0.f;
	}
	return 0.f;
}

void ARTTurnManager::LockInAndResolve()
{
	if (Phase != ERTMatchPhase::Planning)
	{
		return;
	}

	// Chiude la pianificazione: ferma il timer (utile anche per il lock-in manuale).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlanningTimerHandle);
	}

	// Avanza le fasi fino a tornare a Planning; il movimento si applica nella fase Move.
	do
	{
		Phase = URTTurnRules::NextPhase(Phase);
		if (Phase == ERTMatchPhase::Move)
		{
			ResolveMovement();
		}
	} while (Phase != ERTMatchPhase::Planning);

	++TurnNumber;
	UE_LOG(LogRT, Log, TEXT("[RT] Turno risolto, ora turno %d"), TurnNumber);

	// Riavvia la pianificazione del nuovo turno.
	StartPlanningTimer();
}

void ARTTurnManager::ResolveMovement()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	const FVector Origin = Grid ? Grid->GetActorLocation() : FVector::ZeroVector;
	const float CellSize = Grid ? Grid->CellSize : 200.f;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	TArray<FRTMoveRequest> Requests;
	Units.Reserve(Actors.Num());
	Requests.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Units.Add(Unit);

			// Difesa autorevole: un piano fuori portata viene ignorato (l'unita' resta ferma),
			// a prescindere da cosa ha inviato il client.
			const FRTGridCoord Target =
				URTGridLibrary::IsWithinRange(Unit->GridCell, Unit->PlannedCell, Unit->MoveRange)
					? Unit->PlannedCell
					: Unit->GridCell;
			Requests.Add(FRTMoveRequest(Unit->GridCell, Target));
		}
	}

	const TArray<FRTGridCoord> Resolved = URTMovementResolver::ResolveMoves(Requests);
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->PlaceOnCell(Resolved[i], Origin, CellSize);
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Move: risolte %d unita'"), Units.Num());
}
