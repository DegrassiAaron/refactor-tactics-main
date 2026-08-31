#include "Map/RTMapEditLibrary.h"

#include "Map/RTGeometryBake.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapDependencyLibrary.h"
#include "Map/RTStructureIdentityLibrary.h"

namespace
{
	/**
	 * Applica la morte di un insieme di NOMI ai binding di interazione (#1864).
	 *
	 * 🔴 **Una implementazione sola, e non e' pignoleria**: prima ce n'erano due che sbagliavano in direzioni
	 * opposte — la cancellazione della porta rimuoveva il binding intero anche quando il nome era **uno** dei
	 * bersagli (perdita di dato silenziosa), e la cascata della cella guardava la sola sorgente lasciando
	 * bersagli fantasma che `ValidateReferences` segnala. Trovate da una code review, non da un test.
	 *
	 * La regola e' quella che il contratto di `FRTMapDependencySet` gia' dichiarava: **si perde la sorgente,
	 * oppure l'ultimo bersaglio**. Un bersaglio su tre esce dalla lista e il binding resta.
	 */
	void RTApplyOrphanedNames(URTHexMapAsset* Map, const TArray<FName>& DeadNames)
	{
		if (Map == nullptr || DeadNames.Num() == 0)
		{
			return;
		}

		// All'indietro: rimuovere in avanti sposta gli elementi successivi sotto l'indice corrente.
		for (int32 Index = Map->InteractionBindings.Num() - 1; Index >= 0; --Index)
		{
			FRTInteractionBinding& Binding = Map->InteractionBindings[Index];

			if (DeadNames.Contains(Binding.SourceId))
			{
				Map->InteractionBindings.RemoveAt(Index);
				continue;
			}

			for (const FName& Dead : DeadNames)
			{
				Binding.TargetIds.Remove(Dead);
			}

			// Senza bersagli e' gia' un invalido dichiarato — «binding dichiarato senza bersagli» — quindi
			// tenerlo non conserva un dato, produce un errore.
			if (Binding.TargetIds.Num() == 0)
			{
				Map->InteractionBindings.RemoveAt(Index);
			}
		}
	}
}

int32 URTMapEditLibrary::ResolveInteriorWall(const URTHexMapAsset* Map, const FRTMapElementHandle& Handle)
{
	if (Map == nullptr || Handle.Kind != ERTMapElementKind::InteriorWall)
	{
		return INDEX_NONE;
	}

	// Il NOME ha la precedenza: e' l'unica identita' che sopravvive al move.
	if (!Handle.StableId.IsNone())
	{
		for (int32 Index = 0; Index < Map->InteriorWalls.Num(); ++Index)
		{
			if (Map->InteriorWalls[Index].StableId == Handle.StableId)
			{
				return Index;
			}
		}
		// Un nome che non risolve NON ricade sulla chiave: chi ha chiesto quella struttura vuole quella, e
		// restituirne un'altra perche' sta nello stesso posto sarebbe un errore silenzioso.
		return INDEX_NONE;
	}

	// Senza nome, la chiave naturale — unica per una regola che `ValidateMap` gia' applica. `operator==`
	// tratta gli estremi come coppia non ordinata, quindi lo stesso muro percorso al contrario risolve.
	for (int32 Index = 0; Index < Map->InteriorWalls.Num(); ++Index)
	{
		if (Map->InteriorWalls[Index].Cell == Handle.Cell
			&& Map->InteriorWalls[Index].Segment == Handle.Segment)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

ERTMapEditOutcome URTMapEditLibrary::MoveInteriorWall(URTHexMapAsset* Map,
	const FRTMapElementHandle& Handle, const FRTCellId& NewCell, const FRTGeometrySegment& NewSegment)
{
	const int32 Index = ResolveInteriorWall(Map, Handle);
	if (Index == INDEX_NONE)
	{
		return ERTMapEditOutcome::RefusedUnresolved;
	}

	// Un muro su una cella che non esiste e' l'orfano che `ValidateMap` segnala come prima regola.
	if (Map->FindCell(NewCell) == nullptr)
	{
		return ERTMapEditOutcome::RefusedNoSuchCell;
	}

	// La grammatica la decide `ValidateSegment`, che esiste: qui si chiama, non si riscrive.
	if (URTGeometryGrammarLibrary::ValidateSegment(NewSegment) != ERTGeometryViolation::None)
	{
		return ERTMapEditOutcome::RefusedOutOfGrammar;
	}

	// L'invariante di `InteriorWalls`: ci sta solo cio' che nessuna copertura puo' rappresentare. Un
	// segmento che chiude un bordo E' una copertura, e scriverlo qui creerebbe due verita' sullo stesso
	// muro. Si rifiuta il gesto: correggerlo in silenzio, o scriverlo e lasciar protestare il validator,
	// sono i due modi che i Non-goal vietano.
	TArray<ERTHexDirection> TouchedEdges;
	URTGeometryBakeLibrary::EdgesTouchedBy(NewSegment, Map->HexSize, TouchedEdges);
	if (TouchedEdges.Num() > 0)
	{
		return ERTMapEditOutcome::RefusedWouldCloseEdge;
	}

	// Duplicato. Il confronto usa `FRTGeometrySegment::operator==`, che tratta gli estremi come coppia NON
	// ordinata: riscriverlo campo per campo lascerebbe passare lo stesso muro percorso al contrario — il
	// difetto che una code review ha gia' trovato una volta nella cottura.
	//
	// ⚠️ Se stesso escluso: spostare un muro sulla propria posizione non e' un duplicato, e' un gesto nullo.
	for (int32 Other = 0; Other < Map->InteriorWalls.Num(); ++Other)
	{
		if (Other == Index)
		{
			continue;
		}
		if (Map->InteriorWalls[Other].Cell == NewCell
			&& Map->InteriorWalls[Other].Segment == NewSegment)
		{
			return ERTMapEditOutcome::RefusedDuplicate;
		}
	}

	// Si scrive solo dopo aver deciso: un'operazione o si applica intera o non lascia traccia.
	Map->InteriorWalls[Index].Cell = NewCell;
	Map->InteriorWalls[Index].Segment = NewSegment;

	return ERTMapEditOutcome::Applied;
}

ERTMapEditOutcome URTMapEditLibrary::DeleteElement(URTHexMapAsset* Map, const FRTMapElementHandle& Handle)
{
	if (Map == nullptr)
	{
		return ERTMapEditOutcome::RefusedUnresolved;
	}

	// --- Muro interno: risolve per nome, o per chiave se anonimo ------------------------------------
	if (Handle.Kind == ERTMapElementKind::InteriorWall)
	{
		const int32 Index = ResolveInteriorWall(Map, Handle);
		if (Index == INDEX_NONE)
		{
			return ERTMapEditOutcome::RefusedUnresolved;
		}
		Map->InteriorWalls.RemoveAt(Index);
		return ERTMapEditOutcome::Applied;
	}

	// --- Copertura e porta: vivono DENTRO la cella, quindi si riscrive la cella ---------------------
	if (Handle.Kind == ERTMapElementKind::Cover || Handle.Kind == ERTMapElementKind::Door)
	{
		const FRTHexCellData* Existing = Map->FindCell(Handle.Cell);
		if (Existing == nullptr)
		{
			return ERTMapEditOutcome::RefusedUnresolved;
		}

		FRTHexCellData Data = *Existing;
		int32 Removed = 0;
		TArray<FName> OrphanedNames;

		if (Handle.Kind == ERTMapElementKind::Cover)
		{
			Removed = Data.Covers.RemoveAll([&Handle](const FRTHexCover& C) { return C.Edge == Handle.Edge; });
		}
		else
		{
			Removed = Data.Doors.RemoveAll([&Handle, &OrphanedNames](const FRTHexDoor& D)
			{
				if (D.Edge != Handle.Edge)
				{
					return false;
				}
				if (!D.StableId.IsNone())
				{
					OrphanedNames.AddUnique(D.StableId);
				}
				return true;
			});
		}

		if (Removed == 0)
		{
			return ERTMapEditOutcome::RefusedUnresolved;
		}

		Map->AddOrUpdateCell(Data);

		// 🔴 Lo stesso `C2` della cascata della cella: un binding che nomina una struttura sparita diventa
		// «riferimento a una struttura inesistente». La regola non cambia perche' cambia il gesto.
		//
		// ⚠️ Il nome muore solo se NESSUN bordo sopravvive: un portone e' un gruppo, e togliergli un lato non
		// lo cancella. `FindDoorEdges` interroga la mappa GIA' aggiornata, quindi risponde su cio' che resta.
		TArray<FName> ActuallyDead;
		for (const FName& Name : OrphanedNames)
		{
			// Il nome muore solo se NESSUN bordo gli sopravvive: un portone e' un gruppo. `FindDoorEdges`
			// interroga la mappa gia' aggiornata, quindi risponde su cio' che resta.
			if (URTStructureIdentityLibrary::FindDoorEdges(Map, Name).Num() == 0)
			{
				ActuallyDead.Add(Name);
			}
		}
		RTApplyOrphanedNames(Map, ActuallyDead);

		return ERTMapEditOutcome::Applied;
	}

	if (Handle.Kind != ERTMapElementKind::Cell)
	{
		// `Transition` non ha ancora un gesto che la selezioni. Un `Applied` a vuoto sarebbe peggio di un
		// rifiuto: chi chiama crederebbe di aver cancellato qualcosa.
		return ERTMapEditOutcome::RefusedUnresolved;
	}

	if (Map->FindCell(Handle.Cell) == nullptr)
	{
		return ERTMapEditOutcome::RefusedUnresolved;
	}

	const FRTMapDependencySet Set = URTMapDependencyLibrary::CollectDependents(Map, Handle);

	// **Dal piu' alto al piu' basso.** Rimuovere dal basso sposta gli elementi successivi e invalida gli
	// indici gia' raccolti: e' il contratto che `FRTMapDependencySet` dichiara, e sta qui una volta sola
	// perche' nessun chiamante debba ricordarselo.
	TArray<int32> Walls = Set.InteriorWallIndices;
	TArray<int32> Edges = Set.TransitionIndices;
	TArray<int32> Bindings = Set.InteractionBindingIndices;

	auto Descending = [](int32 A, int32 B) { return A > B; };
	Walls.Sort(Descending);
	Edges.Sort(Descending);
	Bindings.Sort(Descending);

	for (const int32 Index : Walls)
	{
		Map->InteriorWalls.RemoveAt(Index);
	}
	for (const int32 Index : Edges)
	{
		Map->Transitions.RemoveAt(Index);
	}
	for (const int32 Index : Bindings)
	{
		Map->InteractionBindings.RemoveAt(Index);
	}

	// I superstiti perdono i bersagli morti, e muoiono se restano senza: stessa regola del ramo porta,
	// stessa funzione. Averne due significherebbe che divergono, ed e' gia' successo.
	RTApplyOrphanedNames(Map, Set.OrphanedStructureNames);

	// La cella per ultima: `CollectDependents` la legge per sapere quali nomi se ne vanno con lei, e
	// toglierla prima renderebbe vuoto l'insieme dei nomi morenti.
	Map->RemoveCell(Handle.Cell);

	return ERTMapEditOutcome::Applied;
}

TArray<FRTMapElementHandle> URTMapEditLibrary::ElementsAt(const URTHexMapAsset* Map,
	const FRTCellId& Cell, ERTHexDirection Edge)
{
	TArray<FRTMapElementHandle> Found;

	if (Map == nullptr)
	{
		return Found;
	}

	const FRTHexCellData* Data = Map->FindCell(Cell);
	if (Data == nullptr)
	{
		return Found;
	}

	// L'ordine e' il contratto: dal piu' specifico al piu' generale, cosi' il primo click prende cio' che
	// l'autore ha piu' probabilmente mirato e i successivi scendono verso la cella.
	//
	// `break` dopo il primo: `ValidateMap` garantisce al massimo UNA porta e UNA copertura per bordo. Su un
	// dato gia' invalido si prende la prima, che e' deterministico — non si inventa una risoluzione qui.
	for (const FRTHexDoor& Door : Data->Doors)
	{
		if (Door.Edge == Edge)
		{
			Found.Add(FRTMapElementHandle::ForDoor(Cell, Edge, Door.StableId));
			break;
		}
	}

	for (const FRTHexCover& Cover : Data->Covers)
	{
		if (Cover.Edge == Edge)
		{
			Found.Add(FRTMapElementHandle::ForCover(Cell, Edge));
			break;
		}
	}

	// I muri interni vivono NELLA cella, non su un bordo: ci sono qualunque bordo si sia cliccato.
	for (const FRTHexInteriorWall& Wall : Map->InteriorWalls)
	{
		if (Wall.Cell == Cell)
		{
			// Nominato quando ha un nome, per chiave quando non ce l'ha: ogni candidato emesso deve
			// RISOLVERE, altrimenti il ciclo di click mostrerebbe un bersaglio che non si puo' prendere.
			Found.Add(Wall.StableId.IsNone()
				? FRTMapElementHandle::ForInteriorWallAt(Wall.Cell, Wall.Segment)
				: FRTMapElementHandle::ForInteriorWall(Wall.StableId));
		}
	}

	Found.Add(FRTMapElementHandle::ForCell(Cell));

	return Found;
}
