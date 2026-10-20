#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "RTGameMode.h"
#include "Tests/RTAbilityFixtures.h"
#include "Tests/RTReactionWindowFixtures.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTReactionWindowViewModel.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakeSuspWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroySuspWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnSuspMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		if (Actor) { Actor->MapAsset = M; }
		return Actor;
	}

	/** Eroe dal CATALOGO, fermo sulla sua cella e non del bot: i piani li scrive il test. */
	ARTUnit* SpawnSuspHero(UWorld* World, FName HeroId, int32 TeamId, const FRTCellId& Cell)
	{
		const URTHeroData* Hero = nullptr;
		for (const URTHeroData* Candidate : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Candidate && Candidate->HeroId == HeroId) { Hero = Candidate; break; }
		}
		if (!World || !Hero) { return nullptr; }

		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay(); // senza, i cooldown non esistono e ogni abilita' risulta sempre pronta
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo salvo che il caso non lo cambi
		return U;
	}

	int32 FindSuspAbilityById(const ARTUnit* Unit, const TCHAR* ActionId)
	{
		if (!Unit) { return INDEX_NONE; }
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* A = Unit->GetAbility(I);
			if (A && A->Def.ActionId == FName(ActionId)) { return I; }
		}
		return INDEX_NONE;
	}
}

/**
 * 🔴 **A RISOLUZIONE SOSPESA SU UNA FINESTRA DI REAZIONE IL MONDO E' IN SOLA LETTURA, ANCHE SENZA PLAYBACK** — `#3837`.
 *
 * D-468 e la spec del puntatore dicono che con una risoluzione in corso nessun input cambia il piano. Il controller
 * lo deriva dal contesto del puntatore, e il contesto guardava `bIsResolving`: la risoluzione SOSPESA su una finestra
 * (l'Overwatch nel Move, qui) lo accende solo se parte il playback parziale, cioe' non con `bEnablePlayback` falso —
 * una proprieta' `EditAnywhere`, la risoluzione istantanea. In quel tratto `Phase` e'
 * `Move`, le azioni di Prep, Dash e Blast sono GIA' consumate e il Cleanup non e' arrivato: un'unita' viva, armata e
 * in ricarica, a mondo scrivibile. Dalla #3835 nessuna guardia di ricarica sull'azione armata la ferma piu'.
 *
 * Cosa succede qui, in ordine:
 *
 *  - **A.** Prima del lock-in l'armo e il click sul bersaglio PIANIFICANO. E' il controllo positivo della sonda: senza,
 *    "il click non pianifica" potrebbe valere per un click che non pianifica mai.
 *  - **B.** Lock-in con un Overwatch che sospende nel Move. Premesse: la finestra e' aperta, la fase e' `Move`,
 *    l'attaccante e' vivo, ancora armato e con la ricarica scritta, e `IsResolving()` dice in quale delle due
 *    configurazioni siamo.
 *  - **C.** Contesto `ResolutionPlayback`, `IsWorldReadOnly()` vero, e un click su un ALTRO nemico (il `Decoy`, cosi'
 *    un piano scritto si distingue da quello consumato) non pianifica niente.
 *
 * ⚠️ **Le due configurazioni girano SEMPRE entrambe.** Con il playback acceso il mondo era gia' in sola lettura (il
 * playback parziale accende `bIsResolving`): e' il controllo positivo che separa il caso del difetto dal caso
 * normale, e resta verde prima e dopo la correzione.
 *
 * ⛔ **Verifica di mutazione**: togliere `IsResolutionSuspended()` da `GetPointerContext()` fa cadere C a playback
 * spento, e solo li'.
 *
 * ⚠️ **Cosa NON copre**: la timeline vuota con il playback acceso, che `BeginPartialPlayback` prevede. Misurata dalla
 * revisione alla sospensione non e' vuota, ne' per l'Overwatch ne' per il Brace: la sequenza non l'ho trovata e non la
 * costruisco a mano. Il sito Blast, la risposta da tasto e il ritorno a mondo scrivibile li prova
 * `Reactions.AWindowStaysAnswerableAndTheWorldWritableAfterwards`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSuspendedResolutionIsReadOnlyTest,
	"RefactorTactics.Reactions.ASuspendedResolutionIsReadOnlyForInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSuspendedResolutionIsReadOnlyTest::RunTest(const FString&)
{
	const auto Prova = [this](bool bPlayback) -> bool
	{
		// La configurazione in ogni messaggio: un rosso deve dire QUALE delle due lo ha prodotto.
		const auto E = [bPlayback](const TCHAR* Testo)
		{
			return FString::Printf(TEXT("playback %s - %s"), bPlayback ? TEXT("acceso") : TEXT("spento"), Testo);
		};

		UWorld* World = MakeSuspWorld();
		if (!TestNotNull(*E(TEXT("mondo di prova")), World)) { return false; }
		SpawnSuspMap(World, /*Radius=*/ 10);

		// Squadra 0: chi attacca (Blast) e chi cammina nella zona (Move). Squadra 1: il bersaglio, un altro nemico per
		// la sonda, e chi tiene l'Overwatch. Il Mover e il Watcher hanno le distanze della fixture di
		// `Reactions.WindowSuspendsResolution`, traslate a est perche' non tocchino la linea dell'attacco.
		ARTUnit* Attacker = SpawnSuspHero(World, TEXT("Hero.Aevik"),  0, FRTCellId(-1, 0, 0));
		ARTUnit* Target   = SpawnSuspHero(World, TEXT("Hero.Ivrin"),  1, FRTCellId( 1, 0, 0));
		ARTUnit* Decoy    = SpawnSuspHero(World, TEXT("Hero.Muiren"), 1, FRTCellId(-3, 0, 0));
		ARTUnit* Mover    = SpawnSuspHero(World, TEXT("Hero.Branth"), 0, FRTCellId( 5, 0, 0));
		ARTUnit* Watcher  = SpawnSuspHero(World, TEXT("Hero.Ivrin"),  1, FRTCellId( 8, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!TestNotNull(*E(TEXT("attaccante")), Attacker) || !TestNotNull(*E(TEXT("bersaglio")), Target)
			|| !TestNotNull(*E(TEXT("secondo nemico")), Decoy) || !TestNotNull(*E(TEXT("chi cammina")), Mover)
			|| !TestNotNull(*E(TEXT("chi tiene l'Overwatch")), Watcher)
			|| !TestNotNull(*E(TEXT("turn manager")), TM) || !TestNotNull(*E(TEXT("controller")), PC))
		{
			DestroySuspWorld(World);
			return false;
		}
		TM->bEnablePlayback = bPlayback;

		// Un'azione ATTIVA con ricarica di almeno due turni, che il click su un'unita' sa pianificare.
		const int32 ConRicarica = FindSuspAbilityById(Attacker, TEXT("Hero.Aevik.LinearDischarge"));
		if (!TestNotEqual(*E(TEXT("premessa: Aevik ha la scarica lineare")), ConRicarica, static_cast<int32>(INDEX_NONE))
			|| !TestTrue(*E(TEXT("premessa: la sua ricarica e' di almeno due turni")),
				Attacker->GetAbility(ConRicarica)->CooldownTurns >= 2))
		{
			DestroySuspWorld(World);
			return false;
		}

		// L'Overwatch e' l'azione PRINCIPALE di chi lo tiene, e il cono E' il facing.
		Watcher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
		Watcher->Facing = ERTHexDirection::W;
		Mover->PlannedCell = FRTCellId(7, 0, 0);

		// --- A. prima del lock-in l'armo e il click PIANIFICANO: la sonda e' viva ---------------------------
		PC->SelectActorForTest(Attacker);
		PC->SelectAbilityForCurrentForTest(ConRicarica);
		PC->HandleClickOnUnitForTest(Target);
		if (!TestEqual(*E(TEXT("A: pronta, l'azione si arma")), Attacker->SelectedAbilityIndex, ConRicarica)
			|| !TestEqual(*E(TEXT("A: il click sul bersaglio pianifica l'azione")), Attacker->PlannedAbilityIndex, ConRicarica)
			|| !TestTrue(*E(TEXT("A: e il bersaglio e' quello cliccato")), Attacker->PlannedAttackTarget == Target))
		{
			DestroySuspWorld(World);
			return false;
		}

		// --- B. lock-in: l'Overwatch sospende nel Move, dopo che il Blast ha consumato l'azione -------------
		int32 FinestreAperte = 0;
		TM->OnReactionWindowOpened.BindLambda(
			[&FinestreAperte](const FRTReactionWindowView&, int32) { ++FinestreAperte; });
		TM->LockInAndResolve();
		RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento

		const bool bPremesse =
			TestTrue(*E(TEXT("premessa: una finestra di reazione si e' aperta")), FinestreAperte > 0)
			&& TestTrue(*E(TEXT("premessa: la risoluzione e' sospesa")), TM->IsResolutionSuspended())
			&& TestEqual(*E(TEXT("premessa: la fase che si e' fermata e' il Move")), TM->GetPhase(), ERTMatchPhase::Move)
			&& TestEqual(*E(TEXT("premessa: IsResolving() dice la configurazione")), TM->IsResolving(), bPlayback)
			&& TestTrue(*E(TEXT("premessa ANTI-VACUITA': l'azione e' stata consumata, quindi e' in ricarica")),
				Attacker->GetAbilityCooldown(ConRicarica) > 0)
			&& TestTrue(*E(TEXT("premessa: l'attaccante e' vivo")), Attacker->IsAlive())
			&& TestEqual(*E(TEXT("premessa: e il Cleanup non e' arrivato, quindi e' ancora armata")),
				Attacker->SelectedAbilityIndex, ConRicarica);
		if (!bPremesse)
		{
			TM->OnReactionWindowOpened.Unbind();
			DestroySuspWorld(World);
			return false;
		}

		// --- C. il mondo e' in sola lettura, e un click non scrive un piano ------------------------------
		TestEqual(*E(TEXT("C: il contesto del puntatore e' la risoluzione in corso")),
			PC->GetPointerContext(), ERTPointerContext::ResolutionPlayback);
		TestTrue(*E(TEXT("C: il mondo e' in sola lettura per l'input")), PC->IsWorldReadOnly());

		PC->HandleClickOnUnitForTest(Decoy);
		TestFalse(*E(TEXT("C: il click su un altro nemico NON pianifica un'azione gia' spesa")),
			Attacker->PlannedAttackTarget == Decoy);

		TM->OnReactionWindowOpened.Unbind();
		DestroySuspWorld(World);
		return true;
	};

	// Le due configurazioni si provano SEMPRE entrambe: una premessa che salta per la prima non deve nascondere l'altra.
	const bool bSenzaPlayback = Prova(false);
	const bool bConPlayback = Prova(true);
	return bSenzaPlayback && bConPlayback;
}

namespace
{
	ARTHexMapActor* SuspWindowMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SuspWindowUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
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

	/** Sito del MOVIMENTO: l'Overwatch di chi guarda sospende nel Move quando l'altro entra nel cono. */
	void SuspArmOverwatch(ARTUnit* Mover, ARTUnit* Watcher)
	{
		Watcher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
		Watcher->Facing = ERTHexDirection::W;
		Watcher->PlannedCell = Watcher->Cell;
		Mover->PlannedCell = FRTCellId(2, 0);
	}

	/** Sito del BLAST: la spinta del Pusher sul Bracer sospende nel Blast, prima del movimento. */
	void SuspArmBrace(ARTUnit* Bracer, ARTUnit* Pusher)
	{
		Bracer->ReactionProfileId = TEXT("Profile.Sidestep");
		Bracer->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Bracer, TEXT("Action.Brace"), 3);
		Bracer->PlannedCell = Bracer->Cell;
		Pusher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Pusher, TEXT("Action.Push"), 3);
		Pusher->PlannedAttackTarget = Bracer;
		Pusher->PlannedCell = Pusher->Cell;
	}
}

/**
 * 🔴 **LA FINESTRA RESTA RISPONDIBILE, E DOPO LA RISPOSTA IL MONDO TORNA SCRIVIBILE** — `#3837`, dalla revisione indipendente.
 *
 * Il primo test di questo file prova che a risoluzione sospesa il mondo e' in sola lettura. Questo prova i due lati che
 * quel test non vede, e che il fix poteva rompere:
 *
 *  - **il giocatore puo' ancora RISPONDERE.** Il tasto del kit con una finestra aperta per chi guarda sceglie l'opzione
 *    (`OnKitHotkey` -> `URTReactionWindowViewModel::ChooseOption` -> `SubmitReactionResponse`). La sola guardia di quella
 *    strada e' `IsGameplayInputBlocked()`, cioe' il Modal, e il fix non deve spostarla sul contesto: sarebbe una finestra a
 *    cui non si puo' rispondere finche' il tempo non scade;
 *  - **dopo la risposta il mondo e' di nuovo scrivibile.** Un contesto che restasse `ResolutionPlayback` a turno finito
 *    bloccherebbe la pianificazione del turno dopo.
 *
 * Entrambi i SITI di sospensione (l'Overwatch nel Move e il Brace nel Blast, `IsResolutionSuspended()` li copre per
 * costruzione) e entrambe le configurazioni del playback.
 *
 * 🔑 **Enter e il lock-in sono le porte che non passavano dal click**: a mondo in sola lettura la dichiarazione "ho
 * deciso" e il Ready sono rifiutati. Il controllo positivo e' in fondo: a mondo scrivibile la stessa dichiarazione riesce,
 * altrimenti il rifiuto di prima potrebbe valere per una unita' non selezionata.
 *
 * ⛔ **Verifica di mutazione** (della revisione): la sospensione contata solo se `Phase == Move` fa cadere il sito Brace;
 * il contesto sempre `ResolutionPlayback` con un TurnManager fa cadere il controllo di ritorno scrivibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWindowStaysAnswerableTest,
	"RefactorTactics.Reactions.AWindowStaysAnswerableAndTheWorldWritableAfterwards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWindowStaysAnswerableTest::RunTest(const FString&)
{
	const auto Prova = [this](bool bBrace, bool bPlayback) -> bool
	{
		const FString Tag = FString::Printf(TEXT("%s, playback %s"),
			bBrace ? TEXT("Brace") : TEXT("Overwatch"), bPlayback ? TEXT("acceso") : TEXT("spento"));
		const auto E = [&Tag](const TCHAR* Testo) { return FString::Printf(TEXT("%s - %s"), *Tag, Testo); };

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(*E(TEXT("mondo")), World)) { return false; }
		World->InitializeActorsForPlay(FURL());
		SuspWindowMap(World, /*Radius=*/ 8);

		// Chi RICEVE la finestra: il Watcher per l'Overwatch (squadra 1), il Bracer per il Brace (squadra 0).
		ARTUnit* Primo = SuspWindowUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Secondo = SuspWindowUnit(World, 1, bBrace ? FRTCellId(1, 0) : FRTCellId(3, 0));
		ARTUnit* Proprio = bBrace ? Primo : Secondo;
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, bBrace ? 0 : 1);
		if (!TestNotNull(*E(TEXT("unita'")), Primo) || !TestNotNull(*E(TEXT("seconda unita'")), Secondo)
			|| !TestNotNull(*E(TEXT("turn manager")), TM) || !TestNotNull(*E(TEXT("game mode")), GameMode)
			|| !TestNotNull(*E(TEXT("controller")), PC))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
		if (bBrace) { SuspArmBrace(Primo, Secondo); } else { SuspArmOverwatch(Primo, Secondo); }
		GameMode->HookReactionWindow(); // la finestra raggiunge il ViewModel del controller, come in partita
		TM->bEnablePlayback = bPlayback;
		PC->SelectActorForTest(Proprio);

		TM->LockInAndResolve();
		RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM);
		URTReactionWindowViewModel* VM = PC->GetReactionWindowViewModel();
		const FRTReactionWindowView Vista = VM ? VM->GetWindow() : FRTReactionWindowView();
		if (!TestTrue(*E(TEXT("premessa: la finestra e' aperta per chi guarda")), Vista.bOpen)
			|| !TestTrue(*E(TEXT("premessa: la risoluzione e' sospesa")), TM->IsResolutionSuspended())
			|| !TestEqual(*E(TEXT("premessa: la fase che si e' fermata")), TM->GetPhase(),
				bBrace ? ERTMatchPhase::Blast : ERTMatchPhase::Move)
			|| !TestEqual(*E(TEXT("premessa: IsResolving() dice la configurazione")), TM->IsResolving(), bPlayback)
			|| !TestTrue(*E(TEXT("premessa: c'e' piu' di un'opzione fra cui scegliere")), Vista.Options.Num() > 1))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}

		// --- il mondo e' in sola lettura, anche per le porte che non passano dal click --------------------
		TestEqual(*E(TEXT("il contesto del puntatore e' la risoluzione in corso")),
			PC->GetPointerContext(), ERTPointerContext::ResolutionPlayback);
		TestTrue(*E(TEXT("il mondo e' in sola lettura per l'input")), PC->IsWorldReadOnly());
		TestFalse(*E(TEXT("Enter non dichiara il piano")), PC->ToggleTurnPlanDeclaredForTest());
		TestFalse(*E(TEXT("e l'unita' non risulta decisa")), Proprio->bTurnPlanDeclared);
		if (!bPlayback)
		{
			// A playback acceso lo stesso tasto salta il playback (presentazione): la porta del Ready si prova dove resta il Ready.
			PC->OnLockInForTest();
			TestFalse(*E(TEXT("il lock-in a risoluzione sospesa non registra nessun Ready")),
				TM->IsParticipantReady(ARTPlayerState::TeamIdOf(PC), ARTPlayerState::ControlGroupOf(PC)));
			TestTrue(*E(TEXT("e la risoluzione resta sospesa")), TM->IsResolutionSuspended());
		}

		// --- ma la finestra resta RISPONDIBILE dal tasto del kit ---------------------------------------
		const FString IdAperta = TM->GetOpenReactionWindowId();
		PC->PressKitHotkeyForTest(Vista.Options.Num() - 1);
		TestTrue(*E(TEXT("il tasto del kit risponde alla finestra")), TM->GetOpenReactionWindowId() != IdAperta);

		for (int32 Giri = 0; (TM->IsResolutionSuspended() || TM->IsResolving()) && Giri < 600; ++Giri)
		{
			TM->Tick(0.05f);
		}
		TestFalse(*E(TEXT("dopo la risposta la risoluzione non e' piu' sospesa")), TM->IsResolutionSuspended());
		TestFalse(*E(TEXT("e il playback e' finito")), TM->IsResolving());
		TestEqual(*E(TEXT("e il turno e' tornato alla pianificazione")), TM->GetPhase(), ERTMatchPhase::Planning);

		// --- e il mondo torna SCRIVIBILE -----------------------------------------------------------------
		TestFalse(*E(TEXT("il mondo e' di nuovo scrivibile")), PC->IsWorldReadOnly());
		TestTrue(*E(TEXT("e il contesto non e' piu' la risoluzione in corso")),
			PC->GetPointerContext() != ERTPointerContext::ResolutionPlayback);
		// Il controllo positivo di Enter: scrivibile, la stessa dichiarazione riesce. Senza, il rifiuto di sopra potrebbe
		// valere per un'unita' non selezionata e non per la sola lettura.
		PC->SelectActorForTest(Proprio);
		TestTrue(*E(TEXT("controllo positivo: a mondo scrivibile Enter dichiara il piano")), PC->ToggleTurnPlanDeclaredForTest());

		RTWorldFixtures::DestroyWorld(World);
		return true;
	};

	// Le quattro combinazioni si provano SEMPRE tutte: una premessa che salta per una non deve nascondere le altre.
	bool bTutte = true;
	for (const bool bBrace : { false, true })
	{
		for (const bool bPlayback : { false, true })
		{
			bTutte = Prova(bBrace, bPlayback) && bTutte;
		}
	}
	return bTutte;
}

/**
 * 🔴 **UN'AZIONE CONSUMATA RESTA ARMATA E IN RICARICA FINO AL CLEANUP, E IL DOCK LA MOSTRA `Selected` MENTRE UNA FINESTRA E' APERTA** — `#3839`.
 *
 * Da [D-531] (`#3830`) l'INPUT non forma piu' la coppia "armata E in ricarica": un'azione in ricarica non si arma. La forma pero' la
 * RISOLUZIONE. `ConsumeAbility` scrive la ricarica quando l'azione esegue, e il disarmo e' del Cleanup: se una finestra di reazione
 * sospende il turno in mezzo, un'unita' viva e' ancora armata e con la ricarica scritta. **Nel Move** (l'Overwatch, il sito di
 * questo test) sono pagate le azioni di Prep, Dash e Blast; **nel Blast** (il Brace) la sospensione precede il pagamento delle azioni
 * del Blast (`SpendStartedAbilities`, a fase finita), quindi ci sono solo quelle di Prep e Dash. I commenti di
 * `ResolveSlotState`, `ComposeAbilityLine` e dei loro test presentavano la coppia come "caso di gioco" prima di `#3830` e poi, per
 * un momento, come "non piu' di gioco": nessuna delle due letture era misurata. Questo e' il test che le sostituisce.
 *
 * Cosa misura, in ordine:
 *
 *  - **A.** A finestra aperta l'attaccante e' vivo, ancora armato, con la ricarica scritta (premesse anti-vacuita').
 *  - **B.** La vista che il dock costruisce (`BuildAbilityCooldowns`, poi `ResolveSlotState` con `bArmed` dall'armo dell'unita', come
 *    `URTActionDockWidget`) risolve lo slot in `Selected`, con la ricarica a vista e `bUsableNow` falso. Il controllo: con `bArmed`
 *    falso la stessa vista sarebbe `Cooldown`, cioe' e' il flag a far vincere `Selected`.
 *  - **C.** Chiusa la finestra e finito il turno, il Cleanup ha disarmato: la coppia era transitoria, e lo slot e' `Cooldown`.
 *
 * ⚠️ **Questo test NON dice che `Selected` sia la lettura giusta** a finestra aperta: dice che e' quella che oggi la vista risolve.
 * La precedenza di `ResolveSlotState` risponde `Selected` a "armata e in ricarica", ma fu pensata per la coppia che formava l'input;
 * che cosa mostrare per un'azione appena spesa non l'ha deciso nessuno, e resta all'autore. Se la cambia, questo test e' il posto in
 * cui dirlo.
 *
 * ⛔ **Non passa dal widget**: `bArmed` lo calcola il test come fa `URTActionDockWidget::GetArmedActionIndex()` (l'armo dell'unita'
 * selezionata). Che nessun widget lo nasconda in risoluzione e' lettura del codice, non una misura: provarlo vuole il banco con il
 * Blueprint del dock (`ScreenHud.DockArmsOnlyTheSelectedAction`).
 *
 * Le due configurazioni del playback girano SEMPRE entrambe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTConsumedArmedActionInTheDockTest,
	"RefactorTactics.Reactions.AConsumedArmedActionReachesTheDockAsSelectedWhileAWindowIsOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTConsumedArmedActionInTheDockTest::RunTest(const FString&)
{
	const auto Prova = [this](bool bPlayback) -> bool
	{
		const auto E = [bPlayback](const TCHAR* Testo)
		{
			return FString::Printf(TEXT("playback %s - %s"), bPlayback ? TEXT("acceso") : TEXT("spento"), Testo);
		};

		UWorld* World = MakeSuspWorld();
		if (!TestNotNull(*E(TEXT("mondo di prova")), World)) { return false; }
		SpawnSuspMap(World, /*Radius=*/ 10);

		// Le stesse distanze di `Reactions.ASuspendedResolutionIsReadOnlyForInput`: chi attacca, il bersaglio, chi cammina nella
		// zona e chi tiene l'Overwatch.
		ARTUnit* Attacker = SpawnSuspHero(World, TEXT("Hero.Aevik"),  0, FRTCellId(-1, 0, 0));
		ARTUnit* Target   = SpawnSuspHero(World, TEXT("Hero.Ivrin"),  1, FRTCellId( 1, 0, 0));
		ARTUnit* Mover    = SpawnSuspHero(World, TEXT("Hero.Branth"), 0, FRTCellId( 5, 0, 0));
		ARTUnit* Watcher  = SpawnSuspHero(World, TEXT("Hero.Ivrin"),  1, FRTCellId( 8, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!TestNotNull(*E(TEXT("attaccante")), Attacker) || !TestNotNull(*E(TEXT("bersaglio")), Target)
			|| !TestNotNull(*E(TEXT("chi cammina")), Mover) || !TestNotNull(*E(TEXT("chi tiene l'Overwatch")), Watcher)
			|| !TestNotNull(*E(TEXT("turn manager")), TM) || !TestNotNull(*E(TEXT("controller")), PC))
		{
			DestroySuspWorld(World);
			return false;
		}
		TM->bEnablePlayback = bPlayback;

		const int32 ConRicarica = FindSuspAbilityById(Attacker, TEXT("Hero.Aevik.LinearDischarge"));
		if (!TestNotEqual(*E(TEXT("premessa: Aevik ha la scarica lineare")), ConRicarica, static_cast<int32>(INDEX_NONE))
			|| !TestTrue(*E(TEXT("premessa: la sua ricarica e' di almeno due turni")),
				Attacker->GetAbility(ConRicarica)->CooldownTurns >= 2))
		{
			DestroySuspWorld(World);
			return false;
		}

		Watcher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
		Watcher->Facing = ERTHexDirection::W;
		Mover->PlannedCell = FRTCellId(7, 0, 0);

		PC->SelectActorForTest(Attacker);
		PC->SelectAbilityForCurrentForTest(ConRicarica);
		Attacker->PlannedAbilityIndex = ConRicarica;
		Attacker->PlannedAttackTarget = Target;

		int32 FinestreAperte = 0;
		TM->OnReactionWindowOpened.BindLambda([&FinestreAperte](const FRTReactionWindowView&, int32) { ++FinestreAperte; });
		TM->LockInAndResolve();
		RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM);

		// --- A. a finestra aperta: viva, ancora armata, con la ricarica scritta ---------------------------
		const bool bPremesse =
			TestTrue(*E(TEXT("premessa: una finestra di reazione si e' aperta")), FinestreAperte > 0)
			&& TestTrue(*E(TEXT("premessa: la risoluzione e' sospesa")), TM->IsResolutionSuspended())
			&& TestTrue(*E(TEXT("A: l'attaccante e' vivo")), Attacker->IsAlive())
			&& TestTrue(*E(TEXT("A: l'azione e' stata consumata, quindi e' in ricarica")), Attacker->GetAbilityCooldown(ConRicarica) > 0)
			&& TestEqual(*E(TEXT("A: e il Cleanup non e' arrivato, quindi e' ancora armata")), Attacker->SelectedAbilityIndex, ConRicarica);
		if (!bPremesse)
		{
			TM->OnReactionWindowOpened.Unbind();
			DestroySuspWorld(World);
			return false;
		}

		// --- B. la vista del dock: `Selected` con la ricarica a vista ------------------------------------
		const auto VistaDellAzione = [&]() -> FRTAbilityCooldownView
		{
			for (const FRTAbilityCooldownView& V : URTHudViewModel::BuildAbilityCooldowns(Attacker))
			{
				if (V.AbilityIndex == ConRicarica) { return V; }
			}
			return FRTAbilityCooldownView();
		};
		const FRTAbilityCooldownView InFinestra = VistaDellAzione();
		if (!TestFalse(*E(TEXT("B: la vista dell'azione esiste")), InFinestra.ActionId.IsNone()))
		{
			TM->OnReactionWindowOpened.Unbind();
			DestroySuspWorld(World);
			return false;
		}
		TestTrue(*E(TEXT("B: la ricarica e' a vista")), InFinestra.TurnsRemaining > 0);
		TestFalse(*E(TEXT("B: e l'azione non e' usabile")), InFinestra.bUsableNow);
		TestEqual(*E(TEXT("B: armata, lo slot e' Selected")),
			URTHudViewModel::ResolveSlotState(InFinestra, /*bArmed=*/ Attacker->SelectedAbilityIndex == InFinestra.AbilityIndex),
			ERTActionSlotState::Selected);
		TestEqual(*E(TEXT("B: controllo, la stessa vista non armata sarebbe Cooldown: il flag fa vincere Selected")),
			URTHudViewModel::ResolveSlotState(InFinestra, /*bArmed=*/ false), ERTActionSlotState::Cooldown);

		// --- C. chiusa la finestra e finito il turno, il Cleanup ha disarmato: la coppia era transitoria --
		for (int32 Giri = 0; TM->IsResolutionSuspended() && Giri < 16; ++Giri)
		{
			TM->ExpireReactionWindow();
		}
		for (int32 Giri = 0; TM->IsResolving() && Giri < 600; ++Giri)
		{
			TM->Tick(0.05f);
		}
		TestFalse(*E(TEXT("C: la risoluzione e' conclusa")), TM->IsResolutionSuspended() || TM->IsResolving());
		TestEqual(*E(TEXT("C: il Cleanup ha disarmato")), Attacker->SelectedAbilityIndex, static_cast<int32>(INDEX_NONE));
		TestEqual(*E(TEXT("C: e lo slot, non armato, e' Cooldown")),
			URTHudViewModel::ResolveSlotState(VistaDellAzione(), /*bArmed=*/ false), ERTActionSlotState::Cooldown);

		TM->OnReactionWindowOpened.Unbind();
		DestroySuspWorld(World);
		return true;
	};

	// Le due configurazioni si provano SEMPRE entrambe: una premessa che salta per la prima non deve nascondere l'altra.
	const bool bSenzaPlayback = Prova(false);
	const bool bConPlayback = Prova(true);
	return bSenzaPlayback && bConPlayback;
}

#endif // WITH_DEV_AUTOMATION_TESTS
