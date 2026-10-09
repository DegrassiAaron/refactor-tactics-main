#pragma once

#include "CoreMinimal.h"
#include "Unit/RTUnit.h"
#include "RTUnitClassProbeForTest.generated.h"

/**
 * Una classe d'unita' distinguibile dal cilindro, solo per i test (`#3586`).
 *
 * Fa le veci di un `BP_Unit_*` con la mesh dell'eroe: quelli in un worktree non si caricano, perche'
 * `Content/FabAsset/` non e' versionato. Non aggiunge niente all'unita' — conta solo che `IsA` la distingua
 * da `ARTUnit`, cosi' un test puo' chiedere *quale* classe l'harness ha posato.
 */
UCLASS(NotBlueprintable, NotPlaceable, HideDropdown)
class ARTUnitClassProbeForTest : public ARTUnit
{
	GENERATED_BODY()
};

/** La gemella ASTRATTA (`#3586`): una classe che non si puo' spawnare deve ricadere sul cilindro. */
UCLASS(Abstract, NotBlueprintable, NotPlaceable, HideDropdown)
class ARTUnitAbstractProbeForTest : public ARTUnit
{
	GENERATED_BODY()
};
