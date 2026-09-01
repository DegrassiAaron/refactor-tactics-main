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

	// L'altezza piena e la larghezza di un carattere, tarate sul caso peggiore: dieci caratteri lungo una
	// direzione (`Layer` e' un int32 senza limiti dichiarati, quindi un layer a due cifre negative e' una
	// cella legittima). Chi verifica che la taratura sia vera, e non solo plausibile, e'
	// `NothingLeavesTheHexagonAtTheTrueWorstCase` su `-10,-10,-1`.
	//
	// 🔑 **Il budget e' la CORDA parallela al lato, non l'apotema** — e il denominatore e' cambiato quando
	// il testo ha smesso di correre in direzione radiale. Per un esagono di circumraggio `R` la corda
	// parallela a un lato, a distanza `t` da quel lato, misura `R + 2t/sqrt(3)`: vale `R` sul lato stesso
	// e `2R` passando per il centro. La base del testo sta a `Margin` dal lato, cioe' nel punto piu'
	// STRETTO della fascia che il testo occupa — salendo verso il centro la corda si allarga soltanto.
	// E' quella la corda che vincola, e usare l'apotema qui sarebbe un numero orfano: misurerebbe una
	// direzione lungo la quale non c'e' piu' niente.
	constexpr int32 WorstCaseChars = 10;
	const double Margin = HexSize * 0.06;   // il bordo non si tocca: la cella ha gia' un contorno
	const double Chord  = HexSize + 2.0 * Margin / 1.7320508;
	const double Usable = Chord - 2.0 * Margin;  // un margine anche ai due capi della riga
	// `* 1.15` perche' la spaziatura fra caratteri va pagata: e' il caso peggiore in cui tutti e dieci
	// fossero a scala piena, che il layer a meta' scala non raggiunge mai.
	const double CharWidth  = Usable / (WorstCaseChars * 1.15);
	const double FullHeight = CharWidth * 1.4;   // rapporto di un display a sette segmenti

	for (int32 Side = 0; Side < 6; Side += 2)
	{
		const FVector Mid    = (Corners[Side] + Corners[(Side + 1) % 6]) * 0.5;
		const FVector Inward = (Centre - Mid).GetSafeNormal();

		// 🔑 **Il testo corre PARALLELO al lato e cresce verso il centro.** Prima correva in direzione
		// radiale — perpendicolare al lato — e si leggeva male per due ragioni indipendenti:
		//
		//  1. **era specchiato.** I glifi avanzavano lungo `Inward` mentre il loro asse `Right` valeva
		//     `-Inward`: `Right . avanzamento = -1` su tutte e tre le run. Ogni carattere era ben formato,
		//     ma la stringa era posata all'indietro — `3,-2,0` si leggeva `0,2-,3`;
		//  2. **era perpendicolare al lato**, cioe' orientato lungo l'unica direzione in cui l'esagono e'
		//     piu' stretto invece che lungo quella in cui e' piu' largo.
		//
		// `Up = Inward` fa crescere il testo verso il centro, che e' cio' che lo tiene dentro l'esagono.
		// `Right = Inward x Z` e' parallelo al lato **per costruzione** e non per una tabella di angoli:
		// se `HexCorners` cambiasse convenzione, questo la seguirebbe.
		//
		// ⚠️ La terna `(Right, Up, Z)` e' destrorsa — `Right x Up = +Z` — ed e' cio' che rende i glifi
		// leggibili da una camera dall'alto invece che riflessi.
		const FVector Up    = Inward;
		const FVector Right = FVector::CrossProduct(Inward, FVector::UpVector);

		// La larghezza totale serve PRIMA di posare il primo carattere: la riga e' centrata sull'asse
		// radiale, quindi bisogna sapere di quanto arretrare l'inizio.
		double TotalWidth = 0.0;
		for (int32 I = 0; I < Text.Len(); ++I)
		{
			const double W = CharWidth * ((I >= Text.Len() - LayerDigits) ? 0.5 : 1.0);
			TotalWidth += (I == Text.Len() - 1) ? W : W * 1.15; // nessuna spaziatura dopo l'ultimo
		}

		const FVector RunStart = Mid + Inward * Margin - Right * (TotalWidth * 0.5);

		double Travelled = 0.0;
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

			// L'angolo in basso a sinistra. Tutti i caratteri poggiano sulla STESSA base a `Margin` dal
			// lato: quelli a meta' scala sono piu' bassi, non centrati verticalmente.
			const FVector Base = RunStart + Right * Travelled;

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
