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

	// LA COTTURA NON E' QUI: e' quella di #621, chiamata.
	//
	// 🔴 Questa riga chiamava `BakeCell`, e il commento che le stava sopra diceva *«idempotente per D-131,
	// quindi ridisegnare sopra non accumula e non cancella cio' che l'autore ha dipinto a mano»*. Vero
	// sulle coperture a MANO, e falso sulle generate: `BakeCell` le rimuove tutte prima di riscrivere,
	// perche' ha un contratto di rebake — «questi segmenti sono lo stato completo della cella». Il tool ne
	// possiede uno solo, quindi ogni tratto cancellava il precedente. Trovato in `U22` disegnando due muri
	// che condividono un vertice: il secondo faceva sparire il primo.
	//
	// La via additiva non cambia `BakeCell`, che per il suo chiamante e' giusta: aggiunge un ingresso per
	// chi vede un gesto per volta.
	Properties->LastBakedCovers =
		URTGeometryBakeLibrary::AddSegmentsToCell(Map, ActiveCell, { Preview }, HexSize);

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

	// IL GHOST MOSTRA CIO' CHE VERRA' COTTO, NON IL SEGMENTO.
	//
	// 🔴 Prima mostrava il segmento quantizzato in verde ogni volta che la grammatica lo accettava, e
	// `U22` ha misurato che quella promessa era falsa: il risultato della cottura non e' un segmento, e'
	// un insieme di **coperture sui bordi** (`EdgesTouchedBy`). Le due cose divergono, e non di poco —
	// `Offset` e' quantizzato in dodicesimi della perpendicolare, cioe' 7,22 uu su una cella da 100, quindi
	// mezzo quanto (3,61 uu, pochi pixel in viewport) basta a portare la linea DENTRO o FUORI la cella.
	// A `Offset = 11` attraversa due lati che non hai tracciato; a `13` non ne attraversa nessuno. Misurato:
	// con uno scarto della mano del 4% del raggio si vedono i primi muri sul lato sbagliato, all'8% un
	// gesto su tre non produce niente. E il ghost, in tutti quei casi, era VERDE.
	//
	// Quindi la regola cambia di soggetto, ed e' l'unica che l'autore possa usare:
	//   verde = QUESTI bordi diventeranno coperture · rosso = non verra' creato niente.
	// Un segmento legale che non chiude nessun bordo e' rosso, perche' e' cio' che produce: niente.
	if (bPreviewValid)
	{
		TArray<ERTHexDirection> Edges;
		URTGeometryBakeLibrary::EdgesTouchedBy(Preview, HexSize, Edges);

		// Il segmento resta, ma SOTTILE e in secondo piano: serve ancora a far vedere che lo snap sta
		// scattando sulle direttrici — che e' l'altra meta' di cio' che l'autore deve leggere — senza piu'
		// spacciarsi per il risultato.
		const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Preview, HexSize);
		if (Line.Points.Num() >= 2)
		{
			const FVector A(Centre.X + Line.Points[0].X, Centre.Y + Line.Points[0].Y, Z);
			const FVector B(Centre.X + Line.Points[1].X, Centre.Y + Line.Points[1].Y, Z);
			PDI->DrawLine(A, B, Edges.Num() > 0 ? FColor::Silver : FColor::Red, SDPG_Foreground, 1.0f);
		}

		if (Edges.Num() > 0)
		{
			// I sei vertici da `HexCorners`: la stessa funzione che disegna il contorno della cella e che
			// costruisce il prisma, quindi il ghost non puo' finire su un esagono diverso da quello vero.
			const TArray<FVector> Corners = URTHexLibrary::HexCorners(FVector(Centre.X, Centre.Y, Z), HexSize);
			if (Corners.Num() == 6)
			{
				// Il muro pieno piu' spesso del muretto: la differenza di tipo si legge senza aprire il pannello.
				const float Thickness = (Preview.WallType == ERTHexCoverType::High) ? 6.0f : 3.0f;
				for (const ERTHexDirection Edge : Edges)
				{
					// ⚠️ `Edge` e' una direzione di VICINATO, i vertici sono numerati per ANGOLO, e le due
					// non coincidono su quattro bordi. `EdgeIndexForDirection` e' l'unico posto dove quella
					// conversione vive: la prima stesura di questo blocco faceva `static_cast<int32>(Edge)`
					// e disegnava il diagonale rispecchiato — lo stesso errore della cottura, nel verso
					// opposto, che e' esattamente cio' che succede quando la formula viene ricopiata.
					const int32 Index = URTHexLibrary::EdgeIndexForDirection(Edge);
					if (Corners.IsValidIndex(Index))
					{
						PDI->DrawLine(Corners[Index], Corners[(Index + 1) % 6], FColor::Green,
							SDPG_Foreground, Thickness);
					}
				}
			}
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
