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


/**
 * **I controlli di trasporto, e la prova che non riesegue niente** — criterio 2 di `#1625`.
 *
 * 🔴 **La misura decisiva e' che il draft viene CHIUSO prima di navigare.** Un playback che
 * ricalcolasse avrebbe bisogno di una sessione da cui ripartire, e chiuso il draft quella sessione non
 * esiste piu': se dopo la chiusura i marcatori continuano a muoversi lungo la traccia, la sorgente e' la
 * traccia. E' anche esattamente cio' che fa il pulsante «Esegui» del pannello, che chiude il draft subito
 * dopo aver aperto il playback — quindi questo test copre il flusso reale, non uno di comodo.
 *
 * ⚠️ **Non si asserisce dove finiscano i marcatori**: quale cella occupi ogni unita' a ogni fase e' gia'
 * provato in `Replay.State.*` e `Scenario.Playback.*`, con mutazioni su entrambe. Qui si misura la
 * NAVIGAZIONE: che i passi rispettino i bordi, che `RESET` torni davvero indietro, e che la velocita' non
 * cambi dove si arriva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPlaybackTransportTest,
	"RefactorTactics.DevSandboxLauncher.PlaybackTransportDelegatesAndDoesNotRerun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPlaybackTransportTest::RunTest(const FString&)
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

	FRTScenarioRunReport Referto;
	FString Errore;
	if (Authoring->Run(Referto, Errore) != ERTScenarioAuthoringResult::Success
		|| !Preview->OpenPlayback(Authoring))
	{
		AddError(Errore.IsEmpty() ? TEXT("il playback non si e' aperto sulla corsa") : *Errore);
		Authoring->Close();
		Preview->ClearPreview();
		return false;
	}

	// 🔴 Da qui in poi NON c'e' piu' una sessione da cui rieseguire.
	Authoring->Close();

	// --- Si parte prima dell'inizio -------------------------------------------------------------------
	TestFalse(TEXT("all'apertura si e' PRIMA del primo turno"), Preview->GetPlaybackPosition().HasTurn());
	TestFalse(TEXT("e indietro non si va"), Preview->CanPlaybackStepPhase(false));
	TestFalse(TEXT("il passo indietro infatti risponde no"), Preview->PlaybackStepPhase(false));

	// --- Avanti fino in fondo, a draft chiuso ---------------------------------------------------------
	int32 Fasi = 0;
	while (Preview->CanPlaybackStepPhase(true))
	{
		if (!TestTrue(TEXT("il passo avanti riesce quando e' dichiarato possibile"),
			Preview->PlaybackStepPhase(true)))
		{
			break;
		}
		TestTrue(TEXT("e la preview resta viva"), Preview->IsShowing());
		if (++Fasi > 64) { break; }
	}

	// ⛔ ANTI-VACUITA': senza almeno un passo, ogni asserzione sopra e' vera per assenza — e sarebbe verde
	// anche su un playback che non si muove affatto.
	if (!TestTrue(TEXT("la traccia ha almeno una fase da percorrere"), Fasi > 0))
	{
		Preview->ClearPreview();
		return false;
	}

	const FRTReplayPosition Fine = Preview->GetPlaybackPosition();
	TestFalse(TEXT("in fondo non si avanza piu'"), Preview->PlaybackStepPhase(true));

	// --- RESET torna indietro davvero -----------------------------------------------------------------
	TestTrue(TEXT("il reset risponde"), Preview->PlaybackRewind());
	TestFalse(TEXT("e riporta prima dell'inizio"), Preview->GetPlaybackPosition().HasTurn());
	TestFalse(TEXT("fermando la riproduzione"), Preview->IsPlaybackPlaying());

	// --- Il passo di TURNO e' un'altra cosa dal passo di fase -----------------------------------------
	if (Preview->CanPlaybackStepTurn(true))
	{
		TestTrue(TEXT("un turno avanti"), Preview->PlaybackStepTurn(true));
		TestTrue(TEXT("e ora si e' dentro un turno"), Preview->GetPlaybackPosition().HasTurn());
	}

	// --- `Instant` ≡ `1x`: la velocita' e' presentazione ------------------------------------------------
	// ⚠️ Si confronta il punto d'arrivo, non la durata: la velocita' cambia QUANDO si avanza, non DOVE si
	// finisce. Con `Instant` la fase scade subito, quindi ogni tick avanza anche con delta zero.
	auto ScorriTutto = [Preview](ERTPlaybackSpeed Velocita, float Delta)
	{
		Preview->PlaybackRewind();
		Preview->SetPlaybackSpeed(Velocita);
		Preview->PlaybackPlay();
		for (int32 i = 0; i < 128 && Preview->IsPlaybackPlaying(); ++i)
		{
			Preview->PlaybackTick(Delta);
		}
		Preview->PlaybackPause();
		return Preview->GetPlaybackPosition();
	};

	const FRTReplayPosition FineIstantanea = ScorriTutto(ERTPlaybackSpeed::Instant, 0.f);
	const FRTReplayPosition FineNormale = ScorriTutto(ERTPlaybackSpeed::Normal, 10.f);

	TestEqual(TEXT("Instant e 1x finiscono nello stesso turno"),
		FineIstantanea.TurnNumber, FineNormale.TurnNumber);
	TestEqual(TEXT("e nella stessa fase"), FineIstantanea.Phase, FineNormale.Phase);
	TestEqual(TEXT("e nello stesso stato"), FineIstantanea.State, FineNormale.State);

	// ⛔ E finiscono dove finisce la traccia: senza questo, due riproduzioni entrambe FERME all'inizio
	// soddisferebbero le tre uguaglianze qui sopra.
	TestEqual(TEXT("e in fondo, dove ci si era arrivati a passi"), FineNormale.TurnNumber, Fine.TurnNumber);

	// --- Chiuso il playback, i comandi si spengono ----------------------------------------------------
	Preview->ClosePlayback();
	TestFalse(TEXT("a playback chiuso non si avanza"), Preview->CanPlaybackStepPhase(true));
	TestFalse(TEXT("ne' di un turno"), Preview->CanPlaybackStepTurn(true));
	TestFalse(TEXT("e non si sta riproducendo"), Preview->IsPlaybackPlaying());
	TestFalse(TEXT("un tick non fa niente"), Preview->PlaybackTick(1.f));

	Preview->ClearPreview();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
