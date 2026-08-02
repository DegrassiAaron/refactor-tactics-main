#include "Bot/RTBotLibrary.h"
#include "Grid/RTGridLibrary.h"

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
	const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height)
{
	// Considera solo celle raggiungibili col budget di costo (aggira ostacoli e terreno costoso/hazard).
	const TArray<FRTGridCoord> Reachable = URTGridLibrary::ReachableCellsByCost(From, MoveRange, CellCost, Width, Height);

	FRTGridCoord Best = From; // fermarsi e' sempre un'opzione valida
	int32 BestToTarget = FMath::Abs(Target.X - From.X) + FMath::Abs(Target.Y - From.Y);
	int32 BestFromOrigin = 0;

	for (const FRTGridCoord& Cell : Reachable)
	{
		if (Cell == Target)
		{
			continue; // non sovrapporsi al bersaglio
		}
		const int32 ToTarget = FMath::Abs(Target.X - Cell.X) + FMath::Abs(Target.Y - Cell.Y);
		const int32 FromOrigin = FMath::Abs(Cell.X - From.X) + FMath::Abs(Cell.Y - From.Y);
		// Piu' vicino al bersaglio; a parita', mossa piu' corta.
		if (ToTarget < BestToTarget || (ToTarget == BestToTarget && FromOrigin < BestFromOrigin))
		{
			BestToTarget = ToTarget;
			BestFromOrigin = FromOrigin;
			Best = Cell;
		}
	}
	return Best;
}

FRTGridCoord URTBotLibrary::BestKiteCell(const FRTGridCoord& From, const FRTGridCoord& Threat, int32 MoveRange,
	const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height)
{
	// Solo celle raggiungibili col budget di costo (aggira ostacoli e terreno pericoloso).
	const TArray<FRTGridCoord> Reachable = URTGridLibrary::ReachableCellsByCost(From, MoveRange, CellCost, Width, Height);

	FRTGridCoord Best = From;
	int32 BestToThreat = FMath::Abs(Threat.X - From.X) + FMath::Abs(Threat.Y - From.Y);
	int32 BestFromOrigin = 0;

	for (const FRTGridCoord& Cell : Reachable)
	{
		const int32 ToThreat = FMath::Abs(Threat.X - Cell.X) + FMath::Abs(Threat.Y - Cell.Y);
		const int32 FromOrigin = FMath::Abs(Cell.X - From.X) + FMath::Abs(Cell.Y - From.Y);
		// Massimizza la distanza dalla minaccia; a parita', mossa piu' corta.
		if (ToThreat > BestToThreat || (ToThreat == BestToThreat && FromOrigin < BestFromOrigin))
		{
			BestToThreat = ToThreat;
			BestFromOrigin = FromOrigin;
			Best = Cell;
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
