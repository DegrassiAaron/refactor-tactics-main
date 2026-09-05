#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Pathfinding/RTHexPath.h"
#include "RTHexPathLibrary.generated.h"

class URTHexMapAsset;

/**
 * Pathfinding esagonale AUTOREVOLE sul grafo tattico della mappa (A* deterministico, costi interi). Non usa la
 * NavMesh. Le celle su layer diversi si collegano SOLO tramite transizioni esplicite (archi). Nessuna dipendenza
 * dall'ordine di iterazione di TSet/TMap: il tie-break usa l'ID della cella (ordinamento stabile).
 */

constexpr uint8 RT_NoEntrySide = 6;

struct FRTPathNode
	{
	FRTCellId Cell;
	uint8 Entry = RT_NoEntrySide;

	bool operator==(const FRTPathNode& O) const { return Cell == O.Cell && Entry == O.Entry; }
	};

FORCEINLINE uint32 GetTypeHash(const FRTPathNode& N)
	{
	return HashCombine(GetTypeHash(N.Cell), static_cast<uint32>(N.Entry));
	}

	/** Ordine totale e stabile: prima la cella (regola esistente), poi il lato. Nessun ordine di TSet. */
inline bool NodeStableLess(const FRTPathNode& A, const FRTPathNode& B)
	{
	if (!(A.Cell == B.Cell))
	{
		return URTHexLibrary::StableLess(A.Cell, B.Cell);
	}
	return A.Entry < B.Entry;
	}


UCLASS()
class REFACTORTACTICS_API URTHexPathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Vicini raggiungibili dalla cella nel grafo tattico: i 6 vicini orizzontali presenti e non bloccati
	 * (costo = MoveCost della destinazione) piu' le transizioni esplicite uscenti (costo = costo dell'arco).
	 * Ordine deterministico (direzioni E..SE, poi archi nell'ordine dell'asset).
	 */
	/**
	 * SI PUO' ATTRAVERSARE questa cella entrando da un lato e uscendo da un altro?
	 *
	 * 🔴 **La domanda che il grafo non poneva** (#2100). Un muro interno divide lo spazio di posa di
	 * UN solo `FRTCellId` in regioni sconnesse — `spec-cover-placement-intra-hex.md` §6 — e finora
	 * nessuno lo chiedeva: `ERTIntraCellTraversal` esisteva, era testata, e aveva **zero** chiamanti di
	 * produzione. Il difetto non si vedeva perche' il bake produceva coperture di bordo al posto dei muri
	 * interni, e *quelle* fermavano il passo — per la ragione sbagliata (#2085).
	 *
	 * ⚠️ **Le direzioni sono quelle della CELLA verso i vicini**, non del moto: `EntryDir` punta alla
	 * cella da cui si arriva, `ExitDir` a quella verso cui si esce.
	 *
	 * Pinnata da `RefactorTactics.Path.ContinuousWallBlocksTheCrossing`.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool CanTransitCell(const URTHexMapAsset* Map, const FRTCellId& Cell,
		ERTHexDirection EntryDir, ERTHexDirection ExitDir);

	/** Questa cella porta geometria interna? La via rapida di `CanTransitCell`, esposta perche' i percorritori la usano per decidere se lo stato di ricerca debba distinguere il lato d'ingresso. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool CellHasInteriorGeometry(const URTHexMapAsset* Map, const FRTCellId& Cell);

	static TArray<TPair<FRTCellId, int32>> GraphNeighbors(const URTHexMapAsset* Map, const FRTCellId& Cell);

	/**
	 * A* deterministico da Start a Goal sul grafo della mappa. MaxCost > 0 limita il costo (0 = illimitato);
	 * MaxNodes limita i nodi espansi (guardia). Euristica = distanza esagonale (ammissibile per archi locali).
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Hex")
	static FRTHexPathResult FindPath(const URTHexMapAsset* Map, const FRTCellId& Start, const FRTCellId& Goal,
		int32 MaxCost = 0, int32 MaxNodes = 100000);

	/**
	 * Come FindPath, ma tratta le celle in Blocked come non percorribili (ostacoli DINAMICI, es. unita' occupanti:
	 * non appartengono all'asset mappa). Blocked == nullptr equivale a FindPath. Goal bloccato -> NoPath.
	 *
	 * `ExtraCostPerCell` (>= 0) si somma al costo di OGNI arco attraversato (`Action.Slow`, CP 4.7): 0 =
	 * nessun sovrapprezzo, comportamento invariato. E' un parametro del CHIAMANTE (dipende da CHI si muove),
	 * non della mappa: la stessa cella costa diverso per un'unita' rallentata e per una che non lo e'.
	 */
	static FRTHexPathResult FindPathAvoiding(const URTHexMapAsset* Map, const FRTCellId& Start, const FRTCellId& Goal,
		const TSet<FRTCellId>* Blocked, int32 MaxCost = 0, int32 MaxNodes = 100000, int32 ExtraCostPerCell = 0);
};

	/**
	 * Il lato di `Cell` verso `Toward`, o il sentinella se i due non sono adiacenti sul piano.
	 *
	 * Il secondo caso e' l'arco esplicito: non attraversa un bordo del perimetro, quindi non esiste un
	 * settore d'ingresso e la traversata intra-cella non e' esprimibile. Si risponde "nessun lato" invece di
	 * inventarne uno.
	 */
inline uint8 EntrySideOf(const URTHexMapAsset* Map, const FRTCellId& Cell, const FRTCellId& Toward)
	{
	// La distinzione serve SOLO dove c'e' geometria: altrove il sentinella conserva l'ordinamento.
	if (!URTHexPathLibrary::CellHasInteriorGeometry(Map, Cell))
	{
		return RT_NoEntrySide;
	}
	ERTHexDirection Dir = ERTHexDirection::E;
	if (!URTHexLibrary::DirectionBetween(Cell, Toward, Dir))
	{
		return RT_NoEntrySide;
	}
	return static_cast<uint8>(Dir);
}
