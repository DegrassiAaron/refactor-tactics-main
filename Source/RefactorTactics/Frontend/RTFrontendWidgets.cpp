#include "Frontend/RTFrontendWidgets.h"

#define LOCTEXT_NAMESPACE "RTFrontend"

// ─── Loading ─────────────────────────────────────────────────────────────────────────────────────

void URTLoadingScreenWidgetBase::SetPhase(ERTLoadPhase InPhase)
{
	Phase = InPhase;
}

FText URTLoadingScreenWidgetBase::GetPhaseText() const
{
	// Il testo nasce qui e non nel Blueprint, per la stessa ragione per cui esiste `DescribeOutcome`: due
	// schermate che raccontassero la stessa fase con parole diverse sarebbero due verita' sullo stesso
	// stato. `switch` senza `default`: una fase nuova non compila finche' non ha una riga.
	switch (Phase)
	{
	case ERTLoadPhase::Idle:
	case ERTLoadPhase::Ready:
		// Nessuna attesa da spiegare. Vuoto e' il valore giusto: una schermata di caricamento che dice
		// «pronto» resta a schermo dicendo il contrario di cio' che sta facendo.
		return FText::GetEmpty();

	case ERTLoadPhase::Map:
		return LOCTEXT("PhaseMap", "Caricamento della mappa…");
	case ERTLoadPhase::Scenario:
		return LOCTEXT("PhaseScenario", "Allestimento della partita…");
	case ERTLoadPhase::Bots:
		return LOCTEXT("PhaseBots", "Schieramento delle unita'…");
	}

	return FText::GetEmpty();
}

ESlateVisibility URTLoadingScreenWidgetBase::GetLoadingVisibility() const
{
	return IsLoading() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

// ─── Modale d'errore ─────────────────────────────────────────────────────────────────────────────

void URTErrorModalWidgetBase::ShowFor(const FRTStartupNote& Note)
{
	// ⚠️ Il filtro e' qui e non nel chiamante: un modale armato con un ripiego mostrerebbe «non si e'
	// potuto partire» sopra una partita che sta girando. La divisione la decide `IsFatal`, che e' l'unico
	// posto in cui e' scritta.
	if (!URTStartupReportLibrary::IsFatal(Note.Outcome))
	{
		return;
	}

	Outcome = Note.Outcome;
	Detail = Note.Detail;
}

void URTErrorModalWidgetBase::ShowFromReport(const FRTStartupReport& Report)
{
	// Quale nota sia quella fatale lo decide `FindFatal`, che a sua volta chiede a `IsFatal`: il
	// chiamante non deve saperlo, e non deve poterlo ridecidere.
	const ERTStartupOutcome Fatal = URTStartupReportLibrary::FindFatal(Report);
	if (Fatal == ERTStartupOutcome::Ok)
	{
		return;
	}

	for (const FRTStartupNote& Note : Report.Notes)
	{
		if (Note.Outcome == Fatal)
		{
			ShowFor(Note);
			// La fase arriva **solo** da qui: e' l'unica delle tre funzioni che arma ad avere un report.
			// Dopo `ShowFor` e non prima, cosi' un modale che non si e' armato non porta nemmeno la fase.
			if (IsArmed())
			{
				PhaseWhenArmed = Report.Phase;
			}
			return;
		}
	}
}

void URTErrorModalWidgetBase::ShowForOutcome(ERTStartupOutcome InOutcome, const FString& InDetail)
{
	// Passa da `ShowFor` invece di scrivere i campi: il filtro sui non-fatali resta in un posto solo.
	ShowFor(FRTStartupNote(InOutcome, InDetail));
}

FText URTErrorModalWidgetBase::GetReasonText() const
{
	return URTStartupReportLibrary::DescribeOutcome(Outcome);
}

FString URTErrorModalWidgetBase::GetDetail() const
{
#if UE_BUILD_SHIPPING
	// In Shipping il dettaglio **non esiste**, non e' nascosto: e' la differenza fra una guardia nel
	// Blueprint — che qualcuno puo' dimenticare — e una stringa che non arriva. Il progetto usa gia'
	// questa guardia in dodici file.
	return FString();
#else
	return Detail;
#endif
}

bool URTErrorModalWidgetBase::ShouldShowDetails() const
{
	return !GetDetail().IsEmpty();
}

ESlateVisibility URTErrorModalWidgetBase::GetModalVisibility() const
{
	// `Visible` e non `SelfHitTestInvisible`: un modale deve **fermare** i click su cio' che sta sotto.
	// E' l'unica delle tre schermate per cui questo valore non e' una formalita'.
	return IsArmed() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

ESlateVisibility URTErrorModalWidgetBase::GetDetailsVisibility() const
{
	// In Shipping `GetDetail()` e' vuoto per costruzione, quindi qui esce sempre `Collapsed`: il pulsante
	// non esiste invece di aprire il vuoto.
	return ShouldShowDetails() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

// ─── Banner di ripiego ───────────────────────────────────────────────────────────────────────────

FText URTFallbackBannerWidgetBase::GetLinesAsText() const
{
	if (Lines.Num() == 0)
	{
		return FText::GetEmpty();
	}

	// `FText::Join` invece di concatenare stringhe: il separatore resta un `FText` e la localizzazione
	// non si perde per strada. Il carattere e' un a capo vero, non un token da sostituire nel widget.
	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

ESlateVisibility URTFallbackBannerWidgetBase::GetBannerVisibility() const
{
	// `Collapsed` e non `Hidden`: un banner assente non deve occupare spazio nel layout sotto.
	return HasAnything() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

void URTFallbackBannerWidgetBase::SetFromReport(const FRTStartupReport& Report)
{
	Lines.Reset();

	for (const FRTStartupNote& Note : Report.Notes)
	{
		// Solo i degradati: un fatale non ha una partita sotto cui stare, ed e' del modale.
		if (!URTStartupReportLibrary::IsDegraded(Note.Outcome))
		{
			continue;
		}

		FText Line = URTStartupReportLibrary::DescribeOutcome(Note.Outcome);

		// Il dettaglio si aggiunge quando c'e', ma **non sostituisce** la riga: la causa deve restare
		// leggibile anche quando il dettaglio e' un id o un numero che non dice niente a chi guarda.
		if (!Note.Detail.IsEmpty())
		{
			Line = FText::Format(LOCTEXT("BannerLineWithDetail", "{0} ({1})"),
				Line, FText::FromString(Note.Detail));
		}

		Lines.Add(Line);
	}
}

#undef LOCTEXT_NAMESPACE
