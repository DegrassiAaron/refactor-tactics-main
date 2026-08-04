#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/** Comandi (bottoni tool) dell'Editor Mode hex. In H5a nessun comando; H5b aggiunge SelectTool. */
class FRTHexEditorModeCommands : public TCommands<FRTHexEditorModeCommands>
{
public:
	FRTHexEditorModeCommands();

	virtual void RegisterCommands() override;
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();

	/** Tool di selezione a click (H5b). */
	TSharedPtr<FUICommandInfo> SelectTool;

	/** Tool di paint/erase a click (H5c). */
	TSharedPtr<FUICommandInfo> PaintTool;

protected:
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;
};
