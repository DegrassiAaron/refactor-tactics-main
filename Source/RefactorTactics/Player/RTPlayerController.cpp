#include "Player/RTPlayerController.h"
#include "Camera/RTCameraPawn.h"
#include "Selection/RTSelectable.h"
#include "RefactorTactics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "UObject/ConstructorHelpers.h"

ARTPlayerController::ARTPlayerController()
{
	// Caricamento degli asset di input dalle path convenzionali (crea gli asset in /Game/Input/).
	// Finche' non esistono vedrai dei warning "failed to find object": e' atteso.
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC(TEXT("/Game/Input/IMC_Tactical.IMC_Tactical"));
	if (IMC.Succeeded()) { MappingContext = IMC.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> Pan(TEXT("/Game/Input/IA_Pan.IA_Pan"));
	if (Pan.Succeeded()) { PanAction = Pan.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> Zoom(TEXT("/Game/Input/IA_Zoom.IA_Zoom"));
	if (Zoom.Succeeded()) { ZoomAction = Zoom.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> Select(TEXT("/Game/Input/IA_Select.IA_Select"));
	if (Select.Succeeded()) { SelectAction = Select.Object; }
}

void ARTPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (MappingContext)
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
		else
		{
			UE_LOG(LogRT, Warning, TEXT("[RT] IMC_Tactical non trovato: crea gli asset di input in /Game/Input/"));
		}
	}
}

void ARTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PanAction)    { EIC->BindAction(PanAction, ETriggerEvent::Triggered, this, &ARTPlayerController::OnPan); }
		if (ZoomAction)   { EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ARTPlayerController::OnZoom); }
		if (SelectAction) { EIC->BindAction(SelectAction, ETriggerEvent::Started, this, &ARTPlayerController::OnSelect); }
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
	if (GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/ false, Hit) && Hit.GetActor())
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Select: %s @ %s"), *Hit.GetActor()->GetName(), *Hit.Location.ToCompactString());

		if (IRTSelectable* Selectable = Cast<IRTSelectable>(Hit.GetActor()))
		{
			Selectable->OnSelected();
		}
	}
}
