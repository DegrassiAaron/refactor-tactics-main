#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTMapDependencyLibrary.h"
#include "RTMapEditLibrary.generated.h"

class URTHexMapAsset;
struct FRTGeometrySegment;

/**
 * L'esito di un'operazione di authoring (#1864).
 *
 * ⛔ **Un rifiuto e' un valore di ritorno, non un'eccezione ne' un silenzio.** I Non-goal della issue
 * vietano di correggere in silenzio uno stato autorato invalido: o si rifiuta il gesto **dicendo quale
 * regola l'ha fermato**, o non lo si tocca. E' la disciplina di `ERTHexArchPendingCloseReason` (#996) —
 * loggare *che cosa* e *per quale ragione*, non solo che qualcosa non e' successo.
 */
UENUM(BlueprintType)
enum class ERTMapEditOutcome : uint8
{
	/** L'operazione e' stata applicata. */
	Applied,

	/** L'handle non nomina nessun elemento esistente. */
	RefusedUnresolved,

	/** La cella di destinazione non esiste: ci finirebbe un orfano. */
	RefusedNoSuchCell,

	/** Il segmento non sta nella grammatica a 30 gradi. `ValidateSegment` decide, non questa funzione. */
	RefusedOutOfGrammar,

	/**
	 * Il segmento chiuderebbe almeno un bordo, e allora e' una COPERTURA.
	 *
	 * ⚠️ Non e' un tecnicismo: `InteriorWalls` porta per invariante solo cio' che nessuna copertura puo'
	 * rappresentare, e scriverlo in entrambi i posti creerebbe due verita' sullo stesso muro.
	 */
	RefusedWouldCloseEdge,

	/** Un muro identico esiste gia' nella cella di destinazione. */
	RefusedDuplicate
};

/**
 * Le operazioni di authoring sulla mappa: spostare, e in seguito ruotare e cancellare (#1864).
 *
 * Vive nel modulo RUNTIME accanto alla grammatica e alle regole di dipendenza, e per la stessa ragione: la
 * regola e' del dominio, l'editor e' solo il gesto che la invoca. Nessuna regola di gioco nuova qui dentro.
 *
 * ⚠️ A differenza di `URTMapDependencyLibrary`, queste funzioni **modificano** l'asset — e proprio per
 * questo ognuna valida PRIMA di scrivere: un'operazione o si applica intera o non lascia traccia.
 */
UCLASS()
class REFACTORTACTICS_API URTMapEditLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * L'indice del muro interno che l'handle nomina, o `INDEX_NONE`.
	 *
	 * Pura. Un handle con `StableId` vuoto non risolve: `NAME_None` significa «muro senza nome», e ce ne
	 * possono essere molti — risolverne uno a caso sarebbe peggio che non risolvere.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static int32 ResolveInteriorWall(const URTHexMapAsset* Map, const FRTMapElementHandle& Handle);

	/**
	 * Sposta un muro interno, conservandone l'identita'.
	 *
	 * 🔑 E' l'operazione per cui `FRTHexInteriorWall::StableId` esiste: il `Segment` cambia, quindi la
	 * chiave naturale cambia, quindi l'handle deve essere un NOME.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HexMap")
	static ERTMapEditOutcome MoveInteriorWall(URTHexMapAsset* Map, const FRTMapElementHandle& Handle,
		const FRTCellId& NewCell, const FRTGeometrySegment& NewSegment);
};
