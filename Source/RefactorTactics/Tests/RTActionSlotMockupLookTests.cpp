// La resa dello slot secondo il mockup (#3498): fondo e contorno per stato, tasto e nome separati, intestazione
// di gruppo, tinta dell'icona.
//
// 🔑 **I colori si confrontano con gli HEX LETTERALI del sorgente del mockup** (`sorgente-mockup/Main.dc.html`),
// non con le tabelle che il codice applica: leggere la tabella sarebbe confrontare il codice con se stesso.

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

	/** Uno slot con un `StateFrame` disegnato come `RoundedBox`, come lo avra' il Blueprint dopo la seduta. */
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
		return S;
	}

	FRTAbilityCooldownView MockupLookView()
	{
		FRTAbilityCooldownView V;
		V.ActionId = TEXT("Action.Guard");
		V.DisplayName = FText::FromString(TEXT("Guardia"));
		V.HotkeyLabel = FText::FromString(TEXT("G"));
		V.bUsableNow = true;
		V.Slot = ERTActionSlot::Main;
		return V;
	}
}

/**
 * 🔴 **FONDO, CONTORNO E SPESSORE SEGUONO LO STATO, COI VALORI DEL MOCKUP** (#3498).
 *
 * ⚠️ Il controllo sullo spessore e' la ragione della tabella `FrameWidths`: nel mockup Selected ha il bordo
 * doppio di Available, ed e' parte di cio' che li distingue oltre al colore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMockupLookFrameTest,
	"RefactorTactics.ScreenHud.SlotFrameFollowsTheMockupRecipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMockupLookFrameTest::RunTest(const FString&)
{
	URTActionSlotWidget* S = MakeMockupLookSlot();

	struct FRicetta { const TCHAR* Nome; bool bArmata; bool bRicarica; const TCHAR* Fondo; const TCHAR* Contorno; float Spessore; };
	const FRicetta Ricette[] = {
		{ TEXT("Available"), false, false, TEXT("212733"), TEXT("4A5568"), 1.f },
		{ TEXT("Selected"),  true,  false, TEXT("2B2918"), TEXT("FFD456"), 2.f },
		{ TEXT("Cooldown"),  false, true,  TEXT("151A23"), TEXT("2E3746"), 1.f },
	};
	for (const FRicetta& R : Ricette)
	{
		FRTAbilityCooldownView V = MockupLookView();
		if (R.bRicarica) { V.TurnsRemaining = 2; V.bUsableNow = false; }
		S->SetAction(V, R.bArmata);
		const FSlateBrush& Brush = S->StateFrame->Background;
		TestTrue(*FString::Printf(TEXT("%s: fondo #%s"), R.Nome, R.Fondo),
			S->StateFrame->GetBrushColor().Equals(MockupLookHex(R.Fondo)));
		TestTrue(*FString::Printf(TEXT("%s: contorno #%s"), R.Nome, R.Contorno),
			Brush.OutlineSettings.Color.GetSpecifiedColor().Equals(MockupLookHex(R.Contorno)));
		TestEqual(*FString::Printf(TEXT("%s: spessore"), R.Nome), Brush.OutlineSettings.Width, R.Spessore);
		TestEqual(*FString::Printf(TEXT("%s: resta un RoundedBox"), R.Nome), Brush.DrawAs, ESlateBrushDrawType::RoundedBox);
		TestEqual(*FString::Printf(TEXT("%s: l'alone solo da armata"), R.Nome), S->SelectedGlow->GetVisibility(),
			R.bArmata ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Una REAZIONE armata e' viola, non ambra: stesso stato, resa diversa.
	FRTAbilityCooldownView Reazione = MockupLookView();
	Reazione.Slot = ERTActionSlot::Reaction;
	S->SetAction(Reazione, /*bInArmed=*/ true);
	TestTrue(TEXT("reazione armata: fondo #221E3A"), S->StateFrame->GetBrushColor().Equals(MockupLookHex(TEXT("221E3A"))));
	TestTrue(TEXT("reazione armata: contorno #7C5CFF"),
		S->StateFrame->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(MockupLookHex(TEXT("7C5CFF"))));
	TestEqual(TEXT("reazione armata: nessun alone ambra"), S->SelectedGlow->GetVisibility(), ESlateVisibility::Collapsed);
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
 * 🔴 **L'INTESTAZIONE DEL GRUPPO STA SULLA PRIMA VOCE, E SULLE ALTRE TIENE IL POSTO** (#3498).
 *
 * ⚠️ `Hidden` e non `Collapsed` sulle altre: uno slot con l'intestazione e uno senza devono restare alla stessa
 * altezza, altrimenti la fila si scalina.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMockupLookGroupHeaderTest,
	"RefactorTactics.ScreenHud.SlotGroupHeaderOnTheFirstOfEachGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMockupLookGroupHeaderTest::RunTest(const FString&)
{
	// La lista B K C C N K: in lettura C C | B | K N K.
	const ERTActionGroup Kit[] = { ERTActionGroup::Base, ERTActionGroup::Kit, ERTActionGroup::Common,
		ERTActionGroup::Common, ERTActionGroup::None, ERTActionGroup::Kit };
	TArray<FRTAbilityCooldownView> Voci;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Kit); ++i)
	{
		FRTAbilityCooldownView V = MockupLookView();
		V.AbilityIndex = i;
		V.Group = Kit[i];
		Voci.Add(V);
	}
	const TArray<FRTAbilityCooldownView> Lettura = URTHudViewModel::OrderForReading(Voci);
	const bool Prime[] = { true, false, true, true, false, false };
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
	}
	for (const FRTAbilityCooldownView& V : Voci)
	{
		TestFalse(TEXT("lungo l'ordine di kit nessuna voce e' prima di un gruppo"), V.bFirstOfGroup);
	}
	return true;
}

/**
 * 🔴 **L'ICONA SI TINGE PER STATO, ANCHE SENZA GLIFO** (#3498).
 *
 * 🔑 Lo slot di prova non ha un catalogo, quindi `ApplyResolvedIconTo` non trova una texture: la tinta deve
 * arrivare lo stesso, perche' sta prima dell'uscita sulla texture.
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

	FRTAbilityCooldownView InRicarica = MockupLookView();
	InRicarica.TurnsRemaining = 2;
	InRicarica.bUsableNow = false;
	S->SetAction(InRicarica, false);
	S->ApplyResolvedIconTo(Icona);
	TestTrue(TEXT("in ricarica: icona #3A4454"), Icona->GetColorAndOpacity().Equals(MockupLookHex(TEXT("3A4454"))));

	S->SetAction(MockupLookView(), false);
	S->ApplyResolvedIconTo(Icona);
	TestTrue(TEXT("pronta: icona #E6EBF2"), Icona->GetColorAndOpacity().Equals(MockupLookHex(TEXT("E6EBF2"))));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
