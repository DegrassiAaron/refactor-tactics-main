#include "Map/RTHexLedgeLibrary.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

bool URTHexLedgeLibrary::IsEdgeOpen(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge)
{
	// FAIL-CLOSED: senza mappa autorevole non si dichiara aperto un bordo. E' la stessa disciplina di
	// `HexKnockbackDestination`, e va nella direzione sicura — un bordo creduto aperto farebbe cadere.
	if (Map == nullptr)
	{
		return false;
	}

	const FRTHexCellData* Data = Map->FindCell(Cell);
	if (Data == nullptr)
	{
		return false; // la cella di partenza non esiste: non c'e' nessun bordo di cui parlare
	}

	// Il parapetto vince per primo: e' l'unico dato AUTORATO del vocabolario, e la sua ragione d'esistere e'
	// negare un'apertura che la geometria altrimenti implica.
	if (Data->HasGuardOn(Edge))
	{
		return false;
	}

	// «Aperto» = nessun vicino planare su quel lato. `Neighbor` resta sullo stesso layer, che e' esattamente
	// la domanda: le celle di layer diversi non sono adiacenti senza un arco, e un arco non chiude un bordo.
	const FRTCellId Adjacent = URTHexLibrary::Neighbor(Cell, Edge);
	return Map->FindCell(Adjacent) == nullptr;
}

bool URTHexLedgeLibrary::FindLandingCell(const URTHexMapAsset* Map, const FRTCellId& Cell,
	FRTCellId& OutLanding)
{
	OutLanding = Cell;
	if (Map == nullptr)
	{
		return false;
	}

	// La piu' ALTA fra quelle sotto, nella stessa colonna assiale.
	//
	// 🔑 **Si ordina per `Layer`, non per quota.** `Height` e' presentazione — il suo commento lo dichiara —
	// e usarlo qui farebbe decidere il rendering su dove finisce un'unita'. Il confronto e' fra interi, e
	// due mappe che differiscono solo per `Height` producono lo stesso atterraggio.
	//
	// ⚠️ **Non e' `Layer - 1`**: la colonna puo' saltare dei piani. Si scandisce e si tiene il massimo, che
	// e' anche l'unico modo di essere indipendenti dall'ordine dell'array.
	bool bFound = false;
	int32 BestLayer = MIN_int32;

	for (const FRTHexCellData& Other : Map->Cells)
	{
		if (Other.Id.X != Cell.X || Other.Id.Y != Cell.Y)
		{
			continue; // altra colonna
		}
		if (Other.Id.Layer >= Cell.Layer)
		{
			continue; // se stessa, o sopra
		}
		if (Other.Id.Layer > BestLayer)
		{
			BestLayer = Other.Id.Layer;
			OutLanding = Other.Id;
			bFound = true;
		}
	}

	return bFound;
}
