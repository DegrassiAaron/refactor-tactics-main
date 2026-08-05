#include "Grid/RTGridLibrary.h"
#include "Algo/Reverse.h"

FVector URTGridLibrary::CellToWorld(const FRTCellId& Cell, const FVector& Origin, float CellSize)
{
	// Centro della cella: (indice + 0.5) * dimensione, a partire dall'origine. Z invariato.
	return FVector(
		Origin.X + (static_cast<double>(Cell.X) + 0.5) * CellSize,
		Origin.Y + (static_cast<double>(Cell.Y) + 0.5) * CellSize,
		Origin.Z);
}

FVector URTGridLibrary::CellToWorldElevated(const FRTCellId& Cell, const FVector& Origin, float CellSize,
	float ZOffset, float LayerHeight)
{
	// Centro-cella (X,Y a terra) piu' l'offset verticale del pivot e l'elevazione del layer.
	const FVector Center = CellToWorld(Cell, Origin, CellSize);
	return Center + FVector(0.0, 0.0,
		static_cast<double>(ZOffset) + static_cast<double>(Cell.Layer) * static_cast<double>(LayerHeight));
}

float URTGridLibrary::DirectionYaw(const FVector& From, const FVector& To)
{
	const FVector Dir = To - From;
	if (Dir.SizeSquared2D() <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.f; // nessuna direzione planare -> nessun orientamento
	}
	// Atan2(Y,X): +X=0, +Y=90, -X=+/-180, -Y=-90 (convenzione yaw UE). Z ignorata (facing planare).
	return FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
}

FRTCellId URTGridLibrary::WorldToCell(const FVector& World, const FVector& Origin, float CellSize)
{
	// Floor della posizione locale in unita' di cella: ogni punto della cella mappa allo stesso indice.
	const double LocalX = (World.X - Origin.X) / CellSize;
	const double LocalY = (World.Y - Origin.Y) / CellSize;
	return FRTCellId(FMath::FloorToInt32(LocalX), FMath::FloorToInt32(LocalY));
}

bool URTGridLibrary::IsInBounds(const FRTCellId& Cell, int32 Width, int32 Height)
{
	return Cell.X >= 0 && Cell.X < Width && Cell.Y >= 0 && Cell.Y < Height;
}

bool URTGridLibrary::IsInsideGrid(const FRTCellId& Cell, int32 Width, int32 Height)
{
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Width && Cell.Y < Height;
}

int32 URTGridLibrary::ManhattanDistance(const FRTCellId& A, const FRTCellId& B)
{
	return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

bool URTGridLibrary::IsWithinRange(const FRTCellId& From, const FRTCellId& To, int32 Range)
{
	return ManhattanDistance(From, To) <= Range;
}

bool URTGridLibrary::HasLineOfSight(const FRTCellId& From, const FRTCellId& To, const TArray<FRTCellId>& Blockers)
{
	if (Blockers.Num() == 0 || From == To)
	{
		return true;
	}

	// Campiona il segmento 2D tra i centri delle due celle. Regola di ELEVAZIONE: un blocker blocca
	// la LOS solo se sta allo STESSO layer del tiratore (From.Layer). Cosi' da terra si spara "sotto"
	// il ponte (layer 1 != 0) e dal ponte si spara "oltre" le coperture basse (layer 0 != 1).
	// From e To non contano come ostacoli (confronto solo su X,Y).
	const int32 DX = To.X - From.X;
	const int32 DY = To.Y - From.Y;
	const int32 Steps = FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) * 4; // risoluzione sufficiente per la griglia

	for (int32 i = 1; i < Steps; ++i)
	{
		const double T = static_cast<double>(i) / Steps;
		const int32 CX = FMath::RoundToInt32(From.X + DX * T);
		const int32 CY = FMath::RoundToInt32(From.Y + DY * T);

		const bool bIsEndpoint = (CX == From.X && CY == From.Y) || (CX == To.X && CY == To.Y);
		if (!bIsEndpoint && Blockers.Contains(FRTCellId(CX, CY, From.Layer)))
		{
			return false;
		}
	}
	return true;
}

TArray<FRTCellId> URTGridLibrary::CellsInRadius(const FRTCellId& Center, int32 Radius)
{
	TArray<FRTCellId> Result;
	const int32 R = FMath::Max(0, Radius);
	for (int32 DY = -R; DY <= R; ++DY)
	{
		for (int32 DX = -R; DX <= R; ++DX)
		{
			if (FMath::Abs(DX) + FMath::Abs(DY) <= R)
			{
				Result.Add(FRTCellId(Center.X + DX, Center.Y + DY));
			}
		}
	}
	return Result;
}

TArray<FRTCellId> URTGridLibrary::CellsInLine(const FRTCellId& From, const FRTCellId& To)
{
	TArray<FRTCellId> Result;
	if (From == To)
	{
		Result.Add(To);
		return Result;
	}

	// Campiona il segmento From->To; raccoglie le celle attraversate (From escluso), senza duplicati.
	const int32 DX = To.X - From.X;
	const int32 DY = To.Y - From.Y;
	const int32 Steps = FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) * 4;
	for (int32 i = 1; i <= Steps; ++i)
	{
		const double T = static_cast<double>(i) / Steps;
		const FRTCellId Cell(
			FMath::RoundToInt32(From.X + DX * T),
			FMath::RoundToInt32(From.Y + DY * T));
		if (Cell != From)
		{
			Result.AddUnique(Cell);
		}
	}
	return Result;
}

namespace
{
	// Vicini ortogonali in ordine fisso (E, W, S, N) -> espansione BFS deterministica.
	constexpr int32 GStepDX[4] = { 1, -1, 0, 0 };
	constexpr int32 GStepDY[4] = { 0, 0, 1, -1 };

	FORCEINLINE bool IsTraversable(const FRTCellId& Cell, const TArray<FRTCellId>& Blockers, int32 Width, int32 Height)
	{
		return URTGridLibrary::IsInsideGrid(Cell, Width, Height) && !Blockers.Contains(Cell);
	}
}

TArray<FRTCellId> URTGridLibrary::ReachableCells(const FRTCellId& From, int32 MoveRange,
	const TArray<FRTCellId>& Blockers, int32 Width, int32 Height)
{
	TArray<FRTCellId> Result;
	if (!IsTraversable(From, Blockers, Width, Height))
	{
		return Result; // origine fuori griglia o bloccata: nessuna cella
	}

	// BFS a costo uniforme: la distanza in passi = numero minimo di mosse ortogonali.
	TMap<FRTCellId, int32> Distance;
	TArray<FRTCellId> Frontier;
	Distance.Add(From, 0);
	Frontier.Add(From);
	Result.Add(From); // From è sempre "raggiungibile" (fermarsi è lecito)

	const int32 Budget = FMath::Max(0, MoveRange);
	int32 Head = 0;
	while (Head < Frontier.Num())
	{
		const FRTCellId Current = Frontier[Head++];
		const int32 Dist = Distance[Current];
		if (Dist >= Budget)
		{
			continue; // esaurito il budget di passi da questa cella
		}
		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			const FRTCellId Next(Current.X + GStepDX[Dir], Current.Y + GStepDY[Dir]);
			if (Distance.Contains(Next) || !IsTraversable(Next, Blockers, Width, Height))
			{
				continue;
			}
			Distance.Add(Next, Dist + 1);
			Frontier.Add(Next);
			Result.Add(Next);
		}
	}
	return Result;
}

TArray<FRTCellId> URTGridLibrary::FindPath(const FRTCellId& From, const FRTCellId& To,
	const TArray<FRTCellId>& Blockers, int32 Width, int32 Height)
{
	TArray<FRTCellId> Path;
	if (!IsTraversable(From, Blockers, Width, Height) || !IsTraversable(To, Blockers, Width, Height))
	{
		return Path; // estremi non validi
	}
	if (From == To)
	{
		Path.Add(From);
		return Path;
	}

	// BFS con tracciamento del predecessore; costo uniforme -> il primo raggiungimento è minimo.
	TMap<FRTCellId, FRTCellId> Parent;
	TArray<FRTCellId> Frontier;
	Parent.Add(From, From); // il predecessore di From è se stesso (sentinella)
	Frontier.Add(From);

	int32 Head = 0;
	bool bFound = false;
	while (Head < Frontier.Num() && !bFound)
	{
		const FRTCellId Current = Frontier[Head++];
		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			const FRTCellId Next(Current.X + GStepDX[Dir], Current.Y + GStepDY[Dir]);
			if (Parent.Contains(Next) || !IsTraversable(Next, Blockers, Width, Height))
			{
				continue;
			}
			Parent.Add(Next, Current);
			if (Next == To) { bFound = true; break; }
			Frontier.Add(Next);
		}
	}

	if (!bFound)
	{
		return Path; // To irraggiungibile
	}

	// Ricostruzione From..To risalendo i predecessori, poi inversione.
	for (FRTCellId Cell = To; ; Cell = Parent[Cell])
	{
		Path.Add(Cell);
		if (Cell == From) { break; }
	}
	Algo::Reverse(Path);
	return Path;
}

namespace
{
	// Costo di ENTRATA in una cella: assente dalla mappa -> 1. Negativo -> impassabile.
	FORCEINLINE int32 EnterCost(const FRTCellId& Cell, const TMap<FRTCellId, int32>& CellCost)
	{
		const int32* Found = CellCost.Find(Cell);
		return Found ? *Found : 1;
	}

	// Dijkstra deterministico su griglia (tie-break per (X,Y)). Riempie Dist (costo minimo per cella
	// raggiungibile entro Budget) e, se Parent != nullptr, i predecessori. Se Goal != nullptr, si ferma
	// appena Goal e' definitivo. Budget < 0 = illimitato.
	void DijkstraGrid(const FRTCellId& From, int32 Budget,
		const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height,
		TMap<FRTCellId, int32>& Dist, TMap<FRTCellId, FRTCellId>* Parent, const FRTCellId* Goal,
		const TMap<FRTCellId, TArray<TPair<FRTCellId, int32>>>* EdgeAdj = nullptr)
	{
		static const int32 DX[4] = { 1, -1, 0, 0 };
		static const int32 DY[4] = { 0, 0, 1, -1 };

		Dist.Add(From, 0);
		if (Parent) { Parent->Add(From, From); }
		TSet<FRTCellId> Settled;

		while (true)
		{
			// Cella non definitiva a distanza minima; a parita', ordine (X, Y, Layer) -> deterministico.
			FRTCellId Best;
			int32 BestDist = MAX_int32;
			bool bFound = false;
			for (const TPair<FRTCellId, int32>& It : Dist)
			{
				if (Settled.Contains(It.Key)) { continue; }
				const bool bLess = It.Value < BestDist
					|| (It.Value == BestDist && (It.Key.X < Best.X
						|| (It.Key.X == Best.X && (It.Key.Y < Best.Y
							|| (It.Key.Y == Best.Y && It.Key.Layer < Best.Layer)))));
				if (!bFound || bLess)
				{
					Best = It.Key; BestDist = It.Value; bFound = true;
				}
			}
			if (!bFound) { break; }
			Settled.Add(Best);
			if (Goal && Best == *Goal) { break; }

			// Rilassa un vicino raggiunto con un dato costo (ortogonale o via arco).
			auto Relax = [&](const FRTCellId& Next, int32 Step)
			{
				if (Step < 0 || Settled.Contains(Next)) { return; }
				const int32 ND = BestDist + Step;
				if (Budget >= 0 && ND > Budget) { return; }
				const int32* Cur = Dist.Find(Next);
				if (!Cur || ND < *Cur)
				{
					Dist.Add(Next, ND);
					if (Parent) { Parent->Add(Next, Best); }
				}
			};

			// Vicini ortogonali (stesso layer).
			for (int32 Dir = 0; Dir < 4; ++Dir)
			{
				const FRTCellId Next(Best.X + DX[Dir], Best.Y + DY[Dir], Best.Layer);
				if (URTGridLibrary::IsInsideGrid(Next, Width, Height))
				{
					Relax(Next, EnterCost(Next, CellCost));
				}
			}

			// Archi di traversata uscenti (scale/portali/cross-layer).
			if (EdgeAdj)
			{
				if (const TArray<TPair<FRTCellId, int32>>* Out = EdgeAdj->Find(Best))
				{
					for (const TPair<FRTCellId, int32>& E : *Out)
					{
						Relax(E.Key, FMath::Max(0, E.Value));
					}
				}
			}
		}
	}
}

TArray<FRTCellId> URTGridLibrary::ReachableCellsByCost(const FRTCellId& From, int32 CostBudget,
	const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height)
{
	TArray<FRTCellId> Result;
	if (!IsInsideGrid(From, Width, Height) || EnterCost(From, CellCost) < 0)
	{
		return Result;
	}

	TMap<FRTCellId, int32> Dist;
	DijkstraGrid(From, FMath::Max(0, CostBudget), CellCost, Width, Height, Dist, nullptr, nullptr);

	// Ordine stabile: scansione della griglia in (X,Y).
	for (int32 X = 0; X < Width; ++X)
	{
		for (int32 Y = 0; Y < Height; ++Y)
		{
			const FRTCellId Cell(X, Y);
			if (Dist.Contains(Cell)) { Result.Add(Cell); }
		}
	}
	return Result;
}

TArray<FRTCellId> URTGridLibrary::FindPathByCost(const FRTCellId& From, const FRTCellId& To,
	const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height)
{
	TArray<FRTCellId> Path;
	if (!IsInsideGrid(From, Width, Height) || !IsInsideGrid(To, Width, Height)
		|| EnterCost(From, CellCost) < 0 || EnterCost(To, CellCost) < 0)
	{
		return Path;
	}
	if (From == To)
	{
		Path.Add(From);
		return Path;
	}

	TMap<FRTCellId, int32> Dist;
	TMap<FRTCellId, FRTCellId> Parent;
	DijkstraGrid(From, -1, CellCost, Width, Height, Dist, &Parent, &To);

	if (!Parent.Contains(To))
	{
		return Path; // irraggiungibile
	}
	for (FRTCellId Cell = To; ; Cell = Parent[Cell])
	{
		Path.Add(Cell);
		if (Cell == From) { break; }
	}
	Algo::Reverse(Path);
	return Path;
}

namespace
{
	// Adiacenza degli archi: From -> lista di (To, Costo).
	TMap<FRTCellId, TArray<TPair<FRTCellId, int32>>> BuildEdgeAdj(const TArray<FRTTraversalEdge>& Edges)
	{
		TMap<FRTCellId, TArray<TPair<FRTCellId, int32>>> Adj;
		for (const FRTTraversalEdge& E : Edges)
		{
			Adj.FindOrAdd(E.From).Add(TPair<FRTCellId, int32>(E.To, E.Cost));
		}
		return Adj;
	}
}

TArray<FRTCellId> URTGridLibrary::ReachableCellsByGraph(const FRTCellId& From, int32 CostBudget,
	const TMap<FRTCellId, int32>& CellCost, const TArray<FRTTraversalEdge>& Edges, int32 Width, int32 Height)
{
	TArray<FRTCellId> Result;
	if (!IsInsideGrid(From, Width, Height) || EnterCost(From, CellCost) < 0)
	{
		return Result;
	}
	const TMap<FRTCellId, TArray<TPair<FRTCellId, int32>>> EdgeAdj = BuildEdgeAdj(Edges);

	TMap<FRTCellId, int32> Dist;
	DijkstraGrid(From, FMath::Max(0, CostBudget), CellCost, Width, Height, Dist, nullptr, nullptr, &EdgeAdj);

	// Ordine stabile su (Layer, X, Y): include le celle su qualsiasi layer.
	Dist.GetKeys(Result);
	Result.Sort([](const FRTCellId& A, const FRTCellId& B)
	{
		if (A.Layer != B.Layer) { return A.Layer < B.Layer; }
		if (A.X != B.X) { return A.X < B.X; }
		return A.Y < B.Y;
	});
	return Result;
}

TArray<FRTCellId> URTGridLibrary::FindPathByGraph(const FRTCellId& From, const FRTCellId& To,
	const TMap<FRTCellId, int32>& CellCost, const TArray<FRTTraversalEdge>& Edges, int32 Width, int32 Height)
{
	TArray<FRTCellId> Path;
	if (EnterCost(From, CellCost) < 0 || EnterCost(To, CellCost) < 0)
	{
		return Path;
	}
	if (From == To)
	{
		Path.Add(From);
		return Path;
	}
	const TMap<FRTCellId, TArray<TPair<FRTCellId, int32>>> EdgeAdj = BuildEdgeAdj(Edges);

	TMap<FRTCellId, int32> Dist;
	TMap<FRTCellId, FRTCellId> Parent;
	DijkstraGrid(From, -1, CellCost, Width, Height, Dist, &Parent, &To, &EdgeAdj);

	if (!Parent.Contains(To))
	{
		return Path; // irraggiungibile
	}
	for (FRTCellId Cell = To; ; Cell = Parent[Cell])
	{
		Path.Add(Cell);
		if (Cell == From) { break; }
	}
	Algo::Reverse(Path);
	return Path;
}

TArray<FRTCellId> URTGridLibrary::BuildCompositePath(const FRTCellId& Start,
	const TArray<FRTCellId>& Waypoints, const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height,
	const TArray<FRTTraversalEdge>& Edges)
{
	TArray<FRTCellId> Path;
	Path.Add(Start);
	FRTCellId Cur = Start;
	for (const FRTCellId& WP : Waypoints)
	{
		if (WP == Cur)
		{
			continue; // waypoint sulla cella corrente: nessun tratto
		}
		const TArray<FRTCellId> Seg = FindPathByGraph(Cur, WP, CellCost, Edges, Width, Height);
		if (Seg.Num() < 2 || Seg[0] != Cur)
		{
			return TArray<FRTCellId>(); // tratto irraggiungibile -> percorso invalido
		}
		for (int32 i = 1; i < Seg.Num(); ++i)
		{
			Path.Add(Seg[i]); // salta Seg[0] (= Cur, giunzione gia' presente)
		}
		Cur = WP;
	}
	return Path;
}

int32 URTGridLibrary::PathCost(const TArray<FRTCellId>& Path, const TMap<FRTCellId, int32>& CellCost,
	const TArray<FRTTraversalEdge>& Edges)
{
	if (Path.Num() <= 1)
	{
		return 0; // fermo
	}
	int32 Total = 0;
	for (int32 i = 1; i < Path.Num(); ++i)
	{
		const FRTCellId& Prev = Path[i - 1];
		const FRTCellId& Cur = Path[i];

		// Passo ortogonale (stesso layer): costo = costo della cella.
		if (Prev.Layer == Cur.Layer && ManhattanDistance(Prev, Cur) == 1)
		{
			const int32* Found = CellCost.Find(Cur);
			const int32 Enter = Found ? *Found : 1;
			if (Enter < 0) { return -1; } // cella impassabile
			Total += Enter;
			continue;
		}

		// Altrimenti dev'essere un arco Prev->Cur.
		bool bEdge = false;
		for (const FRTTraversalEdge& E : Edges)
		{
			if (E.From == Prev && E.To == Cur)
			{
				Total += FMath::Max(0, E.Cost);
				bEdge = true;
				break;
			}
		}
		if (!bEdge) { return -1; } // ne' adiacente ne' collegato da un arco
	}
	return Total;
}

TArray<FRTCellId> URTGridLibrary::CellsInCone(const FRTCellId& From, const FRTCellId& Target, int32 Range)
{
	TArray<FRTCellId> Result;

	const int32 DX = Target.X - From.X;
	const int32 DY = Target.Y - From.Y;
	const bool bAxisX = FMath::Abs(DX) >= FMath::Abs(DY);
	const int32 Dir = bAxisX ? FMath::Sign(DX) : FMath::Sign(DY);
	if (Dir == 0)
	{
		return Result; // bersaglio sull'origine: nessuna direzione
	}

	// A profondità d lungo l'asse dominante, il cono si allarga di d celle per lato.
	for (int32 D = 1; D <= Range; ++D)
	{
		for (int32 Lateral = -D; Lateral <= D; ++Lateral)
		{
			const FRTCellId Cell = bAxisX
				? FRTCellId(From.X + Dir * D, From.Y + Lateral)
				: FRTCellId(From.X + Lateral, From.Y + Dir * D);
			Result.Add(Cell);
		}
	}
	return Result;
}
