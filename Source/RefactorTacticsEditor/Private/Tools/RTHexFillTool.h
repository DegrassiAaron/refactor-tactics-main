#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexSurface (pennello + readout)
#include "RTHexFillTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Factory del tool secchiello. */
UCLASS()
class URTHexFillToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Proprieta' del secchiello: pennello (Surface/costo/blocco) applicato alla regione + readout ultimo riempimento. */
UCLASS(Transient)
class URTHexFillToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Superficie applicata alle celle della regione. */
	UPROPERTY(EditAnywhere, Category = "Hex|Fill")
	ERTHexSurface Surface = ERTHexSurface::Normal;

	/** Costo di movimento applicato (intero). */
	UPROPERTY(EditAnywhere, Category = "Hex|Fill", meta = (ClampMin = "0"))
	int32 MoveCost = 1;

	/** Blocca il movimento. */
	UPROPERTY(EditAnywhere, Category = "Hex|Fill")
	bool bBlocksMovement = false;

	/** Cella di partenza dell'ultimo riempimento (sola lettura). */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Fill")
	FRTCellId LastCell;

	/** Numero di celle riempite nell'ultimo flood (sola lettura). */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Fill")
	int32 FilledCount = 0;
};

/**
 * Secchiello (flood-fill): click su una cella esistente -> riempie l'intera regione contigua della stessa superficie
 * col pennello corrente, in un solo Undo. Click su cella vuota (assente nell'asset) -> nessuna azione.
 */
UCLASS()
class URTHexFillTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	UPROPERTY()
	TObjectPtr<URTHexFillToolProperties> Properties;

	UWorld* TargetWorld = nullptr;

	bool bHasMarker = false;
	FVector MarkerCenter = FVector::ZeroVector;
	float MarkerRadius = 50.f;
};
