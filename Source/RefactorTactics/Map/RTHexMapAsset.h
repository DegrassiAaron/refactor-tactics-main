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

	/** Versione del formato dati (per migrazioni future). v2: le transizioni entrano nell'hash + campo Kind sugli archi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 FormatVersion = 2;

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

	/**
	 * Logica pura del 'pennello': se Existing != nullptr parte da esso (PRESERVA Height e bBlocksLineOfSight),
	 * altrimenti da una cella nuova con l'Id dato; applica Surface/MoveCost/bBlocksMovement. Non muta l'asset.
	 */
	static FRTHexCellData ApplyBrush(const FRTHexCellData* Existing, const FRTCellId& Id,
		ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement);

	/** Rimuove la cella con l'Id dato; vero se esisteva. */
	bool RemoveCell(const FRTCellId& Id);

	/** Puntatore alla cella con l'Id dato, o nullptr se assente. */
	const FRTHexCellData* FindCell(const FRTCellId& Id) const;

	/** Vero se la mappa contiene la cella. */
	bool ContainsCell(const FRTCellId& Id) const;

	int32 NumCells() const { return Cells.Num(); }

	/** Layer distinti presenti nelle celle, ordinati in modo crescente. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HexMap")
	TArray<int32> GetLayers() const;

	/** Id delle celle appartenenti al Layer indicato, in ordine stabile. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HexMap")
	TArray<FRTCellId> CellsInLayer(int32 Layer) const;

	/**
	 * Aggiunge (o aggiorna, se From->To esiste gia') una transizione verticale/speciale; se bBidirectional aggiunge
	 * anche l'arco inverso To->From. Incrementa la revisione. Nessun arco duplicato per direzione.
	 */
	void AddTransition(const FRTCellId& From, const FRTCellId& To, int32 Cost = 1,
		ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair, bool bBidirectional = true);

	/** Rimuove la transizione From->To (e, se bBothDirections, anche To->From). Vero se ne ha rimossa almeno una. */
	bool RemoveTransition(const FRTCellId& From, const FRTCellId& To, bool bBothDirections = true);

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
