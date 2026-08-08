#include "Map/RTHexVisionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
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
	for (int32 I = 1; I < Line.Num(); ++I)
	{
		// Copertura ALTA sul bordo ATTRAVERSATO (CP 9.2): a differenza di `bBlocksLineOfSight`, che e' una
		// proprieta' della CELLA, questa sta fra due celle — quindi conta anche il primo e l'ultimo passo,
		// che il ciclo per-cella esclude. Un muro addossato al bersaglio lo copre: non "si copre da solo",
		// e' la barriera davanti a lui.
		if (URTHexCoverLibrary::BlocksTraversal(Map, Line[I - 1], Line[I]))
		{
			return false;
		}

		if (I == Line.Num() - 1)
		{
			break; // estremi esclusi dalla regola per-cella: non ci si oscura da soli
		}
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
