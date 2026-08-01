#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RTTypes.h"
#include "RTGridLibrary.generated.h"

/**
 * Utility pure per la griglia logica, indipendenti dal rendering e dal mondo.
 * Le conversioni prendono origine e dimensione cella espliciti cosi' restano testabili senza un Actor.
 */
UCLASS()
class REFACTORTACTICS_API URTGridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Centro-mondo (X,Y) della cella; Z = Origin.Z. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static FVector CellToWorld(const FRTGridCoord& Cell, const FVector& Origin, float CellSize);

	/** Cella che contiene la posizione mondo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static FRTGridCoord WorldToCell(const FVector& World, const FVector& Origin, float CellSize);

	/** Vero se la cella e' dentro una griglia Width x Height con origine logica (0,0). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static bool IsInsideGrid(const FRTGridCoord& Cell, int32 Width, int32 Height);

	/** Distanza di Manhattan tra due celle. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Grid")
	static int32 ManhattanDistance(const FRTGridCoord& A, const FRTGridCoord& B);
};
