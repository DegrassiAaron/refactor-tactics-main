#include "Map/RTHexDoorLibrary.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

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
	if (const FRTHexCellData* Near = Map->FindCell(From))
	{
		if (const FRTHexDoor* Door = Near->DoorOn(Forward)) { GroupId = Door->DoorId; }
	}
	if (GroupId == INDEX_NONE)
	{
		if (const FRTHexCellData* Far = Map->FindCell(To))
		{
			if (const FRTHexDoor* Door = Far->DoorOn(Backward)) { GroupId = Door->DoorId; }
		}
	}

	// Si scandisce l'array delle celle nel suo ordine, che e' stabile: due esecuzioni della stessa partita
	// producono le stesse voci nella stessa sequenza.
	TArray<FRTHexCellData> Updated;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		FRTHexCellData Copy = Cell;
		bool bTouched = false;
		for (FRTHexDoor& Door : Copy.Doors)
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
			bTouched = true;

			FRTDoorChange Change;
			Change.Cell = Cell.Id;
			Change.Toward = NeighborAcross(Cell.Id, Door.Edge);
			Change.State = State;
			Change.bBlocking = StateBlocks(State);
			Changes.Add(Change);
		}
		if (bTouched)
		{
			Updated.Add(Copy);
		}
	}

	// UNA sola revisione per l'intero gruppo: chi la osserva non deve vedere tre cambi dove ce n'e' stato uno.
	// Con `Updated` vuoto non incrementa nulla — una transizione rifiutata non e' un cambiamento della mappa.
	Map->UpdateCells(Updated);

	// Ordine canonico: da qui escono voci di TurnLog, e due esecuzioni devono scriverle nello stesso ordine.
	Changes.Sort([](const FRTDoorChange& A, const FRTDoorChange& B)
	{
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return URTHexLibrary::StableLess(A.Toward, B.Toward);
	});
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
		}
	}

	Merged.Sort([](const FRTDoorOp& A, const FRTDoorOp& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		return URTHexLibrary::StableLess(A.To, B.To);
	});

	for (const FRTDoorOp& Op : Merged)
	{
		Changes.Append(SetDoorState(Map, Op.From, Op.To, Op.State));
	}
	return Changes;
}
