#pragma once

#include "CoreMinimal.h"
#include "RTTypes.generated.h"

/**
 * Coordinata logica di una cella nella griglia tattica.
 * E' la posizione autorevole (il FVector serve solo al rendering).
 * Il campo Layer e' riservato alla mappa multilivello (north-star) e per ora resta implicito a 0.
 */
USTRUCT(BlueprintType)
struct FRTGridCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
	int32 Y = 0;

	/** Livello/piano nella mappa multilivello (north-star). Default 0 = griglia 2D classica. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
	int32 Layer = 0;

	FRTGridCoord() = default;
	FRTGridCoord(int32 InX, int32 InY) : X(InX), Y(InY) {}
	FRTGridCoord(int32 InX, int32 InY, int32 InLayer) : X(InX), Y(InY), Layer(InLayer) {}

	bool operator==(const FRTGridCoord& Other) const { return X == Other.X && Y == Other.Y && Layer == Other.Layer; }
	bool operator!=(const FRTGridCoord& Other) const { return !(*this == Other); }
};

FORCEINLINE uint32 GetTypeHash(const FRTGridCoord& Coord)
{
	return HashCombine(HashCombine(GetTypeHash(Coord.X), GetTypeHash(Coord.Y)), GetTypeHash(Coord.Layer));
}
