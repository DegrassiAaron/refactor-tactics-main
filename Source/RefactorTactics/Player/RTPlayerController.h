#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/RTTypes.h"
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

	/** Come sopra, per i test: il nome dichiara che il raycast e' stato saltato. */
	void HandleClickOnUnitForTest(class ARTUnit* ClickedUnit) { HandleClickOnUnit(ClickedUnit); }

	/**
	 * Seleziona un actor come farebbe un click su di esso (per i test dell'interazione). Prende `AActor*` e non
	 * `ARTUnit*` perche' qui `ARTUnit` e' solo dichiarato: la conversione al puntatore base non sarebbe visibile.
	 */
	void SelectActorForTest(AActor* Actor) { SelectedActor = Actor; }

	/** Ricostruisce il percorso dai waypoint correnti, come fa l'annullamento (per i test dell'interazione). */
	void RebuildPlannedPathForTest() { RebuildPlannedPath(); }
};
