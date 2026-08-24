// CP 46.6 (#941) — la pausa, e lo smontaggio che non lascia stato vivo.
//
// ⚠️ **Qui non si prova un menu.** Il layout di `WBP_RT_PauseMenu` e la leggibilita' dei tre pulsanti sono
// della seduta d'editor `U30` e di `PIE-V01-FRONTEND-PAUSE`. Cio' che si prova senza editor e' tutto il
// resto, ed e' la parte che il DoD chiama *«criterio verificabile invece che dichiarato»*: chi possiede la
// transizione, che cosa sopravvive a un cambio di livello, e se una partita avviata dopo il ritorno al menu
// e' indistinguibile da una avviata da fresco.
//
// 🔴 **Due difetti che questo file misura, e che esistevano su `main` prima di `#941`:**
//
//   1. nessun percorso del codebase riapriva il livello del frontend. `ReturnMain()` muove lo stack, e il
//      menu si disegnava **sopra una partita ancora viva** — lo stato che CP 46.2 dichiara «vietato da
//      CP 46.6». Valeva anche per il `MAIN MENU` di CP 46.5, gia' chiuso: il suo DoD era vero dello stack
//      e falso del mondo;
//   2. `PLAY AGAIN` dal Result annunciava a **zero ascoltatori**. `OnMatchRequested` lo raccoglie
//      `ARTFrontendGameMode`, che vive sulla mappa del menu; il Result invece si apre dentro il livello di
//      partita. La richiesta restava pendente e il `PLAY` successivo veniva rifiutato da
//      `MatchRequestNotConsumed` — la guardia funzionava, era il consumatore a mancare.

#include "Misc/AutomationTest.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "Frontend/RTStartupReport.h"
// 🔴 Il binding di `Error` deve essere una `URTErrorModalWidgetBase`, non una `UUserWidget` qualsiasi:
// `ArmErrorModal` fa un `Cast` e, se fallisce, lascia il menu disabilitato sotto un modale invisibile —
// il soft-lock che `RTFrontendNavigator.cpp` dichiara. Misurato: due test rossi alla prima run.
#include "Frontend/RTFrontendWidgets.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
// Una `UUserWidget` concreta da registrare: `UUserWidget` e' `Abstract` e `CreateWidget` la rifiuta.
#include "UI/RTScreenHudWidgets.h"
#include "RTGameMode.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/RTGameModeLevelSpyForTest.h"
#include "Tests/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTPauseTestsLocal
{
	/** Il livello di menu che i test dichiarano: una stringa qualsiasi, purche' la stessa ovunque. */
	static const TCHAR* FrontendLevelName = TEXT("/Game/Test/L_TestFrontend");
	static const TCHAR* MatchLevelName = TEXT("/Game/Test/L_TestMatch");

	URTFrontendNavigator* MakeNavigator(UGameInstance*& OutGI)
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

	void ReleaseNavigator(UGameInstance* GI)
	{
		if (GI)
		{
			GI->Shutdown();
			GI->RemoveFromRoot();
		}
	}

	FRTScreenBinding MakeBinding(FName ScreenId)
	{
		FRTScreenBinding Binding;
		Binding.ScreenId = ScreenId;
		Binding.WidgetClass = TSoftClassPtr<UUserWidget>(URTScreenHudWidgetBase::StaticClass());
		return Binding;
	}

	/**
	 * Un navigatore avviato con le quattro schermate che CP 46.6 attraversa, e i due livelli dichiarati.
	 *
	 * ⚠️ **Non legge il `.ini`**: `StartFrontendFrom` esiste apposta, e un test che dipendesse dalla
	 * configurazione misurerebbe il `.ini` invece del proprio soggetto.
	 */
	URTFrontendNavigator* MakeStartedNavigator(UGameInstance*& OutGI, bool bWithFrontendLevel = true)
	{
		URTFrontendNavigator* Nav = MakeNavigator(OutGI);
		if (!Nav)
		{
			return nullptr;
		}

		TArray<FRTScreenBinding> Screens;
		Screens.Add(MakeBinding(RTScreenIds::Main));
		Screens.Add(MakeBinding(RTScreenIds::Settings));
		Screens.Add(MakeBinding(RTScreenIds::Pause));

		// ⚠️ **`Error` ha una classe SUA, e non e' pedanteria di allestimento.** Il modale non viene solo
		// presentato: viene **armato** con l'esito, e `ArmErrorModal` ci arriva con un `Cast`. Con una
		// `UUserWidget` qualsiasi il cast fallisce, il navigatore logga «il modale d'errore non e' armabile»
		// — un `Error`, che in automation fa cadere il test — e a schermo resterebbe un modale invisibile
		// sopra una schermata disabilitata. E' il soft-lock che #939 aveva gia' trovato in code review, e
		// alla prima run di questo file l'ho riprodotto nei test invece che nel codice.
		FRTScreenBinding ErrorBinding;
		ErrorBinding.ScreenId = RTScreenIds::ErrorModal;
		ErrorBinding.WidgetClass = TSoftClassPtr<UUserWidget>(URTErrorModalWidgetBase::StaticClass());
		Screens.Add(ErrorBinding);
		Nav->StartFrontendFrom(Screens);

		Nav->MatchLevel = MatchLevelName;
		Nav->FrontendLevel = bWithFrontendLevel ? FrontendLevelName : FString();
		return Nav;
	}
}

// ─── La pausa e' una schermata, e il DoD dipende da questo ───────────────────────────────────────────

/**
 * `SETTINGS` dalla pausa apre **lo stesso pannello** del Main Menu, e la pausa resta sotto.
 *
 * 🔴 **E' il criterio che decide schermata-vs-modale, e non era dichiarato da nessuna parte.** Con un
 * modale aperto `PushScreen` risponde `BlockedByModal`: se la pausa fosse un modale, *«`SETTINGS` apre lo
 * stesso pannello di CP 46.3 — non una seconda copia»* sarebbe **irrealizzabile**, e l'unica via sarebbe
 * un secondo widget, cioe' la copia che il DoD vieta.
 *
 * ✅ La risposta era gia' scritta a CP 46.1, in `RTScreenStack.h`: *«`Settings` aperto dal Main e
 * `Settings` aperto dalla Pause sono la stessa schermata con due ritorni diversi»*. Questo test la rende
 * una proprieta' invece di una previsione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPauseSettingsIsTheSamePanelTest,
	"RefactorTactics.Frontend.PauseSettingsIsTheSamePanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPauseSettingsIsTheSamePanelTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	TestEqual(TEXT("ESC apre la pausa"), Nav->ShowPause(), ERTNavResult::Ok);
	TestEqual(TEXT("ed e' la schermata corrente"), Nav->GetCurrentScreen(), RTScreenIds::Pause);

	// 🔴 L'asserzione che vale il test: se la pausa fosse un modale, questo sarebbe `BlockedByModal`.
	TestEqual(TEXT("SETTINGS dalla pausa si apre"), Nav->OpenSettings(), ERTNavResult::Ok);
	TestEqual(TEXT("ed e' lo STESSO pannello del Main Menu, non una seconda copia"),
		Nav->GetCurrentScreen(), RTScreenIds::Settings);

	// La pausa e' ancora nello stack, sotto: e' il «ritorno diverso» di cui parla `RTScreenStack.h`.
	TestTrue(TEXT("la pausa e' ancora aperta sotto le impostazioni"), Nav->IsPauseOpen());
	TestEqual(TEXT("e il Back torna alla pausa, non al menu"), Nav->PopScreen(), ERTNavResult::Ok);
	TestEqual(TEXT("infatti eccola"), Nav->GetCurrentScreen(), RTScreenIds::Pause);

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * `RESUME` riporta la partita **da qualunque profondita'**, impostazioni comprese.
 *
 * ⚠️ **Il caso che morde e' `RESUME` premuto da dentro `SETTINGS`.** Un `PopScreen` solo tornerebbe alla
 * pausa — un `Back` travestito da ripresa — e il giocatore si troverebbe ancora coperto credendo di aver
 * ripreso. Il DoD dice *«restituisce l'input alla partita»*, non «risale di uno».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResumeReturnsTheMatchFromAnyDepthTest,
	"RefactorTactics.Frontend.ResumeReturnsTheMatchFromAnyDepth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResumeReturnsTheMatchFromAnyDepthTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	const int32 DepthInMatch = Nav->GetDepth();

	TestEqual(TEXT("ESC"), Nav->ShowPause(), ERTNavResult::Ok);
	TestEqual(TEXT("SETTINGS"), Nav->OpenSettings(), ERTNavResult::Ok);
	TestTrue(TEXT("la pausa copre ancora la partita"), Nav->IsPauseOpen());

	TestEqual(TEXT("RESUME da dentro le impostazioni"), Nav->ResumeMatch(), ERTNavResult::Ok);
	TestFalse(TEXT("la pausa e' uscita dallo stack, e con lei le impostazioni"), Nav->IsPauseOpen());
	TestEqual(TEXT("lo stack e' esattamente com'era prima di ESC"), Nav->GetDepth(), DepthInMatch);

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * Due rifiuti che **non sono `Ok`**, e nessuno dei due e' silenzioso.
 *
 * ⚠️ `RESUME` senza pausa e `ESC` con la pausa gia' aperta sono due modi di non fare niente, e un `Ok`
 * li renderebbe indistinguibili dall'aver fatto qualcosa — la regola di `RTScreenStack.h`: *«chi chiama
 * deve poter distinguere “non si puo' adesso” da “non e' successo niente”»*.
 *
 * ⛔ Il secondo non e' pedanteria: due `Pause` impilate sarebbero **due voci di stack e un widget solo**,
 * e il `RESUME` ne toglierebbe una lasciando l'altra a coprire la partita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPauseRefusalsCarryTheirReasonTest,
	"RefactorTactics.Frontend.PauseRefusalsCarryTheirReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPauseRefusalsCarryTheirReasonTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	TestEqual(TEXT("RESUME senza pausa aperta"), Nav->ResumeMatch(), ERTNavResult::NoPauseOpen);

	TestEqual(TEXT("ESC apre"), Nav->ShowPause(), ERTNavResult::Ok);
	TestEqual(TEXT("un secondo ESC non impila una seconda pausa"),
		Nav->ShowPause(), ERTNavResult::ScreenIsAlreadyOnStack);
	TestEqual(TEXT("e la profondita' non e' cambiata"), Nav->GetCurrentScreen(), RTScreenIds::Pause);

	TestEqual(TEXT("RESUME chiude"), Nav->ResumeMatch(), ERTNavResult::Ok);
	TestEqual(TEXT("e un secondo RESUME non mangia la schermata sotto"),
		Nav->ResumeMatch(), ERTNavResult::NoPauseOpen);

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

// ─── Il ritorno al menu: una richiesta, e chi la raccoglie ───────────────────────────────────────────

/**
 * `RETURN TO MAIN MENU` **chiede il livello del frontend**, e lascia lo stack alla radice.
 *
 * 🔴 **E' la meta' che mancava.** Prima di `#941` la sola cosa disponibile era `ReturnMain()`, che muove lo
 * stack: dopo di esso il menu si disegna sopra una partita ancora viva, e nessun `OpenLevel` la smonta.
 * Misurato su `327208c4`: `git grep -n "OpenLevel" Source/` produceva l'apertura della *partita* e un
 * restart della stessa mappa — niente che riportasse al menu.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReturnToMainMenuRequestsTheFrontendLevelTest,
	"RefactorTactics.Frontend.ReturnToMainMenuRequestsTheFrontendLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReturnToMainMenuRequestsTheFrontendLevelTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	Nav->ShowPause();
	Nav->OpenSettings();

	TestEqual(TEXT("RETURN TO MAIN MENU"), Nav->RequestReturnToMainMenu(), ERTNavResult::Ok);

	TestEqual(TEXT("lo stack e' tornato alla radice"), Nav->GetCurrentScreen(), RTScreenIds::Main);
	TestFalse(TEXT("la pausa non c'e' piu'"), Nav->IsPauseOpen());
	TestEqual(TEXT("e non si puo' tornare indietro in una partita lasciata"), Nav->GetDepth(), 1);

	// 🔴 L'asserzione che distingue questo checkpoint da `ReturnMain()`: la richiesta di smontaggio esiste.
	TestEqual(TEXT("il livello del menu e' stato CHIESTO"),
		Nav->ConsumePendingFrontendLevel(), FString(RTPauseTestsLocal::FrontendLevelName));
	TestEqual(TEXT("e consumarla la azzera: due letture non aprono due livelli"),
		Nav->ConsumePendingFrontendLevel(), FString());

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * Senza `FrontendLevel` configurato il ritorno **fallisce rumorosamente**, non in silenzio.
 *
 * ⛔ Un pulsante che non fa niente e' il soft-lock che CP 46.1 chiama dead-end, con l'aggravante che il
 * giocatore crede di aver lasciato la partita. Stessa forma e stessa ragione di `MatchLevelUnset`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReturnToMainMenuWithoutLevelIsRefusedTest,
	"RefactorTactics.Frontend.ReturnToMainMenuWithoutLevelIsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReturnToMainMenuWithoutLevelIsRefusedTest::RunTest(const FString&)
{
	AddExpectedError(TEXT("Ritorno al menu rifiutato"), EAutomationExpectedErrorFlags::Contains, 1);

	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI, /*bWithFrontendLevel=*/ false);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	Nav->ShowPause();
	const ERTNavResult Result = Nav->RequestReturnToMainMenu();

	TestNotEqual(TEXT("il ritorno non riesce"), Result, ERTNavResult::Ok);
	TestTrue(TEXT("e la causa e' a schermo: il modale d'errore e' aperto"), Nav->IsModalOpen());

	// 🔴 **Aperto non basta: dev'essere ARMATO.** `GetModalVisibility` resta `Collapsed` finche' `IsArmed()`
	// e' falso, quindi un modale presentato e non armato e' invisibile — e la schermata sotto resta
	// disabilitata. Un soft-lock indistinguibile da «non e' successo niente», che e' proprio cio' che questo
	// rifiuto esiste per impedire.
	URTErrorModalWidgetBase* Modal = Cast<URTErrorModalWidgetBase>(Nav->FindLiveWidget(RTScreenIds::ErrorModal));
	if (TestNotNull(TEXT("il modale d'errore e' stato presentato"), Modal))
	{
		TestTrue(TEXT("ed e' ARMATO, quindi visibile"), Modal->IsArmed());
		TestEqual(TEXT("con la causa giusta, che manda a controllare il file giusto"),
			Modal->GetOutcome(), ERTStartupOutcome::FrontendLevelUnset);
	}

	TestEqual(TEXT("nessun livello e' stato chiesto"), Nav->ConsumePendingFrontendLevel(), FString());

	// ⚠️ La pausa e' rimasta: il rifiuto non ha mosso lo stack sotto il modale, quindi chiudendo il modale
	// il giocatore torna dov'era invece che in un posto nuovo.
	TestTrue(TEXT("la pausa e' ancora li' sotto"), Nav->IsPauseOpen());

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * Una richiesta **mai raccolta** rifiuta la successiva, e nomina la causa giusta.
 *
 * ⚠️ Gemella della guardia di `StartMatch`, e per lo stesso difetto: il consumatore vive in un altro file e
 * puo' non esserci. Senza questa guardia, un aggancio mancante si manifesterebbe come «il pulsante a volte
 * non funziona» invece che come un errore che nomina cosa collegare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReturnToMainMenuTwiceWithoutConsumerIsRefusedTest,
	"RefactorTactics.Frontend.ReturnToMainMenuTwiceWithoutConsumerIsRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReturnToMainMenuTwiceWithoutConsumerIsRefusedTest::RunTest(const FString&)
{
	AddExpectedError(TEXT("Ritorno al menu rifiutato"), EAutomationExpectedErrorFlags::Contains, 1);
	// ⚠️ **Qui NON si dichiara la rete di `Deinitialize`, e la prima stesura lo faceva.** Questo test
	// consuma entrambe le richieste prima di uscire, quindi quel messaggio non viene emesso — e
	// `Occurrences = 0` **non** significa «ignora» ma «esigo che compaia»: il test cadeva su
	// «did not occur», cioe' su un'aspettativa che descriveva uno scenario diverso dal proprio.
	// (Per ignorare davvero serve `-1`, come documenta `RTFrontendMainMenuTests.cpp`.)

	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	TestEqual(TEXT("la prima richiesta passa"), Nav->RequestReturnToMainMenu(), ERTNavResult::Ok);
	TestNotEqual(TEXT("la seconda no, perche' nessuno ha raccolto la prima"),
		Nav->RequestReturnToMainMenu(), ERTNavResult::Ok);

	// La causa nomina l'aggancio mancante, non il file di configurazione: sono due correzioni diverse.
	URTErrorModalWidgetBase* Modal = Cast<URTErrorModalWidgetBase>(Nav->FindLiveWidget(RTScreenIds::ErrorModal));
	if (TestNotNull(TEXT("il modale d'errore e' stato presentato"), Modal))
	{
		TestEqual(TEXT("e dice che manca il CONSUMATORE, non il livello"),
			Modal->GetOutcome(), ERTStartupOutcome::FrontendReturnNotConsumed);
	}

	// Consumata la prima, il percorso torna disponibile: la guardia protegge, non blocca.
	TestEqual(TEXT("la richiesta pendente era quella"), Nav->ConsumePendingFrontendLevel(),
		FString(RTPauseTestsLocal::FrontendLevelName));
	Nav->CloseModal();
	TestEqual(TEXT("e adesso si puo' chiedere di nuovo"), Nav->RequestReturnToMainMenu(), ERTNavResult::Ok);
	Nav->ConsumePendingFrontendLevel();

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

// ─── Il consumatore, e l'aggancio che non si chiama a mano ───────────────────────────────────────────

/**
 * `ARTGameMode` raccoglie **entrambe** le richieste e apre il livello — e nessuno lo ha iscritto a mano.
 *
 * 🔴 **L'iscrizione passa da `BeginPlay`, ed e' il punto del test.** `#939` insegna che otto test verdi non
 * videro il consumatore scollegato perche' lo collegavano tutti da se': qui si chiama `DispatchBeginPlay()`
 * e si guarda cosa succede, senza mai nominare `ListenForLevelRequests`.
 *
 * ⚠️ **`PLAY AGAIN` e' provato insieme al ritorno**, perche' e' lo stesso buco: il Result si apre **dentro**
 * il livello di partita, dove `ARTFrontendGameMode` non esiste. Prima di `#941` quell'annuncio non aveva
 * ascoltatori e la partita nuova non partiva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchGameModeConsumesBothLevelRequestsTest,
	"RefactorTactics.Frontend.MatchGameModeConsumesBothLevelRequests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchGameModeConsumesBothLevelRequestsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		RTPauseTestsLocal::ReleaseNavigator(GI); RTWorldFixtures::DestroyWorld(World); return false;
	}
	World->SetGameInstance(GI);

	// ⚠️ Senza, `AActor::ProcessEvent` scarta ogni evento dinamico: e' la lezione di `#939`, e i due
	// `AddUniqueDynamic` di `ListenForLevelRequests` sono esattamente quel tipo di delegate.
	World->InitializeActorsForPlay(FURL());

	ARTGameModeLevelSpyForTest* GameMode = World->SpawnActor<ARTGameModeLevelSpyForTest>();
	if (!TestNotNull(TEXT("GameMode di partita"), GameMode))
	{
		RTPauseTestsLocal::ReleaseNavigator(GI); RTWorldFixtures::DestroyWorld(World); return false;
	}

	// 🔴 Il ciclo di vita, non l'iscrizione a mano.
	GameMode->DispatchBeginPlay();

	TestEqual(TEXT("RETURN TO MAIN MENU"), Nav->RequestReturnToMainMenu(), ERTNavResult::Ok);
	if (TestEqual(TEXT("un livello e' stato aperto"), GameMode->OpenedLevels.Num(), 1))
	{
		TestEqual(TEXT("ed e' quello del menu"), GameMode->OpenedLevels[0],
			FString(RTPauseTestsLocal::FrontendLevelName));
	}
	TestEqual(TEXT("la richiesta e' stata CONSUMATA, non solo sentita"),
		Nav->ConsumePendingFrontendLevel(), FString());

	// `PLAY AGAIN`: stesso confine, verso opposto.
	TestEqual(TEXT("PLAY AGAIN"), Nav->PlayAgain(), ERTNavResult::Ok);
	if (TestEqual(TEXT("un secondo livello e' stato aperto"), GameMode->OpenedLevels.Num(), 2))
	{
		TestEqual(TEXT("ed e' quello di partita"), GameMode->OpenedLevels[1],
			FString(RTPauseTestsLocal::MatchLevelName));
	}
	TestEqual(TEXT("anche questa e' stata consumata"), Nav->ConsumePendingMatchLevel(), FString());

	RTPauseTestsLocal::ReleaseNavigator(GI);
	RTWorldFixtures::DestroyWorld(World);
	return true;
}

// ─── Cio' che sopravvive al cambio di mondo ──────────────────────────────────────────────────────────

/**
 * I widget del frontend **muoiono col mondo che li ha costruiti**.
 *
 * 🔴 **Chiude un difetto che `RTFrontendNavigator.h` dichiarava e che nessuno aveva ancora incontrato**:
 * *«`ReturnMain`, `PushScreen` e `ShowModal` raggiungono `PresentWidget` senza passare da
 * `InitializeFrontend`. Se fra la costruzione di un widget e una di quelle chiamate il mondo e' cambiato,
 * la cache restituisce ancora l'istanza vecchia»*. Finche' il mondo non cambiava mai era un'ipotesi; con
 * `RETURN TO MAIN MENU` cambia a ogni ritorno, e diventa il percorso normale.
 *
 * ⚠️ **Il filtro sulla `GameInstance` e' parte della proprieta', non un dettaglio**: `OnWorldCleanup` e'
 * globale, e senza filtro chiudere un mondo qualsiasi svuoterebbe il menu di questa sessione. Qui si
 * verificano tutti e due i versi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWidgetCacheDiesWithItsWorldTest,
	"RefactorTactics.Frontend.WidgetCacheDiesWithItsWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWidgetCacheDiesWithItsWorldTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	UWorld* MatchWorld = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di partita"), MatchWorld))
	{
		RTPauseTestsLocal::ReleaseNavigator(GI); return false;
	}
	MatchWorld->SetGameInstance(GI);

	Nav->ShowPause();
	TestNotNull(TEXT("la pausa ha costruito il suo widget"), Nav->FindLiveWidget(RTScreenIds::Pause));

	// Un mondo ESTRANEO che se ne va non deve toccare niente: e' il verso che il filtro protegge.
	UWorld* ForeignWorld = RTWorldFixtures::MakeWorld();
	RTWorldFixtures::DestroyWorld(ForeignWorld);
	TestNotNull(TEXT("un mondo altrui che muore non svuota la cache"),
		Nav->FindLiveWidget(RTScreenIds::Pause));

	// Il mondo di QUESTA sessione, invece, se li porta via.
	RTWorldFixtures::DestroyWorld(MatchWorld);
	TestNull(TEXT("il widget non sopravvive al mondo che lo ha costruito"),
		Nav->FindLiveWidget(RTScreenIds::Pause));

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

// ─── Il criterio forte del DoD ───────────────────────────────────────────────────────────────────────

namespace RTPauseTestsLocal
{
	/**
	 * Gioca una partita in un mondo NUOVO attaccato a `GI`, e restituisce la sequenza di hash per turno.
	 *
	 * ⚠️ **Un hash per turno e non uno solo**: `ARTTurnManager::TurnLog` viene azzerato a ogni turno
	 * (`TurnLog.Reset()`), quindi l'ultimo da solo descriverebbe l'ultimo turno. La sequenza distingue
	 * anche due partite che finiscono uguali passando per strade diverse.
	 *
	 * @param TurnsToPlay  quanti turni giocare al massimo; la partita si ferma prima se finisce.
	 * @param OutWorld     se valorizzato, il mondo NON viene distrutto e resta al chiamante.
	 * @param BetweenTurns eseguita dopo ogni turno: e' il punto in cui un test infila un `ESC`.
	 */
	TArray<uint32> PlayMatchInFreshWorld(UGameInstance* GI, int32 TurnsToPlay, UWorld** OutWorld = nullptr,
		TFunction<void(int32)> BetweenTurns = nullptr)
	{
		TArray<uint32> Hashes;

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World)
		{
			return Hashes;
		}
		World->SetGameInstance(GI);
		World->InitializeActorsForPlay(FURL());

		ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
		World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		ARTTurnManager* TM = Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass()));

		if (!HexMap || !TM || !GameMode)
		{
			RTWorldFixtures::DestroyWorld(World);
			return Hashes;
		}

		GameMode->bAutobattle = true;
		GameMode->SetupHexMatch(HexMap);

		int32 Turni = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < TurnsToPlay)
		{
			TM->LockInAndResolve();
			for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
			{
				TM->Tick(0.05f);
			}
			Hashes.Add(URTTurnLogLibrary::HashTurnLogOrdered(TM->GetTurnLog()));
			++Turni;
			if (BetweenTurns)
			{
				BetweenTurns(Turni);
			}
		}

		if (OutWorld)
		{
			*OutWorld = World;
		}
		else
		{
			RTWorldFixtures::DestroyWorld(World);
		}
		return Hashes;
	}
}

/**
 * Una partita giocata **attraversando la pausa** e' identica a una giocata senza mai aprirla.
 *
 * 🔴 **E' il vincolo offline-only del DoD reso test invece che promessa.** Il DoD dice: *«in
 * multiplayer non esistera' una pausa globale […] la differenza va preservata nell'architettura, non
 * scoperta in v0.5»*. Una frase cosi' non ha un oracolo, e questo test gliene da' uno: se qualcuno
 * introducesse un `SetPause`, una dilatazione del tempo o un flag nel `TurnManager`, il resolver smetterebbe
 * di avanzare mentre la pausa e' aperta e le due sequenze divergerebbero — o il turno non si chiuderebbe.
 *
 * ⛔ **Non dice che il gioco “va avanti durante la pausa”**, e la differenza conta: il turno simultaneo non
 * avanza da solo — `CLAUDE.md` vieta il Tick per decidere sequencing — e mentre la pausa e' a schermo
 * nessuno chiama `LockInAndResolve`, perche' il puntatore e' in `Modal`. Cio' che si prova qui e' che la
 * pausa **non e' nel percorso della simulazione**: non la sospende, non la tocca, non lascia traccia.
 *
 * Il criterio complementare e' un comando che cerca le CHIAMATE e scarta i commenti — un pattern nudo
 * troverebbe la prosa che nomina i token, e con la parentesi troverebbe il comando stesso:
 *
 *     git grep -n "SetPause(\|SetGlobalTimeDilation(" -- Source/ \
 *       | grep -vE "^[^:]+:[0-9]+:[[:space:]]*(//|\*|/\*)"        ->  zero righe (2026-08-23)
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPauseLeavesNoTraceInTheSimulationTest,
	"RefactorTactics.Frontend.PauseLeavesNoTraceInTheSimulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPauseLeavesNoTraceInTheSimulationTest::RunTest(const FString&)
{
	constexpr int32 Turni = 3;

	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	// (a) senza mai aprire la pausa.
	const TArray<uint32> SenzaPausa = RTPauseTestsLocal::PlayMatchInFreshWorld(GI, Turni);
	if (!TestEqual(TEXT("la partita di riferimento ha giocato i turni chiesti"), SenzaPausa.Num(), Turni))
	{
		RTPauseTestsLocal::ReleaseNavigator(GI); return false;
	}

	// (b) la stessa partita, con un ESC e un RESUME fra un turno e l'altro.
	int32 PauseAperte = 0;
	const TArray<uint32> ConPausa = RTPauseTestsLocal::PlayMatchInFreshWorld(GI, Turni, nullptr,
		[&PauseAperte, Nav](int32 /*TurnIndex*/)
		{
			if (Nav->ShowPause() == ERTNavResult::Ok)
			{
				++PauseAperte;
				Nav->ResumeMatch();
			}
		});

	TestEqual(TEXT("la pausa e' stata davvero aperta a ogni turno: il test non e' vacuo"), PauseAperte, Turni);
	TestEqual(TEXT("stessi turni"), ConPausa.Num(), SenzaPausa.Num());
	TestTrue(TEXT("e turno per turno la partita e' la stessa: la pausa non tocca la simulazione"),
		ConPausa == SenzaPausa);

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

/**
 * **Il criterio forte del DoD**: dopo il ritorno al menu, una partita nuova e' indistinguibile da una
 * avviata da fresco.
 *
 * ⚠️ **La clausola «con lo stesso seed» del DoD e' INERTE, e il test non ci si appoggia.** Il progetto non
 * ha alcun RNG — `RTTestScenario.h` lo dice (*«Seed dichiarato ma non consumato»*) e
 * `RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed` lo prova. Il determinismo qui viene
 * dall'ordinamento canonico, non da un seme: cio' che morde e' **il confronto degli esiti**, e scriverlo
 * come «stesso seed» avrebbe portato a passare un numero credendo che facesse il lavoro.
 *
 * ⚠️ **Si interrompe a META' partita**, e non e' un dettaglio d'allestimento: a partita conclusa il Cleanup
 * ha gia' ripulito quasi tutto, e il test piu' facile da scrivere sarebbe anche quello cieco. Il caso che
 * rivela lo stato vivo e' `ESC` con unita' mosse e intenti in volo.
 *
 * 🔴 **Con la controprova, perche' un test di uguaglianza su un sistema senza stato vivo passerebbe senza
 * provare niente**: la seconda meta' allestisce una partita **senza** smontare la prima, e verifica che
 * l'oracolo la distingua. Se non lo facesse, l'uguaglianza qui sopra non significherebbe nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSameOutcomeAfterReturnToMainMenuTest,
	"RefactorTactics.Frontend.SameOutcomeAfterReturnToMainMenu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSameOutcomeAfterReturnToMainMenuTest::RunTest(const FString&)
{
	constexpr int32 TurniDiRiferimento = 4;
	constexpr int32 TurniPrimaDiMollare = 2;

	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = RTPauseTestsLocal::MakeStartedNavigator(GI);
	if (!TestNotNull(TEXT("il navigatore esiste"), Nav)) { RTPauseTestsLocal::ReleaseNavigator(GI); return false; }

	// (a) la misura di riferimento: una partita da fresco, in una sessione appena nata.
	const TArray<uint32> Riferimento = RTPauseTestsLocal::PlayMatchInFreshWorld(GI, TurniDiRiferimento);
	if (!TestEqual(TEXT("la partita di riferimento ha giocato i turni chiesti"),
		Riferimento.Num(), TurniDiRiferimento))
	{
		RTPauseTestsLocal::ReleaseNavigator(GI); return false;
	}

	// (b) una partita LASCIATA A META': ESC, e ritorno al menu.
	UWorld* AbandonedWorld = nullptr;
	RTPauseTestsLocal::PlayMatchInFreshWorld(GI, TurniPrimaDiMollare, &AbandonedWorld);
	if (!TestNotNull(TEXT("la partita da abbandonare esiste"), AbandonedWorld))
	{
		RTPauseTestsLocal::ReleaseNavigator(GI); return false;
	}

	TestEqual(TEXT("ESC a meta' partita"), Nav->ShowPause(), ERTNavResult::Ok);
	TestEqual(TEXT("RETURN TO MAIN MENU"), Nav->RequestReturnToMainMenu(), ERTNavResult::Ok);
	Nav->ConsumePendingFrontendLevel();

	// ⛔ **Questo e' lo smontaggio.** Nel gioco lo fa `OpenLevel`, che distrugge il mondo con dentro
	// `ARTTurnManager`, le `ARTUnit` e il GameMode; qui lo fa la fixture, che e' la stessa cosa senza un
	// livello vero da caricare. Cio' che sopravvive — la `GameInstance` e i suoi subsystem — e' identico
	// nei due casi, ed e' esattamente il soggetto della misura.
	RTWorldFixtures::DestroyWorld(AbandonedWorld);

	// (c) una partita nuova, nella STESSA sessione: deve essere indistinguibile da (a).
	const TArray<uint32> DopoIlRitorno = RTPauseTestsLocal::PlayMatchInFreshWorld(GI, TurniDiRiferimento);
	TestEqual(TEXT("la partita dopo il ritorno al menu ha giocato gli stessi turni"),
		DopoIlRitorno.Num(), Riferimento.Num());
	TestTrue(TEXT("e turno per turno e' identica a una partita avviata da fresco: nessuno stato e' sopravvissuto"),
		DopoIlRitorno == Riferimento);

	// (d) 🔴 CONTROPROVA. Senza lo smontaggio l'oracolo deve accorgersene, o (c) non prova niente.
	UWorld* StillAliveWorld = nullptr;
	RTPauseTestsLocal::PlayMatchInFreshWorld(GI, TurniPrimaDiMollare, &StillAliveWorld);
	if (TestNotNull(TEXT("la partita che NON viene smontata esiste"), StillAliveWorld))
	{
		// Una seconda partita allestita mentre la prima e' ancora viva: nello stesso mondo restano il
		// `TurnManager` e le unita' della precedente, ed e' quello che «lasciare stato vivo» significa.
		ARTHexMapActor* SecondMap = StillAliveWorld->SpawnActor<ARTHexMapActor>();
		ARTGameMode* SecondMode = StillAliveWorld->SpawnActor<ARTGameMode>();
		ARTTurnManager* TM = Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(StillAliveWorld, ARTTurnManager::StaticClass()));

		if (SecondMap && SecondMode && TM)
		{
			SecondMode->bAutobattle = true;
			SecondMode->SetupHexMatch(SecondMap);

			TArray<uint32> Sporca;
			int32 Turni = 0;
			while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < TurniDiRiferimento)
			{
				TM->LockInAndResolve();
				for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
				{
					TM->Tick(0.05f);
				}
				Sporca.Add(URTTurnLogLibrary::HashTurnLogOrdered(TM->GetTurnLog()));
				++Turni;
			}

			TestFalse(TEXT("l'oracolo DISTINGUE una partita avviata sopra una viva: non e' un test vacuo"),
				Sporca == Riferimento);
		}

		RTWorldFixtures::DestroyWorld(StillAliveWorld);
	}

	RTPauseTestsLocal::ReleaseNavigator(GI);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
