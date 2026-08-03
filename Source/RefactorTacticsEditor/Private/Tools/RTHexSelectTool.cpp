#include "Tools/RTHexSelectTool.h"
#include "InteractiveToolManager.h"
#include "ToolContextInterfaces.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_* (come lo scaffold 5.8)
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"       // TActorIterator
#include "Editor.h"           // GEditor
#include "Selection.h"        // USelection
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h" // FRTHexCellData (readout)
#include "Map/RTHexLibrary.h"

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
	AddToolPropertySource(Properties);
}

ARTHexMapActor* URTHexSelectTool::FindTargetMapActor() const
{
	// Preferenza: un ARTHexMapActor selezionato nel Level Editor.
	if (GEditor)
	{
		if (USelection* Sel = GEditor->GetSelectedActors())
		{
			for (FSelectionIterator It(*Sel); It; ++It)
			{
				if (ARTHexMapActor* A = Cast<ARTHexMapActor>(*It))
				{
					return A;
				}
			}
		}
	}
	// Fallback: l'unico ARTHexMapActor nel mondo.
	ARTHexMapActor* Found = nullptr;
	if (TargetWorld)
	{
		for (TActorIterator<ARTHexMapActor> It(TargetWorld); It; ++It)
		{
			if (Found) { return nullptr; } // piu' di uno e nessuno selezionato: ambiguo -> nessuna azione
			Found = *It;
		}
	}
	return Found;
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

	const URTHexMapAsset* Map = Actor->MapAsset;
	const float HexSize = Map ? Map->HexSize : Actor->HexSize;
	const float LayerH = Map ? Map->LayerHeight : Actor->LayerHeight;
	const FVector Origin = Actor->GetActorLocation();
	// Fonte unica del layer attivo: l'actor (pilota anche la viz ActiveOnly). Lo rispecchiamo nel pannello.
	const int32 Layer = Actor->ActiveLayer;
	Properties->ActiveLayer = Layer;

	// Punto-mondo del click: colpo sull'ISM del TARGET se c'e', altrimenti intersezione col piano del layer attivo.
	FVector HitPoint;
	const FVector RayStart = ClickPos.WorldRay.Origin;
	const FVector RayEnd = ClickPos.WorldRay.PointAt(999999.0);
	FHitResult Result;
	const bool bHitTarget = TargetWorld
		&& TargetWorld->LineTraceSingleByObjectType(Result, RayStart, RayEnd,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects))
		&& (Result.GetActor() == Actor); // solo l'ISM del target: ignora unita'/ostacoli tra camera e mappa
	if (bHitTarget)
	{
		HitPoint = Result.ImpactPoint;
	}
	else
	{
		const double PlaneZ = Origin.Z + static_cast<double>(Layer) * static_cast<double>(LayerH);
		const FPlane LayerPlane(FVector(0, 0, PlaneZ), FVector(0, 0, 1));
		HitPoint = FMath::RayPlaneIntersection(ClickPos.WorldRay.Origin, ClickPos.WorldRay.Direction, LayerPlane);
	}

	const FRTCellId Cell = URTHexLibrary::WorldToAxial(HitPoint, Origin, HexSize, Layer);
	Properties->SelectedCell = Cell;

	// Readout dati cella (spec §3): superficie/costo/blocco se la cella esiste nell'asset.
	const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
	Properties->bSelectedCellExists = (Data != nullptr);
	if (Data)
	{
		Properties->Surface = Data->Surface;
		Properties->MoveCost = Data->MoveCost;
		Properties->bBlocksMovement = Data->bBlocksMovement;
	}

	// Centro-mondo della cella per il marker (usa la quota del layer, non quella del colpo).
	SelectedWorldCenter = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerH);
	MarkerRadius = HexSize * 0.9f;
	bHasSelection = true;

	UE_LOG(LogTemp, Log, TEXT("[HexMode] Selezione %s (esiste=%d) su layer %d."),
		*Cell.ToString(), Properties->bSelectedCellExists ? 1 : 0, Layer);
}

void URTHexSelectTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bHasSelection || !RenderAPI) { return; }
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();
	if (!PDI) { return; }

	// Esagono di selezione (6 vertici pointy-top) sul piano orizzontale, colore giallo.
	const FColor Color = FColor::Yellow;
	FVector Prev = FVector::ZeroVector;
	for (int32 I = 0; I <= 6; ++I)
	{
		const double Angle = PI / 180.0 * (60.0 * I - 30.0); // pointy-top: primo vertice a -30 gradi
		const FVector V = SelectedWorldCenter + FVector(MarkerRadius * FMath::Cos(Angle), MarkerRadius * FMath::Sin(Angle), 2.0);
		if (I > 0)
		{
			PDI->DrawLine(Prev, V, Color, SDPG_Foreground, 2.0f);
		}
		Prev = V;
	}
}

#undef LOCTEXT_NAMESPACE
