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

FRTGridCoord URTBotLibrary::StepAway(const FRTGridCoord& From, const FRTGridCoord& Threat, int32 MoveRange, int32 Width, int32 Height)
{
	int32 Budget = FMath::Max(0, MoveRange);
	int32 AwayX = FMath::Sign(From.X - Threat.X);
	int32 AwayY = FMath::Sign(From.Y - Threat.Y);
	if (AwayX == 0 && AwayY == 0)
	{
		AwayX = 1; // stessa cella: ritirata deterministica lungo +X
	}

	FRTGridCoord Result = From;
	if (AwayX != 0 && AwayY != 0)
	{
		// Ritirata diagonale: divide il budget fra i due assi.
		const int32 StepX = (Budget + 1) / 2;
		const int32 StepY = Budget - StepX;
		Result.X += AwayX * StepX;
		Result.Y += AwayY * StepY;
	}
	else if (AwayX != 0)
	{
		Result.X += AwayX * Budget;
	}
	else
	{
		Result.Y += AwayY * Budget;
	}

	// Resta dentro la griglia.
	Result.X = FMath::Clamp(Result.X, 0, FMath::Max(0, Width - 1));
	Result.Y = FMath::Clamp(Result.Y, 0, FMath::Max(0, Height - 1));
	return Result;
}

int32 URTBotLibrary::AttackScore(int32 Damage, int32 TargetHealth)
{
	if (Damage >= TargetHealth)
	{
		// Kill: priorità massima; tra i kill si preferisce il bersaglio più debole.
		return 100000 - TargetHealth;
	}
	// Non-kill: si preferisce il danno maggiore.
	return Damage;
}
