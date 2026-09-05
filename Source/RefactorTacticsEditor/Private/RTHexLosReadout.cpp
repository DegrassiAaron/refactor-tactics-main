#include "RTHexLosReadout.h"

#include "RTHexHoverGate.h"
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

		case ERTLineOfSightBlock::InteriorGeometry:
			// La geometria intra-cella (`D-269`, `#1830`). Si nomina UNA cella — quella che contiene il muro —
			// e non due come `EdgeBlocker`: il segmento sta DENTRO, non fra due celle. Non si nomina il muro
			// perche' neanche la ragione lo porta: chi vuole sapere quale segmento e' stato chiede a
			// `URTHexOcclusionLibrary`, che ne e' l'autorita'.
			Out.Reason = FString::Printf(TEXT("InteriorGeometry  %s"), *CellText(Los.BlockedAt));
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
		// 🔵 Il corpo si e' spostato in `RTHexHover` il 2026-08-31, quando la sonda di movimento (#711) ne ha
		// avuto bisogno IDENTICO. Ricopiarlo avrebbe dato due cancelli con la stessa regola; la firma qui non
		// cambia, e i quattro casi che questa funzione ha sempre coperto restano i suoi test — che ora
		// misurano anche l'estrazione.
		return RTHexHover::ShouldRequery(bLastValid, Last, bNowValid, Now);
	}
}
