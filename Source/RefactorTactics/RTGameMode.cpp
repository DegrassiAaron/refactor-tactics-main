#include "RTGameMode.h"
#include "Camera/RTCameraPawn.h"
#include "Player/RTPlayerController.h"
#include "Grid/RTGridActor.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"

ARTGameMode::ARTGameMode()
{
	DefaultPawnClass = ARTCameraPawn::StaticClass();
	PlayerControllerClass = ARTPlayerController::StaticClass();
}

void ARTGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Se il livello non contiene gia' una griglia, ne creo una all'origine.
	if (!UGameplayStatics::GetActorOfClass(this, ARTGridActor::StaticClass()))
	{
		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<ARTGridActor>(ARTGridActor::StaticClass(), FTransform::Identity);
			UE_LOG(LogRT, Log, TEXT("[RT] GameMode: griglia generata automaticamente"));
		}
	}
}
