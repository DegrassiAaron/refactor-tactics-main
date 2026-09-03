#include "Misc/AutomationTest.h"
#include "RTScenarioPreviewSubsystem.h"
#include "Editor.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioKnowledge.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **La preview si muove lungo la traccia** — l'ultimo anello del criterio 1 di `#1625`.
 *
 * ⚠️ **Cosa questi test provano, e cosa no.** Provano che il subsystem legga la traccia dell'ultima corsa,
 * traduca gli id e ridisegni: cioe' che la catena `#2095` → `#2173` → `#2176` → `#2185` sia **collegata**.
 * ⛔ Non provano che a schermo si veda qualcosa: `ARTScenarioPreviewActor` posa istanze in un viewport
 * d'editor, e nessun automation test puo' guardarle. Quello resta la seduta `U26`.
 *
 * 🔴 **Il caso che morde e' il RIFIUTO.** Un playback aperto su uno scenario mai eseguito, o su tracce
 * illeggibili, non deve aprirsi «vuoto»: mostrerebbe un campo senza unita' che somiglia a una partita in
 * cui non e' successo niente — e sono due affermazioni diverse.
 */
namespace
{
	URTScenarioPreviewSubsystem* PlaybackPreview()
	{
		return GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr;
	}

	/** Apre il primo scenario con almeno due squadre e lo mostra. Il draft resta APERTO: serve per correre. */
	URTScenarioAuthoring* ShowAndKeep(URTScenarioPreviewSubsystem* Preview)
	{
		URTScenarioAuthoring* Authoring = URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage());
		if (!Authoring) { return nullptr; }

		for (const FString& Id : URTScenarioAuthoring::ListScenarioIds(FString(), FString()))
		{
			FString OpenError;
			if (Authoring->OpenById(Id, OpenError) != ERTScenarioAuthoringResult::Success) { continue; }

			if (RTScenarioKnowledge::TeamIds(Authoring->ListUnits()).Num() >= 2
				&& Preview->ShowScenario(Authoring))
			{
				return Authoring; // ⚠️ NON si chiude: `OpenPlayback` legge le tracce dallo stesso draft
			}
			Authoring->Close();
		}
		return nullptr;
	}
}

/**
 * **Senza una corsa non si apre un playback**, e la preview resta quella d'authoring.
 *
 * 🔑 E' il caso piu' probabile in uso reale — si apre uno scenario e si preme il pulsante prima di
 * eseguire — ed e' quello in cui un'implementazione permissiva fa il danno peggiore: un campo vuoto che
 * sembra un esito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPlaybackRefusesWithoutRunTest,
	"RefactorTactics.DevSandboxLauncher.PlaybackRefusesWithoutARun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPlaybackRefusesWithoutRunTest::RunTest(const FString&)
{
	URTScenarioPreviewSubsystem* Preview = PlaybackPreview();
	if (!TestNotNull(TEXT("il subsystem d'anteprima esiste"), Preview)) { return false; }

	// --- Senza nemmeno una preview -------------------------------------------------------------------
	Preview->ClearPreview();
	TestFalse(TEXT("senza preview non si apre"), Preview->OpenPlayback(nullptr));
	TestFalse(TEXT("e non risulta aperto"), Preview->IsPlaybackOpen());

	// --- Con una preview ma senza corsa ---------------------------------------------------------------
	URTScenarioAuthoring* Authoring = ShowAndKeep(Preview);
	if (!TestNotNull(TEXT("uno scenario a due squadre si mostra"), Authoring))
	{
		Preview->ClearPreview();
		return false;
	}

	TestFalse(TEXT("senza aver corso il playback NON si apre"), Preview->OpenPlayback(Authoring));
	TestFalse(TEXT("e non risulta aperto"), Preview->IsPlaybackOpen());

	// ⛔ La meta' che conta: la preview resta viva e mostra l'authoring, non un campo vuoto.
	TestTrue(TEXT("e la preview resta quella d'authoring"), Preview->IsShowing());

	// E spostarsi non fa niente, invece di fingere.
	TestFalse(TEXT("posizionarsi senza playback risponde no"),
		Preview->SetPlaybackPosition(1, ERTMatchPhase::Move));

	Authoring->Close();
	Preview->ClearPreview();
	return true;
}

/**
 * **Dopo una corsa il playback si apre, si sposta, e si chiude tornando all'authoring.**
 *
 * ⚠️ **Non si asserisce DOVE finiscano i marcatori**, e non e' una rinuncia: quale cella occupi ogni unita'
 * a ogni fase e' gia' provato — `Replay.State.*` sulla ricostruzione, `Scenario.Playback.*` sulla
 * traduzione, con mutazioni su entrambe. Ripetere qui quelle asserzioni misurerebbe una terza volta le
 * stesse funzioni, attraverso un allestimento piu' fragile. Cio' che **solo** questo livello puo' dire e'
 * che i pezzi siano **collegati**: che la traccia venga letta, gli id tradotti, e il disegno rifatto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPlaybackOpensAndMovesTest,
	"RefactorTactics.DevSandboxLauncher.PlaybackOpensOnTheLastRunAndMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPlaybackOpensAndMovesTest::RunTest(const FString&)
{
	URTScenarioPreviewSubsystem* Preview = PlaybackPreview();
	if (!TestNotNull(TEXT("il subsystem d'anteprima esiste"), Preview)) { return false; }

	Preview->ClearPreview();
	URTScenarioAuthoring* Authoring = ShowAndKeep(Preview);
	if (!TestNotNull(TEXT("uno scenario a due squadre si mostra"), Authoring))
	{
		Preview->ClearPreview();
		return false;
	}

	FRTScenarioRunReport Report;
	FString Error;
	const ERTScenarioAuthoringResult Esito = Authoring->Run(Report, Error);
	if (!TestEqual(TEXT("lo scenario si esegue"), Esito, ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		Authoring->Close();
		Preview->ClearPreview();
		return false;
	}

	// ⛔ Anti-vacuita': senza tracce e senza il ponte degli id il test sotto sarebbe verde per assenza.
	TestTrue(TEXT("la corsa ha lasciato delle tracce"), Authoring->GetLastRunTraces().Num() > 0);
	TestTrue(TEXT("e il ponte fra i due spazi di id"), Authoring->GetLastRunScenarioIds().Num() > 0);

	// --- Si apre, e si posiziona all'inizio -----------------------------------------------------------
	if (!TestTrue(TEXT("il playback si apre sull'ultima corsa"), Preview->OpenPlayback(Authoring)))
	{
		Authoring->Close();
		Preview->ClearPreview();
		return false;
	}
	TestTrue(TEXT("e risulta aperto"), Preview->IsPlaybackOpen());
	TestTrue(TEXT("con la preview ancora viva"), Preview->IsShowing());

	// --- Ci si sposta, e la preview regge --------------------------------------------------------------
	// ⚠️ Ogni posizione passa da `ApplyPerspective`, che ricalcola conoscenza, velo e marcatori: se una di
	// quelle strade fosse rotta, `IsShowing()` cadrebbe o il subsystem crasherebbe qui.
	for (const ERTMatchPhase Fase : { ERTMatchPhase::Prep, ERTMatchPhase::Blast, ERTMatchPhase::Cleanup })
	{
		TestTrue(TEXT("ci si posiziona nel turno 1"), Preview->SetPlaybackPosition(1, Fase));
		TestTrue(TEXT("e la preview resta viva"), Preview->IsShowing());
	}

	// Una posizione OLTRE la fine non e' un errore: e' l'ultimo stato. Chiedere il turno 99 di una partita
	// di tre e' cio' che fa un controllo «vai alla fine» scritto senza conoscere la durata.
	TestTrue(TEXT("una posizione oltre la fine si accetta"),
		Preview->SetPlaybackPosition(99, ERTMatchPhase::Cleanup));

	// --- E la prospettiva continua a funzionare DURANTE il playback -----------------------------------
	// 🔑 E' la giuntura che un ramo separato romperebbe: se i marcatori venissero dal playback e il velo
	// dall'authoring, questa chiamata disegnerebbe due istanti diversi insieme.
	const TArray<int32> Squadre = Preview->GetSelectableTeams();
	if (Squadre.Num() > 0)
	{
		TestTrue(TEXT("si cambia prospettiva mentre il playback e' aperto"),
			Preview->SetPerspective(Squadre[0]));
		TestTrue(TEXT("e la preview regge"), Preview->IsShowing());
	}

	// --- Si chiude, e si torna all'authoring ----------------------------------------------------------
	Preview->ClosePlayback();
	TestFalse(TEXT("il playback e' chiuso"), Preview->IsPlaybackOpen());
	TestTrue(TEXT("e la preview mostra di nuovo lo scenario"), Preview->IsShowing());

	Authoring->Close();
	Preview->ClearPreview();
	TestFalse(TEXT("e `ClearPreview` non lascia un playback orfano"), Preview->IsPlaybackOpen());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
