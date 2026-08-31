#include "Tools/RTHexGeometryTool.h"
#include "RTHexEditorClick.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "PrimitiveDrawingUtils.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTGeometryBake.h"
#include "RTHexAnchorReadout.h"
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

namespace
{
	/**
	 * Il gesto, tagliato in un segmento per ogni cella che attraversa.
	 *
	 * ⚠️ E' una funzione LIBERA e non un metodo, ed e' una scelta di confine: `RTHexGeometryTool.h` sta nel
	 * `writable` di un'altra track, e aggiungere una firma la' significherebbe prendersi un file per una
	 * riga. Tutto cio' che serve arriva come parametro.
	 */
	/**
	 * L'AGGANCIO: due punti di un gesto diventano due anchor della palette, e il runtime dice se la coppia
	 * si esprime — `#1895`.
	 *
	 * 🔴 **QUESTO SOSTITUISCE `SnapToGrammar` NEL GESTO, ed e' un cambio di comportamento dichiarato.**
	 * `SnapToGrammar` e' deliberatamente tollerante: enumera le coppie di punti notevoli e *«tiene l'asse che
	 * sbaglia meno»*, quindi su una delle ventiquattro coppie che nessun asse porta **non fallisce** —
	 * produce un muro legale e DIVERSO da quello chiesto, e chi disegna non ha modo di saperlo.
	 * `RefactorTactics.Anchor.SnapNeverInventsTheInexpressible` lo misura.
	 *
	 * Qui si aggancia ciascun estremo al **suo** anchor piu' vicino e si chiede al runtime se quella coppia
	 * si dice. Cosi' il gesto e' PREVEDIBILE — l'autore sa a cosa si e' attaccato, perche' il ghost glielo
	 * scrive — e quando la risposta e' no, e' no con la ragione, invece di un muro inventato.
	 *
	 * ⚠️ **Nessuna regola qui dentro.** Quale anchor: `NearestAnchor`. Se la coppia si dice: `ExplainPair`.
	 * Quale segmento: `SegmentBetweenAnchors`. Tre chiamate al runtime, dove stanno i test; questa funzione
	 * le mette in fila e basta.
	 */
	struct FGestureSnap
	{
		FRTAnchorRef From;
		FRTAnchorRef To;
		ERTAnchorPairRefusal Refusal = ERTAnchorPairRefusal::SameAnchor;
		FRTGeometrySegment Segment;
		bool bValid = false;
	};

	FGestureSnap SnapGestureToAnchors(const FRTCellId& Cell, const FVector2D& LocalStart,
		const FVector2D& LocalEnd, float HexSize)
	{
		FGestureSnap Out;
		Out.From = URTGeometryGrammarLibrary::NearestAnchor(Cell, LocalStart, HexSize);
		Out.To = URTGeometryGrammarLibrary::NearestAnchor(Cell, LocalEnd, HexSize);
		Out.Refusal = URTGeometryGrammarLibrary::ExplainPair(Out.From, Out.To, HexSize);

		if (Out.Refusal == ERTAnchorPairRefusal::None)
		{
			Out.bValid = URTGeometryGrammarLibrary::SegmentBetweenAnchors(Out.From, Out.To, HexSize,
				Out.Segment);
		}
		return Out;
	}

	void GestureAcrossCells(const FRTCellId& ActiveCell, const FVector2D& LocalStart, const FVector2D& LocalEnd,
		const FVector& Origin, float HexSize, float LayerHeight,
		TArray<URTHexLibrary::FRTCellSegment>& Out)
	{
		// I due estremi sono relativi ad `ActiveCell`: si torna in mondo una volta sola, qui, e da li' in
		// poi ogni cella riceve le proprie coordinate locali dal taglio.
		const FVector Centre = URTHexLibrary::AxialToWorld(ActiveCell, Origin, HexSize, LayerHeight);
		const FVector2D WorldStart(Centre.X + LocalStart.X, Centre.Y + LocalStart.Y);
		const FVector2D WorldEnd(Centre.X + LocalEnd.X, Centre.Y + LocalEnd.Y);

		// La soglia scarta le celle sfiorate a un angolo. Un decimo del raggio sta sotto la distanza fra
		// due punti notevoli, quindi non puo' buttare via una porzione che avrebbe prodotto un muro vero.
		URTHexLibrary::SplitSegmentAcrossCells(WorldStart, WorldEnd, Origin, HexSize, ActiveCell.Layer,
			HexSize * 0.1f, Out);
	}
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

	// LA REGOLA NON E' QUI: aggancio, esprimibilita' e segmento vivono nel runtime, dove esistono i test.
	const FGestureSnap Snap = SnapGestureToAnchors(ActiveCell, LocalStart, LocalEnd, HexSize);

	// ⚠️ **Si azzera SEMPRE per primo.** Prima questa riga non c'era e `bPreviewValid` restava a `true`
	// dal fotogramma precedente: trascinando da una posa valida a una che non lo e', il ghost continuava
	// a mostrarsi verde. Un difetto che si vede solo in movimento, cioe' proprio durante il gesto.
	bPreviewValid = false;

	// Il gesto appena premuto non e' un rifiuto: e' l'assenza della domanda, e ha una frase sua.
	const RTHexAnchor::FReadout Readout = Snap.From == Snap.To
		? RTHexAnchor::DescribePending(Snap.From)
		: RTHexAnchor::Describe(Snap.From, Snap.To, Snap.Refusal);

	if (Snap.bValid)
	{
		Preview = Snap.Segment;
		Preview.Layer = ActiveCell.Layer; // il layer e' contesto d'editor, non geometria
		Preview.WallType = Properties->WallType;
		bPreviewValid = true;

		Properties->SnappedAxis = Preview.Axis;
		Properties->SnappedOffset = Preview.Offset;
		Properties->SnappedFrom = Preview.AlongStart;
		Properties->SnappedTo = Preview.AlongEnd;
	}

	AnchorFrom = Snap.From;
	AnchorTo = Snap.To;
	Properties->SnappedAnchors = Readout.Anchor;
	Properties->Refusal = Readout.Reason;
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
	//
	// ➕ **E il gesto attraversa PIU' CELLE.** Prima si cuoceva solo `ActiveCell`, quella della pressione:
	// un muro tracciato lungo tre esagoni ne riempiva uno e si fermava — *«non si estende oltre il primo
	// esagono»*. La grammatica e' definita per cella, quindi il gesto va tagliato una volta per ciascuna e
	// agganciato ai punti notevoli di quella: `SplitSegmentAcrossCells` fa il taglio, il resto e' il ciclo.
	TArray<URTHexLibrary::FRTCellSegment> Pieces;
	GestureAcrossCells(ActiveCell, LocalStart, LocalEnd, Origin, HexSize, LayerHeight, Pieces);

	int32 Baked = 0;
	for (const URTHexLibrary::FRTCellSegment& Piece : Pieces)
	{
		// 🔴 **La stessa via del ghost, e non e' un dettaglio.** Se qui restasse `SnapToGrammar` mentre il
		// ghost aggancia agli anchor, il rilascio produrrebbe un muro che il ghost aveva appena dichiarato
		// impossibile — il difetto peggiore dei due, perche' l'autore ha visto il rosso e ottiene comunque
		// una linea. Cio' che si vede e cio' che si commette devono uscire dalla stessa funzione.
		const FGestureSnap PieceSnap =
			SnapGestureToAnchors(Piece.Cell, Piece.LocalStart, Piece.LocalEnd, HexSize);
		if (!PieceSnap.bValid)
		{
			continue; // in questa cella il gesto non produce niente di legale: le altre non ne soffrono
		}
		FRTGeometrySegment Segment = PieceSnap.Segment;
		Segment.Layer = Piece.Cell.Layer;
		Segment.WallType = Properties->WallType;
		Baked += URTGeometryBakeLibrary::AddSegmentsToCell(Map, Piece.Cell, { Segment }, HexSize);
	}
	Properties->LastBakedCovers = Baked;

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

	const double Z = URTHexLibrary::AxialToWorld(ActiveCell, Origin, HexSize, LayerHeight).Z + 2.0;

	// L'OVERLAY DEI TREDICI ANCHOR — `#1895`, parte 1 e 2 dello scope.
	//
	// I punti su cui il gesto puo' agganciarsi, con quello AGGANCIATO piu' grande: e' cio' che rende
	// l'aggancio dichiarato invece che indovinato dalla posizione del ghost.
	//
	// ⚠️ **Si vedono durante il GESTO, non in hover libero**, ed e' una restrizione dichiarata: mostrarli al
	// solo passaggio del mouse chiederebbe un `IHoverBehavior` su un `UClickDragTool`, cioe' riscrivere il
	// gesto di `#712` che questa issue dichiara di non toccare. Durante il gesto la cella attiva e' quella
	// sotto il cursore, che e' il momento in cui i punti servono davvero.
	//
	// ⛔ La palette non si ricalcola qui: `AnchorsOfCell` e `AnchorLocal` sono di `#1893`. Un secondo elenco
	// di tredici punti sarebbe la terza tassonomia che i Non-goals vietano — sono i confini di settore di
	// `FRTOccupancyMask` piu' il centro, non un altro alfabeto.
	{
		TArray<FRTAnchorRef> Anchors;
		URTGeometryGrammarLibrary::AnchorsOfCell(ActiveCell, Anchors);

		const FVector CellCentre = URTHexLibrary::AxialToWorld(ActiveCell, Origin, HexSize, LayerHeight);

		for (const FRTAnchorRef& Ref : Anchors)
		{
			const FVector2D Local = URTGeometryGrammarLibrary::AnchorLocal(Ref, HexSize);
			const FVector At(CellCentre.X + Local.X, CellCentre.Y + Local.Y, Z);

			// I tre generi si distinguono, che e' il criterio: il centro, i sei vertici, i sei punti medi.
			const bool bAnchored = (Ref == AnchorFrom) || (bDragging && Ref == AnchorTo);
			FLinearColor Tint = FLinearColor(0.55f, 0.55f, 0.62f);
			switch (Ref.Kind)
			{
			case ERTAnchorKind::Center:  Tint = FLinearColor(0.95f, 0.85f, 0.35f); break;
			case ERTAnchorKind::Vertex:  Tint = FLinearColor(0.45f, 0.75f, 0.95f); break;
			case ERTAnchorKind::EdgeMid: Tint = FLinearColor(0.65f, 0.65f, 0.70f); break;
			default: break;
			}

			// L'agganciato e' piu' grande e piu' acceso: la differenza deve leggersi senza confrontare due
			// punti fra loro, perche' l'occhio guarda il ghost e non la palette.
			PDI->DrawPoint(At, bAnchored ? Tint * 1.6f : Tint,
				bAnchored ? 14.0f : 6.0f, SDPG_Foreground);
		}
	}


	// ➕ **IL GHOST SEGUE IL GESTO SU TUTTE LE CELLE**, come la cottura. Prima disegnava solo `ActiveCell`,
	// quindi trascinando oltre il primo esagono l'anteprima si fermava mentre il gesto continuava — e da
	// quando la cottura attraversa le celle, un ghost fermo alla prima direbbe il falso su cio' che sta
	// per succedere. Lo stesso taglio, chiamato dalle stesse righe: e' l'unico modo perche' non divergano.
	TArray<URTHexLibrary::FRTCellSegment> Pieces;
	GestureAcrossCells(ActiveCell, LocalStart, LocalEnd, Origin, HexSize, LayerHeight, Pieces);

	// IL GHOST MOSTRA CIO' CHE VERRA' COTTO, NON IL SEGMENTO — e ora su OGNI cella attraversata.
	//
	// 🔴 Prima mostrava il segmento quantizzato in verde ogni volta che la grammatica lo accettava, e `U22`
	// ha misurato che quella promessa era falsa: il risultato della cottura non e' un segmento, e' un
	// insieme di **coperture sui bordi** (`EdgesTouchedBy`).
	//
	// La regola, per ciascuna cella che il gesto attraversa:
	//
	// ```text
	// bordi chiusi          verdi e spessi     e' il risultato: quelle coperture verranno create
	// segmento sui bordi    argento e sottile  e' una guida: mostra che lo snap sta scattando
	// nessun bordo chiuso   verde e spesso     E' LUI il risultato: diventera' un muro interno
	// gesto fuori grammatica  rosso            non verra' creato niente
	// ```
	//
	// ⚠️ Il ciclo e' lo STESSO di `OnClickRelease`, con la stessa chiamata a `GestureAcrossCells` e allo
	// snap. Non e' ripetizione da togliere: e' l'unica forma in cui anteprima e risultato non possono
	// divergere, che e' il difetto che questa seduta ha inseguito sei volte.
	if (bPreviewValid)
	{
		const float Thickness = (Properties->WallType == ERTHexCoverType::High) ? 6.0f : 3.0f;

		for (const URTHexLibrary::FRTCellSegment& Piece : Pieces)
		{
			FRTGeometrySegment Segment;
			if (!URTGeometryGrammarLibrary::SnapToGrammar(Piece.LocalStart, Piece.LocalEnd, HexSize, Segment))
			{
				continue; // in questa cella non produce niente: le altre si disegnano lo stesso
			}
			Segment.WallType = Properties->WallType;

			const FVector CellCentre = URTHexLibrary::AxialToWorld(Piece.Cell, Origin, HexSize, LayerHeight);
			const FVector Base(CellCentre.X, CellCentre.Y, Z);

			TArray<ERTHexDirection> Edges;
			URTGeometryBakeLibrary::EdgesTouchedBy(Segment, HexSize, Edges);

			const FRTOccupancyPolyline Line = URTGeometryGrammarLibrary::ToPolyline(Segment, HexSize);
			if (Line.Points.Num() >= 2)
			{
				const FVector A(Base.X + Line.Points[0].X, Base.Y + Line.Points[0].Y, Z);
				const FVector B(Base.X + Line.Points[1].X, Base.Y + Line.Points[1].Y, Z);

				const bool bIsInterior = Edges.Num() == 0;
				PDI->DrawLine(A, B, bIsInterior ? FColor::Green : FColor::Silver, SDPG_Foreground,
					bIsInterior ? Thickness : 1.0f);
			}

			if (Edges.Num() == 0)
			{
				continue;
			}

			// I sei vertici da `HexCorners`: la stessa funzione che disegna il contorno della cella e che
			// costruisce il prisma, quindi il ghost non puo' finire su un esagono diverso da quello vero.
			const TArray<FVector> Corners = URTHexLibrary::HexCorners(Base, HexSize);
			if (Corners.Num() != 6)
			{
				continue;
			}

			for (const ERTHexDirection Edge : Edges)
			{
				// ⚠️ `Edge` e' una direzione di VICINATO, i vertici sono numerati per ANGOLO, e le due non
				// coincidono su quattro bordi. `EdgeIndexForDirection` e' l'unico posto dove quella
				// conversione vive: la prima stesura di questo blocco faceva `static_cast<int32>(Edge)` e
				// disegnava il diagonale rispecchiato — lo stesso errore della cottura, nel verso opposto,
				// che e' esattamente cio' che succede quando una formula viene ricopiata.
				const int32 Index = URTHexLibrary::EdgeIndexForDirection(Edge);
				if (Corners.IsValidIndex(Index))
				{
					PDI->DrawLine(Corners[Index], Corners[(Index + 1) % 6], FColor::Green,
						SDPG_Foreground, Thickness);
				}
			}
		}
	}
	else
	{
		// Il gesto grezzo, in rosso: si vede DOVE si sta tracciando anche quando non produce nulla di legale.
		const FVector Centre = URTHexLibrary::AxialToWorld(ActiveCell, Origin, HexSize, LayerHeight);
		const FVector A(Centre.X + LocalStart.X, Centre.Y + LocalStart.Y, Z);
		const FVector B(Centre.X + LocalEnd.X, Centre.Y + LocalEnd.Y, Z);
		PDI->DrawLine(A, B, FColor::Red, SDPG_Foreground, 2.0f);
	}
}

#undef LOCTEXT_NAMESPACE
