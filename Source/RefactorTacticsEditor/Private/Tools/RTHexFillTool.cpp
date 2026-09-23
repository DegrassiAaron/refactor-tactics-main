#include "Tools/RTHexFillTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "ScopedTransaction.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"

#define LOCTEXT_NAMESPACE "URTHexFillTool"

#if WITH_EDITOR
void URTHexFillToolProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(URTHexFillToolProperties, Surface))
	{
		const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(Surface);
		MoveCost = Def.MoveCost;
		bBlocksMovement = false; // nessun terreno del catalogo v0.1 blocca il movimento normale
	}
}
#endif

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
	if (!RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	// #921: PRIMA della guardia di stato qui sotto. L'overlay e' del MODE e non di questo strumento: se lo
	// disegnassimo dopo, le superfici non si vedrebbero finche' non si clicca almeno una volta — e il
	// secchiello e' lo strumento che DIPINGE superfici, cioe' il caso peggiore che #921 esiste per correggere.
	if (RTHexEditor::ShouldShowSurfaceOverlay(GetToolManager()))
	{
		RTHexEditor::DrawSurfaceOverlay(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	}

	// 🔑 **Le transizioni, con QUALUNQUE strumento attivo e senza dipendere da un toggle** (#1768).
	// Fuori dal blocco qui sopra di proposito: `bShowSurfaceOverlay` spegne i marcatori di superficie,
	// che sono una preferenza di chi dipinge — un arco assente dallo schermo e' invece una mappa che
	// mente per omissione, ed e' il difetto che #1768 chiude.
	RTHexEditor::DrawTransitions(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));

	if (!bHasMarker) { return; }
	RTHexEditor::DrawHexMarker(PDI, MarkerCenter, MarkerRadius, FColor(120, 255, 120));
}

#undef LOCTEXT_NAMESPACE
