#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexTransitionKind
#include "RTHexArchTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;
class UTransformProxy;
class UCombinedTransformGizmo;

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

	/** Back-pointer al tool per i bottoni (impostato in Setup). */
	TWeakObjectPtr<class URTHexArchTool> WeakTool;

	/** Crea la transizione From->To con i parametri correnti. */
	UFUNCTION(CallInEditor, Category = "Hex|Arco")
	void Commit();

	/** Annulla l'arco pendente (nessuna scrittura). */
	UFUNCTION(CallInEditor, Category = "Hex|Arco")
	void ClearArch();
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
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	void CommitArch();
	void ClearPending();

protected:
	void OnGizmoMoved(UTransformProxy* InProxy, FTransform InTransform);
	void DestroyPendingGizmo();

	UPROPERTY()
	TObjectPtr<UTransformProxy> Proxy;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> Gizmo;

	UPROPERTY()
	TObjectPtr<ARTHexMapActor> TargetActor = nullptr;

	FRTCellId From;
	FRTCellId To;
	bool bHasFrom = false;
	bool bToValid = false;
	bool bSnapping = false;
	FVector FromWorld = FVector::ZeroVector;
	FVector ToWorld = FVector::ZeroVector;
	float MarkerRadius = 90.f;

	UPROPERTY()
	TObjectPtr<URTHexArchToolProperties> Properties;

	UWorld* TargetWorld = nullptr;
};
