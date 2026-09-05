// #2486 — il comando che rende RAGGIUNGIBILE il verdetto di boundary.
//
// La forma e' quella degli otto `rt.Debug.*` di #80: il CONTENUTO sta in funzioni che si verificano
// headless — `URTDebugReportLibrary::DescribeBoundaryChecksums` e `DescribeBoundaryDivergence`, pinnate da
// `RefactorTactics.Debug.BoundaryChecksumReportNamesEveryBoundary` e
// `BoundaryDivergenceReportNamesTheTriple` — e questo file e' il wrapper sottile che le stampa.
//
// ⚠️ **Sta nel modulo EDITOR e non accanto agli altri otto, e non e' una dimenticanza.** La sola via che
// possiede lo schieramento iniziale e' lo Scenario Preview, che e' editor-only: `ChecksumsAlongTrace` vuole
// `Initial`, la traccia dichiara i CAMBIAMENTI e non le partenze, e `ARTTurnManager` espone lo stato
// CORRENTE. Un `rt.Debug.*` nel runtime non potrebbe raggiungere nessuno dei tre ingressi — ed e' la ragione
// per cui l'ipotesi iniziale di #2486, un comando sulla partita viva, non era realizzabile.
//
// **Sola lettura**: non apre, non chiude e non muove il playback. Uno strumento di ispezione che modifica
// cio' che ispeziona non e' uno strumento di ispezione.

#include "RTScenarioPreviewSubsystem.h"

#include "Editor.h"
#include "HAL/IConsoleManager.h"

namespace
{
	void RTScenarioDumpBoundariesCommand(const TArray<FString>& /*Args*/, UWorld* /*World*/, FOutputDevice& Ar)
	{
		// ⚠️ `GEditor` puo' essere nullo in una `-game` o in commandlet: dirlo invece di dereferenziare.
		if (!GEditor)
		{
			Ar.Log(TEXT("[RT] Nessun editor: questo comando legge lo Scenario Preview, che e' editor-only."));
			return;
		}

		const URTScenarioPreviewSubsystem* Preview =
			GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>();
		if (!Preview)
		{
			Ar.Log(TEXT("[RT] Scenario Preview non disponibile."));
			return;
		}

		// Il subsystem dichiara da se' il caso «nessun playback aperto»: qui non si reinterpreta.
		for (const FString& Riga : Preview->DescribePlaybackBoundaries())
		{
			Ar.Log(*Riga);
		}
	}
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTScenarioDumpBoundaries(
	TEXT("rt.Scenario.DumpBoundaries"),
	TEXT("I boundary checksum della corsa in playback, e dove diverge dalla corsa precedente. Sola lettura."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTScenarioDumpBoundariesCommand));
