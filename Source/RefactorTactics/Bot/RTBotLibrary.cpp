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

FRTGridCoord URTBotLibrary::BestApproachCell(const FRTGridCoord& From, const FRTGridCoord& Target, int32 MoveRange,
	const TArray<FRTGridCoord>& Blockers, int32 Width, int32 Height)
{
	const int32 Budget = FMath::Max(0, MoveRange);

	FRTGridCoord Best = From; // fermarsi e' sempre un'opzione valida
	int32 BestToTarget = FMath::Abs(Target.X - From.X) + FMath::Abs(Target.Y - From.Y);
	int32 BestFromOrigin = 0;

	// Scansione deterministica del rombo raggiungibile (X poi Y crescenti).
	for (int32 X = 0; X < Width; ++X)
	{
		for (int32 Y = 0; Y < Height; ++Y)
		{
			const FRTGridCoord Cell(X, Y);
			const int32 FromOrigin = FMath::Abs(X - From.X) + FMath::Abs(Y - From.Y);
			if (FromOrigin > Budget || Cell == Target || Blockers.Contains(Cell))
			{
				continue; // fuori portata, sul bersaglio, o su una copertura
			}
			const int32 ToTarget = FMath::Abs(Target.X - X) + FMath::Abs(Target.Y - Y);
			// Piu' vicino al bersaglio; a parita', mossa piu' corta.
			if (ToTarget < BestToTarget || (ToTarget == BestToTarget && FromOrigin < BestFromOrigin))
			{
				BestToTarget = ToTarget;
				BestFromOrigin = FromOrigin;
				Best = Cell;
			}
		}
	}
	return Best;
}

FRTGridCoord URTBotLibrary::BestKiteCell(const FRTGridCoord& From, const FRTGridCoord& Threat, int32 MoveRange,
	const TArray<FRTGridCoord>& Blockers, int32 Width, int32 Height)
{
	const int32 Budget = FMath::Max(0, MoveRange);

	FRTGridCoord Best = From;
	int32 BestToThreat = FMath::Abs(Threat.X - From.X) + FMath::Abs(Threat.Y - From.Y);
	int32 BestFromOrigin = 0;

	for (int32 X = 0; X < Width; ++X)
	{
		for (int32 Y = 0; Y < Height; ++Y)
		{
			const FRTGridCoord Cell(X, Y);
			const int32 FromOrigin = FMath::Abs(X - From.X) + FMath::Abs(Y - From.Y);
			if (FromOrigin > Budget || Blockers.Contains(Cell))
			{
				continue; // fuori portata o su una copertura
			}
			const int32 ToThreat = FMath::Abs(Threat.X - X) + FMath::Abs(Threat.Y - Y);
			// Massimizza la distanza dalla minaccia; a parita', mossa piu' corta.
			if (ToThreat > BestToThreat || (ToThreat == BestToThreat && FromOrigin < BestFromOrigin))
			{
				BestToThreat = ToThreat;
				BestFromOrigin = FromOrigin;
				Best = Cell;
			}
		}
	}
	return Best;
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
