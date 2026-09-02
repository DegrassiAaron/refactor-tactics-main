#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnLogLibrary.h"
#include "RTDebugReportLibrary.generated.h"

struct FRTPlannedIntent;
struct FRTTurnLogEntry;
struct FRTHexCellData;
struct FRTHexSnapshot;
struct FRTCellId;
struct FRTOccupancyMask;
struct FRTPlacementRegion;

/**
 * L'esito di `rt.Debug.VerifyReplay`: il VERDETTO separato dalle righe che lo stampano.
 *
 * ⚠️ **`Comparison` non e' un booleano, e la differenza e' il punto.** Due tracce con formati o topologie
 * diverse **non sono confrontabili**, e uno strumento che le dichiarasse «divergenti» mentirebbe: la
 * divergenza e' una proprieta' di due tracce dello stesso contesto. `ERTTraceComparison` codifica gia'
 * questa distinzione e qui si riporta invece di riassumerla.
 */
USTRUCT()
struct FRTDebugReplayVerdict
{
	GENERATED_BODY()

	/** Esito del confronto, riportato tale e quale da `CompareSerializedTraces`. */
	ERTTraceComparison Comparison = ERTTraceComparison::Identical;

	/** La prima divergenza, o stringa vuota. Popolata **solo** quando `Comparison == Divergence`. */
	FString FirstDivergence;

	/** Le righe da stampare. Il comando le passa a `FOutputDevice`; il test guarda i due campi sopra. */
	TArray<FString> Lines;
};

/**
 * Il CONTENUTO degli otto comandi `rt.Debug.*` (CP 11.4, `#80`), separato dai comandi che lo stampano.
 *
 * **Perche' la separazione esiste.** Un `FAutoConsoleCommand` legge un `UWorld` e scrive su un
 * `FOutputDevice`: non e' verificabile headless, e un DoD che chiedesse solo «il comando esiste» si
 * chiuderebbe con uno strumento che stampa la cosa sbagliata. Qui vivono funzioni **pure** che compongono
 * le righe; i comandi in `RTDebugConsole.cpp` sono wrapper sottili sopra di esse. E' la stessa forma di
 * `ARTHUD::ComputePlannedHitMarks` e `ComposeSlotLines`.
 *
 * ⚠️ Vale anche per i quattro comandi che DISEGNANO: l'etichetta di una cella e' testo, e il testo si
 * verifica qui. Che la linea compaia a schermo resta `PIE-V01-DEBUG` (seduta U15), e nessun test di questo
 * file lo dimostra.
 *
 * **Sola lettura**: nessuna funzione qui tocca lo stato di gioco. Uno strumento di ispezione che modifica
 * cio' che ispeziona non e' uno strumento di ispezione.
 */
UCLASS()
class REFACTORTACTICS_API URTDebugReportLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Le righe di `rt.Debug.DrawIntent` per un osservatore, una per intento visibile.
	 *
	 * 🔴 **Passa da `URTIntentPrivacyLibrary::FilterForTeam` e non riscrive la regola.** E' il punto del
	 * checkpoint: il DoD chiede che l'invariante #6 valga «anche negli strumenti di debug», e uno
	 * strumento di debug e' precisamente dove la tentazione di stampare la lista intera e' massima.
	 * Comporre da `Intents` invece che dalla vista filtrata renderebbe questo comando l'unico punto del
	 * gioco da cui un avversario si legge — e non lo direbbe nessun test che guardi solo il DTO.
	 *
	 * Pinnata da `RefactorTactics.Debug.DrawIntentHidesEnemyIntent`, che verifica anche il canale
	 * laterale: due scene identiche per l'osservatore danno lo stesso output quali che siano i piani altrui.
	 */
	static TArray<FString> DescribeIntents(int32 ObserverTeamId, const TArray<FRTPlannedIntent>& Intents);

	/**
	 * Il verdetto di `rt.Debug.VerifyReplay`: la traccia appena prodotta coincide con quella di riferimento?
	 *
	 * Serializza la traccia corrente — e' `CompareSerializedTraces` a pretendere byte — poi localizza la
	 * prima divergenza con `DescribeFirstDivergence`. Nessuna delle due regole viene riscritta qui: uno
	 * strumento che rispondesse «uguali» con un criterio proprio direbbe una cosa diversa dal gate `G4`,
	 * e sarebbe peggio di non averlo.
	 *
	 * ⚠️ **Il riferimento arriva gia' in BYTE, la traccia corrente in voci.** L'asimmetria e' voluta e
	 * corrisponde all'uso reale: il golden e' un file, la partita e' in memoria. Una firma che prendesse
	 * due `TArray<FRTTurnLogEntry>` e le serializzasse entrambe qui non potrebbe **mai** produrre
	 * `FormatMismatch` — avrebbero per costruzione lo stesso contesto — e lo strumento perderebbe proprio
	 * il caso che deve saper distinguere da una divergenza.
	 *
	 * `GoldenEntries` serve solo a `DescribeFirstDivergence`, che confronta voci e non byte: se e' vuoto
	 * la divergenza viene rilevata lo stesso, ma non localizzata. Il verdetto lo dichiara invece di
	 * tacerlo.
	 *
	 * Pinnata da `RefactorTactics.Debug.VerifyReplayDetectsDivergence`.
	 */
	static FRTDebugReplayVerdict VerifyReplay(const TArray<uint8>& GoldenBytes,
		const TArray<FRTTurnLogEntry>& GoldenEntries, const TArray<FRTTurnLogEntry>& Actual,
		ERTLogTopology Topology, FName FormatId);

	/**
	 * Una cella con i **sette campi** che il DoD di #80 elenca, in una riga.
	 *
	 * ⚠️ **`OccupantUnitId` e `Revision` arrivano da fuori, e non e' un dettaglio di comodo**: nel modello
	 * dati l'occupante vive in `FRTHexSnapshot::Occupancy` e la revisione in `URTHexMapAsset::Revision`.
	 * Una firma che prendesse la sola cella non potrebbe mostrarli — ed e' il motivo per cui il DoD, che
	 * li elenca sotto «le celle mostrano», andava riconciliato prima di scrivere il codice.
	 *
	 * `INDEX_NONE` per `OccupantUnitId` significa **cella libera**, e si stampa come tale: `0` e' un
	 * UnitId valido e usarlo da sentinella confonderebbe «vuota» con «ci sta l'unita' zero».
	 *
	 * Pinnata da `RefactorTactics.Debug.CellReportCarriesEveryDeclaredField`.
	 */
	static FString DescribeCell(const FRTHexCellData& Cell, int32 OccupantUnitId, int32 Revision);

	/**
	 * Una voce di TurnLog con gli **otto campi** che il DoD elenca per le azioni.
	 *
	 * La coda narrativa viene da `URTTurnLogLibrary::DescribeEntry`, che resta l'owner della traduzione
	 * degli esiti: riscriverla qui darebbe due frasi diverse per lo stesso evento — una nel combat log e
	 * una nel dump — e chi le confrontasse non saprebbe quale credere.
	 *
	 * `SequenceIndex` e' l'`EventSequence` del DoD: nel modello non esiste un campo, l'ordine **e'**
	 * l'indice nell'array. Passarlo esplicitamente lo rende visibile invece di sottinteso.
	 *
	 * Pinnata da `RefactorTactics.Debug.ActionReportCarriesEveryDeclaredField`.
	 */
	static FString DescribeLogEntry(const FRTTurnLogEntry& Entry, int32 SequenceIndex);

	/** Le righe di `rt.Debug.DumpSnapshot`: intestazione, unita' e celle notevoli dello snapshot. */
	static TArray<FString> DescribeSnapshot(const FRTHexSnapshot& Snapshot);

	/** Le righe di `rt.Debug.DumpTurnLog`: una per voce, piu' un'intestazione col conteggio e l'hash. */
	static TArray<FString> DescribeTurnLogEntries(const TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Le righe di `rt.Debug.DumpCellPlacement`: la maschera dei dodici settori di una cella e le sue
	 * regioni libere, ciascuna col proprio `FirstWedge` e `Size`.
	 *
	 * 🔑 **Esiste perche' [#1826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1826) lo
	 * chiedeva e non c'era**: la sua stessa sezione *Debug / log evidence* dichiarava *«oggi non esiste
	 * alcun comando che lo faccia, e la misura di questa issue e' stata ottenuta da un test»*. Una regola
	 * che si puo' osservare solo da un test e' una regola che, in partita, nessuno puo' guardare.
	 *
	 * ⚠️ **`Regions` si passa invece di ricalcolarla qui.** Questa funzione formatta e non decide: se
	 * chiamasse `ComputeFreeRegions` da se', due chiamanti potrebbero stampare regioni diverse da quelle
	 * che hanno usato. Il comando calcola una volta e passa cio' che ha calcolato.
	 *
	 * Pinnata da `RefactorTactics.Debug.CellPlacementReportShowsMaskAndRegions`.
	 *
	 * 🔴 **`Covers` non e' un di piu': senza, questo report MENTE per omissione.** Un raggio disegnato dal
	 * centro al punto medio di un lato viene scritto dal bake come copertura di **bordo** — `EdgesTouchedBy`
	 * non esce vuoto — e la maschera dei settori non la porta. Il comando rispondeva quindi *«zero settori,
	 * zero muri interni»* a una cella che geometria ne aveva: vero campo per campo, e falso come risposta.
	 * Misurato in seduta PIE il 2026-09-02, dove e' costato mezz'ora di diagnosi nella direzione sbagliata.
	 */
	static TArray<FString> DescribeCellPlacement(const FRTCellId& Cell, const FRTOccupancyMask& Mask,
		const TArray<FRTPlacementRegion>& Regions, const TArray<FRTHexCover>& Covers);
};
