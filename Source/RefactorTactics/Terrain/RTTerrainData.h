#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Terrain/RTTerrainTypes.h"
#include "RTTerrainData.generated.h"

/**
 * Definizione data-driven di un tipo di terreno (Primary Data Asset).
 * Incapsula le proprieta' di gioco (FRTTerrainProps) piu' presentazione e i riferimenti per il
 * terreno dinamico (ignite/reversione). Nuovi terreni = nuovi asset, senza toccare il C++.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTTerrainData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Nome mostrato (UI/log). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	FText DisplayName;

	/** Colore della cella (rendering). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	FLinearColor DisplayColor = FLinearColor::White;

	/** Proprieta' di gioco (usate dalle funzioni pure). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	FRTTerrainProps Props;

	/** Terreno risultante se questa cella viene incendiata (nullo = non infiammabile in pratica). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	TObjectPtr<URTTerrainData> IgnitesTo = nullptr;

	/** Terreno a cui tornare al termine di TransientDuration (es. Erba bruciata). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	TObjectPtr<URTTerrainData> RevertsTo = nullptr;

	const FRTTerrainProps& GetProps() const { return Props; }
};
