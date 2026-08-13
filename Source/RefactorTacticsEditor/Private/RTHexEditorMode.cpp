#include "RTHexEditorMode.h"
#include "RTHexEditorModeToolkit.h"
#include "RTHexEditorModeCommands.h"
#include "InteractiveToolManager.h"
#include "Tools/RTHexSelectTool.h"
#include "Tools/RTHexPaintTool.h"
#include "Tools/RTHexArchTool.h"
#include "Tools/RTHexGeometryTool.h"
#include "Tools/RTHexFillTool.h"

#define LOCTEXT_NAMESPACE "RTHexEditorMode"

const FEditorModeID URTHexEditorMode::EM_RTHexEditorModeId = TEXT("EM_RTHexEditorMode");

URTHexEditorMode::URTHexEditorMode()
{
	// Impostare Info nel costruttore E' cio' che registra il mode nella toolbar (nessuna RegisterMode esplicita).
	Info = FEditorModeInfo(
		EM_RTHexEditorModeId,
		LOCTEXT("RTHexEditorModeName", "Hex Map"),
		FSlateIcon(),
		true /*bVisible*/);
}

void URTHexEditorMode::Enter()
{
	UEdMode::Enter();

	const FRTHexEditorModeCommands& Commands = FRTHexEditorModeCommands::Get();
	RegisterTool(Commands.SelectTool, TEXT("RTHexSelectTool"), NewObject<URTHexSelectToolBuilder>(this));
	RegisterTool(Commands.PaintTool, TEXT("RTHexPaintTool"), NewObject<URTHexPaintToolBuilder>(this));
	RegisterTool(Commands.ArchTool, TEXT("RTHexArchTool"), NewObject<URTHexArchToolBuilder>(this));
	RegisterTool(Commands.FillTool, TEXT("RTHexFillTool"), NewObject<URTHexFillToolBuilder>(this));
	RegisterTool(Commands.GeometryTool, TEXT("RTHexGeometryTool"), NewObject<URTHexGeometryToolBuilder>(this));

	GetToolManager()->SelectActiveToolType(EToolSide::Left, TEXT("RTHexSelectTool"));
}

void URTHexEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FRTHexEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> URTHexEditorMode::GetModeCommands() const
{
	return FRTHexEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
