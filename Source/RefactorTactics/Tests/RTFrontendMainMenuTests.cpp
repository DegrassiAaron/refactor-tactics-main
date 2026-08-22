// CP 46.3 (#938) — il Main Menu, e cio' che lo porta a schermo.
//
// ⚠️ **Qui non si prova il menu, si prova quello che il menu legge.** Il layout, il focus visibile e la
// navigazione da tastiera vivono dentro `WBP_RT_MainMenu.uasset` e restano di `PIE-V01-FRONTEND-MAIN`:
// nessun test headless puo' dire se un bordo di focus si vede. Cio' che invece si prova senza editor e'
// tutto quanto sta **sotto** il layout — la label di versione, la dichiarazione di *coming soon*, e
// soprattutto l'aggancio che a CP 46.1/46.2 non esisteva: `InitializeFrontend` non era chiamata da
// nessuna parte fuori dai test, misurato su `18b3f105`.
//
//     grep -rn "InitializeFrontend\|RegisterScreen" Source/ | grep -v Frontend/RTFrontendNavigator
//
// rispondeva solo con file di test e con un commento di `RTReplayViewerSubsystem.h` che rimandava qui.

#include "Misc/AutomationTest.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "Frontend/RTFrontendWidgets.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
// Solo per avere una `UUserWidget` **concreta** da registrare: `UUserWidget` e' `Abstract` e
// `CreateWidget` la rifiuta. Stessa scelta, e stessa motivazione, di `RTFrontendNavigationTests.cpp`.
#include "UI/RTScreenHudWidgets.h"
#include "Misc/ConfigCacheIni.h"
// Per il GameMode del frontend e il mondo in cui viene spawnato: l'aggancio fra la mappa e il navigatore
// non si prova senza un mondo, e il progetto ha gia' il modo di costruirne uno (`RTCameraPawnTests.cpp`).
#include "Frontend/RTFrontendGameMode.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
// Il mondo di prova: `RTWorldFixtures::MakeWorld()`/`DestroyWorld()`, gia' usate da sette file di test.
#include "Tests/RTWorldFixtures.h"
// Per verificare che il GameMode del frontend NON porti il pawn volante di `AGameModeBase`.
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Un mondo minimo con una `GameInstance` attaccata.
	 *
	 * ⚠️ **La `GameInstance` non e' un dettaglio di allestimento: e' il soggetto del test.** Il navigatore e'
	 * un `UGameInstanceSubsystem`, quindi il GameMode lo raggiunge *attraverso* la `GameInstance` del mondo —
	 * e un mondo senza `GameInstance` e' esattamente il caso in cui quel percorso si rompe.
	 *
	 * ⚠️ **Il mondo lo costruisce `RTWorldFixtures::MakeWorld()`, non questo file.** La prima stesura ne riscriveva il
	 * corpo — `CreateWorld` + `CreateNewWorldContext` — che e' esattamente la duplicazione per cui
	 * `RTWorldFixtures.h` e' stato estratto, e che sette file usano gia'. Trovato in code review. Qui resta
	 * solo cio' che quella fixture non fa: attaccare la `GameInstance`.
	 */
	UWorld* MakeFrontendWorld(UGameInstance*& OutGI, bool bWithGameInstance = true)
	{
		OutGI = nullptr;

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World || !bWithGameInstance)
		{
			return World;
		}

		OutGI = NewObject<UGameInstance>(GetTransientPackage());
		if (OutGI)
		{
			OutGI->AddToRoot();
			OutGI->Init();
			World->SetGameInstance(OutGI);
		}

		return World;
	}

	void DestroyFrontendWorld(UWorld* World, UGameInstance* GI)
	{
		RTWorldFixtures::DestroyWorld(World);
		if (GI)
		{
			GI->Shutdown();
			GI->RemoveFromRoot();
		}
	}

	/**
	 * I `WBP_RT_*` del frontend **non esistono ancora** — sono lavoro d'editor (seduta U28) — quindi ogni
	 * test che arriva alla presentazione produce un tentativo di caricamento che fallisce.
	 *
	 * ⚠️ **Si dichiara invece di lasciarlo passare.** Trovato in code review: la docstring di questo file
	 * sosteneva che i test non caricassero nulla, ed era vero solo per la *registrazione* — `StartFrontend`
	 * apre subito la radice, e `PresentWidget` risolve il `TSoftClassPtr`. Il log ne portava la prova
	 * (`Failed to find object .../WBP_RT_MainMenu_C`) mentre i test uscivano verdi: output sporco che
	 * nessuno guardava. Dichiararlo rende il rumore un'aspettativa, e un rumore **diverso** un fallimento.
	 */
	void ExpectMissingFrontendAssets(FAutomationTestBase& Test)
	{
		// `0` = «una o piu' volte»: il warning del progetto e' **deterministico**, perche' ogni test parte da
		// un navigatore nuovo con `LiveWidgets` vuota e ripassa da `LoadSynchronous`. Se un giorno smettesse
		// di comparire, il test lo direbbe — ed e' cio' che serve, dato che quel warning e' l'unico segnale
		// di un nome sbagliato nel `.ini`.
		Test.AddExpectedMessage(TEXT("non si carica"),
			ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 0);

		// ⚠️ **`-1` = «ignora», e la prima stesura ci aveva messo `0`.** Il messaggio dell'engine e'
		// **dedotto una volta per processo**: `LogUObjectGlobals` lo emette al primo lookup fallito e tace
		// per tutti i successivi, quindi *quale* test lo veda dipende dall'ordine della run, non dal test.
		// Asserirne la presenza faceva fallire ogni test tranne il primo — misurato: la suite completa e'
		// passata da 0 a 1 fallimento, e il fallimento diceva testualmente *«did not occur»* su un test che
		// non aveva alcun modo di farlo occorrere. Un'aspettativa che dipende dall'ordine non e'
		// un'aspettativa.
		Test.AddExpectedMessage(TEXT("Failed to find object"),
			ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ -1);
	}

	URTFrontendNavigator* MakeMainMenuNavigator(UGameInstance*& OutGI)
	{
		OutGI = NewObject<UGameInstance>(GetTransientPackage());
		if (!OutGI)
		{
			return nullptr;
		}
		OutGI->AddToRoot();
		OutGI->Init();
		return OutGI->GetSubsystem<URTFrontendNavigator>();
	}

	void ReleaseMainMenuNavigator(UGameInstance* GI)
	{
		if (GI)
		{
			GI->Shutdown();
			GI->RemoveFromRoot();
		}
	}

	/** Un binding valido verso una `UUserWidget` concreta qualsiasi: qui la classe non conta, conta l'id. */
	FRTScreenBinding MakeMainMenuBinding(FName ScreenId)
	{
		FRTScreenBinding Binding;
		Binding.ScreenId = ScreenId;
		Binding.WidgetClass = TSoftClassPtr<UUserWidget>(URTScreenHudWidgetBase::StaticClass());
		return Binding;
	}
}

// ─── La label di versione ────────────────────────────────────────────────────────────────────────

/**
 * La label di versione **porta la versione configurata**, non una stringa scritta a mano.
 *
 * ⚠️ E' l'unica proprieta' che valga la pena provare qui, e la ragione e' il modo in cui questa riga
 * marcisce: una label letterale (`"v0.1"`) resta a schermo identica dopo il bump, e nessuno se ne accorge
 * finche' qualcuno non segnala un bug su una build che crede di essere un'altra. Il test fallisce se
 * `GetVersionLabel()` smette di leggere `ProjectVersion`.
 *
 * Cosa NON prova: che la label sia *leggibile a schermo*. Quella meta' e' di `PIE-V01-FRONTEND-MAIN`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMainMenuVersionLabelCarriesProjectVersionTest,
	"RefactorTactics.Frontend.MainMenuVersionLabelCarriesProjectVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMainMenuVersionLabelCarriesProjectVersionTest::RunTest(const FString&)
{
	FString ConfiguredVersion;
	GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectVersion"), ConfiguredVersion, GGameIni);

	if (!TestFalse(TEXT("il progetto dichiara una ProjectVersion in DefaultGame.ini"),
		ConfiguredVersion.IsEmpty()))
	{
		return false;
	}

	URTMainMenuWidgetBase* Menu = NewObject<URTMainMenuWidgetBase>(GetTransientPackage());
	if (!TestNotNull(TEXT("il widget base esiste"), Menu)) { return false; }

	const FString Label = Menu->GetVersionLabel().ToString();

	TestFalse(TEXT("la label non e' vuota"), Label.IsEmpty());
	TestTrue(TEXT("la label contiene la versione configurata"), Label.Contains(ConfiguredVersion));

	return true;
}

// ─── SETTINGS, e il dead-end che il DoD vieta ────────────────────────────────────────────────────

/**
 * `SETTINGS` **dichiara di essere *coming soon***, invece di tacere.
 *
 * ⚠️ Il DoD di #938 e' esplicito su questo, e la ragione e' precisa: *«un pulsante che non fa nulla senza
 * dirlo sarebbe un dead-end»*. La voce deve esistere — il back stack la attraversa, e il menu non cambia
 * forma in v0.2 — ma deve **dire** che il contenuto non c'e'. Le due meta' non si separano: un flag senza
 * un testo lascerebbe al Blueprint il compito di inventarsi la frase, che e' esattamente cio' che
 * `RTFrontendWidgets.h` impedisce alle altre tre schermate.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMainMenuSettingsDeclaresComingSoonTest,
	"RefactorTactics.Frontend.MainMenuSettingsDeclaresComingSoon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMainMenuSettingsDeclaresComingSoonTest::RunTest(const FString&)
{
	URTMainMenuWidgetBase* Menu = NewObject<URTMainMenuWidgetBase>(GetTransientPackage());
	if (!TestNotNull(TEXT("il widget base esiste"), Menu)) { return false; }

	TestTrue(TEXT("in v0.1 SETTINGS e' coming soon"), Menu->IsSettingsComingSoon());
	TestFalse(TEXT("e lo dice con un testo, non con un silenzio"),
		Menu->GetSettingsNoticeText().IsEmpty());

	return true;
}

// ─── La registrazione delle schermate ────────────────────────────────────────────────────────────

/**
 * Ogni binding valido diventa una schermata registrata, e il conteggio restituito e' **quello vero**.
 *
 * Il valore di ritorno non e' una comodita': e' l'unico modo che il chiamante ha di accorgersi che una
 * configurazione sbagliata gli ha lasciato un frontend senza schermate. Confrontarlo con `Num()` e' cio'
 * che distingue «registrate tutte» da «registrate alcune».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNavigatorRegistersEveryValidBindingTest,
	"RefactorTactics.Frontend.NavigatorRegistersEveryValidBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNavigatorRegistersEveryValidBindingTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeMainMenuNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseMainMenuNavigator(GI); return false; }

	TArray<FRTScreenBinding> Bindings;
	Bindings.Add(MakeMainMenuBinding(RTScreenIds::Main));
	Bindings.Add(MakeMainMenuBinding(RTScreenIds::Settings));

	TestEqual(TEXT("due binding, due schermate"), Nav->RegisterScreens(Bindings), 2);

	const TArray<FName> Registered = Nav->GetRegisteredScreenIds();
	TestEqual(TEXT("e nessuna in piu'"), Registered.Num(), 2);
	TestTrue(TEXT("Main e' registrata"), Registered.Contains(RTScreenIds::Main));
	TestTrue(TEXT("Settings e' registrata"), Registered.Contains(RTScreenIds::Settings));

	ReleaseMainMenuNavigator(GI);
	return true;
}

/**
 * Un binding **incompleto viene scartato**, e non conta.
 *
 * ⚠️ I due modi di essere incompleto non sono lo stesso difetto, e il test li tiene distinti:
 *
 * - **id vuoto** → non e' indirizzabile: nessun `PushScreen` potrebbe mai nominarlo;
 * - **classe nulla** → e' indirizzabile e non disegna niente, che e' peggio. Lo stack si muoverebbe,
 *   `GetCurrentScreen()` risponderebbe, e lo schermo resterebbe quello di prima: un fallimento
 *   indistinguibile dal successo, cioe' la cosa che `ERTNavResult` esiste per impedire.
 *
 * Scartare **senza contare** e' la meta' che conta: se le voci scartate finissero nel totale, un `.ini`
 * scritto male restituirebbe «4 registrate» e nessuno cercherebbe l'errore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNavigatorSkipsIncompleteBindingsTest,
	"RefactorTactics.Frontend.NavigatorSkipsIncompleteBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNavigatorSkipsIncompleteBindingsTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeMainMenuNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseMainMenuNavigator(GI); return false; }

	FRTScreenBinding NoId = MakeMainMenuBinding(NAME_None);

	FRTScreenBinding NoClass;
	NoClass.ScreenId = RTScreenIds::Settings;
	NoClass.WidgetClass = nullptr;

	TArray<FRTScreenBinding> Bindings;
	Bindings.Add(MakeMainMenuBinding(RTScreenIds::Main));
	Bindings.Add(NoId);
	Bindings.Add(NoClass);

	TestEqual(TEXT("solo il binding completo e' registrato"), Nav->RegisterScreens(Bindings), 1);

	const TArray<FName> Registered = Nav->GetRegisteredScreenIds();
	TestEqual(TEXT("una sola schermata"), Registered.Num(), 1);
	TestTrue(TEXT("ed e' Main"), Registered.Contains(RTScreenIds::Main));
	TestFalse(TEXT("la schermata senza classe non e' entrata"),
		Registered.Contains(RTScreenIds::Settings));

	ReleaseMainMenuNavigator(GI);
	return true;
}

// ─── L'aggancio che mancava ──────────────────────────────────────────────────────────────────────

/**
 * `StartFrontend` e' **l'unica cosa che il GameMode del frontend deve sapere**: legge i binding dalla
 * configurazione e apre la radice.
 *
 * ⚠️ Esiste per non lasciare la sequenza dentro un `BeginPlay`. Registrare-poi-inizializzare **in
 * quest'ordine** non e' un dettaglio: `InitializeFrontend` presenta subito la radice, e se i binding non
 * ci fossero ancora `SyncPresentation` uscirebbe alla prima riga lasciando lo stack corretto e lo schermo
 * vuoto. Dentro un `BeginPlay` quell'ordine sarebbe una convenzione da ricordare; qui e' provato.
 *
 * 🔴 **La prima stesura di questa nota diceva che il test non carica nulla, e non era vero.** Registrare
 * davvero non carica — un `TSoftClassPtr` e' un percorso — ma `StartFrontend` **apre subito la radice**, e
 * `PresentWidget` risolve il percorso. Il log lo diceva (`Failed to find object .../WBP_RT_MainMenu_C`)
 * mentre il test usciva verde: output sporco che nessuno guardava. Trovato in code review, e adesso il
 * rumore e' dichiarato da `ExpectMissingFrontendAssets`, cosi' un rumore **diverso** fa fallire.
 *
 * ⚠️ **E nessun gate verifica che quei percorsi risolvano.** La nota rimandava a
 * `RTFrontendWidgetAssetTests.cpp`, che pero' apre solo i tre package di CP 46.2: le due schermate nuove
 * non sono coperte da niente finche' gli `.uasset` non esistono (seduta U28). Cio' che copre il caso a
 * runtime e' il warning di `PresentWidget`, che nomina schermata e percorso invece di lasciare uno
 * schermo nero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNavigatorStartFrontendOpensMainTest,
	"RefactorTactics.Frontend.NavigatorStartFrontendOpensMain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNavigatorStartFrontendOpensMainTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeMainMenuNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseMainMenuNavigator(GI); return false; }

	ExpectMissingFrontendAssets(*this);

	TestTrue(TEXT("il frontend parte"), Nav->StartFrontend());

	TestEqual(TEXT("la radice e' il Main Menu"), Nav->GetCurrentScreen(), RTScreenIds::Main);
	TestEqual(TEXT("profondita' 1"), Nav->GetDepth(), 1);
	TestFalse(TEXT("e dalla radice non si torna indietro"), Nav->CanGoBack());

	const TArray<FName> Registered = Nav->GetRegisteredScreenIds();
	TestTrue(TEXT("Main e' arrivata dalla configurazione"), Registered.Contains(RTScreenIds::Main));
	TestTrue(TEXT("e Settings anche: il back stack deve poterla attraversare"),
		Registered.Contains(RTScreenIds::Settings));

	ReleaseMainMenuNavigator(GI);
	return true;
}

/**
 * `SETTINGS` e' raggiungibile e **si torna indietro**: e' il «nessun dead-end» del DoD applicato alla voce
 * che in v0.1 non ha contenuto.
 *
 * La voce esiste proprio perche' il back stack la attraversi — se non ci si potesse tornare indietro, un
 * pannello *coming soon* sarebbe un vicolo cieco invece di una promessa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMainMenuSettingsIsReachableAndReversibleTest,
	"RefactorTactics.Frontend.MainMenuSettingsIsReachableAndReversible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMainMenuSettingsIsReachableAndReversibleTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeMainMenuNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseMainMenuNavigator(GI); return false; }

	ExpectMissingFrontendAssets(*this);

	if (!TestTrue(TEXT("premessa: il frontend e' partito"), Nav->StartFrontend()))
	{
		ReleaseMainMenuNavigator(GI);
		return false;
	}

	TestEqual(TEXT("SETTINGS si apre"), Nav->PushScreen(RTScreenIds::Settings), ERTNavResult::Ok);
	TestEqual(TEXT("ed e' la schermata corrente"), Nav->GetCurrentScreen(), RTScreenIds::Settings);
	TestTrue(TEXT("da qui si torna indietro"), Nav->CanGoBack());

	TestEqual(TEXT("il Back riporta al menu"), Nav->PopScreen(), ERTNavResult::Ok);
	TestEqual(TEXT("siamo di nuovo sul Main Menu"), Nav->GetCurrentScreen(), RTScreenIds::Main);

	ReleaseMainMenuNavigator(GI);
	return true;
}

// ─── Il GameMode del frontend ────────────────────────────────────────────────────────────────────

/**
 * La mappa del frontend **apre il menu**: e' l'ultimo anello della catena che rende vera la voce del DoD
 * *«il gioco packaged avvia sul Main Menu, non su una mappa»*.
 *
 * ⚠️ **Perche' un GameMode e non la `GameInstance`.** Un `UGameInstanceSubsystem` nasce una volta per
 * partita e sopravvive ai cambi di livello — che e' cio' che serve al back stack, e la ragione per cui il
 * navigatore e' fatto cosi'. Ma proprio per questo non puo' essere *lui* a decidere di aprirsi: nascendo su
 * ogni mappa, si aprirebbe anche sopra una partita in corso. Il GameMode invece appartiene alla **mappa**,
 * quindi il frontend si apre esattamente dove e quando quella mappa viene caricata — compreso il ritorno
 * dal match, che e' CP 46.5.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendGameModeStartsTheFrontendTest,
	"RefactorTactics.Frontend.FrontendGameModeStartsTheFrontend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendGameModeStartsTheFrontendTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	UWorld* World = MakeFrontendWorld(GI);
	if (!TestNotNull(TEXT("il mondo esiste"), World)) { DestroyFrontendWorld(World, GI); return false; }
	if (!TestNotNull(TEXT("il mondo ha una GameInstance"), GI)) { DestroyFrontendWorld(World, GI); return false; }

	ARTFrontendGameMode* GameMode = World->SpawnActor<ARTFrontendGameMode>();
	if (!TestNotNull(TEXT("il GameMode del frontend si spawna"), GameMode))
	{
		DestroyFrontendWorld(World, GI);
		return false;
	}

	ExpectMissingFrontendAssets(*this);

	TestTrue(TEXT("il frontend parte"), GameMode->StartFrontendForThisGame());

	URTFrontendNavigator* Nav = GI->GetSubsystem<URTFrontendNavigator>();
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { DestroyFrontendWorld(World, GI); return false; }

	TestEqual(TEXT("e la schermata aperta e' il Main Menu"), Nav->GetCurrentScreen(), RTScreenIds::Main);
	TestEqual(TEXT("alla radice, senza niente sotto"), Nav->GetDepth(), 1);

	DestroyFrontendWorld(World, GI);
	return true;
}

/**
 * Senza navigatore il GameMode **dice di no invece di crollare**.
 *
 * ⚠️ Non e' un caso di laboratorio: e' il modo in cui questo codice fallirebbe in un pacchetto. Il GameMode
 * raggiunge il navigatore attraversando `GetGameInstance()`, e un puntatore nullo li' produrrebbe un crash
 * **all'avvio** — cioe' il difetto piu' costoso possibile per una schermata che esiste per essere la prima
 * cosa che si vede. Il valore di ritorno esiste per questo: `false` e una riga di log, non un'eccezione.
 *
 * ⚠️ Il tipo di ritorno e' `bool` e **non** `ERTNavResult`, di proposito: qui non c'e' nessuna transizione
 * di navigazione da qualificare — non c'e' il navigatore che la produrrebbe. Riusare quell'enum
 * costringerebbe a scegliere un motivo che descrive un'altra cosa (`InvalidScreen` parla di un nome vuoto),
 * e un motivo sbagliato e' peggio di nessun motivo. La causa va nel log, che e' il canale giusto per un
 * guasto d'ambiente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendGameModeWithoutNavigatorSaysNoTest,
	"RefactorTactics.Frontend.FrontendGameModeWithoutNavigatorSaysNo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendGameModeWithoutNavigatorSaysNoTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	UWorld* World = MakeFrontendWorld(GI, /*bWithGameInstance=*/ false);
	if (!TestNotNull(TEXT("il mondo esiste"), World)) { DestroyFrontendWorld(World, GI); return false; }
	TestNull(TEXT("e non ha GameInstance: e' il caso in prova"), World->GetGameInstance());

	ARTFrontendGameMode* GameMode = World->SpawnActor<ARTFrontendGameMode>();
	if (!TestNotNull(TEXT("il GameMode si spawna lo stesso"), GameMode))
	{
		DestroyFrontendWorld(World, GI);
		return false;
	}

	AddExpectedError(TEXT("Frontend non avviato"), EAutomationExpectedErrorFlags::Contains, 1);

	TestFalse(TEXT("il frontend non parte, e lo dichiara"), GameMode->StartFrontendForThisGame());

	DestroyFrontendWorld(World, GI);
	return true;
}

/**
 * Il GameMode del frontend **non porta il pawn volante di `AGameModeBase`**.
 *
 * 🔴 Trovato in code review, ed era il difetto che rendeva il DoD irraggiungibile. `ARTFrontendGameMode`
 * non aveva costruttore, quindi ereditava i default di `AGameModeBase` — `DefaultPawnClass =
 * ADefaultPawn::StaticClass()` (`GameModeBase.cpp:71`). Su una mappa di menu significa: un pawn volante
 * possiede il giocatore, il mouse-look cattura il cursore, e `bShowMouseCursor` resta **falso** perche'
 * nessuno lo accende — `ARTPlayerController` lo fa, ma quello e' il controller della **partita**.
 *
 * Il DoD di #938 chiede `PLAY · SETTINGS · QUIT` *«navigabili da mouse e tastiera»*: con un cursore che
 * non si disegna, i tre pulsanti non sono cliccabili. Nessun test lo copriva, e il difetto sarebbe
 * comparso solo davanti a chi apriva la mappa in editor.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendGameModeHasNoFlyingPawnTest,
	"RefactorTactics.Frontend.FrontendGameModeHasNoFlyingPawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendGameModeHasNoFlyingPawnTest::RunTest(const FString&)
{
	const ARTFrontendGameMode* Defaults = GetDefault<ARTFrontendGameMode>();
	if (!TestNotNull(TEXT("il CDO esiste"), Defaults)) { return false; }

	TestNotEqual(TEXT("il menu non eredita il pawn volante di AGameModeBase"),
		Defaults->DefaultPawnClass, TSubclassOf<APawn>(ADefaultPawn::StaticClass()));
	TestNull(TEXT("e su una mappa di menu non possiede nessun pawn"),
		Defaults->DefaultPawnClass.Get());

	return true;
}

/**
 * Il frontend **accende il cursore**, altrimenti i pulsanti non si cliccano.
 *
 * ⚠️ Sta in una funzione pubblica e non dentro `PostLogin` per la stessa ragione di
 * `StartFrontendForThisGame`: dentro l'hook sarebbe verificabile solo in PIE. Qui il test spawna un
 * `APlayerController` vero e chiede alla funzione cio' che il DoD chiede alla mappa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendGameModeShowsTheCursorTest,
	"RefactorTactics.Frontend.FrontendGameModeShowsTheCursor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendGameModeShowsTheCursorTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	UWorld* World = MakeFrontendWorld(GI);
	if (!TestNotNull(TEXT("il mondo esiste"), World)) { DestroyFrontendWorld(World, GI); return false; }

	APlayerController* PC = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("il controller esiste"), PC)) { DestroyFrontendWorld(World, GI); return false; }

	TestFalse(TEXT("premessa: il cursore parte spento"), PC->bShowMouseCursor);

	ARTFrontendGameMode::ConfigureMenuInput(PC);

	TestTrue(TEXT("il frontend lo accende"), PC->bShowMouseCursor);

	DestroyFrontendWorld(World, GI);
	return true;
}

/**
 * Senza schermate registrate il frontend **dichiara di non essere partito**.
 *
 * 🔴 Trovato in code review. `StartFrontend` calcolava il conteggio, ne faceva un warning, e poi
 * restituiva `InitializeFrontend(Main)` — cioe' `Ok`. `StartFrontendForThisGame` rispondeva `true` sopra
 * uno schermo nero: il fallimento indistinguibile dal successo che questo file argomenta contro in tre
 * punti diversi. Il conteggio che l'header chiama *«l'unico segnale»* veniva prodotto e buttato.
 *
 * ⚠️ **Lo stack si inizializza lo stesso**, e non e' una svista: senza binding la navigazione resta
 * legale — e' cio' che `NavigationWorksWithoutWidgetBindings` verifica dal CP 46.1. A cambiare e' solo
 * cosa *si risponde a chi ha chiesto di avviare*: `false`, perche' il menu non si e' aperto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStartFrontendWithoutScreensSaysNoTest,
	"RefactorTactics.Frontend.StartFrontendWithoutScreensSaysNo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStartFrontendWithoutScreensSaysNoTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeMainMenuNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseMainMenuNavigator(GI); return false; }

	AddExpectedMessage(TEXT("senza schermate registrate"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 1);

	TestEqual(TEXT("zero binding registrati"), Nav->RegisterScreens(TArray<FRTScreenBinding>()), 0);
	TestFalse(TEXT("e allora il frontend non e' partito"), Nav->StartFrontendFrom(TArray<FRTScreenBinding>()));

	ReleaseMainMenuNavigator(GI);
	return true;
}

/**
 * Due `+Screens=` con lo **stesso id** non contano due volte.
 *
 * 🔴 Trovato in code review, e in modo indipendente rileggendo il diff. `RegisterScreen` fa
 * `Bindings.Add`, che **sovrascrive**: due righe con `ScreenId="Main"` producono un binding solo, mentre
 * il conteggio ne dichiarava due. Il valore di ritorno esiste per distinguere «registrate tutte» da
 * «registrate alcune», e in questo caso diceva la prima mentendo.
 *
 * ⚠️ Il duplicato **non e' un errore fatale** — l'ultimo vince, che e' la regola dei `.ini` a strati e
 * va lasciata funzionare. Cio' che non deve fare e' passare inosservato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNavigatorDoesNotCountDuplicateScreenIdsTwiceTest,
	"RefactorTactics.Frontend.NavigatorDoesNotCountDuplicateScreenIdsTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNavigatorDoesNotCountDuplicateScreenIdsTwiceTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeMainMenuNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseMainMenuNavigator(GI); return false; }

	AddExpectedMessage(TEXT("dichiarata due volte"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ 1);

	TArray<FRTScreenBinding> Bindings;
	Bindings.Add(MakeMainMenuBinding(RTScreenIds::Main));
	Bindings.Add(MakeMainMenuBinding(RTScreenIds::Main));
	Bindings.Add(MakeMainMenuBinding(RTScreenIds::Settings));

	TestEqual(TEXT("tre righe, due schermate"), Nav->RegisterScreens(Bindings), 2);
	TestEqual(TEXT("e il conteggio combacia con i binding veri"),
		Nav->GetRegisteredScreenIds().Num(), 2);

	ReleaseMainMenuNavigator(GI);
	return true;
}

/**
 * La riga *coming soon* ha una **visibilita' collegabile**, non solo un `bool`.
 *
 * 🔴 Trovato in code review, ed e' la quarta volta che questo file incontra lo stesso ostacolo:
 * `RTFrontendWidgets.h` lo spiega tre volte — un `bool` non compare nel menu dei binding di `Visibility`,
 * che vuole un `ESlateVisibility`, quindi chi cerca **non lo trova**. Il commento di `GetLoadingVisibility`
 * dice testualmente *«averne risolta una sola avrebbe lasciato le altre due a far perdere tempo nello
 * stesso identico punto»*, e la classe nuova era stata aggiunta senza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMainMenuNoticeVisibilityFollowsTheFlagTest,
	"RefactorTactics.Frontend.MainMenuNoticeVisibilityFollowsTheFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMainMenuNoticeVisibilityFollowsTheFlagTest::RunTest(const FString&)
{
	URTMainMenuWidgetBase* Menu = NewObject<URTMainMenuWidgetBase>(GetTransientPackage());
	if (!TestNotNull(TEXT("il widget base esiste"), Menu)) { return false; }

	// `Collapsed` e non `Hidden`, come il banner: una riga assente non deve occupare spazio nel layout.
	TestEqual(TEXT("coming soon: la riga si vede"),
		Menu->GetSettingsNoticeVisibility(),
		Menu->IsSettingsComingSoon() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	TestEqual(TEXT("e oggi e' Visible, perche' in v0.1 il flag e' acceso"),
		Menu->GetSettingsNoticeVisibility(), ESlateVisibility::Visible);

	return true;
}

/**
 * **Un nuovo avvio del frontend non riusa i widget del precedente.**
 *
 * 🔴 Trovato in code review su PR #1264, e **non corretto in quel giro**: il difetto e' rimasto scritto in
 * un commento di PR e in nessuna issue, che e' il modo in cui un modo di guasto noto viene riscoperto da
 * zero sei mesi dopo.
 *
 * Il navigatore e' un `UGameInstanceSubsystem` **apposta**, per sopravvivere al cambio di livello: e' cio'
 * che permette a `ReturnMain` di funzionare dopo una partita. Ma la stessa proprieta' fa sopravvivere
 * `LiveWidgets`, e i widget dentro appartengono al **mondo in cui sono stati costruiti**. Al secondo
 * ingresso nel frontend — `Main Menu -> partita -> Main Menu`, cioe' il ciclo di CP 46.5 —
 * `PresentWidget` trovava l'istanza vecchia, saltava `CreateWidget` e chiamava `AddToViewport` su un
 * widget il cui mondo era stato smontato.
 *
 * ⚠️ **Non contraddice la cache dichiarata da CP 46.1.** Quel commento dice che i widget *«restano
 * istanziati dopo un `Pop`»*, ed e' vero e resta vero: la cache serve fra `PushScreen` e `PopScreen`, cioe'
 * **dentro** una sessione di frontend. `InitializeFrontend` non e' una navigazione — e' l'inizio di una
 * sessione nuova, e `LiveWidgets` era l'unico stato che non veniva azzerato insieme allo stack.
 *
 * ⚠️ **Serve un mondo vero**: senza, `CreateWidget` non costruisce e il test non avrebbe soggetto. Se in
 * questo ambiente non costruisce, il test lo dichiara invece di asserire su due `nullptr` — che sarebbero
 * «uguali» e lo farebbero passare per la ragione sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendRestartDoesNotReuseStaleWidgetsTest,
	"RefactorTactics.Frontend.RestartDoesNotReuseStaleWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendRestartDoesNotReuseStaleWidgetsTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	UWorld* World = MakeFrontendWorld(GI);
	if (!TestNotNull(TEXT("il mondo esiste"), World)) { DestroyFrontendWorld(World, GI); return false; }
	if (!TestNotNull(TEXT("il mondo ha una GameInstance"), GI)) { DestroyFrontendWorld(World, GI); return false; }

	URTFrontendNavigator* Nav = GI->GetSubsystem<URTFrontendNavigator>();
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { DestroyFrontendWorld(World, GI); return false; }

	Nav->RegisterScreen(RTScreenIds::Main,
		TSoftClassPtr<UUserWidget>(URTScreenHudWidgetBase::StaticClass()));

	TestEqual(TEXT("primo avvio"), Nav->InitializeFrontend(RTScreenIds::Main), ERTNavResult::Ok);

	UUserWidget* First = Nav->FindLiveWidget(RTScreenIds::Main);
	if (!First)
	{
		AddInfo(TEXT("CreateWidget non ha costruito in questo ambiente: riuso non verificabile qui"));
		DestroyFrontendWorld(World, GI);
		return true;
	}

	TestEqual(TEXT("secondo avvio"), Nav->InitializeFrontend(RTScreenIds::Main), ERTNavResult::Ok);

	UUserWidget* Second = Nav->FindLiveWidget(RTScreenIds::Main);
	if (!TestNotNull(TEXT("il secondo avvio produce un widget"), Second))
	{
		DestroyFrontendWorld(World, GI);
		return false;
	}

	TestNotEqual(TEXT("e non e' l'istanza del primo avvio"), Second, First);

	DestroyFrontendWorld(World, GI);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
