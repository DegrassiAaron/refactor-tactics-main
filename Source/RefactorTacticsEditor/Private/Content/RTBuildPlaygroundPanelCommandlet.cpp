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

	/**
	 * 🔑 **Ogni widget diventa una VARIABILE.** Senza questo il grafo non ha maniglia: un
	 * `Variables|WBP_RT_GrayKitPlayground|GetTxt_MapState` semplicemente non esiste, e cablare un testo
	 * e' impossibile. Misurato il 2026-09-02: il pannello generato aveva variabili solo per bottoni e
	 * combo, e i cinque `Txt_*` che il grafo scrive hanno dovuto essere convertiti a mano.
	 */
	void MakeEveryWidgetAVariable(UWidgetTree* Tree)
	{
		Tree->ForEachWidget([](UWidget* Widget)
		{
			if (Widget)
			{
				Widget->bIsVariable = true;
			}
		});
	}

	/**
	 * Le voci delle due combo, **dal modello**.
	 *
	 * ⛔ **Perche' qui e non nel grafo**, che sarebbe stata la sede naturale: `ComboBox|AddOption` e
	 * `ComboBox|ClearOptions` esistono in DUE varianti — `UComboBoxKey` e `UComboBoxString` — con lo
	 * **stesso** `type_id`, e il DSL del toolset Blueprint prende la prima:
	 * *«Could not connect pin Cmb_Station to self»*. Il C++ non ha quell'ambiguita'.
	 *
	 * ⚠️ Restano comunque **derivate**, non incise: `GetStations()` delega alla planimetria e
	 * `GetFacingOptions()` a `StaticEnum<ERTHexDirection>()`. Cambiare una station qui non serve.
	 */
	bool ApplyComboOptionsFromModel(UWidgetTree* Tree, int32& OutStations, int32& OutFacings)
	{
		UComboBoxString* StationCombo = Cast<UComboBoxString>(Tree->FindWidget(TEXT("Cmb_Station")));
		UComboBoxString* FacingCombo  = Cast<UComboBoxString>(Tree->FindWidget(TEXT("Cmb_Facing")));
		if (!StationCombo || !FacingCombo)
		{
			return false;
		}

		// ⚠️ `DefaultOptions` e' **private** in UE 5.8: si passa dall'API pubblica, non dal campo.
		StationCombo->ClearOptions();
		for (const FRTPlaygroundStationInfo& Station : URTPlaygroundPanelLibrary::GetStations())
		{
			StationCombo->AddOption(Station.Name);
		}
		FacingCombo->ClearOptions();
		for (const FString& Option : URTPlaygroundPanelLibrary::GetFacingOptions())
		{
			FacingCombo->AddOption(Option);
		}

		OutStations = StationCombo->GetOptionCount();
		OutFacings  = FacingCombo->GetOptionCount();
		return true;
	}

	/** Il salvataggio, uguale per la generazione e per il refresh. */
	bool SavePanelPackage(UPackage* Package, UBlueprint* PanelBP)
	{
		const FString FileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, PanelBP, *FileName, SaveArgs);
	}
}

int32 URTBuildPlaygroundPanelCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	const bool bForce = Switches.Contains(TEXT("Force"));
	const bool bRefreshOptions = Switches.Contains(TEXT("RefreshOptions"));

	UEditorUtilityWidgetBlueprint* Existing = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, RTPanelPackage);

	// 🔑 **Aggiornamento NON distruttivo.** Rigenerare cancella il grafo autorato (`RTPlaygroundPanelGraph.dsl`);
	// questa via tocca soltanto le voci delle combo e i flag di variabile, e il grafo resta dov'e'.
	if (Existing && bRefreshOptions)
	{
		UWidgetTree* ExistingTree = Existing->WidgetTree;
		int32 RefreshedStations = 0;
		int32 RefreshedFacings = 0;
		if (!ExistingTree || !ApplyComboOptionsFromModel(ExistingTree, RefreshedStations, RefreshedFacings))
		{
			UE_LOG(LogRTPlaygroundPanel, Error,
				TEXT("[PlaygroundPanel] refresh impossibile: Cmb_Station o Cmb_Facing non trovate in %s."), RTPanelAsset);
			return 1;
		}
		MakeEveryWidgetAVariable(ExistingTree);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Existing);
		FKismetEditorUtilities::CompileBlueprint(Existing);
		Existing->MarkPackageDirty();
		if (!SavePanelPackage(Existing->GetOutermost(), Existing))
		{
			UE_LOG(LogRTPlaygroundPanel, Error, TEXT("[PlaygroundPanel] salvataggio fallito: %s"), RTPanelPackage);
			return 1;
		}
		UE_LOG(LogRTPlaygroundPanel, Display,
			TEXT("[PlaygroundPanel] refresh di %s — %d station, %d direzioni dal modello. Grafo NON toccato."),
			RTPanelAsset, RefreshedStations, RefreshedFacings);
		return 0;
	}

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

	// Lo scheletro nasce gia' pronto per il grafo: voci dal modello e ogni widget raggiungibile.
	int32 StationOptions = 0;
	int32 FacingOptions = 0;
	if (!ApplyComboOptionsFromModel(Tree, StationOptions, FacingOptions))
	{
		UE_LOG(LogRTPlaygroundPanel, Error, TEXT("[PlaygroundPanel] combo non trovate subito dopo averle costruite."));
		return 1;
	}
	MakeEveryWidgetAVariable(Tree);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(PanelBP);
	FKismetEditorUtilities::CompileBlueprint(PanelBP);

	FAssetRegistryModule::AssetCreated(PanelBP);
	PanelBP->MarkPackageDirty();

	if (!SavePanelPackage(Package, PanelBP))
	{
		UE_LOG(LogRTPlaygroundPanel, Error, TEXT("[PlaygroundPanel] salvataggio fallito: %s"), RTPanelPackage);
		return 1;
	}

	UE_LOG(LogRTPlaygroundPanel, Display,
		TEXT("[PlaygroundPanel] creato %s — %d widget, %d righe DIAGNOSTICS, %d station, %d direzioni dal modello."),
		RTPanelAsset, Tree->RootWidget ? Root->GetChildrenCount() : 0, Declared.Num(), StationOptions, FacingOptions);
	UE_LOG(LogRTPlaygroundPanel, Display,
		TEXT("[PlaygroundPanel] ⛔ il GRAFO non e' generato: si riapplica con `write_graph_dsl` da RTPlaygroundPanelGraph.dsl."));
	return 0;
}
