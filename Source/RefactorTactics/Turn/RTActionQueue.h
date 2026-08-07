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
	 * Ordine di dichiarazione dell'azione nel turno: ULTIMO tie-break dell'ordinamento.
	 * Serve a rendere l'ordine TOTALE quando tutto il resto coincide — non e' una priorita' nascosta.
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
