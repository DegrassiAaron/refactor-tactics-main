#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
// `FRTGeometrySegment` come chiave di riserva del muro interno anonimo (vedi `Segment` sotto).
#include "Map/RTGeometryGrammar.h"
#include "RTMapDependencyLibrary.generated.h"

class URTHexMapAsset;

/**
 * Che TIPO di elemento autorato un handle nomina (#1864).
 *
 * `Cell` e' la superficie esagonale; gli altri sono cio' che le vive addosso o fra due di esse.
 */
UENUM(BlueprintType)
enum class ERTMapElementKind : uint8
{
	None,
	Cell,
	InteriorWall,
	Cover,
	Door,
	Transition
};

/**
 * L'identita' di un elemento autorato, come DATO e non come oggetto (#1864).
 *
 * Non e' un puntatore ne' un Actor: sopravvive a salvataggio, ricarica e cottura, con la stessa disciplina
 * di `FRTHexDoor::StableId` (CP 23.3, #832). Quali campi siano significativi dipende da `Kind`, e i
 * costruttori nominati sono l'unico modo previsto di comporlo: un handle assemblato a mano puo' nominare
 * un `Kind` e riempire i campi di un altro.
 */
USTRUCT(BlueprintType)
struct FRTMapElementHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	ERTMapElementKind Kind = ERTMapElementKind::None;

	/** La cella che porta l'elemento. Significativo per `Cell`, `InteriorWall`, `Cover`, `Door`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	FRTCellId Cell;

	/**
	 * Il bordo occupato. Significativo per `Cover` e `Door`, che vivono su un lato e non nella cella.
	 *
	 * 🔑 Per la copertura questa **e'** l'identita', insieme a `Cell`: `(Cell, Edge)` e' unica per una regola
	 * che `ValidateMap` gia' applica — un bordo porta al massimo una copertura — ed e' la ragione per cui
	 * `FRTHexCover` non ha preso un campo nome quando il muro interno l'ha preso.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	ERTHexDirection Edge = ERTHexDirection::E;

	/**
	 * Nome stabile dell'elemento. Significativo per `InteriorWall`, `Door` e `Transition`; per `Cell` e
	 * `Cover` l'identita' e' la chiave naturale, che per quei due e' gia' stabile.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	FName StableId;

	/**
	 * Chiave di RISERVA del muro interno, usata quando `StableId` e' vuoto.
	 *
	 * 🔑 **Non e' un ripensamento su v12: e' cio' che rende selezionabili i muri che un nome non ce l'hanno.**
	 * `StableId` nasce `NAME_None`, quindi ogni muro disegnato prima di v12 e' anonimo — e un handle che
	 * sapesse identificare solo i muri nominati emetterebbe candidati che non risolvono.
	 *
	 * L'unicita' non e' un'assunzione: `ValidateMap` vieta due muri identici sulla stessa cella. Ma questa
	 * chiave **cambia col move**, ed e' esattamente la ragione per cui il nome esiste e ha la precedenza:
	 * un handle per chiave regge la selezione, non l'operazione che sposta il suo bersaglio.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	FRTGeometrySegment Segment;

	FRTMapElementHandle() = default;

	/** La superficie esagonale. La sua identita' e' `FRTCellId`, che e' gia' stabile per costruzione. */
	static FRTMapElementHandle ForCell(const FRTCellId& InCell)
	{
		FRTMapElementHandle Handle;
		Handle.Kind = ERTMapElementKind::Cell;
		Handle.Cell = InCell;
		return Handle;
	}

	/**
	 * Un muro interno, per NOME.
	 *
	 * ⚠️ Non per `(Cell, Segment)`: quella chiave cambia nel move, che e' l'operazione a cui questo handle
	 * deve sopravvivere. Vedi `FRTHexInteriorWall::StableId` (formato v12).
	 */
	static FRTMapElementHandle ForInteriorWall(FName InStableId)
	{
		FRTMapElementHandle Handle;
		Handle.Kind = ERTMapElementKind::InteriorWall;
		Handle.StableId = InStableId;
		return Handle;
	}

	/**
	 * Un muro interno per **chiave naturale**, quando non ha un nome.
	 *
	 * ⚠️ Regge la selezione, non il move: spostarlo cambia il `Segment`, cioe' la chiave. Chi vuole
	 * un'identita' che sopravviva all'operazione gli da' un nome.
	 */
	static FRTMapElementHandle ForInteriorWallAt(const FRTCellId& InCell, const FRTGeometrySegment& InSegment)
	{
		FRTMapElementHandle Handle;
		Handle.Kind = ERTMapElementKind::InteriorWall;
		Handle.Cell = InCell;
		Handle.Segment = InSegment;
		return Handle;
	}

	/**
	 * Una copertura, per chiave naturale `(Cell, Edge)`.
	 *
	 * Non prende un nome perche' non le serve: un bordo porta al massimo una copertura, e la regola non e'
	 * un'assunzione di questo handle — `ValidateMap` la applica (`RTHexMapAsset.cpp`).
	 */
	static FRTMapElementHandle ForCover(const FRTCellId& InCell, ERTHexDirection InEdge)
	{
		FRTMapElementHandle Handle;
		Handle.Kind = ERTMapElementKind::Cover;
		Handle.Cell = InCell;
		Handle.Edge = InEdge;
		return Handle;
	}

	/**
	 * Una porta.
	 *
	 * ⚠️ Porta **entrambi**: il nome pubblico (CP 23.3) e il bordo da cui la si e' raggiunta. Il nome
	 * identifica la STRUTTURA, che puo' essere un gruppo di bordi su celle diverse; `(Cell, Edge)` dice
	 * quale bordo di quel gruppo e' stato cliccato. Un tool che evidenzia la selezione ha bisogno del
	 * secondo, e un'operazione che agisce sulla porta ha bisogno del primo.
	 */
	static FRTMapElementHandle ForDoor(const FRTCellId& InCell, ERTHexDirection InEdge, FName InStableId)
	{
		FRTMapElementHandle Handle;
		Handle.Kind = ERTMapElementKind::Door;
		Handle.Cell = InCell;
		Handle.Edge = InEdge;
		Handle.StableId = InStableId;
		return Handle;
	}
};

/**
 * Che cosa muore insieme all'elemento nominato da un handle (#1864).
 *
 * ⚠️ Sono INDICI negli array dell'asset, validi finche' l'asset non cambia: chi li consuma li usa dentro la
 * stessa transazione in cui li ha chiesti, e rimuove **dall'indice piu' alto al piu' basso**.
 */
USTRUCT(BlueprintType)
struct FRTMapDependencySet
{
	GENERATED_BODY()

	/** Indici in `URTHexMapAsset::InteriorWalls`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<int32> InteriorWallIndices;

	/** Indici in `URTHexMapAsset::Transitions`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<int32> TransitionIndices;

	/**
	 * Indici in `URTHexMapAsset::InteractionBindings`.
	 *
	 * Un binding entra qui quando perde la **sorgente** oppure l'**ultimo** bersaglio: in entrambi i casi
	 * cio' che resterebbe e' gia' un errore dichiarato di `ValidateInteractionGraph` — «binding senza
	 * sorgente» e «binding dichiarato senza bersagli» — quindi tenerlo in piedi non e' conservare un dato,
	 * e' produrre un invalido.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<int32> InteractionBindingIndices;
};

/**
 * Le regole di dipendenza dell'authoring: CHE COSA muore con che cosa (#1864).
 *
 * Vive nel modulo RUNTIME ed e' pura, per la stessa ragione per cui `URTGeometryGrammarLibrary` non sta
 * nell'editor: la regola e' del dominio, non dello strumento, e deve poter essere provata headless.
 *
 * ⛔ Nessuna funzione qui dentro MODIFICA l'asset. Raccogliere e cancellare sono due gesti distinti, e
 * tenerli separati e' cio' che permette a un tool di mostrare l'elenco prima di aprire la transazione.
 */
UCLASS()
class REFACTORTACTICS_API URTMapDependencyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Gli elementi che non possono sopravvivere alla scomparsa di quello nominato.
	 *
	 * Raccoglie solo cio' che e' STRETTAMENTE dipendente: un elemento la cui dipendenza sia ambigua non
	 * entra qui e non viene corretto in silenzio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static FRTMapDependencySet CollectDependents(const URTHexMapAsset* Map,
		const FRTMapElementHandle& Handle);
};
