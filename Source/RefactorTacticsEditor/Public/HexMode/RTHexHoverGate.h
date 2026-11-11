#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

/**
 * Il cancello dell'hover, condiviso da chi risponde a una cella sorvolata nel viewport.
 *
 * 🔴 **Estratto, non inventato.** La regola nasce con l'ispettore LOS (#1755) come `RTHexLos::ShouldRequery`
 * e ha i suoi test; la sonda di movimento (#711) ne aveva bisogno identica. Ricopiarla avrebbe dato due
 * cancelli con la stessa regola — che divergono, e il giorno in cui divergono se ne accorge chi vede il
 * framerate cadere, non un test. `RTHexLos::ShouldRequery` resta al suo posto e **delega qui**: la firma
 * pubblica non cambia e i test di #1755 restano l'oracolo di questa estrazione.
 *
 * ⚠️ Non e' un'ottimizzazione: e' cio' che tiene una query event-driven dall'essere una query per
 * fotogramma. Il mouse produce eventi anche quando si muove DENTRO la stessa cella.
 */
namespace RTHexHover
{
	/**
	 * Va rifatta la domanda? Vero solo se la cella puntata e' cambiata — entrare e uscire dalla mappa
	 * inclusi, perche' un pannello che tiene l'ultimo verdetto quando il cursore esce mostra una risposta
	 * a una domanda che nessuno sta piu' facendo.
	 *
	 * Il `Layer` fa parte dell'identita' della cella: `(3,4,L0)` -> `(3,4,L1)` e' un cambio.
	 */
	bool ShouldRequery(bool bLastValid, const FRTCellId& Last, bool bNowValid, const FRTCellId& Now);
}
