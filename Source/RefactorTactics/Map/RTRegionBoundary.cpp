#include "Map/RTRegionBoundary.h"
#include "Map/RTHexLibrary.h"

namespace
{
	/** Le sei direzioni in ordine stabile 0..5, come le enumera `URTHexLibrary`. */
	constexpr ERTHexDirection RTAllDirections[] = {
		ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
		ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE
	};
	static_assert(UE_ARRAY_COUNT(RTAllDirections) == 6, "l'esagono ha sei lati");
}

bool URTRegionBoundaryLibrary::CellLess(const FRTCellId& A, const FRTCellId& B)
{
	if (A.Layer != B.Layer) { return A.Layer < B.Layer; }
	if (A.X != B.X)         { return A.X < B.X; }
	return A.Y < B.Y;
}

TArray<FRTBoundaryEdge> URTRegionBoundaryLibrary::ExtractBoundaryEdges(const TSet<FRTCellId>& Region)
{
	TArray<FRTBoundaryEdge> Edges;
	Edges.Reserve(Region.Num() * 2); // stima larga: una regione compatta ne ha molti meno

	for (const FRTCellId& Cell : Region)
	{
		for (const ERTHexDirection Dir : RTAllDirections)
		{
			// 🔑 L'intera regola sta qui. `Neighbor` conserva il `Layer`, quindi questo confronto non puo'
			// attraversare un piano nemmeno volendo: e' da questa riga che discendono il multilayer e il
			// vincolo del cliff, non da un controllo aggiunto dopo.
			if (!Region.Contains(URTHexLibrary::Neighbor(Cell, Dir)))
			{
				Edges.Emplace(Cell, Dir);
			}
		}
	}

	// ⛔ **L'ordine del `TSet` non esce di qui.** Iterarlo da' un ordine che dipende dall'hash e dalla storia
	// degli inserimenti: due regioni con le stesse celle inserite in ordine diverso darebbero due uscite
	// diverse, e un test che passasse su una potrebbe cadere sull'altra senza che nulla sia cambiato.
	Edges.Sort([](const FRTBoundaryEdge& A, const FRTBoundaryEdge& B)
	{
		if (!(A.Cell == B.Cell)) { return CellLess(A.Cell, B.Cell); }
		return static_cast<uint8>(A.Dir) < static_cast<uint8>(B.Dir);
	});
	return Edges;
}

TArray<FRTBoundaryEdge> URTRegionBoundaryLibrary::ExtractBoundaryEdges(const TArray<FRTCellId>& Region)
{
	return ExtractBoundaryEdges(TSet<FRTCellId>(Region));
}

TArray<FRTBoundaryEdge> URTRegionBoundaryLibrary::ExtractBoundaryEdges(const FRTOverlayArea& Area)
{
	return ExtractBoundaryEdges(Area.Cells);
}

TArray<TArray<FRTCellId>> URTRegionBoundaryLibrary::ConnectedComponents(const TSet<FRTCellId>& Region)
{
	// Le celle si visitano in ordine CANONICO, non in ordine di `TSet`: e' cio' che rende ripetibile sia
	// quale componente esce per prima sia il contenuto di ciascuna.
	TArray<FRTCellId> Ordered = Region.Array();
	Ordered.Sort(&URTRegionBoundaryLibrary::CellLess);

	TArray<TArray<FRTCellId>> Components;
	TSet<FRTCellId> Seen;
	Seen.Reserve(Region.Num());

	for (const FRTCellId& Start : Ordered)
	{
		if (Seen.Contains(Start))
		{
			continue;
		}

		// Visita in ampiezza. La coda parte da una cella scelta in ordine canonico, quindi anche il
		// contenuto della componente non dipende dall'hash.
		TArray<FRTCellId> Component;
		TArray<FRTCellId> Queue;
		Queue.Add(Start);
		Seen.Add(Start);

		while (Queue.Num() > 0)
		{
			const FRTCellId Current = Queue.Pop(EAllowShrinking::No);
			Component.Add(Current);

			for (const ERTHexDirection Dir : RTAllDirections)
			{
				const FRTCellId Next = URTHexLibrary::Neighbor(Current, Dir);
				// Stesso vincolo di sopra: `Neighbor` non cambia piano, quindi due gruppi di celle con lo
				// stesso `X/Y` su `Layer` diversi restano due componenti.
				if (Region.Contains(Next) && !Seen.Contains(Next))
				{
					Seen.Add(Next);
					Queue.Add(Next);
				}
			}
		}

		Component.Sort(&URTRegionBoundaryLibrary::CellLess);
		Components.Add(MoveTemp(Component));
	}

	Components.Sort([](const TArray<FRTCellId>& A, const TArray<FRTCellId>& B)
	{
		// Entrambe sono gia' ordinate, quindi `[0]` e' la cella minima della componente.
		if (A.Num() == 0 || B.Num() == 0) { return A.Num() > B.Num(); }
		return CellLess(A[0], B[0]);
	});
	return Components;
}

TArray<TArray<FRTCellId>> URTRegionBoundaryLibrary::ConnectedComponents(const TArray<FRTCellId>& Region)
{
	return ConnectedComponents(TSet<FRTCellId>(Region));
}
