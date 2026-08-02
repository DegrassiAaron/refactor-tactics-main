#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/RTTypes.h"
#include "Terrain/RTTerrainData.h"
#include "RTGridActor.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;

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

	/** Turni residui dei terreni temporanei (es. Fuoco); assente = permanente. */
	UPROPERTY()
	TMap<FRTGridCoord, int32> TerrainTurnsLeft;

	/** Terreno di una cella (nullo se normale). */
	const URTTerrainData* GetTerrainAt(const FRTGridCoord& Cell) const;

	/** Mappa costo-per-cella per il pathfinding pesato: Muro/terreno-bloccante = RT_BLOCKED_COST. */
	void BuildCostMap(TMap<FRTGridCoord, int32>& OutCost) const;

	/** Celle non calpestabili: BlockedCells + terreni con bBlocksMovement. */
	TArray<FRTGridCoord> GetMoveBlockers() const;

	/** Celle che bloccano la linea di tiro: BlockedCells + terreni con bBlocksVision. */
	TArray<FRTGridCoord> GetVisionBlockers() const;

	/** Imposta il terreno di una cella (nullo = normale); TurnsLeft > 0 lo rende temporaneo. */
	void SetTerrainAt(const FRTGridCoord& Cell, URTTerrainData* Terrain, int32 TurnsLeft);

	/** Fine turno: decrementa i terreni temporanei; a 0 tornano a RevertsTo (o si rimuovono). */
	void TickTerrain();

	/** Ricostruisce i piani colorati sopra le celle-terreno (riflette anche ignite/reversione). */
	void RefreshTerrainVisuals();

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

	/** Piani colorati sopra le celle-terreno (uno per cella, materiale M_Unit con parametro Color). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> TerrainVisuals;

	/** Materiale per colorare il terreno (default M_Unit con VectorParameter "Color"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TSoftObjectPtr<UMaterialInterface> TerrainMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Unit.M_Unit")));
};
