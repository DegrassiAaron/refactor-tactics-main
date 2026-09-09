#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTOverlayArea.h"
#include "RTRegionBoundary.generated.h"

/**
 * Un lato di **perimetro**: la cella che sta DENTRO la regione, e la direzione che guarda fuori.
 *
 * 🔑 **Si nomina dal lato interno, e non e' un dettaglio.** Lo stesso bordo fisico ha due nomi — `{A, E}` e
 * `{vicino a est di A, W}` — e `URTHexLibrary::EdgeMidpointWorld` garantisce che portino allo stesso punto
 * nel mondo. Per un perimetro la scelta e' obbligata: il vicino **non appartiene alla regione**, quindi
 * l'unico dei due nomi che esiste sempre e' quello della cella interna. Nominarlo dal fuori richiederebbe una
 * cella che sul bordo della mappa non c'e'.
 *
 * ⛔ **Non porta geometria.** Dove sia questo lato nel mondo lo sa gia' `EdgeMidpointWorld`, e ricalcolarlo
 * qui sarebbe il secondo modello spaziale che #1941 e #1942 vietano entrambe.
 */
USTRUCT(BlueprintType)
struct FRTBoundaryEdge
{
	GENERATED_BODY()

	/** La cella **interna** alla regione. Porta lei il `Layer` del perimetro. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Boundary")
	FRTCellId Cell;

	/** Il lato che guarda **fuori** dalla regione. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Boundary")
	ERTHexDirection Dir = ERTHexDirection::E;

	FRTBoundaryEdge() = default;
	FRTBoundaryEdge(const FRTCellId& InCell, ERTHexDirection InDir) : Cell(InCell), Dir(InDir) {}

	bool operator==(const FRTBoundaryEdge& Other) const
	{
		return Cell == Other.Cell && Dir == Other.Dir;
	}
};

/**
 * Il **perimetro** di una regione semantica (OVL-02, #1942).
 *
 * 🔴 **Il difetto che chiude non e' estetico, e' di conteggio.** Oggi un'area si legge cella per cella:
 * `DrawPlanningPreview` disegna un contorno esagonale su ognuna delle raggiungibili, e con venti celle si
 * vedono venti esagoni, non una zona con un bordo. Chi deve decidere «arrivo o non arrivo» legge il
 * **perimetro**, e venti contorni annidati sono venti oggetti da integrare a occhio.
 *
 * ⛔ **Questa e' la meta' PURA della issue, ed e' apposta.** La DoD chiede che l'estrazione si provi con un
 * test *«headless — funzione pura, senza PIE»*. La ribbon a schermo, e in particolare la scelta sul **depth
 * test**, la issue le dichiara non risolvibili a tavolino: *«si decide guardando, e la voce PIE di questa
 * issue e' l'oracolo»*. Qui non c'e' un pixel, e nessuna di quelle due e' stata anticipata.
 *
 * ## Il multilayer non e' implementato: e' una conseguenza
 *
 * `URTHexLibrary::Neighbor` conserva il `Layer` (`RTHexLibrary.cpp`: `FRTCellId(Cell.X + D.X, Cell.Y + D.Y,
 * Cell.Layer)`). ∴ una ricerca di vicinato non attraversa **mai** un piano, e due regioni con lo stesso
 * `X/Y` su `Layer` diversi producono due perimetri distinti **per costruzione** — non per un controllo che
 * qualcuno potrebbe togliere. Vale anche per il vincolo del cliff: ogni edge restituito nomina una cella
 * della regione, quindi vive sulla superficie del **suo** piano e non ha modo di scendere verso quello
 * sotto.
 */
UCLASS()
class REFACTORTACTICS_API URTRegionBoundaryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Gli edge di perimetro di una regione: quelli il cui vicino **non** appartiene alla regione.
	 *
	 * Un edge **interno** — condiviso da due celle della regione — non compare: lo si guarda due volte, una
	 * per lato, e tutte e due le volte il vicino e' dentro. Non serve un passo di deduplicazione, e non
	 * essercene uno e' il motivo per cui non puo' sbagliarsi.
	 *
	 * 🔴 **L'ordine e' STABILE e non e' quello del `TSet`.** Iterare un `TSet` e' un ordine che dipende
	 * dall'hash e dalla storia degli inserimenti; farne dipendere un output sarebbe la classe di difetto che
	 * il repository vieta a monte. L'uscita e' ordinata per `Layer`, poi `X`, poi `Y`, poi `Dir`.
	 */
	static TArray<FRTBoundaryEdge> ExtractBoundaryEdges(const TSet<FRTCellId>& Region);

	/** Comodita' per i chiamanti che hanno un array. Le celle duplicate non contano due volte. */
	static TArray<FRTBoundaryEdge> ExtractBoundaryEdges(const TArray<FRTCellId>& Region);

	/**
	 * Il perimetro di un'area semantica di #1941.
	 *
	 * ⛔ Non legge `Meaning`, `Source` ne' `Certainty`: il perimetro e' una proprieta' dell'**insieme di
	 * celle**, e farlo dipendere dal significato aprirebbe una grammatica che #1943 possiede.
	 */
	static TArray<FRTBoundaryEdge> ExtractBoundaryEdges(const FRTOverlayArea& Area);

	/**
	 * Le componenti connesse della regione: una regione disconnessa ha **un perimetro per componente**, e
	 * senza questa separazione i suoi edge tornerebbero in un mucchio solo.
	 *
	 * Ordine stabile per la stessa ragione di sopra: le componenti escono ordinate per la loro cella minima,
	 * e le celle dentro ciascuna sono ordinate.
	 */
	static TArray<TArray<FRTCellId>> ConnectedComponents(const TSet<FRTCellId>& Region);

	/** Comodita' per i chiamanti che hanno un array. */
	static TArray<TArray<FRTCellId>> ConnectedComponents(const TArray<FRTCellId>& Region);

	/**
	 * L'ordine canonico fra due celle: `Layer`, poi `X`, poi `Y`. E' l'ordinamento che rende stabile ogni
	 * uscita di questa libreria, in un posto solo perche' non se ne possano scrivere due versioni.
	 */
	static bool CellLess(const FRTCellId& A, const FRTCellId& B);
};
