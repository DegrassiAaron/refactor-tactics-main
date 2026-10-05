#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTKnowledgeVeilPresenter.h"
#include "Perception/RTPerceptionLibrary.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHUD.h"
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

namespace
{
	/**
	 * Fa avanzare la sessione finche' il playback non torna fermo. Ritorna i passi spesi, o `-1` se non si e'
	 * fermato entro il tetto: un `Step` che non arriva al proprio confine e' un difetto da dire, non un'attesa
	 * da allungare.
	 */
	int32 AvanzaFinoAlConfine(FRTScenarioSession& Session, const ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400; ++I)
		{
			if (TM->IsPlaybackPaused() || !TM->IsResolving()) { return I; }
			Session.Step(0.05f, /*bPumpTurnManager=*/ true);
		}
		return -1;
	}

	/** Un'osservazione: cio' che il playback ha mostrato, e cio' che il file dice che avrebbe dovuto mostrare. */
	struct FRTPassoOsservato
	{
		int32 Passo = 0;
		/** Vero sul confine raggiunto con `Step`; falso a 0.9 del segmento, raggiunto con un tick solo. */
		bool bAlConfine = true;
		bool bAnimata = false;
		FRTCellId Animata;
		FRTCellId Dichiarata;
		/** La cella di B1 e' fra le visibili della squadra di A1, nella conoscenza di playback. */
		bool bVisibile = false;
		/** E l'HUD lo mostra davvero: `IsKnownToObserver()` dopo `UpdateObserverVeil()`. */
		bool bMostrato = false;
		bool bAtteso = false;

		bool IsGiusto() const
		{
			return bAnimata && Animata == Dichiarata && bVisibile == bAtteso && bMostrato == bAtteso;
		}

		FString Descrivi() const
		{
			return FString::Printf(
				TEXT("passo %d %s: A1 animata %s, dichiarata %s — B1 visibile %s, mostrato %s, atteso %s"),
				Passo, bAlConfine ? TEXT("al confine") : TEXT("a 0.9 del segmento"),
				bAnimata ? *Animata.ToString() : TEXT("-"), *Dichiarata.ToString(),
				bVisibile ? TEXT("si'") : TEXT("no"), bMostrato ? TEXT("si'") : TEXT("no"),
				bAtteso ? TEXT("si'") : TEXT("no"));
		}
	};

	/**
	 * `Visual.Perception.RevealDuringMove` a passo singolo — `#3458`. Il playback parte fermo; per ogni cella
	 * lo si riprende per UN tick lungo 0.9 del segmento, lo si ferma, e poi `StepMicroStep` lo porta al
	 * confine, come premendo `L`. A ogni fermata si registrano la cella animata di A1, la conoscenza di
	 * playback della sua squadra, e se l'HUD mostra B1.
	 *
	 * ⚠️ **L'oracolo sta nel FILE, non nel TurnManager**: la posa di ogni fermata e' quella che il file
	 * dichiara, e chi la vede lo dice la stessa funzione pura del gioco chiesta su quella posa. Leggere
	 * l'atteso da `AnimatedCellFor` renderebbe il test d'accordo con qualunque cella il playback creda di avere.
	 *
	 * 🔑 **Il campione a 0.9 e' la meta' che il confine non vede**: li' l'unita' e' ancora sulla cella di
	 * prima, e una tolleranza troppo grande nel calcolo della cella — mezza cella, un arrotondamento al piu'
	 * vicino — mostrerebbe B1 prima che A1 arrivi dove lo vede. Al confine si vede solo il verso dell'errore.
	 *
	 * L'ultimo confine non si registra: porta la fase alla fine, il playback si chiude, e da li' vale la
	 * conoscenza canonica — un'altra domanda, che questo banco non pone.
	 *
	 * @param CelleAlSecondo velocita' di locomozione del playback; non positiva = il default del TurnManager.
	 * @return `false` con un motivo se il percorso non arriva in fondo: perche' il banco non si allestisce, o
	 *         perche' il playback non si ferma dove deve. ⚠️ Le fermate osservate fino a li' restano in `Out`, e
	 *         il chiamante le giudica comunque: un difetto che interrompe il percorso non deve nascondere quelli
	 *         che l'hanno preceduto.
	 */
	bool PercorriAPassi(float CelleAlSecondo, TArray<FRTPassoOsservato>& Out, FString& Motivo)
	{
		FString Errore;
		const FString Percorso = URTScenarioIndex::ResolvePath(TEXT("Visual.Perception.RevealDuringMove"), Errore);
		FRTTestScenario Scenario;
		if (Percorso.IsEmpty() || !URTScenarioLoader::LoadFromFile(Percorso, Scenario, Errore))
		{
			Motivo = FString::Printf(TEXT("lo scenario non si carica: %s"), *Errore);
			return false;
		}

		// La posa di partenza di A1, i suoi waypoint e la cella di B1: tutto dal file.
		const FRTScenarioUnit* DichA1 =
			Scenario.Units.FindByPredicate([](const FRTScenarioUnit& U) { return U.Id == TEXT("A1"); });
		const FRTScenarioUnit* DichB1 =
			Scenario.Units.FindByPredicate([](const FRTScenarioUnit& U) { return U.Id == TEXT("B1"); });
		const FRTScenarioIntent* Cammino = Scenario.Turns.Num() > 0
			? Scenario.Turns[0].Intents.FindByPredicate([](const FRTScenarioIntent& I) { return I.UnitId == TEXT("A1"); })
			: nullptr;
		if (!(DichA1 && DichB1 && Cammino && Cammino->Move.Num() > 1))
		{
			Motivo = TEXT("il file non dichiara A1, B1 e il cammino di A1");
			return false;
		}
		TArray<FRTCellId> Pose;
		Pose.Add(DichA1->Cell);
		Pose.Append(Cammino->Move);

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World)
		{
			Motivo = TEXT("il mondo non si crea");
			return false;
		}

		FRTScenarioSession Session;
		Session.TurnPauseSeconds = 0.f;
		if (!Session.Start(World, Scenario))
		{
			Motivo = FString::Printf(TEXT("la sessione non parte: %s"), *Session.GetResult().ErrorMessage);
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}

		// Una sola uscita per tutte le strade che seguono: un mondo lasciato in piedi tiene vivi gli actor della
		// prova dopo, e qui le prove sono una per velocita'.
		const bool bPercorso = [&]() -> bool
		{
			ARTTurnManager* TM = nullptr;
			for (TActorIterator<ARTTurnManager> It(World); It; ++It) { TM = *It; break; }
			const ARTUnit* A1 = RTWorldFixtures::FirstUnitOfTeam(World, DichA1->TeamId);
			const ARTUnit* B1 = RTWorldFixtures::FirstUnitOfTeam(World, DichB1->TeamId);
			const ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
			// L'HUD vero: e' lui a decidere se B1 si vede, e senza giocatore guarda dalla squadra 0 (`TeamIdOf`).
			ARTHUD* Hud = World->SpawnActor<ARTHUD>();
			if (!(TM && A1 && B1 && Mappa && Mappa->MapAsset && Hud))
			{
				Motivo = TEXT("turn manager, A1, B1, mappa o HUD mancano");
				return false;
			}
			if (DichA1->TeamId != 0)
			{
				Motivo = TEXT("A1 deve stare nella squadra 0: e' da li' che l'HUD guarda senza un giocatore");
				return false;
			}

			if (CelleAlSecondo > 0.f)
			{
				TM->PlaybackCellsPerSecond = CelleAlSecondo;
			}
			const float Velocita = TM->PlaybackCellsPerSecond;

			// L'ordine non e' libero: `StartPaused` vale solo con i controlli abilitati.
			TM->SetPlaybackControlsEnabled(true);
			TM->SetStartPlaybackPaused(true);
			for (int32 I = 0; I < 400 && !(TM->IsResolving() && TM->IsPlaybackPaused()) && !Session.IsFinished(); ++I)
			{
				Session.Step(0.05f, /*bPumpTurnManager=*/ true);
			}
			if (!(TM->IsResolving() && TM->IsPlaybackPaused()))
			{
				Motivo = TEXT("il playback non e' partito fermo");
				return false;
			}

			// Chi vede cosa da una posa: la STESSA funzione pura del gioco, chiesta sulla posa che il file dichiara.
			const auto VedeB1Da = [&](const FRTCellId& Cella)
			{
				FRTPerceiver P;
				P.Cell = Cella;
				P.Facing = A1->Facing;
				P.VisionRange = A1->VisionRange;
				return URTPerceptionLibrary::VisibleCells(Mappa->MapAsset, P).Contains(DichB1->Cell);
			};

			// Premessa, ed e' cio' che rende il test non vacuo: e' lo scenario a dire che B1 compare al PRIMO
			// passo e non da dove A1 parte. Su una geometria in cui comparisse dalla partenza, un ritardo non si
			// vedrebbe.
			if (VedeB1Da(Pose[0]) || !VedeB1Da(Pose[1]))
			{
				Motivo = TEXT("premessa caduta: B1 deve essere invisibile dalla partenza e visibile dal primo passo");
				return false;
			}

			const auto Osserva = [&](int32 Passo, bool bAlConfine, const FRTCellId& Dichiarata)
			{
				FRTPassoOsservato O;
				O.Passo = Passo;
				O.bAlConfine = bAlConfine;
				O.bAnimata = TM->AnimatedCellFor(A1, O.Animata);
				O.Dichiarata = Dichiarata;
				O.bVisibile = TM->PlaybackKnowledgeForTeam(DichA1->TeamId).VisibleCells.Contains(DichB1->Cell);
				Hud->UpdateObserverVeil();
				O.bMostrato = B1->IsKnownToObserver();
				O.bAtteso = VedeB1Da(Dichiarata);
				Out.Add(O);
			};

			Osserva(0, /*bAlConfine=*/ true, Pose[0]);
			for (int32 K = 1; K < Pose.Num(); ++K)
			{
				// 0.9 del segmento con UN tick, non con `Step`: e' tempo che scorre, come in partita.
				TM->ResumePlayback();
				Session.Step(0.9f / Velocita, /*bPumpTurnManager=*/ true);
				TM->PausePlayback();
				if (!TM->IsResolving())
				{
					Motivo = FString::Printf(TEXT("passo %d: il playback si e' chiuso prima del confine"), K);
					return false;
				}
				Osserva(K, /*bAlConfine=*/ false, Pose[K - 1]);

				if (K == Pose.Num() - 1)
				{
					break; // l'ultimo confine chiude la fase: vedi il commento della funzione
				}
				TM->StepMicroStep();
				if (AvanzaFinoAlConfine(Session, TM) < 0 || !TM->IsResolving())
				{
					Motivo = FString::Printf(TEXT("passo %d: il playback non si ferma al confine"), K);
					return false;
				}
				Osserva(K, /*bAlConfine=*/ true, Pose[K]);
			}
			return true;
		}();

		Session.TearDown();
		RTWorldFixtures::DestroyWorld(World);
		return bPercorso;
	}
}

/**
 * `#3458` — **il nemico compare nel micro-step in cui l'unita' lo vede, non in un altro.**
 *
 * La seduta `U62` (2026-10-05) l'ha guardato a passo singolo su `Visual.Perception.RevealDuringMove`: A1
 * cammina da (-3,0) a (2,0) con la sola consapevolezza ravvicinata, B1 sta fermo a (0,-2), e la distanza fra
 * i due vale 3 · 2 · 2 · 2 · 3 · 4 lungo il percorso. Premendo `L`, A1 avanzava di una cella al primo passo
 * ma B1 compariva solo al SECONDO.
 *
 * ⚠️ **E' la scena della seduta alla velocita' di default, e va letta per cio' che e': verde PRIMA e dopo
 * la correzione di `#3458`.** Qui TurnManager e HUD danno la cella e la comparsa giuste a ogni fermata, quindi
 * il ritardo visto in `U62` non nasce da cio' che questo banco osserva. Il difetto che la correzione chiude
 * sta ad altre velocita': vedi il test qui sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilRevealsAtTheMicroStepThatSeesItTest,
	"RefactorTactics.Veil.RevealsAtTheMicroStepThatSeesIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilRevealsAtTheMicroStepThatSeesItTest::RunTest(const FString&)
{
	TArray<FRTPassoOsservato> Passi;
	FString Motivo;
	const bool bInFondo = PercorriAPassi(/*CelleAlSecondo=*/ 0.f, Passi, Motivo);
	TestTrue(FString::Printf(TEXT("il percorso a passo singolo arriva in fondo (%s)"), *Motivo), bInFondo);
	TestTrue(TEXT("almeno una fermata osservata"), Passi.Num() > 0);

	// Le fermate si giudicano anche se il percorso si e' interrotto: vedi `PercorriAPassi`.
	for (const FRTPassoOsservato& O : Passi)
	{
		AddInfo(O.Descrivi());
		TestTrue(FString::Printf(TEXT("%s — A1 e' sulla cella che il file dichiara"), *O.Descrivi()),
			O.bAnimata && O.Animata == O.Dichiarata);
		TestEqual(FString::Printf(TEXT("%s — B1 e' nella conoscenza se e solo se A1 lo vede da qui"), *O.Descrivi()),
			O.bVisibile, O.bAtteso);
		TestEqual(FString::Printf(TEXT("%s — e l'HUD lo mostra se e solo se A1 lo vede da qui"), *O.Descrivi()),
			O.bMostrato, O.bAtteso);
	}
	return true;
}

/**
 * `#3458` — **dopo `k` passi l'unita' e' sulla cella `k`, a QUALUNQUE velocita' di locomozione.**
 *
 * Il test qui sopra fissa il caso della seduta. Questo chiede la regola: `Step` si ferma al confine di un
 * micro-step, e un micro-step e' una cella. La velocita' non c'entra, quindi non deve poter cambiare la cella
 * su cui ci si ferma — ne' la conoscenza e la comparsa che velo e HUD ne ricavano.
 *
 * ⚠️ **Una griglia, non un valore scelto**: una velocita' presa perche' qualcuno ha visto che falliva sarebbe un
 * test di quel numero. La griglia va da `0.50` a `3.00` a passi di `0.05` e passa ACCANTO al default —
 * `1.40` e `1.45` — senza toccarlo: il default lo fissa il test qui sopra, e la regola pura la percorre
 * `Playback.MicroStepAtAlphaLandsOnTheBoundaryAStepStopsAt`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilRevealsAtTheMicroStepAtAnyRateTest,
	"RefactorTactics.Veil.RevealsAtTheMicroStepAtAnyPlaybackRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilRevealsAtTheMicroStepAtAnyRateTest::RunTest(const FString&)
{
	int32 Percorse = 0;
	for (int32 Centesimi = 50; Centesimi <= 300; Centesimi += 5)
	{
		const float CelleAlSecondo = Centesimi / 100.f;
		TArray<FRTPassoOsservato> Passi;
		FString Motivo;
		const bool bInFondo = PercorriAPassi(CelleAlSecondo, Passi, Motivo);

		// Prima le fermate, poi l'interruzione: il primo errore osservato e' spesso la causa del secondo.
		const FRTPassoOsservato* Sbagliato = Passi.FindByPredicate([](const FRTPassoOsservato& O) { return !O.IsGiusto(); });
		if (Sbagliato)
		{
			AddError(FString::Printf(TEXT("a %.2f celle/s — %s"), CelleAlSecondo, *Sbagliato->Descrivi()));
		}
		if (!bInFondo)
		{
			AddError(FString::Printf(TEXT("a %.2f celle/s il percorso si interrompe: %s"), CelleAlSecondo, *Motivo));
			continue;
		}
		++Percorse;
	}
	TestTrue(TEXT("almeno una velocita' percorsa"), Percorse > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
