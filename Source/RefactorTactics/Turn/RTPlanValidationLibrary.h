#pragma once

#include "CoreMinimal.h"
#include "Ability/RTActionDef.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTHexSim.h"
#include "RTPlanValidationLibrary.generated.h"

/**
 * Una voce del piano di un'unita': l'azione dichiarata, piu' lo stato che la rende ripetibile o no.
 *
 * Il cooldown residuo e' un PARAMETRO e non un campo dello snapshot perche' oggi vive altrove:
 * `ARTUnit::AbilityCooldowns` e' privato e parallelo ad `Abilities`, sull'Actor. Passarlo qui e' lo stesso
 * pattern gia' in uso in `URTCombatLibrary::IsAbilityUsable(CooldownRemaining, Energy, EnergyCost)`, e
 * mantiene la validazione una funzione pura: stessa coppia (snapshot, piano) => stesso verdetto.
 */
USTRUCT(BlueprintType)
struct FRTPlannedAction
{
	GENERATED_BODY()

	/**
	 * L'azione dichiarata. Di lei la validazione legge `ActionId`, `Slot` e `CostMP`.
	 *
	 * ⚠️ `CostMP` oggi non ha produttore: il catalogo v0.1 dichiara il budget di movimento in `RangeCells`
	 * con `ERTMovementStyle::Budget`, non qui. Vedi la nota estesa in `ValidatePlan`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	FRTActionDef Def;

	/** Turni di cooldown ancora da scontare; 0 = ripetibile ora. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	int32 CooldownRemaining = 0;
};

/** Verdetto della validazione: legale, oppure illegale con il motivo e l'azione che lo ha causato. */
USTRUCT(BlueprintType)
struct FRTPlanValidation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	bool bLegal = true;

	/** `None` quando il piano e' legale. Estende la famiglia gia' serializzata del fallback. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	ERTActionInvalidReason Reason = ERTActionInvalidReason::None;

	/** Quale voce del piano ha causato il rifiuto; `NAME_None` se il piano e' legale. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Actions")
	FName OffendingActionId;
};

/**
 * Il punto unico che risponde LEGALE / ILLEGALE + motivo PRIMA del commit del piano (CP 38.2, epic E38).
 *
 * Cio' che esisteva prima — `URTActionFallbackLibrary::ValidateInstance` — agisce IN RISOLUZIONE: un piano
 * illegale si scopriva quando non funzionava, cadendo su un fallback. Qui si scopre quando lo si compone.
 *
 * Regola in vigore: D-028, confermata da D-114 il 2026-08-12. L'economia e' a **slot** e la legalita' e'
 * STRUTTURALE — due principali non si sommano perche' occupano la stessa cosa, non perche' un totale sfori.
 */
UCLASS()
class REFACTORTACTICS_API URTPlanValidationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Valida il piano di UNA unita' contro il suo stato nello snapshot.
	 *
	 * Precedenza dei motivi, deterministica e indipendente dall'ordine in cui il giocatore compone:
	 *   1. `OnCooldown` — proprieta' INTRINSECA di una voce sola;
	 *   2. `InsufficientMovementPoints` — somma di piano, gia' indipendente dall'ordine;
	 *   3. `SlotOccupied` — proprieta' della COMBINAZIONE, quindi per ultima.
	 *
	 * L'ordine dell'array non entra nel verdetto: le voci si esaminano in un ordine canonico (prima chi
	 * occupa piu' slot, poi per `ActionId`), altrimenti lo stesso piano composto in due sequenze diverse
	 * darebbe due motivi diversi — cioe' la presentazione deciderebbe la regola.
	 *
	 * PRECONDIZIONE: `Unit` e' viva. La liveness non si controlla qui e non ha un motivo proprio, perche'
	 * un'unita' morta non compone piani: chi chiama filtra gia' i morti, come fa ogni consumatore di
	 * `FRTHexSimUnit::bAlive`. Aggiungere un motivo per un caso che nessun chiamante puo' produrre
	 * significherebbe un valore in piu' in un enum serializzato, mai scritto in nessuna traccia.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Actions")
	static FRTPlanValidation ValidatePlan(const FRTHexSimUnit& Unit, const TArray<FRTPlannedAction>& Plan);

	/** Quanti slot occupa un'azione: 0 per `None`, 2 per `MovementAndMain`, 1 per le altre. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Actions")
	static int32 SlotWidth(ERTActionSlot Slot);
};
