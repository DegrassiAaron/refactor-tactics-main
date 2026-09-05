#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h" // ERTHexDoorState: entra nella firma di AddDoor (#2330)
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
	RefusedDuplicate,

	/**
	 * Oltre quel bordo non c'e' nessuna cella: l'elemento non negherebbe **nessuna adiacenza**.
	 *
	 * 🔴 **Esiste per le porte, ed e' il loro modo di essere inerti senza sembrarlo** (`#2330`). Una porta e'
	 * **sottrattiva** — `spec-porte-cp93.md`: *«nega un'adiacenza che esiste»* — quindi sul bordo esterno
	 * della mappa non toglie niente. L'asset si salverebbe, l'hash cambierebbe, la porta si vedrebbe pure, e
	 * **nessun oracolo suonerebbe**: e' la classe di difetto che `#170` ha pagato tre settimane, dove una
	 * regola girava perfettamente su un contenuto che non poteva esercitarla.
	 *
	 * ⛔ **Non si riusa `RefusedNoSuchCell`**, benche' sia anch'esso «una cella che manca»: quello dice *«la
	 * cella di DESTINAZIONE non esiste, ci finirebbe un orfano»*, cioe' parla del posto dove scrivi. Questo
	 * parla del posto **dall'altra parte**, dove non scrivi niente. Due cause diverse sotto lo stesso esito
	 * mandano a correggere la cosa sbagliata — il difetto di `#1921`.
	 */
	RefusedNoNeighbour
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

	/**
	 * Cancella l'elemento nominato **insieme a tutto cio' che non puo' sopravvivergli**.
	 *
	 * Chiede l'elenco a `URTMapDependencyLibrary::CollectDependents` e lo applica: e' l'altra meta' di quella
	 * funzione, che dice *che cosa* muore e non lo esegue.
	 *
	 * ⚠️ **Esiste perche' quel passaggio non lo rifaccia ogni chiamante.** Rimuovere per indice richiede di
	 * andare dal piu' alto al piu' basso, e chi lo scrive a mano lo scrive giusto la prima volta: due
	 * implementazioni della stessa regola sono il modo in cui la regola diverge.
	 *
	 * ⛔ Non apre transazioni. Chi la chiama da un tool la avvolge nel proprio `FScopedTransaction`, cosi'
	 * l'intera cascata resta **un solo** Undo.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HexMap")
	static ERTMapEditOutcome DeleteElement(URTHexMapAsset* Map, const FRTMapElementHandle& Handle);

	/**
	 * Gli elementi autorati che vivono sotto un punto, **dal piu' specifico al piu' generale**.
	 *
	 * E' il dominio su cui poggia il ciclo di selezione: un click ripetuto scorre questa lista, quindi
	 * l'ordine e' parte del contratto e non un dettaglio. `ValidateMap` permette una porta e una copertura
	 * `Low` sullo stesso bordo, quindi due candidati nello stesso punto sono uno stato normale.
	 *
	 * ```text
	 * Door  ->  Cover  ->  InteriorWall(i della cella)  ->  Cell
	 * ```
	 *
	 * ⛔ **Le transizioni non compaiono, ed e' dichiarato**: un arco collega celle su layer diversi e non
	 * giace su un bordo, quindi la domanda «che cosa c'e' sotto questo bordo» non lo raggiunge. Il suo
	 * hit-test e' un problema di viewport, e appartiene al tool.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static TArray<FRTMapElementHandle> ElementsAt(const URTHexMapAsset* Map, const FRTCellId& Cell,
		ERTHexDirection Edge);

	/**
	 * Posa una PORTA su un bordo di cella (`#2330`).
	 *
	 * 🔴 **E' l'anello che mancava, e mancava solo lui.** Misurato prima di scriverla: le mesh del kit sono
	 * committate (`SM_Graybox_Door_Panel`, `SM_Graybox_Door_Locked`), `ARTHexMapActor` le disegna sul bordo,
	 * `Action.Interact` le apre (CP 10.1) e il formato mappa le porta dalla **v4** — ma **niente sapeva
	 * crearne una** su un asset: `RTSetObjectiveCell` timbra il solo `bIsObjective`, e questa libreria sapeva
	 * cancellare, spostare ed enumerare. La conseguenza, misurata da `#2312`: nell'intero contenuto
	 * versionato **non esiste una porta**.
	 *
	 * ⚠️ **Vive qui e non nel commandlet**, per la ragione che l'intestazione di questa classe dichiara gia':
	 * *«la regola e' del dominio, l'editor e' solo il gesto che la invoca»*. E porta un secondo vantaggio
	 * misurabile: qui la primitiva e' esercitabile dall'automation, dentro un commandlet sarebbe raggiungibile
	 * solo aprendo un Editor.
	 *
	 * ⛔ **Scrive UNA faccia sola, ed e' voluto.** `spec-porte-cp93.md` §3: *«il bordo puo' essere dichiarato
	 * dalla cella A verso B, da B verso A, o da entrambe … una porta disegnata da un lato solo vale
	 * comunque»*, e la lettura passa da `URTHexDoorLibrary::DoorBetween`. Scriverne due sarebbe la divergenza
	 * che quella regola esiste per evitare.
	 *
	 * ⛔ **Non inventa lo `StableId`.** Il nome pubblico (v9, `#832`) e' cio' che uno scenario cita e un
	 * replay risolve: sceglierlo qui sarebbe decidere del contenuto, con la stessa disciplina per cui
	 * `RTSetObjectiveCell` *«non decide DOVE va l'obiettivo»*. `NAME_None` e' legale, ed e' cio' che ogni
	 * porta scritta prima della v9 e' diventata.
	 *
	 * ⚠️ **Non rifiuta su `bBlocksMovement`**, al contrario dell'obiettivo: una porta fra una cella libera e
	 * un muro e' ridondante, non e' un errore di contenuto, e vietarla sarebbe una regola inventata qui.
	 *
	 * Rifiuta, **prima** di scrivere:
	 *
	 * | | |
	 * |---|---|
	 * | la cella non esiste | `RefusedNoSuchCell` |
	 * | oltre il bordo non c'e' una cella | `RefusedNoNeighbour` |
	 * | quel bordo ha gia' una porta | `RefusedDuplicate` |
	 *
	 * ⚠️ `DoorId` vale `-1` e non `INDEX_NONE` **solo perche' UHT non sa leggere quella costante** come
	 * default di una `UFUNCTION` (*«C++ Default parameter not parsed»*). Sono lo stesso numero, e
	 * `FRTHexDoor::DoorId` continua a dichiarare `INDEX_NONE` come «porta singola».
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HexMap")
	static ERTMapEditOutcome AddDoor(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge,
		ERTHexDoorState State, int32 DoorId = -1, FName StableId = NAME_None);
};
