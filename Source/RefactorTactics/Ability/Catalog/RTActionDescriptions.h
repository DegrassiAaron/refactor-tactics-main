#pragma once

#include "CoreMinimal.h"

/**
 * La FRASE d'autore di un'azione: cio' che il tooltip mostra sopra i numeri (`#3499`).
 *
 * 🔑 **Una tabella sola, chiavata per `ActionId`**, come i nomi (`HeroActionDisplayName`,
 * `GenericActionDisplayName`): le fabbriche del catalogo la applicano dove applicano il nome, e il gate
 * `Catalog.EveryRosterActionHasADescription` verifica che nessuna azione del roster ne resti senza.
 *
 * ⛔ **Una frase non porta numeri.** Danno, portata e ricarica li compone il gioco dal catalogo
 * (`URTHudViewModel::BuildActionTooltip`): una frase che li ripetesse invecchierebbe al primo ribilanciamento,
 * e nessun gate se ne accorgerebbe.
 *
 * 📄 Le stesse frasi stanno in `docs/balance/RT_ActionDescriptions_v0.1.md`, che e' cio' che l'autore rilegge.
 * `tools/radar/action-descriptions.ts --check` confronta le due copie, e non dice quale correggere.
 */
namespace RTActionDescriptions
{
	/** La frase di `ActionId`, oppure vuota: un'azione senza frase mostra i soli numeri. */
	FText For(const FName& ActionId);

	/** Tutte le voci, nell'ordine del documento: le leggono il gate e i test. */
	const TArray<TPair<FName, FString>>& All();
}
