// I widget della barra dei comandi collegati PER NOME (#3489): lo slot, la lettura del movimento, Conferma/Annulla.
//
// 🔑 **I widget si iniettano a mano, come farebbe il `WidgetTree`.** I membri `BindWidgetOptional` sono puntatori
// pubblici: in una run headless non c'e' un Blueprint che li riempia, e assegnarli e' esattamente cio' che
// `UUserWidget::Initialize` fa per nome. Cio' che si misura qui e' la meta' C++ — quale widget si accende, quale
// testo si scrive, dove arriva un click. Che il `.uasset` dichiari quei nomi lo misurera' il gate sull'asset,
// nello stesso commit delle sedute `U61`, `U63` e `U64`.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTMovementProfileLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Map/RTCellId.h"
#include "Player/RTPlayerController.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTScreenHudWidgets.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	ARTUnit* SpawnNamedPortsUnit(UWorld* World)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = 0;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeAevik());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(FRTCellId(0, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Uno slot coi widget che il Designer dichiarera', assegnati per nome come farebbe il `WidgetTree`. */
	URTActionSlotWidget* MakeNamedPortsSlot()
	{
		URTActionSlotWidget* S = NewObject<URTActionSlotWidget>();
		S->PhaseStrip = NewObject<UBorder>(S);
		S->PhaseLabelText = NewObject<UTextBlock>(S);
		S->StateFrame = NewObject<UBorder>(S);
		S->SelectedBar = NewObject<UBorder>(S);
		S->PlannedCorner = NewObject<UBorder>(S);
		S->UnavailableHatch = NewObject<UBorder>(S);
		S->InvalidMark = NewObject<UBorder>(S);
		S->WarningMark = NewObject<UBorder>(S);
		return S;
	}

	bool NamedPortsIsShown(const UWidget* W)
	{
		return W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden;
	}

	/** Una vista che produce lo stato chiesto, e la premessa che lo produca davvero. */
	FRTAbilityCooldownView NamedPortsViewFor(ERTActionSlotState State, bool& bOutArmed)
	{
		FRTAbilityCooldownView V;
		V.ActionId = TEXT("Action.Guard");
		V.bUsableNow = true;
		bOutArmed = false;
		switch (State)
		{
		case ERTActionSlotState::Empty:       V.ActionId = NAME_None; break;
		case ERTActionSlotState::Available:   break;
		case ERTActionSlotState::Selected:    bOutArmed = true; break;
		case ERTActionSlotState::Planned:     V.bPlanned = true; break;
		case ERTActionSlotState::Cooldown:    V.TurnsRemaining = 2; V.bUsableNow = false; break;
		case ERTActionSlotState::Unavailable: V.bUsableNow = false; break;
		case ERTActionSlotState::Invalid:     V.bPlanInvalid = true; break;
		case ERTActionSlotState::Warning:     V.bPlanned = true; V.bPlanDegraded = true; break;
		}
		return V;
	}
}

/**
 * 🔴 **OGNI STATO ACCENDE SOLO IL PROPRIO INDICATORE** (`#3489`, `progettazione-hud.md` §47-bis.1).
 *
 * 🔑 **E' la forma strutturale del criterio in scala di grigi**: due stati che accendessero lo stesso widget
 * sarebbero indistinguibili senza colore. L'oracolo e' `IndicatorNameFor`, la tabella che il gate sull'asset
 * leggera'; lo stato lo decide `ResolveSlotState`, e il test ne fa una premessa invece di darlo per scontato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNamedPortsSlotIndicatorTest,
	"RefactorTactics.ScreenHud.SlotLightsOnlyTheIndicatorOfItsState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNamedPortsSlotIndicatorTest::RunTest(const FString&)
{
	URTActionSlotWidget* S = MakeNamedPortsSlot();
	const TPair<FName, UWidget*> Indicatori[] = {
		{ TEXT("SelectedBar"), S->SelectedBar },
		{ TEXT("PlannedCorner"), S->PlannedCorner },
		{ TEXT("UnavailableHatch"), S->UnavailableHatch },
		{ TEXT("InvalidMark"), S->InvalidMark },
		{ TEXT("WarningMark"), S->WarningMark },
	};

	const UEnum* Enum = StaticEnum<ERTActionSlotState>();
	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i) // l'ultimo e' `_MAX`
	{
		const ERTActionSlotState Stato = static_cast<ERTActionSlotState>(Enum->GetValueByIndex(i));
		const FString Nome = Enum->GetNameStringByIndex(i);
		bool bArmata = false;
		const FRTAbilityCooldownView V = NamedPortsViewFor(Stato, bArmata);
		if (!TestEqual(*FString::Printf(TEXT("premessa: la vista produce %s"), *Nome),
			URTHudViewModel::ResolveSlotState(V, bArmata), Stato))
		{
			continue;
		}

		S->SetAction(V, bArmata);
		const FName Atteso = URTActionSlotWidget::IndicatorNameFor(Stato);
		for (const TPair<FName, UWidget*>& Voce : Indicatori)
		{
			TestEqual(*FString::Printf(TEXT("%s: %s acceso se e solo se e' il suo indicatore"),
				*Nome, *Voce.Key.ToString()), NamedPortsIsShown(Voce.Value), Voce.Key == Atteso);
		}
		if (const FLinearColor* Colore = S->FrameColors.Find(Stato))
		{
			TestTrue(*FString::Printf(TEXT("%s: il bordo prende il colore del proprio stato"), *Nome),
				S->StateFrame->GetBrushColor().Equals(*Colore));
		}
		else
		{
			AddError(FString::Printf(TEXT("%s non ha un colore di bordo in FrameColors"), *Nome));
		}
	}
	return true;
}

/**
 * 🔴 **OGNI STATO DIVERSO DA `Empty` E `Available` HA UN SECONDO CANALE, E NON LO CONDIVIDE.**
 *
 * ⚠️ **Il presidio per il futuro**: un valore nuovo di `ERTActionSlotState` senza un indicatore farebbe rosso
 * questo test invece di comparire a schermo con il solo colore. E ogni nome della tabella, tranne
 * `CooldownText` che vive nel Blueprint da prima, e' un membro `BindWidgetOptional` dello slot: il nome nella
 * tabella e il nome del membro non possono divergere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNamedPortsEveryStateHasAChannelTest,
	"RefactorTactics.ScreenHud.EverySlotStateHasItsOwnSecondChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNamedPortsEveryStateHasAChannelTest::RunTest(const FString&)
{
	const UEnum* Enum = StaticEnum<ERTActionSlotState>();
	TSet<FName> Visti;
	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
	{
		const ERTActionSlotState Stato = static_cast<ERTActionSlotState>(Enum->GetValueByIndex(i));
		const FString Nome = Enum->GetNameStringByIndex(i);
		const FName Indicatore = URTActionSlotWidget::IndicatorNameFor(Stato);

		if (Stato == ERTActionSlotState::Empty || Stato == ERTActionSlotState::Available)
		{
			TestTrue(*FString::Printf(TEXT("%s non ha un secondo canale"), *Nome), Indicatore.IsNone());
			continue;
		}
		if (!TestFalse(*FString::Printf(TEXT("%s ha un secondo canale"), *Nome), Indicatore.IsNone()))
		{
			continue;
		}
		TestFalse(*FString::Printf(TEXT("%s non condivide il canale con un altro stato"), *Nome),
			Visti.Contains(Indicatore));
		Visti.Add(Indicatore);

		if (Indicatore != FName(TEXT("CooldownText")))
		{
			const FProperty* Membro = URTActionSlotWidget::StaticClass()->FindPropertyByName(Indicatore);
			TestTrue(*FString::Printf(TEXT("%s: '%s' e' un membro BindWidgetOptional dello slot"), *Nome,
				*Indicatore.ToString()), Membro && Membro->HasMetaData(TEXT("BindWidgetOptional")));
		}
	}
	return true;
}

/**
 * 🔴 **LA STRISCIA DI FASE LEGGE `PhaseMark`, E L'ETICHETTA C'E' SEMPRE CHE CI SIA UNA FASE** ([D-232], [D-233]).
 *
 * 🔑 `Cleanup` e' il caso che rende il test non ovvio: ha un'etichetta e un neutro, non una tinta di fase.
 * `None` non ha voce, e la striscia si chiude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNamedPortsPhaseStripTest,
	"RefactorTactics.ScreenHud.SlotPhaseStripReadsThePhaseMark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNamedPortsPhaseStripTest::RunTest(const FString&)
{
	URTActionSlotWidget* S = MakeNamedPortsSlot();
	const UEnum* Enum = StaticEnum<ERTActionPhaseMark>();
	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
	{
		const ERTActionPhaseMark Segno = static_cast<ERTActionPhaseMark>(Enum->GetValueByIndex(i));
		const FString Nome = Enum->GetNameStringByIndex(i);

		FRTAbilityCooldownView V;
		V.ActionId = TEXT("Action.Guard");
		V.bUsableNow = true;
		V.PhaseMark = Segno;
		V.PhaseLabel = Segno == ERTActionPhaseMark::None ? FText::GetEmpty() : FText::FromString(Nome.ToUpper());
		S->SetAction(V, /*bInArmed=*/ false);

		const FLinearColor* Colore = S->PhaseColors.Find(Segno);
		if (Segno == ERTActionPhaseMark::None)
		{
			TestNull(TEXT("None: nessun colore di fase"), Colore);
			TestFalse(TEXT("None: la striscia si chiude"), NamedPortsIsShown(S->PhaseStrip));
			TestFalse(TEXT("None: e l'etichetta vuota pure"), NamedPortsIsShown(S->PhaseLabelText));
			continue;
		}
		if (!TestNotNull(*FString::Printf(TEXT("%s ha un colore"), *Nome), Colore))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s: la striscia e' accesa"), *Nome), NamedPortsIsShown(S->PhaseStrip));
		TestTrue(*FString::Printf(TEXT("%s: col colore della propria fase"), *Nome),
			S->PhaseStrip->GetBrushColor().Equals(*Colore));
		TestTrue(*FString::Printf(TEXT("%s: l'etichetta e' accesa"), *Nome), NamedPortsIsShown(S->PhaseLabelText));
		TestEqual(*FString::Printf(TEXT("%s: e dice la fase"), *Nome),
			S->PhaseLabelText->GetText().ToString(), V.PhaseLabel.ToString());
	}

	// ⚠️ `Cleanup` porta il neutro, non una tinta che lo confonda con una fase.
	const FLinearColor* Pulizia = S->PhaseColors.Find(ERTActionPhaseMark::Cleanup);
	const FLinearColor* Prep = S->PhaseColors.Find(ERTActionPhaseMark::Prep);
	if (Pulizia && Prep)
	{
		TestFalse(TEXT("Cleanup non ha la tinta di Prep"), Pulizia->Equals(*Prep));
	}
	return true;
}

/**
 * 🔴 **IL SEPARATORE DEI GRUPPI E' PADDING, E CADE DOVE `OrderForReading` DICE** ([D-455] punto 2).
 *
 * 🔑 **L'oracolo e' il rango dei gruppi, non `Group`**: una posizione vuota (`None`) si legge col Kit, e fra le
 * due non c'e' un confine. La lista di partenza e' quella in cui i gruppi NON sono contigui — `B K C C N K` — e
 * lungo quella nessuna voce porta il flag.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNamedPortsGroupBreakTest,
	"RefactorTactics.HudViewModel.ReadingOrderMarksTheGroupBreaks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNamedPortsGroupBreakTest::RunTest(const FString&)
{
	const ERTActionGroup Kit[] = { ERTActionGroup::Base, ERTActionGroup::Kit, ERTActionGroup::Common,
		ERTActionGroup::Common, ERTActionGroup::None, ERTActionGroup::Kit };
	TArray<FRTAbilityCooldownView> Voci;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Kit); ++i)
	{
		FRTAbilityCooldownView V;
		V.AbilityIndex = i;
		V.Group = Kit[i];
		Voci.Add(V);
	}

	const TArray<FRTAbilityCooldownView> Lettura = URTHudViewModel::OrderForReading(Voci);
	// Atteso: C C | B | K N K — confini prima della posizione 2 e della 3, e nessuno fra K e N.
	const bool Atteso[] = { false, false, true, true, false, false };
	if (TestEqual(TEXT("premessa: la lettura ha tutte le voci"), Lettura.Num(), (int32)UE_ARRAY_COUNT(Atteso)))
	{
		for (int32 i = 0; i < Lettura.Num(); ++i)
		{
			TestEqual(*FString::Printf(TEXT("posizione %d (indice di kit %d)"), i, Lettura[i].AbilityIndex),
				Lettura[i].bGroupBreakBefore, Atteso[i]);
		}
	}
	for (const FRTAbilityCooldownView& V : Voci)
	{
		TestFalse(TEXT("lungo l'ordine di kit nessuna voce porta il confine"), V.bGroupBreakBefore);
	}

	// Lo slot lo traduce nel padding del proprio posto nella fila.
	UHorizontalBox* Fila = NewObject<UHorizontalBox>();
	URTActionSlotWidget* S = MakeNamedPortsSlot();
	UHorizontalBoxSlot* Posto = Fila->AddChildToHorizontalBox(S);
	if (TestNotNull(TEXT("premessa: lo slot ha un posto nella fila"), Posto))
	{
		FRTAbilityCooldownView V = Lettura[2];
		V.ActionId = TEXT("Action.Guard");
		S->SetAction(V, false);
		TestEqual(TEXT("un confine apre col separatore"), Posto->GetPadding().Left, S->GroupGap);
		V.bGroupBreakBefore = false;
		S->SetAction(V, false);
		TestEqual(TEXT("dentro un gruppo resta il gap"), Posto->GetPadding().Left, S->ItemGap);
	}
	return true;
}

/**
 * 🔴 **LA LETTURA DEL MOVIMENTO SEGUE LA VISTA, E IL BADGE DICHIARA** ([D-457], `#3489`).
 *
 * ⚠️ **Il controllo C e' la ragione per cui `BindNamedButtons` e' idempotente**: `NativeConstruct` puo'
 * girare piu' volte nella vita di un widget, e un click legato due volte dichiarerebbe e ritirerebbe nello
 * stesso gesto — a schermo, un badge che non reagisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNamedPortsMovementReadoutTest,
	"RefactorTactics.ScreenHud.MovementReadoutFollowsTheViewAndTheBadgeDeclares",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNamedPortsMovementReadoutTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = SpawnNamedPortsUnit(World);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!Unit || !PC || !Dock)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita', controller e dock esistono"), false);
	}
	Dock->MovementReadout = NewObject<UHorizontalBox>(Dock);
	Dock->MovementReadoutText = NewObject<UTextBlock>(Dock);
	Dock->SneakBadge = NewObject<UButton>(Dock);
	Dock->SneakBadgeText = NewObject<UTextBlock>(Dock);
	PC->SelectActorForTest(Unit);
	Dock->SetCommandControllerForTest(PC);
	Dock->SetSelectedUnitForTest(Unit);

	// --- A. la vista arriva ai widget ---------------------------------------------------------------------
	Dock->RefreshMovementReadout();
	const FRTMovementReadoutView Vista = Dock->GetMovementReadout();
	if (!TestTrue(TEXT("premessa: la vista dell'unita' comandata e' autorizzata"), Vista.bAuthorized))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	TestTrue(TEXT("A: la lettura e' visibile"), NamedPortsIsShown(Dock->MovementReadout));
	TestEqual(TEXT("A: il testo e' l'etichetta della vista"),
		Dock->MovementReadoutText->GetText().ToString(), Vista.Label.ToString());
	TestEqual(TEXT("A: il badge porta il tasto della vista"),
		Dock->SneakBadgeText->GetText().ToString(), Vista.SneakKeyLabel.ToString());
	TestEqual(TEXT("A: Sneak non dichiarato -> badge spento"),
		Dock->SneakBadge->GetRenderOpacity(), Dock->SneakBadgeIdleOpacity);

	// --- B. il click del badge dichiara Sneak -------------------------------------------------------------
	Dock->BindNamedButtons();
	Dock->SneakBadge->OnClicked.Broadcast();
	TestEqual(TEXT("B: il click dichiara Sneak"), Unit->PlannedMovementProfileId,
		URTMovementProfileLibrary::ProfileSneak);
	Dock->RefreshMovementReadout();
	TestEqual(TEXT("B: e il badge si accende"), Dock->SneakBadge->GetRenderOpacity(), 1.f);

	// --- C. collegare due volte non raddoppia il click -----------------------------------------------------
	Dock->BindNamedButtons();
	Dock->SneakBadge->OnClicked.Broadcast();
	TestTrue(TEXT("C: un click, un gesto — Sneak ritirato, non dichiarato e ritirato"),
		Unit->PlannedMovementProfileId.IsNone());

	// --- D. senza un'unita' comandata la lettura si CHIUDE, non si spegne --------------------------------
	PC->SelectActorForTest(nullptr);
	Dock->SetSelectedUnitForTest(nullptr);
	Dock->RefreshMovementReadout();
	TestFalse(TEXT("D: senza unita' la lettura e' chiusa"), NamedPortsIsShown(Dock->MovementReadout));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **CONFERMA E ANNULLA: IL CLICK ARRIVA ALLE PORTE, IL TESTO PORTA IL TASTO** ([D-458], `#3489`).
 *
 * ⚠️ **Il controllo D e' la correzione del foglio di `U64`**, che chiedeva di spegnere entrambi i pulsanti
 * senza un'unita' comandata: `UndoStep` ritira il Ready senza chiedere un'unita', e un `Annulla` spento lo
 * renderebbe irraggiungibile dal pulsante.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNamedPortsPlanCommitTest,
	"RefactorTactics.ScreenHud.PlanCommitButtonsFollowThePorts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNamedPortsPlanCommitTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = SpawnNamedPortsUnit(World);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTPlanCommitWidget* W = NewObject<URTPlanCommitWidget>(World);
	if (!Unit || !PC || !W)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita', controller e widget esistono"), false);
	}
	W->ConfirmButton = NewObject<UButton>(W);
	W->ConfirmText = NewObject<UTextBlock>(W);
	W->UndoButton = NewObject<UButton>(W);
	W->UndoText = NewObject<UTextBlock>(W);
	// ⚠️ Due selezioni, come in gioco: il controller per le porte, il widget per cio' che mostra.
	PC->SelectActorForTest(Unit);
	W->SetCommandControllerForTest(PC);
	W->SetSelectedUnitForTest(Unit);
	W->BindNamedButtons();

	// --- A. testi e abilitazione con un'unita' comandata --------------------------------------------------
	W->RefreshButtons();
	const FString Tasto = W->GetConfirmKeyLabel().ToString();
	TestTrue(TEXT("A: Conferma e' accesa"), W->ConfirmButton->GetIsEnabled());
	TestTrue(TEXT("A: il testo dice Conferma"), W->ConfirmText->GetText().ToString().Contains(TEXT("Conferma")));
	TestTrue(TEXT("A: e porta il tasto"), !Tasto.IsEmpty() && W->ConfirmText->GetText().ToString().Contains(Tasto));
	TestTrue(TEXT("A: Annulla porta il proprio tasto"),
		W->UndoText->GetText().ToString().Contains(W->GetUndoKeyLabel().ToString()));

	// --- B. il click di Conferma dichiara, e il testo diventa Ritira -------------------------------------
	W->ConfirmButton->OnClicked.Broadcast();
	TestTrue(TEXT("B: il click dichiara il piano"), Unit->bTurnPlanDeclared);
	W->RefreshButtons();
	TestTrue(TEXT("B: il testo dice Ritira"), W->ConfirmText->GetText().ToString().Contains(TEXT("Ritira")));

	// --- C. collegare due volte non raddoppia il click -----------------------------------------------------
	W->BindNamedButtons();
	W->ConfirmButton->OnClicked.Broadcast();
	TestFalse(TEXT("C: un click, un gesto — il piano e' ritratto"), Unit->bTurnPlanDeclared);

	// --- D. senza unita': Conferma si spegne, Annulla NO --------------------------------------------------
	PC->SelectActorForTest(nullptr);
	W->SetSelectedUnitForTest(nullptr);
	W->RefreshButtons();
	TestFalse(TEXT("D: senza unita' Conferma e' spenta"), W->ConfirmButton->GetIsEnabled());
	TestTrue(TEXT("D: Annulla resta accesa: il Back non chiede un'unita'"), W->UndoButton->GetIsEnabled());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
