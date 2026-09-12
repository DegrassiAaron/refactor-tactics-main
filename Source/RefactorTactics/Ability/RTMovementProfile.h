#pragma once

#include "CoreMinimal.h"
#include "RTMovementProfile.generated.h"

/**
 * Il PROFILO DI MOVIMENTO come entita' ([D-116] · [D-117] · `#653`).
 *
 * 🔴 **Prima di `#653` non esisteva come tipo**: `Move`, `Sprint`, `Withdraw` e `Sneak` vivevano solo nel
 * catalogo markdown §2.1 coi loro budget, e nel codice `Action.Sprint` era un `ActionId` con
 * `MovementStyle::Budget` — non un profilo con proprieta'. Mancando il tipo, [D-117] non aveva dove
 * scrivere i due budget e la soglia di [#606] non aveva dove scrivere il primo dei suoi due numeri.
 *
 * ⚠️ **Non e' un'azione, e i due non vanno fusi.** L'azione dichiara *cosa* l'unita' ha scelto di fare;
 * il profilo dichiara *con quanta misura* il movimento che ne segue viene speso. Un'azione di movimento
 * NOMINA il suo profilo (`FRTActionDef::MovementProfileId`), e il profilo dell'unita' si RICAVA dal piano
 * — non e' un secondo campo da tenere d'accordo con l'azione. Vedi `ProfileForPlan`.
 */
USTRUCT(BlueprintType)
struct FRTMovementProfile
{
	GENERATED_BODY()

	/** Identita' stabile del profilo (`MovementProfile.Move`, `...Sprint`, ...). `NAME_None` = non trovato. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Movement")
	FName Id;

	/**
	 * **Passi**: quanto lontano si arriva — ogni cella attraversata vale `1` ([D-117] voce 1).
	 *
	 * `InheritFromUnit` significa «lo dichiara l'unita'», ed e' il valore del profilo `Move`: e' cio' che
	 * rende vera la clausola di `#653` per cui **nessun comportamento attuale cambia** finche' nessuno
	 * sceglie un altro profilo. Un numero cablato qui sovrascriverebbe il `MoveRange` dell'eroe.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Movement")
	int32 StepBudget = InheritFromUnit;

	/**
	 * **Asperita'**: quanta ne assorbe il movimento ([D-117] voce 2).
	 *
	 * ⚠️ **Oggi porta lo stesso valore di `StepBudget`, e non e' una svista.** La funzione di costo che
	 * separa le due misure — `max(0, MoveCost - 1 + MoveCostModifier)` — e' [#666], che dipende da questo
	 * checkpoint. Finche' quella non esiste, ogni cella costa `1` e le due misure coincidono: separare qui i
	 * CAMPI senza separare ancora i VALORI e' esattamente il prerequisito che `#653` doveva consegnare.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Movement")
	int32 MoveBudget = InheritFromUnit;

	/**
	 * Quanta stabilita' il profilo CONCEDE, contro il `MinStability` che l'azione RICHIEDE
	 * ([D-116] voce 2, `spec-compatibilita-azioni-movimento.md` §3.2).
	 *
	 * 🔑 **E' una soglia e non una sottrazione**, nella forma che il repository usa gia' per
	 * `PushResistance` ([D-038]): `legale ⇔ Profilo.Stability >= Azione.MinStability`. Quattro numeri qui
	 * piu' uno per azione, invece delle 124 celle di una matrice azione × profilo.
	 *
	 * ⛔ **Il confronto non si fa qui: e' [#606].** Questo campo esiste perche' senza di lui quella issue
	 * non ha dove atterrare — che e' la ragione per cui `#653` la precede.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Movement")
	int32 Stability = 0;

	/**
	 * Se il profilo si possa SCEGLIERE in Planning.
	 *
	 * ⛔ **`Sneak` e' `false`, ed e' il punto di `AE-5`**: il catalogo markdown gli assegna «—» invece di un
	 * budget, cioe' non ha numeri. Il tipo lo PREVEDE — sta nel catalogo, con la sua `Stability` — ma il
	 * dato non c'e', e un profilo senza budget non e' pianificabile senza inventare il numero che manca.
	 * Ometterlo dal catalogo avrebbe nascosto la lacuna invece di dichiararla.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Movement")
	bool bPlannable = true;

	/** «Il budget lo dichiara l'unita'»: si veda `StepBudget`. Non e' un budget di valore negativo. */
	static constexpr int32 InheritFromUnit = INDEX_NONE;

	/** Il profilo esiste (`FindProfile` ha trovato qualcosa). */
	bool IsValid() const { return !Id.IsNone(); }

	/** Il budget dei passi per QUESTA unita': quello del profilo, o quello dell'unita' se ereditato. */
	int32 ResolveStepBudget(int32 UnitMoveRange) const
	{
		return StepBudget == InheritFromUnit ? UnitMoveRange : StepBudget;
	}

	/** Il budget di asperita' per QUESTA unita': quello del profilo, o quello dell'unita' se ereditato. */
	int32 ResolveMoveBudget(int32 UnitMoveRange) const
	{
		return MoveBudget == InheritFromUnit ? UnitMoveRange : MoveBudget;
	}
};
