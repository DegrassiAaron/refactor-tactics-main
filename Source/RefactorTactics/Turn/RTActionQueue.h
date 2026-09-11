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
	 * sono rimaste: `URTActionQueueLibrary::SortActionInstances` ha un solo chiamante fuori dai test
	 * (`ARTTurnManager::ResolvePrep`), che era anche l'unico a numerare davvero. La chiave sbagliata stava su
	 * istanze che nessuno ordinava — inerte, finche' qualcuno non le ordina.
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
