// La resa dello slot secondo il mockup (#3498): fondo e contorno per stato, tasto e nome separati, intestazione
// e divisore di gruppo, tinta dell'icona e del nome, striscia ed etichetta di fase, barra di Selected.
//
// 🔑 **I colori si confrontano con gli HEX LETTERALI del sorgente del mockup** (`sorgente-mockup/Stati.dc.html` per
// gli otto stati, `Main.dc.html` per la reazione e lo slot vuoto), non con le tabelle che il codice applica:
// leggere la tabella sarebbe confrontare il codice con se stesso.

#include "Misc/AutomationTest.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTScreenHudWidgets.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FLinearColor MockupLookHex(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
	}

	/** Uno slot con uno `StateFrame` disegnato come `RoundedBox`, come lo avra' il Blueprint dopo la seduta. */
	URTActionSlotWidget* MakeMockupLookSlot()
	{
		URTActionSlotWidget* S = NewObject<URTActionSlotWidget>();
		S->StateFrame = NewObject<UBorder>(S);
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings = FSlateBrushOutlineSettings(6.f);
		S->StateFrame->SetBrush(Brush);
		S->HotkeyBadge = NewObject<UBorder>(S);
		S->HotkeyText = NewObject<UTextBlock>(S);
		S->ActionNameText = NewObject<UTextBlock>(S);
		S->GroupHeaderText = NewObject<UTextBlock>(S);
		S->SelectedGlow = NewObject<UBorder>(S);
		S->GroupDivider = NewObject<UBorder>(S);
		S->PhaseStrip = NewObject<UBorder>(S);
		S->PhaseLabelText = NewObject<UTextBlock>(S);
		S->SelectedBar = NewObject<UBorder>(S);
		return S;
	}

	FRTAbilityCooldownView MockupLookView()
	{
		FRTAbilityCooldownView V;
		V.ActionId = TEXT("Action.Guard");
		V.DisplayName = FText::FromString(TEXT("Guardia"));
		V.HotkeyLabel = FText::FromString(TEXT("G"));
		V.PhaseMark = ERTActionPhaseMark::Blast;
		V.PhaseLabel = FText::FromString(TEXT("BLAST"));
		V.bUsableNow = true;
		V.Slot = ERTActionSlot::Main;
		return V;
	}

	/** La vista che produce `Stato` con `ResolveSlotState`, e se lo slot va armato per produrlo. */
	FRTAbilityCooldownView MockupLookViewFor(ERTActionSlotState Stato, bool& bOutArmed)
	{
		FRTAbilityCooldownView V = MockupLookView();
		bOutArmed = false;
		switch (Stato)
		{
		case ERTActionSlotState::Empty:       V.ActionId = NAME_None; break;
		case ERTActionSlotState::Available:   break;
		case ERTActionSlotState::Selected:    bOutArmed = true; break;
		case ERTActionSlotState::Planned:     V.bPlanned = true; break;
		case ERTActionSlotState::Cooldown:    V.TurnsRemaining = 2; V.bUsableNow = false; break;
		case ERTActionSlotState::Unavailable: V.bUsableNow = false; break;
		case ERTActionSlotState::Invalid:     V.bPlanInvalid = true; break;
		case ERTActionSlotState::Warning:     V.bPlanDegraded = true; break;
		}
		return V;
	}

	FLinearColor MockupLookTextColor(const UTextBlock* Text)
	{
		return Text->GetColorAndOpacity().GetSpecifiedColor();
	}
}

/**
 * 🔴 **OGNI STATO HA LA RICETTA DEL MOCKUP**: fondo, contorno, spessore, icona e nome (#3498).
 *
 * ⚠️ Lo spessore e' la ragione della tabella `FrameWidths`: nel mockup Selected ha il bordo doppio di Available,
 * ed e' parte di cio' che li distingue oltre al colore. Il fondo di Empty e' trasparente: si confronta l'alfa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMockupLookFrameTest,
	"RefactorTactics.ScreenHud.SlotFrameFollowsTheMockupRecipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMockupLookFrameTest::RunTest(const FString&)
{
	URTActionSlotWidget* S = MakeMockupLookSlot();
	UImage* Icona = NewObject<UImage>(S);

	struct FRicetta
	{
		ERTActionSlotState Stato; const TCHAR* Nome;
		const TCHAR* Fondo; const TCHAR* Contorno; float Spessore; const TCHAR* Glifo; const TCHAR* Testo;
	};
	// `nullptr` come fondo = trasparente.
	const FRicetta Ricette[] = {
		{ ERTActionSlotState::Empty,       TEXT("Empty"),       nullptr,        TEXT("4A5568"), 1.f, TEXT("A9B4C2"), TEXT("A9B4C2") },
		{ ERTActionSlotState::Available,   TEXT("Available"),   TEXT("212733"), TEXT("4A5568"), 1.f, TEXT("E6EBF2"), TEXT("E6EBF2") },
		{ ERTActionSlotState::Selected,    TEXT("Selected"),    TEXT("2B2918"), TEXT("FFD456"), 2.f, TEXT("FFD456"), TEXT("E6EBF2") },
		{ ERTActionSlotState::Planned,     TEXT("Planned"),     TEXT("212733"), TEXT("FFD456"), 2.f, TEXT("E6EBF2"), TEXT("E6EBF2") },
		{ ERTActionSlotState::Cooldown,    TEXT("Cooldown"),    TEXT("151A23"), TEXT("2E3746"), 1.f, TEXT("3A4454"), TEXT("6B7684") },
		{ ERTActionSlotState::Unavailable, TEXT("Unavailable"), TEXT("151A23"), TEXT("2E3746"), 1.f, TEXT("4A5568"), TEXT("6B7684") },
		{ ERTActionSlotState::Invalid,     TEXT("Invalid"),     TEXT("2A1719"), TEXT("FF4D4D"), 2.f, TEXT("E6EBF2"), TEXT("E6EBF2") },
		{ ERTActionSlotState::Warning,     TEXT("Warning"),     TEXT("212733"), TEXT("FFD456"), 2.f, TEXT("E6EBF2"), TEXT("E6EBF2") },
	};
	for (const FRicetta& R : Ricette)
	{
		bool bArmata = false;
		const FRTAbilityCooldownView V = MockupLookViewFor(R.Stato, bArmata);
		if (!TestEqual(*FString::Printf(TEXT("premessa: la vista produce %s"), R.Nome),
			URTHudViewModel::ResolveSlotState(V, bArmata), R.Stato))
		{
			continue;
		}
		S->SetAction(V, bArmata);
		S->ApplyResolvedIconTo(Icona);
		const FSlateBrush& Brush = S->StateFrame->Background;
		if (R.Fondo)
		{
			TestTrue(*FString::Printf(TEXT("%s: fondo #%s"), R.Nome, R.Fondo),
				S->StateFrame->GetBrushColor().Equals(MockupLookHex(R.Fondo)));
		}
		else
		{
			TestEqual(*FString::Printf(TEXT("%s: fondo trasparente"), R.Nome), S->StateFrame->GetBrushColor().A, 0.f);
		}
		TestTrue(*FString::Printf(TEXT("%s: contorno #%s"), R.Nome, R.Contorno),
			Brush.OutlineSettings.Color.GetSpecifiedColor().Equals(MockupLookHex(R.Contorno)));
		TestEqual(*FString::Printf(TEXT("%s: spessore"), R.Nome), Brush.OutlineSettings.Width, R.Spessore);
		TestEqual(*FString::Printf(TEXT("%s: resta un RoundedBox"), R.Nome), Brush.DrawAs, ESlateBrushDrawType::RoundedBox);
		TestTrue(*FString::Printf(TEXT("%s: icona #%s"), R.Nome, R.Glifo),
			Icona->GetColorAndOpacity().Equals(MockupLookHex(R.Glifo)));
		TestTrue(*FString::Printf(TEXT("%s: nome #%s"), R.Nome, R.Testo),
			MockupLookTextColor(S->ActionNameText).Equals(MockupLookHex(R.Testo)));
		TestEqual(*FString::Printf(TEXT("%s: l'alone solo da armata"), R.Nome), S->SelectedGlow->GetVisibility(),
			R.Stato == ERTActionSlotState::Selected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Una REAZIONE armata e' viola, non ambra: stesso stato, resa diversa (`Main.dc.html`, `REACT`).
	FRTAbilityCooldownView Reazione = MockupLookView();
	Reazione.Slot = ERTActionSlot::Reaction;
	S->SetAction(Reazione, /*bInArmed=*/ true);
	S->ApplyResolvedIconTo(Icona);
	TestTrue(TEXT("reazione armata: fondo #221E3A"), S->StateFrame->GetBrushColor().Equals(MockupLookHex(TEXT("221E3A"))));
	TestTrue(TEXT("reazione armata: contorno #7C5CFF"),
		S->StateFrame->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(MockupLookHex(TEXT("7C5CFF"))));
	TestTrue(TEXT("reazione armata: icona #B9A8FF"), Icona->GetColorAndOpacity().Equals(MockupLookHex(TEXT("B9A8FF"))));
	TestEqual(TEXT("reazione armata: nessun alone ambra"), S->SelectedGlow->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

/**
 * 🔴 **STRISCIA, ETICHETTA DI FASE E BARRA DI SELECTED SEGUONO LO STATO E IL TIPO DI SLOT** (#3498).
 *
 * `Stati.dc.html`: indisponibile ha la striscia grigia `#4A5568` al posto della fase, in ricarica la fase resta al
 * 30%. `Main.dc.html`: l'etichetta `REAZ.` e' `#B9A8FF`, le altre `#A9B4C2`; la barra di una reazione armata e'
 * viola, quella di un'azione armata ambra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMockupLookStripAndBarTest,
	"RefactorTactics.ScreenHud.SlotStripLabelAndBarFollowTheMockup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMockupLookStripAndBarTest::RunTest(const FString&)
{
	URTActionSlotWidget* S = MakeMockupLookSlot();
	const FLinearColor Blast = MockupLookHex(TEXT("D55E00"));

	S->SetAction(MockupLookView(), false);
	TestTrue(TEXT("pronta: la striscia ha il colore della fase, piena"), S->PhaseStrip->GetBrushColor().Equals(Blast));
	TestTrue(TEXT("pronta: etichetta #A9B4C2"),
		MockupLookTextColor(S->PhaseLabelText).Equals(MockupLookHex(TEXT("A9B4C2"))));

	bool bArmata = false;
	S->SetAction(MockupLookViewFor(ERTActionSlotState::Cooldown, bArmata), false);
	TestTrue(TEXT("in ricarica: la fase resta, al 30%"),
		S->PhaseStrip->GetBrushColor().Equals(Blast.CopyWithNewOpacity(0.3f)));

	S->SetAction(MockupLookViewFor(ERTActionSlotState::Unavailable, bArmata), false);
	TestTrue(TEXT("indisponibile: striscia grigia #4A5568"),
		S->PhaseStrip->GetBrushColor().Equals(MockupLookHex(TEXT("4A5568"))));

	S->SetAction(MockupLookView(), /*bInArmed=*/ true);
	TestTrue(TEXT("armata: barra ambra #FFD456"), S->SelectedBar->GetVisibility() != ESlateVisibility::Collapsed
		&& Cast<UBorder>(S->SelectedBar)->GetBrushColor().Equals(MockupLookHex(TEXT("FFD456"))));

	FRTAbilityCooldownView Reazione = MockupLookView();
	Reazione.Slot = ERTActionSlot::Reaction;
	Reazione.PhaseMark = ERTActionPhaseMark::Reaction;
	Reazione.PhaseLabel = FText::FromString(TEXT("REAZ."));
	S->SetAction(Reazione, false);
	TestTrue(TEXT("reazione: etichetta #B9A8FF anche non armata"),
		MockupLookTextColor(S->PhaseLabelText).Equals(MockupLookHex(TEXT("B9A8FF"))));
	S->SetAction(Reazione, /*bInArmed=*/ true);
	TestTrue(TEXT("reazione armata: barra viola #7C5CFF, non ambra"),
		Cast<UBorder>(S->SelectedBar)->GetBrushColor().Equals(MockupLookHex(TEXT("7C5CFF"))));
	return true;
}

/**
 * 🔴 **TASTO E NOME SONO DUE TESTI, E IL RIQUADRO DEL TASTO SPARISCE SENZA TASTO** (#3498).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMockupLookHotkeyTest,
	"RefactorTactics.ScreenHud.SlotShowsHotkeyAndNameApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMockupLookHotkeyTest::RunTest(const FString&)
{
	URTActionSlotWidget* S = MakeMockupLookSlot();
	S->SetAction(MockupLookView(), false);
	TestEqual(TEXT("il tasto nel suo riquadro"), S->HotkeyText->GetText().ToString(), FString(TEXT("G")));
	TestEqual(TEXT("il nome senza il tasto"), S->ActionNameText->GetText().ToString(), FString(TEXT("Guardia")));
	TestTrue(TEXT("il riquadro del tasto e' visibile"), S->HotkeyBadge->GetVisibility() != ESlateVisibility::Collapsed);

	FRTAbilityCooldownView SenzaTasto = MockupLookView();
	SenzaTasto.HotkeyLabel = FText::GetEmpty();
	S->SetAction(SenzaTasto, false);
	TestEqual(TEXT("senza tasto il riquadro si chiude"), S->HotkeyBadge->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

/**
 * 🔴 **INTESTAZIONE E DIVISORE STANNO SULLA PRIMA VOCE DEL GRUPPO** (#3498).
 *
 * ⚠️ L'intestazione e' `Hidden` e non `Collapsed` sulle altre voci: uno slot con l'intestazione e uno senza devono
 * restare alla stessa altezza, altrimenti la fila si scalina. Il divisore invece sta fuori dallo slot e non ne
 * cambia la misura: dove non serve si chiude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMockupLookGroupHeaderTest,
	"RefactorTactics.ScreenHud.SlotGroupHeaderOnTheFirstOfEachGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMockupLookGroupHeaderTest::RunTest(const FString&)
{
	// La lista B K C C N K: in lettura C C | B | K N K.
	const ERTActionGroup Gruppi[] = { ERTActionGroup::Base, ERTActionGroup::Kit, ERTActionGroup::Common,
		ERTActionGroup::Common, ERTActionGroup::None, ERTActionGroup::Kit };
	TArray<FRTAbilityCooldownView> Voci;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Gruppi); ++i)
	{
		FRTAbilityCooldownView V = MockupLookView();
		V.AbilityIndex = i;
		V.Group = Gruppi[i];
		Voci.Add(V);
	}
	const TArray<FRTAbilityCooldownView> Lettura = URTHudViewModel::OrderForReading(Voci);
	const bool Prime[] = { true, false, true, true, false, false };
	const bool Divisori[] = { false, false, true, true, false, false };
	const TCHAR* Testi[] = { TEXT("COMUNI"), TEXT("COMUNI"), TEXT("BASE"), TEXT("KIT"), TEXT("KIT"), TEXT("KIT") };
	if (!TestEqual(TEXT("premessa: la lettura ha tutte le voci"), Lettura.Num(), (int32)UE_ARRAY_COUNT(Prime)))
	{
		return false;
	}

	URTActionSlotWidget* S = MakeMockupLookSlot();
	for (int32 i = 0; i < Lettura.Num(); ++i)
	{
		TestEqual(*FString::Printf(TEXT("posizione %d: prima del gruppo"), i), Lettura[i].bFirstOfGroup, Prime[i]);
		S->SetAction(Lettura[i], false);
		TestEqual(*FString::Printf(TEXT("posizione %d: il testo del gruppo"), i),
			S->GroupHeaderText->GetText().ToString(), FString(Testi[i]));
		TestEqual(*FString::Printf(TEXT("posizione %d: visibile solo sulla prima, Hidden sulle altre"), i),
			S->GroupHeaderText->GetVisibility(),
			Prime[i] ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		TestEqual(*FString::Printf(TEXT("posizione %d: il divisore solo dove un gruppo ne segue un altro"), i),
			S->GroupDivider->GetVisibility(),
			Divisori[i] ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	for (const FRTAbilityCooldownView& V : Voci)
	{
		TestFalse(TEXT("lungo l'ordine di kit nessuna voce e' prima di un gruppo"), V.bFirstOfGroup);
	}

	// `Main.dc.html`: 22 px, il divisore da 1 px, altri 22 px.
	TestEqual(TEXT("il gap fra i gruppi e' 22 + 1 + 22"), S->GroupGap, 45.f);
	TestEqual(TEXT("il gap dentro un gruppo e' 8"), S->ItemGap, 8.f);
	return true;
}

/**
 * 🔴 **L'ICONA SI TINGE PER STATO, ANCHE SENZA GLIFO** (#3498).
 *
 * 🔑 Lo slot di prova non ha un catalogo, quindi `ApplyResolvedIconTo` non trova una texture: la tinta deve
 * arrivare lo stesso, perche' sta prima dell'uscita sulla texture. I valori per stato li fissa
 * `SlotFrameFollowsTheMockupRecipe`; qui conta che la tinta arrivi anche dove la texture manca.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMockupLookIconTintTest,
	"RefactorTactics.ScreenHud.SlotIconTintFollowsTheState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMockupLookIconTintTest::RunTest(const FString&)
{
	URTActionSlotWidget* S = MakeMockupLookSlot();
	UImage* Icona = NewObject<UImage>(S);

	S->SetAction(MockupLookView(), /*bInArmed=*/ true);
	S->ApplyResolvedIconTo(Icona);
	TestTrue(TEXT("armata: icona #FFD456"), Icona->GetColorAndOpacity().Equals(MockupLookHex(TEXT("FFD456"))));

	bool bArmata = false;
	S->SetAction(MockupLookViewFor(ERTActionSlotState::Cooldown, bArmata), false);
	S->ApplyResolvedIconTo(Icona);
	TestTrue(TEXT("in ricarica: icona #3A4454"), Icona->GetColorAndOpacity().Equals(MockupLookHex(TEXT("3A4454"))));

	S->SetAction(MockupLookView(), false);
	S->ApplyResolvedIconTo(Icona);
	TestTrue(TEXT("pronta: icona #E6EBF2"), Icona->GetColorAndOpacity().Equals(MockupLookHex(TEXT("E6EBF2"))));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
