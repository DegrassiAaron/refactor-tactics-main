#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/RTPlayerEvent.h"
#include "RTPlayerEventProjector.generated.h"

struct FRTTurnLogEntry;

/**
 * Dal `TurnLog` canonico al feed di chi gioca — `#1936` §C, epic `#1937`.
 *
 * ```text
 * Resolver autorevole -> TurnLog canonico -> predicato di autorizzazione -> QUI -> FRTPlayerEvent[]
 * ```
 *
 * 🔴 **Pura e non autoritativa.** Legge il `TurnLog`, non lo tocca, e non entra nel suo hash: la
 * derivazione degli eventi giocatore e' a valle di tutto cio' che decide la partita. Se questa classe
 * sparisse, nessun esito cambierebbe.
 *
 * ⛔ **Tre divieti, e nascono dal difetto che `#295` ha gia' chiuso una volta:**
 *
 * · non si deriva un evento facendo il **parsing** di una stringa diagnostica — il testo e' una vista, non
 *   una fonte;
 * · non si **ricalcola** la conoscenza: si consuma il verdetto che ogni voce porta gia', congelato quando
 *   la voce e' nata ([D-223]);
 * · non si proietta prima e si filtra dopo. L'autorizzazione e' il **primo** passo, e un fatto non
 *   autorizzato non diventa mai un evento — nemmeno un evento vuoto, un conteggio o un tipo.
 *
 * 🔑 **Il predicato non e' nuovo, ed e' la misura che ha ridotto lo scope di `#1936`.** Il corpo della issue
 * chiedeva di estrarne uno da `ComposeVisibleLogLines`, *«dove filtro e composizione stanno nella stessa
 * funzione»*. Misurato il 2026-09-02: non e' cosi'. Quella funzione riceve righe **gia' composte** e applica
 * `FRTKnowledgeVerdict::AllowsTeam`, che vive in `Perception/RTTeamKnowledge.h` ed e' gia' isolato e
 * fail-closed. Qui si chiama **lo stesso**, sul `Verdict` che ogni `FRTTurnLogEntry` porta — nessuna
 * estrazione, nessun secondo contratto di conoscenza.
 */
UCLASS()
class REFACTORTACTICS_API URTPlayerEventProjector : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Proietta le voci autorizzate per `ObserverTeamId` in eventi leggibili, in ordine di produzione.
	 *
	 * L'ordine e' quello del `TurnLog` ricevuto e non viene riordinato: e' l'ordine di risoluzione, ed e'
	 * cio' che rende il feed una cronaca invece di un elenco.
	 *
	 * @param Entries          il `TurnLog` canonico, completo. Non viene modificato.
	 * @param ObserverTeamId   chi guarda. Fuori intervallo -> nessun evento (fail-closed di `AllowsTeam`).
	 */
	static TArray<FRTPlayerEvent> Project(const TArray<FRTTurnLogEntry>& Entries, int32 ObserverTeamId);

	/**
	 * Il solo passo di autorizzazione, esposto perche' e' **verificabile**.
	 *
	 * Esiste per `UI.PlayerEventLog.AuthorizationMatchesLogLines`: senza un punto nominabile, «il proiettore
	 * e il canale testuale ammettono lo stesso insieme di fatti» sarebbe un'affermazione invece che
	 * un'assertion. Chiama `AllowsTeam` e nient'altro.
	 */
	static bool IsAuthorized(const FRTTurnLogEntry& Entry, int32 ObserverTeamId);
};
