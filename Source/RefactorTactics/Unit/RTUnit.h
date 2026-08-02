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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Unit")
	FRTGridCoord GridCell;

	/** Cella di destinazione pianificata per il turno corrente (default = cella attuale). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Unit")
	FRTGridCoord PlannedCell;

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
