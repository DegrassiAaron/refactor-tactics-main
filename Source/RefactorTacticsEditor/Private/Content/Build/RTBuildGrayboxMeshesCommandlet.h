#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTBuildGrayboxMeshesCommandlet.generated.h"

/**
 * Costruisce le mesh del kit graybox e le SALVA come `.uasset` ai path di `D-173`.
 *
 * Perche' generate e non modellate — e' `D-229`: i budget di forma sono FRAZIONI (contratto graybox
 * §6.3), e una funzione che le applica si diffa e si testa mentre un binario autorato no. I vertici
 * di pianta vengono da `URTHexLibrary`, mai da una seconda trigonometria scritta qui: e' la regola
 * di `spec-hex-geometry-authoring.md` §4.
 *
 * Perche' salvate e non transienti — sempre `D-229`: una mesh in `GetTransientPackage()` NON si
 * serializza in un `.umap`, quindi la scena di validazione di U25 perderebbe i suoi riferimenti alla
 * riapertura. `ARTHexMapActor::GetCellPrismMesh` puo' permetterselo perche' nasce a ogni avvio dentro
 * l'attore che la consuma; un asset posato in un livello salvato no.
 *
 * ⚠️ Le mesh nascono alle misure CANONICHE — `HexSize` e `LayerHeight` letti dal CDO di
 * `URTHexMapAsset`, non ricopiati — e chi le posa su una mappa che sovrascrive `HexSize` le scala di
 * `HexSize / canonico`. Il contratto misura in frazioni proprio perche' quel rapporto esista.
 *
 * Uso:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildGrayboxMeshes [-DryRun]
 *         [-Only=Cover_Low] [-Package=/Game/RT/World/Graybox]
 *
 * `-DryRun` non scrive nulla: stampa le misure derivate e i path che toccherebbe. E' il primo comando
 * da lanciare, come per `RTBuildIconCatalog`, perche' un budget sbagliato si vede nei numeri prima
 * che a schermo.
 */
UCLASS()
class URTBuildGrayboxMeshesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTBuildGrayboxMeshesCommandlet();

	virtual int32 Main(const FString& Params) override;
};
