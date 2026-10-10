// I widget della barra dei comandi collegati PER NOME (#3489): lo slot, la lettura del movimento, Conferma/Annulla.
//
// 🔑 **I widget si iniettano a mano, come farebbe il `WidgetTree`.** I membri `BindWidgetOptional` sono puntatori
// pubblici: in una run headless non c'e' un Blueprint che li riempia, e assegnarli e' esattamente cio' che
// `UUserWidget::Initialize` fa per nome. Cio' che si misura qui e' la meta' C++ — quale widget si accende, quale
// testo si scrive, dove arriva un click. Che il `.uasset` dichiari quei nomi lo misurera' il gate sull'asset,
// nello stesso commit delle sedute `U61`, `U63` e `U64`.

#include "Misc/AutomationTest.h"
#include "UI/RTUIPalette.h" // la resa di Conferma si confronta coi token (#3633)
#include "Turn/RTTurnManager.h"
#include "Components/Image.h"
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

	// I default del bordo contro i token di §32, LETTERALI: leggere la tabella che il codice applica sarebbe
	// confrontare il codice con se stesso.
	const TPair<ERTActionSlotState, const TCHAR*> Token[] = {
		{ ERTActionSlotState::Available, TEXT("4A5568") }, { ERTActionSlotState::Selected, TEXT("FFD456") },
		{ ERTActionSlotState::Planned, TEXT("FFD456") }, { ERTActionSlotState::Invalid, TEXT("FF4D4D") },
		{ ERTActionSlotState::Warning, TEXT("FFD456") } };
	for (const TPair<ERTActionSlotState, const TCHAR*>& T : Token)
	{
		const FLinearColor* Colore = S->FrameColors.Find(T.Key);
		TestTrue(*FString::Printf(TEXT("il bordo di %s e' #%s"),
			*StaticEnum<ERTActionSlotState>()->GetNameStringByValue((int64)T.Key), T.Value),
			Colore && Colore->Equals(FLinearColor::FromSRGBColor(FColor::FromHex(T.Value))));
	}

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
			TestNotNull(*FString::Printf(TEXT("%s: '%s' e' un membro dello slot"), *Nome,
				*Indicatore.ToString()), Membro);
			// ⚠️ `HasMetaData` esiste solo con i metadati: in Game Development i test sono accesi e i metadati
			// no, e senza questa guardia `main` non compila in quella configurazione (`RTScenarioAuthoringTests`).
#if WITH_METADATA
			TestTrue(*FString::Printf(TEXT("%s: '%s' e' BindWidgetOptional"), *Nome, *Indicatore.ToString()),
				Membro && Membro->HasMetaData(TEXT("BindWidgetOptional")));
#endif
		}
	}
	return true;
}

/**
 * 🔴 **LA STRISCIA DI FASE LEGGE `PhaseMark`, CON I COLORI DI [D-233]; L'ETICHETTA C'E' SEMPRE** ([D-232]).
 *
 * 🔑 **Gli HEX sono LETTERALI, e vengono dalla decisione, non dalla tabella che il codice applica**: e' il gate
 * che rende la palette in C++ una copia controllata. `Cleanup` e `None` sono i casi che rendono il test non
 * ovvio: nessuna striscia, ma un'etichetta — `CLEANUP` e `—` — come la scrive `PhaseMarkLabel`. Solo una
 * posizione di kit vuota non ha etichetta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNamedPortsPhaseStripTest,
	"RefactorTactics.ScreenHud.SlotPhaseStripReadsThePhaseMark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNamedPortsPhaseStripTest::RunTest(const FString&)
{
	URTActionSlotWidget* S = MakeNamedPortsSlot();

	// [D-233] per le macro-fasi, §32 (`RT_UI_Violet`) per la reazione. `nullptr` = nessuna striscia.
	const TPair<ERTActionPhaseMark, const TCHAR*> Palette[] = {
		{ ERTActionPhaseMark::None, nullptr }, { ERTActionPhaseMark::Prep, TEXT("56B4E9") },
		{ ERTActionPhaseMark::Dash, TEXT("009E73") }, { ERTActionPhaseMark::Blast, TEXT("D55E00") },
		{ ERTActionPhaseMark::Move, TEXT("0072B2") }, { ERTActionPhaseMark::Cleanup, nullptr },
		{ ERTActionPhaseMark::Reaction, TEXT("7C5CFF") } };
	const UEnum* Enum = StaticEnum<ERTActionPhaseMark>();
	TestEqual(TEXT("premessa: la palette del test copre ogni segno"),
		(int32)UE_ARRAY_COUNT(Palette), Enum->NumEnums() - 1);

	for (const TPair<ERTActionPhaseMark, const TCHAR*>& Voce : Palette)
	{
		const FString Nome = Enum->GetNameStringByValue((int64)Voce.Key);
		FRTAbilityCooldownView V;
		V.ActionId = TEXT("Action.Guard");
		V.bUsableNow = true;
		V.PhaseMark = Voce.Key;
		V.PhaseLabel = URTHudViewModel::PhaseMarkLabel(Voce.Key); // l'etichetta che il gioco scrive
		S->SetAction(V, /*bInArmed=*/ false);

		if (!TestFalse(*FString::Printf(TEXT("premessa: %s ha un'etichetta"), *Nome), V.PhaseLabel.IsEmpty()))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s: l'etichetta e' accesa"), *Nome), NamedPortsIsShown(S->PhaseLabelText));
		TestEqual(*FString::Printf(TEXT("%s: e dice la fase"), *Nome),
			S->PhaseLabelText->GetText().ToString(), V.PhaseLabel.ToString());

		if (Voce.Value == nullptr)
		{
			TestNull(*FString::Printf(TEXT("%s: nessun colore di fase"), *Nome), S->PhaseColors.Find(Voce.Key));
			TestFalse(*FString::Printf(TEXT("%s: la striscia si chiude"), *Nome), NamedPortsIsShown(S->PhaseStrip));
			continue;
		}
		const FLinearColor Atteso = FLinearColor::FromSRGBColor(FColor::FromHex(Voce.Value));
		TestTrue(*FString::Printf(TEXT("%s: la striscia e' accesa"), *Nome), NamedPortsIsShown(S->PhaseStrip));
		TestTrue(*FString::Printf(TEXT("%s: col colore #%s"), *Nome, Voce.Value),
			S->PhaseStrip->GetBrushColor().Equals(Atteso));
	}

	// Una posizione di kit vuota non ha etichetta, e lo slot non ne mostra una.
	FRTAbilityCooldownView Vuota;
	S->SetAction(Vuota, false);
	TestFalse(TEXT("posizione vuota: nessuna etichetta"), NamedPortsIsShown(S->PhaseLabelText));
	TestFalse(TEXT("posizione vuota: nessuna striscia"), NamedPortsIsShown(S->PhaseStrip));
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
	// Lungo l'ordine di kit VERO — quello che `BuildAbilityCooldowns` consegna — nessuna voce porta il confine.
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		if (ARTUnit* Unit = SpawnNamedPortsUnit(World))
		{
			const TArray<FRTAbilityCooldownView> OrdineDiKit = URTHudViewModel::BuildAbilityCooldowns(Unit);
			TestTrue(TEXT("premessa: il kit ha voci"), OrdineDiKit.Num() > 1);
			for (const FRTAbilityCooldownView& V : OrdineDiKit)
			{
				TestFalse(*FString::Printf(TEXT("ordine di kit, voce %d: nessun confine"), V.AbilityIndex),
					V.bGroupBreakBefore);
			}
		}
		RTWorldFixtures::DestroyWorld(World);
	}

	// Lo slot lo traduce nel padding del proprio posto nella fila — solo il lato sinistro.
	UHorizontalBox* Fila = NewObject<UHorizontalBox>();
	URTActionSlotWidget* S = MakeNamedPortsSlot();
	UHorizontalBoxSlot* Posto = Fila->AddChildToHorizontalBox(S);
	if (TestNotNull(TEXT("premessa: lo slot ha un posto nella fila"), Posto)
		&& TestTrue(TEXT("premessa: la lettura ha la voce che apre la Base"), Lettura.IsValidIndex(2)))
	{
		Posto->SetPadding(FMargin(0.f, 3.f, 5.f, 7.f)); // gli altri tre lati sono del Designer
		FRTAbilityCooldownView V = Lettura[2];
		V.ActionId = TEXT("Action.Guard");
		S->SetAction(V, false);
		TestEqual(TEXT("un confine apre col separatore"), Posto->GetPadding().Left, S->GroupGap);
		TestTrue(TEXT("e gli altri lati restano del Designer"),
			Posto->GetPadding().Top == 3.f && Posto->GetPadding().Right == 5.f && Posto->GetPadding().Bottom == 7.f);
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
	if (!TestFalse(TEXT("premessa: la vista ha un'etichetta"), Vista.Label.IsEmpty())
		|| !TestFalse(TEXT("premessa: e un tasto per il badge"), Vista.SneakKeyLabel.IsEmpty()))
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
	const FString TastoAnnulla = W->GetUndoKeyLabel().ToString();
	TestTrue(TEXT("A: Annulla porta il proprio tasto"),
		!TastoAnnulla.IsEmpty() && W->UndoText->GetText().ToString().Contains(TastoAnnulla));

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

/**
 * 🔑 **CONFERMA COME LA TAVOLA** ([D-496], #3633): il tasto nel suo badge e in italiano, i colori da palette, il
 * contatore degli avvisi con un numero per livello ([D-494]).
 *
 * ⚠️ Le cornici sono `RoundedBox`, come nella tavola: e' li' che fondo e contorno sono due colori. Con un `Border`
 * a texture il colore e' uno solo, e quel ramo lo copre lo slot (#3498).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNamedPortsPlanCommitLookTest,
	"RefactorTactics.ScreenHud.PlanCommitLookFollowsTheBoard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNamedPortsPlanCommitLookTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = SpawnNamedPortsUnit(World);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	URTPlanCommitWidget* W = NewObject<URTPlanCommitWidget>(World);
	if (!Unit || !PC || !TM || !W)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita', controller, turn manager e widget esistono"), false);
	}
	const auto Cornice = [W]()
	{
		UBorder* B = NewObject<UBorder>(W);
		FSlateBrush Brush = B->Background;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		B->SetBrush(Brush);
		return B;
	};
	W->ConfirmButton = NewObject<UButton>(W);
	W->ConfirmText = NewObject<UTextBlock>(W);
	W->UndoButton = NewObject<UButton>(W);
	W->UndoText = NewObject<UTextBlock>(W);
	W->ConfirmKeyText = NewObject<UTextBlock>(W);
	W->UndoKeyText = NewObject<UTextBlock>(W);
	W->ConfirmFrame = Cornice();
	W->UndoFrame = Cornice();
	W->ConfirmIcon = NewObject<UTextBlock>(W);
	W->CommitRoot = NewObject<UBorder>(W);
	W->WarningCounter = NewObject<UBorder>(W);
	W->CriticalCountText = NewObject<UTextBlock>(W);
	W->CriticalBadge = NewObject<UBorder>(W);
	W->WarningCountText = NewObject<UTextBlock>(W);
	W->WarningBadge = NewObject<UBorder>(W);
	W->InfoCountText = NewObject<UTextBlock>(W);
	W->InfoBadge = NewObject<UBorder>(W);
	W->SetMatchContextForTest(TM, Unit->TeamId);
	W->SetControlGroupForTest(Unit->ControlGroup);
	PC->SelectActorForTest(Unit);
	W->SetCommandControllerForTest(PC);
	W->SetSelectedUnitForTest(Unit);

	const auto Colore = [](ERTUIToken Token) { return URTUIPalette::ColorFor(Token); };
	const auto Contorno = [](const UBorder* B) { return B->Background.OutlineSettings.Color.GetSpecifiedColor(); };

	// --- A. un'unita' comandata con un piano pulito ------------------------------------------------------------
	W->RefreshButtons();
	TestEqual(TEXT("A: col badge, il testo di Conferma e' il solo verbo"), W->ConfirmText->GetText().ToString(), FString(TEXT("Conferma")));
	TestEqual(TEXT("A: il badge di Conferma dice INVIO (D-496)"), W->ConfirmKeyText->GetText().ToString(), FString(TEXT("INVIO")));
	TestEqual(TEXT("A: il testo di Annulla e' il solo verbo"), W->UndoText->GetText().ToString(), FString(TEXT("Annulla")));
	TestEqual(TEXT("A: il badge di Annulla dice BACKSPACE"), W->UndoKeyText->GetText().ToString(), FString(TEXT("BACKSPACE")));
	TestTrue(TEXT("A: Conferma accesa ha il fondo BG_ProfileActive"), W->ConfirmFrame->GetBrushColor().Equals(Colore(ERTUIToken::BG_ProfileActive)));
	TestTrue(TEXT("A: e il contorno Cyan"), Contorno(W->ConfirmFrame).Equals(Colore(ERTUIToken::Cyan)));
	TestTrue(TEXT("A: Annulla ha il contorno Frame_Mid"), Contorno(W->UndoFrame).Equals(Colore(ERTUIToken::Frame_Mid)));
	TestTrue(TEXT("A: la spunta e' Cyan"), W->ConfirmIcon->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Cyan)));
	TestEqual(TEXT("A: il contorno di Conferma accesa e' spesso 1, come nel sorgente della tavola"), W->ConfirmFrame->Background.OutlineSettings.Width, 1.f);
	TestTrue(TEXT("A: il verbo di Conferma e' White"), W->ConfirmText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::White)));
	TestTrue(TEXT("A: il badge del tasto e' Text_Primary"), W->ConfirmKeyText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Text_Primary)));
	TestTrue(TEXT("A: fuori dalla Risoluzione il riquadro c'e'"), NamedPortsIsShown(W->CommitRoot));
	TestFalse(TEXT("A: senza avvisi il contatore e' chiuso"), NamedPortsIsShown(W->WarningCounter));

	// --- B. un piano illegale: un Critical nel contatore, e solo il suo badge --------------------------------------
	int32 ConRicarica = INDEX_NONE;
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* A = Unit->GetAbility(i);
		if (A && A->CooldownTurns > 0 && !A->Def.ActionId.IsNone())
		{
			ConRicarica = i;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: il kit ha un'azione con ricarica"), ConRicarica != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	Unit->PlannedAbilityIndex = ConRicarica;
	Unit->ConsumeAbility(ConRicarica);
	W->RefreshButtons();
	TestTrue(TEXT("B: con un avviso il contatore si apre"), NamedPortsIsShown(W->WarningCounter));
	TestTrue(TEXT("B: il badge del Critical si accende"), NamedPortsIsShown(W->CriticalBadge));
	TestEqual(TEXT("B: e dice 1"), W->CriticalCountText->GetText().ToString(), FString(TEXT("1")));
	TestTrue(TEXT("B: il numero del Critical e' Red"), W->CriticalCountText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Red)));
	TestFalse(TEXT("B: il badge del Warning resta chiuso"), NamedPortsIsShown(W->WarningBadge));
	TestFalse(TEXT("B: e quello dell'Info"), NamedPortsIsShown(W->InfoBadge));

	// --- C. senza un'unita' comandata Conferma si spegne anche nella resa -----------------------------------------
	PC->SelectActorForTest(nullptr);
	W->SetSelectedUnitForTest(nullptr);
	W->RefreshButtons();
	TestTrue(TEXT("C: Conferma spenta ha il contorno Frame_Off"), Contorno(W->ConfirmFrame).Equals(Colore(ERTUIToken::Frame_Off)));
	TestTrue(TEXT("C: e il fondo BG_Panel"), W->ConfirmFrame->GetBrushColor().Equals(Colore(ERTUIToken::BG_Panel)));
	TestTrue(TEXT("C: la spunta e' Text_Disabled"), W->ConfirmIcon->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Text_Disabled)));
	TestEqual(TEXT("C: e il contorno resta spesso 1"), W->ConfirmFrame->Background.OutlineSettings.Width, 1.f);
	TestTrue(TEXT("C: il verbo di Conferma e' Text_Disabled"), W->ConfirmText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Text_Disabled)));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
