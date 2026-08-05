#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"

TArray<FRTCellId> URTMatchSetupLibrary::PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer)
{
	TArray<FRTCellId> Result;
	if (!Map || NumPerTeam <= 0)
	{
		return Result;
	}

	// Celle percorribili del layer. CellsInLayer garantisce gia' l'ordine stabile (Layer, X, Y): nessuna
	// dipendenza dall'ordine di una TMap, quindi l'allestimento e' deterministico (invariante #4).
	TArray<FRTCellId> Walkable;
	for (const FRTCellId& Id : Map->CellsInLayer(Layer))
	{
		const FRTHexCellData* Data = Map->FindCell(Id);
		if (Data && !Data->bBlocksMovement)
		{
			Walkable.Add(Id);
		}
	}

	// Non si allestisce a meta': o ci stanno tutte le unita', o il chiamante non allestisce affatto.
	if (Walkable.Num() < NumPerTeam * 2)
	{
		return Result;
	}

	// Team 0 dall'inizio dell'ordine, team 1 dalla fine: le squadre partono agli estremi della mappa.
	Result.Reserve(NumPerTeam * 2);
	for (int32 i = 0; i < NumPerTeam; ++i)
	{
		Result.Add(Walkable[i]);
	}
	for (int32 i = 0; i < NumPerTeam; ++i)
	{
		Result.Add(Walkable[Walkable.Num() - 1 - i]);
	}
	return Result;
}

TMap<FRTCellId, int32> URTMatchSetupLibrary::BuildOccupancy(const TArray<FRTCellId>& Cells,
	const TArray<int32>& UnitIds, const TArray<bool>& Alive)
{
	TMap<FRTCellId, int32> Occupancy;
	if (Cells.Num() != UnitIds.Num() || Cells.Num() != Alive.Num())
	{
		return Occupancy;
	}

	// Le unita' non vive non occupano celle (stessa regola di FRTHexSnapshot::Occupancy).
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		if (Alive[i])
		{
			Occupancy.Add(Cells[i], UnitIds[i]);
		}
	}
	return Occupancy;
}
