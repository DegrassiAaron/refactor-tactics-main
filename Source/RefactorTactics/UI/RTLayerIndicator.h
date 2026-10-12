#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "RTLayerIndicator.generated.h"

/**
 * **Cosa mostra l'indicatore del piano attivo nel dock** ([D-501], #3680): «Piano N di M», con ▲ e ▼.
 *
 * 🔑 **N e M sono ordinali fra i piani che la mappa ha, da 1, non la quota del layer.** I limiti si misurano sulle
 * celle (`ARTPlayerController::GetMapLayerRange`), come fa `SetActiveLayer`: una mappa che comincia a quota 1 mostra
 * «Piano 1 di 2», non «Piano 1 di 1» o «Piano 2». La quota grezza resta nel log (`Piano attivo: N`). Scelta della PR
 * di #3680, aperta nella issue e reversibile qui sola.
 */
struct REFACTORTACTICS_API FRTLayerIndicatorView
{
	/** Falso senza mappa e con un piano solo: indicatore e tasti non compaiono (#3680, scope 4). */
	bool bVisible = false;

	/** N: il piano attivo contato dal piu' basso, da 1. */
	int32 Ordinal = 0;

	/** M: quanti piani separano il piu' basso dal piu' alto, estremi inclusi. */
	int32 Count = 0;

	/** ▲ e ▼ sono disattivati ai limiti, non nascosti: il giocatore vede dove sta. */
	bool bCanGoUp = false;
	bool bCanGoDown = false;

	/** «Piano N di M». Vuoto quando non e' visibile. */
	FText Label;
};

namespace RTLayerIndicator
{
	/**
	 * La vista dell'indicatore da (piano attivo, limiti della mappa). Pura: il dock la chiede a ogni tick, e i test
	 * la misurano senza mondo.
	 *
	 * ⚠️ `Active` fuori dai limiti si clampa per il conto, come farebbe `SetActiveLayer`: non e' uno stato che il
	 * controller produca, ma una vista non deve dire «Piano 0 di 3».
	 */
	REFACTORTACTICS_API FRTLayerIndicatorView Build(bool bHasRange, int32 Active, int32 MinLayer, int32 MaxLayer);
}

/**
 * **Un bottone che si clicca e non prende il fuoco della tastiera** (#3680).
 *
 * 🔴 **Un `UButton` di default e' focalizzabile, e uno `SButton` con il fuoco tratta `Spazio` e `Invio` come un
 * click.** Dopo un click su ▲, `Spazio` — il lock-in — avrebbe ripremuto ▲ invece di chiudere il turno. Il fuoco si
 * decide prima che lo `SWidget` nasca (`UButton::InitIsFocusable`, protetta), quindi serve una sottoclasse.
 */
UCLASS()
class REFACTORTACTICS_API URTMouseOnlyButton : public UButton
{
	GENERATED_BODY()

public:
	URTMouseOnlyButton(const FObjectInitializer& ObjectInitializer);
};
