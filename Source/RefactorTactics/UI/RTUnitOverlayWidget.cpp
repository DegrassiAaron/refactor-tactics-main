#include "UI/RTUnitOverlayWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

FString URTUnitOverlayWidget::ComposeHealthLabel(int32 Health, int32 MaxHealth)
{
	if (MaxHealth <= 0)
	{
		// Nessuna unita', non un morto: un `0/0` direbbe che c'e' qualcuno a zero vita. La distinzione la
		// porta `MaxHealth`, ed e' la stessa che `BuildUnitOverlay` usa per la vista neutra.
		return FString();
	}

	// ⚠️ La vita si CLAMPA a zero e non sotto: il danno che eccede non si mostra come `-3/100`. Il valore
	// negativo, se mai arrivasse, e' un difetto del simulatore e va visto li', non stampato qui.
	return FString::Printf(TEXT("%d/%d"), FMath::Max(0, Health), MaxHealth);
}

float URTUnitOverlayWidget::SafeFraction(int32 Value, int32 Max)
{
	if (Max <= 0)
	{
		return 0.f;
	}
	return FMath::Clamp(static_cast<float>(Value) / static_cast<float>(Max), 0.f, 1.f);
}

void URTUnitOverlayWidget::SetOverlayView(const FRTUnitOverlayView& InView)
{
	View = InView;

	// Ogni elemento e' opzionale: un `WBP` incompleto mostra il resto invece di non compilare.
	if (NameText)
	{
		NameText->SetText(FText::FromString(View.DisplayName));
		// Il colore viene dalla vista, che lo ha derivato da `bIsAlly`: alleato o avversario **per chi
		// guarda**, mai «team 0/team 1».
		NameText->SetColorAndOpacity(FSlateColor(View.TeamColor));
	}

	if (HealthBar)
	{
		HealthBar->SetPercent(SafeFraction(View.Card.Health, View.Card.MaxHealth));
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(ComposeHealthLabel(View.Card.Health, View.Card.MaxHealth)));
	}

	if (ShieldBar)
	{
		// Lo scudo si misura su `MaxHealth`, non su se stesso: e' la stessa scala che usava il canvas, ed e'
		// cio' che rende leggibile «quanto scudo» rispetto a «quanta vita».
		ShieldBar->SetPercent(SafeFraction(View.Card.Shield, View.Card.MaxHealth));
	}

	if (EnergyBar)
	{
		EnergyBar->SetPercent(SafeFraction(View.Card.Energy, View.Card.MaxEnergy));
	}

	if (StatusBox)
	{
		// Svuotato e riempito: gli stati cambiano di numero e di ordine, e un pool da riciclare sarebbe una
		// seconda struttura da tenere allineata a `View.Statuses`.
		StatusBox->ClearChildren();

		for (const FRTStatusBadgeView& Badge : View.Statuses)
		{
			UTextBlock* Voce = NewObject<UTextBlock>(this);
			if (Voce == nullptr) { continue; }

			// ⚠️ **Testo e non icona, ed e' una scelta dichiarata.** Le undici `RT_UI_Icon_Status_*` esistono
			// e `Badge.IconId` porta gia' la chiave giusta, ma risolverla richiede il **catalogo**
			// (`URTIconLibrary::ResolveIcon` su `URTIconCatalogData`), che questo widget non ha e che
			// nessuno gli passa oggi. Mostrare la sigla e' il ripiego onesto: si vede quale stato c'e' e per
			// quanto, e chi aggancia il catalogo sostituisce QUESTO blocco senza toccare il resto.
			//
			// 🔑 Non e' il ripiego che `#2244` denunciava: quello mostrava **due** stati su undici e li
			// escludeva a vicenda. Qui ci sono tutti, ordinati per gravita', con la durata.
			const FString Sigla = Badge.Tag.ToString().Replace(TEXT("Status."), TEXT(""));
			const FString Testo = Badge.bCellBound
				// Legato alla cella: niente conteggio, perche' non c'e' un numero di turni da mostrare.
				? Sigla
				: FString::Printf(TEXT("%s %d"), *Sigla, Badge.RemainingTurns);

			Voce->SetText(FText::FromString(Testo));
			StatusBox->AddChild(Voce);
		}
	}

	// Il layout puo' aggiungere il suo; non e' piu' il canale del disegno.
	OnOverlayUpdated();
}
