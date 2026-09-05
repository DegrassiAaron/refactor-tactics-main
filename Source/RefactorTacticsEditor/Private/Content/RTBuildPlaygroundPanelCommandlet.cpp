#include "Content/RTBuildPlaygroundPanelCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
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
#include "World/RTGrayboxUnitFacingFixture.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTPlaygroundPanel, Log, All);

namespace
{
	const TCHAR* RTPanelPackage = TEXT("/Game/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground");
	const TCHAR* RTPanelAsset   = TEXT("WBP_RT_GrayKitPlayground");

	/**
	 * Le tre misure del pannello.
	 *
	 * 🔴 **Il default di `UTextBlock` e' 24**, e a 24 il pannello era illeggibile: quattro righe
	 * riempivano l'altezza e il titolo non si distingueva dal corpo, perche' erano identici. La
	 * gerarchia la fa la **differenza** fra le dimensioni, non la dimensione in se'.
	 */
	constexpr int32 FontTitle   = 14;
	constexpr int32 FontHeader  = 11;
	constexpr int32 FontBody    = 10;

	/** Un testo nel box, con nome STABILE: e' la maniglia con cui il grafo lo raggiungera'. */
	UTextBlock* AddLine(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, const FString& Text,
		int32 FontSize = FontBody)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
		Block->SetText(FText::FromString(Text));
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Box->AddChildToVerticalBox(Block);
		return Block;
	}

	/**
	 * Un pulsante **con l'etichetta dentro**.
	 *
	 * 🔴 Senza figlio di testo un `UButton` e' una **barra grigia muta**: il pannello ne mostrava sei, e
	 * il verdetto di chi lo apriva e' stato *«non c'e' molto selezionabile»*. C'era: non si leggeva.
	 */
	UButton* AddButton(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, const FString& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("%s_Label"), Name)));
		Text->SetText(FText::FromString(Label));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontBody;
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		Button->AddChild(Text);
		Box->AddChildToVerticalBox(Button);
		return Button;
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
	 * 🔴 **`AddOption` NON persiste.** Misurato il 2026-09-02: scrive soltanto `Options`, che e' un
	 * `TArray<TSharedPtr<FString>>` **transiente** e non una `UPROPERTY`. Il campo salvato e'
	 * `DefaultOptions`, da cui `PostLoad` ricostruisce `Options` — e `DefaultOptions` e' **private**,
	 * senza setter. Un commandlet che chiamasse `AddOption` scriverebbe un `.uasset` con le combo
	 * VUOTE e stamperebbe comunque «8 voci», perche' rileggerebbe cio' che ha appena messo in memoria:
	 * una misura che conferma se stessa. Il rosso di `PanelComboOptionsComeFromTheModel` e' stato
	 * questo. ∴ si scrive la `UPROPERTY` per riflessione, che ignora l'access specifier.
	 */
	bool SetPersistedComboOptions(UComboBoxString* Combo, const TArray<FString>& Values)
	{
		static const FName DefaultOptionsName(TEXT("DefaultOptions"));
		FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(UComboBoxString::StaticClass(), DefaultOptionsName);
		FStrProperty* InnerProp = ArrayProp ? CastField<FStrProperty>(ArrayProp->Inner) : nullptr;
		if (!Combo || !ArrayProp || !InnerProp)
		{
			return false;
		}

		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Combo));
		Helper.EmptyValues();
		for (const FString& Value : Values)
		{
			InnerProp->SetPropertyValue(Helper.GetElementPtr(Helper.AddValue()), Value);
		}

		// E l'array runtime, cosi' il widget e' coerente anche prima del prossimo `PostLoad`.
		Combo->ClearOptions();
		for (const FString& Value : Values)
		{
			Combo->AddOption(Value);
		}
		return true;
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

		TArray<FString> StationNames;
		for (const FRTPlaygroundStationInfo& Station : URTPlaygroundPanelLibrary::GetStations())
		{
			// Il formato vive nella libreria: qui una seconda copia divergerebbe dal test che la sorveglia.
			StationNames.Add(URTPlaygroundPanelLibrary::StationOptionLabel(Station));
		}
		if (!SetPersistedComboOptions(StationCombo, StationNames) ||
			!SetPersistedComboOptions(FacingCombo, URTPlaygroundPanelLibrary::GetFacingOptions()))
		{
			return false;
		}

		OutStations = StationNames.Num();
		OutFacings  = URTPlaygroundPanelLibrary::GetFacingOptions().Num();
		return true;
	}

	/**
	 * 🔑 **I controlli che la DoD di #1993 chiede e che lo scheletro non aveva.** Idempotente: crea solo
	 * cio' che manca, quindi gira sia su un albero appena costruito sia su uno gia' salvato — ed e' per
	 * questo che `-RefreshOptions` puo' aggiungerli **senza rigenerare**, che cancellerebbe il grafo
	 * autorato (`RTPlaygroundPanelGraph.dsl`).
	 *
	 * 🔴 **Perche' mancavano, e perche' nessuno se n'era accorto.** Due criteri della DoD —
	 * *«i cinque parametri si leggono e si modificano dal pannello»* e *«i tre toggle accendono e
	 * spengono guida, label e bounds»* — non avevano **nessun** widget: misurato il 2026-09-05 sul
	 * `.uasset`, zero `SpinBox`, zero `CheckBox`, zero `Slider`. Il solo parametro cablato era `Facing`,
	 * e `ApplyFixtureParameters` era una `UFUNCTION` con **zero chiamanti nel grafo**. ⚠️ E
	 * `PanelGraphCallsTheModel` era **verde**: elencava dieci funzioni attese e quella non c'era, quindi
	 * certificava l'assenza invece di trovarla. E' lo stesso difetto di `bLive`, chiuso in questa issue
	 * il 2026-09-02 — dichiarato nel modello, letto da nessuno.
	 *
	 * ⛔ **Un toggle solo, e non tre.** `Chk_Labels` esiste perche' le etichette sono `ATextRenderActor`
	 * e la classe e' un aggancio stabile. Guida e bounds no: vedi `SetStationLabelsVisible`, e la
	 * integration request verso #1991.
	 */
	void EnsureFixtureAndViewControls(UWidgetBlueprint* Blueprint)
	{
		UWidgetTree* Tree = Blueprint ? Blueprint->WidgetTree : nullptr;
		UVerticalBox* Root = Tree ? Cast<UVerticalBox>(Tree->RootWidget) : nullptr;
		if (!Root)
		{
			return;
		}

		// Inserisce subito DOPO un widget noto, invece di appendere in fondo: un controllo del fixture in
		// coda a `DIAGNOSTICS` sarebbe cablato e comunque non si troverebbe.
		auto InsertAfter = [Root](const TCHAR* AnchorName, UWidget* NewChild)
		{
			// Se l'ancora non c'e', si appende in fondo: un controllo in coda si trova male, ma perderlo
			// sarebbe peggio.
			int32 Index = Root->GetChildrenCount();
			for (int32 I = 0; I < Root->GetChildrenCount(); ++I)
			{
				const UWidget* Child = Root->GetChildAt(I);
				if (Child && Child->GetFName() == FName(AnchorName))
				{
					Index = I + 1;
					break;
				}
			}
			Root->InsertChildAt(Index, NewChild);
		};

		// ---- FIXTURE: i quattro parametri numerici -------------------------------
		if (!Tree->FindWidget(TEXT("Spn_BodyRadius")))
		{
			// ⛔ I valori iniziali vengono dal CDO, non da letterali: e' la stessa disciplina di
			// `ResetFixture`. Incidere `60 / 180 / 120 / 70` qui vorrebbe dire che il giorno in cui il
			// fixture cambia default il pannello nasce mostrando valori che non lo sono piu'.
			const ARTGrayboxUnitFacingFixture* Defaults = GetDefault<ARTGrayboxUnitFacingFixture>();
			struct FParamRow { const TCHAR* SpinName; const TCHAR* LabelName; const TCHAR* Caption; float Value; };
			const FParamRow Rows[] = {
				{ TEXT("Spn_BodyRadius"),   TEXT("Txt_BodyRadiusLabel"),   TEXT("Body Radius"),   Defaults->BodyRadius },
				{ TEXT("Spn_BodyHeight"),   TEXT("Txt_BodyHeightLabel"),   TEXT("Body Height"),   Defaults->BodyHeight },
				{ TEXT("Spn_FaceHeight"),   TEXT("Txt_FaceHeightLabel"),   TEXT("Face Height"),   Defaults->FaceHeight },
				{ TEXT("Spn_MarkerLength"), TEXT("Txt_MarkerLengthLabel"), TEXT("Marker Length"), Defaults->MarkerLength },
			};

			// Si inseriscono in ordine dopo `Btn_ResetFixture`, che chiude la sezione FIXTURE. Ogni riga
			// entra dopo la precedente, cosi' l'ordine dichiarato qui e' quello che si legge a schermo.
			const TCHAR* Anchor = TEXT("Btn_ResetFixture");
			for (const FParamRow& Row : Rows)
			{
				UTextBlock* Caption = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Row.LabelName));
				Caption->SetText(FText::FromString(Row.Caption));
				FSlateFontInfo CaptionFont = Caption->GetFont();
				CaptionFont.Size = FontBody;
				Caption->SetFont(CaptionFont);
				InsertAfter(Anchor, Caption);

				USpinBox* Spin = Tree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), FName(Row.SpinName));
				Spin->SetValue(Row.Value);
				// ⚠️ Un raggio o un'altezza negativi non sono un caso da gestire nel grafo: si vietano qui.
				Spin->SetMinValue(0.f);
				InsertAfter(Row.LabelName, Spin);
				Anchor = Row.SpinName;
			}
		}

		// ---- VIEW: il toggle delle etichette -------------------------------------
		if (!Tree->FindWidget(TEXT("Chk_Labels")))
		{
			UCheckBox* Check = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("Chk_Labels"));
			// 🔴 Una casella senza etichetta e' muta come un `UButton` senza figlio di testo: e' il
			// difetto che `U41` ha letto come *«non c'e' molto selezionabile»*.
			UTextBlock* Caption = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
				TEXT("Txt_LabelsToggleLabel"));
			Caption->SetText(FText::FromString(TEXT("Labels")));
			FSlateFontInfo CaptionFont = Caption->GetFont();
			CaptionFont.Size = FontBody;
			Caption->SetFont(CaptionFont);
			Check->SetContent(Caption);
			// Le etichette nascono VISIBILI nella scena, quindi la casella nasce spuntata: un toggle che
			// parte dallo stato opposto a cio' che si vede e' un controllo che mente prima di essere usato.
			Check->SetIsChecked(true);
			InsertAfter(TEXT("Txt_ViewHeader"), Check);
		}
	}

	/**
	 * 🔑 **La presentazione, riconciliata su un albero QUALUNQUE.** Idempotente: gira sia su uno appena
	 * costruito sia su uno gia' salvato, e per questo il refresh non ha bisogno di rigenerare — che
	 * cancellerebbe il grafo autorato (`RTPlaygroundPanelGraph.dsl`).
	 *
	 * ⚠️ Tutto cio' che sta qui e' cio' che la seduta `U41` ha visto e nessun test guardava: le tre righe
	 * di `DIAGNOSTICS` in doppia copia, sei pulsanti muti, e un font unico per titolo e corpo.
	 */
	void ReconcilePresentation(UWidgetBlueprint* Blueprint)
	{
		UWidgetTree* Tree = Blueprint ? Blueprint->WidgetTree : nullptr;
		if (!Tree)
		{
			return;
		}

		// 1. La copia di troppo. `Txt_Declared_*` diceva le stesse tre righe che ora scrive il grafo:
		//    nessuna delle due sbagliata, ed e' per questo che il difetto si vede solo a occhio.
		TArray<UWidget*> Doomed;
		Tree->ForEachWidget([&Doomed](UWidget* Widget)
		{
			if (Widget && Widget->GetName().StartsWith(TEXT("Txt_Declared_")))
			{
				Doomed.Add(Widget);
			}
		});
		for (UWidget* Widget : Doomed)
		{
			// 🔴 **Togliere il widget non basta.** Ogni widget-variabile ha un GUID in
			// `WidgetVariableNameToGuidMap`, che serve a riparare i riferimenti esterni dopo un rename.
			// Lasciandolo, `CompileBlueprint` **ASSERISCE**: *«Variable [Txt_Declared_1] was deleted but
			// still has a GUID referenced by WidgetBlueprint»* — misurato, il commandlet crashava.
			const FName DeletedName = Widget->GetFName();
			Tree->RemoveWidget(Widget);
			Blueprint->WidgetVariableNameToGuidMap.Remove(DeletedName);
		}

		// 2. Le tre righe, dal modello, in UNA sola serie.
		const TArray<FString> Declared = URTPlaygroundPanelLibrary::DiagnosticsLines();
		const TCHAR* DiagNames[3] = { TEXT("Txt_DiagStation"), TEXT("Txt_DiagBounds"), TEXT("Txt_DiagActor") };
		for (int32 I = 0; I < 3; ++I)
		{
			if (UTextBlock* Block = Cast<UTextBlock>(Tree->FindWidget(FName(DiagNames[I]))))
			{
				Block->SetText(FText::FromString(Declared.IsValidIndex(I) ? Declared[I] : TEXT("—")));
			}
		}

		// 3. La gerarchia dei corpi. Il default e' 24 per tutti, e a quel punto non c'e' gerarchia.
		auto SetSize = [Tree](const TCHAR* Name, int32 Size)
		{
			if (UTextBlock* Block = Cast<UTextBlock>(Tree->FindWidget(FName(Name))))
			{
				FSlateFontInfo Font = Block->GetFont();
				Font.Size = Size;
				Block->SetFont(Font);
			}
		};
		SetSize(TEXT("Txt_Title"), FontTitle);
		for (const TCHAR* Header : { TEXT("Txt_MapState"), TEXT("Txt_StationHeader"),
			TEXT("Txt_FixtureHeader"), TEXT("Txt_ViewHeader"), TEXT("Txt_DiagHeader") })
		{
			SetSize(Header, FontHeader);
		}
		for (const TCHAR* Body : { TEXT("Txt_FixtureName"), TEXT("Txt_DiagStation"),
			TEXT("Txt_DiagBounds"), TEXT("Txt_DiagActor"),
			// Le didascalie dei controlli aggiunti dopo lo scheletro: elencate qui perche' su un albero
			// gia' salvato nessuno le ha mai portate giu' da 24, e a 24 non c'e' gerarchia.
			TEXT("Txt_BodyRadiusLabel"), TEXT("Txt_BodyHeightLabel"), TEXT("Txt_FaceHeightLabel"),
			TEXT("Txt_MarkerLengthLabel"), TEXT("Txt_LabelsToggleLabel") })
		{
			SetSize(Body, FontBody);
		}

		// 4. I pulsanti muti. Un `UButton` senza figlio di testo e' una barra grigia, e sei barre grigie
		//    si leggono come *«non c'e' molto selezionabile»*.
		// ⚠️ I bracci NON si scrivono a mano nell'etichetta: vengono da `CameraPresetArmLengths()`, che e'
		// la stessa fonte che il grafo consuma. Un numero copiato qui direbbe `450` mentre la camera va a
		// `600`, e sarebbe il pannello a mentire — con l'aria di funzionare.
		const TArray<float> Arms = URTPlaygroundPanelLibrary::CameraPresetArmLengths();
		auto ArmLabel = [&Arms](const TCHAR* Name, int32 Index) -> FString
		{
			return Arms.IsValidIndex(Index)
				? FString::Printf(TEXT("%s  %.0f"), Name, Arms[Index])
				: FString::Printf(TEXT("%s  (preset assente)"), Name);
		};

		struct FButtonLabel { const TCHAR* Name; FString Label; };
		const FButtonLabel Labels[] = {
			{ TEXT("Btn_Focus"),         TEXT("Focus Station") },
			{ TEXT("Btn_SelectFixture"), TEXT("Select Fixture") },
			{ TEXT("Btn_ResetFixture"),  TEXT("Reset Fixture") },
			{ TEXT("Btn_CamClose"),      ArmLabel(TEXT("Close"),    0) },
			{ TEXT("Btn_CamTactical"),   ArmLabel(TEXT("Tactical"), 1) },
			{ TEXT("Btn_CamOverview"),   ArmLabel(TEXT("Overview"), 2) },
		};
		for (const FButtonLabel& Entry : Labels)
		{
			UButton* Button = Cast<UButton>(Tree->FindWidget(FName(Entry.Name)));
			if (!Button)
			{
				continue;
			}
			UTextBlock* Label = nullptr;
			for (int32 I = 0; I < Button->GetChildrenCount(); ++I)
			{
				if (UTextBlock* Existing = Cast<UTextBlock>(Button->GetChildAt(I)))
				{
					Label = Existing;
					break;
				}
			}
			if (!Label)
			{
				Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
					FName(*FString::Printf(TEXT("%s_Label"), Entry.Name)));
				Button->AddChild(Label);
			}
			Label->SetText(FText::FromString(Entry.Label));
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = FontBody;
			Label->SetFont(Font);
			Label->SetJustification(ETextJustify::Center);
		}
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
		EnsureFixtureAndViewControls(Existing);
		ReconcilePresentation(Existing);
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
	AddLine(Tree, Root, TEXT("Txt_Title"), TEXT("GRAY KIT PLAYGROUND    v0.1"), FontTitle);
	AddLine(Tree, Root, TEXT("Txt_MapState"), TEXT("<mappa>  —  <Ready/Error>"), FontHeader);

	// ---- STATION ---------------------------------------------------------------
	AddLine(Tree, Root, TEXT("Txt_StationHeader"), TEXT("STATION"), FontHeader);
	UComboBoxString* StationCombo =
		Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("Cmb_Station"));
	// ⛔ Le otto voci NON sono incise qui: il grafo le prende da `GetStations()`, che delega alla
	// planimetria. Incidere «01..08» sarebbe la seconda copia che `#1459` ha gia' fatto pagare.
	Root->AddChildToVerticalBox(StationCombo);
	AddButton(Tree, Root, TEXT("Btn_Focus"), TEXT("Focus Station"));

	// ---- FIXTURE ---------------------------------------------------------------
	AddLine(Tree, Root, TEXT("Txt_FixtureHeader"), TEXT("FIXTURE"), FontHeader);
	AddLine(Tree, Root, TEXT("Txt_FixtureName"), TEXT("<nessun fixture selezionato>"));
	// ⛔ Le sei voci le mette `GetFacingOptions()`, derivate da `StaticEnum<ERTHexDirection>()`.
	Root->AddChildToVerticalBox(
		Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("Cmb_Facing")));
	AddButton(Tree, Root, TEXT("Btn_SelectFixture"), TEXT("Select Fixture"));
	AddButton(Tree, Root, TEXT("Btn_ResetFixture"),  TEXT("Reset Fixture"));

	// ---- VIEW ------------------------------------------------------------------
	AddLine(Tree, Root, TEXT("Txt_ViewHeader"), TEXT("VIEW"), FontHeader);
	AddButton(Tree, Root, TEXT("Btn_CamClose"),    TEXT("Close"));
	AddButton(Tree, Root, TEXT("Btn_CamTactical"), TEXT("Tactical"));
	AddButton(Tree, Root, TEXT("Btn_CamOverview"), TEXT("Overview"));

	// ---- DIAGNOSTICS -----------------------------------------------------------
	// 🔑 **Le tre righe vengono dal MODELLO**, sia qui (cosi' il designer mostra il vero) sia a runtime
	// (`EventConstruct` le rilegge). 🔴 **Erano SEI**: c'era anche un blocco `Txt_Declared_*` con le
	// stesse tre righe, e da quando il grafo riempie queste il pannello le mostrava **due volte**. Una
	// sola serie: due copie della stessa verita' divergono, e intanto si leggono come un difetto.
	AddLine(Tree, Root, TEXT("Txt_DiagHeader"), TEXT("DIAGNOSTICS"), FontHeader);
	const TArray<FString> Declared = URTPlaygroundPanelLibrary::DiagnosticsLines();
	const TCHAR* DiagNames[3] = { TEXT("Txt_DiagStation"), TEXT("Txt_DiagBounds"), TEXT("Txt_DiagActor") };
	for (int32 I = 0; I < 3; ++I)
	{
		AddLine(Tree, Root, DiagNames[I], Declared.IsValidIndex(I) ? Declared[I] : TEXT("—"));
	}

	// Lo scheletro nasce gia' pronto per il grafo: voci dal modello e ogni widget raggiungibile.
	int32 StationOptions = 0;
	int32 FacingOptions = 0;
	if (!ApplyComboOptionsFromModel(Tree, StationOptions, FacingOptions))
	{
		UE_LOG(LogRTPlaygroundPanel, Error, TEXT("[PlaygroundPanel] combo non trovate subito dopo averle costruite."));
		return 1;
	}
	EnsureFixtureAndViewControls(PanelBP);
	ReconcilePresentation(PanelBP);
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
