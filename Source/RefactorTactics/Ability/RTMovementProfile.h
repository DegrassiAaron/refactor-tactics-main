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
	 * **Passi**: quanto lontano si arriva, in **percentuale del budget base dell'unita'** ([D-412]).
	 * Ogni cella attraversata vale `1` ([D-117] voce 1).
	 *
	 * 🔴 **Era un ASSOLUTO fino al 2026-09-13, e [D-412] lo rende un MOLTIPLICATORE**: `Withdraw` ×0,25 ·
	 * `Sneak` ×0,5 · `Move` ×1 · `Sprint` ×2, cioe' `25` · `50` · `100` · `200` qui. Il valore precedente —
	 * `Sprint` 8, `Withdraw` 2 e `InheritFromUnit` per il neutro — cablava numeri che non seguivano l'eroe:
	 * uno `Sprint` da 8 era piu' lento del `Move` di chi ne vale 9.
	 *
	 * 🔑 **Percentuale intera e non `float`, e non e' pedanteria**: la risoluzione dev'essere deterministica
	 * ([D-096]), e un moltiplicatore in virgola mobile porterebbe l'arrotondamento dell'hardware dentro il
	 * budget. La divisione intera **tronca**, che e' esattamente l'*«arrotondare per difetto»* che [D-412]
	 * prescrive.
	 *
	 * ⚠️ **Il valore `100` NON e' un default innocuo**: e' la dichiarazione «questo profilo vale quanto
	 * l'unita'», che prima si scriveva `InheritFromUnit`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Movement")
	int32 StepBudgetPercent = NeutralPercent;

	/**
	 * **Asperita'**: quanta ne assorbe il movimento ([D-117] voce 2), nella stessa percentuale.
	 *
	 * ⚠️ **Oggi porta lo stesso valore di `StepBudgetPercent`, e non e' una svista.** La funzione di costo
	 * che separa le due misure — `max(0, MoveCost - 1 + MoveCostModifier)` — e' [#666]. Finche' quella non
	 * esiste, ogni cella costa `1` e le due misure coincidono: separare qui i CAMPI senza separare ancora i
	 * VALORI e' il prerequisito che `#653` ha consegnato. 🔑 **E [D-412] vale su ENTRAMBI**: il
	 * moltiplicatore e' del profilo, non di uno dei due budget.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Movement")
	int32 MoveBudgetPercent = NeutralPercent;

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
	 * ✅ **`Sneak` e' tornato `true` il 2026-09-13**: `AE-5` e' chiusa da [D-412], che gli da' i tre numeri
	 * che mancavano — budget ×0,5, cadenza 1 passo ogni 2 tick, **sempre silenzioso**. ⏱️ *Era `false`, ed
	 * era giusto che lo fosse: il catalogo gli assegnava «—» invece di un budget, e un profilo senza numeri
	 * non e' pianificabile senza inventare quello che manca.*
	 *
	 * ⚠️ **Il campo resta**, e non e' un residuo: `Still` e `Withdraw` non si scelgono per due ragioni
	 * diverse — il primo e' DERIVATO dall'assenza di waypoint, il secondo e' RISERVATO da chi lo impone —
	 * e nessuna delle due e' «non ha numeri». Le tre esclusioni vivranno insieme nel selettore di [#1410],
	 * che su `origin/main` non esiste ancora.
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

	/** Il profilo neutro: vale quanto l'unita' dichiara. `Move` e' questo ([D-412]). */
	static constexpr int32 NeutralPercent = 100;

	/** Il profilo esiste (`FindProfile` ha trovato qualcosa). */
	bool IsValid() const { return !Id.IsNone(); }

	/** Il budget dei passi per QUESTA unita': la percentuale del profilo, troncata per difetto ([D-412]). */
	int32 ResolveStepBudget(int32 UnitMoveRange) const
	{
		return ScaleBudget(UnitMoveRange, StepBudgetPercent);
	}

	/** Il budget di asperita' per QUESTA unita': stessa percentuale, stesso troncamento ([D-412]). */
	int32 ResolveMoveBudget(int32 UnitMoveRange) const
	{
		return ScaleBudget(UnitMoveRange, MoveBudgetPercent);
	}

private:
	/**
	 * `base × percentuale / 100`, in **aritmetica intera** e con un pavimento a zero.
	 *
	 * 🔑 **Il troncamento E' la regola**, non un effetto collaterale della divisione intera: [D-412]
	 * prescrive di arrotondare **per difetto**, e con operandi non negativi `/` in C++ tronca verso lo zero,
	 * cioe' fa esattamente questo. Scriverlo con un `FMath::FloorToInt` su un `float` darebbe lo stesso
	 * numero e porterebbe la virgola mobile dentro una risoluzione che dev'essere deterministica ([D-096]).
	 *
	 * ⚠️ **Il pavimento a zero protegge da un dato malformato, non da un caso di gioco**: nessun profilo
	 * del catalogo dichiara una percentuale negativa, e se uno lo facesse il posto in cui deve fallire e' il
	 * test del catalogo — qui si evita solo che un budget negativo raggiunga il pathfinding.
	 */
	static int32 ScaleBudget(int32 UnitMoveRange, int32 Percent)
	{
		if (UnitMoveRange <= 0 || Percent <= 0)
		{
			return 0;
		}
		return (UnitMoveRange * Percent) / NeutralPercent;
	}
};
