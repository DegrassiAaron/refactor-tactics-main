#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLabel.h"

#include "RTHexLabelLibrary.generated.h"

/**
 * Le coordinate della cella incise sul pavimento (#1920): dove cade ogni carattere, e che forma ha.
 *
 * ⛔ **Non disegna niente.** Restituisce pose; chi le traccia e' il guscio d'editor. Se un giorno questo
 * file includesse un componente o un PDI, sarebbe la presentazione entrata nella regola.
 */
UCLASS()
class REFACTORTACTICS_API URTHexLabelLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * I segmenti che disegnano un carattere dentro il quadrato unitario.
	 *
	 * Set CHIUSO: `0`-`9`, `,` e `-`. Un carattere fuori set restituisce un array vuoto — meglio niente
	 * che un glifo inventato, che a schermo sembrerebbe una cifra sbagliata invece che un dato mancante.
	 */
	static TArray<FRTLabelStroke> GlyphStrokes(TCHAR Character);

	/**
	 * Dove cade ogni carattere della terna `(x, y, layer)` di questa cella.
	 *
	 * Tre run a `0/120/240` gradi — i punti medi di tre lati alternati, **non i vertici**: con la
	 * convenzione pointy-top di `HexCorners` i vertici stanno a `-30/30/90/150/210/270`, e `0/120/240`
	 * cade sui punti medi. Ogni run corre **dal bordo verso il centro**, e il `layer` e' a meta' scala.
	 *
	 * ⚠️ I sei vertici li da' `URTHexLibrary::CellCorners`, e i punti medi si DERIVANO da quelli: una
	 * seconda formula per la stessa forma e' il difetto visto in `U22` — celle piene tonde e contorno
	 * esagonale.
	 */
	static FRTCellLabel BuildCellLabel(const FRTCellId& Cell, const FVector& Origin,
		float HexSize, float LayerHeight);
};
