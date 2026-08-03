#pragma once

#include "CoreMinimal.h"
#include "Tools/UEdMode.h"
#include "RTHexEditorMode.generated.h"

/**
 * Editor Mode dedicato alla mappa esagonale (UEdMode + Interactive Tools Framework). Non ha autorita' sui dati di
 * gioco: scrive solo sull'asset mappa via transaction. In H5a e' un guscio (nessun tool); H5b aggiunge la selezione.
 */
UCLASS()
class URTHexEditorMode : public UEdMode
{
	GENERATED_BODY()

public:
	const static FEditorModeID EM_RTHexEditorModeId;

	URTHexEditorMode();

	// UEdMode interface
	virtual void Enter() override;
	virtual void CreateToolkit() override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;
};
