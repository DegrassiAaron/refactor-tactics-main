#include "SRTAnimBrowserPanel.h"

#include "Animation/AnimSequence.h"
#include "SRTAnimPreviewViewport.h"
#include "Unit/RTAnimCatalogLibrary.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "RTAnimBrowser"

const FName SRTAnimBrowserPanel::TabId(TEXT("RTAnimBrowser"));

namespace
{
	FText TestoStato(ERTAnimClipStatus Status)
	{
		const UEnum* Enum = StaticEnum<ERTAnimClipStatus>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Status)) : FText::GetEmpty();
	}

	/** Il colore dice lo stato a colpo d'occhio; il testo resta comunque, perche' il colore non e' un dato. */
	FSlateColor ColoreStato(ERTAnimClipStatus Status)
	{
		switch (Status)
		{
		case ERTAnimClipStatus::Promoted:  return FSlateColor(FLinearColor(0.35f, 0.80f, 0.35f));
		case ERTAnimClipStatus::Rejected:  return FSlateColor(FLinearColor(0.80f, 0.35f, 0.35f));
		case ERTAnimClipStatus::Candidate: return FSlateColor(FLinearColor(0.85f, 0.75f, 0.35f));
		default:                           return FSlateColor::UseSubduedForeground();
		}
	}
}

void SRTAnimBrowserPanel::Construct(const FArguments&)
{
	PercorsoCatalogo = URTAnimCatalogLibrary::DefaultCatalogPath();
	Ricarica();

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.45f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f) [ CostruisciBarraFiltri() ]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
			[
				SAssignNew(Lista, SListView<FRowPtr>)
				.ListItemsSource(&Righe)
				.OnGenerateRow(this, &SRTAnimBrowserPanel::GeneraRiga)
				.OnSelectionChanged(this, &SRTAnimBrowserPanel::OnSelezione)
				.SelectionMode(ESelectionMode::Single)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f) [ CostruisciAzioni() ]
		]
		+ SSplitter::Slot().Value(0.55f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(Anteprima, SRTAnimPreviewViewport)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f) [ CostruisciTrasporto() ]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(STextBlock)
				.Text(this, &SRTAnimBrowserPanel::TestoStatoSelezione)
				.AutoWrapText(true)
			]
		]
	];
}

void SRTAnimBrowserPanel::Ricarica()
{
	UltimoErrore.Reset();
	if (!Modello.LoadFrom(PercorsoCatalogo, UltimoErrore))
	{
		// ⛔ Un catalogo illeggibile NON diventa una lista vuota: una lista vuota si legge come «non ci
		// sono clip», che e' un'affermazione, mentre il fatto e' «non ho potuto leggerle».
		Righe.Reset();
		if (Lista.IsValid()) { Lista->RequestListRefresh(); }
		return;
	}
	RiapplicaFiltri();
}

void SRTAnimBrowserPanel::RiapplicaFiltri()
{
	Righe.Reset();
	for (const FRTAnimBrowserRow& R : Modello.VisibleRows())
	{
		Righe.Add(MakeShared<FRTAnimBrowserRow>(R));
	}
	if (Lista.IsValid()) { Lista->RequestListRefresh(); }
}

void SRTAnimBrowserPanel::Salva()
{
	FString Errore;
	if (!Modello.SaveTo(PercorsoCatalogo, Errore))
	{
		UltimoErrore = Errore;
	}
}

TSharedRef<SWidget> SRTAnimBrowserPanel::CostruisciBarraFiltri()
{
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot().AutoHeight()
	[
		SNew(SSearchBox)
		.HintText(LOCTEXT("Cerca", "Nome della clip o AV_ID"))
		.OnTextChanged_Lambda([this](const FText& T)
		{
			Modello.SetSearchText(T.ToString());
			RiapplicaFiltri();
		})
	]
	+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(STextBlock).Text(LOCTEXT("Personaggio", "Personaggio:"))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SEditableTextBox)
			.HintText(LOCTEXT("TuttiPack", "tutti"))
			.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type)
			{
				Modello.SetPackFilter(T.ToString());
				RiapplicaFiltri();
			})
		]
	];
}

TSharedRef<ITableRow> SRTAnimBrowserPanel::GeneraRiga(FRowPtr Riga, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<FRowPtr>, Owner)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
		[
			SNew(STextBlock).Text(FText::FromName(Riga->Id)).MinDesiredWidth(70.f)
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
		[
			SNew(STextBlock).Text(FText::FromString(Riga->AssetName))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
		[
			// Lo stato e' TESTO oltre che colore: il colore da solo non e' un dato leggibile da tutti.
			SNew(STextBlock)
			.Text(TestoStato(Riga->Status))
			.ColorAndOpacity(ColoreStato(Riga->Status))
			.MinDesiredWidth(80.f)
		]
	];
}

void SRTAnimBrowserPanel::OnSelezione(FRowPtr Riga, ESelectInfo::Type)
{
	Selezione = Riga;
	if (!Anteprima.IsValid())
	{
		return;
	}
	if (!Riga.IsValid())
	{
		Anteprima->SetClip(nullptr);
		return;
	}

	// ⚠️ `LoadObject` su un pack non installato restituisce `nullptr` senza rumore: `Content/FabAsset/` e'
	// gitignorato, e su un checkout senza i pack l'anteprima resta vuota. Non e' un difetto — ed e' il
	// motivo per cui il messaggio sotto la preview lo dice invece di lasciare un riquadro nero muto.
	UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, *Riga->AssetPath);
	Anteprima->SetClip(Clip);
}

TSharedRef<SWidget> SRTAnimBrowserPanel::CostruisciAzioni()
{
	auto Pulsante = [this](const FText& Etichetta, ERTAnimClipStatus Nuovo)
	{
		return SNew(SButton)
			.Text(Etichetta)
			.IsEnabled_Lambda([this]() { return Selezione.IsValid(); })
			.OnClicked_Lambda([this, Nuovo]() { return OnComandoStato(Nuovo); });
	};

	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
	[ Pulsante(LOCTEXT("Promuovi", "Promote"), ERTAnimClipStatus::Promoted) ]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
	[ Pulsante(LOCTEXT("Candidata", "Candidate"), ERTAnimClipStatus::Candidate) ]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
	[ Pulsante(LOCTEXT("Scarta", "Reject"), ERTAnimClipStatus::Rejected) ]
	+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 2.f, 2.f, 2.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("Ricarica", "Ricarica"))
		.OnClicked_Lambda([this]() { Ricarica(); return FReply::Handled(); })
	];
}

FReply SRTAnimBrowserPanel::OnComandoStato(ERTAnimClipStatus Nuovo)
{
	// 🔴 **Questo e' l'unico ingresso ai comandi di stato del pannello**, e passa da `ApplyUserStatus`,
	// che a sua volta e' l'unico punto del progetto che scrive `Status`. La catena e' corta apposta:
	// `Promoted` deve poter risalire a un click, sempre.
	if (!Selezione.IsValid())
	{
		return FReply::Handled();
	}
	if (Modello.ApplyUserStatus(Selezione->Id, Nuovo))
	{
		Salva();
		RiapplicaFiltri();
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SRTAnimBrowserPanel::CostruisciTrasporto()
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
	[
		SNew(SButton).Text(LOCTEXT("Play", "Play"))
		.OnClicked_Lambda([this]() { if (Anteprima.IsValid()) { Anteprima->Play(); } return FReply::Handled(); })
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
	[
		SNew(SButton).Text(LOCTEXT("Pausa", "Pause"))
		.OnClicked_Lambda([this]() { if (Anteprima.IsValid()) { Anteprima->Pause(); } return FReply::Handled(); })
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
	[
		SNew(SButton).Text(LOCTEXT("Riavvia", "Restart"))
		.OnClicked_Lambda([this]() { if (Anteprima.IsValid()) { Anteprima->Restart(); } return FReply::Handled(); })
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(8.f, 2.f, 2.f, 2.f)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]()
		{
			return (Anteprima.IsValid() && Anteprima->IsLooping())
				? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this](ECheckBoxState S)
		{
			if (Anteprima.IsValid()) { Anteprima->SetLooping(S == ECheckBoxState::Checked); }
		})
		[ SNew(STextBlock).Text(LOCTEXT("Loop", "Loop")) ]
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(8.f, 2.f, 2.f, 2.f)
	[
		SNew(STextBlock).Text(LOCTEXT("Velocita", "Velocita':"))
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2.f).MinWidth(90.f)
	[
		// Fino a 0,1x: giudicare la partenza e il recovery di un attacco al rallentatore e' l'uso per cui
		// questo controllo esiste.
		SNew(SSpinBox<float>)
		.MinValue(0.1f).MaxValue(2.0f).Delta(0.05f)
		.Value_Lambda([this]() { return Anteprima.IsValid() ? Anteprima->GetPlayRate() : 1.f; })
		.OnValueChanged_Lambda([this](float V) { if (Anteprima.IsValid()) { Anteprima->SetPlayRate(V); } })
	];
}

FText SRTAnimBrowserPanel::TestoStatoSelezione() const
{
	if (!UltimoErrore.IsEmpty())
	{
		return FText::Format(LOCTEXT("ErroreCatalogo", "Catalogo non letto: {0}"),
			FText::FromString(UltimoErrore));
	}
	if (!Selezione.IsValid())
	{
		return FText::Format(LOCTEXT("NessunaSelezione", "{0} clip a catalogo. Selezionane una."),
			FText::AsNumber(Righe.Num()));
	}
	if (Anteprima.IsValid() && !Anteprima->GetLastError().IsEmpty())
	{
		// La preview vuota dichiara PERCHE'. Un riquadro nero muto si legge come «la clip e' rotta».
		return FText::Format(LOCTEXT("AnteprimaVuota", "{0}  —  anteprima non disponibile: {1}"),
			FText::FromName(Selezione->Id), FText::FromString(Anteprima->GetLastError()));
	}
	return FText::Format(LOCTEXT("Selezionata", "{0}  ·  {1}  ·  {2}  ·  {3} legami"),
		FText::FromName(Selezione->Id),
		FText::FromString(Selezione->AssetName),
		TestoStato(Selezione->Status),
		FText::AsNumber(Selezione->NumBindings));
}

#undef LOCTEXT_NAMESPACE
