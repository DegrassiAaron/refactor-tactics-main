#include "Tools/RTHexSelectTool.h"
#include "RTHexEditorClick.h"
#include "RTHexSelectionStore.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTMapEditLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTHexMapActor.h"
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
		// ⏱️ **Queste venti righe vivevano QUI**, ed erano il gesto di selezione per intero: bordo
		// mirato, `Ctrl`, aggiungi-o-sostituisci. Sono salite in `RTHexEditor::ApplyClickToSelection`
		// perche' da `#1864` **anche Geometry** deve poter selezionare, e due stesure dello stesso gesto
		// divergono — e' la ragione per cui `DrawSelectedElement` era gia' salita di li'.
		RTHexEditor::ApplyClickToSelection(Actor, Cell, ClickedPoint);

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

	if (RTHexEditor::ShouldShowSurfaceOverlay(GetToolManager()))
	{
		RTHexEditor::DrawSurfaceOverlay(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	}

	// 🔑 **Le transizioni, con QUALUNQUE strumento attivo e senza dipendere da un toggle** (#1768).
	// Fuori dal blocco qui sopra di proposito: `bShowSurfaceOverlay` spegne i marcatori di superficie,
	// che sono una preferenza di chi dipinge — un arco assente dallo schermo e' invece una mappa che
	// mente per omissione, ed e' il difetto che #1768 chiude.
	RTHexEditor::DrawTransitions(PDI, RTHexEditor::FindTargetMapActor(TargetWorld));
	// La selezione si disegna DALLO STORE, cosi' cio' che si vede e cio' che il readout dichiara sono la
	// stessa cosa. Disegnare la cella comunque — anche quando e' selezionata una copertura — mostrerebbe un
	// bersaglio diverso da quello che `Canc` porterebbe via.
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	URTHexSelectionStore* Store = GEditor ? GEditor->GetEditorSubsystem<URTHexSelectionStore>() : nullptr;

	if (Store && Actor && Store->GetSelection().Num() > 0)
	{
		// 🔑 **Il disegno e' salito nel namespace condiviso** (#1864): una selezione che solo lo
		// strumento che l'ha fatta sa disegnare non e' condivisa, e' passata di mano. Ora Geometry e Arch
		// chiamano la stessa funzione, e cambiando strumento la selezione resta visibile.
		RTHexEditor::DrawSharedSelection(PDI, Actor);
		return;
	}

	if (bHasSelection)
	{
		RTHexEditor::DrawHexMarker(PDI, SelectedWorldCenter, MarkerRadius, FColor::Yellow);
	}
}


#undef LOCTEXT_NAMESPACE
