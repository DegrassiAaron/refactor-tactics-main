#include "SRTLauncherScenarioPanel.h"

#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioKnowledge.h"
#include "RTDevSandboxLauncherSubsystem.h"
#include "RTLauncherWorkspace.h"
#include "RTScenarioPreviewSubsystem.h"
#include "Editor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
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
	/** Il sottosistema d'anteprima, o `nullptr` fuori dall'editor. */
	URTScenarioPreviewSubsystem* PreviewSubsystem()
	{
		return GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr;
	}

	/** Il nome leggibile di una velocita', preso dall'enum: il `DisplayName` e' gia' dichiarato li'. */
	FText DescribeSpeed(ERTPlaybackSpeed Velocita)
	{
		if (const UEnum* Tipo = StaticEnum<ERTPlaybackSpeed>())
		{
			return Tipo->GetDisplayNameTextByValue(static_cast<int64>(Velocita));
		}
		return FText::GetEmpty();
	}

	// ⛔ **`DescribePosition` non e' piu' qui.** Viveva in questo anonimo, e il commento nel suo corpo
	// dichiarava che nessun automation test poteva vederla — accanto a un difetto che solo una seduta in
	// Editor aveva potuto trovare. Da #2788 e' `FRTLauncherScenarioBrowser::DescribePlaybackPosition`,
	// insieme alla riga di trasporto che la usa: stesso testo, stesso posto di `DescribeEmptyState`, e un
	// test che le vede entrambe.

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

		// --- prospettiva tecnica (#1754) ----------------------------------------------------------
		//
		// Sta accanto al readout e non fra i filtri: i filtri restringono l'ELENCO, questo cambia cosa il
		// viewport mostra dello scenario gia' scelto. Metterlo lassu' lo farebbe leggere come un terzo
		// criterio di ricerca.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding, 0.0f, RowPadding, RowPadding)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, RowPadding, 0.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("PerspectiveLabel", "View:"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(PerspectiveCombo, SComboBox<TSharedPtr<int32>>)
				.OptionsSource(&PerspectiveOptions)
				.OnGenerateWidget_Lambda([](TSharedPtr<int32> Option)
				{
					return SNew(STextBlock).Text(FRTLauncherScenarioBrowser::DescribePerspective(
						Option.IsValid() ? *Option : RTScenarioKnowledge::OmniscientTeamId));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<int32> NewValue, ESelectInfo::Type)
				{
					if (!NewValue.IsValid())
					{
						return;
					}
					// ⛔ Cambia solo cosa si MOSTRA. Il sottosistema ridisegna velo, marcatori e confine; il
					// draft, lo snapshot, il replay e `PlayerTeamId` non li tocca nessuno.
					if (URTScenarioPreviewSubsystem* Preview = GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr)
					{
						Preview->SetPerspective(*NewValue);
					}
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						// La verita' e' del sottosistema, non di una copia locale: e' lui a possedere la
						// prospettiva, e un valore tenuto anche qui divergerebbe al primo scenario riaperto.
						const URTScenarioPreviewSubsystem* Preview = GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr;
						return FRTLauncherScenarioBrowser::DescribePerspective(
							Preview ? Preview->GetPerspective() : RTScenarioKnowledge::OmniscientTeamId);
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

		// --- sessione (#1682) ------------------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, RowPadding, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("StartSession", "Start Session"))
				.ToolTipText(LOCTEXT("StartSessionTip", "Apre lo scenario selezionato attraverso la facade canonica. Non lo esegue: Run e' un altro gesto."))
				.OnClicked(this, &SRTLauncherScenarioPanel::OnStartSessionClicked)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, RowPadding, 0.0f)
			[
				SNew(SEditableTextBox)
				.HintText(LOCTEXT("NewScenarioHint", "id dello scenario nuovo"))
				.OnTextChanged_Lambda([this](const FText& NewText) { NewScenarioId = NewText.ToString(); })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("NewScenario", "New Scenario"))
				.ToolTipText(LOCTEXT("NewScenarioTip", "Crea uno scenario nuovo dalla facade — CreateScenarioDraft + NewScenario — e ci apre la sessione."))
				.OnClicked(this, &SRTLauncherScenarioPanel::OnNewScenarioClicked)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text_Lambda([this]()
			{
				return SessionMessage.IsEmpty()
					? LOCTEXT("NoSession", "Nessuna sessione avviata.")
					: FText::FromString(SessionMessage);
			})
		]

		// --- piazzamento delle unita' (#2786) ---------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding)
		[
			BuildPlacementRow()
		]

		// --- trasporto del playback (`#1625`) ---------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding)
		[
			BuildTransportRow()
		]

		// --- superfici del workspace, dal registro ----------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(RowPadding)
		[
			BuildSurfaceRow()
		]
	];

	// Le sei velocita' vengono dalla libreria, non da un elenco riscritto qui: l'enum ha gia' un posto in
	// cui e' enumerato, e un secondo elenco divergerebbe alla prima velocita' aggiunta.
	SpeedOptions.Reset();
	for (const ERTPlaybackSpeed Velocita : URTPlaybackSpeedLibrary::AllSpeeds())
	{
		SpeedOptions.Add(MakeShared<ERTPlaybackSpeed>(Velocita));
	}

	RefreshPerspectiveOptions();

	// Il roster si legge subito: la tendina degli eroi non dipende da uno scenario aperto, e lasciarla
	// vuota fino alla prima selezione farebbe sembrare rotto il pannello appena aperto.
	RefreshPlacementOptions();

	RefreshFilters();
}

TSharedRef<SWidget> SRTLauncherScenarioPanel::BuildSurfaceRow()
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);

	// ⛔ Si ITERA il registro. Elencare qui le superfici a mano le farebbe divergere da cio' che
	// `ActivateSurface` sa attivare, e sarebbe la divergenza piu' facile da non notare: il pulsante c'e'.
	for (const FRTLauncherSurface& Surface : FRTLauncherWorkspace::Surfaces())
	{
		if (!Surface.bDeclared)
		{
			// Una superficie che non esiste NON diventa un pulsante spento: diventa una frase che dice
			// quale issue la porta. Un pulsante inerte si legge come strumento rotto.
			Row->AddSlot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, RowPadding, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FRTLauncherWorkspace::PendingLabel(Surface)))
				];
			continue;
		}

		const FName Key = Surface.Key;
		Row->AddSlot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, RowPadding, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromName(Key))
				.OnClicked(this, &SRTLauncherScenarioPanel::OnSurfaceClicked, Key)
			];
	}

	return Row;
}

FReply SRTLauncherScenarioPanel::OnStartSessionClicked()
{
	URTDevSandboxLauncherSubsystem* Launcher = GEditor ? GEditor->GetEditorSubsystem<URTDevSandboxLauncherSubsystem>() : nullptr;
	if (!Launcher)
	{
		SessionMessage = TEXT("il launcher non e' disponibile: il difetto non e' nello scenario.");
		return FReply::Handled();
	}

	const FRTLauncherStartDecision Decision = Launcher->StartSession(SelectedId);

	SessionMessage = Decision.bAllowed
		? FString::Printf(TEXT("Sessione aperta su '%s'."), *SelectedId)
		: Decision.Reason;

	return FReply::Handled();
}

FReply SRTLauncherScenarioPanel::OnNewScenarioClicked()
{
	URTDevSandboxLauncherSubsystem* Launcher = GEditor ? GEditor->GetEditorSubsystem<URTDevSandboxLauncherSubsystem>() : nullptr;
	if (!Launcher)
	{
		SessionMessage = TEXT("il launcher non e' disponibile: il difetto non e' nello scenario.");
		return FReply::Handled();
	}

	const FRTLauncherStartDecision Decision = Launcher->StartNewSession(NewScenarioId);

	SessionMessage = Decision.bAllowed
		? FString::Printf(TEXT("Sessione nuova su '%s'."), *NewScenarioId)
		: Decision.Reason;

	return FReply::Handled();
}

FReply SRTLauncherScenarioPanel::OnSurfaceClicked(FName SurfaceKey)
{
	URTDevSandboxLauncherSubsystem* Launcher = GEditor ? GEditor->GetEditorSubsystem<URTDevSandboxLauncherSubsystem>() : nullptr;
	if (!Launcher || !Launcher->ActivateSurface(SurfaceKey))
	{
		SessionMessage = FString::Printf(TEXT("La superficie '%s' non si e' aperta."), *SurfaceKey.ToString());
	}

	return FReply::Handled();
}

void SRTLauncherScenarioPanel::RefreshPerspectiveOptions()
{
	PerspectiveOptions.Reset();

	// `Omniscient` c'e' sempre, anche senza anteprima: e' la prospettiva del designer, ed e' quella che il
	// viewport mostra per costruzione.
	PerspectiveOptions.Add(MakeShared<int32>(RTScenarioKnowledge::OmniscientTeamId));

	if (const URTScenarioPreviewSubsystem* Preview = GEditor ? GEditor->GetEditorSubsystem<URTScenarioPreviewSubsystem>() : nullptr)
	{
		// ⚠️ Le squadre vengono dal DATO dello scenario mostrato — vedi `RTScenarioKnowledge::TeamIds`. Un
		// `{0, 1}` cablato mentirebbe sul 4v4 e su ogni scenario a squadra sola.
		for (const int32 TeamId : Preview->GetSelectableTeams())
		{
			PerspectiveOptions.Add(MakeShared<int32>(TeamId));
		}
	}

	if (PerspectiveCombo.IsValid())
	{
		PerspectiveCombo->RefreshOptions();
	}
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

	// Senza scenario non ci sono unita' schierate: tenere il bersaglio precedente lascerebbe i tre pulsanti
	// abilitati su un'unita' che nessuno puo' piu' raggiungere.
	PlacedUnitOptions.Reset();
	SelectedUnitId.Reset();
}

void SRTLauncherScenarioPanel::RefreshReadout()
{
	ReadoutLines.Reset();
	ReadoutError.Reset();

	// ⚠️ **E la corsa insieme a loro** (#2788). Questa funzione riporta a schermo la posa d'AUTHORING:
	// `ShowScenario` comincia con `ClearPreview`, che chiude il playback. Una corsa ricordata qui
	// sopravvivrebbe alla traccia che descriveva, e la riga di trasporto annuncerebbe una corsa di N turni
	// sopra un campo che non la sta piu' mostrando.
	//
	// 🔑 **Ed e' la ragione per cui i due rami di rifiuto di `OnRunScenarioClicked` scrivono `Failed`
	// DOPO aver chiamato questa funzione**, non prima: scritto prima, verrebbe cancellato qui. Vale identico
	// per `ReadoutError`, che quei rami perdevano esattamente cosi'.
	LastRunState = ERTLauncherRunState::NotRun;
	LastRunTurns = 0;

	if (SelectedId.IsEmpty())
	{
		// Nessuna selezione, nessuna anteprima: lasciare a schermo lo scenario di prima mostrerebbe qualcosa
		// che il pannello non sta piu' dicendo. E con l'anteprima se ne vanno le sue squadre: un selettore
		// che offrisse `Team 1` senza niente a schermo prometterebbe una vista che non c'e'.
		ClearScenarioPreview();
		RefreshPerspectiveOptions();
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
		RefreshPerspectiveOptions();
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

		// Le squadre sono quelle dello scenario appena posato, e cambiano con esso: il selettore si
		// ricostruisce QUI, non una volta sola al `Construct`. Vale anche quando l'anteprima fallisce —
		// l'elenco torna alla sola `Omniscient` invece di offrire le squadre di quello precedente.
		RefreshPerspectiveOptions();
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

	// Le unita' schierate sono di QUESTO scenario: senza questa riga le tendine di sposta/ruota/ritira
	// resterebbero su quelle del precedente, e il primo gesto fallirebbe con `NotFound` accusando lo
	// scenario nuovo di non avere un'unita' che non ha mai avuto.
	RefreshPlacementOptions();
}

void SRTLauncherScenarioPanel::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText.ToString();

	// Solo la ricerca: nessuna rilettura dell'indice mentre si digita. Vedi `RefreshFilters`.
	RefreshVisible();
}


// --- Piazzamento delle unita' (#2786) ---------------------------------------------------------------

void SRTLauncherScenarioPanel::RefreshPlacementOptions()
{
	// Il roster e' `static`: non dipende dallo scenario aperto e si legge anche a selezione vuota. Leggerlo
	// una volta sola al `Construct` lo terrebbe fermo su un catalogo che il codice puo' cambiare mentre
	// l'editor e' aperto — e la tendina offrirebbe un eroe che `AddUnit` rifiuta.
	HeroOptions.Reset();
	for (const FName HeroId : URTScenarioAuthoring::ListHeroIds())
	{
		HeroOptions.Add(MakeShared<FName>(HeroId));
	}

	// Se l'eroe scelto non e' piu' nel roster, la scelta cade invece di restare a puntare al nulla: un
	// `AddUnit` con un `HeroId` sparito tornerebbe `Invalid` accusando lo scenario di un difetto del panel.
	const bool bHeroStillListed = HeroOptions.ContainsByPredicate(
		[this](const TSharedPtr<FName>& Option) { return Option.IsValid() && *Option == PlacementHeroId; });
	if (!bHeroStillListed)
	{
		PlacementHeroId = HeroOptions.Num() > 0 ? *HeroOptions[0] : NAME_None;
	}

	// Le unita' schierate arrivano dal readout appena ricostruito: `ReadoutLines` non le contiene come dato,
	// quindi si riapre il draft il minimo indispensabile. E' lo stesso ciclo di `RefreshReadout`.
	PlacedUnitOptions.Reset();
	if (SelectedId.IsEmpty() || !Authoring.IsValid())
	{
		SelectedUnitId.Empty();
		return;
	}

	FString OpenError;
	if (Authoring->OpenById(SelectedId, OpenError) != ERTScenarioAuthoringResult::Success)
	{
		// Non si scrive `ReadoutError` qui: l'ha gia' fatto `RefreshReadout`, che apre lo stesso scenario e
		// fallirebbe allo stesso modo. Un secondo messaggio direbbe due volte la stessa cosa.
		Authoring->Close();
		SelectedUnitId.Empty();
		return;
	}

	for (const FRTScenarioUnitView& Unit : Authoring->ListUnits())
	{
		PlacedUnitOptions.Add(MakeShared<FString>(Unit.Id));
	}
	Authoring->Close();

	const bool bUnitStillPlaced = PlacedUnitOptions.ContainsByPredicate(
		[this](const TSharedPtr<FString>& Option) { return Option.IsValid() && *Option == SelectedUnitId; });
	if (!bUnitStillPlaced)
	{
		// Dopo un `RemoveUnit` il bersaglio non esiste piu'. Lasciarlo selezionato farebbe fallire il gesto
		// successivo con `NotFound`, che accuserebbe lo scenario di un'unita' che il pannello stesso ha tolto.
		SelectedUnitId = PlacedUnitOptions.Num() > 0 ? *PlacedUnitOptions[0] : FString();
	}
}

FReply SRTLauncherScenarioPanel::RunPlacementGesture(
	TFunctionRef<ERTScenarioAuthoringResult(URTScenarioAuthoring&, FString&)> Mutate)
{
	if (SelectedId.IsEmpty())
	{
		SessionMessage = TEXT("Nessuno scenario selezionato: scegli una riga nell'elenco.");
		return FReply::Handled();
	}

	if (!Authoring.IsValid())
	{
		Authoring.Reset(URTScenarioAuthoring::CreateScenarioDraft(GetTransientPackage()));
	}

	if (!Authoring.IsValid())
	{
		SessionMessage = TEXT("la facade d'authoring non e' disponibile: il difetto non e' nello scenario.");
		return FReply::Handled();
	}

	FString OpenError;
	if (Authoring->OpenById(SelectedId, OpenError) != ERTScenarioAuthoringResult::Success)
	{
		SessionMessage = OpenError;
		Authoring->Close();
		return FReply::Handled();
	}

	FString MutateError;
	const ERTScenarioAuthoringResult MutateResult = Mutate(*Authoring, MutateError);
	if (MutateResult != ERTScenarioAuthoringResult::Success)
	{
		// ⛔ **Nessuna riscrittura della frase.** `DescribeResult` distingue `Invalid` da `WriteFailed` e da
		// `NotFound`, e `OutError` nomina il campo: fonderli in «non e' stato possibile» manderebbe a
		// cercare nel posto sbagliato — vedi il contratto del widget, che lo dichiara per gli stessi esiti.
		SessionMessage = MutateError.IsEmpty()
			? URTScenarioAuthoring::DescribeResult(MutateResult).ToString()
			: MutateError;
		Authoring->Close();
		return FReply::Handled();
	}

	// 🔑 Senza questa riga il gesto non esiste: la `Close()` qui sotto butterebbe la mutazione, e il pannello
	// mostrerebbe un readout identico a prima senza alcun errore — cioe' un pulsante che sembra non fare
	// niente. E' `DEC-1` della issue.
	FString SaveError;
	const ERTScenarioAuthoringResult SaveResult = Authoring->SaveInPlace(SaveError);
	Authoring->Close();

	if (SaveResult != ERTScenarioAuthoringResult::Success)
	{
		// ⚠️ **Il disco vince.** La mutazione era riuscita in memoria e non e' arrivata al file: il readout
		// si ricostruisce comunque dal file, quindi tornera' a mostrare lo stato di prima. Dire «aggiunta»
		// qui sarebbe l'unico modo di far divergere schermo e disco.
		SessionMessage = SaveError.IsEmpty()
			? URTScenarioAuthoring::DescribeResult(SaveResult).ToString()
			: SaveError;
		RefreshReadout();
		RefreshPlacementOptions();
		return FReply::Handled();
	}

	// Rilettura dal file, non dalla memoria: e' cio' che rende osservabile il salvataggio invece di
	// presupporlo.
	RefreshReadout();
	RefreshPlacementOptions();
	return FReply::Handled();
}

FReply SRTLauncherScenarioPanel::OnAddUnitClicked()
{
	return RunPlacementGesture([this](URTScenarioAuthoring& Facade, FString& OutError)
	{
		// Il conio legge le unita' della facade **gia' aperta**: una lista presa prima dell'apertura
		// sarebbe di un istante diverso, e due gesti rapidi conierebbero lo stesso id.
		const FString UnitId = FRTLauncherScenarioBrowser::CoinUnitId(Facade.ListUnits());
		return Facade.AddUnit(UnitId, PlacementHeroId, PlacementTeamId, PlacementCell, PlacementFacing, OutError);
	});
}

FReply SRTLauncherScenarioPanel::OnMoveUnitClicked()
{
	if (SelectedUnitId.IsEmpty())
	{
		SessionMessage = TEXT("Nessuna unita' selezionata: scegline una fra quelle schierate.");
		return FReply::Handled();
	}

	return RunPlacementGesture([this](URTScenarioAuthoring& Facade, FString& OutError)
	{
		return Facade.MoveUnit(SelectedUnitId, PlacementCell, OutError);
	});
}

FReply SRTLauncherScenarioPanel::OnFaceUnitClicked()
{
	if (SelectedUnitId.IsEmpty())
	{
		SessionMessage = TEXT("Nessuna unita' selezionata: scegline una fra quelle schierate.");
		return FReply::Handled();
	}

	return RunPlacementGesture([this](URTScenarioAuthoring& Facade, FString& OutError)
	{
		return Facade.SetUnitFacing(SelectedUnitId, PlacementFacing, OutError);
	});
}

FReply SRTLauncherScenarioPanel::OnRemoveUnitClicked()
{
	if (SelectedUnitId.IsEmpty())
	{
		SessionMessage = TEXT("Nessuna unita' selezionata: scegline una fra quelle schierate.");
		return FReply::Handled();
	}

	return RunPlacementGesture([this](URTScenarioAuthoring& Facade, FString& OutError)
	{
		return Facade.RemoveUnit(SelectedUnitId, OutError);
	});
}

// --- Trasporto del playback (`#1625`) ---------------------------------------------------------------
//
// 🔴 **Qui non si calcola nessuna fase e nessun turno.** Ogni pulsante chiama una funzione del
// sottosistema, che chiede al view model di `#472` di spostarsi. In questo blocco non compare aritmetica
// su `ERTMatchPhase` ne' su `TurnNumber` — ed e' la proprieta' che il criterio 2 chiede di poter
// verificare **per assenza**, leggendo.
//
// ⚠️ E' anche la regola che l'intestazione di questo file dichiara per tutto il resto: cio' che puo'
// sbagliare non sta nel guscio. Una «fase successiva» scritta qui sarebbe una regola che nessun test vede.

FReply SRTLauncherScenarioPanel::OnRunScenarioClicked()
{
	URTScenarioPreviewSubsystem* Preview = PreviewSubsystem();
	if (!Preview || SelectedId.IsEmpty() || !Authoring.IsValid())
	{
		return FReply::Handled();
	}

	// Il draft si riapre per la corsa e si richiude subito: e' lo stesso ciclo della selezione, e il
	// pannello continua a non possedere una sessione.
	FString ApriErrore;
	if (Authoring->OpenById(SelectedId, ApriErrore) != ERTScenarioAuthoringResult::Success)
	{
		Authoring->Close();
		RefreshReadout();

		// ⚠️ **Dopo `RefreshReadout()`, non prima**: quella funzione azzera `ReadoutError` in testa. Scritto
		// prima, il messaggio sopravviveva solo perche' la riapertura falliva a sua volta e ne rimetteva uno
		// identico — una stesura sbagliata che dava il risultato giusto.
		ReadoutError = ApriErrore;
		LastRunState = ERTLauncherRunState::Failed;
		LastRunTurns = 0;
		return FReply::Handled();
	}

	FRTScenarioRunReport Referto;
	FString CorsaErrore;
	const ERTScenarioAuthoringResult Esito = Authoring->Run(Referto, CorsaErrore);

	if (Esito != ERTScenarioAuthoringResult::Success)
	{
		// ⛔ Una corsa fallita NON apre un playback. Il campo resterebbe sulla posa d'authoring, che e'
		// indistinguibile da uno scenario in cui non succede niente — e sono due affermazioni diverse.
		Authoring->Close();
		RefreshReadout();

		// 🔴 **`CorsaErrore` arriva sullo schermo solo scrivendolo QUI.** Prima era assegnato davanti a
		// `RefreshReadout()`, che lo azzera in testa e poi riapre lo scenario — con successo, perche' a
		// fallire e' stata la CORSA e non l'apertura. Il messaggio della facade non raggiungeva quindi mai
		// il pannello, mentre il criterio 3 di #2788 lo dava per acquisito.
		ReadoutError = CorsaErrore;
		LastRunState = ERTLauncherRunState::Failed;
		LastRunTurns = 0;
		return FReply::Handled();
	}

	// ⚠️ Prima il campo torna a mostrare lo scenario: `OpenPlayback` pretende una preview viva, e muove i
	// marcatori di QUELLA. Senza, una corsa lanciata mentre l'anteprima non c'e' non aprirebbe niente, e il
	// pulsante sembrerebbe non funzionare.
	Preview->ShowScenario(Authoring.Get());
	Preview->OpenPlayback(Authoring.Get());

	Authoring->Close();

	// 🔴 **Qui NON si chiama `RefreshReadout()`, ed e' la riga che fa funzionare il pulsante.** Quella
	// funzione riapre il draft e rifa' `ShowScenario`, che comincia con `ClearPreview()` — la quale chiude il
	// playback. Il primo tentativo la chiamava in fondo «per aggiornare il referto», e il risultato era che
	// il playback si apriva e veniva chiuso una riga dopo: i controlli restavano spenti e il pannello
	// diceva ancora *«Nessun playback»* dopo una corsa riuscita.
	//
	// ⚠️ **Nessun automation test poteva vederlo**: i test chiamano `OpenPlayback` direttamente, mentre il
	// difetto stava nella SEQUENZA del pannello. L'ha trovato una sessione d'editor pilotata via MCP, il
	// 2026-09-04.
	//
	// Il referto non ne soffre: e' gia' quello dello scenario selezionato, e la riga di stato del trasporto
	// legge il sottosistema a ogni frame, quindi dice da sola se il playback si e' aperto.
	//
	// ⚠️ **«Se il playback si e' aperto» non e' «se una corsa e' avvenuta»** (#2788): una corsa che non
	// produce turni non apre nessun playback, e il sottosistema non ha modo di distinguerla da un pulsante
	// mai premuto. Il pannello se lo ricorda qui, e la riga lo legge insieme alla posizione.
	//
	// 🔑 **Il conteggio viene dal referto della facade, non da un giro proprio.** `TurnsPlayed` e' cio' che
	// il runner ha misurato; ricavarlo qui dalle tracce sarebbe una seconda autorita' su un numero che esiste
	// gia', e divergerebbe sugli scenari a varianti — dove l'aggregato non porta traccia.
	LastRunState = ERTLauncherRunState::Ran;
	LastRunTurns = Referto.TurnsPlayed;

	return FReply::Handled();
}

void SRTLauncherScenarioPanel::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Il delta si passa e basta. Il sottosistema ridisegna solo quando la posizione cambia, quindi qui non
	// serve — e non si deve — decidere niente in base al tempo trascorso.
	if (URTScenarioPreviewSubsystem* Preview = PreviewSubsystem())
	{
		Preview->PlaybackTick(DeltaTime);
	}
}

TSharedRef<SWidget> SRTLauncherScenarioPanel::BuildPlacementRow()
{
	// Le sei direzioni si costruiscono una volta: l'enum non cambia a runtime, e ricostruirle a ogni
	// ridisegno romperebbe l'identita' di puntatore su cui `SComboBox` tiene la selezione.
	if (FacingOptions.Num() == 0)
	{
		for (const ERTHexDirection Dir : { ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
			ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE })
		{
			FacingOptions.Add(MakeShared<ERTHexDirection>(Dir));
		}
	}

	// I nomi delle direzioni sono quelli del sorgente (`E`, `NE`, …), non tradotti: sono gli stessi che
	// compaiono nel JSON dello scenario e nel TurnLog, e un secondo vocabolario costringerebbe a tradurre
	// mentalmente ogni volta che si confronta la schermata con il file.
	auto NomeFacing = [](ERTHexDirection Dir)
	{
		switch (Dir)
		{
		case ERTHexDirection::E:  return FText::FromString(TEXT("E"));
		case ERTHexDirection::NE: return FText::FromString(TEXT("NE"));
		case ERTHexDirection::NW: return FText::FromString(TEXT("NW"));
		case ERTHexDirection::W:  return FText::FromString(TEXT("W"));
		case ERTHexDirection::SW: return FText::FromString(TEXT("SW"));
		default:                  return FText::FromString(TEXT("SE"));
		}
	};

	// Un campo di coordinata: legge e scrive un `int32` della cella corrente. Nessun limite imposto qui —
	// quale cella esista lo decide `AddUnit`, e un intervallo scritto nel pannello sarebbe una seconda
	// regola di arena che diverge il giorno che una mappa cambia raggio.
	auto Coordinata = [this](FText Etichetta, TFunction<int32()> Leggi, TFunction<void(int32)> Scrivi)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 2.0f, 0.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Etichetta)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).MinDesiredWidth(48.0f)
				[
					SNew(SNumericEntryBox<int32>)
					.AllowSpin(false)
					.Value_Lambda([Leggi]() { return TOptional<int32>(Leggi()); })
					.OnValueCommitted_Lambda([Scrivi](int32 NewValue, ETextCommit::Type) { Scrivi(NewValue); })
				]
			];
	};

	return SNew(SVerticalBox)

		// Riga 1 — cosa schierare, e dove.
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SComboBox<TSharedPtr<FName>>)
				.OptionsSource(&HeroOptions)
				.ToolTipText(LOCTEXT("PlaceHeroTip", "Gli HeroId del roster, da ListHeroIds(). Nessun elenco scritto nel pannello."))
				.OnGenerateWidget_Lambda([](TSharedPtr<FName> Option)
				{
					return SNew(STextBlock).Text(FText::FromName(Option.IsValid() ? *Option : NAME_None));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FName> Option, ESelectInfo::Type)
				{
					if (Option.IsValid()) { PlacementHeroId = *Option; }
				})
				[
					SNew(STextBlock).Text_Lambda([this]() { return FText::FromName(PlacementHeroId); })
				]
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				Coordinata(LOCTEXT("PlaceTeam", "team"),
					[this]() { return PlacementTeamId; },
					[this](int32 V) { PlacementTeamId = V; })
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				Coordinata(LOCTEXT("PlaceQ", "q"),
					[this]() { return PlacementCell.X; },
					[this](int32 V) { PlacementCell.X = V; })
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				Coordinata(LOCTEXT("PlaceR", "r"),
					[this]() { return PlacementCell.Y; },
					[this](int32 V) { PlacementCell.Y = V; })
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				Coordinata(LOCTEXT("PlaceLayer", "L"),
					[this]() { return PlacementCell.Layer; },
					[this](int32 V) { PlacementCell.Layer = V; })
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SComboBox<TSharedPtr<ERTHexDirection>>)
				.OptionsSource(&FacingOptions)
				.ToolTipText(LOCTEXT("PlaceFacingTip", "Una delle sei direzioni esagonali. Mai un angolo libero."))
				.OnGenerateWidget_Lambda([NomeFacing](TSharedPtr<ERTHexDirection> Option)
				{
					return SNew(STextBlock).Text(NomeFacing(Option.IsValid() ? *Option : ERTHexDirection::E));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<ERTHexDirection> Option, ESelectInfo::Type)
				{
					if (Option.IsValid()) { PlacementFacing = *Option; }
				})
				[
					SNew(STextBlock).Text_Lambda([this, NomeFacing]() { return NomeFacing(PlacementFacing); })
				]
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("AddUnit", "+ UNIT"))
				.ToolTipText(LOCTEXT("AddUnitTip", "Schiera l'unita' attraverso URTScenarioAuthoring::AddUnit e salva. L'id lo conia il pannello; validita' e occupazione le decide la facade."))
				.IsEnabled_Lambda([this]() { return !SelectedId.IsEmpty(); })
				.OnClicked(this, &SRTLauncherScenarioPanel::OnAddUnitClicked)
			]
		]

		// Riga 2 — cosa fare di una gia' schierata.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&PlacedUnitOptions)
				.ToolTipText(LOCTEXT("PlacedUnitTip", "Le unita' gia' schierate, rilette da ListUnits() dopo ogni gesto."))
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
				{
					return SNew(STextBlock).Text(FText::FromString(Option.IsValid() ? *Option : FString()));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Option, ESelectInfo::Type)
				{
					if (Option.IsValid()) { SelectedUnitId = *Option; }
				})
				[
					SNew(STextBlock).Text_Lambda([this]()
					{
						return SelectedUnitId.IsEmpty()
							? LOCTEXT("NoPlacedUnit", "nessuna unita'")
							: FText::FromString(SelectedUnitId);
					})
				]
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("MoveUnit", "Sposta"))
				.ToolTipText(LOCTEXT("MoveUnitTip", "MoveUnit sulla cella dei campi q/r/L."))
				.IsEnabled_Lambda([this]() { return !SelectedUnitId.IsEmpty(); })
				.OnClicked(this, &SRTLauncherScenarioPanel::OnMoveUnitClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("FaceUnit", "Ruota"))
				.ToolTipText(LOCTEXT("FaceUnitTip", "SetUnitFacing sulla direzione scelta."))
				.IsEnabled_Lambda([this]() { return !SelectedUnitId.IsEmpty(); })
				.OnClicked(this, &SRTLauncherScenarioPanel::OnFaceUnitClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("RemoveUnit", "Ritira"))
				.ToolTipText(LOCTEXT("RemoveUnitTip", "RemoveUnit sull'unita' selezionata."))
				.IsEnabled_Lambda([this]() { return !SelectedUnitId.IsEmpty(); })
				.OnClicked(this, &SRTLauncherScenarioPanel::OnRemoveUnitClicked)
			]
		];
}

TSharedRef<SWidget> SRTLauncherScenarioPanel::BuildTransportRow()
{
	// Un pulsante di passo: etichetta, suggerimento, l'azione e la condizione che lo abilita. Entrambe
	// interrogano il sottosistema — il pannello non tiene una copia di dove il playback e' arrivato.
	auto Passo = [](FText Etichetta, FText Aiuto, TFunction<bool()> Agisci, TFunction<bool()> Puo)
	{
		return SNew(SButton)
			.Text(Etichetta)
			.ToolTipText(Aiuto)
			.IsEnabled_Lambda([Puo]() { return Puo(); })
			.OnClicked_Lambda([Agisci]() { Agisci(); return FReply::Handled(); });
	};

	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("RunScenario", "Esegui"))
			.ToolTipText(LOCTEXT("RunScenarioTip",
				"Esegue lo scenario selezionato e apre il playback della risoluzione nel viewport."))
			.IsEnabled_Lambda([this]() { return !SelectedId.IsEmpty(); })
			.OnClicked(this, &SRTLauncherScenarioPanel::OnRunScenarioClicked)
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 2.0f, 0.0f)
		[
			SNew(SButton)
			.Text_Lambda([]()
			{
				const URTScenarioPreviewSubsystem* P = PreviewSubsystem();
				return (P && P->IsPlaybackPlaying())
					? LOCTEXT("PlaybackPause", "Pausa")
					: LOCTEXT("PlaybackRun", "Riproduci");
			})
			.ToolTipText(LOCTEXT("PlaybackRunTip", "Avvia o ferma la riproduzione automatica."))
			.IsEnabled_Lambda([]()
			{
				const URTScenarioPreviewSubsystem* P = PreviewSubsystem();
				return P && P->IsPlaybackOpen();
			})
			.OnClicked_Lambda([]()
			{
				if (URTScenarioPreviewSubsystem* P = PreviewSubsystem())
				{
					if (P->IsPlaybackPlaying()) { P->PlaybackPause(); } else { P->PlaybackPlay(); }
				}
				return FReply::Handled();
			})
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 2.0f, 0.0f)
		[
			Passo(LOCTEXT("PrevTurn", "|<"), LOCTEXT("PrevTurnTip", "Turno precedente."),
				[]() { URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->PlaybackStepTurn(false); },
				[]() { const URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->CanPlaybackStepTurn(false); })
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 2.0f, 0.0f)
		[
			Passo(LOCTEXT("PrevPhase", "<"), LOCTEXT("PrevPhaseTip", "Fase precedente."),
				[]() { URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->PlaybackStepPhase(false); },
				[]() { const URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->CanPlaybackStepPhase(false); })
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 2.0f, 0.0f)
		[
			Passo(LOCTEXT("NextPhase", ">"), LOCTEXT("NextPhaseTip", "Fase successiva."),
				[]() { URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->PlaybackStepPhase(true); },
				[]() { const URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->CanPlaybackStepPhase(true); })
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			Passo(LOCTEXT("NextTurn", ">|"), LOCTEXT("NextTurnTip", "Turno successivo."),
				[]() { URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->PlaybackStepTurn(true); },
				[]() { const URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->CanPlaybackStepTurn(true); })
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			Passo(LOCTEXT("PlaybackReset", "Reset"),
				LOCTEXT("PlaybackResetTip", "Torna alla posa iniziale e ferma la riproduzione."),
				[]() { URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->PlaybackRewind(); },
				[]() { const URTScenarioPreviewSubsystem* P = PreviewSubsystem(); return P && P->IsPlaybackOpen(); })
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SAssignNew(SpeedCombo, SComboBox<TSharedPtr<ERTPlaybackSpeed>>)
			.OptionsSource(&SpeedOptions)
			.OnGenerateWidget_Lambda([](TSharedPtr<ERTPlaybackSpeed> Opzione)
			{
				return SNew(STextBlock).Text(DescribeSpeed(Opzione.IsValid() ? *Opzione : ERTPlaybackSpeed::Normal));
			})
			.OnSelectionChanged_Lambda([](TSharedPtr<ERTPlaybackSpeed> Nuova, ESelectInfo::Type)
			{
				if (!Nuova.IsValid())
				{
					return;
				}
				if (URTScenarioPreviewSubsystem* P = PreviewSubsystem())
				{
					P->SetPlaybackSpeed(*Nuova);
				}
			})
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					// La verita' e' del sottosistema, come per la prospettiva: una copia locale divergerebbe.
					const URTScenarioPreviewSubsystem* P = PreviewSubsystem();
					return DescribeSpeed(P ? P->GetPlaybackSpeed() : ERTPlaybackSpeed::Normal);
				})
			]
		]

		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				// 🔴 **Qui non si sceglie nessuna frase**, ed e' la stessa regola del blocco qui sopra: si
				// mettono insieme i fatti — due dal sottosistema, due dalla memoria della corsa — e a tradurli
				// e' `FRTLauncherScenarioBrowser`, dove un automation test li vede tutti.
				const URTScenarioPreviewSubsystem* P = PreviewSubsystem();

				FRTLauncherTransportStatus Stato;
				Stato.Run = LastRunState;
				Stato.TurnsPlayed = LastRunTurns;

				// L'apertura si richiede a OGNI ridisegno e non si ricorda: il playback puo' chiudersi da
				// sotto — un `ClearPreview` per un'altra via — e una copia locale direbbe «aperto» su un
				// campo vuoto.
				Stato.bPlaybackOpen = P && P->IsPlaybackOpen();
				if (Stato.bPlaybackOpen)
				{
					Stato.Position = P->GetPlaybackPosition();
				}

				return FRTLauncherScenarioBrowser::DescribeTransport(Stato);
			})
		];
}

#undef LOCTEXT_NAMESPACE
