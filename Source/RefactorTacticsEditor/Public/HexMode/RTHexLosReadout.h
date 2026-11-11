#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

struct FRTLineOfSightResult;

/**
 * Cosa l'ispettore SCRIVE, e QUANDO decide di richiedere. #1755, consumer d'editor della LOS canonica.
 *
 * 🔑 **Perche' queste due decisioni stanno qui e non nel tool.** Di un `UInteractiveTool` con hover un
 * automation test vede quasi niente: non puo' muovere un mouse, non puo' guardare un pannello. Cio' che
 * invece puo' esaminare e' *cosa* si scrive dato un esito, e *se* una nuova query e' dovuta. Sono le due
 * cose che, sbagliate, producono un ispettore plausibile e falso — una ragione inventata, o una query per
 * fotogramma. E' la stessa scelta di `FRTLauncherScenarioBrowser` (#1705) e `RTScenarioViewport` (#1753).
 *
 * ⛔ **Nessuna LOS qui dentro, e nemmeno mezza.** `Describe` RICEVE un `FRTLineOfSightResult` gia' prodotto
 * da `URTHexVisionLibrary::DescribeLineOfSight` e lo traduce. Non ripercorre la linea, non guarda le celle,
 * non deduce. Se un giorno questo file includesse `RTHexCoverLibrary` o `RTHexLibrary` per decidere
 * qualcosa, sarebbe la seconda LOS che #1712 e #1755 esistono per impedire.
 */
namespace RTHexLos
{
	/** Le tre righe del pannello. Stringhe: e' presentazione, e il verdetto tipizzato sta a monte. */
	struct FReadout
	{
		/** `CLEAR` · `BLOCKED` · `—` quando manca un estremo. */
		FString Verdict;

		/** La causa canonica col suo punto, oppure `—`. Mai una ragione inventata. */
		FString Reason;

		/**
		 * Il piano su cui la LOS sta ragionando.
		 *
		 * ⚠️ **E' quello del TIRATORE, non del bersaglio**, e la differenza e' il caveat che #1712 registra:
		 * `HexLine` tiene la linea sul layer di chi guarda — *«da terra si spara sotto un ponte, da un piano
		 * superiore si spara oltre le coperture basse»*. Un ispettore che mostrasse il layer del bersaglio
		 * direbbe su quale piano sta il bersaglio, non su quale piano e' stata decisa la linea.
		 */
		int32 Layer = 0;
	};

	/**
	 * La riga del pannello per un esito gia' deciso dal runtime.
	 *
	 * Senza origine o senza bersaglio non si inventa niente: `—` su entrambe le righe. Un ispettore che
	 * mostrasse `CLEAR` prima che qualcuno abbia scelto due punti direbbe che la vista passa fra due celle
	 * che nessuno ha nominato.
	 */
	FReadout Describe(bool bHasOrigin, const FRTCellId& Origin,
		bool bHasTarget, const FRTCellId& Target,
		const FRTLineOfSightResult& Los);

	/**
	 * Va rifatta la query? Vero solo se la cella puntata e' **cambiata**.
	 *
	 * 🔴 **E' il guardrail di #1755 in forma di funzione, non un'ottimizzazione.** Il mouse produce eventi
	 * mentre si muove dentro la stessa cella, e una LOS ricalcolata a ogni pixel sarebbe una query per
	 * fotogramma travestita da event-driven. Qui la domanda si fa una volta per cella, e il test la pinna:
	 * ⛔ *«Nessun Tick […] Una query LOS su hover puo' essere event-driven dal cambio della cella hoverata,
	 * non per-frame se nulla cambia.»*
	 *
	 * ⚠️ Il layer fa parte dell'identita' della cella: passare da `(3,4,L0)` a `(3,4,L1)` **e'** un cambio,
	 * e trattarlo come «stessa cella» mostrerebbe il verdetto del piano sbagliato.
	 */
	bool ShouldRequery(bool bLastValid, const FRTCellId& Last, bool bNowValid, const FRTCellId& Now);
}
