#pragma once

#include "CoreMinimal.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnRules.h"
#include "RTReplayManifest.generated.h"

/**
 * Versione del formato del manifest. Versionato dal primo byte anche se oggi ha pochi campi, perche' il
 * TurnLog ha gia' dimostrato cosa succede altrimenti: e' alla `v7` e le versioni dalla 2 in poi restano
 * tutte leggibili **solo** perche' ogni estensione ha accodato campi invece di inserirli in mezzo.
 *
 * ⚠️ Non e' UENUM (uint16 esce dai vincoli UHT del BlueprintType uint8): e' una costante di formato interna,
 * come `ERTTurnLogFormatVersion`.
 */
enum class ERTReplayManifestVersion : uint16
{
	/** Header di partita, hash ordinati per turno, esito, chiusura. Nessun campo di compatibilita' (#413). */
	Initial = 1,

	/**
	 * Versione che questo binario SCRIVE. Chi aggiunge un campo alza questo alias e lascia in piedi il valore
	 * storico sopra, cosi' l'elenco resta la storia del formato invece di una riga riscritta ogni volta.
	 *
	 * ⚠️ **Un numero di versione e' una risorsa contesa** (`#471`): prima di prenderlo, controllare TUTTI i
	 * branch remoti e non solo `main`. La `v6` del TurnLog fu rivendicata da due branch insieme, e il
	 * duplicato non si rinumera da solo — corrompe tracce gia' scritte.
	 */
	Current = Initial
};

/**
 * Header di una partita registrata: l'indice dei suoi turni e cio' che serve a nominarla senza aprirla.
 *
 * E' l'artefatto deciso da [D-077](../../../docs/decisions/RT_PDR_00_Decision_Log.md) — «un manifest per
 * partita piu' una traccia per turno» — e non e' un file in piu': e' **lo stesso** che l'indice di `#416`
 * chiede, e la casa che `D-062` aveva gia' assegnato a `HashTurnLogOrdered` senza che esistesse.
 *
 * JSON e non binario, per una ragione operativa: il payload sono le tracce, questo sono **metadati**, e un
 * archivio rotto lo si diagnostica leggendo l'header a occhio. `Json`/`JsonUtilities` sono gia' dipendenze
 * del modulo — l'harness le usa per i report — quindi non ne entra una nuova.
 */
USTRUCT(BlueprintType)
struct FRTReplayManifest
{
	GENERATED_BODY()

	/**
	 * Identita' della REGISTRAZIONE, non del contenuto (`D-077`): un `FGuid` generato all'avvio della
	 * partita. Il contenuto ha gia' i suoi hash, e derivare l'id dal setup farebbe collidere due partite
	 * giocate davvero due volte.
	 *
	 * ⚠️ **Fuori da ogni hash**, come il wall-clock. Cambiarlo non cambia nessun hash di nessuna traccia.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FGuid MatchId;

	/** Formato di partita in vigore (`Format.Skirmish2v2`): la stessa identita' che l'header del TurnLog porta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	FName FormatId;

	/** `true` = hex. Le celle di una traccia quadrata non significano la stessa cosa: senza, un confronto mente. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	bool bHexTopology = true;

	/**
	 * `HashTurnLogOrdered` di ogni turno, in ordine di turno. `D-062` gli aveva assegnato «l'header del
	 * Replay Archive», e `D-077` ha dato a quell'header questa forma: non e' ricalcolabile dai byte della
	 * traccia, perche' quelli sono in forma canonica (`D-SR-1`) e hanno perso l'ordine di append.
	 *
	 * `int64` e non `uint32` perche' `UPROPERTY` non supporta `uint32`: i valori sono hash a 32 bit senza
	 * segno, e l'allargamento e' una zero-extension che li preserva — `0xFFFFFFFF` diventa `4294967295`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	TArray<int64> OrderedHashPerTurn;

	/**
	 * Checksum dello stato di fine partita. `0` = non calcolato — vedi `bClosed`.
	 *
	 * ⚠️ Il tipo e' `int64` per lo stesso vincolo di `UPROPERTY`, ma il valore che ci arriva e' un **uint32**:
	 * `HashMatchState` ritorna 32 bit. Se un giorno il checksum diventasse davvero a 64 bit, questo campo
	 * andrebbe scritto nel JSON come **stringa**, perche' oltre 2^53 un numero JSON e' un double e comincia
	 * ad arrotondare in silenzio.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int64 FinalStateHash = 0;

	/** Esito, quando la partita e' finita. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	ERTMatchOutcome Outcome = ERTMatchOutcome::InProgress;

	/** Durata in secondi reali. Vive SOLO qui e in nessun campo che entri in un hash. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	float WallClockSeconds = 0.f;

	/**
	 * `false` = la partita non e' arrivata alla fine: crash, uscita, interruzione.
	 *
	 * E' cosi' che un archivio **parziale si dichiara tale** senza bisogno di un secondo meccanismo: il
	 * recorder scrive le tracce **durante** il match e chiude il manifest solo a partita conclusa, quindi un
	 * manifest non chiuso *e'* la dichiarazione di parzialita'. Chi lo legge sa che `Outcome`,
	 * `FinalStateHash` e la durata non sono stati scritti, invece di leggerli come «pareggio, hash zero».
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	bool bClosed = false;

	/** Numero di turni registrati. Ridondante con `OrderedHashPerTurn.Num()`, e serve: un manifest parziale
	 *  puo' avere piu' tracce su disco che hash scritti, e la differenza e' diagnostica. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Replay")
	int32 TurnCount = 0;
};
