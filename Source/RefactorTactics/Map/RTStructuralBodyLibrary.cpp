#include "Map/RTStructuralBodyLibrary.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapVisuals.h"

float URTStructuralBodyLibrary::FillFraction(ERTHexBodyFill Fill)
{
	switch (Fill)
	{
	case ERTHexBodyFill::Third:      return 1.f / 3.f;
	case ERTHexBodyFill::TwoThirds:  return 2.f / 3.f;
	case ERTHexBodyFill::Full:       return 1.f;
	case ERTHexBodyFill::None:
	default:                         return 0.f;
	}
}

TArray<FRTStructuralBody> URTStructuralBodyLibrary::DeriveBodies(const URTHexMapAsset* Map)
{
	TArray<FRTStructuralBody> Bodies;
	if (Map == nullptr || Map->NumCells() == 0)
	{
		return Bodies;
	}

	const float LayerH = Map->LayerHeight;

	// La quota del CENTRO di una cella, in spazio mappa: il piano da `Layer`, l'offset d'autore da `Height`.
	// E' la stessa composizione che `RebuildInstances` applica al rendering (`AxialToWorld` + `Height`),
	// scritta qui senza l'origine dell'actor perche' questa funzione e' pura.
	auto CenterZ = [LayerH](const FRTHexCellData& C)
	{
		return static_cast<float>(C.Id.Layer) * LayerH + static_cast<float>(C.Height);
	};

	// ⚠️ Ordine CANONICO e non quello dell'array: due asset con le stesse celle inserite in ordine diverso
	// devono produrre la stessa lista, ed e' l'AC di determinismo di #1865.
	TArray<FRTHexCellData> Sorted = Map->Cells;
	Sorted.Sort([](const FRTHexCellData& A, const FRTHexCellData& B)
	{
		return URTHexLibrary::StableLess(A.Id, B.Id);
	});

	for (const FRTHexCellData& Cell : Sorted)
	{
		const float Fraction = FillFraction(Cell.BodyFill);
		if (Fraction <= 0.f)
		{
			continue; // nessun corpo dichiarato: la superficie resta un disco, com'era prima di v15
		}

		FRTStructuralBody Body;
		Body.Cell = Cell.Id;

		// Il corpo comincia sotto il TILE, non sotto il centro: il tile e' spesso `2 * RTCellTopZ` e il
		// corpo che partisse dal centro lo attraverserebbe per meta'.
		Body.TopZ = CenterZ(Cell) - RTCellTopZ;

		// Cio' che l'autore ha chiesto.
		const float Requested = Fraction * LayerH;
		Body.BottomZ = Body.TopZ - Requested;

		// 🔑 **Il vincolo: la prima cella SOTTO nella stessa colonna.** Si cerca la piu' ALTA fra quelle il
		// cui tile sta sotto questa superficie — non semplicemente `Layer - 1`, perche' `Height` puo' aver
		// spostato le quote e la colonna puo' saltare dei piani.
		const FRTHexCellData* Below = nullptr;
		float BelowTop = -FLT_MAX;
		for (const FRTHexCellData& Other : Sorted)
		{
			if (Other.Id.X != Cell.Id.X || Other.Id.Y != Cell.Id.Y || Other.Id == Cell.Id)
			{
				continue; // altra colonna, o la superficie stessa
			}
			const float OtherTop = CenterZ(Other) + RTCellTopZ;
			if (OtherTop <= Body.TopZ && OtherTop > BelowTop)
			{
				BelowTop = OtherTop;
				Below = &Other;
			}
		}

		if (Below != nullptr && Body.BottomZ < BelowTop)
		{
			// La frazione avrebbe portato la base DENTRO la cella sottostante: si tronca sulla sua faccia
			// superiore. Le due geometrie si toccano, e nessuna compenetra l'altra.
			Body.BottomZ = BelowTop;
			Body.bTruncated = true;
		}

		if (Body.BottomZ >= Body.TopZ)
		{
			// Spazio esaurito: la cella sottostante e' immediatamente sotto il tile. Un corpo di altezza
			// nulla non e' un corpo, e disegnarlo produrrebbe due facce coincidenti — z-fighting garantito.
			continue;
		}

		Bodies.Add(Body);
	}

	return Bodies;
}
