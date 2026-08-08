#include "Pathfinding/RTHexPathLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Algo/Reverse.h"

TArray<TPair<FRTCellId, int32>> URTHexPathLibrary::GraphNeighbors(const URTHexMapAsset* Map, const FRTCellId& Cell)
{
	TArray<TPair<FRTCellId, int32>> Out;
	if (!Map)
	{
		return Out;
	}

	// Vicini orizzontali (stesso layer): presenti, non bloccati e non separati da una barriera; costo =
	// MoveCost della destinazione. La copertura ALTA (CP 9.2) nega il PASSO fra le due celle senza toglierne
	// nessuna dalla mappa: restano entrambe occupabili, semplicemente non si passa da una all'altra.
	for (const FRTCellId& N : URTHexLibrary::Neighbors(Cell))
	{
		if (const FRTHexCellData* D = Map->FindCell(N))
		{
			if (!D->bBlocksMovement && !URTHexCoverLibrary::BlocksTraversal(Map, Cell, N))
			{
				Out.Add(TPair<FRTCellId, int32>(N, FMath::Max(0, D->MoveCost)));
			}
		}
	}

	// Transizioni esplicite uscenti (rampe/ponti/scale/tunnel): costo = costo dell'arco. Un arco non ATTIVO
	// (CP 9.4) non e' un collegamento: le due celle tornano irraggiungibili l'una dall'altra, e il percorso
	// FALLISCE invece di teletrasportare — non esiste un'adiacenza planare di riserva fra due layer.
	for (const FRTHexEdge& E : Map->Transitions)
	{
		if (E.From == Cell && E.State == ERTHexArcState::Active)
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
	return FindPathAvoiding(Map, Start, Goal, nullptr, MaxCost, MaxNodes);
}

FRTHexPathResult URTHexPathLibrary::FindPathAvoiding(const URTHexMapAsset* Map, const FRTCellId& Start,
	const FRTCellId& Goal, const TSet<FRTCellId>* Blocked, int32 MaxCost, int32 MaxNodes, int32 ExtraCostPerCell)
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
	// Ostacolo dinamico sul goal: la cella esiste ma e' occupata -> nessun percorso (non e' un goal invalido).
	if (Blocked && Blocked->Contains(Goal) && Goal != Start)
	{
		Result.Status = ERTHexPathStatus::NoPath;
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
			if (Blocked && Blocked->Contains(Step.Key))
			{
				continue; // cella occupata da un'altra unita' (ostacolo dinamico)
			}
			const int32 Tentative = GScore[Current] + Step.Value + FMath::Max(0, ExtraCostPerCell);
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
