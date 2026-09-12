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
};
