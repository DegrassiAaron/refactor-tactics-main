#include "Map/RTStructureIdentityLibrary.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h" // EdgeDirection: la direzione fra due celle adiacenti, in un posto solo
#include "Map/RTHexLibrary.h"      // OppositeDirection: il bordo visto dall'altro lato
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

FName URTStructureIdentityLibrary::FindDoorIdOnEdge(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To)
{
	if (Map == nullptr)
	{
		return NAME_None;
	}

	// La direzione fra due celle adiacenti la sa `URTHexCoverLibrary`, che ne e' l'unica sede: riscriverla
	// qui darebbe una seconda regola di adiacenza da tenere allineata alla prima.
	ERTHexDirection Dir = ERTHexDirection::E;
	if (!URTHexCoverLibrary::EdgeDirection(From, To, Dir))
	{
		return NAME_None; // non adiacenti, o su layer diversi: nessun bordo da guardare
	}

	// Il bordo si guarda dai DUE lati. Il dato sta su una cella sola — chi disegna ne mette una — e la
	// struttura e' fisica: la stessa disciplina di `CoverBetween`, decisa sulla issue #70.
	if (const FRTHexCellData* Here = Map->FindCell(From))
	{
		for (const FRTHexDoor& Door : Here->Doors)
		{
			if (Door.Edge == Dir && !Door.StableId.IsNone())
			{
				return Door.StableId;
			}
		}
	}
	if (const FRTHexCellData* There = Map->FindCell(To))
	{
		const ERTHexDirection Opposite = URTHexLibrary::OppositeDirection(Dir);
		for (const FRTHexDoor& Door : There->Doors)
		{
			if (Door.Edge == Opposite && !Door.StableId.IsNone())
			{
				return Door.StableId;
			}
		}
	}
	return NAME_None;
}

bool URTStructureIdentityLibrary::IsInteractionSource(const URTHexMapAsset* Map, FName StableId)
{
	if (Map == nullptr || StableId.IsNone())
	{
		return false;
	}
	for (const FRTInteractionBinding& Binding : Map->InteractionBindings)
	{
		if (Binding.SourceId == StableId)
		{
			return true;
		}
	}
	return false;
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
	const URTHexMapAsset* Map, FName SourceId, TArray<FString>* OutErrors)
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
		// `INT-5` e' APERTA: il dato PUO' rappresentare due sorgenti sullo stesso bersaglio — rifiutarlo in
		// `ValidateInteractionGraph` farebbe rispondere un validator a una decisione aperta. Ma risolverlo
		// come se niente fosse la deciderebbe **qui**, in silenzio: due sorgenti comanderebbero la stessa
		// struttura senza che nessuno abbia detto cosa succede quando agiscono entrambe. Quindi si rifiuta,
		// con reason code, ed e' la risoluzione a farlo — esattamente dove l'header di #832 lo dichiara.
		//
		// 🔴 Il rifiuto e' dell'INTERA risoluzione e non del singolo bersaglio conteso — ma **non per
		// atomicita'**, che [D-150] ha fatto cadere il 2026-08-16 («si applicano i bersagli applicabili e si
		// riporta l'esito degli altri»). Quella era la ragione scritta qui, ed e' sopravvissuta alla decisione
		// che la smentiva. La ragione vera: `INT-5` non ha deciso **chi comanda** un bersaglio conteso, quindi
		// quel bersaglio non ha un comandante riconosciuto; risolvere gli altri attribuirebbe a questa sorgente
		// un'operazione parziale che nessuno le ha assegnato. E' una lacuna di AUTORITA', non di transazione.
		for (const FName& TargetId : Binding.TargetIds)
		{
			for (const FRTInteractionBinding& Other : Map->InteractionBindings)
			{
				if (Other.SourceId == SourceId || Other.SourceId.IsNone())
				{
					continue;
				}
				if (Other.TargetIds.Contains(TargetId))
				{
					if (OutErrors != nullptr)
					{
						OutErrors->Add(FString::Printf(
							TEXT("Error: il bersaglio '%s' e' comandato anche da '%s' (INT-5 aperta): ")
							TEXT("risoluzione rifiutata"),
							*TargetId.ToString(), *Other.SourceId.ToString()));
					}
					return TArray<FRTStructureEdgeRef>();
				}
			}
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

		// Un binding DICHIARATO senza bersagli non e' «una sorgente che non comanda nulla»: quella e' una
		// sorgente **senza binding**, che risolve in un array vuoto e non e' un errore. Questo e' un binding
		// scritto a meta', e senza il controllo passerebbe identico a un `TargetIds` svuotato per sbaglio.
		if (Binding.TargetIds.Num() == 0 && !Binding.SourceId.IsNone())
		{
			Errors.Add(FString::Printf(
				TEXT("Error: la sorgente '%s' dichiara un binding senza bersagli"),
				*Binding.SourceId.ToString()));
		}

		// Una struttura che comanda SE STESSA supera `ValidateReferences` — il nome esiste — ed e' un anello:
		// il runtime percorrerebbe il comando su una porta che sta gia' cambiando stato. Si rifiuta qui,
		// dove costa un confronto, invece di scoprirlo quando qualcuno lo scrive in una mappa.
		if (!Binding.SourceId.IsNone() && Binding.TargetIds.Contains(Binding.SourceId))
		{
			Errors.Add(FString::Printf(
				TEXT("Error: la sorgente '%s' comanda se stessa"), *Binding.SourceId.ToString()));
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

		// 🔴 **Il bersaglio che risolve un ARCO e non una porta e' il «binding cross-layer» dello Scope**, ed e'
		// l'unica forma di quel difetto che questo repository puo' MISURARE: gli archi sono la sola struttura
		// che collega layer diversi — `spec-mappa-multilivello.md` dice che celle su layer diversi non sono
		// adiacenti «mai, nemmeno se si trovano una sopra l'altra», e si collegano solo con transizioni
		// esplicite. Una PORTA cross-layer non e' rappresentabile: `FindDoorEdges` la costruisce da
		// `(cella, direzione)`, e il vicino di un bordo sta per costruzione sullo stesso layer.
		//
		// Senza questa regola il difetto e' SILENZIOSO, ed e' peggio di un bersaglio fantasma: `ValidateReferences`
		// accetta il nome — sa che un riferimento risolve «una porta OPPURE un arco» (#832) — mentre
		// `ResolveInteractionTargets` passa solo da `FindDoorEdges` e restituisce zero bersagli. L'asset e'
		// valido, l'`Interact` e' legale, e non succede niente: nessuno dei due lati ha modo di accorgersene.
		//
		// ⚠️ Si rifiuta il bersaglio, NON si estende la risoluzione agli archi. Un arco ha uno stato proprio
		// (`ERTHexArcState`, solo `Active` e' percorribile) e un ingresso di mutazione proprio, quindi
		// «comandare un arco» e' una semantica che nessuna decisione ha preso: deciderla qui la darebbe per
		// implementazione, che e' lo stesso errore che `INT-5` evita poco sopra.
		for (const FName& TargetId : Binding.TargetIds)
		{
			if (TargetId.IsNone())
			{
				continue; // gia' segnalato da `ValidateReferences`: un difetto, un errore
			}
			if (FindDoorEdges(Map, TargetId).Num() == 0 && FindArcs(Map, TargetId).Num() > 0)
			{
				Errors.Add(FString::Printf(
					TEXT("Error: il bersaglio '%s' e' un arco (struttura cross-layer) e la risoluzione ")
					TEXT("comanda solo porte: binding non applicabile"), *TargetId.ToString()));
			}
		}
	}
	return Errors;
}
