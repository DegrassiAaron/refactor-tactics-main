#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Turn/RTTurnLog.h"
#include "RTHexSim.generated.h"

class URTHexMapAsset;

/**
 * Stato minimo di un'unita' per la simulazione esagonale di un turno. L'identita' e' un INTERO STABILE
 * (UnitId), mai un pointer: e' la stessa disciplina del TurnLog (determinismo/replay).
 */
USTRUCT(BlueprintType)
struct FRTHexSimUnit
{
	GENERATED_BODY()

	/** Identita' stabile dell'unita' nel turno (chiave di occupazione e di risultato). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 UnitId = 0;

	/** Posizione autorevole a inizio fase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	FRTCellId Cell;

	/** Le unita' non vive non occupano celle e non partecipano al movimento. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	bool bAlive = true;

	/** Costo massimo (intero) spendibile nel turno per il movimento; 0 = immobile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 MoveBudget = 0;

	/**
	 * Sovrapprezzo (intero, >= 0) aggiunto al costo di OGNI cella attraversata, per QUESTA unita' (`Action.Slow`,
	 * CP 4.7): 0 = nessun sovrapprezzo. E' un costo di pathfinding, non una riduzione del budget totale — la
	 * differenza conta perche' rende piu' cara la strada lunga senza rendere impossibile quella corta,
	 * mentre dimezzare il budget penalizza allo stesso modo un passo e dieci.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 MoveCostModifier = 0;

	FRTHexSimUnit() = default;
	FRTHexSimUnit(int32 InUnitId, const FRTCellId& InCell, int32 InMoveBudget = 0, bool bInAlive = true)
		: UnitId(InUnitId), Cell(InCell), bAlive(bInAlive), MoveBudget(InMoveBudget) {}
};

/** Cella raggiungibile entro il budget, col costo cumulato dalla partenza. */
USTRUCT(BlueprintType)
struct FRTHexReachableCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	int32 Cost = 0;

	FRTHexReachableCell() = default;
	FRTHexReachableCell(const FRTCellId& InCell, int32 InCost) : Cell(InCell), Cost(InCost) {}
};

/** Esito del movimento simultaneo di un'unita': cella finale, celle attraversate (partenza esclusa), reason code. */
USTRUCT(BlueprintType)
struct FRTHexMoveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	FRTCellId Final;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	TArray<FRTCellId> Entered;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	ERTMoveOutcome Outcome = ERTMoveOutcome::Stayed;
};

/**
 * Stato CONGELATO a inizio fase per la risoluzione su griglia esagonale ("raccogli poi applica", invariante #3).
 *
 * NON e' una USTRUCT e NON va conservata oltre la fase che la produce: contiene un puntatore non-UPROPERTY
 * all'asset mappa (nessuna protezione dal GC). L'occupazione e' COPIATA (cambia durante la risoluzione);
 * la mappa e' solo referenziata insieme a hash/revisione, perche' e' immutabile per tutto il turno —
 * URTHexSimLibrary::IsSnapshotStale rileva la violazione di questa assunzione invece di ignorarla.
 */
struct FRTHexSnapshot
{
	/** Mappa autorevole (non posseduta): valida solo per la durata della fase. */
	const URTHexMapAsset* Map = nullptr;

	/** Hash del contenuto della mappa al momento della cattura. */
	uint32 MapHash = 0;

	/** Revisione della mappa al momento della cattura. */
	int32 Revision = 0;

	/** Unita' del turno, ordinate per UnitId (ordine stabile). */
	TArray<FRTHexSimUnit> Units;

	/** Cella -> UnitId dell'occupante (solo unita' vive). */
	TMap<FRTCellId, int32> Occupancy;
};
