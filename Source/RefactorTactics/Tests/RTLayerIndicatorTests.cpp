// L'indicatore del piano attivo nel dock (#3680, [D-501]): «Piano N di M» con ▲ e ▼.
//
// 🔑 **I tasti non hanno una strada propria.** Inoltrano a `ARTPlayerController::StepActiveLayer`, il corpo di
// `PageUp`/`PageDown`, e la vista legge piano attivo e limiti dallo stesso controller. I test lo misurano dal lato
// del controller: dopo un click si guarda `GetActiveLayer()`, non il testo.

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "UI/RTLayerIndicator.h"
#include "UI/RTScreenHudWidgets.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	bool LayerIndicatorIsShown(const UWidget* W)
	{
		return W != nullptr && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden;
	}

	/** Una mappa nel mondo, come la trova `HexMapWithContext`. */
	ARTHexMapActor* SpawnLayerIndicatorMap(UWorld* World, URTHexMapAsset* Asset)
	{
		ARTHexMapActor* Map = World->SpawnActor<ARTHexMapActor>();
		if (Map)
		{
			Map->MapAsset = Asset;
		}
		return Map;
	}
}

/**
 * **N e M sono ordinali fra i piani della mappa, e i tasti si spengono ai limiti** (#3680, punto aperto chiuso dalla
 * PR: l'ordinale, da 1).
 *
 * ⚠️ Il caso `1..2` e' la ragione dell'ordinale: con la quota grezza direbbe «Piano 1» su una mappa il cui piano piu'
 * basso e' 1, e «di 2» non si capirebbe di cosa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerIndicatorViewTest,
	"RefactorTactics.ScreenHud.LayerIndicatorCountsTheLayersTheMapHas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerIndicatorViewTest::RunTest(const FString&)
{
	TestFalse(TEXT("senza mappa non compare"), RTLayerIndicator::Build(false, 0, 0, 2).bVisible);
	TestFalse(TEXT("con un piano solo non compare"), RTLayerIndicator::Build(true, 0, 0, 0).bVisible);
	TestTrue(TEXT("e non scrive niente"), RTLayerIndicator::Build(true, 0, 0, 0).Label.IsEmpty());

	const FRTLayerIndicatorView Basso = RTLayerIndicator::Build(true, 0, 0, 2);
	TestTrue(TEXT("tre piani: compare"), Basso.bVisible);
	TestEqual(TEXT("sul piu' basso: Piano 1 di 3"), Basso.Label.ToString(), FString(TEXT("Piano 1 di 3")));
	TestTrue(TEXT("sul piu' basso si sale"), Basso.bCanGoUp);
	TestFalse(TEXT("e non si scende"), Basso.bCanGoDown);

	const FRTLayerIndicatorView Mezzo = RTLayerIndicator::Build(true, 1, 0, 2);
	TestEqual(TEXT("in mezzo: Piano 2 di 3"), Mezzo.Label.ToString(), FString(TEXT("Piano 2 di 3")));
	TestTrue(TEXT("in mezzo si sale e si scende"), Mezzo.bCanGoUp && Mezzo.bCanGoDown);

	const FRTLayerIndicatorView Alto = RTLayerIndicator::Build(true, 2, 0, 2);
	TestEqual(TEXT("sul piu' alto: Piano 3 di 3"), Alto.Label.ToString(), FString(TEXT("Piano 3 di 3")));
	TestFalse(TEXT("sul piu' alto non si sale"), Alto.bCanGoUp);
	TestTrue(TEXT("e si scende"), Alto.bCanGoDown);

	// L'ordinale e non la quota: una mappa che comincia a quota 1.
	const FRTLayerIndicatorView DaUno = RTLayerIndicator::Build(true, 1, 1, 2);
	TestEqual(TEXT("piani 1..2, sul piano 1: Piano 1 di 2"), DaUno.Label.ToString(), FString(TEXT("Piano 1 di 2")));

	// Un piano attivo fuori dai limiti non e' uno stato del controller, ma la vista non dice «Piano 0».
	TestEqual(TEXT("fuori dai limiti si conta sul limite"), RTLayerIndicator::Build(true, 5, 0, 2).Ordinal, 3);
	return true;
}

/**
 * 🔴 **▲ e ▼ cambiano il piano del CONTROLLER, e l'indicatore segue anche la tastiera** (#3680, scope 2 e 3).
 *
 * Sulla fixture vera a tre piani (`LayerStack`, #3678). Il controllo C e' la ragione per cui l'indicatore si rinfresca
 * dalla vista e non dal click: `PageUp` passa da `StepActiveLayer` senza toccare il dock.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerIndicatorFollowsControllerTest,
	"RefactorTactics.ScreenHud.LayerIndicatorFollowsTheControllerAndItsButtonsStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerIndicatorFollowsControllerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Map = SpawnLayerIndicatorMap(World, URTMatchSetupLibrary::MakeLayerStackArena(World));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!TestNotNull(TEXT("la mappa"), Map) || !TestNotNull(TEXT("il controller"), PC) || !TestNotNull(TEXT("il dock"), Dock))
	{
		return false;
	}
	int32 Min = 0;
	int32 Max = 0;
	if (!TestTrue(TEXT("premessa: il controller misura i piani della mappa"), PC->GetMapLayerRange(Min, Max))
		|| !TestTrue(TEXT("premessa: sono tre, da 0 a 2"), Min == 0 && Max == 2)
		|| !TestEqual(TEXT("premessa: si parte dal piano 0"), PC->GetActiveLayer(), 0))
	{
		return false;
	}

	Dock->SetCommandControllerForTest(PC);
	Dock->EnsureLayerIndicator();
	if (!TestNotNull(TEXT("il dock costruisce l'indicatore"), Dock->LayerIndicator.Get())
		|| !TestNotNull(TEXT("con il testo"), Dock->LayerIndicatorText.Get())
		|| !TestNotNull(TEXT("con ▲"), Dock->LayerUpButton.Get())
		|| !TestNotNull(TEXT("e con ▼"), Dock->LayerDownButton.Get()))
	{
		return false;
	}
	UWidget* Costruito = Dock->LayerIndicator;
	Dock->EnsureLayerIndicator();
	TestTrue(TEXT("costruirlo due volte non ne fa un secondo"), Dock->LayerIndicator == Costruito);

	// 🔴 Un bottone col fuoco prende `Spazio` come click: dopo ▲, il lock-in ripremerebbe ▲.
	TestFalse(TEXT("▲ non prende il fuoco della tastiera"), Dock->LayerUpButton->GetIsFocusable());
	TestFalse(TEXT("▼ nemmeno"), Dock->LayerDownButton->GetIsFocusable());

	Dock->BindNamedButtons();
	Dock->BindNamedButtons(); // due volte: un click deve restare un passo

	// --- A. lo stato iniziale ------------------------------------------------------------------------------
	Dock->RefreshLayerIndicator();
	TestTrue(TEXT("A: con tre piani l'indicatore compare"), LayerIndicatorIsShown(Dock->LayerIndicator));
	TestEqual(TEXT("A: Piano 1 di 3"), Dock->LayerIndicatorText->GetText().ToString(), FString(TEXT("Piano 1 di 3")));
	TestTrue(TEXT("A: ▲ attivo"), Dock->LayerUpButton->GetIsEnabled());
	TestFalse(TEXT("A: ▼ disattivato sul piano piu' basso"), Dock->LayerDownButton->GetIsEnabled());

	// --- B. ▲ passa dal controller, e un click e' UN passo anche legato due volte -----------------------------
	Dock->LayerUpButton->OnClicked.Broadcast();
	TestEqual(TEXT("B: ▲ porta il controller al piano 1"), PC->GetActiveLayer(), 1);
	Dock->RefreshLayerIndicator();
	TestEqual(TEXT("B: Piano 2 di 3"), Dock->LayerIndicatorText->GetText().ToString(), FString(TEXT("Piano 2 di 3")));
	TestTrue(TEXT("B: in mezzo i due tasti sono attivi"),
		Dock->LayerUpButton->GetIsEnabled() && Dock->LayerDownButton->GetIsEnabled());

	// --- C. la tastiera: `PageUp` e' `StepActiveLayer(+1)`, e il dock non ne sa niente --------------------------
	TestTrue(TEXT("C: premessa, il tasto sale"), PC->StepActiveLayer(+1));
	Dock->RefreshLayerIndicator();
	TestEqual(TEXT("C: l'indicatore segue la tastiera — Piano 3 di 3"),
		Dock->LayerIndicatorText->GetText().ToString(), FString(TEXT("Piano 3 di 3")));
	TestFalse(TEXT("C: ▲ disattivato sul piano piu' alto"), Dock->LayerUpButton->GetIsEnabled());

	// --- D. al limite un click non porta fuori dalla mappa ------------------------------------------------------
	Dock->LayerUpButton->OnClicked.Broadcast();
	TestEqual(TEXT("D: ▲ sul piano piu' alto non sale"), PC->GetActiveLayer(), 2);

	// --- E. ▼ scende di uno -------------------------------------------------------------------------------------
	Dock->LayerDownButton->OnClicked.Broadcast();
	TestEqual(TEXT("E: ▼ porta il controller al piano 1"), PC->GetActiveLayer(), 1);
	return true;
}

/**
 * **Con un piano solo, e senza controller, l'indicatore non compare** (#3680, scope 4).
 *
 * ⚠️ Il controllo positivo e' il secondo piano aggiunto alla stessa mappa: senza, «non compare» sarebbe vero anche di
 * un indicatore che non compare mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLayerIndicatorSingleLayerTest,
	"RefactorTactics.ScreenHud.LayerIndicatorHidesOnASingleLayerMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTLayerIndicatorSingleLayerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	URTHexMapAsset* Asset = NewObject<URTHexMapAsset>(World);
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), /*Radius=*/ 1))
	{
		Asset->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Asset->SortCells();
	ARTHexMapActor* Map = SpawnLayerIndicatorMap(World, Asset);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!TestNotNull(TEXT("la mappa"), Map) || !TestNotNull(TEXT("il controller"), PC) || !TestNotNull(TEXT("il dock"), Dock))
	{
		return false;
	}
	Dock->EnsureLayerIndicator();
	if (!TestNotNull(TEXT("l'indicatore esiste"), Dock->LayerIndicator.Get())) { return false; }

	Dock->RefreshLayerIndicator();
	TestFalse(TEXT("senza controller non compare"), LayerIndicatorIsShown(Dock->LayerIndicator));

	Dock->SetCommandControllerForTest(PC);
	Dock->RefreshLayerIndicator();
	TestFalse(TEXT("con un piano solo non compare"), LayerIndicatorIsShown(Dock->LayerIndicator));

	// Controllo positivo: la stessa mappa con un secondo piano.
	Asset->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, 1)));
	Asset->SortCells();
	Dock->RefreshLayerIndicator();
	TestTrue(TEXT("con un secondo piano compare"), LayerIndicatorIsShown(Dock->LayerIndicator));
	TestEqual(TEXT("e dice Piano 1 di 2"), Dock->LayerIndicatorText->GetText().ToString(), FString(TEXT("Piano 1 di 2")));
	return true;
}

/**
 * **L'asset vero del dock monta l'indicatore accanto alla lettura del movimento, e Slate lo costruisce** (#3680).
 *
 * 🔑 Il dock sta nella zona `Bottom` (`ScreenHud.EveryZoneOwnerIsMountedByClass`, e per la zona
 * `ScreenHud.TenantsFitTheirBands`): l'indicatore, stando nella sua riga, sta nella fascia bassa. Qui si misura che ci
 * sia davvero, nel pannello di `MovementReadout` e nello Slate costruito — `InsertChildAt` dopo la costruzione non
 * avrebbe raggiunto lo Slate, ed e' il difetto che questo test esclude.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDockMountsLayerIndicatorTest,
	"RefactorTactics.ScreenHud.DockMountsTheLayerIndicatorBesideTheMovementReadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTDockMountsLayerIndicatorTest::RunTest(const FString&)
{
	UClass* DockClass = LoadClass<URTActionDockWidget>(nullptr, TEXT("/Game/RT/UI/Match/WBP_RT_ActionDock.WBP_RT_ActionDock_C"));
	if (!TestNotNull(TEXT("WBP_RT_ActionDock si carica"), DockClass)) { return false; }

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	URTActionDockWidget* Dock = CreateWidget<URTActionDockWidget>(World, DockClass);
	if (!TestNotNull(TEXT("il dock si istanzia"), Dock)) { return false; }
	if (!TestNotNull(TEXT("premessa: l'asset dichiara MovementReadout"), Dock->MovementReadout.Get())
		|| !TestNotNull(TEXT("premessa: e la lettura sta in un pannello"), Dock->MovementReadout->GetParent()))
	{
		return false;
	}
	if (!TestNotNull(TEXT("l'indicatore c'e' dopo l'inizializzazione, senza che nessuno lo chieda"), Dock->LayerIndicator.Get()))
	{
		return false;
	}
	TestTrue(TEXT("sta nello stesso pannello della lettura del movimento"),
		Dock->LayerIndicator->GetParent() == Dock->MovementReadout->GetParent());
	TestTrue(TEXT("e l'albero del dock lo trova per nome"),
		Dock->WidgetTree->FindWidget(TEXT("LayerIndicator")) == Dock->LayerIndicator);

	TSharedRef<SWidget> Slate = Dock->TakeWidget();
	TestTrue(TEXT("Slate ha costruito l'indicatore"), Dock->LayerIndicator->GetCachedWidget().IsValid());
	TestTrue(TEXT("e i suoi tasti"),
		Dock->LayerUpButton && Dock->LayerUpButton->GetCachedWidget().IsValid()
		&& Dock->LayerDownButton && Dock->LayerDownButton->GetCachedWidget().IsValid());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
