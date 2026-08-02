#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RTTerrainTypes.generated.h"

/** Costo sentinella per una cella non attraversabile (usato dal pathfinding pesato). */
#define RT_BLOCKED_COST (-1)

/**
 * Proprieta' di gioco di un tipo di terreno, in forma "plain" (nessun UObject).
 * Le funzioni pure (derivazione, pathfinding, resolver) lavorano su questo, cosi' restano testabili
 * senza asset ne' Actor. L'asset autorevole e' URTTerrainData, che incapsula un FRTTerrainProps.
 */
USTRUCT(BlueprintType)
struct FRTTerrainProps
{
	GENERATED_BODY()

	/** Costo aggiuntivo di attraversamento (il costo cella = 1 + ExtraMoveCost; additivo intero — R3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 ExtraMoveCost = 0;

	/** La cella non e' attraversabile (copertura/muro). Prevale su ExtraMoveCost. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bBlocksMovement = false;

	/** La cella blocca la linea di tiro (indipendente dal movimento — es. cespuglio). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bBlocksVision = false;

	/** Danno per ogni cella pericolosa attraversata (fase Move — v2). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 CrossDamage = 0;

	/** Danno a chi termina il turno sulla cella (fase Cleanup). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 EndTurnDamage = 0;

	/** Status applicato a chi occupa la cella a fine turno (nessuno se il tag e' vuoto). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	FGameplayTag StatusOnEnter;

	/** Durata in turni dello status. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 StatusDuration = 0;

	/** Bonus al danno inflitto da chi occupa la cella (buff, es. altura). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 OccupantDamageBonus = 0;

	/** Bonus alla portata di chi occupa la cella. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 OccupantRangeBonus = 0;

	/** Riduzione danno per chi occupa la cella. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 OccupantDefenseBonus = 0;

	/** La cella puo' essere incendiata da un'abilita' (terreno dinamico). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bFlammable = false;

	/** Se > 0, terreno temporaneo: torna al terreno base dopo N turni (es. Fuoco). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 TransientDuration = 0;
};
