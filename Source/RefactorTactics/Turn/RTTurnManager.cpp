#include "Turn/RTTurnManager.h"
#include "Turn/RTMovementResolver.h"
#include "Grid/RTGridActor.h"
#include "Unit/RTUnit.h"
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"

ARTTurnManager::ARTTurnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARTTurnManager::LockInAndResolve()
{
	if (Phase != ERTMatchPhase::Planning)
	{
		return;
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
			Requests.Add(FRTMoveRequest(Unit->GridCell, Unit->PlannedCell));
		}
	}

	const TArray<FRTGridCoord> Resolved = URTMovementResolver::ResolveMoves(Requests);
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->PlaceOnCell(Resolved[i], Origin, CellSize);
	}

	UE_LOG(LogRT, Log, TEXT("[RT] Fase Move: risolte %d unita'"), Units.Num());
}
