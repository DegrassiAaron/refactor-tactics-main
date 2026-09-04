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
	 * L'hash dello stato **dopo** che la traccia e' stata applicata fino a questo boundary compreso.
	 *
	 * ⚠️ **Non e' il `FinalStateHash`, e non pretende di esserlo.** Vedi il contratto di
	 * `URTBoundaryChecksumLibrary::ChecksumsAlongTrace`: cio' che entra qui e' cio' che la **traccia** puo'
	 * dire, non tutto cio' che `FRTUnitStateDigest` sa portare.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int64 Hash = 0;

	/** `T2|Blast` — l'identificatore che un messaggio di fallimento stampa. */
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
 * `FRTUnitStateDigest` ha sette campi; una traccia canonica ne puo' ricostruire **quattro**: `UnitId`,
 * `Cell`, `Facing` e la vita/morte. `Health`, `Shield` e `Statuses` restano al loro default,
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
	 * ⚠️ **Il boundary di oggi e' `(TurnNumber, Phase)`.** `#1880` ha reso il micro-step indirizzabile —
	 * `FRTTurnLogEntry::MicroStepIndex` esiste e `URTReplaySeekLibrary::SeekToBoundary` lo cerca — ma
	 * `URTReplayStateLibrary::UnitsAtPosition` ricostruisce lo stato per turno e fase, e finche' e' cosi'
	 * un checksum per micro-step avrebbe una chiave piu' fine dello stato che misura: due boundary diversi
	 * con lo stesso hash, per costruzione. ⛔ Si aggiunge la terza coordinata **quando la ricostruzione la
	 * accetta**, non prima — un campo che non discrimina e' peggio di un campo assente, perche' sembra una
	 * misura.
	 *
	 * `Initial` e' lo schieramento di partenza: la traccia dichiara i **cambiamenti**, non le posizioni
	 * iniziali, quindi senza di esso non c'e' niente da muovere.
	 */
	static TArray<FRTBoundaryChecksum> ChecksumsAlongTrace(const URTHexMapAsset* Map,
		const TArray<FRTTurnLogEntry>& Entries, const TArray<FRTTracedUnitState>& Initial);

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
	 * Il messaggio che **nomina il boundary**: `«divergono al boundary T2|Blast: 0x… contro 0x…»`.
	 *
	 * Stringa vuota se le due sequenze coincidono — cosi' chi lo usa in un test scrive
	 * `TestEqual(Describe(...), TEXT(""))` e ottiene, quando fallisce, il luogo invece di un booleano.
	 */
	static FString DescribeDivergence(const TArray<FRTBoundaryChecksum>& A,
		const TArray<FRTBoundaryChecksum>& B);
};
