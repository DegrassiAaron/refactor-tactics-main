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
	// Predecessore di ogni cella: serve al FACING, che deriva dall'ultimo passo (CP 13.5). La partenza e'
	// predecessore di se stessa — chi non si muove non ha un «da dove», e il suo orientamento non cambia.
	TMap<FRTCellId, FRTCellId> From;
	TArray<FRTCellId> Frontier;
	TSet<FRTCellId> Closed;

	Dist.Add(Unit->Cell, 0);
	From.Add(Unit->Cell, Unit->Cell);
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
			const int32 Tentative = Dist[Current] + Step.Value + FMath::Max(0, Unit->MoveCostModifier);
			if (Tentative > Budget)
			{
				continue; // fuori dal budget di movimento
			}
			const int32* Existing = Dist.Find(Step.Key);
			if (!Existing || Tentative < *Existing)
			{
				Dist.Add(Step.Key, Tentative);
				From.Add(Step.Key, Current);
				Frontier.Add(Step.Key);
			}
			else if (Tentative == *Existing)
			{
				// PARITA' di costo: due percorsi ugualmente economici arrivano sulla stessa cella, e il facing
				// che se ne deriva dipende da quale si sceglie. Il tie-break e' esplicito e sull'ID, come quello
				// dell'estrazione: senza, il predecessore lo deciderebbe l'ordine di visita — cioe' un dettaglio
				// che nessuno ha dichiarato, e che il resolver non e' tenuto a riprodurre.
				const FRTCellId* Prev = From.Find(Step.Key);
				if (Prev && URTHexLibrary::StableLess(Current, *Prev))
				{
					From.Add(Step.Key, Current);
				}
			}
		}
	}

	Out.Reserve(Dist.Num());
	for (const TPair<FRTCellId, int32>& Entry : Dist)
	{
		const FRTCellId* Prev = From.Find(Entry.Key);
		Out.Add(FRTHexReachableCell(Entry.Key, Entry.Value, Prev ? *Prev : Entry.Key));
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
	return URTHexPathLibrary::FindPathAvoiding(Snapshot.Map, Unit->Cell, Goal, &Blocked, Budget,
		/*MaxNodes*/ 100000, FMath::Max(0, Unit->MoveCostModifier));
}

TArray<FRTCellId> URTHexSimLibrary::ApplyIceSliding(const FRTHexSnapshot& Snapshot, int32 UnitId, const TArray<FRTCellId>& Path)
{
	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (!Snapshot.Map || !Unit || Path.Num() < 2)
	{
		return Path;
	}

	// Regola dal CATALOGO, non dall'enum: e' un dato del terreno come il costo e il blocco allo scatto. Con
	// `SlideCells <= 0` non si scivola. Oggi si estende comunque di UNA sola cella (vedi FRTTerrainDef::SlideCells).
	const FRTCellId LastCell = Path.Last();
	const FRTHexCellData* LastData = Snapshot.Map->FindCell(LastCell);
	if (!LastData || URTTerrainLibrary::FindTerrainDef(LastData->Surface).SlideCells <= 0)
	{
		return Path;
	}

	// Stessa formula di TruncatePathToBudget: costo della cella PIU' il modificatore dell'unita' (`Slow` lo
	// alza, CP 4.7). Sommare il solo MoveCost sottostimerebbe la spesa di chi e' rallentato, e lo si vedrebbe
	// scivolare con un budget che in realta' aveva gia' finito.
	int32 PathCost = 0;
	const int32 ExtraPerCell = FMath::Max(0, Unit->MoveCostModifier);
	for (int32 I = 1; I < Path.Num(); ++I)
	{
		const FRTHexCellData* StepData = Snapshot.Map->FindCell(Path[I]);
		PathCost += (StepData ? StepData->TotalMoveCost() : 0) + ExtraPerCell;
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

		const FRTHexPathResult Leg = URTHexPathLibrary::FindPathAvoiding(Snapshot.Map, Current, Waypoint, &Blocked,
			Remaining, /*MaxNodes*/ 100000, FMath::Max(0, Unit->MoveCostModifier));
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

TArray<FRTCellId> URTHexSimLibrary::TruncatePathToBudget(const FRTHexSnapshot& Snapshot, int32 UnitId,
	const TArray<FRTCellId>& Path)
{
	if (Path.Num() == 0)
	{
		return Path;
	}

	const FRTHexSimUnit* Unit = FindUnit(Snapshot, UnitId);
	if (!Snapshot.Map || !Unit)
	{
		return Path; // dato non verificabile: si lascia il piano invariato, non lo si annulla ne' lo si taglia a caso
	}

	const int32 Budget = FMath::Max(0, Unit->MoveBudget);
	const int32 ExtraPerCell = FMath::Max(0, Unit->MoveCostModifier);

	TArray<FRTCellId> Truncated;
	Truncated.Add(Path[0]);
	int32 Spent = 0;

	for (int32 k = 1; k < Path.Num(); ++k)
	{
		const FRTHexCellData* Data = Snapshot.Map->FindCell(Path[k]);
		const int32 StepCost = (Data ? Data->TotalMoveCost() : 0) + ExtraPerCell;
		if (Spent + StepCost > Budget)
		{
			break; // il budget finisce qui: il resto del piano non e' piu' affrontabile con lo stato attuale
		}
		Spent += StepCost;
		Truncated.Add(Path[k]);
	}
	return Truncated;
}

TArray<FRTCellId> URTHexSimLibrary::TruncatePathToTopology(const FRTHexSnapshot& Snapshot,
	const TArray<FRTCellId>& Path)
{
	if (!Snapshot.Map || Path.Num() < 2)
	{
		return Path; // dato non verificabile, o niente da camminare
	}

	TArray<FRTCellId> Walkable;
	Walkable.Add(Path[0]);
	for (int32 k = 1; k < Path.Num(); ++k)
	{
		// Si CHIEDE AL GRAFO invece di rileggere i bordi: la regola su cosa separa due celle vive in un posto
		// solo (`URTHexCoverLibrary::BlocksTraversal`, che `GraphNeighbors` gia' interroga), e riscriverla qui
		// significherebbe due risposte alla stessa domanda, destinate a divergere.
		bool bStepStillExists = false;
		for (const TPair<FRTCellId, int32>& Step : URTHexPathLibrary::GraphNeighbors(Snapshot.Map, Path[k - 1]))
		{
			if (Step.Key == Path[k])
			{
				bStepStillExists = true;
				break;
			}
		}
		if (!bStepStillExists)
		{
			break; // la topologia e' cambiata da quando il piano e' stato scritto: si ferma QUI
		}
		Walkable.Add(Path[k]);
	}
	return Walkable;
}

namespace
{
	// Un solo microstep, letto e scritto sullo STATO invece che sulle variabili locali di un ciclo. Il corpo
	// e' rimasto quello che era: CP 14.2 sposta il confine della funzione, non una riga di regola.
	bool StepHexMovement(FRTMovementResolutionState& State)
	{
		const int32 N = State.Num();

		// Priorita' 0 (parita' con tutti) e non-lineare per chi non ha un valore dichiarato: con entrambi gli
		// array vuoti l'esito e' identico alla variante senza priorita'.
		auto PriorityOf = [&State](int32 i) { return State.Priorities.IsValidIndex(i) ? State.Priorities[i] : 0; };
		auto IsLinearMover = [&State](int32 i) { return State.bLinearMovers.IsValidIndex(i) && State.bLinearMovers[i]; };
		auto PassesThrough = [&State](int32 i) { return State.bPassThrough.IsValidIndex(i) && State.bPassThrough[i]; };

		const TArray<TArray<FRTCellId>>& Paths = State.Paths;
		TArray<FRTCellId>& Pos = State.Pos;
		TArray<int32>& Prog = State.Prog;
		TArray<bool>& Done = State.Done;
		TArray<ERTMoveOutcome>& BlockReason = State.BlockReason;
		TArray<bool>& ReasonLocked = State.ReasonLocked;
		TArray<FRTHexMoveResult>& Results = State.Results;

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

					bool bBlocked = false;
					ERTMoveOutcome Reason = ERTMoveOutcome::BlockedByUnit;

					// Destinazione contesa: 2+ unita' in movimento verso la stessa cella. A parita' di
					// priorita' fra i contendenti (compreso il caso senza priorita' dichiarata) tutti fermi,
					// come nella variante base; a priorita' diverse, solo la piu' bassa fra i contendenti
					// entra, le altre perdono la cella.
					int32 Contenders = 0;
					int32 MinPriority = 0;
					for (int32 j = 0; j < N; ++j)
					{
						if (Moving[j] && Target[j] == Target[i])
						{
							MinPriority = (Contenders == 0) ? PriorityOf(j) : FMath::Min(MinPriority, PriorityOf(j));
							++Contenders;
						}
					}
					if (Contenders >= 2)
					{
						int32 Winners = 0;
						for (int32 j = 0; j < N; ++j)
						{
							if (Moving[j] && Target[j] == Target[i] && PriorityOf(j) == MinPriority)
							{
								++Winners;
							}
						}
						if (Winners >= 2 || PriorityOf(i) != MinPriority)
						{
							bBlocked = true;
							Reason = (Winners >= 2) ? ERTMoveOutcome::BlockedContested : ERTMoveOutcome::BlockedByPriority;
						}
					}

					// Scontro frontale: due mobilita' LINEARI (Action.Charge e affini) che si scambierebbero la
					// cella nello stesso microstep (l'una entra dove sta l'altra, e viceversa) si fermano
					// l'una davanti all'altra invece di attraversarsi. Lo scambio fra mobilita' non entrambe
					// lineari resta consentito (comportamento di base, invariato).
					if (!bBlocked && IsLinearMover(i))
					{
						for (int32 j = 0; j < N; ++j)
						{
							if (j != i && Moving[j] && IsLinearMover(j) && Target[i] == Pos[j] && Target[j] == Pos[i])
							{
								bBlocked = true;
								Reason = ERTMoveOutcome::BlockedByImpact;
								break;
							}
						}
					}

					// Bloccata da un'unita' che RESTA (esaurita o congelata) sulla cella di destinazione.
					//
					// Chi ATTRAVERSA ci passa in mezzo, ma solo se quella cella non e' la sua ULTIMA: si
					// transita dentro qualcuno, non ci si ferma. Due unita' nella stessa cella a fine turno non
					// sono rappresentabili, e un'eccezione qui le renderebbe possibili per una sola azione.
					const bool bFinalStep = Paths.IsValidIndex(i) && (Prog[i] + 1) == (Paths[i].Num() - 1);
					const bool bCrossesStationary = PassesThrough(i) && !bFinalStep;
					if (!bBlocked && !bCrossesStationary)
					{
						for (int32 j = 0; j < N; ++j)
						{
							if (j != i && !Moving[j] && Pos[j] == Target[i])
							{
								bBlocked = true;
								Reason = ERTMoveOutcome::BlockedByUnit;
								break;
							}
						}
					}
					if (bBlocked)
					{
						ToFreeze.Add(i);
						if (!ReasonLocked[i])
						{
							BlockReason[i] = Reason;
							ReasonLocked[i] = true;
						}
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
			++State.MicroStepIndex;
			return bAnyMoved;
		}
	}

	// Reason code finale: dipende solo da Final/Paths -> indipendente dall'ordine, e quindi anche da QUANTI
	// microstep sono serviti e da dove il chiamante li ha interrotti.
	void FinalizeHexMovementOutcomes(FRTMovementResolutionState& State)
	{
		for (int32 i = 0; i < State.Num(); ++i)
		{
			if (State.Paths[i].Num() <= 1)
			{
				State.Results[i].Outcome = ERTMoveOutcome::Stayed;
			}
			else if (State.Results[i].Final == State.Paths[i].Last())
			{
				State.Results[i].Outcome = ERTMoveOutcome::Moved;
			}
			else
			{
				State.Results[i].Outcome = State.BlockReason[i];
			}
		}
	}
}

FRTMovementResolutionState URTHexSimLibrary::BeginHexMovement(const TArray<TArray<FRTCellId>>& Paths,
	const TArray<int32>& Priorities, const TArray<bool>& bLinearMovers, const TArray<bool>& bPassThrough)
{
	FRTMovementResolutionState State;
	State.Paths = Paths;
	State.Priorities = Priorities;
	State.bLinearMovers = bLinearMovers;
	State.bPassThrough = bPassThrough;

	const int32 N = Paths.Num();
	State.Results.SetNum(N);
	State.Pos.SetNum(N);
	State.Prog.SetNum(N);
	State.Done.SetNum(N);
	for (int32 i = 0; i < N; ++i)
	{
		State.Pos[i] = Paths[i].Num() > 0 ? Paths[i][0] : FRTCellId();
		State.Prog[i] = 0;
		State.Done[i] = Paths[i].Num() <= 1;
		State.Results[i].Final = State.Pos[i];
	}

	// Motivo del PRIMO congelamento per unita' (reason code del TurnLog): resta quello, anche se un
	// microstep successivo la bloccherebbe per un motivo diverso. Senza questa "memoria", una Move che
	// perde la cella contesa contro una Charge con priorita' migliore (CP 4.8) - e la RITENTA al
	// microstep successivo, perche' non e' mai arrivata a destinazione - la troverebbe occupata dalla
	// Charge ormai ferma li', e il motivo diventerebbe "cella occupata" invece di "priorita' avversa":
	// vero all'ULTIMO microstep, ma non la causa reale per cui non e' mai entrata.
	State.BlockReason.Init(ERTMoveOutcome::BlockedByUnit, N);
	State.ReasonLocked.Init(false, N);

	return State;
}

bool URTHexSimLibrary::ResolveNextHexMicroStep(FRTMovementResolutionState& State)
{
	if (State.bFinished)
	{
		// Idempotente dopo la fine: chiamarlo ancora non muove nulla e non riscrive `Results`. Serve a chi
		// pilota il ciclo da fuori - una finestra di reazione - e non ha modo di sapere se era l'ultimo giro.
		return false;
	}

	const bool bAnyMoved = StepHexMovement(State);
	if (!bAnyMoved)
	{
		State.bFinished = true;
		FinalizeHexMovementOutcomes(State);
	}
	return bAnyMoved;
}

void URTHexSimLibrary::StopUnitInPlace(FRTMovementResolutionState& State, int32 UnitId, ERTMoveOutcome Reason)
{
	if (!State.Done.IsValidIndex(UnitId))
	{
		return; // indice non valido: non c'e' un'unita' da fermare, e inventarne una sarebbe peggio
	}

	// `Done` e' cio' che il microstep legge (`Moving[i] = !Done[i]`): da qui in poi questa unita' non avanza.
	State.Done[UnitId] = true;

	// La posizione corrente E' quella finale. `Pos` e `Results[].Final` sono gia' allineati dal microstep
	// appena eseguito — non si riscrive `Entered`, perche' le celle attraversate fin qui sono state
	// attraversate davvero, ed e' esattamente cio' che il TurnLog deve raccontare.
	State.Results[UnitId].Final = State.Pos[UnitId];

	// Terminale, quindi vince sulla memoria del primo congelamento: vedi il commento nell'header.
	State.BlockReason[UnitId] = Reason;
	State.ReasonLocked[UnitId] = true;
}

TArray<FRTHexMoveResult> URTHexSimLibrary::FinishHexMovement(FRTMovementResolutionState& State)
{
	// Chi salta il ciclo e chiede il risultato subito ottiene comunque una risposta coerente: il movimento si
	// esaurisce, non resta a meta'. Senza, un chiamante distratto leggerebbe `Outcome` non ancora scritti.
	while (ResolveNextHexMicroStep(State)) {}
	return State.Results;
}

TArray<FRTHexMoveResult> URTHexSimLibrary::ResolveHexPaths(const TArray<TArray<FRTCellId>>& Paths)
{
	return ResolveHexPaths(Paths, TArray<int32>(), TArray<bool>(), TArray<bool>());
}

TArray<FRTHexMoveResult> URTHexSimLibrary::ResolveHexPaths(const TArray<TArray<FRTCellId>>& Paths,
	const TArray<int32>& Priorities, const TArray<bool>& bLinearMovers, const TArray<bool>& bPassThrough)
{
	// `initialize -> while(!finished) step -> result`: la via a passi e quella in blocco sono LO STESSO
	// codice, non due algoritmi che qualcuno dovra' tenere allineati. E' la condizione per cui CP 14.2 puo'
	// dichiarare "nessun comportamento cambia" invece di sperarlo.
	FRTMovementResolutionState State = BeginHexMovement(Paths, Priorities, bLinearMovers, bPassThrough);
	return FinishHexMovement(State);
}

TArray<FRTTurnLogEntry> URTHexSimLibrary::BuildMoveLog(const TArray<TArray<FRTCellId>>& Paths,
	const TArray<FRTHexMoveResult>& Results, FName CauseActionId, int32 CausePriority)
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
		// PERCHE' si e' spostata (#307): il log diceva CHE e con quale esito, non per quale causa. Uno
		// spostamento senza sorgente e' indistinguibile da un difetto del resolver.
		Entry.ActionId = CauseActionId;
		Entry.Priority = CausePriority; // con quale precedenza il movimento ha risolto (CP 11.3, formato v7)
		Log.Add(Entry);
	}
	return Log;
}
