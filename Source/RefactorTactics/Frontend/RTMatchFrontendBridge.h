#pragma once

#include "CoreMinimal.h"

class UGameInstance;
class URTFrontendNavigator;
struct FRTMatchResult;
struct FRTMatchState;

/**
 * IL LATO DI PARTITA DEL CONFINE COL FRONTEND: cosa si apre, quando, e cosa si fa se non c'e' nessuno.
 *
 * ## Cosa sta qui e cosa resta nel GameMode
 *
 * I delegate del navigatore sono **dinamici**, quindi il receiver deve essere un `UObject` con `UFUNCTION`:
 * l'iscrizione (`AddUniqueDynamic`) e l'apertura del livello (`OpenLevelByName`, che e' anche il seam dei
 * test) restano quindi in `ARTGameMode`. Qui vive la **politica**: consumare la richiesta invece di
 * leggerla, dichiarare il caso «annuncio senza richiesta pendente», e decidere che fare quando il frontend
 * non esiste affatto.
 *
 * ∴ le callback del GameMode diventano adattatori sottili — chiedono a questo bridge *cosa* aprire e
 * aprono. Non e' una indirezione gratuita: la ragione per cui il GameMode cambia (Unreal, delegate, seam
 * dei test) e quella per cui cambia questa politica (il flusso del frontend) sono due, e prima erano una.
 *
 * ## Perche' funzioni statiche e non un `UObject`
 *
 * Non c'e' stato da tenere: il navigatore e' un subsystem della `GameInstance` e la richiesta pendente vive
 * dentro di lui. Un `UObject` qui esisterebbe solo per poter ricevere delegate che il GameMode riceve gia'.
 */
struct REFACTORTACTICS_API FRTMatchFrontendBridge
{
	/**
	 * Il navigatore di questa sessione, o `nullptr`.
	 *
	 * ⚠️ **`nullptr` non e' un errore**: uno scenario headless o un test di simulazione girano senza
	 * frontend, e la partita deve poter girare lo stesso. E' la differenza con `ARTFrontendGameMode`, dove
	 * un navigatore assente significa un menu che non aprirebbe niente.
	 */
	static URTFrontendNavigator* FindNavigator(UGameInstance* GameInstance);

	/**
	 * `PLAY AGAIN` dal Result: consuma la richiesta e dice **quale livello** riaprire. Vuoto = niente da aprire.
	 *
	 * Si CONSUMA, non si legge: una richiesta che resta li' fa rifiutare il `PLAY` successivo con «mai
	 * consumata», che punta il dito su chi non consuma invece che su chi non si e' iscritto.
	 */
	static FString ConsumeMatchLevel(UGameInstance* GameInstance, const FString& AnnouncedLevel);

	/**
	 * `RETURN TO MAIN MENU`: consuma la richiesta e dice quale livello aprire. Vuoto = niente da aprire.
	 *
	 * ⛔ **Aprire quel livello e' cio' che fa finire la partita davvero**: cambiare livello distrugge il
	 * mondo, e con lui `ARTTurnManager`, le `ARTUnit` e il GameMode. E' il motivo per cui il DoD puo'
	 * chiedere «nessuno stato vivo» invece di un elenco di cose da azzerare.
	 */
	static FString ConsumeFrontendLevel(UGameInstance* GameInstance, const FString& AnnouncedLevel);

	/**
	 * Porta il verdetto di fine partita alla schermata di Result.
	 *
	 * ⚠️ **Non ricalcola niente**: passa a `ShowResult` il `FRTMatchResult` ricevuto, che e' la stessa regola
	 * per cui il view model legge invece di ridare il verdetto. La simulazione annuncia il verdetto che ha
	 * gia' dato, e non deve sapere che esista una UI — ecco perche' questo non sta nel `TurnManager`.
	 */
	static void ShowResult(UGameInstance* GameInstance, const FRTMatchResult& Result, const FRTMatchState& State);
};
