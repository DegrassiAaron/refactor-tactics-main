#include "RTHexSelectionStore.h"

#include "Map/RTMapEditLibrary.h"

namespace
{
	/**
	 * Due handle nominano lo stesso elemento?
	 *
	 * Confronto per `Kind` piu' i campi che quel `Kind` rende significativi — non membro a membro alla cieca:
	 * un handle di cella porta un `Edge` che vale il suo default, e confrontarlo direbbe «diversi» per un
	 * campo che per quel tipo non significa niente.
	 */
	bool SameElement(const FRTMapElementHandle& A, const FRTMapElementHandle& B)
	{
		if (A.Kind != B.Kind)
		{
			return false;
		}

		switch (A.Kind)
		{
		case ERTMapElementKind::Cell:
			return A.Cell == B.Cell;

		case ERTMapElementKind::Cover:
			return A.Cell == B.Cell && A.Edge == B.Edge;

		case ERTMapElementKind::Door:
			// Il nome identifica la STRUTTURA, che puo' essere un gruppo di bordi: due bordi dello stesso
			// portone sono lo stesso elemento. Senza nome resta il bordo.
			return A.StableId.IsNone() || B.StableId.IsNone()
				? (A.Cell == B.Cell && A.Edge == B.Edge)
				: (A.StableId == B.StableId);

		case ERTMapElementKind::InteriorWall:
			return A.StableId.IsNone() || B.StableId.IsNone()
				? (A.Cell == B.Cell && A.Segment == B.Segment)
				: (A.StableId == B.StableId);

		default:
			return false;
		}
	}
}

bool URTHexSelectionStore::SelectAt(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge)
{
	const TArray<FRTMapElementHandle> Candidates = URTMapEditLibrary::ElementsAt(Map, Cell, Edge);
	if (Candidates.Num() == 0)
	{
		return false;
	}

	// Stesso punto -> si avanza nel ciclo; punto nuovo -> si riparte dal piu' specifico. Senza il confronto
	// col punto precedente questo indice sarebbe un contatore globale, e il primo click su una cella nuova
	// prenderebbe un elemento a caso a seconda dei click fatti altrove.
	const bool bSamePoint = bHasCycle && CycleCell == Cell && CycleEdge == Edge;
	CycleIndex = bSamePoint ? (CycleIndex + 1) % Candidates.Num() : 0;

	CycleCell = Cell;
	CycleEdge = Edge;
	bHasCycle = true;

	Selection.Reset();
	Selection.Add(Candidates[CycleIndex]);
	return true;
}

bool URTHexSelectionStore::AddAt(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge)
{
	const TArray<FRTMapElementHandle> Candidates = URTMapEditLibrary::ElementsAt(Map, Cell, Edge);
	if (Candidates.Num() == 0)
	{
		return false;
	}

	// L'aggiunta non cicla: prende il piu' specifico. Un ciclo qui chiederebbe di ricordare a che punto del
	// giro si sta per ciascun punto gia' selezionato.
	const FRTMapElementHandle& Taken = Candidates[0];

	for (const FRTMapElementHandle& Already : Selection)
	{
		if (SameElement(Already, Taken))
		{
			// Gia' dentro: non e' un fallimento del gesto, e' un gesto senza effetto.
			return true;
		}
	}

	Selection.Add(Taken);

	// Un'aggiunta interrompe il ciclo: il prossimo `SelectAt` su quel punto riparte dal piu' specifico
	// invece di continuare un giro che l'utente ha lasciato a meta' due gesti fa.
	bHasCycle = false;
	CycleIndex = INDEX_NONE;

	return true;
}

void URTHexSelectionStore::Clear()
{
	Selection.Reset();
	bHasCycle = false;
	CycleIndex = INDEX_NONE;
}
