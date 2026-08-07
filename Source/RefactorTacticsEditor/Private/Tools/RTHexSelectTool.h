#pragma once

#include "BaseTools/SingleClickTool.h"
#include "Map/RTHexCellData.h" // FRTCellId + ERTHexSurface (readout selezione)
#include "RTHexSelectTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Factory del tool di selezione. */
UCLASS()
class URTHexSelectToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/** Proprieta' del tool: readout (sola lettura) della selezione. Il layer attivo e' quello dell'ARTHexMapActor
 *  (fonte unica: pilota anche la visualizzazione ActiveOnly), qui solo rispecchiato. */
UCLASS(Transient)
class URTHexSelectToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Layer attivo (sola lettura: rispecchia ARTHexMapActor::ActiveLayer). */
	UPROPERTY(VisibleAnywhere, Category = "Hex")
	int32 ActiveLayer = 0;

	/** Ultima cella selezionata (q, r, Layer). */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Selezione")
	FRTCellId SelectedCell;

	/** La cella selezionata esiste nell'asset? */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Selezione")
	bool bSelectedCellExists = false;

	/** Dati della cella (validi se bSelectedCellExists) — readout richiesto dalla spec §3. */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Selezione")
	ERTHexSurface Surface = ERTHexSurface::Floor;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Selezione")
	int32 MoveCost = 0;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Selezione")
	bool bBlocksMovement = false;

	/** [Overlay] Colora le celle per superficie (debug read-only); le bloccate con esagono rosso. */
	UPROPERTY(EditAnywhere, Category = "Hex|Overlay")
	bool bShowOverlay = false;
};

/**
 * Seleziona una cella cliccando nel viewport: ray -> punto-mondo (ISM del target o piano del layer attivo) ->
 * WorldToAxial -> lookup. Non modifica dati (sola selezione); un esagono evidenzia la cella. La selezione con
 * modifica e la multi-selezione arrivano dopo (H5c).
 */
UCLASS()
class URTHexSelectTool : public USingleClickTool
{
	GENERATED_BODY()
public:
	virtual void SetWorld(UWorld* World);
	virtual void Setup() override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

protected:
	UPROPERTY()
	TObjectPtr<URTHexSelectToolProperties> Properties;

	UWorld* TargetWorld = nullptr;

	/** ARTHexMapActor bersaglio: quello selezionato, altrimenti l'unico presente nel mondo. */
	ARTHexMapActor* FindTargetMapActor() const;

	bool bHasSelection = false;
	FVector SelectedWorldCenter = FVector::ZeroVector;
	float MarkerRadius = 50.f;
};
