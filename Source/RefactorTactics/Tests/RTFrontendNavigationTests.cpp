// CP 46.1 (#936) — lo stack di navigazione del frontend.
//
// ⚠️ **Questi test esistono contro una previsione sbagliata, e vale la pena dirlo qui.** D-144 e la spec
// owner dichiaravano che i checkpoint di E46 non avessero «un test automatico possibile, perche' il
// repository non ha infrastruttura di test UI». E' falso in due modi: `RTScreenHudWidgetTests.cpp` prova
// gia' widget UMG headless costruendo un mondo (CP 11.7), e soprattutto **la navigazione non e' UI** — e'
// una macchina a stati che *governa* la UI. Separata dalla presentazione, si prova senza mondo, senza
// widget e senza editor.
//
// Cio' che resta davvero non automatizzabile e' il layout dentro il `.uasset`, ed e' di `PIE-V01-FRONTEND-NAV`.

#include "Misc/AutomationTest.h"
#include "Frontend/RTScreenStack.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FName Main(TEXT("Main"));
	const FName Play(TEXT("Play"));
	const FName Settings(TEXT("Settings"));
	const FName ConfirmQuit(TEXT("ConfirmQuit"));
	const FName ErrorModal(TEXT("Error"));
}

/**
 * La radice non ha Back. E' la prima meta' del «nessun dead-end» del DoD: non si esce dal frontend
 * cadendo fuori dallo stack, e `PopScreen` sulla radice non e' un errore da gestire — e' un no-op
 * dichiarato, perche' l'alternativa (uno stack vuoto) non ha nessuna schermata da mostrare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendRootHasNoBackTest,
	"RefactorTactics.Frontend.RootHasNoBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendRootHasNoBackTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);

	TestEqual(TEXT("la radice e' la schermata corrente"), Stack.CurrentScreen(), Main);
	TestEqual(TEXT("profondita' 1"), Stack.Depth(), 1);
	TestFalse(TEXT("dalla radice non si torna indietro"), Stack.CanGoBack());

	const ERTNavResult Result = Stack.PopScreen();

	TestEqual(TEXT("il Pop sulla radice e' rifiutato"), Result, ERTNavResult::BlockedAtRoot);
	TestEqual(TEXT("e non svuota lo stack"), Stack.Depth(), 1);
	TestEqual(TEXT("la radice e' ancora li'"), Stack.CurrentScreen(), Main);

	return true;
}

/**
 * `Back` risale a **chi ha spinto**, non a una schermata scelta a parte: e' l'unica proprieta' che rende
 * il back stack «esplicito» invece che una convenzione fra widget.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendBackReturnsToPusherTest,
	"RefactorTactics.Frontend.BackReturnsToTheScreenThatPushed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendBackReturnsToPusherTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);

	TestEqual(TEXT("push accettato"), Stack.PushScreen(Play), ERTNavResult::Ok);
	TestEqual(TEXT("siamo su Play"), Stack.CurrentScreen(), Play);
	TestTrue(TEXT("ora il Back esiste"), Stack.CanGoBack());

	TestEqual(TEXT("secondo push"), Stack.PushScreen(Settings), ERTNavResult::Ok);
	TestEqual(TEXT("siamo su Settings"), Stack.CurrentScreen(), Settings);
	TestEqual(TEXT("profondita' 3"), Stack.Depth(), 3);

	TestEqual(TEXT("pop"), Stack.PopScreen(), ERTNavResult::Ok);
	TestEqual(TEXT("torniamo a chi ha spinto Settings"), Stack.CurrentScreen(), Play);

	TestEqual(TEXT("pop"), Stack.PopScreen(), ERTNavResult::Ok);
	TestEqual(TEXT("e poi alla radice"), Stack.CurrentScreen(), Main);
	TestFalse(TEXT("dove il Back sparisce di nuovo"), Stack.CanGoBack());

	return true;
}

/**
 * Spingere due volte la **stessa** schermata la impila due volte, e il Back le attraversa entrambe.
 *
 * Non e' una svista: lo stack non deduplica **di proposito**. Un `Settings` aperto dal Main e uno aperto
 * dalla Pause sono la stessa schermata con due ritorni diversi, e collassarli manderebbe il Back nel posto
 * sbagliato — che e' il difetto che questo checkpoint esiste per rendere impossibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendStackDoesNotDedupeTest,
	"RefactorTactics.Frontend.SameScreenPushedTwiceKeepsTwoReturns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendStackDoesNotDedupeTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);
	Stack.PushScreen(Settings);
	Stack.PushScreen(Play);
	Stack.PushScreen(Settings);

	TestEqual(TEXT("profondita' 4"), Stack.Depth(), 4);
	TestEqual(TEXT("in cima Settings"), Stack.CurrentScreen(), Settings);

	Stack.PopScreen();
	TestEqual(TEXT("sotto c'e' Play, non l'altro Settings"), Stack.CurrentScreen(), Play);

	Stack.PopScreen();
	TestEqual(TEXT("e sotto ancora il primo Settings"), Stack.CurrentScreen(), Settings);

	return true;
}

/**
 * `ReturnMain` e' l'uscita di sicurezza del «nessun dead-end»: da **qualunque** stato riporta alla radice,
 * modali compresi. Se ci fosse uno stato da cui non riporta, esisterebbe un dead-end.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendReturnMainEmptiesEverythingTest,
	"RefactorTactics.Frontend.ReturnMainClearsScreensAndModals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendReturnMainEmptiesEverythingTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);
	Stack.PushScreen(Play);
	Stack.PushScreen(Settings);
	Stack.ShowModal(ConfirmQuit);

	TestEqual(TEXT("ReturnMain accettato"), Stack.ReturnMain(), ERTNavResult::Ok);

	TestEqual(TEXT("siamo alla radice"), Stack.CurrentScreen(), Main);
	TestEqual(TEXT("profondita' 1"), Stack.Depth(), 1);
	TestFalse(TEXT("nessun modale sopravvive"), Stack.IsModalOpen());
	TestFalse(TEXT("e nessun Back residuo"), Stack.CanGoBack());
	TestTrue(TEXT("la schermata torna interattiva"), Stack.IsScreenInteractive());

	return true;
}

/**
 * Un modale **disabilita** la schermata sotto. E' il DoD alla lettera, ed e' la proprieta' da cui dipende
 * tutto il resto del comportamento modale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendModalDisablesScreenTest,
	"RefactorTactics.Frontend.OpenModalDisablesTheScreenBelow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendModalDisablesScreenTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);
	Stack.PushScreen(Play);

	TestTrue(TEXT("senza modali la schermata e' interattiva"), Stack.IsScreenInteractive());

	TestEqual(TEXT("modale aperto"), Stack.ShowModal(ErrorModal), ERTNavResult::Ok);

	TestTrue(TEXT("il modale risulta aperto"), Stack.IsModalOpen());
	TestEqual(TEXT("ed e' quello in cima"), Stack.TopModal(), ErrorModal);
	TestFalse(TEXT("la schermata sotto NON e' interattiva"), Stack.IsScreenInteractive());
	TestEqual(TEXT("ma resta la schermata corrente"), Stack.CurrentScreen(), Play);

	TestEqual(TEXT("chiusura"), Stack.CloseModal(), ERTNavResult::Ok);

	TestFalse(TEXT("nessun modale aperto"), Stack.IsModalOpen());
	TestTrue(TEXT("e la schermata torna interattiva"), Stack.IsScreenInteractive());

	return true;
}

/**
 * Navigare con un modale aperto e' **rifiutato con un motivo**, non ignorato in silenzio.
 *
 * Decisione presa in sessione il 2026-08-16, perche' il DoD non la copriva: se la schermata sotto non
 * riceve input, non puo' nemmeno navigare. L'alternativa — chiudere il modale e proseguire — scarterebbe
 * un dialogo di conferma proprio quando serve a fermare qualcosa. Il *reason code* segue la stessa regola
 * di CP 11.8: un rifiuto silenzioso e' un difetto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendModalBlocksNavigationTest,
	"RefactorTactics.Frontend.ModalBlocksNavigationWithAReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendModalBlocksNavigationTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);
	Stack.PushScreen(Play);
	Stack.ShowModal(ConfirmQuit);

	const int32 DepthBefore = Stack.Depth();

	TestEqual(TEXT("il push e' bloccato dal modale"),
		Stack.PushScreen(Settings), ERTNavResult::BlockedByModal);
	TestEqual(TEXT("anche il Back lo e'"),
		Stack.PopScreen(), ERTNavResult::BlockedByModal);

	TestEqual(TEXT("e lo stack non si e' mosso"), Stack.Depth(), DepthBefore);
	TestEqual(TEXT("siamo ancora su Play"), Stack.CurrentScreen(), Play);

	Stack.CloseModal();

	TestEqual(TEXT("chiuso il modale il push passa"),
		Stack.PushScreen(Settings), ERTNavResult::Ok);
	TestEqual(TEXT("siamo su Settings"), Stack.CurrentScreen(), Settings);

	return true;
}

/**
 * `CloseModal` senza modali aperti e' rifiutato, non un crash ne' un pop di schermata.
 *
 * E' il caso che confonde i due stack: senza questa guardia un `CloseModal` di troppo — un doppio click
 * sul pulsante di chiusura — mangerebbe una schermata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendCloseModalWithoutModalTest,
	"RefactorTactics.Frontend.CloseModalWithoutModalDoesNotPopAScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendCloseModalWithoutModalTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);
	Stack.PushScreen(Play);

	TestEqual(TEXT("niente da chiudere"), Stack.CloseModal(), ERTNavResult::NoModalOpen);
	TestEqual(TEXT("la schermata e' intatta"), Stack.CurrentScreen(), Play);
	TestEqual(TEXT("profondita' invariata"), Stack.Depth(), 2);

	return true;
}

/**
 * I modali si impilano e si chiudono in ordine inverso: un errore aperto **sopra** una conferma non
 * cancella la conferma.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendModalsStackTest,
	"RefactorTactics.Frontend.ModalsCloseInReverseOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendModalsStackTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);
	Stack.ShowModal(ConfirmQuit);
	Stack.ShowModal(ErrorModal);

	TestEqual(TEXT("in cima l'ultimo aperto"), Stack.TopModal(), ErrorModal);

	Stack.CloseModal();

	TestTrue(TEXT("sotto c'e' ancora la conferma"), Stack.IsModalOpen());
	TestEqual(TEXT("ed e' lei la cima"), Stack.TopModal(), ConfirmQuit);
	TestFalse(TEXT("la schermata resta disabilitata"), Stack.IsScreenInteractive());

	Stack.CloseModal();

	TestFalse(TEXT("ora e' libera"), Stack.IsModalOpen());
	TestTrue(TEXT("e interattiva"), Stack.IsScreenInteractive());

	return true;
}

/**
 * **Nessun dead-end, verificato per esplorazione invece che per ispezione.**
 *
 * Il DoD chiede che «da ogni schermata raggiungibile esista un percorso di ritorno alla radice». Gli altri
 * test lo mostrano su cammini scelti a mano — cioe' provano i casi a cui ho pensato. Questo genera una
 * sequenza deterministica di operazioni miste, e dopo **ciascuna** verifica due invarianti: lo stack non e'
 * mai vuoto, e `ReturnMain` riporta sempre alla radice. Il seme e' fisso: un test che cambia input a ogni
 * esecuzione non e' riproducibile, ed e' la stessa ragione per cui gli scenari hanno un `Seed`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendNoDeadEndTest,
	"RefactorTactics.Frontend.NoReachableStateIsADeadEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendNoDeadEndTest::RunTest(const FString&)
{
	const FName Screens[] = { Play, Settings, FName(TEXT("Scenarios")) };
	const FName Modals[] = { ConfirmQuit, ErrorModal };

	FRTScreenStack Stack(Main);
	uint32 Rng = 0x5bd1e995u; // seme fisso: la sequenza deve essere la stessa a ogni esecuzione

	for (int32 Step = 0; Step < 200; ++Step)
	{
		Rng = Rng * 1664525u + 1013904223u;
		const uint32 Op = (Rng >> 16) % 4u;

		switch (Op)
		{
		case 0: Stack.PushScreen(Screens[(Rng >> 8) % 3u]); break;
		case 1: Stack.PopScreen();                          break;
		case 2: Stack.ShowModal(Modals[(Rng >> 8) % 2u]);   break;
		default: Stack.CloseModal();                        break;
		}

		if (!TestTrue(TEXT("lo stack non e' mai vuoto"), Stack.Depth() >= 1))
		{
			return false;
		}
		if (!TestFalse(TEXT("la schermata corrente e' sempre valida"), Stack.CurrentScreen().IsNone()))
		{
			return false;
		}

		// L'uscita di sicurezza deve funzionare da QUI, qualunque sia lo stato raggiunto.
		FRTScreenStack Probe = Stack;
		Probe.ReturnMain();
		if (!TestEqual(TEXT("ReturnMain riporta alla radice da ogni stato"), Probe.CurrentScreen(), Main))
		{
			return false;
		}
		if (!TestFalse(TEXT("e non lascia modali"), Probe.IsModalOpen()))
		{
			return false;
		}
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Il navigatore. Qui serve un `UGameInstance` — non un mondo, non un viewport: le schermate senza binding
// non disegnano nulla, e la navigazione resta valida lo stesso. E' quella divisione a rendere provabile
// il navigatore invece che solo lo stack.
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
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
}

/**
 * Il navigatore delega allo stack invece di tenere una seconda copia dello stato.
 *
 * ⚠️ E' la proprieta' che impedisce il difetto piu' probabile di questa classe: due sorgenti di verita' —
 * lo stack e i widget a schermo — che divergono in silenzio. Qui si verifica che le query pubbliche
 * rispondano **dallo stack**, non da cio' che e' stato disegnato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendNavigatorDelegatesToStackTest,
	"RefactorTactics.Frontend.NavigatorAnswersFromTheStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendNavigatorDelegatesToStackTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->InitializeFrontend(Main);

	TestEqual(TEXT("radice"), Nav->GetCurrentScreen(), Main);
	TestFalse(TEXT("niente Back sulla radice"), Nav->CanGoBack());
	TestEqual(TEXT("profondita' 1"), Nav->GetDepth(), 1);

	TestEqual(TEXT("push"), Nav->PushScreen(Play), ERTNavResult::Ok);
	TestEqual(TEXT("cima aggiornata"), Nav->GetCurrentScreen(), Play);
	TestTrue(TEXT("ora si torna indietro"), Nav->CanGoBack());

	TestEqual(TEXT("modale"), Nav->ShowModal(ConfirmQuit), ERTNavResult::Ok);
	TestTrue(TEXT("modale aperto"), Nav->IsModalOpen());
	TestFalse(TEXT("schermata non interattiva"), Nav->IsScreenInteractive());
	TestFalse(TEXT("e il Back e' sospeso"), Nav->CanGoBack());

	TestEqual(TEXT("push bloccato"), Nav->PushScreen(Settings), ERTNavResult::BlockedByModal);
	TestEqual(TEXT("la cima non e' cambiata"), Nav->GetCurrentScreen(), Play);

	TestEqual(TEXT("ReturnMain non e' mai bloccato"), Nav->ReturnMain(), ERTNavResult::Ok);
	TestEqual(TEXT("siamo alla radice"), Nav->GetCurrentScreen(), Main);
	TestFalse(TEXT("senza modali"), Nav->IsModalOpen());

	ReleaseNavigator(GI);
	return true;
}

/**
 * **Una schermata senza binding naviga lo stesso.**
 *
 * E' la divisione che regge tutto il checkpoint: la navigazione e' logica, il widget e' presentazione. Se
 * un push fallisse per un asset mancante, la logica dipenderebbe dagli asset — e nessun test potrebbe
 * esistere senza `.uasset`, che e' esattamente la previsione sbagliata da cui questo file e' nato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendNavigationWithoutAssetsTest,
	"RefactorTactics.Frontend.NavigationWorksWithoutWidgetBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendNavigationWithoutAssetsTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->InitializeFrontend(Main);

	TestEqual(TEXT("push senza binding"), Nav->PushScreen(Settings), ERTNavResult::Ok);
	TestEqual(TEXT("lo stack si e' mosso"), Nav->GetCurrentScreen(), Settings);
	TestNull(TEXT("ma nessun widget e' stato creato"), Nav->FindLiveWidget(Settings));

	TestEqual(TEXT("e si torna indietro"), Nav->PopScreen(), ERTNavResult::Ok);
	TestEqual(TEXT("fino alla radice"), Nav->GetCurrentScreen(), Main);

	ReleaseNavigator(GI);
	return true;
}

/**
 * Un nome vuoto non identifica niente, e viene rifiutato **prima** di entrare nello stack.
 *
 * Senza questa guardia `CurrentScreen()` potrebbe restituire `NAME_None` su uno stack costruito
 * correttamente, e ogni chiamante dovrebbe difendersi da uno stato che il dominio non ha.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendRejectsEmptyNameTest,
	"RefactorTactics.Frontend.EmptyScreenNameIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendRejectsEmptyNameTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);

	TestEqual(TEXT("push vuoto rifiutato"), Stack.PushScreen(NAME_None), ERTNavResult::InvalidScreen);
	TestEqual(TEXT("modale vuoto rifiutato"), Stack.ShowModal(NAME_None), ERTNavResult::InvalidScreen);
	TestEqual(TEXT("lo stack non si e' mosso"), Stack.Depth(), 1);
	TestFalse(TEXT("nessun modale"), Stack.IsModalOpen());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
