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

	/** Ultimo spareggio, per il solo caso in cui `StableUnitId` non esista ancora. */
	FString ActorName;

	FRTUnitOrderKey() = default;
	FRTUnitOrderKey(const FRTCellId& InCell, int32 InStableUnitId, FString InActorName)
		: Cell(InCell), StableUnitId(InStableUnitId), ActorName(MoveTemp(InActorName)) {}
};

/**
 * Ordine di risoluzione delle azioni di un turno: pura, deterministica, senza Actor.
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
	 * 🔑 **La cella resta la PRIMA chiave**: dove il comparatore non pareggia l'ordine non si sposta di una
	 * posizione, quindi nessuna regola di gameplay cambia e il corpus golden non ha ragione di cambiare. Le
	 * due chiavi in coda sono spareggi TECNICI, non una priorita' di gioco: chi vince una sovrapposizione
	 * resta l'errore strutturale che `ReportSnapshotOverlaps` segnala, e qui si stabilisce soltanto che
	 * l'esito non dipenda dall'ordine di registrazione degli Actor (`CLAUDE.md` §11).
	 *
	 * `StableUnitId` e' il tie-break **gia' disponibile e gia' corretto** ([D-063]): lo assegna
	 * `ARTTurnManager::EnsureMatchRoster()` una volta per partita, con `MatchRosterLess` che e' gia' un ordine
	 * totale. Il nome dell'Actor chiude il solo caso in cui vale ancora `0` — la pianificazione prima del
	 * primo lock-in, e `ARTGameMode::AssignUnitControlGroups` — ed e' lo stesso ultimo confronto che
	 * `MatchRosterLess` usa gia', per la stessa ragione dichiarata li'.
	 */
	static bool UnitOrderLess(const FRTUnitOrderKey& A, const FRTUnitOrderKey& B);

	/** La chiave d'ordine di un'unita' viva o morta. L'unico punto che legge i tre campi dall'Actor. */
	static FRTUnitOrderKey MakeUnitOrderKey(const ARTUnit& Unit);

	/**
	 * Ordina in place con `UnitOrderLess`: permutare l'ingresso non cambia la sequenza risolta, nemmeno
	 * quando due unita' condividono una cella.
	 *
	 * ⚠️ **Precondizione: nessun `nullptr` nell'array** — la stessa che avevano i sei `Sort` scritti a mano
	 * che questa funzione sostituisce (`TDereferenceWrapper` dereferenzia comunque). I chiamanti riempiono
	 * l'array da un `Cast<ARTUnit>`, quindi la condizione e' vera per costruzione. Un ripiego silenzioso
	 * sarebbe peggio del crash: una chiave di default ordinerebbe il `nullptr` **in mezzo** alle unita' vere,
	 * come se stesse sulla cella `(0,0,0)`.
	 */
	static void SortUnitsForResolution(TArray<ARTUnit*>& Units);
};
