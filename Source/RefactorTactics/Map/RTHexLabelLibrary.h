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
	 * 🔴 **UNA sola run, dritta e centrata sulla cella.** Il `layer` e' a meta' scala.
	 *
	 * Erano **tre**, ai punti medi di tre lati alternati, con la promessa *«girando attorno alla cella,
	 * almeno una e' leggibile»*. Guardata a schermo quella promessa si legge al rovescio: da una camera
	 * FERMA — il caso normale — una sola e' dritta e le altre due sono ruotate di `120` gradi, cioe' due
	 * terzi dell'inchiostro somiglia a una coordinata sbagliata. Chi vuole leggerla da un'altra angolazione
	 * ruota la camera, non la cella: il testo su un tabellone ha un verso, come su una plancia da tavolo.
	 *
	 * Le tre conseguenze, tutte misurate:
	 *
	 * | | |
	 * |---|---|
	 * | dimensione | la riga prende la corda **lunga** (`2R`) invece di una parallela a un lato: `CharWidth` da `12,38` a `22,91` uu con `HexSize = 150`, **+85%** |
	 * | costo | le linee emesse diventano **un terzo** — `PIE-HEX-COORD-COSTO` ne ha misurate `567 090` sull'arena piena |
	 * | ⚠️ riserva | il centro non e' piu' libero: la terna compete col **disco della cella** e col glifo di superficie |
	 *
	 * ⚠️ **Gli assi vengono dalla convenzione del motore, non da una scelta di stile.** Unreal e'
	 * **mancino** e la sua vista dall'alto mette `X` in su e `Y` a destra sullo schermo: sono esattamente
	 * `Up` e `Right` del testo, e la terna risulta `Right x Up = -Z`. E' il segno **opposto** a quello
	 * della base destrorsa della matematica — l'errore con cui questa funzione ha gia' spedito glifi
	 * riflessi una volta, `2` disegnato come `5` e la virgola in alto.
	 *
	 * ⛔ **Non usa piu' `CellCorners`**: non essendoci lati da seguire, la geometria che serve e' il solo
	 * centro. Il budget della larghezza pero' e' derivato — non tarato a occhio — dalla forma
	 * dell'esagono: vedi l'equazione nel `.cpp`.
	 */
	static FRTCellLabel BuildCellLabel(const FRTCellId& Cell, const FVector& Origin,
		float HexSize, float LayerHeight);
};
