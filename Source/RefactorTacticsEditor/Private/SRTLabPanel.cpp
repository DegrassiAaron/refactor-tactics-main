#include "SRTLabPanel.h"

#include "Ability/RTHeroLab.h"
#include "RTLabPieLauncher.h"

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
		// ⚠️ L'hint DICHIARA la forma, e prima non lo faceva (`#3461`): il confronto a valle e' esatto,
		// quindi `Ivrin` non e' `Hero.Ivrin` e la lista si svuotava senza che nulla lo dicesse. Chi usa il
		// pannello scrive il nome nudo -- e' la forma che viene in mente -- e il campo non chiedeva altro.
		.HintText(LOCTEXT("TuttiGliEroi",
			"vuoto = tutto il catalogo canonico · oppure un HeroId completo, es. Hero.Ivrin"))
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
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("EseguiInPie", "Esegui in PIE"))
		.ToolTipText(LOCTEXT("EseguiInPieTip",
			"Salva la fixture in Saved/RTLab/Scenarios, imposta rt.Test.Scenario e avvia PIE su L_DevSandbox. "
			"Le CVar tornano com'erano a fine PIE."))
		.OnClicked(this, &SRTLabPanel::OnEseguiInPie)
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
	// L'Id dell'ultimo lancio PIE non si azzera qui: lo fa `Modello.Run`, perche' la riga di stato mostra
	// una cosa sola — l'ultimo gesto — e quella regola sta nel modello, dove si misura.
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

FReply SRTLabPanel::OnEseguiInPie()
{
	UltimoErrore.Reset();

	// ⛔ **Prima si chiede se si puo' lanciare, poi si scrive.** `PrepareForPie` salva la fixture su disco: con
	// PIE gia' in corso il lancio rifiuta comunque, e il file resterebbe per un lancio mai avvenuto.
	FString Errore;
	if (!FRTLabPieLauncher::CanLaunch(Errore))
	{
		UltimoErrore = Errore;
		return FReply::Handled();
	}

	FString Id;
	if (!Modello.PrepareForPie(Id, Errore))
	{
		UltimoErrore = Errore;
		return FReply::Handled();
	}

	// Il PIE puo' finire dopo che il pannello e' stato chiuso: il lanciatore tiene la callback, quindi la
	// si lega **debole**. Il modello e' membro del widget e non sopravvive a lui.
	const bool bLanciato = FRTLabPieLauncher::Launch(Id, Errore,
		[Debole = TWeakPtr<SRTLabPanel>(SharedThis(this))]()
		{
			if (const TSharedPtr<SRTLabPanel> Pannello = Debole.Pin())
			{
				Pannello->Modello.NoteLaunchFinished();
			}
		});
	if (!bLanciato)
	{
		UltimoErrore = Errore;
		return FReply::Handled();
	}

	Modello.NoteLaunched(Id);
	return FReply::Handled();
}

FText SRTLabPanel::TestoIdentita() const
{
	// 🔑 **Il perche' lo decide il MODELLO, la frase e' di qui** (`#3461`). Prima questa funzione
	// chiamava `GetHeroReadout`, che risponde `false` a due domande diverse -- «nessun filtro» e «un filtro
	// che non matcha» -- e le rendeva con la stessa frase. Nel secondo caso quella frase era il contrario
	// di cio' che accadeva: l'elenco era vuoto proprio PERCHE' il filtro aveva matchato nulla.
	FRTHeroLabEntry Eroe;
	switch (Modello.DescribeFilterState(Eroe))
	{
	case FRTLabViewModel::EFilterState::NoFilter:
		return LOCTEXT("NessunEroe",
			"Catalogo canonico intero — nessun eroe filtrato. Scrivi un HeroId per vedere un kit.");

	case FRTLabViewModel::EFilterState::UnknownHeroId:
	{
		// ⚠️ Gli id si **derivano** da `ListCanonicalHeroes()`, non si scrivono qui: un roster in una
		// stringa invecchia da solo, e un eroe aggiunto domani lascerebbe questo messaggio a mentire.
		TArray<FString> Id;
		for (const FRTHeroLabEntry& Voce : URTHeroLabLibrary::ListCanonicalHeroes())
		{
			Id.Add(Voce.HeroId.ToString());
		}
		return FText::FromString(FString::Printf(
			TEXT("Nessun eroe con id \"%s\" — l'elenco a sinistra e' vuoto PER QUESTO, non perche'"
				" l'eroe non abbia ability proprie.\nGli id canonici sono: %s"),
			*Modello.GetHeroFilter().ToString(),
			Id.Num() > 0 ? *FString::Join(Id, TEXT(" · ")) : TEXT("(il catalogo non ne dichiara nessuno)")));
	}

	case FRTLabViewModel::EFilterState::HeroWithEmptyKit:
		// ⛔ L'altro modo in cui la lista resta vuota, e senza questo ramo si legge identico al precedente.
		// `ListHeroKit` esclude le azioni core di proposito: un kit a zero voci e' un fatto del catalogo.
		return FText::FromString(FString::Printf(
			TEXT("%s — trovato, ma il suo kit proprio e' VUOTO: l'elenco a sinistra e' vuoto per questo."
				" Le azioni core non ne fanno parte, per scelta.\nPV %d · MP %d · vista %d"),
			*Eroe.HeroId.ToString(), Eroe.MaxHealth, Eroe.MovePoints, Eroe.VisionRange));

	case FRTLabViewModel::EFilterState::HeroWithKit:
		break;
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
	const ERTActionReadoutResult Esito = Modello.DescribeSelection(Parametri);

	// `#3473`: nessuna unita' porta quest'azione col suo id, quindi un valore «letto» non esiste. Si mostra la
	// sola casa del catalogo e si dice perche' — il «letto» di un oggetto costruito dal Lab sarebbe inventato,
	// e il suo ⚠ un falso allarme.
	if (Esito == ERTActionReadoutResult::CatalogOnly)
	{
		FString SoloCatalogo = FString::Printf(
			TEXT("%s — solo catalogo: nessuna unita' la impugna, quindi non c'e' un valore letto"),
			*Selezionata.ToString());
		for (const FRTActionParameterView& P : Parametri)
		{
			SoloCatalogo += FString::Printf(TEXT("\n  %s: catalogo %d"), *P.ParameterKey.ToString(), P.DeclaredValue);
		}
		return FText::FromString(SoloCatalogo);
	}
	if (Esito != ERTActionReadoutResult::Ok)
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

	// Lo stato dell'ultimo lancio e' del modello: si azzera a fine PIE e a ogni gesto successivo.
	if (Modello.WasLaunchFinished())
	{
		return LOCTEXT("PieTerminato", "PIE terminato: le CVar sono tornate com'erano.");
	}

	const FString& IdLanciato = Modello.LaunchedScenarioId();
	if (!IdLanciato.IsEmpty())
	{
		// La riga di log la scrive `FRTScenarioCoordinator` e COMINCIA cosi'; seguono turni e pausa.
		return FText::FromString(FString::Printf(
			TEXT("PIE richiesto per %s su L_DevSandbox.\nNel log cerca: [RT-Test] AUTO-RUN %s (da: console rt.Test.Scenario)\n"
				 "A fine PIE le CVar tornano com'erano."),
			*IdLanciato, *IdLanciato));
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
