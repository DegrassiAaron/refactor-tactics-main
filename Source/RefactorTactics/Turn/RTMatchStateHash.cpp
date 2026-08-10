#include "Turn/RTMatchStateHash.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

uint32 URTMatchStateHashLibrary::HashMatchState(const URTHexMapAsset* Map,
	const TArray<FRTUnitStateDigest>& Units, const TArray<int32>& TeamScores)
{
	uint32 Hash = 2166136261u;
	auto Mix = [&Hash](uint32 V) { Hash ^= V; Hash *= 16777619u; };
	auto MixName = [&Mix](const FName& Name)
	{
		// Il TESTO, mai l'indice della name table: quello dipende dall'ordine in cui i nomi sono stati creati
		// nel processo, quindi due esecuzioni della stessa partita darebbero due hash diversi (invariante #4).
		for (const TCHAR Ch : Name.ToString())
		{
			Mix(static_cast<uint32>(Ch));
		}
	};
	auto MixCell = [&Mix](const FRTCellId& Cell)
	{
		Mix(static_cast<uint32>(Cell.X));
		Mix(static_cast<uint32>(Cell.Y));
		Mix(static_cast<uint32>(Cell.Layer));
	};

	// --- Unità -------------------------------------------------------------------------------------------
	// In ordine di `StableUnitId`: l'identità è assegnata una volta sola da `EnsureMatchRoster` e non si muove
	// per tutta la partita, l'ordine dell'array sì (`GetAllActorsOfClass` non è ordinato).
	TArray<FRTUnitStateDigest> SortedUnits = Units;
	SortedUnits.Sort([](const FRTUnitStateDigest& A, const FRTUnitStateDigest& B)
		{ return A.StableUnitId < B.StableUnitId; });

	for (const FRTUnitStateDigest& U : SortedUnits)
	{
		Mix(static_cast<uint32>(U.StableUnitId));
		MixCell(U.Cell);
		Mix(static_cast<uint32>(U.Health));
		Mix(static_cast<uint32>(U.Shield));
		Mix(static_cast<uint32>(U.Energy));
		Mix(U.bAlive ? 1u : 0u);

		// Stati ORDINATI: arrivano da `TMap`/`TSet`, la cui iterazione non è deterministica. Senza l'ordine,
		// due esecuzioni identiche darebbero hash diversi — il checksum diventerebbe la fonte del falso
		// allarme che dovrebbe scoprire.
		TArray<FName> SortedStatuses = U.Statuses;
		SortedStatuses.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });
		for (const FName& Tag : SortedStatuses)
		{
			MixName(Tag);
		}
	}

	// --- Mappa: terreni modificati e strutture -----------------------------------------------------------
	if (Map)
	{
		// La revisione da sola non basta — dice CHE la topologia è cambiata, non COME — ma entra lo stesso:
		// distingue due mappe che il resto dei campi renderebbe identiche (una porta aperta e richiusa).
		Mix(static_cast<uint32>(Map->Revision));

		TArray<FRTHexCellData> Cells = Map->Cells;
		Cells.Sort([](const FRTHexCellData& A, const FRTHexCellData& B)
		{
			return URTHexLibrary::StableLess(A.Id, B.Id);
		});

		for (const FRTHexCellData& Cell : Cells)
		{
			MixCell(Cell.Id);
			Mix(static_cast<uint32>(Cell.Surface));   // il TERRENO MODIFICATO: fuoco, acqua, ghiaccio
			Mix(static_cast<uint32>(Cell.Height));
			Mix(static_cast<uint32>(Cell.MoveCost));
			Mix(Cell.bBlocksMovement ? 1u : 0u);
			Mix(Cell.bBlocksLineOfSight ? 1u : 0u);

			// STRUTTURE sul bordo. Ordinate per bordo: l'array segue l'ordine in cui sono state erette.
			TArray<FRTHexCover> Covers = Cell.Covers;
			Covers.Sort([](const FRTHexCover& A, const FRTHexCover& B)
			{
				return static_cast<uint8>(A.Edge) < static_cast<uint8>(B.Edge);
			});
			for (const FRTHexCover& Cover : Covers)
			{
				Mix(static_cast<uint32>(Cover.Edge));
				Mix(static_cast<uint32>(Cover.Type));
				Mix(static_cast<uint32>(Cover.Integrity));
			}

			TArray<FRTHexDoor> Doors = Cell.Doors;
			Doors.Sort([](const FRTHexDoor& A, const FRTHexDoor& B)
			{
				return static_cast<uint8>(A.Edge) < static_cast<uint8>(B.Edge);
			});
			for (const FRTHexDoor& Door : Doors)
			{
				Mix(static_cast<uint32>(Door.Edge));
				Mix(static_cast<uint32>(Door.State));
				Mix(static_cast<uint32>(Door.DoorId));
			}
		}

		// ARCHI (ponti, CP 9.4): un collegamento è additivo, quindi la sua assenza non si vede dalle celle.
		TArray<FRTHexEdge> Arcs = Map->Transitions;
		Arcs.Sort([](const FRTHexEdge& A, const FRTHexEdge& B)
		{
			if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
			return URTHexLibrary::StableLess(A.To, B.To);
		});
		for (const FRTHexEdge& Arc : Arcs)
		{
			MixCell(Arc.From);
			MixCell(Arc.To);
			Mix(static_cast<uint32>(Arc.Cost));
			Mix(static_cast<uint32>(Arc.Kind));
			Mix(static_cast<uint32>(Arc.State));
			Mix(static_cast<uint32>(Arc.Integrity));
		}
	}

	// --- Progresso degli obiettivi ------------------------------------------------------------------------
	// Indicizzato per TeamId, quindi l'ordine è già quello e non va riordinato: due squadre con punteggi
	// scambiati sono uno stato diverso, non lo stesso stato permutato.
	for (int32 TeamId = 0; TeamId < TeamScores.Num(); ++TeamId)
	{
		Mix(static_cast<uint32>(TeamId));
		Mix(static_cast<uint32>(TeamScores[TeamId]));
	}

	return Hash;
}
