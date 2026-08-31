#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
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
};
