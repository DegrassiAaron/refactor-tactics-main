#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Terrain/RTTerrainData.h"
#include "RTTerrainLibrary.generated.h"

/**
 * Lettura e validazione del catalogo terreni: pura, deterministica, senza Actor e senza asset.
 * Stesso schema di URTCatalogLibrary per le azioni (RT_TerrainCatalog_v0.1.md).
 */
UCLASS()
class REFACTORTACTICS_API URTTerrainLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Gli 8 terreni del catalogo v0.1 (RT_TerrainCatalog_v0.1.md §1). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static TArray<FRTTerrainDef> GetTerrainCatalog();

	/** Definizione del terreno indicato, o `FRTTerrainDef` di default (Floor) se assente dal catalogo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static FRTTerrainDef FindTerrainDef(ERTHexSurface Surface);

	/** Errori strutturali del catalogo SPEDITO (vuoto = valido): id duplicato, costo o limite di targeting negativi. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Terrain")
	static TArray<FString> ValidateTerrainCatalog();

	/** Stessa validazione di ValidateTerrainCatalog, ma su un catalogo ARBITRARIO (testabile con input rotto). */
	static TArray<FString> ValidateCatalogEntries(const TArray<FRTTerrainDef>& Catalog);
};
