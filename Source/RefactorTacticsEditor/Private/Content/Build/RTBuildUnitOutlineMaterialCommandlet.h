#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTBuildUnitOutlineMaterialCommandlet.generated.h"

/**
 * Costruisce `M_UnitLayerOutline`, il post-process che disegna il contorno con cui un'unita' dice il suo piano
 * (#3705, [D-508]), e lo SALVA al percorso di `RTUnitLayerOutline`.
 *
 * Perche' generato e non autorato a mano — e' `D-229`: il materiale e' un nodo Custom il cui codice dipende dagli
 * stencil del runtime, e una funzione che lo scrive si diffa e si testa mentre un grafo in un binario no. C'e' anche
 * una ragione misurata: collegare un pin radice fuori dal Material Editor lascia accesa la costante in linea, e il
 * nodo collegato non si compila (`M_HexCell`, da #3679 a #3704). La ricetta la spegne, e un test lo guarda.
 *
 * Uso, dal clone principale:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildUnitOutlineMaterial [-DryRun]
 *
 * `-DryRun` non scrive nulla: stampa il percorso, gli stencil e il codice del nodo. E' il primo comando da lanciare.
 */
UCLASS()
class URTBuildUnitOutlineMaterialCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTBuildUnitOutlineMaterialCommandlet();

	virtual int32 Main(const FString& Params) override;
};
