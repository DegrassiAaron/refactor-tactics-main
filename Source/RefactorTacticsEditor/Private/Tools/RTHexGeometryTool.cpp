#include "Tools/RTHexGeometryTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "PrimitiveDrawingUtils.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTGeometryBake.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "URTHexGeometryTool"

UInteractiveTool* URTHexGeometryToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URTHexGeometryTool* NewTool = NewObject<URTHexGeometryTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void URTHexGeometryTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void URTHexGeometryTool::Setup()
{
	UClickDragTool::Setup();
	Properties = NewObject<URTHexGeometryToolProperties>(this);
	AddToolPropertySource(Properties);
}

bool URTHexGeometryTool::ProjectToCellPlane(const FInputDeviceRay& Ray, FVector& OutWorld) const
{
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld.Get());
	if (Actor == nullptr)
	{
		return false;
	}

	FVector Origin; float HexSize = 0.f; float LayerHeight = 0.f;
	if (Actor->GetHexContext(Origin, HexSize, LayerHeight) == nullptr)
	{
		return false;
	}

	// Il gesto vive sul PIANO del layer attivo: un muro si disegna in pianta, e proiettare la' evita che la
	// profondita' del raggio sposti il segmento a seconda dell'inclinazione della camera.
	const double PlaneZ = Origin.Z + ActiveCell.Layer * LayerHeight;
	const double DirZ = Ray.WorldRay.Direction.Z;
	if (FMath::Abs(DirZ) <= UE_KINDA_SMALL_NUMBER)
	{
		return false; // raggio parallelo al piano: nessuna intersezione utile
	}

	const double T = (PlaneZ - Ray.WorldRay.Origin.Z) / DirZ;
	if (T <= 0.0)
	{
		return false; // il piano e' dietro la camera
	}

	OutWorld = Ray.WorldRay.Origin + Ray.WorldRay.Direction * T;
	return true;
}

void URTHexGeometryTool::UpdatePreview(const FInputDeviceRay& Ray)
{
	bPreviewValid = false;

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld.Get());
	if (Actor == nullptr)
	{
		return;
	}

	FVector Origin; float HexSize = 0.f; float LayerHeight = 0.f;
	if (Actor->GetHexContext(Origin, HexSize, LayerHeight) == nullptr)
	{
		return;
	}

	FVector World;
	if (!ProjectToCellPlane(Ray, World))
	{
		return;
	}

	// Coordinate LOCALI della cella attiva: e' il sistema in cui la grammatica e' definita.
	const FVector Centre = URTHexLibrary::AxialToWorld(ActiveCell, Origin, HexSize, LayerHeight);
	LocalEnd = FVector2D(World.X - Centre.X, World.Y - Centre.Y);

	// LA REGOLA NON E' QUI: lo snap e la validazione vivono nel runtime, dove esistono i test.
	if (URTGeometryGrammarLibrary::SnapToGrammar(LocalStart, LocalEnd, HexSize, Preview))
	{
		Preview.Layer = ActiveCell.Layer; // il layer e' contesto d'editor, non geometria
		Preview.WallType = Properties->WallType;
		bPreviewValid = true;

		Properties->SnappedAxis = Preview.Axis;
		Properties->SnappedOffset = Preview.Offset;
		Properties->SnappedFrom = Preview.AlongStart;
		Properties->SnappedTo = Preview.AlongEnd;
	}

	Properties->Cell = ActiveCell;
}

FInputRayHit URTHexGeometryTool::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld.Get());
	if (Actor == nullptr)
	{
		return FInputRayHit();
	}

	FRTCellId Cell; FVector Centre;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld.Get(), Actor, PressPos, Cell, Centre))
	{
		return FInputRayHit();
	}

	return FInputRayHit(0.0f); // il gesto puo' cominciare
}

void URTHexGeometryTool::OnClickPress(const FInputDeviceRay& PressPos)
{
	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld.Get());
	if (Actor == nullptr)
	{
		return;
	}

	FVector Centre;
	if (!RTHexEditor::ResolveClickedCell(TargetWorld.Get(), Actor, PressPos, ActiveCell, Centre))
	{
		return;
	}

	FVector Origin; float HexSize = 0.f; float LayerHeight = 0.f;
	if (Actor->GetHexContext(Origin, HexSize, LayerHeight) == nullptr)
	{
		return;
	}

	FVector World;
	if (!ProjectToCellPlane(PressPos, World))
	{
		return;
	}

	const FVector CellCentre = URTHexLibrary::AxialToWorld(ActiveCell, Origin, HexSize, LayerHeight);
	LocalStart = FVector2D(World.X - CellCentre.X, World.Y - CellCentre.Y);
	LocalEnd = LocalStart;
	bDragging = true;
	bPreviewValid = false;
}

void URTHexGeometryTool::OnClickDrag(const FInputDeviceRay& DragPos)
{
	if (bDragging)
	{
		UpdatePreview(DragPos); // il ghost si aggiorna MENTRE si traccia: si vede prima di rilasciare
	}
}

void URTHexGeometryTool::OnClickRelease(const FInputDeviceRay& ReleasePos)
{
	if (!bDragging)
	{
		return;
	}
	bDragging = false;

	UpdatePreview(ReleasePos);
	if (!bPreviewValid)
	{
		return; // il ghost era invalido: non si committa un segmento fuori grammatica
	}

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld.Get());
	if (Actor == nullptr)
	{
		return;
	}

	URTHexMapAsset* Map = Actor->MapAsset;
	FVector Origin; float HexSize = 0.f; float LayerHeight = 0.f;
	if (Map == nullptr || Actor->GetHexContext(Origin, HexSize, LayerHeight) == nullptr)
	{
		return;
	}

	// UNA GESTURE = UNA TRANSAZIONE: un solo `Ctrl+Z` annulla il segmento, non meta'. La transazione
	// racchiude la cottura intera, che e' l'unica cosa che tocca l'asset.
	const FScopedTransaction Transaction(LOCTEXT("BakeGeometry", "Disegna geometria"));
	Map->Modify();

	// LA COTTURA NON E' QUI: e' quella di #621, chiamata. Idempotente per `D-131`, quindi ridisegnare sopra
	// non accumula e non cancella cio' che l'autore ha dipinto a mano.
	Properties->LastBakedCovers = URTGeometryBakeLibrary::BakeCell(Map, ActiveCell, { Preview }, HexSize);

	Actor->RebuildInstances();
}

void URTHexGeometryTool::OnTerminateDragSequence()
{
	// Gesto interrotto (ESC, focus perso): nessun commit e nessun residuo.
	bDragging = false;
	bPreviewValid = false;
}

void URTHexGeometryTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bDragging || RenderAPI == nullptr)
	{
		return;
	}

	ARTHexMapActor* Actor = RTHexEditor::FindTargetMapActor(TargetWorld.Get());
	if (Actor == nullptr)
	{
		return;
	}

	FVector Origin; float HexSize = 0.f; float LayerHeight = 0.f;
	if (Actor->GetHexContext(Origin, HexSize, LayerHeight) == nullptr)
	{
		return;
	}

	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (PDI == nullptr)
	{
		return;
	}

	const FVector Centre = URTHexLibrary::AxialToWorld(ActiveCell, Origin, HexSize, LayerHeight);
	const double Z = Centre.Z + 2.0; // appena sopra il piano, per non essere mangiato dal terreno

	// IL GHOST. Verde = questo gesto produrra' un segmento legale; rosso = non lo produrra'. La differenza si
	// vede PRIMA del rilascio, che e' l'unica cosa che rende la grammatica una regola invece di una sorpresa.
	if (bPreviewValid)
	{
		const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Preview, HexSize);
		if (Line.Points.Num() >= 2)
		{
			const FVector A(Centre.X + Line.Points[0].X, Centre.Y + Line.Points[0].Y, Z);
			const FVector B(Centre.X + Line.Points[1].X, Centre.Y + Line.Points[1].Y, Z);

			// Il muro pieno piu' spesso del muretto: la differenza di tipo si legge senza aprire il pannello.
			const float Thickness = (Preview.WallType == ERTHexCoverType::High) ? 6.0f : 3.0f;
			PDI->DrawLine(A, B, FColor::Green, SDPG_Foreground, Thickness);
		}
	}
	else
	{
		// Il gesto grezzo, in rosso: si vede DOVE si sta tracciando anche quando non produce nulla di legale.
		const FVector A(Centre.X + LocalStart.X, Centre.Y + LocalStart.Y, Z);
		const FVector B(Centre.X + LocalEnd.X, Centre.Y + LocalEnd.Y, Z);
		PDI->DrawLine(A, B, FColor::Red, SDPG_Foreground, 2.0f);
	}
}

#undef LOCTEXT_NAMESPACE
