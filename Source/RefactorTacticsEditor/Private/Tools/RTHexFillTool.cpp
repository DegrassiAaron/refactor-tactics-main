#include "Tools/RTHexFillTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "ScopedTransaction.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

#define LOCTEXT_NAMESPACE "URTHexFillTool"

UInteractiveTool* URTHexFillToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexFillTool* NewTool = NewObject<URTHexFillTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexFillTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexFillTool::Setup()
{
	USingleClickTool::Setup();
	Properties = NewObject<URTHexFillToolProperties>(this);
	AddToolPropertySource(Properties);
}

void URTHexFillTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	bHasMarker = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor || !Actor->MapAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Fill: nessun ARTHexMapActor con MapAsset."));
		return;
	}
	URTHexMapAsset* Map = Actor->MapAsset;

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center)) { return; }

	// Il secchiello riempie solo regioni ESISTENTI: cella vuota -> nessuna azione (non crea celle nuove).
	if (!Map->FindCell(Cell))
	{
		UE_LOG(LogTemp, Log, TEXT("[HexMode] Fill: cella %s vuota, niente da riempire."), *Cell.ToString());
		return;
	}

	const TArray<FRTCellId> Region = Map->FloodRegion(Cell);
	if (Region.Num() == 0) { return; }

	{
		const FScopedTransaction Transaction(LOCTEXT("HexFill", "Hex: Flood Fill"));
		Map->BeginStroke();
		for (const FRTCellId& C : Region)
		{
			Map->PaintCellInStroke(C, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement);
		}
		Map->EndStroke();
		Actor->RebuildInstances();
	}

	Properties->LastCell = Cell;
	Properties->FilledCount = Region.Num();
	MarkerCenter = Center;
	MarkerRadius = Map->HexSize * 0.9f;
	bHasMarker = true;

	UE_LOG(LogTemp, Log, TEXT("[HexMode] Fill: %d celle riempite dalla regione di %s."), Region.Num(), *Cell.ToString());
}

void URTHexFillTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasMarker || !RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }
	RTHexEditor::DrawHexMarker(PDI, MarkerCenter, MarkerRadius, FColor(120, 255, 120));
}

#undef LOCTEXT_NAMESPACE
