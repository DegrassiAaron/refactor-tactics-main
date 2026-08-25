#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
// `RTActionData.h` e non `RTActionDef.h`: serve anche `ERTAbilityShape`, che vive li' — `Shape` e
// `StructureOp` sono i due dati da cui `TargetKindForAction` deriva la forma del bersaglio.
#include "Ability/RTActionData.h"
#include "RTPointerInteraction.generated.h"

/**
 * Contratto del puntatore (CP 11.8) — i TIPI, e la parte che si decide senza un viewport.
 *
 * Owner documentale: `docs/technical/systems/spec-pointer-interaction.md`. Questo file implementa §4 (i contesti e il
 * `TargetKind`), §4.1 (la precedenza semantica dentro il mondo) e §5.5 (l'ordine totale del Back). Non
 * implementa §5 per intero: la matrice completa vive nel controller, perche' meta' delle sue righe hanno
 * bisogno dei servizi autorevoli (portata, percorso, occupazione) e non sono funzioni pure.
 *
 * ⚠️ **Qui non si decide nulla di gameplay.** Il puntatore produce al massimo un *target logico*; legalita',
 * percorso, bersagli e commit restano dei servizi di §3. Se una domanda serve qui e non e' in quella tabella,
 * si aggiunge un consumatore del servizio — mai un calcolo in questo file.
 */

/**
 * Il modo corrente dell'interazione. **Non e' una fase del turno**: vale insieme alla fase.
 *
 * L'ordine dei valori non e' arbitrario — cresce con la priorita' del Back di §5.5, e
 * `URTPointerLibrary::ResolveBack` ci si appoggia.
 */
UENUM(BlueprintType)
enum class ERTPointerContext : uint8
{
	/** Planning, nessuna unita' selezionata. */
	IdleSelection,
	/** Planning, unita' propria selezionata, **nessuna abilita' armata**. E' lo stato NEUTRO di D-128. */
	Planning,
	/** Planning, si stanno posando waypoint di movimento. */
	Pathing,
	/** Planning, un'azione e' armata esplicitamente e attende un bersaglio. Porta un `ERTPointerTargetKind`. */
	Targeting,
	/** Planning, si dichiara la rotazione finale. */
	Facing,
	/** Dal primo segmento risolto al Cleanup: nessun input cambia il piano. */
	ResolutionPlayback,
	/** Una Decision Window e' aperta (E14): solo le risposte sanificate dell'opportunity sono raggiungibili. */
	ReactionWindow,
	/** Modale bloccante. */
	Modal
};

/**
 * Che FORMA di bersaglio accetta l'azione armata. **Non e' una modalita' che il giocatore sceglie**: la
 * dichiara l'azione (`URTPointerLibrary::TargetKindForAction`), e da qui dipende sia cosa vince sotto il
 * cursore sia quale campo del piano viene scritto.
 */
UENUM(BlueprintType)
enum class ERTPointerTargetKind : uint8
{
	/** Nessun bersaglio da chiedere (azione su se stessi, o nessuna azione armata). */
	None,
	/** Un'unita' legale -> `PlannedAttackTarget`. */
	Unit,
	/** Una cella, anche occupata, anche vuota -> `PlannedAttackCell` + `bAttackTargetsCell`. */
	Cell,
	/** Uno dei sei bordi di una cella -> `PlannedCoverEdge` + `bHasPlannedCoverEdge`. */
	Edge,
	/** L'oggetto logico intero (porta, arco, objective). */
	Object
};

/**
 * Gli esiti ammessi. L'elenco e' **chiuso**: se un'interazione non produce uno di questi, il contratto non la
 * descrive e la si dichiara prima di scriverla.
 */
UENUM(BlueprintType)
enum class ERTPointerOutcome : uint8
{
	NoOp,
	Inspect,
	Select,
	Preview,
	Confirm,
	Cancel,
	OpenContext,
	Blocked
};

/**
 * Cosa il raycast ha trovato sotto il cursore, **prima** che qualcuno decida cosa significa.
 *
 * E' deliberatamente un sacco di candidati e non un singolo hit: il raycast restituisce cio' che sta piu'
 * vicino alla camera, che e' una proprieta' della geometria di collisione e non del significato. Chi decide
 * e' `ResolveTarget`, col contesto in mano.
 */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTPointerCandidates
{
	GENERATED_BODY()

	/** Unita' colpita, se il raggio ne ha toccata una. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	TObjectPtr<class ARTUnit> Unit = nullptr;

	/** Vero se il punto colpito ricade su una cella della mappa. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	bool bHasCell = false;

	/** Cella sotto il punto colpito (quota inclusa: il ponte da' la cella del ponte). */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	FRTCellId Cell;

	/** Vero se il punto colpito e' abbastanza vicino a un bordo da poterlo dichiarare. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	bool bHasEdge = false;

	/** Bordo piu' vicino al punto colpito, valido solo con `bHasEdge`. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	ERTHexDirection Edge = ERTHexDirection::E;

	/**
	 * Vero se il raggio ha colpito la mesh di un ELEMENTO LOGICO (porta, ponte, objective).
	 *
	 * ⚠️ E' un booleano e non un id: in v0.1 non si introduce un `MapElementId` generico — cella, bordo,
	 * `DoorId` e arco esistono gia' e bastano. L'identita' stabile e' E23 (#324).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	bool bMapElement = false;

	/** Vero se il raggio ha colpito un ghost della UI. Non e' mai un bersaglio di gioco. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	bool bGhost = false;
};

/** Il target logico che il puntatore produce. Mai un verdetto: solo *su cosa* si sta chiedendo qualcosa. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTPointerTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	ERTPointerTargetKind Kind = ERTPointerTargetKind::None;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	TObjectPtr<class ARTUnit> Unit = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Pointer")
	ERTHexDirection Edge = ERTHexDirection::E;

	bool IsValid() const { return Kind != ERTPointerTargetKind::None; }
};

/** Cosa ha smontato un `RMB`. Serve ai test: «ha annullato qualcosa» non distingue *quale* livello. */
UENUM(BlueprintType)
enum class ERTPointerBackStep : uint8
{
	/** Niente da annullare. */
	None,
	/** Fallback esplicito di una Decision Window aperta. */
	ReactionFallback,
	/** Chiusura di un modale. */
	Modal,
	/** Chiusura di un inspector pinnato. */
	Inspector,
	/** Uscita da `Targeting` o `Facing`: torna a `Planning`. */
	Declaration,
	/** Rimozione dell'ultimo waypoint. */
	Waypoint,
	/** Uscita da `Pathing` senza waypoint: torna a `Planning`. */
	Pathing,
	/** `PhaseFocus` torna ad `Auto`. */
	PhaseFocus
};

UCLASS()
class REFACTORTACTICS_API URTPointerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Che forma di bersaglio chiede questa azione. **Derivata dal catalogo, non un campo nuovo**: aggiungere
	 * un `TargetKind` al dato avrebbe creato una seconda fonte di verita' accanto a `Shape`/`StructureOp`, che
	 * gia' lo dicono.
	 *
	 * - `bSelfTarget` -> `None`: si pianifica subito, non c'e' niente da puntare.
	 * - `StructureOp != None` -> `Edge`: erigere o spostare una copertura agisce su un BORDO (CP 9.5), e il
	 *   resolver rifiuta con `CoverRejected` se il piano non lo dichiara.
	 * - `Shape == Area` -> `Cell`: un'area si centra dove si vuole, anche su un varco vuoto. E' il caso che
	 *   rende `PlannedAttackCell` raggiungibile dal giocatore.
	 * - altrimenti -> `Unit`.
	 *
	 * ⚠️ La mobilita' rapida NON compare qui: uno scatto non e' un bersaglio d'azione, e' una destinazione.
	 * Passa da `Pathing`/`HandleClickOnCell`, che la gestisce da prima di questo contratto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	static ERTPointerTargetKind TargetKindForAction(const FRTActionDef& Def, bool bSelfTarget, ERTAbilityShape Shape);

	/**
	 * §4.1 — **la mesh non governa la UX.** Sceglie, fra i candidati grezzi, quello che il contesto corrente
	 * rende significativo.
	 *
	 * Le tre conseguenze che questa funzione esiste per garantire, e che il raycast da solo sbaglia:
	 *
	 *   1. in `Pathing` una porta o un ponte **non rubano** la cella: si puo' indicare la cella oltre la porta,
	 *      e se quel percorso non si puo' fare lo rifiuta la topologia, con un reason;
	 *   2. in `Targeting`/`Cell` un'unita' sopra la cella **non ruba** la cella: e' il caso che rende un'area
	 *      utilizzabile, perche' la si centra su chi la occupa;
	 *   3. un ghost non e' **mai** un bersaglio di gioco: dove copre qualcosa, vince cio' che sta sotto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	static FRTPointerTarget ResolveTarget(ERTPointerContext Context, ERTPointerTargetKind Kind,
		const FRTPointerCandidates& Candidates);

	/**
	 * §5.5 — `RMB` e' un Back, e l'ordine e' **totale**. Restituisce quale livello smonterebbe, dato lo stato.
	 *
	 * Separata dall'esecuzione perche' l'ordine e' la cosa da provare: un test che chiama il controller e
	 * guarda l'effetto non distingue «ha rimosso il waypoint perche' era il livello giusto» da «ha rimosso il
	 * waypoint perche' e' l'unica cosa che sa fare».
	 *
	 * ⚠️ Non compare la deselezione, ed e' deliberato: `RMB` **non deseleziona mai** implicitamente l'unita'.
	 * Uscire da un targeting non deve costare la selezione.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	static ERTPointerBackStep ResolveBack(ERTPointerContext Context, bool bInspectorPinned, int32 WaypointCount,
		bool bPhaseFocusPinned);
};
