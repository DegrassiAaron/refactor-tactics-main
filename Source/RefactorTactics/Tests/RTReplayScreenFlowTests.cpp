#include "Misc/AutomationTest.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "Replay/RTReplayScreenWidgets.h"
#include "Replay/RTReplayViewerSubsystem.h"
#include "Replay/RTReplayPrivacyLibrary.h" // FRTPublicReplayEntry: le voci che il viewer elenca
#include "Debug/RTContextInspector.h"      // DescribePublicEvent: la riga che il viewer mostra per ogni voce
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Tests/RTReplayTestFixtures.h"
#include "Tests/RTWidgetAssetTestHelpers.h" // LoadWidgetClass: il WBP vero, non la classe base
// Solo per avere una `UUserWidget` **concreta** da registrare: `UUserWidget` e' `Abstract` e
// `CreateWidget` la rifiuta. Stessa scelta, e stessa motivazione, di `RTFrontendMainMenuTests.cpp`.
#include "UI/RTScreenHudWidgets.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il flusso delle due schermate del replay (`#472`, R6): `MatchHistory` sceglie, `ReplayViewer` guarda.
 *
 * ⚠️ **Senza widget e senza mondo, ed e' il DoD di #472 alla lettera**: *«posizione, bordi, esiti di
 * apertura e assenza del resolver coperti da test sul view model, senza widget e senza mondo — il confine
 * dell'automatizzabile e' dichiarato qui, non deciso a fine lavoro»*. Qui si aggiunge l'unico pezzo che il
 * view model non puo' conoscere: **il passaggio del dato fra due schermate**.
 *
 * 🔴 **Per questo la logica e' in funzioni STATICHE con le dipendenze in ingresso.** I test costruiscono i
 * widget con `NewObject`, che non da' loro una `GameInstance`: un metodo d'istanza che chiamasse
 * `GetGameInstance()` risponderebbe sempre `nullptr` qui, e la regola finirebbe provata solo in PIE — cioe'
 * non provata. E' la stessa forma di `ARTHUD::ComputePlannedHitMarks` e `BuildVersionLabel`.
 *
 * ⛔ **Cosa questi test NON coprono, e va detto invece che sottinteso**: che le due schermate **esistano**
 * come `.uasset` e che il `.ini` le dichiari. Quello e' lavoro d'editor, e finche' non c'e'
 * `RefactorTactics.Frontend.EveryConfiguredScreenLoads` non ha niente di nuovo da guardare.
 *
 * ⚠️ Dal #3868 un test carica il WBP del viewer (`ViewerAssetHasItsPorts`): le porte opzionali non protestano se mancano.
 */
using namespace RTReplayFixtures;

namespace
{
	/** Un navigatore con le schermate del replay registrate, e la radice aperta. */
	URTFrontendNavigator* MakeReplayNavigator(UGameInstance* GI)
	{
		URTFrontendNavigator* Nav = GI ? GI->GetSubsystem<URTFrontendNavigator>() : nullptr;
		if (!Nav) { return nullptr; }

		// ⚠️ Binding **veri** ma verso una classe qualunque: quello che si prova qui e' il flusso degli id,
		// non quale widget compaia. `PresentWidget` esce alla prima riga su un id senza binding, quindi
		// senza queste tre righe `PushScreen` risponderebbe `Ok` senza che nulla si muova — il fallimento
		// indistinguibile dal successo che `RTFrontendScreenIds.h` nomina.
		TArray<FRTScreenBinding> Bindings;
		for (const FName Id : { RTScreenIds::Main, RTScreenIds::MatchHistory, RTScreenIds::ReplayViewer })
		{
			FRTScreenBinding B;
			B.ScreenId = Id;
			B.WidgetClass = URTScreenHudWidgetBase::StaticClass();
			Bindings.Add(B);
		}
		Nav->StartFrontendFrom(Bindings);
		return Nav;
	}
}

/**
 * **Il `MatchId` attraversa il `PushScreen`, e lo attraversa una volta sola.**
 *
 * 🔴 Il difetto che chiude: `PushScreen` prende un `FName` e basta. Senza un posto dove posare la scelta,
 * il viewer si aprirebbe **senza sapere cosa mostrare** — e `spec-frontend-navigazione.md` §2.2 chiama
 * questa *«l'unica relazione del frontend in cui una schermata ne spinge un'altra portandosi dietro un
 * dato»*.
 *
 * ⛔ **ANTI-VACUITA': si verifica anche che la selezione NON sopravviva.** Un'implementazione che leggesse
 * invece di consumare passerebbe la prima meta' e fallirebbe la seconda: la comparsa successiva del viewer
 * riaprirebbe la partita di prima, che e' un difetto **muto** — nessun errore, la schermata sbagliata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayScreenFlowCarriesMatchIdTest,
	"RefactorTactics.Replay.Screens.MatchIdCrossesThePushAndIsConsumedOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayScreenFlowCarriesMatchIdTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	URTFrontendNavigator* Nav = MakeReplayNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem del replay esiste"), V)
		|| !TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		ReleaseSubsystemHost(GI);
		return false;
	}

	const FGuid Scelta = FGuid::NewGuid();

	// --- La lista sceglie: dichiara e naviga ----------------------------------------------------------
	TestFalse(TEXT("all'inizio non c'e' nessuna selezione"), V->HasSelectedMatch());
	TestTrue(TEXT("la scelta di una partita porta al viewer"),
		URTMatchHistoryWidgetBase::SelectAndNavigate(V, Nav, Scelta));
	TestEqual(TEXT("e la schermata corrente e' il viewer"), Nav->GetCurrentScreen(), RTScreenIds::ReplayViewer);

	// --- Il viewer consuma: la stessa partita, una volta sola ------------------------------------------
	TestTrue(TEXT("il viewer trova una selezione ad attenderlo"), V->HasSelectedMatch());
	const FGuid Consumata = V->ConsumeSelectedMatch();
	TestEqual(TEXT("ed e' ESATTAMENTE quella scelta dalla lista"), Consumata, Scelta);

	// ⛔ La meta' che un'implementazione che «legge» fallirebbe.
	TestFalse(TEXT("dopo il consumo la selezione non c'e' piu'"), V->HasSelectedMatch());
	TestFalse(TEXT("e un secondo consumo non restituisce una partita"),
		V->ConsumeSelectedMatch().IsValid());

	ReleaseSubsystemHost(GI);
	return true;
}

/**
 * **Una navigazione rifiutata non lascia una selezione appesa.**
 *
 * ⚠️ Il caso e' raggiungibile: `PushScreen` risponde `BlockedByModal` con un modale aperto — un errore
 * d'avvio, una conferma — e in quel momento la lista ha gia' dichiarato la scelta. Se restasse li', la
 * **prossima** comparsa del viewer aprirebbe una partita che nessuno ha scelto adesso.
 *
 * 🔑 E' lo stesso difetto di `ConsumePendingMatchLevel` letto invece che consumato, in un altro punto della
 * stessa catena: uno stato pendente che sopravvive al proprio fallimento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayScreenFlowRefusedNavTest,
	"RefactorTactics.Replay.Screens.ARefusedNavigationLeavesNoSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayScreenFlowRefusedNavTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	URTFrontendNavigator* Nav = MakeReplayNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem del replay esiste"), V)
		|| !TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		ReleaseSubsystemHost(GI);
		return false;
	}

	// Un modale aperto blocca la navigazione: e' la condizione, non una simulazione.
	TestEqual(TEXT("si apre un modale"), Nav->ShowModal(RTScreenIds::ErrorModal), ERTNavResult::Ok);

	const FGuid Scelta = FGuid::NewGuid();
	TestFalse(TEXT("la scelta non porta da nessuna parte"),
		URTMatchHistoryWidgetBase::SelectAndNavigate(V, Nav, Scelta));
	TestFalse(TEXT("e NON lascia una selezione appesa"), V->HasSelectedMatch());

	// --- Un `MatchId` invalido non viene nemmeno dichiarato -------------------------------------------
	TestEqual(TEXT("si chiude il modale"), Nav->CloseModal(), ERTNavResult::Ok);
	TestFalse(TEXT("una partita senza id non si apre"),
		URTMatchHistoryWidgetBase::SelectAndNavigate(V, Nav, FGuid()));
	TestFalse(TEXT("e non sporca la selezione"), V->HasSelectedMatch());

	ReleaseSubsystemHost(GI);
	return true;
}

/**
 * **Il viewer si apre con gli occhi di chi ha giocato, e senza selezione lo dice.**
 *
 * ⛔ **Il test non passa mai un `TeamId` né un `MatchId` al viewer**: entrambi arrivano dal flusso — la
 * partita dalla lista, l'osservatore dall'archivio ([D-317]). E' la catena intera di `#472` + `#2156`.
 *
 * ⚠️ **Senza selezione l'esito e' `ManifestUnreadable` e non un quinto valore**: non c'e' un archivio da
 * leggere, ed e' esattamente cio' che quel valore significa. Inventare un `NoSelection` darebbe alla
 * schermata un ramo in piu' da disegnare per una condizione che, se il flow e' corretto, non accade mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayScreenFlowViewerOpensTest,
	"RefactorTactics.Replay.Screens.ViewerOpensTheSelectedMatchAsItsObserver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayScreenFlowViewerOpensTest::RunTest(const FString&)
{
	const FString R = TransientRoot(TEXT("ScreenFlow"));
	Pulisci(R);

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	URTFrontendNavigator* Nav = MakeReplayNavigator(GI);
	if (!TestNotNull(TEXT("il subsystem del replay esiste"), V)
		|| !TestNotNull(TEXT("il navigatore esiste"), Nav))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	V->SetReplaysRoot(R);

	// --- Senza selezione: si dice, non si apre --------------------------------------------------------
	TestEqual(TEXT("senza una partita scelta il viewer non apre niente"),
		URTReplayViewerWidgetBase::OpenSelectedOn(V), ERTReplayOpenResult::ManifestUnreadable);

	// --- Con una partita vera: la catena intera -------------------------------------------------------
	//
	// 🔑 **Si passa DALLA LISTA, e non e' allestimento superfluo.** La prima stesura di questo test spingeva
	// il viewer direttamente dal Main, e `Back` tornava al Main — asserendo il flusso senza averlo
	// costruito. E' il difetto che §2.2 nomina: *«`Back` torna alla lista, non al Main»* e' una proprieta'
	// dello **stack**, non di `BackToListOn`, che si limita a fare `PopScreen`.
	TestEqual(TEXT("si e' nella lista"), Nav->PushScreen(RTScreenIds::MatchHistory), ERTNavResult::Ok);

	const FGuid Id = ArchivioDueTurni(R, /*bChiudi=*/ true, /*bIndicizza=*/ true);
	TestTrue(TEXT("la lista sceglie e naviga"),
		URTMatchHistoryWidgetBase::SelectAndNavigate(V, Nav, Id));
	TestEqual(TEXT("il viewer apre la partita scelta"),
		URTReplayViewerWidgetBase::OpenSelectedOn(V), ERTReplayOpenResult::Opened);
	TestEqual(TEXT("ed e' quella"), V->GetManifest().MatchId, Id);

	// --- `Back`: si torna alla lista E l'archivio si chiude --------------------------------------------
	// ⚠️ Le due meta' insieme: tornare senza chiudere lascerebbe in memoria il TurnLog di ogni turno, per
	// tutta la vita del processo — un subsystem di `GameInstance` sopravvive a ogni caricamento di livello.
	TestTrue(TEXT("si torna indietro"), URTReplayViewerWidgetBase::BackToListOn(V, Nav));
	TestEqual(TEXT("e si e' di nuovo nella lista, non al Main"),
		Nav->GetCurrentScreen(), RTScreenIds::MatchHistory);
	TestFalse(TEXT("l'archivio e' stato chiuso"), V->IsOpen());

	ReleaseSubsystemHost(GI);
	Pulisci(R);
	return true;
}

/**
 * 🔑 **IL VIEWER HA PLAY E PAUSA, E IL TEMPO CHE FA AVANZARE IL REPLAY** (#3868).
 *
 * Il viewer del pacchetto v3 aveva quattro pulsanti di passo e basta: nessun nodo del grafo chiamava `Play`, `Pause` o
 * `Tick` (`grep -a -c CallFunc_Play Content/RT/UI/Framework/WBP_RT_ReplayViewer.uasset` → 0). Due metà, entrambe
 * necessarie: **un pulsante che alterna** e **qualcuno che da' il tempo** — il view model avanza solo se riceve `Tick`.
 *
 *   1. il replay si apre in pausa; `TogglePlayOn` lo avvia e risponde che riproduce; un secondo click lo ferma;
 *   2. senza subsystem non succede niente e non si crasha;
 *   3. l'etichetta dice l'AZIONE del prossimo click: «PLAY» in pausa, «PAUSA» in riproduzione;
 *   4. in pausa il tempo non fa avanzare la posizione; in riproduzione si', di una fase per superamento della soglia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerPlayPauseTest,
	"RefactorTactics.Replay.Screens.ViewerPlayPauseTogglesAndTheTickAdvances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayViewerPlayPauseTest::RunTest(const FString&)
{
	const FString R = TransientRoot(TEXT("ViewerPlay"));
	Pulisci(R);

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem del replay esiste"), V))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	V->SetReplaysRoot(R);
	const FGuid Id = ArchivioDueTurni(R, /*bChiudi=*/ true, /*bIndicizza=*/ true);
	if (!TestEqual(TEXT("premessa: l'archivio si apre"), V->OpenMatchAsRecordedObserver(Id), ERTReplayOpenResult::Opened))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	TestFalse(TEXT("1: il replay si apre in pausa"), V->IsPlaying());

	// --- 1. Il pulsante alterna ----------------------------------------------------------------------------------------
	TestTrue(TEXT("1: il primo click avvia, e la risposta lo dice"), URTReplayViewerWidgetBase::TogglePlayOn(V));
	TestTrue(TEXT("1: e il subsystem riproduce"), V->IsPlaying());
	TestFalse(TEXT("1: il secondo click ferma, e la risposta lo dice"), URTReplayViewerWidgetBase::TogglePlayOn(V));
	TestFalse(TEXT("1: e il subsystem e' in pausa"), V->IsPlaying());

	// --- 2. Senza subsystem ---------------------------------------------------------------------------------------------
	TestFalse(TEXT("2: senza subsystem il click non avvia niente"), URTReplayViewerWidgetBase::TogglePlayOn(nullptr));
	TestFalse(TEXT("2: e il tempo non muove niente"), URTReplayViewerWidgetBase::TickPlaybackOn(nullptr, 1.f));

	// --- 3. L'etichetta dice l'azione del prossimo click ----------------------------------------------------------------
	TestTrue(TEXT("3: in pausa l'etichetta e' PLAY"),
		URTReplayViewerWidgetBase::GetPlayPauseText(false).ToString().Equals(TEXT("PLAY"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("3: in riproduzione e' PAUSA"),
		URTReplayViewerWidgetBase::GetPlayPauseText(true).ToString().Equals(TEXT("PAUSA"), ESearchCase::CaseSensitive));

	// --- 4. Il tempo fa avanzare il replay solo se riproduce ----------------------------------------------------------------
	V->SetSecondsPerPhase(1.f);
	V->StepPhaseForward(); // la prima fase osservabile: un punto da cui si possa avanzare ancora
	const FRTReplayPosition Ferma = V->GetPosition();
	TestFalse(TEXT("4: in pausa il tempo non muove la posizione"), URTReplayViewerWidgetBase::TickPlaybackOn(V, 5.f));
	TestTrue(TEXT("4: e la posizione e' quella di prima"),
		V->GetPosition().TurnNumber == Ferma.TurnNumber && V->GetPosition().Phase == Ferma.Phase && V->GetPosition().State == Ferma.State);

	TestTrue(TEXT("4: Play avvia"), URTReplayViewerWidgetBase::TogglePlayOn(V));
	TestFalse(TEXT("4: sotto la soglia di una fase la posizione resta"), URTReplayViewerWidgetBase::TickPlaybackOn(V, 0.4f));
	TestTrue(TEXT("4: oltre la soglia si avanza di una fase"), URTReplayViewerWidgetBase::TickPlaybackOn(V, 0.7f));
	TestTrue(TEXT("4: e la posizione e' cambiata"),
		V->GetPosition().TurnNumber != Ferma.TurnNumber || V->GetPosition().Phase != Ferma.Phase);

	ReleaseSubsystemHost(GI);
	Pulisci(R);
	return true;
}

/**
 * 🔑 **IL VIEWER MOSTRA CIO' CHE E' ACCADUTO NELLA FASE, E L'ETICHETTA DEL PULSANTE INDIETRO** (#3868).
 *
 * `GetCurrentPhaseEntries` esisteva ma nessuno la chiamava (`CallFunc`: 0 nel WBP). Le voci sono **pubbliche** — passano dal
 * filtro del replay —, e la riga e' quella di `DescribePublicEvent`, la stessa dell'ispettore: colonne, non una seconda
 * traduzione in italiano dello stesso fatto.
 *
 *   1. prima di avere una fase le voci non ci sono e il testo lo dice;
 *   2. a una fase con voci il testo e' esattamente le righe di `DescribePublicEvent` di ognuna, nell'ordine;
 *   3. cambiando fase il testo cambia — la rilettura segue la posizione;
 *   4. riaprendo l'archivio alla STESSA posizione il testo non e' quello di prima per caso: la rilettura riparte;
 *   5. cambiando SOLO la fase, dentro lo stesso turno, la rilettura riparte (le gambe 3 e 4 cambiano anche il turno);
 *   6. le etichette: «PLAY» in pausa, «PAUSA» in riproduzione, e il pulsante indietro ha la sua;
 *   7. una fase con molte voci non fa uscire il testo dallo schermo: dodici righe e «… e altre N».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerShowsPhaseEntriesTest,
	"RefactorTactics.Replay.Screens.ViewerShowsThePhaseEntriesAndLabelsItsButtons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayViewerShowsPhaseEntriesTest::RunTest(const FString&)
{
	const FString R = TransientRoot(TEXT("ViewerEntries"));
	Pulisci(R);

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem del replay esiste"), V))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	V->SetReplaysRoot(R);
	const FGuid Id = ArchivioDueTurni(R, /*bChiudi=*/ true, /*bIndicizza=*/ true);
	if (!TestEqual(TEXT("premessa: l'archivio si apre"), V->OpenMatchAsRecordedObserver(Id), ERTReplayOpenResult::Opened))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}

	URTReplayViewerWidgetBase* W = NewObject<URTReplayViewerWidgetBase>(GetTransientPackage());
	if (!TestNotNull(TEXT("il widget"), W))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	W->PlayPauseLabel = NewObject<UTextBlock>(W);
	W->PhaseEntriesText = NewObject<UTextBlock>(W);
	W->BackButtonLabel = NewObject<UTextBlock>(W);
	const auto Testo = [](const UTextBlock* B) { return B->GetText().ToString(); };

	// --- 1. Prima di una fase non ci sono voci, e il testo lo dice --------------------------------------------------------
	W->RefreshLookOn(V);
	TestTrue(TEXT("1: in pausa l'etichetta e' PLAY"), Testo(W->PlayPauseLabel).Equals(TEXT("PLAY"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("1: il pulsante indietro ha la sua etichetta"), Testo(W->BackButtonLabel).Equals(TEXT("INDIETRO"), ESearchCase::CaseSensitive));
	TestFalse(TEXT("1: il testo delle voci non e' vuoto: dice che non ce ne sono"), Testo(W->PhaseEntriesText).IsEmpty());

	// --- 2. A una fase con voci: esattamente le righe di DescribePublicEvent -----------------------------------------------
	V->StepPhaseForward();
	const TArray<FRTPublicReplayEntry> Voci = V->GetCurrentPhaseEntries();
	if (!TestTrue(TEXT("premessa: la prima fase ha delle voci"), Voci.Num() > 0))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	W->RefreshLookOn(V);
	TArray<FString> Attese;
	for (const FRTPublicReplayEntry& Voce : Voci) { Attese.Add(URTContextInspectorLibrary::DescribePublicEvent(Voce)); }
	TestEqual(TEXT("2: il testo e' una riga per voce, come DescribePublicEvent"),
		Testo(W->PhaseEntriesText), FString::Join(Attese, TEXT("\n")));

	// --- 3. Cambiando fase il testo segue la posizione ----------------------------------------------------------------------
	V->StepTurnForward();
	const FRTReplayPosition PosizioneDiPrima = V->GetPosition();
	W->RefreshLookOn(V);
	TArray<FString> AtteseDopo;
	for (const FRTPublicReplayEntry& Voce : V->GetCurrentPhaseEntries()) { AtteseDopo.Add(URTContextInspectorLibrary::DescribePublicEvent(Voce)); }
	TestEqual(TEXT("3: dopo un passo il testo e' quello della fase nuova"),
		Testo(W->PhaseEntriesText), FString::Join(AtteseDopo, TEXT("\n")));

	// --- 4. Riaprendo alla stessa posizione la rilettura riparte -------------------------------------------------------------
	// Un secondo archivio, con una traccia diversa, aperto e portato alla STESSA posizione (turno, fase): la chiave della
	// rilettura e' uguale a quella di prima, e solo un'invalidazione all'apertura impedisce di mostrare le voci vecchie.
	TArray<TArray<FRTTurnLogEntry>> Tracce;
	Tracce.Add(Traccia(1, { ERTMatchPhase::Blast, ERTMatchPhase::Blast, ERTMatchPhase::Blast, ERTMatchPhase::Move }));
	Tracce.Add(Traccia(2, { ERTMatchPhase::Blast, ERTMatchPhase::Blast, ERTMatchPhase::Blast, ERTMatchPhase::Move }));
	const FGuid Altro = ScriviArchivio(R, Tracce, /*bChiudi=*/ true, /*bIndicizza=*/ true);
	V->Close();
	V->SelectMatch(Altro);
	if (!TestEqual(TEXT("premessa: il secondo archivio si apre"), W->OpenSelectedWith(V), ERTReplayOpenResult::Opened))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	V->StepPhaseForward();
	V->StepTurnForward(); // la stessa posizione di prima: (turno 2, prima fase)
	TestTrue(TEXT("premessa: e' proprio la posizione di prima"),
		V->GetPosition().TurnNumber == PosizioneDiPrima.TurnNumber && V->GetPosition().Phase == PosizioneDiPrima.Phase);
	W->RefreshLookOn(V);
	TArray<FString> AtteseAltro;
	for (const FRTPublicReplayEntry& Voce : V->GetCurrentPhaseEntries()) { AtteseAltro.Add(URTContextInspectorLibrary::DescribePublicEvent(Voce)); }
	TestEqual(TEXT("4: aperto un altro archivio alla stessa posizione, il testo e' quello dell'archivio nuovo"),
		Testo(W->PhaseEntriesText), FString::Join(AtteseAltro, TEXT("\n")));

	// --- 5. Cambiando SOLO la fase, dentro lo stesso turno, la rilettura riparte ---------------------------------------------
	// La chiave e' (stato, turno, fase): se ignorasse la fase, due fasi dello stesso turno mostrerebbero il testo della prima.
	// Le gambe 3 e 4 cambiano anche il turno, quindi la chiave cambia comunque e non distinguono questo difetto.
	const FString TestoPrimaDiFase = Testo(W->PhaseEntriesText);
	const FRTReplayPosition PosizionePrimaDiFase = V->GetPosition();
	if (!TestTrue(TEXT("premessa: si avanza di una fase"), V->StepPhaseForward())
		|| !TestTrue(TEXT("premessa: il turno e' lo stesso e la fase e' un'altra"),
			V->GetPosition().TurnNumber == PosizionePrimaDiFase.TurnNumber && V->GetPosition().Phase != PosizionePrimaDiFase.Phase))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	W->RefreshLookOn(V);
	TArray<FString> AtteseFase;
	for (const FRTPublicReplayEntry& Voce : V->GetCurrentPhaseEntries()) { AtteseFase.Add(URTContextInspectorLibrary::DescribePublicEvent(Voce)); }
	const FString TestoAtteso = FString::Join(AtteseFase, TEXT("\n"));
	TestNotEqual(TEXT("premessa: le due fasi non hanno lo stesso testo, altrimenti la gamba non distingue"), TestoAtteso, TestoPrimaDiFase);
	TestEqual(TEXT("5: cambiata solo la fase, il testo e' quello della fase nuova"), Testo(W->PhaseEntriesText), TestoAtteso);

	// --- 6. In riproduzione l'etichetta cambia ---------------------------------------------------------------------------------
	V->Play();
	W->RefreshLookOn(V);
	TestTrue(TEXT("6: in riproduzione l'etichetta e' PAUSA"), Testo(W->PlayPauseLabel).Equals(TEXT("PAUSA"), ESearchCase::CaseSensitive));

	// --- 7. Il tetto delle righe ---------------------------------------------------------------------------------------------------
	TArray<FRTPublicReplayEntry> Molte;
	Molte.SetNum(15);
	TArray<FString> Righe;
	URTReplayViewerWidgetBase::ComposePhaseEntriesText(Molte).ToString().ParseIntoArrayLines(Righe, /*bCullEmpty=*/ false);
	TestEqual(TEXT("7: dodici righe di voci piu' quella che conta le altre"), Righe.Num(), 13);
	if (Righe.Num() == 13)
	{
		TestTrue(TEXT("7: l'ultima dice quante altre"), Righe[12].EndsWith(TEXT("e altre 3"), ESearchCase::CaseSensitive));
	}
	TestFalse(TEXT("7: con dodici voci il testo non dice «altre»"),
		URTReplayViewerWidgetBase::ComposePhaseEntriesText(TArray<FRTPublicReplayEntry>(Molte.GetData(), 12)).ToString().Contains(TEXT("altre")));

	ReleaseSubsystemHost(GI);
	Pulisci(R);
	return true;
}

/**
 * 🔑 **IL `.uasset` DEL VIEWER HA LE QUATTRO PORTE, E IL C++ SCRIVE NEI SUOI WIDGET** (#3868).
 *
 * ⚠️ Le porte sono `BindWidgetOptional`: se nel WBP manca un widget, ne ha uno con un altro nome o un altro tipo, il
 * widget compila, il membro resta `nullptr` e nessuno protesta. Il difetto di #3868 (nessun Play, nessuna etichetta, nessuna
 * voce) tornerebbe identico con tutti gli altri test verdi, perche' provano il C++ su `UTextBlock` creati a mano. Qui si
 * istanzia il Blueprint vero, `Initialize` risolve i `BindWidget`, e si guarda cosa scrive il C++ in quei widget:
 *
 *   1. le quattro porte sono risolte;
 *   2. le due etichette stanno dentro il loro pulsante (un'etichetta fuori dal pulsante e' un testo orfano);
 *   3. nessuna e' nascosta: un `Collapsed` renderebbe invisibile tutto cio' che il C++ scrive;
 *   4. `RefreshLookOn` su un replay aperto scrive PLAY, INDIETRO e le voci della fase nei widget dell'ASSET, e con il replay
 *      in riproduzione l'etichetta del pulsante diventa PAUSA.
 *
 * ⛔ Non copre il click (`NativeConstruct` lega `OnClicked` solo con il widget in un viewport) ne' l'aspetto: restano PIE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayViewerAssetHasItsPortsTest,
	"RefactorTactics.Replay.Screens.ViewerAssetHasItsPorts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayViewerAssetHasItsPortsTest::RunTest(const FString&)
{
	UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(
		TEXT("/Game/RT/UI/Framework/WBP_RT_ReplayViewer.WBP_RT_ReplayViewer_C"));
	if (!TestNotNull(TEXT("WBP_RT_ReplayViewer si carica"), Class))
	{
		return false;
	}
	URTReplayViewerWidgetBase* W = Cast<URTReplayViewerWidgetBase>(NewObject<UUserWidget>(GetTransientPackage(), Class));
	if (!TestNotNull(TEXT("la classe del WBP deriva da URTReplayViewerWidgetBase"), W))
	{
		return false;
	}
	W->Initialize(); // duplica l'albero e risolve i BindWidget: senza, i membri restano nullptr

	// --- 1. Le quattro porte sono risolte -------------------------------------------------------------------------------------
	const TArray<TPair<const TCHAR*, UWidget*>> Porte = {
		{ TEXT("PlayPauseButton"), W->PlayPauseButton },
		{ TEXT("PlayPauseLabel"), W->PlayPauseLabel },
		{ TEXT("PhaseEntriesText"), W->PhaseEntriesText },
		{ TEXT("BackButtonLabel"), W->BackButtonLabel },
	};
	bool bTutte = true;
	for (const TPair<const TCHAR*, UWidget*>& Porta : Porte)
	{
		bTutte &= TestNotNull(*FString::Printf(TEXT("1: la porta %s e' risolta"), Porta.Key), Porta.Value);
	}
	if (!bTutte)
	{
		return false;
	}

	// --- 2. Le etichette stanno nel loro pulsante ------------------------------------------------------------------------------
	TestTrue(TEXT("2: PlayPauseLabel sta dentro PlayPauseButton"), W->PlayPauseLabel->GetParent() == W->PlayPauseButton.Get());
	const UWidget* Indietro = W->BackButtonLabel->GetParent();
	TestTrue(TEXT("2: BackButtonLabel sta dentro un pulsante"), Indietro != nullptr && Indietro->IsA<UButton>());
	TestEqual(TEXT("2: ed e' il pulsante BackButton"), Indietro ? Indietro->GetFName() : FName(NAME_None), FName(TEXT("BackButton")));

	// --- 3. Nessuna e' nascosta -----------------------------------------------------------------------------------------------
	for (const TPair<const TCHAR*, UWidget*>& Porta : Porte)
	{
		const ESlateVisibility Vis = Porta.Value->GetVisibility();
		TestTrue(*FString::Printf(TEXT("3: %s non e' nascosta"), Porta.Key),
			Vis != ESlateVisibility::Collapsed && Vis != ESlateVisibility::Hidden);
	}

	// --- 4. Il C++ scrive nei widget dell'asset -------------------------------------------------------------------------------
	const FString R = TransientRoot(TEXT("ViewerAsset"));
	Pulisci(R);
	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem del replay esiste"), V))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	V->SetReplaysRoot(R);
	const FGuid Id = ArchivioDueTurni(R, /*bChiudi=*/ true, /*bIndicizza=*/ true);
	if (!TestEqual(TEXT("premessa: l'archivio si apre"), V->OpenMatchAsRecordedObserver(Id), ERTReplayOpenResult::Opened))
	{
		ReleaseSubsystemHost(GI);
		Pulisci(R);
		return false;
	}
	V->StepPhaseForward();
	W->RefreshLookOn(V);

	const auto Testo = [](const UTextBlock* B) { return B->GetText().ToString(); };
	TestTrue(TEXT("4: in pausa l'etichetta del pulsante e' PLAY"), Testo(W->PlayPauseLabel).Equals(TEXT("PLAY"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("4: il pulsante indietro ha la sua etichetta"), Testo(W->BackButtonLabel).Equals(TEXT("INDIETRO"), ESearchCase::CaseSensitive));
	const FString VociAttese = URTReplayViewerWidgetBase::ComposePhaseEntriesText(V->GetCurrentPhaseEntries()).ToString();
	TestFalse(TEXT("premessa: la fase ha delle voci da mostrare"), VociAttese.IsEmpty());
	TestEqual(TEXT("4: il testo delle voci nel widget dell'asset e' quello della fase"), Testo(W->PhaseEntriesText), VociAttese);

	V->Play();
	W->RefreshLookOn(V);
	TestTrue(TEXT("4: in riproduzione l'etichetta del pulsante e' PAUSA"), Testo(W->PlayPauseLabel).Equals(TEXT("PAUSA"), ESearchCase::CaseSensitive));

	ReleaseSubsystemHost(GI);
	Pulisci(R);
	return true;
}

/**
 * **«Nessun replay» e «non ho potuto leggere» sono due esiti diversi.**
 *
 * ⚠️ Un `TArray` vuoto da solo non li distingue, e una schermata che li confondesse direbbe *«non hai
 * ancora giocato»* a chi ha l'indice illeggibile — la stessa classe di difetto che
 * `URTReplayRecorderLibrary::DefaultReplaysRoot` cita: *«elencherebbe una cartella vuota su una macchina
 * piena di registrazioni»*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayScreenFlowEmptyVsFailedTest,
	"RefactorTactics.Replay.Screens.EmptyListAndUnreadableIndexAreDistinct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayScreenFlowEmptyVsFailedTest::RunTest(const FString&)
{
	const FString R = TransientRoot(TEXT("ScreenFlowEmpty"));
	Pulisci(R);

	UGameInstance* GI = nullptr;
	URTReplayViewerSubsystem* V = MakeSubsystemHost<URTReplayViewerSubsystem>(GI);
	if (!TestNotNull(TEXT("il subsystem esiste"), V)) { ReleaseSubsystemHost(GI); Pulisci(R); return false; }
	V->SetReplaysRoot(R);

	// --- Nessun indice su disco: e' una LETTURA FALLITA, non una lista vuota --------------------------
	bool bFallita = false;
	TArray<FRTMatchHistoryEntry> Nulla = URTMatchHistoryWidgetBase::LoadMatchesFrom(V, bFallita);
	TestEqual(TEXT("non si elenca niente"), Nulla.Num(), 0);
	TestTrue(TEXT("e lo si dichiara come lettura fallita, non come «nessuna partita»"), bFallita);

	// --- Con un indice vero: si legge, e non e' un fallimento ------------------------------------------
	ArchivioDueTurni(R, /*bChiudi=*/ true, /*bIndicizza=*/ true);
	bool bFallita2 = true; // sporca l'uscita: il valore deve venire dalla lettura
	TArray<FRTMatchHistoryEntry> Righe = URTMatchHistoryWidgetBase::LoadMatchesFrom(V, bFallita2);
	TestFalse(TEXT("la lettura riesce"), bFallita2);
	TestEqual(TEXT("e c'e' una partita da elencare"), Righe.Num(), 1);

	// --- Senza subsystem: fallimento, e non una lista vuota -------------------------------------------
	bool bFallita3 = false;
	URTMatchHistoryWidgetBase::LoadMatchesFrom(nullptr, bFallita3);
	TestTrue(TEXT("senza subsystem e' un fallimento di lettura"), bFallita3);

	ReleaseSubsystemHost(GI);
	Pulisci(R);
	return true;
}


/**
 * L'etichetta di una riga dice **come e' finita**, e distingue l'archivio incompleto — `#2379`.
 *
 * 🔑 **Il ramo che conta e' `bReplayComplete`**: una partita interrotta si apre e finisce prima di dove il
 * suo `TurnCount` promette, e questa riga e' l'unico punto in cui chi guarda lo sa **senza aprirla** —
 * l'indice per design non guarda dentro le cartelle.
 *
 * ⚠️ Le asserzioni sono su `Contains` e non sul testo intero: il formato di data e ora dipende dalla
 * localizzazione della macchina, e pinnarlo renderebbe il test rosso su un'altra impostazione senza che
 * nulla sia rotto. Cio' che si verifica e' che ogni ramo dica **la sua** parola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchHistoryEntryLabelTest,
	"RefactorTactics.Replay.Screens.EntryLabelNamesTheOutcomeAndTheIncompleteArchive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchHistoryEntryLabelTest::RunTest(const FString&)
{
	FRTMatchHistoryEntry Voce;
	Voce.FormatId = TEXT("Format.Skirmish2v2");
	Voce.TurnCount = 12;
	Voce.bReplayComplete = true;

	Voce.Outcome = ERTMatchOutcome::Team0Wins;
	const FString Vittoria = URTMatchHistoryWidgetBase::GetEntryLabel(Voce).ToString();
	TestTrue(TEXT("la vittoria della squadra 0 si legge"), Vittoria.Contains(TEXT("squadra 0")));
	TestTrue(TEXT("il formato si legge"), Vittoria.Contains(TEXT("Skirmish2v2")));
	TestTrue(TEXT("i turni si leggono"), Vittoria.Contains(TEXT("12")));
	TestFalse(TEXT("e un archivio completo non dice «incompleta»"), Vittoria.Contains(TEXT("incompleta")));

	Voce.Outcome = ERTMatchOutcome::Draw;
	TestTrue(TEXT("il pareggio si legge"),
		URTMatchHistoryWidgetBase::GetEntryLabel(Voce).ToString().Contains(TEXT("pareggio")));

	// `InProgress` in un archivio chiuso non e' uno stato: e' una partita che il registratore non ha visto
	// finire.
	Voce.Outcome = ERTMatchOutcome::InProgress;
	TestTrue(TEXT("una partita non finita si dichiara interrotta"),
		URTMatchHistoryWidgetBase::GetEntryLabel(Voce).ToString().Contains(TEXT("interrotta")));

	// 🔑 Il ramo dell'archivio incompleto e' indipendente dall'esito: si somma, non sostituisce.
	Voce.Outcome = ERTMatchOutcome::Team1Wins;
	Voce.bReplayComplete = false;
	const FString Incompleta = URTMatchHistoryWidgetBase::GetEntryLabel(Voce).ToString();
	TestTrue(TEXT("l'archivio incompleto si dichiara"), Incompleta.Contains(TEXT("incompleta")));
	TestTrue(TEXT("e l'esito resta leggibile accanto"), Incompleta.Contains(TEXT("squadra 1")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
