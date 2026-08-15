#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapCustomVersion.h"
#include "Serialization/CustomVersion.h"

const FGuid FRTHexMapCustomVersion::GUID(0x7A3C1E44, 0x9B2D4F10, 0xA6E85C37, 0x1D0F62B9);

// Registrazione nel registro globale: da qui in poi ogni package che salva un `URTHexMapAsset` porta GUID
// e versione nel proprio summary.
FCustomVersionRegistration GRegisterRTHexMapCustomVersion(
	FRTHexMapCustomVersion::GUID, FRTHexMapCustomVersion::LatestVersion, TEXT("RTHexMapVer"));

// I due numeri sono lo STESSO numero e devono restare tali. Alzare `CurrentFormatVersion` senza aggiungere
// il valore corrispondente all'enum rimetterebbe in piedi il difetto di #687 sotto un altro nome: la
// migrazione partirebbe da una versione che nessun archivio puo' dichiarare.
static_assert(FRTHexMapCustomVersion::LatestVersion == URTHexMapAsset::CurrentFormatVersion,
	"La versione del custom version e CurrentFormatVersion sono divergenti: aggiungi il valore mancante "
	"a FRTHexMapCustomVersion::Type nello stesso commit in cui alzi CurrentFormatVersion.");

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

bool URTHexMapAsset::PaintCellInStroke(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost,
	bool bBlocksMovement, TOptional<bool> InBlocksLineOfSight)
{
	AddOrUpdateCell(URTHexMapAsset::ApplyBrush(FindCell(Id), Id, Surface, MoveCost, bBlocksMovement,
		InBlocksLineOfSight));
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
	ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement, TOptional<bool> InBlocksLineOfSight)
{
	FRTHexCellData Cell = Existing ? *Existing : FRTHexCellData(Id);
	Cell.Id = Id; // garantisce l'Id (anche se Existing arrivasse con Id diverso)
	Cell.Surface = Surface;
	Cell.MoveCost = MoveCost;
	Cell.bBlocksMovement = bBlocksMovement;
	// Non passato = preserva, che e' il comportamento storico e resta quello di default: `ApplyBrushMerge` lo
	// pinna. Passato = scrive, ed e' cio' che permette al pennello di dipingere un muro invece di doverlo
	// scrivere a mano nell'asset.
	if (InBlocksLineOfSight.IsSet())
	{
		Cell.bBlocksLineOfSight = InBlocksLineOfSight.GetValue();
	}
	return Cell;
}

bool URTHexMapAsset::ClearAll()
{
	if (Cells.Num() == 0 && Transitions.Num() == 0)
	{
		return false; // nessuna modifica: la revisione non deve muoversi
	}

	Cells.Reset();
	Transitions.Reset();
	bLookupDirty = true; // gli indici non sopravvivono a un reset
	++Revision;          // UNA volta per l'intero svuotamento, come `UpdateCells`
	return true;
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

void URTHexMapAsset::UpdateTransitions(const TArray<FRTHexEdge>& InEdges)
{
	if (InEdges.Num() == 0)
	{
		return; // nessuna modifica: la revisione non deve muoversi
	}

	for (const FRTHexEdge& Updated : InEdges)
	{
		bool bReplaced = false;
		for (FRTHexEdge& Existing : Transitions)
		{
			if (Existing.From == Updated.From && Existing.To == Updated.To)
			{
				Existing = Updated;
				bReplaced = true;
				break;
			}
		}
		if (!bReplaced)
		{
			Transitions.Add(Updated);
		}
	}
	++Revision; // UNA volta per l'intero gruppo (un ponte bidirezionale e' un evento solo)
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
		// Il sovrapprezzo di geometria e' dato autorevole quanto `MoveCost`: cambia quanto costa entrare, e
		// due mappe che si giocano diversamente non possono avere lo stesso hash (formato v7, #619).
		Hash = HashCombine(Hash, GetTypeHash(C.OccupancySurcharge));
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
		// Stato, integrita' e conduttivita' (CP 9.4) sono dato autorevole quanto il costo: un ponte spento e
		// uno acceso non sono la stessa mappa, ed e' anche cio' che permette a `IsSnapshotStale` di
		// accorgersene senza dipendere dalla sola revisione.
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(E.State)));
		Hash = HashCombine(Hash, GetTypeHash(E.Integrity));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(E.bConductsElectricity ? 1 : 0)));
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
		if (C.OccupancySurcharge < 0)
		{
			// `TotalMoveCost()` lo clampa, quindi un negativo non produce un costo negativo — ma passerebbe la
			// validazione in silenzio, e questo e' il gate che deve restare verde sui dati COTTI (#621).
			Errors.Add(FString::Printf(TEXT("Error: sovrapprezzo di occupazione negativo su %s"),
				*C.Id.ToString()));
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
		// Un arco ANCORA IN PIEDI con integrita' non positiva e' un ponte gia' crollato che non lo dichiara:
		// il grafo lo offrirebbe, e chi ci cammina sopra passerebbe su una struttura a zero punti (CP 9.4).
		if (E.State != ERTHexArcState::Destroyed && E.Integrity <= 0)
		{
			Errors.Add(FString::Printf(TEXT("Error: arco con integrita' %d ancora attivo %s -> %s"),
				E.Integrity, *E.From.ToString(), *E.To.ToString()));
		}

		// Ridondante: stesso layer e celle gia' adiacenti orizzontalmente -> l'arco esplicito e' superfluo.
		if (E.From != E.To && E.From.Layer == E.To.Layer && URTHexLibrary::HexDistance(E.From, E.To) == 1)
		{
			Errors.Add(FString::Printf(TEXT("Warning: transizione ridondante tra celle gia' adiacenti %s -> %s"),
				*E.From.ToString(), *E.To.ToString()));
		}
	}

	// Identita' stabile delle strutture (CP 23.3, #832): UN nome, UNA struttura.
	//
	// La condivisione fra i bordi di uno stesso portone e' LEGITTIMA — e' cio' che `DoorId` gia' esprime —
	// quindi la regola non e' «un nome, un bordo» ma «un nome, un GRUPPO». Se fosse la prima, validare una
	// porta larga diventerebbe impossibile, e la porta larga ha gia' dei test verdi.
	//
	// ⚠️ `INDEX_NONE` non e' un gruppo: e' l'ASSENZA di gruppo, e tutte le porte singole della mappa la
	// condividono. Trattarlo come un gruppo farebbe passare due porte scollegate con lo stesso nome, che e'
	// il difetto preciso che questa validazione esiste per intercettare.
	{
		struct FRTNamedDoorEdge
		{
			ERTHexDirection Edge;
			int32 DoorId;
		};
		struct FRTNamedStructure
		{
			TArray<FRTNamedDoorEdge> DoorEdges;
			TArray<FRTHexEdge> Arcs;
		};
		TMap<FName, FRTNamedStructure> ByName;

		for (const FRTHexCellData& C : Cells)
		{
			for (const FRTHexDoor& Door : C.Doors)
			{
				if (Door.StableId.IsNone())
				{
					continue; // il nome vuoto non e' un nome: le anonime non collidono fra loro
				}
				ByName.FindOrAdd(Door.StableId).DoorEdges.Add({ Door.Edge, Door.DoorId });
			}
		}
		for (const FRTHexEdge& Arc : Transitions)
		{
			if (Arc.StableId.IsNone())
			{
				continue;
			}
			ByName.FindOrAdd(Arc.StableId).Arcs.Add(Arc);
		}

		// L'ordine di iterazione di una `TMap` non e' deterministico (invariante n. 3): i nomi si ordinano
		// prima di emettere, o due validazioni della stessa mappa darebbero gli stessi errori in ordine
		// diverso — e un diff di validazione smetterebbe di essere leggibile.
		TArray<FName> Names;
		ByName.GetKeys(Names);
		Names.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });

		for (const FName& Name : Names)
		{
			const FRTNamedStructure& Parts = ByName[Name];

			// Un nome non attraversa i domini: una porta e un arco sono due strutture diverse, e `Interact
			// <nome>` non saprebbe quale ha davanti.
			if (Parts.DoorEdges.Num() > 0 && Parts.Arcs.Num() > 0)
			{
				Errors.Add(FString::Printf(
					TEXT("Error: identita' di struttura '%s' usata sia da una porta sia da un arco"),
					*Name.ToString()));
				continue;
			}

			// Porte: piu' bordi con lo stesso nome sono legittimi solo se sono lo STESSO gruppo.
			// `INDEX_NONE` non e' un gruppo — e' l'assenza di gruppo, che tutte le porte singole
			// condividono — quindi prenderlo per tale farebbe passare due porte scollegate omonime.
			if (Parts.DoorEdges.Num() > 1)
			{
				const int32 Group = Parts.DoorEdges[0].DoorId;
				bool bSameGroup = (Group != INDEX_NONE);
				for (const FRTNamedDoorEdge& NamedEdge : Parts.DoorEdges)
				{
					if (NamedEdge.DoorId != Group)
					{
						bSameGroup = false;
						break;
					}
				}
				if (!bSameGroup)
				{
					Errors.Add(FString::Printf(
						TEXT("Error: identita' di struttura duplicata '%s' su %d bordi che non sono un gruppo"),
						*Name.ToString(), Parts.DoorEdges.Num()));
				}
			}

			// Archi: qui il gruppo non e' un campo, e' una RELAZIONE. Due archi condividono legittimamente
			// il nome solo se sono reciproci — `UpdateTransitions` lo dichiara gia': «bidirezionale sono due
			// archi ma un evento solo». Tre archi omonimi non sono un ponte, sono un errore d'asset.
			if (Parts.Arcs.Num() > 1)
			{
				const bool bReciprocalPair = Parts.Arcs.Num() == 2
					&& Parts.Arcs[0].From == Parts.Arcs[1].To
					&& Parts.Arcs[0].To == Parts.Arcs[1].From;
				if (!bReciprocalPair)
				{
					Errors.Add(FString::Printf(
						TEXT("Error: identita' di struttura duplicata '%s' su %d archi non reciproci"),
						*Name.ToString(), Parts.Arcs.Num()));
				}
			}
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
	// v4 -> v5 (CP 9.4): gli archi guadagnano stato, integrita' e conduttivita'. I default sono quelli di un
	// ponte SANO (`Active`, 40, non conduttivo): un arco letto da un asset vecchio non deve risultare spento,
	// o la mappa cambierebbe significato solo per essere stata ricaricata.
	// v7 -> v8 (#621): le coperture guadagnano la PROVENIENZA (`bGenerated`). Il default `false` significa
	// «dipinta a mano», che e' esattamente cio' che ogni copertura scritta prima di questo campo era: nessun
	// dato cambia significato, e non c'e' nulla da trasformare.
	// v5 -> v6 (CP 19.1): il formato guadagna la classe di mappa. Il default (`Skirmish`) e' cio' che una
	// mappa scritta prima gia' era — il vertical slice e' 2v2 — quindi anche qui non c'e' niente da convertire.
	// v6 -> v7 (#619): la cella guadagna il sovrapprezzo di occupazione. Il default (0) e' cio' che una mappa
	// scritta prima gia' era — nessuna geometria cotta, nessuna cella stretta — quindi niente da convertire.
	// v8 -> v9 (#832, CP 23.3): porte e archi guadagnano un nome pubblico stabile (`StableId`). Il default
	// (`NAME_None`) e' cio' che una mappa scritta prima gia' era — nessuna struttura nominata — quindi anche
	// qui non c'e' niente da convertire, e nessun nome viene inventato da una ricarica.
	// In nessuno di questi c'e' qualcosa da convertire — il campo nuovo nasce vuoto e una mappa che non lo usa
	// si comporta esattamente come prima — quindi la migrazione si limita a dichiarare la versione. Il giorno
	// in cui una migrazione dovra' TRASFORMARE dati, il posto e' questo, un `if (FormatVersion < N)` per
	// volta, in ordine.
	//
	// ✅ E da #687 quel giorno e' possibile: `FormatVersion` arriva qui dai BYTE (via `FRTHexMapCustomVersion`,
	// letta in `Serialize`) e non piu' dal default del CDO. Prima di allora questa funzione era inerte per
	// gli asset che avrebbe dovuto proteggere — usciva alla prima riga credendosi gia' aggiornata.
	FormatVersion = CurrentFormatVersion;
}

void URTHexMapAsset::Serialize(FArchive& Ar)
{
	// Il lato-SCRITTURA: e' questa chiamata che fa finire GUID e versione nel summary del package, ed e'
	// l'unica ragione per cui il numero viaggia. In lettura e' un no-op dichiarato (`FArchive::
	// UsingCustomVersion` esce subito se `IsLoading`): li' il container arriva gia' pieno dal summary, ed
	// e' cio' che `CustomVer` interroga qui sotto. Sta in cima al metodo per leggibilita', non perche'
	// l'ordine rispetto a `Super::Serialize` sia vincolante.
	Ar.UsingCustomVersion(FRTHexMapCustomVersion::GUID);

	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		// -1 = l'archivio non porta questa chiave, cioe' l'asset e' stato scritto PRIMA che il meccanismo
		// esistesse. Non e' «versione 1»: e' «non lo sappiamo», e si riparte dall'inizio della catena.
		// Vedi `FRTHexMapCustomVersion::BeforeCustomVersionWasAdded` per il perche' oggi costa zero.
		const int32 Declared = Ar.CustomVer(FRTHexMapCustomVersion::GUID);
		LoadedFormatVersion = (Declared < 0) ? FRTHexMapCustomVersion::BeforeCustomVersionWasAdded : Declared;

		// La property e' il canale di lettura del resto del codice, ma non e' la fonte: il valore che
		// `Super::Serialize` le ha appena dato viene dal CDO, non dai byte (#687). Si sovrascrive con la
		// versione vera, altrimenti `MigrateToCurrentFormat` torna a credersi gia' aggiornata.
		FormatVersion = LoadedFormatVersion;
	}
}

void URTHexMapAsset::PostLoad()
{
	Super::PostLoad();
	MigrateToCurrentFormat();
}
