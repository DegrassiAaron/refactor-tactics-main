#include "Turn/RTHexSimLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPathLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

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

TArray<FRTCellId> URTHexSimLibrary::LinearDashPath(const FRTHexSnapshot& Snapshot, int32 UnitId,
	const FRTCellId& Goal)
{
	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (!Snapshot.Map || !Unit || Goal == Unit->Cell)
	{
		return {};
	}

	// Lo scatto e' lineare sul PIANO: un layer diverso non e' mai "in linea", quindi non si sale per una
	// transizione (a differenza del movimento normale, che passa dal grafo).
	const FRTCellId Start = Unit->Cell;
	if (Goal.Layer != Start.Layer)
	{
		return {};
	}

	// La destinazione deve stare su una delle sei direzioni: delta = k * direzione, con k > 0 intero.
	const int32 DeltaQ = Goal.X - Start.X;
	const int32 DeltaR = Goal.Y - Start.Y;
	int32 Steps = 0;
	FIntPoint Step(0, 0);
	for (int32 D = 0; D < 6; ++D)
	{
		const FIntPoint Dir = URTHexLibrary::AxialDirection(static_cast<ERTHexDirection>(D));
		// k si ricava dalla componente non nulla della direzione; l'altra deve tornare con lo stesso k.
		const int32 K = (Dir.X != 0) ? (DeltaQ / Dir.X) : (Dir.Y != 0 ? (DeltaR / Dir.Y) : 0);
		if (K <= 0)
		{
			continue;
		}
		if (Dir.X * K == DeltaQ && Dir.Y * K == DeltaR)
		{
			Steps = K;
			Step = Dir;
			break;
		}
	}
	if (Steps <= 0)
	{
		return {}; // non allineata: lo scatto non gira gli angoli
	}

	// Cammino cella per cella: ogni cella attraversata deve esistere, non bloccare ed essere libera. Un ostacolo
	// sulla traiettoria annulla lo scatto: non lo si aggira e non ci si ferma prima (la cella scelta e' quella).
	const TSet<FRTCellId> Blocked = BlockedCellsFor(Snapshot, UnitId);
	const int32 Budget = FMath::Max(0, Unit->MoveBudget);
	TArray<FRTCellId> Path;
	Path.Reserve(Steps + 1);
	Path.Add(Start);
	int32 Cost = 0;
	FRTCellId Current = Start;
	for (int32 I = 0; I < Steps; ++I)
	{
		Current = FRTCellId(Current.X + Step.X, Current.Y + Step.Y, Start.Layer);
		const FRTHexCellData* Data = Snapshot.Map->FindCell(Current);
		if (!Data || Data->bBlocksMovement || Blocked.Contains(Current)
			|| URTTerrainLibrary::FindTerrainDef(Data->Surface).bBlocksDashCharge)
		{
			return {};
		}
		Cost += FMath::Max(1, Data->MoveCost);
		if (Cost > Budget)
		{
			return {}; // oltre la portata dello scatto
		}
		Path.Add(Current);
	}
	return Path;
}

bool URTHexSimLibrary::IsLinearDashReachable(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTCellId& Goal)
{
	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (Unit && Goal == Unit->Cell)
	{
		return true; // "resto dov'e' sono" e' sempre praticabile: non e' uno scatto illegale
	}
	return LinearDashPath(Snapshot, UnitId, Goal).Num() >= 2;
}

TArray<FRTCellId> URTHexSimLibrary::ApplyIceSliding(const FRTHexSnapshot& Snapshot, int32 UnitId, const TArray<FRTCellId>& Path)
{
	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (!Snapshot.Map || !Unit || Path.Num() < 2)
	{
		return Path;
	}

	const FRTCellId LastCell = Path.Last();
	const FRTHexCellData* LastData = Snapshot.Map->FindCell(LastCell);
	if (!LastData || LastData->Surface != ERTHexSurface::Ice)
	{
		return Path;
	}

	int32 PathCost = 0;
	for (int32 I = 1; I < Path.Num(); ++I)
	{
		const FRTHexCellData* StepData = Snapshot.Map->FindCell(Path[I]);
		PathCost += StepData ? FMath::Max(1, StepData->MoveCost) : 1;
	}
	if (Unit->MoveBudget - PathCost < 2)
	{
		return Path;
	}

	const FRTCellId PrevCell = Path[Path.Num() - 2];
	if (PrevCell.Layer != LastCell.Layer)
	{
		return Path; // arrivo via transizione: nessuna "direzione" da cui scivolare
	}

	const int32 StepQ = LastCell.X - PrevCell.X;
	const int32 StepR = LastCell.Y - PrevCell.Y;
	FIntPoint Dir(0, 0);
	bool bValidDirection = false;
	for (int32 D = 0; D < 6; ++D)
	{
		const FIntPoint Candidate = URTHexLibrary::AxialDirection(static_cast<ERTHexDirection>(D));
		if (Candidate.X == StepQ && Candidate.Y == StepR)
		{
			Dir = Candidate;
			bValidDirection = true;
			break;
		}
	}
	if (!bValidDirection)
	{
		return Path; // ultimo passo non e' un vicino diretto
	}

	const FRTCellId SlideCell(LastCell.X + Dir.X, LastCell.Y + Dir.Y, LastCell.Layer);
	const FRTHexCellData* SlideData = Snapshot.Map->FindCell(SlideCell);
	if (!SlideData || SlideData->bBlocksMovement)
	{
		return Path;
	}

	TArray<FRTCellId> Extended = Path;
	Extended.Add(SlideCell);
	return Extended;
}

ERTHexWaypointReason URTHexSimLibrary::ClassifyWaypointCell(const FRTHexSnapshot& Snapshot, int32 UnitId,
	const FRTCellId& Cell)
{
	if (!Snapshot.Map || !Snapshot.Map->ContainsCell(Cell))
	{
		return ERTHexWaypointReason::NotOnMap;
	}
	if (const FRTHexCellData* Data = Snapshot.Map->FindCell(Cell))
	{
		if (Data->bBlocksMovement)
		{
			return ERTHexWaypointReason::BlocksMovement;
		}
	}
	// Un'unita' non blocca se stessa: la propria cella e' un waypoint legittimo (annulla il movimento).
	if (const int32* Occupant = Snapshot.Occupancy.Find(Cell))
	{
		if (*Occupant != UnitId)
		{
			return ERTHexWaypointReason::Occupied;
		}
	}
	return ERTHexWaypointReason::Ok;
}

FRTHexPathResult URTHexSimLibrary::BuildCompositeHexPath(const FRTHexSnapshot& Snapshot, int32 UnitId,
	const TArray<FRTCellId>& Waypoints)
{
	FRTHexPathResult Result;

	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (!Snapshot.Map || !Unit)
	{
		Result.Status = ERTHexPathStatus::StartInvalid;
		return Result;
	}

	Result.Status = ERTHexPathStatus::Success;
	Result.Path.Add(Unit->Cell);

	const TSet<FRTCellId> Blocked = BlockedCellsFor(Snapshot, UnitId);
	int32 Remaining = FMath::Max(0, Unit->MoveBudget);
	FRTCellId Current = Unit->Cell;

	for (const FRTCellId& Waypoint : Waypoints)
	{
		if (Waypoint == Current)
		{
			continue; // click ripetuto sulla stessa cella: tratto a costo zero, niente da accodare
		}
		if (Remaining <= 0)
		{
			// MaxCost == 0 significa "illimitato" per l'A*: senza budget residuo non si chiama.
			FRTHexPathResult Rejected;
			Rejected.Status = Snapshot.Map->ContainsCell(Waypoint)
				? ERTHexPathStatus::NoPath : ERTHexPathStatus::GoalInvalid;
			return Rejected;
		}

		const FRTHexPathResult Leg =
			URTHexPathLibrary::FindPathAvoiding(Snapshot.Map, Current, Waypoint, &Blocked, Remaining);
		if (Leg.Status != ERTHexPathStatus::Success)
		{
			return Leg; // rifiuto dell'INTERO percorso (Path vuoto): il chiamante scarta il waypoint aggiunto
		}

		// Leg.Path parte da Current, che e' gia' l'ultima cella accumulata: la giunzione non si duplica.
		for (int32 i = 1; i < Leg.Path.Num(); ++i)
		{
			Result.Path.Add(Leg.Path[i]);
		}
		Result.TotalCost += Leg.TotalCost;
		Result.NodesVisited += Leg.NodesVisited;
		Remaining -= Leg.TotalCost;
		Current = Waypoint;
	}

	return Result;
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
		// La cella entra nel log com'e': i tre interi sono q, r e Layer e la topologia e' dichiarata dal
		// formato (ERTLogTopology::Hex), non da una reinterpretazione qui.
		Entry.SrcCell = Start;
		Entry.TgtCell = Results[i].Final;
		Entry.Amount = Results[i].Entered.Num(); // celle effettivamente percorse
		Log.Add(Entry);
	}
	return Log;
}
