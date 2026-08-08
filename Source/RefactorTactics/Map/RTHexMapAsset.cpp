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

void URTHexMapAsset::UpdateCells(const TArray<FRTHexCellData>& InCells)
{
	if (InCells.Num() == 0)
	{
		return; // nessuna modifica: la revisione non deve muoversi
	}

	EnsureLookup();
	for (const FRTHexCellData& Cell : InCells)
	{
		if (const int32* Idx = Lookup.Find(Cell.Id))
		{
			Cells[*Idx] = Cell; // aggiorna in loco (indice stabile)
		}
		else
		{
			const int32 NewIdx = Cells.Add(Cell);
			Lookup.Add(Cell.Id, NewIdx); // la cache resta valida (append non muove gli altri)
		}
	}
	++Revision; // UNA volta per l'intero gruppo
}

void URTHexMapAsset::BeginStroke()
{
	Modify();
}

bool URTHexMapAsset::PaintCellInStroke(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)
{
	AddOrUpdateCell(URTHexMapAsset::ApplyBrush(FindCell(Id), Id, Surface, MoveCost, bBlocksMovement));
	return true;
}

bool URTHexMapAsset::EraseCellInStroke(const FRTCellId& Id)
{
	if (!ContainsCell(Id))
	{
		return false;
	}
	return RemoveCell(Id);
}

void URTHexMapAsset::EndStroke()
{
	SortCells();
	MarkPackageDirty();
}

FRTHexCellData URTHexMapAsset::ApplyBrush(const FRTHexCellData* Existing, const FRTCellId& Id,
	ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement)
{
	FRTHexCellData Cell = Existing ? *Existing : FRTHexCellData(Id);
	Cell.Id = Id; // garantisce l'Id (anche se Existing arrivasse con Id diverso)
	Cell.Surface = Surface;
	Cell.MoveCost = MoveCost;
	Cell.bBlocksMovement = bBlocksMovement;
	return Cell;
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

TArray<int32> URTHexMapAsset::GetLayers() const
{
	TArray<int32> Out;
	for (const FRTHexCellData& C : Cells)
	{
		Out.AddUnique(C.Id.Layer);
	}
	Out.Sort();
	return Out;
}

TArray<FRTCellId> URTHexMapAsset::CellsInLayer(int32 Layer) const
{
	TArray<FRTCellId> Out;
	for (const FRTHexCellData& C : Cells)
	{
		if (C.Id.Layer == Layer)
		{
			Out.Add(C.Id);
		}
	}
	Out.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
	return Out;
}

void URTHexMapAsset::AddTransition(const FRTCellId& From, const FRTCellId& To, int32 Cost,
	ERTHexTransitionKind Kind, bool bBidirectional)
{
	auto UpsertArc = [this](const FRTCellId& A, const FRTCellId& B, int32 InCost, ERTHexTransitionKind InKind)
	{
		for (FRTHexEdge& E : Transitions)
		{
			if (E.From == A && E.To == B)
			{
				E.Cost = InCost; // aggiorna in loco (nessun duplicato per direzione)
				E.Kind = InKind;
				return;
			}
		}
		Transitions.Add(FRTHexEdge(A, B, InCost, InKind));
	};

	UpsertArc(From, To, Cost, Kind);
	if (bBidirectional)
	{
		UpsertArc(To, From, Cost, Kind);
	}
	++Revision;
}

bool URTHexMapAsset::RemoveTransition(const FRTCellId& From, const FRTCellId& To, bool bBothDirections)
{
	const int32 Removed = Transitions.RemoveAll([&](const FRTHexEdge& E)
	{
		return (E.From == From && E.To == To) || (bBothDirections && E.From == To && E.To == From);
	});
	if (Removed > 0)
	{
		++Revision;
		return true;
	}
	return false;
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
	for (FRTHexCellData& C : Sorted)
	{
		Hash = HashCombine(Hash, GetTypeHash(C.Id));
		Hash = HashCombine(Hash, GetTypeHash(C.Height));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(C.Surface)));
		Hash = HashCombine(Hash, GetTypeHash(C.MoveCost));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(C.bBlocksMovement ? 1 : 0)));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(C.bBlocksLineOfSight ? 1 : 0)));

		// Le coperture sono dato autorevole (cambiano il danno subito): entrano nell'hash, ordinate per bordo
		// perche' l'ordine dell'array lo decide chi edita l'asset, non la mappa. Una cella scoperta non
		// aggiunge nulla: l'hash di una mappa senza coperture dipende solo dai campi di prima.
		C.Covers.Sort([](const FRTHexCover& A, const FRTHexCover& B)
		{
			return static_cast<uint8>(A.Edge) < static_cast<uint8>(B.Edge);
		});
		for (const FRTHexCover& Cover : C.Covers)
		{
			Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(Cover.Edge)));
			Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(Cover.Type)));
			Hash = HashCombine(Hash, GetTypeHash(Cover.Integrity));
		}

		// Le porte cambiano la topologia, quindi sono dato autorevole quanto le coperture: entrano nell'hash
		// con lo stesso criterio (ordinate per bordo). E' anche cio' che permette a `IsSnapshotStale` di
		// accorgersi di una porta che si e' mossa, non solo alla revisione.
		C.Doors.Sort([](const FRTHexDoor& A, const FRTHexDoor& B)
		{
			return static_cast<uint8>(A.Edge) < static_cast<uint8>(B.Edge);
		});
		for (const FRTHexDoor& Door : C.Doors)
		{
			Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(Door.Edge)));
			Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(Door.State)));
			Hash = HashCombine(Hash, GetTypeHash(Door.DoorId));
		}
	}

	// Le transizioni sono dato autorevole (i ponti/tunnel cambiano il pathing): entrano nell'hash, ordinate
	// stabilmente (From, To, Kind) cosi' l'hash e' indipendente dall'ordine di inserimento nell'array.
	TArray<FRTHexEdge> SortedEdges = Transitions;
	SortedEdges.Sort([](const FRTHexEdge& A, const FRTHexEdge& B)
	{
		if (A.From != B.From) { return URTHexLibrary::StableLess(A.From, B.From); }
		if (A.To != B.To) { return URTHexLibrary::StableLess(A.To, B.To); }
		return static_cast<uint8>(A.Kind) < static_cast<uint8>(B.Kind);
	});
	for (const FRTHexEdge& E : SortedEdges)
	{
		Hash = HashCombine(Hash, GetTypeHash(E.From));
		Hash = HashCombine(Hash, GetTypeHash(E.To));
		Hash = HashCombine(Hash, GetTypeHash(E.Cost));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(E.Kind)));
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

		// Coperture: si scrivono a mano nell'editor delle proprieta', quindi qui si intercettano gli stati
		// che nessuna regola sa risolvere — meglio un errore leggibile di un comportamento a sorte.
		for (int32 I = 0; I < C.Covers.Num(); ++I)
		{
			const FRTHexCover& Cover = C.Covers[I];
			if (Cover.Type == ERTHexCoverType::None)
			{
				Errors.Add(FString::Printf(TEXT("Error: voce di copertura senza tipo su %s"), *C.Id.ToString()));
			}
			else if (Cover.Integrity <= 0)
			{
				// Un riparo a zero punti struttura e' un riparo gia' distrutto: va tolto, non tenuto a 0
				// (CP 9.2 lo scalera' davvero, e vorra' trovare solo coperture in piedi).
				Errors.Add(FString::Printf(TEXT("Error: copertura con integrita' %d su %s"),
					Cover.Integrity, *C.Id.ToString()));
			}
			// "Non sovrapponibile" (catalogo, `Action.CreateCover`): un bordo ha al massimo una copertura.
			for (int32 J = 0; J < I; ++J)
			{
				if (C.Covers[J].Edge == Cover.Edge)
				{
					Errors.Add(FString::Printf(TEXT("Error: coperture sovrapposte sul bordo %d di %s"),
						static_cast<int32>(Cover.Edge), *C.Id.ToString()));
					break;
				}
			}
		}

		// Porte (CP 9.3): stessi due difetti che le coperture non sanno risolvere, piu' quello nuovo — una
		// porta dentro un muro pieno. A runtime il muro vince comunque (`BlocksTraversal` e' un OR
		// restrittivo), quindi la porta non si aprirebbe mai: meglio dirlo a chi disegna il livello che
		// lasciargli credere di aver messo un varco.
		for (int32 I = 0; I < C.Doors.Num(); ++I)
		{
			const FRTHexDoor& Door = C.Doors[I];
			for (int32 J = 0; J < I; ++J)
			{
				if (C.Doors[J].Edge == Door.Edge)
				{
					Errors.Add(FString::Printf(TEXT("Error: porte sovrapposte sul bordo %d di %s"),
						static_cast<int32>(Door.Edge), *C.Id.ToString()));
					break;
				}
			}
			if (C.CoverOn(Door.Edge) == ERTHexCoverType::High)
			{
				Errors.Add(FString::Printf(TEXT("Error: porta dentro una copertura alta sul bordo %d di %s"),
					static_cast<int32>(Door.Edge), *C.Id.ToString()));
			}
		}
	}
	for (int32 I = 0; I < Transitions.Num(); ++I)
	{
		const FRTHexEdge& E = Transitions[I];

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
		if (E.From == E.To)
		{
			Errors.Add(FString::Printf(TEXT("Error: transizione self-loop su %s"), *E.From.ToString()));
		}
		// Duplicato: stessa coppia From->To gia' presente in un arco precedente (segnalato una sola volta).
		for (int32 J = 0; J < I; ++J)
		{
			if (Transitions[J].From == E.From && Transitions[J].To == E.To)
			{
				Errors.Add(FString::Printf(TEXT("Error: transizione duplicata %s -> %s"),
					*E.From.ToString(), *E.To.ToString()));
				break;
			}
		}
		// Ridondante: stesso layer e celle gia' adiacenti orizzontalmente -> l'arco esplicito e' superfluo.
		if (E.From != E.To && E.From.Layer == E.To.Layer && URTHexLibrary::HexDistance(E.From, E.To) == 1)
		{
			Errors.Add(FString::Printf(TEXT("Warning: transizione ridondante tra celle gia' adiacenti %s -> %s"),
				*E.From.ToString(), *E.To.ToString()));
		}
	}
	return Errors;
}

TArray<FRTCellId> URTHexMapAsset::FloodRegion(const FRTCellId& Start) const
{
	TArray<FRTCellId> Region;
	const FRTHexCellData* StartData = FindCell(Start);
	if (!StartData)
	{
		return Region; // start inesistente: nessuna regione
	}
	const ERTHexSurface Target = StartData->Surface;

	TSet<FRTCellId> Visited;
	Visited.Add(Start);
	TArray<FRTCellId> Frontier;
	Frontier.Add(Start);

	while (Frontier.Num() > 0)
	{
		const FRTCellId Current = Frontier.Pop(EAllowShrinking::No);
		Region.Add(Current);
		for (const FRTCellId& N : URTHexLibrary::Neighbors(Current))
		{
			if (Visited.Contains(N))
			{
				continue;
			}
			const FRTHexCellData* NData = FindCell(N);
			if (NData && NData->Surface == Target)
			{
				Visited.Add(N);
				Frontier.Add(N);
			}
		}
	}
	return Region;
}

#if WITH_EDITOR
void URTHexMapAsset::PostEditUndo()
{
	Super::PostEditUndo();

	// L'undo/redo riscrive Cells direttamente, senza passare dalle API di questa classe: la cache Id->indice
	// resterebbe allineata allo stato precedente e FindCell leggerebbe l'indice sbagliato — o fuori dall'array,
	// se le celle sono diminuite. Va invalidata PRIMA che qualcuno interroghi l'asset.
	InvalidateLookup();

	// Chi mostra l'asset (ARTHexMapActor) non fa parte della transazione e non saprebbe di dover ridisegnare.
	OnMapChanged.Broadcast();
}
#endif

FRTCellId URTHexMapAsset::GetCenterCell() const
{
	// Layer piu' basso = piano di gioco principale: e' quello che la camera deve inquadrare.
	int32 BaseLayer = MAX_int32;
	for (const FRTHexCellData& Cell : Cells)
	{
		BaseLayer = FMath::Min(BaseLayer, Cell.Id.Layer);
	}
	if (BaseLayer == MAX_int32)
	{
		return FRTCellId(); // mappa vuota
	}

	int32 MinQ = MAX_int32, MaxQ = MIN_int32, MinR = MAX_int32, MaxR = MIN_int32;
	for (const FRTHexCellData& Cell : Cells)
	{
		if (Cell.Id.Layer != BaseLayer) { continue; }
		MinQ = FMath::Min(MinQ, Cell.Id.X);
		MaxQ = FMath::Max(MaxQ, Cell.Id.X);
		MinR = FMath::Min(MinR, Cell.Id.Y);
		MaxR = FMath::Max(MaxR, Cell.Id.Y);
	}

	// Mediana del bounding box assiale: divisione intera (nessun float nelle coordinate, invariante #4).
	return FRTCellId(FMath::DivideAndRoundDown(MinQ + MaxQ, 2), FMath::DivideAndRoundDown(MinR + MaxR, 2), BaseLayer);
}

void URTHexMapAsset::MigrateToCurrentFormat()
{
	if (FormatVersion >= CurrentFormatVersion)
	{
		return; // gia' aggiornato: idempotente, PostLoad puo' ripetersi
	}

	// v2 -> v3 (CP 9.1): il formato guadagna le coperture per bordo.
	// v3 -> v4 (CP 9.3): il formato guadagna le porte per bordo.
	// In nessuno dei due c'e' qualcosa da convertire — il campo nuovo nasce vuoto e una mappa che non lo usa
	// si comporta esattamente come prima — quindi la migrazione si limita a dichiarare la versione. Il giorno
	// in cui una migrazione dovra' TRASFORMARE dati, il posto e' questo, un `if (FormatVersion < N)` per
	// volta, in ordine.
	FormatVersion = CurrentFormatVersion;
}

void URTHexMapAsset::PostLoad()
{
	Super::PostLoad();
	MigrateToCurrentFormat();
}
