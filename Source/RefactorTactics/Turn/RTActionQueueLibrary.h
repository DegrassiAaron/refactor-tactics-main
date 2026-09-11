#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTTurnRules.h" // ERTMatchPhase: le chiavi d'ordine partono dalla macro-fase
#include "RTActionQueueLibrary.generated.h"

class ARTUnit;

/**
 * Le chiavi con cui UN'UNITA' entra nell'ordine di risoluzione — cella, identita' stabile, nome dell'Actor.
 *
 * 🔑 **E' una struct e non tre parametri sciolti** perche' la chiave e' cio' che va tenuto insieme: un
 * comparatore che ne prende sei allineati e' il posto in cui prima o poi si invertono due argomenti dello
 * stesso tipo. E perche' un test possa costruirla **senza un mondo**: `ARTUnit` e' un Actor, e la regola
 * d'ordine non ha bisogno di uno per essere provata.
 *
 * ⛔ Nessun puntatore, nessun ordine di spawn, nessun indice di registrazione: sono i tre input che
 * `URTActionQueueLibrary::UnitOrderLess` esiste per tenere fuori dalla decisione.
 */
struct FRTUnitOrderKey
{
	/** Prima chiave: la cella logica, confrontata con `URTHexLibrary::StableLess`. */
	FRTCellId Cell;

	/** Identita' di partita ([D-063]). `0` significa «non ancora assegnata», non «unita' zero». */
	int32 StableUnitId = 0;

	/**
	 * Ultimo spareggio, per il solo caso in cui `StableUnitId` non esista ancora.
	 *
	 * 🔑 **`FName` e non `FString`, e la ragione e' il costo.** `AActor::GetName()` passa da
	 * `FName::ToString()` e **alloca**; `GetFName()` no. ⚠️ La prima stesura giustificava il cambio dicendo
	 * che `PlanningSnapshotFor` *«prima non allocava niente»*: e' **falso** e va detto — quel percorso
	 * costruisce gia' un `TArray<AActor*>`, un `TArray<ARTUnit*>` e un intero `FRTHexSnapshot` per chiamata.
	 * La ragione vera e' piu' modesta: una chiave **senza allocazioni** si puo' costruire dentro il
	 * comparatore senza pensarci, e questa lo e'.
	 *
	 * ⚠️ **Il confronto e' `LexicalLess`, che l'engine documenta *«stable / deterministic over process
	 * runs»*** — l'opposto di `FastLess`, che ordina per indice della name table ed e' stabile solo dentro un
	 * processo. E' lo stesso confronto che `InstanceLess` usa gia' per `ActionId`.
	 *
	 * 🔴 **Il suffisso numerico si confronta come NUMERO, non come testo**, ed e' il contrario di cio'
	 * che questo commento affermava. Misurato nel sorgente dell'engine — `FName::CompareInternal`
	 * (`UnrealNames.cpp`): a parita' di parte testuale restituisce `GetNumber() - Other.GetNumber()`. Quindi
	 * `..._2` precede `..._10`, mentre la `FString::Compare` della stesura precedente metteva `_10` per
	 * primo. ⚠️ **E' un cambio di comportamento rispetto a cio' che #2923 ha mergiato**, visibile solo
	 * dove decide la terza chiave — cioe' prima del lock-in. E' un ordine piu' naturale, non solo diverso.
	 *
	 * ⚠️ `LexicalLess` e' case-INSENSITIVE. Due Actor **dello stesso livello** non possono avere nomi che
	 * differiscono solo per il caso — l'unicita' degli `UObject` e' case-insensitive dentro lo stesso Outer,
	 * e per un Actor l'Outer e' la `ULevel`. ⛔ **Ma l'unicita' non vale FRA livelli**, e gli array che si
	 * ordinano qui nascono da `GetAllActorsOfClass`, che li attraversa tutti: due unita' in sublevel diversi
	 * possono portare lo stesso nome. Il pareggio completo richiede allora **tre** coincidenze insieme —
	 * stessa cella, `StableUnitId` uguale, e stesso nome da due livelli — e nella risoluzione la seconda non
	 * si da', perche' `EnsureMatchRoster` ha gia' assegnato id distinti. Resta rappresentabile nelle
	 * anteprime di pianificazione, dove gli id valgono tutti `0`. Trovato in code review.
	 */
	FName ActorName;

	FRTUnitOrderKey() = default;
	FRTUnitOrderKey(const FRTCellId& InCell, int32 InStableUnitId, const FName& InActorName)
		: Cell(InCell), StableUnitId(InStableUnitId), ActorName(InActorName) {}
};

/**
 * Ordine di risoluzione di un turno: quello delle **azioni** e quello delle **unita'**, deterministici
 * entrambi.
 *
 * ⚠️ **La regola e' pura; due funzioni di comodo no.** `MakeUnitOrderKey` e `SortUnitsForResolution`
 * prendono `ARTUnit`, e l'header lo dichiara in avanti invece di includerlo. Fino a #2922 questa classe
 * prometteva *«senza Actor»*: la promessa e' stata **ritirata**, non violata in silenzio. Cio' che resta
 * vero, ed e' la parte che conta, e' che `InstanceLess` e `UnitOrderLess` — le due regole — non toccano
 * nessun Actor e si provano senza un mondo.
 *
 * Una sola sede per la regola d'ordine, come `URTTurnLogLibrary` lo e' per il TurnLog. Se la scelta di
 * "chi risolve prima" fosse sparsa fra le fasi, due punti del codice potrebbero divergere e l'esito di un
 * turno dipenderebbe da quale ha ragione.
 */
UCLASS()
class REFACTORTACTICS_API URTActionQueueLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Ordine TOTALE fra due azioni pianificate. Vero se A risolve prima di B.
	 *
	 * Chiavi, in ordine: **macro-fase di Atlas** (`Prep -> Dash -> Blast -> Move -> Cleanup`) -> `Priority`
	 * intera crescente -> `ActionId` -> `SourceUnitId` -> `EventSequence`.
	 *
	 * La macro-fase viene PRIMA della priorita': un `Action.Move` a priorita' 50 risolve dopo un attacco a
	 * priorita' 80, perche' il Move e' una fase successiva (ADR-0003 §1 — il catalogo v0.1 metteva invece il
	 * movimento prima dell'attacco). La priorita' ordina DENTRO la fase, non fra le fasi.
	 *
	 * E' l'UNICO ordine di risoluzione del gioco. Questo commento diceva di «estendere la regola APNAP di
	 * `piano-canonico-mvp.md §5.1`»: quella regola e' stata **ritirata** da D-293 (2026-08-31) perche'
	 * presupponeva un'unita' attiva che un gioco a turni simultanei non ha. Non esiste un secondo ordine da
	 * estendere, e non va costruito.
	 *
	 * Nessuna chiave e' un float; nessuna dipende dall'ordine di una `TMap`.
	 */
	static bool InstanceLess(const FRTActionInstance& A, const FRTActionInstance& B);

	/** Ordina in place con `InstanceLess`: permutare l'ingresso non cambia la sequenza risolta. */
	static void SortActionInstances(TArray<FRTActionInstance>& Instances);

	/**
	 * Le sole istanze che risolvono nella macro-fase indicata, nell'ordine di risoluzione.
	 * Il chiamante lavora una fase per volta senza rifiltrare a mano (e senza sbagliare la rimappatura).
	 */
	static TArray<FRTActionInstance> InstancesForPhase(const TArray<FRTActionInstance>& Instances,
		ERTMatchPhase Phase);

	// --- Ordine delle UNITA' (#2922) --------------------------------------------------------------------

	/**
	 * Ordine TOTALE fra due unita' nella risoluzione. Vero se A entra prima di B.
	 *
	 * Chiavi, in ordine: **cella** (`URTHexLibrary::StableLess`) -> **`StableUnitId`** -> **nome dell'Actor**.
	 *
	 * 🔴 **La cella da sola non e' un ordine totale, e per anni si e' creduto che lo fosse.** `#990` lo
	 * scrisse esplicitamente — *«`StableLess` sulla cella e' un ordine totale sulle unita' vive, perche' due
	 * unita' vive non condividono una cella»* — e `#1733`/`#1970` hanno misurato il contrario: la
	 * sovrapposizione esiste, `URTHexSimLibrary::MakeSnapshot` la **registra** in `FRTHexSnapshot::Overlaps`
	 * e `ARTTurnManager::ReportSnapshotOverlaps` la segnala a runtime. L'invariante e' un rilevatore di
	 * difetto, non una garanzia, e un comparatore non puo' poggiarci sopra.
	 *
	 * ⚠️ **Su un pareggio decideva `GetAllActorsOfClass`**: `TArray::Sort` inoltra ad `Algo::Sort` — introsort,
	 * NON stabile — quindi l'ordine d'ingresso sopravvive. Cioe' esattamente l'input che `#990` aveva tolto
	 * dalla decisione, rientrato da un anello piu' in la': l'indice in `CollectLivingUnits` **e'**
	 * `FRTHexSimUnit::UnitId`, e `MakeSnapshot` tiene come occupante l'`UnitId` minore.
	 *
	 * 🔑 **La cella resta la PRIMA chiave**: dove le celle differiscono l'ordine non si sposta di una
	 * posizione. Le due chiavi in coda sono spareggi TECNICI, non una priorita' di gioco: chi vince una
	 * sovrapposizione resta l'errore strutturale che `ReportSnapshotOverlaps` segnala, e qui si stabilisce
	 * soltanto che l'esito non dipenda dall'ordine di registrazione degli Actor (`CLAUDE.md` §11).
	 *
	 * ⚠️ **Dove le celle pareggiano, invece, l'ordine CAMBIA rispetto a prima** — da arbitrario a
	 * deterministico. Il caso e' raggiungibile senza patologie: una vittima uccisa nel Blast resta nel mondo
	 * fino a `DestroyDefeatedUnits`, e `MakeSnapshot` conta occupante solo chi e' VIVO, quindi nel Move
	 * un'altra unita' puo' salire sulla sua cella. La prima stesura di questo commento diceva *«il corpus
	 * golden non ha ragione di cambiare»*: la ragione formulata cosi' non regge, ed e' `GoldenCorpusMatches`
	 * verde a dirlo — una misura, non una deduzione. Trovato in code review.
	 *
	 * `StableUnitId` e' il tie-break **gia' disponibile e gia' corretto** ([D-063]): lo assegna
	 * `ARTTurnManager::EnsureMatchRoster()` una volta per partita, con `MatchRosterLess` che e' gia' un ordine
	 * totale. Il nome dell'Actor chiude il caso in cui vale ancora `0` — la pianificazione prima del primo
	 * lock-in — ed e' lo stesso ultimo confronto che `MatchRosterLess` usa gia'.
	 *
	 * ⛔ **E il nome NON e' riproducibile fra processi diversi, dove decide.** Un'unita' spawnata senza nome
	 * esplicito lo riceve da `MakeUniqueObjectName`, che appende un contatore per-classe vivo quanto il
	 * processo: due partite nello stesso Editor danno `..._0..3` e `..._4..7`. (⚠️ Il suffisso si ordina
	 * **numericamente**, non lessicalmente — si veda `FRTUnitOrderKey::ActorName`.) Non e' un difetto
	 * introdotto qui — `MatchRosterLess` ha lo stesso ultimo
	 * confronto, con la stessa proprieta' — ma la terza chiave rende l'ordine totale e ripetibile **dentro
	 * una esecuzione**, non oltre. Oltre, a rendere l'ordine riproducibile e' `StableUnitId`, che nella
	 * risoluzione c'e' sempre.
	 */
	static bool UnitOrderLess(const FRTUnitOrderKey& A, const FRTUnitOrderKey& B);

	/**
	 * La chiave d'ordine di un'unita' viva o morta. L'unico punto che legge i tre campi dall'Actor.
	 *
	 * Non alloca: `GetFName()` e non `GetName()`, per la ragione scritta su `FRTUnitOrderKey::ActorName`.
	 */
	static FRTUnitOrderKey MakeUnitOrderKey(const ARTUnit& Unit);

	/**
	 * Ordina in place con `UnitOrderLess`: permutare l'ingresso non cambia la sequenza risolta, nemmeno
	 * quando due unita' condividono una cella.
	 *
	 * Chiamanti, per nome invece che per conteggio (`AGENTS.md` §14): `ARTGameMode::AssignUnitControlGroups`,
	 * `ARTTurnManager::ConcludeResolution`, `::ResolveEnvironment`, `::ResolvePrep`, `::CollectLivingUnits`,
	 * `::GatherBlastUnits`. Prima di #2922 ognuno aveva la propria copia del comparatore.
	 *
	 * ⚠️ **Precondizione: nessun `nullptr` nell'array** — la stessa dei `Sort` scritti a mano che questa
	 * funzione sostituisce (`TDereferenceWrapper` dereferenzia comunque). I chiamanti riempiono l'array da un
	 * `Cast<ARTUnit>`, quindi la condizione e' vera per costruzione. Un ripiego silenzioso sarebbe peggio del
	 * crash: una chiave di default ordinerebbe il `nullptr` **in mezzo** alle unita' vere, come se stesse
	 * sulla cella `(0,0,0)`.
	 *
	 * ⛔ **Ordina IN PLACE, e non si riscrive l'array del chiamante.** Una stesura precedente decorava,
	 * ordinava gli indici e faceva `Units = MoveTemp(Sorted)`: buttava via il buffer che `CollectLivingUnits`
	 * riusa con `Reset()`+`Reserve()` — e che `FRTScenarioSession` tiene per tutta la partita — riallocandolo
	 * a ogni turno. Trovato in code review.
	 */
	static void SortUnitsForResolution(TArray<ARTUnit*>& Units);
};
