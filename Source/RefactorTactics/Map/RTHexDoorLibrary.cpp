#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTStructureIdentityLibrary.h"

namespace
{
	/**
	 * Quanto uno stato RESTRINGE il bordo. Non e' l'ordine dell'enum: `Destroyed` viene dopo `Locked` ma non
	 * blocca niente. Serve a due domande diverse — quale faccia decide, e quale ordine vince a parita' di
	 * bordo — che devono rispondere con lo stesso criterio.
	 */
	int32 Restriction(ERTHexDoorState State)
	{
		switch (State)
		{
		case ERTHexDoorState::Locked:    return 2;
		case ERTHexDoorState::Closed:    return 1;
		case ERTHexDoorState::Open:      return 0;
		case ERTHexDoorState::Destroyed: return 0;
		}
		return 0;
	}

	/** Cella oltre il bordo `Edge` di `Cell` (stesso layer). */
	FRTCellId NeighborAcross(const FRTCellId& Cell, ERTHexDirection Edge)
	{
		const FIntPoint D = URTHexLibrary::AxialDirection(Edge);
		return FRTCellId(Cell.X + D.X, Cell.Y + D.Y, Cell.Layer);
	}

	/**
	 * Vero se la transizione e' ammessa. Sono REGOLE DI GIOCO, non stati del dato, e stanno qui perche' questo
	 * e' l'unico ingresso di mutazione: chi le bypassasse produrrebbe una porta sfondata che si richiude.
	 */
	bool CanTransition(ERTHexDoorState Current, ERTHexDoorState Wanted)
	{
		if (Current == Wanted)
		{
			return false; // gia' li': non e' un cambio, e la revisione non deve muoversi
		}
		if (Current == ERTHexDoorState::Destroyed)
		{
			return false; // terminale: una porta sfondata non si richiude
		}
		if (Current == ERTHexDoorState::Locked && Wanted == ERTHexDoorState::Open)
		{
			return false; // non si apre da sola: serve l'apertura autorizzata di CP 10.1
		}
		return true;
	}

	/** Nome leggibile di uno stato, per i reason code. Uno `switch` e non la reflection: `StaticEnum` dipende
	 *  dai nomi generati, che in packaged sono un'altra cosa — e un reason code che cambia col target non e'
	 *  osservabile. */
	const TCHAR* StateName(ERTHexDoorState State)
	{
		switch (State)
		{
		case ERTHexDoorState::Open:      return TEXT("Open");
		case ERTHexDoorState::Closed:    return TEXT("Closed");
		case ERTHexDoorState::Locked:    return TEXT("Locked");
		case ERTHexDoorState::Destroyed: return TEXT("Destroyed");
		}
		return TEXT("?");
	}

	/** Ordine canonico delle voci: da qui escono righe di TurnLog, e due esecuzioni le scrivono uguali. */
	void SortDoorChanges(TArray<FRTDoorChange>& Changes)
	{
		Changes.Sort([](const FRTDoorChange& A, const FRTDoorChange& B)
		{
			if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
			return URTHexLibrary::StableLess(A.Toward, B.Toward);
		});
	}

	/**
	 * Commuta un bordo dentro un array di celle **di lavoro**, senza toccare la mappa.
	 *
	 * ⚠️ **Il commit e' del chiamante, e questa e' l'unica ragione per cui la funzione esiste.** Finche' la
	 * commutazione e la scrittura stavano insieme, N bersagli producevano N revisioni: il DoD di #833 ne
	 * chiede **una** per l'intera operazione, e chi la osserva non deve vedere tre cambi dove ce n'e' stato
	 * uno. Separandole, `SetDoorState` resta il caso `N = 1` e l'interazione su piu' bersagli commette in
	 * fondo — senza un secondo ingresso di mutazione, che e' l'invariante che questa libreria difende.
	 *
	 * `OutTouched` accumula gli INDICI delle celle modificate, non le celle: due bersagli che condividono una
	 * cella devono comporsi sulla stessa copia, e portarsi via una copia per bersaglio farebbe vincere
	 * l'ultimo — cioe' perdere in silenzio il cambio del primo.
	 */
	TArray<FRTDoorChange> CommutateEdge(TArray<FRTHexCellData>& Work, const FRTCellId& From,
		const FRTCellId& To, ERTHexDoorState State, TSet<int32>& OutTouched)
	{
		TArray<FRTDoorChange> Changes;

		ERTHexDirection Forward = ERTHexDirection::E;
		ERTHexDirection Backward = ERTHexDirection::E;
		if (!URTHexCoverLibrary::EdgeDirection(From, To, Forward)
			|| !URTHexCoverLibrary::EdgeDirection(To, From, Backward))
		{
			return Changes; // non adiacenti: nessun bordo da commutare
		}

		// Il GRUPPO, se una delle due facce lo dichiara: un portone largo si commuta tutto insieme, altrimenti
		// chi lo comanda dovrebbe sapere da se' di quali bordi e' fatto — conoscenza che finirebbe nel chiamante
		// invece che nel dato.
		int32 GroupId = INDEX_NONE;
		for (const FRTHexCellData& Cell : Work)
		{
			if (Cell.Id == From)
			{
				if (const FRTHexDoor* Door = Cell.DoorOn(Forward)) { GroupId = Door->DoorId; }
				break;
			}
		}
		if (GroupId == INDEX_NONE)
		{
			for (const FRTHexCellData& Cell : Work)
			{
				if (Cell.Id == To)
				{
					if (const FRTHexDoor* Door = Cell.DoorOn(Backward)) { GroupId = Door->DoorId; }
					break;
				}
			}
		}

		// Si scandisce l'array delle celle nel suo ordine, che e' stabile: due esecuzioni della stessa partita
		// producono le stesse voci nella stessa sequenza.
		for (int32 Index = 0; Index < Work.Num(); ++Index)
		{
			FRTHexCellData& Cell = Work[Index];
			for (FRTHexDoor& Door : Cell.Doors)
			{
				// Con un gruppo contano tutte le voci che lo dichiarano, ovunque siano; senza, solo le due facce
				// di QUESTO bordo — e in entrambi i casi si scrive su ogni faccia dichiarata, cosi' le due non
				// possono divergere.
				const bool bInOperation = (GroupId != INDEX_NONE)
					? (Door.DoorId == GroupId)
					: ((Cell.Id == From && Door.Edge == Forward) || (Cell.Id == To && Door.Edge == Backward));
				if (!bInOperation || !CanTransition(Door.State, State))
				{
					continue;
				}

				Door.State = State;
				OutTouched.Add(Index);

				FRTDoorChange Change;
				Change.Cell = Cell.Id;
				Change.Toward = NeighborAcross(Cell.Id, Door.Edge);
				Change.State = State;
				Change.bBlocking = URTHexDoorLibrary::StateBlocks(State);
				Changes.Add(Change);
			}
		}
		return Changes;
	}

	/**
	 * Scrive sulla mappa le celle toccate: **una** chiamata, quindi **una** revisione.
	 *
	 * ⚠️ L'ordine e' quello di `Work`, non quello di `Touched`: si scandisce l'array e si chiede al set se
	 * l'indice c'e'. Iterare il `TSet` darebbe lo stesso INSIEME in un ordine che dipende dall'hash — l'esatta
	 * dipendenza che l'invariante n. 3 vieta, e che qui finirebbe dentro le voci scritte nell'asset.
	 */
	void CommitTouched(URTHexMapAsset* Map, const TArray<FRTHexCellData>& Work, const TSet<int32>& Touched)
	{
		if (Map == nullptr || Touched.Num() == 0)
		{
			return; // una transizione rifiutata non e' un cambiamento della mappa: la revisione non si muove
		}

		TArray<FRTHexCellData> Updated;
		Updated.Reserve(Touched.Num());
		for (int32 Index = 0; Index < Work.Num(); ++Index)
		{
			if (Touched.Contains(Index))
			{
				Updated.Add(Work[Index]);
			}
		}
		Map->UpdateCells(Updated);
	}
}

ERTHexDoorState URTHexDoorLibrary::DoorBetween(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To)
{
	if (Map == nullptr)
	{
		return ERTHexDoorState::Open;
	}

	ERTHexDirection Forward = ERTHexDirection::E;
	ERTHexDirection Backward = ERTHexDirection::E;
	if (!URTHexCoverLibrary::EdgeDirection(From, To, Forward)
		|| !URTHexCoverLibrary::EdgeDirection(To, From, Backward))
	{
		return ERTHexDoorState::Open; // non adiacenti: non condividono un bordo
	}

	// Il bordo ha due facce e la barriera e' fisica: vale la piu' RESTRITTIVA, cosi' la regola non dipende da
	// quale delle due celle ha ricevuto il dato (stessa scelta delle coperture, issue #70).
	ERTHexDoorState Best = ERTHexDoorState::Open;
	if (const FRTHexCellData* Near = Map->FindCell(From))
	{
		if (const FRTHexDoor* Door = Near->DoorOn(Forward))
		{
			if (Restriction(Door->State) > Restriction(Best)) { Best = Door->State; }
		}
	}
	if (const FRTHexCellData* Far = Map->FindCell(To))
	{
		if (const FRTHexDoor* Door = Far->DoorOn(Backward))
		{
			if (Restriction(Door->State) > Restriction(Best)) { Best = Door->State; }
		}
	}
	return Best;
}

bool URTHexDoorLibrary::BlocksBetween(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
{
	return StateBlocks(DoorBetween(Map, From, To));
}

TArray<FRTDoorChange> URTHexDoorLibrary::SetDoorState(URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To, ERTHexDoorState State)
{
	TArray<FRTDoorChange> Changes;
	if (Map == nullptr)
	{
		return Changes;
	}

	// Il caso `N = 1` della commutazione: si lavora su una copia e si commette in fondo. La sequenza e'
	// identica a quella di prima — stesse voci, stesso ordine, UNA sola revisione per l'intero gruppo — ma il
	// calcolo ora vive in un posto solo, che e' cio' che permette all'interazione su N bersagli di comporsi
	// senza aprire un secondo ingresso di mutazione.
	TArray<FRTHexCellData> Work = Map->Cells;
	TSet<int32> Touched;
	Changes = CommutateEdge(Work, From, To, State, Touched);
	CommitTouched(Map, Work, Touched);

	SortDoorChanges(Changes);
	return Changes;
}

TArray<FRTDoorChange> URTHexDoorLibrary::ApplyInteraction(URTHexMapAsset* Map, FName SourceId,
	ERTHexDoorState State, int32 ActorId, TArray<FString>* OutRefusals)
{
	TArray<FRTDoorChange> Changes;
	if (Map == nullptr || SourceId.IsNone())
	{
		return Changes;
	}

	// La risoluzione decide CHI e' comandato — compreso il rifiuto per bersaglio conteso (`INT-5`), che
	// arriva qui come reason code e non come lista vuota senza spiegazione.
	TArray<FString> ResolveErrors;
	const TArray<FRTStructureEdgeRef> Targets =
		URTStructureIdentityLibrary::ResolveInteractionTargets(Map, SourceId, &ResolveErrors);
	if (OutRefusals != nullptr)
	{
		OutRefusals->Append(ResolveErrors);
	}
	if (Targets.Num() == 0)
	{
		return Changes; // sorgente senza binding: legale, e non fa niente
	}

	TArray<FRTHexCellData> Work = Map->Cells;
	TSet<int32> Touched;
	for (const FRTStructureEdgeRef& Target : Targets)
	{
		// Lo stato si legge PRIMA di commutare e sulla copia di lavoro, non sulla mappa: un bersaglio
		// precedente puo' aver gia' mosso questo stesso bordo — un portone largo condiviso — e un reason code
		// che citasse lo stato iniziale spiegherebbe un rifiuto con un fatto non piu' vero.
		ERTHexDoorState Before = ERTHexDoorState::Open;
		FName TargetName;
		for (const FRTHexCellData& Cell : Work)
		{
			if (Cell.Id == Target.Cell)
			{
				if (const FRTHexDoor* Door = Cell.DoorOn(Target.Edge))
				{
					Before = Door->State;
					TargetName = Door->StableId;
				}
				break;
			}
		}

		TArray<FRTDoorChange> Local = CommutateEdge(Work, Target.Cell,
			NeighborAcross(Target.Cell, Target.Edge), State, Touched);

		if (Local.Num() == 0)
		{
			// 🔴 **«Nessun cambio» non significa «rifiutato», e confonderli produce falsi rifiuti su un caso
			// REALE: il portone largo.** `FindDoorEdges` restituisce TUTTI i bordi che portano quel nome, quindi
			// un portone di tre bordi arriva qui come tre bersagli; il primo li commuta tutti e tre insieme —
			// `CommutateEdge` propaga sul `DoorId` — e i due successivi trovano il lavoro gia' fatto. Riportarli
			// direbbe al giocatore che due terzi del portone hanno rifiutato, mentre e' aperto.
			//
			// Il discrimine e' lo stato che il bordo ha ORA: se e' gia' quello richiesto, l'operazione ha
			// ottenuto cio' che voleva su quel bersaglio — che sia stata lei un attimo fa o che ci fosse gia'.
			// Se e' un altro, la transizione e' stata negata davvero (`Locked -> Open`, `Destroyed -> qualunque`)
			// ed e' il caso che [D-150] vuole riportato.
			if (OutRefusals != nullptr && Before != State)
			{
				// ⚠️ **Si riporta e si prosegue** ([D-150]): l'operazione su N bersagli NON e' atomica, applica gli
				// applicabili e dichiara gli altri. E' il comportamento che il motore ha gia' un livello sotto —
				// `CanTransition` salta il bordo che non puo' transitare e commuta il resto — invece di
				// contraddirlo con una pre-validazione «tutto o niente» che nessuno ha chiesto.
				OutRefusals->Add(FString::Printf(
					TEXT("Refused: il bersaglio '%s' su %s e' %s e non transita a %s"),
					TargetName.IsNone() ? TEXT("?") : *TargetName.ToString(),
					*Target.Cell.ToString(), StateName(Before), StateName(State)));
			}
			continue;
		}

		// Ordine canonico DENTRO il bersaglio; fra bersagli vince l'ordine dichiarato in `TargetIds`, che e'
		// autorevole (invariante n. 3). Un `Sort` globale in fondo lo cancellerebbe, e con esso l'unica cosa
		// che il giocatore puo' osservare dell'ordine di applicazione.
		SortDoorChanges(Local);
		for (FRTDoorChange& Change : Local)
		{
			Change.ActorId = ActorId; // chi ha comandato viaggia con l'esito (#405)
		}
		Changes.Append(Local);
	}

	// UNA revisione per l'intera interazione, qualunque sia N: e' il requisito che ha imposto di separare la
	// commutazione dal commit.
	CommitTouched(Map, Work, Touched);
	return Changes;
}

TArray<FRTDoorChange> URTHexDoorLibrary::ApplyDoorOps(URTHexMapAsset* Map, const TArray<FRTDoorOp>& Ops)
{
	TArray<FRTDoorChange> Changes;
	if (Map == nullptr || Ops.Num() == 0)
	{
		return Changes;
	}

	// Un ordine per BORDO: a parita' di bordo vince lo stato piu' restrittivo. Senza questa fusione, due
	// unita' che nello stesso turno una apre e una chiude darebbero esiti diversi a seconda di chi viene
	// prima nell'array — cioe' l'ordine delle richieste deciderebbe la partita (invariante #3).
	TArray<FRTDoorOp> Merged;
	for (const FRTDoorOp& Op : Ops)
	{
		FRTDoorOp* Existing = Merged.FindByPredicate([&Op](const FRTDoorOp& M)
		{
			return (M.From == Op.From && M.To == Op.To) || (M.From == Op.To && M.To == Op.From);
		});
		if (Existing == nullptr)
		{
			Merged.Add(Op);
			continue;
		}
		if (Restriction(Op.State) > Restriction(Existing->State))
		{
			Existing->State = Op.State;
			// L'attore segue lo STATO: se questa richiesta ha vinto la fusione, la porta l'ha chiusa lei. Lasciare
			// l'attore precedente attribuirebbe l'esito a chi e' stato scavalcato.
			Existing->ActorId = Op.ActorId;
		}
	}

	Merged.Sort([](const FRTDoorOp& A, const FRTDoorOp& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		return URTHexLibrary::StableLess(A.To, B.To);
	});

	for (const FRTDoorOp& Op : Merged)
	{
		const int32 FirstFromThisOp = Changes.Num();
		Changes.Append(SetDoorState(Map, Op.From, Op.To, Op.State));
		// Chi ha chiesto il cambio viaggia con l'esito (#405). `SetDoorState` resta ignaro dell'attore: apre e
		// chiude allo stesso modo per chiunque.
		for (int32 c = FirstFromThisOp; c < Changes.Num(); ++c)
		{
			Changes[c].ActorId = Op.ActorId;
		}
	}
	return Changes;
}
