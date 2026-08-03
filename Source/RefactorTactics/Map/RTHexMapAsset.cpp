#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"

void URTHexMapAsset::EnsureLookup() const
{
	if (!bLookupDirty)
	{
		return;
	}
	Lookup.Reset();
	Lookup.Reserve(Cells.Num());
	for (int32 I = 0; I < Cells.Num(); ++I)
	{
		Lookup.Add(Cells[I].Id, I);
	}
	bLookupDirty = false;
}

void URTHexMapAsset::AddOrUpdateCell(const FRTHexCellData& Cell)
{
	EnsureLookup();
	if (const int32* Idx = Lookup.Find(Cell.Id))
	{
		Cells[*Idx] = Cell; // aggiorna in loco (indice stabile)
	}
	else
	{
		const int32 NewIdx = Cells.Add(Cell);
		Lookup.Add(Cell.Id, NewIdx); // la cache resta valida (append non muove gli altri)
	}
	++Revision;
}

bool URTHexMapAsset::RemoveCell(const FRTCellId& Id)
{
	EnsureLookup();
	const int32* Idx = Lookup.Find(Id);
	if (!Idx)
	{
		return false;
	}
	Cells.RemoveAt(*Idx); // gli indici successivi scalano -> cache non piu' valida
	bLookupDirty = true;
	++Revision;
	return true;
}

const FRTHexCellData* URTHexMapAsset::FindCell(const FRTCellId& Id) const
{
	EnsureLookup();
	if (const int32* Idx = Lookup.Find(Id))
	{
		return &Cells[*Idx];
	}
	return nullptr;
}

bool URTHexMapAsset::ContainsCell(const FRTCellId& Id) const
{
	EnsureLookup();
	return Lookup.Contains(Id);
}

void URTHexMapAsset::SortCells()
{
	Cells.Sort([](const FRTHexCellData& A, const FRTHexCellData& B) { return URTHexLibrary::StableLess(A.Id, B.Id); });
	bLookupDirty = true;
}

uint32 URTHexMapAsset::ComputeHash() const
{
	// Ordine stabile -> hash indipendente dall'ordine di inserimento (copia locale per non mutare l'asset).
	TArray<FRTHexCellData> Sorted = Cells;
	Sorted.Sort([](const FRTHexCellData& A, const FRTHexCellData& B) { return URTHexLibrary::StableLess(A.Id, B.Id); });

	uint32 Hash = GetTypeHash(FormatVersion);
	for (const FRTHexCellData& C : Sorted)
	{
		Hash = HashCombine(Hash, GetTypeHash(C.Id));
		Hash = HashCombine(Hash, GetTypeHash(C.Height));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(C.Surface)));
		Hash = HashCombine(Hash, GetTypeHash(C.MoveCost));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(C.bBlocksMovement ? 1 : 0)));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(C.bBlocksLineOfSight ? 1 : 0)));
	}
	return Hash;
}

TArray<FString> URTHexMapAsset::ValidateMap() const
{
	TArray<FString> Errors;
	TSet<FRTCellId> Seen;
	for (const FRTHexCellData& C : Cells)
	{
		bool bAlready = false;
		Seen.Add(C.Id, &bAlready);
		if (bAlready)
		{
			Errors.Add(FString::Printf(TEXT("Error: cella duplicata %s"), *C.Id.ToString()));
		}
		if (C.MoveCost < 0)
		{
			Errors.Add(FString::Printf(TEXT("Error: costo negativo su %s"), *C.Id.ToString()));
		}
	}
	for (const FRTHexEdge& E : Transitions)
	{
		if (!Seen.Contains(E.From) || !Seen.Contains(E.To))
		{
			Errors.Add(FString::Printf(TEXT("Error: transizione verso cella inesistente %s -> %s"),
				*E.From.ToString(), *E.To.ToString()));
		}
		if (E.Cost < 0)
		{
			Errors.Add(FString::Printf(TEXT("Error: costo transizione negativo %s -> %s"),
				*E.From.ToString(), *E.To.ToString()));
		}
	}
	return Errors;
}
