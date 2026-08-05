#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "RTBotLibrary.generated.h"

/** Una mossa candidata del bot, valutabile dall'utility scoring (BU.1). */
USTRUCT()
struct FRTBotPlan
{
	GENERATED_BODY()

	/** Cella in cui il bot finisce (o resta). */
	UPROPERTY() FRTCellId DestCell;
	/** Se il piano include un attacco da DestCell. */
	UPROPERTY() bool bHasAttack = false;
	/** Danno dell'attacco pianificato. */
	UPROPERTY() int32 AttackDamage = 0;
	/** HP+scudo del bersaglio (per rilevare il kill). */
	UPROPERTY() int32 TargetHealth = 0;

	/** BU.3: indice dell'abilità d'attacco da DestCell (INDEX_NONE = nessun attacco). Puro (no Actor). */
	UPROPERTY() int32 AbilityIndex = INDEX_NONE;

	/** BU.3: indice del bersaglio nell'array di unità del chiamante (INDEX_NONE = nessuno). Puro (no Actor). */
	UPROPERTY() int32 TargetIndex = INDEX_NONE;

	/** BU.3c: DestCell è raggiunta con lo SCATTO (fase Dash, prima del Blast) anziché col movimento normale. */
	UPROPERTY() bool bViaDash = false;
};

/** Contesto puro per valutare una candidata (nessun Actor: testabile in automation). */
USTRUCT()
struct FRTBotContext
{
	GENERATED_BODY()

	/** Posizioni dei nemici vivi. */
	UPROPERTY() TArray<FRTCellId> Enemies;
	/** Gittata di ciascun nemico (parallelo a Enemies): usata per la minaccia sulla cella. */
	UPROPERTY() TArray<int32> EnemyRanges;

	/** Celle che bloccano la linea di tiro (copertura): un nemico non minaccia una cella se la LOS e' interrotta. */
	UPROPERTY() TArray<FRTCellId> VisionBlockers;
	/** >0 = kiter (mantiene la distanza di sicurezza); 0 = mischia (chiude la distanza). */
	UPROPERTY() int32 KiteStandoff = 0;

	/** Posizione attuale del bot: tie-break di ChooseBestPlan (a parità di score, mossa minima da qui). */
	UPROPERTY() FRTCellId Origin;

	// Pesi interi (bilanciabili senza toccare la logica; invariante #4: niente float). Default: il kill domina.
	UPROPERTY() int32 WKill = 10000;
	UPROPERTY() int32 WDamage = 10;
	UPROPERTY() int32 WThreat = 100;
	UPROPERTY() int32 WKiteViolation = 50;
	UPROPERTY() int32 WApproach = 10;
	/** Bonus per l'ELEVAZIONE della cella (Layer): premia la quota alta (tiro oltre coperture basse, +danno). */
	UPROPERTY() int32 WElevation = 20;
};

/** Decisioni di base del bot (logica pura, indipendente dagli Actor). */
UCLASS()
class REFACTORTACTICS_API URTBotLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Cella verso cui muoversi per avvicinarsi a Target restando entro MoveRange passi.
	 * Avvicinamento greedy (prima X poi Y); si ferma a distanza 1 dal bersaglio
	 * (non si sovrappone) e non si muove se e' gia' adiacente o sulla stessa cella.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static FRTCellId StepToward(const FRTCellId& From, const FRTCellId& Target, int32 MoveRange);

	/**
	 * Cella di avvicinamento a Target raggiungibile entro il budget di COSTO (CellCost: pathfinding
	 * pesato, hazard/impassabili esclusi), dentro la griglia: sceglie quella piu' vicina al bersaglio
	 * (a parita', la mossa minima). Aggira ostacoli e terreno costoso; non si sovrappone al bersaglio.
	 */
	static FRTCellId BestApproachCell(const FRTCellId& From, const FRTCellId& Target, int32 MoveRange,
		const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());

	/**
	 * Cella di fuga per il kiting: fra quelle raggiungibili entro il budget di costo (CellCost), dentro
	 * la griglia, sceglie quella che MASSIMIZZA la distanza dalla minaccia (a parita', la mossa minima).
	 * Aggira bordi, ostacoli e terreno pericoloso. Deterministica.
	 */
	static FRTCellId BestKiteCell(const FRTCellId& From, const FRTCellId& Threat, int32 MoveRange,
		const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());

	/**
	 * Priorità di un attacco per la scelta del bersaglio del bot.
	 * Un colpo che uccide (Damage >= TargetHealth) ha priorità massima, e tra i kill si preferisce
	 * il bersaglio più debole; tra i non-kill si preferisce il danno maggiore.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static int32 AttackScore(int32 Damage, int32 TargetHealth);

	/**
	 * Confronto deterministico fra due attacchi candidati: preferisce l'AttackScore maggiore; a parità, il
	 * bersaglio con coordinate (X,Y,Layer) minori (tie-break ASSOLUTO, invariante #4). Vero se A è migliore di B.
	 * Rende la scelta del bersaglio indipendente dall'ordine di iterazione delle unità.
	 */
	static bool AttackIsBetter(int32 DamageA, int32 TargetHealthA, const FRTCellId& TargetCellA,
		int32 DamageB, int32 TargetHealthB, const FRTCellId& TargetCellB);

	/**
	 * Utility score (intero) di una mossa candidata: focus-fire (danno + bonus se uccide) meno la minaccia
	 * subita nella cella di destinazione meno la penalità di posizionamento (kiter sotto standoff / mischia
	 * lontana). Pura e deterministica (interi, invariante #4). Il bot sceglie la candidata a punteggio massimo.
	 */
	static int32 ScorePlan(const FRTBotPlan& Plan, const FRTBotContext& Context);

	/**
	 * Sceglie la candidata a punteggio massimo (ScorePlan). TIE-BREAK ASSOLUTO: a parità di score
	 * preferisce la MOSSA MINIMA da Context.Origin (restare vince), poi le coordinate (X,Y,Layer)
	 * crescenti → permutare le candidate non cambia l'esito (invariante #4, determinismo).
	 * Con nessuna candidata ripiega sul "resta a Origin". Pura e deterministica.
	 */
	static FRTBotPlan ChooseBestPlan(const TArray<FRTBotPlan>& Candidates, const FRTBotContext& Context);

	/**
	 * Miglior POSIZIONE DI TIRO raggiungibile: fra le celle entro il budget di costo (grafo, incl.
	 * rampe/ponte), quelle da cui il bot ha linea di tiro (LOS) e gittata sul bersaglio. Preferisce
	 * l'ALTA QUOTA (elevazione: si spara oltre le coperture basse); a parità di quota, la mossa più corta.
	 * Se nessuna cella raggiungibile offre un tiro, ritorna From (il chiamante ripiega sull'avvicinamento).
	 * Non si sovrappone al bersaglio. Deterministica.
	 */
	static FRTCellId BestFiringCell(const FRTCellId& From, const FRTCellId& Target, int32 MoveRange,
		const TMap<FRTCellId, int32>& CellCost, int32 Width, int32 Height,
		const TArray<FRTCellId>& VisionBlockers, int32 AttackRange,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());
};
