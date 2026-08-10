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
#include "Turn/RTMatchSetupLibrary.h"

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
	// Contorno dall'unica definizione condivisa col runtime (+2 di Z per non finire dentro la mesh).
	const TArray<FVector> Corners = URTHexLibrary::HexCorners(Center + FVector(0, 0, 2.0), Radius);
	for (int32 I = 0; I < Corners.Num(); ++I)
	{
		PDI->DrawLine(Corners[I], Corners[(I + 1) % Corners.Num()], Color, SDPG_Foreground, 2.0f);
	}
}
FColor SurfaceColor(ERTHexSurface Surface)
{
	// Delega al runtime: una sola tavolozza per il marker dell'editor e per l'overlay in partita, altrimenti la
	// stessa cella cambierebbe colore fra i due contesti.
	return URTHexLibrary::SurfaceColor(Surface);
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
		// Due marcatori DISTINTI, come in partita (`ARTHexMapActor::DrawCellOverlay`): sono due regole diverse
		// — dove non si passa e dove non si vede — e una cella puo' avere l'una, l'altra o entrambe. Stessi
		// raggi del runtime, cosi' chi dipinge vede la mappa come la vedra' giocando.
		if (Cell.bBlocksLineOfSight)
		{
			DrawHexMarker(PDI, Center, HexSize * 0.64f, URTHexLibrary::SightBlockerColor()); // giallo: non si vede attraverso
		}
		if (Cell.bBlocksMovement)
		{
			DrawHexMarker(PDI, Center, HexSize * 0.45f, URTHexLibrary::BlockedCellColor()); // rosso: non ci si passa
		}
	}

	// Celle di PARTENZA, calcolate con la stessa funzione che allestisce la partita. Non sono un dato della
	// mappa: si spostano da sole appena una cella viene aggiunta o resa impercorribile, e senza vederle si
	// disegna una mappa senza sapere da dove partono le squadre — lo si scopre dal log a lavoro finito.
	//
	// Anello ESTERNO (1.0), piu' largo del contorno di superficie: non compete con i marcatori di regola, che
	// stanno tutti dentro.
	const int32 NumPerTeam = 2; // la v0.1 e' 2v2; il resto della scala e' un problema del formato, non dell'overlay
	const TArray<FRTCellId> Starts = URTMatchSetupLibrary::PickStartCells(Map, NumPerTeam, ActiveLayer);
	for (int32 I = 0; I < Starts.Num(); ++I)
	{
		if (bActiveOnly && Starts[I].Layer != ActiveLayer) { continue; }
		const FVector Center = URTHexLibrary::AxialToWorld(Starts[I], Origin, HexSize, LayerH);
		const FColor Color = (I < NumPerTeam)
			? URTHexLibrary::SpawnTeam0Color()
			: URTHexLibrary::SpawnTeam1Color();
		DrawHexMarker(PDI, Center, HexSize * 1.0f, Color);
	}
}
} // namespace RTHexEditor
