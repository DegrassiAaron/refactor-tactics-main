#include "Tools/RTHexPaintTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

#define LOCTEXT_NAMESPACE "URTHexPaintTool"

UInteractiveTool* URTHexPaintToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexPaintTool* NewTool = NewObject<URTHexPaintTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexPaintTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexPaintTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexPaintToolProperties>(this);
	AddToolPropertySource(Properties);
}

void URTHexPaintTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	bHasMarker = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio (selezionane uno se ce ne sono piu' di uno)."));
		return;
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)) { return; }

	const URTHexMapAsset* Map = Actor->MapAsset;
	const bool bExisted = (Map && Map->FindCell(Cell) != nullptr);

	if (Properties->Operation == ERTHexPaintOp::Paint)
	{
		Actor->PaintCellData(Cell, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement);
		MarkerColor = FColor::Green;
	}
	else // Erase
	{
		Actor->EraseCell(Cell);
		MarkerColor = FColor::Red;
	}

	Properties->ActiveLayer = Actor->ActiveLayer;
	Properties->LastCell = Cell;
	Properties->bLastExisted = bExisted;

	MarkerCenter = Center;
	MarkerRadius = (Map ? Map->HexSize : Actor->HexSize) * 0.9f;
	bHasMarker = true;

	UE_LOG(LogTemp, Log, TEXT("[HexMode] %s su %s (esisteva=%d) layer %d."),
		Properties->Operation == ERTHexPaintOp::Paint ? TEXT("Paint") : TEXT("Erase"),
		*Cell.ToString(), bExisted ? 1 : 0, Actor->ActiveLayer);
}

void URTHexPaintTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasMarker || !RenderAPI) { return; }
	RTHexEditor::DrawHexMarker(RenderAPI->GetPrimitiveDrawInterface(), MarkerCenter, MarkerRadius, MarkerColor);
}

#undef LOCTEXT_NAMESPACE
