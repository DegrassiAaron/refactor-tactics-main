#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Map/RTHexMapActor.h"
#include "Perception/RTKnowledgeVeilPresenter.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `#2876` — **il velo segue il movimento**: il target si aggiorna sui confini di micro-step del playback,
 * non solo ai due punti di refresh della conoscenza.
 *
 * ## Il difetto
 *
 * L'ordine delle fasi e' `Planning → Prep → Dash → Blast → Move → Cleanup`, e i due refresh —
 * `RefreshTeamKnowledgeForPlanning` e `RefreshTeamKnowledgeForBlast` — cadono **fuori** dalla fase `Move`.
 * ∴ il velo restava congelato sulle posizioni pre-`Move` per tutta la durata visiva del movimento, e
 * cambiava in un colpo solo a fine turno. L'attenuazione di `#2875` da sola non lo chiude: smussa un salto
 * che avviene comunque a movimento gia' finito.
 *
 * ## ⛔ Cosa NON e'
 *
 * **Non e' un terzo punto di refresh della conoscenza canonica.** `AdvancePlaybackKnowledge` scrive
 * `PlaybackKnowledgeState`, che e' presentazione: non entra nello snapshot, nel `TurnLog` ne' nello
 * `StateHash`, e `OnTeamKnowledgeRefreshed` non emette. `Veil.FollowsRefreshPoints` resta verde per
 * costruzione, e questo file non lo duplica — quel test c'e' gia' e sorveglia gia' quella proprieta'.
 */

namespace
{
	/** Una partita vera col velo agganciato: e' il percorso che il gioco esegue, non una scorciatoia. */
	struct FRTFollowFixture
	{
		UWorld* World = nullptr;
		ARTTurnManager* TurnManager = nullptr;
		URTKnowledgeVeilPresenter* Presenter = nullptr;
	};

	bool MakeFollowFixture(FRTFollowFixture& Out)
	{
		Out.World = RTWorldFixtures::MakeWorld();
		if (!Out.World) { return false; }

		// 🔴 Senza, il delegate dinamico del GameMode non arriva: `AActor::ProcessEvent` scarta ogni evento
		// se `AreActorsInitialized()` e' falso. E' la lezione di `#939`, gia' registrata due volte.
		Out.World->InitializeActorsForPlay(FURL());

		ARTHexMapActor* HexMap = Out.World->SpawnActor<ARTHexMapActor>();
		Out.TurnManager = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTGameMode* GameMode = Out.World->SpawnActor<ARTGameMode>();
		if (!HexMap || !Out.TurnManager || !GameMode) { return false; }

		GameMode->bAutobattle = true; // i bot muovono da soli: e' il movimento che il velo deve seguire
		GameMode->SetupHexMatch(HexMap);
		GameMode->HookKnowledgeVeil();
		Out.Presenter = GameMode->GetKnowledgeVeilPresenter();
		return Out.Presenter != nullptr;
	}

	/**
	 * Gioca dei turni lasciando scorrere il playback: e' li' che i confini di micro-step esistono.
	 *
	 * ⚠️ **Piu' di un turno, ed e' una correzione pagata dalla misura.** Con un turno solo i bot di
	 * questa fixture non producono nessun movimento da riprodurre, `IsResolving()` e' falso subito e il
	 * ciclo spende **zero** tick: il test cadeva sulla propria premessa. E' la stessa ragione per cui
	 * `Veil.FollowsRefreshPoints` ne gioca due.
	 */
	int32 PlayTurnsWithPlayback(ARTTurnManager* TM, int32 Turni)
	{
		int32 Tick = 0;
		for (int32 T = 0; T < Turni && TM->GetPhase() != ERTMatchPhase::MatchEnded; ++T)
		{
			TM->LockInAndResolve();
			for (int32 I = 0; I < 400 && TM->IsResolving(); ++I, ++Tick)
			{
				TM->Tick(0.05f);
			}
		}
		return Tick;
	}
}

/**
 * 🔴 **IL TEST CHE RIPRODUCE IL DIFETTO**: durante il movimento il velo si ridipinge **piu' di una volta**.
 *
 * ⚠️ **Il contatore e' separato da `Applications` di proposito**, ed e' cio' che rende il test non vacuo:
 * `Applications` conta le stesure dai punti di refresh e cresce comunque. Se i due fossero lo stesso
 * numero, questo test passerebbe anche con il velo fermo per tutta la fase `Move`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilFollowsTheMoveTest,
	"RefactorTactics.Veil.FollowsTheMoveAcrossMicroSteps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilFollowsTheMoveTest::RunTest(const FString&)
{
	FRTFollowFixture Fx;
	if (!TestTrue(TEXT("la partita col velo agganciato si allestisce"), MakeFollowFixture(Fx)))
	{
		RTWorldFixtures::DestroyWorld(Fx.World);
		return false;
	}

	// Prima di giocare: nessun passo di playback. E' la premessa che rende misurabile il delta.
	TestEqual(TEXT("prima del turno il velo non ha seguito nessun movimento"),
		Fx.Presenter->GetPlaybackApplications(), 0);

	const int32 TickSpesi = PlayTurnsWithPlayback(Fx.TurnManager, /*Turni=*/ 3);

	// Anti-vacuita': senza playback non ci sono confini da attraversare, e il test misurerebbe il nulla.
	if (!TestTrue(TEXT("il playback ha davvero speso tick"), TickSpesi > 0))
	{
		RTWorldFixtures::DestroyWorld(Fx.World);
		return false;
	}

	const int32 Passi = Fx.Presenter->GetPlaybackApplications();
	AddInfo(FString::Printf(TEXT("il velo ha seguito il movimento %d volte, in %d tick di playback"),
		Passi, TickSpesi));

	TestTrue(*FString::Printf(
			TEXT("durante il movimento il velo si ridipinge PIU' DI UNA VOLTA (%d)"), Passi),
		Passi > 1);

	// ⚠️ E i due contatori restano distinti: le stesure dai refresh non contano i passi del playback.
	AddInfo(FString::Printf(TEXT("stesure dai punti di refresh: %d"), Fx.Presenter->GetApplications()));

	RTWorldFixtures::DestroyWorld(Fx.World);
	return true;
}

/**
 * 🔑 **Il velo del playback non mostra MAI piu' di quel che la squadra sa.**
 *
 * E' la direzione che conta: la conoscenza di playback parte dallo stato **pre-turno** e ci unisce solo
 * cio' che le pose animate hanno gia' osservato, quindi a fine movimento dev'essere **contenuta** in quella
 * canonica — che di quel transito ha gia' tutto ([D-379]).
 *
 * ⚠️ **L'asserzione e' il contenimento, non l'uguaglianza**, e la differenza va detta: l'accumulo canonico
 * gira anche su unita' che il playback non anima — chi muore durante il turno, e chi si muove senza che la
 * sua rotta sia osservabile — quindi l'uguaglianza esatta dipenderebbe da cosa succede nella partita e
 * renderebbe il test una scommessa. Il contenimento e' l'invariante di **privacy**, e vale sempre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilPlaybackNeverShowsMoreThanKnownTest,
	"RefactorTactics.Veil.PlaybackNeverShowsMoreThanTheTeamKnows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilPlaybackNeverShowsMoreThanKnownTest::RunTest(const FString&)
{
	FRTFollowFixture Fx;
	if (!TestTrue(TEXT("la partita col velo agganciato si allestisce"), MakeFollowFixture(Fx)))
	{
		RTWorldFixtures::DestroyWorld(Fx.World);
		return false;
	}

	PlayTurnsWithPlayback(Fx.TurnManager, /*Turni=*/ 3);

	const FRTTeamKnowledge DalPlayback = Fx.TurnManager->PlaybackKnowledgeForTeam(0);
	const FRTTeamKnowledge Canonica = Fx.TurnManager->KnowledgeForTeamPublic(0);

	const TSet<FRTCellId> Note(Canonica.ExploredCells);
	TArray<FRTCellId> InPiu;
	for (const FRTCellId& C : DalPlayback.ExploredCells)
	{
		if (!Note.Contains(C)) { InPiu.Add(C); }
	}

	AddInfo(FString::Printf(TEXT("playback: %d esplorate, %d visibili — canonica: %d esplorate, %d visibili"),
		DalPlayback.ExploredCells.Num(), DalPlayback.VisibleCells.Num(),
		Canonica.ExploredCells.Num(), Canonica.VisibleCells.Num()));

	TestEqual(*FString::Printf(
			TEXT("il playback non mostra nessuna cella che la squadra non conosca (%d in piu')"), InPiu.Num()),
		InPiu.Num(), 0);

	// Anti-vacuita': con una conoscenza canonica vuota il contenimento sarebbe vero per niente.
	TestTrue(TEXT("premessa: la squadra conosce qualcosa"), Canonica.ExploredCells.Num() > 0);

	RTWorldFixtures::DestroyWorld(Fx.World);
	return true;
}

/**
 * Il playback **non tocca l'esito canonico**: con la riproduzione spenta il turno finisce allo stesso modo.
 *
 * ⚠️ **Questo test non duplica `Match.Autobattle.DeterminismIsIndependentOfPlayback`**, che confronta due
 * partite intere sul `TurnLog`. Qui la domanda e' piu' stretta e riguarda `#2876`: la conoscenza
 * **canonica** dev'essere la stessa con e senza playback, cioe' lo stato di presentazione introdotto da
 * questa issue non deve essere rifluito nel gioco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilPlaybackDoesNotTouchCanonicalTest,
	"RefactorTactics.Veil.PlaybackStateDoesNotLeakIntoTheCanonicalKnowledge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilPlaybackDoesNotTouchCanonicalTest::RunTest(const FString&)
{
	FRTFollowFixture ConPlayback;
	if (!TestTrue(TEXT("prima partita allestita"), MakeFollowFixture(ConPlayback)))
	{
		RTWorldFixtures::DestroyWorld(ConPlayback.World);
		return false;
	}
	PlayTurnsWithPlayback(ConPlayback.TurnManager, /*Turni=*/ 3);
	const FRTTeamKnowledge A = ConPlayback.TurnManager->KnowledgeForTeamPublic(0);
	const int32 PassiA = ConPlayback.Presenter->GetPlaybackApplications();
	RTWorldFixtures::DestroyWorld(ConPlayback.World);

	FRTFollowFixture SenzaPlayback;
	if (!TestTrue(TEXT("seconda partita allestita"), MakeFollowFixture(SenzaPlayback)))
	{
		RTWorldFixtures::DestroyWorld(SenzaPlayback.World);
		return false;
	}
	SenzaPlayback.TurnManager->bEnablePlayback = false; // `ConcludeTurn` viene chiamata in linea
	for (int32 T = 0; T < 3 && SenzaPlayback.TurnManager->GetPhase() != ERTMatchPhase::MatchEnded; ++T)
	{
		SenzaPlayback.TurnManager->LockInAndResolve();
	}
	const FRTTeamKnowledge B = SenzaPlayback.TurnManager->KnowledgeForTeamPublic(0);
	const int32 PassiB = SenzaPlayback.Presenter->GetPlaybackApplications();
	RTWorldFixtures::DestroyWorld(SenzaPlayback.World);

	AddInfo(FString::Printf(TEXT("con playback: %d esplorate, %d passi — senza: %d esplorate, %d passi"),
		A.ExploredCells.Num(), PassiA, B.ExploredCells.Num(), PassiB));

	TestEqual(TEXT("la conoscenza canonica non dipende dal playback"),
		A.ExploredCells.Num(), B.ExploredCells.Num());

	// 🔑 Il discriminante: senza playback non c'e' nessun confine da attraversare, quindi nessun passo.
	TestEqual(TEXT("senza playback il velo non segue nessun movimento"), PassiB, 0);
	TestTrue(TEXT("mentre col playback lo segue"), PassiA > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
