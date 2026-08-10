#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Replay/RTReplayManifest.h"
#include "Turn/RTTurnLog.h"
#include "RTReplayRecorderLibrary.generated.h"

/**
 * Scrive l'archivio replay deciso da [D-077](../../../docs/decisions/RT_PDR_00_Decision_Log.md): un manifest
 * per partita piu' una traccia per turno (`#469`).
 *
 * ```
 * Replays/<MatchId>/match.rtmanifest    <- questo header, in JSON
 *                   turn-001.rtlog      <- SerializeTurnLog, invariato
 * ```
 *
 * **Scrive, e sa rileggere il proprio formato.** Non decide, non calcola esiti, non ordina — l'ordine canonico e' gia' di `SortTurnLog`,
 * e il recorder che riordinasse produrrebbe byte diversi dalla traccia che la partita ha risolto. Per la
 * stessa ragione la traccia la serializza `SerializeTurnLog` e non un secondo serializzatore scritto qui:
 * e' l'unico modo di rendere vero il criterio «byte-identiche», invece di sperarlo.
 *
 * ⚠️ **Il recorder sta dal lato di chi produce, non di chi riproduce**, e non contraddice
 * [ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §3: quel confine dice che chi
 * **riproduce** non chiama il resolver. Scrivere non e' riprodurre, e questa libreria non riproduce niente.
 *
 * ⚠️ **La rilettura che c'e' qui e' del FORMATO, non della partita.** `ManifestFromJson` e `LoadManifest`
 * servono a chi ha scritto — per riprendere, per diagnosticare, e ai test — e non fanno di questa classe un
 * Player: aprire un archivio per **riprodurlo** e' R3, e quando arrivera' usera' queste funzioni invece di
 * riscriverle. Se un giorno il Player dovesse vivere in un modulo separato dal produttore, e' questa la
 * coppia da spostare per prima.
 *
 * Libreria pura di funzioni statiche: nessun Actor, nessun World, nessuno stato nascosto fra una chiamata e
 * l'altra. Il chiamante tiene il manifest e lo passa; cosi' i test girano senza una partita.
 */
UCLASS()
class REFACTORTACTICS_API URTReplayRecorderLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Il manifest come JSON. Versionato dal primo campo: vedi `ERTReplayManifestVersion`. */
	static FString ManifestToJson(const FRTReplayManifest& Manifest);

	/**
	 * Rilegge un manifest. `false` = non e' leggibile, e `OutManifest` resta com'era.
	 *
	 * Fail-closed sulle versioni sconosciute: rifiutare invece di interpretare campi arbitrari e' la
	 * convenzione che `DeserializeTurnLog` applica gia' al formato binario, ed e' cio' che
	 * [ADR-0009](../../../docs/decisions/adr-0009-replay-logico-canonico.md) §4 chiede al Player in apertura.
	 */
	static bool ManifestFromJson(const FString& Json, FRTReplayManifest& OutManifest);

	/** Nome del file di traccia di un turno: `turn-001.rtlog`. Zero-padded, cosi' l'ordine alfabetico dei
	 *  file coincide con quello dei turni — un elenco di cartella e' spesso il primo strumento di diagnosi. */
	static FString TurnFileName(int32 TurnNumber);

	/** Cartella di una partita dentro la radice degli archivi. */
	static FString MatchDirectory(const FString& ReplaysRoot, const FGuid& MatchId);

	/**
	 * Scrive la traccia di un turno e ne registra l'hash ordinato nel manifest, che aggiorna in luogo.
	 *
	 * Scrive **durante** il match e non alla fine: e' la scelta che rende utile un archivio quando il gioco
	 * muore a meta', che e' esattamente il caso in cui un replay serve.
	 */
	static bool RecordTurn(const FString& ReplaysRoot, FRTReplayManifest& Manifest, int32 TurnNumber,
		const TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Chiude il manifest e lo scrive: da qui in poi l'archivio e' completo.
	 *
	 * Finche' questa non viene chiamata `bClosed` resta `false`, ed e' cosi' che un archivio parziale si
	 * dichiara tale senza un secondo meccanismo.
	 */
	static bool CloseMatch(const FString& ReplaysRoot, FRTReplayManifest& Manifest, ERTMatchOutcome Outcome,
		int64 FinalStateHash, float WallClockSeconds);

	/** Rilegge il manifest di una partita dall'archivio. `false` = assente o illeggibile. */
	static bool LoadManifest(const FString& ReplaysRoot, const FGuid& MatchId, FRTReplayManifest& OutManifest);
};
