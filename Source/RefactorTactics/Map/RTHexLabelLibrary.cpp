#include "Map/RTHexLabelLibrary.h"

#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"

namespace
{
	/**
	 * I sette segmenti, nel quadrato unitario. Nomi da display digitale: A in alto, poi in senso orario,
	 * G di mezzo.
	 *
	 *      A          (0,1) ---- (1,1)
	 *    F   B          |    G     |
	 *      G          (0,.5) --- (1,.5)
	 *    E   C          |          |
	 *      D          (0,0) ---- (1,0)
	 */
	const FRTLabelStroke RTHexLabelSegA{ FVector2D(0.f, 1.f),  FVector2D(1.f, 1.f)  };
	const FRTLabelStroke RTHexLabelSegB{ FVector2D(1.f, 1.f),  FVector2D(1.f, 0.5f) };
	const FRTLabelStroke RTHexLabelSegC{ FVector2D(1.f, 0.5f), FVector2D(1.f, 0.f)  };
	const FRTLabelStroke RTHexLabelSegD{ FVector2D(0.f, 0.f),  FVector2D(1.f, 0.f)  };
	const FRTLabelStroke RTHexLabelSegE{ FVector2D(0.f, 0.5f), FVector2D(0.f, 0.f)  };
	const FRTLabelStroke RTHexLabelSegF{ FVector2D(0.f, 1.f),  FVector2D(0.f, 0.5f) };
	const FRTLabelStroke RTHexLabelSegG{ FVector2D(0.f, 0.5f), FVector2D(1.f, 0.5f) };
}

TArray<FRTLabelStroke> URTHexLabelLibrary::GlyphStrokes(TCHAR Character)
{
	switch (Character)
	{
	case TEXT('0'): return { RTHexLabelSegA, RTHexLabelSegB, RTHexLabelSegC, RTHexLabelSegD, RTHexLabelSegE, RTHexLabelSegF };
	case TEXT('1'): return { RTHexLabelSegB, RTHexLabelSegC };
	case TEXT('2'): return { RTHexLabelSegA, RTHexLabelSegB, RTHexLabelSegG, RTHexLabelSegE, RTHexLabelSegD };
	case TEXT('3'): return { RTHexLabelSegA, RTHexLabelSegB, RTHexLabelSegG, RTHexLabelSegC, RTHexLabelSegD };
	case TEXT('4'): return { RTHexLabelSegF, RTHexLabelSegG, RTHexLabelSegB, RTHexLabelSegC };
	case TEXT('5'): return { RTHexLabelSegA, RTHexLabelSegF, RTHexLabelSegG, RTHexLabelSegC, RTHexLabelSegD };
	case TEXT('6'): return { RTHexLabelSegA, RTHexLabelSegF, RTHexLabelSegG, RTHexLabelSegE, RTHexLabelSegD, RTHexLabelSegC };
	case TEXT('7'): return { RTHexLabelSegA, RTHexLabelSegB, RTHexLabelSegC };
	case TEXT('8'): return { RTHexLabelSegA, RTHexLabelSegB, RTHexLabelSegC, RTHexLabelSegD, RTHexLabelSegE, RTHexLabelSegF, RTHexLabelSegG };
	case TEXT('9'): return { RTHexLabelSegA, RTHexLabelSegB, RTHexLabelSegC, RTHexLabelSegD, RTHexLabelSegF, RTHexLabelSegG };

	// Il meno e' il segmento di mezzo: stessa altezza della barra del `4`, quindi non si confonde con
	// una cifra e si legge alla stessa quota.
	case TEXT('-'): return { RTHexLabelSegG };

	// La virgola vive nella meta' bassa e sporge a sinistra: senza la coda si leggerebbe come un punto,
	// e la terna `0.0.0` non e' la terna `0,0,0`.
	case TEXT(','): return {
		FRTLabelStroke{ FVector2D(0.45f, 0.2f), FVector2D(0.35f, 0.f) } };

	default:
		// ⛔ Nessun glifo di ripiego: vedi la doc dell'header.
		return {};
	}
}

FRTCellLabel URTHexLabelLibrary::BuildCellLabel(const FRTCellId& Cell, const FVector& Origin,
	float HexSize, float LayerHeight)
{
	FRTCellLabel Out;

	const TArray<FVector> Corners = URTHexLibrary::CellCorners(Cell, Origin, HexSize, LayerHeight);
	if (Corners.Num() != 6)
	{
		return Out; // geometria inattesa: meglio nessuna etichetta che una posata su un'ipotesi
	}
	const FVector Centre = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);

	// 🔑 I punti medi si DERIVANO dai vertici (vedi la doc): il lato `k` va da `Corners[k]` a
	// `Corners[k+1]`, e il suo punto medio cade a `60k` gradi. Prendendo i lati pari si ottengono
	// esattamente 0, 120 e 240.
	const FString Text = FString::Printf(TEXT("%d,%d,%d"), Cell.X, Cell.Y, Cell.Layer);
	const int32 LayerDigits = FString::FromInt(Cell.Layer).Len();

	// L'altezza piena e la larghezza di un carattere, tarate sull'APOTEMA e sul caso peggiore. Non sono
	// numeri scelti a occhio: sono il piu' grande valore per cui `NothingLeavesTheHexagon` resta verde
	// su `-10,-10,1`, cioe' dieci caratteri lungo una direzione.
	constexpr int32 WorstCaseChars = 10;
	const double Apothem   = HexSize * 0.8660254; // cos(30°)
	const double Margin    = HexSize * 0.06;      // il bordo non si tocca: la cella ha gia' un contorno
	const double Usable    = Apothem - Margin;
	const double CharWidth = Usable / WorstCaseChars;
	const double FullHeight = CharWidth * 1.4;    // rapporto di un display a sette segmenti

	for (int32 Side = 0; Side < 6; Side += 2)
	{
		const FVector Mid = (Corners[Side] + Corners[(Side + 1) % 6]) * 0.5;

		// Verso il CENTRO: la prima cifra sta al bordo e l'ultima al centro, come chiesto.
		const FVector Inward = (Centre - Mid).GetSafeNormal();
		const FVector Right  = -Inward;              // il testo si legge venendo dal bordo
		const FVector Up     = FVector::CrossProduct(FVector::UpVector, Right).GetSafeNormal();

		double Travelled = Margin;
		for (int32 I = 0; I < Text.Len(); ++I)
		{
			const TCHAR Ch = Text[I];

			// ⚠️ Il layer a META' scala: sono gli ULTIMI `LayerDigits` caratteri, non «tutto dopo la
			// seconda virgola» — contare le virgole rompeva su coordinate negative, dove il meno non e'
			// un separatore ma fa parte del numero.
			const bool bIsLayer = I >= Text.Len() - LayerDigits;
			const double Scale  = bIsLayer ? 0.5 : 1.0;

			const double W = CharWidth * Scale;
			const double H = FullHeight * Scale;

			// L'angolo in basso a sinistra: si parte dal bordo e si cammina verso il centro, e il
			// carattere e' centrato sull'asse della run.
			const FVector Base = Mid + Inward * (Travelled + W) + Up * (-H * 0.5);

			FRTLabelGlyph Glyph;
			Glyph.Character = Ch;
			Glyph.Origin    = Base;
			Glyph.Right     = Right * W;
			Glyph.Up        = Up * H;
			Out.Glyphs.Add(Glyph);

			Travelled += W * 1.15; // il 15% e' la spaziatura fra caratteri
		}
	}

	return Out;
}
