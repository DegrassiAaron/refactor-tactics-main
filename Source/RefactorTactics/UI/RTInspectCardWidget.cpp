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

void URTInspectLineWidget::SetLine(const FString& InText, const ARTUnit* InHoverUnit, int32 InAbilityIndex)
{
	// La riga ha un hover registrato solo se il cursore e' entrato e non e' ancora uscito: e' il suo modo di sapere di essere sotto il
	// cursore, senza chiederlo a Slate. Se in quel momento cambia skill, quello vecchio si spegne e il nuovo si accende.
	const bool bSameTarget = HoverUnit.Get() == InHoverUnit && AbilityIndex == InAbilityIndex;
	const bool bRetarget = !bSameTarget && HoverRegisteredIndex != INDEX_NONE;
	if (bRetarget)
	{
		HoverEnd();
	}

	const bool bTextChanged = Text != InText;
	Text = InText;
	HoverUnit = InHoverUnit;
	AbilityIndex = InAbilityIndex;
	// Il testo si riscrive solo se cambia: la scheda ricompone a 5 Hz, e un `FText` nuovo per riga a ogni giro invaliderebbe Slate per niente.
	if (LineText && (bTextChanged || !bTextWritten))
	{
		LineText->SetText(FText::FromString(Text));
		bTextWritten = true;
	}

	if (bRetarget)
	{
		HoverBegin();
	}
}

void URTInspectLineWidget::HoverBegin()
{
	// Una riga che non e' di una skill non chiede niente: ne' la portata, ne' un log. Il passaggio del cursore non e' un gesto.
	const ARTUnit* Unit = HoverUnit.Get();
	if (Unit == nullptr || AbilityIndex < 0)
	{
		return;
	}
	if (ARTPlayerController* PC = ResolveHoverController())
	{
		PC->SetSkillHover(Unit, AbilityIndex);
		HoverRegisteredUnit = Unit;
		HoverRegisteredIndex = AbilityIndex;
	}
}

void URTInspectLineWidget::HoverEnd()
{
	// Una riga che non ha registrato niente non ha niente da spegnere: nemmeno l'hover di un'altra riga o della barra.
	if (HoverRegisteredIndex == INDEX_NONE)
	{
		return;
	}
	const ARTUnit* RegisteredUnit = HoverRegisteredUnit.Get();
	const int32 RegisteredIndex = HoverRegisteredIndex;
	HoverRegisteredUnit.Reset();
	HoverRegisteredIndex = INDEX_NONE;
	if (ARTPlayerController* PC = ResolveHoverController())
	{
		// Solo il PROPRIO hover: `ClearSkillHover` confronta unita' e posizione, quindi non cancella quello che nel frattempo e' entrato altrove.
		PC->ClearSkillHover(RegisteredUnit, RegisteredIndex);
	}
}

void URTInspectLineWidget::SetHoverControllerForTest(ARTPlayerController* InController)
{
	HoverControllerForTest = InController;
}

ARTPlayerController* URTInspectLineWidget::ResolveHoverController() const
{
	// L'iniezione dei test viene prima, e solo perche' in gioco e' sempre nulla: senza un `ULocalPlayer` una run headless non ha un
	// proprietario (vedi `URTActionSlotWidget::ResolveArmingController`).
	if (ARTPlayerController* Iniettato = HoverControllerForTest.Get())
	{
		return Iniettato;
	}
	return Cast<ARTPlayerController>(GetOwningPlayer());
}

void URTInspectLineWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	HoverBegin();
}

void URTInspectLineWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	HoverEnd();
}

void URTInspectLineWidget::NativeDestruct()
{
	HoverEnd();
	Super::NativeDestruct();
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
	if (bRecompose)
	{
		// La squadra del cancello e' quella del controller, riletta a ogni composizione come fanno il velo e il fuoco: `GetPlayerTeamId()` e'
		// una cache scritta all'acquisizione del contesto, e un client che acquisisce prima che il PlayerState abbia la squadra lascerebbe
		// il cancello su quella sbagliata (`TeamIdOf` ripiega su 0, e un nemico di squadra 0 diventerebbe «proprio», sempre `Live`).
		const int32 ObserverTeam = PC != nullptr ? ARTPlayerState::TeamIdOf(PC) : GetPlayerTeamId();
		CachedView = ViewForTest.IsSet() ? ViewForTest.GetValue()
			: ARTHUD::BuildInspectCard(GetTurnManager(), Subject, GatherUnitsInWorld(), ObserverTeam);
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
	// Nascosta: una riga collassata non riceve l'uscita del cursore, e l'hover che aveva registrato resterebbe appeso al controller
	// (la mappa continuerebbe a mostrare la portata di una skill che nessuna riga mostra piu').
	if (bShown && !bInShown)
	{
		for (URTInspectLineWidget* Row : LineWidgets)
		{
			if (Row) { Row->HoverEnd(); }
		}
	}
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
			Last->HoverEnd(); // una riga tolta dall'albero non riceve l'uscita del cursore
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

	const ARTUnit* Subject = CachedSubject.Get();
	for (int32 I = 0; I < Window.Lines.Num(); ++I)
	{
		const int32 AbilityIndex = Window.LineAbilityIndices.IsValidIndex(I) ? Window.LineAbilityIndices[I] : INDEX_NONE;
		LineWidgets[I]->SetLine(Window.Lines[I], Subject, AbilityIndex);
		// Una riga di skill deve vedere il cursore, per l'hover; le altre lasciano passare i gesti al pannello, che li consuma.
		LineWidgets[I]->SetVisibility(AbilityIndex >= 0 ? ESlateVisibility::Visible : ESlateVisibility::SelfHitTestInvisible);
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
