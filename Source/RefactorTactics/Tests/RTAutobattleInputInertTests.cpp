// L'INPUT INERTE DELLA PARTITA NON PRESIDIATA (#971).
//
// `Match.Autobattle.PlaysToCompletionWithoutInput` copre gia' il DoD di #954 — «nessun input NECESSARIO» —
// e qui non si duplica: quello dimostra che la partita arriva a un vincitore da sola, questi dimostrano che
// l'input umano non aggancia piu' niente mentre lo fa.
//
// ⚠️ **Perche' un file a parte e non `RTMatchAutobattleTests.cpp`**: la fixture e' un'altra. Li' si allestisce
// una partita e si guarda il TurnLog; qui serve un `ARTPlayerController` con un'unita' selezionata e le
// azioni armate, cioe' la fixture dei test di `RefactorTactics.PlayerInput.*`. I nomi restano sotto
// `RefactorTactics.Match.Autobattle.*` perche' e' la modalita' a essere sotto misura, non il puntatore.
//
// ⛔ **Il criterio di non-regressione NON e' `RTCombatLibraryTests`.** La strada scelta non tocca
// `URTCombatLibrary::CanPlayerControlUnit`, quindi quelle quattro asserzioni sarebbero verdi per
// costruzione — un criterio che non puo' fallire non misura niente. La rete vera e' la famiglia
// `RefactorTactics.Pacing.*`, che resta invariata, piu' il passo di CONTROLLO dentro ogni test qui sotto:
// con l'autobattle spento gli stessi cinque siti pianificano ancora.

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Camera/RTCameraPawn.h"
#include "RTGameMode.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTPacing.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTCellId.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "HAL/IConsoleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Definite in RTGameMode.cpp: le due sorgenti «di adesso» della modalita' non presidiata. */
extern TAutoConsoleVariable<int32> CVarRTAutobattle;
extern TAutoConsoleVariable<float> CVarRTPlanningSeconds;

/** Definita in ScenarioHarness/RTTestConsole.cpp: con uno scenario impostato vince lo scenario. */
extern TAutoConsoleVariable<FString> CVarRTTestScenario;

namespace
{
	// ⚠️ Nomi tutti prefissati `Inert`: la unity build condivide la translation unit con gli altri file di
	// test, e `FindAreaAbility`/`SpawnPointerUnit` esistono gia' con un altro corpo.

	/**
	 * Riporta riga di comando e console variable com'erano, qualunque cosa succeda nel test.
	 *
	 * Stessa guardia — e stessa ragione misurata — di `RTMatchAutobattleTests.cpp`: sono stato del PROCESSO,
	 * e uno sporco qui non fallisce in questo file ma nel prossimo, che in una unity build puo' essere
	 * qualsiasi cosa.
	 */
	struct FRTScopedInertSessionState
	{
		FString SavedCommandLine;
		int32 SavedMode;
		float SavedPlanning;
		FString SavedScenario;

		FRTScopedInertSessionState()
			: SavedCommandLine(FCommandLine::Get())
			, SavedMode(CVarRTAutobattle.GetValueOnGameThread())
			, SavedPlanning(CVarRTPlanningSeconds.GetValueOnGameThread())
			, SavedScenario(CVarRTTestScenario.GetValueOnGameThread())
		{
			// La modalita' la decide la proprieta' del GameMode in questi test: le due sorgenti «di adesso»
			// si mettono a riposo, altrimenti una CVar lasciata accesa da un altro test deciderebbe al posto
			// del passo di controllo — e il controllo diventerebbe la misura.
			FCommandLine::Set(*SavedCommandLine);
			CVarRTAutobattle->Set(-1, ECVF_SetByCode);
			CVarRTTestScenario->Set(TEXT(""), ECVF_SetByCode);
		}

		~FRTScopedInertSessionState()
		{
			FCommandLine::Set(*SavedCommandLine);
			CVarRTAutobattle->Set(SavedMode, ECVF_SetByCode);
			CVarRTPlanningSeconds->Set(SavedPlanning, ECVF_SetByCode);
			CVarRTTestScenario->Set(*SavedScenario, ECVF_SetByCode);
		}
	};

	ARTUnit* SpawnInertUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay(); // senza, i cooldown restano vuoti e ogni abilita' risulta sempre pronta
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** L'azione si CERCA per proprieta': un riordino del kit non deve rendere rosso questo file. */
	int32 FindInertAreaAbility(const ARTUnit* U)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && A->Shape == ERTAbilityShape::Area && !A->bSelfTarget) { return i; }
		}
		return INDEX_NONE;
	}

	int32 FindInertStructureAbility(const ARTUnit* U)
	{
		for (int32 i = 0; i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && A->Def.StructureOp != ERTStructureOp::None) { return i; }
		}
		return INDEX_NONE;
	}

	/** Azzera cio' che i cinque siti scrivono, cosi' il passo successivo misura sole scritture NUOVE. */
	void ClearInertPlan(ARTUnit* U)
	{
		if (!U) { return; }
		U->PlannedAbilityIndex = INDEX_NONE;
		U->PlannedAttackTarget = nullptr;
		U->PlannedAttackCell = FRTCellId();
		U->bAttackTargetsCell = false;
		U->bHasPlannedCoverEdge = false;
		U->bDeclaresPlannedFacing = false;
		U->SelectAbility(INDEX_NONE);
	}

	/**
	 * Il banco: mappa, TurnManager, GameMode, controller e tre unita' a distanza 1 l'una dall'altra.
	 *
	 * ⚠️ Le unita' si posano PRIMA di `SetupHexMatch`, ed e' deliberato: e' il ramo del «livello che porta
	 * gia' le proprie unita'», dove l'allestimento automatico ritorna prima di `SpawnHero` e la modalita' si
	 * applica lo stesso. E' anche il quarto caso che #971 usa per distinguere le due strade, quindi qui non
	 * serve un test separato: **ogni** test di questo file lo attraversa.
	 */
	struct FRTInertBench
	{
		UWorld* World = nullptr;
		ARTHexMapActor* Map = nullptr;
		ARTGameMode* GameMode = nullptr;
		ARTTurnManager* TurnManager = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Gadget = nullptr;   // team 0, azione base + area
		ARTUnit* Riktor = nullptr;   // team 0, azione su struttura
		ARTUnit* Enemy = nullptr;    // team 1

		bool IsComplete() const
		{
			return World && Map && GameMode && TurnManager && PC && Gadget && Riktor && Enemy;
		}

		/** Rilatcha la modalita' passando dalla porta vera. */
		void Setup(bool bAutobattle)
		{
			GameMode->bAutobattle = bAutobattle;
			GameMode->SetupHexMatch(Map);
		}
	};

	FRTInertBench MakeInertBench(bool bWithTurnManagerBeginPlay)
	{
		FRTInertBench B;
		B.World = RTWorldFixtures::MakeWorld();
		if (!B.World) { return B; }

		URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 5);
		B.Map = B.World->SpawnActor<ARTHexMapActor>();
		if (B.Map) { B.Map->MapAsset = Asset; }

		B.TurnManager = B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (B.TurnManager && bWithTurnManagerBeginPlay)
		{
			// E' `BeginPlay` a chiamare `StartPlanningTimer`, che APRE il campione di pacing del primo
			// turno. Senza, il campione sarebbe non aperto e i tempi risulterebbero `Unmeasured` per la
			// PRIMA causa: il test sulla seconda non distinguerebbe piu' niente.
			B.TurnManager->DispatchBeginPlay();
		}

		B.Gadget = SpawnInertUnit(B.World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(0, 0, 0));
		B.Riktor = SpawnInertUnit(B.World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(-1, 0, 0));
		B.Enemy  = SpawnInertUnit(B.World, 1, URTHeroCatalogLibrary::MakeWraith(), FRTCellId(1, 0, 0));

		B.GameMode = B.World->SpawnActor<ARTGameMode>();
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		return B;
	}
}

// ======================================================================================================
// I cinque siti che registrano un `ERTPlanningInput::Order`
// ======================================================================================================

/**
 * NESSUNO DEI CINQUE SITI `Order` PRODUCE UN PIANO CON L'AUTOBATTLE IN VIGORE.
 *
 * 🔴 **Perche' cinque e non uno.** Il DoD originale chiedeva che «un click su un'unita' qualsiasi non
 * produca un piano umano», ed e' un insieme piu' stretto del titolo: gli ordini non passano tutti da un
 * click su unita' — conferma del bersaglio a cella, bordo, rotazione e i dieci tasti abilita' hanno ingressi
 * propri. Una guardia sulla sola selezione avrebbe lasciato vivo il difetto **superando il criterio che
 * doveva misurarlo**.
 *
 * ⚠️ **Ogni sito ha il suo CONTROLLO nello stesso test**, con l'autobattle spento e sulla stessa fixture:
 * senza, «non ha pianificato» sarebbe indistinguibile da «la fixture non permetteva di pianificare», che e'
 * il modo piu' comune di scrivere un verde che non misura niente.
 *
 * Verifica di mutazione: togliere la guardia da uno qualunque dei cinque `Handle*`/`SelectAbilityForCurrent`
 * fa cadere la riga corrispondente del passo 2, e solo quella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleOrderSitesInertTest,
	"RefactorTactics.Match.Autobattle.PlanningInputIsInertOnEveryOrderSite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleOrderSitesInertTest::RunTest(const FString&)
{
	FRTScopedInertSessionState StateGuard;

	FRTInertBench B = MakeInertBench(/*bWithTurnManagerBeginPlay=*/ false);
	if (!TestTrue(TEXT("banco completo"), B.IsComplete()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	const int32 AreaIdx = FindInertAreaAbility(B.Gadget);
	const int32 StructIdx = FindInertStructureAbility(B.Riktor);
	if (!TestTrue(TEXT("premessa: Gadget ha un'azione ad AREA"), AreaIdx != INDEX_NONE)
		|| !TestTrue(TEXT("premessa: Riktor ha un'azione su STRUTTURA"), StructIdx != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// ---------------------------------------------------------------------------------------------
	// PASSO 1 — CONTROLLO: autobattle spento, gli stessi cinque siti pianificano.
	// ---------------------------------------------------------------------------------------------
	B.Setup(/*bAutobattle=*/ false);
	if (!TestFalse(TEXT("controllo: l'input non e' inerte"), B.PC->IsPlanningInputInert()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// (1) click sul nemico con l'azione armata
	B.PC->SelectActorForTest(B.Gadget);
	B.Gadget->SelectAbility(0);
	B.PC->HandleClickOnUnitForTest(B.Enemy);
	TestTrue(TEXT("controllo (1): il click sul nemico registra il bersaglio"),
		B.Gadget->PlannedAttackTarget == B.Enemy);

	// (2) tasto abilita'
	ClearInertPlan(B.Gadget);
	B.PC->SelectAbilityForCurrentForTest(AreaIdx);
	TestEqual(TEXT("controllo (2): il tasto abilita' arma l'azione"),
		B.Gadget->SelectedAbilityIndex, AreaIdx);

	// (3) bersaglio a cella
	TestTrue(TEXT("controllo (3): il bersaglio a cella e' accettato"),
		B.PC->HandleTargetCell(FRTCellId(0, -1, 0)));

	// (4) bersaglio a bordo
	ClearInertPlan(B.Gadget);
	B.PC->SelectActorForTest(B.Riktor);
	B.Riktor->SelectAbility(StructIdx);
	TestTrue(TEXT("controllo (4): il bersaglio a bordo e' accettato"),
		B.PC->HandleTargetEdge(FRTCellId(-2, 0, 0), ERTHexDirection::NE));

	// (5) rotazione dichiarata
	ClearInertPlan(B.Riktor);
	B.PC->SelectActorForTest(B.Gadget);
	B.PC->BeginFacingDeclaration();
	TestTrue(TEXT("controllo (5): la rotazione dichiarata e' accettata"),
		B.PC->HandleFacingSector(ERTHexDirection::W));

	// ---------------------------------------------------------------------------------------------
	// PASSO 2 — LA MISURA: autobattle in vigore, gli stessi cinque siti non agganciano piu' niente.
	// ---------------------------------------------------------------------------------------------
	ClearInertPlan(B.Gadget);
	ClearInertPlan(B.Riktor);
	B.Setup(/*bAutobattle=*/ true);
	if (!TestTrue(TEXT("la misura: l'input e' inerte"), B.PC->IsPlanningInputInert()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	B.PC->SelectActorForTest(B.Gadget);
	B.Gadget->SelectAbility(0);
	B.PC->HandleClickOnUnitForTest(B.Enemy);
	TestNull(TEXT("(1) il click sul nemico non registra nessun bersaglio"),
		(void*)B.Gadget->PlannedAttackTarget.Get());
	TestEqual(TEXT("(1) e non pianifica nessuna azione"),
		B.Gadget->PlannedAbilityIndex, (int32)INDEX_NONE);

	B.Gadget->SelectAbility(INDEX_NONE);
	B.PC->SelectAbilityForCurrentForTest(AreaIdx);
	TestEqual(TEXT("(2) il tasto abilita' non arma niente"),
		B.Gadget->SelectedAbilityIndex, (int32)INDEX_NONE);

	// Il bersaglio a cella si prova con l'azione armata A MANO: cosi' il rifiuto viene dalla guardia e non
	// dal fatto che il passo (2) ha gia' impedito di armarla.
	B.Gadget->SelectAbility(AreaIdx);
	TestFalse(TEXT("(3) il bersaglio a cella e' rifiutato"),
		B.PC->HandleTargetCell(FRTCellId(0, -1, 0)));
	TestFalse(TEXT("(3) e non ha sporcato il piano"), B.Gadget->bAttackTargetsCell);

	B.PC->SelectActorForTest(B.Riktor);
	B.Riktor->SelectAbility(StructIdx);
	TestFalse(TEXT("(4) il bersaglio a bordo e' rifiutato"),
		B.PC->HandleTargetEdge(FRTCellId(-2, 0, 0), ERTHexDirection::NE));
	TestFalse(TEXT("(4) e nessun lato e' stato registrato"), B.Riktor->bHasPlannedCoverEdge);

	B.PC->SelectActorForTest(B.Gadget);
	B.PC->BeginFacingDeclaration();
	TestFalse(TEXT("(5) la rotazione dichiarata e' rifiutata"),
		B.PC->HandleFacingSector(ERTHexDirection::W));
	TestFalse(TEXT("(5) e nessun facing e' stato dichiarato"), B.Gadget->bDeclaresPlannedFacing);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

// ======================================================================================================
// Il turno, che non e' un piano
// ======================================================================================================

/**
 * IL LOCK-IN NON CHIUDE IL TURNO IN UNA SESSIONE NON PRESIDIATA.
 *
 * 🔴 **Il difetto che nessun criterio scritto sui siti `Order` avrebbe raggiunto.** `OnLockIn` — il tasto
 * Spazio — non chiama `RecordPlanningInput` affatto: durante il playback **salta la risoluzione**,
 * altrimenti **chiude la pianificazione**. Per una modalita' che esiste per essere registrata in video e'
 * peggio del piano che evapora: non rende il filmato confuso, lo taglia.
 *
 * Il controllo e la misura usano banchi separati perche' il turno, una volta risolto, non si annulla.
 *
 * ---
 *
 * 🔵 **Riscritto il 2026-09-04 dopo `#2193`** (`#2356`), che ha cambiato *cosa* fa il tasto: non chiude piu'
 * il turno subito, **arma un countdown di 3 s** e committa al suo scadere. Il canary del controllo e' caduto
 * su `main`, e ha fatto il suo mestiere — senza di lui la misura sarebbe diventata **vacua in silenzio**:
 * *«il lock-in non chiude il turno»* aveva smesso di distinguere l'autobattle da qualunque altra modalita',
 * perche' il lock-in non chiudeva piu' niente da nessuna parte.
 *
 * ⚠️ **La correzione non rilassa il controllo: lo raddoppia.** `A` prova che il tasto arriva al
 * `TurnManager` (il countdown si arma), `B` che con la via sincrona chiude davvero un turno. E la misura
 * gira con `ReadyCountdownSeconds = 0`, cioe' nella configurazione in cui un input non inerte chiuderebbe il
 * turno **nello stesso frame**: e' piu' severa di prima, non meno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleLockInInertTest,
	"RefactorTactics.Match.Autobattle.LockInDoesNotCloseTheTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleLockInInertTest::RunTest(const FString&)
{
	FRTScopedInertSessionState StateGuard;

	// CONTROLLO A: senza la modalita', il tasto ARMA il countdown. E' l'osservabile che separa le due
	// modalita' sulla semantica nuova, ed e' piu' vicino al difetto di quello vecchio: prova che l'input e'
	// arrivato al `TurnManager`, non che il turno sia avanzato.
	{
		FRTInertBench B = MakeInertBench(/*bWithTurnManagerBeginPlay=*/ true);
		if (!TestTrue(TEXT("banco di controllo completo"), B.IsComplete()))
		{
			RTWorldFixtures::DestroyWorld(B.World);
			return false;
		}
		B.Setup(/*bAutobattle=*/ false);

		B.PC->OnLockInForTest();
		TestTrue(TEXT("controllo: il lock-in ha ARMATO il countdown"),
			B.TurnManager->IsReadyCountdownActive());

		RTWorldFixtures::DestroyWorld(B.World);
	}

	// CONTROLLO B: e con il countdown a zero lo stesso tasto chiude un turno, come prima di `#2193`.
	//
	// 🔴 **Serve, e non e' ridondante col controllo A.** Il countdown si arma anche se il commit poi non
	// arrivasse mai: senza questo passo, «il tasto fa qualcosa» non sarebbe «il tasto chiude il turno», e la
	// misura qui sotto tornerebbe a non distinguere la guardia da un banco che non sapeva avanzare — che e'
	// la ragione per cui il controllo esiste dal 2026-08 (`#971`).
	{
		FRTInertBench B = MakeInertBench(/*bWithTurnManagerBeginPlay=*/ true);
		if (!TestTrue(TEXT("banco di controllo completo"), B.IsComplete()))
		{
			RTWorldFixtures::DestroyWorld(B.World);
			return false;
		}
		B.Setup(/*bAutobattle=*/ false);
		B.TurnManager->SetReadyCountdownSeconds(0.f); // la via sincrona, senza tempo di parete nel test

		const int32 Before = B.TurnManager->GetPacingSamples().Num();
		B.PC->OnLockInForTest();
		TestEqual(TEXT("controllo: senza countdown il lock-in chiude un turno"),
			B.TurnManager->GetPacingSamples().Num(), Before + 1);

		RTWorldFixtures::DestroyWorld(B.World);
	}

	// LA MISURA — con il countdown a ZERO, cioe' nella configurazione in cui un input NON inerte chiuderebbe
	// il turno **nello stesso frame**. E' piu' severa della stesura precedente: prima il countdown non
	// esisteva e questa scelta non c'era; adesso lasciarlo a 3 s renderebbe «non ha chiuso il turno» vero
	// anche per un input arrivato e semplicemente in attesa.
	{
		FRTInertBench B = MakeInertBench(/*bWithTurnManagerBeginPlay=*/ true);
		if (!TestTrue(TEXT("banco della misura completo"), B.IsComplete()))
		{
			RTWorldFixtures::DestroyWorld(B.World);
			return false;
		}
		B.Setup(/*bAutobattle=*/ true);
		B.TurnManager->SetReadyCountdownSeconds(0.f);

		const int32 Before = B.TurnManager->GetPacingSamples().Num();
		B.PC->OnLockInForTest();
		TestEqual(TEXT("il lock-in non chiude il turno"),
			B.TurnManager->GetPacingSamples().Num(), Before);
		TestFalse(TEXT("e non ha nemmeno armato un countdown: l'input non e' arrivato affatto"),
			B.TurnManager->IsReadyCountdownActive());

		RTWorldFixtures::DestroyWorld(B.World);
	}

	return true;
}

// ======================================================================================================
// Cosa resta vivo allo spettatore
// ======================================================================================================

/**
 * L'INERZIA E' DELLA PIANIFICAZIONE, NON DELLA VISTA.
 *
 * ⚠️ **L'attore e' lo SPETTATORE**, ed e' la ragione per cui questa riga e' una decisione e non un
 * dettaglio: una telecamera che smette di rispondere e' il terzo modo di rovinare la registrazione, dopo il
 * piano che evapora e il turno che si chiude da solo.
 *
 * La risposta non e' nuova: `IsGameplayInputBlocked()` la dichiara gia' per la propria causa — *«non blocca
 * la CAMERA, ed e' deliberato: pan, zoom, orbita e recenter non toccano il piano ne' la simulazione»*. Qui
 * si applica lo stesso contratto alla seconda causa, e lo si rende osservabile.
 *
 * 🔴 **La seconda meta' di questo test pinna una DECISIONE DI DISEGNO, non un comportamento**:
 * `IsGameplayInputBlocked()` resta **falso** mentre la sessione e' non presidiata. Sono due predicati e non
 * uno perche' quel funnel include `OnRestart`, che agisce solo a `MatchEnded`: fonderli — la
 * semplificazione ovvia, una riga invece di otto — toglierebbe allo spettatore l'unico modo di rilanciare
 * la demo finita. Chi la tentasse fa cadere questa riga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattleCameraStaysAliveTest,
	"RefactorTactics.Match.Autobattle.CameraStaysAliveForTheSpectator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattleCameraStaysAliveTest::RunTest(const FString&)
{
	FRTScopedInertSessionState StateGuard;

	FRTInertBench B = MakeInertBench(/*bWithTurnManagerBeginPlay=*/ false);
	if (!TestTrue(TEXT("banco completo"), B.IsComplete()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	ARTCameraPawn* Cam = B.World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("camera"), Cam))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	Cam->SetSensitivitiesForTest(/*Yaw=*/ 1.f, /*Pitch=*/ 1.f);
	B.PC->Possess(Cam);

	B.Setup(/*bAutobattle=*/ true);
	if (!TestTrue(TEXT("premessa: l'input di pianificazione e' inerte"), B.PC->IsPlanningInputInert()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	const float Yaw0 = Cam->GetCameraYaw();
	B.PC->SetOrbitingForTest(true);
	B.PC->OrbitCameraForTest(FVector2D(30.f, 0.f));
	TestNotEqual(TEXT("con la partita non presidiata la telecamera risponde ancora"),
		Cam->GetCameraYaw(), Yaw0);

	// La decisione di disegno, resa falsificabile: due predicati, non uno.
	TestFalse(TEXT("e il contratto del puntatore resta intatto: nessuna schermata bloccante"),
		B.PC->IsGameplayInputBlocked());

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

// ======================================================================================================
// Il campione di ritmo
// ======================================================================================================

/**
 * I TEMPI DEL CAMPIONE DI RITMO DICHIARANO DI NON ESSERE STATI MISURATI.
 *
 * 🔴 **La domanda di #971 era binaria — «registra, o tace?» — e ha una terza risposta, che il progetto
 * pratica gia'.** Rendere l'input inerte fa tacere i quattro siti che alimentano il campione, ma
 * `StartPlanningTimer` lo APRE lo stesso: i tempi resterebbero tutti misurabili e tutti veri di un
 * cronometro che nessuno guardava. `MsSinceLastInput` verrebbe uguale a `MsToLockIn`, che classifica il
 * turno fra le **attese a vuoto** — la classificazione giusta per un umano che non ha toccato niente, e
 * falsa per una partita in cui non c'era nessun umano.
 *
 * `FRTPacingSample::Unmeasured` esiste esattamente per non produrre quel dato plausibile e falso (#1421), e
 * questa e' la sua seconda causa.
 *
 * ⚠️ **I CONTEGGI restano, e sono veri**: nessun input e' stato accettato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAutobattlePacingUnmeasuredTest,
	"RefactorTactics.Match.Autobattle.PacingTimesAreDeclaredUnmeasured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAutobattlePacingUnmeasuredTest::RunTest(const FString&)
{
	FRTScopedInertSessionState StateGuard;

	// CONTROLLO: senza la modalita', lo stesso banco produce tempi MISURATI. E' cio' che rende la misura
	// qui sotto una misura e non una tautologia — un campione mai aperto sarebbe `Unmeasured` comunque, per
	// la PRIMA causa.
	{
		FRTInertBench B = MakeInertBench(/*bWithTurnManagerBeginPlay=*/ true);
		if (!TestTrue(TEXT("banco di controllo completo"), B.IsComplete()))
		{
			RTWorldFixtures::DestroyWorld(B.World);
			return false;
		}
		B.Setup(/*bAutobattle=*/ false);
		TestFalse(TEXT("controllo: la sessione e' presidiata"), B.TurnManager->IsUnattendedSession());

		RTWorldFixtures::PlayOneTurn(B.TurnManager);
		if (TestTrue(TEXT("controllo: un campione e' stato chiuso"),
			B.TurnManager->GetPacingSamples().Num() > 0))
		{
			const FRTPacingSample& S = B.TurnManager->GetPacingSamples().Last();
			TestNotEqual(TEXT("controllo: il tempo al lock-in E' stato misurato"),
				S.MsToLockIn, FRTPacingSample::Unmeasured);
		}

		RTWorldFixtures::DestroyWorld(B.World);
	}

	// LA MISURA.
	{
		FRTInertBench B = MakeInertBench(/*bWithTurnManagerBeginPlay=*/ true);
		if (!TestTrue(TEXT("banco della misura completo"), B.IsComplete()))
		{
			RTWorldFixtures::DestroyWorld(B.World);
			return false;
		}
		B.Setup(/*bAutobattle=*/ true);

		// Il cablaggio: e' l'allestimento a INFORMARE il TurnManager, che non interroga nessuno.
		TestTrue(TEXT("l'allestimento ha dichiarato la sessione non presidiata"),
			B.TurnManager->IsUnattendedSession());

		RTWorldFixtures::PlayOneTurn(B.TurnManager);
		if (TestTrue(TEXT("un campione e' stato chiuso"), B.TurnManager->GetPacingSamples().Num() > 0))
		{
			const FRTPacingSample& S = B.TurnManager->GetPacingSamples().Last();
			TestEqual(TEXT("il tempo al lock-in dichiara di non essere misurato"),
				S.MsToLockIn, FRTPacingSample::Unmeasured);
			TestEqual(TEXT("e cosi' il tempo dall'ultimo input"),
				S.MsSinceLastInput, FRTPacingSample::Unmeasured);
			TestEqual(TEXT("e il tempo al primo input"),
				S.MsToFirstInput, FRTPacingSample::Unmeasured);

			// I conteggi NON sono `Unmeasured`: sono zero, ed e' un fatto vero.
			TestEqual(TEXT("nessun ordine e' stato registrato"), S.OrderCount, 0);
			TestEqual(TEXT("nessuna selezione"), S.SelectionCount, 0);
			TestEqual(TEXT("nessun annullamento"), S.UndoCount, 0);
		}

		RTWorldFixtures::DestroyWorld(B.World);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
