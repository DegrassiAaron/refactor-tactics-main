#pragma once

#include "CoreMinimal.h"
#include "Toolkits/BaseToolkit.h"
#include "RTHexEditorMode.h"

/** Pannello del mode: in H5a mostra la palette (vuota) e i details del tool attivo (nessuno). */
class FRTHexEditorModeToolkit : public FModeToolkit
{
public:
	FRTHexEditorModeToolkit();

	// FModeToolkit interface
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	// IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
};
