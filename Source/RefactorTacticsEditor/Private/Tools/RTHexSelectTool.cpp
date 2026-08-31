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
	// La selezione si disegna DALLO STORE, cosi' cio' che si vede e cio' che il readout dichiara sono la
	// stessa cosa. Disegnare la cella comunque — anche quando e' selezionata una copertura — mostrerebbe un
	// bersaglio diverso da quello che `Canc` porterebbe via.
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld);
	URTHexSelectionStore* Store = GEditor ? GEditor->GetEditorSubsystem<URTHexSelectionStore>() : nullptr;

	if (Store && Actor && Store->GetSelection().Num() > 0)
	{
		FVector Origin = FVector::ZeroVector;
		float HexSize = 0.f;
		float LayerH = 0.f;
		Actor->GetHexContext(Origin, HexSize, LayerH);

		for (const FRTMapElementHandle& Handle : Store->GetSelection())
		{
			DrawSelectedElement(PDI, Actor, Handle, Origin, HexSize, LayerH);
		}
		return;
	}

	if (bHasSelection)
	{
		RTHexEditor::DrawHexMarker(PDI, SelectedWorldCenter, MarkerRadius, FColor::Yellow);
	}
}

void URTHexSelectTool::DrawSelectedElement(FPrimitiveDrawInterface* PDI, ARTHexMapActor* Actor,
	const FRTMapElementHandle& Handle, const FVector& Origin, float HexSize, float LayerHeight) const
{
	// Alzato da terra quanto il marker della cella: sotto la superficie non si vedrebbe.
	const FVector Lift(0.0, 0.0, 3.0);
	constexpr float Thick = 4.0f;

	switch (Handle.Kind)
	{
	case ERTMapElementKind::Cell:
	{
		const FVector Centre = URTHexLibrary::AxialToWorld(Handle.Cell, Origin, HexSize, LayerHeight);
		RTHexEditor::DrawHexMarker(PDI, Centre, HexSize * 0.9f, FColor::Yellow);
		break;
	}

	case ERTMapElementKind::Cover:
	case ERTMapElementKind::Door:
	{
		// Il LATO, non la cella: si disegna fra i due vertici piu' vicini al centro del bordo.
		//
		// ⚠️ Trovati per distanza invece che per indice: la corrispondenza «bordo N ↔ vertici N e N+1» e' una
		// convenzione che vive dentro `HexCorners`, e riscriverla qui sarebbe la seconda copia che prima o
		// poi diverge. Per distanza il risultato e' corretto per costruzione.
		const FVector Mid = URTHexLibrary::EdgeMidpointWorld(Handle.Cell, Handle.Edge, Origin, HexSize, LayerHeight);
		const FVector Centre = URTHexLibrary::AxialToWorld(Handle.Cell, Origin, HexSize, LayerHeight);

		TArray<FVector> Corners = URTHexLibrary::HexCorners(Centre, HexSize);
		Corners.Sort([&Mid](const FVector& A, const FVector& B)
		{
			return FVector::DistSquaredXY(A, Mid) < FVector::DistSquaredXY(B, Mid);
		});

		if (Corners.Num() >= 2)
		{
			const FColor Colour = (Handle.Kind == ERTMapElementKind::Door) ? FColor::Cyan : FColor::Orange;
			PDI->DrawLine(Corners[0] + Lift, Corners[1] + Lift, Colour, SDPG_Foreground, Thick);
		}
		break;
	}

	case ERTMapElementKind::InteriorWall:
	{
		// La GIACITURA vera del muro, non un simbolo al centro della cella: e' l'unico modo per distinguere
		// due muri interni sulla stessa cella, che e' precisamente il caso che il ciclo deve saper scorrere.
		const URTHexMapAsset* Map = Actor->MapAsset;
		const int32 Index = URTMapEditLibrary::ResolveInteriorWall(Map, Handle);
		if (Index == INDEX_NONE)
		{
			break;
		}

		const FRTHexInteriorWall& Wall = Map->InteriorWalls[Index];
		const FVector Centre = URTHexLibrary::AxialToWorld(Wall.Cell, Origin, HexSize, LayerHeight);

		// `ToPolyline` e' il derivato di calcolo del segmento: il float nasce qui, a valle dell'authority.
		const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Wall.Segment, HexSize);
		for (int32 I = 0; I + 1 < Line.Points.Num(); ++I)
		{
			const FVector A(Centre.X + Line.Points[I].X, Centre.Y + Line.Points[I].Y, Centre.Z);
			const FVector B(Centre.X + Line.Points[I + 1].X, Centre.Y + Line.Points[I + 1].Y, Centre.Z);
			PDI->DrawLine(A + Lift, B + Lift, FColor::Green, SDPG_Foreground, Thick);
		}
		break;
	}

	default:
		break;
	}
}

#undef LOCTEXT_NAMESPACE
