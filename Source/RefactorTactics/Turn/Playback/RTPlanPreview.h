#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"          // ERTResolutionPhase, ERTAbilityShape: le fasi canoniche
#include "Combat/RTHexCombatLibrary.h"    // FRTBlastPreview: l'origine e l'area le deriva gia' lui
#include "Combat/RTCombatLibrary.h"       // ERTTargetRefusal: il rifiuto nella forma DICIBILE al giocatore
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"             // ERTHexDirection
#include "Turn/Actions/RTIntentPrivacyLibrary.h"  // ERTIntentCertainty: i tre livelli esistono gia'
// La definizione e non una forward declaration: `MakePlanPreview` prende lo snapshot per riferimento const e
// il `.cpp` ne legge i campi.
#include "Turn/Resolution/RTHexSim.h"
#include "RTPlanPreview.generated.h"

/**
 * LA TIMELINE DEL PIANO, UNA VOCE PER FASE — `CP 11.5` ([#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172)).
 *
 * ## Perche' esiste
 *
 * Un turno simultaneo va reso leggibile **prima** di confermarlo. L'anteprima che c'era mostrava la cella di
 * destinazione e l'area del colpo; non diceva **da dove** l'unita' agira', ne' in quale ordine le cose
 * accadranno. Questo view model porta le fasi che il giocatore subisce — Prep, Dash, Blast, Move, e il Cleanup
 * quando l'azione principale e' ambientale ([D-470]) — ciascuna con la propria origine, la propria destinazione e
 * il proprio grado di certezza.
 *
 * ## 🔴 Non e' una seconda autorita', e ogni campo dice da dove viene
 *
 * ⛔ **Nessuna regola e' riscritta qui.** Il percorso lo produce `URTHexSimLibrary::BuildCompositeHexPath` —
 * lo **stesso** A* del resolver, sullo **stesso** snapshot; l'area la produce
 * `URTHexCombatLibrary::MakeBlastPreview`, che a sua volta chiama `HexHitCells`, la forma canonica del
 * combat; il facing lo deriva `URTFacingLibrary::FacingFromPath`. Se una di queste divergesse, il giocatore
 * leggerebbe una cosa e ne subirebbe un'altra — ed e' precisamente il difetto che ADR-0005 §5 vieta.
 *
 * ⚠️ **`ReadFacingForConsumer` NON si usa qui, ed e' una scelta, non una svista.** Quella funzione **scrive
 * una voce di TurnLog** (`RecordFacingChange`/`FRTTurnLogEntry`): e' la lettura che il RESOLVER dichiara di
 * aver fatto. Un'anteprima che la chiamasse inciderebbe nel registro canonico una lettura mai avvenuta in
 * partita — il difetto che `RTTurnLog.h` descrive con parole sue. `FacingFromPath` e' la stessa derivazione
 * senza il registro.
 */

/**
 * 🔑 **Il facing che il ghost mostra, e da dove lo sa.**
 *
 * Non e' un duplicato di `ERTIntentCertainty`: quello dice quanto e' certo il PIANO, questo da quale fonte
 * viene il facing di **questa** fase. Due fasi dello stesso piano possono avere fonti diverse — il Blast
 * eredita il facing dello scatto, il Move lo deriva dal proprio percorso — e appiattirle perderebbe
 * l'informazione che serve a spiegare un esito.
 */
UENUM(BlueprintType)
enum class ERTPreviewFacingSource : uint8
{
	/** Il facing autorevole dello snapshot: nessuna fase precedente lo cambia. */
	Authoritative,
	/** Derivato dal percorso di questa fase (`FacingFromPath`). */
	DerivedFromPath,
	// ⬜ `Declared` non c'e' per la stessa ragione di `PoseId`: nessun percorso lo assegnerebbe oggi, e un
	// valore di enum che nessuno produce e' un caso che i consumatori devono gestire per niente. Entra quando
	// il facing dichiarato in planning arrivera' fin qui.
	/** Ereditato dalla fase precedente della stessa timeline. */
	InheritedFromPreviousPhase
};

/**
 * UNA fase del piano, come il giocatore la vedra' accadere.
 *
 * ⚠️ **`TargetCells` e `AffectedCells` sono cose diverse e vanno tenute separate**: le prime sono cio' che il
 * piano **dichiara** di bersagliare — una cella, l'unita' scelta — le seconde cio' che l'azione
 * **toccherebbe**, area compresa. Su un `Single` coincidono; su un `Area` no, ed e' li' che il giocatore
 * decide se il proprio compagno e' dentro il raggio.
 */
USTRUCT(BlueprintType)
struct FRTPhasePreviewEntry
{
	GENERATED_BODY()

	/**
	 * ⚠️ **`ERTResolutionPhase` e non un enum nuovo**, benche' questa timeline non ne usi tutti i valori. Le fasi
	 * sono gia' una tassonomia canonica (ADR-0003 §3), con `FastMovement` e `NormalMovement` sdoppiate apposta:
	 * un secondo enum «delle fasi che si disegnano» sarebbe una seconda autorita' sullo stesso ordinamento, e
	 * divergerebbe al primo che ne cambia uno.
	 *
	 * 🔑 **La voce dell'azione principale porta la fase DEL CATALOGO** ([D-470]): `Preparation`, `Control`,
	 * `Attack` o `Environment`. ⏱️ *Fino a [D-470] diceva sempre `Attack`.*
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTResolutionPhase Phase = ERTResolutionPhase::Snapshot;

	/** Indice dell'unita' nello snapshot: la stessa identita' che usa il resolver, mai un pointer. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	int32 UnitId = INDEX_NONE;

	/** L'azione che questa fase esegue. `NAME_None` per una fase che il piano non riempie. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FName ActionId;

	/**
	 * Da dove l'unita' agisce in questa fase — il punto che l'anteprima vecchia non diceva. Per l'azione
	 * principale e' la sua origine di MIRA ([D-464]).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FRTCellId PreviewOrigin;

	/**
	 * Dove sara' a fase conclusa: e' qui che si posa il ghost. Per le fasi che non spostano coincide con
	 * `PreviewOrigin`, con un'eccezione: la principale del Cleanup mira da dove e' stata pianificata ([D-464]) e
	 * risolve dopo il Move, quindi la sua destinazione e' la cella dopo il Move ([D-470]).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FRTCellId PreviewDestination;

	/** Il percorso completo, quando la fase ne ha uno. Vuoto per le fasi che non spostano. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	TArray<FRTCellId> PreviewPath;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Vedi `ERTPreviewFacingSource`: da quale fonte viene il facing qui sopra. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTPreviewFacingSource FacingSource = ERTPreviewFacingSource::Authoritative;

	// ⬜ **`PoseId` NON c'e', ed e' una scelta.** La DoD lo elenca fra i campi della entry, e una prima
	// stesura lo aveva aggiunto scrivendoci quattro letterali (`Pose.Prep`, `Pose.Dash`, …) che **nessuno**
	// legge: il progetto non ha un sistema di pose. Sarebbe superficie `USTRUCT` esposta a Blueprint, da
	// versionare e da spiegare, per un consumatore che non esiste — cioe' il placeholder per una roadmap
	// lontana che `CLAUDE.md` §4 vieta. Torna insieme al sistema che lo consuma, e sta in FOLLOW-UP.

	/** Cio' che il piano DICHIARA di bersagliare. Vedi la nota della struct. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	TArray<FRTCellId> TargetCells;

	/** Cio' che l'azione TOCCHEREBBE, area compresa. Vedi la nota della struct. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	TArray<FRTCellId> AffectedCells;

	/** Sottoinsieme di `AffectedCells` occupato da alleati vivi: fuoco amico. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	TArray<FRTCellId> AllyCells;

	/**
	 * Quanto e' certo cio' che questa voce mostra. ⚠️ **`ERTIntentCertainty` e non un enum nuovo**: i tre
	 * livelli disegnabili — `Confirmed`, `Predicted`, `Uncertain` — esistono gia' e hanno gia' le loro chiavi
	 * icona (`UI.Icon.Certainty.*`). `Unknown` qui e' un difetto a monte, non uno stato previsto.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTIntentCertainty Certainty = ERTIntentCertainty::Unknown;

	/**
	 * Perche' il bersaglio non e' valido, quando non lo e'.
	 *
	 * 🔴 **`ERTTargetRefusal` e NON `ERTHexTargetReason`, ed e' una scelta di privacy, non di gusto.**
	 * Quel secondo enum e' la classificazione **interna**: il suo stesso header dichiara che *«dice il vero
	 * anche quando il vero non e' dicibile»*, e distingue `OutOfRange` da `NoLineOfSight` da `NoMap`. Portarlo
	 * in un view model che la presentazione consuma darebbe al giocatore un rilevatore di presenze: la
	 * differenza fra due messaggi sarebbe **essa stessa** il canale, contro [D-225].
	 *
	 * `ERTTargetRefusal` e' cio' che si puo' mostrare, e la differenza sta tutta in `Nothing`, che copre
	 * insieme «cella vuota» e «c'e' un nemico che la tua squadra non osserva». Qui si **trasporta** il
	 * verdetto gia' prodotto a monte; non si riclassifica niente.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTTargetRefusal TargetRefusal = ERTTargetRefusal::None;

	/**
	 * **Il posto della voce nella pila della sua colonna**, da 1 a N (HUD-AP.5b, #4159; `progettazione-hud.md` §19-bis.7).
	 * La colonna e' la macro-fase, `URTCatalogLibrary::MapResolutionPhase(Phase)`: una `Control` e un `Attack` stanno
	 * nella stessa pila, quella del Blast.
	 *
	 * 🔑 **L'ordine e' quello del resolver, mai quello di dichiarazione** ([D-615] (5)): permutare le voci del piano non
	 * cambia la pila. Lo decidono `URTActionQueueLibrary::NumberInstancesByContent` nella colonna del Prep e
	 * `URTActionQueueLibrary::MainVisitLess` in quelle del Blast e del Cleanup, le stesse funzioni che il resolver usa.
	 * `0` = voce costruita a mano, che nessuna timeline ha numerato.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	int32 StackIndex = 0;

	FRTPhasePreviewEntry() = default;
};

/**
 * 🔴 **LA REAZIONE, E NON E' UNA QUINTA FASE.**
 *
 * Una struttura separata, non un elemento di `Phases`, ed e' una voce esplicita della DoD di `CP 11.5`. La
 * ragione e' strutturale e non estetica: le fasi sono un **ordinamento** — accadono una dopo l'altra, in un
 * momento che la timeline dichiara — mentre una reazione armata e' una **condizione**: non ha un posto
 * nell'ordine, scatta se e quando il suo innesco si verifica, e puo' non scattare affatto.
 *
 * ⚠️ Infilarla nella lista costringerebbe chi disegna a rispondere «quando?» inventando una risposta, e chi
 * legge la userebbe per prevedere un ordine che non esiste. `Preview.ReactionIsNotAPhaseEntry` lo pinna.
 */
USTRUCT(BlueprintType)
struct FRTReactionPreview
{
	GENERATED_BODY()

	/** Vero se l'unita' ha armato una reazione. Senza questo, il resto non si legge. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	bool bArmed = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	int32 UnitId = INDEX_NONE;

	/** Il profilo armato dal `Brace` ([D-047]). `NAME_None` = profilo base. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FName ReactionProfileId;

	/**
	 * Da dove l'unita' sorveglia. ⚠️ **E' la cella di FINE Dash, non quella di fine Move**: la reazione e'
	 * armata in Prep e la fase Dash risolve prima del Blast, mentre il Move arriva dopo — sorvegliare dalla
	 * cella di arrivo direbbe che l'unita' e' gia' li' quando l'innesco scatta.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FRTCellId WatchOrigin;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTHexDirection Facing = ERTHexDirection::E;

	/**
	 * ⚠️ **Non puo' essere `Confirmed`, mai.** Una reazione armata dipende da un innesco che potrebbe non
	 * verificarsi: dichiararla certa sarebbe una promessa che la simulazione non fa.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTIntentCertainty Certainty = ERTIntentCertainty::Unknown;

	FRTReactionPreview() = default;
};

/** La timeline completa: le fasi in ordine di risoluzione, piu' la reazione che fase non e'. */
USTRUCT(BlueprintType)
struct FRTPlanPreview
{
	GENERATED_BODY()

	/**
	 * Le voci che il piano riempie, **in ordine di risoluzione**. Una fase che il piano non usa non compare:
	 * una voce vuota costringerebbe chi disegna a distinguere «non pianificata» da «pianificata a vuoto»
	 * guardando dentro i campi.
	 *
	 * ➕ **Piu' voci nella stessa fase stanno una dopo l'altra, nell'ordine della loro pila** (HUD-AP.5b, #4159): due azioni
	 * di Prep, due principali del Blast. Il posto nella pila lo dice `FRTPhasePreviewEntry::StackIndex`. Il movimento resta
	 * una voce sola, con tutti i suoi tratti.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	TArray<FRTPhasePreviewEntry> Phases;

	/** Vedi `FRTReactionPreview`: separata di proposito. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FRTReactionPreview Reaction;

	FRTPlanPreview() = default;
};

/**
 * UNA voce di Prep del piano in ingresso (E38 CP 38.13; HUD-AP.5b, #4159): un'azione di preparazione aggiunta, come una
 * Guard. Non sposta e non bersaglia, quindi non porta che cio' che serve a nominarla e a ordinarla.
 */
USTRUCT(BlueprintType)
struct FRTPrepPreviewInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FName ActionId;

	/** `FRTActionDef::Priority`: la seconda chiave di `InstanceLess`, dopo la fase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	int32 Priority = 0;

	/** La posizione della voce nel piano. Decide solo fra copie uguali anche nel contenuto, dove l'ordine non si vede. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	int32 DeclarationIndex = INDEX_NONE;

	FRTPrepPreviewInput() = default;
};

/**
 * UNA voce principale del piano in ingresso (E38 CP 38.14; HUD-AP.5b, #4159). Ognuna ha la sua mira, quindi la sua area e
 * la sua origine.
 */
USTRUCT(BlueprintType)
struct FRTMainPreviewInput
{
	GENERATED_BODY()

	/**
	 * Il piano della voce, nella forma che `MakeBlastPreview` gia' consuma. La sua `Phase` decide anche l'etichetta e la
	 * colonna della voce nella timeline ([D-470]).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FRTBlastPreviewPlan Blast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FName ActionId;

	/**
	 * Il rifiuto gia' tradotto in cio' che il giocatore puo' sapere. Vedi
	 * `FRTPhasePreviewEntry::TargetRefusal`: la traduzione da `ERTHexTargetReason` avviene **a monte**, dove
	 * si sa cosa l'osservatore osserva, e non qui.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	ERTTargetRefusal TargetRefusal = ERTTargetRefusal::None;

	// ── Le chiavi d'ordine che `Blast` non porta (`FRTMainVisitKey`) ───────────────────────────────────────────────
	// La macro-fase, l'azione e la cella di mira `MakePlanPreview` le ricava da `Blast` e da `ActionId`: due copie degli
	// stessi dati potrebbero contraddirsi.

	/** `FRTActionDef::Priority`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	int32 Priority = 0;

	/** Il bersaglio-unita' per identita' di partita ([D-063]), come lo confronta il resolver; `INDEX_NONE` senza. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	int32 TargetStableUnitId = INDEX_NONE;

	/** Il bordo di copertura dichiarato, nella forma di `FRTMainVisitKey::CoverEdge`: `0` nessuno, la direzione piu' uno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	uint8 CoverEdge = 0;

	/** La posizione della voce nel piano: l'ultimo spareggio, fra copie identiche in tutto il contenuto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	int32 DeclarationIndex = INDEX_NONE;

	FRTMainPreviewInput() = default;
};

/**
 * Il PIANO in ingresso, nella forma minima che serve a derivare la timeline.
 *
 * 🔑 **Una struct di ingresso e non `ARTUnit`, come per `FRTBlastPreviewPlan`.** L'unita' e' un Actor e non
 * esiste in un test headless; il piano invece e' un dato. E' il pattern che questa famiglia usa gia', ed e'
 * cio' che rende `MakePlanPreview` verificabile senza un mondo.
 */
USTRUCT(BlueprintType)
struct FRTPlanPreviewInput
{
	GENERATED_BODY()

	/** Indice dell'unita' nello snapshot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	int32 UnitId = INDEX_NONE;

	// ── Prep ────────────────────────────────────────────────────────────────────────────────────────────
	/** Vero se il piano arma una reazione (`Brace`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	bool bReactionArmed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FName ReactionProfileId;

	/** L'azione di preparazione dichiarata, se c'e' (stance, trappola). `NAME_None` = nessuna. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FName PrepActionId;

	/**
	 * Le voci di Prep del piano (E38 CP 38.13), in qualunque ordine: la pila le ordina come `ResolvePrep`, insieme a una
	 * principale di `Preparation` (HUD-AP.5b, #4159).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	TArray<FRTPrepPreviewInput> PrepEntries;

	// ── Dash ────────────────────────────────────────────────────────────────────────────────────────────
	/** Vero se il piano dichiara uno scatto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	bool bDashPlanned = false;

	/**
	 * Vero se lo scatto si applica DAVVERO prima del Blast. ⚠️ Non e' `bDashPlanned`: `ResolveDash` puo'
	 * fermarlo (collisione simultanea, CP 4.8). La risposta e' `ARTUnit::PlannedDashMoves()` ([D-471]): la domanda
	 * del resolver, compreso il rifiuto dello stato.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	bool bDashResolves = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FRTCellId PlannedDashCell;

	/**
	 * Lo stile dello scatto pianificato, come lo dichiara il catalogo (`FRTActionDef::MovementStyle`). Per uno stile
	 * LINEARE la rotta del fantasma e' quella che `URTMovementActionLibrary::ResolveLinearMove` percorre ([D-524]):
	 * la stessa retta, lo stesso arresto su un ostacolo, un bordo o un nemico. Con `None` (o `Budget`) la rotta si
	 * cerca col grafo, come il resolver fa per le mobilita' a budget.
	 *
	 * ⏱️ *Fino a [D-524] la rotta veniva sempre dall'A\* del movimento, con un troncamento a parte per la carica oltre
	 * la portata (`DashReachCells`).* L'A\* ha piu' percorsi minimi sulle celle fuori asse e puo' scegliere un ultimo
	 * passo diverso da quello della risoluzione, e davanti a un ostacolo lo aggira mentre lo scatto si ferma.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	ERTMovementStyle DashStyle = ERTMovementStyle::None;

	/**
	 * La portata dello scatto in CELLE, come la legge il resolver (`ARTUnit::EffectiveDashRangeOf`): letta solo per
	 * uno stile lineare. Una carica col bersaglio oltre la portata corre fin li' e si ferma ([D-516]), ed e' questo
	 * numero a dire dove — non un conteggio a parte.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	int32 DashRange = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FName DashActionId;

	// ── Le principali ─────────────────────────────────────────────────────────────────────────────────────
	/**
	 * Le voci principali del piano (E38 CP 38.14), in qualunque ordine. Ognuna va nella colonna della sua fase ([D-470]) e,
	 * dentro la colonna, al posto che le da' l'ordine del resolver (HUD-AP.5b, #4159). Una voce senza azione
	 * (`Blast.bHasAction` falso) non compare.
	 *
	 * ⏱️ *Fino a HUD-AP.5b l'ingresso aveva una principale sola* (`Blast`, `BlastActionId`, `BlastTargetRefusal`): il
	 * produttore ci metteva quella a fuoco, e le altre non si vedevano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	TArray<FRTMainPreviewInput> Mains;

	// ── Move ────────────────────────────────────────────────────────────────────────────────────────────
	/** I waypoint dichiarati. Il percorso lo ricava `BuildCompositeHexPath`, non il chiamante. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	TArray<FRTCellId> PlannedWaypoints;

	/**
	 * Vero se il Move dichiarato si compie DAVVERO: la domanda e' `!ARTUnit::PlannedNormalMoveDeniedByStun()` ([D-569]), la
	 * stessa del resolver (`BeginMovementResolution`). Falso, la voce Move resta nella timeline ma e' una rotta RIFIUTATA —
	 * ghost sulla cella dove l'unita' si trova, nessun percorso — come uno scatto che non si applica: nasconderla direbbe
	 * che il Move non e' stato pianificato, e il click lo ha accettato ([D-565] (2)). Il default e' `true`: chi non conosce lo
	 * stordimento, e i test che costruiscono l'ingresso a mano, restano com'erano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	bool bMoveResolves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FName MoveActionId;

	FRTPlanPreviewInput() = default;
};

UCLASS()
class REFACTORTACTICS_API URTPlanPreviewLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 🔑 **Deriva la timeline dal piano e dallo snapshot. Funzione PURA: nessuna scrittura, nessun Actor.**
	 *
	 * `CombatUnits` e' l'insieme nella forma che il Blast riceve, con gli **stessi indici** dello snapshot:
	 * cosi' l'identita' dell'attaccante e quella dei bersagli sono le stesse da entrambi i lati.
	 *
	 * ⛔ **Non decide se il piano sia legale**: quello lo fa `ValidatePlan` (#605). Qui si mostra cio' che il
	 * piano produrrebbe **se** risolvesse, e `Certainty` dice quanto sia probabile che ci riesca.
	 *
	 * ⚠️ **Non e' una `UFUNCTION`, e non e' una dimenticanza.** `FRTHexSnapshot` e' una struct C++ semplice e
	 * **non** una `USTRUCT`: non attraversa la reflection, quindi non puo' essere parametro di una funzione
	 * esposta a Blueprint. E' la stessa ragione per cui `URTHexSimLibrary::BuildCompositeHexPath` — che di
	 * snapshot ne prende uno — resta una `static` nuda. ⛔ Renderla `USTRUCT` per esporre questa funzione
	 * sarebbe aprire lo stato canonico del turno a Blueprint per comodita' di un'anteprima.
	 */
	static FRTPlanPreview MakePlanPreview(const FRTHexSnapshot& Snapshot, const FRTPlanPreviewInput& Plan,
		const TArray<FRTHexCombatUnit>& CombatUnits);
};
