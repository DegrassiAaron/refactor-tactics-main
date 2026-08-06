#pragma once

#include "CoreMinimal.h"
#include "Map/RTHexCellData.h"
#include "Turn/RTActionEvent.h"
#include "RTTerrainData.generated.h"

/**
 * Definizione dichiarativa di un terreno del catalogo (RT_TerrainCatalog_v0.1.md §1): costo di movimento,
 * blocchi, conducibilita' elettrica, limite di targeting ed effetti applicati all'ingresso. Vive nel
 * catalogo letterale di URTTerrainLibrary::GetTerrainCatalog, non in un asset ne' in uno switch C++.
 */
USTRUCT(BlueprintType)
struct FRTTerrainDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	ERTHexSurface Surface = ERTHexSurface::Floor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 MoveCost = 1;

	/** Vieta Dash/Charge (mobilita' rapida) attraverso la cella; il movimento normale non e' affetto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bBlocksDashCharge = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bBlocksLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bConductsElectricity = false;

	/** 0 = nessun limite; N > 0 = la portata effettiva di un intento la cui linea attraversa questa cella e' min(RangeCells, N). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 MaxTargetingRangeThrough = 0;

	/** Effetti applicati a un'unita' quando entra nella cella (Damage/Status; riusa il vocabolario delle azioni). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	TArray<FRTActionEffectSpec> OnEnterEffects;

	FRTTerrainDef() = default;
};
