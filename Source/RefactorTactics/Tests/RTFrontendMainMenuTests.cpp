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

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Un mondo minimo con una `GameInstance` attaccata.
	 *
	 * ⚠️ **La `GameInstance` non e' un dettaglio di allestimento: e' il soggetto del test.** Il navigatore e'
	 * un `UGameInstanceSubsystem`, quindi il GameMode lo raggiunge *attraverso* la `GameInstance` del mondo —
	 * e un mondo senza `GameInstance` e' esattamente il caso in cui quel percorso si rompe.
	 */
	UWorld* MakeFrontendWorld(UGameInstance*& OutGI, bool bWithGameInstance = true)
	{
		OutGI = nullptr;

		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (!World || !GEngine)
		{
			return World;
		}

		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(World);

		if (bWithGameInstance)
		{
			OutGI = NewObject<UGameInstance>(GetTransientPackage());
			if (OutGI)
			{
				OutGI->AddToRoot();
				OutGI->Init();
				World->SetGameInstance(OutGI);
			}
		}

		return World;
	}

	void DestroyFrontendWorld(UWorld* World, UGameInstance* GI)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
		if (GI)
		{
			GI->Shutdown();
			GI->RemoveFromRoot();
		}
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
 * ⚠️ Cosa NON prova, ed e' dichiarato: che i `WBP_RT_*` nominati nel `.ini` **esistano**. Un
 * `TSoftClassPtr` e' un percorso, e registrarlo non lo carica — di proposito, perche' i binari sono
 * lavoro d'editor e questo test deve poter girare prima che esistano. Che il percorso risolva e' di
 * `RTFrontendWidgetAssetTests.cpp`, che apre i package davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNavigatorStartFrontendOpensMainTest,
	"RefactorTactics.Frontend.NavigatorStartFrontendOpensMain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNavigatorStartFrontendOpensMainTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeMainMenuNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseMainMenuNavigator(GI); return false; }

	TestEqual(TEXT("il frontend parte"), Nav->StartFrontend(), ERTNavResult::Ok);

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

	Nav->StartFrontend();

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

#endif // WITH_DEV_AUTOMATION_TESTS
