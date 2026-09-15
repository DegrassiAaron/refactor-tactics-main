#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTMovementProfile.h"
#include "Turn/RTPlanValidationLibrary.h"
#include "RTMovementProfileLibrary.generated.h"

/**
 * Il catalogo dei profili di movimento, e la proiezione che ricava dal PIANO quale profilo l'unita' abbia
 * scelto (`#653`, prerequisito di [#641] e [#606]).
 */
UCLASS()
class REFACTORTACTICS_API URTMovementProfileLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** `MovementProfile.Still` — nessun movimento pianificato. */
	static const FName ProfileStill;
	/** `MovementProfile.Move` — il profilo neutro, che eredita il budget dall'unita'. */
	static const FName ProfileMove;
	/** `MovementProfile.Sprint` — 8 punti, [D-116]. */
	static const FName ProfileSprint;
	/** `MovementProfile.Withdraw` — 2 punti, il ripiegamento che [D-070] vincola all'Overwatch. */
	static const FName ProfileWithdraw;
	/** `MovementProfile.Sneak` — dichiarato SENZA numeri (`AE-5`), quindi non pianificabile. */
	static const FName ProfileSneak;

	/**
	 * I profili spediti. L'ordine e' stabile perche' da qui puo' nascere una UI, e un elenco che cambia
	 * ordine e' un elenco che si legge diverso a ogni apertura.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static TArray<FRTMovementProfile> GetCoreMovementProfileCatalog();

	/** Il profilo con quell'`Id`, o un profilo invalido (`IsValid() == false`) se non esiste. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FRTMovementProfile FindProfile(FName ProfileId);

	/**
	 * Quale profilo il PIANO dichiara: quello nominato dall'azione di movimento pianificata, oppure
	 * `Still` se l'unita' non ne ha pianificata nessuna.
	 *
	 * 🔑 **Il profilo si RICAVA dal piano e non e' un campo accanto ad esso**, ed e' la scelta strutturale
	 * di questo checkpoint. Un secondo campo si potrebbe compilare `Move` mentre l'azione dice `Sprint`:
	 * sarebbero due autorita' sulla stessa domanda, e il resolver dovrebbe sceglierne una. Derivandolo,
	 * la coerenza non e' una regola da far rispettare — e' una proprieta' del tipo.
	 *
	 * ⚠️ **Un piano con piu' di un'azione di movimento restituisce la PRIMA in ordine di piano**, non un
	 * errore: che due azioni possano occupare insieme lo slot `Movement` e' una domanda di
	 * `ValidatePlan`, che la rifiuta gia' per conto suo ([D-028]). Qui non si duplica quel verdetto.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FRTMovementProfile ProfileForPlan(const TArray<FRTPlannedAction>& Plan);

	/**
	 * L'azione del catalogo core che NOMINA quel profilo, o un `Def` vuoto (`ActionId.IsNone()`) se
	 * nessuna lo fa (`#1410`).
	 *
	 * E' l'inversa di `FRTActionDef::MovementProfileId`, e serve al selettore: il giocatore sceglie un
	 * **profilo**, il piano porta un'**azione**. Senza questa funzione il selettore dovrebbe conoscere la
	 * corrispondenza per nome, che e' la seconda sede che `#653` ha evitato.
	 *
	 * ⚠️ **Il catalogo si scorre una volta sola.** `GetCoreActionCatalog()` costruisce l'array a ogni
	 * invocazione, e questa funzione la chiama il compositore del piano — cioe' la HUD a ogni click. La
	 * cache e' la stessa disciplina gia' applicata a `MakePlanFor` e a `GetReactionProfileCatalog`.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FRTActionDef FindActionForProfile(FName ProfileId);

	/**
	 * I profili che il giocatore puo' SCEGLIERE, nell'ordine del catalogo (`#1410`, `AC-1`).
	 *
	 * Sono quelli `bPlannable` **meno** i due che l'`AC-4` esclude, e le due esclusioni hanno ragioni
	 * diverse che non vanno confuse:
	 *
	 * - `Withdraw` e' **riservato**: lo impone l'`Overwatch` ([D-070]), e sceglierlo a mano sarebbe una
	 *   seconda verita' sullo stesso vincolo;
	 * - `Still` e' **derivato**: e' la lettura di «non ho pianificato movimento», e si ottiene cancellando
	 *   i waypoint. Offrirlo darebbe due gesti per lo stesso fatto.
	 *
	 * ⛔ **E un profilo senza un'azione che lo nomini non e' offribile**, per quanto `bPlannable` dica di
	 * si': il piano porta azioni, quindi un profilo irraggiungibile comparirebbe nel selettore e non
	 * arriverebbe mai al piano. E' il caso di `Sprint` finche' `Action.Sprint` non entra nel kit di
	 * qualcuno.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static TArray<FRTMovementProfile> OfferableProfiles();

	/**
	 * Il profilo a cui il PIANO riserva lo slot movimento, o `NAME_None` se nessuna voce lo fa
	 * (`#1410` `AC-5`, [D-070]).
	 *
	 * Legge `FRTActionDef::ReservesMovementProfileId`, quindi vale per qualunque azione lo dichiari e non
	 * per il solo `Action.Overwatch`: chi la consuma non ha bisogno di sapere **quale** azione ha imposto
	 * il vincolo, solo che c'e' e a cosa costringe.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Movement")
	static FName ReservedProfileForPlan(const TArray<FRTPlannedAction>& Plan);
};
