#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Combat/RTCombatResolver.h" // FRTAttack: i colpi di ritorno escono di qui

class ARTUnit;

/**
 * Cosa ha spostato un'unita' contro la sua volonta' (`#307`), raccolto durante il Blast e consumato quando la
 * spinta o la trazione si applicano.
 *
 * **Una struct e non tre mappe parallele**, e la ragione e' un difetto vero: la prima stesura teneva
 * `ActionId` e `BaseActionId` in due `TMap` distinte **condivise fra spinta e trazione**, e con entrambe sullo
 * stesso bersaglio la seconda `Add` sovrascriveva la prima — una voce finiva per dichiarare l'attaccante
 * sbagliato. Trovato in code review, corretto separando le mappe per effetto. Tenere i campi INSIEME toglie
 * la classe di errore a monte: non esiste piu' un modo di disallinearli fra loro.
 */
struct FRTDisplacementCause
{
	/** Azione che ha prodotto lo spostamento (`Action.Push`, `Guardian.Sweep`, …). */
	FName ActionId;
	/** Generica di cui `ActionId` e' un profilo, quando la dichiara (D-033). */
	FName BaseActionId;
	/** Priorita' intra-fase dell'azione (CP 11.3): con quale precedenza ha risolto. */
	int32 Priority = 0;
};

/**
 * UN colpo di ritorno, con tutto cio' che lo qualifica.
 *
 * **Un record e non sei array paralleli** (`#2587`), e la ragione e' la stessa che ha prodotto
 * `FRTDisplacementCause` qui sopra: sei `Add` in fila sono sei occasioni di scriverne cinque, e il primo
 * `continue` infilato in mezzo attribuirebbe il contrattacco all'unita' sbagliata. Fino al 2026-09-06 il
 * commento della struct qui sotto DICHIARAVA il rischio e lo lasciava in piedi; tenere i campi insieme lo
 * toglie a monte — non esiste piu' un modo di disallinearli fra loro.
 *
 * ⚠️ **Non e' una `USTRUCT`**, come il resto di questo header: e' un dato transiente della risoluzione, non
 * entra in serializzazione ne' in replica, e `Actor` resta un puntatore grezzo esattamente com'era quando
 * viveva in `TArray<ARTUnit*>`. Il refactor non cambia il lifetime: lo sposta.
 */
struct FRTCounterAttack
{
	/** Il colpo vero e proprio, accodato ai colpi della fase. */
	FRTAttack Attack;
	/** Cella di chi contrattacca, per il TurnLog e per la voce direzionale (`#2128`). */
	FRTCellId SourceCell;
	/** Identita' della reazione che ha prodotto il colpo di ritorno (CP 11.3, `#79`). */
	FName ActionId;
	/** Generica di cui `ActionId` e' un profilo, quando la dichiara (D-033). */
	FName BaseActionId;
	/** Priorita' intra-fase della reazione. */
	int32 Priority = 0;
	/** E CHI contrattacca: una cella non identifica un'unita' ([D-063]). */
	ARTUnit* Actor = nullptr;
};

/**
 * Cio' che il pass delle reazioni RACCOGLIE e che il chiamante applica insieme al resto della propria fase
 * (CP 5.1, `#505`).
 *
 * Una struct e non sette variabili locali perche' il pass smette di essere uno solo: con `D-092` diventa
 * richiamabile per fase, e sette parametri d'uscita separati sono sette occasioni di passarne uno in meno.
 *
 * Non contiene le FUGHE (`SelfReposition`): quelle il pass le raccoglie e le applica al proprio interno, dopo
 * aver valutato tutte le reazioni sullo snapshot congelato (`D-094`). Uscire di qui con delle destinazioni da
 * applicare piu' tardi rimetterebbe in gioco proprio l'ordine che quella decisione toglie di mezzo.
 */
struct FRTReactionPassResult
{
	/** Riduzione del danno per bersaglio dichiarata dalle reazioni attivate: entra nel delta del PRIMO danno. */
	TArray<int32> DeflectDelta;

	/**
	 * Colpi di ritorno, accodati ai colpi veri della fase, **in ordine di produzione**.
	 *
	 * L'ordine e' quello in cui il pass li raccoglie e non va ricostruito: chi consuma accoda in coda ad
	 * `Attacks`, e la voce direzionale di `#2128` itera esattamente quella coda.
	 */
	TArray<FRTCounterAttack> CounterAttacks;

	/**
	 * Chi ha ANNULLATO lo spostamento che stava per subire (`Reaction.Anchor`, `CancelDisplacement`): indici
	 * in `Units`, che i rami di spinta e trazione consultano prima di muovere.
	 *
	 * Un `TSet` perche' l'uso e' solo `Contains`: non ci si itera sopra, quindi l'ordine non deterministico di
	 * `TSet` non puo' entrare nell'esito (invariante #3).
	 */
	TSet<int32> CancelledDisplacements;

	/**
	 * Chi ha ANNULLATO il controllo che stava per ricevere (`Reaction.Cleanse`, `CancelStatus`): indici in
	 * `Units`, che il punto di applicazione degli stati consulta prima di applicarli.
	 *
	 * Quale dei controlli in arrivo salti non e' qui: lo decide chi applica, che ha davanti la lista completa
	 * e sceglie **il piu' grave** (`URTReactionLibrary::ControlSeverityRank`). Metterlo qui vorrebbe dire
	 * scegliere due volte, in due posti che possono divergere.
	 */
	TSet<int32> CancelledControls;

	/**
	 * Chi fugge SENZA una sorgente da cui allontanarsi (`Reaction.HazardEscape`): indici in `Units`, e
	 * `HazardFleeDistance` parallelo con quante celle.
	 *
	 * Lista separata dalle fughe del Blast, e non e' una duplicazione: quelle hanno una direzione — via da chi
	 * ha innescato — e il pass le applica al proprio interno con la geometria della spinta. Qui la direzione
	 * non esiste: c'e' una cella diventata pericolosa e basta, quindi **dove** si va lo decide il chiamante
	 * (`URTTerrainLibrary::FindEscapeCell`, che guarda il facing). Un solo array per entrambe avrebbe
	 * costretto il pass a conoscere due geometrie.
	 */
	TArray<int32> HazardFlees;
	TArray<int32> HazardFleeDistance;
};
