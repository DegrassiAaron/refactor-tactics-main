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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
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

#endif // WITH_DEV_AUTOMATION_TESTS
