#pragma once

#include "CoreMinimal.h"
#include "RTActionDef.generated.h"

/**
 * Fase di risoluzione dichiarata da un'azione, con i codici del catalogo v0.1 (0/10/20/30/40/50/60).
 *
 * NON e' la fase in cui l'azione risolve davvero: quella e' una macro-fase di Atlas (`ERTMatchPhase`), e la
 * conversione e' URTCatalogLibrary::MapResolutionPhase. I codici restano perche' sono la chiave di lettura dei
 * due PDF del catalogo, e perche' senza di essi non si potrebbe piu' dire da dove viene un valore.
 *
 * Il codice **20** del catalogo si SDOPPIA: `FastMovement` (Dash/Charge/Leap/Sprint/Reposition -> macro-fase
 * Dash, PRIMA del Blast) e `NormalMovement` (`Action.Move` -> macro-fase Move, DOPO il Blast). E' l'unica
 * divergenza strutturale dal catalogo, decisa in ADR-0003 §3.
 */
UENUM(BlueprintType)
enum class ERTResolutionPhase : uint8
{
	/** 0 — congelamento di stato, intenti e seed: nessuna azione risolve qui. */
	Snapshot,
	/** 10 — scudi, stance, trappole, reazioni preparate. */
	Preparation,
	/** 20 — mobilita' rapida (Dash, Charge, Leap, Sprint, Reposition). */
	FastMovement,
	/** 20 — percorso normale (Action.Move). */
	NormalMovement,
	/** 30 — root, push, interrupt, interposizione. */
	Control,
	/** 40 — attacchi, abilita', cure, interazioni. */
	Attack,
	/** 50 — fuoco, acqua, elettricita' e propagazione. */
	Environment,
	/** 60 — KO, obiettivi, cooldown, TurnLog. */
	Cleanup
};

/**
 * Comportamento dell'azione quando, al momento della risoluzione, non e' piu' eseguibile come pianificata
 * (il bersaglio si e' spostato, il percorso si e' chiuso, la cella e' occupata).
 *
 * Il fallback e' DICHIARATO nel catalogo, mai scelto a runtime: una scelta automatica (es. "il nemico piu'
 * vicino") produce esiti poco leggibili, e la leggibilita' tattica e' un pilastro di prodotto.
 */
UENUM(BlueprintType)
enum class ERTActionFallback : uint8
{
	/** Si ferma all'ultima posizione valida (regola standard del movimento). */
	Stop,
	/** Sostituisce l'azione con un'attesa. */
	Wait,
	/** Colpisce comunque la cella pianificata (AoE). */
	AttackCell,
	/** Segue il bersaglio, se ancora valido. */
	AttackTarget,
	/** Usa l'attacco base sul bersaglio valido piu' vicino. */
	BasicAttack,
	/** Non esegue nulla (attacchi diretti e cure). */
	Cancel
};

/**
 * Definizione di un'azione del catalogo v0.1: la parte di dati comune a ogni azione, indipendente dai suoi
 * effetti. Solo INTERI (invariante #4): niente float in costi, priorita', portata o cooldown — il test
 * `RefactorTactics.Catalog.NoFloatInIntegerFields` lo verifica per reflection, non a occhio.
 *
 * Riferimento: docs/design/balance/RT_ActionCatalog_v0.1.md
 */
USTRUCT(BlueprintType)
struct FRTActionDef
{
	GENERATED_BODY()

	/** ID stabile del catalogo (es. `Action.Move`). Chiave del data asset e del TurnLog: non cambia mai. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName ActionId;

	/** Fase dichiarata (codice del catalogo); la macro-fase reale viene da URTCatalogLibrary::MapResolutionPhase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTResolutionPhase ResolutionPhase = ERTResolutionPhase::Attack;

	/** Priorita' intera intra-fase: valore MINORE risolve prima. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 Priority = 50;

	/** Portata in celle esagonali (0 = su se stessi). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 RangeCells = 0;

	/** Costo in punti movimento (0 = nessun costo di movimento). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 CostMP = 0;

	/** Ricarica in turni completi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 CooldownTurns = 0;

	/** Cosa succede se l'azione non e' piu' eseguibile come pianificata. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTActionFallback Fallback = ERTActionFallback::Cancel;

	/**
	 * Limite di propagazione ambientale in celle: **0 = non propaga**, N > 0 = si ferma a N celle.
	 * Un valore negativo significherebbe "senza limite" ed e' rifiutato dal validator: una propagazione
	 * illimitata su una mappa d'acqua colpisce tutti e rende il turno impredicibile (errore da evitare
	 * elencato dal catalogo).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 PropagationLimit = 0;

	/** Se falso, `Action.Interrupt` non ha effetto su questa azione. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bCanBeInterrupted = true;

	FRTActionDef() = default;
};
