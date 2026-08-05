#include "Turn/RTHexSimLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPathLibrary.h"

namespace
{
	const FRTHexSimUnit* FindUnit(const FRTHexSnapshot& Snapshot, int32 UnitId)
	{
		return Snapshot.Units.FindByPredicate([UnitId](const FRTHexSimUnit& U) { return U.UnitId == UnitId; });
	}

	/** Celle occupate da unita' vive DIVERSE da ForUnitId: ostacoli dinamici (non appartengono all'asset mappa). */
	TSet<FRTCellId> BlockedCellsFor(const FRTHexSnapshot& Snapshot, int32 ForUnitId)
	{
		TSet<FRTCellId> Out;
		for (const TPair<FRTCellId, int32>& Entry : Snapshot.Occupancy)
		{
			if (Entry.Value != ForUnitId)
			{
				Out.Add(Entry.Key);
			}
		}
		return Out;
	}
}

FRTHexSnapshot URTHexSimLibrary::MakeSnapshot(const URTHexMapAsset* Map, const TArray<FRTHexSimUnit>& Units)
{
	FRTHexSnapshot Snapshot;
	Snapshot.Map = Map;
	if (Map)
	{
		Snapshot.MapHash = Map->ComputeHash();
		Snapshot.Revision = Map->Revision;
	}

	// Ordine stabile: UnitId, poi cella (l'ordine dell'input non deve influenzare lo snapshot).
	Snapshot.Units = Units;
	Snapshot.Units.Sort([](const FRTHexSimUnit& A, const FRTHexSimUnit& B)
	{
		return A.UnitId != B.UnitId ? A.UnitId < B.UnitId : URTHexLibrary::StableLess(A.Cell, B.Cell);
	});

	// Occupazione delle sole unita' vive; a parita' di cella vince l'UnitId minore (le sovrapposizioni sono un
	// errore strutturale, segnalato da ValidateSnapshot: qui serve solo un esito deterministico).
	for (const FRTHexSimUnit& Unit : Snapshot.Units)
	{
		if (Unit.bAlive && !Snapshot.Occupancy.Contains(Unit.Cell))
		{
			Snapshot.Occupancy.Add(Unit.Cell, Unit.UnitId);
		}
	}
	return Snapshot;
}

TArray<FString> URTHexSimLibrary::ValidateSnapshot(const FRTHexSnapshot& Snapshot)
{
	TArray<FString> Errors;
	if (!Snapshot.Map)
	{
		Errors.Add(TEXT("Snapshot senza mappa."));
		return Errors;
	}

	TSet<int32> SeenIds;
	TMap<FRTCellId, int32> CellOwner;
	for (const FRTHexSimUnit& Unit : Snapshot.Units)
	{
		if (SeenIds.Contains(Unit.UnitId))
		{
			Errors.Add(FString::Printf(TEXT("UnitId duplicato: %d."), Unit.UnitId));
		}
		SeenIds.Add(Unit.UnitId);

		if (Unit.MoveBudget < 0)
		{
			Errors.Add(FString::Printf(TEXT("Unita' %d: MoveBudget negativo (%d)."), Unit.UnitId, Unit.MoveBudget));
		}

		if (!Unit.bAlive)
		{
			continue; // le unita' eliminate non occupano e non devono stare sulla mappa
		}

		if (!Snapshot.Map->ContainsCell(Unit.Cell))
		{
			Errors.Add(FString::Printf(TEXT("Unita' %d su cella assente dalla mappa: %s."),
				Unit.UnitId, *Unit.Cell.ToString()));
		}

		if (const int32* Other = CellOwner.Find(Unit.Cell))
		{
			Errors.Add(FString::Printf(TEXT("Celle sovrapposte: unita' %d e %d su %s."),
				*Other, Unit.UnitId, *Unit.Cell.ToString()));
		}
		else
		{
			CellOwner.Add(Unit.Cell, Unit.UnitId);
		}
	}
	return Errors;
}

bool URTHexSimLibrary::IsSnapshotStale(const FRTHexSnapshot& Snapshot)
{
	if (!Snapshot.Map)
	{
		return true;
	}
	return Snapshot.Map->ComputeHash() != Snapshot.MapHash || Snapshot.Map->Revision != Snapshot.Revision;
}

bool URTHexSimLibrary::IsCellFree(const FRTHexSnapshot& Snapshot, const FRTCellId& Cell, int32 ForUnitId)
{
	if (!Snapshot.Map)
	{
		return false;
	}
	const FRTHexCellData* Data = Snapshot.Map->FindCell(Cell);
	if (!Data || Data->bBlocksMovement)
	{
		return false;
	}
	const int32* Occupant = Snapshot.Occupancy.Find(Cell);
	return Occupant == nullptr || *Occupant == ForUnitId;
}

TArray<FRTHexReachableCell> URTHexSimLibrary::ReachableCells(const FRTHexSnapshot& Snapshot, int32 UnitId)
{
	TArray<FRTHexReachableCell> Out;

	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (!Snapshot.Map || !Unit || !Snapshot.Map->ContainsCell(Unit->Cell))
	{
		return Out;
	}

	const TSet<FRTCellId> Blocked = BlockedCellsFor(Snapshot, UnitId);
	const int32 Budget = FMath::Max(0, Unit->MoveBudget);

	// Dijkstra su costi interi: estrazione del minimo con tie-break sull'ID (nessuna dipendenza dall'ordine di TMap).
	TMap<FRTCellId, int32> Dist;
	TArray<FRTCellId> Frontier;
	TSet<FRTCellId> Closed;

	Dist.Add(Unit->Cell, 0);
	Frontier.Add(Unit->Cell);

	while (Frontier.Num() > 0)
	{
		int32 BestIdx = 0;
		for (int32 I = 1; I < Frontier.Num(); ++I)
		{
			const int32 D = Dist[Frontier[I]];
			const int32 BestD = Dist[Frontier[BestIdx]];
			if (D < BestD || (D == BestD && URTHexLibrary::StableLess(Frontier[I], Frontier[BestIdx])))
			{
				BestIdx = I;
			}
		}

		const FRTCellId Current = Frontier[BestIdx];
		Frontier.RemoveAt(BestIdx);
		if (Closed.Contains(Current))
		{
			continue; // gia' finalizzata (duplicato nella frontiera)
		}
		Closed.Add(Current);

		for (const TPair<FRTCellId, int32>& Step : URTHexPathLibrary::GraphNeighbors(Snapshot.Map, Current))
		{
			if (Blocked.Contains(Step.Key))
			{
				continue; // occupata da un'altra unita'
			}
			const int32 Tentative = Dist[Current] + Step.Value;
			if (Tentative > Budget)
			{
				continue; // fuori dal budget di movimento
			}
			const int32* Existing = Dist.Find(Step.Key);
			if (!Existing || Tentative < *Existing)
			{
				Dist.Add(Step.Key, Tentative);
				Frontier.Add(Step.Key);
			}
		}
	}

	Out.Reserve(Dist.Num());
	for (const TPair<FRTCellId, int32>& Entry : Dist)
	{
		Out.Add(FRTHexReachableCell(Entry.Key, Entry.Value));
	}
	Out.Sort([](const FRTHexReachableCell& A, const FRTHexReachableCell& B)
	{
		return URTHexLibrary::StableLess(A.Cell, B.Cell);
	});
	return Out;
}

FRTHexPathResult URTHexSimLibrary::FindPathForUnit(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTCellId& Goal)
{
	FRTHexPathResult Result;

	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (!Snapshot.Map || !Unit)
	{
		Result.Status = ERTHexPathStatus::StartInvalid;
		return Result;
	}

	const int32 Budget = FMath::Max(0, Unit->MoveBudget);
	if (Budget <= 0)
	{
		// MaxCost == 0 significa "illimitato" per l'A*: il caso "immobile" va gestito qui.
		if (Goal == Unit->Cell && Snapshot.Map->ContainsCell(Goal))
		{
			Result.Status = ERTHexPathStatus::Success;
			Result.Path.Add(Goal);
		}
		else
		{
			Result.Status = Snapshot.Map->ContainsCell(Goal) ? ERTHexPathStatus::NoPath : ERTHexPathStatus::GoalInvalid;
		}
		return Result;
	}

	const TSet<FRTCellId> Blocked = BlockedCellsFor(Snapshot, UnitId);
	return URTHexPathLibrary::FindPathAvoiding(Snapshot.Map, Unit->Cell, Goal, &Blocked, Budget);
}

TArray<FRTHexMoveResult> URTHexSimLibrary::ResolveHexPaths(const TArray<TArray<FRTCellId>>& Paths)
{
	const int32 N = Paths.Num();
	TArray<FRTHexMoveResult> Results;
	Results.SetNum(N);

	TArray<FRTCellId> Pos;  Pos.SetNum(N);   // posizione corrente
	TArray<int32> Prog;     Prog.SetNum(N);  // indice raggiunto nel path
	TArray<bool> Done;      Done.SetNum(N);  // path esaurito / nessun movimento
	for (int32 i = 0; i < N; ++i)
	{
		Pos[i] = Paths[i].Num() > 0 ? Paths[i][0] : FRTCellId();
		Prog[i] = 0;
		Done[i] = Paths[i].Num() <= 1;
		Results[i].Final = Pos[i];
	}

	// Motivo dell'ultimo congelamento per unita' (reason code del TurnLog).
	TArray<ERTMoveOutcome> BlockReason; BlockReason.Init(ERTMoveOutcome::BlockedByUnit, N);

	// Microstep sincroni: tutti avanzano di una cella, si risolvono le collisioni, si ripete.
	while (true)
	{
		TArray<FRTCellId> Target; Target.SetNum(N);
		TArray<bool> Moving;      Moving.SetNum(N);
		for (int32 i = 0; i < N; ++i)
		{
			Moving[i] = !Done[i];
			Target[i] = Done[i] ? Pos[i] : Paths[i][Prog[i] + 1];
		}

		// Punto fisso del microstep: si puo' solo passare da "in movimento" a "fermo" (monotono) -> l'esito
		// non dipende dall'ordine delle richieste.
		bool bChanged = true;
		while (bChanged)
		{
			bChanged = false;
			TArray<int32> ToFreeze;
			for (int32 i = 0; i < N; ++i)
			{
				if (!Moving[i])
				{
					continue;
				}

				// Destinazione contesa: 2+ unita' in movimento verso la stessa cella.
				int32 Contenders = 0;
				for (int32 j = 0; j < N; ++j)
				{
					if (Moving[j] && Target[j] == Target[i])
					{
						++Contenders;
					}
				}
				bool bBlocked = (Contenders >= 2);

				// Bloccata da un'unita' che RESTA (esaurita o congelata) sulla cella di destinazione.
				if (!bBlocked)
				{
					for (int32 j = 0; j < N; ++j)
					{
						if (j != i && !Moving[j] && Pos[j] == Target[i])
						{
							bBlocked = true;
							break;
						}
					}
				}
				if (bBlocked)
				{
					ToFreeze.Add(i);
					BlockReason[i] = (Contenders >= 2) ? ERTMoveOutcome::BlockedContested : ERTMoveOutcome::BlockedByUnit;
				}
			}
			for (int32 Idx : ToFreeze)
			{
				Moving[Idx] = false;
				bChanged = true;
			}
		}

		// Applica i movimenti del microstep.
		bool bAnyMoved = false;
		for (int32 i = 0; i < N; ++i)
		{
			if (!Moving[i])
			{
				continue;
			}
			Pos[i] = Target[i];
			Results[i].Entered.Add(Target[i]);
			Results[i].Final = Target[i];
			++Prog[i];
			if (Prog[i] >= Paths[i].Num() - 1)
			{
				Done[i] = true;
			}
			bAnyMoved = true;
		}
		if (!bAnyMoved)
		{
			break;
		}
	}

	// Reason code finale: dipende solo da Final/Paths -> indipendente dall'ordine.
	for (int32 i = 0; i < N; ++i)
	{
		if (Paths[i].Num() <= 1)
		{
			Results[i].Outcome = ERTMoveOutcome::Stayed;
		}
		else if (Results[i].Final == Paths[i].Last())
		{
			Results[i].Outcome = ERTMoveOutcome::Moved;
		}
		else
		{
			Results[i].Outcome = BlockReason[i];
		}
	}

	return Results;
}

FRTGridCoord URTHexSimLibrary::ToLogCoord(const FRTCellId& Cell)
{
	// I tre interi restano q, r e Layer: la topologia e' dichiarata nel formato (ERTLogTopology::Hex), non qui.
	FRTGridCoord Out;
	Out.X = Cell.X;
	Out.Y = Cell.Y;
	Out.Layer = Cell.Layer;
	return Out;
}

TArray<FRTTurnLogEntry> URTHexSimLibrary::BuildMoveLog(const TArray<TArray<FRTCellId>>& Paths,
	const TArray<FRTHexMoveResult>& Results)
{
	TArray<FRTTurnLogEntry> Log;
	const int32 N = FMath::Min(Paths.Num(), Results.Num());
	Log.Reserve(N);

	for (int32 i = 0; i < N; ++i)
	{
		// Chiave stabile dell'unita' nel turno: la sua cella di PARTENZA (max 1 unita' per cella), mai un pointer.
		const FRTCellId Start = Paths[i].Num() > 0 ? Paths[i][0] : Results[i].Final;

		FRTTurnLogEntry Entry;
		Entry.Phase = ERTMatchPhase::Move;
		Entry.Category = ERTLogCategory::Move;
		Entry.Outcome = static_cast<uint8>(Results[i].Outcome);
		Entry.SrcCell = ToLogCoord(Start);
		Entry.TgtCell = ToLogCoord(Results[i].Final);
		Entry.Amount = Results[i].Entered.Num(); // celle effettivamente percorse
		Log.Add(Entry);
	}
	return Log;
}
