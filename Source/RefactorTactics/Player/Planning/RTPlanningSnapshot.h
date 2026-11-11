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
}
