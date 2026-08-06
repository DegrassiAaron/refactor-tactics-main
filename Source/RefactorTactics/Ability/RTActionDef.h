#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Turn/RTActionEvent.h"
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
 * Slot del turno che un'azione occupa. Il catalogo v0.1 (§«Slot per turno») ne dichiara uno per riga: senza
 * questo dato, «Sprint consuma movimento **e** azione principale» diventerebbe un `if` sull'ActionId dentro
 * l'orchestratore — cioe' il tipo di eccezione hard-coded che il motore azioni esiste per togliere.
 */
UENUM(BlueprintType)
enum class ERTActionSlot : uint8
{
	/** Non occupa slot: resta osservabile nel TurnLog ma non toglie nulla al piano (`Action.Wait`). */
	None,
	/** Slot movimento (`Action.Move`). */
	Movement,
	/** Azione principale: attacchi, scatti, guardia, cure. E' il caso comune. */
	Main,
	/** Consuma ENTRAMBI gli slot: chi la usa non si muove oltre e non agisce (`Action.Sprint`). */
	MovementAndMain,
	/**
	 * Slot reazione (CP 5.1, epic E5): 0-1 per turno, indipendente da Movimento e Principale — un eroe puo'
	 * muoversi, agire E tenere una reazione pronta nello stesso turno. Si dichiara in planning come le altre
	 * azioni; a differenza loro non ha una cella o un bersaglio scelti dal giocatore, ma un `ReactionTrigger`
	 * valutato deterministicamente sullo snapshot del Blast.
	 */
	Reaction
};

/**
 * Condizione che fa scattare una reazione (CP 5.1), valutata sullo snapshot GIA' RACCOLTO del Blast
 * (`FRTHexBlastPlan::Hits`, dopo il filtro di `Action.Interrupt`): mai un `Delay`, una timeline o un montage
 * nel resolver (invariante #3) — e' per questo che il trigger e' un dato puro da confrontare con un array,
 * non un evento a cui reagire mentre il turno gira.
 */
UENUM(BlueprintType)
enum class ERTReactionTrigger : uint8
{
	/** Nessun trigger dichiarato: non e' una reazione, o non ne ha ancora uno (dato incompleto). */
	None,
	/**
	 * L'unita' e' bersaglio di almeno un colpo DIRETTO (`ERTAbilityShape::Single`) andato a segno in questo
	 * Blast (`Action.Counter`, `Action.Deflect` — CP 5.2 aggiunge i loro effetti sulla stessa valutazione).
	 */
	HitByDirectAttack
};

/**
 * COME si sposta un'azione di mobilita'. E' un dato del catalogo, non un ramo nell'orchestratore: `Dash` e
 * `Sprint` risolvono nella stessa macro-fase ma si muovono in due modi diversi, e senza questo campo la
 * differenza finirebbe in un `if` sull'ActionId.
 */
UENUM(BlueprintType)
enum class ERTMovementStyle : uint8
{
	/** L'azione non sposta chi la usa (attacchi, guardia, interazioni). */
	None,
	/** Percorso a costi interi dentro un budget di MP, ostacoli aggirati in pianificazione (`Action.Sprint`). */
	Budget,
	/** Linea retta su una delle sei direzioni: si ferma davanti a muri e unita' (`Dash`, `Reposition`). */
	LinearDash,
	/** Come `LinearDash`, ma si ferma SUL primo nemico incontrato e lo colpisce (`Charge`). */
	LinearCharge,
	/** Salto: ignora unita' e celle intermedie, conta solo dove si atterra (`Leap`). */
	LinearLeap
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
	 * Slot del turno consumato dall'azione. Default `Main`: e' lo slot della maggior parte delle azioni
	 * (attacchi, scatti, guardia) e coincide con quello delle abilita' gia' spedite.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTActionSlot Slot = ERTActionSlot::Main;

	/** Come l'azione sposta chi la usa. `None` per tutto cio' che non e' mobilita'. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTMovementStyle MovementStyle = ERTMovementStyle::None;

	/**
	 * Limite di propagazione ambientale in celle: **0 = non propaga**, N > 0 = si ferma a N celle.
	 * Un valore negativo significherebbe "senza limite" ed e' rifiutato dal validator: una propagazione
	 * illimitata su una mappa d'acqua colpisce tutti e rende il turno impredicibile (errore da evitare
	 * elencato dal catalogo).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 PropagationLimit = 0;

	/**
	 * Effetti prodotti dall'azione, nell'ordine in cui si applicano. E' il campo che il registry traduce in
	 * eventi: cambiare cosa fa un'azione significa cambiare QUESTO, non aggiungere un ramo nell'orchestratore.
	 *
	 * E' una LISTA perche' molte azioni combinano piu' effetti (la Spazzata infligge danno **e** respinge).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TArray<FRTActionEffectSpec> Effects;

	/**
	 * Se falso, chi usa questa azione NON puo' tenere pronta una reazione in questo turno (`Action.Sprint`:
	 * chi corre a perdifiato non para). Fatta valere da `ARTTurnManager::ResolveCombat` (CP 5.1): una reazione
	 * pianificata insieme a un'azione con questo campo a falso finisce nel TurnLog come non disponibile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bAllowsReaction = true;

	/**
	 * Condizione che fa scattare QUESTA azione, se `Slot == Reaction` (CP 5.1). `None` per tutto cio' che
	 * reazione non e'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTReactionTrigger ReactionTrigger = ERTReactionTrigger::None;

	/** Se falso, `Action.Interrupt` non ha effetto su questa azione. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bCanBeInterrupted = true;

	/**
	 * L'azione colpisce anche gli ALLEATI di chi la usa (`Action.CircularAoE`, catalogo §3). Default falso:
	 * il fuoco amico e' l'eccezione dichiarata da una singola azione, non il comportamento di base.
	 *
	 * Sta nei DATI e non nel codice che costruisce l'intento: altrimenti chi assegnera' l'area a un eroe (E6)
	 * dovrebbe scrivere `if (ActionId == "Action.CircularAoE")`, cioe' l'eccezione hard-coded che il motore
	 * azioni esiste per togliere.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	bool bFriendlyFire = false;

	FRTActionDef() = default;
};
