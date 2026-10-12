#pragma once

#include "CoreMinimal.h"

/**
 * Il NOME e la FRASE di una risposta in una finestra di reazione rapida ([D-491], #3615).
 *
 * 🔑 **Una tabella sola, chiavata per risposta**, come le frasi delle azioni lo sono per `ActionId`
 * (`RTActionDescriptions`). Prima di questa tabella l'opzione mostrava il token del core cosi' com'era
 * (`FIRE:3`, `Hold Ground`), e il commento di `URTFastDecisionOptionWidget::GetOptionLabel` diceva che dargli un
 * nome era *«lavoro di contenuto, con una tabella e un owner»*.
 *
 * ⛔ **La frase non nomina il bersaglio.** Per D-491 la descrizione e' generica: il nome del bersaglio arriva con
 * la parte «e bersaglio» di #166, che ha il proprio controllo di privacy.
 *
 * ⛔ **Una frase non porta numeri**, per la stessa ragione di `RTActionDescriptions`.
 *
 * Il gate e' `RefactorTactics.Reactions.EveryOfferedResponseHasANameAndADescription`: ogni risposta che una
 * finestra puo' offrire ha una voce. Una risposta nuova senza voce lo fa fallire.
 */
namespace RTReactionResponseText
{
	/** Il nome dell'opzione. Una risposta sconosciuta torna col proprio token: un bottone non resta mai muto. */
	FText NameFor(const FString& Response);

	/** La frase dell'opzione, oppure vuota per una risposta sconosciuta. */
	FText DescriptionFor(const FString& Response);
}
