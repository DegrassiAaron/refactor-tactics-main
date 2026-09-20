#include "PieSession/RTPieVerdictOverlay.h"

#include "Engine/GameInstance.h"
#include "PieSession/RTPieSessionSubsystem.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

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
	return SNew(SBorder)
		.Padding(FMargin(12.f, 8.f))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.75f))
		[
			SAssignNew(PromptText, STextBlock)
				.Text_Lambda([this]() { return CurrentPrompt(); })
				.AutoWrapText(true)
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
