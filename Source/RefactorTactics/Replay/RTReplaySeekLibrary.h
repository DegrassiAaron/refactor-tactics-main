#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnRules.h"
#include "RTReplaySeekLibrary.generated.h"

/**
 * Esito di un seek. Esiste perche' la convenzione del replay e' **fail-closed**: un bersaglio che non c'e'
 * deve dirlo, non restituire un indice plausibile. E' la stessa scelta che `DeserializeTurnLog` applica al
 * formato — rifiutare invece di interpretare byte arbitrari.
 */
UENUM(BlueprintType)
enum class ERTReplaySeekResult : uint8
{
	/** Il bersaglio esiste: il cursore in uscita e' valido. */
	Found,
	/** Nessuna traccia della sequenza dichiara quel turno. Il cursore in uscita NON e' stato toccato. */
	TurnNotFound,
	/** Il turno esiste, ma nella sua traccia quella fase non ha prodotto nessuna voce. */
	PhaseNotFound,

	/**
	 * La fase c'e', ma nessuna sua voce appartiene al micro-step chiesto (`#1880`).
	 *
	 * ⚠️ **E' un esito distinto da `PhaseNotFound` e deve esserlo**: la fase esiste, quindi il turno si e'
	 * svolto: cio' che manca e' quella barriera. Confonderli direbbe «la fase non c'e'» di un turno in cui
	 * la fase c'era, ed e' l'errore che manda a cercare nel posto sbagliato.
	 */
	BoundaryNotFound
};

/**
 * Posizione nella riproduzione di una sequenza di TurnLog: quale traccia, e quale voce dentro di essa.
 *
 * Non e' lo stato del gioco, ed e' deliberato: un cursore non ricalcola niente. Il confine
 * `ReplayPlayer`/`ReplayVerifier` — chi riproduce non chiama il resolver — e' il rischio `REPLAY-04` del risk
 * register, ed e' fissato da [ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §3
 * ([D-078](../../../docs/decisions/RT_PDR_00_Decision_Log.md), `#412`): questa libreria e' **Player**.
 *
 * Qui la regola non vale per conformita' ma **per costruzione** — non c'e' modo di chiamare il resolver da
 * qui — ed e' l'ordine giusto: l'ADR ha preso questo file come precedente, chiedendo la separazione
 * strutturale come requisito e il test d'architettura solo come rete. Un test si aggira con un `#include`;
 * una dipendenza che non esiste no.
 */
USTRUCT(BlueprintType)
struct FRTReplayCursor
{
	GENERATED_BODY()

	/** Indice della traccia nella sequenza. NON e' il numero di turno: le tracce lo dichiarano in `TurnNumber`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 TraceIndex = 0;

	/** Prima voce NON ancora consumata dentro la traccia. `Trace.Num()` = traccia esaurita. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 EntryIndex = 0;

	bool operator==(const FRTReplayCursor& Other) const
	{
		return TraceIndex == Other.TraceIndex && EntryIndex == Other.EntryIndex;
	}
	bool operator!=(const FRTReplayCursor& Other) const { return !(*this == Other); }
};

/**
 * Seek in una sequenza di TurnLog: aprire un replay al turno 8 senza riprodurre i turni 1-7 (`#415`).
 *
 * ⚠️ **Precondizione: le tracce sono in forma canonica** — l'ordine di `EntryLess` (`spec-turnlog.md` §6),
 * che e' quello in cui `SerializeTurnLog` le scrive e `DeserializeTurnLog` le rilegge (`D-SR-1`). Non e' una
 * difesa che questa libreria ripete: la proprieta' e' prodotta a monte, e verificarla di nuovo qui
 * nasconderebbe chi la rompe invece di mostrarlo.
 *
 * Il seek alla **fase** funziona per una ragione precisa e non per fortuna: `Phase` e' la PRIMA chiave di
 * `EntryLess` e `ERTMatchPhase` e' dichiarato in ordine cronologico (`RTTurnRules.h`), quindi dentro una
 * traccia canonica le voci di una fase sono contigue e le fasi si susseguono nell'ordine in cui sono state
 * giocate. ⚠️ In pratica le fasi osservabili sono **cinque**: nessun punto del resolver emette voci con
 * `Planning` o `MatchEnded`, quindi un seek a quelle due risponde sempre `PhaseNotFound` — corretto, ma
 * sorprendente se non lo si sa. Il **turno** invece non e' un intervallo contiguo: in `EntryLess` sta dopo la fase, quindi
 * concatenare piu' turni in un solo array li mescolerebbe. Per questo l'unita' del seek al turno e' la
 * traccia, che e' gia' l'unita' del TurnLog (`FRTTurnLogEntry::TurnNumber` e' costante per traccia).
 *
 * Il **micro-step resta fuori** e non e' una semplificazione di comodo: la voce non porta ne' indice di
 * sequenza ne' micro-step, e l'ordine delle voci serializzate e' la chiave di sort, non l'ordine di
 * emissione. Chiederglielo sarebbe chiedere alla traccia un'informazione che non ha (conflict report §4.1).
 */
UCLASS()
class REFACTORTACTICS_API URTReplaySeekLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Indice della prima voce della fase dentro UNA traccia canonica.
	 *
	 * Una fase senza voci non e' un errore del chiamante ma non e' nemmeno una posizione: il turno puo'
	 * essersi svolto senza che nessuno agisse in `Dash`. `PhaseNotFound` lo dice invece di restituire
	 * l'indice della fase successiva, che sarebbe la risposta a un'altra domanda.
	 */
	static ERTReplaySeekResult SeekToPhase(const TArray<FRTTurnLogEntry>& Trace, ERTMatchPhase Phase,
		int32& OutEntryIndex);

	/**
	 * Indice della prima voce del **boundary** — `(Phase, MicroStepIndex)` — dentro una traccia canonica
	 * (`#1880`).
	 *
	 * 🔑 **E' la terza coordinata, e chiude la terna**: `SeekToTurn` trova il turno, `SeekToPhase` la
	 * fase, questa il micro-step dentro la fase. Il boundary e' `(TurnNumber, Phase, MicroStepIndex)`, la
	 * stessa terna che `FRTReactionOpportunityKey` usa gia' come identita' di una finestra di reazione.
	 *
	 * ⛔ **Trova il GRUPPO, non un evento.** Le voci che condividono il boundary sono state decise insieme:
	 * questa funzione restituisce l'indice della prima in ordine canonico, che e' il punto da cui il gruppo
	 * comincia — non «la prima che e' accaduta», perche' quella domanda non ha risposta.
	 *
	 * ⚠️ **Su una traccia phase-only ogni voce vale `MicroStepIndex == 0`**, e il comportamento e'
	 * dichiarato invece che dedotto: chiedere il boundary `0` trova la prima voce della fase — e' l'unico
	 * boundary che quella traccia conosce, e rispondere e' corretto; chiedere un boundary diverso da `0`
	 * da' `BoundaryNotFound`, perche' quella traccia davvero non lo porta. Nessuna inferenza dalla
	 * posizione, nessun fallback alla fase intera.
	 */
	static ERTReplaySeekResult SeekToBoundary(const TArray<FRTTurnLogEntry>& Trace, ERTMatchPhase Phase,
		int32 MicroStepIndex, int32& OutEntryIndex);

	/**
	 * Cursore all'inizio del turno DICHIARATO da una traccia della sequenza.
	 *
	 * Cerca `TurnNumber` nelle voci, non l'indice nell'array: una sequenza puo' iniziare da un turno
	 * qualsiasi. Le tracce scritte prima del formato v6 dichiarano `0` — non sono indirizzabili per turno, e
	 * questo e' l'esito corretto, non un buco da tappare inferendo il turno dalla posizione.
	 */
	static ERTReplaySeekResult SeekToTurn(const TArray<TArray<FRTTurnLogEntry>>& Sequence, int32 TurnNumber,
		FRTReplayCursor& OutCursor);

	/** Cursore alla fase dentro il turno: `SeekToTurn` seguito da `SeekToPhase` sulla traccia trovata. */
	static ERTReplaySeekResult SeekToTurnPhase(const TArray<TArray<FRTTurnLogEntry>>& Sequence, int32 TurnNumber,
		ERTMatchPhase Phase, FRTReplayCursor& OutCursor);

	/**
	 * Avanza il cursore di UNA voce, saltando alla traccia successiva quando la corrente e' esaurita.
	 * `false` = la sequenza e' finita e il cursore non si e' mosso.
	 *
	 * E' la scansione lineare, ed esiste in produzione e non nei test perche' e' meta' del contratto del
	 * seek: «saltare al turno N» ha senso solo se produce **la stessa posizione** che si otterrebbe
	 * scorrendo. Senza le due funzioni nello stesso posto, l'equivalenza non sarebbe verificabile.
	 *
	 * ⚠️ Non si chiama «playback» di proposito: in questo progetto la parola e' gia' presa dalla
	 * PRESENTAZIONE — `URTPlaybackLibrary` e il playback di `ARTTurnManager`, che interpolano e misurano
	 * durate — e `spec-turnlog.md` §13 la dichiara «struttura sorella, **non riusata**». Qui non c'e' nulla
	 * di temporale: si avanza di un indice.
	 */
	static bool AdvanceOneEntry(const TArray<TArray<FRTTurnLogEntry>>& Sequence, FRTReplayCursor& Cursor);
};
