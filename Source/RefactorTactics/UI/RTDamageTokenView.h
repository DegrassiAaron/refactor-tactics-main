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
 * (`ApplyDamageDelta`, `ApplyAbsorptionPool` e `ApplyEligibleHitDelta` in `ResolveCombatPasses`), e lo scudo assorbe piu' a valle ancora. 🔑 Ne segue che
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

	/**
	 * Vero per il token di una CURA ([D-505], `#3600`): lo stesso token, con il segno `+`. Chi disegna puo'
	 * sceglierne il colore quando §32 ne avra' uno; oggi il segno e' l'unico canale, ed e' sufficiente (`#2453`).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bIsHeal = false;

	/**
	 * Vero quando la barra della vita deve pulsare: la vita e' cambiata, in giu' (danno) o in su (cura).
	 * 🔑 **Un campo, come `bHasDamage`, e per la stessa ragione**: la regola la decidono i costruttori del token,
	 * non chi disegna. ⏱️ *Fino a #3600 il widget pulsava su `bHasDamage`, che una cura non puo' avere.*
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bPulsesHealthBar = false;

	/** Il testo gia' composto: `-17` quando c'e' danno, `+18` per una cura, l'etichetta del caso zero altrimenti. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FString Label;
};
