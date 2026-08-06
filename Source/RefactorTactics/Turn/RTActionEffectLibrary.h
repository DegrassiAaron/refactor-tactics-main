#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTActionEvent.h"
#include "Turn/RTActionQueue.h"
#include "RTActionEffectLibrary.generated.h"

/**
 * Traduzione AZIONE -> EVENTI: il "registry" degli effetti del motore azioni.
 *
 * Un'azione non muta lo stato: dichiara un effetto, e questa libreria lo traduce negli eventi elementari che
 * il chiamante applica in blocco sullo snapshot della fase (invariante #3). Aggiungere un'azione nuova
 * significa dichiarare il suo effetto nel catalogo — non aggiungere un ramo in `ARTTurnManager`, che lavora
 * sugli eventi e non sa di che azione siano figli.
 *
 * Pura e totale: ogni valore di `ERTActionEffect` e' gestito esplicitamente, cosi' aggiungerne uno senza
 * tradurlo diventa un errore di compilazione e non un silenzio a runtime.
 *
 * Riferimento: docs/design/spec-motore-azioni-e4.md §3.
 */
UCLASS()
class REFACTORTACTICS_API URTActionEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Eventi prodotti da un'azione pianificata. Vuoto quando l'azione non ha effetto (`None`), quando
	 * l'entita' non e' positiva o quando manca il bersaglio: meglio nessun evento che un evento che "cura
	 * zero" e sporca log e TurnLog.
	 */
	static TArray<FRTActionEvent> ProduceEvents(const FRTActionInstance& Instance);

	/** Eventi di piu' azioni, nell'ordine in cui le azioni risolvono (il chiamante le ordina prima). */
	static TArray<FRTActionEvent> ProduceEventsForAll(const TArray<FRTActionInstance>& Instances);
};
