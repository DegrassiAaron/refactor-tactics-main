#include "Map/RTHexLabelLibrary.h"

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
