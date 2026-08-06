#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Pathfinding/RTHexPath.h"
#include "Turn/RTHexSim.h"
#include "RTHexSimLibrary.generated.h"

class URTHexMapAsset;

/**
 * Ingredienti PURI della risoluzione di un turno su griglia esagonale: snapshot di inizio fase, occupazione,
 * movement budget e collisioni simultanee. Nessun Actor, nessun DeltaTime, nessuna dipendenza dall'ordine di
 * iterazione di TMap/TSet (i risultati sono ordinati con URTHexLibrary::StableLess).
 *
 * E' l'UNICO strato di risoluzione del movimento: il turn loop quadrato (Grid/ + URTMovementResolver) e'
 * stato rimosso al CP 7.2, dopo che M6 ha portato la partita su esagoni. Il punto di ritorno resta il tag
 * git `pre-hex-only`. Vedi docs/design/h6-hex-sim-spec.md.
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
	 * `Priorities`/`bLinearMovers` vuoti o piu' corti di `Paths` -> priorita' 0 (parita' con tutti) e non-lineare
	 * per le unita' mancanti: con entrambi vuoti il risultato e' IDENTICO a `ResolveHexPaths(Paths)`.
	 */
	static TArray<FRTHexMoveResult> ResolveHexPaths(const TArray<TArray<FRTCellId>>& Paths,
		const TArray<int32>& Priorities, const TArray<bool>& bLinearMovers);

	/**
	 * Voci di TurnLog dagli esiti del movimento simultaneo: una per unita', nell'ordine dell'input
	 * (Phase/Category = Move, Outcome = ERTMoveOutcome, SrcCell = cella di PARTENZA del turno — chiave stabile
	 * dell'unita', mai un pointer —, TgtCell = cella finale, Amount = celle percorse). L'invarianza per
	 * permutazione e' garantita a valle da URTTurnLogLibrary::SortTurnLog/HashTurnLog.
	 */
	static TArray<FRTTurnLogEntry> BuildMoveLog(const TArray<TArray<FRTCellId>>& Paths,
		const TArray<FRTHexMoveResult>& Results);
};
