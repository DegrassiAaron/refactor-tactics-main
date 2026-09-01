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

	const FVector Centre = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight);

	const FString Text = FString::Printf(TEXT("%d,%d,%d"), Cell.X, Cell.Y, Cell.Layer);
	const int32 LayerDigits = FString::FromInt(Cell.Layer).Len();

	// L'altezza piena e la larghezza di un carattere, tarate sul caso peggiore: dieci caratteri, che
	// `-10,-10,-1` raggiunge (`Layer` e' un `int32` senza limiti dichiarati, quindi un layer negativo a due
	// cifre e' una cella legittima). Chi verifica che la taratura sia vera, e non solo plausibile, sono
	// `NothingLeavesTheHexagon` e il suo gemello sul vero caso peggiore.
	//
	// 🔑 **Il budget e' la corda LUNGA dell'esagono, e passa per il centro.** Un pointy-top di circumraggio
	// `R` e' largo `2R` lungo `Y` — dai due vertici a `90` e `270` gradi — contro `R*sqrt(3)` lungo `X`.
	// La riga corre lungo `Y` e sta al centro, quindi prende la corda maggiore che l'esagono abbia.
	//
	// A quota `x` dal centro la semi-larghezza vale `R - |x|/sqrt(3)`, e la riga occupa `[-H/2, +H/2]`:
	// il punto piu' stretto della fascia e' quindi a `H/2`, e l'altezza entra nel budget della LARGHEZZA.
	// E' un'equazione, non due numeri tarati a occhio:
	//
	//     N*1.15*W + 1.4*W/sqrt(3) = 2R - 2*Margin        con H = 1.4*W
	//
	// ⚠️ **Il denominatore e' cambiato due volte, e ogni volta perche' era cambiata la GEOMETRIA**: era
	// l'apotema quando il testo correva in direzione radiale, poi la corda parallela al lato, adesso la
	// corda lunga. Un budget che sopravvive a un cambio di direzione e' un numero orfano: misura una
	// direzione lungo la quale non c'e' piu' niente.
	constexpr int32 WorstCaseChars = 10;
	constexpr double GlyphAspect   = 1.4;        // rapporto di un display a sette segmenti
	constexpr double Tracking      = 1.15;       // il 15% e' la spaziatura fra caratteri
	const double Margin = HexSize * 0.06;        // il bordo non si tocca: la cella ha gia' un contorno
	const double CharWidth  = (2.0 * HexSize - 2.0 * Margin)
	                        / (WorstCaseChars * Tracking + GlyphAspect / 1.7320508);
	const double FullHeight = CharWidth * GlyphAspect;

	// 🔴 **UNA SOLA run, dritta e centrata** (2026-09-01). Erano tre, ai punti medi di tre lati alternati,
	// e la ragione dichiarata era: *«girando attorno alla cella, almeno una delle tre e' leggibile»*.
	// Guardata a schermo, quella promessa si legge al rovescio: da una camera FERMA — che e' il caso
	// normale, non l'eccezione — una sola e' dritta e **le altre due sono ruotate di 120 gradi**, cioe'
	// due terzi dell'inchiostro e' rumore che sembra una coordinata sbagliata.
	//
	// Tre conseguenze, tutte misurate:
	//
	//  - la riga prende la corda LUNGA invece di quella parallela a un lato: `CharWidth` passa da
	//    `12,38` a `22,91` uu con `HexSize = 150`, **+85%** — ed e' la ragione per cui le coordinate
	//    «non si vedevano»;
	//  - le linee emesse diventano **un terzo**. `PIE-HEX-COORD-COSTO` ha misurato `567 090` linee
	//    sull'arena piena a raggio 50: e' il rischio principale di questa feature, e questa scelta lo
	//    divide per tre senza toccare nient'altro;
	//  - il centro della cella non e' piu' libero. ⚠️ La riserva si sposta li': la terna adesso compete
	//    col **disco della cella** e col glifo di superficie, che occupano lo stesso pavimento.
	//
	// ⛔ Chi vuole leggerla da un'altra angolazione ruota la camera, non la cella: il testo su un tabellone
	// ha un verso, come su una plancia da tavolo.
	//
	// ⚠️ **Gli assi vengono dalla convenzione del motore, non da una scelta di stile.** Unreal e' mancino
	// e la sua vista dall'alto mette `X` in SU e `Y` a DESTRA sullo schermo: sono esattamente `Up` e
	// `Right` del testo. La terna `(Right, Up, Z)` risulta `Right x Up = -Z`, ed e' la condizione di
	// leggibilita' — il segno opposto a quello della base destrorsa della matematica, che e' l'errore con
	// cui questa funzione ha gia' spedito glifi riflessi una volta.
	const FVector Right = FVector::YAxisVector;
	const FVector Up    = FVector::XAxisVector;

	// La larghezza totale serve PRIMA di posare il primo carattere: la riga e' centrata sulla cella,
	// quindi bisogna sapere di quanto arretrare l'inizio.
	double TotalWidth = 0.0;
	for (int32 I = 0; I < Text.Len(); ++I)
	{
		const double W = CharWidth * ((I >= Text.Len() - LayerDigits) ? 0.5 : 1.0);
		TotalWidth += (I == Text.Len() - 1) ? W : W * Tracking; // nessuna spaziatura dopo l'ultimo
	}

	// Centrata sui due assi: la base scende di meta' altezza, l'inizio arretra di meta' larghezza.
	const FVector RunStart = Centre - Up * (FullHeight * 0.5) - Right * (TotalWidth * 0.5);

	double Travelled = 0.0;
	for (int32 I = 0; I < Text.Len(); ++I)
	{
		// ⚠️ Il layer a META' scala: sono gli ULTIMI `LayerDigits` caratteri, non «tutto dopo la seconda
		// virgola» — contare le virgole rompeva su coordinate negative, dove il meno non e' un separatore
		// ma fa parte del numero.
		const double Scale = (I >= Text.Len() - LayerDigits) ? 0.5 : 1.0;
		const double W = CharWidth * Scale;
		const double H = FullHeight * Scale;

		// L'angolo in basso a sinistra. Tutti i caratteri poggiano sulla STESSA base: quelli a meta' scala
		// sono piu' bassi, non centrati verticalmente.
		FRTLabelGlyph Glyph;
		Glyph.Character = Text[I];
		Glyph.Origin    = RunStart + Right * Travelled;
		Glyph.Right     = Right * W;
		Glyph.Up        = Up * H;
		Out.Glyphs.Add(Glyph);

		Travelled += W * Tracking;
	}

	return Out;
}
