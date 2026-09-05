// #2486 — il comando che rende RAGGIUNGIBILE il verdetto di boundary.
//
// La forma e' quella degli otto `rt.Debug.*` di #80: il CONTENUTO sta in funzioni che si verificano
// headless — `URTDebugReportLibrary::DescribeBoundaryChecksums` e `DescribeBoundaryDivergence`, pinnate da
// `RefactorTactics.Debug.BoundaryChecksumReportNamesEveryBoundary` e
// `BoundaryDivergenceReportNamesTheTriple` — e questo file e' il wrapper sottile che le stampa.
//
// ⚠️ **Sta nel modulo EDITOR ma resta nel namespace `rt.Debug.`, ed e' deliberato in entrambe le meta'.**
// Il modulo non e' mai stato una ragione per cambiare prefisso: `rt.Debug.Los` vive in `Map/` e
// `rt.Debug.Pacing` in `Turn/`. Il prefisso e' la superficie con cui un designer SCOPRE i comandi — digita
// `rt.Debug.` e vede cosa c'e' — ed e' quella che `Debug.NamespaceDeclaresAllCommands` enumera. Un
// `rt.Scenario.` avrebbe reso l'unico ingresso di questa feature invisibile a entrambi.
//
// Quello che invece dipende davvero dal modulo e' il DATO: La sola via che
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
	TEXT("rt.Debug.DumpBoundaries"),
	TEXT("I boundary checksum della corsa in playback, e dove diverge dalla corsa precedente. Sola lettura."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTScenarioDumpBoundariesCommand));
