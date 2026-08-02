#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Terrain/RTTerrainTypes.h"
#include "RTTerrainLibrary.generated.h"

/**
 * Derivazione pura delle proprieta' di terreno rilevanti ai sistemi (costo/blocco/vista),
 * indipendente da asset e Actor. Base del wiring verso il pathfinding pesato (PF.3) e la LOS.
 */
UCLASS()
class REFACTORTACTICS_API URTTerrainLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Costo di attraversamento della cella: RT_BLOCKED_COST se non attraversabile, altrimenti
	 * 1 + max(0, ExtraMoveCost). Il blocco prevale sul costo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static int32 CellMoveCost(const FRTTerrainProps& Props);

	/** Vero se la cella non e' attraversabile. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static bool BlocksMovement(const FRTTerrainProps& Props);

	/** Vero se la cella blocca la linea di tiro. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static bool BlocksVision(const FRTTerrainProps& Props);
};
