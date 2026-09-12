#include "Misc/AutomationTest.h"

#include "Editor.h"
#include "RTLauncherScenarioBrowser.h"
#include "RTScenarioPreviewSubsystem.h"
#include "SRTLauncherScenarioPanel.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioKnowledge.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La **SEQUENZA** del pannello del Tactical Designer (#3074) — cioe' l'unica cosa che, di questa schermata,
 * cinque difetti su cinque hanno usato per nascondersi.
 *
 * 🔑 **Cosa questi due test aggiungono a quelli che c'erano gia'.** `RTLauncherScenarioBrowserTests.cpp`
 * copre le funzioni pure — quali id restano, che frase esce da quali fatti — e `RTLauncherLayoutTests` il
 * layout. Nessuno dei due attraversa `SRTLauncherScenarioPanel`, e i difetti di #2788 e #2836 stavano tutti
 * **li'**: `OnRunScenarioClicked` che chiamava `RefreshReadout()` in fondo e richiudeva il playback appena
 * aperto (2026-09-04, trovato pilotando l'Editor via MCP), `ClearSelection()` che non dimenticava la corsa e
 * non chiudeva l'anteprima (2026-09-12, trovato leggendo). Ordine di chiamate, non regole: un test che
 * compone i fatti a mano e interroga `DescribeTransport` li vede tutti e due verdi.
 *
 * ⛔ **Cosa NON coprono, e resta la seduta `U44`.** Che il playback si **capisca** — che scorrendolo si veda
 * *perche'* lo scenario finisce cosi' — e' giudizio umano e non si automatizza. Qui si misura che il gesto
 * produca lo stato e la parola giusti; `PIE-SCEN-PLAYBACK` tiene il resto.
 *
 * ⚠️ **Headless e' possibile, ed e' stato misurato prima di scriverli** (`DEC-1` di #3074): un
 * `SNew(SRTLauncherScenarioPanel)` in un test `EditorContext` con `-nullrhi` costruisce l'albero intero
 * senza `FSlateApplication::InitializeAsStandaloneApplication`, e `SButton::SimulateClick()` esegue il
 * delegato legato. L'intestazione di `RTLauncherLayoutTests.cpp` dichiara testabili gli oggetti puri del
 * layout *«senza una finestra»*: questo file dice che, per costruire e pilotare, nemmeno un
 * `SCompoundWidget` ne ha bisogno.
 */
namespace
{
	/**
	 * Ripulisce cio' che questi test sporcano: l'anteprima **e** la corsa promossa al paragone.
	 *
	 * 🔴 **La seconda meta' non e' prudenza.** `ClearPreview()` non butta la corsa uscente, la *promuove* a
	 * termine di confronto — e deve farlo, o il paragone del pannello morirebbe a ogni corsa. Ma il
	 * subsystem vive quanto l'editor e nessuna via di produzione lo azzera, quindi un test che apre un
	 * playback la lascia accesa per tutti quelli dopo di lui. Misurato il 2026-09-12: questi due test
	 * precedono `PlaybackComparesWithThePreviousRun` in ordine alfabetico e lo facevano diventare rosso
	 * sulla sua asserzione anti-vacuita', senza toccarne una riga.
	 */
	void LauncherPanelReset(URTScenarioPreviewSubsystem& Preview)
	{
		Preview.ClearPreview();
		Preview.TestForgetPreviousRun();
	}

	void LauncherPanelCollect(const TSharedRef<SWidget>& Root, TArray<TSharedRef<SWidget>>& Out)
	{
		Out.Add(Root);
		if (FChildren* Children = Root->GetChildren())
		{
			for (int32 i = 0; i < Children->Num(); ++i)
			{
				LauncherPanelCollect(Children->GetChildAt(i), Out);
			}
		}
	}

	/** Il pulsante la cui etichetta e' `Label`. Il testo sta in uno `STextBlock` DENTRO il bottone. */
	TSharedPtr<SButton> LauncherPanelButton(const TSharedRef<SWidget>& Root, const FString& Label)
	{
		TArray<TSharedRef<SWidget>> All;
		LauncherPanelCollect(Root, All);
		for (const TSharedRef<SWidget>& W : All)
		{
			if (W->GetType() != FName(TEXT("SButton"))) { continue; }

			TArray<TSharedRef<SWidget>> Inner;
			LauncherPanelCollect(W, Inner);
			for (const TSharedRef<SWidget>& I : Inner)
			{
				if (I->GetType() == FName(TEXT("STextBlock"))
					&& StaticCastSharedRef<STextBlock>(I)->GetText().ToString() == Label)
				{
					return StaticCastSharedRef<SButton>(W);
				}
			}
		}
		return nullptr;
	}

	/**
	 * La riga di stato del trasporto, trovata **per quello che dice** su un pannello appena costruito.
	 *
	 * ⚠️ **Non per posizione nell'albero**, che cambierebbe al primo slot aggiunto. Un pannello senza
	 * selezione porta la frase di `FRTLauncherTransportStatus` allo stato di default, e `OutQuante` serve a
	 * dire se e' unica: se domani due punti dello schermo portassero la stessa frase, questo helper
	 * sceglierebbe in silenzio, e i test devono rifiutarsi invece di misurare il widget sbagliato.
	 */
	TSharedPtr<STextBlock> LauncherPanelTransportLine(const TSharedRef<SWidget>& Root, int32& OutQuante)
	{
		const FString Attesa = FRTLauncherScenarioBrowser::DescribeTransport(FRTLauncherTransportStatus()).ToString();

		TArray<TSharedRef<SWidget>> All;
		LauncherPanelCollect(Root, All);

		TSharedPtr<STextBlock> Trovata;
		OutQuante = 0;
		for (const TSharedRef<SWidget>& W : All)
		{
			if (W->GetType() != FName(TEXT("STextBlock"))) { continue; }

			const TSharedRef<STextBlock> Testo = StaticCastSharedRef<STextBlock>(W);
			if (Testo->GetText().ToString() == Attesa)
			{
				++OutQuante;
				if (!Trovata.IsValid()) { Trovata = Testo; }
			}
		}
		return Trovata;
	}

	/**
	 * Il primo id del corpus con almeno **due** squadre.
	 *
	 * ⛔ Non un id scritto qui: legherebbe i test a uno scenario che qualcuno puo' rinominare, e la
	 * condizione che serve non e' *quello* scenario — e' che la corsa abbia qualcosa da giocare. A una
	 * squadra sola non c'e' nessun avversario, la corsa puo' chiudersi a zero turni e il playback non si
	 * apre: il test misurerebbe il rifiuto invece della sequenza.
	 */
	FString LauncherPanelTwoTeamScenario()
	{
		TStrongObjectPtr<URTScenarioAuthoring> Authoring(URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage()));
		if (!Authoring.IsValid()) { return FString(); }

		for (const FString& Id : URTScenarioAuthoring::ListScenarioIds(FString(), FString()))
		{
			FString OpenError;
			if (Authoring->OpenById(Id, OpenError) != ERTScenarioAuthoringResult::Success) { continue; }

			const bool bDueSquadre = RTScenarioKnowledge::TeamIds(Authoring->ListUnits()).Num() >= 2;
			Authoring->Close();
			if (bDueSquadre) { return Id; }
		}
		return FString();
	}
}

/**
 * **`Esegui` cambia la parola sullo schermo**, e il playback resta APERTO dietro di lei.
 *
 * 🔴 **E' il difetto del 2026-09-04 messo sotto misura.** `OnRunScenarioClicked` chiamava `RefreshReadout()`
 * in fondo «per aggiornare il referto»: quella funzione rifa' `ShowScenario`, che comincia con
 * `ClearPreview()` — il playback si apriva e veniva chiuso una riga dopo. I controlli restavano spenti e la
 * riga continuava a dire *«Nessun playback»* dopo una corsa riuscita. La suite era verde, perche' i test
 * chiamavano `OpenPlayback` direttamente e la sequenza non passava di li'.
 *
 * ⚠️ **L'oracolo che morde e' `IsPlaybackOpen()`, non il testo.** Il testo lo compone `DescribeTransport`
 * dai fatti che trova: con il playback richiuso sarebbe *coerente con i fatti sbagliati*, quindi
 * confrontarlo con un'attesa costruita dagli stessi fatti passerebbe comunque. L'apertura del playback e la
 * disuguaglianza col testo di partenza sono le due asserzioni che il difetto non puo' soddisfare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherPanelRunChangesTransportTest,
	"RefactorTactics.DevSandboxLauncher.PanelRunButtonChangesTheTransportLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherPanelRunChangesTransportTest::RunTest(const FString&)
{
	URTScenarioPreviewSubsystem* Preview = GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("il subsystem d'anteprima esiste"), Preview)) { return false; }
	LauncherPanelReset(*Preview);

	const FString Id = LauncherPanelTwoTeamScenario();
	if (!TestFalse(TEXT("il corpus offre uno scenario a due squadre"), Id.IsEmpty())) { return false; }

	const TSharedRef<SRTLauncherScenarioPanel> Panel = SNew(SRTLauncherScenarioPanel);

	int32 Quante = 0;
	const TSharedPtr<STextBlock> Riga = LauncherPanelTransportLine(Panel, Quante);
	if (!TestEqual(TEXT("una sola riga di trasporto nell'albero"), Quante, 1)) { return false; }

	const FString Prima = Riga->GetText().ToString();

	Panel->TestApplySelection(MakeShared<FString>(Id));

	const TSharedPtr<SButton> Esegui = LauncherPanelButton(Panel, TEXT("Esegui"));
	if (!TestTrue(TEXT("il pulsante Esegui e' nell'albero"), Esegui.IsValid()))
	{
		LauncherPanelReset(*Preview);
		return false;
	}

	Esegui->SimulateClick();

	// --- Cio' che il difetto del 2026-09-04 non poteva produrre -------------------------------------
	TestTrue(TEXT("dopo Esegui il playback e' APERTO"), Preview->IsPlaybackOpen());

	const SRTLauncherScenarioPanel::FTestRunProbe Corsa = Panel->TestRunProbe();
	TestEqual(TEXT("il pannello ricorda una corsa avvenuta"),
		static_cast<int32>(Corsa.RunState), static_cast<int32>(ERTLauncherRunState::Ran));
	TestTrue(TEXT("con almeno un turno giocato"), Corsa.RunTurns > 0);

	// --- E la parola sullo schermo -------------------------------------------------------------------
	const FString Dopo = Riga->GetText().ToString();
	TestNotEqual(TEXT("la riga di trasporto e' CAMBIATA"), Dopo, Prima);

	// Composta dai fatti veri — il referto del pannello, l'apertura e la posizione del sottosistema — e non
	// da una frase scritta qui: cosi' un cambio di wording non rende rosso un test sulla sequenza.
	FRTLauncherTransportStatus Atteso;
	Atteso.Run = Corsa.RunState;
	Atteso.TurnsPlayed = Corsa.RunTurns;
	Atteso.bHasTrace = Corsa.bHasTrace;
	Atteso.bScenarioSelected = true;
	Atteso.bPlaybackOpen = Preview->IsPlaybackOpen();
	if (Atteso.bPlaybackOpen) { Atteso.Position = Preview->GetPlaybackPosition(); }

	TestEqual(TEXT("e descrive la corsa appena avvenuta"),
		Dopo, FRTLauncherScenarioBrowser::DescribeTransport(Atteso).ToString());

	LauncherPanelReset(*Preview);
	return true;
}

/**
 * **Il clic nel vuoto dimentica la corsa E chiude l'anteprima.**
 *
 * 🔴 **E' la meta' che #2836 dichiara scoperta.** Quel giro ha corretto entrambe le cose e le ha coperte a
 * meta': `DevSandboxLauncher.TransportForgetsTheRunOnDeselect` misura la **conseguenza** su un
 * `FRTLauncherTransportStatus` costruito a mano — che la frase resti onesta con `bScenarioSelected = false`
 * — non il metodo. E la prova che la meta' scoperta conta e' che la **prima stesura di #2836 peggiorava il
 * difetto**: lasciava il playback aperto, con `Riproduci`, i quattro passi e `Reset` attivi e i marcatori
 * che si muovono, sotto un pannello che dichiarava nessuna selezione. La suite restava verde.
 *
 * ⚠️ **La precondizione e' asserita e non sperata**: se dopo `Esegui` il playback non fosse aperto, questo
 * test verificherebbe che una cosa gia' chiusa e' chiusa, e sarebbe verde per il motivo sbagliato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherPanelDeselectClosesPlaybackTest,
	"RefactorTactics.DevSandboxLauncher.PanelDeselectForgetsTheRunAndClosesThePlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherPanelDeselectClosesPlaybackTest::RunTest(const FString&)
{
	URTScenarioPreviewSubsystem* Preview = GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("il subsystem d'anteprima esiste"), Preview)) { return false; }
	LauncherPanelReset(*Preview);

	const FString Id = LauncherPanelTwoTeamScenario();
	if (!TestFalse(TEXT("il corpus offre uno scenario a due squadre"), Id.IsEmpty())) { return false; }

	const TSharedRef<SRTLauncherScenarioPanel> Panel = SNew(SRTLauncherScenarioPanel);
	Panel->TestApplySelection(MakeShared<FString>(Id));

	const TSharedPtr<SButton> Esegui = LauncherPanelButton(Panel, TEXT("Esegui"));
	if (!TestTrue(TEXT("il pulsante Esegui e' nell'albero"), Esegui.IsValid()))
	{
		LauncherPanelReset(*Preview);
		return false;
	}

	Esegui->SimulateClick();

	if (!TestTrue(TEXT("PRECONDIZIONE: dopo Esegui il playback e' aperto"), Preview->IsPlaybackOpen()))
	{
		LauncherPanelReset(*Preview);
		return false;
	}
	if (!TestEqual(TEXT("PRECONDIZIONE: e il pannello ricorda la corsa"),
		static_cast<int32>(Panel->TestRunProbe().RunState), static_cast<int32>(ERTLauncherRunState::Ran)))
	{
		LauncherPanelReset(*Preview);
		return false;
	}

	// Il clic nell'area vuota sotto le righe: `SListView` lo comunica con un item nullo.
	Panel->TestApplySelection(nullptr);

	const SRTLauncherScenarioPanel::FTestRunProbe Dopo = Panel->TestRunProbe();
	TestTrue(TEXT("la selezione se n'e' andata"), Dopo.SelectedId.IsEmpty());

	// I quattro `LastRun*`, uno per uno: e' cio' che un aggregato costruito a mano non puo' dire.
	//
	// ⚠️ **`LastRunDetail` e' l'unico dei quattro che la mutazione NON falsifica**, e va detto invece di
	// contarlo fra le prove: `DescribeRunDetail` restituisce testo vuoto per una corsa riuscita, quindi qui
	// il campo e' gia' vuoto prima del clic. Falsificarlo vorrebbe dire un esito `Blocked` o `Errored`,
	// che si ferma a zero turni e non apre nessun playback — cioe' non supererebbe la precondizione.
	TestEqual(TEXT("LastRunState torna a NotRun"),
		static_cast<int32>(Dopo.RunState), static_cast<int32>(ERTLauncherRunState::NotRun));
	TestEqual(TEXT("LastRunTurns torna a zero"), Dopo.RunTurns, 0);
	TestFalse(TEXT("LastRunHasTrace torna a falso"), Dopo.bHasTrace);
	TestTrue(TEXT("LastRunDetail torna vuoto"), Dopo.RunDetail.IsEmpty());

	// E la meta' che la prima stesura di #2836 aveva lasciato indietro.
	TestFalse(TEXT("e l'ANTEPRIMA e' chiusa: nessuna selezione, nessun playback"), Preview->IsPlaybackOpen());

	LauncherPanelReset(*Preview);
	return true;
}

#endif
