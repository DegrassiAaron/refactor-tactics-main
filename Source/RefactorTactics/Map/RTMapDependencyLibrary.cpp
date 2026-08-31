#include "Map/RTMapDependencyLibrary.h"

#include "Map/RTHexMapAsset.h"
#include "Map/RTStructureIdentityLibrary.h"

FRTMapDependencySet URTMapDependencyLibrary::CollectDependents(const URTHexMapAsset* Map,
	const FRTMapElementHandle& Handle)
{
	FRTMapDependencySet Set;

	if (Map == nullptr || Handle.Kind == ERTMapElementKind::None)
	{
		return Set;
	}

	if (Handle.Kind == ERTMapElementKind::Cell)
	{
		// I muri interni sono uno dei tre array che vivono FUORI dalla cella: `Covers` e `Doors` stanno
		// dentro `FRTHexCellData` e se ne vanno con lei, senza che nessuno debba raccoglierli.
		for (int32 Index = 0; Index < Map->InteriorWalls.Num(); ++Index)
		{
			if (Map->InteriorWalls[Index].Cell == Handle.Cell)
			{
				Set.InteriorWallIndices.Add(Index);
			}
		}

		// Una transizione con un estremo su una cella che non esiste piu' e' l'errore «verso cella
		// inesistente» di `ValidateMap`. Entrambi i versi: `FRTHexEdge` e' direzionale, e la cella puo'
		// essere citata come sorgente o come bersaglio.
		for (int32 Index = 0; Index < Map->Transitions.Num(); ++Index)
		{
			const FRTHexEdge& Edge = Map->Transitions[Index];
			if (Edge.From == Handle.Cell || Edge.To == Handle.Cell)
			{
				Set.TransitionIndices.Add(Index);
			}
		}

		// I NOMI che se ne vanno con la cella. Un binding che li cita come sorgente diventerebbe
		// «riferimento a una struttura inesistente» per `ValidateReferences`.
		//
		// ⚠️ **Portare un bordo di quella struttura non significa esserne l'unica sede.** Un portone e' un
		// GRUPPO di bordi che condividono il nome (CP 23.3): cancellare una delle sue celle ne toglie
		// meta', e il nome continua a risolvere. Rimuovere il binding qui sarebbe una correzione
		// silenziosa di uno stato ancora valido — e per giunta con perdita di dato.
		TSet<FName> DyingNames;
		if (const FRTHexCellData* Cell = Map->FindCell(Handle.Cell))
		{
			for (const FRTHexDoor& Door : Cell->Doors)
			{
				if (Door.StableId.IsNone() || DyingNames.Contains(Door.StableId))
				{
					continue;
				}

				bool bSurvivesElsewhere = false;
				for (const FRTStructureEdgeRef& Ref :
					URTStructureIdentityLibrary::FindDoorEdges(Map, Door.StableId))
				{
					if (!(Ref.Cell == Handle.Cell))
					{
						bSurvivesElsewhere = true;
						break;
					}
				}

				if (!bSurvivesElsewhere)
				{
					DyingNames.Add(Door.StableId);
				}
			}
		}

		Set.OrphanedStructureNames = DyingNames.Array();
		Set.OrphanedStructureNames.Sort(FNameLexicalLess());

		// 🔴 **Un binding muore se perde la SORGENTE oppure l'ULTIMO bersaglio** — la regola che il contratto
		// di `FRTMapDependencySet` dichiarava e che questo ciclo non applicava: guardava la sola `SourceId`,
		// quindi un bersaglio morto restava citato e `ValidateReferences` lo segnalava. Trovato da una code
		// review, non da un test.
		//
		// ⚠️ Chi perde SOLO ALCUNI bersagli non entra qui: sopravvive, e l'applicatore gli toglie i nomi
		// morti usando `OrphanedStructureNames`. Metterlo fra gli indici lo cancellerebbe intero — perdita
		// di dato silenziosa, che e' il difetto opposto e peggiore.
		for (int32 Index = 0; Index < Map->InteractionBindings.Num(); ++Index)
		{
			const FRTInteractionBinding& Binding = Map->InteractionBindings[Index];

			if (DyingNames.Contains(Binding.SourceId))
			{
				Set.InteractionBindingIndices.Add(Index);
				continue;
			}

			const bool bEveryTargetDies = Binding.TargetIds.Num() > 0
				&& !Binding.TargetIds.ContainsByPredicate(
					[&DyingNames](const FName& Target) { return !DyingNames.Contains(Target); });

			if (bEveryTargetDies)
			{
				Set.InteractionBindingIndices.Add(Index);
			}
		}
	}

	return Set;
}
