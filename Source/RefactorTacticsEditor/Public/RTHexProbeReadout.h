#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

enum class ERTHexProbeExclusion : uint8;

/**
 * Cosa la SONDA DI MOVIMENTO scrive, e QUANDO decide di richiedere. #711, consumer d'editor del reachable
 * set canonico.
 *
 * 🔑 **Perche' queste due decisioni stanno qui e non nel tool.** Di un `UInteractiveTool` con hover un
 * automation test vede quasi niente: non puo' muovere un mouse, non puo' guardare un pannello. Cio' che
 * invece puo' esaminare e' *cosa* si scrive dato un esito, e *se* una nuova query e' dovuta. Sono le due
 * cose che, sbagliate, producono una sonda plausibile e falsa — un motivo che non corrisponde
 * all'esclusione, o un A* per fotogramma. E' la stessa scelta di `RTHexLos` (#1755).
 *
 * ⛔ **Nessuna regola di movimento qui dentro, e nemmeno mezza.** `Describe` RICEVE un
 * `ERTHexProbeExclusion` gia' prodotto da `URTHexSimLibrary::ClassifyProbeCell` e lo traduce. Non cerca
 * percorsi, non somma costi, non guarda la mappa. Se un giorno questo file includesse `RTHexPathLibrary`
 * sarebbe la seconda ricerca che il divieto di #711 esiste per impedire.
 */
namespace RTHexProbe
{
	/** Le righe del pannello. Stringhe: e' presentazione, e il verdetto tipizzato sta a monte. */
	struct FReadout
	{
		/** `2 / 4 MP` per una cella raggiungibile, `—` per una esclusa. */
		FString Cost;

		/**
		 * Perche' quella cella no — una frase per ognuno dei cinque motivi, mai una che ne copra due.
		 *
		 * Vuota (o `—`) solo quando non c'e' niente da spiegare, cioe' su una cella del set.
		 */
		FString Reason;

		/**
		 * I passi del percorso in hover: celle meno la partenza.
		 *
		 * ⚠️ **Non e' il costo.** Su terreno costoso tre passi possono valere sei punti movimento, ed e'
		 * esattamente la differenza che il designer sta cercando quando dipinge una superficie.
		 */
		int32 Steps = 0;
	};

	/**
	 * La riga del pannello per un'esclusione gia' decisa dal runtime.
	 *
	 * `bHasUnit == false` non e' un'esclusione: e' l'assenza della domanda, e ha una frase sua. Senza, una
	 * sonda appena aperta direbbe «nessuna strada» — un verdetto sulla mappa dove non c'e' ancora nessuno a
	 * cui applicarlo.
	 */
	/**
	 * Il budget di un eroe, e **se quell'eroe esiste**.
	 *
	 * 🔴 Le due cose stanno insieme perche' separarle e' cio' che ha prodotto il difetto: un `HeroId`
	 * sconosciuto dava `0`, e `0` e' indistinguibile da «un eroe che non puo' muoversi». Il pannello ne
	 * ricavava «fuori budget», mandando a correggere il numero invece del nome.
	 *
	 * ⚠️ **Chiede il catalogo, quindi NON si chiama a ogni hover.** Il roster si costruisce da zero a ogni
	 * invocazione — quattro `URTHeroData` e una `NewObject` per ciascuna delle loro azioni — e pagarlo per
	 * cella sorvolata satura il game thread. Si risolve quando l'eroe **cambia**, e il valore si tiene.
	 */
	struct FBudget
	{
		/** `false` = nessun eroe con quell'id nel catalogo. Non e' «zero movimento». */
		bool bKnown = false;

		/** I punti movimento dell'eroe. `0` quando `bKnown` e' `false`. */
		int32 Points = 0;
	};

	FBudget ResolveBudget(FName HeroId);

	FReadout Describe(bool bHasUnit, ERTHexProbeExclusion Exclusion, int32 Cost, int32 Budget, int32 PathCells,
		bool bKnownHero = true);

	/**
	 * Va rifatta la domanda? Vero solo se la cella puntata e' **cambiata**.
	 *
	 * 🔴 Il gate e' quello di `RTHexLos::ShouldRequery` — la stessa regola, **chiamata** e non ricopiata
	 * (`RTHexHover`). Due cancelli con la stessa regola divergono, e qui il costo di un evento non filtrato
	 * e' piu' alto che nell'ispettore LOS: per una cella esclusa e libera la classificazione chiede al
	 * pathfinder un percorso a costo illimitato.
	 */
	bool ShouldRequery(bool bLastValid, const FRTCellId& Last, bool bNowValid, const FRTCellId& Now);
}
