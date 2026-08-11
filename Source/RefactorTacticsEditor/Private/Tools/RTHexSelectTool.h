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

/** Proprieta' del tool: readout della selezione, piu' il layer attivo. L'ARTHexMapActor resta la fonte unica
 *  (pilota anche la visualizzazione): qui si rispecchia e si riscrive, non si tiene uno stato parallelo. */
UCLASS(Transient)
class URTHexSelectToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/**
	 * Layer attivo: scriverlo qui lo scrive sull'ARTHexMapActor bersaglio e ne ricostruisce la vista.
	 * E' modificabile dal pannello del tool perche' cambiare piano dal Details dell'actor costringe a
	 * uscire dal tool, riselezionare l'actor e rientrare — a ogni cambio di piano.
	 */
	UPROPERTY(EditAnywhere, Category = "Hex")
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
	/** Propaga all'actor il layer attivo scritto nel pannello (le altre proprieta' sono di sola lettura). */
	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

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
