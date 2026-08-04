#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "RTHexMapActor.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class URTHexMapAsset;

/** Modalita' di visualizzazione dei layer (H4): tutti i piani impilati, oppure solo il layer attivo. */
UENUM(BlueprintType)
enum class ERTLayerViewMode : uint8
{
	AllLayers,  // mostra tutte le celle di tutti i layer (impilate per quota)
	ActiveOnly  // mostra solo le celle del layer attivo (isola il piano)
};

/**
 * Visualizzatore della mappa esagonale: genera un'ISTANZA per cella (ISM), NON un Actor per cella. Nessuna
 * autorita' sui dati: legge le celle da URTHexMapAsset (o genera un graybox demo se l'asset e' assente).
 * La logica (coordinate/pathfinding) resta separata dal rendering.
 */
UCLASS()
class REFACTORTACTICS_API ARTHexMapActor : public AActor
{
	GENERATED_BODY()

public:
	ARTHexMapActor();

	/** Sorgente autorevole delle celle (se assente/vuota, si usa il graybox demo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	TObjectPtr<URTHexMapAsset> MapAsset;

	/** Mesh della cella (fallback: cilindro engine appiattito = disco graybox). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	TSoftObjectPtr<UStaticMesh> CellMesh;

	/** Dimensione esagono (cm) usata se MapAsset e' assente. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	float HexSize = 100.f;

	/** Quota tra layer (cm) usata se MapAsset e' assente. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	float LayerHeight = 250.f;

	/** [H4] Layer attivo: usato per il filtro di visualizzazione e come layer di generazione/painting. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Layer")
	int32 ActiveLayer = 0;

	/** [H4] Come mostrare i layer: tutti impilati, o solo quello attivo (isola il piano; la viz non li confonde). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Layer")
	ERTLayerViewMode LayerView = ERTLayerViewMode::AllLayers;

	/** Se MapAsset e' assente/vuoto, genera un esagono pieno di questo raggio (0 = niente demo). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	int32 DemoRadius = 4;

	/** Superficie assegnata dal generatore editor (GenerateIntoAsset). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap")
	ERTHexSurface DemoSurface = ERTHexSurface::Normal;

	/** [Editor] Genera nell'asset assegnato un esagono pieno (DemoRadius, DemoSurface), marca dirty e ridisegna. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap")
	void GenerateIntoAsset();

	/** [Editor] Svuota le celle e le transizioni dell'asset, marca dirty e ridisegna. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap")
	void ClearAsset();

	/** [Editor] Esegue il validator sull'asset e logga gli errori. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap")
	void ValidateAsset();

	/** [Editor] Cella bersaglio del painting per-cella. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Paint")
	FRTCellId PaintCellTarget;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Paint")
	ERTHexSurface PaintSurface = ERTHexSurface::Normal;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Paint")
	int32 PaintMoveCost = 1;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Paint")
	bool bPaintBlocksMovement = false;

	/** [Editor] Applica superficie/costo/blocco a PaintCellTarget (la crea se assente). Annullabile (Undo/Redo). */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap|Paint")
	void PaintTargetCell();

	/** Scrive Surface/MoveCost/bBlocksMovement sulla cella Id (la crea se assente, preserva Height/LOS). Annullabile. */
	void PaintCellData(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement);

	/** Rimuove la cella Id dall'asset. Vero se esisteva. Annullabile. */
	bool EraseCell(const FRTCellId& Id);

	/** [H4] Cella di partenza della transizione verticale/speciale (bridge/tunnel/scale/ascensore). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	FRTCellId TransitionFrom;

	/** [H4] Cella di arrivo della transizione (tipicamente su un altro layer). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	FRTCellId TransitionTo;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	int32 TransitionCost = 2;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	ERTHexTransitionKind TransitionKind = ERTHexTransitionKind::Stair;

	/** Se vero, crea/rimuove anche l'arco inverso (transizione percorribile nei due sensi). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|HexMap|Transition")
	bool bTransitionBidirectional = true;

	/** [Editor] Aggiunge la transizione TransitionFrom->TransitionTo (e l'inversa se bidirezionale). Annullabile. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap|Transition")
	void AddVerticalTransition();

	/** Aggiunge la transizione From->To (e l'inversa se bidirezionale) se entrambe le celle esistono. Annullabile. */
	void AddTransitionData(const FRTCellId& From, const FRTCellId& To, int32 Cost,
		ERTHexTransitionKind Kind, bool bBidirectional);

	/** [Editor] Rimuove la transizione TransitionFrom->TransitionTo (e l'inversa se bidirezionale). Annullabile. */
	UFUNCTION(CallInEditor, Category = "RefactorTactics|HexMap|Transition")
	void RemoveVerticalTransition();

	/** Rimuove la transizione From->To (e l'inversa se bBothDirections) dall'asset. Vero se ha rimosso. Annullabile. */
	bool RemoveTransitionData(const FRTCellId& From, const FRTCellId& To, bool bBothDirections);

	/** Ricostruisce tutte le istanze dalle celle (asset o demo). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "RefactorTactics|HexMap")
	void RebuildInstances();

	/** Cella corrispondente a un'istanza (INDEX_NONE fuori range). */
	FRTCellId CellForInstance(int32 InstanceIndex) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TObjectPtr<UInstancedStaticMeshComponent> Cells;

	/** Mapping instance index -> FRTCellId (per selezione/debug). */
	TArray<FRTCellId> InstanceCells;
};
