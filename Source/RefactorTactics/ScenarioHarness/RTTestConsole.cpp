#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestReportWriter.h"
#include "ScenarioHarness/RTTestResult.h"
#include "ScenarioHarness/RTScenarioLoader.h"

/**
 * Comandi console dell'RT Scenario Test Harness. Stesso namespace di `rt.Debug.DrawCells`.
 *
 *   rt.Test.List                  elenca gli scenari versionati
 *   rt.Test.Run <ScenarioId>      esegue uno scenario nel mondo corrente e scrive il report
 *   rt.Test.DumpResult [Id]       stampa l'ultimo result.json
 *   rt.Test.Scenario <Id>         (variabile) scenario da eseguire AUTOMATICAMENTE all'avvio della partita
 *
 * `rt.Test.Scenario` e' l'auto-run richiesto dal documento di specifica: impostata prima di premere Play,
 * il GameMode esegue lo scenario invece di allestire la partita normale, senza che l'utente tocchi altro.
 * E' una variabile e non un Actor da trascinare nel livello perche' cosi' non serve modificare nessun
 * `.umap`: si imposta da console, da `DefaultEngine.ini` o da riga di comando con `-ExecCmds`.
 */

TAutoConsoleVariable<FString> CVarRTTestScenario(
	TEXT("rt.Test.Scenario"),
	TEXT(""),
	TEXT("Scenario da eseguire automaticamente all'avvio della partita (es. Movement.Basic). Vuoto = partita normale."),
	ECVF_Default);

/**
 * `rt.Map.Source` scavalca la proprieta' `MapSource` del GameMode, con la stessa regola di
 * `rt.Test.Scenario`: la proprieta' e' la configurazione persistente, la console variable e' l'intento di
 * chi lancia adesso.
 *
 * Esiste per una ragione misurata: la proprieta' vive nei Class Defaults di `BP_GameMode`, cioe' in un
 * `.uasset`. Cambiarla richiede l'editor — e CP 12.5 ha misurato una build pacchettizzata che girava
 * sull'arena di PROVA senza che ci fosse modo di dirle altro da fuori. Con questa variabile la scelta si
 * fa da riga di comando, che e' quanto serve a una verifica automatica e a una sessione di playtest.
 *
 * Valori accettati: i nomi dell'enum `ERTMapSource` (`LevelAsset`, `GeneratedTestArena`, ...), senza
 * distinzione fra maiuscole e minuscole. Un valore sconosciuto NON ripiega in silenzio: il GameMode lo
 * dichiara e tiene la proprieta', perche' una mappa scelta per sbaglio e' un playtest buttato.
 *
 * ⚠️ **Da riga di comando serve `-dpcvars=`, non `-ExecCmds=`** — misurato sul pacchettizzato il
 * 2026-08-10. `-ExecCmds` gira DOPO l'inizializzazione, quando il GameMode ha gia' allestito la
 * partita: la variabile viene impostata e non serve a niente, senza un errore che lo dica.
 *
 *     RefactorTactics.exe -dpcvars=rt.Map.Source=LevelAsset
 */
TAutoConsoleVariable<FString> CVarRTMapSource(
	TEXT("rt.Map.Source"),
	TEXT(""),
	TEXT("Sorgente della mappa, scavalca MapSource del GameMode (es. LevelAsset). Vuoto = usa la proprieta'."),
	ECVF_Default);

/**
 * La FIXTURE su cui giocare, per nome. Vince su `rt.Map.Source`.
 *
 * 🔴 **Esiste per rendere misurabile una verifica visiva che oggi non lo e'.** Le due arene generabili
 * da riga di comando non portano superfici: `MakeDemoArena` le lascia tutte `Default` e `MakeTestArena`
 * tutte `Rough`. Chi vuole guardare a schermo se le nove tinte della tavolozza si distinguono — e' la
 * domanda di `#1290` — doveva aprire l'editor e scrivere la fixture nell'asset a mano, quindi la sua
 * misura non si riproduceva e la sua evidenza viveva in `Saved/`, che non e' versionato.
 *
 * `RelayBasin` porta **otto** superfici distinte ed e' la sola board adatta a quella domanda.
 *
 *     RefactorTactics.exe -dpcvars=rt.Map.Fixture=RelayBasin
 *
 * ⚠️ Un nome sconosciuto **non ripiega in silenzio**: si dice e si gioca con la sorgente configurata,
 * per la stessa ragione per cui `rt.Map.Source` fa lo stesso — un playtest attribuito a una board che non
 * era in vigore e' peggio di un playtest mancato.
 */
/**
 * Vista a PICCO sull'intera board, per le misure che si fanno su un'immagine.
 *
 * 🔴 **Esiste per la stessa ragione di `rt.Map.Fixture`: una misura che richiede l'editor non e' una
 * misura che si ripete.** `#1290` misura la luminanza delle facce delle celle, e va fatta guardando la
 * board dall'alto: la camera di partita e' obliqua e inquadra bordi, lati e ombre, cioe' varieta' che non
 * viene dalla tavolozza e che falserebbe l'istogramma verso il basso.
 *
 * L'inquadratura si deriva dal bounding box della mappa, non da un numero scritto a mano: cosi' vale su
 * qualunque fixture e sopravvive al cambio di `HexSize`.
 *
 *     RefactorTactics.exe -dpcvars=rt.Map.Fixture=RelayBasin,rt.Camera.TopDown=1
 *
 * ⚠️ **E' una vista di MISURA, non una modalita' di gioco**: scavalca l'apertura sulla propria squadra e
 * il pitch configurato, e resta attiva finche' la cvar lo e'.
 */
/**
 * Scatta un `HighResShot` quando la vista a picco e' pronta, e dice dove l'ha messo.
 *
 * 🔴 **Serve a confrontare due catture dello STESSO frame.** La misura di `#1290` veniva da
 * `HighResShot`, che ha un path di rendering proprio; una cattura della finestra mostra invece cio' che
 * il giocatore vede. Sulle stesse condizioni le due davano numeri diversi — 31% di pixel saturi contro
 * meno dell'1% — e finche' non si confrontano sullo stesso fotogramma non si sa quale delle due misuri
 * male.
 *
 * ⚠️ **Il ritardo non e' un margine di sicurezza, e' parte della misura**: l'esposizione automatica
 * impiega qualche decimo a stabilizzarsi, e scattare subito fotograferebbe una board a meta' adattamento.
 * Il valore della cvar sono i **secondi** da aspettare.
 *
 *     -dpcvars=rt.Map.Fixture=RelayBasin,rt.Camera.TopDown=1,rt.Camera.TopDownShot=3
 */
TAutoConsoleVariable<int32> CVarRTCameraTopDownShot(
	TEXT("rt.Camera.TopDownShot"),
	0,
	TEXT("Secondi dopo i quali scattare un HighResShot della vista a picco. 0 = nessuno scatto."),
	ECVF_Default);

TAutoConsoleVariable<int32> CVarRTCameraTopDown(
	TEXT("rt.Camera.TopDown"),
	0,
	TEXT("1 = camera a picco sull'intera board (per le misure su immagine). 0 = vista di gioco."),
	ECVF_Default);

TAutoConsoleVariable<FString> CVarRTMapFixture(
	TEXT("rt.Map.Fixture"),
	TEXT(""),
	TEXT("Fixture di mappa per nome (ArenaV01, RelayBasin, RelayLite, TestArena, CoverYard, DemoArena). "
		 "Vince su rt.Map.Source. Vuoto = nessun effetto."),
	ECVF_Default);

namespace
{
	/** Ultimo esito, per `rt.Test.DumpResult` senza argomenti. Solo diagnostica: non decide nulla. */
	FString GLastScenarioId;

	void RTTestListCommand(const TArray<FString>&, UWorld*, FOutputDevice& Ar)
	{
		const TArray<FString> Ids = URTScenarioRunner::ListScenarioIds();
		if (Ids.Num() == 0)
		{
			Ar.Logf(TEXT("[RT-Test] nessuno scenario in %s"), *URTScenarioLoader::ScenariosRoot());
			return;
		}
		Ar.Logf(TEXT("[RT-Test] %d scenari:"), Ids.Num());
		for (const FString& Id : Ids)
		{
			Ar.Logf(TEXT("  %s"), *Id);
		}
	}

	void RTTestRunCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
	{
		if (Args.Num() == 0)
		{
			Ar.Log(TEXT("[RT-Test] uso: rt.Test.Run <ScenarioId>   (rt.Test.List per l'elenco)"));
			return;
		}
		if (!World)
		{
			Ar.Log(TEXT("[RT-Test] nessun mondo attivo: avvia una partita (Play) prima di eseguire uno scenario."));
			return;
		}

		const FString ScenarioId = Args[0];
		FString ReportDir;
		const FRTTestResult Result = URTScenarioRunner::RunById(World, ScenarioId, ReportDir);
		GLastScenarioId = ScenarioId;

		Ar.Logf(TEXT("[RT-Test] %s -> %s (%d/%d assertion, %d turni)"),
			*ScenarioId, *Result.OutcomeString(),
			Result.PassedCount(), Result.Assertions.Num(), Result.TurnsPlayed);

		if (!Result.ErrorMessage.IsEmpty())
		{
			Ar.Logf(TEXT("[RT-Test]   ERROR: %s"), *Result.ErrorMessage);
		}
		// Le fallite si stampano per intero: chi guarda la console deve poter capire senza aprire il file.
		for (const FRTAssertionResult& A : Result.Assertions)
		{
			if (!A.bPassed)
			{
				Ar.Logf(TEXT("[RT-Test]   FALLITA %s: atteso %s, ottenuto %s (turno %d)"),
					*A.Description, *A.Expected, *A.Actual, A.Turn);
			}
		}
		if (!ReportDir.IsEmpty())
		{
			Ar.Logf(TEXT("[RT-Test]   report: %s"), *FPaths::Combine(ReportDir, TEXT("result.json")));
		}
	}

	void RTTestDumpResultCommand(const TArray<FString>& Args, UWorld*, FOutputDevice& Ar)
	{
		const FString ScenarioId = Args.Num() > 0 ? Args[0] : GLastScenarioId;
		if (ScenarioId.IsEmpty())
		{
			Ar.Log(TEXT("[RT-Test] nessuno scenario eseguito in questa sessione: rt.Test.DumpResult <ScenarioId>"));
			return;
		}

		const FString Dir = URTTestReportWriter::FindLatestRunDirectory(ScenarioId);
		if (Dir.IsEmpty())
		{
			Ar.Logf(TEXT("[RT-Test] nessuna run per '%s'"), *ScenarioId);
			return;
		}

		FString Json;
		const FString Path = FPaths::Combine(Dir, TEXT("result.json"));
		if (!FFileHelper::LoadFileToString(Json, *Path))
		{
			Ar.Logf(TEXT("[RT-Test] report illeggibile: %s"), *Path);
			return;
		}
		Ar.Logf(TEXT("[RT-Test] %s"), *Path);
		Ar.Log(*Json);
	}
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTTestList(
	TEXT("rt.Test.List"),
	TEXT("Elenca gli scenari di test versionati in Scenarios/."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTTestListCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTTestRun(
	TEXT("rt.Test.Run"),
	TEXT("rt.Test.Run <ScenarioId> — esegue lo scenario nel mondo corrente e scrive Saved/RTTests/<Id>/<Run>/result.json."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTTestRunCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTTestDumpResult(
	TEXT("rt.Test.DumpResult"),
	TEXT("rt.Test.DumpResult [ScenarioId] — stampa l'ultimo result.json (default: ultimo scenario eseguito)."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTTestDumpResultCommand));
