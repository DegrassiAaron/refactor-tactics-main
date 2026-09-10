#include "Map/RTSightLines.h"

#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"

TArray<FRTSightLine> URTSightLineLibrary::AuthorizedSightLines(const URTHexMapAsset* Map,
	const FRTCellId& From, const TArray<FRTObservedTarget>& Targets)
{
	TArray<FRTSightLine> Lines;

	// ⛔ **FAIL-CLOSED, e per la stessa ragione di `ClassifyHexTargeting`**: senza mappa autorevole la
	// traiettoria non e' verificabile, quindi non si afferma niente su di essa. Non «nessun ostacolo»:
	// nessuna linea.
	if (Map == nullptr)
	{
		return Lines;
	}

	Lines.Reserve(Targets.Num());
	for (const FRTObservedTarget& Target : Targets)
	{
		// 🔴 **Il filtro di conoscenza viene PRIMA della geometria, e l'ordine e' il requisito.**
		//
		// Invertirlo non cambierebbe il risultato di questa funzione, ma cambierebbe cosa questa funzione
		// E': calcolare la linea verso un bersaglio ignoto e poi scartarla significa che il dato e' stato
		// prodotto, e un domani qualcuno lo restituirebbe «solo per il log». Qui la linea di un bersaglio
		// ignoto non viene mai calcolata.
		//
		// ⚠️ E il bersaglio ignoto **sparisce**, non produce una voce vuota: una voce «linea assente»
		// sarebbe distinguibile da «nessuna voce», e la differenza fra le due sarebbe il canale ([D-225]).
		if (!Target.bKnownToObserver)
		{
			continue;
		}

		FRTSightLine Line;
		Line.From = From;
		Line.To = Target.Cell;
		// La geometria resta di chi la possiede: nessun cammino riscritto qui.
		Line.Sight = URTHexVisionLibrary::DescribeLineOfSight(Map, From, Target.Cell);
		Lines.Add(Line);
	}

	return Lines;
}
