#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnLog.h"
#include "RTTurnLogLibrary.generated.h"

/**
 * Ordinamento e hash del TurnLog per l'osservabilita'/replay. Pura, deterministica, SOLO interi
 * (invariante #4: niente float). L'hash e' usato per la verifica di replay ("replay divergence = 0"),
 * mai per la logica di gioco.
 */
UCLASS()
class REFACTORTACTICS_API URTTurnLogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Ordine TOTALE deterministico fra due voci: Phase -> Category -> SrcCell(X,Y,Layer) ->
	 * TgtCell(X,Y,Layer) -> Outcome -> Amount. Vero se A precede B. Con un ordine totale, riordinare
	 * un insieme di voci da' sempre la stessa sequenza, indipendentemente dall'ordine d'inserimento.
	 */
	static bool EntryLess(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B);

	/** Ordina il TurnLog in place con EntryLess (ordine totale deterministico). */
	static void SortTurnLog(TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Hash intero (FNV-1a sui campi interi) del TurnLog, PERMUTAZIONE-INVARIANTE (ordina con EntryLess
	 * prima di mescolare). Deterministico, solo interi. Uso: verifica di replay, mai logica di gioco.
	 */
	static uint32 HashTurnLog(const TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Serializza il TurnLog in un buffer binario VERSIONATO e in forma CANONICA (ordina con SortTurnLog
	 * prima di scrivere -> byte permutazione-invarianti, come l'hash). Header: magic + versione +
	 * topologia (flags) + conteggio; poi ogni voce come interi little-endian espliciti (invariante #4).
	 * Topology dichiara come vanno lette le celle (offset quadrate o assiali esagonali); il default Square
	 * scrive flags = 0, quindi i byte restano identici a quelli prodotti prima di questa estensione.
	 */
	static TArray<uint8> SerializeTurnLog(const TArray<FRTTurnLogEntry>& Entries,
		ERTLogTopology Topology = ERTLogTopology::Square);

	/**
	 * Ricostruisce il TurnLog da un buffer prodotto da SerializeTurnLog. Ritorna false (fail-closed, nessun
	 * crash) se magic/versione/topologia non riconosciuti o buffer troncato; in tal caso OutEntries e' svuotato.
	 * Se OutTopology != nullptr riceve la topologia dichiarata nel file.
	 * Round-trip garantito a livello di hash: HashTurnLog(in) == HashTurnLog(out).
	 */
	static bool DeserializeTurnLog(const TArray<uint8>& Bytes, TArray<FRTTurnLogEntry>& OutEntries,
		ERTLogTopology* OutTopology = nullptr);

	/**
	 * Salva il TurnLog su file (serializzazione binaria versionata + checksum). Ritorna false se la
	 * scrittura fallisce. Thin wrapper su SerializeTurnLog + FFileHelper.
	 */
	static bool SaveTurnLogToFile(const FString& Path, const TArray<FRTTurnLogEntry>& Entries,
		ERTLogTopology Topology = ERTLogTopology::Square);

	/**
	 * Carica il TurnLog da file. Ritorna false (fail-closed, OutEntries svuotato) se il file manca o il
	 * contenuto e' invalido/corrotto (magic/versione/topologia/troncamento/checksum). Se OutTopology != nullptr
	 * riceve la topologia dichiarata nel file. Wrapper su FFileHelper + DeserializeTurnLog.
	 */
	static bool LoadTurnLogFromFile(const FString& Path, TArray<FRTTurnLogEntry>& OutEntries,
		ERTLogTopology* OutTopology = nullptr);
};
