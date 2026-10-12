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
#include "Tests/RTWorldFixtures.h"
#include "Misc/ScopeExit.h" // ON_SCOPE_EXIT: il mondo si distrugge anche sui `return false` intermedi
#include "RTWorldFixtures.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "UI/RTScreenHudWidgets.h"
#include "UI/RTHudLayout.h"     // #3661: le fasce in cui gli inquilini devono entrare
#include "UI/RTHudZoneWidget.h"
#include "Components/CanvasPanelSlot.h" // #3661: la posa delle zone letta dagli slot
#include "Tests/RTHudGeometryForTest.h" // #3661: le risoluzioni della DoD
#include "Unit/RTUnit.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h" // Class->Bindings: il binding e' meta' del contratto
#include "Components/HorizontalBox.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchFormatData.h" // FRTMatchRules: il limite di round viene dal FORMATO
#include "Engine/World.h"
#include "EngineUtils.h" // TActorIterator
#include "Player/RTPlayerController.h" // il percorso REALE di armamento (#2986)
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // DefaultLoadoutFor: il loadout che `SpawnHero` equipaggia
#include "Ability/RTHeroCatalogLibrary.h" // #3419: il roster spedito, non uno scenario
#include "Ability/RTHeroData.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

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

	/**
	 * Gli INDICI DI KIT degli slot accesi, cioe' l'identita' che ciascuno porta in `Action.AbilityIndex`. Un
	 * `TArray` e non un conteggio: «quale» e' meta' della domanda.
	 *
	 * ⌫ **Restituiva la POSIZIONE nella fila**, e i due test qui sotto confrontavano quella con l'indice
	 * armato. Era vero finche' la dock disponeva gli slot nell'ordine di kit; dalla seduta `U63` li dispone in
	 * ordine di lettura ([D-455], #3478) e le due cose divergono — in partita l'indice 1 sta in sesta posizione.
	 * ⛔ La posizione a schermo non e' mai un indice ([D-397] punti 2 e 4): l'identita' e' il campo.
	 */
	TArray<int32> ArmedIndices(const TArray<URTActionSlotWidget*>& Slots)
	{
		TArray<int32> Out;
		for (const URTActionSlotWidget* Slot : Slots)
		{
			if (Slot->bArmed)
			{
				Out.Add(Slot->Action.AbilityIndex);
			}
		}
		return Out;
	}

	/** Lo slot che porta l'indice di kit chiesto, ovunque stia nella fila; `nullptr` se nessuno lo porta. */
	URTActionSlotWidget* SlotForKitIndex(const TArray<URTActionSlotWidget*>& Slots, int32 KitIndex)
	{
		for (URTActionSlotWidget* Slot : Slots)
		{
			if (Slot->Action.AbilityIndex == KitIndex)
			{
				return Slot;
			}
		}
		return nullptr;
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

	ARTUnit* Mine = RTWorldFixtures::FirstUnitOfTeam(World, 0);
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

	ARTUnit* Mine = RTWorldFixtures::FirstUnitOfTeam(World, 0);
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
 *  - il **numero arriva a schermo** dalle porte dell'asset. Senza, la funzione puo' essere perfetta e a schermo
 *    comparire tutt'altro — ed e' esattamente la forma del difetto trovato sul dock allo Step 7.4, dove il C++
 *    era corretto e il grafo no.
 *
 * ⌫ *Fino alla seduta `U75` ([#3635]) la seconda meta' cercava il binding di proprieta' fra `RoundText.Text` e
 * `GetRoundCounterText`. La tavola A spezza il contatore in due testi, `RoundValueText` e `RoundLimitText`, che
 * scrive `RefreshLook`: il binding non c'e' piu', e la meta' legge le porte dell'istanza del Blueprint vero.*
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

	// --- meta' 2: e quel numero arriva a schermo ------------------------------------------------------
	// Le porte dell'istanza del Blueprint vero: se l'asset non le avesse, `RefreshLook` non scriverebbe niente
	// e la funzione resterebbe perfetta senza che nulla comparisse.
	UTextBlock* Valore = Header->RoundValueText;
	UTextBlock* Limite = Header->RoundLimitText;
	if (!TestNotNull(TEXT("WBP_RT_TurnHeader ha la porta `RoundValueText`"), Valore)) { return false; }
	if (!TestNotNull(TEXT("WBP_RT_TurnHeader ha la porta `RoundLimitText`"), Limite)) { return false; }

	const auto Mostrato = [Limite]()
	{
		return Limite->GetVisibility() != ESlateVisibility::Collapsed && Limite->GetVisibility() != ESlateVisibility::Hidden;
	};
	Rules.FormatId = TEXT("Test.Format.Twelve");
	Rules.RoundLimit = 12;
	TM->SetMatchRules(Rules);
	Header->RefreshLook(/*bRecheckObjective=*/ false);
	const FString ScrittoDodici = Limite->GetText().ToString();
	TestTrue(*FString::Printf(TEXT("col formato a 12 la porta dice «/ 12» (letto: '%s')"), *ScrittoDodici),
		ScrittoDodici.Equals(TEXT("/ 12"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("col formato a 12 la porta del limite si vede"), Mostrato());
	// Due cifre almeno, come la tavola («04»): un testo di default o vuoto, cioe' una porta mai scritta, cade qui.
	const FString Round = Valore->GetText().ToString();
	TestTrue(*FString::Printf(TEXT("il round a schermo e' un numero di due cifre (letto: '%s')"), *Round),
		Round.Len() >= 2 && Round.IsNumeric());

	Rules.FormatId = TEXT("Test.Format.Seven");
	Rules.RoundLimit = 7;
	TM->SetMatchRules(Rules);
	Header->RefreshLook(false);
	const FString ScrittoSette = Limite->GetText().ToString();
	TestTrue(*FString::Printf(TEXT("cambiato il formato, la porta segue: «/ 7» (letto: '%s')"), *ScrittoSette),
		ScrittoSette.Equals(TEXT("/ 7"), ESearchCase::CaseSensitive));
	TestTrue(TEXT("col formato a 7 la porta del limite si vede"), Mostrato());

	Rules.FormatId = NAME_None;
	Rules.RoundLimit = 0;
	TM->SetMatchRules(Rules);
	Header->RefreshLook(false);
	TestFalse(TEXT("senza limite la porta del limite non si vede: «/ 0» sarebbe una partita scaduta"), Mostrato());

	// Il C++ e' la sola sede: un binding di proprieta' rimasto nell'asset sarebbe una seconda fonte dello stesso
	// testo, e la prossima modifica ne aggiornerebbe una sola.
	const UWidgetBlueprintGeneratedClass* AsWidgetClass = Cast<UWidgetBlueprintGeneratedClass>(HeaderClass);
	if (!TestNotNull(TEXT("la classe del Blueprint si legge"), AsWidgetClass)) { return false; }
	for (const FDelegateRuntimeBinding& Binding : AsWidgetClass->Bindings)
	{
		AddError(FString::Printf(TEXT("WBP_RT_TurnHeader ha ancora un binding di proprieta': %s.%s <- %s"),
			*Binding.ObjectName, *Binding.PropertyName.ToString(), *Binding.FunctionName.ToString()));
	}

	return true;
}

/**
 * 🔴 **UNA POSIZIONE DI KIT VUOTA ARRIVA FINO AL GRAFO DEL DOCK, E NON SPOSTA GLI SLOT** (`#2989`).
 *
 * 🔑 **E' l'ultimo tratto che nessun oracolo attraversava.** Il buco nel kit era misurato **solo sul
 * ViewModel** — `HudViewModel.KitHoleDoesNotRenumberTheSlots` — e il grafo di `WBP_RT_ActionDock` era
 * esercitato **solo su kit sani**, da `DockArmsOnlyTheSelectedAction`. Fra i due restava scoperto
 * precisamente il punto in cui i due si incontrano: il `For Loop` del dock che prende `Azioni[i]` per
 * POSIZIONE e lo passa allo slot i-esimo.
 *
 * ⚠️ **Il grafo indicizza per posizione, e questo test e' cio' che lo rende lecito.** L'ubergraph confronta
 * con `Equal (Integer)` l'indice del ciclo e `GetArmedActionIndex()`: e' corretto **soltanto** finche'
 * `BuildAbilityCooldowns` emette una riga per ogni posizione di kit. Prima di `#2987` saltava le vuote, e
 * su un kit con un buco il dock avrebbe acceso lo slot sbagliato — senza che niente smettesse di
 * compilare, e senza che nessun test cadesse.
 *
 * 🔑 **L'oracolo forte e' il controllo C**, non il conteggio: si arma una posizione **dopo** il buco. E'
 * l'unico che distingue «il dock disegna il numero giusto di riquadri» da «il dock accende quello giusto»,
 * ed e' esattamente cio' che una rinumerazione romperebbe.
 *
 * ⛔ **Non duplica `DockArmsOnlyTheSelectedAction`**: quello prova che il dock accenda UNO slot e sia
 * l'armato, su un kit senza buchi. Qui il kit ne ha uno, ed e' l'unica differenza — deliberata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudDockKitHoleTest,
	"RefactorTactics.ScreenHud.DockSurvivesAHoleInTheKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudDockKitHoleTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	FString ReportDir;
	const FRTTestResult Result =
		URTScenarioRunner::RunById(World, TEXT("Spec.Hud.DockArmsTheSelectedAction"), ReportDir);
	if (!TestEqual(TEXT("lo scenario gira"),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito scenario: %s"), *Result.ErrorMessage));
		return false;
	}

	ARTUnit* Mine = RTWorldFixtures::FirstUnitOfTeam(World, 0);
	if (!TestNotNull(TEXT("lo scenario ha lasciato in campo un'unita' del team 0"), Mine)) { return false; }

	// Anti-vacuita': serve un buco FRA due posizioni popolate, e una posizione DOPO il buco da armare.
	if (!TestTrue(TEXT("premessa: il kit ha almeno tre posizioni"), Mine->NumAbilities() >= 3))
	{
		return false;
	}

	// Il buco. `Abilities` e' pubblico: e' lo stesso modo in cui `KitHoleDoesNotRenumberTheSlots` lo scava,
	// e resta l'unico modo di produrre il caso senza un catalogo malformato.
	constexpr int32 Buco = 1;
	constexpr int32 DopoIlBuco = 2;
	const int32 PosizioniDiKit = Mine->NumAbilities();
	Mine->Abilities[Buco] = nullptr;

	UClass* DockClass = LoadClass<URTActionDockWidget>(nullptr, DockBlueprintPath);
	if (!TestNotNull(TEXT("WBP_RT_ActionDock si carica"), DockClass)) { return false; }

	URTActionDockWidget* Dock = CreateWidget<URTActionDockWidget>(World, DockClass);
	if (!TestNotNull(TEXT("il dock si istanzia"), Dock)) { return false; }
	Dock->SetSelectedUnitForTest(Mine);

	// --- A. il dock disegna un riquadro per ogni POSIZIONE, buco compreso -----------------------------
	Dock->Tick(FGeometry(), 0.f);
	const TArray<URTActionSlotWidget*> Slots = SlotsOf(Dock);

	TestEqual(TEXT("A: un riquadro per ogni posizione di kit, la vuota compresa"),
		Slots.Num(), PosizioniDiKit);

	// --- B. il riquadro del buco porta la posizione vuota, non l'azione seguente ----------------------
	// ⚠️ Il riquadro si CERCA per indice di kit, non per posizione: la fila e' in ordine di lettura (`U63`).
	if (const URTActionSlotWidget* RiquadroDelBuco = SlotForKitIndex(Slots, Buco);
		TestNotNull(TEXT("premessa: un riquadro porta l'indice del buco"), RiquadroDelBuco))
	{
		TestTrue(TEXT("B: il riquadro del buco porta una posizione VUOTA"),
			RiquadroDelBuco->Action.ActionId.IsNone());
	}

	// --- C. armando DOPO il buco si accende quel riquadro, non uno slittato ---------------------------
	// 🔑 E' l'asserzione che cade se il ViewModel torna a saltare le posizioni vuote: li' `Azioni[2]`
	// sarebbe l'azione della posizione 3, e il dock accenderebbe il riquadro sbagliato.
	Mine->SelectAbility(DopoIlBuco);
	Dock->Tick(FGeometry(), 0.f);

	const TArray<int32> Accesi = ArmedIndices(SlotsOf(Dock));
	TestEqual(TEXT("C: si accende un riquadro solo"), Accesi.Num(), 1);
	if (Accesi.Num() == 1)
	{
		TestEqual(TEXT("C: ed e' quello della posizione armata, non uno slittato dal buco"),
			Accesi[0], DopoIlBuco);
	}

	// --- D. dal PERCORSO REALE, la posizione vuota non accende niente ---------------------------------
	// ⌫ **Questo controllo chiamava `ARTUnit::SelectAbility(Buco)` e falliva, ed era il TEST a essere mal
	// posto.** Quel metodo e' il setter di basso livello: accetta un indice valido-ma-nullo e scrive
	// `SelectedAbilityIndex`, cosi' il dock accendeva un riquadro vuoto. Ma quello stato **non e'
	// raggiungibile in gioco**: misurato, i soli chiamanti di produzione sono `SelectAbilityForCurrent`
	// — che passa un indice **dopo** la guardia sulla posizione vuota — e tre siti che passano
	// `INDEX_NONE` (il disarmo col click, l'uscita dal targeting con `RMB`, il Cleanup di [D-397] §5).
	//
	// 🔑 **Cosi' il controllo misura l'integrazione invece del setter**: `#2986` garantisce che una
	// posizione vuota **disarmi** invece di restare armata — *«nulla e' stato armato»* — e qui si verifica
	// che il dock lo renda, cioe' che quella garanzia arrivi fino allo schermo. Senza il percorso reale,
	// questo controllo provava una combinazione che nessun input puo' produrre.
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (TestNotNull(TEXT("premessa di D: il controller esiste"), PC))
	{
		PC->SelectActorForTest(Mine);
		// Si parte da uno stato ARMATO — il controllo C lo ha appena acceso — perche' un dock che non
		// accendesse mai nulla passerebbe questo controllo senza significare niente.
		PC->SelectAbilityForCurrentForTest(Buco);

		// ⚠️ **Il modello NON torna al neutro, ed e' corretto.** `#2986` garantisce che una posizione vuota
		// *non risulti armata*, non che disarmi cio' che lo era: una richiesta rifiutata non cambia lo
		// stato. ⌫ La prima stesura di questo controllo chiedeva `INDEX_NONE` e falliva con
		// *«to be -1, but it was 2»* — avevo letto il ramo del disarmo esplicito come se fosse quello della
		// posizione vuota.
		TestNotEqual(TEXT("D: la posizione vuota non diventa quella armata (#2986)"),
			Mine->SelectedAbilityIndex, Buco);

		Dock->Tick(FGeometry(), 0.f);
		TestFalse(TEXT("D: e il riquadro del buco non si accende MAI, qualunque cosa si prema"),
			ArmedIndices(SlotsOf(Dock)).Contains(Buco));
	}

	return true;
}

// =====================================================================================================
// #3419 — la forma dell'impronta, dal roster allo schermo
// =====================================================================================================

/**
 * ⛔ **Il tooltip d'abilita' dice la forma, nel dock della partita** (#3419).
 *
 * 🔑 E' tutto il percorso della partita: l'unita' allestita come la allestisce `SpawnHero` — la classe
 * Blueprint dell'eroe, i dati del roster spedito, il loadout di default —, il **grafo** di `WBP_RT_ActionDock`
 * che a ogni `Tick` passa le righe agli slot, lo slot dell'asset e il testo della porta `LinesText` del suo
 * tooltip. I test della forma in `RTActionTooltipTests.cpp` costruiscono la riga a mano, e non vedono nessuno di
 * questi passaggi: togliere la copia di `Shape` in `BuildAbilityCooldowns` non rendeva rosso nessun test di
 * `HudViewModel.*` e `ScreenHud.*`.
 *
 * ⚠️ L'oracolo e' l'azione dell'unita', non una lista d'eroi: un'area nuova nel roster si controlla da sola.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDockTooltipShowsTheShapeTest,
	"RefactorTactics.ScreenHud.DockTooltipShowsTheShapeOfEveryRosterArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTDockTooltipShowsTheShapeTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	UClass* DockClass = LoadClass<URTActionDockWidget>(nullptr, DockBlueprintPath);
	if (!TestNotNull(TEXT("WBP_RT_ActionDock si carica"), DockClass)) { return false; }

	int32 Aree = 0;
	int32 Cella = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		// La classe che `ARTGameMode` associa all'eroe (`HeroUnitClasses`): `Hero.Aevik` -> `BP_Unit_Aevik`.
		const FString Nome = ARTUnit::ShortHeroName(Hero->HeroId, FString());
		UClass* UnitClass = LoadClass<ARTUnit>(nullptr, *FString::Printf(
			TEXT("/Game/RT/Characters/%s/Blueprints/BP_Unit_%s.BP_Unit_%s_C"), *Nome, *Nome, *Nome));
		if (!TestNotNull(*FString::Printf(TEXT("%s: la classe Blueprint dell'eroe"), *Nome), UnitClass)) { continue; }
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(UnitClass, FTransform::Identity);
		if (!TestNotNull(TEXT("unita'"), Unit)) { continue; }
		Unit->TeamId = 0;
		Unit->ConfigureFromHeroData(Hero);
		Unit->EquipLoadout(URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId));
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(FRTCellId(Cella++, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);

		URTActionDockWidget* Dock = CreateWidget<URTActionDockWidget>(World, DockClass);
		if (!TestNotNull(TEXT("il dock si istanzia"), Dock)) { continue; }
		Dock->SetSelectedUnitForTest(Unit);
		Dock->Tick(FGeometry(), 0.f);

		const TArray<URTActionSlotWidget*> Slots = SlotsOf(Dock);
		TestTrue(*FString::Printf(TEXT("%s: il dock costruisce gli slot"), *Nome), Slots.Num() > 0);
		for (const URTActionSlotWidget* Slot : Slots)
		{
			const URTActionData* A = Slot ? Unit->GetAbility(Slot->Action.AbilityIndex) : nullptr;
			if (!A || A->Shape != ERTAbilityShape::Area || A->AreaRadius <= 0) { continue; }
			++Aree;
			const FString Id = A->Def.ActionId.ToString();
			const FString Attesa = FString::Printf(TEXT("Forma  Area, raggio %d"), A->AreaRadius);
			const URTActionTooltipWidget* Tooltip = Cast<URTActionTooltipWidget>(Slot->GetToolTip());
			if (!TestNotNull(*FString::Printf(TEXT("%s: il tooltip e' il widget dell'asset"), *Id), Tooltip)) { continue; }
			if (TestNotNull(*FString::Printf(TEXT("%s: la porta delle righe"), *Id), Tooltip->LinesText.Get()))
			{
				const FString Letto = Tooltip->LinesText->GetText().ToString().Replace(TEXT("\n"), TEXT(" | "));
				AddInfo(FString::Printf(TEXT("%s, righe a schermo: %s"), *Id, *Letto));
				TestTrue(*FString::Printf(TEXT("%s: a schermo c'e' '%s' (letto: '%s')"), *Id, *Attesa, *Letto),
					Letto.Contains(Attesa));
			}
		}
	}
	TestTrue(TEXT("premessa: il roster ha almeno un'area con un raggio"), Aree > 0);
	return true;
}

namespace
{
	/** Tutti i widget dello Screen HUD sotto `Radice`, scendendo DENTRO i `UUserWidget` innestati. */
	void RTRaccogliWidgetHud(UUserWidget* Radice, TArray<URTScreenHudWidgetBase*>& Out, int32 Profondita = 0)
	{
		if (Radice == nullptr || Radice->WidgetTree == nullptr || Profondita > 8)
		{
			return;
		}
		Radice->WidgetTree->ForEachWidget([&Out, Profondita](UWidget* W)
		{
			if (UUserWidget* Innestato = Cast<UUserWidget>(W))
			{
				if (URTScreenHudWidgetBase* Hud = Cast<URTScreenHudWidgetBase>(Innestato))
				{
					Out.AddUnique(Hud);
				}
				RTRaccogliWidgetHud(Innestato, Out, Profondita + 1);
			}
		});
	}
}

/**
 * 🔴 **Gli inquilini entrano nelle loro fasce, a ogni risoluzione di #3661** (D-497 (2)).
 *
 * 🔑 **Esiste perche' il test sul rettangolo da solo non puo' fallire**: moltiplica frazioni per dimensioni. Qui
 * l'HUD tattico VERO si costruisce in uno scenario, gli si da' il contesto della partita e una selezione, e Slate
 * calcola la misura desiderata di ogni zona — cornice e inquilino — in unita' Slate. La si confronta con lo spazio
 * che il dato le assegna alle quattro risoluzioni: a 16:10 lo schermo in unita' Slate e' piu' stretto (1728).
 *
 * 🔴 **Ha gia' trovato un difetto alla prima esecuzione**: il roster con tre unita' chiede 259 di altezza, e la
 * fascia alta al 19% gliene lasciava 197. L'autore ha scelto che la colonna sinistra abbia il suo taglio
 * (`FRTHudBands::LeftTop`). Due scenari: due unita' proprie (il formato della v0.1) e tre (la sandbox).
 *
 * ⚠️ **Limiti dichiarati.** E' la misura del contenuto che lo scenario produce, non del caso peggiore: il registro
 * vuoto ha misura zero e non si misura. E gli slot delle azioni del dock li costruisce il grafo Blueprint al tick,
 * che headless senza paint non gira: la misura del dock qui e' quella della sua cornice. In PIE (`U79`) il dock
 * occupa circa 1020x110 unita', contro le 1720 della fascia bassa a 16:10. Restano di PIE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudTenantsFitTheirBandsTest,
	"RefactorTactics.ScreenHud.TenantsFitTheirBands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHudTenantsFitTheirBandsTest::RunTest(const FString&)
{
	UClass* HudClass = LoadClass<URTTacticalHUDWidget>(nullptr,
		TEXT("/Game/RT/UI/Match/WBP_RT_TacticalHUD.WBP_RT_TacticalHUD_C"));
	if (!TestNotNull(TEXT("WBP_RT_TacticalHUD si carica"), HudClass)) { return false; }

	const FRTHudBands& F = RTHudLayout::Bands();
	const float M = RTHudLayout::ZoneMargin;

	for (const TCHAR* Scenario : { TEXT("Spec.Hud.MatchWithTwoAllies"), TEXT("Visual.Map.LayerStack") })
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		FString ReportDir;
		const FRTTestResult Result = URTScenarioRunner::RunById(World, Scenario, ReportDir);
		if (!TestEqual(*FString::Printf(TEXT("lo scenario %s gira"), Scenario),
			static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
		{
			AddError(FString::Printf(TEXT("esito scenario: %s"), *Result.ErrorMessage));
			return false;
		}
		ARTTurnManager* TM = FirstTurnManager(World);
		ARTUnit* Mia = RTWorldFixtures::FirstUnitOfTeam(World, 0);
		if (!TestNotNull(TEXT("il turn manager"), TM) || !TestNotNull(TEXT("un'unita' del team 0"), Mia)) { return false; }

		URTTacticalHUDWidget* Hud = CreateWidget<URTTacticalHUDWidget>(World, HudClass);
		if (!TestNotNull(TEXT("l'HUD si istanzia"), Hud)) { return false; }

		// Costruirlo: `NativeConstruct` posa le zone dal dato e i figli costruiscono i loro pezzi.
		TSharedRef<SWidget> Radice = Hud->TakeWidget();

		// 🔴 **Il cablaggio: `NativeConstruct` chiama davvero `ApplyBandLayout`** (rilievo della review di #3690).
		// Gli altri due test posano il dato a mano su una copia dell'albero: senza questo controllo la chiamata
		// potrebbe sparire, gli anchor a frazioni dell'asset tornerebbero in partita, e tutti resterebbero verdi.
		// ⚠️ Che `NativeConstruct`/`NativeDestruct` chiedano e restituiscano il rettangolo al viewport client non si
		// misura qui: in un mondo di prova non c'e' viewport. Resta di PIE (`PIE-HUD-SCENA`).
		{
			int32 ZonePosate = 0;
			int32 FinestrePosate = 0;
			Hud->WidgetTree->ForEachWidget([&](UWidget* W)
			{
				const UCanvasPanelSlot* Slot = W ? Cast<UCanvasPanelSlot>(W->Slot) : nullptr;
				if (Slot == nullptr || Slot->Parent != Hud->WidgetTree->RootWidget)
				{
					return;
				}
				const FAnchorData Reale = Slot->GetLayout();
				if (const URTHudZoneWidget* Zona = Cast<URTHudZoneWidget>(W))
				{
					const FAnchorData Atteso = RTHudLayout::ZoneLayout(Zona->ZoneId, F);
					TestTrue(*FString::Printf(TEXT("%s: la zona %s e' posata dal dato in NativeConstruct"), Scenario,
						*URTHudZoneWidget::ZoneName(Zona->ZoneId)),
						Reale.Offsets == Atteso.Offsets
						&& Reale.Anchors.Minimum.Equals(Atteso.Anchors.Minimum)
						&& Reale.Anchors.Maximum.Equals(Atteso.Anchors.Maximum));
					++ZonePosate;
				}
				else if (W->IsA<URTFastDecisionWidget>())
				{
					const FVector2D Spostamento = RTHudLayout::SceneCenterOffsetSlate(F);
					TestTrue(*FString::Printf(TEXT("%s: la finestra di reazione e' sul centro della scena"), Scenario),
						FMath::IsNearlyEqual(Reale.Offsets.Left, static_cast<float>(Spostamento.X), 0.01f)
						&& FMath::IsNearlyEqual(Reale.Offsets.Top, static_cast<float>(Spostamento.Y), 0.01f));
					++FinestrePosate;
				}
			});
			TestEqual(*FString::Printf(TEXT("%s: le zone posate sono tutte"), Scenario), ZonePosate,
				static_cast<int32>(ERTHudZone::Count));
			TestEqual(*FString::Printf(TEXT("%s: una finestra di reazione posata"), Scenario), FinestrePosate, 1);
		}

		// Il contesto della partita e la selezione, a TUTTI i widget dell'HUD, poi un giro di aggiornamento. Il dock
		// riceve anche un controller: l'indicatore del piano legge da li' piano attivo e limiti della mappa (#3680).
		ARTPlayerController* Comandante = World->SpawnActor<ARTPlayerController>();
		URTActionDockWidget* DockDellHud = nullptr;
		TArray<URTScreenHudWidgetBase*> Widget;
		RTRaccogliWidgetHud(Hud, Widget);
		for (URTScreenHudWidgetBase* W : Widget)
		{
			if (URTActionDockWidget* Dock = Cast<URTActionDockWidget>(W))
			{
				Dock->SetCommandControllerForTest(Comandante);
				Dock->RefreshLayerIndicator();
				DockDellHud = Dock;
			}
			W->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
			W->SetSelectedUnitForTest(Mia);
			if (URTTeamRosterWidget* Roster = Cast<URTTeamRosterWidget>(W)) { Roster->RefreshCards(); }
			if (URTSelectedUnitPanelWidget* Pannello = Cast<URTSelectedUnitPanelWidget>(W)) { Pannello->RefreshLook(); }
			if (URTTurnHeaderWidget* Testata = Cast<URTTurnHeaderWidget>(W)) { Testata->RefreshLook(); }
			if (URTPlayerEventLogWidget* Registro = Cast<URTPlayerEventLogWidget>(W)) { Registro->RefreshLook(); }
			if (URTPlanCommitWidget* Conferma = Cast<URTPlanCommitWidget>(W)) { Conferma->RefreshButtons(); }
			if (URTActionDockWidget* Dock = Cast<URTActionDockWidget>(W)) { Dock->RefreshMovementReadout(); }
		}

		Radice->SlatePrepass(1.f); // la misura desiderata in unita' Slate

		int32 Misurate = 0;
		Hud->WidgetTree->ForEachWidget([&](UWidget* W)
		{
			const URTHudZoneWidget* Zona = Cast<URTHudZoneWidget>(W);
			if (Zona == nullptr)
			{
				return;
			}
			const FVector2D Misura = Zona->GetDesiredSize();
			const UWidget* Inquilino = Zona->GetContentForSlot(TEXT("Content"));
			AddInfo(FString::Printf(TEXT("%s: %-14s inquilino %-30s misura desiderata %.0fx%.0f"), Scenario,
				*URTHudZoneWidget::ZoneName(Zona->ZoneId),
				Inquilino ? *Inquilino->GetClass()->GetName() : TEXT("<nessuno>"), Misura.X, Misura.Y));
			if (Inquilino == nullptr || Misura.X <= 1.0 || Misura.Y <= 1.0)
			{
				return; // vuota, o collassata in questo scenario (il registro senza eventi): niente da misurare
			}
			++Misurate;

			for (const RTHudRisoluzioni::FRisoluzione& Ris : RTHudRisoluzioni::Tutte())
			{
				const FVector2D S = RTHudRisoluzioni::InSlate(Ris);
				const float CentroX = static_cast<float>(S.X) - F.Left - F.Right;
				const float CentroY = static_cast<float>(S.Y) - F.Top - F.Bottom;
				FVector2D Spazio;
				switch (Zona->ZoneId)
				{
				case ERTHudZone::TopLeft:     Spazio = FVector2D(F.Left, F.LeftTop); break;
				case ERTHudZone::TopCenter:   Spazio = FVector2D(CentroX, F.Top); break;
				case ERTHudZone::TopRight:    Spazio = FVector2D(F.Right, F.Top); break;
				case ERTHudZone::MiddleLeft:  Spazio = FVector2D(F.Left, static_cast<float>(S.Y) - F.LeftTop - F.Bottom); break;
				case ERTHudZone::MiddleRight: Spazio = FVector2D(F.Right, CentroY); break;
				case ERTHudZone::Bottom:      Spazio = FVector2D(S.X, F.Bottom); break;
				default:                      Spazio = FVector2D::ZeroVector; break;
				}
				Spazio -= FVector2D(2.f * M, 2.f * M);
				TestTrue(*FString::Printf(TEXT("%s, %s, %s: la zona chiede %.0fx%.0f e ha %.0fx%.0f"),
					Scenario, Ris.Nome, *URTHudZoneWidget::ZoneName(Zona->ZoneId), Misura.X, Misura.Y, Spazio.X, Spazio.Y),
					Misura.X <= Spazio.X + 0.5 && Misura.Y <= Spazio.Y + 0.5);
			}
		});
		// Anti-vacuita': roster, testata, Conferma, scheda e dock hanno contenuto in ogni scenario.
		TestTrue(*FString::Printf(TEXT("%s: almeno quattro zone misurate (%d)"), Scenario, Misurate), Misurate >= 4);

		// 🔑 **L'indicatore del piano sta nel dock, e il dock nella zona `Bottom`** (#3680): la misura sopra lo
		// contiene, perche' la zona misura il dock con la sua riga. Compare solo dove la mappa ha piu' di un piano, e
		// `LayerStack` ne ha tre: li' deve comparire, altrimenti la misura non lo conterrebbe affatto.
		if (TestNotNull(*FString::Printf(TEXT("%s: l'HUD monta un dock"), Scenario), DockDellHud)
			&& TestNotNull(*FString::Printf(TEXT("%s: il dock ha l'indicatore del piano"), Scenario), DockDellHud->LayerIndicator.Get()))
		{
			const URTHudZoneWidget* ZonaDelDock = nullptr;
			Hud->WidgetTree->ForEachWidget([&](UWidget* W)
			{
				const URTHudZoneWidget* Zona = Cast<URTHudZoneWidget>(W);
				if (Zona && Zona->GetContentForSlot(TEXT("Content")) == DockDellHud)
				{
					ZonaDelDock = Zona;
				}
			});
			TestTrue(*FString::Printf(TEXT("%s: il dock, e con lui l'indicatore, sta nella zona Bottom"), Scenario),
				ZonaDelDock != nullptr && ZonaDelDock->ZoneId == ERTHudZone::Bottom);

			int32 Min = 0;
			int32 Max = 0;
			const bool bPiuPiani = Comandante && Comandante->GetMapLayerRange(Min, Max) && Max > Min;
			const bool bMostrato = DockDellHud->LayerIndicator->GetVisibility() != ESlateVisibility::Collapsed;
			TestEqual(*FString::Printf(TEXT("%s: l'indicatore compare se e solo se la mappa ha piu' piani (%d..%d)"),
				Scenario, Min, Max), bMostrato, bPiuPiani);
			if (FCString::Strcmp(Scenario, TEXT("Visual.Map.LayerStack")) == 0)
			{
				TestTrue(TEXT("LayerStack: tre piani, l'indicatore compare e ha una misura"),
					bMostrato && DockDellHud->LayerIndicator->GetDesiredSize().X > 1.0);
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
