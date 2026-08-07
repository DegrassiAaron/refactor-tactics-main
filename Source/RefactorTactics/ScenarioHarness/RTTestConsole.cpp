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
