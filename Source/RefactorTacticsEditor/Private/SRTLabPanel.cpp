#include "SRTLabPanel.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "RTLabPanel"

const FName SRTLabPanel::TabId(TEXT("RTLab"));

void SRTLabPanel::Construct(const FArguments&)
{
	RiapplicaElenco();

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.42f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f) [ CostruisciFiltro() ]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
			[
				SAssignNew(Lista, SListView<FVoce>)
				.ListItemsSource(&Voci)
				.OnGenerateRow(this, &SRTLabPanel::GeneraRiga)
				.OnSelectionChanged(this, &SRTLabPanel::OnSelezione)
				.SelectionMode(ESelectionMode::Single)
			]
		]
		+ SSplitter::Slot().Value(0.58f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(STextBlock).Text(this, &SRTLabPanel::TestoIdentita).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(STextBlock).Text(this, &SRTLabPanel::TestoParametri).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f) [ CostruisciEsecuzione() ]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
			[
				SNew(STextBlock).Text(this, &SRTLabPanel::TestoEsito).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock).Text(this, &SRTLabPanel::TestoTurnLog).AutoWrapText(true)
				]
			]
		]
	];
}

void SRTLabPanel::RiapplicaElenco()
{
	Voci.Reset();
	for (const FRTAbilityLabEntry& Entry : Modello.VisibleAbilities())
	{
		Voci.Add(MakeShared<FRTAbilityLabEntry>(Entry));
	}
	if (Lista.IsValid())
	{
		Lista->RequestListRefresh();
	}
}

TSharedRef<SWidget> SRTLabPanel::CostruisciFiltro()
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
	[
		SNew(STextBlock).Text(LOCTEXT("Eroe", "Eroe:"))
	]
	+ SHorizontalBox::Slot().FillWidth(1.f)
	[
		SAssignNew(CampoFiltro, SEditableTextBox)
		.HintText(LOCTEXT("TuttiGliEroi", "vuoto = tutto il catalogo canonico"))
		.OnTextCommitted_Lambda([this](const FText& Testo, ETextCommit::Type)
		{
			const FString Grezzo = Testo.ToString().TrimStartAndEnd();
			// Il modello decide cosa succede alla selezione quando il filtro cambia: qui si riferisce
			// soltanto il gesto.
			Modello.SetHeroFilter(Grezzo.IsEmpty() ? NAME_None : FName(*Grezzo));
			UltimoErrore.Reset();
			RiapplicaElenco();
		})
	];
}

TSharedRef<SWidget> SRTLabPanel::CostruisciEsecuzione()
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().AutoWidth()
	[
		SNew(SButton)
		.Text(LOCTEXT("Esegui", "Esegui"))
		.ToolTipText(LOCTEXT("EseguiTip",
			"Costruisce la fixture deterministica e la esegue con il resolver reale."))
		.OnClicked(this, &SRTLabPanel::OnEsegui)
	];
}

TSharedRef<ITableRow> SRTLabPanel::GeneraRiga(FVoce Voce, const TSharedRef<STableViewBase>& Owner)
{
	const FText Etichetta = Voce.IsValid()
		? FText::FromName(Voce->AbilityId)
		: LOCTEXT("VoceVuota", "—");

	return SNew(STableRow<FVoce>, Owner)
	[
		SNew(STextBlock).Text(Etichetta)
	];
}

void SRTLabPanel::OnSelezione(FVoce Voce, ESelectInfo::Type)
{
	if (!Voce.IsValid())
	{
		return;
	}

	// Il rifiuto e' possibile e va mostrato: e' la regola d'appartenenza di #2600, non un errore di UI.
	if (!Modello.SelectAbility(Voce->AbilityId))
	{
		UltimoErrore = FString::Printf(
			TEXT("'%s' non e' nell'elenco visibile: cambia il filtro per poterla eseguire."),
			*Voce->AbilityId.ToString());
		return;
	}

	UltimoErrore.Reset();
}

FReply SRTLabPanel::OnEsegui()
{
	UltimoErrore.Reset();

	// Un mondo transitorio, creato e distrutto qui. Il livello aperto nell'editor non viene toccato.
	UWorld* Mondo = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!Mondo)
	{
		UltimoErrore = TEXT("Impossibile creare il mondo della fixture.");
		return FReply::Handled();
	}

	if (GEngine)
	{
		FWorldContext& Contesto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contesto.SetCurrentWorld(Mondo);
	}

	FString Errore;
	const bool bEseguito = Modello.Run(Mondo, Errore);

	if (GEngine)
	{
		GEngine->DestroyWorldContext(Mondo);
	}
	Mondo->DestroyWorld(/*bInformEngineOfWorld=*/ false);

	if (!bEseguito)
	{
		UltimoErrore = Errore;
	}

	return FReply::Handled();
}

FText SRTLabPanel::TestoIdentita() const
{
	FRTHeroLabEntry Eroe;
	if (!Modello.GetHeroReadout(Eroe))
	{
		return LOCTEXT("NessunEroe",
			"Catalogo canonico intero — nessun eroe filtrato. Scrivi un HeroId per vedere un kit.");
	}

	return FText::FromString(FString::Printf(
		TEXT("%s — PV %d · MP %d · vista %d · udito %d · spinta %d\nAffinita' %s · debolezza %s · reazione %s\nvoci di kit dichiarate: %d"),
		*Eroe.HeroId.ToString(), Eroe.MaxHealth, Eroe.MovePoints, Eroe.VisionRange,
		Eroe.HearingThreshold, Eroe.PushResistance,
		*Eroe.Affinity.ToString(), *Eroe.Weakness.ToString(), *Eroe.ReactionProfileId.ToString(),
		Eroe.DeclaredAbilityCount));
}

FText SRTLabPanel::TestoParametri() const
{
	const FName Selezionata = Modello.GetSelectedAbility();
	if (Selezionata.IsNone())
	{
		return LOCTEXT("NessunaSelezione", "Nessuna ability selezionata.");
	}

	TArray<FRTActionParameterView> Parametri;
	if (Modello.DescribeSelection(Parametri) != ERTActionReadoutResult::Ok)
	{
		return FText::FromString(FString::Printf(
			TEXT("%s — il catalogo non la conosce."), *Selezionata.ToString()));
	}

	FString Testo = Selezionata.ToString();
	for (const FRTActionParameterView& P : Parametri)
	{
		// Entrambe le case, sempre: sceglierne una mostrerebbe un numero che il gioco puo' non usare.
		Testo += FString::Printf(TEXT("\n  %s: catalogo %d · letto %d%s"),
			*P.ParameterKey.ToString(), P.DeclaredValue, P.ConsumedValue,
			P.bHomesAgree ? TEXT("") : TEXT("   ⚠ le due case non concordano"));
	}
	return FText::FromString(Testo);
}

FText SRTLabPanel::TestoEsito() const
{
	if (!UltimoErrore.IsEmpty())
	{
		return FText::FromString(FString::Printf(TEXT("⛔ %s"), *UltimoErrore));
	}

	const FRTLabRunResult& Esito = Modello.LastRun();
	if (!Esito.bHasRun)
	{
		return LOCTEXT("NonEseguito", "Non ancora eseguito.");
	}

	FString Testo = FString::Printf(TEXT("%s — %d turno/i giocato/i"), *Esito.Outcome, Esito.TurnsPlayed);
	for (const FRTUnitStateDiff& Diff : Esito.Diffs)
	{
		for (const FRTUnitFieldChange& Cambio : Diff.Changes)
		{
			Testo += FString::Printf(TEXT("\n  unita' %d · %s: %s -> %s"),
				Diff.UnitId, *Cambio.Field.ToString(), *Cambio.Before, *Cambio.After);
		}
	}
	return FText::FromString(Testo);
}

FText SRTLabPanel::TestoTurnLog() const
{
	const FRTLabRunResult& Esito = Modello.LastRun();
	if (!Esito.bHasRun)
	{
		return FText::GetEmpty();
	}
	if (Esito.TurnLogLines.Num() == 0)
	{
		return LOCTEXT("TurnLogVuoto", "La run non ha prodotto voci di TurnLog.");
	}
	return FText::FromString(FString::Join(Esito.TurnLogLines, TEXT("\n")));
}

#undef LOCTEXT_NAMESPACE
