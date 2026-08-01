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
