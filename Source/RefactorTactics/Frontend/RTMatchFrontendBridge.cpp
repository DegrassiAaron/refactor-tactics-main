#include "Frontend/RTMatchFrontendBridge.h"

#include "Engine/GameInstance.h"
#include "Frontend/RTFrontendNavigator.h"
#include "RefactorTactics.h" // LogRT

URTFrontendNavigator* FRTMatchFrontendBridge::FindNavigator(UGameInstance* GameInstance)
{
	return GameInstance ? GameInstance->GetSubsystem<URTFrontendNavigator>() : nullptr;
}

FString FRTMatchFrontendBridge::ConsumeMatchLevel(UGameInstance* GameInstance, const FString& AnnouncedLevel)
{
	URTFrontendNavigator* Navigator = FindNavigator(GameInstance);

	// Si CONSUMA, non si legge: una richiesta che resta li' fa rifiutare il `PLAY` successivo con
	// «mai consumata», che punta il dito su chi non consuma invece che su chi non si e' iscritto.
	const FString Consumed = Navigator ? Navigator->ConsumePendingMatchLevel() : FString();
	if (Consumed.IsEmpty())
	{
		// ⚠️ «di partita» e' il consumatore, non l'evento: `ARTFrontendGameMode` emette la riga gemella dal
		// mondo del menu, e senza questa distinzione il log non dice quale dei due ha parlato.
		UE_LOG(LogRT, Error,
			TEXT("[RT] PLAY AGAIN dal mondo di partita: annuncio per '%s' senza richiesta pendente, "
				 "nulla da aprire."), *AnnouncedLevel);
		return FString();
	}

	UE_LOG(LogRT, Log, TEXT("[RT] PLAY AGAIN: riapertura del livello di partita '%s'"), *Consumed);
	return Consumed;
}

FString FRTMatchFrontendBridge::ConsumeFrontendLevel(UGameInstance* GameInstance, const FString& AnnouncedLevel)
{
	URTFrontendNavigator* Navigator = FindNavigator(GameInstance);

	const FString Consumed = Navigator ? Navigator->ConsumePendingFrontendLevel() : FString();
	if (Consumed.IsEmpty())
	{
		UE_LOG(LogRT, Error,
			TEXT("[RT] Annuncio di ritorno al menu per '%s' senza richiesta pendente: nulla da aprire."),
			*AnnouncedLevel);
		return FString();
	}

	// ⛔ **Qui la partita finisce davvero, e non perche' qualcuno la spenga**: cambiare livello distrugge il
	// mondo, e con lui `ARTTurnManager`, le `ARTUnit` e il GameMode. E' il motivo per cui il DoD puo'
	// chiedere «nessuno stato vivo» invece di un elenco di cose da azzerare: cio' che vive nel mondo se ne
	// va con il mondo, e cio' che sopravvive — i subsystem della `GameInstance` — e' un elenco corto e noto.
	// Il navigatore l'ha gia' ripulito in `ReturnMain`, e i suoi widget li smonta `OnWorldCleanup`.
	UE_LOG(LogRT, Log, TEXT("[RT] RETURN TO MAIN MENU: smontaggio della partita, apertura di '%s'"), *Consumed);
	return Consumed;
}

void FRTMatchFrontendBridge::ShowResult(UGameInstance* GameInstance, const FRTMatchResult& Result,
	const FRTMatchState& State)
{
	URTFrontendNavigator* Navigator = FindNavigator(GameInstance);
	if (!Navigator)
	{
		// ⚠️ Non e' un errore: uno scenario headless o un test di simulazione girano senza frontend, e la
		// partita deve poter finire lo stesso. Chi ha bisogno del Result e' il gioco, non il resolver.
		UE_LOG(LogRT, Verbose,
			TEXT("[RT] Partita finita senza frontend: nessuna schermata di Result da aprire."));
		return;
	}

	const ERTNavResult NavResult = Navigator->ShowResult(Result, State);
	if (NavResult != ERTNavResult::Ok)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT] Fine partita: il Result non si e' aperto (%s)."),
			*UEnum::GetValueAsString(NavResult));
	}
}
