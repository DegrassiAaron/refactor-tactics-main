#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * Controller tattico: gestisce Enhanced Input (pan/zoom della camera, selezione via cursore).
 * Gli asset di input vengono caricati da /Game/Input/ (li crei nell'editor) e sono
 * comunque sovrascrivibili nel dettaglio del controller.
 */
UCLASS()
class REFACTORTACTICS_API ARTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARTPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Input")
	TObjectPtr<UInputAction> PanAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Input")
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Input")
	TObjectPtr<UInputAction> SelectAction;

	/** Attore attualmente selezionato (se implementa IRTSelectable). */
	UPROPERTY()
	TObjectPtr<AActor> SelectedActor;

	void OnPan(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnSelect(const FInputActionValue& Value);
};
