#include "Pathfinding/RTHexPathLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTHexCoverPlacementLibrary.h"
#include "Algo/Reverse.h"


bool URTHexPathLibrary::CellHasInteriorGeometry(const URTHexMapAsset* Map, const FRTCellId& Cell)
{
	if (!Map)
	{
		return false;
	}
	return Map->InteriorWalls.ContainsByPredicate(
		[&Cell](const FRTHexInteriorWall& Wall) { return Wall.Cell == Cell; });
}

bool URTHexPathLibrary::CanTransitCell(const URTHexMapAsset* Map, const FRTCellId& Cell,
	ERTHexDirection EntryDir, ERTHexDirection ExitDir)
{
	// 🔑 **LA VIA RAPIDA NON E' UN'OTTIMIZZAZIONE, E' CIO' CHE TIENE FERMO L'ORDINAMENTO.** Una cella
	// senza geometria interna non ha regioni da separare: qualunque coppia di lati sta nella stessa
	// regione, e il lato d'ingresso non porta informazione. Rispondere `true` senza guardarlo permette
	// ai percorritori di NON distinguere lo stato per quelle celle — cioe' a tutte le mappe versionate
	// oggi (`InteriorWall` compare in **zero** `.uasset`) di conservare esattamente l'ordine di visita
	// che avevano. Nessun percorso di pari costo cambia, e il corpus golden non si rigenera.
	if (!CellHasInteriorGeometry(Map, Cell))
	{
		return true;
	}

	// La maschera si deriva dai muri interni di QUESTA cella, come fa il bake: e' l'unico produttore.
	TArray<FRTOccupancyPolyline> Geometry;
	for (const FRTHexInteriorWall& Wall : Map->InteriorWalls)
	{
		if (Wall.Cell == Cell)
		{
			Geometry.Add(URTGeometryGrammarLibrary::ToPolyline(Wall.Segment, Map->HexSize));
		}
	}
	const FRTOccupancyMask Mask = URTHexOccupancyLibrary::ComputeMask(Geometry, Map->HexSize);

	// Il lato geometrico `k` copre DUE settori consecutivi, `2k` e `2k+1`: e' l'ancoraggio dichiarato da
	// `FRTOccupancyMask::Sectors`, dove ogni direzione esagonale copre esattamente due spicchi.
	const int32 EntryEdge = URTHexLibrary::EdgeIndexForDirection(EntryDir);
	const int32 ExitEdge = URTHexLibrary::EdgeIndexForDirection(ExitDir);

	// ⚠️ **Basta UNA coppia di settori collegata, e non tutte.** Chi attraversa sceglie dove camminare
	// dentro la cella: pretendere che tutti e quattro gli accoppiamenti siano validi vieterebbe transiti
	// leciti — un muro che tocca il bordo d'ingresso senza dividerlo dal bordo d'uscita.
	for (int32 EntryHalf = 0; EntryHalf < 2; ++EntryHalf)
	{
		for (int32 ExitHalf = 0; ExitHalf < 2; ++ExitHalf)
		{
			const int32 FromWedge = (2 * EntryEdge + EntryHalf) % RT_OccupancySectorCount;
			const int32 ToWedge = (2 * ExitEdge + ExitHalf) % RT_OccupancySectorCount;

			if (URTHexCoverPlacementLibrary::ClassifyIntraCellTraversal(Mask, FromWedge, ToWedge)
				== ERTIntraCellTraversal::SameRegion)
			{
				return true;
			}
		}
	}

	return false;
}

TArray<TPair<FRTCellId, int32>> URTHexPathLibrary::GraphNeighbors(const URTHexMapAsset* Map, const FRTCellId& Cell)
{
	TArray<TPair<FRTCellId, int32>> Out;
	if (!Map)
	{
		return Out;
	}

	// Vicini orizzontali (stesso layer): presenti, non bloccati e non separati da una barriera; costo =
	// MoveCost della destinazione. La copertura ALTA (CP 9.2) nega il PASSO fra le due celle senza toglierne
	// nessuna dalla mappa: restano entrambe occupabili, semplicemente non si passa da una all'altra.
	for (const FRTCellId& N : URTHexLibrary::Neighbors(Cell))
	{
		if (const FRTHexCellData* D = Map->FindCell(N))
		{
			if (!D->bBlocksMovement && !URTHexCoverLibrary::BlocksTraversal(Map, Cell, N))
			{
				Out.Add(TPair<FRTCellId, int32>(N, D->TotalMoveCost()));
			}
		}
	}

	// Transizioni esplicite uscenti (rampe/ponti/scale/tunnel): costo = costo dell'arco. Un arco non ATTIVO
	// (CP 9.4) non e' un collegamento: le due celle tornano irraggiungibili l'una dall'altra, e il percorso
	// FALLISCE invece di teletrasportare — non esiste un'adiacenza planare di riserva fra due layer.
	for (const FRTHexEdge& E : Map->Transitions)
	{
		if (E.From == Cell && E.State == ERTHexArcState::Active)
		{
			if (const FRTHexCellData* D = Map->FindCell(E.To))
			{
				if (!D->bBlocksMovement)
				{
					Out.Add(TPair<FRTCellId, int32>(E.To, FMath::Max(0, E.Cost)));
				}
			}
		}
	}
	return Out;
}

FRTHexPathResult URTHexPathLibrary::FindPath(const URTHexMapAsset* Map, const FRTCellId& Start, const FRTCellId& Goal,
	int32 MaxCost, int32 MaxNodes)
{
	return FindPathAvoiding(Map, Start, Goal, nullptr, MaxCost, MaxNodes);
}


/**
 * IL NODO DELLA RICERCA: una cella PIU' il lato da cui ci si e' entrati (#2100).
 *
 * 🔴 **Perche' la cella da sola non basta.** Un muro interno divide lo spazio di posa di UN solo
 * `FRTCellId` in regioni sconnesse, quindi *poter uscire* verso un vicino dipende da *dove si e' entrati*.
 * Con lo stato indicizzato per sola cella, A* deciderebbe l'uscita guardando un predecessore che una
 * espansione successiva puo' riscrivere: la risposta sarebbe corretta per un cammino e applicata a un altro.
 *
 * 🔑 **`Entry == RT_NoEntrySide` NON e' "sconosciuto": e' "il lato non porta informazione".** Vale per una
 * cella senza geometria interna — dove ogni coppia di lati sta nella stessa regione — e per l'ingresso da un
 * ARCO esplicito (rampa, ponte, tunnel), che non attraversa un bordo del perimetro e quindi non ha settore.
 *
 * ⚠️ **Ed e' cio' che tiene fermo l'ordine di visita.** Su una mappa senza muri interni ogni nodo nasce
 * con il sentinella, la chiave e' in corrispondenza 1:1 con la cella, e `StableLess` sul nodo si riduce a
 * `StableLess` sulla cella: la ricerca visita esattamente cio' che visitava prima. `InteriorWall` compare oggi in
 * **zero** `.uasset` versionati, quindi nessuna traccia registrata cambia e il corpus golden non si rigenera.
 */

FRTHexPathResult URTHexPathLibrary::FindPathAvoiding(const URTHexMapAsset* Map, const FRTCellId& Start,
	const FRTCellId& Goal, const TSet<FRTCellId>* Blocked, int32 MaxCost, int32 MaxNodes, int32 ExtraCostPerCell)
{
	FRTHexPathResult Result;

	if (!Map || !Map->ContainsCell(Start))
	{
		Result.Status = ERTHexPathStatus::StartInvalid;
		return Result;
	}
	if (!Map->ContainsCell(Goal))
	{
		Result.Status = ERTHexPathStatus::GoalInvalid;
		return Result;
	}
	// Ostacolo dinamico sul goal: la cella esiste ma e' occupata -> nessun percorso (non e' un goal invalido).
	if (Blocked && Blocked->Contains(Goal) && Goal != Start)
	{
		Result.Status = ERTHexPathStatus::NoPath;
		return Result;
	}
	if (Start == Goal)
	{
		Result.Status = ERTHexPathStatus::Success;
		Result.Path.Add(Start);
		return Result;
	}

	// Euristica ammissibile: distanza esagonale (planare) verso il goal.
	auto Heuristic = [&Goal](const FRTCellId& C) { return URTHexLibrary::HexDistance(C, Goal); };

	TMap<FRTPathNode, int32> GScore;
	TMap<FRTPathNode, FRTPathNode> CameFrom;
	TSet<FRTPathNode> Closed;
	TArray<FRTPathNode> Open;

	// La partenza non ha un lato d'ingresso: non ci si e' entrati, ci si era gia'.
	const FRTPathNode StartNode{ Start, RT_NoEntrySide };
	GScore.Add(StartNode, 0);
	Open.Add(StartNode);

	while (Open.Num() > 0)
	{
		// Estrai il nodo con f minimo; tie-break deterministico: g minore, poi ID stabile (no ordine TSet/TMap).
		int32 BestIdx = 0;
		int32 BestF = GScore[Open[0]] + Heuristic(Open[0].Cell);
		int32 BestG = GScore[Open[0]];
		for (int32 I = 1; I < Open.Num(); ++I)
		{
			const int32 G = GScore[Open[I]];
			const int32 F = G + Heuristic(Open[I].Cell);
			const bool bBetter =
				(F < BestF) ||
				(F == BestF && G < BestG) ||
				(F == BestF && G == BestG && NodeStableLess(Open[I], Open[BestIdx]));
			if (bBetter)
			{
				BestIdx = I; BestF = F; BestG = G;
			}
		}

		const FRTPathNode Current = Open[BestIdx];
		Open.RemoveAt(BestIdx);
		if (Closed.Contains(Current))
		{
			continue; // gia' finalizzato (duplicato nell'open)
		}
		Closed.Add(Current);
		++Result.NodesVisited;

		// Il goal e' raggiunto da QUALUNQUE lato: la meta e' la cella, non il modo di entrarci.
		if (Current.Cell == Goal)
		{
			// Ricostruisci il percorso (goal -> start) e invertilo. Si cammina sui NODI e si emettono le
			// celle: due nodi della stessa cella non possono susseguirsi, perche' ogni arco cambia cella.
			TArray<FRTCellId> Rev;
			FRTPathNode Node = Current;
			Rev.Add(Node.Cell);
			while (const FRTPathNode* Prev = CameFrom.Find(Node))
			{
				Node = *Prev;
				Rev.Add(Node.Cell);
			}
			Algo::Reverse(Rev);
			Result.Path = MoveTemp(Rev);
			Result.TotalCost = GScore[Current];
			Result.Status = ERTHexPathStatus::Success;
			return Result;
		}

		if (Result.NodesVisited > MaxNodes)
		{
			Result.Status = ERTHexPathStatus::NodeLimit;
			return Result;
		}

		for (const TPair<FRTCellId, int32>& Step : GraphNeighbors(Map, Current.Cell))
		{
			if (Blocked && Blocked->Contains(Step.Key))
			{
				continue; // cella occupata da un'altra unita' (ostacolo dinamico)
			}

			// 🔴 **LA TRAVERSATA INTRA-CELLA** (#2100): si esce da `Current` solo se il lato d'uscita sta
			// nella stessa regione libera del lato da cui si e' entrati. Con `RT_NoEntrySide` la domanda non si
			// pone — cella senza geometria, o ingresso per arco — e il passo resta lecito come prima.
			if (Current.Entry != RT_NoEntrySide)
			{
				ERTHexDirection ExitDir = ERTHexDirection::E;
				if (URTHexLibrary::DirectionBetween(Current.Cell, Step.Key, ExitDir)
					&& !CanTransitCell(Map, Current.Cell,
						static_cast<ERTHexDirection>(Current.Entry), ExitDir))
				{
					continue; // geometria interna: le due sponde non si parlano
				}
			}

			const int32 Tentative = GScore[Current] + Step.Value + FMath::Max(0, ExtraCostPerCell);
			if (MaxCost > 0 && Tentative > MaxCost)
			{
				continue; // oltre il budget di movimento
			}

			const FRTPathNode Next{ Step.Key, EntrySideOf(Map, Step.Key, Current.Cell) };
			const int32* Existing = GScore.Find(Next);
			if (!Existing || Tentative < *Existing)
			{
				GScore.Add(Next, Tentative);
				CameFrom.Add(Next, Current);
				Open.Add(Next); // puo' duplicare; il set Closed evita ri-espansioni
			}
		}
	}

	Result.Status = ERTHexPathStatus::NoPath;
	return Result;
}
