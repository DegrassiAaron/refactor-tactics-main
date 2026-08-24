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
// Per `ERTLoadPhase`: il `BACK` del modale d'errore sceglie fra `PopScreen` e `ReturnMain` guardando la
// fase raggiunta dall'allestimento, che e' un dato d'avvio e non di navigazione (CP 46.2).
#include "Frontend/RTStartupReport.h"
// Per `RTScreenIds::Main` / `::Settings`: i nomi canonici delle due schermate vere (CP 46.3).
#include "Frontend/RTFrontendScreenIds.h"
#include "Frontend/RTFrontendWidgets.h"
#include "RTFrontendMatchListenerForTest.h"
#include "RTWorldFixtures.h"   // URTErrorModalWidgetBase: il modale si verifica ARMATO
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
// Solo per avere una `UUserWidget` **concreta** da istanziare: `UUserWidget` e' `Abstract`.
#include "UI/RTScreenHudWidgets.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ **`Main` e `Settings` non sono piu' scritte qui**: sono schermate vere del frontend, e il loro nome
	// canonico vive in `RTFrontendScreenIds.h` da CP 46.3. Questo blocco era l'unico posto in cui `"Main"`
	// compariva, ed e' la ragione per cui quell'header esiste — lasciarci la copia avrebbe reso falsa la
	// sua stessa motivazione, e un rename del nome canonico avrebbe continuato a compilare qui asserendo
	// contro il letterale vecchio. Trovato in code review.
	const FName& Main = RTScreenIds::Main;
	const FName& Settings = RTScreenIds::Settings;

	// Questi tre invece restano locali: sono nomi **di comodo per i test**, non schermate dichiarate. Il
	// navigatore accetta qualunque `FName` non vuoto, ed e' esattamente cio' che serve per provare lo
	// stack senza legarlo alle schermate che il gioco spedisce.
	const FName Play(TEXT("Play"));
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
 * **Nessun dead-end, verificato per esplorazione — e il test misura la propria copertura.**
 *
 * Il DoD chiede che «da ogni schermata raggiungibile esista un percorso di ritorno alla radice». Gli altri
 * test lo mostrano su cammini scelti a mano, cioe' provano i casi a cui ho pensato; questo esplora.
 *
 * 🔴 **La prima stesura esplorava un solo stato, e la code review l'ha MISURATO.** Sceglieva l'operazione
 * a caso fra quattro *senza guardare lo stato*, e siccome `ShowModal` non fallisce mai mentre un modale
 * aperto blocca sia `PushScreen` sia `PopScreen`, il cammino veniva **assorbito nel regime modale**.
 * Simulando lo stesso LCG: profondita' 1 in **197 passi su 200** (massimo 2), **188 su 200** con un modale
 * aperto (fino a 14 impilati), **92 operazioni su 200 rifiutate**. Il ramo di `ReturnMain` che svuota le
 * schermate — cioe' quello che il test dichiara di provare — era esercitato **3 volte su 200**. Cambiare
 * seme non salvava: su sette semi la profondita' massima restava fra 2 e 8. Il difetto era
 * **strutturale**, non del seme.
 *
 * Due correzioni, e servono entrambe:
 * **(a)** si sceglie fra le operazioni **legali nello stato corrente**, e i modali sono limitati a 2, cosi'
 *     il cammino non degenera;
 * **(b)** il test **asserisce la propria copertura** in fondo. E' la parte che conta: un test generativo
 *     che non misura la propria distribuzione dichiara di aver esplorato cio' che non ha mai visitato, e
 *     resta verde mentre lo fa. Con la prima stesura, le asserzioni di (b) sarebbero fallite.
 *
 * Il seme resta fisso: un test che cambia input a ogni esecuzione non e' riproducibile — stessa ragione
 * per cui gli scenari hanno un `Seed`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendNoDeadEndTest,
	"RefactorTactics.Frontend.NoReachableStateIsADeadEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendNoDeadEndTest::RunTest(const FString&)
{
	const FName ScreenPool[] = { Play, Settings, FName(TEXT("Scenarios")) };
	const FName ModalPool[] = { ConfirmQuit, ErrorModal };
	constexpr int32 MaxModals = 2;
	constexpr int32 Steps = 400;

	FRTScreenStack Stack(Main);
	uint32 Rng = 0x5bd1e995u; // seme fisso: la sequenza deve essere la stessa a ogni esecuzione

	int32 MaxDepthSeen = 1;
	int32 MaxModalsSeen = 0;
	int32 PushesDone = 0, PopsDone = 0, ModalsOpened = 0, ModalsClosed = 0;
	int32 ProbesFromDeepState = 0;

	for (int32 Step = 0; Step < Steps; ++Step)
	{
		Rng = Rng * 1664525u + 1013904223u;

		// Le operazioni LEGALI adesso. E' questo che impedisce al cammino di degenerare: scegliendo fra
		// tutte e quattro, il regime modale assorbiva il 94% dei passi.
		TArray<int32, TInlineAllocator<4>> Legal;
		if (Stack.IsModalOpen())
		{
			Legal.Add(3);                                        // CloseModal
			if (Stack.ModalDepth() < MaxModals) { Legal.Add(2); }
		}
		else
		{
			Legal.Add(0);                                        // PushScreen
			Legal.Add(2);                                        // ShowModal
			if (Stack.CanGoBack()) { Legal.Add(1); }             // PopScreen
		}

		const int32 Op = Legal[(Rng >> 16) % (uint32)Legal.Num()];
		switch (Op)
		{
		case 0:
			if (Stack.PushScreen(ScreenPool[(Rng >> 8) % 3u]) == ERTNavResult::Ok) { ++PushesDone; }
			break;
		case 1:
			if (Stack.PopScreen() == ERTNavResult::Ok) { ++PopsDone; }
			break;
		case 2:
			// `ShowModal` rifiuta duplicati e id gia' in stack: si prova la coppia finche' uno passa.
			for (const FName& Candidate : ModalPool)
			{
				if (Stack.ShowModal(Candidate) == ERTNavResult::Ok) { ++ModalsOpened; break; }
			}
			break;
		default:
			if (Stack.CloseModal() == ERTNavResult::Ok) { ++ModalsClosed; }
			break;
		}

		MaxDepthSeen = FMath::Max(MaxDepthSeen, Stack.Depth());
		MaxModalsSeen = FMath::Max(MaxModalsSeen, Stack.ModalDepth());

		if (!TestTrue(TEXT("lo stack non e' mai vuoto"), Stack.Depth() >= 1))
		{
			return false;
		}
		if (!TestTrue(TEXT("la schermata corrente non e' mai NAME_None"), !Stack.CurrentScreen().IsNone()))
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
		if (Stack.Depth() >= 3)
		{
			++ProbesFromDeepState;
		}
	}

	// (b) **La copertura fa parte del risultato.** Senza queste righe il test resterebbe verde anche
	//     esplorando un solo stato — che e' esattamente cio' che faceva prima della code review.
	TestTrue(TEXT("la sequenza raggiunge almeno profondita' 4"), MaxDepthSeen >= 4);
	TestTrue(TEXT("e impila due modali"), MaxModalsSeen >= 2);
	TestTrue(TEXT("con almeno 40 push riusciti"), PushesDone >= 40);
	TestTrue(TEXT("almeno 40 pop riusciti"), PopsDone >= 40);
	TestTrue(TEXT("almeno 40 modali aperti"), ModalsOpened >= 40);
	TestTrue(TEXT("almeno 40 modali chiusi"), ModalsClosed >= 40);
	TestTrue(TEXT("e ReturnMain provato da almeno 50 stati profondi"), ProbesFromDeepState >= 50);

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

/**
 * 🔴 **Un `FName` non puo' essere insieme schermata e modale, e senza questa guardia era un dead-end
 * raggiungibile da Blueprint.** Trovato in code review.
 *
 * La presentazione tiene **un widget per nome** (`LiveWidgets` e' chiusa per `FName`), quindi lo stesso id
 * nei due ruoli produce un widget solo. `SyncPresentation` lo disabilita in quanto «schermata sotto un
 * modale» mentre *e'* il modale: pulsante di chiusura compreso. Da li' non si esce — `PushScreen` e
 * `PopScreen` rispondono `BlockedByModal` — se non con un `ReturnMain` chiamato da fuori quel widget.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendModalCannotBeAScreenTest,
	"RefactorTactics.Frontend.ModalCannotShareItsNameWithAScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendModalCannotBeAScreenTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);
	Stack.PushScreen(Settings);

	TestEqual(TEXT("la schermata corrente non puo' diventare modale"),
		Stack.ShowModal(Settings), ERTNavResult::ScreenIsAlreadyOnStack);
	TestEqual(TEXT("nemmeno la radice, che e' piu' in basso"),
		Stack.ShowModal(Main), ERTNavResult::ScreenIsAlreadyOnStack);

	TestFalse(TEXT("nessun modale e' stato aperto"), Stack.IsModalOpen());
	TestTrue(TEXT("e la schermata resta interattiva"), Stack.IsScreenInteractive());

	// Un id che NON e' nello stack resta legittimo.
	TestEqual(TEXT("un modale vero passa"), Stack.ShowModal(ConfirmQuit), ERTNavResult::Ok);

	return true;
}

/**
 * Lo stesso modale due volte e' rifiutato: due voci nello stack e **un solo widget** darebbero una
 * finestra che chiede due chiusure. Lo stack sarebbe coerente, lo schermo no.
 *
 * ⚠️ Non vale per le schermate: `Settings` aperto dal Main e dalla Pause sono due ritorni diversi, e
 * `SameScreenPushedTwiceKeepsTwoReturns` lo pretende. La differenza e' che le schermate si vedono **una
 * per volta**, i modali tutti insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendModalIsNotOpenedTwiceTest,
	"RefactorTactics.Frontend.SameModalIsNotStackedTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendModalIsNotOpenedTwiceTest::RunTest(const FString&)
{
	FRTScreenStack Stack(Main);

	TestEqual(TEXT("prima apertura"), Stack.ShowModal(ErrorModal), ERTNavResult::Ok);
	TestEqual(TEXT("la seconda e' rifiutata"),
		Stack.ShowModal(ErrorModal), ERTNavResult::ScreenIsAlreadyOnStack);
	TestEqual(TEXT("un solo modale impilato"), Stack.ModalDepth(), 1);

	// Una chiusura sola deve bastare.
	Stack.CloseModal();
	TestFalse(TEXT("e lo schermo e' libero"), Stack.IsModalOpen());

	return true;
}

/**
 * 🔴 **Uno stack default-costruito e' VUOTO, e `ReturnMain` vi rispondeva `Ok`.** Trovato in code review.
 *
 * `USTRUCT` con `GENERATED_BODY()` obbliga a un costruttore di default, quindi lo stato vuoto e'
 * rappresentabile per forza — e `URTFrontendNavigator::Stack` lo attraversa davvero, prima di
 * `InitializeFrontend`. Non potendolo vietare, va reso **innocuo e interrogabile**: `IsValid()` lo
 * distingue, e l'uscita di sicurezza non dichiara piu' successo restando senza schermata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendDefaultStackIsInvalidTest,
	"RefactorTactics.Frontend.DefaultConstructedStackIsInvalidAndSaysSo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendDefaultStackIsInvalidTest::RunTest(const FString&)
{
	FRTScreenStack Empty;

	TestFalse(TEXT("non e' valido"), Empty.IsValid());
	TestEqual(TEXT("profondita' zero"), Empty.Depth(), 0);
	TestTrue(TEXT("nessuna schermata corrente"), Empty.CurrentScreen().IsNone());
	TestFalse(TEXT("niente Back"), Empty.CanGoBack());

	TestEqual(TEXT("ReturnMain NON dichiara successo"), Empty.ReturnMain(), ERTNavResult::InvalidScreen);
	TestEqual(TEXT("il Pop e' rifiutato"), Empty.PopScreen(), ERTNavResult::BlockedAtRoot);

	// Anche una radice vuota produce uno stack non valido, invece di uno stack con una schermata `None`.
	FRTScreenStack NoneRoot{ NAME_None };
	TestFalse(TEXT("radice vuota => stack non valido"), NoneRoot.IsValid());

	return true;
}

/**
 * `InitializeFrontend` con un nome vuoto **dice di no**, invece di restare muto.
 *
 * 🔴 Prima era `void`: il navigatore restava non inizializzato e ogni push successivo rispondeva `Ok`
 * mentre nessun widget compariva mai — un fallimento indistinguibile dal successo, che e' cio' che
 * `ERTNavResult` esiste per impedire. Trovato in code review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendInitRejectsEmptyRootTest,
	"RefactorTactics.Frontend.InitializeWithEmptyRootIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendInitRejectsEmptyRootTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	TestEqual(TEXT("radice vuota rifiutata"),
		Nav->InitializeFrontend(NAME_None), ERTNavResult::InvalidScreen);

	TestEqual(TEXT("e con una radice vera passa"),
		Nav->InitializeFrontend(Main), ERTNavResult::Ok);
	TestEqual(TEXT("radice attiva"), Nav->GetCurrentScreen(), Main);

	ReleaseNavigator(GI);
	return true;
}

/**
 * **La presentazione con un binding vero.**
 *
 * 🔴 La code review ha misurato che nessun test registrava una schermata, quindi `PresentWidget` usciva
 * alla prima riga in **tutti** i test: creazione, `AddToViewport`, `DismissWidget`, l'ordine smonta-poi-
 * monta e `SetIsEnabled` avevano copertura **zero**. `TestNull(FindLiveWidget(...))` passava per assenza
 * di binding — sarebbe passato identico con `PresentWidget` svuotata a `return;`.
 *
 * Il binding c'e' e non serve un `.uasset`: basta **una `UUserWidget` concreta qualsiasi**.
 *
 * ⚠️ **Non `UUserWidget` stesso**, ed e' costato un giro di build: e' `Abstract`, e `CreateWidget` la
 * rifiuta con *«Abstract, Deprecated or Replaced classes are not allowed to be used to construct a user
 * widget»*. Si usa `URTScreenHudWidgetBase`, che e' concreta (CP 11.7) e qui vale **solo come classe di
 * comodo**: il frontend non dipende dall'HUD in-match, e il navigatore non sa cosa sta istanziando —
 * e' proprio la proprieta' che questo test verifica. Una classe di prova propria richiederebbe un header
 * `UCLASS()` dentro il modulo di gioco, che costerebbe piu' di quanto valga (stessa scelta, e stessa
 * motivazione, di `RTScreenHudWidgetTests.cpp`).
 *
 * ⚠️ Cosa questo test NON prova, ed e' dichiarato: senza un viewport reale `AddToViewport` non mette nulla
 * a schermo, quindi `IsInViewport()` resta falso e il ramo di `SetIsEnabled` non viene esercitato. Quella
 * meta' resta di `PIE-V01-FRONTEND-NAV`. Cio' che si prova qui e' il **ciclo di vita** dei widget, che e'
 * dove stavano i due difetti trovati in review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendPresentsAndReusesWidgetsTest,
	"RefactorTactics.Frontend.NavigatorCreatesAndReusesWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendPresentsAndReusesWidgetsTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->RegisterScreen(Main, TSoftClassPtr<UUserWidget>(URTScreenHudWidgetBase::StaticClass()));
	Nav->RegisterScreen(Play, TSoftClassPtr<UUserWidget>(URTScreenHudWidgetBase::StaticClass()));
	Nav->InitializeFrontend(Main);

	UUserWidget* RootWidget = Nav->FindLiveWidget(Main);
	if (!TestNotNull(TEXT("la radice ha prodotto un widget"), RootWidget))
	{
		// Senza un mondo utilizzabile `CreateWidget` puo' non costruire: in quel caso il resto del test
		// non ha soggetto, e dirlo e' meglio che asserire su un `nullptr`.
		AddInfo(TEXT("CreateWidget non ha costruito in questo ambiente: presentazione non verificabile qui"));
		ReleaseNavigator(GI);
		return true;
	}

	Nav->PushScreen(Play);
	UUserWidget* PlayWidget = Nav->FindLiveWidget(Play);
	TestNotNull(TEXT("anche la seconda schermata"), PlayWidget);
	TestNotEqual(TEXT("e sono due istanze diverse"), (void*)RootWidget, (void*)PlayWidget);

	// Il Back non distrugge: i widget sono cache, ed e' la ragione per cui `FindLiveWidget` risponde
	// ancora dopo un pop. Il commento della funzione lo dichiara — qui e' verificato.
	Nav->PopScreen();
	TestEqual(TEXT("tornando indietro la radice e' la stessa istanza"),
		(void*)Nav->FindLiveWidget(Main), (void*)RootWidget);
	TestNotNull(TEXT("e il widget uscito resta istanziato (cache voluta)"), Nav->FindLiveWidget(Play));

	// Ri-navigare non ricrea.
	Nav->PushScreen(Play);
	TestEqual(TEXT("la seconda visita riusa l'istanza"),
		(void*)Nav->FindLiveWidget(Play), (void*)PlayWidget);

	ReleaseNavigator(GI);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// CP 46.2 (#937) — il `BACK` del modale d'errore.
//
// La voce del DoD chiede **due** esiti, non uno: `PopScreen` se l'errore compare durante il loading,
// `ReturnMain` se compare a partita gia' viva. Non e' una raffinatezza — `PopScreen` a partita viva
// lascerebbe una partita **sotto** il menu, che e' lo stato che CP 46.6 vieta.
//
// ⚠️ **Il discriminante non e' un flag nuovo**: e' `FRTStartupReport::Phase`, che vale `Ready` solo se
// l'allestimento e' arrivato in fondo. Il dato esisteva gia' e nessuno lo leggeva.
//
// ⚠️ **Il widget non naviga da se'**: espone la fase in cui e' stato armato, e il navigatore decide. E'
// l'invariante di CP 46.1 — un solo owner del flow — e questi test la esercitano dal lato del navigatore.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Errore durante il loading: non e' stato costruito niente, quindi si torna alla schermata precedente.
 *
 * ⚠️ **Il modale va chiuso PRIMA del pop, e il test lo verifica dall'esito e non dall'ordine dei
 * comandi**: `PopScreen` con un modale aperto risponde `BlockedByModal`, perche' la schermata sotto e'
 * disabilitata. Un'implementazione che facesse solo `PopScreen()` restituirebbe quel codice e lascerebbe
 * il giocatore **dentro il modale** — cioe' proprio il dead-end che il DoD vieta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackFromErrorDuringLoadingPopsTest,
	"RefactorTactics.Frontend.BackFromErrorDuringLoadingPopsScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackFromErrorDuringLoadingPopsTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	// ⚠️ **Profondita' 3, e non e' un dettaglio.** A profondita' 2 (`Main → Play`) questo test sarebbe
	// **vacuo**: `PopScreen` e `ReturnMain` porterebbero entrambi a `Main` con `GetDepth() == 1`, quindi
	// passerebbe anche con la regola invertita. Trovato ragionando sulla mutazione prima di committare —
	// la prima stesura usava due schermate e sarebbe stata verde per il motivo sbagliato.
	Nav->InitializeFrontend(Main);
	Nav->PushScreen(Play);
	Nav->PushScreen(Settings);
	Nav->ShowModal(ErrorModal);
	TestTrue(TEXT("precondizione: il modale e' aperto"), Nav->IsModalOpen());

	// Ogni fase che non sia `Ready` e' «durante il loading»: l'allestimento non e' arrivato in fondo.
	const ERTNavResult Result = Nav->BackFromError(ERTLoadPhase::Scenario);

	TestEqual(TEXT("la transizione riesce"), Result, ERTNavResult::Ok);
	TestFalse(TEXT("il modale e' chiuso"), Nav->IsModalOpen());
	// Un livello sotto — `Play` — e **non** la radice: e' l'asserzione che distingue `PopScreen` da
	// `ReturnMain`, e senza di essa i due esiti sarebbero indistinguibili.
	TestEqual(TEXT("si torna alla schermata precedente, non alla radice"), Nav->GetCurrentScreen(), Play);
	TestEqual(TEXT("e lo stack e' sceso di UNO, non smontato"), Nav->GetDepth(), 2);

	ReleaseNavigator(GI);
	return true;
}

/**
 * Errore a partita gia' viva: `ReturnMain`, che **smonta**.
 *
 * La differenza con il test sopra non e' cosmetica: qui lo stack torna alla radice da qualunque
 * profondita', mentre un `PopScreen` scenderebbe di un solo livello e lascerebbe la partita viva sotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackFromErrorWhenLiveReturnsMainTest,
	"RefactorTactics.Frontend.BackFromErrorWhenGameIsLiveReturnsMain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackFromErrorWhenLiveReturnsMainTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	// Profondita' 3: se il `BACK` facesse `PopScreen` si fermerebbe a `Play`, e la differenza fra i due
	// esiti sarebbe invisibile a profondita' 2. E' la ragione per cui questo test non usa lo stack minimo.
	Nav->InitializeFrontend(Main);
	Nav->PushScreen(Play);
	Nav->PushScreen(Settings);
	Nav->ShowModal(ErrorModal);

	const ERTNavResult Result = Nav->BackFromError(ERTLoadPhase::Ready);

	TestEqual(TEXT("la transizione riesce"), Result, ERTNavResult::Ok);
	TestFalse(TEXT("nessun modale resta aperto"), Nav->IsModalOpen());
	TestEqual(TEXT("si torna alla radice"), Nav->GetCurrentScreen(), Main);
	TestEqual(TEXT("e NON di un solo livello: lo stack e' smontato"), Nav->GetDepth(), 1);

	ReleaseNavigator(GI);
	return true;
}

/**
 * Il `BACK` non e' un dead-end **da nessuna delle fasi**, e il test misura la propria copertura.
 *
 * ⚠️ **Senza l'ultima asserzione questo test sarebbe vacuo**: un ciclo su un enum non dice quante fasi
 * abbia guardato, e il giorno in cui `ERTLoadPhase` ne guadagnasse una il ciclo continuerebbe a passare
 * ignorandola. E' il difetto che questo repository ha gia' pagato con i test generativi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBackFromErrorHasNoDeadEndTest,
	"RefactorTactics.Frontend.BackFromErrorLeavesNoDeadEndFromAnyPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBackFromErrorHasNoDeadEndTest::RunTest(const FString&)
{
	const ERTLoadPhase Phases[] = { ERTLoadPhase::Idle, ERTLoadPhase::Map, ERTLoadPhase::Scenario,
									ERTLoadPhase::Bots, ERTLoadPhase::Ready };

	int32 Covered = 0;
	for (const ERTLoadPhase Phase : Phases)
	{
		UGameInstance* GI = nullptr;
		URTFrontendNavigator* Nav = MakeNavigator(GI);
		if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

		Nav->InitializeFrontend(Main);
		Nav->PushScreen(Play);
		Nav->ShowModal(ErrorModal);

		const ERTNavResult Result = Nav->BackFromError(Phase);

		TestEqual(*FString::Printf(TEXT("fase %d: la transizione riesce"), (int32)Phase),
			Result, ERTNavResult::Ok);
		TestFalse(*FString::Printf(TEXT("fase %d: nessun modale resta aperto"), (int32)Phase),
			Nav->IsModalOpen());
		TestTrue(*FString::Printf(TEXT("fase %d: la schermata sotto e' interattiva"), (int32)Phase),
			Nav->IsScreenInteractive());

		++Covered;
		ReleaseNavigator(GI);
	}

	// La copertura e' il risultato, non il numero di giri: se l'enum cresce, questo assert cade.
	TestEqual(TEXT("coperte tutte le fasi dichiarate da ERTLoadPhase"),
		Covered, (int32)ERTLoadPhase::Ready + 1);

	return true;
}

// =================================================================================================
// CP 46.4 (#939) — `PLAY` avvia il vertical slice, e un avvio impossibile lo DICE
// =================================================================================================

/**
 * Senza un livello di partita configurato, `StartMatch` fallisce **rumorosamente**: error modal con la
 * causa, non un ritorno silenzioso al menu.
 *
 * 🔴 **E' il difetto che il DoD di #939 nomina per primo**, e il progetto ne ha gia' una famiglia: un
 * percorso che non risolve produce «un fallimento indistinguibile dal successo» (il `.ini` delle
 * schermate lo dichiara, e #1277 ha misurato che il log da solo non lo coglie). Qui il livello arriva
 * dalla stessa sede — una `UPROPERTY(Config)` — quindi lo stesso refuso e' possibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendStartMatchWithoutLevelFailsLoudTest,
	"RefactorTactics.Frontend.StartMatchWithoutLevelFailsLoud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendStartMatchWithoutLevelFailsLoudTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	// 🔴 **`InitializeFrontend` e non solo `MakeNavigator`.** Senza radice lo stack e' nello stato che
	// `RTScreenStack.h` documenta come illegale, `SyncPresentation` esce alla prima riga, e il test
	// asserirebbe su un navigatore che non presenta nulla: la prima stesura faceva cosi' e non poteva
	// vedere ne' il binding mancante ne' il modale disarmato — entrambi trovati in code review.
	Nav->RegisterScreensFromConfig();
	Nav->InitializeFrontend(RTScreenIds::Main);

	Nav->MatchLevel.Reset();   // nessun livello dichiarato: e' il caso in prova

	// ⚠️ **L'`Error` a log e' il comportamento voluto, quindi si DICHIARA.** «Rumoroso» significa proprio
	// questo: un avvio rifiutato lascia una traccia di livello Error, che in automation fa fallire il test
	// se non e' attesa. Senza questa riga il test cadrebbe sul proprio successo — misurato scrivendolo.
	AddExpectedError(TEXT("Avvio partita rifiutato"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedMessage(TEXT("does not have a World"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ -1);

	const ERTNavResult Result = Nav->StartMatch();
	TestEqual(TEXT("l'avvio e' rifiutato"), Result, ERTNavResult::InvalidScreen);
	TestTrue(TEXT("e lo dice con un modale, invece di tornare in silenzio al menu"), Nav->IsModalOpen());

	// 🔴 **Il modale dev'essere ARMATO, non solo aperto.** `GetModalVisibility` restituisce `Collapsed`
	// finche' `IsArmed()` e' falso, quindi un modale presentato ma non armato e' invisibile — e la
	// schermata sotto resta disabilitata: un soft-lock. La prima stesura asseriva su una `FString` privata
	// del navigatore, che nessuno in produzione leggeva.
	URTErrorModalWidgetBase* Modal = Cast<URTErrorModalWidgetBase>(
		Nav->FindLiveWidget(RTScreenIds::ErrorModal));
	if (TestNotNull(TEXT("il modale d'errore e' stato presentato: il binding esiste"), Modal))
	{
		TestTrue(TEXT("ed e' ARMATO, quindi visibile"), Modal->IsArmed());
		TestEqual(TEXT("con l'esito giusto"), Modal->GetOutcome(), ERTStartupOutcome::MatchLevelUnset);
	}

	TestTrue(TEXT("e nessun livello e' stato chiesto"), Nav->ConsumePendingMatchLevel().IsEmpty());

	ReleaseNavigator(GI);
	return true;
}

/**
 * Con un livello dichiarato, `StartMatch` chiede di aprirlo — **una volta sola**.
 *
 * ⚠️ **Il navigatore non apre il livello, lo CHIEDE.** Non ha un mondo (`UGameInstance` senza
 * `WorldContext` nei test, e per costruzione e' un subsystem che non tocca la scena), quindi decide *cosa*
 * e *quando* e lascia l'apertura a chi il mondo ce l'ha. Non e' un secondo percorso di avvio: e' lo stesso
 * percorso in due meta', e la partita resta allestita da `ARTGameMode` col formato spedito da C++.
 *
 * `Consume` e non `Get`: una richiesta letta due volte aprirebbe il livello due volte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendStartMatchRequestsTheLevelOnceTest,
	"RefactorTactics.Frontend.StartMatchRequestsTheLevelOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendStartMatchRequestsTheLevelOnceTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->RegisterScreensFromConfig();
	Nav->InitializeFrontend(RTScreenIds::Main);
	Nav->MatchLevel = TEXT("/Game/RT/Maps/Dev/L_HexArena/L_HexArena");

	TestEqual(TEXT("l'avvio e' accettato"), Nav->StartMatch(), ERTNavResult::Ok);
	TestFalse(TEXT("nessun modale d'errore"), Nav->IsModalOpen());

	const FString First = Nav->ConsumePendingMatchLevel();
	TestEqual(TEXT("il livello chiesto e' quello dichiarato"), First, Nav->MatchLevel);
	TestTrue(TEXT("e la richiesta si consuma: una seconda lettura non riapre nulla"),
		Nav->ConsumePendingMatchLevel().IsEmpty());

	ReleaseNavigator(GI);
	return true;
}

/**
 * Una richiesta mai consumata NON passa in silenzio.
 *
 * 🔴 **E' il costo dichiarato della forma decisione/esecuzione**, e senza questo test resterebbe una
 * dichiarazione: il navigatore chiede l'apertura e qualcun altro la esegue, quindi se l'aggancio non viene
 * collegato `PLAY` non fa nulla — e il difetto vivrebbe nell'ASSENZA di una chiamata, che nessun grep trova
 * e nessun gate vede. La scelta fra le due forme e' stata fatta sapendo questo; la guardia e' cio' che
 * impedisce al costo di diventare silenzioso.
 *
 * Il segnale piu' tempestivo e' un secondo `PLAY`: se la richiesta precedente e' ancora li', nessuno l'ha
 * presa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendUnconsumedMatchRequestIsLoudTest,
	"RefactorTactics.Frontend.UnconsumedMatchRequestIsLoud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendUnconsumedMatchRequestIsLoudTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->RegisterScreensFromConfig();
	Nav->InitializeFrontend(RTScreenIds::Main);
	Nav->MatchLevel = TEXT("/Game/RT/Maps/Dev/L_HexArena/L_HexArena");

	TestEqual(TEXT("il primo avvio passa"), Nav->StartMatch(), ERTNavResult::Ok);

	// Nessuno consuma: e' il caso in prova. L'`Error` e' voluto, quindi si dichiara.
	//
	// ⚠️ Il testo arriva da `DescribeOutcome`, non da una stringa composta qui: e' il punto del canale
	// unico, e infatti la prima stesura attendeva la propria frase e non quella localizzata.
	AddExpectedError(TEXT("Avvio partita rifiutato"), EAutomationExpectedErrorFlags::Contains, 1);

	// Il modale ora si costruisce davvero, quindi `AddToViewport` logga su una GameInstance senza World.
	// Dichiarato come nell'altro file di test del frontend: un rumore DIVERSO deve far fallire.
	AddExpectedMessage(TEXT("does not have a World"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, /*Occurrences=*/ -1);

	TestEqual(TEXT("il secondo avvio e' rifiutato: la richiesta e' ancora li'"),
		Nav->StartMatch(), ERTNavResult::InvalidScreen);
	TestTrue(TEXT("e lo dice con un modale"), Nav->IsModalOpen());
	if (URTErrorModalWidgetBase* Modal2 = Cast<URTErrorModalWidgetBase>(
		Nav->FindLiveWidget(RTScreenIds::ErrorModal)))
	{
		TestEqual(TEXT("e l'esito distingue l'aggancio mancante dalla config assente"),
			Modal2->GetOutcome(), ERTStartupOutcome::MatchRequestNotConsumed);
	}

	// ⚠️ Consumata la richiesta, un nuovo avvio deve tornare possibile: la guardia protegge, non blocca.
	Nav->ConsumePendingMatchLevel();
	Nav->CloseModal();
	TestEqual(TEXT("consumata la richiesta, si riparte"), Nav->StartMatch(), ERTNavResult::Ok);

	Nav->ConsumePendingMatchLevel();   // pulita, per non far scattare la rete in Deinitialize
	ReleaseNavigator(GI);
	return true;
}

/**
 * `StartMatch` ANNUNCIA la richiesta: chi ha il mondo la sente e la consuma.
 *
 * 🔴 **Senza l'annuncio il consumatore non saprebbe QUANDO.** Il navigatore decide, ma chi apre il livello
 * — `ARTFrontendGameMode`, che il mondo ce l'ha — non ha modo di accorgersi che il giocatore ha premuto
 * PLAY: dovrebbe interrogare lo stato a ogni frame, e `CLAUDE.md` vieta il Tick per il sequencing.
 *
 * ⚠️ Il broadcast avviene DOPO che la richiesta e' pendente: un ascoltatore che consuma dentro il callback
 * deve trovarla, non anticiparla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendStartMatchAnnouncesTheRequestTest,
	"RefactorTactics.Frontend.StartMatchAnnouncesTheRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendStartMatchAnnouncesTheRequestTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->RegisterScreensFromConfig();
	Nav->InitializeFrontend(RTScreenIds::Main);
	Nav->MatchLevel = TEXT("/Game/RT/Maps/Dev/L_HexArena/L_HexArena");

	// Un ascoltatore che si comporta come il consumatore vero: sente, consuma, e registra cosa ha visto.
	URTFrontendMatchListenerForTest* Listener = NewObject<URTFrontendMatchListenerForTest>();
	Listener->Nav = Nav;
	Nav->OnMatchRequested.AddUniqueDynamic(Listener, &URTFrontendMatchListenerForTest::OnRequested);

	TestEqual(TEXT("l'avvio passa"), Nav->StartMatch(), ERTNavResult::Ok);

	TestEqual(TEXT("l'ascoltatore e' stato avvisato una volta"), Listener->Calls, 1);
	TestEqual(TEXT("e con il livello dichiarato"), Listener->SeenLevel, Nav->MatchLevel);
	TestTrue(TEXT("la richiesta era gia' pendente quando l'ha sentita: ha potuto consumarla"),
		Listener->bConsumedSomething);
	TestTrue(TEXT("e dopo il consumo non resta nulla"), Nav->ConsumePendingMatchLevel().IsEmpty());

	// ⚠️ Non-vacuita': un secondo PLAY deve poter ripartire, cioe' il consumo ha davvero liberato lo stato.
	TestEqual(TEXT("e si puo' rigiocare"), Nav->StartMatch(), ERTNavResult::Ok);
	Nav->ConsumePendingMatchLevel();

	Nav->OnMatchRequested.RemoveDynamic(Listener, &URTFrontendMatchListenerForTest::OnRequested);
	ReleaseNavigator(GI);
	return true;
}

/**
 * Il consumatore VERO: `ARTFrontendGameMode` sente l'annuncio, consuma la richiesta e apre il livello.
 *
 * 🔴 **E' l'aggancio la cui assenza la guardia di `StartMatch` segnala.** Finche' non esisteva, `PLAY`
 * produceva una richiesta che nessuno raccoglieva: questo test e' cio' che impedisce di tornarci senza
 * accorgersene.
 *
 * ⚠️ **`OpenMatchLevel` e' sostituita, non eseguita**: `UGameplayStatics::OpenLevel` in un test aprirebbe
 * davvero un livello. Il seam sta dove l'apertura avviene, cosi' il resto della catena — iscrizione,
 * consumo, livello passato — resta verificato headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendGameModeConsumesTheRequestTest,
	"RefactorTactics.Frontend.GameModeConsumesTheMatchRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendGameModeConsumesTheRequestTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav))
	{
		RTWorldFixtures::DestroyWorld(World);
		ReleaseNavigator(GI);
		return false;
	}

	Nav->RegisterScreensFromConfig();
	Nav->InitializeFrontend(RTScreenIds::Main);
	Nav->MatchLevel = TEXT("/Game/RT/Maps/Dev/L_HexArena/L_HexArena");

	// 🔴 **Il mondo deve aver inizializzato i suoi actor, o il broadcast non arriva — e non lo dice.**
	// `AActor::ProcessEvent` scarta ogni evento se `GetWorld()->AreActorsInitialized()` è falso, e un
	// delegate **dinamico** invoca proprio da lì. Misurato: senza questa riga il flag è `0` e il GameMode
	// non riceve niente, mentre un `UObject` legato allo stesso delegate riceve — perché passa da
	// `UObject::ProcessEvent`, che quella guardia non ce l'ha.
	//
	// ⚠️ **Non basta `DispatchBeginPlay()` sull'actor**, che era il tentativo naturale e falso: misurato
	// `HasActorBegunPlay=1` insieme ad `AreActorsInitialized=0`. Il flag è del **mondo**, non dell'actor,
	// e le due domande si somigliano abbastanza da far cercare per ore dalla parte sbagliata.
	// ⚠️ **E la `GameInstance` va legata al mondo prima**, o il `GameMode` che ora riceve davvero l'evento
	// si ferma sul suo primo controllo: «questa mappa non ha una GameInstance, e il navigatore vive lì».
	// In produzione la mappa ce l'ha sempre; qui il navigatore nasceva in una GI scollegata dal mondo, ed
	// era una differenza che nessuno vedeva finché il broadcast non arrivava a destinazione.
	World->SetGameInstance(GI);
	World->InitializeActorsForPlay(FURL());

	ARTFrontendGameModeForTest* GameMode = World->SpawnActor<ARTFrontendGameModeForTest>();
	if (!TestNotNull(TEXT("il GameMode del frontend esiste"), GameMode))
	{
		RTWorldFixtures::DestroyWorld(World);
		ReleaseNavigator(GI);
		return false;
	}

	// ⚠️ Un actor spawnato in un mondo di prova non ha ancora ricevuto `BeginPlay`, e un delegate dinamico
	// salta i target che l'engine non considera pronti: gli altri test di questo repository lo fanno per lo
	// stesso motivo.
	if (!GameMode->HasActorBegunPlay()) { GameMode->DispatchBeginPlay(); }
	GameMode->ListenForMatchRequests(Nav);

	TestEqual(TEXT("l'avvio passa"), Nav->StartMatch(), ERTNavResult::Ok);

	if (TestEqual(TEXT("il GameMode ha chiesto UNA apertura"), GameMode->OpenedLevels.Num(), 1))
	{
		TestEqual(TEXT("e del livello dichiarato"), GameMode->OpenedLevels[0], Nav->MatchLevel);
	}
	TestTrue(TEXT("e la richiesta e' stata CONSUMATA: non ne resta traccia"),
		Nav->ConsumePendingMatchLevel().IsEmpty());

	// ⚠️ Non-vacuita': il secondo `PLAY` deve passare, cioe' il consumo ha davvero liberato lo stato — se
	// il GameMode avesse solo letto senza consumare, qui la guardia rifiuterebbe.
	TestEqual(TEXT("e si puo' rigiocare"), Nav->StartMatch(), ERTNavResult::Ok);
	TestEqual(TEXT("con una seconda apertura"), GameMode->OpenedLevels.Num(), 2);

	RTWorldFixtures::DestroyWorld(World);
	ReleaseNavigator(GI);
	return true;
}

/**
 * 🔴 **`BeginPlay` aggancia il consumatore da solo.**
 *
 * E' il difetto che otto test verdi non vedevano: lo chiamavano tutti `ListenForMatchRequests` a mano,
 * quindi provavano che il consumatore *funziona* e mai che qualcuno lo *colleghi*. In PIE `PLAY` scriveva
 * la richiesta, la annunciava a zero ascoltatori, e nulla si apriva — con un sintomo che punta altrove: il
 * `PLAY` successivo veniva rifiutato con «richiesta mai consumata», che accusa chi non consuma invece di
 * chi non si e' mai iscritto. Trovato in code review.
 *
 * ⚠️ **Qui `ListenForMatchRequests` non si chiama**, ed e' tutto il punto: se comparisse, il test
 * tornerebbe a provare il consumatore e il difetto resterebbe invisibile una seconda volta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendBeginPlayHooksTheConsumerTest,
	"RefactorTactics.Frontend.BeginPlayHooksTheMatchConsumer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendBeginPlayHooksTheConsumerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav))
	{
		RTWorldFixtures::DestroyWorld(World);
		ReleaseNavigator(GI);
		return false;
	}

	Nav->RegisterScreensFromConfig();
	Nav->InitializeFrontend(RTScreenIds::Main);
	Nav->MatchLevel = TEXT("/Game/RT/Maps/Dev/L_HexArena/L_HexArena");

	World->SetGameInstance(GI);
	World->InitializeActorsForPlay(FURL());

	ARTFrontendGameModeForTest* GameMode = World->SpawnActor<ARTFrontendGameModeForTest>();
	if (!TestNotNull(TEXT("il GameMode del frontend esiste"), GameMode))
	{
		RTWorldFixtures::DestroyWorld(World);
		ReleaseNavigator(GI);
		return false;
	}

	// L'unico gesto: far partire il ciclo di vita. Nessuna iscrizione a mano.
	if (!GameMode->HasActorBegunPlay()) { GameMode->DispatchBeginPlay(); }

	TestEqual(TEXT("l'avvio passa"), Nav->StartMatch(), ERTNavResult::Ok);
	if (TestEqual(TEXT("il GameMode si era iscritto da solo, e ha aperto"), GameMode->OpenedLevels.Num(), 1))
	{
		TestEqual(TEXT("il livello dichiarato"), GameMode->OpenedLevels[0], Nav->MatchLevel);
	}
	TestTrue(TEXT("e la richiesta non resta pendente"), Nav->ConsumePendingMatchLevel().IsEmpty());

	RTWorldFixtures::DestroyWorld(World);
	ReleaseNavigator(GI);
	return true;
}

// ---------------------------------------------------------------------------------------------------
// CP 46.5 (#940) — la schermata di fine partita, e le due uscite che ne partono.
// ---------------------------------------------------------------------------------------------------

/**
 * `ShowResult` porta il risultato canonico a schermo — e lo scrive **solo se la schermata si apre**.
 *
 * ⚠️ **L'ordine e' l'opposto di `StartMatch`, e non e' una svista.** La' il `PendingMatchLevel` si scrive
 * *prima* del broadcast perche' l'ascoltatore lo legge durante il broadcast. Qui nessuno legge durante il
 * push, quindi la scrittura puo' aspettare l'esito — e deve: un risultato memorizzato mentre la schermata
 * resta chiusa e' uno stato che nessuno mostra e che la partita successiva rileggerebbe come fresco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendShowResultTest,
	"RefactorTactics.Frontend.ShowResultOpensTheScreenAndStoresTheOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendShowResultTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->InitializeFrontend(Main);
	TestFalse(TEXT("prima della partita non c'e' nessun risultato"), Nav->GetMatchResult().bIsFinished);

	FRTMatchResult Canonical;
	Canonical.Outcome = ERTMatchOutcome::Team1Wins;
	Canonical.Reason = ERTMatchEndReason::Elimination;
	FRTMatchState State;
	State.RoundNumber = 9;

	TestEqual(TEXT("il Result si apre"), Nav->ShowResult(Canonical, State), ERTNavResult::Ok);
	TestEqual(TEXT("ed e' la schermata corrente"), Nav->GetCurrentScreen(), RTScreenIds::Result);
	TestTrue(TEXT("il risultato c'e'"), Nav->GetMatchResult().bIsFinished);
	TestEqual(TEXT("con il vincitore canonico"), Nav->GetMatchResult().WinningTeamId, 1);
	TestEqual(TEXT("e i round giocati"), Nav->GetMatchResult().RoundNumber, 9);

	// 🔴 Con un modale aperto il push e' rifiutato — e allora il risultato nuovo si annulla, lasciando in
	// piedi quello di prima. ⚠️ **Senza passare da `ReturnMain`**, che azzera: da li' il confronto
	// misurerebbe l'azzeramento invece del rollback, e i due sono difetti diversi.
	Nav->ShowModal(ConfirmQuit);
	FRTMatchResult Other;
	Other.Outcome = ERTMatchOutcome::Team0Wins;
	FRTMatchState OtherState;
	OtherState.RoundNumber = 4;
	TestEqual(TEXT("bloccato dal modale"), Nav->ShowResult(Other, OtherState), ERTNavResult::BlockedByModal);
	TestEqual(TEXT("e il risultato di prima non e' stato sovrascritto"),
		Nav->GetMatchResult().RoundNumber, 9);

	ReleaseNavigator(GI);
	return true;
}

/**
 * `PLAY AGAIN` ripercorre CP 46.4 — **lo stesso** `StartMatch`, non una seconda via d'avvio.
 *
 * ⚠️ E svuota il risultato prima di ripartire. Un Result stantio che sopravvive alla partita nuova sarebbe
 * la seconda autorita' che il DoD vieta al widget, spostata di un livello: la schermata leggerebbe un dato
 * fresco all'apparenza e vecchio di una partita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendPlayAgainTest,
	"RefactorTactics.Frontend.PlayAgainClearsTheResultAndRequestsAMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendPlayAgainTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->MatchLevel = TEXT("/Game/RT/Maps/Dev/L_Test/L_Test");
	Nav->InitializeFrontend(Main);

	FRTMatchResult Canonical;
	Canonical.Outcome = ERTMatchOutcome::Team0Wins;
	FRTMatchState State;
	State.RoundNumber = 6;
	Nav->ShowResult(Canonical, State);

	TestEqual(TEXT("PLAY AGAIN accettato"), Nav->PlayAgain(), ERTNavResult::Ok);
	TestEqual(TEXT("si riparte dalla radice, non da sopra il Result"), Nav->GetCurrentScreen(), Main);
	TestFalse(TEXT("e senza back stack"), Nav->CanGoBack());
	TestFalse(TEXT("il risultato vecchio e' sparito"), Nav->GetMatchResult().bIsFinished);
	TestEqual(TEXT("il risultato vecchio e' sparito davvero, round azzerati"),
		Nav->GetMatchResult().RoundNumber, 0);
	TestEqual(TEXT("ed e' passato da CP 46.4: la richiesta di livello c'e'"),
		Nav->ConsumePendingMatchLevel(), Nav->MatchLevel);

	ReleaseNavigator(GI);
	return true;
}

/**
 * `MAIN MENU` chiude la partita: dal menu, `Back` non ci rientra.
 *
 * ⚠️ **Non duplica `ReturnMainClearsScreensAndModals` ne' la sonda generativa**, che provano la proprieta'
 * su stack costruiti a mano e su 50 stati casuali. Qui il soggetto e' il percorso reale del DoD — `Result`
 * raggiunto da `ShowResult` — e la domanda e' quella del criterio: dopo `MAIN MENU`, `CanGoBack()` e' falso.
 * Se un domani `ShowResult` cambiasse il modo di impilare, la sonda generativa non se ne accorgerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendMainMenuFromResultTest,
	"RefactorTactics.Frontend.MainMenuFromResultCannotGoBackIntoTheMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendMainMenuFromResultTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->InitializeFrontend(Main);
	FRTMatchResult Canonical;
	Canonical.Outcome = ERTMatchOutcome::Draw;
	Canonical.Reason = ERTMatchEndReason::RoundLimit;
	FRTMatchState State;
	State.RoundNumber = 12;
	Nav->ShowResult(Canonical, State);
	TestTrue(TEXT("dal Result si tornerebbe indietro"), Nav->CanGoBack());

	TestEqual(TEXT("MAIN MENU"), Nav->ReturnMain(), ERTNavResult::Ok);
	TestEqual(TEXT("siamo al menu"), Nav->GetCurrentScreen(), Main);
	TestFalse(TEXT("e il Back non rientra nella partita conclusa"), Nav->CanGoBack());
	TestEqual(TEXT("lo stack e' profondo uno"), Nav->GetDepth(), 1);

	ReleaseNavigator(GI);
	return true;
}

/**
 * 🔴 **`MAIN MENU` non lascia dietro il risultato della partita finita.**
 *
 * L'azzeramento viveva solo in `PlayAgain`, ma il DoD descrive **due** uscite dal Result e `MAIN MENU` e'
 * `ReturnMain` chiamato dal widget. Il percorso che restava scoperto: partita 1 finisce → `MAIN MENU` →
 * `PLAY`, che e' `StartMatch` diretto e non passa da `PlayAgain` → il risultato della partita 1 restava
 * leggibile per tutta la partita 2. Trovato in review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFrontendMainMenuClearsTheResultTest,
	"RefactorTactics.Frontend.MainMenuLeavesNoStaleResultBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFrontendMainMenuClearsTheResultTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTFrontendNavigator* Nav = MakeNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), Nav)) { ReleaseNavigator(GI); return false; }

	Nav->MatchLevel = TEXT("/Game/RT/Maps/Dev/L_Test/L_Test");
	Nav->InitializeFrontend(Main);

	FRTMatchResult First;
	First.Outcome = ERTMatchOutcome::Team0Wins;
	FRTMatchState FirstState;
	FirstState.RoundNumber = 8;
	Nav->ShowResult(First, FirstState);
	TestEqual(TEXT("il risultato della prima partita c'e'"), Nav->GetMatchResult().RoundNumber, 8);

	TestEqual(TEXT("MAIN MENU"), Nav->ReturnMain(), ERTNavResult::Ok);
	TestFalse(TEXT("e il risultato non sopravvive al ritorno al menu"), Nav->GetMatchResult().bIsFinished);
	TestEqual(TEXT("azzerato davvero"), Nav->GetMatchResult().RoundNumber, 0);

	// Il percorso completo: PLAY dal menu e' `StartMatch` diretto, e non deve resuscitare niente.
	TestEqual(TEXT("PLAY riparte"), Nav->StartMatch(), ERTNavResult::Ok);
	TestFalse(TEXT("durante la partita nuova non c'e' un esito vecchio"),
		Nav->GetMatchResult().bIsFinished);

	// ⚠️ La richiesta va consumata: `Deinitialize` segnala come **errore** un avvio mai raccolto, ed e' la
	// guardia di CP 46.4 che fa il suo lavoro — non un rumore da zittire.
	Nav->ConsumePendingMatchLevel();
	ReleaseNavigator(GI);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
