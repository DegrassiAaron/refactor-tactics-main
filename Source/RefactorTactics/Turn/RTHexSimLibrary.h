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
	 * Cosa non va nella cella indicata come waypoint per l'unita': fuori mappa, ostacolo, occupata da un'altra
	 * unita' — oppure `Ok`, e allora un eventuale rifiuto del percorso e' questione di **budget**.
	 * Serve a spiegare il rifiuto con il motivo giusto invece di elencarne tre.
	 */
	static ERTHexWaypointReason ClassifyWaypointCell(const FRTHexSnapshot& Snapshot, int32 UnitId,
		const FRTCellId& Cell);

	/**
	 * Movimento simultaneo lungo path esagonali gia' troncati al budget (ogni path e' From..To, From = indice 0),
	 * con MICROSTEP sincroni: a ogni passo tutte le unita' avanzano di una cella e si risolvono le collisioni
	 * (destinazione contesa -> contendenti fermi da li'; cella di un'unita' ferma -> bloccata; scambio diretto
	 * -> consentito), finche' nessuno avanza. Punto fisso monotono: il risultato NON dipende dall'ordine
	 * delle richieste. Eredita la semantica del resolver quadrato che ha sostituito (rimosso al CP 7.2).
	 */
	static TArray<FRTHexMoveResult> ResolveHexPaths(const TArray<TArray<FRTCellId>>& Paths);

	/**
	 * Voci di TurnLog dagli esiti del movimento simultaneo: una per unita', nell'ordine dell'input
	 * (Phase/Category = Move, Outcome = ERTMoveOutcome, SrcCell = cella di PARTENZA del turno — chiave stabile
	 * dell'unita', mai un pointer —, TgtCell = cella finale, Amount = celle percorse). L'invarianza per
	 * permutazione e' garantita a valle da URTTurnLogLibrary::SortTurnLog/HashTurnLog.
	 */
	static TArray<FRTTurnLogEntry> BuildMoveLog(const TArray<TArray<FRTCellId>>& Paths,
		const TArray<FRTHexMoveResult>& Results);
};
