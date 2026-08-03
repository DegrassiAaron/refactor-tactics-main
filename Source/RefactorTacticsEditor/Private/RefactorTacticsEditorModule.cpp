#include "RefactorTacticsEditorModule.h"
#include "RTHexEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "FRefactorTacticsEditorModule"

void FRefactorTacticsEditorModule::StartupModule()
{
	// Il mode si registra da solo via CDO (Info nel costruttore di URTHexEditorMode). Qui solo i comandi dei tool.
	FRTHexEditorModeCommands::Register();
}

void FRefactorTacticsEditorModule::ShutdownModule()
{
	FRTHexEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRefactorTacticsEditorModule, RefactorTacticsEditor)
