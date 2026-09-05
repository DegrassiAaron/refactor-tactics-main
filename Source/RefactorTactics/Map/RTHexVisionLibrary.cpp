#include "Map/RTHexVisionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexOcclusionLibrary.h"

FRTLineOfSightResult URTHexVisionLibrary::DescribeLineOfSight(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
{
	FRTLineOfSightResult Result;

	if (!Map)
	{
		return Result; // nessun dato di mappa: nessun ostacolo noto
	}
	if (From.X == To.X && From.Y == To.Y)
	{
		return Result; // stessa colonna: non c'e' nulla in mezzo
	}

	// La linea e' planare e resta sul layer del TIRATORE (HexLine usa il layer di A): e' la regola di elevazione
	// del quadrato: un ostacolo blocca solo se sta al livello di chi spara.
	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(From, To);

	// La GEOMETRIA INTRA-CELLA della cella che si sta lasciando (`D-269`, `#1830`). Il passo `I` guarda la
	// cella `I - 1`, cosi' che l'ultima venga esaminata dopo il ciclo: e' l'ordine che tiene ogni cella
	// esaminata una volta sola, con la corda che la attraversa davvero.
	//
	// `Prev == Cell` dice a `BlocksSight` che la linea NASCE li' (il capo della corda e' il centro); nel
	// ciclo il vicino d'ingresso c'e' sempre tranne al primo passo.
	auto InteriorBlocks = [&Line](const URTHexMapAsset* M, int32 Index) -> bool
	{
		const FRTCellId& Cell = Line[Index];
		const FRTCellId& Prev = (Index > 0) ? Line[Index - 1] : Cell;
		const FRTCellId& Next = (Index + 1 < Line.Num()) ? Line[Index + 1] : Cell;
		return URTHexOcclusionLibrary::BlocksSight(M, Prev, Cell, Next);
	};

	for (int32 I = 1; I < Line.Num(); ++I)
	{
		// Prima la cella che si lascia, poi il bordo che si attraversa: la corda sta DENTRO la cella, quindi
		// viene percorsa prima di uscirne. L'ordine e' dichiarato perche' decide quale ragione vince quando
		// due ostacoli sono in fila, e non perche' un ordine sia piu' vero dell'altro.
		if (InteriorBlocks(Map, I - 1))
		{
			Result.Block = ERTLineOfSightBlock::InteriorGeometry;
			Result.BlockedFrom = (I >= 2) ? Line[I - 2] : Line[I - 1];
			Result.BlockedAt = Line[I - 1];
			Result.StepIndex = I - 1;
			return Result;
		}

		// Copertura ALTA sul bordo ATTRAVERSATO (CP 9.2): a differenza di `bBlocksLineOfSight`, che e' una
		// proprieta' della CELLA, questa sta fra due celle — quindi conta anche il primo e l'ultimo passo,
		// che il ciclo per-cella esclude. Un muro addossato al bersaglio lo copre: non "si copre da solo",
		// e' la barriera davanti a lui.
		if (URTHexCoverLibrary::BlocksTraversal(Map, Line[I - 1], Line[I]))
		{
			Result.Block = ERTLineOfSightBlock::EdgeBlocker;
			Result.BlockedFrom = Line[I - 1];
			Result.BlockedAt = Line[I];
			Result.StepIndex = I;
			return Result;
		}

		if (I == Line.Num() - 1)
		{
			break; // estremi esclusi dalla regola per-cella: non ci si oscura da soli
		}
		if (const FRTHexCellData* Data = Map->FindCell(Line[I]))
		{
			if (Data->bBlocksLineOfSight)
			{
				Result.Block = ERTLineOfSightBlock::CellBlocker;
				Result.BlockedFrom = Line[I - 1];
				Result.BlockedAt = Line[I];
				Result.StepIndex = I;
				return Result;
			}
		}
		// cella assente = buco nella mappa: non e' un muro, la vista passa
	}

	// L'ULTIMA cella, che il ciclo lascia fuori: la sua corda va dal lato d'ingresso al CENTRO, dove sta il
	// bersaglio. Un muro fra il bordo e lui lo protegge, ed e' la stessa asimmetria con cui `EdgeBlocker`
	// conta l'ultimo passo: cio' che si esclude e' coprirsi da soli, non una barriera davanti.
	const int32 Last = Line.Num() - 1;
	if (Last > 0 && InteriorBlocks(Map, Last))
	{
		Result.Block = ERTLineOfSightBlock::InteriorGeometry;
		Result.BlockedFrom = Line[Last - 1];
		Result.BlockedAt = Line[Last];
		Result.StepIndex = Last;
		return Result;
	}

	return Result;
}

bool URTHexVisionLibrary::HasLineOfSight(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
{
	// Un solo attraversamento della linea in tutto il progetto: il bool E' `Reason == None`, non una seconda
	// lettura delle stesse due condizioni. Vedi il commento esteso su `DescribeLineOfSight`.
	return DescribeLineOfSight(Map, From, To).IsClear();
}
