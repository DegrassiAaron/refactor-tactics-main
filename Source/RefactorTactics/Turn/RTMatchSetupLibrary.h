#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTMatchSetupLibrary.generated.h"

class URTHexMapAsset;

/**
 * Funzioni PURE di allestimento della partita su mappa esagonale: scelta delle celle di partenza e
 * ricostruzione dell'occupazione dallo stato delle unita'. Nessuno stato, nessun Actor, nessun World:
 * il GameMode le chiama, i test le verificano headless.
 */
UCLASS()
class REFACTORTACTICS_API URTMatchSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Celle di partenza per un NumPerTeam vs NumPerTeam sul layer indicato: le prime NumPerTeam al team 0,
	 * le successive al team 1. Prende le celle percorribili in ordine STABILE (Layer, X, Y) e assegna i due
	 * team dalle due estremita' dell'ordine, cosi' le squadre partono lontane senza dipendere da RNG.
	 * Mappa nulla o celle percorribili insufficienti -> array vuoto (il chiamante non allestisce a meta').
	 */
	static TArray<FRTCellId> PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer);

	/**
	 * Occupazione cella -> UnitId ricostruita dallo stato delle unita': solo le vive occupano.
	 * I tre array sono paralleli; lunghezze incoerenti -> mappa vuota (input non fidato, nessun indovinare).
	 */
	static TMap<FRTCellId, int32> BuildOccupancy(const TArray<FRTCellId>& Cells,
		const TArray<int32>& UnitIds, const TArray<bool>& Alive);
};
