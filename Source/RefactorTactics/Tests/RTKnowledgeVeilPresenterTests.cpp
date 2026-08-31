// IL VIEWER DEL VELO APPARTIENE AL CLIENT, non alla partita.
//
// È la fetta 4 di `E-SOLID`: `ARTGameMode` non risponde piu' a «di chi e' la vista», perche' in multiplayer
// il GameMode e' uno solo e sta sul server, mentre i viewer sono quanti sono i client. Questi test pinnano
// la nuova sede — l'`Outer` del presenter — e il ripiego headless che le run senza client continuano ad avere.

#include "Misc/AutomationTest.h"
#include "Map/RTHexMapActor.h"
#include "Perception/RTKnowledgeVeilPresenter.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTTurnManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * 🔑 **La vista e' quella del controller che possiede il presenter, e si RILEGGE.**
 *
 * ⚠️ La seconda meta' del test — cambiare la squadra sul `ARTPlayerState` e richiedere la risposta — e'
 * quella che discrimina: un presenter che copiasse il valore alla costruzione passerebbe la prima asserzione
 * e fallirebbe questa, ed e' esattamente il difetto che «si rilegge, non si copia» esiste per evitare. Un
 * secondo posto dove il team e' scritto e' un posto che puo' divergere.
 *
 * ⚠️ **Usa `MakePlayerOnTeam`, non uno `SpawnActor<ARTPlayerController>` nudo.** Misurato in
 * `Player.TeamFallsBackWithoutPlayerState`: un controller spawnato da solo in un mondo senza
 * `InitializeActorsForPlay` non ha un `PlayerState`, quindi `ARTPlayerState::TeamIdOf` — la fonte di
 * `ViewerTeamId()` dopo la migrazione a una porta sola — ripiegherebbe sempre su `0` e questo test non
 * discriminerebbe piu' nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilPresenterReadsItsOwnerTest,
	"RefactorTactics.Veil.PresenterViewerIsItsOwningController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilPresenterReadsItsOwnerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, 1);
	if (!TestNotNull(TEXT("controller"), PC)) { RTWorldFixtures::DestroyWorld(World); return false; }

	URTKnowledgeVeilPresenter* Presenter = PC->GetKnowledgeVeilPresenter();
	if (!TestNotNull(TEXT("il controller possiede un presenter"), Presenter))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	TestEqual(TEXT("la vista e' quella del PlayerState"), Presenter->ViewerTeamId(), 1);

	Cast<ARTPlayerState>(PC->PlayerState)->AssignTeam(0);
	TestEqual(TEXT("e SEGUE lo stato, non una copia"), Presenter->ViewerTeamId(), 0);

	// Il presenter e' UNO per controller: chiederlo due volte non ne apre un secondo, altrimenti due velo
	// diversi conterebbero ciascuno le proprie applicazioni e nessuno dei due direbbe la verita'.
	TestTrue(TEXT("il presenter di un controller e' uno solo"), PC->GetKnowledgeVeilPresenter() == Presenter);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Senza un controller che lo possieda, il viewer e' la squadra `0` — **dichiaratamente**.
 *
 * E' il caso delle run headless e dell'harness: la board sta nel mondo anche quando nessun client la guarda,
 * e il velo deve poter essere steso lo stesso. E' la stessa regola di ripiego di `ARTCameraPawn::FrameOwnTeam`
 * e di `ARTHUD::DrawHUD`, entrambi lettori della stessa `ARTPlayerState::TeamIdOf` che il loro test pinna:
 * tre lettori, un solo comportamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilPresenterFallsBackToTeamZeroTest,
	"RefactorTactics.Veil.PresenterWithoutControllerFallsBackToTeamZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilPresenterFallsBackToTeamZeroTest::RunTest(const FString&)
{
	// Nessun mondo e nessun controller: l'`Outer` e' il package transitorio, che non e' un controller.
	URTKnowledgeVeilPresenter* Presenter = NewObject<URTKnowledgeVeilPresenter>(GetTransientPackage());
	if (!TestNotNull(TEXT("presenter"), Presenter)) { return false; }

	TestEqual(TEXT("senza controller il viewer e' la squadra 0"), Presenter->ViewerTeamId(), 0);

	// ⚠️ **Un `Apply` senza turn manager non deve CONTARE**: il contatore dice quante volte il velo e' stato
	// steso, non quante volte qualcuno ci ha provato. Se contasse anche i tentativi a vuoto,
	// `Veil.GameModeAppliesTheVeilForTheViewerTeam` misurerebbe un numero che non parla della board.
	Presenter->Apply();
	TestEqual(TEXT("un'applicazione senza turn manager non conta"), Presenter->GetApplications(), 0);

	return true;
}

/**
 * L'aggancio stende il velo SUBITO, e il conteggio lo rende osservabile.
 *
 * ⚠️ Il primo fotogramma e' l'unico che nessun test tardivo puo' prendere: senza l'applicazione immediata la
 * board nasce interamente visibile e si vela al primo refresh — un frame che rivela tutta la mappa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilPresenterHookAppliesImmediatelyTest,
	"RefactorTactics.Veil.PresenterHookAppliesImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilPresenterHookAppliesImmediatelyTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("mappa"), HexMap) || !TestNotNull(TEXT("TurnManager"), TM))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	URTKnowledgeVeilPresenter* Presenter = NewObject<URTKnowledgeVeilPresenter>(TM);
	TestEqual(TEXT("prima dell'aggancio il velo non e' mai stato steso"), Presenter->GetApplications(), 0);

	Presenter->Hook(TM);
	TestEqual(TEXT("l'aggancio stende il velo una volta"), Presenter->GetApplications(), 1);

	// Agganciare due volte non raddoppia l'iscrizione (`AddUniqueDynamic`), ma stende di nuovo: e' la
	// differenza fra «iscriversi» e «disegnare», e la seconda e' idempotente per costruzione.
	Presenter->Hook(TM);
	TestEqual(TEXT("un secondo aggancio ristende, senza duplicare l'iscrizione"),
		Presenter->GetApplications(), 2);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
