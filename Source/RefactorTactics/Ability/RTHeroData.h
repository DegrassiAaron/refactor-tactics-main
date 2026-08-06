#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTHeroData.generated.h"

class URTActionData;

/**
 * Definizione data-driven di un EROE del catalogo v0.1 (Flux, Riva, Bastion, Vektor).
 *
 * Contiene solo cio' che il catalogo dichiara come **fisso** dell'eroe: identita', statistiche base e le
 * quattro azioni fondamentali. Cio' che e' configurabile (variante d'arma, gadget, modulo di reazione,
 * variante d'abilita') sta in `URTEquipmentData` e nel loadout, non qui.
 *
 * Solo interi (invariante #4). Riferimento: docs/design/balance/RT_HeroCatalog_v0.1.md
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTHeroData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** ID stabile dell'eroe (es. `Hero.Flux`). Chiave del data asset: non cambia mai. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName HeroId;

	/** Nome mostrato (UI/log). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 MaxHealth = 100;

	/** Punti movimento per turno (budget standard del catalogo: 5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 MovePoints = 5;

	/** Portata visiva in celle esagonali. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 VisionRange = 5;

	/** Celle di spinta assorbite prima di essere spostato (Bastion: 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 PushResistance = 0;

	/** Affinita' ambientale dichiarata (elettricita', acqua, strutture, movimento). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName Affinity;

	/** Le azioni fondamentali dell'eroe (quattro nel catalogo v0.1, attacco base compreso). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TArray<TObjectPtr<URTActionData>> Actions;

	/** ID primario stabile: `RTHero:<HeroId>`; senza HeroId ricade sul nome dell'asset. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		static const FPrimaryAssetType Type(TEXT("RTHero"));
		return FPrimaryAssetId(Type, HeroId.IsNone() ? GetFName() : HeroId);
	}
};
