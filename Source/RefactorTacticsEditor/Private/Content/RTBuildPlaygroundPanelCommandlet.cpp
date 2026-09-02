#include "Content/RTBuildPlaygroundPanelCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

#include "RTPlaygroundPanelLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTPlaygroundPanel, Log, All);

namespace
{
	const TCHAR* RTPanelPackage = TEXT("/Game/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground");
	const TCHAR* RTPanelAsset   = TEXT("WBP_RT_GrayKitPlayground");

	/** Un testo nel box, con nome STABILE: e' la maniglia con cui il grafo lo raggiungera'. */
	UTextBlock* AddLine(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, const FString& Text)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
		Block->SetText(FText::FromString(Text));
		Box->AddChildToVerticalBox(Block);
		return Block;
	}
}

int32 URTBuildPlaygroundPanelCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	const bool bForce = Switches.Contains(TEXT("Force"));

	UEditorUtilityWidgetBlueprint* Existing = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, RTPanelPackage);
	if (Existing && !bForce)
	{
		// ⛔ Si rifiuta invece di sovrascrivere: il grafo e le rifiniture di layout sono authoring, e
		// rigenerare li cancella senza dirlo. E' la stessa regola del fixture di #1992.
		UE_LOG(LogRTPlaygroundPanel, Error,
			TEXT("[PlaygroundPanel] %s esiste gia'. -Force per rigenerarlo, sapendo che il grafo autorato va perso."),
			RTPanelAsset);
		return 1;
	}
	if (Existing)
	{
		// ⚠️ `CreateBlueprint` ASSERISCE se un oggetto con quel nome vive gia' nel package — misurato su
		// #1992 (`Kismet2.cpp:441`): non ritorna `nullptr`, fa crashare il commandlet.
		Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty);
	}

	UPackage* Package = CreatePackage(RTPanelPackage);
	if (!Package)
	{
		UE_LOG(LogRTPlaygroundPanel, Error, TEXT("[PlaygroundPanel] package non creabile."));
		return 1;
	}

	UEditorUtilityWidgetBlueprint* PanelBP = Cast<UEditorUtilityWidgetBlueprint>(
		FKismetEditorUtilities::CreateBlueprint(
			UEditorUtilityWidget::StaticClass(), Package, FName(RTPanelAsset),
			BPTYPE_Normal, UEditorUtilityWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));

	if (!PanelBP || !PanelBP->WidgetTree)
	{
		UE_LOG(LogRTPlaygroundPanel, Error, TEXT("[PlaygroundPanel] creazione del widget blueprint fallita."));
		return 1;
	}

	UWidgetTree* Tree = PanelBP->WidgetTree;
	UVerticalBox* Root = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	Tree->RootWidget = Root;

	// ---- HEADER ----------------------------------------------------------------
	AddLine(Tree, Root, TEXT("Txt_Title"), TEXT("GRAY KIT PLAYGROUND    v0.1"));
	AddLine(Tree, Root, TEXT("Txt_MapState"), TEXT("<mappa>  —  <Ready/Error>"));

	// ---- STATION ---------------------------------------------------------------
	AddLine(Tree, Root, TEXT("Txt_StationHeader"), TEXT("STATION"));
	UComboBoxString* StationCombo =
		Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("Cmb_Station"));
	// ⛔ Le otto voci NON sono incise qui: il grafo le prende da `GetStations()`, che delega alla
	// planimetria. Incidere «01..08» sarebbe la seconda copia che `#1459` ha gia' fatto pagare.
	Root->AddChildToVerticalBox(StationCombo);
	Root->AddChildToVerticalBox(
		Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Focus")));

	// ---- FIXTURE ---------------------------------------------------------------
	AddLine(Tree, Root, TEXT("Txt_FixtureHeader"), TEXT("FIXTURE"));
	AddLine(Tree, Root, TEXT("Txt_FixtureName"), TEXT("<nessun fixture selezionato>"));
	// ⛔ Le sei voci le mette `GetFacingOptions()`, derivate da `StaticEnum<ERTHexDirection>()`.
	Root->AddChildToVerticalBox(
		Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("Cmb_Facing")));
	Root->AddChildToVerticalBox(
		Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_SelectFixture")));
	Root->AddChildToVerticalBox(
		Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_ResetFixture")));

	// ---- VIEW ------------------------------------------------------------------
	AddLine(Tree, Root, TEXT("Txt_ViewHeader"), TEXT("VIEW"));
	Root->AddChildToVerticalBox(Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_CamClose")));
	Root->AddChildToVerticalBox(Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_CamTactical")));
	Root->AddChildToVerticalBox(Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_CamOverview")));

	// ---- DIAGNOSTICS -----------------------------------------------------------
	AddLine(Tree, Root, TEXT("Txt_DiagHeader"), TEXT("DIAGNOSTICS"));
	AddLine(Tree, Root, TEXT("Txt_DiagStation"), TEXT("Station: —"));
	AddLine(Tree, Root, TEXT("Txt_DiagBounds"),  TEXT("Bounds: —"));
	AddLine(Tree, Root, TEXT("Txt_DiagActor"),   TEXT("Selected: —"));

	// 🔑 **Le tre righe vengono dal MODELLO, non riscritte qui.** Sono quelle che
	// `PanelDiagnosticsLinesAreExact` asserisce: una seconda copia nel widget divergerebbe in silenzio, ed
	// e' esattamente il difetto che quel test esiste per impedire.
	const TArray<FString> Declared = URTPlaygroundPanelLibrary::DiagnosticsLines();
	for (int32 I = 0; I < Declared.Num(); ++I)
	{
		AddLine(Tree, Root, *FString::Printf(TEXT("Txt_Declared_%d"), I), Declared[I]);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(PanelBP);
	FKismetEditorUtilities::CompileBlueprint(PanelBP);

	FAssetRegistryModule::AssetCreated(PanelBP);
	PanelBP->MarkPackageDirty();

	const FString FileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, PanelBP, *FileName, SaveArgs))
	{
		UE_LOG(LogRTPlaygroundPanel, Error, TEXT("[PlaygroundPanel] salvataggio fallito: %s"), RTPanelPackage);
		return 1;
	}

	UE_LOG(LogRTPlaygroundPanel, Display,
		TEXT("[PlaygroundPanel] creato %s — %d widget, %d righe DIAGNOSTICS dal modello."),
		RTPanelAsset, Tree->RootWidget ? Root->GetChildrenCount() : 0, Declared.Num());
	UE_LOG(LogRTPlaygroundPanel, Display,
		TEXT("[PlaygroundPanel] ⛔ il GRAFO non e' generato: gli eventi vanno cablati in UMG sulle funzioni di URTPlaygroundPanelLibrary."));
	return 0;
}
