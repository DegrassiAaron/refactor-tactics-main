#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"          // ERTResolutionPhase, ERTAbilityShape: le fasi canoniche
#include "Combat/RTHexCombatLibrary.h"    // FRTBlastPreview: l'origine e l'area le deriva gia' lui
#include "Combat/RTCombatLibrary.h"       // ERTTargetRefusal: il rifiuto nella forma DICIBILE al giocatore
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"             // ERTHexDirection
#include "Turn/RTIntentPrivacyLibrary.h"  // ERTIntentCertainty: i tre livelli esistono gia'
// La definizione e non una forward declaration: `MakePlanPreview` prende lo snapshot per riferimento const e
// il `.cpp` ne legge i campi.
#include "Turn/RTHexSim.h"
#include "RTPlanPreview.generated.h"

/**
 * LA TIMELINE DEL PIANO, UNA VOCE PER FASE — `CP 11.5` ([#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172)).
 *
 * ## Perche' esiste
 *
 * Un turno simultaneo va reso leggibile **prima** di confermarlo. L'anteprima che c'era mostrava la cella di
 * destinazione e l'area del colpo; non diceva **da dove** l'unita' agira', ne' in quale ordine le cose
 * accadranno. Questo view model porta le quattro fasi che il giocatore subisce — Prep, Dash, Blast, Move —
 * ciascuna con la propria origine, la propria destinazione e il proprio grado di certezza.
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
	/** Dichiarato in planning e accettato dall'insieme legale. */
	Declared,
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
	 * ⚠️ **`ERTResolutionPhase` e non un enum nuovo di quattro valori**, benche' questa timeline ne usi
	 * quattro. Le fasi sono gia' una tassonomia canonica (ADR-0003 §3), con `FastMovement` e `NormalMovement`
	 * sdoppiate apposta: un secondo enum «delle fasi che si disegnano» sarebbe una seconda autorita' sullo
	 * stesso ordinamento, e divergerebbe al primo che ne cambia uno.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTResolutionPhase Phase = ERTResolutionPhase::Snapshot;

	/** Indice dell'unita' nello snapshot: la stessa identita' che usa il resolver, mai un pointer. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	int32 UnitId = INDEX_NONE;

	/** L'azione che questa fase esegue. `NAME_None` per una fase che il piano non riempie. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FName ActionId;

	/** Da dove l'unita' agisce in questa fase — il punto che l'anteprima vecchia non diceva. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FRTCellId PreviewOrigin;

	/** Dove sara' a fase conclusa. Uguale a `PreviewOrigin` per le fasi che non spostano. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FRTCellId PreviewDestination;

	/** Il percorso completo, quando la fase ne ha uno. Vuoto per Prep e Blast. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	TArray<FRTCellId> PreviewPath;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Vedi `ERTPreviewFacingSource`: da quale fonte viene il facing qui sopra. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	ERTPreviewFacingSource FacingSource = ERTPreviewFacingSource::Authoritative;

	/**
	 * ⚠️ **Un SUGGERIMENTO di presentazione, e non ha nessuna autorita'.** Il progetto non ha un sistema di
	 * pose — `grep -c PoseId` fuori da questa famiglia risponde **0** — quindi qui non si sta consumando un
	 * dato esistente: si sta nominando la posa che il ghost dovrebbe assumere, derivata dalla fase e
	 * dall'azione. Chi disegna puo' ignorarlo; nessuna regola lo legge.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FName PoseId;

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
	 * Le fasi che il piano riempie, **in ordine di risoluzione**. Una fase che il piano non usa non compare:
	 * una voce vuota costringerebbe chi disegna a distinguere «non pianificata» da «pianificata a vuoto»
	 * guardando dentro i campi.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	TArray<FRTPhasePreviewEntry> Phases;

	/** Vedi `FRTReactionPreview`: separata di proposito. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Preview")
	FRTReactionPreview Reaction;

	FRTPlanPreview() = default;
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

	// ── Dash ────────────────────────────────────────────────────────────────────────────────────────────
	/** Vero se il piano dichiara uno scatto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	bool bDashPlanned = false;

	/**
	 * Vero se lo scatto si applica DAVVERO prima del Blast. ⚠️ Non e' `bDashPlanned`: `ResolveDash` puo'
	 * fermarlo (collisione simultanea, CP 4.8), ed e' la stessa domanda che `ARTUnit::PlannedDashApplies()`
	 * risponde per il resolver.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	bool bDashResolves = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FRTCellId PlannedDashCell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FName DashActionId;

	// ── Blast ───────────────────────────────────────────────────────────────────────────────────────────
	/** Il piano dell'azione principale, nella forma che `MakeBlastPreview` gia' consuma. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FRTBlastPreviewPlan Blast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	FName BlastActionId;

	/**
	 * Il rifiuto gia' tradotto in cio' che il giocatore puo' sapere. Vedi
	 * `FRTPhasePreviewEntry::TargetRefusal`: la traduzione da `ERTHexTargetReason` avviene **a monte**, dove
	 * si sa cosa l'osservatore osserva, e non qui.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	ERTTargetRefusal BlastTargetRefusal = ERTTargetRefusal::None;

	// ── Move ────────────────────────────────────────────────────────────────────────────────────────────
	/** I waypoint dichiarati. Il percorso lo ricava `BuildCompositeHexPath`, non il chiamante. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Preview")
	TArray<FRTCellId> PlannedWaypoints;

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
