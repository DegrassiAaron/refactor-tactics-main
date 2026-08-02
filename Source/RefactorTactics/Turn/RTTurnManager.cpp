#include "Turn/RTTurnManager.h"
#include "Turn/RTMovementResolver.h"
#include "Combat/RTCombatResolver.h"
#include "Bot/RTBotLibrary.h"
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

void ARTTurnManager::PlanBots()
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	Units.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Units.Add(Unit);
		}
	}

	for (ARTUnit* Bot : Units)
	{
		if (!Bot->bIsBotControlled || !Bot->IsAlive())
		{
			continue;
		}

		// Nemico vivo piu' vicino.
		ARTUnit* Nearest = nullptr;
		int32 BestDistance = MAX_int32;
		for (ARTUnit* Other : Units)
		{
			if (Other->IsAlive() && Other->TeamId != Bot->TeamId)
			{
				const int32 D = URTGridLibrary::ManhattanDistance(Bot->GridCell, Other->GridCell);
				if (D < BestDistance)
				{
					BestDistance = D;
					Nearest = Other;
				}
			}
		}

		Bot->PlannedCell = Bot->GridCell;   // default: fermo
		Bot->PlannedAttackTarget = nullptr;
		if (!Nearest)
		{
			continue;
		}

		if (URTGridLibrary::IsWithinRange(Bot->GridCell, Nearest->GridCell, Bot->AttackRange))
		{
			Bot->PlannedAttackTarget = Nearest; // in portata: attacca, resta fermo
		}
		else
		{
			Bot->PlannedCell = URTBotLibrary::StepToward(Bot->GridCell, Nearest->GridCell, Bot->MoveRange);
		}
	}
}

void ARTTurnManager::StartPlanningTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	PlanBots(); // il bot pianifica a inizio turno

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
		if (Phase == ERTMatchPhase::Blast)
		{
			ResolveCombat(); // gli attacchi usano la posizione PRIMA del movimento
		}
		else if (Phase == ERTMatchPhase::Move)
		{
			ResolveMovement();
		}
	} while (Phase != ERTMatchPhase::Planning);

	++TurnNumber;
	UE_LOG(LogRT, Log, TEXT("[RT] Turno risolto, ora turno %d"), TurnNumber);

	// Riavvia la pianificazione del nuovo turno.
	StartPlanningTimer();
}

void ARTTurnManager::ResolveCombat()
{
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);

	TArray<ARTUnit*> Units;
	TMap<ARTUnit*, int32> IndexOf;
	TArray<FRTUnitCombatState> States;
	Units.Reserve(Actors.Num());
	States.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			IndexOf.Add(Unit, Units.Num());
			Units.Add(Unit);
			States.Add(FRTUnitCombatState(Unit->Health, Unit->Shield));
		}
	}

	// Raccogli gli attacchi validi: bersaglio nemico, vivo, entro la portata (posizione attuale).
	TArray<FRTAttack> Attacks;
	for (ARTUnit* Unit : Units)
	{
		ARTUnit* Target = Unit->PlannedAttackTarget;
		if (Target && IndexOf.Contains(Target) && Target->TeamId != Unit->TeamId
			&& URTGridLibrary::IsWithinRange(Unit->GridCell, Target->GridCell, Unit->AttackRange))
		{
			Attacks.Add(FRTAttack(IndexOf[Target], Unit->AttackPower));
		}
		Unit->PlannedAttackTarget = nullptr; // consumato nel turno
	}

	if (Attacks.Num() == 0)
	{
		return;
	}

	const TArray<FRTUnitCombatState> Resolved = URTCombatResolver::ResolveAttacks(States, Attacks);
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		Units[i]->ApplyCombatState(Resolved[i].Health, Resolved[i].Shield); // puo' distruggere l'unita'
	}
	UE_LOG(LogRT, Log, TEXT("[RT] Fase Blast: %d attacchi risolti"), Attacks.Num());
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
