#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTHeroCatalogLibrary.generated.h"

class URTHeroData;

/**
 * Lettura e validazione del catalogo eroi: pura, deterministica, senza Actor.
 *
 * Stessa disciplina di `URTCatalogLibrary` per le azioni: il validator e' una funzione pura per costruzione,
 * cosi' un roster incompleto (debolezza mancante, struttura sbagliata) si scopre in CI, non in partita.
 */
UCLASS()
class REFACTORTACTICS_API URTHeroCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Errori strutturali di un roster di eroi (vuoto = roster valido). Rifiuta:
	 * HeroId assente o duplicato · statistiche non positive (salute, movimento) o negative (vista, resistenza
	 * push) · affinita' o debolezza non dichiarate · numero di azioni diverso da **5** (attacco base + quattro
	 * fondamentali, catalogo v0.1 §"Struttura di un eroe") · zero o piu' di UNA azione fondamentale con
	 * varianti dichiarate (vincolo v0.1: una sola abilita' configurabile per eroe).
	 *
	 * Ogni messaggio nomina l'eroe colpevole: un errore che non dice QUALE eroe e' rotto costringe a
	 * ricontrollare tutto il roster a mano.
	 *
	 * Non e' `UFUNCTION`: UHT non riflette `TArray<const T*>` (stesso limite dichiarato da
	 * `URTCatalogLibrary::ValidateEquipment`), quindi resta una funzione C++ pura, non chiamabile da Blueprint.
	 */
	static TArray<FString> ValidateHeroes(const TArray<const URTHeroData*>& Heroes);
};
