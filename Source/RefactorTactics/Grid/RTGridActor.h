#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTGridActor.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Rappresentazione visuale della griglia: una mesh istanziata per cella,
 * posizionata usando URTGridLibrary (la logica resta separata dal rendering).
 */
UCLASS()
class REFACTORTACTICS_API ARTGridActor : public AActor
{
	GENERATED_BODY()

public:
	ARTGridActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	int32 Width = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	int32 Height = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	float CellSize = 200.f;

protected:
	virtual void BeginPlay() override;

	/** Ricostruisce tutte le istanze delle celle. */
	void BuildGrid();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Grid")
	TObjectPtr<UInstancedStaticMeshComponent> Cells;
};
