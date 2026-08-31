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

	/**
	 * GLI ANCHOR AGGANCIATI, per nome — `#1895`.
	 *
	 * 🔑 Il criterio dell'issue e' *«quale anchor e' agganciato si vede, non si indovina dalla posizione del
	 * ghost»*: e' questa riga a soddisfarlo, e per questo porta i NOMI (`C`, `V3`, `E1`) e non le coordinate.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	FString SnappedAnchors;

	/**
	 * PERCHE' il gesto corrente non produce un muro, vuota quando lo produce.
	 *
	 * ⚠️ Il testo non si compone qui: arriva da `RTHexAnchor::Describe`, che traduce un reason code deciso
	 * dal runtime. Comporlo nel tool sarebbe decidere quale rifiuto e' quale, cioe' una regola nel modulo
	 * senza autorita'.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	FString Refusal;

	/**
	 * L'INCIDENZA con un muro GIA' presente nella cella, vuota quando non ce n'e' — `#1895` parte 4.
	 *
	 * ⚠️ Diversa da `Refusal`: quello dice che il muro **non si fa**, questa che si fa **insieme a un
	 * difetto**. La riga nomina l'altro muro per indice, perche' il criterio e' che l'incidenza si veda
	 * sui **due** segmenti coinvolti.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Hex|Geometria")
	FString Incidence;
};

/**
 * IL GESTO DELL'AUTORE — `#712`: si trascina il mouse e nasce un muro.
 *
 * ⚠️ **Questo tool non contiene una sola regola.** Le tre che servono vivono nel modulo runtime, dove
 * esistono i test, e qui vengono CHIAMATE:
 *
 * ```text
 * URTGeometryGrammarLibrary::NearestAnchor         a quale punto il gesto si aggancia
 * URTGeometryGrammarLibrary::ExplainPair           se quella coppia si dice, e perche' no
 * URTGeometryGrammarLibrary::SegmentBetweenAnchors quale segmento ne esce
 * URTGeometryBakeLibrary::AddSegmentsToCell        che cosa produce
 * ```
 *
 * 🔴 **`SnapToGrammar` NON e' piu' la via del gesto — `#1895`, `GEO-8` di `D-288`.** Quella funzione e'
 * deliberatamente tollerante — *«tiene l'asse che sbaglia meno»* — e sulle ventiquattro coppie di anchor
 * che nessun asse tattico porta **non fallisce**: produce un muro legale e DIVERSO da quello chiesto, senza
 * dirlo. Per un'interfaccia d'authoring e' il peggio possibile, perche' riesce.
 * `RefactorTactics.Anchor.SnapNeverInventsTheInexpressible` lo misura, e resta il pin di quel comportamento.
 *
 * ⚠️ La frase del rifiuto la compone `RTHexAnchor::Describe`, che **traduce** un reason code deciso dal
 * runtime: comporla qui sarebbe decidere quale rifiuto e' quale, cioe' di nuovo una regola.
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

	/**
	 * I due anchor su cui il gesto si e' agganciato — `#1895`. L'overlay li disegna piu' grandi, ed e' cio'
	 * che rende l'aggancio dichiarato invece che indovinato.
	 */
	FRTAnchorRef AnchorFrom;
	FRTAnchorRef AnchorTo;

	/**
	 * L'incidenza fra il segmento in corso e uno gia' presente — `#1895` parte 4.
	 *
	 * Due campi e non la struct interna: quella vive nel namespace anonimo del `.cpp`, dove sta la
	 * chiamata al validator, e non deve affacciarsi sull'header per essere ricordata fra un fotogramma e
	 * l'altro. `IncidentWallIndex` indicizza `InteriorWalls` dell'asset, non il gruppo della cella.
	 */
	ERTGeometryViolation IncidentViolation = ERTGeometryViolation::None;
	int32 IncidentWallIndex = INDEX_NONE;

	/** Il segmento che lo snap ha prodotto, e se ne ha prodotto uno. */
	FRTGeometrySegment Preview;
	bool bPreviewValid = false;
	bool bDragging = false;
};
