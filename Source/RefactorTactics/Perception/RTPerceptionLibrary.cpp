#include "Perception/RTPerceptionLibrary.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexOcclusionLibrary.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

namespace
{
	/**
	 * Il contatto regge fra due celle? Tre condizioni, tutte necessarie: portata, fumo, linea di vista.
	 *
	 * Il **cap del fumo** passa da `URTTerrainLibrary::EffectiveTargetingRange`, che di quella regola e'
	 * l'owner: ricalcolarlo qui darebbe due verita' sullo stesso numero, e la seconda divergerebbe alla prima
	 * modifica. La sua convenzione include gli estremi — stare NEL fumo cappa quanto guardarci attraverso.
	 */
	bool ContactHolds(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To, int32 Range)
	{
		const int32 Distance = URTHexLibrary::HexDistance(From, To);
		if (Distance > Range)
		{
			return false;
		}
		if (Distance > URTTerrainLibrary::EffectiveTargetingRange(Map, From, To, Range))
		{
			return false; // fumo dentro o attraverso: il contatto si ferma prima
		}

		// ⛔ **Fra due piani la linea e' un'altra** (`D-511`, #3743; e `D-510`, #3741, che ne e' il caso della
		// stessa colonna). Va da testa a testa e si ferma sui pavimenti: chi sta sotto una piattaforma non vede
		// che cosa ha sopra, e chi sta sul bordo copre chi gli sta dietro.
		//
		// 🔑 **Sta QUI e non in `URTHexVisionLibrary::DescribeLineOfSight`, ed e' una scelta misurata.** La prima
		// stesura di #3741 aveva messo la sua regola nella linea di vista: la suite ha fatto cadere
		// `Bot.StalemateProbeGeneratedArenaFilterNeverStepsBack` e `…ClosesNoCycleOfAnyPeriod` sulle otto coppie
		// di celle impilate dell'arena generata. Da una cella esattamente sotto il bersaglio ogni cella che
		// «vede» e' piu' lontana: il filtro dei bot arretra, e l'orbita di #1287 si chiude. La linea di vista e'
		// la geometria su cui i bot scelgono le celle; il CONTATTO e' cio' che una squadra sa. La regola e' del
		// secondo, e colpire un altro piano e' gia' un rifiuto dichiarato (`D-393`, `D-509`).
		if (From.Layer != To.Layer)
		{
			return URTPerceptionLibrary::HasSightAcrossLayers(Map, From, To);
		}
		return URTHexVisionLibrary::HasLineOfSight(Map, From, To);
	}
}

FRTCrossLayerSightResult URTPerceptionLibrary::DescribeSightAcrossLayers(const URTHexMapAsset* Map,
	const FRTCellId& From, const FRTCellId& To)
{
	FRTCrossLayerSightResult Result;
	if (!Map || From.Layer == To.Layer)
	{
		return Result; // nessun pavimento noto, oppure stesso piano: non e' la domanda di questa regola
	}

	// 🔑 Sempre dalla cella piu' BASSA alla piu' ALTA, chiunque sia `From`. E' cio' che rende l'esito lo stesso
	// nei due versi senza doverlo asserire: non esistono due linee da tenere allineate, ne esiste una.
	const bool bFromIsLower = From.Layer < To.Layer;
	const FRTCellId& Lower = bFromIsLower ? From : To;
	const FRTCellId& Upper = bFromIsLower ? To : From;
	const int32 Rise = Upper.Layer - Lower.Layer; // >= 1

	auto OnLayer = [](const FRTCellId& Cell, int32 Layer) { return FRTCellId(Cell.X, Cell.Y, Layer); };
	auto Stopped = [&Result](ERTCrossLayerSightBlock Why, const FRTCellId& At) -> FRTCrossLayerSightResult
	{
		Result.Block = Why;
		Result.BlockedAt = At;
		return Result;
	};

	// `D-510`, #3741: «Se uno sta nella cella sopra un nemico, i 2 non si vedono (la linea di visuale passa al
	// centro perpendicolare)». La linea e' verticale e buca il pavimento su cui sta chi e' sopra. Il ramo e'
	// esplicito perche' la decisione non dipende dall'asset: vale anche se quella cella non e' fra i dati.
	if (Lower.X == Upper.X && Lower.Y == Upper.Y)
	{
		return Stopped(ERTCrossLayerSightBlock::Floor, Upper);
	}

	// --- 1. I PAVIMENTI -------------------------------------------------------------------------------------
	// La testa sta a `HeadNum / HeadDen` di piano sopra il pavimento, ai due capi: in altezze di piano la linea
	// sale di `Rise` in tutto, e raggiunge la quota del piano `Lower.Layer + Climb` dopo averne salite
	// `Climb - HeadNum / HeadDen`. Quindi a `(HeadDen * Climb - HeadNum) / (HeadDen * Rise)` della distanza,
	// contando da chi sta sotto: due quinti, con un piano di dislivello e la testa a tre quinti. Nessuna misura
	// in unita' di mondo entra nel conto.
	constexpr int32 HeadNum = SightHeadHeightNum;
	constexpr int32 HeadDen = SightHeadHeightDen;
	for (int32 Climb = 1; Climb <= Rise; ++Climb)
	{
		const FRTCellId Crossing =
			URTHexLibrary::HexLinePoint(Lower, Upper, HeadDen * Climb - HeadNum, HeadDen * Rise);
		const FRTCellId FloorCell = OnLayer(Crossing, Lower.Layer + Climb);
		if (Map->FindCell(FloorCell) != nullptr)
		{
			return Stopped(ERTCrossLayerSightBlock::Floor, FloorCell);
		}
		// cella assente = buco nel pavimento: la linea passa
	}

	// --- 2. GLI OSTACOLI, DEL PIANO ATTRAVERSATO ------------------------------------------------------------
	// La linea in pianta e' quella di `HexLine`, col suo passo: ogni cella vale da mezzo passo prima a mezzo
	// passo dopo il proprio campione. In MEZZI passi dalla cella bassa la quota e' una frazione intera:
	//
	//     quota = Lower.Layer + HeadNum / HeadDen + Rise * Mezzi / (2 * N)
	//
	// e il piano e' la sua parte intera. `LayerAt` e' il piano in quel punto; `LayerBefore` quello appena
	// prima, cosi' una cella non risponde di un piano che la linea tocca solo uscendone.
	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(Lower, Upper);
	const int32 N = Line.Num() - 1; // >= 1: la stessa colonna e' gia' uscita
	const int64 Scale = static_cast<int64>(2) * HeadDen * N;
	auto ScaledHeight = [&](int32 HalfSteps) -> int64
	{
		return static_cast<int64>(2) * HeadNum * N + static_cast<int64>(HeadDen) * Rise * HalfSteps;
	};
	auto LayerAt = [&](int32 HalfSteps) { return Lower.Layer + static_cast<int32>(ScaledHeight(HalfSteps) / Scale); };
	auto LayerBefore = [&](int32 HalfSteps)
	{
		return Lower.Layer + static_cast<int32>((ScaledHeight(HalfSteps) - 1) / Scale);
	};

	// I muri interni di una cella, su un piano: la corda va dal lato d'ingresso a quello d'uscita, e ai due
	// capi nasce o muore al centro (`Prev == Cell`, `Next == Cell`), come in `DescribeLineOfSight`.
	auto InteriorBlocks = [&](int32 Index, int32 Layer) -> bool
	{
		const FRTCellId Cell = OnLayer(Line[Index], Layer);
		const FRTCellId Prev = (Index > 0) ? OnLayer(Line[Index - 1], Layer) : Cell;
		const FRTCellId Next = (Index < N) ? OnLayer(Line[Index + 1], Layer) : Cell;
		return URTHexOcclusionLibrary::BlocksSight(Map, Prev, Cell, Next);
	};

	// I due capi rispondono del PROPRIO piano soltanto. Dentro la colonna di un capo la linea puo' cambiare
	// piano solo bucando il pavimento di quella colonna, e quello l'ha gia' chiesto il passo 1.
	if (InteriorBlocks(0, Lower.Layer))
	{
		return Stopped(ERTCrossLayerSightBlock::InteriorGeometry, Lower);
	}

	for (int32 I = 1; I <= N; ++I)
	{
		// Il bordo fra `I - 1` e `I` sta a `2 * I - 1` mezzi passi: conta la copertura alta, o la porta chiusa,
		// del piano in cui la linea lo attraversa. Come dentro un piano, contano anche il primo e l'ultimo.
		const int32 EdgeLayer = LayerAt(2 * I - 1);
		const FRTCellId Entering = OnLayer(Line[I], EdgeLayer);
		if (URTHexCoverLibrary::BlocksTraversal(Map, OnLayer(Line[I - 1], EdgeLayer), Entering))
		{
			return Stopped(ERTCrossLayerSightBlock::EdgeBlocker, Entering);
		}

		if (I == N)
		{
			break; // l'ultima cella e' un capo: non ci si oscura da soli
		}

		// Una cella intermedia risponde di ogni piano che la linea occupa mentre l'attraversa.
		const int32 LastLayer = LayerBefore(2 * I + 1);
		for (int32 Layer = EdgeLayer; Layer <= LastLayer; ++Layer)
		{
			const FRTCellId Cell = OnLayer(Line[I], Layer);
			if (const FRTHexCellData* Data = Map->FindCell(Cell))
			{
				if (Data->bBlocksLineOfSight)
				{
					return Stopped(ERTCrossLayerSightBlock::CellBlocker, Cell);
				}
			}
			if (InteriorBlocks(I, Layer))
			{
				return Stopped(ERTCrossLayerSightBlock::InteriorGeometry, Cell);
			}
		}
	}

	if (InteriorBlocks(N, Upper.Layer))
	{
		return Stopped(ERTCrossLayerSightBlock::InteriorGeometry, Upper);
	}

	return Result;
}

bool URTPerceptionLibrary::HasSightAcrossLayers(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To)
{
	return DescribeSightAcrossLayers(Map, From, To).IsClear();
}

TArray<FRTCellId> URTPerceptionLibrary::VisibleCells(const URTHexMapAsset* Map, const FRTPerceiver& Observer)
{
	TArray<FRTCellId> Seen;
	Seen.Add(Observer.Cell); // si sa sempre dove si e'

	if (!Map)
	{
		return Seen; // fail-closed: senza mappa non si verifica ne' LOS ne' terreno
	}

	// Il candidato piu' lontano possibile: l'arco frontale arriva a `VisionRange`, il canale ravvicinato a
	// `CloseAwarenessRange`. Si esplora l'area piu' grande delle due e si filtra per canale.
	const int32 Reach = FMath::Max(FMath::Max(Observer.VisionRange, 0), CloseAwarenessRange);
	const FRTCellId Ahead = URTHexLibrary::Neighbor(Observer.Cell, Observer.Facing);

	// I candidati si prendono dalle celle CHE ESISTONO, non da `HexArea`: quella genera un'area piatta sulla
	// quota dell'osservatore (`Center.Layer` cablato), e su una mappa multilivello una vista costruita cosi'
	// dichiarerebbe invisibile tutto cio' che sta su un altro piano — un'unita' sul ponte sopra la testa, o
	// chi guarda dal ponte verso il fondo.
	//
	// Non e' una regola nuova sulla verticalita': e' l'allineamento a quella che il gioco gia' applica.
	// `URTHexVisionLibrary::HasLineOfSight` non guarda il layer — la linea di tiro si valuta sulla proiezione
	// esagonale — e `HexDistance` nemmeno. Fermare la VISTA alla propria quota mentre la LINEA DI TIRO
	// l'attraversa erano due verita' sullo stesso spazio; finche' nessuno consumava la vista la differenza
	// non si vedeva, e CP 13.2 la rende visibile rifiutando ogni ingaggio fra piani diversi.
	// High Ground resta senza bonus numerico alla vista (canone v0.1): qui non si concede un vantaggio, si
	// toglie una cecita' che nessuna decisione aveva stabilito.
	//
	// ➕ `D-510` (#3741) e poi `D-511` (#3743): la cecita' fra piani ora e' stabilita, ed e' quella di una
	// linea vera. I candidati restano le celle di ogni piano, e distanza e arco restano sulla proiezione; e' il
	// CONTATTO che fra due piani chiede `DescribeSightAcrossLayers`, da testa a testa e ferma sui pavimenti.
	// ⚠️ Vale per la CELLA, non solo per chi la occupa (scelta d'autore: «Anche le celle»): chi sta sotto un
	// mezzanino non ne vede le celle, e il velo le disegna solo se le vede un compagno da fuori o se sono gia'
	// ricordate.
	for (const FRTHexCellData& CellData : Map->Cells)
	{
		const FRTCellId& Candidate = CellData.Id;
		if (Candidate == Observer.Cell)
		{
			continue; // gia' aggiunta
		}

		const int32 Distance = URTHexLibrary::HexDistance(Observer.Cell, Candidate);
		if (Distance > Reach)
		{
			continue; // fuori da entrambi i canali: si scarta prima di pagare LOS e terreno
		}

		// Canale 1 — consapevolezza ravvicinata: ogni direzione, entro il cap, purche' ci sia LOS.
		const bool bClose = Distance <= CloseAwarenessRange
			&& ContactHolds(Map, Observer.Cell, Candidate, CloseAwarenessRange);

		// Canale 2 — arco frontale: dentro il cono e dentro la vista.
		//
		// Il cono si chiede a profondita' `Distance` e non `VisionRange`: `HexCone` restituisce il ventaglio
		// fino alla profondita' richiesta, quindi chiedere la distanza esatta risponde alla sola domanda che
		// serve — «questa cella e' nell'arco?» — senza costruire il ventaglio intero per ogni candidata.
		// L'arco si valuta sulla PROIEZIONE esagonale, come gia' fanno distanza e linea di tiro: `HexCone`
		// genera il ventaglio alla quota del proprio centro, quindi confrontarci una candidata di un altro
		// piano darebbe sempre falso. Appiattire la candidata sulla quota dell'osservatore chiede la sola cosa
		// che serve — «sta nel settore che l'unita' copre?» — senza inventare una geometria verticale del cono,
		// che nessuna decisione ha stabilito e che High Ground esclude per la v0.1.
		const FRTCellId Flattened(Candidate.X, Candidate.Y, Observer.Cell.Layer);
		const bool bInArc = Distance <= Observer.VisionRange
			&& URTHexLibrary::HexCone(Observer.Cell, Ahead, Distance).Contains(Flattened)
			&& ContactHolds(Map, Observer.Cell, Candidate, Observer.VisionRange);

		if (bClose || bInArc)
		{
			Seen.Add(Candidate);
		}
	}

	// Ordine canonico del progetto: layer, poi q, poi r. Non l'ordine di scoperta, che dipenderebbe da come
	// `HexArea` enumera.
	Seen.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
	return Seen;
}

TArray<FRTCellId> URTPerceptionLibrary::TeamVisibleCells(const URTHexMapAsset* Map, const TArray<FRTPerceiver>& Team)
{
	TSet<FRTCellId> Union;
	for (const FRTPerceiver& Observer : Team)
	{
		Union.Append(TSet<FRTCellId>(VisibleCells(Map, Observer)));
	}

	TArray<FRTCellId> Out = Union.Array();
	// L'ordine di un TSet dipende dall'hash e dall'ordine di inserimento: senza questo Sort il risultato
	// cambierebbe permutando gli osservatori, che e' esattamente cio' che l'invariante #3 vieta.
	Out.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
	return Out;
}

ERTAwareness URTPerceptionLibrary::TeamAwarenessOfCell(const URTHexMapAsset* Map, const TArray<FRTPerceiver>& Team,
	const FRTCellId& Cell)
{
	for (const FRTPerceiver& Observer : Team)
	{
		if (VisibleCells(Map, Observer).Contains(Cell))
		{
			return ERTAwareness::Detected;
		}
	}
	// Mai `Uncertain`: la vista non ha le informazioni che lo producono. Lo riempiranno il rumore (CP 13.4)
	// e la memoria del contatto (CP 13.2).
	return ERTAwareness::Hidden;
}
