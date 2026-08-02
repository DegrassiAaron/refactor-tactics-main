#include "Grid/RTGridLibrary.h"

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
