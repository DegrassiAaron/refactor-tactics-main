#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexSurface (brush)
#include "RTHexPaintTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Operazione del pennello: dipinge (crea/aggiorna) o cancella (rimuove) la cella cliccata. */
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

/** Palette minima del pennello (property set del tool). */
UCLASS(Transient)
class URTHexPaintToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Operazione: Paint (crea/aggiorna) o Erase (rimuove). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	ERTHexPaintOp Operation = ERTHexPaintOp::Paint;

	/** Superficie da applicare (modalita' Paint). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	ERTHexSurface Surface = ERTHexSurface::Normal;

	/** Costo di movimento da applicare (modalita' Paint). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello", meta = (ClampMin = "0"))
	int32 MoveCost = 1;

	/** Blocca il movimento (modalita' Paint). */
	UPROPERTY(EditAnywhere, Category = "Hex|Pennello")
	bool bBlocksMovement = false;

	/** Layer attivo (sola lettura: rispecchia ARTHexMapActor::ActiveLayer). */
	UPROPERTY(VisibleAnywhere, Category = "Hex")
	int32 ActiveLayer = 0;

	/** Ultima cella toccata dal pennello. */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Ultimo")
	FRTCellId LastCell;

	/** La cella esisteva prima dell'operazione? (Paint: gia' presente; Erase: presente e rimossa). */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Ultimo")
	bool bLastExisted = false;
};

/**
 * Dipinge o cancella una cella cliccando nel viewport: ray -> punto-mondo (ISM del target o piano del layer attivo)
 * -> WorldToAxial -> PaintCellData/EraseCell sull'actor. Marker verde (paint) / rosso (erase).
 */
UCLASS()
class URTHexPaintTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	UPROPERTY()
	TObjectPtr<URTHexPaintToolProperties> Properties;

	UWorld* TargetWorld = nullptr;

	bool bHasMarker = false;
	FVector MarkerCenter = FVector::ZeroVector;
	float MarkerRadius = 50.f;
	FColor MarkerColor = FColor::Green;
};
