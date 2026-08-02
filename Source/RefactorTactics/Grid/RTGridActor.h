#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/RTTypes.h"
#include "Terrain/RTTerrainData.h"
#include "RTGridActor.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Rappresentazione visuale della griglia: una mesh istanziata per cella,
 * posizionata usando URTGridLibrary (la logica resta separata dal rendering).
 */
UCLASS()
class REFACTORTACTICS_API ARTGridActor : public AActor
{
	GENERATED_BODY()

public:
	ARTGridActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	int32 Width = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	int32 Height = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	float CellSize = 200.f;

	/** Celle-ostacolo (Muro): bloccano la linea di tiro e non sono calpestabili. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TArray<FRTGridCoord> BlockedCells;

	/** Terreno per cella (assente = normale, costo 1). Istanze create a runtime (nessun .uasset). */
	UPROPERTY()
	TMap<FRTGridCoord, TObjectPtr<URTTerrainData>> TerrainCells;

	/** Terreno di una cella (nullo se normale). */
	const URTTerrainData* GetTerrainAt(const FRTGridCoord& Cell) const;

	/** Mappa costo-per-cella per il pathfinding pesato: Muro/terreno-bloccante = RT_BLOCKED_COST. */
	void BuildCostMap(TMap<FRTGridCoord, int32>& OutCost) const;

	/** Celle non calpestabili: BlockedCells + terreni con bBlocksMovement. */
	TArray<FRTGridCoord> GetMoveBlockers() const;

	/** Celle che bloccano la linea di tiro: BlockedCells + terreni con bBlocksVision. */
	TArray<FRTGridCoord> GetVisionBlockers() const;

protected:
	virtual void BeginPlay() override;

	/** Ricostruisce le istanze di celle e ostacoli. */
	void BuildGrid();

	/** Popola TerrainCells con terreni demo creati a runtime (Fango, Cespuglio, ...). */
	void SpawnDemoTerrain();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TObjectPtr<UInstancedStaticMeshComponent> Cells;

	/** Mesh alte sulle celle-ostacolo (copertura visibile). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TObjectPtr<UInstancedStaticMeshComponent> Obstacles;
};
