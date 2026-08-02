#include "Player/RTPlayerController.h"
#include "Camera/RTCameraPawn.h"
#include "Selection/RTSelectable.h"
#include "Grid/RTGridActor.h"
#include "Grid/RTGridLibrary.h"
#include "Unit/RTUnit.h"
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

	// Click su un'unita' nemica, con una nostra unita' selezionata -> pianifica un attacco.
	if (ClickedUnit && SelectedUnit && ClickedUnit != SelectedUnit && ClickedUnit->TeamId != SelectedUnit->TeamId)
	{
		if (URTGridLibrary::IsWithinRange(SelectedUnit->GridCell, ClickedUnit->GridCell, SelectedUnit->AttackRange))
		{
			SelectedUnit->PlannedAttackTarget = ClickedUnit;
			UE_LOG(LogRT, Log, TEXT("[RT] Piano attacco: %s -> %s"), *SelectedUnit->GetName(), *ClickedUnit->GetName());
		}
		else
		{
			UE_LOG(LogRT, Log, TEXT("[RT] %s fuori portata attacco (max %d)"), *ClickedUnit->GetName(), SelectedUnit->AttackRange);
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
			if (!URTGridLibrary::IsWithinRange(SelectedUnit->GridCell, Cell, SelectedUnit->MoveRange))
			{
				UE_LOG(LogRT, Log, TEXT("[RT] Cella (%d,%d) fuori portata (max %d) per %s"),
					Cell.X, Cell.Y, SelectedUnit->MoveRange, *SelectedUnit->GetName());
				return;
			}
			SelectedUnit->PlannedCell = Cell;
			UE_LOG(LogRT, Log, TEXT("[RT] Piano: %s -> cella (%d,%d)"), *SelectedUnit->GetName(), Cell.X, Cell.Y);
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
