#include "RTHexHoverGate.h"

namespace RTHexHover
{
	bool ShouldRequery(bool bLastValid, const FRTCellId& Last, bool bNowValid, const FRTCellId& Now)
	{
		if (bLastValid != bNowValid)
		{
			// Entrare o uscire dalla mappa e' un cambio: il pannello deve smettere di mostrare l'ultimo
			// verdetto quando il cursore esce, invece di lasciarlo li' come se valesse ancora.
			return true;
		}
		if (!bNowValid)
		{
			return false; // fuori mappa prima e adesso: non c'e' niente di nuovo da chiedere
		}
		// `operator==` di `FRTCellId` confronta anche il `Layer`, ed e' il comportamento che serve.
		return !(Last == Now);
	}
}
