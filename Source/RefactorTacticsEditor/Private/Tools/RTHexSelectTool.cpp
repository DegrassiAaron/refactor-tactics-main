#include "Tools/RTHexSelectTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h" // FRTHexCellData (readout)

#define LOCTEXT_NAMESPACE "URTHexSelectTool"

UInteractiveTool* URTHexSelectToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexSelectTool* NewTool = NewObject<URTHexSelectTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexSelectTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexSelectTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexSelectToolProperties>(this);
	AddToolPropertySource(Properties);
}

ARTHexMapActor* URTHexSelectTool::FindTargetMapActor() const
{
	return RTHexEditor::FindTargetMapActor(TargetWorld);
}

void URTHexSelectTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	bHasSelection = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = FindTargetMapActor();
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio (selezionane uno se ce ne sono piu' di uno)."));
		return;
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)) { return; }

	Properties->ActiveLayer = Actor->ActiveLayer;
	Properties->SelectedCell = Cell;

	// Readout dati cella: superficie/costo/blocco se la cella esiste nell'asset.
	const URTHexMapAsset* Map = Actor->MapAsset;
	const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
	Properties->bSelectedCellExists = (Data != nullptr);
	if (Data)
	{
		Properties->Surface = Data->Surface;
		Properties->MoveCost = Data->MoveCost;
		Properties->bBlocksMovement = Data->bBlocksMovement;
	}

	SelectedWorldCenter = Center;
	MarkerRadius = (Map ? Map->HexSize : Actor->HexSize) * 0.9f;
	bHasSelection = true;

	UE_LOG(LogTemp, Log, TEXT("[HexMode] Selezione %s (esiste=%d) su layer %d."),
		*Cell.ToString(), Properties->bSelectedCellExists ? 1 : 0, Actor->ActiveLayer);
}

void URTHexSelectTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	if (Properties && Properties->bShowOverlay)
	{
		RTHexEditor::DrawSurfaceOverlay(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	}
	if (bHasSelection)
	{
		RTHexEditor::DrawHexMarker(PDI, SelectedWorldCenter, MarkerRadius, FColor::Yellow);
	}
}

#undef LOCTEXT_NAMESPACE
