#include "SRTLauncherScenarioPanel.h"

#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "RTScenarioPreviewSubsystem.h"
#include "Editor.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "RTLauncherScenarioPanel"

namespace
{
	/**
	 * Toglie l'anteprima di scenario dal viewport, se ce n'e' una.
	 *
	 * Funzione libera e non membro: il pannello non possiede l'anteprima — la chiede al sottosistema, che
	 * la possiede per tutta la vita dell'editor. Un membro suggerirebbe una proprieta' che non c'e'.
	 */
	void ClearScenarioPreview()
	{
		if (URTScenarioPreviewSubsystem* Preview = GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr)
		{
			Preview->ClearPreview();
		}
	}

	/** Margine unico per le righe del pannello: cambiarlo in un posto solo evita una griglia che balla. */
	constexpr float RowPadding = 4.0f;
}

void SRTLauncherScenarioPanel::Construct(const FArguments&)
{
	// Il vocabolario arriva dai file. La prima voce e' il «nessun filtro», ed e' una stringa VUOTA perche'
	// e' esattamente cio' che `ListIds` intende per «non restringere»: una parola speciale tipo "(tutti)"
	// andrebbe tradotta prima di ogni chiamata, e la traduzione e' il posto dove ci si dimentica un ramo.
	TagOptions.Add(MakeShared<FString>());

	// ⚠️ `KnownTag` e non `Tag`: `SWidget::Tag` esiste (un `FName` di debug), e una variabile locale che lo
	// nasconde e' un `C4458` — che qui e' un errore, non un avviso.
	for (const FString& KnownTag : URTScenarioIndex::ListTags())
	{
		TagOptions.Add(MakeShared<FString>(KnownTag));
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		// --- filtri -------------------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, RowPadding, 0.0f)
			[
				SAssignNew(FilterABox, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TagOptions)
				.OnGenerateWidget(this, &SRTLauncherScenarioPanel::OnGenerateTagOption)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewValue, ESelectInfo::Type)
				{
					FilterA = NewValue.IsValid() ? *NewValue : FString();
					RefreshList();
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FilterA.IsEmpty() ? LOCTEXT("AnyTagA", "tag: tutti") : FText::FromString(FilterA); })
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(FilterBBox, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TagOptions)
				.OnGenerateWidget(this, &SRTLauncherScenarioPanel::OnGenerateTagOption)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewValue, ESelectInfo::Type)
				{
					FilterB = NewValue.IsValid() ? *NewValue : FString();
					RefreshList();
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FilterB.IsEmpty() ? LOCTEXT("AnyTagB", "e tag: tutti") : FText::FromString(FilterB); })
				]
			]
		]

		// --- ricerca ------------------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding, 0.0f, RowPadding, RowPadding)
		[
			SAssignNew(SearchBox, SSearchBox)
			.HintText(LOCTEXT("SearchHint", "cerca fra gli scenari filtrati"))
			.OnTextChanged(this, &SRTLauncherScenarioPanel::OnSearchTextChanged)
		]

		// --- elenco -------------------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(RowPadding, 0.0f, RowPadding, RowPadding)
		[
			SNew(SBorder)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FString>>)
					.ListItemsSource(&VisibleItems)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SRTLauncherScenarioPanel::OnGenerateScenarioRow)
					.OnSelectionChanged(this, &SRTLauncherScenarioPanel::OnScenarioSelected)
				]

				// Il messaggio del vuoto occupa il posto della lista solo quando la lista e' vuota, e dice
				// QUALE causa (#1705 AC): allargare i filtri e svuotare la ricerca sono due gesti diversi.
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(RowPadding)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text_Lambda([this]() { return FRTLauncherScenarioBrowser::DescribeEmptyState(ListState); })
					.Visibility_Lambda([this]()
					{
						return ListState == ERTLauncherListState::Populated ? EVisibility::Collapsed : EVisibility::Visible;
					})
				]
			]
		]

		// --- readout ------------------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding)
		[
			SNew(SBox)
			.MinDesiredHeight(96.0f)
			[
				SNew(SBorder)
				.Padding(RowPadding)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return SelectedId.IsEmpty()
								? LOCTEXT("NoSelection", "Nessuno scenario selezionato.")
								: FText::FromString(SelectedId);
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text_Lambda([this]()
						{
							if (!ReadoutError.IsEmpty())
							{
								return FText::Format(LOCTEXT("ReadoutError", "non leggibile: {0}"), FText::FromString(ReadoutError));
							}

							return FText::FromString(FString::Join(ReadoutLines, TEXT("\n")));
						})
					]
				]
			]
		]
	];

	RefreshList();
}

void SRTLauncherScenarioPanel::RefreshList()
{
	// L'elenco filtrato viene dall'indice, sempre. Ricalcolarlo a ogni cambio di filtro costa una lettura
	// dell'indice, non l'apertura degli scenari: e' la ragione per cui `ReadHeader` esiste.
	const TArray<FString> Filtered = URTScenarioIndex::ListIds(FilterA, FilterB);
	FilteredCount = Filtered.Num();

	const TArray<FString> Visible = FRTLauncherScenarioBrowser::ApplySearch(Filtered, SearchText);
	ListState = FRTLauncherScenarioBrowser::Classify(FilteredCount, Visible.Num());

	VisibleItems.Reset(Visible.Num());
	for (const FString& Id : Visible)
	{
		VisibleItems.Add(MakeShared<FString>(Id));
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}

	// ⛔ `SelectedId` e `ReadoutLines` NON si toccano qui. I filtri sono una lente: chi restringe per
	// cercare un secondo scenario da confrontare non deve perdere il primo.
}

void SRTLauncherScenarioPanel::RefreshReadout()
{
	ReadoutLines.Reset();
	ReadoutError.Reset();

	if (SelectedId.IsEmpty())
	{
		// Nessuna selezione, nessuna anteprima: lasciare a schermo lo scenario di prima mostrerebbe qualcosa
		// che il pannello non sta piu' dicendo.
		ClearScenarioPreview();
		return;
	}

	if (!Authoring.IsValid())
	{
		Authoring.Reset(URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage()));
	}

	if (!Authoring.IsValid())
	{
		ReadoutError = TEXT("la facade d'authoring non e' disponibile");
		return;
	}

	FString OpenError;
	if (Authoring->OpenById(SelectedId, OpenError) != ERTScenarioAuthoringResult::Success)
	{
		// Uno scenario illeggibile resta ELENCATO e lo dichiara: l'indice lo trova per header, e farlo
		// sparire dalla lista renderebbe invisibile proprio il file da riparare.
		ClearScenarioPreview();
		ReadoutError = OpenError;
		return;
	}

	ReadoutLines = FRTLauncherScenarioBrowser::BuildReadout(Authoring->GetSummary(), Authoring->ListUnits());

	// L'anteprima nel viewport (#1753): si posa mentre la facade e' ANCORA aperta, perche' e' da li' che
	// arrivano l'arena canonica e le unita'. Il pannello non possiede una sessione — la chiude due righe
	// sotto come prima — e l'anteprima tiene il risultato, non la facade.
	if (URTScenarioPreviewSubsystem* Preview = GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr)
	{
		if (Preview->ShowScenario(Authoring.Get()))
		{
			// Su quale piano si sta guardando: senza, due celle con lo stesso X/Y e Layer diverso si
			// leggono come una sola.
			ReadoutLines.Add(FString::Printf(TEXT("Viewport: %s"), *Preview->GetLayerReadout()));
		}
	}

	// Chiusura esplicita: la facade non e' una sessione d'authoring aperta dal launcher — quella e' #1682.
	// Tenerla aperta qui significherebbe che il pannello possiede uno stato che #1682 dovra' possedere.
	Authoring->Close();
}

TSharedRef<ITableRow> SRTLauncherScenarioPanel::OnGenerateScenarioRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew(STextBlock).Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty())
		];
}

TSharedRef<SWidget> SRTLauncherScenarioPanel::OnGenerateTagOption(TSharedPtr<FString> Option) const
{
	return SNew(STextBlock).Text(TagOptionLabel(Option));
}

FText SRTLauncherScenarioPanel::TagOptionLabel(const TSharedPtr<FString>& Option)
{
	if (!Option.IsValid() || Option->IsEmpty())
	{
		return LOCTEXT("AnyTagOption", "(tutti)");
	}

	return FText::FromString(*Option);
}

void SRTLauncherScenarioPanel::OnScenarioSelected(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	// ⚠️ `Direct` esce: e' la selezione che `SListView` rifa' da sola dopo un `RequestListRefresh`, e
	// trattarla come una scelta dell'utente farebbe riaprire lo scenario a ogni digitazione nella ricerca.
	if (SelectInfo == ESelectInfo::Direct)
	{
		return;
	}

	if (!Item.IsValid())
	{
		return;
	}

	SelectedId = *Item;
	RefreshReadout();
}

void SRTLauncherScenarioPanel::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText.ToString();
	RefreshList();
}

#undef LOCTEXT_NAMESPACE
