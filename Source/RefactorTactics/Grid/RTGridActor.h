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
	TArray<FRTCellId> BlockedCells;

	/** Celle valide del layer 1 (ponte sopraelevato). Le celle di layer 1 non elencate sono impassabili. */
	UPROPERTY()
	TArray<FRTCellId> Layer1Cells;

	/** Archi di traversata (rampe/scale/portali) che collegano celle non adiacenti / su layer diversi. */
	UPROPERTY()
	TArray<FRTTraversalEdge> Edges;

	/** Quota (cm) di un layer rispetto al precedente, per il rendering e il posizionamento. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	float LayerHeight = 250.f;

	/** Archi di traversata per il pathfinding a grafo. */
	const TArray<FRTTraversalEdge>& GetEdges() const { return Edges; }

	/** Layer del click in base al componente ISM colpito (BridgeCells -> 1, altrimenti 0). */
	int32 LayerFromHitComponent(const class UPrimitiveComponent* Comp) const;

	/** Terreno per cella (assente = normale, costo 1). Istanze create a runtime (nessun .uasset). */
	UPROPERTY()
	TMap<FRTCellId, TObjectPtr<URTTerrainData>> TerrainCells;

	/** Turni residui dei terreni temporanei (es. Fuoco); assente = permanente. */
	UPROPERTY()
	TMap<FRTCellId, int32> TerrainTurnsLeft;

	/** Terreno di una cella (nullo se normale). */
	const URTTerrainData* GetTerrainAt(const FRTCellId& Cell) const;

	/** Mappa costo-per-cella per il pathfinding pesato: Muro/terreno-bloccante = RT_BLOCKED_COST. */
	void BuildCostMap(TMap<FRTCellId, int32>& OutCost) const;

	/** Celle non calpestabili: BlockedCells + terreni con bBlocksMovement. */
	TArray<FRTCellId> GetMoveBlockers() const;

	/** Cost map per il bot: come BuildCostMap ma con gli hazard (EndTurnDamage>0) resi impassabili. */
	void BuildBotCostMap(TMap<FRTCellId, int32>& OutCost) const;

	/** Celle che bloccano la linea di tiro: BlockedCells + terreni con bBlocksVision. */
	TArray<FRTCellId> GetVisionBlockers() const;

	/** Imposta il terreno di una cella (nullo = normale); TurnsLeft > 0 lo rende temporaneo. */
	void SetTerrainAt(const FRTCellId& Cell, URTTerrainData* Terrain, int32 TurnsLeft);

	/** Fine turno: decrementa i terreni temporanei; a 0 tornano a RevertsTo (o si rimuovono). */
	void TickTerrain();

	/** Ricostruisce i piani colorati sopra le celle-terreno (riflette anche ignite/reversione). */
	void RefreshTerrainVisuals();

	/** Evidenzia la cella sotto il cursore (hover). bValid=false -> nasconde l'highlight. Solo presentazione. */
	void SetHoveredCell(const FRTCellId& Cell, bool bValid);

protected:
	virtual void BeginPlay() override;

	/** Ricostruisce le istanze di celle e ostacoli. */
	void BuildGrid();

	/** Popola TerrainCells con terreni demo creati a runtime (Fango, Cespuglio, ...). */
	void SpawnDemoTerrain();

	/** Popola il ponte demo (celle di layer 1 + rampe) creati a runtime. */
	void SpawnBridge();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TObjectPtr<UInstancedStaticMeshComponent> Cells;

	/** Mesh alte sulle celle-ostacolo (copertura visibile). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TObjectPtr<UInstancedStaticMeshComponent> Obstacles;

	/** Celle del layer 1 (ponte) renderizzate a quota; con collisione per il click->layer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TObjectPtr<UInstancedStaticMeshComponent> BridgeCells;

	/** Piani colorati sopra le celle-terreno (uno per cella, materiale M_Unit con parametro Color). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> TerrainVisuals;

	/** Materiale per colorare il terreno (default M_Global_Tint con VectorParameter "Color"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TSoftObjectPtr<UMaterialInterface> TerrainMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/RT/Art/GlobalMaterials/M_Global_Tint.M_Global_Tint")));

	/** Evidenziazione della cella sotto il cursore (hover): un piano colorato, nascosto quando fuori griglia. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TObjectPtr<UStaticMeshComponent> HoverHighlight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HoverDynMaterial;
};
