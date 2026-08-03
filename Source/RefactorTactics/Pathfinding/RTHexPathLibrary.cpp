#include "Pathfinding/RTHexPathLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Algo/Reverse.h"

TArray<TPair<FRTCellId, int32>> URTHexPathLibrary::GraphNeighbors(const URTHexMapAsset* Map, const FRTCellId& Cell)
{
	TArray<TPair<FRTCellId, int32>> Out;
	if (!Map)
	{
		return Out;
	}

	// Vicini orizzontali (stesso layer): presenti e non bloccati; costo = MoveCost della destinazione.
	for (const FRTCellId& N : URTHexLibrary::Neighbors(Cell))
	{
		if (const FRTHexCellData* D = Map->FindCell(N))
		{
			if (!D->bBlocksMovement)
			{
				Out.Add(TPair<FRTCellId, int32>(N, FMath::Max(0, D->MoveCost)));
			}
		}
	}

	// Transizioni esplicite uscenti (rampe/ponti/scale/tunnel): costo = costo dell'arco.
	for (const FRTHexEdge& E : Map->Transitions)
	{
		if (E.From == Cell)
		{
			if (const FRTHexCellData* D = Map->FindCell(E.To))
			{
				if (!D->bBlocksMovement)
				{
					Out.Add(TPair<FRTCellId, int32>(E.To, FMath::Max(0, E.Cost)));
				}
			}
		}
	}
	return Out;
}

FRTHexPathResult URTHexPathLibrary::FindPath(const URTHexMapAsset* Map, const FRTCellId& Start, const FRTCellId& Goal,
	int32 MaxCost, int32 MaxNodes)
{
	FRTHexPathResult Result;

	if (!Map || !Map->ContainsCell(Start))
	{
		Result.Status = ERTHexPathStatus::StartInvalid;
		return Result;
	}
	if (!Map->ContainsCell(Goal))
	{
		Result.Status = ERTHexPathStatus::GoalInvalid;
		return Result;
	}
	if (Start == Goal)
	{
		Result.Status = ERTHexPathStatus::Success;
		Result.Path.Add(Start);
		return Result;
	}

	// Euristica ammissibile: distanza esagonale (planare) verso il goal.
	auto Heuristic = [&Goal](const FRTCellId& C) { return URTHexLibrary::HexDistance(C, Goal); };

	TMap<FRTCellId, int32> GScore;
	TMap<FRTCellId, FRTCellId> CameFrom;
	TSet<FRTCellId> Closed;
	TArray<FRTCellId> Open;

	GScore.Add(Start, 0);
	Open.Add(Start);

	while (Open.Num() > 0)
	{
		// Estrai il nodo con f minimo; tie-break deterministico: g minore, poi ID stabile (no ordine TSet/TMap).
		int32 BestIdx = 0;
		int32 BestF = GScore[Open[0]] + Heuristic(Open[0]);
		int32 BestG = GScore[Open[0]];
		for (int32 I = 1; I < Open.Num(); ++I)
		{
			const int32 G = GScore[Open[I]];
			const int32 F = G + Heuristic(Open[I]);
			const bool bBetter =
				(F < BestF) ||
				(F == BestF && G < BestG) ||
				(F == BestF && G == BestG && URTHexLibrary::StableLess(Open[I], Open[BestIdx]));
			if (bBetter)
			{
				BestIdx = I; BestF = F; BestG = G;
			}
		}

		const FRTCellId Current = Open[BestIdx];
		Open.RemoveAt(BestIdx);
		if (Closed.Contains(Current))
		{
			continue; // gia' finalizzato (duplicato nell'open)
		}
		Closed.Add(Current);
		++Result.NodesVisited;

		if (Current == Goal)
		{
			// Ricostruisci il percorso (goal -> start) e invertilo.
			TArray<FRTCellId> Rev;
			FRTCellId Node = Goal;
			Rev.Add(Node);
			while (const FRTCellId* Prev = CameFrom.Find(Node))
			{
				Node = *Prev;
				Rev.Add(Node);
			}
			Algo::Reverse(Rev);
			Result.Path = MoveTemp(Rev);
			Result.TotalCost = GScore[Goal];
			Result.Status = ERTHexPathStatus::Success;
			return Result;
		}

		if (Result.NodesVisited > MaxNodes)
		{
			Result.Status = ERTHexPathStatus::NodeLimit;
			return Result;
		}

		for (const TPair<FRTCellId, int32>& Step : GraphNeighbors(Map, Current))
		{
			const int32 Tentative = GScore[Current] + Step.Value;
			if (MaxCost > 0 && Tentative > MaxCost)
			{
				continue; // oltre il budget di movimento
			}
			const int32* Existing = GScore.Find(Step.Key);
			if (!Existing || Tentative < *Existing)
			{
				GScore.Add(Step.Key, Tentative);
				CameFrom.Add(Step.Key, Current);
				Open.Add(Step.Key); // puo' duplicare; il set Closed evita ri-espansioni
			}
		}
	}

	Result.Status = ERTHexPathStatus::NoPath;
	return Result;
}
