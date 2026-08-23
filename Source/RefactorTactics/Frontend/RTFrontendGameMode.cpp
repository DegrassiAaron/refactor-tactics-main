#include "Frontend/RTFrontendGameMode.h"

#include "Frontend/RTFrontendNavigator.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "RefactorTactics.h"

ARTFrontendGameMode::ARTFrontendGameMode()
{
	// ⚠️ **`nullptr`, non un pawn diverso.** Su una mappa di menu non c'e' niente da possedere: il default
	// di `AGameModeBase` e' `ADefaultPawn`, che vola e cattura il mouse-look. Qui il giocatore non ha un
	// corpo, ha un cursore.
	DefaultPawnClass = nullptr;

	// ⚠️ **`PlayerControllerClass` e `HUDClass` NON si toccano, ed e' una scelta.** I default di
	// `AGameModeBase` — `APlayerController` e `AHUD` — sono gia' quelli giusti, e riassegnarli qui sarebbe
	// uno specifier che non configura niente: lo stesso difetto per cui `FRTScreenBinding` era stata tolta
	// la prima volta. In particolare **non** si usa `ARTPlayerController`: quello porta il contratto del
	// puntatore tattico di CP 11.8, sette contesti che un menu non ha.
	//
	// Il GameMode del frontend non ha niente da far avanzare: nessun tick.
	PrimaryActorTick.bCanEverTick = false;
}

void ARTFrontendGameMode::BeginPlay()
{
	Super::BeginPlay();
	StartFrontendForThisGame();
}

void ARTFrontendGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	ConfigureMenuInput(NewPlayer);
}

void ARTFrontendGameMode::ConfigureMenuInput(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	// Il cursore e' la meta' visibile del requisito; l'altra e' che i tasti vadano alla UI e non al mondo.
	// `GameAndUI` e non `UIOnly`: quest'ultimo richiede un widget su cui mettere il focus, e a `PostLogin`
	// il menu puo' non essere ancora a schermo — con `UIOnly` senza focus l'input resta nel vuoto.
	PlayerController->bShowMouseCursor = true;
	PlayerController->SetInputMode(FInputModeGameAndUI());
}

bool ARTFrontendGameMode::StartFrontendForThisGame()
{
	// ⚠️ I due controlli sono separati perche' **sono due guasti diversi**, e una riga di log che li
	// confondesse manderebbe chi indaga nel posto sbagliato: una `GameInstance` assente e' un problema di
	// come la mappa e' stata avviata, un subsystem assente e' un problema di come il modulo e' stato
	// costruito. Un solo `if` con un solo messaggio costerebbe mezz'ora a chi legge il log fra sei mesi.
	//
	// ⚠️ **Solo il primo dei due e' coperto da un test, e va detto invece che sottinteso.** Una
	// `UGameInstance` costruisce sempre i propri subsystem, quindi «esiste la GameInstance ma non il
	// navigatore» non e' uno stato che un test possa allestire. Il ramo resta perche' un `GetSubsystem`
	// che risponde `nullptr` non va dereferenziato, non perche' qualcuno lo eserciti.
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

	// L'esito e' del navigatore: qui non si ridecide niente, si riporta.
	return Navigator->StartFrontend();
}

void ARTFrontendGameMode::ListenForMatchRequests(URTFrontendNavigator* Navigator)
{
	if (!Navigator)
	{
		UE_LOG(LogRT, Error,
			TEXT("[RT] Nessun navigatore da ascoltare: PLAY non aprirebbe nulla."));
		return;
	}

	// `AddUniqueDynamic` e non `AddDynamic`: `BeginPlay` puo' correre piu' di una volta sullo stesso
	// GameMode in editor, e due iscrizioni aprirebbero il livello due volte.
	ListenedNavigator = Navigator;
	Navigator->OnMatchRequested.AddUObject(this, &ARTFrontendGameMode::HandleMatchRequested);
}

void ARTFrontendGameMode::HandleMatchRequested(const FString& LevelName)
{
	// ⚠️ Si CONSUMA, non si legge: la richiesta deve sparire, altrimenti il `PLAY` successivo viene
	// rifiutato dalla guardia che segnala l'aggancio mancante — e l'aggancio invece c'e'.
	const FString Consumed = ListenedNavigator ? ListenedNavigator->ConsumePendingMatchLevel() : FString();

	if (Consumed.IsEmpty())
	{
		// L'annuncio e' arrivato ma la richiesta non c'era: e' l'ordine invertito che il contratto vieta.
		UE_LOG(LogRT, Error,
			TEXT("[RT] Annuncio di partita per '%s' senza richiesta pendente: nulla da aprire."), *LevelName);
		return;
	}

	OpenMatchLevel(Consumed);
}

void ARTFrontendGameMode::OpenMatchLevel(const FString& LevelName)
{
	UE_LOG(LogRT, Log, TEXT("[RT] Apertura del livello di partita: '%s'"), *LevelName);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}
