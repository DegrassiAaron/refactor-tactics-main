#pragma once

#include "CoreMinimal.h"
#include "RTDamageTokenView.generated.h"

/**
 * Il token di danno che compare accanto a un'unita' quando il playback rivela un colpo (`#2455`).
 *
 * 🔴 **Porta un fatto GIA' RISOLTO, e la presentazione non lo ricalcola.** Il numero e' quello che
 * `FRTResolvedEvent::Amount` porta: ⛔ **mai** un delta osservato sulla barra della vita. Dedurre il danno
 * guardando la barra scendere e' il difetto che l'epic `#2453` elenca fra i quattro modi in cui questo
 * sistema fallirebbe in silenzio — *«widget osserva delta HP -> deduce un attacco»*.
 *
 * ⚠️ **`Amount` e' il danno NOMINALE, non gli HP persi, ed e' una convenzione DICHIARATA.** Per un
 * `Attack` vale `Hit.Power`, cioe' la potenza dell'intento **meno la sola copertura**
 * (`URTHexCombatLibrary::CollectHexAttacks`): Deflect, Guard e Brace agiscono dopo, su un array diverso
 * (`ApplyAbsorptionPool` in `ResolveCombatPasses`), e lo scudo assorbe piu' a valle ancora. 🔑 Ne segue che
 * un colpo da 30 su un bersaglio in Brace con scudo mostra `-30` mentre la barra scende di meno o di
 * niente: **e' voluto**. E' la stessa convenzione che `#2460` ha fissato per `HazardDamage`, e cambiarla
 * qui scollegherebbe i due canali che devono raccontare lo stesso colpo.
 */
USTRUCT(BlueprintType)
struct FRTDamageTokenView
{
	GENERATED_BODY()

	/**
	 * Chi SUBISCE, come `ARTUnit::StableUnitId`.
	 *
	 * ⚠️ **`0` non e' l'unita' numero zero: e' «nessuno»** ([D-063]). `EnsureMatchRoster` assegna gli id a
	 * partire da `1` e lascia lo zero libero apposta. Un token attribuito all'unita' zero sarebbe un colpo
	 * disegnato sopra la testa sbagliata.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 TargetStableUnitId = 0;

	/** Il valore dell'evento, cosi' com'e' arrivato. Vedi la convenzione nel commento della struct. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Amount = 0;

	/**
	 * Vero quando c'e' una quantita' da mostrare come cifra; falso nel caso zero.
	 *
	 * 🔑 **E' un campo e non un `Amount > 0` ricalcolato da chi disegna**: la regola dello zero e' una
	 * decisione (vedi `URTHudViewModel::BuildDamageToken`), e lasciarla dedurre a ogni consumatore
	 * significherebbe che il primo che la scrive al contrario non fa fallire niente.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bHasDamage = false;

	/** Il testo gia' composto: `-17` quando c'e' danno, l'etichetta del caso zero altrimenti. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FString Label;
};
