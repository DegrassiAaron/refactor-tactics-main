#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/RTTypes.h"
#include "Player/RTPointerInteraction.h" // il contesto esplicito di CP 11.8 e i suoi tipi
#include "RTPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * Controller tattico. Costruisce Enhanced Input interamente in C++ (nessun .uasset richiesto):
 * pan della camera con WASD, zoom con la rotellina, selezione col click sinistro.
 */
UCLASS()
class REFACTORTACTICS_API ARTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/**
	 * Squadra comandata da questo giocatore: si selezionano e si pianificano SOLO le unita' con questo TeamId
	 * (regola in URTCombatLibrary::CanPlayerControlUnit). Nel demo il team 1 e' del bot. In multiplayer arrivera'
	 * dal PlayerState, ma la regola di autorita' resta la stessa.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|Player")
	int32 PlayerTeamId = 0;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Crea (una sola volta) Input Action e Mapping Context via codice. */
	void BuildInputMappings();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PanAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RotateAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SelectAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LockInAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RestartAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> Ability1Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> Ability2Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> Ability3Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> Ability4Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UndoAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RecenterAction;

	/** Centra la camera sull'unita' selezionata (lo zoom orbita attorno al pawn: senza spostarlo ci si
	    avvicina al centro della mappa invece che al personaggio). */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FocusAction;

	/** Attore attualmente selezionato (se implementa IRTSelectable). */
	UPROPERTY()
	TObjectPtr<AActor> SelectedActor;

	void OnPan(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnRotate(const FInputActionValue& Value);
	void OnSelect(const FInputActionValue& Value);
	void OnLockIn(const FInputActionValue& Value);
	void OnRestart(const FInputActionValue& Value);
	void OnAbility1(const FInputActionValue& Value);
	void OnAbility2(const FInputActionValue& Value);
	void OnAbility3(const FInputActionValue& Value);
	void OnAbility4(const FInputActionValue& Value); // seleziona lo scatto (4a abilita')
	void OnUndoWaypoint(const FInputActionValue& Value);
	void OnRecenter(const FInputActionValue& Value);
	void OnFocusSelected(const FInputActionValue& Value);

	/** Ricostruisce PlannedPath dell'unita' dai PathWaypoints correnti (o lo azzera se vuoti). */
	void RebuildPlannedPath();

	void SelectAbilityForCurrent(int32 Index);

public:
	/** Unita' attualmente selezionata dal giocatore (nullo se nessuna). */
	class ARTUnit* GetSelectedUnit() const;

	/**
	 * Decide cosa fare per un click su una CELLA: waypoint di movimento, scatto, oppure rifiuto col motivo.
	 *
	 * Separata dal raycast (`OnSelect` fa solo punto-mondo -> cella) perche' la decisione e' verificabile
	 * headless mentre il raycast no: richiede un viewport. Stessa disciplina di `PlanBotsForTest`.
	 * NON e' una scorciatoia per l'autorita': valida sempre sullo snapshot del TurnManager.
	 */
	void HandleClickOnCell(const FRTCellId& Cell);

	/**
	 * Decide cosa fare per un click su un'UNITA' avversaria: attacco, carica, o rifiuto col motivo. Estratta da
	 * `OnSelect` per la stessa ragione di `HandleClickOnCell` — la decisione e' verificabile headless, il
	 * raycast che la precede no.
	 */
	void HandleClickOnUnit(class ARTUnit* ClickedUnit);

	/**
	 * SELEZIONA un actor, con tutto cio' che la selezione comporta: evidenziazione, anteprima del piano,
	 * telemetria di ritmo.
	 *
	 * Da non confondere con `HandleClickOnUnit`, che **non seleziona**: quella presuppone una selezione e
	 * tratta l'argomento come BERSAGLIO. Scambiarle non produce un errore, produce silenzio — la chiamata
	 * esce subito e a schermo non cambia niente (successo il 2026-08-08).
	 *
	 * @param bRecordAsPlayerInput false quando a selezionare non e' una persona (uno scenario che allestisce
	 *        un'anteprima): la telemetria di ritmo misura quanto impiega un GIOCATORE a decidere, e una
	 *        selezione automatica falserebbe i numeri di `PIE-V01-MATCHLEN`.
	 */
	void SelectUnit(AActor* Actor, bool bRecordAsPlayerInput = true);

	/** Come sopra, per i test: il nome dichiara che il raycast e' stato saltato. */
	void HandleClickOnUnitForTest(class ARTUnit* ClickedUnit) { HandleClickOnUnit(ClickedUnit); }

	/**
	 * Inquadra un'unita' con la camera: quello che fa il tasto `F` una volta stabilito CHI inquadrare.
	 *
	 * Estratto da `OnFocusSelected` perche' la scelta della quota — la **cella**, non la posizione
	 * dell'attore — e' la sostanza di `#887` e va verificata senza passare da un `FInputActionValue`.
	 * `OnFocusSelected` resta il solo punto che decide *quale* unita': questo decide *dove*.
	 */
	void FocusCameraOnUnit(const class ARTUnit* Unit);

	/**
	 * Seleziona un actor come farebbe un click su di esso (per i test dell'interazione). Prende `AActor*` e non
	 * `ARTUnit*` perche' qui `ARTUnit` e' solo dichiarato: la conversione al puntatore base non sarebbe visibile.
	 */
	void SelectActorForTest(AActor* Actor) { SelectedActor = Actor; }

	/** Ricostruisce il percorso dai waypoint correnti, come fa l'annullamento (per i test dell'interazione). */
	void RebuildPlannedPathForTest() { RebuildPlannedPath(); }

	// ---- Contratto del puntatore (CP 11.8) ------------------------------------------------------------
	//
	// Owner della regola: `docs/technical/spec-pointer-interaction.md`. Qui c'e' lo STATO ESPLICITO che il
	// DoD chiedeva al posto della cascata di `if` sul tipo di Actor colpito.

	/**
	 * Il contesto corrente, **derivato** e non memorizzato.
	 *
	 * Derivato di proposito: un secondo stato accanto a `SelectedActor`, `SelectedAbilityIndex` e
	 * `PlannedWaypoints` sarebbe una copia che diverge, ed e' il difetto che questo repository paga di
	 * continuo con i totali scritti a mano. Qui il contesto e' una *lettura* di quei tre, piu' la fase.
	 *
	 * ⚠️ **Due contesti non sono ancora producibili, e non e' una svista.** `ReactionWindow` e `Modal`
	 * esistono nell'enum e in `URTPointerLibrary::ResolveBack` — che li ordina correttamente — ma nessuno li
	 * produce: la finestra di reazione e' **E14** e il modale e' **#613**. Aggiungere qui un flag che nessuno
	 * scrive avrebbe creato un campo senza produttore, che e' il difetto che CP 11.8 ha appena finito di
	 * documentare. Quando quegli owner arrivano, aggiungono il proprio ramo qui.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	ERTPointerContext GetPointerContext() const;

	/** Che forma di bersaglio chiede l'azione armata. `None` se non c'e' targeting in corso. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pointer")
	ERTPointerTargetKind GetPointerTargetKind() const;

	/**
	 * §5.5 — applica il Back e dichiara **quale livello** ha smontato.
	 *
	 * Restituisce il livello invece di `void` perche' l'ordine e' la cosa da provare: un test che guarda solo
	 * l'effetto non distingue «ha rimosso il waypoint perche' era il livello giusto» da «ha rimosso il
	 * waypoint perche' e' l'unica cosa che sa fare».
	 */
	ERTPointerBackStep ApplyBack();

	/**
	 * `Targeting`/`Cell` -> `PlannedAttackCell` + `bAttackTargetsCell`. Primo produttore di questo campo che
	 * non sia lo Scenario Harness (#737).
	 * @return true se il bersaglio e' stato registrato.
	 */
	bool HandleTargetCell(const FRTCellId& Cell);

	/**
	 * `Targeting`/`Edge` -> `PlannedCoverEdge` + `bHasPlannedCoverEdge`, insieme alla cella su cui agire.
	 * @return true se il lato e' stato registrato.
	 */
	bool HandleTargetEdge(const FRTCellId& Cell, ERTHexDirection Edge);

	/**
	 * `Facing` -> `PlannedFacing` + `bDeclaresPlannedFacing`. Chiude l'anello che #291 aveva lasciato aperto:
	 * le regole della rotazione dichiarata esistevano e nessun input le raggiungeva.
	 *
	 * ⚠️ Rifiuta un settore **illegale** invece di correggerlo in silenzio: e' la regola di
	 * `URTFacingLibrary`, e qui la si chiede, non la si riscrive.
	 * @return true se la rotazione e' stata dichiarata.
	 */
	bool HandleFacingSector(ERTHexDirection Sector);

	/** Entra nel contesto `Facing` (l'unita' selezionata deve esserci). */
	void BeginFacingDeclaration();

	/** Esce da `Facing` senza dichiarare nulla. */
	void EndFacingDeclaration();

	/** Vero mentre si sta dichiarando una rotazione. */
	bool IsDeclaringFacing() const { return bDeclaringFacing; }

	/**
	 * Inspector pinnato: livello 3 del Back. Il produttore e' lo Screen HUD (#613); qui c'e' il flag perche'
	 * l'ORDINE del Back e' di questo contratto e senza il livello l'elenco sarebbe incompleto.
	 */
	void SetInspectorPinned(bool bPinned) { bInspectorPinned = bPinned; }
	bool IsInspectorPinned() const { return bInspectorPinned; }

	/**
	 * `PhaseFocus` pinnato: livello 7, l'ultimo prima di `NoOp`. Owner dello scrubbing: CP 11.6 (#173).
	 * Qui c'e' solo il fatto che `RMB` lo smonta **per ultimo**.
	 */
	void SetPhaseFocusPinned(bool bPinned) { bPhaseFocusPinned = bPinned; }
	bool IsPhaseFocusPinned() const { return bPhaseFocusPinned; }

protected:
	/** Vero fra `BeginFacingDeclaration` e la conferma/annullamento. */
	bool bDeclaringFacing = false;

	bool bInspectorPinned = false;

	bool bPhaseFocusPinned = false;
};
