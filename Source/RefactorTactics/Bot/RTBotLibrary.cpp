#include "Bot/RTBotLibrary.h"

FRTGridCoord URTBotLibrary::StepToward(const FRTGridCoord& From, const FRTGridCoord& Target, int32 MoveRange)
{
	const int32 Distance = FMath::Abs(Target.X - From.X) + FMath::Abs(Target.Y - From.Y);
	if (Distance <= 1)
	{
		return From; // gia' adiacente o sulla stessa cella: non muoversi
	}

	// Budget di passi, lasciando almeno una cella tra bot e bersaglio (non sovrapporsi).
	int32 Budget = FMath::Min(MoveRange, Distance - 1);

	FRTGridCoord Result = From;

	// Avvicinamento greedy: prima lungo X, poi lungo Y.
	const int32 DX = Target.X - Result.X;
	const int32 StepX = FMath::Min(Budget, FMath::Abs(DX));
	Result.X += FMath::Sign(DX) * StepX;
	Budget -= StepX;

	const int32 DY = Target.Y - Result.Y;
	const int32 StepY = FMath::Min(Budget, FMath::Abs(DY));
	Result.Y += FMath::Sign(DY) * StepY;

	return Result;
}
