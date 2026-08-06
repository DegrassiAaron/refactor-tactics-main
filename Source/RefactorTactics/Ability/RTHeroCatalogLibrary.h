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

	/**
	 * Costruisce **Flux**, tecnico della conduzione (catalogo eroi v0.1 §1): 90 HP, 5 MP, vista 6, resistenza
	 * push 0, affinita' elettricita', debolezza acqua (combo dichiarata con Riva, CP 6.3). Nuova istanza a
	 * ogni chiamata — stesso idioma di `URTCatalogLibrary::GetCoreActionCatalog`: il catalogo eroi non e' un
	 * singleton, e un chiamante che ne vuole due (es. specchio nello stesso 2v2) non condivide lo stato.
	 *
	 * Limiti dichiarati (nessun sistema a valle esiste ancora, quindi l'effetto non e' rappresentabile):
	 * `ConductiveNode` non ha un modello di "cella conduttiva" (E8/E9); `Overload` non ha un modello di
	 * "dispositivo interrompibile" (E7, gadget); `ReactiveCapacitor` non ha uno slot di reazione ne' un modo
	 * di riferire "l'attaccante" a runtime (E5). Ogni azione esiste comunque come DATO, con la sua identita',
	 * portata e cooldown dal catalogo — solo l'effetto aggiuntivo resta un numero non ancora consumato.
	 */
	static URTHeroData* MakeFlux();

	/**
	 * Costruisce **Riva**, manipolatrice dell'acqua (catalogo eroi v0.1 §2): 95 HP, 5 MP, vista 5, resistenza
	 * push 0, affinita' acqua, debolezza elettricita' — simmetrica a Flux (stesso `Affinity.Electricity`),
	 * cosi' la rivalita' fra i due e' un solo identificatore condiviso, non due nomi.
	 *
	 * Limiti dichiarati: `CircularTide` dichiara Heal E Status.Wet nella stessa lista Effects, ma **nessun
	 * resolver applica oggi effetti diversi ad alleati e nemici dentro la stessa area** (`bFriendlyFire`
	 * decide solo SE un alleato viene colpito, non CON QUALE effetto) — la differenziazione arriva quando
	 * Riva sara' davvero cablata (CP 6.6+). `FluidTrail` (acqua lungo il percorso) e `MistVeil` (fumo) non
	 * hanno un modello di terreno dinamico (E8/E9): identita', fase, portata e cooldown sono dati veri,
	 * l'effetto no. `FlowReaction` e' interamente non rappresentabile (nessuno slot Reazione, E5): Effects
	 * vuoto, come `Flux.ReactiveCapacitor` per la sua meta' non rappresentabile.
	 */
	static URTHeroData* MakeRiva();
};
