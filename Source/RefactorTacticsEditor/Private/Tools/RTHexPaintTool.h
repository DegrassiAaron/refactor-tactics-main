#pragma once

#include "BaseTools/ClickDragTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexSurface (brush)
#include "ScopedTransaction.h" // TUniquePtr<FScopedTransaction> membro
#include "RTHexPaintTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Operazione del pennello: dipinge (crea/aggiorna) o cancella (rimuove) la cella. */
UENUM()
enum class ERTHexPaintOp : uint8
{
	Paint,
	Erase
};

/** Factory del tool di paint. */
UCLASS()
class URTHexPaintToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Palette minima del pennello (invariata rispetto a H5c.1). */
UCLASS(Transient)
class URTHexPaintToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	ERTHexPaintOp Operation = ERTHexPaintOp::Paint;

	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	ERTHexSurface Surface = ERTHexSurface::Normal;

	UPROPERTY(EditAnywhere, Category = "Hex|Pennello", meta = (ClampMin = "0"))
	int32 MoveCost = 1;

	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	bool bBlocksMovement = false;

	UPROPERTY(VisibleAnywhere, Category = "Hex")
	int32 ActiveLayer = 0;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Ultimo")
	FRTCellId LastCell;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Ultimo")
	bool bLastExisted = false;
};

/**
 * Drag-brush: dipinge/cancella celle cliccando o trascinando nel viewport. Una pennellata (press->release) = una
 * transazione (un Undo). Click singolo = pennellata di 1 cella. Scrive via le primitive di stroke di URTHexMapAsset.
 */
UCLASS()
class URTHexPaintTool : public UClickDragTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual FInputRayHit CanBeginClickDragSequence(const FInputDeviceRay& PressPos) override;
	virtual void OnClickPress(const FInputDeviceRay& PressPos) override;
	virtual void OnClickDrag(const FInputDeviceRay& DragPos) override;
	virtual void OnClickRelease(const FInputDeviceRay& ReleasePos) override;
	virtual void OnTerminateDragSequence() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	void ApplyOne(ARTHexMapActor* Actor, const FRTCellId& Cell, const FVector& Center);
	void EndStrokeIfActive();

	UPROPERTY()
	TObjectPtr<URTHexPaintToolProperties> Properties;

	UPROPERTY()
	TObjectPtr<ARTHexMapActor> TargetActor;

	UWorld* TargetWorld = nullptr;

	TUniquePtr<FScopedTransaction> StrokeTransaction;
	bool bStrokeActive = false;
	TSet<FRTCellId> PaintedThisStroke;

	bool bHasMarker = false;
	FVector MarkerCenter = FVector::ZeroVector;
	float MarkerRadius = 50.f;
	FColor MarkerColor = FColor::Green;
};
