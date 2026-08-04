#include "RTHexEditorModeCommands.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "RTHexEditorModeCommands"

FRTHexEditorModeCommands::FRTHexEditorModeCommands()
	: TCommands<FRTHexEditorModeCommands>("RTHexEditorMode",
		NSLOCTEXT("RTHexEditorMode", "RTHexEditorModeCommands", "Hex Map Editor Mode"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FRTHexEditorModeCommands::RegisterCommands()
{
	TArray<TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);
	UI_COMMAND(SelectTool, "Select", "Seleziona una cella cliccando nel viewport (layer attivo)",
		EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(SelectTool);

	UI_COMMAND(PaintTool, "Paint", "Dipinge o cancella una cella cliccando nel viewport (layer attivo)",
		EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(PaintTool);

	UI_COMMAND(ArchTool, "Arch", "Crea transizioni tra celle cliccando e usando il gizmo (layer attivo)",
		EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(ArchTool);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FRTHexEditorModeCommands::GetCommands()
{
	return FRTHexEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE
