#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTBuildHexVolumeMaterialCommandlet.generated.h"

/**
 * Costruisce `M_HexVolume`, il materiale con cui muri, coperture, porte, blocchi e corpi della mappa dicono il loro
 * piano con una tinta e ricevono il trattamento dei piani (#3706, [D-508], [D-501]), e lo SALVA al percorso di
 * `RTLayerPresentation`.
 *
 * Perche' generato e non autorato a mano: e' `D-229`, con la stessa ragione misurata di `M_UnitLayerOutline` —
 * collegare un pin radice fuori dal Material Editor lascia accesa la costante in linea, e la maschera di `M_HexCell`
 * e' rimasta inerte da #3679 a #3704 proprio cosi'. La ricetta la spegne, e un test lo guarda.
 *
 * Uso, dal clone principale:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildHexVolumeMaterial [-DryRun]
 *
 * `-DryRun` non scrive nulla: stampa il percorso, il colore proprio e il codice del nodo. E' il primo comando da lanciare.
 */
UCLASS()
class URTBuildHexVolumeMaterialCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTBuildHexVolumeMaterialCommandlet();

	virtual int32 Main(const FString& Params) override;
};
