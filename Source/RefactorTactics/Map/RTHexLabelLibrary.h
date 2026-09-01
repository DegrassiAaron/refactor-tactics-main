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
	 * Tre run **posate** ai punti medi di tre lati alternati — `0/120/240` gradi, **non i vertici**: con
	 * la convenzione pointy-top di `HexCorners` i vertici stanno a `-30/30/90/150/210/270`, e `0/120/240`
	 * cade sui punti medi. Il `layer` e' a meta' scala.
	 *
	 * 🔑 **Ogni run corre PARALLELA al proprio lato**, non dal bordo verso il centro. Il testo e' quindi
	 * orientato a `90/210/330` gradi — le direzioni dei tre lati — mentre le sue POSIZIONI restano a
	 * `0/120/240`. Sono due angoli diversi della stessa run, ed e' la distinzione che il difetto di
	 * partenza confondeva: la riga correva lungo il raggio, cioe' lungo l'unica direzione in cui
	 * l'esagono e' piu' stretto, e per giunta con i caratteri posati **all'indietro** rispetto al proprio
	 * asse di lettura.
	 *
	 * 🔴 **L'alto dei caratteri guarda verso il BORDO, non verso il centro**, e il verso non e' una
	 * preferenza: e' cio' che li rende leggibili da una camera dall'alto. La prima stesura lo aveva al
	 * contrario e i glifi uscivano **ribaltati sull'orizzontale** — `2` disegnato come `5`, che a sette
	 * segmenti ne e' esattamente il ribaltamento, e la virgola in alto. L'errore era di convenzione:
	 * Unreal e' **mancino** e la sua vista dall'alto mette `X` in su e `Y` a destra, quindi la condizione
	 * di leggibilita' e' l'opposta di quella della base destrorsa della matematica.
	 *
	 * ⚠️ I sei vertici li da' `URTHexLibrary::CellCorners`, e i punti medi e le direzioni dei lati si
	 * DERIVANO da quelli: una seconda formula per la stessa forma e' il difetto visto in `U22` — celle
	 * piene tonde e contorno esagonale.
	 */
	static FRTCellLabel BuildCellLabel(const FRTCellId& Cell, const FVector& Origin,
		float HexSize, float LayerHeight);
};
