#include "Grid/RTGridLibrary.h"
#include "Algo/Reverse.h"

FVector URTGridLibrary::CellToWorld(const FRTGridCoord& Cell, const FVector& Origin, float CellSize)
{
	// Centro della cella: (indice + 0.5) * dimensione, a partire dall'origine. Z invariato.
	return FVector(
		Origin.X + (static_cast<double>(Cell.X) + 0.5) * CellSize,
		Origin.Y + (static_cast<double>(Cell.Y) + 0.5) * CellSize,
		Origin.Z);
}

FRTGridCoord URTGridLibrary::WorldToCell(const FVector& World, const FVector& Origin, float CellSize)
{
	// Floor della posizione locale in unita' di cella: ogni punto della cella mappa allo stesso indice.
	const double LocalX = (World.X - Origin.X) / CellSize;
	const double LocalY = (World.Y - Origin.Y) / CellSize;
	return FRTGridCoord(FMath::FloorToInt32(LocalX), FMath::FloorToInt32(LocalY));
}

bool URTGridLibrary::IsInsideGrid(const FRTGridCoord& Cell, int32 Width, int32 Height)
{
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Width && Cell.Y < Height;
}

int32 URTGridLibrary::ManhattanDistance(const FRTGridCoord& A, const FRTGridCoord& B)
{
	return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

bool URTGridLibrary::IsWithinRange(const FRTGridCoord& From, const FRTGridCoord& To, int32 Range)
{
	return ManhattanDistance(From, To) <= Range;
}

bool URTGridLibrary::HasLineOfSight(const FRTGridCoord& From, const FRTGridCoord& To, const TArray<FRTGridCoord>& Blockers)
{
	if (Blockers.Num() == 0 || From == To)
	{
		return true;
	}

	// Campiona il segmento tra i centri delle due celle e controlla ogni cella attraversata.
	// From e To non contano come ostacoli.
	const int32 DX = To.X - From.X;
	const int32 DY = To.Y - From.Y;
	const int32 Steps = FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) * 4; // risoluzione sufficiente per la griglia

	for (int32 i = 1; i < Steps; ++i)
	{
		const double T = static_cast<double>(i) / Steps;
		const FRTGridCoord Cell(
			FMath::RoundToInt32(From.X + DX * T),
			FMath::RoundToInt32(From.Y + DY * T));

		if (Cell != From && Cell != To && Blockers.Contains(Cell))
		{
			return false;
		}
	}
	return true;
}

TArray<FRTGridCoord> URTGridLibrary::CellsInRadius(const FRTGridCoord& Center, int32 Radius)
{
	TArray<FRTGridCoord> Result;
	const int32 R = FMath::Max(0, Radius);
	for (int32 DY = -R; DY <= R; ++DY)
	{
		for (int32 DX = -R; DX <= R; ++DX)
		{
			if (FMath::Abs(DX) + FMath::Abs(DY) <= R)
			{
				Result.Add(FRTGridCoord(Center.X + DX, Center.Y + DY));
			}
		}
	}
	return Result;
}

TArray<FRTGridCoord> URTGridLibrary::CellsInLine(const FRTGridCoord& From, const FRTGridCoord& To)
{
	TArray<FRTGridCoord> Result;
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
		const FRTGridCoord Cell(
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

	FORCEINLINE bool IsTraversable(const FRTGridCoord& Cell, const TArray<FRTGridCoord>& Blockers, int32 Width, int32 Height)
	{
		return URTGridLibrary::IsInsideGrid(Cell, Width, Height) && !Blockers.Contains(Cell);
	}
}

TArray<FRTGridCoord> URTGridLibrary::ReachableCells(const FRTGridCoord& From, int32 MoveRange,
	const TArray<FRTGridCoord>& Blockers, int32 Width, int32 Height)
{
	TArray<FRTGridCoord> Result;
	if (!IsTraversable(From, Blockers, Width, Height))
	{
		return Result; // origine fuori griglia o bloccata: nessuna cella
	}

	// BFS a costo uniforme: la distanza in passi = numero minimo di mosse ortogonali.
	TMap<FRTGridCoord, int32> Distance;
	TArray<FRTGridCoord> Frontier;
	Distance.Add(From, 0);
	Frontier.Add(From);
	Result.Add(From); // From è sempre "raggiungibile" (fermarsi è lecito)

	const int32 Budget = FMath::Max(0, MoveRange);
	int32 Head = 0;
	while (Head < Frontier.Num())
	{
		const FRTGridCoord Current = Frontier[Head++];
		const int32 Dist = Distance[Current];
		if (Dist >= Budget)
		{
			continue; // esaurito il budget di passi da questa cella
		}
		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			const FRTGridCoord Next(Current.X + GStepDX[Dir], Current.Y + GStepDY[Dir]);
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

TArray<FRTGridCoord> URTGridLibrary::FindPath(const FRTGridCoord& From, const FRTGridCoord& To,
	const TArray<FRTGridCoord>& Blockers, int32 Width, int32 Height)
{
	TArray<FRTGridCoord> Path;
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
	TMap<FRTGridCoord, FRTGridCoord> Parent;
	TArray<FRTGridCoord> Frontier;
	Parent.Add(From, From); // il predecessore di From è se stesso (sentinella)
	Frontier.Add(From);

	int32 Head = 0;
	bool bFound = false;
	while (Head < Frontier.Num() && !bFound)
	{
		const FRTGridCoord Current = Frontier[Head++];
		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			const FRTGridCoord Next(Current.X + GStepDX[Dir], Current.Y + GStepDY[Dir]);
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
	for (FRTGridCoord Cell = To; ; Cell = Parent[Cell])
	{
		Path.Add(Cell);
		if (Cell == From) { break; }
	}
	Algo::Reverse(Path);
	return Path;
}

TArray<FRTGridCoord> URTGridLibrary::CellsInCone(const FRTGridCoord& From, const FRTGridCoord& Target, int32 Range)
{
	TArray<FRTGridCoord> Result;

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
			const FRTGridCoord Cell = bAxisX
				? FRTGridCoord(From.X + Dir * D, From.Y + Lateral)
				: FRTGridCoord(From.X + Lateral, From.Y + Dir * D);
			Result.Add(Cell);
		}
	}
	return Result;
}
