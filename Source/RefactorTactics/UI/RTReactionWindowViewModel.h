#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
// FRTReactionWindowView: il DTO sanitizzato viaggia per VALORE dentro questo view model, e il container
// ne pretende il tipo definito. E' l'unico include di dominio che serve: il `TurnManager` sta in weak ptr.
#include "Turn/RTReactionWindowView.h"
#include "RTReactionWindowViewModel.generated.h"

class ARTTurnManager;

/**
 * CHI RENDE RAGGIUNGIBILE LA FINESTRA DI REAZIONE, e per conto di chi (`#2723`, CP 14.6, [D-355]).
 *
 * ## Il fatto che questo oggetto chiude
 *
 * 🔑 **`OnReactionWindowOpened` non lo legava nessuno, e una riga bastava a spegnere tre issue.** `#2679`
 * ha costruito la sospensione nel movimento, `#2692` quella del `Brace`, `#2717` l'orologio che le fa
 * scadere — ma il ramo che apre la finestra e' protetto da quattro condizioni, e la prima e'
 * `OnReactionWindowOpened.IsBound()`. Fino a questo file le sole occorrenze erano nei test che lo legavano
 * apposta: in partita quel ramo **non si prendeva mai**, l'esito restava `NoDecider`, la risposta sicura
 * veniva applicata d'ufficio, e il giocatore non sapeva che una scelta esisteva.
 *
 * 🔑 **`IsBound()` non e' un dettaglio implementativo: e' il segnale che distingue «umano con UI» da
 * «umano senza UI»**, ed e' cio' che tiene in piedi bot, test e Verifier senza un ramo che li nomini.
 * Legarlo e' precisamente l'atto che accende il comportamento — e per la stessa ragione **non** va legato
 * dove una UI non c'e' (vedi `ARTGameMode::HookReactionWindow`).
 *
 * ## Push per ESISTERE, pull per DISEGNARE
 *
 * ⚠️ **Il pattern dei view model del progetto e' `pull`** — `URTHudViewModel` e' una
 * `UBlueprintFunctionLibrary` che il widget interroga. La finestra e' l'eccezione, e per una ragione
 * meccanica: il delegate va **legato**, o il ramo non si prende. Le due cose convivono, e la divisione e'
 * netta:
 *
 * · si **lega** per esistere — `Hook` e' cio' che rende il ramo interattivo raggiungibile;
 * · si **interroga** per disegnare — countdown, apertura e identita' si leggono dal manager a ogni frame.
 *
 * 🔴 **E vale anche per «la finestra e' ancora aperta?», non solo per «quanto tempo resta».** Non esiste un
 * `OnReactionWindowClosed`: il manager notifica l'apertura e nient'altro, mentre la finestra si chiude
 * **anche per scadenza** — `TickReactionWindow` -> `ExpireReactionWindow`, un percorso che non passa di
 * qui. Un `bOpen` proprio resterebbe vero per sempre dopo il primo timeout, e il widget disegnerebbe una
 * finestra che il core ha gia' chiuso. ∴ la vista in cache vale **solo finche'**
 * `GetOpenReactionWindowId()` nomina ancora la finestra per cui e' stata costruita — ed e' `IsWindowOpen`
 * a verificarlo, non un flag.
 *
 * ## ⛔ Cosa questo oggetto NON fa
 *
 * · **non decide**: la scadenza e' dell'orologio del manager (`#2717`), la legalita' della risposta e' di
 *   `AskReactionDecision`, la durata e' server-authoritative (`FastReactionDuration`, ADR-0004 §8). Qui si
 *   **inoltra**, e una risposta illegale diventa `Rejected` nello stesso posto in cui lo diventa quella di
 *   un bot;
 * · **non conta il tempo**: un countdown contato dal client e' un client che decide quando scade;
 * · **non costruisce una vista**: `MakeReactionWindowView` sanitizza per squadra a monte, e cio' che arriva
 *   qui e' gia' cio' che quel giocatore puo' vedere.
 *
 * ## ⚠️ La privacy per squadra NON e' verificata qui, e la riga ha una data di scadenza
 *
 * Il view model non controlla che la squadra del proprietario sia la propria, e non e' una dimenticanza:
 *
 * · il gate a monte e' `!bOwnerIsBot`, e in v0.1 — 2v2 offline, **un umano** con due unita' ([D-155]) —
 *   quel gate **e'** il gate di squadra;
 * · `FRTReactionWindowOpenedSignature` e' un `DECLARE_DELEGATE_TwoParams`, cioe' **single-cast**: due
 *   giocatori umani locali non potrebbero nemmeno legare due view model. La forma del delegate esclude gia'
 *   il caso, e un gate qui difenderebbe da uno stato irrappresentabile.
 *
 * ⛔ **Il giorno del secondo umano per squadra questa e' la prima riga da rileggere** — insieme al payload
 * del delegate, che oggi non porta l'`OwnerTeamId` che servirebbe a decidere. Owner: `#166` / multiplayer.
 */
UCLASS()
class REFACTORTACTICS_API URTReactionWindowViewModel : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Lega `OnReactionWindowOpened` al `TurnManager`: da questo momento il ramo interattivo e' raggiungibile.
	 *
	 * 🔑 **`BindUObject`, e non e' una preferenza di stile.** `FDelegateBase::IsBound()` e'
	 * `DelegateInstance && DelegateInstance->IsSafeToExecute()`, e per un binding `UObject`
	 * `IsSafeToExecute()` interroga il weak pointer: un view model distrutto rende `IsBound()` **falso da
	 * solo**, e il gioco ricade sul modello sincrono senza che nessuno disfi il binding al teardown del
	 * mondo. Un `BindLambda` che catturasse `this` avrebbe lo stesso codice e nessuna di queste garanzie.
	 *
	 * ⚠️ **Chiamarla due volte e' sicuro**: il delegate e' single-cast, quindi il secondo binding sostituisce
	 * il primo invece di aggiungersi. E' cio' che la rende usabile da chi non sa se il ciclo di vita sia
	 * gia' passato di qui.
	 *
	 * `nullptr` non fa nulla e non lega niente: senza manager non c'e' una finestra da attendere.
	 */
	void Hook(ARTTurnManager* InTurnManager);

	/**
	 * C'e' una finestra da disegnare, ADESSO?
	 *
	 * 🔴 **La domanda la risponde il manager, non un flag di questo oggetto.** La vista consegnata vale
	 * finche' `GetOpenReactionWindowId()` nomina ancora quella finestra: dopo una scadenza l'id cambia o si
	 * svuota, e questa funzione dice falso **senza aver ricevuto alcuna notifica**. E' l'unico disegno che
	 * non lascia una seconda verita' sull'esistenza della finestra.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	bool IsWindowOpen() const;

	/**
	 * La finestra aperta, gia' sanitizzata per chi la riceve. Vista ai default quando nessuna attende —
	 * `bOpen` falso e nient'altro, che e' la stessa forma che `FilterWindowForTeam` da' a un avversario.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	FRTReactionWindowView GetWindow() const;

	/**
	 * Secondi che restano alla finestra. **Negativo** quando nessuna attende — la convenzione di
	 * `FRTMatchHeaderView::PlanningSecondsRemaining`, dove `0.f` direbbe «scaduta adesso», che e' un'altra
	 * cosa.
	 *
	 * ⛔ **Non lo calcola questo oggetto**: lo chiede a `GetOpenReactionWindowRemainingSeconds()`, che legge
	 * l'orologio autorevole. Il widget lo mostra e non lo decrementa.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Reaction")
	float GetRemainingSeconds() const;

	/**
	 * Inoltra la risposta del giocatore alla finestra aperta, che si chiude e riprende la resolution.
	 *
	 * ⚠️ **Nomina la finestra per cui la vista e' stata costruita, non quella aperta adesso.** E' cio' che
	 * fa valere il gate dell'identita' del manager anche su questo canale: se nel frattempo la finestra e'
	 * scaduta e un'altra si e' aperta, la risposta e' arrivata tardi e non deve chiudere quella sbagliata.
	 * Passare `GetOpenReactionWindowId()` — cioe' «qualunque finestra ci sia» — disarmerebbe il gate proprio
	 * dal lato da cui arriva l'input umano.
	 *
	 * La legalita' della risposta non si giudica qui: passa da `AskReactionDecision` come ogni altra.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Reaction")
	void SubmitResponse(const FString& Response);

private:
	/** L'handler legato a `OnReactionWindowOpened`. Registra la finestra; non risponde e non decide. */
	void HandleWindowOpened(const FRTReactionWindowView& View, int32 OwnerUnitId);

	/** Chi ha la finestra. Weak: il manager e' un Actor del mondo, questo oggetto vive col controller. */
	TWeakObjectPtr<ARTTurnManager> TurnManager;

	/** L'ultima finestra consegnata dal delegate, gia' sanitizzata. */
	FRTReactionWindowView Window;

	/**
	 * L'identita' della finestra per cui `Window` e' stata costruita.
	 *
	 * 🔑 **Non e' una copia di comodo: e' cio' che rende falsificabile la validita' della vista.** Senza,
	 * `IsWindowOpen` potrebbe solo chiedere «c'e' una finestra?» e risponderebbe vero anche per una
	 * finestra **diversa**, aperta dopo che la prima e' scaduta — e il widget mostrerebbe le opzioni di una
	 * domanda a cui il gioco non sta piu' chiedendo di rispondere.
	 *
	 * Si legge da `GetOpenReactionWindowId()` invece di derivarla da `Window.Key`: `DeriveOpportunityId` e'
	 * un FORMATO con un solo produttore, e ricomporlo qui ne creerebbe un secondo.
	 */
	FString WindowOpportunityId;
};
