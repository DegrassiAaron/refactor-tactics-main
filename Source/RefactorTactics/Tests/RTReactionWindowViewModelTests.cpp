#include "Misc/AutomationTest.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "RTGameMode.h"
#include "Tests/RTAbilityFixtures.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTReactionWindowViewModel.h"
#include "UI/RTScreenHudWidgets.h" // URTFastDecisionWidget: la meta' UI delle voci 2 · 3 · 4 di CP 14.6
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `#2723` — LA FINESTRA DI REAZIONE DIVENTA RAGGIUNGIBILE IN PARTITA.
 *
 * 🔴 **Il difetto che questi test chiudono e' un binding che non c'era.** `#2679` ha costruito la
 * sospensione nel movimento, `#2692` quella del `Brace`, `#2717` l'orologio che le fa scadere — e tutte e
 * tre erano irraggiungibili, perche' il ramo che apre la finestra e' protetto da
 * `OnReactionWindowOpened.IsBound()` e le sole occorrenze del binding stavano **nei test che lo legavano
 * apposta**. Una suite verde su un meccanismo che in partita nessuno invoca e' indistinguibile, da dentro,
 * da una suite verde su un meccanismo che funziona: e' la stessa lezione che `#1467` ha pagato per il velo.
 *
 * ⚠️ **Percio' il primo test misura il CABLAGGIO e non il meccanismo.** Chiama
 * `ARTGameMode::HookReactionWindow`, che e' lo stesso codice che `BeginPlay` esegue in partita, e osserva
 * che una finestra si apre **davvero**. Un `grep` che trovasse il binding non lo distinguerebbe da un
 * `Hook()` che nessuno chiama.
 *
 * ⛔ **E il secondo test misura il caso in cui il binding NON deve esistere**, che e' la meta' meno ovvia:
 * senza `ARTPlayerController` nessuno disegna e nessuno risponde, e una finestra aperta li' fermerebbe la
 * resolution per sempre.
 */
namespace
{
	/**
	 * 🔴 **Senza questa riga il cablaggio non trova nessun client, e il test fallisce dicendo un'altra cosa.**
	 *
	 * `HookReactionWindow` risolve il controller con `UGameplayStatics::GetPlayerController`, che iterA la
	 * `PlayerControllerList` del mondo. Un `APlayerController` vi si iscrive da
	 * `AController::PostInitializeComponents` — e `AActor::PostActorConstruction` chiama quella funzione
	 * **solo se `World->AreActorsInitialized()`**. In un mondo di prova non inizializzato il controller
	 * esiste, e' spawnato, e' valido, e `GetPlayerController` risponde `nullptr`: i sintomi sono quelli di un
	 * cablaggio rotto, e non lo e'.
	 *
	 * ⚠️ **Misurato il 2026-09-09**, sulla prima esecuzione di questa suite: cinque assertion rosse in cascata
	 * da una causa sola. `RTVeilTests` aveva gia' registrato la stessa famiglia di trappole per un'altra
	 * ragione — li' e' `ProcessEvent` che scarta gli eventi dinamici — e la riga e' la stessa.
	 *
	 * ⛔ **Non fa cominciare il gioco**: `DispatchBeginPlay` dipende da `World->HasBegunPlay()`, che resta
	 * falso. Gli actor vengono INIZIALIZZATI, non avviati, ed e' cio' che tiene questi test deterministici
	 * come quelli che guidano il turno a mano.
	 */
	void InitVmWorld(UWorld* World)
	{
		if (World)
		{
			World->InitializeActorsForPlay(FURL());
		}
	}

	ARTHexMapActor* SpawnVmMap(UWorld* World, int32 Radius = 8)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnVmUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}

	/**
	 * Lo scenario dell'Overwatch: un mover che entra nella zona di un watcher armato e non-bot.
	 *
	 * ⚠️ **`PlannedAbilityIndex` e non `PlannedReactionAbility`**, e il cono E' il facing: sono le due
	 * condizioni che `RTMovementResumeTests` ha gia' pagato di misurare — armare lo slot reazione non
	 * produce nessun watcher, e un cono orientato altrove non vede passare nessuno.
	 */
	void ArmOverwatchScenario(ARTUnit* Mover, ARTUnit* Watcher, bool bWatcherIsBot = false)
	{
		Watcher->bIsBotControlled = bWatcherIsBot;
		Watcher->PlannedAbilityIndex =
			RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
		Watcher->Facing = ERTHexDirection::W;
		Watcher->PlannedCell = Watcher->Cell;
		Mover->PlannedCell = FRTCellId(2, 0);
	}

	/** Lo scenario del `Brace`: una spinta contro un bracer non-bot con profilo a piu' risposte. */
	void ArmBraceScenario(ARTUnit* Bracer, ARTUnit* Pusher)
	{
		Bracer->bIsBotControlled = false;
		Bracer->ReactionProfileId = TEXT("Profile.Sidestep");
		Bracer->PlannedAbilityIndex =
			RTAbilityFixtures::AddCoreAbilityInSlot(Bracer, TEXT("Action.Brace"), 3);
		Bracer->PlannedCell = Bracer->Cell;

		Pusher->PlannedAbilityIndex =
			RTAbilityFixtures::AddCoreAbilityInSlot(Pusher, TEXT("Action.Push"), 3);
		Pusher->PlannedAttackTarget = Bracer;
		Pusher->PlannedCell = Pusher->Cell;
	}
}

/**
 * 🔴 **IL CABLAGGIO: `ARTGameMode` lega il delegate, e la finestra si apre in partita** (`#2723`).
 *
 * 🔑 **La prova non e' che il binding esista: e' che una finestra si apra.** La DoD della issue proponeva
 * *«un `grep` fuori da `Tests/` che continua a dare zero»* come falsificatore — e misura il testo del
 * sorgente, non il comportamento. Un `Hook()` scritto e mai chiamato passerebbe quel `grep` lasciando il
 * difetto esattamente dov'era. Qui si chiama il cablaggio di produzione e si guarda la resolution fermarsi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionVmProductionBindingTest,
	"RefactorTactics.Reactions.ViewModel.GameModeBindsTheWindowInProduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionVmProductionBindingTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ArmOverwatchScenario(Mover, Watcher);

	// --- 1. Lo stato di partenza, che rende non vacuo il punto 2 -------------------------------------
	TestFalse(TEXT("prima dell'aggancio il delegate NON e' legato (stato di partenza)"),
		TM->OnReactionWindowOpened.IsBound());

	// --- 2. IL CABLAGGIO: lo stesso codice che `BeginPlay` esegue ------------------------------------
	GameMode->HookReactionWindow();

	TestTrue(TEXT("dopo l'aggancio il delegate ha un binding di PRODUZIONE"),
		TM->OnReactionWindowOpened.IsBound());

	URTReactionWindowViewModel* ViewModel = PC->GetReactionWindowViewModel();
	if (!TestNotNull(TEXT("il controller possiede il view model"), ViewModel))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	TestFalse(TEXT("prima che una finestra si apra il view model non ne ha nessuna"),
		ViewModel->IsWindowOpen());

	// --- 3. LA PROVA: una finestra si apre DAVVERO --------------------------------------------------
	TM->LockInAndResolve();

	TestTrue(TEXT("la resolution si e' sospesa: una finestra attende"), TM->IsResolutionSuspended());
	TestTrue(TEXT("e il view model la sta tenendo"), ViewModel->IsWindowOpen());
	TestTrue(TEXT("la vista consegnata e' APERTA per chi decide"), ViewModel->GetWindow().bOpen);
	TestTrue(TEXT("e offre piu' di una risposta: e' un boundary vero"),
		ViewModel->GetWindow().Options.Num() > 1);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * ⛔ **SENZA UN CLIENT NON SI LEGA NIENTE, e non e' una cautela: e' il significato di `IsBound()`**
 * (`#2723`).
 *
 * 🔴 **E' la meta' che il precedente del velo avrebbe fatto sbagliare.** `ARTGameMode::HookKnowledgeVeil`
 * crea un presenter anche senza `ARTPlayerController` — *«harness headless, test di simulazione»* — perche'
 * la board e' nel mondo e il velo va steso lo stesso. Copiare quel ramo qui avrebbe legato il delegate in
 * **ogni** test che monta un GameMode: li' nessuno disegna, nessuno risponde e il `Tick` del manager puo'
 * non girare mai, quindi la prima finestra sospenderebbe la resolution senza che nessuno la riprenda.
 *
 * 🔑 `IsBound()` distingue «umano con UI» da «umano senza UI». Un ripiego senza proprietario gli farebbe
 * rispondere «c'e' una UI» proprio quando non c'e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionVmNoClientNoBindingTest,
	"RefactorTactics.Reactions.ViewModel.NoPlayerControllerLeavesTheDelegateUnbound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionVmNoClientNoBindingTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ArmOverwatchScenario(Mover, Watcher);

	// ⚠️ **Nessun `ARTPlayerController` in questo mondo**, ed e' la condizione del test.
	GameMode->HookReactionWindow();

	TestFalse(TEXT("senza client il delegate resta SLEGATO"), TM->OnReactionWindowOpened.IsBound());

	// 🔑 **E il modello sincrono resta identico a se stesso**: nessuna finestra, nessuna sospensione, il
	// turno si risolve come prima di questa issue.
	TM->LockInAndResolve();

	TestFalse(TEXT("nessuna finestra ha sospeso la resolution"), TM->IsResolutionSuspended());
	TestTrue(TEXT("l'id della finestra aperta e' vuoto: non ne e' mai stata aperta una"),
		TM->GetOpenReactionWindowId().IsEmpty());
	TestTrue(TEXT("il turno e' arrivato in fondo e ha prodotto un TurnLog"), TM->GetTurnLog().Num() > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Il tempo residuo e' leggibile, e lo conta l'AUTORITA'** (`#2723`, sopra l'orologio di `#2717`).
 *
 * 🔴 **Era il pezzo mancante che rendeva impossibile disegnare il countdown senza barare.**
 * `OpenWindowElapsed` e' privato, e fino a questa issue non esisteva un modo di chiedere *quanto manca*:
 * il widget avrebbe dovuto contarselo da solo — cioe' tenere una seconda verita' sul tempo. Un countdown
 * contato dal client e' un client che decide quando scade.
 *
 * ⚠️ **`-1` e non `0` quando nessuna finestra attende**: e' la convenzione gia' motivata da
 * `FRTMatchHeaderView::PlanningSecondsRemaining`, e qui i due significati sono entrambi reali.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionVmRemainingSecondsTest,
	"RefactorTactics.Reactions.ViewModel.RemainingSecondsFollowTheAuthoritativeClock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionVmRemainingSecondsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ArmOverwatchScenario(Mover, Watcher);

	// 🔑 **Prima che una finestra esista, la domanda non si applica.**
	TestTrue(TEXT("senza finestra il residuo del manager e' NEGATIVO"),
		TM->GetOpenReactionWindowRemainingSeconds() < 0.f);

	// Una durata corta rende il test rapido senza cambiare la regola: e' un setting di partita ([D-348]).
	TM->SetFastReactionDuration(0.5f);
	GameMode->HookReactionWindow();
	URTReactionWindowViewModel* ViewModel = PC->GetReactionWindowViewModel();

	TestTrue(TEXT("senza finestra il residuo del view model e' NEGATIVO"),
		ViewModel->GetRemainingSeconds() < 0.f);

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("una finestra attende"), TM->IsResolutionSuspended()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- Appena aperta: il residuo E' la durata intera ------------------------------------------------
	TestEqual(TEXT("appena aperta, restano i secondi della durata autorevole"),
		ViewModel->GetRemainingSeconds(), 0.5f, /*Tolerance=*/ 0.001f);

	// --- Scorre con l'orologio del gioco, e nessuno lo decrementa a mano ------------------------------
	TM->Tick(0.05f);
	TestTrue(TEXT("dopo un tick il residuo e' SCESO"), ViewModel->GetRemainingSeconds() < 0.5f);
	TestTrue(TEXT("e la finestra e' ancora aperta"), ViewModel->IsWindowOpen());

	// 🔑 **LA PROVA che il clamp serve.** Il residuo non deve MAI essere negativo mentre una finestra e'
	// aperta: `TickReactionWindow` somma il delta prima di confrontarlo, quindi l'ultimo tick porta
	// `Elapsed` oltre la durata. Senza clamp quell'istante sarebbe indistinguibile da «nessuna finestra».
	int32 Tick = 0;
	bool bResiduoNegativoConFinestraAperta = false;
	while (TM->IsResolutionSuspended() && Tick < 200)
	{
		if (TM->GetOpenReactionWindowRemainingSeconds() < 0.f)
		{
			bResiduoNegativoConFinestraAperta = true;
		}
		TM->Tick(0.05f);
		++Tick;
	}
	TestFalse(TEXT("con una finestra aperta il residuo non e' MAI negativo"),
		bResiduoNegativoConFinestraAperta);

	// --- Chiusa: si torna a «la domanda non si applica» ----------------------------------------------
	TestFalse(TEXT("scaduta la finestra, il view model non la dice piu' aperta"),
		ViewModel->IsWindowOpen());
	TestTrue(TEXT("e il residuo torna NEGATIVO"), ViewModel->GetRemainingSeconds() < 0.f);
	TestTrue(TEXT("e la vista torna ai default: niente da disegnare"),
		!ViewModel->GetWindow().bOpen);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Una risposta inoltrata chiude la finestra e riprende — SITO 1: l'Overwatch nel movimento** (`#2723`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionVmSubmitOverwatchTest,
	"RefactorTactics.Reactions.ViewModel.SubmitClosesAndResumesOverwatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionVmSubmitOverwatchTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	URTReactionWindowViewModel* ViewModel = PC->GetReactionWindowViewModel();

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("una finestra dell'Overwatch attende"), ViewModel->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// La scelta sicura e' una risposta legale come le altre, e il DTO la nomina invece di farla indovinare.
	const FString Sicura = ViewModel->GetWindow().SafeResponse;
	TestTrue(TEXT("il DTO dichiara la scelta sicura per nome"), !Sicura.IsEmpty());

	const FString IdInoltrata = TM->GetOpenReactionWindowId();

	// 🔑 **LA PROVA.** La risposta passa dal view model, non da `SubmitReactionResponse` a mano.
	ViewModel->SubmitResponse(Sicura);

	// 🔴 **L'osservabile e' l'IDENTITA', non «nessuna finestra aperta», e la differenza e' stata misurata:
	// la prima stesura di questo test asseriva `!IsWindowOpen()` ed e' andata rossa.** Chiudere una finestra
	// **riprende** la resolution, e la ripresa puo' aprirne subito un'altra sullo stesso boundary — che il
	// view model, essendo legato, riceve nello stesso stack. `IsWindowOpen()` torna vero, e ha ragione: c'e'
	// una finestra, e non e' piu' questa.
	//
	// ⚠️ **Il sito del `Brace` non lo mostra**, perche' quello scenario ne apre una sola: e' il motivo per
	// cui i due siti hanno due test e non uno parametrizzato.
	TestTrue(TEXT("la finestra inoltrata non e' piu' quella aperta: si e' chiusa"),
		TM->GetOpenReactionWindowId() != IdInoltrata);

	// E la resolution riprende: si porta il turno in fondo facendo scadere le finestre che restano.
	int32 Giri = 0;
	while (TM->IsResolutionSuspended() && Giri < 200)
	{
		TM->Tick(0.05f);
		++Giri;
	}
	TestFalse(TEXT("la resolution e' ripresa e conclusa"), TM->IsResolutionSuspended());
	TestTrue(TEXT("il turno ha prodotto un TurnLog"), TM->GetTurnLog().Num() > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Una risposta inoltrata chiude la finestra e riprende — SITO 2: il `Brace` nel Blast** (`#2723`).
 *
 * ⚠️ **Non e' lo stesso codice del sito 1, ed e' il motivo per cui questo test esiste separato.**
 * `SubmitReactionResponse` ha due rami e li ORDINA — il `Brace` per primo, con un `return` incondizionato —
 * e un test solo sul movimento lascerebbe scoperto proprio il ramo che `#2692` ha aggiunto per ultimo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionVmSubmitBraceTest,
	"RefactorTactics.Reactions.ViewModel.SubmitClosesAndResumesBrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionVmSubmitBraceTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Bracer = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 0);
	if (!TestNotNull(TEXT("Bracer"), Bracer) || !TestNotNull(TEXT("Pusher"), Pusher)
		|| !TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ArmBraceScenario(Bracer, Pusher);
	GameMode->HookReactionWindow();
	URTReactionWindowViewModel* ViewModel = PC->GetReactionWindowViewModel();

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("una finestra del `Brace` attende"), ViewModel->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// ➕ Il residuo si legge anche quando e' il `Brace` ad attendere: l'accessore percorre entrambi i siti.
	TestTrue(TEXT("il residuo del `Brace` e' leggibile e positivo"),
		ViewModel->GetRemainingSeconds() > 0.f);

	const FString Sicura = ViewModel->GetWindow().SafeResponse;
	ViewModel->SubmitResponse(Sicura);

	TestFalse(TEXT("la finestra del `Brace` e' CHIUSA"), ViewModel->IsWindowOpen());

	int32 Giri = 0;
	while (TM->IsResolutionSuspended() && Giri < 200)
	{
		TM->Tick(0.05f);
		++Giri;
	}
	TestFalse(TEXT("la resolution e' ripresa e conclusa"), TM->IsResolutionSuspended());

	// Allo scadere del `Brace` vale `Hold Ground`: la risposta sicura tiene la cella, e averla inoltrata
	// dal view model non cambia l'esito rispetto alla stessa risposta presa dalla via sincrona.
	TestTrue(TEXT("la scelta sicura ha tenuto la cella"), Bracer->Cell == FRTCellId(0, 0));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **UNA RISPOSTA CHE NOMINA UN'ALTRA FINESTRA NON CHIUDE NIENTE, anche dal canale umano** (`#2723`).
 *
 * 🔑 **E' il gate dell'identita' portato fin dentro il view model.** `SubmitResponse` nomina la finestra
 * per cui la vista e' stata costruita — non `GetOpenReactionWindowId()`, cioe' «qualunque finestra ci sia».
 * La differenza si vede solo qui: un view model che avesse registrato la finestra A e rispondesse mentre e'
 * aperta la B chiuderebbe **la B**, cioe' farebbe decidere il giocatore su un mondo che non c'e' piu'.
 *
 * ⚠️ **Il montaggio usa due view model perche' il delegate e' SINGLE-CAST**: legare il secondo sostituisce
 * il primo, e il primo resta con la finestra A in mano mentre il gioco e' passato alla B. E' l'unico modo
 * di produrre lo scarto senza inventare un setter che il codice di produzione non ha.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionVmStaleResponseTest,
	"RefactorTactics.Reactions.ViewModel.StaleResponseClosesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionVmStaleResponseTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	URTReactionWindowViewModel* Primo = PC->GetReactionWindowViewModel();

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("la finestra A attende"), Primo->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const FString IdA = TM->GetOpenReactionWindowId();

	// Il secondo view model prende il posto del primo: da qui in poi le finestre le riceve lui.
	URTReactionWindowViewModel* Secondo = NewObject<URTReactionWindowViewModel>(PC);
	Secondo->Hook(TM);

	// La finestra A scade da sola. Se la ripresa ne apre un'altra, la riceve `Secondo`.
	int32 Giri = 0;
	while (TM->IsResolutionSuspended() && TM->GetOpenReactionWindowId() == IdA && Giri < 200)
	{
		TM->Tick(0.05f);
		++Giri;
	}

	// 🔑 **Il primo view model non dice piu' aperta la sua finestra, e non ha ricevuto nessuna notifica.**
	// E' la meta' di questo test che vale anche se nessuna finestra B si apre: non esiste un
	// `OnReactionWindowClosed`, e un `bOpen` proprio sarebbe rimasto vero per sempre.
	TestFalse(TEXT("dopo la scadenza il primo view model NON dice piu' aperta la finestra A"),
		Primo->IsWindowOpen());

	if (TM->IsResolutionSuspended() && Secondo->IsWindowOpen())
	{
		// --- Lo scarto c'e' davvero: A e' chiusa, B e' aperta, e il primo risponde ancora per A ---------
		const FString IdB = TM->GetOpenReactionWindowId();
		TestTrue(TEXT("la finestra B e' un'altra finestra"), IdB != IdA);

		Primo->SubmitResponse(TEXT("HOLD"));

		TestTrue(TEXT("la risposta stale NON ha chiuso la finestra B"), Secondo->IsWindowOpen());
		TestEqual(TEXT("e la finestra aperta e' ancora la B"), TM->GetOpenReactionWindowId(), IdB);
	}
	else
	{
		// --- Nessuna finestra B: la risposta stale non deve comunque toccare niente --------------------
		AddInfo(TEXT("una sola finestra in questo scenario: si verifica il no-op invece dello scarto"));
		const bool bSospesaPrima = TM->IsResolutionSuspended();
		Primo->SubmitResponse(TEXT("HOLD"));
		TestEqual(TEXT("una risposta senza la sua finestra non cambia lo stato della resolution"),
			TM->IsResolutionSuspended(), bSospesaPrima);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * ⛔ **CON UN'UNITA' BOT IL RAMO INTERATTIVO NON SI PRENDE, e il modello sincrono resta identico**
 * (`#2723`).
 *
 * 🔑 **Il gate e' `!bOwnerIsBot`, e sta a monte del view model.** E' cio' che tiene bot, test e Verifier
 * sul percorso sincrono senza un ramo che li nomini — e in v0.1, con **un solo umano** ([D-155]), e' anche
 * cio' che rende il gate di squadra non necessario dentro il view model: nessuna finestra dell'avversario
 * puo' raggiungerlo, perche' l'avversario e' il bot.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionVmBotOwnerTest,
	"RefactorTactics.Reactions.ViewModel.BotOwnerNeverOpensAWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionVmBotOwnerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// ⚠️ **L'unica differenza col primo test di questo file**: il watcher e' del bot.
	ArmOverwatchScenario(Mover, Watcher, /*bWatcherIsBot=*/ true);

	GameMode->HookReactionWindow();
	URTReactionWindowViewModel* ViewModel = PC->GetReactionWindowViewModel();

	// Il delegate E' legato — c'e' un client, e la UI esiste. Cio' che non accade e' l'apertura.
	TestTrue(TEXT("il delegate e' legato: un client c'e'"), TM->OnReactionWindowOpened.IsBound());

	TM->LockInAndResolve();

	TestFalse(TEXT("nessuna finestra ha raggiunto il view model"), ViewModel->IsWindowOpen());
	TestFalse(TEXT("la resolution non si e' sospesa"), TM->IsResolutionSuspended());
	TestTrue(TEXT("il residuo dice «nessuna finestra»"), ViewModel->GetRemainingSeconds() < 0.f);
	TestTrue(TEXT("il turno si e' risolto per la via sincrona"), TM->GetTurnLog().Num() > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

// =====================================================================================================
// `URTFastDecisionWidget` — la meta' UI delle voci 2 · 3 · 4 di CP 14.6 (`#166`)
//
// ⚠️ **Perche' stanno in QUESTO file e non in `RTScreenHudWidgetTests.cpp`**: quei test allestiscono un
// mondo e un `ARTTurnManager` nudo, e nessuna finestra vi si apre. Qui c'e' la fixture che ne apre una vera
// — mappa, mover, watcher armato, cablaggio di produzione — e duplicarla costerebbe piu' di quanto valga.
// I NOMI restano `RefactorTactics.ScreenHud.*` perche' e' il widget a essere sotto misura, non il view
// model: e' la stessa scelta gia' fatta da `RTAutobattleInputInertTests`, dove *«i nomi restano sotto
// `RefactorTactics.Match.Autobattle.*` perche' e' la modalita' a essere sotto misura, non il puntatore»*.
//
// ⛔ **Il widget si guida per INIEZIONE, e non e' una scorciatoia.** `AcquireMatchContext` risolve il view
// model dall'**owning player**, e `UUserWidget::SetOwningPlayer` memorizza il `ULocalPlayer` — che una run
// headless non ha. Senza `SetReactionWindowForTest` questi test proverebbero solo il ramo «nessuna
// finestra», che e' il verde-per-la-ragione-sbagliata gia' pagato da `ActionDockShowsTheNeutralState`.
// =====================================================================================================

/**
 * 🔑 **IL WIDGET LEGGE LA FINESTRA VERA, e i tre numeri che mostra vengono tutti da fuori** (`#166`, voci
 * 2 · 3).
 *
 * ⚠️ **Il countdown si confronta con l'orologio AUTOREVOLE, non con una costante.** Asserire «3,0 s»
 * fisserebbe qui il valore che [`D-348`](../../../docs/decisions/RT_PDR_00_Decision_Log.md) ha appena dichiarato **configurabile**: il test diventerebbe
 * rosso il giorno in cui la durata diventa un setting di partita, e per la ragione sbagliata. Cio' che va
 * pinnato e' che il numero **coincida con quello del manager**, qualunque esso sia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastDecisionReadsWindowTest,
	"RefactorTactics.ScreenHud.FastDecisionReadsTheInjectedWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFastDecisionReadsWindowTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	URTFastDecisionWidget* Widget = NewObject<URTFastDecisionWidget>(World);
	if (!TestNotNull(TEXT("widget"), Widget))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- 1. Senza view model: chiuso, ai default, residuo negativo -----------------------------------
	// E' il ramo di un HUD nato prima del proprietario, che nel percorso normale esiste davvero.
	TestFalse(TEXT("senza view model la finestra e' chiusa"), Widget->IsWindowOpen());
	TestFalse(TEXT("e la vista e' ai default"), Widget->GetWindow().bOpen);
	TestTrue(TEXT("e il residuo dice «nessuna finestra», non «scaduta adesso»"),
		Widget->GetRemainingSeconds() < 0.f);

	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	URTReactionWindowViewModel* ViewModel = PC->GetReactionWindowViewModel();
	Widget->SetReactionWindowForTest(ViewModel);

	// --- 2. Con il view model ma prima che una finestra si apra: ancora chiuso -----------------------
	// Senza questo passo il punto 3 sarebbe soddisfatto anche da un widget che dice sempre «aperta».
	TestFalse(TEXT("con il view model ma senza finestra: ancora chiuso"), Widget->IsWindowOpen());

	// --- 3. La finestra si apre DAVVERO, e il widget la legge ----------------------------------------
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: la resolution si e' sospesa"), TM->IsResolutionSuspended()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	TestTrue(TEXT("il widget vede la finestra aperta"), Widget->IsWindowOpen());

	const FRTReactionWindowView Vista = Widget->GetWindow();
	TestTrue(TEXT("la vista consegnata al widget e' APERTA"), Vista.bOpen);
	TestTrue(TEXT("e offre piu' di una risposta: e' un boundary vero"), Vista.Options.Num() > 1);
	TestFalse(TEXT("e nomina la scelta sicura, che il widget non deve comporre"),
		Vista.SafeResponse.IsEmpty());

	// 🔑 Il countdown viene dall'orologio del manager, non da un contatore del widget.
	TestTrue(TEXT("il countdown coincide con l'orologio autorevole"),
		FMath::IsNearlyEqual(Widget->GetRemainingSeconds(),
			TM->GetOpenReactionWindowRemainingSeconds(), 1e-3f));
	TestTrue(TEXT("e la durata dichiarata e' quella server-authoritative"),
		FMath::IsNearlyEqual(Vista.WindowSeconds, TM->GetFastReactionDuration(), 1e-3f));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **IL WIDGET SCEGLIE PER INDICE, e non puo' nominare una risposta** (`#166`, voce 2 — *«nessuna logica
 * di gioco nel widget»*).
 *
 * ⚠️ **La meta' che conta e' il fail-closed.** Un indice fuori range non inoltra niente: le opzioni cambiano
 * quando la finestra cambia, e in una finestra da 3,0 s un bottone disegnato per la precedente porta con se'
 * il proprio indice. Se quel caso passasse, il widget risponderebbe a una domanda che il gioco non sta piu'
 * facendo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastDecisionChoosesByIndexTest,
	"RefactorTactics.ScreenHud.FastDecisionChoosesByIndexAndNeverNamesAResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFastDecisionChoosesByIndexTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	URTFastDecisionWidget* Widget = NewObject<URTFastDecisionWidget>(World);
	if (!TestNotNull(TEXT("widget"), Widget))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	Widget->SetReactionWindowForTest(PC->GetReactionWindowViewModel());

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: una finestra attende"), Widget->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FString IdAperta = TM->GetOpenReactionWindowId();
	const int32 Opzioni = Widget->GetWindow().Options.Num();

	// --- 1. FAIL-CLOSED: fuori range non inoltra niente, e la finestra resta quella --------------------
	// ⚠️ La warning e' attesa: si dichiara al framework, o il test la trasformerebbe in un fallimento.
	AddExpectedError(TEXT("FastDecision: opzione .* fuori range"), EAutomationExpectedErrorFlags::Contains, 0);
	Widget->ChooseOption(-1);
	Widget->ChooseOption(Opzioni);

	TestTrue(TEXT("un indice fuori range non ha chiuso la finestra"), Widget->IsWindowOpen());
	TestEqual(TEXT("ed e' ancora la stessa finestra"), TM->GetOpenReactionWindowId(), IdAperta);

	// --- 2. LA SCELTA: un indice valido inoltra la risposta che il CORE ha prodotto --------------------
	TestTrue(TEXT("premessa: la finestra offre almeno un'opzione"), Opzioni > 0);
	Widget->ChooseOption(0);

	// 🔑 La prova non e' «il widget ha chiamato qualcosa»: e' che la finestra a cui aveva risposto **non e'
	// piu' quella aperta**. Chiudere e riprendere puo' aprirne subito un'altra (`#2723`), quindi non si
	// asserisce «nessuna finestra» — si asserisce che *questa* e' chiusa.
	TestTrue(TEXT("la scelta ha chiuso la finestra a cui rispondeva"),
		TM->GetOpenReactionWindowId() != IdAperta);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **LA VISIBILITA' SEGUE L'APERTURA, NON IL NUMERO — e qui il numero dice davvero zero** (`#166`,
 * voce 3; rilievo `F7` dello spec panel del 2026-09-09).
 *
 * 🔑 **E' la meta' che nessun test poteva misurare finche' il vestito del binding stava in un grafo.**
 * `GetOpenReactionWindowRemainingSeconds()` scorre col `Tick` dell'Actor e il widget disegna col proprio:
 * il countdown mostrato tocca lo zero **prima** che la finestra si chiuda. Un binding di `Visibility`
 * costruito sul numero — la scorciatoia naturale nel Designer — farebbe sparire il prompt mentre il gioco
 * sta ancora aspettando una risposta, e la risposta mancata diventerebbe un `HoldTimeout` che nel TurnLog
 * e' indistinguibile da una scelta deliberata.
 *
 * Il test porta la finestra **dentro** quella fessura e ci guarda: numero a zero, finestra ancora aperta,
 * `Visible`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastDecisionVisibilityFollowsOpennessTest,
	"RefactorTactics.ScreenHud.FastDecisionVisibilityFollowsOpennessNotTheCountdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFastDecisionVisibilityFollowsOpennessTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	URTFastDecisionWidget* Widget = NewObject<URTFastDecisionWidget>(World);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("widget"), Widget))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- 1. Senza finestra: collassata, e i due testi vuoti ------------------------------------------
	// `Collapsed` e non `Hidden`: una finestra chiusa non deve lasciare un buco nel layout.
	TestEqual(TEXT("senza finestra la visibilita' e' Collapsed"),
		static_cast<int32>(Widget->GetWindowVisibility()),
		static_cast<int32>(ESlateVisibility::Collapsed));
	TestTrue(TEXT("e il countdown e' vuoto, non «0»"), Widget->GetCountdownText().IsEmpty());
	TestTrue(TEXT("e l'etichetta e' vuota"), Widget->GetPromptText().IsEmpty());

	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	Widget->SetReactionWindowForTest(PC->GetReactionWindowViewModel());

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: una finestra attende"), Widget->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- 2. Finestra appena aperta: visibile, e il countdown dice qualcosa ---------------------------
	TestEqual(TEXT("con la finestra aperta la visibilita' e' Visible"),
		static_cast<int32>(Widget->GetWindowVisibility()),
		static_cast<int32>(ESlateVisibility::Visible));
	TestFalse(TEXT("e il countdown non e' vuoto"), Widget->GetCountdownText().IsEmpty());
	TestFalse(TEXT("e l'etichetta non e' vuota"), Widget->GetPromptText().IsEmpty());

	// --- 3. LA MISURA: dentro la fessura fra «il numero dice zero» e «la finestra si chiude» ---------
	// Tick deterministici: la durata e' server-authoritative e il residuo e' `durata - trascorso`, quindi
	// questo ciclo atterra sempre nello stesso punto. Il tetto esiste per far fallire il test invece di
	// appenderlo, se un giorno la finestra smettesse di scadere.
	int32 Giri = 0;
	while (Widget->IsWindowOpen() && Widget->GetRemainingSeconds() > 0.05f && Giri < 2000)
	{
		TM->Tick(0.01f);
		++Giri;
	}

	if (!TestTrue(TEXT("la finestra e' ancora APERTA a residuo quasi nullo — e' la fessura da misurare"),
			Widget->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// 🔑 Il numero disegnato E' zero: senza questa riga il punto 4 non proverebbe niente, perche' potrebbe
	// essere verde semplicemente perche' il countdown non ci e' mai arrivato.
	const FString Countdown = Widget->GetCountdownText().ToString();
	TestTrue(*FString::Printf(TEXT("il numero mostrato e' a zero (letto: «%s»)"), *Countdown),
		Countdown.Contains(TEXT("0")) && !Countdown.Contains(TEXT("1"))
			&& !Countdown.Contains(TEXT("2")) && !Countdown.Contains(TEXT("3")));

	// --- 4. E la visibilita' NON lo segue ------------------------------------------------------------
	TestEqual(TEXT("il prompt e' ANCORA visibile: il gioco sta ancora aspettando una risposta"),
		static_cast<int32>(Widget->GetWindowVisibility()),
		static_cast<int32>(ESlateVisibility::Visible));

	// ⚠️ **Mai un segno meno.** Fra l'ultimo tick dell'orologio e la chiusura il residuo puo' essere di poco
	// negativo, e sarebbe l'unico momento in cui il widget mostra qualcosa che il gioco non ha mai detto.
	TestFalse(TEXT("e il numero non porta mai un segno meno"), Countdown.Contains(TEXT("-")));

	// --- 5. Chiusa la finestra, tutto torna neutro ---------------------------------------------------
	Giri = 0;
	while (Widget->IsWindowOpen() && Giri < 2000)
	{
		TM->Tick(0.01f);
		++Giri;
	}
	TestEqual(TEXT("chiusa la finestra la visibilita' torna Collapsed"),
		static_cast<int32>(Widget->GetWindowVisibility()),
		static_cast<int32>(ESlateVisibility::Collapsed));
	TestTrue(TEXT("e il countdown torna vuoto"), Widget->GetCountdownText().IsEmpty());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **IL RILEVAMENTO DEL CAMBIO FINESTRA distingue «e' cambiata» da «c'e' una finestra»** (`#166`, voce 2).
 *
 * 🔑 **E' la proprieta' su cui poggia la ricostruzione dei bottoni**, e la ragione per cui esiste un evento
 * invece di un binding: due finestre consecutive sono **entrambe** «aperta», quindi `IsWindowOpen()` non le
 * distingue. Un grafo che ricostruisse sulla transizione `falso → vero` terrebbe i bottoni della domanda
 * precedente per tutta la seconda finestra — e il primo click risponderebbe a una domanda che il gioco non
 * sta piu' facendo.
 *
 * ⚠️ **Il poll CONSUMA**: e' cio' che rende il test capace di dire che il segnale non e' semplicemente «c'e'
 * una finestra». Senza il passo 3, un `PollWindowChanged` che restituisse sempre `true` passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastDecisionWindowChangedTest,
	"RefactorTactics.ScreenHud.FastDecisionDetectsTheWindowChanging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFastDecisionWindowChangedTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	URTFastDecisionWidget* Widget = NewObject<URTFastDecisionWidget>(World);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("widget"), Widget))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- 1. Stato di partenza: niente finestra, niente cambio -----------------------------------------
	// Senza questo passo il punto 2 non proverebbe nulla: un poll che dicesse sempre `true` sarebbe verde.
	TestFalse(TEXT("senza finestra il primo poll non segnala nessun cambio"),
		Widget->PollWindowChangedForTest());

	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	Widget->SetReactionWindowForTest(PC->GetReactionWindowViewModel());

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: la finestra A attende"), Widget->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const FString IdA = TM->GetOpenReactionWindowId();

	// --- 2. La finestra A si apre: cambio segnalato ----------------------------------------------------
	TestTrue(TEXT("l'apertura della finestra A e' un cambio"), Widget->PollWindowChangedForTest());

	// --- 3. Il poll CONSUMA: la stessa finestra non e' un cambio nuovo ---------------------------------
	// 🔑 E' la meta' che distingue il segnale da «c'e' una finestra»: se questo fosse `true`, il grafo
	// ricostruirebbe i bottoni a ogni frame — e un click Slate, che e' due eventi su due frame, si
	// perderebbe fra un'istanza e l'altra.
	TestFalse(TEXT("la STESSA finestra non e' un cambio nuovo"), Widget->PollWindowChangedForTest());
	TestFalse(TEXT("e nemmeno al terzo poll"), Widget->PollWindowChangedForTest());

	// --- 4. La finestra cambia davvero: si porta A a scadenza -----------------------------------------
	int32 Giri = 0;
	while (TM->IsResolutionSuspended() && TM->GetOpenReactionWindowId() == IdA && Giri < 400)
	{
		TM->Tick(0.05f);
		++Giri;
	}

	// 🔑 **Il cambio va segnalato in ENTRAMBI i casi**, e sono due fatti diversi: se dopo A si apre una
	// finestra B il widget deve ricostruire per B; se non se ne apre nessuna deve ricostruire per il vuoto,
	// cioe' togliere i bottoni. Un evento che segnalasse solo le aperture lascerebbe a schermo la domanda
	// precedente.
	const bool bAltraFinestra = TM->IsResolutionSuspended() && Widget->IsWindowOpen();
	AddInfo(bAltraFinestra
		? TEXT("dopo A si e' aperta un'altra finestra: si verifica il cambio A -> B")
		: TEXT("dopo A non si e' aperta nessuna finestra: si verifica il cambio A -> vuoto"));

	TestTrue(TEXT("la finestra A non e' piu' quella corrente"),
		TM->GetOpenReactionWindowId() != IdA || !Widget->IsWindowOpen());
	TestTrue(TEXT("e il cambio e' segnalato"), Widget->PollWindowChangedForTest());
	TestFalse(TEXT("e poi si riconsuma"), Widget->PollWindowChangedForTest());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **L'OPZIONE INOLTRA IL PROPRIO INDICE, e non puo' nominare una risposta** (`#166`, voce 2).
 *
 * 🔑 **Il widget figlio esiste per una ragione meccanica**: in un `ForEach` di Blueprint l'indice non e'
 * catturabile dentro un delegate — `OnClicked` non porta parametri — e tutti i bottoni risponderebbero con
 * lo stesso indice, l'ultimo. Tenere l'indice nel figlio e' la forma che lo risolve, ed e' il precedente di
 * `URTActionSlotWidget`.
 *
 * ⚠️ **E la scelta sicura la decide il C++, non il testo.** Nel `Brace` si chiama `Hold Ground`: un grafo
 * che cercasse la parola `HOLD` sarebbe corretto oggi e sbagliato con la prima finestra che non e' un
 * Overwatch.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastDecisionOptionForwardsIndexTest,
	"RefactorTactics.ScreenHud.FastDecisionOptionForwardsItsOwnIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFastDecisionOptionForwardsIndexTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
	URTFastDecisionWidget* Finestra = NewObject<URTFastDecisionWidget>(World);
	if (!TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("widget"), Finestra))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- 0. Un'opzione SENZA proprietario non risponde per nessuno ------------------------------------
	// Il caso non e' teorico: il grafo puo' tenere in vita un bottone oltre la ricostruzione.
	URTFastDecisionOptionWidget* Orfana = NewObject<URTFastDecisionOptionWidget>(World);
	Orfana->Choose(); // non deve esplodere, e non deve fare nulla
	TestEqual(TEXT("un'opzione mai assegnata ha indice INDEX_NONE"),
		Orfana->OptionIndex, (int32)INDEX_NONE);

	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	Finestra->SetReactionWindowForTest(PC->GetReactionWindowViewModel());

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: una finestra attende"), Finestra->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTReactionWindowView Vista = Finestra->GetWindow();
	if (!TestTrue(TEXT("premessa: la finestra offre piu' di una risposta"), Vista.Options.Num() > 1))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const FString IdAperta = TM->GetOpenReactionWindowId();

	// --- 1. LA VIA DI PRODUZIONE: `MakeOptionWidget`, cio' che il grafo chiama davvero ----------------
	// 🔑 Il test costruisce i bottoni **come li costruisce il gioco**. Chiamare `SetOption` a mano
	// proverebbe il figlio e lascerebbe scoperta la funzione che il grafo usa — dove vivono le tre
	// decisioni che il grafo NON deve prendere: proprietario, indice, quale opzione e' la sicura.
	TestEqual(TEXT("il conteggio delle opzioni coincide con la vista"),
		Finestra->GetOptionCount(), Vista.Options.Num());

	int32 Sicure = 0;
	for (int32 i = 0; i < Vista.Options.Num(); ++i)
	{
		URTFastDecisionOptionWidget* Bottone =
			Finestra->MakeOptionWidget(URTFastDecisionOptionWidget::StaticClass(), i);
		if (!TestNotNull(*FString::Printf(TEXT("il bottone %d e' stato costruito"), i), Bottone))
		{
			continue;
		}

		TestEqual(*FString::Printf(TEXT("il bottone %d tiene il proprio indice"), i),
			Bottone->OptionIndex, i);
		TestFalse(*FString::Printf(TEXT("il bottone %d ha un'etichetta"), i),
			Bottone->GetOptionLabel().IsEmpty());
		Sicure += Bottone->bIsSafeChoice ? 1 : 0;
	}

	// 🔑 **La scelta sicura la marca il C++, e ne esiste UNA.** Viene da `SafeResponse`, non dalla
	// parola «HOLD»: nel `Brace` si chiama `Hold Ground`, e un grafo che cercasse la parola sarebbe
	// corretto oggi e sbagliato con la prima finestra che non e' un Overwatch.
	TestEqual(TEXT("esattamente una opzione e' marcata come scelta sicura"), Sicure, 1);

	// --- 2. FAIL-CLOSED: il grafo non puo' costruire un bottone che non ha una domanda ----------------
	AddExpectedError(TEXT("FastDecision: opzione .* fuori range"), EAutomationExpectedErrorFlags::Contains, 0);
	AddExpectedError(TEXT("FastDecision: nessuna classe"), EAutomationExpectedErrorFlags::Contains, 0);
	TestNull(TEXT("indice fuori range non produce nessun bottone"),
		Finestra->MakeOptionWidget(URTFastDecisionOptionWidget::StaticClass(), Vista.Options.Num()));
	TestNull(TEXT("classe nulla non produce nessun bottone"),
		Finestra->MakeOptionWidget(nullptr, 0));

	// --- 3. LA MISURA: il click di UN bottone inoltra QUELL'indice -------------------------------------
	// Si sceglie l'ultimo: con l'indice non catturato — il difetto che questo widget esiste per evitare —
	// ogni bottone risponderebbe con l'ultimo, e un test sul primo non lo vedrebbe.
	const int32 Ultimo = Vista.Options.Num() - 1;
	URTFastDecisionOptionWidget* Premuto =
		Finestra->MakeOptionWidget(URTFastDecisionOptionWidget::StaticClass(), Ultimo);
	if (!TestNotNull(TEXT("il bottone dell'ultima opzione esiste"), Premuto))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	TestEqual(TEXT("ed e' davvero l'ultimo indice"), Premuto->OptionIndex, Ultimo);
	Premuto->Choose();

	TestTrue(TEXT("il click ha chiuso la finestra a cui rispondeva"),
		TM->GetOpenReactionWindowId() != IdAperta);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **LA SLOW-MOTION SI RIPRISTINA SU ENTRAMBE LE USCITE** ([`D-350`], `#166`, voce 11).
 *
 * 🔑 **Il difetto che questo test esiste per impedire non e' «la slow-motion non parte»: e' «non
 * finisce».** Le uscite di una finestra sono quattro — risposta e scadenza, per il movimento e per il
 * `Brace` — e un ripristino scritto su ognuna e' disciplina in quattro posti. Dimenticarne uno lascia la
 * partita al 35% **per il resto del match**, e si manifesta solo dopo quella particolare uscita.
 *
 * ⚠️ **Percio' il ramo che conta e' la SCADENZA**, non la risposta: e' quella che un giocatore nota meno,
 * riproduce peggio, e che una correzione frettolosa dimentica per prima.
 *
 * ⛔ Il test NON asserisce il valore `0.35`: e' una manopola di pacing, e fissarlo qui renderebbe rosso un
 * cambio di taratura. Asserisce le due proprieta' che sono contratto: **rallentato mentre attende**
 * (`< 1`), e **esattamente 1** quando non attende piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionSlowMotionRestoresTest,
	"RefactorTactics.Reactions.SlowMotionRestoresOnBothExits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionSlowMotionRestoresTest::RunTest(const FString&)
{
	// Il banco si allestisce due volte: una per la RISPOSTA, una per la SCADENZA. Una lambda invece di due
	// copie — e il ramo che cambia e' l'unico parametro.
	auto Esegui = [this](bool bRispondi, const TCHAR* Etichetta)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return; }
		InitVmWorld(World);
		SpawnVmMap(World);

		ARTUnit* Mover = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
		ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 1);
		if (!TM || !GameMode || !PC) { RTWorldFixtures::DestroyWorld(World); return; }

		ArmOverwatchScenario(Mover, Watcher);
		GameMode->HookReactionWindow();
		URTReactionWindowViewModel* ViewModel = PC->GetReactionWindowViewModel();

		// --- 1. Stato di partenza: nessuna finestra, velocita' piena ---------------------------------
		TestTrue(*FString::Printf(TEXT("%s: si parte a velocita' piena"), Etichetta),
			FMath::IsNearlyEqual(TM->ViewerPlaybackSpeed, 1.f, 1e-3f));

		TM->LockInAndResolve();
		if (!TestTrue(*FString::Printf(TEXT("%s: premessa — una finestra attende"), Etichetta),
				TM->IsResolutionSuspended()))
		{
			RTWorldFixtures::DestroyWorld(World);
			return;
		}
		const FString IdAperta = TM->GetOpenReactionWindowId();

		// --- 2. Un tick, e la riproduzione RALLENTA --------------------------------------------------
		// ⚠️ Serve un tick: l'allineamento vive nel `Tick`, non nel sito che apre. E' cio' che lo rende
		// indipendente da QUALE dei due siti ha aperto.
		TM->Tick(0.01f);
		TestTrue(*FString::Printf(TEXT("%s: mentre la finestra attende la riproduzione e' rallentata"), Etichetta),
			TM->ViewerPlaybackSpeed < 1.f);
		TestTrue(*FString::Printf(TEXT("%s: e resta positiva — zero varrebbe «non scelto»"), Etichetta),
			TM->ViewerPlaybackSpeed > 0.f);

		// --- 3. L'USCITA: risposta oppure scadenza ---------------------------------------------------
		if (bRispondi)
		{
			ViewModel->SubmitResponse(ViewModel->GetWindow().SafeResponse);
			TM->Tick(0.01f);
		}
		else
		{
			// Si lascia scadere: nessuno risponde, e l'orologio fa il proprio lavoro.
			int32 Giri = 0;
			while (TM->GetOpenReactionWindowId() == IdAperta && Giri < 2000)
			{
				TM->Tick(0.01f);
				++Giri;
			}
		}

		// --- 4. LA MISURA -----------------------------------------------------------------------------
		// 🔑 Se dopo l'uscita un'ALTRA finestra si e' aperta, restare rallentati e' corretto: la
		// proprieta' non e' «si torna a 1», e' «la velocita' segue la presenza di una finestra».
		if (TM->GetOpenReactionWindowId().IsEmpty())
		{
			TestTrue(*FString::Printf(TEXT("%s: chiusa l'ultima finestra si torna a velocita' PIENA"), Etichetta),
				FMath::IsNearlyEqual(TM->ViewerPlaybackSpeed, 1.f, 1e-3f));
		}
		else
		{
			AddInfo(FString::Printf(
				TEXT("%s: dopo l'uscita un'altra finestra e' aperta — si verifica che resti rallentata"),
				Etichetta));
			TestTrue(*FString::Printf(TEXT("%s: con un'altra finestra aperta resta rallentata"), Etichetta),
				TM->ViewerPlaybackSpeed < 1.f);

			// ...e si porta il turno in fondo, per misurare comunque il ritorno a 1.
			int32 Giri = 0;
			while (TM->IsResolutionSuspended() && Giri < 4000) { TM->Tick(0.01f); ++Giri; }
			TestTrue(*FString::Printf(TEXT("%s: esaurite le finestre si torna a velocita' PIENA"), Etichetta),
				FMath::IsNearlyEqual(TM->ViewerPlaybackSpeed, 1.f, 1e-3f));
		}

		RTWorldFixtures::DestroyWorld(World);
	};

	Esegui(/*bRispondi=*/ true, TEXT("uscita per RISPOSTA"));
	Esegui(/*bRispondi=*/ false, TEXT("uscita per SCADENZA"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
