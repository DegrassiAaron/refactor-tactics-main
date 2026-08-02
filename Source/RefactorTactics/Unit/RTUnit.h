#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/RTTypes.h"
#include "Selection/RTSelectable.h"
#include "RTUnit.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

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

	/** Bersaglio dell'attacco pianificato per il turno (nullo = nessun attacco). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Combat")
	TObjectPtr<ARTUnit> PlannedAttackTarget = nullptr;

	/** Applica lo stato di combattimento risolto; se HP<=0 avvia l'eliminazione. */
	void ApplyCombatState(int32 NewHealth, int32 NewShield);

	bool IsAlive() const { return Health > 0; }

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
};
