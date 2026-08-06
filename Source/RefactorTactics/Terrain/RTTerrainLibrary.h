#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Terrain/RTTerrainData.h"
#include "RTTerrainLibrary.generated.h"

class URTHexMapAsset;

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

	/**
	 * Portata di targeting EFFETTIVA fra due celle: il minimo fra `RangeCells` e il `MaxTargetingRangeThrough`
	 * dei terreni incontrati sulla linea di tiro (oggi solo il Fumo, cap 2).
	 *
	 * **Un solo posto** per questa regola: chi decide "il bersaglio e' a portata" DEVE passare di qui, altrimenti
	 * accetta un intento che il resolver poi scarta — l'azione spende lo slot e non succede nulla, senza una riga
	 * di log che lo spieghi.
	 *
	 * Estremi INCLUSI (cella dell'attaccante e del bersaglio): il DoD canonico dice «targeting max 2 celle
	 * **dentro o attraverso**», quindi stare NEL fumo cappa la propria portata quanto sparare attraverso.
	 * Diverge di proposito dalla convenzione di `URTHexVisionLibrary::HasLineOfSight`, che gli estremi li esclude.
	 *
	 * `Map == nullptr` -> `RangeCells` invariato: senza mappa non c'e' terreno da leggere, e il fail-closed lo
	 * decide il chiamante (che ha il proprio esito da registrare), non questa funzione.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static int32 EffectiveTargetingRange(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		int32 RangeCells);

	/** Errori strutturali del catalogo SPEDITO (vuoto = valido): id duplicato, costo o limite di targeting negativi. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Terrain")
	static TArray<FString> ValidateTerrainCatalog();

	/** Stessa validazione di ValidateTerrainCatalog, ma su un catalogo ARBITRARIO (testabile con input rotto). */
	static TArray<FString> ValidateCatalogEntries(const TArray<FRTTerrainDef>& Catalog);
};
