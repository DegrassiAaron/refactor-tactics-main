#include "Map/RTMapEditLibrary.h"

#include "Map/RTGeometryBake.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexMapAsset.h"

int32 URTMapEditLibrary::ResolveInteriorWall(const URTHexMapAsset* Map, const FRTMapElementHandle& Handle)
{
	// `NAME_None` non risolve: significa «muro senza nome», e ce ne possono essere molti. Restituirne uno a
	// caso sarebbe peggio che non restituirne nessuno — un tool crederebbe di avere un bersaglio.
	if (Map == nullptr
		|| Handle.Kind != ERTMapElementKind::InteriorWall
		|| Handle.StableId.IsNone())
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < Map->InteriorWalls.Num(); ++Index)
	{
		if (Map->InteriorWalls[Index].StableId == Handle.StableId)
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
