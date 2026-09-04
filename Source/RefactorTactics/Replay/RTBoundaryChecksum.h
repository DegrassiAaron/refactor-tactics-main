#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnLog.h"           // FRTTurnLogEntry, ERTMatchPhase
#include "Replay/RTReplayStateLibrary.h" // FRTTracedUnitState
#include "RTBoundaryChecksum.generated.h"

class URTHexMapAsset;

/**
 * Il checksum di stato a **un** boundary della traccia (`#2189`).
 *
 * 🔑 **Perche' esiste**: il progetto aveva un solo hash per partita — `FRTReplayManifest::FinalStateHash` —
 * quindi quando due esecuzioni divergevano il gate diceva **che** avevano divergiato, non **dove**. Questo
 * tipo porta la stessa domanda a una granularita' in cui la risposta e' utile.
 */
USTRUCT(BlueprintType)
struct FRTBoundaryChecksum
{
	GENERATED_BODY()

	/** Il turno del boundary. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 TurnNumber = 0;

	/** La fase del boundary. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	/**
	 * Il micro-step del boundary, o `INDEX_NONE` per **la fase intera** (`#2374`).
	 *
	 * 🔑 **E' la terza coordinata che `#2272` ha reso misurabile.** Fino ad allora la chiave era
	 * `(TurnNumber, Phase)` perche' la ricostruzione non accettava di piu': due micro-step della stessa fase
	 * avrebbero avuto lo stesso hash per costruzione. Ora `URTReplayStateLibrary::UnitsAtBoundary` accetta la
	 * terna, e il checksum la porta.
	 *
	 * ⚠️ **`INDEX_NONE` non e' l'assenza di un dato: e' la fase intera**, ed e' il boundary a cui appartengono
	 * le voci fuori da un ciclo di micro-step — fra cui **tutte** le `Action.Move`. Sta **dopo** i boundary di
	 * ciclo dello stesso turno e fase, perche' l'arrivo di un'unita' e' posteriore a ogni barriera che ha
	 * attraversato per arrivarci. Vedi `URTReplayStateLibrary::UnitsAtBoundary`.
	 *
	 * ⛔ Vale `INDEX_NONE` su **ogni** boundary quando la traccia e' antecedente a
	 * `ERTTurnLogFormatVersion::WithMicroStep`: li' il campo non esiste, e dichiarare un micro-step `0` che
	 * quella traccia non ha mai scritto sarebbe una misura inventata.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 MicroStepIndex = INDEX_NONE;

	/**
	 * L'hash dello stato **dopo** che la traccia e' stata applicata fino a questo boundary compreso.
	 *
	 * ⚠️ **Non e' il `FinalStateHash`, e non pretende di esserlo.** Vedi il contratto di
	 * `URTBoundaryChecksumLibrary::ChecksumsAlongTrace`: cio' che entra qui e' cio' che la **traccia** puo'
	 * dire, non tutto cio' che `FRTUnitStateDigest` sa portare.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int64 Hash = 0;

	/**
	 * `T2|Blast` per la fase intera, `T1|Move#3` per un micro-step — l'identificatore che un messaggio di
	 * fallimento stampa.
	 *
	 * ⚠️ **Il `#N` compare solo quando il boundary e' di ciclo.** Stamparlo sempre direbbe `#-1` sulle fasi
	 * che un ciclo non ce l'hanno, e `#0` sulle tracce che il campo non lo portano: due modi diversi di
	 * dichiarare una coordinata che non esiste.
	 */
	FString ToString() const;
};

/**
 * Il checksum di stato **per boundary**, e il confronto che nomina il primo punto di divergenza (`#2189`).
 *
 * ## Perche' e' materia del Verifier
 *
 * ⛔ **Il Player continua a non chiamare il resolver** (ADR-0009 §3): niente qui rigioca una partita. Si
 * legge una traccia gia' prodotta e si ricostruisce lo stato che dichiara, con `URTReplayStateLibrary` —
 * cioe' la stessa strada che il playback usa per sapere dove disegnare le unita'.
 *
 * ## 🔴 Cosa questo checksum guarda, e cosa no
 *
 * `FRTUnitStateDigest` ha otto campi; una traccia canonica ne puo' ricostruire **quattro**: `UnitId`,
 * `Cell`, `Facing` e la vita/morte. `Health`, `Shield`, `Energy` e `Statuses` restano al loro default,
 * **uguali per tutte le unita' e per tutte le esecuzioni**, quindi non discriminano: due stati che
 * differissero solo per gli HP produrrebbero lo stesso checksum di boundary.
 *
 * ⚠️ **Non e' un difetto da tappare inferendo quei valori**, ed e' la ragione per cui e' scritto qui invece
 * che taciuto: dedurli dalle voci di danno sarebbe ricostruire uno stato che la traccia non dichiara — cioe'
 * un secondo simulatore, che il guardrail di `#1880` e di questa issue vieta. Chi ha bisogno degli otto
 * campi ha `FinalStateHash`, che nasce dal mondo vivo e resta la misura completa; questo checksum e' il
 * **localizzatore**, e localizza cio' che la traccia sa.
 *
 * \U0001F511 **Nessuna seconda aritmetica di hash**: il calcolo passa da `URTMatchStateHashLibrary::HashMatchState`,
 * verificabile per assenza in questo file — non c'e' un solo `^`, `*` o costante FNV.
 */
UCLASS()
class REFACTORTACTICS_API URTBoundaryChecksumLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * I boundary che la traccia attraversa, in ordine, ciascuno col checksum dello stato **a quel punto**.
	 *
	 * ✅ **Il boundary e' `(TurnNumber, Phase, MicroStepIndex)`** dal `#2374`. La versione precedente si
	 * fermava a due coordinate e lo diceva: *«si aggiunge la terza quando la ricostruzione la accetta, non
	 * prima — un campo che non discrimina e' peggio di un campo assente, perche' sembra una misura»*. `#2272`
	 * ha portato `URTReplayStateLibrary::UnitsAtBoundary`, che la accetta. Questa e' la raccolta di quella
	 * precondizione, non un allargamento opportunistico.
	 *
	 * ## ⛔ `Version` non ha un default, ed e' voluto
	 *
	 * Una traccia antecedente a `ERTTurnLogFormatVersion::WithMicroStep` non porta il campo: la
	 * deserializzazione le lascia `0` su **ogni** voce (`FRTTurnLogEntry::MicroStepIndex`), che li' significa
	 * *«non dichiarato»* e non *«primo micro-step»*. Chiavare su quel `0` produrrebbe boundary etichettati
	 * `T2|Move#0` in tracce che un micro-step non l'hanno mai scritto.
	 *
	 * Sotto `WithMicroStep` questa funzione **degrada a un boundary per fase**, con `MicroStepIndex` a
	 * `INDEX_NONE` — il comportamento storico, dichiarato invece che dedotto. Il parametro e' obbligatorio
	 * perche' un default sceglierebbe **al posto** del chiamante quale delle due misure sta facendo, ed e'
	 * esattamente il fallback silenzioso che questo tipo esiste per non avere.
	 *
	 * ## Cosa NON cambia
	 *
	 * ⛔ Nessun replay chiama il resolver (ADR-0009 §3). ⛔ Gli eventi che condividono la terna restano un
	 * **gruppo simultaneo** e non si riordinano. ⛔ Lo stato a un boundary di ciclo e' **parziale per
	 * costruzione**: a meta' movimento le unita' non hanno ancora la cella finale, ed e' il mondo a quella
	 * barriera — non a fine fase.
	 *
	 * `Initial` e' lo schieramento di partenza: la traccia dichiara i **cambiamenti**, non le posizioni
	 * iniziali, quindi senza di esso non c'e' niente da muovere.
	 */
	static TArray<FRTBoundaryChecksum> ChecksumsAlongTrace(const URTHexMapAsset* Map,
		const TArray<FRTTurnLogEntry>& Entries, const TArray<FRTTracedUnitState>& Initial,
		ERTTurnLogFormatVersion Version);

	/**
	 * L'indice del **primo** boundary in cui due sequenze differiscono; `INDEX_NONE` se coincidono.
	 *
	 * \U0001F511 **E' la domanda che il gate di determinismo non sapeva porre.**
	 * `Replay.Verifier.ReportsFirstDivergence` confronta due **tracce** e trova la prima **voce** diversa:
	 * il sintomo. Questa funzione confronta due **stati** e trova il primo **luogo** diverso — e i due non
	 * coincidono ogni volta che un difetto si manifesta N voci dopo la propria causa.
	 *
	 * ⚠️ **Due sequenze di lunghezza diversa divergono alla prima posizione che una sola delle due ha.** Non
	 * e' un caso limite: un'esecuzione che finisce prima ha attraversato meno boundary, e dire «uguali fin
	 * dove entrambe arrivano» nasconderebbe proprio quella differenza.
	 */
	static int32 FirstDivergence(const TArray<FRTBoundaryChecksum>& A,
		const TArray<FRTBoundaryChecksum>& B);

	/**
	 * Il messaggio che **nomina il boundary**: `«divergono al boundary T2|Blast: 0x… contro 0x…»`, e
	 * `«… al boundary T1|Move#1 …»` quando la divergenza e' dentro un ciclo di micro-step.
	 *
	 * Stringa vuota se le due sequenze coincidono — cosi' chi lo usa in un test scrive
	 * `TestEqual(Describe(...), TEXT(""))` e ottiene, quando fallisce, il luogo invece di un booleano.
	 */
	static FString DescribeDivergence(const TArray<FRTBoundaryChecksum>& A,
		const TArray<FRTBoundaryChecksum>& B);
};
