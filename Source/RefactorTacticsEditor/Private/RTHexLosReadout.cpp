#include "RTHexLosReadout.h"

#include "Map/RTHexVisionLibrary.h"

namespace RTHexLos
{
	namespace
	{
		/** Cio' che si scrive quando manca un estremo. Un trattino, non una stringa vuota: il campo esiste. */
		const FString Dash(TEXT("—"));

		FString CellText(const FRTCellId& Cell)
		{
			return FString::Printf(TEXT("(%d,%d,L%d)"), Cell.X, Cell.Y, Cell.Layer);
		}
	}

	FReadout Describe(bool bHasOrigin, const FRTCellId& Origin,
		bool bHasTarget, const FRTCellId& Target,
		const FRTLineOfSightResult& Los)
	{
		FReadout Out;
		Out.Layer = Origin.Layer;

		if (!bHasOrigin || !bHasTarget)
		{
			// Nessuna delle due righe si riempie: senza due punti non c'e' una linea di cui dire qualcosa.
			Out.Verdict = Dash;
			Out.Reason = Dash;
			return Out;
		}

		if (Los.IsClear())
		{
			Out.Verdict = TEXT("CLEAR");
			Out.Reason = Dash;
			return Out;
		}

		Out.Verdict = TEXT("BLOCKED");

		switch (Los.Block)
		{
		case ERTLineOfSightBlock::EdgeBlocker:
			// Il bordo e' fra DUE celle, quindi si nominano entrambe: dire solo quella d'arrivo lascerebbe
			// ambiguo quale dei suoi sei lati sta bloccando.
			Out.Reason = FString::Printf(TEXT("EdgeBlocker  %s -> %s"),
				*CellText(Los.BlockedFrom), *CellText(Los.BlockedAt));
			break;

		case ERTLineOfSightBlock::CellBlocker:
			Out.Reason = FString::Printf(TEXT("CellBlocker  %s"), *CellText(Los.BlockedAt));
			break;

		default:
			// ⚠️ Un valore che questo `switch` non conosce. Si dichiara `unavailable` invece di ripiegare su
			// una delle due cause note: una ragione plausibile e sbagliata e' peggio di nessuna ragione, e
			// #1755 lo mette per iscritto — *«e' preferibile a inventare una ragione»*.
			Out.Reason = TEXT("unavailable");
			break;
		}

		return Out;
	}

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
