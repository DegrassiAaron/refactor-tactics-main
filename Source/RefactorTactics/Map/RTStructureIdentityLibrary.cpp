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

TArray<FRTStructureEdgeRef> URTStructureIdentityLibrary::ResolveInteractionTargets(
	const URTHexMapAsset* Map, FName SourceId)
{
	TArray<FRTStructureEdgeRef> Targets;
	if (Map == nullptr || SourceId.IsNone())
	{
		return Targets;
	}

	// Scansione LINEARE dell'array, non una `TMap` costruita al volo: l'ordine dei bersagli e' quello
	// dichiarato nell'asset, e costruire un indice per poi riordinarlo introdurrebbe proprio la dipendenza
	// da un ordine di iterazione che l'invariante n. 3 vieta. Con un binding duplicato vince il PRIMO — ma
	// quel caso e' un errore d'asset (`ValidateInteractionGraph`), non una regola su cui appoggiarsi.
	for (const FRTInteractionBinding& Binding : Map->InteractionBindings)
	{
		if (Binding.SourceId != SourceId)
		{
			continue;
		}
		for (const FName& TargetId : Binding.TargetIds)
		{
			Targets.Append(FindDoorEdges(Map, TargetId));
		}
		break;
	}
	return Targets;
}

TArray<FString> URTStructureIdentityLibrary::ValidateInteractionGraph(const URTHexMapAsset* Map)
{
	TArray<FString> Errors;
	if (Map == nullptr)
	{
		return Errors;
	}

	TSet<FName> SeenSources;
	for (const FRTInteractionBinding& Binding : Map->InteractionBindings)
	{
		// Una sorgente senza nome non e' «nessuna sorgente»: e' un campo lasciato indietro, la stessa
		// distinzione che `ValidateReferences` fa sui bersagli.
		if (Binding.SourceId.IsNone())
		{
			Errors.Add(TEXT("Error: binding di interazione senza sorgente"));
		}
		// `Add` restituisce se l'elemento c'era gia': il duplicato NON «vince l'ultimo», fallisce. Senza,
		// due liste comanderebbero la stessa struttura e nessuno saprebbe quale.
		else if (SeenSources.Contains(Binding.SourceId))
		{
			Errors.Add(FString::Printf(
				TEXT("Error: binding duplicato per la sorgente '%s'"), *Binding.SourceId.ToString()));
		}
		else
		{
			SeenSources.Add(Binding.SourceId);
		}

		// La sorgente dev'essere una struttura vera quanto i bersagli, e i bersagli passano dalla regola
		// che #832 ha gia' scritto: `ValidateReferences` sa che un nome risolve una porta OPPURE un arco.
		// La sorgente vuota e' gia' stata segnalata sopra: rimandarla a `ValidateReferences` darebbe due
		// errori per un difetto solo, e un conteggio di errori che non corrisponde ai difetti e' il genere
		// di cosa su cui poi qualcuno scrive un'asserzione.
		if (!Binding.SourceId.IsNone())
		{
			Errors.Append(ValidateReferences(Map, { Binding.SourceId }));
		}
		Errors.Append(ValidateReferences(Map, Binding.TargetIds));
	}
	return Errors;
}
