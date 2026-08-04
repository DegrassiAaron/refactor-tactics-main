#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexTransitionKind
#include "RTHexArchTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Factory del tool archi. */
UCLASS()
class URTHexArchToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Palette del tool archi: parametri della transizione + readout. (Bottoni Commit/Clear aggiunti in H5c.2b.) */
UCLASS(Transient)
class URTHexArchToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Hex|Arco")
	ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair;

	UPROPERTY(EditAnywhere, Category = "Hex|Arco", meta = (ClampMin = "0"))
	int32 Cost = 2;

	UPROPERTY(EditAnywhere, Category = "Hex|Arco")
	bool bBidirectional = true;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	FRTCellId From;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	bool bHasFrom = false;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	FRTCellId To;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Arco")
	bool bToValid = false;
};

/**
 * Crea transizioni (FRTHexEdge) nel viewport: click su From, gizmo per To (H5c.2b). In H5c.2a disegna solo le
 * transizioni esistenti dell'asset (rese visibili per la prima volta). Non modifica dati in questa fetta.
 */
UCLASS()
class URTHexArchTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	UPROPERTY()
	TObjectPtr<URTHexArchToolProperties> Properties;

	UWorld* TargetWorld = nullptr;
};
