#include "UI/RTInspectCardWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Input/Events.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"          // la squadra di chi guarda si rilegge dal controller, non da una cache del widget
#include "UI/RTReactionWindowViewModel.h"  // IsWindowOpen: la finestra di reazione nasconde la scheda (spec §3 regola 11)
#include "UI/RTHUD.h"
#include "Unit/RTUnit.h"

namespace
{
	/** Crea una riga col proprietario giusto, o col mondo in sua assenza. Nome distinto per file: il progetto usa unity build. */
	URTInspectLineWidget* InspectCreateRow(UUserWidget& Parent, TSubclassOf<URTInspectLineWidget> Class)
	{
		if (!Class)
		{
			return nullptr;
		}
		if (APlayerController* Owner = Parent.GetOwningPlayer())
		{
			return CreateWidget<URTInspectLineWidget>(Owner, Class);
		}
		if (UWorld* World = Parent.GetWorld())
		{
			return CreateWidget<URTInspectLineWidget>(World, Class);
		}
		return nullptr;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// URTInspectLineWidget
// ---------------------------------------------------------------------------------------------------------------------

void URTInspectLineWidget::SetLine(const FString& InText)
{
	const bool bTextChanged = Text != InText;
	Text = InText;
	// Il testo si riscrive solo se cambia: la scheda ricompone a 5 Hz, e un `FText` nuovo per riga a ogni giro invaliderebbe Slate per niente.
	if (LineText && (bTextChanged || !bTextWritten))
	{
		LineText->SetText(FText::FromString(Text));
		bTextWritten = true;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// URTInspectCardWidget
// ---------------------------------------------------------------------------------------------------------------------

void URTInspectCardWidget::SetViewForTest(const FRTInspectView& InView)
{
	ViewForTest = InView;
}

void URTInspectCardWidget::RefreshLook(bool bRecompose)
{
	const ARTPlayerController* PC = ResolveCommandController();
	const ARTUnit* Subject = GetInspectedUnit();
	const bool bPinned = PC != nullptr && PC->IsInspectorPinned() && Subject != nullptr;
	// 🔴 **`GetPointerContext` non produce `ReactionWindow`**: nessuno lo scrive ancora (E14), e una risoluzione sospesa su una finestra
	// vale `ResolutionPlayback`, in cui la scheda resterebbe a schermo e continuerebbe a consumare i gesti. La finestra si legge quindi dal
	// suo view model, che e' il solo a saperla aperta. Senza controller il contesto non e' un modale ne' una finestra: la scheda non ha
	// motivo di nascondersi.
	const URTReactionWindowViewModel* Window = GetReactionWindow();
	const bool bReactionWindowOpen = Window != nullptr && Window->IsWindowOpen();
	const ERTPointerContext Context = ContextForTest.IsSet() ? ContextForTest.GetValue()
		: (bReactionWindowOpen ? ERTPointerContext::ReactionWindow
			: (PC != nullptr ? PC->GetPointerContext() : ERTPointerContext::Planning));

	// Chiusa davvero (tasto destro, conoscenza): lo stato di scorrimento e la scheda composta non devono sopravvivere, o la stessa unita'
	// fissata di nuovo riaprirebbe a meta' di una scheda vecchia. Nascosta da un modale e' un'altra cosa: lo stato resta.
	if (!bPinned)
	{
		CachedSubject.Reset();
		CachedHoverIndex = INDEX_NONE;
		CachedView = FRTInspectView();
		FirstLine = 0;
	}

	if (!URTInspectLibrary::ShouldShowCard(Context, bPinned))
	{
		SetCardShown(false);
		return;
	}

	if (CachedSubject.Get() != Subject)
	{
		CachedSubject = Subject;
		FirstLine = 0;
		bRecompose = true;
	}

	// 🔑 **L'hover vale solo se e' dello STESSO soggetto** (decisione d'autore del 2026-10-31): `SetSkillHover` registra l'unita' dello slot, che e'
	// quella della barra propria o di quella in sola lettura, e il box del nemico non deve cambiare perche' il cursore e' sulla tua barra. Si
	// riconosce dall'identita' dell'unita', non dall'indice. Un indice fuori kit passa comunque: lo scarta `BuildInspectSubject`, e la riga di invito resta.
	const int32 HoverIndex = (PC != nullptr && PC->GetSkillHoverUnit() == Subject) ? PC->GetSkillHoverIndex() : INDEX_NONE;
	if (CachedHoverIndex != HoverIndex)
	{
		// Entrare, cambiare o togliere l'hover cambia il contenuto e la sua lunghezza: si ricompone SUBITO (non a 5 Hz) e si riparte dalla testata.
		CachedHoverIndex = HoverIndex;
		FirstLine = 0;
		bRecompose = true;
	}
	if (bRecompose)
	{
		// La squadra del cancello e' quella del controller, riletta a ogni composizione come fanno il velo e il fuoco: `GetPlayerTeamId()` e'
		// una cache scritta all'acquisizione del contesto, e un client che acquisisce prima che il PlayerState abbia la squadra lascerebbe
		// il cancello su quella sbagliata (`TeamIdOf` ripiega su 0, e un nemico di squadra 0 diventerebbe «proprio», sempre `Live`).
		const int32 ObserverTeam = PC != nullptr ? ARTPlayerState::TeamIdOf(PC) : GetPlayerTeamId();
		CachedView = ViewForTest.IsSet() ? ViewForTest.GetValue()
			: ARTHUD::BuildInspectCard(GetTurnManager(), Subject, GatherUnitsInWorld(), ObserverTeam, HoverIndex);
	}

	// Una scheda senza righe e' un nemico che la squadra non osserva: come una cella vuota, niente da mostrare.
	if (CachedView.Lines.Num() == 0)
	{
		SetCardShown(false);
		return;
	}

	const bool bWasShown = bShown;
	if (bRecompose || !bWasShown)
	{
		ApplyWindow();
	}
	SetCardShown(true);
}

void URTInspectCardWidget::ScrollBy(int32 Lines)
{
	if (CachedView.Lines.Num() == 0)
	{
		return;
	}
	FirstLine += Lines;
	ApplyWindow();
}

void URTInspectCardWidget::SetCardShown(bool bInShown)
{
	bShown = bInShown;
	if (CardRoot)
	{
		CardRoot->SetVisibility(bShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void URTInspectCardWidget::ApplyWindow()
{
	FirstLine = URTInspectLibrary::ClampFirstLine(CachedView.Lines.Num(), MaxRighe, FirstLine);
	RebuildLines(URTInspectLibrary::VisibleWindow(CachedView, FirstLine, MaxRighe));
}

void URTInspectCardWidget::RebuildLines(const FRTInspectView& Window)
{
	if (LinesBox == nullptr || !LineClass)
	{
		LineWidgets.Reset();
		return;
	}

	while (LineWidgets.Num() > Window.Lines.Num())
	{
		if (URTInspectLineWidget* Last = LineWidgets.Last())
		{
			Last->RemoveFromParent();
		}
		LineWidgets.Pop();
	}
	while (LineWidgets.Num() < Window.Lines.Num())
	{
		URTInspectLineWidget* Row = InspectCreateRow(*this, LineClass);
		if (Row == nullptr)
		{
			return;
		}
		LinesBox->AddChild(Row);
		LineWidgets.Add(Row);
	}

	for (int32 I = 0; I < Window.Lines.Num(); ++I)
	{
		LineWidgets[I]->SetLine(Window.Lines[I]);
		// Nessuna riga vede il cursore (non c'e' piu' un hover sulla riga): lasciano passare i gesti al pannello, che li consuma.
		LineWidgets[I]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void URTInspectCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SecondsSinceCompose += InDeltaTime;
	const bool bRecompose = SecondsSinceCompose >= 0.2f;
	if (bRecompose)
	{
		SecondsSinceCompose = 0.f;
	}
	RefreshLook(bRecompose);
}

bool URTInspectCardWidget::IsPrimaryButton(const FPointerEvent& Event)
{
	return Event.GetEffectingButton() == EKeys::LeftMouseButton;
}

FReply URTInspectCardWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// La rotella sopra il pannello e' del pannello (spec §3 regola 6): scorre la scheda e non arriva allo zoom del mondo. Si consuma
	// anche quando tutto entra e non c'e' niente da scorrere, perche' il gesto e' comunque sul pannello. Il delta e' proporzionale come
	// quello dello zoom: le tacche intere scorrono, la frazione resta per l'evento dopo.
	WheelRemainder += InMouseEvent.GetWheelDelta();
	const int32 Ticks = FMath::TruncToInt(WheelRemainder);
	WheelRemainder -= static_cast<float>(Ticks);
	if (Ticks != 0)
	{
		ScrollBy(-Ticks * LinesPerWheelStep);
	}
	return FReply::Handled();
}

FReply URTInspectCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return IsPrimaryButton(InMouseEvent) ? FReply::Handled() : Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply URTInspectCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Down e Up insieme: un Up che arrivasse al gioco senza il suo Down sarebbe un rilascio orfano.
	return IsPrimaryButton(InMouseEvent) ? FReply::Handled() : Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply URTInspectCardWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return IsPrimaryButton(InMouseEvent) ? FReply::Handled() : Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}
