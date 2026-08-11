#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

class UWorld;
class ARTHexMapActor;
class FPrimitiveDrawInterface;
struct FInputDeviceRay;
enum class ERTHexSurface : uint8;

/** Helper condivisi tra i tool click dell'Editor Mode hex (SelectTool, PaintTool, ...). */
namespace RTHexEditor
{
	/** ARTHexMapActor bersaglio: selezionato nel Level Editor; altrimenti l'unico presente; altrimenti nullptr (ambiguo). */
	ARTHexMapActor* FindTargetMapActor(UWorld* World);

	/** Risolve la cella cliccata sul layer attivo dell'actor (raycast ISM del target, fallback piano del layer).
	 *  Ritorna false se Actor è nullptr. OutCenter = centro-mondo della cella (per il marker). */
	bool ResolveClickedCell(UWorld* World, ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
		FRTCellId& OutCell, FVector& OutCenter);

	/**
	 * Scrive il layer attivo sull'actor e ne ricostruisce la vista. Transazionale (annullabile) e idempotente:
	 * se il valore non cambia non apre nessuna transazione e ritorna false.
	 *
	 * Esiste perche' i tool possono cambiare il piano dal proprio pannello: assegnare `ActiveLayer` da codice
	 * non passa da `PostEditChangeProperty`, quindi senza la ricostruzione esplicita la vista resterebbe ferma
	 * sul piano precedente.
	 */
	bool SetActiveLayer(ARTHexMapActor* Actor, int32 NewLayer);

	/** Disegna un esagono pointy-top (marker) sul PDI. */
	void DrawHexMarker(FPrimitiveDrawInterface* PDI, const FVector& Center, float Radius, const FColor& Color,
		float Thickness = 2.0f);

	/** Colore d'overlay per una superficie cella (presentazione editor). */
	FColor SurfaceColor(ERTHexSurface Surface);

	/** Overlay debug: ogni cella dell'asset come esagono colorato per superficie; le bloccate con un esagono rosso interno. */
	void DrawSurfaceOverlay(FPrimitiveDrawInterface* PDI, const ARTHexMapActor* Actor);
}
