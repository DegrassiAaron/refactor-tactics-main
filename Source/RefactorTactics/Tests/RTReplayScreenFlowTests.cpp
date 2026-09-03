#include "Misc/AutomationTest.h"
#include "Frontend/RTFrontendNavigator.h"
#include "Frontend/RTFrontendScreenIds.h"
#include "Replay/RTReplayScreenWidgets.h"
#include "Replay/RTReplayViewerSubsystem.h"
#include "Tests/RTReplayTestFixtures.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
