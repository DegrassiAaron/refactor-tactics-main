#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h" // FRTCellId nelle strutture di sonda (#3107)
#include "Turn/RTTurnLogLibrary.h"
#include "RTDebugReportLibrary.generated.h"

struct FRTPlannedIntent;
struct FRTTurnLogEntry;
struct FRTHexCellData;
struct FRTHexSnapshot;
struct FRTCellId;
struct FRTOccupancyMask;
struct FRTPlacementRegion;
struct FRTBoundaryChecksum;

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

/** Un'unita' in campo, ridotta a cio' che serve per sapere se un tiro sarebbe rifiutato — `#3107`. */
USTRUCT()
struct FRTRefusalProbeUnit
{
	GENERATED_BODY()

	UPROPERTY()
	int32 UnitId = INDEX_NONE;

	UPROPERTY()
	int32 TeamId = INDEX_NONE;

	UPROPERTY()
	FRTCellId Cell;

	/**
	 * Se l'osservatore la conosce. ⛔ **Si LEGGE da `ARTUnit::IsKnownToObserver()`, non si ricostruisce**:
	 * il velo ha un produttore solo (`ARTHUD::UpdateObserverVeil`) e una seconda vista di conoscenza qui
	 * sarebbe la risposta che diverge — la stessa disciplina che `DispatchUnitClick` dichiara.
	 */
	UPROPERTY()
	bool bKnownToObserver = false;
};

/**
 * Se un rifiuto per COPERTURA sia raggiungibile adesso, e con quali unita' — `#3107`.
 *
 * 🔑 **Esiste perche' il caso non si lascia trovare a occhio.** La seduta del 2026-09-12 ci ha provato
 * cinque volte in partita libera senza riuscirci: il rifiuto `Cover` vive in una fessura — la conoscenza e'
 * di SQUADRA, la linea di tiro e' per UNITA' — e serve un compagno che veda mentre un altro non ha la
 * linea. Nessuna delle due meta' si vede guardando lo schermo.
 */
USTRUCT()
struct FRTRefusalReachability
{
	GENERATED_BODY()

	UPROPERTY()
	bool bReachable = false;

	/** L'unita' da SELEZIONARE: e' quella senza linea di tiro. */
	UPROPERTY()
	int32 ShooterUnitId = INDEX_NONE;

	/** L'unita' da CLICCARE. */
	UPROPERTY()
	int32 TargetUnitId = INDEX_NONE;

	/** Chi rende noto il bersaglio: e' la meta' del caso che nessuno vede guardando il campo. */
	UPROPERTY()
	int32 WitnessUnitId = INDEX_NONE;

	UPROPERTY()
	FRTCellId ShooterCell;

	UPROPERTY()
	FRTCellId TargetCell;

	/**
	 * Perche' NON e' raggiungibile, quando non lo e'.
	 *
	 * ⛔ **Non e' decorazione**: un comando che rispondesse solo «no» lascerebbe chi legge davanti alle
	 * stesse cinque ipotesi che hanno bruciato la seduta — nessun nemico noto? tutti in vista? nessuna
	 * unita' propria? La ragione e' la meta' utile della risposta negativa.
	 */
	UPROPERTY()
	FString Reason;
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
/**
 * Le due modalita' del Context Inspector (#2485, handoff §4C).
 *
 * ⚠️ **L'enumeratore `Technical` esiste anche in Shipping; il suo CONTENUTO no.** Togliere il valore
 * dall'enum spezzerebbe ogni chiamante per configurazione — un difetto peggiore di quello che eviterebbe
 * — mentre la guardia che conta e' sulle righe che compone.
 */
UENUM(BlueprintType)
enum class ERTContextView : uint8
{
	/** Solo cio' che questo osservatore ha diritto di sapere. */
	Player,

	/** In piu': la provenienza della vista. Compilata fuori da Shipping. */
	Technical
};

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
	 * Una cella con i **sette campi** che il DoD di #80 elenca, in una riga, **per un osservatore**.
	 *
	 * 🔴 **La firma prende lo snapshot intero, e prima di #2485 prendeva `OccupantUnitId` e `Revision`
	 * gia' estratti.** Quei due `int32` erano corretti e **senza provenienza**: questa funzione non poteva
	 * distinguere un occupante che veniva da uno snapshot onnisciente da uno che veniva da uno snapshot di
	 * squadra, quindi un errore del CHIAMANTE — comporre una vista giocatore da una fonte onnisciente —
	 * era invisibile e non provabile. `DescribeIntents` non ha quel difetto perche' prende
	 * `ObserverTeamId` e filtra dentro; questa lo aveva perche' riceveva il risultato di un filtro altrui
	 * senza sapere quale.
	 *
	 * ⛔ **Rifiuta di comporre l'occupante quando `Snapshot.ObserverTeamId` non coincide con
	 * `ObserverTeamId`**, e lo **dice** invece di ometterlo in silenzio: un campo che sparisce senza
	 * motivo e' indistinguibile da una cella libera. La regola sta in
	 * `URTHexCellVisibilityLibrary::SnapshotEntitles` e non e' riscritta qui.
	 *
	 * ⚠️ I campi della cella restano tutti componibili: sono la mappa, e la mappa non e' segreta. La
	 * classificazione, col perche', vive in `URTHexCellVisibilityLibrary::FieldVisibility()`.
	 *
	 * `INDEX_NONE` come occupante significa **cella libera**, e si stampa come tale: `0` e' un UnitId
	 * valido e usarlo da sentinella confonderebbe «vuota» con «ci sta l'unita' zero».
	 *
	 * Pinnata da `RefactorTactics.Debug.CellReportCarriesEveryDeclaredField` e, per il canale laterale,
	 * da `RefactorTactics.Debug.CellContextHidesEnemyPosition`.
	 */
	static FString DescribeCell(int32 ObserverTeamId, const FRTHexCellData& Cell,
		const FRTHexSnapshot& Snapshot);

	/**
	 * Il contesto dell'esagono sotto il puntatore, per un osservatore: la cella e gli intenti visibili.
	 *
	 * E' il compositore che #2485 chiede nella forma gia' provata di `DescribeIntents` — funzione pura
	 * qui, consumatore sottile sopra — e non riscrive nessuna delle due regole di visibilita': la cella
	 * passa da `DescribeCell`, gli intenti da `DescribeIntents`, che filtra con
	 * `URTIntentPrivacyLibrary::FilterForTeam`.
	 *
	 * ⛔ **`Technical` non esiste in Shipping**, e non perche' un `if` la salti: le righe non vengono
	 * compilate. E' la stessa guardia di `URTErrorModalWidgetBase::GetDetail`, ed e' di compilazione
	 * perche' il rischio e' dimostrato — [#2395] e' una build Shipping rotta da una guardia di runtime.
	 * In Shipping una richiesta `Technical` restituisce la vista giocatore: il contenuto tecnico **non
	 * c'e'**, non e' nascosto.
	 *
	 * ⚠️ **Non aggiunge nessuna informazione sul nemico che il gioco non dichiari gia' pubblica.** Le due
	 * voci dell'handoff §4C — quanto lontano un nemico cammina e corre — restano **fuori**: non risulta
	 * una decisione di repository che le renda pubbliche, e deciderlo qui sarebbe inventare una regola di
	 * visibilita' dentro uno strumento.
	 */
	static TArray<FString> DescribeContext(int32 ObserverTeamId, const FRTHexCellData& Cell,
		const FRTHexSnapshot& Snapshot, const TArray<FRTPlannedIntent>& Intents,
		ERTContextView View);

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

	/**
	 * Se un rifiuto per COPERTURA sia raggiungibile adesso, e con quali unita' — `#3107`.
	 *
	 * Cerca un bersaglio **noto** all'osservatore che una sua unita' NON possa colpire per mancanza di linea
	 * di tiro. E' la fessura in cui `ERTTargetRefusal::Cover` vive:
	 *
	 *     la conoscenza e' di SQUADRA    -> basta che UN compagno veda il nemico
	 *     la linea di tiro e' per UNITA' -> il tiratore puo' non averla
	 *
	 * 🔑 **`WitnessUnitId` non e' un di piu'**: e' la meta' del caso che non si vede guardando il campo, e
	 * senza di essa la risposta «si, e' raggiungibile» resta inspiegabile a chi la legge.
	 *
	 * ⛔ **`bKnownToObserver` arriva GIA' DECISO** e qui non si rifiltra: il velo ha un produttore solo, e
	 * ricostruirlo sarebbe la seconda risposta che diverge ([D-225]). Questa funzione compone due fatti che
	 * altri hanno prodotto — la conoscenza e la geometria — e non ne inventa un terzo.
	 *
	 * ⚠️ **Non risponde «il tiro andrebbe a segno»**: `Range`, cooldown e legalita' non si guardano. Risponde
	 * alla sola domanda per cui esiste — se il RIFIUTO PER COPERTURA sia producibile con un click.
	 */
	static FRTRefusalReachability DescribeRefusalReachability(
		const class URTHexMapAsset* Map, int32 ObserverTeamId,
		const TArray<FRTRefusalProbeUnit>& Units);

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

	/**
	 * Le righe di una serie di boundary checksum: una per boundary, nella forma di `ToString()`.
	 *
	 * 🔑 **Prende i checksum GIA' CALCOLATI, e non la traccia.** Chiamare `ChecksumsAlongTrace` qui dentro
	 * obbligherebbe questa funzione a possedere anche `Initial`, la mappa e la versione di formato — cioe' a
	 * decidere *quale* misura sta facendo il chiamante. La separazione tiene il compositore puro rispetto al
	 * modello: qui non si ricalcola nessun hash e non si riordina nessun boundary.
	 *
	 * ⚠️ **Una serie vuota produce una riga, non il vuoto.** «Nessun boundary» e' un esito che va detto:
	 * un output vuoto e' indistinguibile da un comando che non e' partito.
	 *
	 * Pinnata da `RefactorTactics.Debug.BoundaryChecksumReportNamesEveryBoundary`.
	 */
	static TArray<FString> DescribeBoundaryChecksums(const TArray<FRTBoundaryChecksum>& Checksums);

	/**
	 * Il verdetto del confronto fra due corse **alla granularita' del boundary**: dove divergono, non solo
	 * che divergono.
	 *
	 * 🔴 **E' il consumer che mancava a `#2374`.** `URTBoundaryChecksumLibrary::DescribeDivergence` esisteva,
	 * era testata e non la chiamava nessuno fuori dai test: la capacita' di dire *«divergono a `T1|Move#1`»*
	 * era costruita e irraggiungibile. Questa funzione la rende raggiungibile da un consumer di produzione.
	 *
	 * ⚠️ **Non riscrive il criterio.** `FirstDivergence` decide *dove*, `DescribeDivergence` decide *come si
	 * dice*: un compositore che confrontasse gli hash per conto proprio risponderebbe a una domanda diversa
	 * da quella del gate, ed e' esattamente il difetto che `rt.Debug.VerifyReplay` evita delegando.
	 *
	 * Pinnata da `RefactorTactics.Debug.BoundaryDivergenceReportNamesTheTriple`.
	 */
	static TArray<FString> DescribeBoundaryDivergence(const TArray<FRTBoundaryChecksum>& A,
		const TArray<FRTBoundaryChecksum>& B);
};
