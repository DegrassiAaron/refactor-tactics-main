#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/RTTypes.h"
#include "Selection/RTSelectable.h"
#include "RTUnit.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class URTAbilityData;

/**
 * Unita' segnaposto per il demo: una mesh su una cella, colorata per team e selezionabile.
 * E' un marker minimale (niente statistiche/abilita' qui): quelle arrivano in M2/M3.
 */
UCLASS()
class REFACTORTACTICS_API ARTUnit : public AActor, public IRTSelectable
{
	GENERATED_BODY()

public:
	ARTUnit();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 TeamId = 0;

	/** Numero massimo di celle percorribili in un turno (distanza di Manhattan). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	int32 MoveRange = 4;

	/** Se vero, l'unita' e' pianificata automaticamente dal bot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Unit")
	bool bIsBotControlled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FRTGridCoord GridCell;

	/** Cella di destinazione pianificata per il turno corrente (default = cella attuale). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Unit")
	FRTGridCoord PlannedCell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 MaxHealth = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Health = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	/** Portata dell'attacco base, in celle (distanza di Manhattan). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 AttackRange = 5;

	/** Danno dell'attacco base. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 AttackPower = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 MaxEnergy = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Energy = 0;

	/** Energia guadagnata a ogni turno. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 EnergyPerTurn = 25;

	/** Energia guadagnata quando si porta a segno un attacco (non ultimate). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 EnergyOnHit = 15;

	/** Moltiplicatore di danno dell'ultimate (attacco a energia piena). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 UltimateMultiplier = 2;

	/** Raggio dell'area colpita dall'ultimate attorno al bersaglio (0 = singolo bersaglio). */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Combat")
	int32 UltimateRadius = 1;

	/** Abilita' d'attacco data-driven (opzionale): se assegnata, sovrascrive i valori base. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	TObjectPtr<URTAbilityData> BasicAttackAbility = nullptr;

	/** Abilita' ultimate data-driven (opzionale): se assegnata, sovrascrive i valori base dell'ultimate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Ability")
	TObjectPtr<URTAbilityData> UltimateAbility = nullptr;

	// Valori effettivi: dall'abilita' data-driven se assegnata, altrimenti dai campi base.
	int32 GetAttackRange() const;
	int32 GetAttackPower() const;
	int32 GetUltimatePower() const;
	int32 GetUltimateRadius() const;
	FGameplayTag GetUltimateStatusTag() const;
	int32 GetUltimateStatusDuration() const;

	/** Bersaglio dell'attacco pianificato per il turno (nullo = nessun attacco). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Combat")
	TObjectPtr<ARTUnit> PlannedAttackTarget = nullptr;

	/** Applica lo stato di combattimento risolto; se HP<=0 avvia l'eliminazione. */
	void ApplyCombatState(int32 NewHealth, int32 NewShield);

	bool IsAlive() const { return Health > 0; }

	/** Applica uno status per Turns turni (non accorcia una durata gia' piu' lunga). */
	void ApplyStatus(FGameplayTag Tag, int32 Turns);

	/** Vero se lo status e' attivo (durata residua > 0). */
	bool HasStatus(FGameplayTag Tag) const;

	/** Decrementa la durata di tutti gli status; rimuove quelli scaduti. */
	void TickStatuses();

	/** Range di movimento tenendo conto degli status (Root/Slow). */
	int32 GetEffectiveMoveRange() const;

private:
	/** Status attivi: tag -> turni residui. */
	UPROPERTY()
	TMap<FGameplayTag, int32> StatusTurns;

public:

	/** Posiziona l'unita' al centro-mondo della cella, con la base appoggiata al piano. */
	void PlaceOnCell(const FRTGridCoord& Cell, const FVector& GridOrigin, float CellSize);

	// IRTSelectable
	virtual void OnSelected() override;
	virtual void OnDeselected() override;

protected:
	virtual void BeginPlay() override;

	void ApplyTeamColor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynMaterial;

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FLinearColor Team0Color = FLinearColor(0.10f, 0.40f, 1.00f);

	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FLinearColor Team1Color = FLinearColor(1.00f, 0.20f, 0.15f);

	/** Scala base della mesh; l'evidenziazione di selezione la moltiplica, non la sostituisce. */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	FVector BaseMeshScale = FVector(1.2f, 1.2f, 1.8f);

	/**
	 * Materiale con un parametro vettoriale "Color" usato per il colore-team.
	 * Default: /Game/Materials/M_Unit (da creare nell'editor). Se assente, l'unita' resta grigia.
	 */
	UPROPERTY(EditAnywhere, Category = "RefactorTactics|Unit")
	TSoftObjectPtr<UMaterialInterface> UnitMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Unit.M_Unit")));
};
