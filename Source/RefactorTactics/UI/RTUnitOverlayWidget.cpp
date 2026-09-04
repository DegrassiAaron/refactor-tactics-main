#include "UI/RTUnitOverlayWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/RTIconLibrary.h"       // ResolveIcon: la chiave la porta la vista, il catalogo la traduce
#include "UI/RTIconCatalogData.h"   // FRTIconResolution e URTIconCatalogData: valori di ritorno e campo
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

URTUnitOverlayWidget::URTUnitOverlayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Il catalogo di default, cosi' che l'unico `WBP_RT_UnitOverlay` non debba assegnarlo a mano — e
	// soprattutto non possa dimenticarlo. Resta `EditDefaultsOnly`: il Blueprint conserva l'ultima parola.
	//
	// ⚠️ Se l'asset non c'e', il campo resta nullo e `ResolveIcon` mostra il missing-icon con la sua
	// warning: degrada, non crasha — la stessa forma con cui `OverlayWidgetClass` e `ContactGhostMaterial`
	// sono opzionali.
	static ConstructorHelpers::FObjectFinder<URTIconCatalogData> CatalogoDefault(
		TEXT("/Game/RT/UI/DA_IconCatalog.DA_IconCatalog"));
	if (CatalogoDefault.Succeeded())
	{
		IconCatalog = CatalogoDefault.Object;
	}
}

FString URTUnitOverlayWidget::ComposeStatusDurationLabel(int32 RemainingTurns, bool bCellBound)
{
	if (bCellBound)
	{
		// 🔴 Legato alla cella: **nessun conteggio**. Dura finche' l'unita' resta dov'e', e stampare un
		// numero direbbe una cosa falsa su quando finisce.
		return FString();
	}

	// ⚠️ Una durata non positiva non e' un tempo da mostrare: uno stato scaduto non compare gia' nella
	// vista (`GetActiveStatusTags` scarta i residui non positivi), e se ci arrivasse comunque sarebbe un
	// difetto a monte — non un «0» da disegnare sopra la testa di un'unita'.
	return RemainingTurns > 0 ? FString::FromInt(RemainingTurns) : FString();
}

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
		// 🔴 **L'avviso di fuoco amico sta sul NOME**, ed e' il posto giusto: c'e' gia', l'occhio ci va gia'
		// per sapere chi e' chi, e non aggiunge un elemento nuovo da imparare. E' la stessa forma che il
		// canvas usava prima di `#2288` — cambia il supporto, non il linguaggio.
		FString Etichetta = View.DisplayName;
		FLinearColor Colore = View.TeamColor;
		if (View.bFriendlyFire)
		{
			// L'avviso deve essere piu' forte del colore di squadra: e' l'unico caso in cui chi guarda
			// potrebbe voler cambiare idea prima del lock-in.
			Etichetta = TEXT("! ") + Etichetta;
			Colore = FLinearColor(1.f, 0.6f, 0.12f, 1.f);
		}
		else if (View.bTargeted)
		{
			Etichetta = TEXT("* ") + Etichetta;
			Colore = FLinearColor(1.f, 0.35f, 0.3f, 1.f);
		}

		NameText->SetText(FText::FromString(Etichetta));
		// Fuori dai due avvisi il colore viene dalla vista, che lo ha derivato da `bIsAlly`: alleato o
		// avversario **per chi guarda**, mai «team 0/team 1».
		NameText->SetColorAndOpacity(FSlateColor(Colore));
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

	if (StatusBox)
	{
		// Svuotato e riempito: gli stati cambiano di numero e di ordine, e un pool da riciclare sarebbe una
		// seconda struttura da tenere allineata a `View.Statuses`.
		StatusBox->ClearChildren();

		for (const FRTStatusBadgeView& Badge : View.Statuses)
		{
			// L'ICONA. `Badge.IconId` e' gia' derivato dal tag da `BuildStatusBadges` (`#2274`): qui non si
			// compone una seconda chiave, si risolve quella.
			//
			// ⚠️ **Il `Consumer` non e' decorativo**: `ResolveIcon` lo pretende perche' la warning di
			// un'icona mancante dica **chi** l'ha chiesta — senza, resta un messaggio che non aiuta nessuno,
			// ed e' scritto nel suo doc-comment.
			const FRTIconResolution Risolta = URTIconLibrary::ResolveIcon(
				IconCatalog, Badge.IconId, TEXT("RTUnitOverlayWidget"));

			// ⚠️ **`LoadSynchronous` e non un caricamento asincrono.** Le undici icone di stato sono
			// texture piccole e gia' referenziate dal catalogo, quindi in pratica sono in memoria; un
			// caricamento differito qui significherebbe un fotogramma con l'icona vuota **a ogni
			// aggiornamento**, cioe' uno sfarfallio invece di un risparmio.
			UTexture2D* Texture = Risolta.Asset.LoadSynchronous();

			// Icona e durata SOVRAPPOSTE, non affiancate: sopra la testa di un'unita' lo spazio orizzontale
			// e' la risorsa scarsa, e dieci stati affiancati con il numero accanto sarebbero il doppio
			// larghi. Il conteggio sta nell'angolo dell'icona.
			UOverlay* Casella = NewObject<UOverlay>(this);
			if (Casella == nullptr) { continue; }

			if (Texture)
			{
				UImage* Icona = NewObject<UImage>(this);
				if (Icona)
				{
					Icona->SetBrushFromTexture(Texture, /*bMatchSize*/ false);
					Icona->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
					Casella->AddChildToOverlay(Icona);
				}
			}
			else
			{
				// 🔴 **Il ripiego resta, e serve.** Catalogo assente o chiave non risolta: si mostra la
				// sigla invece di uno spazio vuoto, perche' «lo stato c'e' e non so disegnarlo» e «non c'e'
				// nessuno stato» devono restare distinguibili a schermo. `ResolveIcon` ha gia' scritto la
				// warning che nomina chiave e consumer.
				UTextBlock* Ripiego = NewObject<UTextBlock>(this);
				if (Ripiego)
				{
					Ripiego->SetText(FText::FromString(
						Badge.Tag.ToString().Replace(TEXT("Status."), TEXT(""))));
					Casella->AddChildToOverlay(Ripiego);
				}
			}

			// La durata sopra l'icona, in basso a destra. Vuota per gli stati legati alla cella: li' non c'e'
			// un conteggio, e la regola sta in una funzione pura invece che in questo `if`.
			const FString Durata = ComposeStatusDurationLabel(Badge.RemainingTurns, Badge.bCellBound);
			if (!Durata.IsEmpty())
			{
				UTextBlock* Conteggio = NewObject<UTextBlock>(this);
				if (Conteggio)
				{
					Conteggio->SetText(FText::FromString(Durata));
					if (UOverlaySlot* SlotConteggio = Casella->AddChildToOverlay(Conteggio))
					{
						SlotConteggio->SetHorizontalAlignment(HAlign_Right);
						SlotConteggio->SetVerticalAlignment(VAlign_Bottom);
					}
				}
			}

			StatusBox->AddChild(Casella);
		}
	}

	// Il layout puo' aggiungere il suo; non e' piu' il canale del disegno.
	OnOverlayUpdated();
}
