#include "Tools/RTHexPaintTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "InputState.h" // FInputDeviceRay / FInputRayHit
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h" // HexArea
#include "Terrain/RTTerrainLibrary.h"

#define LOCTEXT_NAMESPACE "URTHexPaintTool"

#if WITH_EDITOR
void URTHexPaintToolProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(URTHexPaintToolProperties, Surface))
	{
		const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(Surface);
		MoveCost = Def.MoveCost;
		bBlocksMovement = false; // nessun terreno del catalogo v0.1 blocca il movimento normale
	}
}
#endif

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

	// Il pannello parte dal piano su cui l'actor e' gia' impostato: senza questo mostrerebbe 0 finche' non si
	// dipinge, e il primo cambio dal pannello sposterebbe il pennello su un piano che l'utente non ha scelto.
	if (const ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld))
	{
		Properties->ActiveLayer = Actor->ActiveLayer;
	}

	AddToolPropertySource(Properties);
}

void URTHexPaintTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	if (!Properties || PropertySet != Properties || !Property) { return; }
	if (Property->GetFName() != GET_MEMBER_NAME_CHECKED(URTHexPaintToolProperties, ActiveLayer)) { return; }

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	if (!Actor)
	{
		Properties->ActiveLayer = 0;
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio: layer attivo non applicato."));
		return;
	}
	RTHexEditor::SetActiveLayer(Actor, Properties->ActiveLayer);
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
		// Apri la transazione/stroke LAZY solo se questa cella cambia davvero (paint cambia sempre; erase solo se esiste).
		if (bPaint || Map->ContainsCell(C))
		{
			EnsureStrokeOpen();
		}
		// Il flag della vista si passa SEMPRE: il pennello lo scrive come scrive `bBlocksMovement`, cosi'
		// dipingere con il flag spento su una cella che lo aveva lo toglie. Senza, il muro sarebbe
		// irreversibile dall'editor — il difetto opposto a quello che questa property risolve.
		const bool bApplied = bPaint
			? Map->PaintCellInStroke(C, Properties->Surface, Properties->MoveCost, Properties->bBlocksMovement,
				TOptional<bool>(Properties->bBlocksLineOfSight))
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

	// La transazione + BeginStroke si aprono LAZY al primo cambiamento reale (EnsureStrokeOpen): niente transazione
	// no-op quando l'erase non tocca celle esistenti (garanzia ripristinata rispetto al click singolo di H5c.1).
	TargetActor = Actor;
	bStrokeActive = true;
	bStrokeOpened = false;
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
	if (bStrokeOpened && TargetActor && TargetActor->MapAsset)
	{
		TargetActor->MapAsset->EndStroke();
		TargetActor->RebuildInstances(); // riallinea InstanceCells all'ordine post-SortCells
	}
	StrokeTransaction.Reset(); // chiude/commit la transazione (o no-op se non aperta)
	bStrokeActive = false;
	bStrokeOpened = false;
	TargetActor = nullptr;
	PaintedThisStroke.Reset();
}

void URTHexPaintTool::EnsureStrokeOpen()
{
	if (bStrokeOpened) { return; }
	StrokeTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("HexBrushStroke", "Hex: Brush Stroke"));
	if (TargetActor && TargetActor->MapAsset)
	{
		TargetActor->MapAsset->BeginStroke(); // Modify() dopo l'apertura della transazione: cattura lo stato pre-pennellata
	}
	bStrokeOpened = true;
}

void URTHexPaintTool::Shutdown(EToolShutdownType ShutdownType)
{
	EndStrokeIfActive(); // chiude uno stroke aperto a un cambio-tool/uscita mode a metà drag (S1)
	UClickDragTool::Shutdown(ShutdownType);
}

void URTHexPaintTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	if (Properties && Properties->bShowOverlay)
	{
		RTHexEditor::DrawSurfaceOverlay(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	}
	if (bHasMarker)
	{
		RTHexEditor::DrawHexMarker(PDI, MarkerCenter, MarkerRadius, MarkerColor);
	}
}

#undef LOCTEXT_NAMESPACE
