#include "Tools/RTHexPaintTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "InputState.h" // FInputDeviceRay / FInputRayHit
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
	UClickDragTool::Setup();
	Properties = NewObject<URTHexPaintToolProperties>(this);
	AddToolPropertySource(Properties);
}

FInputRayHit URTHexPaintTool::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
	// Accetta ogni click nel viewport (la cella si risolve in OnClickPress). bHit=true via profondità.
	return FInputRayHit(TNumericLimits<double>::Max());
}

void URTHexPaintTool::ApplyOne(ARTHexMapActor* Actor, const FRTCellId& Cell, const FVector& Center)
{
	URTHexMapAsset* Map = Actor->MapAsset; // il caller garantisce Actor && Map non nulli
	if (Properties->Operation == ERTHexPaintOp::Paint)
	{
		Properties->bLastExisted = (Map->FindCell(Cell) != nullptr); // prima della mutazione
		Map->PaintCellInStroke(Cell, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement);
		MarkerColor = FColor::Green;
	}
	else
	{
		Properties->bLastExisted = Map->EraseCellInStroke(Cell);
		MarkerColor = FColor::Red;
	}
	PaintedThisStroke.Add(Cell);
	Properties->LastCell = Cell;
	Properties->ActiveLayer = Actor->ActiveLayer;
	MarkerCenter = Center;
	MarkerRadius = Map->HexSize * 0.9f;
	bHasMarker = true;
}

void URTHexPaintTool::OnClickPress(const FInputDeviceRay& PressPos)
{
	bHasMarker = false;
	if (!Properties) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor || !Actor->MapAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Paint: nessun ARTHexMapActor con MapAsset (nessuna pennellata)."));
		return; // niente stroke senza asset (M3)
	}

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, PressPos, Cell, Center)) { return; }

	StrokeTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("HexBrushStroke", "Hex: Brush Stroke"));
	TargetActor = Actor;
	Actor->MapAsset->BeginStroke();
	bStrokeActive = true;
	PaintedThisStroke.Reset();

	ApplyOne(Actor, Cell, Center);
	Actor->RebuildInstances();
}

void URTHexPaintTool::OnClickDrag(const FInputDeviceRay& DragPos)
{
	if (!bStrokeActive || !TargetActor || !TargetActor->MapAsset) { return; }

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, TargetActor, DragPos, Cell, Center)) { return; }
	if (PaintedThisStroke.Contains(Cell)) { return; } // dedup: trascinare all'indietro non ridipinge

	ApplyOne(TargetActor, Cell, Center);
	TargetActor->RebuildInstances();
}

void URTHexPaintTool::OnClickRelease(const FInputDeviceRay& ReleasePos)
{
	EndStrokeIfActive();
}

void URTHexPaintTool::OnTerminateDragSequence()
{
	EndStrokeIfActive();
}

void URTHexPaintTool::EndStrokeIfActive()
{
	if (bStrokeActive && TargetActor && TargetActor->MapAsset)
	{
		TargetActor->MapAsset->EndStroke();
		TargetActor->RebuildInstances(); // riallinea InstanceCells all'ordine post-SortCells
	}
	StrokeTransaction.Reset(); // chiude/commit la transazione (o no-op se non aperta)
	bStrokeActive = false;
	TargetActor = nullptr;
	PaintedThisStroke.Reset();
}

void URTHexPaintTool::Shutdown(EToolShutdownType ShutdownType)
{
	EndStrokeIfActive(); // chiude uno stroke aperto a un cambio-tool/uscita mode a metà drag (S1)
	UClickDragTool::Shutdown(ShutdownType);
}

void URTHexPaintTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasMarker || !RenderAPI) { return; }
	RTHexEditor::DrawHexMarker(RenderAPI->GetPrimitiveDrawInterface(), MarkerCenter, MarkerRadius, MarkerColor);
}

#undef LOCTEXT_NAMESPACE
