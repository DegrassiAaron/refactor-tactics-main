#include "Tools/RTHexSelectTool.h"
#include "RTHexEditorClick.h"
#include "RTHexSelectionStore.h"
#include "Map/RTHexLibrary.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
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

	// Il pannello parte dal piano su cui l'actor e' gia' impostato: senza questo mostrerebbe 0 finche' non si
	// clicca, e il primo cambio dal pannello riporterebbe la mappa al piano sbagliato.
	if (const ARTHexMapActor* Actor = FindTargetMapActor())
	{
		Properties->ActiveLayer = Actor->ActiveLayer;
	}

	AddToolPropertySource(Properties);
}

void URTHexSelectTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	if (!Properties || PropertySet != Properties || !Property) { return; }
	if (Property->GetFName() != GET_MEMBER_NAME_CHECKED(URTHexSelectToolProperties, ActiveLayer)) { return; }

	ARTHexMapActor* Actor = FindTargetMapActor();
	if (!Actor)
	{
		Properties->ActiveLayer = 0;
		UE_LOG(LogTemp, Warning, TEXT("[HexMode] Nessun ARTHexMapActor bersaglio: layer attivo non applicato."));
		return;
	}
	RTHexEditor::SetActiveLayer(Actor, Properties->ActiveLayer);
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
	FVector ClickedPoint;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld, Actor, ClickPos, Cell, Center, &ClickedPoint)) { return; }

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

	// L'elemento sotto il click, e il CICLO: ri-cliccare lo stesso punto scende al candidato successivo.
	// Il bordo mirato lo decide `NearestEdgeDirection`, che sta nel runtime ed e' provata headless — qui non
	// si ricava nessun angolo.
	if (URTHexSelectionStore* Store = GEditor ? GEditor->GetEditorSubsystem<URTHexSelectionStore>() : nullptr)
	{
		FVector Origin = FVector::ZeroVector;
		float HexSize = 0.f;
		float LayerH = 0.f;
		Actor->GetHexContext(Origin, HexSize, LayerH);

		const ERTHexDirection Edge =
			URTHexLibrary::NearestEdgeDirection(Cell, ClickedPoint, Origin, HexSize, LayerH);

		// Ctrl aggiunge invece di sostituire: e' la multi-selezione condivisa che #1864 chiede.
		const bool bAdditive = FSlateApplication::IsInitialized()
			&& FSlateApplication::Get().GetModifierKeys().IsControlDown();

		if (bAdditive)
		{
			Store->AddAt(Map, Cell, Edge);
		}
		else
		{
			Store->SelectAt(Map, Cell, Edge);
		}

		Properties->SelectedCount = Store->GetSelection().Num();
		Properties->SelectedElement = URTHexSelectionStore::Describe(Store->GetSelection());
	}

	UE_LOG(LogTemp, Log, TEXT("[HexMode] Selezione %s -> %s (%d elementi)."),
		*Cell.ToString(), *Properties->SelectedElement, Properties->SelectedCount);
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
