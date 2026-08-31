#pragma once

#include "CoreMinimal.h"
#include "Map/RTGeometryGrammar.h" // ERTAnchorPairRefusal, FRTAnchorRef

/**
 * Cosa il GESTO DI GEOMETRIA dice mentre lo si compie — `#1895`, consumer d'editor della palette di `#1893`.
 *
 * 🔑 **Perche' questa decisione sta qui e non nel tool.** Di un `UClickDragTool` un automation test vede
 * quasi niente: non trascina un mouse, non guarda un ghost. Cio' che invece puo' esaminare e' *cosa si
 * scrive dato un verdetto* — ed e' esattamente la meta' che, sbagliata, produce un'interfaccia plausibile e
 * falsa: una ragione che non corrisponde al rifiuto, o un rifiuto che non dice niente. E' la stessa scelta
 * di `RTHexProbe` (`#711`) e `RTHexLos` (`#1755`).
 *
 * ⛔ **Nessuna regola di grammatica qui dentro, e nemmeno mezza.** `Describe` RICEVE un
 * `ERTAnchorPairRefusal` gia' deciso da `URTGeometryGrammarLibrary::ExplainPair` e lo traduce. Non cerca
 * l'anchor piu' vicino, non prova gli assi, non decide se una coppia si esprime. Il giorno in cui questo
 * file calcolasse una di quelle tre cose sarebbe la regola nel modulo senza autorita' che il primo Non-goal
 * di `#1895` vieta.
 *
 * ⚠️ **E non e' una traduzione di comodo.** Il criterio dell'issue chiede che il ghost rifiuti *«con la
 * ragione detta come testo»*: senza un tipo che la trasporti, `bool bPreviewValid` costringerebbe il tool a
 * comporre la frase da se', cioe' a decidere quale rifiuto e' quale — che e' di nuovo una regola.
 */
namespace RTHexAnchor
{
	/** Le righe che il gesto mostra. Stringhe: e' presentazione, e il verdetto tipizzato sta a monte. */
	struct FReadout
	{
		/**
		 * L'anchor agganciato, nella forma dichiarata da `FRTAnchorRef::ToString` — `C`, `V3`, `E1`.
		 *
		 * 🔑 Il criterio di `#1895` e' *«quale anchor e' agganciato si vede — non si indovina dalla
		 * posizione del ghost»*: e' questa riga a soddisfarlo, e per questo porta il NOME e non le coordinate.
		 */
		FString Anchor;

		/**
		 * Perche' il gesto non produce un muro — una frase per ciascun rifiuto, mai una che ne copra due.
		 *
		 * ⚠️ Il precedente e' esplicito e nasce da un difetto reale (`#711`): *«"oltre il budget, bloccata o
		 * occupata" mette tre difetti diversi nella stessa frase: chi gioca clicca una cella libera, legge
		 * "bloccata" e crede a un difetto del gioco»*. Qui varrebbe uguale — un autore che ha trascinato su
		 * un'altra cella e legge «non sta su nessun asse tattico» va a correggere la geometria invece del
		 * gesto.
		 *
		 * Vuota solo quando non c'e' niente da spiegare, cioe' quando il muro si fa.
		 */
		FString Reason;

		/** Il gesto produce un muro. Falso = ghost invalido, e `Reason` dice quale dei quattro. */
		bool bValid = false;
	};

	/**
	 * La riga del ghost per un rifiuto gia' deciso dal runtime.
	 *
	 * `From` e `To` servono solo a NOMINARE gli estremi nel testo; la decisione e' interamente in `Refusal`.
	 */
	FReadout Describe(const FRTAnchorRef& From, const FRTAnchorRef& To, ERTAnchorPairRefusal Refusal);

	/**
	 * La riga del solo estremo premuto, prima che il trascinamento abbia un secondo anchor.
	 *
	 * ⚠️ **Non e' un rifiuto**: e' l'assenza della domanda, e ha una frase sua. Senza, un gesto appena
	 * iniziato direbbe «lo stesso anchor due volte» — un verdetto su una coppia che non esiste ancora, che e'
	 * lo stesso difetto che in `#711` produceva «nessuna strada» su una sonda senza unita'.
	 */
	FReadout DescribePending(const FRTAnchorRef& From);

	/**
	 * L'INCIDENZA fra il segmento in corso e uno GIA' PRESENTE nella cella — `#1895` parte 4, che consuma i
	 * reason code di `#1894`.
	 *
	 * 🔑 **Perche' e' una funzione separata e non un quinto caso di `Describe`.** I due rifiuti rispondono a
	 * domande diverse e hanno esiti diversi: `ERTAnchorPairRefusal` dice *«questo gesto non produce un
	 * muro»* e il ghost si spegne; `ERTGeometryViolation` dice *«il muro si fa, ma insieme a un altro forma
	 * una configurazione che la grammatica dichiara invalida»* — e il muro si fa lo stesso.
	 *
	 * ⚠️ **SEGNALA, NON BLOCCA, ed e' una scelta dichiarata.** L'incidenza e' una proprieta' della
	 * COLLEZIONE, e `URTGeometryGrammarLibrary::Validate` e' per costruzione lo strato che *segnala* — la
	 * divisione a due strati di `#620`, dove a rifiutare e' `ValidateSegment`. Il criterio di `#1895` chiede
	 * che l'incidenza *«si veda sui due segmenti coinvolti»*, non che sia impedita: bloccarla renderebbe
	 * impossibile un flusso legittimo — disegnare `A`, poi `B` che lo incrocia, poi correggere `A` — e
	 * l'autore si troverebbe il tool che rifiuta senza sapere quale dei due sia «quello sbagliato».
	 *
	 * ⛔ Nessuna regola qui: la violazione arriva gia' decisa da `Validate`, e questa funzione la traduce.
	 * Restituisce una stringa **vuota** per cio' che non e' un'incidenza fra due — `None` e le violazioni
	 * del singolo segmento, che hanno gia' i loro canali.
	 */
	FString DescribeIncidence(ERTGeometryViolation Violation, int32 OtherWallIndex);
}
