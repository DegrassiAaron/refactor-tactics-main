#include "Player/RTPlayerController.h"
#include "Camera/RTCameraPawn.h"
#include "Selection/RTSelectable.h"
#include "Grid/RTGridActor.h"
#include "Grid/RTGridLibrary.h"
#include "Unit/RTUnit.h"
#include "Ability/RTAbilityData.h"
#include "Turn/RTTurnManager.h"
#include "Core/RTTypes.h"
#include "RefactorTactics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"

void ARTPlayerController::BuildInputMappings()
{
	if (MappingContext)
	{
		return; // gia' costruito
	}

	PanAction = NewObject<UInputAction>(this, TEXT("IA_Pan"));
	PanAction->ValueType = EInputActionValueType::Axis2D;

	ZoomAction = NewObject<UInputAction>(this, TEXT("IA_Zoom"));
	ZoomAction->ValueType = EInputActionValueType::Axis1D;

	SelectAction = NewObject<UInputAction>(this, TEXT("IA_Select"));
	SelectAction->ValueType = EInputActionValueType::Boolean;

	LockInAction = NewObject<UInputAction>(this, TEXT("IA_LockIn"));
	LockInAction->ValueType = EInputActionValueType::Boolean;

	RestartAction = NewObject<UInputAction>(this, TEXT("IA_Restart"));
	RestartAction->ValueType = EInputActionValueType::Boolean;

	Ability1Action = NewObject<UInputAction>(this, TEXT("IA_Ability1"));
	Ability1Action->ValueType = EInputActionValueType::Boolean;
	Ability2Action = NewObject<UInputAction>(this, TEXT("IA_Ability2"));
	Ability2Action->ValueType = EInputActionValueType::Boolean;
	Ability3Action = NewObject<UInputAction>(this, TEXT("IA_Ability3"));
	Ability3Action->ValueType = EInputActionValueType::Boolean;

	UndoAction = NewObject<UInputAction>(this, TEXT("IA_UndoWaypoint"));
	UndoAction->ValueType = EInputActionValueType::Boolean;

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Tactical"));

	// Pan (Axis2D): D=+X, A=-X, W=+Y (Swizzle YXZ), S=-Y (Swizzle YXZ + Negate).
	MappingContext->MapKey(PanAction, EKeys::D);
	{
		FEnhancedActionKeyMapping& M = MappingContext->MapKey(PanAction, EKeys::A);
		M.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}
	{
		FEnhancedActionKeyMapping& M = MappingContext->MapKey(PanAction, EKeys::W);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		M.Modifiers.Add(Swizzle);
	}
	{
		FEnhancedActionKeyMapping& M = MappingContext->MapKey(PanAction, EKeys::S);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		M.Modifiers.Add(Swizzle);
		M.Modifiers.Add(NewObject<UInputModifierNegate>(this));
	}

	// Zoom (Axis1D): rotellina del mouse.
	MappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);

	// Select (Boolean): tasto sinistro del mouse.
	MappingContext->MapKey(SelectAction, EKeys::LeftMouseButton);

	// Lock-in (Boolean): barra spaziatrice.
	MappingContext->MapKey(LockInAction, EKeys::SpaceBar);

	// Riavvio partita (Boolean): tasto R (attivo solo a match concluso).
	MappingContext->MapKey(RestartAction, EKeys::R);

	// Selezione abilita' (Boolean): tasti 1/2/3.
	MappingContext->MapKey(Ability1Action, EKeys::One);
	MappingContext->MapKey(Ability2Action, EKeys::Two);
	MappingContext->MapKey(Ability3Action, EKeys::Three);

	// Annulla l'ultimo waypoint della path composita (tasto destro del mouse o Backspace).
	MappingContext->MapKey(UndoAction, EKeys::RightMouseButton);
	MappingContext->MapKey(UndoAction, EKeys::BackSpace);
}

void ARTPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;

	BuildInputMappings();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}
}

void ARTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BuildInputMappings();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(PanAction, ETriggerEvent::Triggered, this, &ARTPlayerController::OnPan);
		EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ARTPlayerController::OnZoom);
		EIC->BindAction(SelectAction, ETriggerEvent::Started, this, &ARTPlayerController::OnSelect);
		EIC->BindAction(LockInAction, ETriggerEvent::Started, this, &ARTPlayerController::OnLockIn);
		EIC->BindAction(RestartAction, ETriggerEvent::Started, this, &ARTPlayerController::OnRestart);
		EIC->BindAction(Ability1Action, ETriggerEvent::Started, this, &ARTPlayerController::OnAbility1);
		EIC->BindAction(Ability2Action, ETriggerEvent::Started, this, &ARTPlayerController::OnAbility2);
		EIC->BindAction(Ability3Action, ETriggerEvent::Started, this, &ARTPlayerController::OnAbility3);
		EIC->BindAction(UndoAction, ETriggerEvent::Started, this, &ARTPlayerController::OnUndoWaypoint);
	}
	else
	{
		UE_LOG(LogRT, Error, TEXT("[RT] InputComponent non e' un UEnhancedInputComponent"));
	}
}

void ARTPlayerController::OnPan(const FInputActionValue& Value)
{
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		Cam->AddPlanarMovement(Value.Get<FVector2D>());
	}
}

void ARTPlayerController::OnZoom(const FInputActionValue& Value)
{
	if (ARTCameraPawn* Cam = Cast<ARTCameraPawn>(GetPawn()))
	{
		Cam->AddZoom(Value.Get<float>());
	}
}

void ARTPlayerController::OnSelect(const FInputActionValue& Value)
{
	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/ false, Hit) || !Hit.GetActor())
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	ARTUnit* ClickedUnit = Cast<ARTUnit>(HitActor);
	ARTUnit* SelectedUnit = Cast<ARTUnit>(SelectedActor);

	// Click su un'unita' nemica, con una nostra unita' selezionata -> pianifica l'abilita' attiva.
	if (ClickedUnit && SelectedUnit && ClickedUnit != SelectedUnit && ClickedUnit->TeamId != SelectedUnit->TeamId)
	{
		const int32 AbilityIndex = SelectedUnit->SelectedAbilityIndex;
		const URTAbilityData* Ability = SelectedUnit->GetAbility(AbilityIndex);
		if (!Ability)
		{
			return;
		}
		const ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
		const bool bReady = SelectedUnit->CanUseAbility(AbilityIndex);
		const bool bInRange = URTGridLibrary::IsWithinRange(SelectedUnit->GridCell, ClickedUnit->GridCell, Ability->RangeCells);
		const bool bHasLOS = !Grid || URTGridLibrary::HasLineOfSight(SelectedUnit->GridCell, ClickedUnit->GridCell, Grid->GetVisionBlockers());
		if (bReady && bInRange && bHasLOS)
		{
			SelectedUnit->PlannedAbilityIndex = AbilityIndex;
			SelectedUnit->PlannedAttackTarget = ClickedUnit;
			UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s usa %s su %s"), *SelectedUnit->GetName(), *Ability->DisplayName.ToString(), *ClickedUnit->GetName());
		}
		else if (!bReady)
		{
			UE_LOG(LogRT, Log, TEXT("[RT] %s non pronta (ricarica/energia)"), *Ability->DisplayName.ToString());
		}
		else if (!bHasLOS)
		{
			UE_LOG(LogRT, Log, TEXT("[RT] %s coperto (nessuna linea di tiro)"), *ClickedUnit->GetName());
		}
		else
		{
			UE_LOG(LogRT, Log, TEXT("[RT] %s fuori portata (max %d)"), *ClickedUnit->GetName(), Ability->RangeCells);
		}
		return;
	}

	// Click su un'unita' (o altro selezionabile) -> selezione.
	if (IRTSelectable* Selectable = Cast<IRTSelectable>(HitActor))
	{
		if (HitActor != SelectedActor)
		{
			if (IRTSelectable* Previous = Cast<IRTSelectable>(SelectedActor))
			{
				Previous->OnDeselected();
			}
			Selectable->OnSelected();
			SelectedActor = HitActor;
			UE_LOG(LogRT, Log, TEXT("[RT] Selezionata: %s"), *HitActor->GetName());
		}
		return;
	}

	// Click sulla griglia con un'unita' selezionata -> pianifica il movimento su quella cella.
	if (SelectedUnit)
	{
		if (ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass())))
		{
			const FRTGridCoord Cell = URTGridLibrary::WorldToCell(Hit.Location, Grid->GetActorLocation(), Grid->CellSize);
			if (!URTGridLibrary::IsInsideGrid(Cell, Grid->Width, Grid->Height))
			{
				return;
			}
			if (Grid->BlockedCells.Contains(Cell))
			{
				UE_LOG(LogRT, Log, TEXT("[RT] Cella (%d,%d) occupata da una copertura"), Cell.X, Cell.Y);
				return;
			}
			const int32 MoveRange = SelectedUnit->GetEffectiveMoveRange();
				TMap<FRTGridCoord, int32> CostMap;
				Grid->BuildCostMap(CostMap);
			if (!URTGridLibrary::ReachableCellsByCost(SelectedUnit->GridCell, MoveRange, CostMap, Grid->Width, Grid->Height).Contains(Cell))
			{
				UE_LOG(LogRT, Log, TEXT("[RT] Cella (%d,%d) non raggiungibile (percorso bloccato o fuori portata) per %s"),
					Cell.X, Cell.Y, *SelectedUnit->GetName());
				return;
			}
			SelectedUnit->PlannedWaypoints.Add(Cell);
				const TArray<FRTGridCoord> WPath = URTGridLibrary::BuildCompositePath(SelectedUnit->GridCell, SelectedUnit->PlannedWaypoints, CostMap, Grid->Width, Grid->Height);
				const int32 WCost = URTGridLibrary::PathCost(WPath, CostMap);
				if (WPath.Num() < 2 || WCost < 0 || WCost > MoveRange)
				{
					SelectedUnit->PlannedWaypoints.Pop(); // waypoint oltre budget o irraggiungibile: rifiutato
					UE_LOG(LogRT, Log, TEXT("[RT] Waypoint (%d,%d) rifiutato (costo %d, budget %d) per %s"),
						Cell.X, Cell.Y, WCost, MoveRange, *SelectedUnit->GetName());
					return;
				}
				SelectedUnit->PlannedPath = WPath;
				SelectedUnit->PlannedCell = WPath.Last();
			UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s -> %d waypoint (costo %d)"), *SelectedUnit->GetName(), SelectedUnit->PlannedWaypoints.Num(), WCost);
		}
	}
}

void ARTPlayerController::OnLockIn(const FInputActionValue& Value)
{
	if (ARTTurnManager* TurnManager = Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass())))
	{
		TurnManager->LockInAndResolve();
	}
}

void ARTPlayerController::OnRestart(const FInputActionValue& Value)
{
	// Riavvia la partita solo quando è conclusa: ricarica il livello corrente.
	const ARTTurnManager* TurnManager =
		Cast<ARTTurnManager>(UGameplayStatics::GetActorOfClass(this, ARTTurnManager::StaticClass()));
	if (TurnManager && TurnManager->GetPhase() == ERTMatchPhase::MatchEnded)
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
}

ARTUnit* ARTPlayerController::GetSelectedUnit() const
{
	return Cast<ARTUnit>(SelectedActor);
}

void ARTPlayerController::SelectAbilityForCurrent(int32 Index)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return;
	}
	Unit->SelectAbility(Index);
	const URTAbilityData* Ability = Unit->GetAbility(Index);
	if (!Ability)
	{
		return;
	}

	if (Ability->bSelfTarget)
	{
		// Supporto: si pianifica immediatamente su se stessi (nessun bersaglio da cliccare).
		if (Unit->CanUseAbility(Index))
		{
			Unit->PlannedAbilityIndex = Index;
			Unit->PlannedAttackTarget = nullptr;
			UE_LOG(LogRT, Log, TEXT("[RT] %s pianifica %s (supporto)"), *Unit->GetName(), *Ability->DisplayName.ToString());
		}
		else
		{
			UE_LOG(LogRT, Log, TEXT("[RT] %s non pronta"), *Ability->DisplayName.ToString());
		}
	}
	else
	{
		UE_LOG(LogRT, Log, TEXT("[RT] %s: abilita' attiva -> %s"), *Unit->GetName(), *Ability->DisplayName.ToString());
	}
}

void ARTPlayerController::OnAbility1(const FInputActionValue& Value) { SelectAbilityForCurrent(0); }
void ARTPlayerController::OnAbility2(const FInputActionValue& Value) { SelectAbilityForCurrent(1); }
void ARTPlayerController::OnAbility3(const FInputActionValue& Value) { SelectAbilityForCurrent(2); }

void ARTPlayerController::OnUndoWaypoint(const FInputActionValue& Value)
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit || Unit->PlannedWaypoints.Num() == 0)
	{
		return;
	}
	Unit->PlannedWaypoints.Pop(); // rimuove l'ultimo waypoint
	RebuildPlannedPath();
	UE_LOG(LogRT, Log, TEXT("[RT] Annullato waypoint: %s -> %d waypoint"), *Unit->GetName(), Unit->PlannedWaypoints.Num());
}

void ARTPlayerController::RebuildPlannedPath()
{
	ARTUnit* Unit = GetSelectedUnit();
	if (!Unit)
	{
		return;
	}
	ARTGridActor* Grid = Cast<ARTGridActor>(UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()));
	if (!Grid)
	{
		return;
	}
	TMap<FRTGridCoord, int32> CostMap;
	Grid->BuildCostMap(CostMap);
	const TArray<FRTGridCoord> Path = URTGridLibrary::BuildCompositePath(Unit->GridCell, Unit->PlannedWaypoints, CostMap, Grid->Width, Grid->Height);
	const int32 Cost = URTGridLibrary::PathCost(Path, CostMap);
	if (Unit->PlannedWaypoints.Num() > 0 && Path.Num() >= 2 && Cost >= 0 && Cost <= Unit->GetEffectiveMoveRange())
	{
		Unit->PlannedPath = Path;
		Unit->PlannedCell = Path.Last();
	}
	else
	{
		Unit->PlannedPath.Reset();
		Unit->PlannedCell = Unit->GridCell;
	}
}
