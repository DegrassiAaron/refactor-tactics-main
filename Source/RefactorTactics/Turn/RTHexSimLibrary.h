#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Pathfinding/RTHexPath.h"
#include "Turn/RTHexSim.h"
#include "RTHexSimLibrary.generated.h"

class URTHexMapAsset;

/**
 * Perche' una cella non e' un waypoint valido. Serve a dire nel log il motivo GIUSTO: "bloccata", "occupata" e
 * "oltre il budget" sono tre difetti diversi da correggere per chi gioca, e accorparli rende il messaggio inutile.
 * `Ok` significa che la cella in se' va bene: se il percorso composito fallisce comunque, il motivo e' il budget.
 */
UENUM(BlueprintType)
enum class ERTHexWaypointReason : uint8
{
	Ok,
	NotOnMap,        // la cella non esiste nella mappa
	BlocksMovement,  // cella non percorribile (ostacolo)
	Occupied         // occupata da un'ALTRA unita' (la propria non blocca)
};

/**
 * Ingredienti PURI della risoluzione di un turno su griglia esagonale: snapshot di inizio fase, occupazione,
 * movement budget e collisioni simultanee. Nessun Actor, nessun DeltaTime, nessuna dipendenza dall'ordine di
 * iterazione di TMap/TSet (i risultati sono ordinati con URTHexLibrary::StableLess).
 *
 * E' l'UNICO strato di risoluzione del movimento: il turn loop quadrato (Grid/ + URTMovementResolver) e'
 * stato rimosso al CP 7.2, dopo che M6 ha portato la partita su esagoni. Il punto di ritorno resta il tag
 * git `pre-hex-only`. Vedi docs/technical/systems/h6-hex-sim-spec.md.
 */
UCLASS()
class REFACTORTACTICS_API URTHexSimLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Congela mappa e unita' a inizio fase: unita' ordinate per UnitId, occupazione delle sole unita' VIVE
	 * (a parita' di cella vince l'UnitId minore), hash/revisione della mappa catturati. Deterministico:
	 * l'ordine dell'input non cambia lo snapshot.
	 */
	static FRTHexSnapshot MakeSnapshot(const URTHexMapAsset* Map, const TArray<FRTHexSimUnit>& Units);

	/**
	 * Errori strutturali dello snapshot (stessa forma di URTHexMapAsset::ValidateMap): due unita' vive sulla
	 * stessa cella, unita' viva su cella assente dalla mappa, UnitId duplicati, MoveBudget negativo.
	 */
	static TArray<FString> ValidateSnapshot(const FRTHexSnapshot& Snapshot);

	/**
	 * Vero se lo snapshot non e' piu' affidabile: mappa distrutta oppure hash/revisione cambiati dopo la
	 * cattura (la mappa NON deve cambiare durante la risoluzione di un turno).
	 */
	static bool IsSnapshotStale(const FRTHexSnapshot& Snapshot);

	/**
	 * Vero se la cella e' percorribile per l'unita' indicata: esiste nella mappa, non blocca il movimento e non
	 * e' occupata da un'unita' DIVERSA (un'unita' non blocca se stessa).
	 */
	static bool IsCellFree(const FRTHexSnapshot& Snapshot, const FRTCellId& Cell, int32 ForUnitId);

	/**
	 * Celle raggiungibili dall'unita' entro il suo MoveBudget (Dijkstra su costi interi, archi espliciti
	 * inclusi), escluse quelle occupate da altre unita'. Include sempre la cella di partenza a costo 0.
	 * Output ordinato per cella (StableLess) -> indipendente dall'ordine di TMap.
	 */
	static TArray<FRTHexReachableCell> ReachableCells(const FRTHexSnapshot& Snapshot, int32 UnitId);

	/**
	 * Celle raggiungibili DOPO aver percorso i waypoint indicati: partenza sulla punta del percorso, budget
	 * ridotto di quanto il percorso ha gia' impegnato. E' il ventaglio che il giocatore deve vedere mentre
	 * pianifica — «quanto mi resta», non «quanto avevo» (#877).
	 *
	 * Waypoint vuoti -> identica a `ReachableCells`. Piano RIFIUTATO da `BuildCompositeHexPath` (waypoint fuori
	 * budget o irraggiungibile) -> identica a `ReachableCells`: non esiste una punta da cui ripartire, e il
	 * piano in vigore e' «resto fermo».
	 *
	 * ⚠️ Deriva uno snapshot in cui l'unita' sta sulla punta: quella fotografia MENTE su dove si trova, ed e'
	 * per questo che non esce da qui. Passarla a un consumatore che non sa di essere in un'ipotesi — il
	 * TurnManager, l'area d'attacco — gli farebbe calcolare l'esito su una posizione che non e' ancora vera.
	 *
	 * Invariante (test `ReachableAfterPlanMatchesWaypointAcceptance`): una cella e' qui se e solo se
	 * `BuildCompositeHexPath` la accetta come waypoint successivo. La zona mostrata e quella subita sono la
	 * stessa, perche' entrambe escono da questa libreria.
	 */
	static TArray<FRTHexReachableCell> ReachableCellsAfterPlan(const FRTHexSnapshot& Snapshot, int32 UnitId,
		const TArray<FRTCellId>& Waypoints);

	/**
	 * Percorso dell'unita' verso Goal entro il suo MoveBudget, evitando le celle occupate da altre unita'
	 * (goal occupato -> NoPath). UnitId sconosciuto -> StartInvalid.
	 */
	static FRTHexPathResult FindPathForUnit(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTCellId& Goal);

	/**
	 * Percorso che passa da OGNI waypoint nell'ordine dato, partendo dalla cella dell'unita': un tratto di A* per
	 * waypoint, budget speso in modo cumulativo, celle occupate da altre unita' evitate. Se un tratto non entra
	 * nel budget residuo o il waypoint non e' raggiungibile, l'intero percorso e' RIFIUTATO (Path vuoto, Status
	 * del tratto che ha fallito): il chiamante scarta il waypoint appena aggiunto invece di pianificare un
	 * percorso a meta'. Nessun waypoint -> Success con la sola cella di partenza, costo 0.
	 */
	static FRTHexPathResult BuildCompositeHexPath(const FRTHexSnapshot& Snapshot, int32 UnitId,
		const TArray<FRTCellId>& Waypoints);

	/**
	 * Il PREFISSO di Path (partenza in Path[0] inclusa) ancora affrontabile entro il budget CORRENTE
	 * dell'unita' in Snapshot: somma il costo di ogni cella (+ il suo `MoveCostModifier`) e si ferma dove il
	 * budget finisce. Non valuta occupazione ne' blocchi — quelli li gestisce `ResolveHexPaths` sui
	 * micro-step, walking il path che gli si da'.
	 *
	 * Serve a intercettare un piano scritto PRIMA che lo status dell'unita' cambiasse NELLO STESSO turno
	 * (`Action.Root` azzera il budget, `Action.Slow` alza il costo per cella — CP 4.7): senza, un percorso
	 * gia' calcolato (dai waypoint, o scritto a mano) verrebbe eseguito com'era, ignorando lo stato attuale.
	 * Se il budget non e' cambiato da quando il piano e' stato scritto, il prefisso coincide con Path intero
	 * — nessuna troncatura, nessun ricalcolo: e' cio' che lascia intatto un ostacolo posizionale (una cella
	 * occupata a meta' via), che resta compito di `ResolveHexPaths`, non di questa funzione.
	 *
	 * Path vuoto -> Path vuoto. UnitId sconosciuto o mappa assente -> Path invariato (fail-open sul dato che
	 * non si puo' verificare, non sul movimento: il chiamante ha gia' un piano, qui si puo' solo accorciarlo).
	 */
	static TArray<FRTCellId> TruncatePathToBudget(const FRTHexSnapshot& Snapshot, int32 UnitId,
		const TArray<FRTCellId>& Path);

	/**
	 * Il PREFISSO di Path ancora percorribile sulla TOPOLOGIA corrente della mappa dello snapshot: si ferma
	 * PRIMA del primo passo che il grafo non consente piu'.
	 *
	 * Serve a intercettare un percorso validato quando la mappa era un'altra — una porta chiusa nel Blast, un
	 * muro alzato — che `TruncatePathToBudget` non vede, perche' quella guarda il budget. Senza, un percorso
	 * gia' pianificato attraverserebbe un varco che nel frattempo si e' chiuso: e' il «path fantasma» che
	 * CP 9.3 esiste per impedire. Il movimento si FERMA all'ultima cella valida (`Fallback.Stop`), non si
	 * annulla.
	 *
	 * Non duplica la regola dei bordi: CHIEDE AL GRAFO — un passo sopravvive solo se la destinazione compare
	 * fra i `GraphNeighbors` della cella corrente. Con una sola domanda copre porte, muri, celle bloccate e
	 * celle sparite, e restera' corretta quando gli archi cambieranno (CP 9.4).
	 *
	 * Path con meno di 2 celle -> invariato. Mappa assente -> Path invariato (fail-open sul dato non
	 * verificabile: il chiamante ha gia' un piano, qui si puo' solo accorciarlo).
	 */
	static TArray<FRTCellId> TruncatePathToTopology(const FRTHexSnapshot& Snapshot,
		const TArray<FRTCellId>& Path);

	/**
	 * Cosa non va nella cella indicata come waypoint per l'unita': fuori mappa, ostacolo, occupata da un'altra
	 * unita' — oppure `Ok`, e allora un eventuale rifiuto del percorso e' questione di **budget**.
	 * Serve a spiegare il rifiuto con il motivo giusto invece di elencarne tre.
	 */
	static ERTHexWaypointReason ClassifyWaypointCell(const FRTHexSnapshot& Snapshot, int32 UnitId,
		const FRTCellId& Cell);

	/**
	 * Se Path termina su una cella Ice e il budget residuo dell'unita' (MoveBudget - costo del percorso) e'
	 * >= 2, estende Path di una cella nella direzione dell'ultimo passo (se esiste e non blocca il
	 * movimento). Altrimenti ritorna Path invariato. Path.Num() < 2, o l'ultimo passo non e' un vicino
	 * diretto sullo stesso layer (es. arrivo via transizione), -> Path invariato.
	 *
	 * La cella aggiunta NON e' verificata per occupazione: il path esteso entra nel microstep di
	 * ResolveHexPaths, che gestisce occupazione/collisione come qualunque altro passo pianificato — quindi
	 * due unita' che scivolano verso la stessa cella libera sono gestite dal resolver esistente, non da
	 * questa funzione. Va chiamata SOLO sul path del movimento normale, MAI su una mobilita' lineare
	 * (`URTMovementActionLibrary::ResolveLinearMove`): lo Scatto non passa dal microstep condiviso, quindi
	 * non avrebbe la stessa garanzia sotto collisione simultanea — vedi docs/gameplay/spec-terreni-e8.md §5.2.
	 */
	static TArray<FRTCellId> ApplyIceSliding(const FRTHexSnapshot& Snapshot, int32 UnitId, const TArray<FRTCellId>& Path);

	/**
	 * Movimento simultaneo lungo path esagonali gia' troncati al budget (ogni path e' From..To, From = indice 0),
	 * con MICROSTEP sincroni: a ogni passo tutte le unita' avanzano di una cella e si risolvono le collisioni
	 * (destinazione contesa -> contendenti fermi da li'; cella di un'unita' ferma -> bloccata; scambio diretto
	 * -> consentito), finche' nessuno avanza. Punto fisso monotono: il risultato NON dipende dall'ordine
	 * delle richieste. Eredita la semantica del resolver quadrato che ha sostituito (rimosso al CP 7.2).
	 */
	static TArray<FRTHexMoveResult> ResolveHexPaths(const TArray<TArray<FRTCellId>>& Paths);

	/**
	 * Come `ResolveHexPaths`, con due dati per unita' (CP 4.8) che valgono SOLO per contendere una cella: la
	 * precedenza dichiarata dall'azione (`FRTActionDef::Priority` del catalogo — numero PIU' BASSO vince, stessa
	 * convenzione di `URTActionQueueLibrary`) e se la mobilita' e' LINEARE con impatto (`Action.Charge` e affini,
	 * `URTMovementActionLibrary::IsLinear`).
	 *
	 * - Destinazione contesa fra priorita' diverse: la piu' bassa entra, le altre si fermano PRIMA
	 *   (`BlockedByPriority`). A PARITA' di priorita' fra i contendenti, si torna al comportamento di base:
	 *   tutti fermi (`BlockedContested`) — "Charge prevale su Move", non "il primo dell'array vince".
	 * - Due mobilita' LINEARI che si scambierebbero la cella (l'una entra dove sta l'altra, e viceversa, nello
	 *   stesso microstep) si fermano l'una davanti all'altra (`BlockedByImpact`) invece di attraversarsi: e'
	 *   la lettura di uno scontro frontale fra due cariche opposte. Lo scambio fra mobilita' NON entrambe
	 *   lineari resta consentito, come nella variante base.
	 *
	 * `bPassThrough` marca chi ATTRAVERSA le unita' ferme lungo la traiettoria (`ERTMovementStyle::LinearPass`,
	 * la lama di Wraith): per costoro un'unita' che resta su una cella INTERMEDIA non blocca il passo.
	 *
	 * Resta un vincolo sulla cella FINALE: si passa in mezzo a qualcuno, non ci si ferma dentro. E resta
	 * invariata la contesa fra due unita' in MOVIMENTO verso la stessa cella — attraversare chi sta fermo e
	 * incrociare chi si muove sono due problemi diversi, e qui e' risolto solo il primo.
	 *
	 * `Priorities`/`bLinearMovers`/`bPassThrough` vuoti o piu' corti di `Paths` -> priorita' 0 (parita' con
	 * tutti), non-lineare e non-attraversante per le unita' mancanti: con tutti vuoti il risultato e'
	 * IDENTICO a `ResolveHexPaths(Paths)`.
	 */
	static TArray<FRTHexMoveResult> ResolveHexPaths(const TArray<TArray<FRTCellId>>& Paths,
		const TArray<int32>& Priorities, const TArray<bool>& bLinearMovers,
		const TArray<bool>& bPassThrough = TArray<bool>());

	/**
	 * Apre una risoluzione di movimento SOSPENDIBILE (CP 14.2): stesso input dei due `ResolveHexPaths`, ma
	 * restituisce lo stato invece del risultato.
	 *
	 * I tre membri di questa terna — `BeginHexMovement`, `ResolveNextHexMicroStep`, `FinishHexMovement` —
	 * **sono** cio' che `ResolveHexPaths` esegue: non e' una seconda implementazione da tenere allineata, e
	 * per questo «i due percorsi danno lo stesso risultato» e' una proprieta' del codice, non una speranza.
	 *
	 * Serve a un Overwatch interattivo, che deve poter fermare il movimento **dentro** il calcolo. Chi non ha
	 * bisogno di fermarlo continua a chiamare `ResolveHexPaths` e non si accorge di nulla.
	 */
	static FRTMovementResolutionState BeginHexMovement(const TArray<TArray<FRTCellId>>& Paths,
		const TArray<int32>& Priorities = TArray<int32>(), const TArray<bool>& bLinearMovers = TArray<bool>(),
		const TArray<bool>& bPassThrough = TArray<bool>());

	/**
	 * Esegue UN microstep: tutte le unita' avanzano di una cella e si risolvono le collisioni.
	 *
	 * Ritorna **falso** quando nessuno si e' mosso, cioe' quando la risoluzione e' finita — ed e' allora che
	 * gli `Outcome` vengono scritti, perche' il reason code finale dipende dalla posizione RAGGIUNTA e non si
	 * puo' decidere a meta' strada. Chiamarlo ancora dopo la fine e' legale e non cambia nulla.
	 *
	 * Fra un microstep e il successivo lo stato e' consistente e si puo' leggere: e' il punto in cui un
	 * decision boundary apre la sua finestra ([ADR-0004](../../../docs/decisions/adr-0004-finestre-di-reazione.md)).
	 */
	static bool ResolveNextHexMicroStep(FRTMovementResolutionState& State);

	/** Esaurisce i microstep rimasti e restituisce i risultati. Su uno stato gia' finito non fa nulla. */
	static TArray<FRTHexMoveResult> FinishHexMovement(FRTMovementResolutionState& State);

	/**
	 * Ferma `UnitId` dove si trova: il resto del suo percorso non verra' percorso (CP 14.5).
	 *
	 * E' la meta' mancante di CP 14.2. Quel checkpoint ha reso il ciclo **sospendibile** — si puo' fermare il
	 * mondo fra un passo e l'altro — ma non ha dato modo di fermare **una** unita': senza, un `FIRE` che
	 * tronca il movimento del bersaglio non ha come dirlo al resolver, e l'unico modo sarebbe correggere i
	 * `Results` a movimento concluso, cioe' dopo che i micro-step successivi hanno gia' calcolato collisioni e
	 * occupazioni con un'unita' che non doveva essere li'. E' precisamente cio' che il DoD chiede di evitare:
	 * *«le collisioni dei micro-step successivi cambiano di conseguenza»*.
	 *
	 * Chiamarla fra un `ResolveNextHexMicroStep` e il successivo. Idempotente, e innocua su un'unita' gia'
	 * ferma o su un indice non valido.
	 *
	 * ⚠️ `Reason` **sovrascrive** la memoria del primo congelamento, a differenza di cio' che accade dentro il
	 * micro-step. Quella memoria esiste perche' i blocchi sono transitori — chi perde una cella contesa la
	 * ritenta al passo dopo, e il motivo vero e' il primo — mentre un arresto chiesto da qui e' **terminale**:
	 * e' l'ultima parola sul perche' quell'unita' non e' arrivata. Non sovrascriverlo produrrebbe un TurnLog
	 * che attribuisce a «cella occupata» un'unita' fermata da un colpo.
	 */
	static void StopUnitInPlace(FRTMovementResolutionState& State, int32 UnitId, ERTMoveOutcome Reason);

	/**
	 * Voci di TurnLog dagli esiti del movimento simultaneo: una per unita', nell'ordine dell'input
	 * (Phase/Category = Move, Outcome = ERTMoveOutcome, SrcCell = cella di PARTENZA del turno — chiave stabile
	 * dell'unita', mai un pointer —, TgtCell = cella finale, Amount = celle percorse). L'invarianza per
	 * permutazione e' garantita a valle da URTTurnLogLibrary::SortTurnLog/HashTurnLog.
	 *
	 * `CauseActionId` dichiara **perche'** l'unita' si e' spostata (#307): `Action.Move` per il movimento
	 * volontario, l'ActionId dello scatto o dell'azione che spinge negli altri casi. Viaggia nel campo
	 * `ActionId`, che esiste gia' nella voce, e' gia' serializzato e gia' entra nell'hash — quindi la causa
	 * non costa una migrazione di formato. Omesso, la voce resta identica a prima.
	 *
	 * `CausePriority` e' la priorita' intra-fase della stessa azione (CP 11.3, formato **v7**): con quale
	 * precedenza quel movimento ha risolto rispetto agli altri della fase. Viene dal catalogo e non e'
	 * ricalcolata qui — chi legge la traccia non deve caricare i data asset per sapere in che ordine il
	 * turno ha risolto. Omessa vale `0`, che nel formato significa «non dichiarata».
	 */
	static TArray<FRTTurnLogEntry> BuildMoveLog(const TArray<TArray<FRTCellId>>& Paths,
		const TArray<FRTHexMoveResult>& Results, FName CauseActionId = NAME_None, int32 CausePriority = 0);
};
