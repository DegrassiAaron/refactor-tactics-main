#include "Perception/RTVisibilityBorder.h"

#include "Map/RTHexLibrary.h"

TArray<FRTExposedEdge> URTVisibilityBorderLibrary::ExposedEdges(const TArray<FRTCellId>& VisibleCells)
{
	// Il set prima del giro: l'appartenenza si chiede una volta per lato, e i duplicati nell'ingresso
	// smettono di esistere qui invece di moltiplicare i lati piu' avanti.
	const TSet<FRTCellId> Visible(VisibleCells);

	TArray<FRTExposedEdge> Out;
	Out.Reserve(Visible.Num()); // le celle interne non emettono: una stima bassa, non un limite

	static const ERTHexDirection AllDirections[] = {
		ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
		ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE
	};

	for (const FRTCellId& Cell : Visible)
	{
		for (const ERTHexDirection Dir : AllDirections)
		{
			// `Neighbor` resta sul layer della cella: due celle con la stessa X/Y e Layer diverso non si
			// vedono a vicenda come vicine, ed e' cio' che tiene i piani separati senza filtrarli qui.
			if (!Visible.Contains(URTHexLibrary::Neighbor(Cell, Dir)))
			{
				// Il vicino non e' nell'unione — cella non vista o vuoto oltre il bordo, che a schermo sono
				// la stessa cosa. Il lato appartiene alla cella VISIBILE, quindi si emette da qui e mai
				// dall'altra parte: emetterlo da entrambe lo poserebbe due volte nello stesso punto.
				Out.Emplace(Cell, Dir);
			}
		}
	}

	// L'ordine di un `TSet` dipende dall'hash e dall'ordine di inserimento: senza questo Sort il risultato
	// cambierebbe permutando le celle in ingresso, che e' l'invariante che `TeamVisibleCells` gia' garantisce
	// a monte e che sarebbe assurdo perdere qui. Stesso comparatore, per la stessa ragione.
	Out.Sort([](const FRTExposedEdge& A, const FRTExposedEdge& B)
	{
		if (A.Cell == B.Cell)
		{
			return static_cast<uint8>(A.Direction) < static_cast<uint8>(B.Direction);
		}
		return URTHexLibrary::StableLess(A.Cell, B.Cell);
	});

	return Out;
}
