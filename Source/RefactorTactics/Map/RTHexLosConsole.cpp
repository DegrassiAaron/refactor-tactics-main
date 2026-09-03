#include "Map/RTHexLosConsole.h"

#include "HAL/IConsoleManager.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"

/**
 * `rt.Debug.Los <fromQ> <fromR> <toQ> <toR> [layer]` — **perche'** una linea di tiro e' bloccata.
 *
 * Perche' esiste: la LOS decide nel bot, nel combattimento, nei criteri d'arena e nella percezione, e fino a
 * qui l'unico modo di sapere *perche'* un tiro non passa era leggere il codice. `#1712`.
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────────────
 * 🔴 **STAMPA, e non disegna — la scelta che il DoD chiedeva di scrivere, col costo dell'alternativa.**
 *
 * Disegnare avrebbe voluto dire un **sesto** blocco dentro `ARTHexMapActor::DrawPlanningPreview`, dove
 * cinque significati — reachable, percorso, colpo, fuoco amico, hover — convivono gia' ciascuno col proprio
 * `FColor` letterale (`RTHexMapActor.cpp:1023-1110`). Il modello che dica *cosa* significa un'area, *chi*
 * l'ha prodotta e *quanto* e' certa e' il lavoro di **#1941**, e la LOS e' il caso che ne ha piu' bisogno:
 * la sua informazione ha una certezza vera — cio' che vedo ora contro cio' che ricordo — mentre le altre
 * cinque no. Un sesto colore letterale avrebbe chiuso questa issue **allargando** quel difetto.
 *
 * ⚠️ E c'e' una collisione di palette gia' dichiarata: la spec v0.2 assegna il ciano-blu a Vision/LOS,
 * mentre in questo repository il ciano `FColor(40, 220, 220)` e' **gia'** la traccia del percorso. Non e'
 * una scelta da fare mentre si scrive un comando: e' una delle collisioni che #1941 porta a un `D-nnn`.
 *
 * ∴ si stampa, come fanno gia' `DrawPaths`, `DrawCover` e `DrawResolution` — e come `RTDebugConsole.cpp:163`
 * dichiara col suo debito, che appartiene a **#80** e resta suo. Il nome dice `Los` e non `DrawVision`
 * apposta: **non promette un disegno che non c'e'**, che e' la meta' del difetto registrato in quell'avviso.
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────────────
 * 🏠 **Vive in `Map/` e non in `Debug/`**, accanto al proprio produttore. E' il precedente che
 * `RTDebugConsole.cpp:7` conta *«cinque file su cinque»*: `rt.Debug.DrawCells` sta in
 * `Map/RTHexOverlayConsole.cpp`, `rt.Debug.Pacing` in `Turn/RTPacingConsole.cpp`. Il namespace e' comune,
 * il dominio decide il file.
 *
 * ⛔ **Sola lettura**: nessuna scrittura di stato, nessun targeting, nessun `Tick`. La regola sta in
 * `RTDebugConsole.cpp:13` — *«uno strumento d'ispezione che muove cio' che ispeziona produce sessioni di
 * debug che non si possono confrontare fra loro»*.
 */

TArray<FString> URTHexLosConsoleLibrary::DescribeVerdict(const FRTLineOfSightResult& Result,
	const FRTCellId& From, const FRTCellId& To)
{
	TArray<FString> Lines;

	// ⚠️ **Il layer, per primo e sempre.** `HasLineOfSight` non lo guarda: la linea resta su quello del
	// tiratore. Dirlo in coda, o solo quando i due differiscono, lascerebbe la lettura ambigua proprio nel
	// caso in cui e' piu' facile sbagliarsi — una mappa multilivello.
	Lines.Add(FString::Printf(
		TEXT("[RT] LOS (q=%d,r=%d) -> (q=%d,r=%d) — ragionata sul layer %d, quello del TIRATORE."),
		From.X, From.Y, To.X, To.Y, From.Layer));

	if (To.Layer != From.Layer)
	{
		Lines.Add(FString::Printf(
			TEXT("     ⚠️ il bersaglio sta sul layer %d: la linea NON ci sale, ed e' la regola d'elevazione."),
			To.Layer));
	}

	if (Result.IsClear())
	{
		Lines.Add(TEXT("     libera: nessun blocco lungo la linea."));
		return Lines;
	}

	// 🔴 Le QUATTRO cause si nominano tutte, `InteriorGeometry` compresa (`#1830`). Un `default:` che
	// tacesse sulla causa nuova e' il difetto che `RTHexLos::Describe` ha gia' dovuto correggere una volta,
	// scrivendo `unavailable` proprio dove serviva un nome.
	switch (Result.Block)
	{
	case ERTLineOfSightBlock::EdgeBlocker:
		// Il bordo ATTRAVERSATO, quindi due celle: la copertura alta e' una proprieta' *fra* di esse, e
		// conta anche sul primo e sull'ultimo passo — un muro addossato al bersaglio lo copre.
		Lines.Add(FString::Printf(
			TEXT("     BLOCCATA da una copertura sul BORDO fra (q=%d,r=%d,L%d) e (q=%d,r=%d,L%d), passo %d."),
			Result.BlockedFrom.X, Result.BlockedFrom.Y, Result.BlockedFrom.Layer,
			Result.BlockedAt.X, Result.BlockedAt.Y, Result.BlockedAt.Layer, Result.StepIndex));
		break;

	case ERTLineOfSightBlock::CellBlocker:
		// La CELLA, quindi una sola: `bBlocksLineOfSight`, con gli estremi esclusi — tiratore e bersaglio
		// non si oscurano da soli.
		Lines.Add(FString::Printf(
			TEXT("     BLOCCATA dalla CELLA (q=%d,r=%d,L%d), passo %d — `bBlocksLineOfSight`."),
			Result.BlockedAt.X, Result.BlockedAt.Y, Result.BlockedAt.Layer, Result.StepIndex));
		break;

	case ERTLineOfSightBlock::InteriorGeometry:
		// ⚠️ Nomina UNA cella e non due: il segmento sta *dentro*, non *fra*. E non nomina il muro — chi
		// vuole sapere quale segmento e' stato lo chiede a `URTHexOcclusionLibrary`, che ne e' l'autorita'.
		// E' la stessa disciplina con cui `EdgeBlocker` non distingue la copertura alta dalla porta chiusa.
		Lines.Add(FString::Printf(
			TEXT("     BLOCCATA dalla GEOMETRIA INTERNA di (q=%d,r=%d,L%d), passo %d — quale muro lo dice `rt.Debug.DumpCellPlacement`."),
			Result.BlockedAt.X, Result.BlockedAt.Y, Result.BlockedAt.Layer, Result.StepIndex));
		break;

	default:
		// Irraggiungibile finche' l'enum non cresce, e serve a dirlo quando crescera': meglio un nome
		// mancante dichiarato che una causa nuova resa come «bloccata» e basta.
		Lines.Add(FString::Printf(
			TEXT("     BLOCCATA da una causa senza nome in questo comando (valore %d): aggiungila qui."),
			static_cast<int32>(Result.Block)));
		break;
	}

	return Lines;
}

namespace
{
	void RTDebugLosCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
	{
		if (!World)
		{
			Ar.Log(TEXT("[RT] Nessun mondo attivo."));
			return;
		}
		if (Args.Num() < 4)
		{
			Ar.Log(TEXT("[RT] Uso: rt.Debug.Los <fromQ> <fromR> <toQ> <toR> [layer]"));
			return;
		}

		ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(World);
		const URTHexMapAsset* Map = HexMap ? HexMap->MapAsset : nullptr;
		if (!Map)
		{
			Ar.Log(TEXT("[RT] Nessuna mappa esagonale nel livello."));
			return;
		}

		// Il layer e' opzionale e vale per ENTRAMBE le celle: chiedere una linea fra due layer diversi
		// significherebbe chiedere una cosa che `HasLineOfSight` non calcola, e il comando non deve
		// suggerire che sia possibile. Chi vuole quel caso lo scrive nei due argomenti e legge l'avviso.
		const int32 Layer = (Args.Num() >= 5) ? FCString::Atoi(*Args[4]) : 0;
		const FRTCellId From(FCString::Atoi(*Args[0]), FCString::Atoi(*Args[1]), Layer);
		const FRTCellId To(FCString::Atoi(*Args[2]), FCString::Atoi(*Args[3]), Layer);

		// ⛔ Si CONSUMA la primitiva canonica: nessuna seconda LOS, nessuna riesecuzione della query. E' il
		// primo criterio del DoD di `#1712`, e la ragione per cui questo comando non puo' divergere da cio'
		// che il gioco crede — `HasLineOfSight` delega a `DescribeLineOfSight`, quindi la fonte e' una sola.
		const FRTLineOfSightResult Result = URTHexVisionLibrary::DescribeLineOfSight(Map, From, To);

		for (const FString& Line : URTHexLosConsoleLibrary::DescribeVerdict(Result, From, To))
		{
			Ar.Log(*Line);
		}
	}
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugLos(
	TEXT("rt.Debug.Los"),
	TEXT("rt.Debug.Los <fromQ> <fromR> <toQ> <toR> [layer] — perche' una linea di tiro e' bloccata. Stampa, non disegna."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugLosCommand));
