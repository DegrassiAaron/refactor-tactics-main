#include "UI/Feed/RTUnitOverlayWidget.h"

#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/Inspect/RTIconLibrary.h"       // ResolveIcon: la chiave la porta la vista, il catalogo la traduce
#include "UI/Inspect/RTIconCatalogData.h"   // FRTIconResolution e URTIconCatalogData: valori di ritorno e campo
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

	if (SkillCalloutOutline)
	{
		// Il contorno del fumetto prende il colore di squadra **cosi' com'e'** nella vista, non quello del nome:
		// l'avviso di fuoco amico o di bersaglio sovrascrive il NOME perche' parla del bersaglio, mentre il
		// fumetto dice chi sta usando la skill e la squadra non cambia con il lock-in.
		SkillCalloutOutline->SetBrushColor(View.TeamColor);
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

	// Lo stato di RIPOSO del token, riaffermato a ogni aggiornamento.
	//
	// 🔑 **Serve perche' il riposo non sia una cosa che accade solo se qualcosa e' successo prima.** Senza
	// questa riga un `WBP` che porta l'elemento lo mostrerebbe **vuoto ma presente** dal primo fotogramma,
	// fino al primo colpo — e un'unita' che non ha mai incassato avrebbe comunque una riga di testo sopra la
	// testa. E' la stessa ragione per cui `OverlayWidget` nasce spento in `ARTUnit`: si accende chi ha
	// qualcosa da dire.
	//
	// ⚠️ Non spegne un token in corso: `bTokenActive` lo protegge, e questa funzione gira **ogni**
	// fotogramma (`ARTHUD::UpdateObserverVeil`).
	if (DamageTokenText && !bTokenActive)
	{
		DamageTokenText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Lo stato di RIPOSO del fumetto della skill (`#4025`), riaffermato a ogni aggiornamento come quello del token.
	//
	// 🔑 **Serve perche' `ApplySkillCalloutState` gira solo su un evento** (`Push`, `Clock`, `Clear`, cambio di
	// vista), e un'unita' che non ha mai gridato puo' non riceverne nessuno: l'orologio ha motivo di arrivare solo a
	// chi ha un fumetto vivo. Senza questa riga un `WBP` il cui root nasce `Visible` mostrerebbe una bolla **vuota ma
	// presente** sopra OGNI unita' dal primo fotogramma, fino al primo grido — che per molte non arriva mai.
	//
	// ⚠️ Non spegne un fumetto in corso: `bCalloutLive` lo protegge, e questa funzione gira **ogni** fotogramma
	// (`ARTHUD::UpdateObserverVeil`). Per questo e' un `Collapsed` secco e non una chiamata a
	// `ApplySkillCalloutState`: il riposo non ha bisogno di rileggere opacita' e scala a ogni giro.
	if (SkillCalloutRoot && !bCalloutLive)
	{
		SkillCalloutRoot->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Il layout puo' aggiungere il suo; non e' piu' il canale del disegno.
	OnOverlayUpdated();
}


float URTUnitOverlayWidget::FadeAlpha(float Elapsed, float Duration)
{
	if (Duration <= 0.f)
	{
		// Durata nulla o negativa: gia' finita. ⚠️ Non e' un errore da segnalare — `DamageTokenSeconds = 0`
		// e' il modo dichiarato per spegnere l'effetto senza ricompilare.
		return 0.f;
	}

	// 🔴 **`1 - t` e non un `Lerp`, e il clamp sta sul TEMPO e non sul risultato**: cosi' a `Elapsed` esatto
	// come `Duration` la sottrazione da' `0.f` esatto, e la barra torna a `(1,1)` invece di fermarsi a un
	// millesimo di scala in piu' per sempre.
	const float T = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
	return 1.f - T;
}

void URTUnitOverlayWidget::PushDamageToken(const FRTDamageTokenView& Token)
{
	// ⛔ **Non si tocca `View`.** La vita che la barra mostra continua ad arrivare da `SetOverlayView`:
	// questo canale aggiunge il CAMBIAMENTO sopra quello che mostra lo STATO, e scrivere qui renderebbe
	// questo widget una seconda fonte di cio' che l'unita' e'.

	if (DamageTokenText)
	{
		// Il testo e' gia' composto da `URTHudViewModel::BuildDamageToken`: qui non si decide niente, si
		// posa. Il caso zero ha la sua etichetta, e non arriva qui come stringa vuota.
		DamageTokenText->SetText(FText::FromString(Token.Label));
		DamageTokenText->SetRenderOpacity(1.f);
		DamageTokenText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// Riparte da zero anche se il token precedente non era finito: `#2455` fissa `un evento -> un token`, e
	// in v0.1 non c'e' aggregazione. Due colpi ravvicinati mostrano il secondo, che e' il piu' recente —
	// ⚠️ **ed e' un limite dichiarato**, non una svista: la leggibilita' dell'affollamento e' lavoro v0.3,
	// e l'epic `#2453` la tiene fuori finche' il problema non e' stato misurato.
	TokenElapsed = 0.f;
	bTokenActive = true;

	// #3600: la pulsazione la decide il token (`bPulsesHealthBar`), perche' anche una cura cambia la vita.
	if (Token.bPulsesHealthBar)
	{
		PulseHealthBar();
	}
	// ⚠️ Nessun `else` che spenga l'enfasi: con `bPulsesHealthBar == false` non e' cambiato niente sulla vita
	// (un colpo a zero, una cura a salute piena), e un'enfasi direbbe il contrario di cio' che e' successo. Il
	// token compare lo stesso — il fatto e' avvenuto — ma la barra tace, perche' non ha nulla da raccontare.
}

void URTUnitOverlayWidget::PushSkillCallout(const FRTSkillCalloutView& Callout)
{
	ActiveCallout = Callout;

	// Il tempo riparte dall'origine del fumetto: `Elapsed = CalloutNow - StartAt = 0`, quindi nasce col pop e
	// pieno di opacita'. Il resto del tempo lo porta `SetSkillCalloutClock`.
	CalloutNow = Callout.StartAt;
	bCalloutLive = true;

	if (SkillCalloutText)
	{
		// Il maiuscolo e' presentazione e si applica QUI, al momento di posare il testo: la vista e il kit
		// conservano il nome com'e' scritto.
		SkillCalloutText->SetText(FText::FromString(Callout.Label.ToUpper()));
	}

	ApplySkillCalloutState();
}

void URTUnitOverlayWidget::SetSkillCalloutClock(float Now)
{
	CalloutNow = Now;
	ApplySkillCalloutState();
}

void URTUnitOverlayWidget::ClearSkillCallout()
{
	bCalloutLive = false;
	ApplySkillCalloutState();
}

void URTUnitOverlayWidget::SetStrategicPresentation(bool bStrategic)
{
	bStrategicPresentation = bStrategic;
	ApplySkillCalloutState();
}

void URTUnitOverlayWidget::ApplySkillCalloutState()
{
	// Senza il root non c'e' niente da accendere: il widget degrada in silenzio, come ogni binding opzionale.
	if (!SkillCalloutRoot)
	{
		return;
	}

	// 🔑 Le due curve sono le funzioni pure di `URTHudViewModel`, e il tempo e' quello che il playback ha portato:
	// qui non si somma nessun `DeltaTime`.
	const float Elapsed = CalloutNow - ActiveCallout.StartAt;
	const float Alpha = URTHudViewModel::SkillCalloutAlpha(Elapsed, SkillCalloutSeconds);
	const float Scale = URTHudViewModel::SkillCalloutScale(Elapsed);

	SkillCalloutRoot->SetRenderOpacity(Alpha);
	SkillCalloutRoot->SetRenderScale(FVector2D(Scale, Scale));

	// Riposo COMPLETO e non trasparenza: un widget invisibile ma presente e' ancora un elemento nel layout, e
	// sopra un cilindro lo spazio e' la risorsa scarsa.
	const bool bHidden = !bCalloutLive || bStrategicPresentation || Alpha <= 0.f;
	SkillCalloutRoot->SetVisibility(bHidden ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void URTUnitOverlayWidget::PulseHealthBar()
{
	PulseElapsed = 0.f;
	bPulseActive = true;
}

void URTUnitOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 🔴 **Il tempo scorre QUI e non decide nessun esito** (invariante #1, `AGENTS.md` §3). `DeltaTime` e'
	// vietato al resolver competitivo; questa e' presentazione pura, e cio' che misura e' per quanti
	// fotogrammi una cifra resta a schermo. Un fotogramma perso allunga o accorcia un'animazione e **non
	// cambia una virgola** di cio' che e' successo nel turno.
	//
	// ⚠️ Il fumetto della skill (`#4025`) NON scorre qui: il suo tempo e' quello del playback, che arriva da fuori
	// con `SetSkillCalloutClock`. Un secondo orologio in questa funzione lo farebbe seguire al frame.

	if (bTokenActive)
	{
		TokenElapsed += InDeltaTime;
		const float Alpha = FadeAlpha(TokenElapsed, DamageTokenSeconds);
		if (DamageTokenText)
		{
			DamageTokenText->SetRenderOpacity(Alpha);
		}
		if (Alpha <= 0.f)
		{
			// Riposo, e riposo COMPLETO: nascosto, non lasciato trasparente. Un widget invisibile ma
			// presente e' ancora un elemento nel layout, e sopra un cilindro lo spazio e' la risorsa scarsa.
			bTokenActive = false;
			if (DamageTokenText)
			{
				DamageTokenText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	if (bPulseActive)
	{
		PulseElapsed += InDeltaTime;
		const float Alpha = FadeAlpha(PulseElapsed, HealthPulseSeconds);
		if (HealthBar)
		{
			// ⚠️ **Scala di render, non colore.** Il colore della barra e' dell'asset, e per modularlo
			// bisognerebbe conoscerne il valore a riposo — cioe' duplicarlo qui, con la divergenza che ne
			// segue. La scala ha un riposo che non va letto da nessuna parte: e' `(1,1)`.
			const float S = 1.f + (HealthPulseScale * Alpha);
			HealthBar->SetRenderScale(FVector2D(S, S));
		}
		if (Alpha <= 0.f)
		{
			bPulseActive = false;
			if (HealthBar)
			{
				// Il valore di riposo si RIASSEGNA esplicitamente invece di fidarsi che `1 + 0.18*0` sia
				// esattamente `1`: e' la stessa riga che l'AC di `#2455` chiede — *«torna a riposo senza
				// restare in uno stato transitorio»* — e costa una moltiplicazione risparmiata.
				HealthBar->SetRenderScale(FVector2D(1.f, 1.f));
			}
		}
	}
}
