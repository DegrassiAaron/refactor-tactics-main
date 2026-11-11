#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTBuildIconCatalogCommandlet.generated.h"

/**
 * Costruisce `DA_IconCatalog` dalle icone generate, e lo fa in modo ripetibile.
 *
 * Perche' un commandlet e non una procedura a mano: le chiavi richieste sono **61**, e riempirle
 * cliccando significa 61 occasioni di scrivere `UI.Icon.Status.Wett` senza che nessuno se ne accorga
 * fino a schermo. Qui la chiave non si digita: si deriva da `URTIconLibrary::RequiredIconIds()`, che e'
 * la stessa funzione che poi verifica la copertura. Le due cose non possono divergere.
 *
 * Non sostituisce l'Editor — lo guida. Al termine stampa `FindMissingRequiredIcons` e
 * `ValidateIconCatalog`: se non scendono a zero, il catalogo non e' pronto e il commandlet lo dice
 * invece di salvare un asset che sembra a posto.
 *
 * Uso:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildIconCatalog [-DryRun]
 *         [-SourceDir=<percorso>] [-Size=48] [-Package=/Game/RT/UI/Icons]
 *         [-Catalog=/Game/RT/UI/DA_IconCatalog]
 *
 * `-DryRun` non scrive nulla: dice quali PNG mancano e quali chiavi resterebbero scoperte. E' il primo
 * comando da lanciare, sempre.
 */
UCLASS()
class URTBuildIconCatalogCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTBuildIconCatalogCommandlet();

	virtual int32 Main(const FString& Params) override;
};
