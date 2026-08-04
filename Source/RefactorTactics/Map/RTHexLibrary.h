#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTHexLibrary.generated.h"

/**
 * Matematica pura della griglia esagonale pointy-top (assiale/cubica). Deterministica: le coordinate restano
 * intere; il float compare solo nelle conversioni verso/da lo spazio-mondo (rendering/input), col risultato
 * assiale sempre arrotondato a intero (arrotondamento cubico). Nessuna dipendenza da Actor/NavMesh.
 */
UCLASS()
class REFACTORTACTICS_API URTHexLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Vettore assiale (dq,dr) della direzione esagonale (pointy-top), ordine stabile 0..5. */
	static FIntPoint AxialDirection(ERTHexDirection Dir);

	/** Cella adiacente nella direzione data (stesso layer). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FRTCellId Neighbor(const FRTCellId& Cell, ERTHexDirection Dir);

	/** I sei vicini orizzontali (stesso layer), in ordine di direzione E..SE. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static TArray<FRTCellId> Neighbors(const FRTCellId& Cell);

	/** Distanza esagonale (cubica) tra due celle. Ignora il Layer (i piani si collegano con archi espliciti). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static int32 HexDistance(const FRTCellId& A, const FRTCellId& B);

	/** Centro-mondo della cella (pointy-top): X,Y dal piano assiale, Z = Origin.Z + Layer*LayerHeight. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FVector AxialToWorld(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight);

	/** Cella che contiene il punto-mondo (arrotondamento cubico), sul Layer indicato (layer attivo). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FRTCellId WorldToAxial(const FVector& World, const FVector& Origin, float HexSize, int32 Layer);

	/** Layer (intero) corrispondente a una quota-mondo Z. LayerHeight<=0 -> 0. RoundToInt = floor(x+0.5). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static int32 WorldToLayer(double WorldZ, double OriginZ, float LayerHeight);

	/** Distanza minima tra la semi-retta (RayOrigin + t*RayDir, t>=0) e il segmento A..B. Pura, per hit-test archi. */
	static float DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B);

	/** Ordinamento stabile deterministico: Layer, poi X, poi Y. */
	static bool StableLess(const FRTCellId& A, const FRTCellId& B);

	/** Esagono PIENO di raggio N attorno al centro (tutte le celle a distanza <= N, stesso layer). 3N(N+1)+1 celle. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static TArray<FRTCellId> HexArea(const FRTCellId& Center, int32 Radius);
};
