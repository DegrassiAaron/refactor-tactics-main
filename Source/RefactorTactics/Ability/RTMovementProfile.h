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

	/**
	 * Se questo profilo sia una CORSA: [D-319] dice che «chi ha perso l'equilibrio non corre», e questo
	 * campo e' il soggetto di quella frase ([D-406]).
	 *
	 * 🔑 **Perche' un campo e non lo stile.** Il criterio era `ERTMovementStyle::Budget` e non l'`ActionId`,
	 * scelta esplicita di [D-319] «perche' resti vero per la prossima azione a budget». Ma `Action.Move` e
	 * `Action.Sprint` dichiarano **lo stesso** stile — entrambi `Budget`, e lo asserisce
	 * `Actions.SprintIsAMoveProfileResolvedPreBlast` — quindi lo stile non distingue il correre dal
	 * camminare: oggi discrimina solo perche' il criterio e' racchiuso nel ciclo del Dash, dove lo `Sprint`
	 * e' l'unica mobilita' a budget che puo' stare. Con [#641] quel recinto sparisce, e il criterio portato
	 * com'e' rifiuterebbe anche il Move normale — cioe' renderebbe `Unbalanced` un'immobilizzazione totale,
	 * che [D-319] non dice.
	 *
	 * ⛔ **E non e' un `if` sul nome del profilo**, che sarebbe lo stesso difetto con un altro campo: la
	 * proprieta' che [D-319] voleva — una mobilita' nuova si copre **dichiarandola**, non modificando una
	 * condizione — e' conservata perche' il criterio resta un DATO.
	 *
	 * ⚠️ **`Withdraw` e' `false`, e non e' una dimenticanza**: [D-070] lo IMPONE a chi arma l'`Overwatch`,
	 * quindi un criterio che lo rifiutasse lascerebbe uno sbilanciato con lo slot movimento riservato a un
	 * profilo che non gli e' permesso. E' la ragione per cui il criterio non e' «il budget non eredita».
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Movement")
	bool bIsRun = false;

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
