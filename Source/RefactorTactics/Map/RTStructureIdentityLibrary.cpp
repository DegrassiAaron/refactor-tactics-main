#include "Map/RTStructureIdentityLibrary.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"

TArray<FRTStructureEdgeRef> URTStructureIdentityLibrary::FindDoorEdges(const URTHexMapAsset* Map,
	FName StableId)
{
	TArray<FRTStructureEdgeRef> Found;
	if (!Map || StableId.IsNone())
	{
		return Found;
	}

	// L'ordine e' quello di `Cells`, che `SortCells` tiene canonico (Layer, X, Y): due risoluzioni dello
	// stesso nome danno la stessa sequenza, che e' cio' su cui #833 poggia l'ordine di applicazione.
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		for (const FRTHexDoor& Door : Cell.Doors)
		{
			if (Door.StableId == StableId)
			{
				Found.Emplace(Cell.Id, Door.Edge);
			}
		}
	}
	return Found;
}

TArray<FRTStructureArcRef> URTStructureIdentityLibrary::FindArcs(const URTHexMapAsset* Map, FName StableId)
{
	TArray<FRTStructureArcRef> Found;
	if (!Map || StableId.IsNone())
	{
		return Found;
	}

	// L'ordine e' quello di `Transitions`, che e' dato d'asset e non un contenitore associativo: due
	// risoluzioni dello stesso nome danno la stessa sequenza.
	for (const FRTHexEdge& Arc : Map->Transitions)
	{
		if (Arc.StableId == StableId)
		{
			Found.Emplace(Arc.From, Arc.To);
		}
	}
	return Found;
}

TArray<FString> URTStructureIdentityLibrary::ValidateReferences(const URTHexMapAsset* Map,
	const TArray<FName>& References)
{
	TArray<FString> Errors;
	for (const FName& Reference : References)
	{
		// Il nome vuoto non e' «nessun bersaglio»: e' un campo lasciato indietro. Risolverlo in silenzio
		// e' esattamente il «comportamento implicito» che il DoD dell'epic vieta.
		if (Reference.IsNone())
		{
			Errors.Add(TEXT("Error: riferimento di struttura vuoto"));
			continue;
		}

		// Un riferimento risolve se il nome trova una porta OPPURE un arco: chi cita `D1` non deve sapere
		// di quale dei due domini sia — e la validazione dell'asset ha gia' escluso che sia entrambi.
		if (FindDoorEdges(Map, Reference).Num() == 0 && FindArcs(Map, Reference).Num() == 0)
		{
			Errors.Add(FString::Printf(
				TEXT("Error: riferimento a una struttura inesistente '%s'"), *Reference.ToString()));
		}
	}
	return Errors;
}
