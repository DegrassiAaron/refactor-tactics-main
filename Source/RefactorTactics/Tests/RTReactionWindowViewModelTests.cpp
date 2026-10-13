#include "Misc/AutomationTest.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "RTGameMode.h"
#include "Tests/RTReactionWindowFixtures.h" // #3666: la finestra si presenta al trattenimento del playback
#include "Tests/RTAbilityFixtures.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLogLibrary.h" // DescribeEntry: il perche' di una finestra che non si apre (#3667)
#include "UI/RTReactionWindowViewModel.h"
#include "UI/RTScreenHudWidgets.h" // URTFastDecisionWidget: la meta' UI delle voci 2 · 3 · 4 di CP 14.6
#include "UI/RTReactionResponseText.h" // nome e frase delle risposte (#3615)
#include "Ability/RTCatalogLibrary.h"   // BraceExecutableResponses: le risposte offribili si chiedono a chi le produce
#include "Ability/RTActionData.h"       // il nome che il kit da' alla reazione: la fonte del titolo (#3667)
#include "Turn/RTActionEvent.h"         // FRTActionEffectSpec: la premessa «quell'attacco spinge» (#3667)
#include "Unit/RTUnit.h"
#include "UI/RTUIPalette.h"         // i colori della resa della tavola E (#3634)
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento

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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento

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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento

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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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
		RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
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

/**
 * 🔑 **IL TASTO DELLA POSIZIONE i SCEGLIE L'OPZIONE i** ([D-491], #3615), e il badge dice quel tasto.
 *
 * La finestra si apre durante la risoluzione, quando i tasti del kit sono rifiutati ([D-468]): senza
 * l'instradamento di `SelectAbilityForCurrent` nessun tasto la raggiunge. La prova non e' «la finestra si e'
 * chiusa», che direbbe vero anche se il tasto scegliesse sempre la prima opzione: e' il TOKEN che il TurnLog
 * registra con esito `Chosen`, e si sceglie un'opzione che NON e' la prima.
 *
 * ⚠️ Prima, il tasto di una posizione senza opzione: non inoltra niente e la finestra resta quella. E le
 * LETTERE generiche, con un'unita' selezionata che le ha nel kit: arrivano a `SelectAbilityForCurrent` per la
 * posizione della generica, e la review di #3615 ha trovato che un instradamento messo la' le faceva scegliere
 * un'opzione ([D-397] §4 lega la lettera all'`ActionId`). Devono lasciare la finestra com'e', senza warning.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionHotkeyChoosesOptionTest,
	"RefactorTactics.Reactions.ViewModel.HotkeyChoosesTheOptionInItsPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionHotkeyChoosesOptionTest::RunTest(const FString&)
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

	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	Finestra->SetReactionWindowForTest(PC->GetReactionWindowViewModel());

	TM->LockInAndResolve();
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
	const FRTReactionWindowView Vista = PC->GetReactionWindowViewModel()->GetWindow();
	if (!TestTrue(TEXT("premessa: una finestra dell'Overwatch attende chi guarda"), Vista.bOpen)
		|| !TestTrue(TEXT("premessa: la finestra offre piu' di una risposta"), Vista.Options.Num() > 1))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const int32 Scelta = Vista.Options.Num() - 1;  // NON la prima
	const FString Attesa = Vista.Options[Scelta].Response;
	if (!TestNotEqual(TEXT("premessa: l'opzione scelta non e' la prima"), Attesa, Vista.Options[0].Response))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// --- 1. I BADGE: ogni bottone dice il tasto della propria posizione, e un nome, non il token -------------
	for (int32 i = 0; i < Vista.Options.Num(); ++i)
	{
		const URTFastDecisionOptionWidget* Bottone =
			Finestra->MakeOptionWidget(URTFastDecisionOptionWidget::StaticClass(), i);
		if (!TestNotNull(*FString::Printf(TEXT("il bottone %d e' stato costruito"), i), Bottone)) { continue; }
		TestEqual(*FString::Printf(TEXT("il badge del bottone %d e' il tasto della posizione %d"), i, i),
			Bottone->GetKeyLabel().ToString(), ARTPlayerController::ReactionOptionKeyLabel(i).ToString());
		TestFalse(*FString::Printf(TEXT("il bottone %d ha un tasto"), i), Bottone->GetKeyLabel().IsEmpty());
		TestFalse(*FString::Printf(TEXT("il bottone %d ha una frase"), i), Bottone->GetOptionDescription().IsEmpty());
		if (URTReactionOpportunityLibrary::FireResponseTarget(Vista.Options[i].Response) != INDEX_NONE)
		{
			TestEqual(TEXT("una risposta FIRE si chiama FIRE, non col suo token"),
				Bottone->GetOptionLabel().ToString(), FString(TEXT("FIRE")));
		}
	}

	// --- 2. FAIL-CLOSED: il tasto di una posizione senza opzione non inoltra niente ----------------------------
	const FString IdAperta = TM->GetOpenReactionWindowId();
	AddExpectedError(TEXT("FastDecision: opzione .* fuori range"), EAutomationExpectedErrorFlags::Contains, 1);
	PC->PressKitHotkeyForTest(Vista.Options.Num());
	TestEqual(TEXT("un tasto senza opzione lascia aperta la stessa finestra"), TM->GetOpenReactionWindowId(), IdAperta);

	// --- 2-bis. LE LETTERE GENERICHE non scelgono un'opzione -------------------------------------------------
	PC->SelectActorForTest(Watcher);
	int32 LetterePremute = 0;
	for (int32 Riga = 0; Riga < ARTPlayerController::GenericHotkeys().Num(); ++Riga)
	{
		const FName Generica = ARTPlayerController::GenericHotkeys()[Riga].Key;
		bool bNelKit = false;
		for (int32 i = 0; i < Watcher->NumAbilities(); ++i)
		{
			bNelKit |= (Watcher->GetAbility(i) && Watcher->GetAbility(i)->Def.ActionId == Generica);
		}
		if (!bNelKit) { continue; } // fuori dal kit la lettera non arriva al punto comune: non proverebbe niente
		PC->PressGenericHotkeyForTest(Riga);
		++LetterePremute;
		TestEqual(*FString::Printf(TEXT("la lettera di `%s` lascia aperta la stessa finestra"), *Generica.ToString()),
			TM->GetOpenReactionWindowId(), IdAperta);
	}
	TestTrue(TEXT("premessa: almeno una lettera generica e' nel kit di chi guarda"), LetterePremute > 0);

	// --- 3. LA SCELTA: il tasto della posizione `Scelta` ----------------------------------------------------
	PC->PressKitHotkeyForTest(Scelta);
	TestTrue(TEXT("il tasto ha chiuso la finestra a cui rispondeva"), TM->GetOpenReactionWindowId() != IdAperta);

	for (int32 Giri = 0; TM->IsResolutionSuspended() && Giri < 200; ++Giri)
	{
		TM->Tick(0.05f);
	}
	const FRTTurnLogEntry* Decisione = TM->GetTurnLog().FindByPredicate([](const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::ReactionDecision
			&& E.Outcome == static_cast<uint8>(ERTReactionDecisionOutcome::Chosen);
	});
	if (TestNotNull(TEXT("il TurnLog registra una decisione scelta"), (const void*)Decisione))
	{
		TestEqual(*FString::Printf(TEXT("e il token e' quello dell'opzione in posizione %d"), Scelta),
			Decisione->ReactionResponse, Attesa);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Ogni risposta che una finestra puo' offrire ha un nome e una frase** ([D-491], #3615).
 *
 * Le risposte offribili si chiedono a chi le produce, non si elencano qui: `HOLD` e `FIRE:<i>` dalla libreria
 * delle opportunity, `Hold Ground` e le manovre da `BraceExecutableResponses` per ogni profilo del catalogo.
 * Una manovra nuova in un profilo, senza una voce in `RTReactionResponseText`, fa fallire questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionResponseTextCoversTest,
	"RefactorTactics.Reactions.EveryOfferedResponseHasANameAndADescription",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionResponseTextCoversTest::RunTest(const FString&)
{
	TArray<FString> Offribili = {
		URTReactionOpportunityLibrary::HoldResponse(),
		URTReactionOpportunityLibrary::FireResponse(0),
		URTReactionOpportunityLibrary::FireResponse(7) };
	Offribili.Append(URTCatalogLibrary::BraceExecutableResponses(NAME_None));
	for (const FRTReactionProfileDef& Profilo : URTCatalogLibrary::GetReactionProfileCatalog())
	{
		for (const FString& Risposta : URTCatalogLibrary::BraceExecutableResponses(Profilo.ProfileId))
		{
			Offribili.AddUnique(Risposta);
		}
	}

	for (const FString& Risposta : Offribili)
	{
		const FText Nome = RTReactionResponseText::NameFor(Risposta);
		TestFalse(*FString::Printf(TEXT("`%s` ha una frase"), *Risposta), RTReactionResponseText::DescriptionFor(Risposta).IsEmpty());
		TestFalse(*FString::Printf(TEXT("`%s` ha un nome"), *Risposta), Nome.IsEmpty());
		TestFalse(*FString::Printf(TEXT("il nome di `%s` non contiene l'indice del bersaglio"), *Risposta),
			Nome.ToString().Contains(TEXT(":")));
	}

	// Una risposta che la tabella non conosce resta leggibile: il bottone mostra il token, non un vuoto.
	TestEqual(TEXT("una risposta sconosciuta mostra il proprio token"),
		RTReactionResponseText::NameFor(TEXT("TOKEN_SCONOSCIUTO")).ToString(), FString(TEXT("TOKEN_SCONOSCIUTO")));
	TestTrue(TEXT("e non inventa una frase"), RTReactionResponseText::DescriptionFor(TEXT("TOKEN_SCONOSCIUTO")).IsEmpty());
	return true;
}

/**
 * **Il tasto di un'opzione e' il tasto del kit nella stessa posizione** ([D-491], #3615): il badge e il tasto
 * che risponde vengono dalla stessa lista, `AbilityHotkeys()`. Oltre l'ultimo tasto non c'e' badge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionOptionKeyLabelTest,
	"RefactorTactics.PlayerInput.ReactionOptionKeyIsTheKitKeyOfItsPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionOptionKeyLabelTest::RunTest(const FString&)
{
	const TArray<FKey>& Tasti = ARTPlayerController::AbilityHotkeys();
	if (!TestTrue(TEXT("premessa: il kit ha dei tasti"), Tasti.Num() > 0)) { return false; }
	for (int32 i = 0; i < Tasti.Num(); ++i)
	{
		TestEqual(*FString::Printf(TEXT("l'opzione %d si sceglie col tasto %s"), i, *Tasti[i].ToString()),
			ARTPlayerController::ReactionOptionKeyLabel(i).ToString(), Tasti[i].GetDisplayName(false).ToString());
	}
	TestTrue(TEXT("oltre l'ultimo tasto nessun badge"), ARTPlayerController::ReactionOptionKeyLabel(Tasti.Num()).IsEmpty());
	TestTrue(TEXT("e nemmeno prima del primo"), ARTPlayerController::ReactionOptionKeyLabel(INDEX_NONE).IsEmpty());
	return true;
}

// =====================================================================================================
// I nomi nella finestra di reazione (`#3667`): titolo, sottotitolo, bersaglio
// =====================================================================================================

namespace
{
	/** Una vista aperta con la reazione e i nomi dati: per i test delle FRASI, che non hanno bisogno di un turno. */
	FRTReactionWindowView VistaConNomi(const TCHAR* ReactionDefId, const TCHAR* Reazione, const TCHAR* Unita,
		const TArray<FString>& Trigger)
	{
		FRTReactionWindowView Vista;
		Vista.bOpen = true;
		Vista.Key.ReactionDefId = FName(ReactionDefId);
		Vista.ReactionName = FText::FromString(Reazione);
		Vista.ReactorName = FText::FromString(Unita);
		for (const FString& Nome : Trigger)
		{
			Vista.TriggerNames.Add(FText::FromString(Nome));
		}
		return Vista;
	}

	/** Il nome con cui il kit di `Unit` chiama l'azione `ActionId`, o vuoto. E' la fonte che il core usa (`I4.2`). */
	FString NomeNelKit(const ARTUnit* Unit, const TCHAR* ActionId)
	{
		for (int32 i = 0; Unit && i < Unit->NumAbilities(); ++i)
		{
			const URTActionData* Azione = Unit->GetAbility(i);
			if (Azione && Azione->Def.ActionId == FName(ActionId) && !Azione->DisplayName.IsEmptyOrWhitespace())
			{
				return Azione->DisplayName.ToString();
			}
		}
		return FString();
	}
}

/**
 * **LE FRASI DELLA FINESTRA SONO QUELLE DELLA TAVOLA E** (`#3667`, R2.3) — titolo, sottotitolo, opzione.
 *
 * Si provano sulla VISTA, senza un turno: la composizione e' pura, e ogni ramo di ripiego ha il proprio caso.
 * I ripieghi contano quanto i casi pieni, perche' sono cio' che il giocatore legge quando il core non ha messo
 * un nome — e nel caso del `Brace` e' una regola di privacy, non un'approssimazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowTextsFollowTheBoardTest,
	"RefactorTactics.Reactions.WindowTextsFollowTheBoard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowTextsFollowTheBoardTest::RunTest(const FString&)
{
	const TCHAR* Overwatch = TEXT("Action.Overwatch");
	const TCHAR* Brace = TEXT("Action.Brace");

	// --- il titolo, e i suoi tre ripieghi ---
	TestEqual(TEXT("titolo: reazione e unita'"),
		RTReactionResponseText::TitleFor(VistaConNomi(Overwatch, TEXT("Guardia reattiva"), TEXT("Aevik"), {})).ToString(),
		FString(TEXT("Guardia reattiva — Aevik")));
	TestEqual(TEXT("titolo: senza il nome della reazione resta neutro, con l'unita'"),
		RTReactionResponseText::TitleFor(VistaConNomi(Overwatch, TEXT(""), TEXT("Aevik"), {})).ToString(),
		FString(TEXT("Reazione — Aevik")));
	TestEqual(TEXT("titolo: senza il nome dell'unita' resta la reazione"),
		RTReactionResponseText::TitleFor(VistaConNomi(Overwatch, TEXT("Guardia reattiva"), TEXT(""), {})).ToString(),
		FString(TEXT("Guardia reattiva")));
	TestEqual(TEXT("titolo: senza nomi e' quello di prima di #3667"),
		RTReactionResponseText::TitleFor(VistaConNomi(Overwatch, TEXT(""), TEXT(""), {})).ToString(),
		FString(TEXT("Reazione")));
	TestEqual(TEXT("titolo: un nome di soli spazi non e' un nome"),
		RTReactionResponseText::TitleFor(VistaConNomi(Overwatch, TEXT("  "), TEXT(" "), {})).ToString(),
		FString(TEXT("Reazione")));

	// --- il sottotitolo dell'Overwatch: uno, due, tre ---
	TestEqual(TEXT("Overwatch, un'unita'"),
		RTReactionResponseText::SubtitleFor(VistaConNomi(Overwatch, TEXT("G"), TEXT("A"), { TEXT("Ivrin") })).ToString(),
		FString(TEXT("Ivrin è entrato nell'area controllata")));
	TestEqual(TEXT("Overwatch, due unita' sullo stesso passo"),
		RTReactionResponseText::SubtitleFor(
			VistaConNomi(Overwatch, TEXT("G"), TEXT("A"), { TEXT("Ivrin"), TEXT("Branth") })).ToString(),
		FString(TEXT("Ivrin e Branth sono entrati nell'area controllata")));
	TestEqual(TEXT("Overwatch, tre unita'"),
		RTReactionResponseText::SubtitleFor(
			VistaConNomi(Overwatch, TEXT("G"), TEXT("A"), { TEXT("Ivrin"), TEXT("Branth"), TEXT("Muiren") })).ToString(),
		FString(TEXT("Ivrin, Branth e Muiren sono entrati nell'area controllata")));
	TestTrue(TEXT("Overwatch, nessun nome: nessuna frase, invece di una frase senza soggetto"),
		RTReactionResponseText::SubtitleFor(VistaConNomi(Overwatch, TEXT("G"), TEXT("A"), {})).IsEmpty());
	TestEqual(TEXT("Overwatch: un nome vuoto in mezzo non rompe la frase"),
		RTReactionResponseText::SubtitleFor(
			VistaConNomi(Overwatch, TEXT("G"), TEXT("A"), { TEXT(""), TEXT("Ivrin") })).ToString(),
		FString(TEXT("Ivrin è entrato nell'area controllata")));

	// --- il sottotitolo del Brace: col nome, e senza ---
	TestEqual(TEXT("Brace, chi spinge e' nominato"),
		RTReactionResponseText::SubtitleFor(VistaConNomi(Brace, TEXT("I"), TEXT("A"), { TEXT("Ivrin") })).ToString(),
		FString(TEXT("Ivrin ti spinge")));
	TestEqual(TEXT("🔴 Brace, nessun nome nella vista: la frase e' neutra"),
		RTReactionResponseText::SubtitleFor(VistaConNomi(Brace, TEXT("I"), TEXT("A"), {})).ToString(),
		FString(TEXT("Un nemico ti spinge")));

	// --- una finestra chiusa non dice niente, qualunque cosa porti ---
	FRTReactionWindowView Chiusa = VistaConNomi(Overwatch, TEXT("Guardia reattiva"), TEXT("Aevik"), { TEXT("Ivrin") });
	Chiusa.bOpen = false;
	TestTrue(TEXT("finestra chiusa: nessun titolo"), RTReactionResponseText::TitleFor(Chiusa).IsEmpty());
	TestTrue(TEXT("finestra chiusa: nessun sottotitolo"), RTReactionResponseText::SubtitleFor(Chiusa).IsEmpty());

	// --- una reazione che non ha una frase: sottotitolo vuoto, non una frase presa in prestito ---
	TestTrue(TEXT("una reazione sconosciuta non prende la frase di un'altra"),
		RTReactionResponseText::SubtitleFor(
			VistaConNomi(TEXT("Action.Inventata"), TEXT("X"), TEXT("A"), { TEXT("Ivrin") })).IsEmpty());

	// --- le opzioni ---
	FRTReactionWindowOptionView Fuoco;
	Fuoco.Response = URTReactionOpportunityLibrary::FireResponse(3);
	Fuoco.TargetSnapshotIndex = 3;
	Fuoco.TargetName = FText::FromString(TEXT("Ivrin"));
	TestEqual(TEXT("FIRE col nome del bersaglio"),
		RTReactionResponseText::DescriptionFor(Fuoco).ToString(), FString(TEXT("Attacca Ivrin")));

	FRTReactionWindowOptionView FuocoSenzaNome = Fuoco;
	FuocoSenzaNome.TargetName = FText::GetEmpty();
	TestEqual(TEXT("FIRE senza nome: la frase generica della tabella, mai un indice"),
		RTReactionResponseText::DescriptionFor(FuocoSenzaNome).ToString(),
		RTReactionResponseText::DescriptionFor(Fuoco.Response).ToString());
	TestFalse(TEXT("e non e' vuota"), RTReactionResponseText::DescriptionFor(FuocoSenzaNome).IsEmpty());

	FRTReactionWindowOptionView Tieni;
	Tieni.Response = FString(URTReactionOpportunityLibrary::HoldResponse());
	// Un nome scritto per errore su un'opzione che non spara non deve finire nella frase.
	Tieni.TargetName = FText::FromString(TEXT("Ivrin"));
	TestEqual(TEXT("HOLD"), RTReactionResponseText::DescriptionFor(Tieni).ToString(), FString(TEXT("Non attaccare")));

	// Le risposte di profilo tengono la frase del catalogo: la tavola non le mostra.
	FRTReactionWindowOptionView TieniLaCella;
	TieniLaCella.Response = TEXT("Hold Ground");
	TestEqual(TEXT("una risposta di profilo tiene la frase della tabella"),
		RTReactionResponseText::DescriptionFor(TieniLaCella).ToString(),
		RTReactionResponseText::DescriptionFor(FString(TEXT("Hold Ground"))).ToString());

	return true;
}

/**
 * **OGNI REAZIONE CHE PUO' APRIRE UNA FINESTRA HA UN SOTTOTITOLO** (`#3667`).
 *
 * `SubtitleFor` sceglie la frase dall'id della reazione, e gli id li scrive due volte: li' e in
 * `BoundaryCapableReactionIds`. Questo test lega i due elenchi: una terza reazione capace di aprire una finestra,
 * aggiunta al core senza una frase, lo fa cadere invece di arrivare a schermo con un sottotitolo vuoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEveryBoundaryReactionHasASubtitleTest,
	"RefactorTactics.Reactions.EveryBoundaryReactionHasASubtitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEveryBoundaryReactionHasASubtitleTest::RunTest(const FString&)
{
	const TArray<FName>& Reazioni = URTReactionOpportunityLibrary::BoundaryCapableReactionIds();
	if (!TestTrue(TEXT("premessa: il core dichiara reazioni che aprono una finestra"), Reazioni.Num() > 0))
	{
		return false;
	}
	for (const FName& Id : Reazioni)
	{
		const FRTReactionWindowView Vista = VistaConNomi(*Id.ToString(), TEXT("X"), TEXT("A"), { TEXT("Ivrin") });
		const FText Frase = RTReactionResponseText::SubtitleFor(Vista);
		TestFalse(*FString::Printf(TEXT("`%s`: con un nome, il sottotitolo c'e'"), *Id.ToString()), Frase.IsEmpty());
		TestTrue(*FString::Printf(TEXT("`%s`: e nomina l'unita'"), *Id.ToString()),
			Frase.ToString().Contains(TEXT("Ivrin")));
	}
	return true;
}

/**
 * **I NOMI SI RISOLVONO NELLO SPAZIO DELLO SNAPSHOT, E NON CAMBIANO DOPO** (`#3667`, esempi 2 e I4.3).
 *
 * 🔴 **Il difetto che la guardia del titolo neutro esisteva per impedire.** `Key.OwnerId` e ogni `FIRE:<i>` sono
 * indici nell'array di `MakeCurrentSnapshot`, che scarta i morti. Con un caduto che ordina prima, lo stesso
 * indice su un roster nomina un'altra unita'. Qui ce n'e' uno, e la premessa asserisce che il roster
 * risponderebbe davvero col nome sbagliato: senza, il test non distinguerebbe i due spazi.
 *
 * 🔑 **E fra la costruzione e la consegna i nomi sono testo.** Da `#3666` la vista si costruisce prima e si
 * presenta dopo: se l'unita' nominata cade, o l'array cambia, il titolo resta quello costruito. Chi tenesse un
 * indice o un puntatore e lo risolvesse alla consegna farebbe cadere la seconda meta'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowNamesResolveInTheSnapshotSpaceTest,
	"RefactorTactics.Reactions.WindowNamesResolveInTheSnapshotSpace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowNamesResolveInTheSnapshotSpaceTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	InitVmWorld(World);
	SpawnVmMap(World);

	ARTUnit* Watcher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(3, 0));
	ARTUnit* Caduto = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(-3, 0));
	ARTUnit* Entrato = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Watcher"), Watcher) || !TestNotNull(TEXT("Caduto"), Caduto)
		|| !TestNotNull(TEXT("Entrato"), Entrato) || !TestNotNull(TEXT("TurnManager"), TM))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	// Le fixture montano lo stesso eroe per tutti: i nomi vanno distinti, o il test non vedrebbe uno scambio.
	Watcher->HeroDisplayName = FText::FromString(TEXT("Aevik"));
	Caduto->HeroDisplayName = FText::FromString(TEXT("Muiren"));
	Entrato->HeroDisplayName = FText::FromString(TEXT("Ivrin"));

	// Il roster, e lo spazio dello snapshot: lo stesso ordine, senza il caduto.
	const TArray<ARTUnit*> Roster = { Watcher, Caduto, Entrato };
	TArray<ARTUnit*> Vivi = { Watcher, Entrato };

	FRTReactionOpportunity Opportunity;
	Opportunity.Key.TurnNumber = 1;
	Opportunity.Key.MacroPhase = ERTMatchPhase::Move;
	Opportunity.Key.OwnerId = 0;
	Opportunity.Key.ReactionDefId = TEXT("Action.Overwatch");
	Opportunity.AllowedResponses = {
		URTReactionOpportunityLibrary::FireResponse(1), FString(URTReactionOpportunityLibrary::HoldResponse()) };

	// La premessa che rende il test capace di fallire: sul ROSTER l'indice 1 e' un'altra unita'.
	if (!TestTrue(TEXT("premessa: l'indice del bersaglio, risolto sul roster, nomina il CADUTO"),
			Roster[1] == Caduto && Vivi[1] == Entrato)
		|| !TestFalse(TEXT("premessa: il kit del watcher nomina la reazione"),
			NomeNelKit(Watcher, TEXT("Action.Overwatch")).IsEmpty()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTReactionWindowNames Nomi = ARTTurnManager::MakeReactionWindowNames(Opportunity, Vivi);
	TestEqual(TEXT("chi decide"), Nomi.ReactorName.ToString(), FString(TEXT("Aevik")));
	TestEqual(TEXT("la reazione ha il nome del SUO kit"),
		Nomi.ReactionName.ToString(), NomeNelKit(Watcher, TEXT("Action.Overwatch")));
	if (TestEqual(TEXT("un bersaglio"), Nomi.TargetNames.Num(), 1) && TestNotNull(TEXT("all'indice 1"), Nomi.TargetNames.Find(1)))
	{
		TestEqual(TEXT("🔴 il bersaglio e' l'unita' ENTRATA, non il caduto che sul roster occupa quell'indice"),
			Nomi.TargetNames[1].ToString(), FString(TEXT("Ivrin")));
	}
	if (TestEqual(TEXT("un trigger"), Nomi.TriggerNames.Num(), 1))
	{
		TestEqual(TEXT("ed e' chi e' entrato"), Nomi.TriggerNames[0].ToString(), FString(TEXT("Ivrin")));
	}

	// La vista, come la riceve la squadra del watcher.
	const FRTReactionWindowView Vista = TM->MakeReactionWindowView(Opportunity, /*Owner=*/1, /*Observer=*/1, Nomi);
	const FString TitoloAllaCostruzione = RTReactionResponseText::TitleFor(Vista).ToString();
	const FString SottotitoloAllaCostruzione = RTReactionResponseText::SubtitleFor(Vista).ToString();
	TestEqual(TEXT("il sottotitolo nomina chi e' entrato"),
		SottotitoloAllaCostruzione, FString(TEXT("Ivrin è entrato nell'area controllata")));
	TestTrue(TEXT("il titolo nomina chi decide"), TitoloAllaCostruzione.EndsWith(TEXT(" — Aevik")));

	// 🔑 **Fra la costruzione e la consegna** (I4.3): l'unita' nominata cambia nome e l'array perde un posto.
	// La vista e' gia' testo, e non segue nessuno dei due.
	Entrato->HeroDisplayName = FText::FromString(TEXT("Un Altro"));
	Vivi.RemoveAt(1);
	TestEqual(TEXT("🔴 il titolo e' quello costruito"), RTReactionResponseText::TitleFor(Vista).ToString(), TitoloAllaCostruzione);
	TestEqual(TEXT("🔴 e cosi' il sottotitolo"), RTReactionResponseText::SubtitleFor(Vista).ToString(), SottotitoloAllaCostruzione);
	bool bTrovataFire = false;
	for (const FRTReactionWindowOptionView& Opzione : Vista.Options)
	{
		if (Opzione.TargetSnapshotIndex == 1)
		{
			bTrovataFire = true;
			TestEqual(TEXT("🔴 e la frase dell'opzione"), RTReactionResponseText::DescriptionFor(Opzione).ToString(),
				FString(TEXT("Attacca Ivrin")));
		}
	}
	TestTrue(TEXT("la vista porta l'opzione che spara"), bTrovataFire);

	// L'avversario non riceve niente, nemmeno passando dal manager coi nomi in mano.
	const FRTReactionWindowView AllAvversario = TM->MakeReactionWindowView(Opportunity, /*Owner=*/1, /*Observer=*/0, Nomi);
	TestFalse(TEXT("l'avversario non ha la finestra"), AllAvversario.bOpen);
	TestTrue(TEXT("ne' un nome"), AllAvversario.ReactorName.IsEmpty() && AllAvversario.ReactionName.IsEmpty()
		&& AllAvversario.TriggerNames.Num() == 0 && AllAvversario.Options.Num() == 0);

	// Indici fuori dall'array non sono nomi: i campi restano vuoti, e il titolo torna neutro.
	FRTReactionOpportunity Fuori = Opportunity;
	Fuori.Key.OwnerId = 42;
	Fuori.AllowedResponses = {
		URTReactionOpportunityLibrary::FireResponse(42), FString(URTReactionOpportunityLibrary::HoldResponse()) };
	const FRTReactionWindowNames Nessuno = ARTTurnManager::MakeReactionWindowNames(Fuori, Roster);
	TestTrue(TEXT("un proprietario fuori dall'array non ha un nome"), Nessuno.ReactorName.IsEmpty());
	TestEqual(TEXT("ne' un bersaglio fuori dall'array"), Nessuno.TargetNames.Num(), 0);
	TestEqual(TEXT("ne' un trigger"), Nessuno.TriggerNames.Num(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **IL `Brace` NOMINA CHI SPINGE SOLO SE LA SQUADRA LO RILEVA** (`#3667`, esempio 3) — dal Blast vero.
 *
 * Tre turni. Chi spinge da lontano, dentro l'arco di chi si irrigidisce: la squadra ne rileva la cella e la
 * finestra dice il nome. Lo stesso turno con chi si irrigidisce girato dall'altra parte: non la rileva, e la
 * finestra dice «Un nemico ti spinge». Il nome non sta nel DTO: lo si cerca in OGNI campo di testo della vista,
 * non solo nel sottotitolo.
 *
 * 🔑 **Il terzo turno e' la correzione di una premessa sbagliata, ed e' un fatto di gioco.** Chi spinge da una
 * cella ADIACENTE e' rilevato anche di spalle: la consapevolezza ravvicinata copre ogni direzione entro
 * `CloseAwarenessRange`. Il caso senza nome nasce solo da una spinta a distanza — l'attacco base di Muiren, che
 * spinge lungo una linea — ed e' per questo che il test la usa. `Action.Push`, a contatto, nomina sempre.
 *
 * ⚠️ **La premessa e' misurata, non assunta**: per ciascun turno il test chiede a `TeamAwarenessOfCell` che cosa
 * la squadra vede, con l'osservatore costruito come lo costruisce il resolver. Se la risposta non e' quella
 * attesa il test si ferma, invece di passare su un turno che non distingue niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBraceWindowNamesThePusherOnlyWhenDetectedTest,
	"RefactorTactics.Reactions.Brace.WindowNamesThePusherOnlyWhenDetected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBraceWindowNamesThePusherOnlyWhenDetectedTest::RunTest(const FString&)
{
	struct FEsito
	{
		bool bValido = false;
		ERTAwareness Quanto = ERTAwareness::Hidden;
		FRTReactionWindowView Vista;
	};
	// `bDaLontano`: Muiren col suo attacco base, a tre celle. Altrimenti `Action.Push`, a contatto.
	const auto Gioca = [this](bool bDaLontano, ERTHexDirection VersoDelBracer) -> FEsito
	{
		FEsito Esito;
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return Esito; }
		InitVmWorld(World);
		ARTHexMapActor* Mappa = SpawnVmMap(World);

		ARTUnit* Bracer = SpawnVmUnit(World, /*TeamId=*/ 0, FRTCellId(0, 0));
		ARTUnit* Pusher = nullptr;
		if (bDaLontano)
		{
			// Come `SpawnVmUnit`, con l'eroe che spinge a distanza: chi sta a tre celle e' fuori dalla
			// consapevolezza ravvicinata, e lo vede solo chi lo ha nell'arco frontale.
			Pusher = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
			if (Pusher)
			{
				Pusher->TeamId = 1;
				Pusher->bIsBotControlled = false;
				Pusher->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeMuiren());
				UGameplayStatics::FinishSpawningActor(Pusher, FTransform::Identity);
				Pusher->PlaceOnCell(FRTCellId(3, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
			}
		}
		else
		{
			Pusher = SpawnVmUnit(World, /*TeamId=*/ 1, FRTCellId(1, 0));
		}
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TestNotNull(TEXT("Bracer"), Bracer) || !TestNotNull(TEXT("Pusher"), Pusher)
			|| !TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("mappa"), Mappa))
		{
			RTWorldFixtures::DestroyWorld(World);
			return Esito;
		}
		Bracer->HeroDisplayName = FText::FromString(TEXT("Aevik"));
		Pusher->HeroDisplayName = FText::FromString(TEXT("Ivrin"));

		if (bDaLontano)
		{
			// L'attacco base di Muiren: una linea che spinge. Si cerca per id, e se ne asseriscono le due
			// proprieta' che il turno richiede — altrimenti un catalogo ritoccato lascerebbe il test a
			// misurare un colpo che non spinge, o che non arriva.
			int32 IndiceDelGetto = INDEX_NONE;
			for (int32 i = 0; i < Pusher->NumAbilities(); ++i)
			{
				const URTActionData* Azione = Pusher->GetAbility(i);
				if (Azione && Azione->Def.ActionId == FName(TEXT("Hero.Muiren.PressureJet"))) { IndiceDelGetto = i; break; }
			}
			const URTActionData* Getto = Pusher->GetAbility(IndiceDelGetto);
			const bool bSpinge = Getto && Getto->Def.Effects.ContainsByPredicate(
				[](const FRTActionEffectSpec& Effetto) { return Effetto.Effect == ERTActionEffect::Push; });
			if (!TestNotNull(TEXT("premessa: Muiren ha il suo attacco base"), (void*)Getto)
				|| !TestTrue(TEXT("premessa: quell'attacco spinge"), bSpinge)
				|| !TestTrue(TEXT("premessa: e arriva a tre celle"), Getto->Def.RangeCells >= 3))
			{
				RTWorldFixtures::DestroyWorld(World);
				return Esito;
			}
			Bracer->bIsBotControlled = false;
			Bracer->ReactionProfileId = TEXT("Profile.Sidestep");
			Bracer->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Bracer, TEXT("Action.Brace"), 3);
			Bracer->PlannedCell = Bracer->Cell;
			Pusher->PlannedAbilityIndex = IndiceDelGetto;
			Pusher->DeclareAttackOnUnit(Bracer);
			Pusher->PlannedCell = Pusher->Cell;
			// Chi spinge GUARDA il bersaglio. ⚠️ Misurato, non dedotto: col verso di default — a est, cioe' di
			// spalle al bersaglio — il colpo non arrivava e nessuna finestra si apriva. Perche', questo test non
			// lo indaga: gli serve una spinta che arrivi, e la premessa sulla finestra qui sotto lo controlla.
			Pusher->Facing = ERTHexDirection::W;
		}
		else
		{
			ArmBraceScenario(Bracer, Pusher);
		}
		Bracer->Facing = VersoDelBracer;

		// Che cosa vede la squadra di chi si irrigidisce, chiesto come lo chiede il resolver.
		FRTPerceiver Osservatore;
		Osservatore.Cell = Bracer->Cell;
		Osservatore.Facing = Bracer->Facing;
		Osservatore.VisionRange = Bracer->VisionRange;
		Esito.Quanto = URTPerceptionLibrary::TeamAwarenessOfCell(Mappa->MapAsset, { Osservatore }, Pusher->Cell);

		int32 Aperte = 0;
		TM->OnReactionWindowOpened.BindLambda([&Aperte, &Esito](const FRTReactionWindowView& Vista, int32)
		{
			++Aperte;
			Esito.Vista = Vista;
		});
		TM->LockInAndResolve();
		RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM);
		if (Aperte != 1)
		{
			for (const FRTTurnLogEntry& Voce : TM->GetTurnLog())
			{
				AddInfo(FString::Printf(TEXT("TurnLog: %s"), *URTTurnLogLibrary::DescribeEntry(Voce)));
			}
		}
		Esito.bValido = TestEqual(TEXT("premessa: si e' aperta una finestra del Brace"), Aperte, 1)
			&& TestTrue(TEXT("premessa: il verso del Bracer non e' cambiato durante il turno"),
				Bracer->Facing == VersoDelBracer);

		for (int32 Giro = 0; Giro < 8 && TM->IsResolutionSuspended(); ++Giro)
		{
			TM->ExpireReactionWindow();
		}
		TM->OnReactionWindowOpened.Unbind();
		RTWorldFixtures::DestroyWorld(World);
		return Esito;
	};

	// Chi spinge sta a est: guardarlo, e dargli le spalle.
	const FEsito LontanoDiFronte = Gioca(/*bDaLontano=*/ true, ERTHexDirection::E);
	const FEsito LontanoDiSpalle = Gioca(/*bDaLontano=*/ true, ERTHexDirection::W);
	const FEsito VicinoDiSpalle = Gioca(/*bDaLontano=*/ false, ERTHexDirection::W);
	if (!LontanoDiFronte.bValido || !LontanoDiSpalle.bValido || !VicinoDiSpalle.bValido) { return false; }
	if (!TestTrue(TEXT("premessa: da lontano e di fronte, la squadra RILEVA la cella di chi spinge"),
			LontanoDiFronte.Quanto == ERTAwareness::Detected)
		|| !TestTrue(TEXT("premessa: da lontano e di spalle, NON la rileva"),
			LontanoDiSpalle.Quanto != ERTAwareness::Detected)
		|| !TestTrue(TEXT("premessa: a contatto la rileva anche di spalle (consapevolezza ravvicinata)"),
			VicinoDiSpalle.Quanto == ERTAwareness::Detected))
	{
		return false;
	}

	const auto OgniTesto = [](const FRTReactionWindowView& Vista)
	{
		FString Tutto = Vista.ReactionName.ToString() + TEXT("|") + Vista.ReactorName.ToString();
		for (const FText& Nome : Vista.TriggerNames) { Tutto += TEXT("|") + Nome.ToString(); }
		for (const FRTReactionWindowOptionView& Opzione : Vista.Options) { Tutto += TEXT("|") + Opzione.TargetName.ToString(); }
		return Tutto;
	};

	// --- rilevato: la finestra lo nomina ---
	for (const FEsito* Rilevato : { &LontanoDiFronte, &VicinoDiSpalle })
	{
		const TCHAR* Quale = (Rilevato == &LontanoDiFronte) ? TEXT("da lontano, di fronte") : TEXT("a contatto, di spalle");
		TestEqual(*FString::Printf(TEXT("%s: chi decide"), Quale), Rilevato->Vista.ReactorName.ToString(), FString(TEXT("Aevik")));
		TestFalse(*FString::Printf(TEXT("%s: la reazione ha un nome"), Quale), Rilevato->Vista.ReactionName.IsEmptyOrWhitespace());
		if (TestEqual(*FString::Printf(TEXT("%s: un trigger"), Quale), Rilevato->Vista.TriggerNames.Num(), 1))
		{
			TestEqual(*FString::Printf(TEXT("%s: ed e' chi spinge"), Quale),
				Rilevato->Vista.TriggerNames[0].ToString(), FString(TEXT("Ivrin")));
		}
		TestEqual(*FString::Printf(TEXT("%s: il sottotitolo lo nomina"), Quale),
			RTReactionResponseText::SubtitleFor(Rilevato->Vista).ToString(), FString(TEXT("Ivrin ti spinge")));
	}

	// --- non rilevato: nessun nome, in nessun campo ---
	TestEqual(TEXT("🔴 non rilevato: nessun trigger nel DTO"), LontanoDiSpalle.Vista.TriggerNames.Num(), 0);
	TestFalse(TEXT("🔴 non rilevato: il nome di chi spinge non sta in NESSUN testo della vista"),
		OgniTesto(LontanoDiSpalle.Vista).Contains(TEXT("Ivrin")));
	TestEqual(TEXT("non rilevato: il sottotitolo e' neutro"),
		RTReactionResponseText::SubtitleFor(LontanoDiSpalle.Vista).ToString(), FString(TEXT("Un nemico ti spinge")));
	// Cio' che e' SUO resta: chi decide e la sua reazione non dipendono da chi lo spinge.
	TestEqual(TEXT("non rilevato: chi decide c'e' lo stesso"),
		LontanoDiSpalle.Vista.ReactorName.ToString(), FString(TEXT("Aevik")));
	TestEqual(TEXT("non rilevato: e il titolo e' lo stesso degli altri turni"),
		RTReactionResponseText::TitleFor(LontanoDiSpalle.Vista).ToString(),
		RTReactionResponseText::TitleFor(LontanoDiFronte.Vista).ToString());

	return true;
}

/**
 * 🔑 **LA FINESTRA COME LA TAVOLA E** (#3634, T8): la barra del countdown, «Allo scadere: …», la cornice e la
 * striscia; e per ogni opzione il tasto, la frase, la cornice e il chip della scelta sicura.
 *
 * ➕ **E i suoi TESTI** (`#3667`): il titolo «{Reazione} — {Unita'}», il sottotitolo con chi e' entrato, e le
 * frasi «Attacca {bersaglio}» e «Non attaccare». I nomi arrivano dal turno vero, non da una vista montata a
 * mano: e' la catena intera — resolver, vista, view model, widget — che deve portarli fin qui.
 *
 * ⚠️ **La frazione si misura DOPO che l'orologio e' scorso.** Appena aperta vale circa 1, e un `return 1.f`
 * passerebbe: il test fa scorrere un secondo della finestra e confronta col residuo autorevole.
 *
 * ⚠️ **Le opzioni passano da `SetOption` a mano, e qui e' l'unica via.** `MakeOptionWidget` crea il bottone dalla
 * classe, e in una run headless la classe C++ non ha l'albero del `.uasset`: le porte resterebbero nulle. La via
 * di produzione e la scelta di QUALE opzione e' sicura le misura `FastDecisionOptionForwardsItsOwnIndex`; qui si
 * misura cio' che `SetOption` scrive nelle porte, con la stessa regola (`Response == SafeResponse`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastDecisionLookFollowsTheBoardTest,
	"RefactorTactics.ScreenHud.FastDecisionLookFollowsTheBoard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFastDecisionLookFollowsTheBoardTest::RunTest(const FString&)
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
	if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
		|| !TestNotNull(TEXT("TurnManager"), TM) || !TestNotNull(TEXT("GameMode"), GameMode)
		|| !TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("widget"), Finestra))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const auto Colore = [](ERTUIToken Token) { return URTUIPalette::ColorFor(Token); };
	const auto Cornice = [World]()
	{
		UBorder* B = NewObject<UBorder>(World);
		FSlateBrush Brush = B->Background;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		B->SetBrush(Brush);
		return B;
	};
	const auto Contorno = [](const UBorder* B) { return B->Background.OutlineSettings.Color.GetSpecifiedColor(); };
	const auto Visibile = [](const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed; };

	Finestra->WindowFrame = Cornice();
	Finestra->AccentStrip = NewObject<UBorder>(World);
	Finestra->CountdownBar = NewObject<UProgressBar>(World);
	Finestra->SafeOutcomeText = NewObject<UTextBlock>(World);
	Finestra->SubtitleText = NewObject<UTextBlock>(World);

	// Le fixture montano lo stesso eroe per tutti e due: senza nomi distinti il test non vedrebbe uno scambio
	// fra chi decide e chi e' entrato.
	Mover->HeroDisplayName = FText::FromString(TEXT("Ivrin"));
	Watcher->HeroDisplayName = FText::FromString(TEXT("Aevik"));

	// --- A. Nessuna finestra: frazione negativa, «Allo scadere» vuoto, barra a zero ------------------------
	// Il sottotitolo parte con un residuo: senza, «vuoto dopo `RefreshLook`» passerebbe anche se nessuno lo scrive.
	Finestra->SubtitleText->SetText(FText::FromString(TEXT("un residuo")));
	// La barra parte a meta': nasce a zero, e senza questo l'asserto sotto passerebbe anche senza `RefreshLook`.
	Finestra->CountdownBar->SetPercent(0.5f);
	Finestra->RefreshLook();
	TestTrue(TEXT("A: senza finestra la frazione e' negativa"), Finestra->GetCountdownFraction() < 0.f);
	TestTrue(TEXT("A: e «Allo scadere» e' vuoto"), Finestra->GetSafeOutcomeText().IsEmpty());
	TestEqual(TEXT("A: la barra non va sotto lo zero"), Finestra->CountdownBar->GetPercent(), 0.f);
	TestTrue(TEXT("A: la cornice ha il fondo BG_Deep"), Finestra->WindowFrame->GetBrushColor().Equals(Colore(ERTUIToken::BG_Deep)));
	TestTrue(TEXT("A: e il contorno White"), Contorno(Finestra->WindowFrame).Equals(Colore(ERTUIToken::White)));
	TestEqual(TEXT("A: di 2 px, come la tavola"), Finestra->WindowFrame->Background.OutlineSettings.Width, 2.f);
	TestTrue(TEXT("A: la striscia e' Violet"), Finestra->AccentStrip->GetBrushColor().Equals(Colore(ERTUIToken::Violet)));
	TestTrue(TEXT("A: senza finestra nessun titolo"), Finestra->GetPromptText().IsEmpty());
	TestTrue(TEXT("A: e nessun sottotitolo"), Finestra->GetSubtitleText().IsEmpty());
	TestTrue(TEXT("A: la porta del sottotitolo e' svuotata"), Finestra->SubtitleText->GetText().IsEmpty());
	TestFalse(TEXT("A: e tolta, invece di lasciare una riga vuota"), Visibile(Finestra->SubtitleText));

	// --- B. Una finestra vera, con un secondo gia' passato ---------------------------------------------------
	ArmOverwatchScenario(Mover, Watcher);
	GameMode->HookReactionWindow();
	Finestra->SetReactionWindowForTest(PC->GetReactionWindowViewModel());
	TM->LockInAndResolve();
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento
	if (!TestTrue(TEXT("premessa: una finestra attende"), Finestra->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const FRTReactionWindowView Vista = Finestra->GetWindow();
	if (!TestTrue(TEXT("premessa: la finestra ha una durata"), Vista.WindowSeconds > 1.5f)
		|| !TestTrue(TEXT("premessa: offre piu' di una risposta"), Vista.Options.Num() > 1))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	for (int32 Giro = 0; Giro < 20 && Finestra->IsWindowOpen(); ++Giro)
	{
		TM->Tick(0.05f);
	}
	if (!TestTrue(TEXT("premessa: dopo un secondo la finestra e' ancora aperta"), Finestra->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const float Frazione = Finestra->GetCountdownFraction();
	TestTrue(TEXT("B: la frazione e' il residuo autorevole sulla durata"),
		FMath::IsNearlyEqual(Frazione, Finestra->GetRemainingSeconds() / Vista.WindowSeconds, 1e-3f));
	TestTrue(TEXT("B: e dopo un secondo non e' piu' piena"), Frazione < 0.9f);
	TestTrue(TEXT("B: ma nemmeno vuota"), Frazione > 0.1f);
	TestEqual(TEXT("B: allo scadere si applica la scelta sicura, col suo nome (HOLD nell'Overwatch)"),
		Finestra->GetSafeOutcomeText().ToString(), FString(TEXT("Allo scadere: HOLD")));

	Finestra->RefreshLook();
	TestTrue(TEXT("B: la barra segue la frazione"), FMath::IsNearlyEqual(Finestra->CountdownBar->GetPercent(), Frazione, 1e-3f));
	TestTrue(TEXT("B: il riempimento e' White"), Finestra->CountdownBar->GetFillColorAndOpacity().Equals(Colore(ERTUIToken::White)));
	TestEqual(TEXT("B: la porta dice «Allo scadere»"), Finestra->SafeOutcomeText->GetText().ToString(), FString(TEXT("Allo scadere: HOLD")));
	TestTrue(TEXT("B: in Text_Primary"), Finestra->SafeOutcomeText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Text_Primary)));

	// --- B-bis. I testi della tavola E (`#3667`): il titolo e il sottotitolo, coi nomi del turno vero -------------
	const FString NomeDellaReazione = NomeNelKit(Watcher, TEXT("Action.Overwatch"));
	if (TestFalse(TEXT("premessa: il kit del watcher nomina la reazione"), NomeDellaReazione.IsEmpty()))
	{
		TestEqual(TEXT("B: il titolo e' «{reazione} — {chi decide}»"),
			Finestra->GetPromptText().ToString(), NomeDellaReazione + TEXT(" — Aevik"));
	}
	TestEqual(TEXT("B: il sottotitolo dice chi e' entrato"),
		Finestra->GetSubtitleText().ToString(), FString(TEXT("Ivrin è entrato nell'area controllata")));
	TestEqual(TEXT("B: e la porta lo porta"),
		Finestra->SubtitleText->GetText().ToString(), FString(TEXT("Ivrin è entrato nell'area controllata")));
	TestTrue(TEXT("B: visibile"), Visibile(Finestra->SubtitleText));
	TestTrue(TEXT("B: in Text_Secondary"),
		Finestra->SubtitleText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Text_Secondary)));

	// --- C. Le opzioni: tasto, frase, cornice, chip della scelta sicura ---------------------------------------
	int32 Sicure = 0;
	for (int32 i = 0; i < Vista.Options.Num(); ++i)
	{
		URTFastDecisionOptionWidget* Opzione = NewObject<URTFastDecisionOptionWidget>(World);
		Opzione->OptionFrame = Cornice();
		Opzione->KeyText = NewObject<UTextBlock>(World);
		Opzione->DescriptionText = NewObject<UTextBlock>(World);
		Opzione->SafeChoiceBadge = NewObject<UBorder>(World);
		const FString& Risposta = Vista.Options[i].Response;
		const bool bSicura = Risposta == Vista.SafeResponse;
		Sicure += bSicura ? 1 : 0;
		Opzione->SetOption(Finestra, Vista.Options[i], i, bSicura);

		const FString Etichetta = FString::Printf(TEXT("C[%d] %s"), i, *Risposta);
		TestEqual(*(Etichetta + TEXT(": il badge e' il tasto del kit in quella posizione")),
			Opzione->KeyText->GetText().ToString(), ARTPlayerController::ReactionOptionKeyLabel(i).ToString());
		TestFalse(*(Etichetta + TEXT(": e non e' vuoto")), Opzione->KeyText->GetText().IsEmpty());
		// La frase della tavola E (`#3667`): chi spara nomina il bersaglio, chi tiene no. In una finestra
		// dell'Overwatch le risposte sono solo queste due specie.
		const bool bSpara = URTReactionOpportunityLibrary::FireResponseTarget(Risposta) != INDEX_NONE;
		TestEqual(*(Etichetta + TEXT(": la frase e' quella della tavola")),
			Opzione->DescriptionText->GetText().ToString(),
			FString(bSpara ? TEXT("Attacca Ivrin") : TEXT("Non attaccare")));
		TestFalse(*(Etichetta + TEXT(": e non e' vuota")), Opzione->DescriptionText->GetText().IsEmpty());
		TestTrue(*(Etichetta + TEXT(": la frase e' Text_Secondary")),
			Opzione->DescriptionText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Text_Secondary)));
		TestTrue(*(Etichetta + TEXT(": il fondo e' BG_Raised")), Opzione->OptionFrame->GetBrushColor().Equals(Colore(ERTUIToken::BG_Raised)));
		TestEqual(*(Etichetta + TEXT(": il chip c'e' solo sulla scelta sicura")), Visibile(Opzione->SafeChoiceBadge), bSicura);
		TestTrue(*(Etichetta + TEXT(": il contorno e' Text_Primary sulla sicura, Frame_Mid sulle altre")),
			Contorno(Opzione->OptionFrame).Equals(Colore(bSicura ? ERTUIToken::Text_Primary : ERTUIToken::Frame_Mid)));
		TestEqual(*(Etichetta + TEXT(": di 2 px sulla sicura, 1 sulle altre")),
			Opzione->OptionFrame->Background.OutlineSettings.Width, bSicura ? 2.f : 1.f);
	}
	TestEqual(TEXT("C: una sola opzione porta il chip"), Sicure, 1);
	RTWorldFixtures::DestroyWorld(World);

	// --- D. Nel `Brace` la scelta sicura e' «Hold Ground»: «Allo scadere» dice il NOME, non la risposta grezza -----
	// Nell'Overwatch nome e risposta coincidono («HOLD»), e un `SafeResponse` passato senza `NameFor` resterebbe
	// verde. Qui divergono.
	UWorld* MondoBrace = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("D: mondo di prova"), MondoBrace)) { return false; }
	InitVmWorld(MondoBrace);
	SpawnVmMap(MondoBrace);
	ARTUnit* Bracer = SpawnVmUnit(MondoBrace, /*TeamId=*/ 0, FRTCellId(0, 0));
	ARTUnit* Pusher = SpawnVmUnit(MondoBrace, /*TeamId=*/ 1, FRTCellId(1, 0));
	ARTTurnManager* TMBrace = MondoBrace->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GMBrace = MondoBrace->SpawnActor<ARTGameMode>();
	ARTPlayerController* PCBrace = RTWorldFixtures::MakePlayerOnTeam(MondoBrace, /*TeamId=*/ 0);
	URTFastDecisionWidget* FinestraBrace = NewObject<URTFastDecisionWidget>(MondoBrace);
	if (!TestNotNull(TEXT("D: Bracer"), Bracer) || !TestNotNull(TEXT("D: Pusher"), Pusher)
		|| !TestNotNull(TEXT("D: TurnManager"), TMBrace) || !TestNotNull(TEXT("D: GameMode"), GMBrace)
		|| !TestNotNull(TEXT("D: PlayerController"), PCBrace) || !TestNotNull(TEXT("D: widget"), FinestraBrace))
	{
		RTWorldFixtures::DestroyWorld(MondoBrace);
		return false;
	}
	ArmBraceScenario(Bracer, Pusher);
	GMBrace->HookReactionWindow();
	FinestraBrace->SetReactionWindowForTest(PCBrace->GetReactionWindowViewModel());
	TMBrace->LockInAndResolve();
	RTReactionWindowFixtures::PompaFinoAllaPresentazione(TMBrace); // #3666: la finestra si presenta al trattenimento
	if (!TestTrue(TEXT("D: premessa: una finestra del `Brace` attende"), FinestraBrace->IsWindowOpen()))
	{
		RTWorldFixtures::DestroyWorld(MondoBrace);
		return false;
	}
	const FString SicuraBrace = FinestraBrace->GetWindow().SafeResponse;
	const FString NomeSicura = RTReactionResponseText::NameFor(SicuraBrace).ToString();
	// ⚠️ **Confronti CaseSensitive, esplicitamente**: `TestEqual`/`TestNotEqual` su `FString` ignorano le maiuscole,
	// e «HOLD GROUND» contro «Hold Ground» risulterebbe uguale — cioe' proprio la differenza da misurare.
	if (!TestFalse(TEXT("D: premessa: qui nome e risposta divergono"), NomeSicura.Equals(SicuraBrace, ESearchCase::CaseSensitive)))
	{
		RTWorldFixtures::DestroyWorld(MondoBrace);
		return false;
	}
	TestTrue(TEXT("D: «Allo scadere» dice il nome della scelta sicura, non la risposta grezza"),
		FinestraBrace->GetSafeOutcomeText().ToString().Equals(FString(TEXT("Allo scadere: ")) + NomeSicura, ESearchCase::CaseSensitive));

	RTWorldFixtures::DestroyWorld(MondoBrace);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
