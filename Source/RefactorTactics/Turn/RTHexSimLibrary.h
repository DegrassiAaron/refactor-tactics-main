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
 * Strato PARALLELO al turn loop quadrato (Grid/ + URTMovementResolver + ARTTurnManager), che resta la base di
 * rollback finche' l'hex non lo sostituisce funzionalmente (ADR-0002). Vedi docs/design/h6-hex-sim-spec.md.
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
	 * Movimento simultaneo lungo path esagonali gia' troncati al budget (ogni path e' From..To, From = indice 0),
	 * con MICROSTEP sincroni: a ogni passo tutte le unita' avanzano di una cella e si risolvono le collisioni
	 * (destinazione contesa -> contendenti fermi da li'; cella di un'unita' ferma -> bloccata; scambio diretto
	 * -> consentito), finche' nessuno avanza. Punto fisso monotono: il risultato NON dipende dall'ordine
	 * delle richieste. Stessa semantica di URTMovementResolver::ResolvePaths sul quadrato.
	 */
	static TArray<FRTHexMoveResult> ResolveHexPaths(const TArray<TArray<FRTCellId>>& Paths);
};
