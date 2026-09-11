#pragma once

#include "CoreMinimal.h"
#include "Ability/RTActionDef.h"
#include "Map/RTCellId.h"
#include "RTActionQueue.generated.h"

/**
 * Un'azione PIANIFICATA da un'unita' per il turno corrente: la definizione di catalogo piu' il bersaglio
 * scelto e l'identita' di chi la esegue.
 *
 * L'identita' e' un INTERO STABILE (`SourceUnitId` = indice nello snapshot), mai un pointer: e' la stessa
 * disciplina di `FRTHexSimUnit` e del TurnLog, e senza di essa l'ordine di risoluzione dipenderebbe da
 * indirizzi di memoria.
 *
 * Riferimento: docs/gameplay/spec-motore-azioni-e4.md §3.
 */
USTRUCT(BlueprintType)
struct FRTActionInstance
{
	GENERATED_BODY()

	/** Definizione di catalogo (ID, fase dichiarata, priorita', portata, costo, fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	FRTActionDef Def;

	/** Chi esegue l'azione. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	int32 SourceUnitId = INDEX_NONE;

	/** Unita' bersaglio, se l'azione ne ha una (INDEX_NONE = nessuna). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	int32 TargetUnitId = INDEX_NONE;

	/** Cella bersaglio: AoE, destinazione di movimento, creazione di terreno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	FRTCellId TargetCell;

	/**
	 * Ordine di dichiarazione dell'azione nel turno. Non e' una priorita' nascosta.
	 *
	 * 🔑 **Si conta dove l'istanza NASCE, con un contatore incrementato di suo** (#2970). Non e' una
	 * preferenza di stile: `Num()` letto su un altro array — o sullo stesso, ma piu' in la' nel ciclo —
	 * **non e' questo numero**, e i modi di sbagliarlo sono gia' stati misurati tutti e tre:
	 *
	 * | Sede | Cosa scriveva | Perche' non spareggiava |
	 * |---|---|---|
	 * | `ARTTurnManager::CollectAttackIntents` | `Intents.Num()` | l'`Add` e' duecento righe sotto ed e' condizionato: dopo un `continue` il contatore resta fermo |
	 * | `ARTTurnManager::ResolveCombatPasses` | `Plan.Hits.Num()` | dentro un range-for su `Plan.Hits` e' una **costante**: ogni istanza usciva con lo stesso numero |
	 * | `URTReactionLibrary::BuildReactionEvents` | `Events.Num()` | conta gli EVENTI prodotti, e uno spec che non ne produce lascia il contatore fermo |
	 *
	 * ⚠️ **Nessuna delle tre produceva un rosso**, e la ragione va detta perche' e' anche il motivo per cui
	 * sono rimaste: delle due porte d'ingresso all'ordinamento fuori dai test, solo `ARTTurnManager::ResolvePrep`
	 * riceve istanze reali — ed era anche l'unica a numerare davvero. La chiave sbagliata stava su istanze che
	 * nessuno ordinava — inerte, finche' qualcuno non le ordina.
	 *
	 * ⚠️ **Le porte sono DUE, e una stesura precedente ne dichiarava una** (#3004): `URTActionQueueLibrary::InstancesForPhase`
	 * chiama anch'essa `SortActionInstances`. Oggi non ha chiamanti e non e' `UFUNCTION`, quindi nessun array
	 * reale ci passa; ⛔ ma e' la sede da cui un secondo produttore arriverebbe senza annunciarsi, ed e' li'
	 * che va guardato prima di dare per buona la premessa qui sotto.
	 *
	 * ⚠️ **`ARTTurnManager::ResolvePrep` e' passato al contatore benche' il suo `Instances.Num()` fosse
	 * corretto**, e non e' pulizia: e' l'unica sede il cui `EventSequence` viene davvero consumato, quindi e'
	 * anche la sola in cui un `continue` inserito fra la lettura e l'`Add` costerebbe qualcosa. Reggeva per
	 * adiacenza di due righe, non per costruzione.
	 *
	 * ⛔ **Questa convenzione NON ha un gate, e va detto invece di lasciarlo intendere.** Il campo si legge
	 * solo da `InstanceLess`, e solo le istanze di `ResolvePrep` passano da un sort: i quattro siti restanti
	 * si possono riportare all'idioma sbagliato con la suite interamente verde. A coprirli servirebbe un test
	 * che osservi i produttori — cioe' un mondo — e #2970 dichiara di non introdurlo. Il giorno in cui #1818
	 * desse piu' chiamanti a `SortActionInstances`, quel test diventa necessario prima del refactor, non dopo.
	 *
	 * ⛔ **Non e' piu' l'ULTIMO tie-break**, e il commento lo diceva: da #2970 seguono `TargetUnitId`,
	 * `TargetCell` e `bInterrupted`, perche' cinque chiavi non erano un ordine totale.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	int32 EventSequence = 0;

	/**
	 * L'azione e' stata INTERROTTA prima di risolvere. Un'azione interrotta non produce alcun evento: niente
	 * mezzo danno, niente effetto parziale (catalogo §3, `HeavyAttack`).
	 *
	 * E' un dato dell'istanza e non della definizione perche' dipende dal turno: la stessa `HeavyAttack` e'
	 * interrotta in questo turno e non nel prossimo. A METTERE il flag sara' `Action.Interrupt` (CP 4.7);
	 * qui c'e' il punto in cui la conseguenza si applica, uno solo per tutte le azioni.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Actions")
	bool bInterrupted = false;

	FRTActionInstance() = default;
};
