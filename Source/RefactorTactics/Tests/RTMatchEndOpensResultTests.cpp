#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "RTGameMode.h"
#include "Turn/RTTurnManager.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 46.5 · l'ultimo anello: **una partita che finisce apre il Result, senza che nessuno lo chieda.**
 *
 * 🔴 **È il difetto che due issue avevano lasciato aperto.** `URTFrontendNavigator::ShowResult` esisteva,
 * era testata da cinque test, e `git grep "ShowResult("` la trovava solo nella propria definizione e nei
 * test: a fine partita non la invocava nessuno. Il DoD di `#939` lo dice dal lato dell'utente — «dopo la
 * partita si arriva a CP 46.5, **non al desktop**».
 *
 * ⚠️ **Qui non si chiama né `ShowResult` né `OnMatchEnded`**, ed è tutto il punto: il test fa girare una
 * partita vera fino alla fine e poi guarda il navigatore. Chiamare l'anello che si vuole provare è il modo
 * in cui gli otto test verdi di `#939` non hanno visto che il consumatore non era collegato a niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchEndOpensResultTest,
	"RefactorTactics.Frontend.MatchEndOpensTheResultScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchEndOpensResultTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
	if (!TestNotNull(TEXT("GameInstance"), GI)) { RTWorldFixtures::DestroyWorld(World); return false; }
	GI->AddToRoot();
	GI->Init();
	World->SetGameInstance(GI);

	URTFrontendNavigator* Nav = GI->GetSubsystem<URTFrontendNavigator>();
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		GI->RemoveFromRoot(); RTWorldFixtures::DestroyWorld(World); return false;
	}
	Nav->InitializeFrontend(RTScreenIds::Main);
	TestFalse(TEXT("prima della partita non c'e' nessun risultato"), Nav->GetMatchResult().bIsFinished);

	// ⚠️ **Senza questa riga l'annuncio non arriva, ed è la lezione di `#939`**: `AActor::ProcessEvent`
	// scarta ogni evento se `GetWorld()->AreActorsInitialized()` è falso, e `OnMatchEnded` è un delegate
	// dinamico che invoca proprio da lì. `ARTGameMode` è un actor: senza inizializzare il mondo, la
	// partita finirebbe e il Result resterebbe chiuso — con la partita che *sembra* funzionare.
	World->InitializeActorsForPlay(FURL());

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		GI->RemoveFromRoot(); RTWorldFixtures::DestroyWorld(World); return false;
	}

	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	// Una partita vera, fino alla fine. ⚠️ Il tetto sui turni non e' decorativo: se la partita non
	// finisse, senza di esso il test girerebbe all'infinito invece di dire cosa non ha visto.
	const int32 MaxTurni = 40;
	int32 Turni = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
		++Turni;
	}

	if (!TestEqual(TEXT("la partita e' finita entro il tetto di turni"),
		TM->GetPhase(), ERTMatchPhase::MatchEnded))
	{
		GI->RemoveFromRoot(); RTWorldFixtures::DestroyWorld(World); return false;
	}

	// 🔴 L'asserzione che vale il test: nessuno ha chiamato `ShowResult`, e il Result e' aperto.
	TestTrue(TEXT("il Result si e' aperto da solo"), Nav->GetMatchResult().bIsFinished);
	TestEqual(TEXT("ed e' la schermata corrente"), Nav->GetCurrentScreen(), RTScreenIds::Result);
	TestEqual(TEXT("con i round davvero giocati"), Nav->GetMatchResult().RoundNumber, Turni);
	TestNotEqual(TEXT("e un esito che non e' 'in corso'"),
		Nav->GetMatchResult().Outcome, ERTMatchOutcome::InProgress);

	GI->RemoveFromRoot();
	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
