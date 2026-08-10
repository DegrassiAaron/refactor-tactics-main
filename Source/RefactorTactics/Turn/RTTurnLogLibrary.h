#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnLog.h"
#include "RTTurnLogLibrary.generated.h"

/**
 * Esito del confronto fra due tracce serializzate. I casi di CONTESTO (formato, topologia) sono distinti
 * dalla divergenza vera: due tracce prodotte con formati diversi non sono un bug del codice, e chiamarle
 * "divergenza" manderebbe a cercare un difetto dove c'e' una configurazione diversa.
 * Non e' UENUM: non serve ai Blueprint, e' un verdetto di libreria.
 */
enum class ERTTraceComparison : uint8
{
	/** Stesso formato, stessa topologia, stesse voci. */
	Identical,
	/** Le tracce dichiarano formati di partita diversi: non sono confrontabili. */
	FormatMismatch,
	/** Le tracce dichiarano topologie diverse: le celle non significano la stessa cosa. */
	TopologyMismatch,
	/** Stesso contesto, voci diverse: **questa** e' una divergenza. */
	Divergence,
	/** Almeno una delle due non e' una traccia valida (magic/versione/troncamento/checksum). */
	Unreadable
};

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
	 * TgtCell(X,Y,Layer) -> Outcome -> Amount -> ActionId -> TurnNumber -> GraphRevision -> UnitId.
	 * Vero se A precede B. Con un ordine totale, riordinare un insieme di voci da' sempre la stessa
	 * sequenza, indipendentemente dall'ordine d'inserimento.
	 *
	 * La catena copre OGNI campo che `SerializeTurnLog` scrive, e deve continuare a farlo: un campo scritto
	 * che il confronto non guarda lascia due voci a pari merito, e a decidere l'ordine resta un sort non
	 * stabile — cioe' due file diversi con lo stesso contenuto. Unica eccezione: `BaseActionId`, che e'
	 * funzione di `ActionId` e non puo' produrre pareggi.
	 *
	 * ⚠️ NON coincide con i campi dell'hash: `UnitId` e `TurnNumber` stanno qui e non in `MixEntryFields`.
	 * Chi vuole «uguali per l'hash» non usi questa funzione (vedi `GoldenEntriesMatch`).
	 */
	static bool EntryLess(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B);

	/** Ordina il TurnLog in place con EntryLess (ordine totale deterministico). */
	static void SortTurnLog(TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Descrizione leggibile di una voce, con le celle in coordinate ASSIALI `(q=..,r=..,L=..)` e il reason
	 * code tradotto in italiano. Serve al combat log: senza, l'esito di un turno resta leggibile solo nel
	 * TurnLog binario e il giocatore non sa PERCHE' l'unita' non si e' mossa o il colpo non e' partito.
	 * Pura (nessun Actor, nessuno stato): il chiamante decide dove mostrarla.
	 */
	static FString DescribeEntry(const FRTTurnLogEntry& Entry);

	/**
	 * L'identita' dell'azione di una voce, come **azione base + profilo** quando la voce sa dirlo:
	 * `Action.BasicAttack · Bastion.ImpactShot`. E' la forma che [D-033](../../../docs/decisions/RT_PDR_00_Decision_Log.md)
	 * chiede — «spiegabile nel TurnLog come azione base + profilo» — e senza di essa una traccia dice solo
	 * `Bastion.ImpactShot`, che e' un'azione d'EROE e non si risolve col catalogo core.
	 *
	 * Ricade sul solo `ActionId` quando `BaseActionId` e' vuoto (traccia di formato < 5, o azione che non e'
	 * profilo di niente) o quando i due coincidono. Pura: legge solo la voce.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|TurnLog")
	static FString DescribeActionIdentity(const FRTTurnLogEntry& Entry);

	/**
	 * Hash intero (FNV-1a sui campi interi) del TurnLog, PERMUTAZIONE-INVARIANTE (ordina con EntryLess
	 * prima di mescolare). Deterministico, solo interi. Uso: verifica di replay, mai logica di gioco.
	 */
	static uint32 HashTurnLog(const TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Hash intero del TurnLog SENSIBILE ALL'ORDINE DI APPEND: mescola le voci come sono arrivate, senza
	 * ordinarle. Stessi campi di `HashTurnLog`, stesso FNV-1a: cambia solo che non c'e' il sort davanti.
	 *
	 * Esiste perche' `HashTurnLog` e' cieco al riordino **per scelta**, e quella cecita' lascia scoperto un
	 * difetto reale: un ciclo che iterasse una `TMap` cambierebbe la sequenza emessa dal resolver senza
	 * cambiare un solo hash, e nessun corpus golden se ne accorgerebbe. I due hash rispondono a domande
	 * diverse e servono entrambi ([D-062](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 *
	 * ⚠️ **Non e' ricalcolabile da un file.** `SerializeTurnLog` scrive in forma canonica (ordinata), quindi
	 * i byte perdono l'ordine di emissione: questo hash si calcola sulla traccia IN MEMORIA, prima di
	 * serializzare, e chi lo vuole conservare lo mette nel proprio header — non in quello del TurnLog, che
	 * deve restare permutazione-invariante (`D-SR-1`, e il test `SerializeCanonicalPermutationInvariant`).
	 */
	static uint32 HashTurnLogOrdered(const TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Serializza il TurnLog in un buffer binario VERSIONATO e in forma CANONICA (ordina con SortTurnLog
	 * prima di scrivere -> byte permutazione-invarianti, come l'hash). Header: magic + versione +
	 * topologia (flags) + conteggio; poi ogni voce come interi little-endian espliciti (invariante #4).
	 * Topology dichiara come vanno lette le celle (offset quadrate o assiali esagonali); il default Square
	 * scrive flags = 0, quindi i byte restano identici a quelli prodotti prima di questa estensione.
	 */
	static TArray<uint8> SerializeTurnLog(const TArray<FRTTurnLogEntry>& Entries,
		ERTLogTopology Topology = ERTLogTopology::Square, FName FormatId = NAME_None);

	/**
	 * Ricostruisce il TurnLog da un buffer prodotto da SerializeTurnLog. Ritorna false (fail-closed, nessun
	 * crash) se magic/versione/topologia non riconosciuti o buffer troncato; in tal caso OutEntries e' svuotato.
	 * Se OutTopology != nullptr riceve la topologia dichiarata nel file; se OutFormatId != nullptr riceve
	 * l'identita' del formato (`NAME_None` per le tracce scritte prima della versione 4).
	 * Round-trip garantito a livello di hash: HashTurnLog(in) == HashTurnLog(out).
	 */
	static bool DeserializeTurnLog(const TArray<uint8>& Bytes, TArray<FRTTurnLogEntry>& OutEntries,
		ERTLogTopology* OutTopology = nullptr, FName* OutFormatId = nullptr);

	/**
	 * Salva il TurnLog su file (serializzazione binaria versionata + checksum). Ritorna false se la
	 * scrittura fallisce. Thin wrapper su SerializeTurnLog + FFileHelper.
	 */
	static bool SaveTurnLogToFile(const FString& Path, const TArray<FRTTurnLogEntry>& Entries,
		ERTLogTopology Topology = ERTLogTopology::Square, FName FormatId = NAME_None);

	/**
	 * Carica il TurnLog da file. Ritorna false (fail-closed, OutEntries svuotato) se il file manca o il
	 * contenuto e' invalido/corrotto (magic/versione/topologia/troncamento/checksum). Se OutTopology != nullptr
	 * riceve la topologia dichiarata nel file; se OutFormatId != nullptr l'identita' del formato.
	 * Wrapper su FFileHelper + DeserializeTurnLog.
	 */
	static bool LoadTurnLogFromFile(const FString& Path, TArray<FRTTurnLogEntry>& OutEntries,
		ERTLogTopology* OutTopology = nullptr, FName* OutFormatId = nullptr);

	/**
	 * Confronta due tracce serializzate verificando il CONTESTO prima del contenuto: formato, poi topologia,
	 * poi le voci (per hash). L'hash non copre formato e topologia — per non invalidare gli hash golden gia'
	 * registrati — quindi deve coprirli questa procedura, o il falso "nessuna divergenza" ricompare un piano
	 * piu' su (issue #185, spec §16.3).
	 */
	static ERTTraceComparison CompareSerializedTraces(const TArray<uint8>& A, const TArray<uint8>& B);

	/**
	 * Descrive la PRIMA divergenza fra una traccia di riferimento e quella appena prodotta, oppure una stringa
	 * vuota se coincidono (CP 12.6, #178).
	 *
	 * `CompareSerializedTraces` risponde al livello del formato — *sono diverse* — e per un corpus golden non
	 * basta: il DoD chiede che una divergenza indichi **turno, fase e `ActionId`**, perche' un test che dice
	 * solo «hash diverso» costringe chi legge a ricostruire da capo *dove*, ed e' cosi' che un corpus finisce
	 * rigenerato invece che letto.
	 *
	 * Il TURNO viene passato dal chiamante e non letto da `FRTTurnLogEntry::TurnNumber` (che dal formato v6
	 * esiste, ma nessun produttore lo valorizza e le tracce < v6 lo portano a `0`): il chiamante lo sa, che
	 * lo conosce: nel corpus e' il file da cui la traccia di riferimento e' stata letta.
	 *
	 * Riporta la PRIMA differenza e non tutte: dopo la prima, le successive sono spesso conseguenze.
	 */
	static FString DescribeFirstDivergence(int32 TurnNumber, const TArray<FRTTurnLogEntry>& Golden,
		const TArray<FRTTurnLogEntry>& Actual);
};
