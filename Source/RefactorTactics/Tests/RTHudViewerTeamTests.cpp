// Di chi e' la vista che l'HUD disegna.
//
// Il difetto che questi test chiudono non era attivo: `DrawHUD` scriveva `const int32 PlayerTeamId = 0;` e
// in partita quello ZERO era la risposta giusta, perche' il giocatore E' la squadra 0. Era una SECONDA SEDE
// della stessa domanda che [D-242] aveva gia' deciso per il velo, e alimentava sei consumatori di cui
// quattro sono filtri di privacy (`FilterForTeam`, `GetRecentEventsForTeam`, `VisibleTrailFor`,
// `ViewForTeam`).
//
// 🔴 **Un letterale non ha un modo di fallire chiuso.** `FilterForTeam` decide con
// `Intent.TeamId == ObserverTeamId`: un osservatore sbagliato non nasconde di meno, rovescia la simmetria —
// gli intenti non rivelati dell'avversario diventano «alleati» e si vedono tutti. Per questo la domanda
// «chi guarda» merita un test suo, e non basta che i filtri siano coperti.
//
// ⚠️ `Knowledge.ViewIsIndependentOfHiddenState` NON copre questa domanda, ed e' il motivo per cui questo
// file esiste: quel test fissa il viewer alla squadra 0 e varia lo stato autoritativo. Verifica che il
// filtro non perda informazione, non che il filtro stia guardando dalla parte giusta.

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/RTPlayerController.h"
#include "UI/RTHUD.h"

namespace
{
	/** Nome distinto per file: la unity build condivide la translation unit. */
	UWorld* HvtMakeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void HvtDestroyWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

/**
 * La squadra dell'HUD e' quella del controller, non una costante.
 *
 * 🔑 **Il valore di prova e' `1`, e non e' un dettaglio.** Con `0` il test resterebbe verde anche
 * rimettendo il letterale, cioe' non falsificherebbe nulla: e' esattamente la mutazione che questo test
 * esiste per prendere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudViewerTeamFollowsControllerTest,
	"RefactorTactics.Hud.ViewerTeamFollowsController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudViewerTeamFollowsControllerTest::RunTest(const FString&)
{
	UWorld* World = HvtMakeWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller"), PC)) { HvtDestroyWorld(World); return false; }

	PC->PlayerTeamId = 1;
	TestEqual(TEXT("la squadra e' quella del controller"), ARTHUD::ViewerTeamIdOf(PC), 1);

	// E segue il campo, non lo copia: un accessore che avesse memorizzato il valore alla prima lettura
	// passerebbe l'asserzione qui sopra e fallirebbe questa.
	PC->PlayerTeamId = 0;
	TestEqual(TEXT("segue il campo, non una copia"), ARTHUD::ViewerTeamIdOf(PC), 0);

	// Un controller che NON e' un `ARTPlayerController` non porta il campo: e' il ramo del `Cast` fallito,
	// distinto dal `nullptr` e altrimenti mai percorso.
	APlayerController* Plain = World->SpawnActor<APlayerController>();
	if (TestNotNull(TEXT("controller generico"), Plain))
	{
		TestEqual(TEXT("controller non-RT: ripiega su 0"), ARTHUD::ViewerTeamIdOf(Plain), 0);
	}

	HvtDestroyWorld(World);
	return true;
}

/**
 * Senza controller si ripiega su `0`, e non su `INDEX_NONE`.
 *
 * E' il caso dell'editor, dei test e di ogni HUD non ancora posseduto. La regola e' la stessa di
 * `ARTGameMode::ViewerTeamId()` e di `ARTCameraPawn::FrameOwnTeam`: una sentinella qui obbligherebbe i sei
 * consumatori a gestirla, e nessuno di loro ha una risposta per «nessuna squadra».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudViewerTeamFallsBackWithoutControllerTest,
	"RefactorTactics.Hud.ViewerTeamFallsBackWithoutController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudViewerTeamFallsBackWithoutControllerTest::RunTest(const FString&)
{
	// Puro: nessun mondo, nessun Actor. La funzione e' statica apposta.
	TestEqual(TEXT("nessun controller: squadra 0"), ARTHUD::ViewerTeamIdOf(nullptr), 0);
	return true;
}
