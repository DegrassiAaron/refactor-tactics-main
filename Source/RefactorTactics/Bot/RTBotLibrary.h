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
	UPROPERTY() FRTGridCoord DestCell;
	/** Se il piano include un attacco da DestCell. */
	UPROPERTY() bool bHasAttack = false;
	/** Danno dell'attacco pianificato. */
	UPROPERTY() int32 AttackDamage = 0;
	/** HP+scudo del bersaglio (per rilevare il kill). */
	UPROPERTY() int32 TargetHealth = 0;
};

/** Contesto puro per valutare una candidata (nessun Actor: testabile in automation). */
USTRUCT()
struct FRTBotContext
{
	GENERATED_BODY()

	/** Posizioni dei nemici vivi. */
	UPROPERTY() TArray<FRTGridCoord> Enemies;
	/** Gittata di ciascun nemico (parallelo a Enemies): usata per la minaccia sulla cella. */
	UPROPERTY() TArray<int32> EnemyRanges;
	/** >0 = kiter (mantiene la distanza di sicurezza); 0 = mischia (chiude la distanza). */
	UPROPERTY() int32 KiteStandoff = 0;

	// Pesi interi (bilanciabili senza toccare la logica; invariante #4: niente float). Default: il kill domina.
	UPROPERTY() int32 WKill = 10000;
	UPROPERTY() int32 WDamage = 10;
	UPROPERTY() int32 WThreat = 100;
	UPROPERTY() int32 WKiteViolation = 50;
	UPROPERTY() int32 WApproach = 10;
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
	static FRTGridCoord StepToward(const FRTGridCoord& From, const FRTGridCoord& Target, int32 MoveRange);

	/**
	 * Cella di avvicinamento a Target raggiungibile entro il budget di COSTO (CellCost: pathfinding
	 * pesato, hazard/impassabili esclusi), dentro la griglia: sceglie quella piu' vicina al bersaglio
	 * (a parita', la mossa minima). Aggira ostacoli e terreno costoso; non si sovrappone al bersaglio.
	 */
	static FRTGridCoord BestApproachCell(const FRTGridCoord& From, const FRTGridCoord& Target, int32 MoveRange,
		const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());

	/**
	 * Cella di fuga per il kiting: fra quelle raggiungibili entro il budget di costo (CellCost), dentro
	 * la griglia, sceglie quella che MASSIMIZZA la distanza dalla minaccia (a parita', la mossa minima).
	 * Aggira bordi, ostacoli e terreno pericoloso. Deterministica.
	 */
	static FRTGridCoord BestKiteCell(const FRTGridCoord& From, const FRTGridCoord& Threat, int32 MoveRange,
		const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());

	/**
	 * Priorità di un attacco per la scelta del bersaglio del bot.
	 * Un colpo che uccide (Damage >= TargetHealth) ha priorità massima, e tra i kill si preferisce
	 * il bersaglio più debole; tra i non-kill si preferisce il danno maggiore.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Bot")
	static int32 AttackScore(int32 Damage, int32 TargetHealth);

	/**
	 * Utility score (intero) di una mossa candidata: focus-fire (danno + bonus se uccide) meno la minaccia
	 * subita nella cella di destinazione meno la penalità di posizionamento (kiter sotto standoff / mischia
	 * lontana). Pura e deterministica (interi, invariante #4). Il bot sceglie la candidata a punteggio massimo.
	 */
	static int32 ScorePlan(const FRTBotPlan& Plan, const FRTBotContext& Context);

	/**
	 * Miglior POSIZIONE DI TIRO raggiungibile: fra le celle entro il budget di costo (grafo, incl.
	 * rampe/ponte), quelle da cui il bot ha linea di tiro (LOS) e gittata sul bersaglio. Preferisce
	 * l'ALTA QUOTA (elevazione: si spara oltre le coperture basse); a parità di quota, la mossa più corta.
	 * Se nessuna cella raggiungibile offre un tiro, ritorna From (il chiamante ripiega sull'avvicinamento).
	 * Non si sovrappone al bersaglio. Deterministica.
	 */
	static FRTGridCoord BestFiringCell(const FRTGridCoord& From, const FRTGridCoord& Target, int32 MoveRange,
		const TMap<FRTGridCoord, int32>& CellCost, int32 Width, int32 Height,
		const TArray<FRTGridCoord>& VisionBlockers, int32 AttackRange,
		const TArray<FRTTraversalEdge>& Edges = TArray<FRTTraversalEdge>());
};
