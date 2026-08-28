#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTScenarioArena.generated.h"

struct FRTTestScenario;
class URTHexMapAsset;

/**
 * L'arena che uno scenario descrive, costruita dal dato.
 *
 * Uno scenario non porta una mappa: porta una **fixture** oppure un **raggio**, piu' gli override di cella che
 * modificano ciò che ne esce. Tradurre quelle tre cose in un `URTHexMapAsset` era un dettaglio privato della
 * sessione, e li' e' rimasto finche' l'unico consumatore era il runner.
 *
 * ⚠️ **Ha smesso di esserlo con `#1116`**: per mostrare le celle raggiungibili di un `Move` durante
 * l'authoring serve la stessa arena su cui girera' la partita — e ricostruirla nell'editor con una regola
 * propria produrrebbe una preview che mostra celle che il runner poi non concede. E' il §3 di
 * `spec-tactical-designer.md` applicato alla mappa: l'editor visualizza una risposta che il gioco da'.
 *
 * ⚠️ **Nessun `UWorld`, nessun `AActor`.** La sessione ne ha bisogno — deve anche piazzare un
 * `ARTHexMapActor` perche' in PIE si veda qualcosa — ma quella meta' e' presentazione e resta sua. Qui c'e'
 * solo il dato, e per questo la funzione gira headless, nei test e nell'editor senza partita.
 */
UCLASS()
class REFACTORTACTICS_API URTScenarioArenaLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * L'arena dello scenario: fixture o raggio piatto, con gli override di cella applicati **sopra**.
	 *
	 * @param Outer proprietario dell'asset creato. Un mondo in partita, il transient package in un test:
	 *        l'asset non deve sopravvivere a chi lo espone.
	 * @return `nullptr` se la fixture ha un nome che il progetto non conosce, o se il raggio non e' valido —
	 *         mai un'arena vuota, che farebbe girare la partita producendo un fallimento che parla di unita'
	 *         fuori mappa invece che della fixture inesistente.
	 */
	static URTHexMapAsset* BuildArena(const FRTTestScenario& Scenario, UObject* Outer);
};
