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

	// 🔴 **Anche l'aggiunta CICLA, e la prima stesura non lo faceva** — trovato in seduta: prendendo sempre
	// il candidato piu' specifico, una cella con una copertura sul bordo mirato era **impossibile da
	// aggiungere**. Ctrl+click dava la copertura, sempre, e nessun gesto scendeva alla cella: un elemento
	// raggiungibile col click semplice e irraggiungibile con quello multiplo, cioe' il rilievo `A1` spostato
	// di un tasto.
	//
	// ⚠️ **Cicla l'ULTIMO aggiunto, non tutta la selezione.** Gli elementi presi prima restano dove sono,
	// altrimenti costruire una selezione di tre elementi sarebbe impossibile: l'ultimo gesto disferebbe i
	// precedenti.
	const bool bSamePoint = bHasCycle && CycleCell == Cell && CycleEdge == Edge;

	if (bSamePoint && Selection.Num() > 0)
	{
		CycleIndex = (CycleIndex + 1) % Candidates.Num();
		Selection.Last() = Candidates[CycleIndex];

		CycleCell = Cell;
		CycleEdge = Edge;
		return true;
	}

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

	// Il ciclo si sposta su QUESTO punto: il prossimo Ctrl+click qui scende al candidato successivo, e un
	// `SelectAt` altrove riparte da capo perche' il punto e' cambiato. Una sola memoria di ciclo per
	// entrambi i gesti — averne due significherebbe che l'una diverge dall'altra.
	CycleCell = Cell;
	CycleEdge = Edge;
	bHasCycle = true;
	CycleIndex = 0;

	return true;
}

void URTHexSelectionStore::Clear()
{
	Selection.Reset();
	bHasCycle = false;
	CycleIndex = INDEX_NONE;
}

FString URTHexSelectionStore::Describe(const TArray<FRTMapElementHandle>& Handles)
{
	if (Handles.Num() == 0)
	{
		return TEXT("niente");
	}

	auto DescribeOne = [](const FRTMapElementHandle& H) -> FString
	{
		switch (H.Kind)
		{
		case ERTMapElementKind::Cell:
			return FString::Printf(TEXT("cella %s"), *H.Cell.ToString());

		case ERTMapElementKind::Cover:
			return FString::Printf(TEXT("copertura su %s bordo %d"),
				*H.Cell.ToString(), static_cast<int32>(H.Edge));

		case ERTMapElementKind::Door:
			// Il nome quando c'e'; altrimenti il bordo. ⚠️ Mai `NAME_None` a schermo: «None» si legge come
			// un nome, e chi lo vedesse lo cercherebbe.
			return H.StableId.IsNone()
				? FString::Printf(TEXT("porta su %s bordo %d"), *H.Cell.ToString(), static_cast<int32>(H.Edge))
				: FString::Printf(TEXT("porta '%s'"), *H.StableId.ToString());

		case ERTMapElementKind::InteriorWall:
			return H.StableId.IsNone()
				? FString::Printf(TEXT("muro interno (senza nome) su %s"), *H.Cell.ToString())
				: FString::Printf(TEXT("muro interno '%s'"), *H.StableId.ToString());

		case ERTMapElementKind::Transition:
			return TEXT("transizione");

		default:
			return TEXT("elemento sconosciuto");
		}
	};

	if (Handles.Num() == 1)
	{
		return DescribeOne(Handles[0]);
	}

	// Con piu' elementi si dichiara il conteggio e il primo: elencarli tutti riempirebbe il pannello, e il
	// numero e' l'informazione che dice quanto porterebbe via un Erase.
	return FString::Printf(TEXT("%d elementi (%s, ...)"), Handles.Num(), *DescribeOne(Handles[0]));
}
