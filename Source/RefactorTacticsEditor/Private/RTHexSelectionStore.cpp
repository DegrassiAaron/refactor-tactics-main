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

	// Un candidato e' PRENDIBILE se non e' gia' in selezione. `Skip` esclude una posizione — serve al ciclo,
	// dove l'elemento che si sta sostituendo non deve contare come duplicato di se stesso.
	auto IsFree = [this](const FRTMapElementHandle& Candidate, int32 Skip) -> bool
	{
		for (int32 I = 0; I < Selection.Num(); ++I)
		{
			if (I != Skip && SameElement(Selection[I], Candidate))
			{
				return false;
			}
		}
		return true;
	};

	const bool bSamePoint = bHasCycle && CycleCell == Cell && CycleEdge == Edge;

	// 🔴 **Il ciclo agisce sull'ultimo SOLO se l'ultimo appartiene a questo punto.** Altrimenti sostituirebbe
	// un elemento preso altrove con un candidato di qui — trovato da una code review insieme al difetto
	// gemello: il vecchio ramo «gia' dentro» usciva senza spostare il ciclo, e tornando su un punto gia'
	// preso ogni Ctrl+click ripeteva lo stesso nulla, rendendo gli altri candidati irraggiungibili.
	const int32 Last = Selection.Num() - 1;
	const bool bLastBelongsHere = bSamePoint && Last >= 0
		&& Candidates.ContainsByPredicate([this, Last](const FRTMapElementHandle& C)
		{
			return SameElement(Selection[Last], C);
		});

	if (bLastBelongsHere)
	{
		// Avanza al prossimo candidato LIBERO, saltando quelli gia' selezionati: senza questo salto il ciclo
		// poteva mettere due volte lo stesso elemento, e `EraseSelection` avrebbe provato a cancellarlo due
		// volte — la seconda su una mappa da cui era gia' sparito.
		for (int32 Step = 1; Step <= Candidates.Num(); ++Step)
		{
			const int32 Next = (CycleIndex + Step) % Candidates.Num();
			if (IsFree(Candidates[Next], Last))
			{
				CycleIndex = Next;
				Selection[Last] = Candidates[Next];
				return true;
			}
		}

		// Tutti gia' presi: il gesto non ha effetto, ma non e' un fallimento.
		return true;
	}

	// Punto nuovo, o l'ultimo viene da altrove: si aggiunge il primo candidato libero. Cosi' un punto i cui
	// candidati specifici sono gia' in selezione contribuisce comunque quelli che restano, invece di
	// diventare un gesto morto.
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		if (IsFree(Candidates[Index], INDEX_NONE))
		{
			Selection.Add(Candidates[Index]);
			CycleCell = Cell;
			CycleEdge = Edge;
			bHasCycle = true;
			CycleIndex = Index;
			return true;
		}
	}

	// Niente da aggiungere, ma il ciclo si sposta comunque QUI: il prossimo gesto su questo punto deve
	// poter scorrere, non ripetere il nulla che ha appena fatto.
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
