#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
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

	FRTMapElementHandle() = default;

	/** La superficie esagonale. La sua identita' e' `FRTCellId`, che e' gia' stabile per costruzione. */
	static FRTMapElementHandle ForCell(const FRTCellId& InCell)
	{
		FRTMapElementHandle Handle;
		Handle.Kind = ERTMapElementKind::Cell;
		Handle.Cell = InCell;
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
