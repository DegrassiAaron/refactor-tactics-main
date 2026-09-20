#include "PieSession/RTPieVerdictOverlay.h"

#include "Engine/GameInstance.h"
#include "PieSession/RTPieSessionSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

FRTPieOverlayPlacement URTPieVerdictOverlay::Placement()
{
	// I valori di default della struct sono gia' quelli voluti: la funzione esiste perche' il layout li
	// legga da UN posto solo, e perche' un gate possa guardarli senza aprire uno schermo.
	return FRTPieOverlayPlacement();
}

FText URTPieVerdictOverlay::ComposePrompt(const FString& PieItem, int32 Criterion,
	const FString& WhatToWatch)
{
	FString Testo = PieItem;
	if (Criterion != INDEX_NONE)
	{
		Testo += FString::Printf(TEXT("  criterio (%d)"), Criterion);
	}

	if (!WhatToWatch.IsEmpty())
	{
		Testo += TEXT("\n\n") + WhatToWatch;
	}

	Testo += TEXT("\n\n[1] si'    [2] no    [3] non giudicabile    [Esc] interrompi");
	return FText::FromString(Testo);
}

ERTPieVerdict URTPieVerdictOverlay::VerdictForKey(const FKey& Key)
{
	if (Key == EKeys::One || Key == EKeys::NumPadOne) { return ERTPieVerdict::Pass; }
	if (Key == EKeys::Two || Key == EKeys::NumPadTwo) { return ERTPieVerdict::Fail; }
	if (Key == EKeys::Three || Key == EKeys::NumPadThree) { return ERTPieVerdict::NotJudgeable; }
	return ERTPieVerdict::Pending;
}

FText URTPieVerdictOverlay::CurrentPrompt() const
{
	const UGameInstance* GI = GetGameInstance();
	const URTPieSessionSubsystem* Conduttore = GI ? GI->GetSubsystem<URTPieSessionSubsystem>() : nullptr;
	if (!Conduttore || Conduttore->State() != ERTPieSessionState::AwaitingVerdict)
	{
		return FText::GetEmpty();
	}

	const FRTPieSessionStep* Passo = Conduttore->CurrentStep();
	return Passo ? ComposePrompt(Passo->PieItem, Passo->Criterion, Passo->WhatToWatch)
	             : FText::GetEmpty();
}

TSharedRef<SWidget> URTPieVerdictOverlay::RebuildWidget()
{
	// Deliberatamente spoglio: deve stare SOPRA la scena senza coprirla, perche' cio' che si giudica e'
	// la scena. Un pannello curato qui toglierebbe spazio a quello che si sta guardando.
	//
	// 🔴 **L'allineamento e' esplicito perche' il default lo posava sui NOMI DEGLI EROI.**
	// `AddToViewport` lascia il widget riempire il viewport, e un `SBorder` senza slot si posa in alto a
	// sinistra — dove sta `URTTeamRosterWidget`, la zona `TopLeft` delle otto
	// (`RTMatchWidgetAssetTests.cpp:991`). Verdetto d'autore alla prima seduta reale: *«la scritta sta
	// sotto i nomi degli eroi e non e' visualizzabile»*. Colonna sinistra, centrato in verticale (#3242).
	const FRTPieOverlayPlacement Posa = Placement();

	return SNew(SOverlay)
		+ SOverlay::Slot()
			.HAlign(Posa.Horizontal)
			.VAlign(Posa.Vertical)
			.Padding(FMargin(Posa.LeftMargin, 0.f, 0.f, 0.f))
			[
				// ⛔ La larghezza e' limitata perche' il CENTRO resta libero: e' il contratto dello
				// Screen HUD — la board non si copre — e un prompt che cresce con la nota dello scenario
				// lo violerebbe senza che nessuno lo decida.
				SNew(SBox)
					.MaxDesiredWidth(Posa.MaxWidth)
					[
						SNew(SBorder)
							.Padding(FMargin(14.f, 10.f))
							.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							// Opaco all'85%: sotto ci puo' essere il pannello dell'unita' selezionata
							// (`MiddleLeft`), e un fondo troppo trasparente rifarebbe lo stesso difetto
							// in un'altra fascia.
							.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.85f))
							[
								SAssignNew(PromptText, STextBlock)
									.Text_Lambda([this]() { return CurrentPrompt(); })
									// 🔴 Il COLORE si dichiara, non si eredita: senza, il testo prende
									// quello di default dello stile e puo' risultare illeggibile sul
									// proprio fondo. E' il secondo difetto di #3242, indipendente dal
									// primo — spostare il pannello senza fissarlo avrebbe spostato il
									// problema invece di chiuderlo.
									.ColorAndOpacity(FSlateColor(FLinearColor::White))
									.AutoWrapText(true)
							]
					]
			];
}

FReply URTPieVerdictOverlay::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UGameInstance* GI = GetGameInstance();
	URTPieSessionSubsystem* Conduttore = GI ? GI->GetSubsystem<URTPieSessionSubsystem>() : nullptr;
	if (!Conduttore || Conduttore->State() != ERTPieSessionState::AwaitingVerdict)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		Conduttore->Abort();
		return FReply::Handled();
	}

	const ERTPieVerdict Verdetto = VerdictForKey(InKeyEvent.GetKey());
	if (Verdetto == ERTPieVerdict::Pending)
	{
		// Un tasto qualunque NON e' un verdetto, e non deve nemmeno essere consumato: chi sta guardando
		// puo' voler muovere la camera prima di decidere.
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	Conduttore->SubmitVerdict(Verdetto, FString());
	return FReply::Handled();
}
