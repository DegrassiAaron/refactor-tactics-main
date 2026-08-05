#include "RTHexEditorClick.h"
#include "PrimitiveDrawingUtils.h" // FPrimitiveDrawInterface / SDPG_*
#include "InputState.h"            // FInputDeviceRay
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"           // TActorIterator
#include "Editor.h"                // GEditor
#include "Selection.h"             // USelection
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"

namespace RTHexEditor
{
ARTHexMapActor* FindTargetMapActor(UWorld* World)
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
	if (World)
	{
		for (TActorIterator<ARTHexMapActor> It(World); It; ++It)
		{
			if (Found) { return nullptr; } // piu' di uno e nessuno selezionato: ambiguo -> nessuna azione
			Found = *It;
		}
	}
	return Found;
}

bool ResolveClickedCell(UWorld* World, ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
	FRTCellId& OutCell, FVector& OutCenter)
{
	if (!Actor) { return false; }

	// Contesto geometrico dall'unica fonte condivisa col runtime (scala dall'asset, origine dall'actor).
	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerH = 0.f;
	Actor->GetHexContext(Origin, HexSize, LayerH);
	const int32 Layer = Actor->ActiveLayer; // in editor il piano lo decide il layer attivo, non la quota del click

	// Punto-mondo del click: colpo sull'ISM del TARGET se c'e', altrimenti intersezione col piano del layer attivo.
	FVector HitPoint;
	const FVector RayStart = ClickPos.WorldRay.Origin;
	const FVector RayEnd = ClickPos.WorldRay.PointAt(999999.0);
	FHitResult Result;
	const bool bHitTarget = World
		&& World->LineTraceSingleByObjectType(Result, RayStart, RayEnd,
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

	OutCell = URTHexLibrary::WorldToAxial(HitPoint, Origin, HexSize, Layer);
	OutCenter = URTHexLibrary::AxialToWorld(OutCell, Origin, HexSize, LayerH);
	return true;
}

void DrawHexMarker(FPrimitiveDrawInterface* PDI, const FVector& Center, float Radius, const FColor& Color)
{
	if (!PDI) { return; }
	// Esagono pointy-top (6 vertici) sul piano orizzontale.
	FVector Prev = FVector::ZeroVector;
	for (int32 I = 0; I <= 6; ++I)
	{
		const double Angle = PI / 180.0 * (60.0 * I - 30.0); // pointy-top: primo vertice a -30 gradi
		const FVector V = Center + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 2.0);
		if (I > 0)
		{
			PDI->DrawLine(Prev, V, Color, SDPG_Foreground, 2.0f);
		}
		Prev = V;
	}
}
FColor SurfaceColor(ERTHexSurface Surface)
{
	switch (Surface)
	{
	case ERTHexSurface::Water:       return FColor(60, 120, 255);
	case ERTHexSurface::Mud:         return FColor(140, 100, 60);
	case ERTHexSurface::Fire:        return FColor(255, 130, 40);
	case ERTHexSurface::Electrified: return FColor(80, 230, 230);
	case ERTHexSurface::Ice:         return FColor(160, 220, 255);
	case ERTHexSurface::Void:        return FColor(150, 40, 150);
	case ERTHexSurface::Normal:
	default:                         return FColor(160, 160, 160);
	}
}

void DrawSurfaceOverlay(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor)
{
	if (!PDI || !Actor || !Actor->MapAsset) { return; }
	const URTHexMapAsset* Map = Actor->MapAsset;
	const FVector Origin = Actor->GetActorLocation();
	const float HexSize = Map->HexSize;
	const float LayerH = Map->LayerHeight;
	// Coerente con RebuildInstances: in ActiveOnly l'overlay mostra solo il layer attivo (niente piani impilati).
	const bool bActiveOnly = (Actor->LayerView == ERTLayerViewMode::ActiveOnly);
	const int32 ActiveLayer = Actor->ActiveLayer;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		if (bActiveOnly && Cell.Id.Layer != ActiveLayer) { continue; }
		const FVector Center = URTHexLibrary::AxialToWorld(Cell.Id, Origin, HexSize, LayerH);
		DrawHexMarker(PDI, Center, HexSize * 0.85f, SurfaceColor(Cell.Surface));
		if (Cell.bBlocksMovement)
		{
			DrawHexMarker(PDI, Center, HexSize * 0.45f, FColor::Red); // esagono interno rosso = bloccata
		}
	}
}
} // namespace RTHexEditor
