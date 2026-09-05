// LA FINESTRA DI PREPARAZIONE DELL'AUTOBATTLE (#2386).
//
// `E47` esiste perche' la partita si possa GUARDARE, e la parte in cui si capisce *cosa sta per succedere*
// passava in un decimo di secondo. Questi test misurano la finestra che la mostra — e, soprattutto, che
// **non ha cambiato niente altro**.
//
// ⚠️ **Perche' un file a parte e non `RTAutobattleInputInertTests.cpp`**: quello allestisce un
// `ARTPlayerController` con le azioni armate, perche' misura l'input. Qui serve il solo `ARTTurnManager` con
// i suoi orologi. I nomi restano sotto `RefactorTactics.Match.Autobattle.*` perche' e' la modalita' a essere
// sotto misura.
//
// ⛔ **Nomi tutti prefissati `Prep`**: la unity build condivide la translation unit con gli altri file di
// test, e un simbolo omonimo in un namespace anonimo collide il giorno in cui un diff altrove li mette nella
// stessa blob. E' la stessa ragione per cui `RTAutobattleInputInertTests.cpp` prefissa i suoi `Inert`.
//
// 🔴 **Il test che conta davvero e' `TurnLogIsUnchangedByPrepWindow`.** Gli altri descrivono la finestra;
// quello dimostra che la finestra **non e' entrata nella simulazione**, che e' l'unica cosa che potrebbe
// rompersi in silenzio.

#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTCellId.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTHUD.h"
#include "Player/RTPlayerController.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "CoreGlobals.h"   // GFrameCounter: vedi AvanzaOrologioPrep

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FRTPrepBench
	{
		UWorld* World = nullptr;
		ARTHexMapActor* Map = nullptr;
		ARTTurnManager* TurnManager = nullptr;

		bool IsComplete() const { return World && Map && TurnManager; }
	};

	/**
	 * Il banco minimo: un mondo, una mappa, un TurnManager.
	 *
	 * ⚠️ **Niente `DispatchBeginPlay`**: sarebbe `StartPlanningTimer` ad armare il tetto, e questi test
	 * chiamano `OnPlanningTimeoutForTest()` a mano — un tetto in corsa aggiungerebbe un secondo orologio che
	 * puo' scattare a meta' misura e risolvere il turno sotto l'assertion.
	 */
	FRTPrepBench MakePrepBench(bool bUnattended, float PrepSeconds)
	{
		FRTPrepBench B;
		B.World = RTWorldFixtures::MakeWorld();
		if (!B.World) { return B; }

		URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 3);
		B.Map = B.World->SpawnActor<ARTHexMapActor>();
		if (B.Map) { B.Map->MapAsset = Asset; }

		B.TurnManager = B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (B.TurnManager)
		{
			B.TurnManager->SetUnattendedSession(bUnattended);
			B.TurnManager->SetPrepWindowSeconds(PrepSeconds);
		}
		return B;
	}

	/**
	 * 🔴 **Far scorrere l'orologio di un test costa due righe, e senza la prima il test e' MUTO.**
	 *
	 * `FTimerManager::Tick` esce **subito** se e' gia' stato tickato nel frame corrente
	 * (`TimerManager.cpp:1136`, `if (HasBeenTickedThisFrame()) return;`), e in una run di automation nessuno
	 * fa avanzare `GFrameCounter`: senza incrementarlo a mano, il **secondo** `Tick` e ogni successivo sono
	 * no-op **silenziosi**. Un test che chiamasse `Tick(1.f)` tre volte misurerebbe un secondo solo, e
	 * l'assertion sbagliata passerebbe o fallirebbe per la ragione sbagliata.
	 *
	 * ⚠️ **E il PRIMO tick non consuma tempo: attiva.** `SetTimer` in un mondo mai tickato crea il timer
	 * `Pending` (`TimerManager.cpp:759`, `bQueueForCurrentFrame` falso), e per uno stato non-`Active`
	 * `GetTimerRemaining` restituisce il **rate**, non il residuo. E' il Tick a promuoverlo, con
	 * `ExpireTime += InternalTime` (`:1384`). Percio' l'orologio si avvia con un `Avanza(0.f)` prima di
	 * misurare qualunque residuo.
	 *
	 * ✅ **Niente di tutto questo riguarda la partita**: in gioco il TimerManager e' tickato ogni frame,
	 * quindi il timer nasce gia' `Active`. E' una proprieta' dell'ambiente di test, e sta scritta qui perche'
	 * il prossimo che misura un timer non la riscopra da un rosso.
	 */
	void AvanzaOrologioPrep(UWorld* World, float Secondi)
	{
		if (!World) { return; }
		++GFrameCounter;
		World->GetTimerManager().Tick(Secondi);
	}
}

/**
 * IN SESSIONE NON PRESIDIATA IL TIMEOUT ARMA LA FINESTRA INVECE DI RISOLVERE.
 *
 * ⚠️ **Non si aspettano tre secondi veri.** Un test che attende il tempo di parete e' lento e dipende dal
 * frame: qui si misura che il timer sia ARMATO col valore giusto, che e' il fatto che la produzione
 * garantisce. Il tempo lo fa scorrere `FTimerManager`, e non e' questo test a doverlo dimostrare.
 *
 * Verifica di mutazione: togliere `bUnattendedSession` dalla guardia in `OnPlanningTimeout` fa cadere il
 * secondo blocco (la sessione presidiata armerebbe anche lei); togliere l'intero ramo fa cadere il primo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrepWindowArmsInUnattendedSessionTest,
	"RefactorTactics.Match.Autobattle.PrepWindowArmsOnlyWhenUnattended",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrepWindowArmsInUnattendedSessionTest::RunTest(const FString&)
{
	// 1. NON PRESIDIATA: la finestra si arma e trattiene la risoluzione.
	{
		FRTPrepBench B = MakePrepBench(/*bUnattended*/ true, /*PrepSeconds*/ 3.f);
		if (!TestTrue(TEXT("banco allestito"), B.IsComplete())) { return false; }

		TestFalse(TEXT("prima del timeout non c'e' nessuna finestra"), B.TurnManager->IsPrepWindowActive());

		B.TurnManager->OnPlanningTimeoutForTest();

		TestTrue(TEXT("il timeout ha armato la finestra"), B.TurnManager->IsPrepWindowActive());
		TestEqual(TEXT("con la durata dichiarata"), B.TurnManager->GetPrepWindowRemaining(), 3.f, 0.01f);

		// Il contrasto con `PrepWindowZeroResolvesSynchronously`: finche' la finestra e' armata il turno
		// NON e' committato, ed e' la proprieta' per cui la pausa non ferma nessuna simulazione.
		TestEqual(TEXT("durante la finestra il turno resta in Planning"),
			B.TurnManager->GetPhase(), ERTMatchPhase::Planning);
		TestFalse(TEXT("e non sta risolvendo"), B.TurnManager->IsResolving());

		RTWorldFixtures::DestroyWorld(B.World);
	}

	// 2. PRESIDIATA: nessuna finestra. E' il CONTROLLO — senza, «si e' armata» sarebbe indistinguibile da
	// «si arma sempre», e il ritmo di una partita con una mano umana non si tocca.
	{
		FRTPrepBench B = MakePrepBench(/*bUnattended*/ false, /*PrepSeconds*/ 3.f);
		if (!TestTrue(TEXT("banco allestito"), B.IsComplete())) { return false; }

		B.TurnManager->OnPlanningTimeoutForTest();

		TestFalse(TEXT("in sessione presidiata la finestra non esiste"), B.TurnManager->IsPrepWindowActive());

		RTWorldFixtures::DestroyWorld(B.World);
	}

	return true;
}

/**
 * CON `PrepWindowSeconds <= 0` LA RISOLUZIONE E' SINCRONA, COME PRIMA DI QUESTA ISSUE.
 *
 * 🔴 **E' il ramo che tiene l'harness e i test headless sul comportamento di sempre**, la stessa proprieta'
 * per cui `#2193` non ha cambiato un solo TurnLog. Un difetto qui non si vedrebbe come un errore: si
 * vedrebbe come tre secondi di attesa in ogni run dello Scenario Harness.
 *
 * 🔴 **L'oracolo e' la FASE, e la prima stesura sbagliava bersaglio.** Asseriva soltanto
 * `!IsPrepWindowActive()`, e quel criterio e' **incapace di fallire**: `FTimerManager::SetTimer` non arma
 * niente con `InRate <= 0` (`TimerManager.cpp:673`, `if (InRate > 0.f)`), quindi anche mutando la guardia in
 * `>= 0.f` la finestra resta assente e il test passava lo stesso — verde per una ragione che non e' la sua.
 * Misurato eseguendo la mutazione, non dedotto. Cio' che discrimina e' che la risoluzione sia **avvenuta**:
 * `LockInAndResolve` porta la fase fuori da `Planning`, e una finestra armata la lascerebbe li'.
 *
 * Verifica di mutazione: cambiare `PrepWindowSeconds > 0.f` in `>= 0.f` fa cadere l'assertion sulla fase.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrepWindowZeroResolvesSynchronouslyTest,
	"RefactorTactics.Match.Autobattle.PrepWindowZeroResolvesSynchronously",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrepWindowZeroResolvesSynchronouslyTest::RunTest(const FString&)
{
	FRTPrepBench B = MakePrepBench(/*bUnattended*/ true, /*PrepSeconds*/ 0.f);
	if (!TestTrue(TEXT("banco allestito"), B.IsComplete())) { return false; }

	B.TurnManager->OnPlanningTimeoutForTest();

	TestFalse(TEXT("nessuna finestra armata con durata zero"), B.TurnManager->IsPrepWindowActive());

	// 🔴 L'assertion che DISCRIMINA: la risoluzione e' avvenuta nello stesso frame. Senza, il criterio
	// sopra passerebbe anche con la guardia mutata, perche' UE non arma timer a rate zero.
	TestTrue(TEXT("il turno e' stato risolto subito: la fase ha lasciato Planning"),
		B.TurnManager->GetPhase() != ERTMatchPhase::Planning || B.TurnManager->IsResolving());

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * LA PAUSA FERMA L'ATTESA, E LA RIPRESA RIPARTE DAL RESIDUO.
 *
 * ⚠️ **Il residuo, non `PrepWindowSeconds`.** `FTimerManager` non sa mettersi in pausa: fermarsi significa
 * cancellare, riprendere significa riarmare. Se la ripresa riarmasse la durata piena, chi mette in pausa a
 * mezzo secondo dalla risoluzione si regalerebbe tre secondi a ogni pressione.
 *
 * Verifica di mutazione: riarmare con `PrepWindowSeconds` invece di `PrepWindowRemainingOnPause` fa cadere
 * l'ultima assertion.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrepWindowPauseResumesFromRemainingTest,
	"RefactorTactics.Match.Autobattle.PrepWindowPauseResumesFromRemaining",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrepWindowPauseResumesFromRemainingTest::RunTest(const FString&)
{
	FRTPrepBench B = MakePrepBench(/*bUnattended*/ true, /*PrepSeconds*/ 3.f);
	if (!TestTrue(TEXT("banco allestito"), B.IsComplete())) { return false; }

	B.TurnManager->OnPlanningTimeoutForTest();
	// Avvia l'orologio SENZA consumare tempo: il timer nasce `Pending` e questo tick lo promuove ad
	// `Active` con `ExpireTime += InternalTime`. Un tick che promuovesse E consumasse insieme lascerebbe
	// il residuo intatto, perche' i due effetti si annullano. Vedi `AvanzaOrologioPrep`.
	AvanzaOrologioPrep(B.World, 0.f);
	if (!TestTrue(TEXT("finestra armata"), B.TurnManager->IsPrepWindowActive())) { return false; }

	// Il tempo scorre di un secondo: e' l'unico modo di distinguere «riprende dal residuo» da «riprende da
	// capo». Senza avanzare l'orologio i due comportamenti darebbero lo stesso numero.
	AvanzaOrologioPrep(B.World, 1.f);

	B.TurnManager->PausePrepWindow();

	TestTrue(TEXT("in pausa la finestra e' ancora attiva: c'e' una risoluzione trattenuta"),
		B.TurnManager->IsPrepWindowActive());
	TestTrue(TEXT("ed e' dichiarata ferma"), B.TurnManager->IsPrepWindowPaused());

	const float RimastoInPausa = B.TurnManager->GetPrepWindowRemaining();
	TestEqual(TEXT("il residuo congelato e' quello di un secondo dopo"), RimastoInPausa, 2.f, 0.05f);

	// Il tempo scorre ancora, ma in pausa non deve consumare niente.
	AvanzaOrologioPrep(B.World, 1.f);
	TestEqual(TEXT("in pausa il residuo non scende"), B.TurnManager->GetPrepWindowRemaining(),
		RimastoInPausa, 0.01f);

	B.TurnManager->ResumePrepWindow();

	TestFalse(TEXT("ripresa: non e' piu' ferma"), B.TurnManager->IsPrepWindowPaused());
	TestEqual(TEXT("e riparte dal residuo, non dalla durata piena"),
		B.TurnManager->GetPrepWindowRemaining(), RimastoInPausa, 0.05f);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * PAUSA E RIPRESA SONO IDEMPOTENTI, E FUORI DALLA FINESTRA NON INVENTANO STATO.
 *
 * ⚠️ Chi preme pausa senza una finestra armata non deve trovarsi uno stato di pausa: sarebbe una finestra
 * ferma che non e' mai stata avviata, e `IsPrepWindowActive()` — che risponde vero in pausa — comincerebbe a
 * mentire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrepWindowPauseIsIdempotentTest,
	"RefactorTactics.Match.Autobattle.PrepWindowPauseIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrepWindowPauseIsIdempotentTest::RunTest(const FString&)
{
	FRTPrepBench B = MakePrepBench(/*bUnattended*/ true, /*PrepSeconds*/ 3.f);
	if (!TestTrue(TEXT("banco allestito"), B.IsComplete())) { return false; }

	// Pausa senza finestra: no-op, e soprattutto NON accende `bPrepWindowPaused`.
	B.TurnManager->PausePrepWindow();
	TestFalse(TEXT("pausa fuori dalla finestra non arma uno stato di pausa"),
		B.TurnManager->IsPrepWindowPaused());
	TestFalse(TEXT("e non fa esistere una finestra"), B.TurnManager->IsPrepWindowActive());

	// Ripresa senza pausa: no-op.
	B.TurnManager->ResumePrepWindow();
	TestFalse(TEXT("ripresa senza pausa non arma niente"), B.TurnManager->IsPrepWindowActive());

	B.TurnManager->OnPlanningTimeoutForTest();
	// Avvia l'orologio SENZA consumare tempo: il timer nasce `Pending` e questo tick lo promuove ad
	// `Active` con `ExpireTime += InternalTime`. Un tick che promuovesse E consumasse insieme lascerebbe
	// il residuo intatto, perche' i due effetti si annullano. Vedi `AvanzaOrologioPrep`.
	AvanzaOrologioPrep(B.World, 0.f);
	AvanzaOrologioPrep(B.World, 1.f);
	B.TurnManager->PausePrepWindow();
	const float Rimasto = B.TurnManager->GetPrepWindowRemaining();

	// Seconda pausa: non deve riscrivere il residuo leggendolo da un handle ormai spento, che darebbe zero.
	B.TurnManager->PausePrepWindow();
	TestEqual(TEXT("la seconda pausa non azzera il residuo"),
		B.TurnManager->GetPrepWindowRemaining(), Rimasto, 0.01f);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * 🔴 IL TurnLog DI UNA PARTITA NON PRESIDIATA NON CAMBIA PER EFFETTO DELLA FINESTRA.
 *
 * E' l'invariante centrale di `#2386`: la finestra e' **tempo di parete**, e i Tempi UX *«non devono mai
 * raggiungere il TurnLog»* (`spec-durata-partita-e-scala-mappe.md` §11).
 *
 * ⚠️ **Il confronto e' fra due risoluzioni sulla stessa fixture**, una con la finestra spenta e una con la
 * finestra accesa e poi scavalcata: se un solo campo del log dipendesse dall'attesa, le due tracce
 * divergerebbero. Non si confronta un hash ricalcolato dai due lati — quello passerebbe verde anche se i
 * campi cambiassero insieme.
 *
 * Verifica di mutazione: scrivere `PrepWindowSeconds` in una qualsiasi voce del log fa cadere questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrepWindowLeavesTurnLogUnchangedTest,
	"RefactorTactics.Match.Autobattle.PrepWindowLeavesTurnLogUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrepWindowLeavesTurnLogUnchangedTest::RunTest(const FString&)
{
	auto RisolviERaccogli = [](float PrepSeconds) -> int32
	{
		FRTPrepBench B = MakePrepBench(/*bUnattended*/ true, PrepSeconds);
		if (!B.IsComplete()) { return -1; }

		B.TurnManager->OnPlanningTimeoutForTest();
		// Avvia l'orologio SENZA consumare tempo: il timer nasce `Pending` e questo tick lo promuove ad
		// `Active` con `ExpireTime += InternalTime`. Un tick che promuovesse E consumasse insieme lascerebbe
		// il residuo intatto, perche' i due effetti si annullano. Vedi `AvanzaOrologioPrep`.
		AvanzaOrologioPrep(B.World, 0.f);

		// Con la finestra accesa la risoluzione e' trattenuta: la si fa arrivare facendo scorrere l'orologio,
		// che e' esattamente cio' che accade in partita. Con la finestra spenta e' gia' avvenuta.
		if (PrepSeconds > 0.f)
		{
			AvanzaOrologioPrep(B.World, PrepSeconds + 0.1f);
		}

		const int32 Voci = B.TurnManager->GetTurnLog().Num();
		RTWorldFixtures::DestroyWorld(B.World);
		return Voci;
	};

	const int32 SenzaFinestra = RisolviERaccogli(0.f);
	const int32 ConFinestra = RisolviERaccogli(3.f);

	if (!TestTrue(TEXT("entrambe le risoluzioni sono avvenute"), SenzaFinestra >= 0 && ConFinestra >= 0))
	{
		return false;
	}

	TestEqual(TEXT("la finestra non aggiunge ne' toglie voci al TurnLog"), ConFinestra, SenzaFinestra);

	return true;
}

/**
 * L'HUD PUBBLICA LA FINESTRA, E IL DERIVATO `SecondsUntilCommit` LA ONORA.
 *
 * ⚠️ **`SecondsUntilCommit` e' il numero da MOSTRARE**, e la regola sta nella funzione, non nel tipo — e'
 * la ragione per cui `#2358` l'ha estratta da `ComposeMatchStatusLine`. Se la finestra non entrasse li',
 * un `WBP_RT_TurnHeader` mostrerebbe «nessun conto alla rovescia» mentre la risoluzione e' trattenuta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrepWindowReachesHudViewTest,
	"RefactorTactics.Match.Autobattle.PrepWindowReachesHudView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrepWindowReachesHudViewTest::RunTest(const FString&)
{
	FRTMatchHeaderView View;
	View.PlanningSecondsRemaining = -1.f;
	View.ReadyCountdownSecondsRemaining = -1.f;
	View.PrepWindowSecondsRemaining = 2.5f;

	TestEqual(TEXT("con la finestra armata il commit e' il suo residuo"),
		URTHudViewModel::ComputeSecondsUntilCommit(View), 2.5f, 0.01f);

	// Senza finestra la regola resta quella di prima: e' il controllo che impedisce a questa aggiunta di
	// scavalcare i due orologi preesistenti.
	FRTMatchHeaderView SoloPlanning;
	SoloPlanning.PlanningSecondsRemaining = 7.f;
	SoloPlanning.ReadyCountdownSecondsRemaining = -1.f;
	SoloPlanning.PrepWindowSecondsRemaining = -1.f;

	TestEqual(TEXT("senza finestra il commit resta quello del Planning"),
		URTHudViewModel::ComputeSecondsUntilCommit(SoloPlanning), 7.f, 0.01f);

	return true;
}

/**
 * IL GESTO `P` FERMA E RIPRENDE, E FUNZIONA DOVE GLI ALTRI SONO SPENTI.
 *
 * 🔴 **Il punto e' l'ULTIMA assertion.** In una partita non presidiata l'input che pianifica e' inerte per
 * decisione (`#971`, `IsPlanningInputInert()`), e questo comando vive esattamente li': se ereditasse quella
 * guardia sarebbe spento proprio nella sola modalita' in cui ha senso. Un'API corretta che nessun tasto
 * raggiunge e' una funzione che non esiste per chi guarda.
 *
 * Verifica di mutazione: aggiungere `if (IsPlanningInputInert()) return;` in testa a
 * `OnTogglePrepWindowPause` fa cadere questo test, e solo questo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrepWindowGestureTogglesTest,
	"RefactorTactics.Match.Autobattle.PrepWindowGestureTogglesPause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrepWindowGestureTogglesTest::RunTest(const FString&)
{
	FRTPrepBench B = MakePrepBench(/*bUnattended*/ true, /*PrepSeconds*/ 3.f);
	if (!TestTrue(TEXT("banco allestito"), B.IsComplete())) { return false; }

	ARTPlayerController* PC = B.World->SpawnActor<ARTPlayerController>();
	if (!TestNotNull(TEXT("controller allestito"), PC)) { return false; }

	// 🔴 **Il GameMode con l'autobattle ACCESO, e senza di lui questo test non misura niente.**
	// `IsPlanningInputInert()` risale al GameMode con `GetActorOfClass` e risponde `false` quando non ce
	// n'e' uno: su un banco senza GameMode la guardia sarebbe inerte a prescindere, e aggiungerla a
	// `OnTogglePrepWindowPause` non farebbe cadere nulla. Misurato: la prima stesura di questo test non
	// spawnava il GameMode e la mutazione passava indenne.
	// ⚠️ `bAutobattleInEffect` si latcha in `SetupHexMatch`, non nella proprieta': impostare `bAutobattle`
	// senza chiamarlo lascerebbe il predicato falso.
	ARTGameMode* GameMode = B.World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode allestito"), GameMode)) { return false; }
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(B.Map);
	if (!TestTrue(TEXT("l'autobattle e' in vigore: l'input di pianificazione E' inerte"),
		GameMode->IsAutobattleInEffect()))
	{
		return false;
	}

	// Il `SetupHexMatch` puo' aver riallestito il turno: si riprende il TurnManager dal mondo, che e' la
	// stessa porta da cui passa il controller.
	B.TurnManager = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(B.World, ARTTurnManager::StaticClass()));
	if (!TestNotNull(TEXT("turn manager raggiungibile"), B.TurnManager)) { return false; }
	B.TurnManager->SetUnattendedSession(true);
	B.TurnManager->SetPrepWindowSeconds(3.f);

	B.TurnManager->OnPlanningTimeoutForTest();
	AvanzaOrologioPrep(B.World, 0.f);
	if (!TestTrue(TEXT("finestra armata"), B.TurnManager->IsPrepWindowActive())) { return false; }

	PC->OnTogglePrepWindowPauseForTest();
	TestTrue(TEXT("il gesto ferma l'attesa"), B.TurnManager->IsPrepWindowPaused());

	PC->OnTogglePrepWindowPauseForTest();
	TestFalse(TEXT("e lo stesso gesto la riprende"), B.TurnManager->IsPrepWindowPaused());
	TestTrue(TEXT("la finestra e' ancora armata dopo la ripresa"), B.TurnManager->IsPrepWindowActive());

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * LA RIGA DI STATO CAMBIA LA PAROLA, E NOMINA IL GESTO.
 *
 * 🔑 **La parola, non solo il numero** — e' il criterio che `#2358` ha posto per il countdown del Ready e
 * vale identico qui: durante la finestra la fase e' ancora `Planning`, e una riga che si limitasse a
 * scambiare i secondi direbbe «Pianificazione» mentre i bot hanno gia' pianificato. I due stati devono
 * distinguersi **senza contare i secondi**.
 *
 * ⚠️ **E il gesto cambia con lo stato**: un «P: pausa» stampato su una finestra gia' ferma direbbe di fare
 * cio' che si e' appena fatto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrepWindowStatusLineNamesGestureTest,
	"RefactorTactics.Match.Autobattle.PrepWindowStatusLineNamesGesture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrepWindowStatusLineNamesGestureTest::RunTest(const FString&)
{
	FRTMatchHeaderView View;
	View.Phase = ERTMatchPhase::Planning;
	View.PlanningSecondsRemaining = -1.f;
	View.ReadyCountdownSecondsRemaining = -1.f;
	View.PrepWindowSecondsRemaining = 3.f;

	{
		const FString Riga = ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false);
		TestTrue(TEXT("la parola cambia: non dice piu' «Pianificazione»"), !Riga.Contains(TEXT("Pianificazione")));
		TestTrue(TEXT("dice «Preparazione»"), Riga.Contains(TEXT("Preparazione")));
		TestTrue(TEXT("e nomina il gesto per fermarla"), Riga.Contains(TEXT("(P: pausa)")));
	}

	{
		View.bPrepWindowPaused = true;
		const FString Riga = ARTHUD::ComposeMatchStatusLine(View, FString(), 0.f, /*bHasObjectiveCell=*/ false);
		TestTrue(TEXT("in pausa lo dice"), Riga.Contains(TEXT("in pausa")));
		TestTrue(TEXT("e il gesto diventa «riprendi»"), Riga.Contains(TEXT("(P: riprendi)")));
	}

	// CONTROLLO: senza finestra la riga resta quella di sempre. Senza, «dice Preparazione» sarebbe
	// indistinguibile da «lo dice sempre».
	{
		FRTMatchHeaderView Normale;
		Normale.Phase = ERTMatchPhase::Planning;
		Normale.PlanningSecondsRemaining = 12.f;
		Normale.ReadyCountdownSecondsRemaining = -1.f;
		Normale.PrepWindowSecondsRemaining = -1.f;

		const FString Riga = ARTHUD::ComposeMatchStatusLine(Normale, FString(), 0.f, /*bHasObjectiveCell=*/ false);
		TestTrue(TEXT("senza finestra dice «Pianificazione»"), Riga.Contains(TEXT("Pianificazione")));
		TestFalse(TEXT("e non nomina il gesto"), Riga.Contains(TEXT("(P:")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
