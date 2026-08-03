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
	// H5a: nessun tool. H5b registrera' qui il comando SelectTool.
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FRTHexEditorModeCommands::GetCommands()
{
	return FRTHexEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE
