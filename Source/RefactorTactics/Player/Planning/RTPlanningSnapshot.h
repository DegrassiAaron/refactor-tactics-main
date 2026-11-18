#pragma once

#include "CoreMinimal.h"

class ARTUnit;
struct FRTHexSnapshot;

/**
 * **LA FOTOGRAFIA SU CUI SI PIANIFICA** (#4065, S5): lo snapshot che il controller usa per scrivere un piano e per
 * mostrarne l'anteprima. Stava nel namespace anonimo del controller; la leggono ora anche la dichiarazione del verso e lo
 * scrittore dell'anteprima, che stanno fuori.
 */
namespace RTPlanningSnapshot
{
	/**
	 * Snapshot dello stato corrente chiesto all'AUTORITA' (il TurnManager) e indice dell'unita' al suo interno.
	 * Il client non si costruisce uno stato parallelo: calcola solo la preview su quello autorevole (invariante #5).
	 * False se manca il turn manager o l'unita' non e' nello snapshot (es. non viva).
	 *
	 * @param OutUnits gli Actor **agli stessi indici** dello snapshot. ⚠️ Quegli id sono INDICI nell'array delle unita'
	 *                 vive, non `StableUnitId`: vanno ricalcolati a ogni interazione, non memorizzati.
	 */
	REFACTORTACTICS_API bool For(const UObject* WorldContext, const ARTUnit* Unit, FRTHexSnapshot& OutSnapshot,
		int32& OutUnitId, TArray<ARTUnit*>* OutUnits = nullptr);

	/**
	 * Porta l'unita' `UnitId` dello snapshot sulla cella da cui il suo Move partira' (`ARTUnit::PlannedMoveStartCell`), cella e
	 * occupazione (E38 CP 38.16, [D-628] (4)): con uno scatto pianificato il Move parte da dove lo scatto si ferma.
	 *
	 * ⛔ **Solo per chi compone il MOVIMENTO** (il waypoint, il ventaglio, il troncamento, la ricostruzione del percorso): il giudizio
	 * dello scatto stesso, la mira e il verso leggono la posizione di adesso, e restano su `For`. Per questo non sta dentro `For`.
	 */
	REFACTORTACTICS_API void StartMovementFromPlannedDash(FRTHexSnapshot& Snapshot, int32 UnitId, const ARTUnit* Unit);
}
