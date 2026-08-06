#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTEquipmentData.generated.h"

/** Categoria di equipaggiamento: uno slot per categoria, per eroe. */
UENUM(BlueprintType)
enum class ERTEquipmentSlot : uint8
{
	WeaponVariant, // variante d'arma (modifica l'attacco base)
	Gadget,        // oggetto attivo con cooldown
	ReactionModule // reazione con trigger dichiarato
};

/**
 * Definizione data-driven di un equipaggiamento del catalogo v0.1.
 *
 * Regola di progetto, verificata dal validator: ogni equipaggiamento dichiara **almeno uno svantaggio**.
 * La scelta e' orizzontale — cambia COME si gioca, non QUANTO si e' forti — ed e' il pilastro «nessuna potenza
 * permanente pay-to-win» del canone. Il catalogo elenca «rendere un equipaggiamento migliore in ogni
 * parametro» fra gli errori da evitare.
 *
 * Riferimento: docs/design/balance/RT_EquipmentCatalog_v0.1.md
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTEquipmentData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** ID stabile (es. `Weapon.Precision`, `Gadget.Medkit`, `Reaction.CounterShot`). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName EquipmentId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	ERTEquipmentSlot Slot = ERTEquipmentSlot::Gadget;

	/** Cosa migliora (testo leggibile: la regola numerica vive nell'azione che modifica). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText Advantage;

	/** Cosa peggiora. **Obbligatorio**: senza, l'equipaggiamento e' una scelta verticale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText Drawback;

	/** Ricarica in turni completi (gadget del catalogo: 3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 CooldownTurns = 0;

	/** ID primario stabile: `RTEquipment:<EquipmentId>`; senza id ricade sul nome dell'asset. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		static const FPrimaryAssetType Type(TEXT("RTEquipment"));
		return FPrimaryAssetId(Type, EquipmentId.IsNone() ? GetFName() : EquipmentId);
	}
};
