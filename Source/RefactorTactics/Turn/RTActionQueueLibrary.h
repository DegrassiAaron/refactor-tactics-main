#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTTurnRules.h" // ERTMatchPhase: le chiavi d'ordine partono dalla macro-fase
#include "RTActionQueueLibrary.generated.h"

/**
 * Ordine di risoluzione delle azioni di un turno: pura, deterministica, senza Actor.
 *
 * Una sola sede per la regola d'ordine, come `URTTurnLogLibrary` lo e' per il TurnLog. Se la scelta di
 * "chi risolve prima" fosse sparsa fra le fasi, due punti del codice potrebbero divergere e l'esito di un
 * turno dipenderebbe da quale ha ragione.
 */
UCLASS()
class REFACTORTACTICS_API URTActionQueueLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Ordine TOTALE fra due azioni pianificate. Vero se A risolve prima di B.
	 *
	 * Chiavi, in ordine: **macro-fase di Atlas** (`Prep -> Dash -> Blast -> Move -> Cleanup`) -> `Priority`
	 * intera crescente -> `ActionId` -> `SourceUnitId` -> `EventSequence`.
	 *
	 * La macro-fase viene PRIMA della priorita': un `Action.Move` a priorita' 50 risolve dopo un attacco a
	 * priorita' 80, perche' il Move e' una fase successiva (ADR-0003 §1 — il catalogo v0.1 metteva invece il
	 * movimento prima dell'attacco). La priorita' ordina DENTRO la fase, non fra le fasi.
	 *
	 * E' l'UNICO ordine di risoluzione del gioco. Questo commento diceva di «estendere la regola APNAP di
	 * `piano-canonico-mvp.md §5.1`»: quella regola e' stata **ritirata** da D-293 (2026-08-31) perche'
	 * presupponeva un'unita' attiva che un gioco a turni simultanei non ha. Non esiste un secondo ordine da
	 * estendere, e non va costruito.
	 *
	 * Nessuna chiave e' un float; nessuna dipende dall'ordine di una `TMap`.
	 */
	static bool InstanceLess(const FRTActionInstance& A, const FRTActionInstance& B);

	/** Ordina in place con `InstanceLess`: permutare l'ingresso non cambia la sequenza risolta. */
	static void SortActionInstances(TArray<FRTActionInstance>& Instances);

	/**
	 * Le sole istanze che risolvono nella macro-fase indicata, nell'ordine di risoluzione.
	 * Il chiamante lavora una fase per volta senza rifiltrare a mano (e senza sbagliare la rimappatura).
	 */
	static TArray<FRTActionInstance> InstancesForPhase(const TArray<FRTActionInstance>& Instances,
		ERTMatchPhase Phase);
};
