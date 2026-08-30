// CP 11.7 (#613) — il HUD guardato da uno SCENARIO, non da un occhio.
//
// 🔴 **La ragione per cui questo file esiste sta in un difetto vissuto in `main`.** Lo Step 7.4 chiede che
// il dock sappia mostrare lo stato neutro di [D-128]: `INDEX_NONE`, nessuno slot acceso. Il C++ era
// corretto — `URTActionDockWidget::GetArmedActionIndex()` restituiva `INDEX_NONE` senza selezione, e
// `RefactorTactics.ScreenHud.ActionDockShowsTheNeutralState` lo verificava e passava — mentre il
// **Blueprint** passava a `SetAction` una costante `false` e non chiamava mai quella funzione. Nessuno
// slot poteva accendersi, mai. Verde sopra, rotto sotto.
//
// Fra i due c'era un buco che nessun gate copriva, e il piano lo dichiarava: *«questa meta' e' tua, e
// nessun gate la controlla»*. `RTScreenHudWidgetTests.cpp` prova le **classi base**;
// `RTMatchWidgetAssetTests.cpp` prova i **property binding** serializzati (`Class->Bindings`) — e una
// chiamata nell'Event Graph non e' un binding. Il comportamento del grafo non lo guardava nessuno.
//
// ⚠️ **Cio' che questo file NON copre resta identico**: colori, font, ingombro, leggibilita' e «centro
// libero» stanno nel layout e restano di `PIE-V01-HUD`. Qui si prova che il widget **legge il dato
// giusto**, non che si veda bene. La differenza e' la stessa che il gemello degli asset gia' dichiara.
//
// **Come funziona la catena**, ed e' il punto riusabile:
//   1. uno scenario versionato (`Scenarios/Spec/Hud/*.json`) costruisce lo stato attraverso il percorso di
//      gioco reale — eroi veri dal catalogo, quindi kit di azioni vero;
//   2. `URTScenarioRunner::Run` **non smonta il mondo** (`RunSingle(..., bTearDownAfter=false)`), e il suo
//      commento dice perche': *«il mondo lo possiede il chiamante, e ripulirlo qui gli toglierebbe da sotto
//      i piedi gli actor su cui potrebbe voler guardare»*. E' l'appiglio, ed e' progettato;
//   3. il test aggancia il **Blueprint** — non la classe base — agli actor rimasti vivi, e legge cosa mostra.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h" // ON_SCOPE_EXIT: il mondo si distrugge anche sui `return false` intermedi
#include "RTWorldFixtures.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "UI/RTScreenHudWidgets.h"
#include "Unit/RTUnit.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Engine/World.h"
#include "EngineUtils.h" // TActorIterator

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* const DockBlueprintPath =
		TEXT("/Game/RT/UI/Match/WBP_RT_ActionDock.WBP_RT_ActionDock_C");

	/** La prima unita' della squadra indicata, fra quelle che lo scenario ha lasciato in campo. */
	ARTUnit* FirstUnitOfTeam(UWorld* World, int32 TeamId)
	{
		for (TActorIterator<ARTUnit> It(World); It; ++It)
		{
			if (It->TeamId == TeamId)
			{
				return *It;
			}
		}
		return nullptr;
	}

	/**
	 * Gli slot che il dock ha davvero costruito, nell'ordine in cui stanno nel pannello.
	 *
	 * Si leggono dal `SlotBox` e non da una variabile del Blueprint di proposito: il pannello e' cio' che il
	 * giocatore vede, una variabile e' cio' che il grafo dice di aver fatto. Quando le due divergono — ed e'
	 * successo: `SlotWidgets` veniva svuotata e mai ripopolata — la seconda mente.
	 */
	TArray<URTActionSlotWidget*> SlotsOf(URTActionDockWidget* Dock)
	{
		TArray<URTActionSlotWidget*> Out;
		if (!Dock || !Dock->WidgetTree)
		{
			return Out;
		}
		UHorizontalBox* Box = Cast<UHorizontalBox>(Dock->WidgetTree->FindWidget(TEXT("SlotBox")));
		if (!Box)
		{
			return Out;
		}
		for (int32 i = 0; i < Box->GetChildrenCount(); ++i)
		{
			if (URTActionSlotWidget* Slot = Cast<URTActionSlotWidget>(Box->GetChildAt(i)))
			{
				Out.Add(Slot);
			}
		}
		return Out;
	}

	/** Gli indici degli slot accesi. Un `TArray` e non un conteggio: «quale» e' meta' della domanda. */
	TArray<int32> ArmedIndices(const TArray<URTActionSlotWidget*>& Slots)
	{
		TArray<int32> Out;
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i]->bArmed)
			{
				Out.Add(i);
			}
		}
		return Out;
	}
}

/**
 * Il dock accende UNO slot — quello armato — e nessuno quando non c'e' niente di armato.
 *
 * 🔴 **Le due meta' contano insieme.** Un dock che non accende mai nulla soddisfa la prima e fallisce la
 * seconda, ed e' esattamente il difetto che questo test e' stato scritto per non far tornare: prima della
 * correzione dello Step 7.4 il Blueprint passava `false` fisso, quindi «nessuno slot acceso» era vero per
 * il motivo sbagliato. Verificare solo lo stato neutro l'avrebbe dichiarato sano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudDockArmedFromScenarioTest,
	"RefactorTactics.ScreenHud.DockArmsOnlyTheSelectedAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudDockArmedFromScenarioTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	// --- 1. Lo scenario costruisce lo stato -----------------------------------------------------------
	FString ReportDir;
	const FRTTestResult Result =
		URTScenarioRunner::RunById(World, TEXT("Spec.Hud.DockArmsTheSelectedAction"), ReportDir);

	if (!TestEqual(TEXT("lo scenario gira"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito scenario: %s"), *Result.ErrorMessage));
		return false;
	}

	ARTUnit* Mine = FirstUnitOfTeam(World, 0);
	if (!TestNotNull(TEXT("lo scenario ha lasciato in campo un'unita' del team 0"), Mine)) { return false; }

	// --- 2. Il BLUEPRINT, non la classe base ---------------------------------------------------------
	UClass* DockClass = LoadClass<URTActionDockWidget>(nullptr, DockBlueprintPath);
	if (!TestNotNull(TEXT("WBP_RT_ActionDock si carica"), DockClass)) { return false; }

	URTActionDockWidget* Dock = CreateWidget<URTActionDockWidget>(World, DockClass);
	if (!TestNotNull(TEXT("il dock si istanzia"), Dock)) { return false; }

	// ⚠️ **La selezione si inietta, non si passa da un `PlayerController`.** Spawnarne uno e chiamargli
	// `SelectUnit` non basta: `SetOwningPlayer` memorizza il `ULocalPlayer`, che in headless non esiste,
	// quindi `GetOwningPlayer()` resta nullo e il dock non vedrebbe nessuna unita'. E' la ragione per cui
	// `SetSelectedUnitForTest` e' stato aggiunto insieme a questo test.
	Dock->SetSelectedUnitForTest(Mine);

	const int32 ActionCount = Dock->GetActions().Num();
	if (!TestTrue(TEXT("il kit dell'eroe da' al dock piu' di una azione: senza, «uno solo» non significa niente"),
		ActionCount >= 2))
	{
		AddError(FString::Printf(TEXT("azioni disponibili: %d"), ActionCount));
		return false;
	}

	// --- 4. Stato NEUTRO: nessuno slot acceso --------------------------------------------------------
	TestEqual(TEXT("niente e' armato dopo la selezione"),
		Dock->GetArmedActionIndex(), (int32)INDEX_NONE);

	Dock->Tick(FGeometry(), 0.f);

	const TArray<URTActionSlotWidget*> Neutral = SlotsOf(Dock);
	if (!TestEqual(TEXT("il dock costruisce uno slot per azione"), Neutral.Num(), ActionCount))
	{
		return false;
	}
	TestEqual(TEXT("con INDEX_NONE nessuno slot e' acceso"), ArmedIndices(Neutral).Num(), 0);

	// --- 5. Il giocatore arma la SECONDA azione ------------------------------------------------------
	// La seconda e non la prima: con l'indice `0` un dock che accendesse sempre il primo slot passerebbe.
	constexpr int32 Armed = 1;
	Mine->SelectAbility(Armed);

	if (!TestEqual(TEXT("l'unita' registra l'azione armata"), Dock->GetArmedActionIndex(), Armed))
	{
		return false;
	}

	Dock->Tick(FGeometry(), 0.f);

	const TArray<int32> NowArmed = ArmedIndices(SlotsOf(Dock));
	TestEqual(TEXT("e a schermo si accende UNO slot solo"), NowArmed.Num(), 1);
	if (NowArmed.Num() == 1)
	{
		TestEqual(TEXT("ed e' quello armato, non un altro"), NowArmed[0], Armed);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
