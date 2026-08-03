#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "RTHexMapAsset.generated.h"

/**
 * Asset AUTOREVOLE e serializzato di una mappa esagonale (formato dati, non decorazione visiva).
 * Le celle sono conservate in un array con ORDINE STABILE (Layer, X, Y); una cache Id->indice velocizza l'accesso
 * runtime senza essere il formato autorevole. Nessun Actor per cella. Coerente con gli invarianti (determinismo).
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTHexMapAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Identita' stabile della mappa (per hash/replay/asset reference). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	FGuid MapId;

	/** Versione del formato dati (per migrazioni future). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 FormatVersion = 1;

	/** Dimensione dell'esagono (cm), usata per axial<->world. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	float HexSize = 100.f;

	/** Quota (cm) tra un layer e il successivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	float LayerHeight = 250.f;

	/** Revisione: incrementata a ogni modifica strutturale (invalidazione cache/path). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 Revision = 0;

	/** Celle serializzate, in ordine stabile (Layer, X, Y). Formato autorevole (non una sola TMap). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<FRTHexCellData> Cells;

	/** Transizioni esplicite (archi verticali/speciali): scale, rampe, ponti, tunnel, ascensori. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<FRTHexEdge> Transitions;

	/** Aggiunge o aggiorna (per Id) una cella; incrementa la revisione. */
	void AddOrUpdateCell(const FRTHexCellData& Cell);

	/** Rimuove la cella con l'Id dato; vero se esisteva. */
	bool RemoveCell(const FRTCellId& Id);

	/** Puntatore alla cella con l'Id dato, o nullptr se assente. */
	const FRTHexCellData* FindCell(const FRTCellId& Id) const;

	/** Vero se la mappa contiene la cella. */
	bool ContainsCell(const FRTCellId& Id) const;

	int32 NumCells() const { return Cells.Num(); }

	/** Ordina le celle in modo stabile (Layer, X, Y) e invalida la cache. */
	void SortCells();

	/** Hash deterministico del contenuto delle celle (indipendente dall'ordine di inserimento). */
	uint32 ComputeHash() const;

	/** Validazione minimale: Id duplicati, costi negativi, transizioni verso celle inesistenti. Ritorna gli errori. */
	TArray<FString> ValidateMap() const;

private:
	/** Cache Id->indice (transient, ricostruita pigramente). Non e' il formato autorevole. */
	mutable TMap<FRTCellId, int32> Lookup;
	mutable bool bLookupDirty = true;
	void EnsureLookup() const;
};
