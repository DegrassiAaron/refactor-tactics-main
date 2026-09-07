#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
// `FRTHeroProfileView` serve COMPLETO: `BuildBaseProfile` lo restituisce per valore, e i `TArray` che
// contiene pretendono il tipo definito per distruttore e copia.
#include "UI/RTHeroProfileView.h"
#include "RTHeroProfileLibrary.generated.h"

class URTHeroData;

/**
 * Le due sole operazioni che il Profilo Tattico possiede: **copiare** cio' che il catalogo dichiara, e
 * **rifiutare** una view incoerente.
 *
 * 🔴 **Cio' che questa library NON fa e' la meta' del suo valore.**
 *
 *  * ⛔ non calcola i sei assi del Profile Radar — vivono in `tools/radar/profile.ts` (D-107, `#562`), e
 *    una `ProfileAxes()` in C++ sarebbe un **secondo derivatore**: due implementazioni della stessa
 *    rubrica divergono alla prima modifica di una sola, e il gate `generate.ts --check` non guarda il C++;
 *  * ⛔ non deriva la proficiency elementale da nome dell'abilita', VFX, `DamageType` o tema (`#995`
 *    possiede la grammatica, e dedurla qui inventerebbe un dato che nessuno ha misurato);
 *  * ⛔ non traduce l'`Affinity` in una proficiency, e non traduce la `Weakness` in un moltiplicatore;
 *  * ⛔ non conosce nessun `HeroId` specifico.
 *
 * ∴ quello che resta e' un travaso onesto piu' un guardiano.
 */
UCLASS()
class REFACTORTACTICS_API URTHeroProfileLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Costruisce la parte del profilo che il catalogo possiede **davvero**: identita', nome mostrato,
	 * affinita' e debolezza. Tutto il resto della view resta vuoto.
	 *
	 * ⚠️ **Il vuoto e' il risultato corretto, non un lavoro a meta'.** `URTHeroData` non dichiara ruoli,
	 * assi radar, relazioni elementali ne' proficiency: riempirli qui vorrebbe dire inventarli. Chi li
	 * possiede li aggiunge alla view a valle, e finche' non esiste un provider la scheda li nasconde.
	 *
	 * 🔵 **Le etichette di affinita' e debolezza restano vuote di proposito.** Il catalogo dichiara gli ID
	 * (`FName`), non il testo player-facing; produrre qui una label facendo `ToString()` dell'ID
	 * spaccerebbe un identificatore tecnico per una traduzione.
	 *
	 * @param Hero il dato d'eroe. `nullptr` restituisce una view vuota, che e' uno stato valido.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HeroProfile")
	static FRTHeroProfileView BuildBaseProfile(const URTHeroData* Hero);

	/**
	 * Verifica che la view sia rappresentabile, e dice **perche'** quando non lo e'.
	 *
	 * ⚠️ **Fail-closed.** Un asse con `MaxValue <= 0` o un `Value` fuori scala non va disegnato «come
	 * meglio si puo'»: un asse mancante reso a zero significa, visivamente, una debolezza misurata — cioe'
	 * un'affermazione sul personaggio che nessuno ha fatto. Meglio rifiutare e nascondere la sezione.
	 *
	 * @param View       la vista da controllare.
	 * @param OutDiagnostics una riga per problema, vuoto quando la view e' valida.
	 * @return `true` se la view e' coerente.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HeroProfile")
	static bool ValidateProfileView(const FRTHeroProfileView& View, TArray<FString>& OutDiagnostics);
};
