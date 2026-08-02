#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Crea (una sola volta) Input Action e Mapping Context via codice. */
	void BuildInputMappings();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PanAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ZoomAction;

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

	/** Attore attualmente selezionato (se implementa IRTSelectable). */
	UPROPERTY()
	TObjectPtr<AActor> SelectedActor;

	void OnPan(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnSelect(const FInputActionValue& Value);
	void OnLockIn(const FInputActionValue& Value);
	void OnRestart(const FInputActionValue& Value);
	void OnAbility1(const FInputActionValue& Value);
	void OnAbility2(const FInputActionValue& Value);
	void OnAbility3(const FInputActionValue& Value);

	void SelectAbilityForCurrent(int32 Index);

public:
	/** Unita' attualmente selezionata dal giocatore (nullo se nessuna). */
	class ARTUnit* GetSelectedUnit() const;
};
