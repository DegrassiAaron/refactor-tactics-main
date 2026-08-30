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
#include "Blueprint/WidgetBlueprintGeneratedClass.h" // Class->Bindings: il binding e' meta' del contratto
#include "Components/HorizontalBox.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchFormatData.h" // FRTMatchRules: il limite di round viene dal FORMATO
#include "Engine/World.h"
#include "EngineUtils.h" // TActorIterator

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* const DockBlueprintPath =
		TEXT("/Game/RT/UI/Match/WBP_RT_ActionDock.WBP_RT_ActionDock_C");
	const TCHAR* const RosterBlueprintPath =
		TEXT("/Game/RT/UI/Match/WBP_RT_TeamRoster.WBP_RT_TeamRoster_C");
	const TCHAR* const PanelBlueprintPath =
		TEXT("/Game/RT/UI/Match/WBP_RT_SelectedUnitPanel.WBP_RT_SelectedUnitPanel_C");
	const TCHAR* const HeaderBlueprintPath =
		TEXT("/Game/RT/UI/Match/WBP_RT_TurnHeader.WBP_RT_TurnHeader_C");

	/** Il turn manager che lo scenario ha lasciato vivo: e' il contesto che i widget consumano. */
	ARTTurnManager* FirstTurnManager(UWorld* World)
	{
		for (TActorIterator<ARTTurnManager> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

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

// =====================================================================================================
// Step 4.5 — il roster
// =====================================================================================================

/**
 * Il roster elenca la PROPRIA squadra, e non perde chi e' caduto.
 *
 * Due meta' che si sorreggono: un roster che mostrasse tutti fallirebbe la prima, uno che togliesse i morti
 * dalla lista fallirebbe la seconda — e il piano dice perche' la seconda conta: *«un'unita' che sparisce
 * dall'elenco si legge come un bug»*, e il giocatore perde il conto di quante ne aveva.
 *
 * ⚠️ **La privacy non e' provata qui e non deve esserlo.** `GetRoster()` non ha un parametro «mostra anche i
 * nemici»: la regola e' una proprieta' della firma, verificata in `FilterForTeam`. Questo test controlla che
 * il widget non aggiri quella firma per conto suo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRosterFromScenarioTest,
	"RefactorTactics.ScreenHud.RosterShowsOnlyOwnTeamAndKeepsTheFallen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudRosterFromScenarioTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	FString ReportDir;
	const FRTTestResult Result =
		URTScenarioRunner::RunById(World, TEXT("Spec.Hud.MatchWithTwoAllies"), ReportDir);
	if (!TestEqual(TEXT("lo scenario gira"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito scenario: %s"), *Result.ErrorMessage));
		return false;
	}

	ARTTurnManager* TM = FirstTurnManager(World);
	if (!TestNotNull(TEXT("lo scenario ha lasciato vivo il turn manager"), TM)) { return false; }

	// Una delle due alleate cade. Si abbatte scrivendo gli HP e non orchestrando un attacco letale: cio' che
	// si osserva e' il ROSTER, e il combattimento ha i suoi scenari — farlo passare di li' legherebbe questo
	// test al bilanciamento di un eroe.
	ARTUnit* Fallen = nullptr;
	int32 OwnTeamCount = 0;
	for (TActorIterator<ARTUnit> It(World); It; ++It)
	{
		if (It->TeamId == 0)
		{
			++OwnTeamCount;
			if (!Fallen) { Fallen = *It; }
		}
	}
	if (!TestEqual(TEXT("lo scenario mette due alleate in campo"), OwnTeamCount, 2)) { return false; }
	Fallen->Health = 0;

	UClass* RosterClass = LoadClass<URTTeamRosterWidget>(nullptr, RosterBlueprintPath);
	if (!TestNotNull(TEXT("WBP_RT_TeamRoster si carica"), RosterClass)) { return false; }

	URTTeamRosterWidget* Roster = CreateWidget<URTTeamRosterWidget>(World, RosterClass);
	if (!TestNotNull(TEXT("il roster si istanzia"), Roster)) { return false; }
	Roster->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	const TArray<FRTUnitCardView> Cards = Roster->GetRoster();

	TestEqual(TEXT("il roster ha una riga per alleata, e nessuna per l'avversaria"), Cards.Num(), 2);

	int32 Fallen_Count = 0;
	for (const FRTUnitCardView& Card : Cards)
	{
		if (!Card.bAlive) { ++Fallen_Count; }
	}
	TestEqual(TEXT("chi e' caduto resta in lista, marcato come non vivo"), Fallen_Count, 1);

	return true;
}

// =====================================================================================================
// Step 5.5 — i tre slot
// =====================================================================================================

/**
 * I tre slot si LEGGONO, non si deducono l'uno dall'altro.
 *
 * 🔺 **Il piano chiedeva un'altra prova, e non e' piu' eseguibile.** Lo Step 5.5 dice: *«pianificando uno
 * `Sprint` si accendono MOVEMENT e MAIN insieme — e' la prova che il widget legge i tre campi invece di
 * dedurli»*. Ma **nessuna azione dei cataloghi dichiara oggi `MovementAndMain`**: `Action.Sprint` lo faceva
 * fino a [D-028], e sia `RTCatalogLibrary.h` sia `BuildUnitSlots` lo scrivono a chiare lettere — quella
 * riga di codice e' *«inerte, non superflua: torna a contare il giorno che un kit usa quella forma»*.
 *
 * La prova equivalente eseguibile oggi e' l'indipendenza dei tre campi: un movimento pianificato occupa
 * MOVEMENT **e lascia MAIN libero**, un'azione principale occupa MAIN **e lascia REACTION libera**. Un
 * pannello che deducesse uno slot dall'altro — «se ho pianificato qualcosa, allora ho usato il turno» —
 * fallirebbe qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudSlotsFromScenarioTest,
	"RefactorTactics.ScreenHud.SlotsAreReadNotDeduced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudSlotsFromScenarioTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	FString ReportDir;
	const FRTTestResult Result =
		URTScenarioRunner::RunById(World, TEXT("Spec.Hud.MatchWithTwoAllies"), ReportDir);
	if (!TestEqual(TEXT("lo scenario gira"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito scenario: %s"), *Result.ErrorMessage));
		return false;
	}

	ARTUnit* Mine = FirstUnitOfTeam(World, 0);
	if (!TestNotNull(TEXT("un'unita' del team 0"), Mine)) { return false; }

	UClass* PanelClass = LoadClass<URTSelectedUnitPanelWidget>(nullptr, PanelBlueprintPath);
	if (!TestNotNull(TEXT("WBP_RT_SelectedUnitPanel si carica"), PanelClass)) { return false; }

	URTSelectedUnitPanelWidget* Panel = CreateWidget<URTSelectedUnitPanelWidget>(World, PanelClass);
	if (!TestNotNull(TEXT("il pannello si istanzia"), Panel)) { return false; }
	Panel->SetSelectedUnitForTest(Mine);

	TestTrue(TEXT("con un'unita' selezionata il pannello si mostra"), Panel->HasSelection());

	// --- niente pianificato: i tre slot sono liberi --------------------------------------------------
	{
		const FRTUnitSlotsView Slots = Panel->GetSlots();
		TestFalse(TEXT("senza piano, MOVEMENT e' libero"), Slots.Movement.bOccupied);
		TestFalse(TEXT("senza piano, MAIN e' libero"), Slots.Main.bOccupied);
		TestFalse(TEXT("senza piano, REACTION e' libera"), Slots.Reaction.bOccupied);
	}

	// --- un percorso occupa il MOVIMENTO, e nient'altro ----------------------------------------------
	Mine->PlannedWaypoints.Add(Mine->Cell);
	{
		const FRTUnitSlotsView Slots = Panel->GetSlots();
		TestTrue(TEXT("un percorso occupa MOVEMENT"), Slots.Movement.bOccupied);
		TestFalse(TEXT("e MAIN resta libero: non si deduce dal movimento"), Slots.Main.bOccupied);
		TestFalse(TEXT("e REACTION resta libera"), Slots.Reaction.bOccupied);
	}

	// --- un'azione principale occupa MAIN, e la reazione resta sua -----------------------------------
	Mine->PlannedAbilityIndex = 0;
	{
		const FRTUnitSlotsView Slots = Panel->GetSlots();
		TestTrue(TEXT("l'azione scelta occupa MAIN"), Slots.Main.bOccupied);
		TestTrue(TEXT("e MOVEMENT resta occupato dal percorso"), Slots.Movement.bOccupied);
		TestFalse(TEXT("e REACTION resta libera: i tre campi sono indipendenti"), Slots.Reaction.bOccupied);
	}

	return true;
}

// =====================================================================================================
// Step 3.6 — il limite di round
// =====================================================================================================

/**
 * Il contatore di round segue il FORMATO in vigore, e non una costante scritta nel Blueprint.
 *
 * 🔴 **E' la prova che lo Step 3.6 chiede a schermo**: *«apri il data asset del formato, cambia `RoundLimit`
 * da 12 a un altro valore, e rientra in PIE. Il numero a schermo DEVE cambiare»*. Qui il formato si cambia
 * due volte di seguito sullo stesso widget: se il numero fosse cablato, la seconda lettura sarebbe uguale
 * alla prima.
 *
 * Le due meta' contano insieme, e la seconda e' quella che il piano teme davvero:
 *  - la **funzione** decide i tre stati (`—`, `Round N/L`, `Round N`) e li fa seguire al formato;
 *  - il **binding** collega `RoundText.Text` a quella funzione. Senza, la funzione puo' essere perfetta e a
 *    schermo comparire tutt'altro — ed e' esattamente la forma del difetto trovato sul dock allo Step 7.4,
 *    dove il C++ era corretto e il grafo no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudRoundLimitFromScenarioTest,
	"RefactorTactics.ScreenHud.RoundLimitComesFromTheFormat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudRoundLimitFromScenarioTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	FString ReportDir;
	const FRTTestResult Result =
		URTScenarioRunner::RunById(World, TEXT("Spec.Hud.MatchWithTwoAllies"), ReportDir);
	if (!TestEqual(TEXT("lo scenario gira"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito scenario: %s"), *Result.ErrorMessage));
		return false;
	}

	ARTTurnManager* TM = FirstTurnManager(World);
	if (!TestNotNull(TEXT("turn manager"), TM)) { return false; }

	UClass* HeaderClass = LoadClass<URTTurnHeaderWidget>(nullptr, HeaderBlueprintPath);
	if (!TestNotNull(TEXT("WBP_RT_TurnHeader si carica"), HeaderClass)) { return false; }

	URTTurnHeaderWidget* Header = CreateWidget<URTTurnHeaderWidget>(World, HeaderClass);
	if (!TestNotNull(TEXT("l'intestazione si istanzia"), Header)) { return false; }
	Header->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);

	// --- meta' 1: il numero segue il formato ---------------------------------------------------------
	FRTMatchRules Rules;
	Rules.FormatId = TEXT("Test.Format.Twelve");
	Rules.RoundLimit = 12;
	TM->SetMatchRules(Rules);
	const FString WithTwelve = Header->GetRoundCounterText().ToString();
	TestTrue(*FString::Printf(TEXT("col formato a 12 il contatore lo dice (letto: '%s')"), *WithTwelve),
		WithTwelve.Contains(TEXT("/12")));

	Rules.FormatId = TEXT("Test.Format.Seven");
	Rules.RoundLimit = 7;
	TM->SetMatchRules(Rules);
	const FString WithSeven = Header->GetRoundCounterText().ToString();
	TestTrue(*FString::Printf(TEXT("cambiato il formato, il numero segue (letto: '%s')"), *WithSeven),
		WithSeven.Contains(TEXT("/7")));

	// La disuguaglianza e' la vera assertion: due `Contains` passerebbero anche su un testo che li contiene
	// entrambi per caso, e un numero cablato darebbe due letture identiche.
	TestNotEqual(TEXT("le due letture differiscono: il limite non e' cablato"), WithTwelve, WithSeven);

	// `RoundLimit == 0` non e' «su zero»: una partita senza formato non e' gia' scaduta.
	Rules.FormatId = NAME_None;
	Rules.RoundLimit = 0;
	TM->SetMatchRules(Rules);
	const FString NoLimit = Header->GetRoundCounterText().ToString();
	TestFalse(*FString::Printf(TEXT("senza limite non compare una barra (letto: '%s')"), *NoLimit),
		NoLimit.Contains(TEXT("/")));

	// --- meta' 2: e quel testo arriva a schermo -------------------------------------------------------
	const UWidgetBlueprintGeneratedClass* AsWidgetClass = Cast<UWidgetBlueprintGeneratedClass>(HeaderClass);
	if (!TestNotNull(TEXT("la classe del Blueprint si legge"), AsWidgetClass)) { return false; }

	bool bRoundTextIsBound = false;
	for (const FDelegateRuntimeBinding& Binding : AsWidgetClass->Bindings)
	{
		if (Binding.ObjectName == TEXT("RoundText")
			&& Binding.FunctionName == TEXT("GetRoundCounterText"))
		{
			bRoundTextIsBound = true;
			break;
		}
	}
	TestTrue(
		TEXT("`RoundText.Text` e' legato a `GetRoundCounterText`: senza il binding la funzione e' perfetta ")
		TEXT("e a schermo compare altro"),
		bRoundTextIsBound);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
