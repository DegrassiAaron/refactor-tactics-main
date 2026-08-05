#include "Map/RTHexVisionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

bool URTHexVisionLibrary::HasLineOfSight(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
{
	if (!Map)
	{
		return true; // nessun dato di mappa: nessun ostacolo noto
	}
	if (From.X == To.X && From.Y == To.Y)
	{
		return true; // stessa colonna: non c'e' nulla in mezzo
	}

	// La linea e' planare e resta sul layer del TIRATORE (HexLine usa il layer di A): e' la regola di elevazione
	// del quadrato: un ostacolo blocca solo se sta al livello di chi spara.
	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(From, To);
	for (int32 I = 1; I < Line.Num() - 1; ++I) // estremi esclusi: non si coprono da soli
	{
		if (const FRTHexCellData* Data = Map->FindCell(Line[I]))
		{
			if (Data->bBlocksLineOfSight)
			{
				return false;
			}
		}
		// cella assente = buco nella mappa: non e' un muro, la vista passa
	}
	return true;
}
