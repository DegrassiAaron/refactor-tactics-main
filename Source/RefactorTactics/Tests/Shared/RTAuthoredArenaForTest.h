#pragma once

#include "CoreMinimal.h"
#include "Map/RTHexMapAsset.h"
#include "UObject/UObjectGlobals.h"

/**
 * La mappa d'autore, in UN posto solo.
 *
 * 🔴 **Erano cinque copie letterali dello stesso path**, ciascuna col proprio `StaticLoadObject` o
 * `LoadObject` e il proprio messaggio: `RTAuthoredMapEngagementTests.cpp` (tre), `RTHexMapTests.cpp` (due).
 * Un rename dell'asset chiedeva cinque modifiche e falliva in cinque punti diversi — la stessa classe di
 * difetto che `#1548` ha appena chiuso sui simboli duplicati («otto copie di `PlayOneTurn`, tre di
 * `AddCoreAbility`»). Il sesto chiamante e' l'occasione per non farne una sesta copia.
 *
 * ⚠️ **Il path resta esposto**, non solo il caricatore: `RTHexMapTests` lo usa dentro un elenco di tre
 * asset da spazzare, dove un caricatore per uno solo non servirebbe.
 */
namespace RTAuthoredArena
{
	/** L'asset della mappa d'autore che la partita carica con `MapSource = LevelAsset`. */
	inline const TCHAR* Path()
	{
		return TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena");
	}

	/** L'asset caricato, o `nullptr`. Il messaggio d'errore lo sceglie il chiamante: e' il suo test. */
	inline URTHexMapAsset* Load()
	{
		return Cast<URTHexMapAsset>(StaticLoadObject(URTHexMapAsset::StaticClass(), nullptr, Path()));
	}
}
