#include "Perception/RTPerceptionLibrary.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

namespace
{
	/**
	 * Il contatto regge fra due celle? Quattro condizioni, tutte necessarie.
	 *
	 * Il **cap del fumo** passa da `URTTerrainLibrary::EffectiveTargetingRange`, che di quella regola e'
	 * l'owner: ricalcolarlo qui darebbe due verita' sullo stesso numero, e la seconda divergerebbe alla prima
	 * modifica. La sua convenzione include gli estremi — stare NEL fumo cappa quanto guardarci attraverso.
	 */
	bool ContactHolds(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To, int32 Range)
	{
		// ⛔ **Stessa colonna, piano diverso: nessun contatto** (`D-510`, #3741). Decisione d'autore del
		// 2026-10-13: «Se uno sta nella cella sopra un nemico, i 2 non si vedono (la linea di visuale passa al
		// centro perpendicolare)». La cella impilata sta a distanza ZERO sulla proiezione, quindi senza questa
		// riga la prende la consapevolezza ravvicinata, in ogni direzione. Vale a ogni distanza di piano.
		//
		// 🔑 **Sta QUI e non in `URTHexVisionLibrary::DescribeLineOfSight`, ed e' una scelta misurata.** La prima
		// stesura l'aveva messa nella linea di vista: la suite ha fatto cadere
		// `Bot.StalemateProbeGeneratedArenaFilterNeverStepsBack` e `…ClosesNoCycleOfAnyPeriod` sulle otto coppie
		// di celle impilate dell'arena generata. Da una cella esattamente sotto il bersaglio ogni cella che
		// «vede» e' piu' lontana: il filtro dei bot arretra, e l'orbita di #1287 si chiude. La linea di vista e'
		// la geometria su cui i bot scelgono le celle; il CONTATTO e' cio' che una squadra sa. La regola e' del
		// secondo, e colpire un altro piano e' gia' un rifiuto dichiarato (`D-393`, `D-509`).
		if (From.X == To.X && From.Y == To.Y && From.Layer != To.Layer)
		{
			return false;
		}

		const int32 Distance = URTHexLibrary::HexDistance(From, To);
		if (Distance > Range)
		{
			return false;
		}
		if (Distance > URTTerrainLibrary::EffectiveTargetingRange(Map, From, To, Range))
		{
			return false; // fumo dentro o attraverso: il contatto si ferma prima
		}
		return URTHexVisionLibrary::HasLineOfSight(Map, From, To);
	}
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
	// ➕ `D-510`, #3741: una cecita' ora e' stabilita, ed e' una sola. Due celle sulla STESSA colonna e su
	// piani diversi non hanno contatto: chi sta sopra la testa di un nemico non lo vede. Sta in
	// `ContactHolds`, qui sopra. ⚠️ Vale anche per la CELLA impilata, non solo per chi la occupa:
	// quell'osservatore non la vede, e il velo la disegna solo se la vede un compagno o se e' gia' ricordata.
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
