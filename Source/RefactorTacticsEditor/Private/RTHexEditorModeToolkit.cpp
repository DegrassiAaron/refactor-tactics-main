#include "RTHexEditorModeToolkit.h"

#define LOCTEXT_NAMESPACE "RTHexEditorModeToolkit"

FRTHexEditorModeToolkit::FRTHexEditorModeToolkit()
{
}

void FRTHexEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FRTHexEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}

FName FRTHexEditorModeToolkit::GetToolkitFName() const
{
	return FName("RTHexEditorMode");
}

FText FRTHexEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "Hex Map");
}

#undef LOCTEXT_NAMESPACE
