#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h" // ERTHexSurface, ERTHexTransitionKind

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
	/**
	 * ⚠️ `OutClickedPoint` (opzionale) e' il punto su cui si e' cliccato, non il centro della cella: serve a
	 * `URTHexLibrary::NearestEdgeDirection` per sapere QUALE BORDO l'autore stava mirando (#1864).
	 *
	 * Quando il raycast non ha un colpo utile e si ripiega sul piano del layer, riceve il **centro cella** —
	 * il che rende il bordo scelto arbitrario ma deterministico. E' dichiarato invece che nascosto: senza un
	 * colpo, «quale lato» e' una domanda a cui la geometria non puo' rispondere.
	 */
	bool ResolveClickedCell(UWorld* World, ARTHexMapActor* Actor, const FInputDeviceRay& ClickPos,
		FRTCellId& OutCell, FVector& OutCenter, FVector* OutClickedPoint = nullptr);

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

	/**
	 * Colore di una transizione per tipo, e freccia From->To. Stanno qui e non nel tool Arch perche' ora hanno
	 * DUE consumatori: il tool, che le disegna mentre le si crea, e l'overlay, che le mostra sempre. Due
	 * definizioni dello stesso vocabolario visivo prima o poi divergono — stessa ragione per cui la tavolozza
	 * delle superfici e' una sola.
	 */
	FColor TransitionKindColor(ERTHexTransitionKind Kind);
	void DrawArrow(FPrimitiveDrawInterface* PDI, const FVector& A, const FVector& B, const FColor& Color);
}
