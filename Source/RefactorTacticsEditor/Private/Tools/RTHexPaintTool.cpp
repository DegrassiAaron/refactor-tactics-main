#include "Tools/RTHexPaintTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "InputState.h" // FInputDeviceRay / FInputRayHit
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h" // HexArea

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

bool URTHexPaintTool::ApplyBrushAt(ARTHexMapActor* Actor, const FRTCellId& CenterCell, const FVector& CenterWorld)
{
	URTHexMapAsset* Map = Actor->MapAsset; // il caller garantisce Actor && Map non nulli
	const bool bPaint = (Properties->Operation == ERTHexPaintOp::Paint);
	const bool bCenterExistedBefore = (Map->FindCell(CenterCell) != nullptr); // per il readout, pre-mutazione

	const TArray<FRTCellId> Area = URTHexLibrary::HexArea(CenterCell, FMath::Max(0, Properties->BrushRadius));
	bool bAnyChanged = false;
	for (const FRTCellId& C : Area)
	{
		if (PaintedThisStroke.Contains(C)) { continue; } // dedup per-cella nella pennellata
		const bool bApplied = bPaint
			? Map->PaintCellInStroke(C, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement)
			: Map->EraseCellInStroke(C);
		PaintedThisStroke.Add(C);
		bAnyChanged = bAnyChanged || bApplied;
	}

	// Readout/marker sul centro.
	MarkerColor = bPaint ? FColor::Green : FColor::Red;
	Properties->bLastExisted = bCenterExistedBefore;
	Properties->LastCell = CenterCell;
	Properties->ActiveLayer = Actor->ActiveLayer;
	MarkerCenter = CenterWorld;
	MarkerRadius = Map->HexSize * 0.9f;
	bHasMarker = true;
	return bAnyChanged;
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

	if (ApplyBrushAt(Actor, Cell, Center))
	{
		Actor->RebuildInstances();
	}
}

void URTHexPaintTool::OnClickDrag(const FInputDeviceRay& DragPos)
{
	if (!bStrokeActive || !TargetActor || !TargetActor->MapAsset) { return; }

	FRTCellId Cell;
	FVector Center;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, TargetActor, DragPos, Cell, Center)) { return; }

	if (ApplyBrushAt(TargetActor, Cell, Center))
	{
		TargetActor->RebuildInstances();
	}
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
