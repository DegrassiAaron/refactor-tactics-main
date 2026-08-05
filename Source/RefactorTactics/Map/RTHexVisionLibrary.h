#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTHexVisionLibrary.generated.h"

class URTHexMapAsset;

/**
 * Linea di vista sulla mappa esagonale: decide se un bersaglio e' colpibile (logica autoritativa, non
 * presentazione). Pura e deterministica, interi soltanto (la linea viene da URTHexLibrary::HexLine).
 *
 * Separata da URTHexLibrary perche' ha bisogno dei dati d'asset (bBlocksLineOfSight), come URTHexPathLibrary
 * per il grafo di movimento. Vedi docs/design/h6-4-hex-vision-spec.md.
 */
UCLASS()
class REFACTORTACTICS_API URTHexVisionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Vero se la linea di tiro From->To non attraversa celle che bloccano la vista.
	 *
	 * Regola di ELEVAZIONE (identica al quadrato, URTGridLibrary::HasLineOfSight): un ostacolo blocca solo se
	 * sta sul layer del TIRATORE -> da terra si spara sotto un ponte, da un piano superiore si spara oltre le
	 * coperture basse. Gli ESTREMI non bloccano mai (tiratore e bersaglio non si coprono da soli). Una cella
	 * ASSENTE dall'asset non blocca (il vuoto non e' un muro). Mappa nulla o From == To -> vero.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool HasLineOfSight(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);
};
