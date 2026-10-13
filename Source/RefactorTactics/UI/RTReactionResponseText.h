#pragma once

#include "CoreMinimal.h"

struct FRTReactionWindowView;
struct FRTReactionWindowOptionView;

/**
 * Il NOME e la FRASE di una risposta in una finestra di reazione rapida ([D-491], #3615).
 *
 * 🔑 **Una tabella sola, chiavata per risposta**, come le frasi delle azioni lo sono per `ActionId`
 * (`RTActionDescriptions`). Prima di questa tabella l'opzione mostrava il token del core cosi' com'era
 * (`FIRE:3`, `Hold Ground`), e il commento di `URTFastDecisionOptionWidget::GetOptionLabel` diceva che dargli un
 * nome era *«lavoro di contenuto, con una tabella e un owner»*.
 *
 * ⛔ **La frase CHIAVATA PER RISPOSTA non nomina il bersaglio.** Per D-491 e' generica. ✅ Il nome del
 * bersaglio e' arrivato con `#3667`, la parte «e bersaglio» di #166: lo portano le funzioni che leggono la
 * VISTA — `DescriptionFor(Option)`, `TitleFor`, `SubtitleFor` — e mai questa tabella. I nomi li ha gia'
 * risolti il core, col proprio controllo di privacy: qui si compone una frase con cio' che la vista porta, e
 * dove la vista non porta un nome la frase resta generica.
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

	/**
	 * La frase di UN'opzione della finestra, come la scrive la tavola E (`#3667`, R2.3):
	 * - un `FIRE` che porta il nome del bersaglio: «Attacca Ivrin»;
	 * - `HOLD`: «Non attaccare»;
	 * - ogni altra risposta — le manovre di profilo — e un `FIRE` senza nome: la frase della tabella.
	 *
	 * 🔑 **Le frasi di `FIRE` e `HOLD` sono per-opzione, e NON stanno nella tabella.** La tabella chiavata per
	 * risposta resta quella di D-491, generica e immutata: chi la interroga per stringa riceve cio' che
	 * riceveva. Qui si compone la frase di un'opzione di una finestra che il core ha gia' riempito.
	 *
	 * ⚠️ Un `FIRE` senza nome ripiega sulla frase generica della tabella, che ha un altro registro di «Non
	 * attaccare». Succede solo se il core non ha potuto risolvere il bersaglio; i due siti di produzione lo
	 * risolvono sempre, perche' l'indice viene dall'array che hanno in mano.
	 */
	FText DescriptionFor(const FRTReactionWindowOptionView& Option);

	/**
	 * Il titolo della finestra: «Guardia reattiva — Aevik» (`#3667`).
	 *
	 * Si compone con cio' che la vista porta, e degrada senza inventare: senza il nome della reazione resta
	 * «Reazione — Aevik», senza quello dell'unita' «Guardia reattiva», senza nessuno dei due «Reazione» — che
	 * e' cio' che la finestra diceva sempre prima di `#3667`. Vuoto se la finestra non e' aperta.
	 */
	FText TitleFor(const FRTReactionWindowView& Window);

	/**
	 * Il sottotitolo: che cosa ha fatto scattare la finestra (`#3667`). La frase la sceglie
	 * `Window.TriggerKind`, che scrive chi apre la finestra — qui non si guarda l'id della reazione.
	 *
	 * - `EnteredControlledArea`: «Ivrin è entrato nell'area controllata»; con piu' unita' nello stesso passo,
	 *   «Ivrin e Branth sono entrati nell'area controllata». Senza nomi, nessuna frase: una riga che non dice
	 *   chi e' entrato non aggiunge niente al titolo.
	 * - `Pushed`: «Ivrin ti spinge». ⚠️ **Senza un nome nella vista, «Un nemico ti spinge»**: e' il caso di chi
	 *   spinge da una cella che la squadra non rileva, e il nome non c'e' perche' il core non lo ha messo.
	 * - `None`, o finestra non aperta: vuoto.
	 */
	FText SubtitleFor(const FRTReactionWindowView& Window);
}
