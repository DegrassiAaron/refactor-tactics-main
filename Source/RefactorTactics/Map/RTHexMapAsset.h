#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "RTHexMapAsset.generated.h"

#if WITH_EDITOR
/** L'asset e' cambiato per una via che i chiamanti non controllano (undo/redo, editing dal suo editor). */
DECLARE_MULTICAST_DELEGATE(FRTHexMapAssetChanged);
#endif

/**
 * Classe di mappa (CP 19.1): per QUALE formato quella mappa e' stata disegnata.
 *
 * Non e' una difficolta' ne' un tema: e' la scala. Una mappa Skirmish ha le distanze del 2v2 e i suoi tempi di
 * attraversamento; metterci dentro un 3v3 Standard non e' «piu' difficile», e' un'altra partita.
 *
 * **La simulazione non ramifica su questo valore.** Nessun `if (MapClass == ...)` nel resolver: la classe serve
 * al VALIDATOR — che rifiuta l'accoppiata sbagliata prima dell'allestimento — e ai parametri che il formato
 * porta con se'. Il giorno in cui una regola dipendesse dalla classe, dipenderebbe da un parametro dichiarato
 * dal formato, non dall'enum.
 *
 * Aggiungere valori solo IN CODA: il valore viaggia serializzato nell'asset mappa.
 */
UENUM(BlueprintType)
enum class ERTMapClass : uint8
{
	/** Scala del 2v2: e' la classe del vertical slice v0.1. */
	Skirmish,
	/** Scala del 3v3 competitivo (v0.2, E24): baseline del formato Standard. */
	Standard,
	/** Scala delle mappe a obiettivi multipli e logistica (v0.4, E30). */
	Operations
};

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

	/**
	 * Versione corrente del formato dati.
	 * v2: le transizioni entrano nell'hash + campo `Kind` sugli archi.
	 * v3 (CP 9.1): coperture per bordo di cella (`FRTHexCellData::Covers`).
	 * v4 (CP 9.3): porte per bordo di cella (`FRTHexCellData::Doors`).
	 * v5 (CP 9.4): stato, integrita' e conduttivita' sugli archi (`FRTHexEdge`).
	 * v6 (CP 19.1): classe di mappa (`MapClass`). Nessun dato esistente cambia significato: una mappa scritta
	 *     prima e' `Skirmish`, che e' la classe del vertical slice, cioe' cio' che quelle mappe gia' erano.
	 * v7 (#619): sovrapprezzo di occupazione sulla cella (`FRTHexCellData::OccupancySurcharge`). Nessun dato
	 *     esistente cambia significato: una mappa scritta prima non ha geometria cotta, quindi il suo
	 *     sovrapprezzo e' zero — che e' il default del campo, ed e' cio' che quelle mappe gia' erano.
	 * v8 (#621): provenienza della copertura (`FRTHexCover::bGenerated`). Nessun dato esistente cambia
	 *     significato: una mappa scritta prima non ha coperture cotte, quindi ogni sua copertura e' dipinta
	 *     a mano — che e' il default `false`, ed e' cio' che quelle coperture gia' erano.
	 *     ⚠️ Nessuna migrazione TRASFORMATIVA: non si riclassificano coperture esistenti. Finche' #687 e'
	 *     aperta una trasformazione non verrebbe eseguita, e il difetto sarebbe silenzioso.
	 */
	static constexpr int32 CurrentFormatVersion = 8;

	/** Versione del formato con cui l'asset e' stato scritto; `MigrateToCurrentFormat` la porta avanti. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 FormatVersion = CurrentFormatVersion;

	/**
	 * Per quale formato questa mappa e' stata disegnata (CP 19.1).
	 *
	 * NON entra in `ComputeHash`: l'hash risponde alla domanda «la stessa geometria produce la stessa
	 * partita?», e la classe non tocca la geometria ne' il comportamento — cambiarla non cambia un solo esito
	 * di turno. Ci finisce indirettamente `FormatVersion`, com'e' gia' successo a v3, v4 e v5.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	ERTMapClass MapClass = ERTMapClass::Skirmish;

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
	 * Aggiunge o aggiorna PIU' celle incrementando la revisione UNA SOLA VOLTA. Serve a chi compie una modifica
	 * topologica che tocca piu' celle ma e' un evento solo — un portone largo tre bordi si apre una volta, non
	 * tre — cosi' che chi osserva la revisione non veda tre cambi dove ce n'e' stato uno.
	 * Un array vuoto non incrementa nulla.
	 */
	void UpdateCells(const TArray<FRTHexCellData>& InCells);

	/** Inizia una pennellata: Modify() una volta (stato pre-pennellata per l'undo). Nessuna transazione (la apre il caller). */
	void BeginStroke();

	/**
	 * Dipinge una cella dentro una pennellata: crea/aggiorna. Vero se applicata. Niente Sort/Dirty/refresh.
	 * `InBlocksLineOfSight` non passato preserva il flag della vista (comportamento storico); passato lo scrive.
	 */
	bool PaintCellInStroke(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement,
		TOptional<bool> InBlocksLineOfSight = TOptional<bool>());

	/** Cancella una cella dentro una pennellata. Vero se esisteva ed è stata rimossa. Niente Sort/Dirty/refresh. */
	bool EraseCellInStroke(const FRTCellId& Id);

	/** Chiude la pennellata: SortCells + MarkPackageDirty. */
	void EndStroke();

	/**
	 * Logica pura del 'pennello': se Existing != nullptr parte da esso (PRESERVA sempre Height), altrimenti da
	 * una cella nuova con l'Id dato; applica Surface/MoveCost/bBlocksMovement. Non muta l'asset.
	 *
	 * `InBlocksLineOfSight` **non passato** preserva `bBlocksLineOfSight` — il comportamento storico, pinnato da
	 * `RefactorTactics.HexMap.ApplyBrushMerge`, e il default. **Passato**, lo scrive.
	 *
	 * L'asimmetria con `bBlocksMovement` (che si scrive sempre) nasceva dal fatto che nessuno strumento
	 * dell'editor sapeva impostare il flag della vista: preservarlo era l'unico modo di non perderlo per
	 * sempre. Da quando il pennello lo espone, chi vuole scriverlo lo chiede.
	 */
	static FRTHexCellData ApplyBrush(const FRTHexCellData* Existing, const FRTCellId& Id,
		ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement,
		TOptional<bool> InBlocksLineOfSight = TOptional<bool>());

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
	 * Regione contigua (flood-fill) a partire da Start: visita i vicini dello STESSO layer che ESISTONO nell'asset
	 * e hanno la STESSA superficie di Start (frontiera a stack; l'ordine di visita non altera l'insieme risultante).
	 * Include Start. Se Start non esiste -> regione vuota. Pura/read-only/deterministica.
	 */
	TArray<FRTCellId> FloodRegion(const FRTCellId& Start) const;

	/**
	 * Aggiunge (o aggiorna, se From->To esiste gia') una transizione verticale/speciale; se bBidirectional aggiunge
	 * anche l'arco inverso To->From. Incrementa la revisione. Nessun arco duplicato per direzione.
	 */
	void AddTransition(const FRTCellId& From, const FRTCellId& To, int32 Cost = 1,
		ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair, bool bBidirectional = true);

	/** Rimuove la transizione From->To (e, se bBothDirections, anche To->From). Vero se ne ha rimossa almeno una. */
	bool RemoveTransition(const FRTCellId& From, const FRTCellId& To, bool bBothDirections = true);

	/**
	 * Sostituisce gli archi indicati (per coppia From->To) e incrementa la revisione UNA SOLA VOLTA: un ponte
	 * bidirezionale sono due archi ma un evento solo, come il portone di CP 9.3. Un array vuoto non incrementa.
	 */
	void UpdateTransitions(const TArray<FRTHexEdge>& InEdges);

	/** Ordina le celle in modo stabile (Layer, X, Y) e invalida la cache. */
	void SortCells();

	/** Hash deterministico del contenuto delle celle (indipendente dall'ordine di inserimento). */
	uint32 ComputeHash() const;

	/**
	 * Cella "centrale" della mappa: mediana del bounding box assiale delle celle del layer piu' basso.
	 * Serve a inquadrare la mappa (camera) senza assumere che sia centrata sull'origine — una mappa
	 * disegnata nell'editor puo' stare tutta altrove. Mappa vuota -> (0,0,0). Pura e deterministica.
	 */
	FRTCellId GetCenterCell() const;

	/** Validazione minimale: Id duplicati, costi negativi, transizioni verso celle inesistenti. Ritorna gli errori. */
	TArray<FString> ValidateMap() const;

	/**
	 * Porta l'asset alla versione corrente del formato. Idempotente (`PostLoad` puo' ripetersi) e conservativa:
	 * v2 -> v3 non converte nulla, perche' il campo `Covers` nasce vuoto — quello che deve dimostrare e' di
	 * NON toccare i dati esistenti.
	 */
	void MigrateToCurrentFormat();

	/** Migra l'asset appena caricato: un asset scritto con un formato vecchio non deve mai arrivare al gioco. */
	virtual void PostLoad() override;

	/**
	 * Invalida la cache Id->indice. Va chiamata quando Cells viene modificato SENZA passare dalle API di questa
	 * classe (tipicamente un undo/redo, che riscrive la property direttamente): la cache resterebbe altrimenti
	 * allineata allo stato precedente e FindCell leggerebbe l'indice sbagliato — o fuori dall'array, se le celle
	 * sono diminuite.
	 */
	void InvalidateLookup() const { bLookupDirty = true; }

#if WITH_EDITOR
	/** Notifica di cambiamento non mediato dalle API (undo/redo): chi mostra l'asset deve riallinearsi. */
	FRTHexMapAssetChanged OnMapChanged;

	/** Invalida la cache e notifica gli osservatori: dopo un undo i dati sono cambiati sotto i piedi a tutti. */
	virtual void PostEditUndo() override;
#endif

private:
	/** Cache Id->indice (transient, ricostruita pigramente). Non e' il formato autorevole. */
	mutable TMap<FRTCellId, int32> Lookup;
	mutable bool bLookupDirty = true;
	void EnsureLookup() const;
};
