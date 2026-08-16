#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTHeroData.generated.h"

class URTActionData;

/**
 * Definizione data-driven di un EROE del catalogo v0.1 (Gadget, Phase, Riktor, Wraith).
 *
 * Contiene solo cio' che il catalogo dichiara come **fisso** dell'eroe: identita', statistiche base e le
 * azioni fondamentali (attacco base compreso). Cio' che e' configurabile FRA eroi diversi (variante d'arma,
 * gadget, modulo di reazione) sta in `URTEquipmentData` e nel loadout; la variante di UNA abilita'
 * fondamentale invece e' dell'eroe stesso (`URTActionData::Variants`), perche' i suoi numeri dipendono
 * dall'abilita' che modifica — la stessa "Scarica ramificata" non avrebbe senso su un'abilita' di un altro
 * eroe.
 *
 * `Actions` ha ESATTAMENTE cinque elementi nel catalogo v0.1: indice 0 l'attacco base (fascia dalla portata,
 * `URTCatalogLibrary::MakeBasicAttack`), indici 1-4 le quattro abilita' fondamentali — di cui **una sola**
 * dichiara varianti (`URTHeroCatalogLibrary::ValidateHeroes` lo fa valere).
 *
 * Solo interi (invariante #4). Riferimento: docs/balance/RT_HeroCatalog_v0.1.md
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTHeroData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** ID stabile dell'eroe (es. `Hero.Gadget`). Chiave del data asset: non cambia mai. */
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

	/**
	 * Soglia d'udito ([D-041](../../../docs/decisions/RT_PDR_00_Decision_Log.md)): il rumore ricevuto va
	 * **almeno** a questo valore perche' l'eroe lo senta. Scala `0-10`, la stessa dell'intensita'
	 * (`Wait 0 · Sprint 5 · Dash 6 · esplosione 10`), e soglia **bassa = orecchio fine**.
	 *
	 * Quarta statistica, e **compensa** la vista invece di seguirla: Gadget 5 · Phase 3 · Riktor 3 · Wraith 5.
	 * Chi vede lontano sente meno. La terza via — udito allineato alla vista — e' stata scartata perche'
	 * raddoppiare lo stesso vantaggio su due canali renderebbe gli eroi da ricognizione *migliori*, non
	 * diversi.
	 *
	 * ⚠️ Il default e' volutamente `5` e non `0`: `0` significherebbe «sente qualunque cosa», cioe' il
	 * contrario di un valore neutro. Un eroe che si dimenticasse di dichiararla sentirebbe tutta la mappa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 HearingThreshold = 5;

	/** Celle di spinta assorbite prima di essere spostato (Riktor: 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 PushResistance = 0;

	/** Affinita' ambientale dichiarata (elettricita', acqua, strutture, movimento). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName Affinity;

	/**
	 * Debolezza dichiarata dell'eroe. Il PDF del catalogo la elenca fra gli elementi fissi ma non la esplicita
	 * per nessuno dei quattro eroi (docs/balance/RT_HeroCatalog_v0.1.md §5): **non si inventa**, va
	 * decisa esplicitamente per ciascun eroe. Un eroe senza debolezza non passa `ValidateHeroes`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName Weakness;

	/** Le azioni dell'eroe: attacco base (indice 0) + quattro abilita' fondamentali (indici 1-4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TArray<TObjectPtr<URTActionData>> Actions;

	/** ID primario stabile: `RTHero:<HeroId>`; senza HeroId ricade sul nome dell'asset. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		static const FPrimaryAssetType Type(TEXT("RTHero"));
		return FPrimaryAssetId(Type, HeroId.IsNone() ? GetFName() : HeroId);
	}
};
