#include "SRTLauncherScenarioPanel.h"

#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioIndex.h"
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
	/** Margine unico per le righe del pannello: cambiarlo in un posto solo evita una griglia che balla. */
	constexpr float RowPadding = 4.0f;
}

void SRTLauncherScenarioPanel::Construct(const FArguments&)
{
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
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TagOptions)
				.OnGenerateWidget(this, &SRTLauncherScenarioPanel::OnGenerateTagOption)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewValue, ESelectInfo::Type)
				{
					FilterA = NewValue.IsValid() ? *NewValue : FString();
					RefreshFilters();
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FilterA.IsEmpty() ? LOCTEXT("AnyTagA", "tag: tutti") : FText::FromString(FilterA); })
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TagOptions)
				.OnGenerateWidget(this, &SRTLauncherScenarioPanel::OnGenerateTagOption)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewValue, ESelectInfo::Type)
				{
					FilterB = NewValue.IsValid() ? *NewValue : FString();
					RefreshFilters();
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
			SNew(SSearchBox)
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
					// ⚠️ Collassa, non si limita a restare vuota: un riquadro alto e bianco sopra la
					// spiegazione la spinge in fondo al pannello, dove non la si legge. Il messaggio deve
					// PRENDERE il posto della lista, non aggiungersi sotto di essa.
					.Visibility_Lambda([this]()
					{
						return ListState == ERTLauncherListState::Populated ? EVisibility::Visible : EVisibility::Collapsed;
					})
				]

				// Il messaggio del vuoto dice QUALE causa (#1705 AC): allargare i filtri, svuotare la
				// ricerca e riparare il corpus sono tre gesti diversi.
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
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

	RefreshFilters();
}

void SRTLauncherScenarioPanel::RefreshFilters()
{
	// Il vocabolario arriva dai file. La prima voce e' il «nessun filtro», ed e' una stringa VUOTA perche'
	// e' esattamente cio' che `ListIds` intende per «non restringere»: una parola speciale tipo "(tutti)"
	// andrebbe tradotta prima di ogni chiamata, e la traduzione e' il posto dove ci si dimentica un ramo.
	TagOptions.Reset();
	TagOptions.Add(MakeShared<FString>());

	// ⚠️ `KnownTag` e non `Tag`: `SWidget::Tag` esiste (un `FName` di debug), e una variabile locale che lo
	// nasconde e' un `C4458` — che qui e' un errore, non un avviso.
	for (const FString& KnownTag : URTScenarioIndex::ListTags())
	{
		TagOptions.Add(MakeShared<FString>(KnownTag));
	}

	FilteredIds = URTScenarioIndex::ListIds(FilterA, FilterB);

	RefreshVisible();
}

void SRTLauncherScenarioPanel::RefreshVisible()
{
	const TArray<FString> Visible = FRTLauncherScenarioBrowser::ApplySearch(FilteredIds, SearchText);

	ListState = FRTLauncherScenarioBrowser::Classify(
		FilteredIds.Num(), Visible.Num(), !FilterA.IsEmpty() || !FilterB.IsEmpty());

	VisibleItems.Reset(Visible.Num());
	for (const FString& Id : Visible)
	{
		// Puntatore STABILE per id: `SListView` confronta i selezionati per identita' di puntatore, e una
		// voce rigenerata perde l'evidenziazione pur restando nell'elenco.
		TSharedPtr<FString>& Item = ItemById.FindOrAdd(Id);
		if (!Item.IsValid())
		{
			Item = MakeShared<FString>(Id);
		}

		VisibleItems.Add(Item);
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();

		// ⛔ `SelectedId` NON si tocca qui: i filtri sono una lente, e chi restringe per cercare un secondo
		// scenario da confrontare non deve perdere il primo. Ma la selezione VISIVA va riaffermata, perche'
		// cambiare la sorgente la fa cadere: senza questa riga la riga resta elencata e smette di essere
		// evidenziata, mentre il readout continua a mostrarla — due parti della stessa schermata che si
		// contraddicono.
		if (const TSharedPtr<FString>* Selected = SelectedId.IsEmpty() ? nullptr : ItemById.Find(SelectedId))
		{
			if (VisibleItems.Contains(*Selected))
			{
				// `Direct`: e' una riaffermazione, non una scelta dell'utente. `OnScenarioSelected` la
				// scarta, quindi non rilegge lo scenario da disco a ogni battuta di tasto.
				ListView->SetSelection(*Selected, ESelectInfo::Direct);
			}
		}
	}
}

void SRTLauncherScenarioPanel::ClearSelection()
{
	SelectedId.Reset();
	ReadoutLines.Reset();
	ReadoutError.Reset();
}

void SRTLauncherScenarioPanel::RefreshReadout()
{
	ReadoutLines.Reset();
	ReadoutError.Reset();

	if (SelectedId.IsEmpty())
	{
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
		ReadoutError = OpenError;

		// ⚠️ Chiudere anche qui. Un'apertura fallita NON azzera cio' che la facade aveva gia' aperto, quindi
		// senza questa riga l'invariante «il draft e' chiuso» dipenderebbe dalla `Close()` in fondo al ramo
		// di successo. Il giorno che quella si sposta, un'apertura fallita lascerebbe il draft precedente
		// aperto e il readout mostrerebbe terreno, squadre e conteggi dello scenario SBAGLIATO sotto il
		// nome di quello nuovo — un referto errato, che e' peggio di un referto assente.
		Authoring->Close();
		return;
	}

	ReadoutLines = FRTLauncherScenarioBrowser::BuildReadout(Authoring->GetSummary(), Authoring->ListUnits());

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
	// quella che questo pannello riafferma in `RefreshVisible`. Trattarla come una scelta dell'utente
	// farebbe riaprire lo scenario da disco a ogni digitazione nella ricerca.
	if (SelectInfo == ESelectInfo::Direct)
	{
		return;
	}

	if (!Item.IsValid())
	{
		// Click nel vuoto sotto le righe: `SListView` deseleziona (`bClearSelectionOnClick`) e ce lo
		// comunica con un item nullo. E' un gesto esplicito, quindi la selezione se ne va DAVVERO — e il
		// readout con lei. Ignorarlo lascerebbe una lista senza evidenziazione sopra il referto di uno
		// scenario che l'interfaccia dichiara non selezionato.
		ClearSelection();
		return;
	}

	SelectedId = *Item;
	RefreshReadout();
}

void SRTLauncherScenarioPanel::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText.ToString();

	// Solo la ricerca: nessuna rilettura dell'indice mentre si digita. Vedi `RefreshFilters`.
	RefreshVisible();
}

#undef LOCTEXT_NAMESPACE
