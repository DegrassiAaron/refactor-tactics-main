#include "Frontend/RTFrontendGameMode.h"

#include "Frontend/RTFrontendNavigator.h"
#include "Engine/GameInstance.h"
#include "RefactorTactics.h"

void ARTFrontendGameMode::BeginPlay()
{
	Super::BeginPlay();
	StartFrontendForThisGame();
}

bool ARTFrontendGameMode::StartFrontendForThisGame()
{
	// ⚠️ I due controlli sono separati perche' **sono due guasti diversi**, e una riga di log che li
	// confondesse manderebbe chi indaga nel posto sbagliato: una `GameInstance` assente e' un problema di
	// come la mappa e' stata avviata, un subsystem assente e' un problema di come il modulo e' stato
	// costruito. Un solo `if` con un solo messaggio costerebbe mezz'ora a chi legge il log fra sei mesi.
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogRT, Error,
			TEXT("Frontend non avviato: questa mappa non ha una GameInstance, e il navigatore vive li'. ")
			TEXT("La schermata restera' vuota."));
		return false;
	}

	URTFrontendNavigator* Navigator = GameInstance->GetSubsystem<URTFrontendNavigator>();
	if (!Navigator)
	{
		UE_LOG(LogRT, Error,
			TEXT("Frontend non avviato: URTFrontendNavigator non e' fra i subsystem della GameInstance. ")
			TEXT("La schermata restera' vuota."));
		return false;
	}

	// L'esito e' del navigatore: qui non si ridecide niente, si riporta. `Ok` e' l'unico successo —
	// ogni altro valore di `ERTNavResult` significa che la radice non si e' aperta.
	return Navigator->StartFrontend() == ERTNavResult::Ok;
}
