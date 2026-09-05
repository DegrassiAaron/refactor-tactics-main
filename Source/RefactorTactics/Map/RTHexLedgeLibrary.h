#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "RTHexLedgeLibrary.generated.h"

class URTHexMapAsset;

/**
 * Query pure sui BORDI APERTI e sulla relazione di atterraggio (`#2401`, [D-332]).
 *
 * Servizio distinto da pathfinding, LOS e targeting, come `AGENTS.md` §3 richiede: risponde a due domande e
 * non ne decide nessun'altra.
 *
 * ⛔ **Non risolve la caduta.** Chi cade, con quali effetti e dove finisce davvero e' `#2402`: qui non si
 * legge l'occupazione, non si applicano effetti e non si scrive niente.
 */
UCLASS()
class REFACTORTACTICS_API URTHexLedgeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Vero se uscendo da `Cell` verso `Edge` si esce nel vuoto: nessuna cella adiacente su quel lato, nello
	 * stesso layer, e nessun parapetto che lo protegga.
	 *
	 * 🔑 **«Aperto» e' DERIVATO, non autorato**, ed e' la ragione per cui `FRTHexEdgeGuard` porta solo la
	 * negazione: l'assenza di un vicino e' gia' scritta nella mappa, mentre «da qui non si cade» non lo e'.
	 * Autorare anche l'apertura avrebbe creato due sedi per lo stesso fatto, e la seconda sarebbe andata
	 * fuori sincrono al primo ridisegno.
	 *
	 * ⚠️ **Non guarda il layer sottostante**: un bordo puo' essere aperto senza avere un atterraggio, ed e'
	 * `FindLandingCell` a dirlo. Tenerle separate e' cio' che permette a `#2404` di segnalare l'atterraggio
	 * isolato come difetto d'authoring invece di far sparire il bordo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex|Ledge")
	static bool IsEdgeOpen(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge);

	/**
	 * La cella su cui si atterra cadendo da `Cell`: la piu' alta della stessa colonna sotto di essa.
	 *
	 * 🔑 **Ordina per `Layer`, e IGNORA `Height`.** `Height` e' dichiarato *«per il rendering; la logica usa
	 * Layer + archi»* (`RTHexCellData.h`), quindi ordinare per quota farebbe decidere la presentazione. Due
	 * mappe che differiscono solo per `Height` devono atterrare nella stessa cella.
	 *
	 * ⚠️ **E non e' `Layer - 1`**: la colonna puo' saltare dei piani, e il piano immediatamente sotto puo'
	 * non esistere. E' lo stesso vincolo che `URTStructuralBodyLibrary::DeriveBodies` risolve per la
	 * presentazione — stessa regola, due sedi, e questa e' l'unica che decide.
	 *
	 * @return `false` se sotto non c'e' nessuna cella: il bordo si affaccia sul nulla.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex|Ledge")
	static bool FindLandingCell(const URTHexMapAsset* Map, const FRTCellId& Cell, FRTCellId& OutLanding);
};
