#pragma once

#include "BaseTools/ClickDragTool.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h" // ERTHexCoverType
#include "RTHexGeometryTool.generated.h"

class ARTHexMapActor;
class IToolsContextRenderAPI;

/** Factory del tool geometria. */
UCLASS()
class URTHexGeometryToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/**
 * Palette del tool geometria: che cosa si sta disegnando, e che cosa lo snap ha capito.
 *
 * I campi `VisibleAnywhere` sono un READOUT del segmento agganciato — servono a rendere leggibile ciò che
 * il ghost mostra, ed e' l'unico modo per verificare in seduta che lo snap stia facendo la cosa giusta.
 */
UCLASS(Transient)
class URTHexGeometryToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** `High` e' il muro pieno, `Low` il muretto. Sono i due tipi del thin slice: il footprint non e' di #712. */
	UPROPERTY(EditAnywhere, Category = "Hex|Geometria")
	ERTHexCoverType WallType = ERTHexCoverType::High;

	/** L'asse su cui lo snap ha agganciato il gesto corrente. */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	ERTTacticalAxis SnappedAxis = ERTTacticalAxis::Deg0;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	int32 SnappedOffset = 0;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	int32 SnappedFrom = 0;

	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	int32 SnappedTo = 0;

	/** La cella investita dal gesto. */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	FRTCellId Cell;

	/** Quante coperture ha prodotto l'ultimo commit. Zero significa «il bordo era gia' dell'autore». */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	int32 LastBakedCovers = 0;
};

/**
 * IL GESTO DELL'AUTORE — `#712`: si trascina il mouse e nasce un muro.
 *
 * ⚠️ **Questo tool non contiene una sola regola.** Le tre che servono vivono nel modulo runtime, dove
 * esistono i test, e qui vengono CHIAMATE:
 *
 * ```text
 * URTGeometryGrammarLibrary::SnapToGrammar   dove puo' stare il segmento
 * URTGeometryGrammarLibrary::ValidateSegment se e' legale        (chiamata dentro lo snap)
 * URTGeometryBakeLibrary::BakeCell           che cosa produce
 * ```
 *
 * Cio' che resta qui e' cio' che e' davvero d'interfaccia e non e' verificabile headless: il ghost, il
 * feedback del drag, e la transazione — **una gesture = un `Ctrl+Z`**.
 *
 * `UClickDragTool` invece di `USingleClickTool` perche' un muro ha due estremi: il gesto e' un trascinamento,
 * e il ghost deve aggiornarsi mentre lo si compie — altrimenti «si vede prima di rilasciare» sarebbe falso.
 */
UCLASS()
class URTHexGeometryTool : public UClickDragTool
{
	GENERATED_BODY()
public:
	virtual void Setup() override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual FInputRayHit CanBeginClickDragSequence(const FInputDeviceRay& PressPos) override;
	virtual void OnClickPress(const FInputDeviceRay& PressPos) override;
	virtual void OnClickDrag(const FInputDeviceRay& DragPos) override;
	virtual void OnClickRelease(const FInputDeviceRay& ReleasePos) override;
	virtual void OnTerminateDragSequence() override;

	void SetWorld(UWorld* World);

	UPROPERTY()
	TObjectPtr<URTHexGeometryToolProperties> Properties = nullptr;

private:
	/** Aggiorna lo snap dal punto corrente del drag. Non commette nulla: e' solo il ghost. */
	void UpdatePreview(const FInputDeviceRay& Ray);

	/** Il piano del layer attivo, in world: dove il raggio del mouse atterra. */
	bool ProjectToCellPlane(const FInputDeviceRay& Ray, FVector& OutWorld) const;

	UPROPERTY()
	TWeakObjectPtr<UWorld> TargetWorld = nullptr;

	/** Estremi del gesto in coordinate LOCALI della cella investita. */
	FVector2D LocalStart = FVector2D::ZeroVector;
	FVector2D LocalEnd = FVector2D::ZeroVector;

	/** La cella su cui il gesto e' iniziato: un gesto appartiene a una cella sola. */
	FRTCellId ActiveCell;

	/** Il segmento che lo snap ha prodotto, e se ne ha prodotto uno. */
	FRTGeometrySegment Preview;
	bool bPreviewValid = false;
	bool bDragging = false;
};
