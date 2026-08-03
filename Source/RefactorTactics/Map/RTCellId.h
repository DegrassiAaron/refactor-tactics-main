#pragma once

#include "CoreMinimal.h"
#include "RTCellId.generated.h"

/**
 * Le sei direzioni esagonali (pointy-top), in ordine STABILE 0..5 (l'ordine non dipende da TMap ne' da
 * indirizzi di memoria). Vettori assiali corrispondenti in URTHexLibrary::AxialDirection.
 */
UENUM(BlueprintType)
enum class ERTHexDirection : uint8
{
	E,   // (+1,  0)
	NE,  // (+1, -1)
	NW,  // ( 0, -1)
	W,   // (-1,  0)
	SW,  // (-1, +1)
	SE   // ( 0, +1)
};

/**
 * Coordinata di cella su griglia ESAGONALE (pointy-top), coordinate ASSIALI:
 *   X = q, Y = r; coordinata cubica derivata Z = -X - Y (invariante q+r+s=0).
 * Layer separa i piani (celle su layer diversi NON sono adiacenti: servono archi espliciti).
 * Interi: nessun float nelle coordinate/hash (invariante determinismo #4).
 */
USTRUCT(BlueprintType)
struct FRTCellId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 X = 0; // q (asse assiale)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Y = 0; // r (asse assiale)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Layer = 0;

	FRTCellId() = default;
	FRTCellId(int32 InX, int32 InY, int32 InLayer = 0) : X(InX), Y(InY), Layer(InLayer) {}

	/** Terza coordinata cubica: s = -q - r. */
	int32 CubeZ() const { return -X - Y; }

	bool operator==(const FRTCellId& O) const { return X == O.X && Y == O.Y && Layer == O.Layer; }
	bool operator!=(const FRTCellId& O) const { return !(*this == O); }

	/** Coerenza cubica (q + r + s == 0). Vera per costruzione: guardia contro manipolazioni errate. */
	bool IsValid() const { return X + Y + CubeZ() == 0; }

	FString ToString() const { return FString::Printf(TEXT("(q=%d,r=%d,L=%d)"), X, Y, Layer); }
};

FORCEINLINE uint32 GetTypeHash(const FRTCellId& C)
{
	return HashCombine(HashCombine(GetTypeHash(C.X), GetTypeHash(C.Y)), GetTypeHash(C.Layer));
}
