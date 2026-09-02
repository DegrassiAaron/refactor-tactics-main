#include "Map/RTHexMapAsset.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapCustomVersion.h"
// `ValidateMap` chiama le regole del grafo di interazione invece di riscriverle: `URTStructureIdentityLibrary`
// e' l'unico punto di lettura da nome a struttura (#832), e un secondo validator sarebbe una doppia verita'.
#include "Map/RTStructureIdentityLibrary.h"
// I muri interni si validano con le stesse funzioni che li producono: grammatica e cottura (#712, v10).
#include "Map/RTGeometryBake.h"
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

bool URTHexMapAsset::ReplaceContent(const TArray<FRTHexCellData>& InCells,
	const TArray<FRTHexEdge>& InTransitions)
{
	if (InCells.Num() == 0 && InTransitions.Num() == 0)
	{
		ClearAll(); // sostituire con NIENTE e' uno svuotamento: la guardia sta li', non duplicata qui
		return false;
	}

	Cells = InCells;
	Transitions = InTransitions;
	bLookupDirty = true; // gli indici non sopravvivono a una sostituzione
	++Revision;          // UNA volta: rimpiazzare la mappa e' un evento, non N
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

namespace
{
	/**
	 * L'identita' stabile di una struttura, nella forma che entra in `ComputeHash` (#986).
	 *
	 * Mescola il **testo normalizzato**, non `GetTypeHash(FName)`: quello e' l'indice della name table, che
	 * dipende dall'ordine in cui i nomi sono stati creati nel processo. E `ToLower()` perche'
	 * `FName::operator==` e' case-insensitive e `URTStructureIdentityLibrary` risolve i bersagli con quello:
	 * `Door.Atrio` e `door.atrio` sono la stessa porta per ogni consumatore, quindi due mappe che si giocano
	 * identiche non possono avere hash diversi.
	 *
	 * ⚠️ E' lo **stesso criterio** di `URTMatchStateHashLibrary::MixName`, scritto due volte invece che
	 * condiviso: `Map/` non puo' dipendere da `Turn/` — la freccia va nell'altro verso, ed e' `RTMatchStateHash.cpp`
	 * a includere `Map/RTHexMapAsset.h`. Se i due criteri divergono, divergono i due hash sullo stesso campo,
	 * che e' precisamente il difetto che #986 e' venuta a chiudere: chi tocca l'uno guardi l'altro.
	 */
	uint32 HashStableId(FName StableId)
	{
		return GetTypeHash(StableId.ToString().ToLower());
	}
}

bool URTHexMapAsset::HasObjectiveCell() const
{
	for (const FRTHexCellData& C : Cells)
	{
		if (C.bIsObjective)
		{
			return true;
		}
	}
	return false;
}

FRTCellId URTHexMapAsset::FirstObjectiveCell() const
{
	FRTCellId Best;
	bool bFound = false;
	for (const FRTHexCellData& C : Cells)
	{
		if (!C.bIsObjective)
		{
			continue;
		}
		if (!bFound || URTHexLibrary::StableLess(C.Id, Best))
		{
			Best = C.Id;
			bFound = true;
		}
	}
	return Best;
}

void URTHexMapAsset::SortInteriorWallsCanonically(TArray<FRTHexInteriorWall>& Walls)
{
	Walls.Sort([](const FRTHexInteriorWall& A, const FRTHexInteriorWall& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		if (A.Segment.Axis != B.Segment.Axis)
		{
			return static_cast<uint8>(A.Segment.Axis) < static_cast<uint8>(B.Segment.Axis);
		}
		if (A.Segment.Offset != B.Segment.Offset) { return A.Segment.Offset < B.Segment.Offset; }
		if (A.Segment.Layer != B.Segment.Layer) { return A.Segment.Layer < B.Segment.Layer; }
		const int32 AMin = FMath::Min(A.Segment.AlongStart, A.Segment.AlongEnd);
		const int32 BMin = FMath::Min(B.Segment.AlongStart, B.Segment.AlongEnd);
		if (AMin != BMin) { return AMin < BMin; }
		return FMath::Max(A.Segment.AlongStart, A.Segment.AlongEnd)
			< FMath::Max(B.Segment.AlongStart, B.Segment.AlongEnd);
	});
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
		// L'obiettivo entra con lo stesso criterio dei due booleani sopra (formato v11, #75): cambia CHI
		// VINCE, quindi due mappe che si giocano diversamente non possono avere lo stesso hash. E' anche
		// cio' che permette a `IsSnapshotStale` di accorgersi di un obiettivo spostato.
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(C.bIsObjective ? 1 : 0)));

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
			// L'IDENTITA' STABILE entra con lo stesso argomento di `DoorId` due righe sopra (#986, dopo #832).
			// Il criterio di questo hash e' scritto accanto ai campi che ne restano FUORI — `MapClass` in
			// `RTHexMapAsset.h`, `bGenerated` in `RTHexCellData.h`: ci entra cio' che puo' cambiare un esito.
			// `URTStructureIdentityLibrary::FindDoorEdges` risolve i bersagli **per nome**, quindi rinominare
			// una porta cambia quale bordo si apre per chiunque la citi. Senza questa riga `IsSnapshotStale`
			// lascia «fresco» uno snapshot in cache dopo un rename che cambia proprio quella risoluzione.
			Hash = HashCombine(Hash, HashStableId(Door.StableId));
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
		// Gli ARCHI, non le sole porte: e' lo stesso problema di identita' (#832), e lasciarlo fuori di qua
		// avrebbe rifatto per gli archi l'asimmetria che #986 e' venuta a chiudere per le porte. Un ponte
		// citato per nome da uno scenario cambia bersaglio se lo si rinomina.
		// Copertura: `RefactorTactics.Simulation.MapHashSeesArcIdentity`.
		Hash = HashCombine(Hash, HashStableId(E.StableId));
	}

	// Il GRAFO DI INTERAZIONE (CP 23.4, #833). Non entrava, e il criterio di esclusione che l'header dichiara
	// — «non tocca la geometria ne' il comportamento» — qui e' falso: scambiare `S1 -> {D1, D2}` con
	// `S1 -> {D2, D1}` cambia l'ordine di APPLICAZIONE dichiarato, e togliere un binding cambia quali porte si
	// aprono. Due mappe che si giocano diverso hashavano identiche, quindi `IsSnapshotStale` e il confronto di
	// determinismo non potevano vedere la divergenza. Trovato da una code review, non da un test.
	//
	// ⚠️ **Due ordini, trattati in modo OPPOSTO, ed e' la parte da non sbagliare:**
	//   · i binding FRA LORO si ordinano per `SourceId` — la risoluzione li cerca per nome, quindi l'ordine
	//     nell'array non e' dato, ed e' la stessa ragione per cui `Cells` viene ordinato piu' sopra;
	//   · i `TargetIds` DENTRO un binding **non** si ordinano: quello e' l'ordine di applicazione, cioe'
	//     esattamente il dato che #833 difende. Ordinarli renderebbe l'hash cieco alla proprieta' che deve
	//     proteggere — pinnato da `InteractionGraph.OrderChangesMapHash` e dal suo gemello
	//     `BindingInsertionOrderDoesNotChangeHash`, che senza questa distinzione passerebbero entrambi con la
	//     scelta sbagliata.
	TArray<FRTInteractionBinding> SortedBindings = InteractionBindings;
	SortedBindings.Sort([](const FRTInteractionBinding& A, const FRTInteractionBinding& B)
	{
		return A.SourceId.LexicalLess(B.SourceId);
	});
	for (const FRTInteractionBinding& B : SortedBindings)
	{
		Hash = HashCombine(Hash, HashStableId(B.SourceId));
		for (const FName& T : B.TargetIds)
		{
			Hash = HashCombine(Hash, HashStableId(T));
		}
	}

	// I MURI INTERNI, da `#1830` — e fino a quel giorno restavano fuori con una motivazione scritta sul tipo
	// (`RTHexMapAsset.h`) che diceva *«vista e passo oggi non lo consultano […] il giorno in cui un muro
	// interno dovra' bloccare la linea di vista […] allora, ma solo allora, questo tipo entrera' nell'hash»*.
	// `D-269` e' quella decisione, e `URTHexOcclusionLibrary` e' il consumatore: il criterio di questo hash —
	// ci entra cio' che puo' cambiare un ESITO — ora li include.
	//
	// 🔴 **Lasciarli fuori sarebbe un FALSO NEGATIVO, non un'omissione estetica**: due mappe che si giocano
	// diversamente avrebbero lo stesso hash, `IsSnapshotStale` lascerebbe «fresco» uno snapshot dopo che un
	// muro si e' spostato, e una divergenza di replay diventerebbe non diagnosticabile — l'inverso del KPI
	// `replay divergence = 0`, e la meta' peggiore.
	//
	// ⛔ **`StableId` NON entra**, ed e' il criterio di sempre applicato al caso opposto: `FRTHexDoor::StableId`
	// ci entra perche' `FindDoorEdges` risolve i bersagli **per nome**, mentre nessuno risolve un muro interno
	// per nome a runtime — lo fa solo l'editor. Rinominarlo non cambia nessun esito.
	//
	// L'ordine dell'array lo decide chi edita l'asset, quindi si ordina prima di mescolare, come per `Covers`
	// e `Transitions`. Gli estremi entrano come coppia NON ordinata: e' lo stesso segmento anche percorso al
	// contrario, ed e' gia' la regola del suo `operator==`.
	TArray<FRTHexInteriorWall> SortedWalls = InteriorWalls;
	SortInteriorWallsCanonically(SortedWalls);
	for (const FRTHexInteriorWall& Wall : SortedWalls)
	{
		Hash = HashCombine(Hash, GetTypeHash(Wall.Cell));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(Wall.Segment.Axis)));
		Hash = HashCombine(Hash, GetTypeHash(Wall.Segment.Offset));
		Hash = HashCombine(Hash, GetTypeHash(Wall.Segment.Layer));
		Hash = HashCombine(Hash, GetTypeHash(FMath::Min(Wall.Segment.AlongStart, Wall.Segment.AlongEnd)));
		Hash = HashCombine(Hash, GetTypeHash(FMath::Max(Wall.Segment.AlongStart, Wall.Segment.AlongEnd)));
		// Il TIPO decide se il muro occlude (`D-271`: solo `High`), quindi cambia un esito ed entra.
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(Wall.Segment.WallType)));
		// La SCAVALCABILITA' decide se si passa (`E23.7`, `D-308`): stesso criterio, stesso hash. Due mappe
		// identiche salvo un muro superabile si giocano diversamente.
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint32>(Wall.bTraversable ? 1 : 0)));
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
				// Un riparo a zero punti struttura e' un riparo gia' distrutto: va tolto, non tenuto a 0.
				// Chi lo scala e' `URTHexCoverLibrary::ApplyStructureDamage`, e a zero RIMUOVE l'entry invece
				// di lasciarla: quindi questo caso non lo produce il gioco, lo produce solo la mano che
				// autora. ⚠️ Diceva «CP 9.2 lo scalera' davvero» al futuro, e CP 9.2 e' chiuso dal
				// 2026-08-07 (#1320).
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

		// LA FACCIA RIDONDANTE DI UN BORDO CONDIVISO — `GEO-7` di `D-288`, `#1893`.
		//
		// ⚠️ **La regola qui sopra guarda DENTRO una cella; questa guarda FRA due.** Un bordo appartiene a
		// due celle, e ciascuna puo' dichiararvi la propria faccia: `URTHexCoverLibrary::CoverBetween` le
		// legge entrambe e tiene la piu' alta — *«la barriera e' fisica, non un attributo di chi la
		// possiede»* — quindi la seconda non cambia nulla nel gioco.
		//
		// 🔴 **Ma cambia l'hash.** Le coperture entrano in `ComputeHash`, ordinate per bordo: due mappe che
		// si giocano in modo identico ne hanno uno diverso solo perche' una dichiara il muro da tutti e due
		// i lati. E' il falso positivo contro `replay divergence = 0` che l'intestazione di
		// `RebakeTouchesOnlyTheInvestedRegion` (#883) gia' nomina: il bake non scrive mai la seconda faccia
		// **proprio per questo**, ma la mano che autora puo' scriverla, e finora nessuno se ne accorgeva.
		//
		// 🔵 **Warning e non Error**, con il precedente di forma sedici righe piu' in basso — *«transizione
		// ridondante tra celle gia' adiacenti»*: lo stato e' legale e risolto, ed e' ridondante, non illegale.
		// Correggerlo da soli sarebbe l'auto-fix silenzioso che il Decision Record vieta.
		//
		// La segnalazione esce UNA volta per coppia: la produce solo il lato che `StableLess` mette per
		// primo — lo stesso ordinamento con cui `SortCells` e `ComputeHash` rendono deterministico il resto.
		for (const FRTHexCover& Cover : C.Covers)
		{
			const FRTCellId Far = URTHexLibrary::Neighbor(C.Id, Cover.Edge);
			if (!URTHexLibrary::StableLess(C.Id, Far))
			{
				continue;
			}
			const FRTHexCellData* Opposite = FindCell(Far);
			if (Opposite == nullptr)
			{
				continue; // il vicino non esiste: nessuna seconda faccia da dichiarare
			}
			if (Opposite->CoverOn(URTHexLibrary::OppositeDirection(Cover.Edge)) != ERTHexCoverType::None)
			{
				Errors.Add(FString::Printf(
					TEXT("Warning: copertura ridondante sulla faccia opposta del bordo condiviso %s / %s"),
					*C.Id.ToString(), *Far.ToString()));
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

	// Il GRAFO DI INTERAZIONE (CP 23.4, #833). Le cinque regole vivono in `URTStructureIdentityLibrary`, che
	// e' l'unico punto di lettura da nome a struttura: qui si CHIAMANO, non si riscrivono — un secondo
	// validator sarebbe la doppia verita' che quella libreria esiste per impedire.
	//
	// 🔴 **Senza questa riga le cinque regole erano raggiungibili SOLO dai test**, trovato da una code review:
	// una mappa spedita con un bersaglio fantasma, un binding duplicato, uno riflessivo o un `TargetIds` vuoto
	// passava `ValidateMap()`, e a runtime `FindDoorEdges` restituiva un array vuoto — indistinguibile da una
	// sorgente che non comanda nulla. Le regole erano il valore dichiarato del lavoro e nessun percorso di
	// produzione poteva attivarle.
	Errors.Append(URTStructureIdentityLibrary::ValidateInteractionGraph(this));

	// I MURI INTERNI (formato v10, #712). Ogni altro array d'autore ha le sue regole qui dentro — celle,
	// coperture, porte, transizioni, `StableId` — e il campo nuovo era l'unico senza.
	//
	// ⚠️ Non e' zelo: il campo e' `EditAnywhere`, quindi il Property Editor ci arriva senza passare da
	// `AddSegmentsToCell`. Le difese scritte nella cottura valgono per **un** chiamante; queste per l'asset.
	for (int32 Index = 0; Index < InteriorWalls.Num(); ++Index)
	{
		const FRTHexInteriorWall& Wall = InteriorWalls[Index];

		// 1. Orfano: un muro in una cella che non esiste. Stessa regola di `Transitions`.
		if (FindCell(Wall.Cell) == nullptr)
		{
			Errors.Add(FString::Printf(TEXT("Error: muro interno %d su cella inesistente %s"),
				Index, *Wall.Cell.ToString()));
		}

		// 2. Fuori grammatica.
		const ERTGeometryViolation Violation = URTGeometryGrammarLibrary::ValidateSegment(Wall.Segment);
		if (Violation != ERTGeometryViolation::None)
		{
			Errors.Add(FString::Printf(TEXT("Error: muro interno %d fuori grammatica su %s (violazione %d)"),
				Index, *Wall.Cell.ToString(), static_cast<int32>(Violation)));
		}

		// 3. Duplicato. Confronto con `operator==`, che tratta gli estremi come coppia NON ordinata: senza,
		//    lo stesso muro percorso al contrario passerebbe — ed e' il difetto che una code review ha
		//    trovato nella cottura, dove il confronto era stato riscritto a mano campo per campo.
		for (int32 Previous = 0; Previous < Index; ++Previous)
		{
			if (InteriorWalls[Previous].Cell == Wall.Cell && InteriorWalls[Previous].Segment == Wall.Segment)
			{
				Errors.Add(FString::Printf(TEXT("Error: muro interno %d duplicato di %d su %s"),
					Index, Previous, *Wall.Cell.ToString()));
				break;
			}
		}

		// 4. L'INVARIANTE dichiarato nell'header: ci finisce solo cio' che nessuna copertura descrive. Un
		//    segmento che chiude un bordo e' gia' rappresentato dalla sua copertura, e scriverlo anche qui
		//    sarebbe una seconda verita' sullo stesso muro — che e' la ragione per cui questo campo ha un
		//    perimetro invece di essere «i segmenti della cella».
		TArray<ERTHexDirection> Edges;
		URTGeometryBakeLibrary::EdgesTouchedBy(Wall.Segment, HexSize, Edges);
		if (Edges.Num() > 0)
		{
			Errors.Add(FString::Printf(
				TEXT("Error: muro interno %d su %s chiude %d bordi: e' una copertura, non un muro interno"),
				Index, *Wall.Cell.ToString(), Edges.Num()));
		}
	}

	// 5. L'INCIDENZA FRA DUE MURI DELLA STESSA CELLA — `#1894`, `GEO-6` di `D-288`.
	//
	// 🔴 **Il rilievo che ha deciso dove va questo blocco.** `URTGeometryGrammarLibrary::Validate` non aveva
	// un solo chiamante di produzione: `git grep` lo trovava tre volte, tutte e tre in un file di test. Le
	// regole d'incidenza scritte solo li' sarebbero nate morte insieme allo strato che le ospita — nessun
	// asset le avrebbe mai attivate.
	//
	// ⚠️ **Il raggruppamento per CELLA non e' un'ottimizzazione, e' la correttezza.** I segmenti sono in
	// coordinate LOCALI di cella: due muri di celle diverse con gli stessi numeri non si incrociano affatto,
	// e validare l'array intero in un colpo solo li segnalerebbe tutti — il difetto piu' facile da
	// introdurre qui, e quello che `ReachesValidateMap` pinna con la sua controprova.
	//
	// L'ordine delle celle e' quello di PRIMA APPARIZIONE nell'array, non quello di una `TMap`: l'iterazione
	// di una `TMap` non e' deterministica (invariante n. 4), e due validazioni della stessa mappa darebbero
	// gli stessi errori in ordine diverso.
	//
	// Si filtrano le sole violazioni di RELAZIONE: `ValidateSegment` e il duplicato hanno gia' le loro regole
	// qui sopra, e riemetterli da qui sarebbe la stessa segnalazione due volte con due formati.
	{
		TArray<FRTCellId> Order;
		TMap<FRTCellId, TArray<int32>> ByCell;
		for (int32 Index = 0; Index < InteriorWalls.Num(); ++Index)
		{
			TArray<int32>& Group = ByCell.FindOrAdd(InteriorWalls[Index].Cell);
			if (Group.Num() == 0)
			{
				Order.Add(InteriorWalls[Index].Cell);
			}
			Group.Add(Index);
		}

		for (const FRTCellId& Cell : Order)
		{
			const TArray<int32>& Group = ByCell[Cell];
			if (Group.Num() < 2)
			{
				continue; // un muro solo non ha con chi incidere
			}

			TArray<FRTGeometrySegment> Segments;
			Segments.Reserve(Group.Num());
			for (const int32 Index : Group)
			{
				Segments.Add(InteriorWalls[Index].Segment);
			}

			TArray<FRTGeometryIssue> Issues;
			URTGeometryGrammarLibrary::Validate(Segments, Issues);

			for (const FRTGeometryIssue& Issue : Issues)
			{
				if (Issue.Violation != ERTGeometryViolation::CrossingOffAnchor
					&& Issue.Violation != ERTGeometryViolation::OverlappingSegments)
				{
					continue;
				}

				// Gli indici tornano a essere quelli dell'ARRAY, non del gruppo: chi legge l'errore apre
				// `InteriorWalls` a quella posizione, e un indice locale lo manderebbe sul muro sbagliato.
				const int32 Mine = Group[Issue.SegmentIndex];
				const int32 Other = Issue.OtherIndex != INDEX_NONE ? Group[Issue.OtherIndex] : INDEX_NONE;

				if (Issue.Violation == ERTGeometryViolation::CrossingOffAnchor)
				{
					Errors.Add(FString::Printf(
						TEXT("Error: muri interni %d e %d su %s: incrocio fuori da un anchor"),
						Mine, Other, *Cell.ToString()));
				}
				else
				{
					Errors.Add(FString::Printf(
						TEXT("Error: muri interni %d e %d su %s si sovrappongono"),
						Mine, Other, *Cell.ToString()));
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
	// v9 -> v10 (#712, seduta `U22`): la mappa guadagna i MURI INTERNI, la geometria che non giace su nessun
	// bordo. L'elenco nasce vuoto, ed e' cio' che una mappa scritta prima gia' era: quei muri non si potevano
	// disegnare, e il segmento che li avrebbe descritti veniva scartato dalla cottura senza dirlo.
	// v10 -> v11 (#75, CP 10.2): la cella guadagna il flag di OBIETTIVO contendibile. Il default `false` e'
	// cio' che una mappa scritta prima gia' era: nessuna sua cella faceva punto, e la partita si decideva per
	// eliminazione o al limite di round. Un obiettivo NON viene inventato da una ricarica.
	// v13 -> v14 (#1828, `E23.7`): il muro interno guadagna la SCAVALCABILITA' (`bTraversable`). Il default
	// `false` e' cio' che ogni muro scritto prima gia' era, e non per omissione: fino a `D-308` la
	// scavalcabilita' non esisteva come dato, e nessuna geometria poteva dichiararsi superabile.
	// ⛔ **Una ricarica non deduce il campo dall'altezza**: un `Low` non diventa scavalcabile migrando, ed e'
	// precisamente cio' che `D-308` vieta — *«la scavalcabilita' e' un dato, non una conseguenza dell'altezza»*.
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
