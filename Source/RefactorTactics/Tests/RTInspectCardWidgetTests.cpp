// Il pannello della scheda dell'ispezione (`#3748`): cosa mostra, quando si nasconde, come scorre, cosa consuma.
//
// Il modello (`URTInspectLibrary::ShouldShowCard`, `VisibleWindow`) e' provato in `RTInspectTests.cpp`. Qui si prova il WIDGET che lo
// usa, senza il WBP: il pannello riceve a mano i pezzi che il WBP gli dara' (`CardRoot`, `LinesBox`, `LineClass`), e la scheda composta
// si inietta (`SetViewForTest`), perche' comporla chiede un manager e un nemico `Live`. Che la composizione sia giusta lo prova l'ultimo
// test, su uno scenario vero.
//
// ⚠️ Cio' che resta di PIE e non si misura qui: che Slate consegni al pannello la rotella e il tasto sinistro prima del gioco, e che il
// pannello stia nella fascia (quello e' `ScreenHud.TenantsFitTheirBands`, che chiede l'asset).

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Tests/RTWorldFixtures.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestResult.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "EngineUtils.h"
#include "Input/Events.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/RTTeamKnowledge.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTInspect.h"
#include "UI/RTInspectCardWidget.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	const TCHAR* CardTestEllipsis() { return TEXT("…"); }

	ARTUnit* CardTestSpawnUnit(UWorld* World, int32 TeamId, int32 HeroIndex, const FRTCellId& Cell)
	{
		const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
		if (!Roster.IsValidIndex(HeroIndex)) { return nullptr; }
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit) { return nullptr; }
		Unit->TeamId = TeamId;
		Unit->ConfigureFromHeroData(Roster[HeroIndex]);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->bIsBotControlled = false;
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, 250.f);
		return Unit;
	}

	/** Una scheda di `NumLines` righe, ognuna col proprio numero («riga 07»). */
	FRTInspectView CardTestView(int32 NumLines)
	{
		FRTInspectView View;
		for (int32 I = 0; I < NumLines; ++I)
		{
			View.Lines.Add(FString::Printf(TEXT("riga %02d"), I));
		}
		return View;
	}

	int32 CardTestDigits(const FString& Text)
	{
		FString Digits;
		for (const TCHAR C : Text) { if (FChar::IsDigit(C)) { Digits.AppendChar(C); } }
		return Digits.IsEmpty() ? -1 : FCString::Atoi(*Digits);
	}

	FPointerEvent CardTestMouse(const FKey& Button, float Wheel = 0.f)
	{
		return FPointerEvent(0, FVector2D::ZeroVector, FVector2D::ZeroVector, TSet<FKey>(), Button, Wheel, FModifierKeysState());
	}

	struct FCardTestRig
	{
		UWorld* World = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Enemy = nullptr;
		URTInspectCardWidget* Card = nullptr;
		UBorder* Root = nullptr;
		UVerticalBox* Box = nullptr;
	};

	/**
	 * Un pannello con i pezzi che il WBP gli dara', un controller con la scheda fissata su un nemico, e una scheda di `NumLines` righe.
	 * Il mondo lo distrugge il chiamante.
	 */
	bool CardTestMakeRig(FAutomationTestBase& Test, FCardTestRig& Rig, int32 NumLines, int32 MaxRighe)
	{
		Rig.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("il mondo di prova esiste"), Rig.World)) { return false; }
		Rig.PC = RTWorldFixtures::MakePlayerOnTeam(Rig.World, 0);
		FRTCellId Cell; Cell.X = 1; Cell.Y = 0;
		Rig.Enemy = CardTestSpawnUnit(Rig.World, 1, 1, Cell);
		Rig.Card = CreateWidget<URTInspectCardWidget>(Rig.World, URTInspectCardWidget::StaticClass());
		if (!Test.TestTrue(TEXT("controller, nemico e pannello esistono"), Rig.PC && Rig.Enemy && Rig.Card)) { return false; }

		Rig.Root = NewObject<UBorder>(Rig.Card);
		Rig.Box = NewObject<UVerticalBox>(Rig.Card);
		Rig.Card->CardRoot = Rig.Root;
		Rig.Card->LinesBox = Rig.Box;
		Rig.Card->LineClass = URTInspectLineWidget::StaticClass();
		Rig.Card->MaxRighe = MaxRighe;
		Rig.Card->SetCommandControllerForTest(Rig.PC);
		Rig.Card->SetInspectedUnitForTest(Rig.Enemy);
		Rig.Card->SetViewForTest(CardTestView(NumLines));
		Rig.Card->SetContextForTest(ERTPointerContext::Planning);
		Rig.PC->PinInspectionForTest(Rig.Enemy);
		return true;
	}
}

/**
 * Una finestra di reazione o un modale nascondono la scheda SENZA chiuderla (`#3748`, spec §3 regola 11): lo scorrimento resta, e la
 * scheda torna come l'avevamo lasciata. Chiusa davvero, invece, non si porta dietro lo scorrimento di prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardHidesWithoutClosingTest,
	"RefactorTactics.ScreenHud.InspectCardHidesWithoutClosing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardHidesWithoutClosingTest::RunTest(const FString&)
{
	FCardTestRig Rig;
	if (!CardTestMakeRig(*this, Rig, 40, 8)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(Rig.World); };

	Rig.Card->RefreshLook(true);
	TestTrue(TEXT("fissata, in Planning la scheda si vede"), Rig.Card->IsCardShown());
	TestTrue(TEXT("e la radice e' visibile"), Rig.Root->GetVisibility() == ESlateVisibility::Visible);
	TestEqual(TEXT("con le righe della finestra"), Rig.Card->GetLineWidgets().Num(), 8);

	Rig.Card->ScrollBy(3);
	TestEqual(TEXT("premessa: ha scorso di tre righe"), Rig.Card->GetFirstLine(), 3);

	for (const ERTPointerContext Dialog : { ERTPointerContext::ReactionWindow, ERTPointerContext::Modal })
	{
		const FString Where = Dialog == ERTPointerContext::Modal ? TEXT("Modal") : TEXT("ReactionWindow");
		Rig.Card->SetContextForTest(Dialog);
		Rig.Card->RefreshLook(false);
		TestFalse(*FString::Printf(TEXT("%s: la scheda e' nascosta"), *Where), Rig.Card->IsCardShown());
		TestTrue(*FString::Printf(TEXT("%s: la radice e' Collapsed"), *Where), Rig.Root->GetVisibility() == ESlateVisibility::Collapsed);
		TestEqual(*FString::Printf(TEXT("%s: lo scorrimento resta"), *Where), Rig.Card->GetFirstLine(), 3);
		TestTrue(*FString::Printf(TEXT("%s: la fissazione resta, non e' stata chiusa"), *Where), Rig.PC->IsInspectorPinned());
	}

	Rig.Card->SetContextForTest(ERTPointerContext::Planning);
	Rig.Card->RefreshLook(false);
	TestTrue(TEXT("chiusa la finestra, la scheda torna"), Rig.Card->IsCardShown());
	TestEqual(TEXT("allo stesso punto di scorrimento"), Rig.Card->GetFirstLine(), 3);

	// Chiusa davvero (il tasto destro azzera pin e soggetto): lo scorrimento non sopravvive, e la stessa unita' rifissata riparte dall'inizio.
	Rig.PC->PinInspectionForTest(nullptr);
	Rig.Card->SetInspectedUnitForTest(nullptr);
	Rig.Card->RefreshLook(false);
	TestFalse(TEXT("senza fissazione non c'e' scheda"), Rig.Card->IsCardShown());
	TestTrue(TEXT("e la radice e' Collapsed"), Rig.Root->GetVisibility() == ESlateVisibility::Collapsed);

	Rig.PC->PinInspectionForTest(Rig.Enemy);
	Rig.Card->SetInspectedUnitForTest(Rig.Enemy);
	Rig.Card->RefreshLook(false);
	TestTrue(TEXT("rifissata, la scheda torna"), Rig.Card->IsCardShown());
	TestEqual(TEXT("ma dalla prima riga, non da dove era rimasta"), Rig.Card->GetFirstLine(), 0);

	// Fissarne un'altra la sostituisce (spec §3 regola 4), e la scheda nuova parte dall'inizio.
	Rig.Card->ScrollBy(5);
	TestEqual(TEXT("premessa: ha scorso di cinque righe"), Rig.Card->GetFirstLine(), 5);
	FRTCellId OtherCell; OtherCell.X = 2; OtherCell.Y = 0;
	ARTUnit* Other = CardTestSpawnUnit(Rig.World, 1, 2, OtherCell);
	if (TestNotNull(TEXT("un'altra unita' esiste"), Other))
	{
		Rig.PC->PinInspectionForTest(Other);
		Rig.Card->SetInspectedUnitForTest(Other);
		Rig.Card->RefreshLook(false);
		TestEqual(TEXT("fissata un'altra unita', la scheda riparte dall'inizio"), Rig.Card->GetFirstLine(), 0);
	}
	return true;
}

/**
 * Un taglio della scheda non e' mai muto, a schermo (`#3748`, spec §7): l'ultima riga dice quante ne restano, scorrendo la prima dice
 * quante ne restano sopra, e se entra tutto non c'e' nessun marcatore. Il modello e' provato a parte; qui si prova che il pannello lo
 * disegna davvero e non una versione sua.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardNeverTruncatesSilentlyTest,
	"RefactorTactics.ScreenHud.InspectCardNeverTruncatesSilently",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardNeverTruncatesSilentlyTest::RunTest(const FString&)
{
	constexpr int32 Total = 40;
	constexpr int32 Max = 8;
	FCardTestRig Rig;
	if (!CardTestMakeRig(*this, Rig, Total, Max)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(Rig.World); };

	Rig.Card->RefreshLook(true);
	const TArray<TObjectPtr<URTInspectLineWidget>>& Rows = Rig.Card->GetLineWidgets();
	if (!TestEqual(TEXT("la scheda riempie le righe che ha, non una di piu'"), Rows.Num(), Max)) { return false; }
	TestEqual(TEXT("anche nel pannello: tante righe quanti figli"), Rig.Box->GetChildrenCount(), Max);
	TestEqual(TEXT("in cima la prima riga vera"), Rows[0]->GetLineString(), FString(TEXT("riga 00")));
	const FString Bottom = Rows.Last()->GetLineString();
	TestTrue(*FString::Printf(TEXT("l'ultima riga e' un marcatore (e' `%s`)"), *Bottom), Bottom.StartsWith(CardTestEllipsis()));
	// Il corpo e' Max - 1 righe (il marcatore in basso ne occupa una): ne restano Total - (Max - 1), e il numero lo dice.
	TestEqual(TEXT("e dice quante ne restano"), CardTestDigits(Bottom), Total - (Max - 1));

	Rig.Card->ScrollBy(1000);
	if (!TestEqual(TEXT("scorrendo a fondo corsa la finestra e' sempre piena"), Rows.Num(), Max)) { return false; }
	const FString Top = Rows[0]->GetLineString();
	TestTrue(*FString::Printf(TEXT("la prima riga e' un marcatore (e' `%s`)"), *Top), Top.StartsWith(CardTestEllipsis()));
	TestEqual(TEXT("e dice quante ne restano sopra"), CardTestDigits(Top), Total - (Max - 1));
	TestEqual(TEXT("a fondo corsa l'ultima riga e' l'ultima vera, non un marcatore"), Rows.Last()->GetLineString(), FString(TEXT("riga 39")));

	// Che entri tutto: nessun marcatore.
	Rig.Card->MaxRighe = 50;
	Rig.Card->ScrollBy(-1000);
	TestEqual(TEXT("se entra tutto, ci sono tutte le righe"), Rows.Num(), Total);
	TestEqual(TEXT("e la prima e' la prima"), Rows[0]->GetLineString(), FString(TEXT("riga 00")));
	TestEqual(TEXT("e l'ultima e' l'ultima"), Rows.Last()->GetLineString(), FString(TEXT("riga 39")));
	return true;
}

/**
 * Il pannello consuma il tasto sinistro e la rotella, e NON il destro (`#3748`, spec §3 regola 6): il click sul pannello non raggiunge
 * la mappa, la rotella scorre la scheda, e il tasto destro resta del controller, che e' quello che chiude la scheda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardConsumesClicksAndWheelTest,
	"RefactorTactics.PlayerInput.InspectCardPanelConsumesClicksAndWheel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardConsumesClicksAndWheelTest::RunTest(const FString&)
{
	FCardTestRig Rig;
	if (!CardTestMakeRig(*this, Rig, 40, 8)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(Rig.World); };
	Rig.Card->RefreshLook(true);

	const FGeometry Geometry;
	TestTrue(TEXT("tasto sinistro premuto: consumato"),
		Rig.Card->NativeOnMouseButtonDown(Geometry, CardTestMouse(EKeys::LeftMouseButton)).IsEventHandled());
	TestTrue(TEXT("tasto sinistro rilasciato: consumato (un rilascio orfano arriverebbe al gioco)"),
		Rig.Card->NativeOnMouseButtonUp(Geometry, CardTestMouse(EKeys::LeftMouseButton)).IsEventHandled());
	TestTrue(TEXT("doppio click sinistro: consumato"),
		Rig.Card->NativeOnMouseButtonDoubleClick(Geometry, CardTestMouse(EKeys::LeftMouseButton)).IsEventHandled());
	TestFalse(TEXT("tasto destro premuto: NON consumato, e' il Back"),
		Rig.Card->NativeOnMouseButtonDown(Geometry, CardTestMouse(EKeys::RightMouseButton)).IsEventHandled());
	TestFalse(TEXT("tasto destro rilasciato: NON consumato"),
		Rig.Card->NativeOnMouseButtonUp(Geometry, CardTestMouse(EKeys::RightMouseButton)).IsEventHandled());
	TestFalse(TEXT("tasto centrale: NON consumato, e' il pan"),
		Rig.Card->NativeOnMouseButtonDown(Geometry, CardTestMouse(EKeys::MiddleMouseButton)).IsEventHandled());

	// La rotella scorre la scheda, di `LinesPerWheelStep` righe per tacca: verso il basso (delta negativo) in avanti, verso l'alto indietro.
	const int32 Step = Rig.Card->LinesPerWheelStep;
	TestTrue(TEXT("rotella verso il basso: consumata"),
		Rig.Card->NativeOnMouseWheel(Geometry, CardTestMouse(FKey(), -1.f)).IsEventHandled());
	TestEqual(TEXT("e scorre in avanti di una tacca"), Rig.Card->GetFirstLine(), Step);
	TestTrue(TEXT("rotella verso l'alto: consumata"),
		Rig.Card->NativeOnMouseWheel(Geometry, CardTestMouse(FKey(), 1.f)).IsEventHandled());
	TestEqual(TEXT("e torna indietro di una tacca"), Rig.Card->GetFirstLine(), 0);
	Rig.Card->NativeOnMouseWheel(Geometry, CardTestMouse(FKey(), 1.f));
	TestEqual(TEXT("in cima non va sotto zero"), Rig.Card->GetFirstLine(), 0);

	// Una rotella a delta frazionari (un touchpad, una rotella ad alta risoluzione) e' proporzionale come lo zoom del mondo: sotto la
	// tacca intera non scorre, e la frazione resta per l'evento dopo. Contare il solo segno farebbe scorrere una tacca intera per evento.
	Rig.Card->NativeOnMouseWheel(Geometry, CardTestMouse(FKey(), -0.4f));
	Rig.Card->NativeOnMouseWheel(Geometry, CardTestMouse(FKey(), -0.4f));
	TestEqual(TEXT("due eventi da 0,4 non fanno una tacca"), Rig.Card->GetFirstLine(), 0);
	Rig.Card->NativeOnMouseWheel(Geometry, CardTestMouse(FKey(), -0.4f));
	TestEqual(TEXT("il terzo la completa: una tacca, non tre"), Rig.Card->GetFirstLine(), Step);

	// Anche quando non c'e' niente da scorrere la rotella e' consumata: il gesto e' comunque sul pannello, e non deve zoomare il mondo.
	Rig.Card->MaxRighe = 50;
	Rig.Card->ScrollBy(0);
	TestTrue(TEXT("se entra tutto, la rotella e' consumata lo stesso"),
		Rig.Card->NativeOnMouseWheel(Geometry, CardTestMouse(FKey(), -1.f)).IsEventHandled());
	TestEqual(TEXT("e non scorre"), Rig.Card->GetFirstLine(), 0);

	// Le righe sono testo e basta (la scheda non ha piu' righe di skill, quindi nessun hover): ognuna scrive la riga della scheda che le
	// spetta, e nessuna vede il cursore, cosi' i gesti sopra una riga arrivano al pannello che li consuma. Con tutto in finestra il conto e'
	// quello della scheda, riga per riga.
	const TArray<TObjectPtr<URTInspectLineWidget>>& Rows = Rig.Card->GetLineWidgets();
	const FRTInspectView Whole = CardTestView(40);
	if (TestEqual(TEXT("con tutto in finestra una riga di pannello per riga di scheda"), Rows.Num(), Whole.Lines.Num()))
	{
		for (int32 I = 0; I < Rows.Num(); ++I)
		{
			TestEqual(*FString::Printf(TEXT("riga %d: il testo"), I), Rows[I]->GetLineString(), Whole.Lines[I]);
			TestTrue(*FString::Printf(TEXT("riga %d: non vede il cursore, i gesti passano al pannello"), I),
				Rows[I]->GetVisibility() == ESlateVisibility::SelfHitTestInvisible);
		}
	}
	return true;
}

/**
 * Il cancello della scheda si chiude sulla squadra del CONTROLLER, non su una cache del widget (`#3748`), e il raccordo widget ->
 * `ARTHUD::BuildInspectCard` e' provato sul percorso vero: nessuna scheda iniettata, manager e unita' di uno scenario.
 *
 * Due schede nello stesso mondo, sulla stessa unita' (di squadra 0), con la cache del widget a 0 in entrambe: quella del controller di
 * squadra 0 la vede, quella del controller di un'altra squadra no, perche' per lei e' un'unita' che non conosce. Una scheda che
 * leggesse la cache avrebbe mostrato la scheda a entrambe: e' il difetto di un client che acquisisce il contesto prima che il
 * PlayerState abbia la squadra.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardGatesOnTheControllersTeamTest,
	"RefactorTactics.ScreenHud.InspectCardGatesOnTheControllersTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardGatesOnTheControllersTeamTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	FString ReportDir;
	const FRTTestResult Result = URTScenarioRunner::RunById(World, TEXT("Spec.Hud.MatchWithTwoAllies"), ReportDir);
	if (!TestEqual(TEXT("lo scenario gira"), static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass))) { return false; }

	ARTTurnManager* TM = nullptr;
	for (TActorIterator<ARTTurnManager> It(World); It; ++It) { TM = *It; break; }
	ARTUnit* Unit = RTWorldFixtures::FirstUnitOfTeam(World, 0);
	// Il rivale e' di una squadra che lo scenario non conosce: senza unita' ne' fotografia, il suo ripiego e' una conoscenza vuota col
	// proprio `TeamId`. ⚠️ Non la squadra 1: lo scenario ha unita' sue, e `PlaybackKnowledgeForTeam` legge prima lo stato di playback
	// seminato dalla run, quindi la squadra 1 puo' vedere davvero l'unita' di squadra 0 e una conoscenza «cieca» impostata a mano non conta.
	constexpr int32 RivalTeam = 2;
	ARTPlayerController* Own = RTWorldFixtures::MakePlayerOnTeam(World, 0);
	ARTPlayerController* Rival = RTWorldFixtures::MakePlayerOnTeam(World, RivalTeam);
	if (!TestTrue(TEXT("manager, unita' di squadra 0 e due controllori esistono"), TM && Unit && Own && Rival)) { return false; }

	// La premessa si MISURA: dalla vista della squadra 0 l'unita' e' `Live`, da quella del rivale non c'e' nessuna voce `Live`.
	TArray<ARTUnit*> Units;
	for (TActorIterator<ARTUnit> It(World); It; ++It) { Units.Add(*It); }
	if (!TestTrue(TEXT("premessa: la squadra 0 vede la sua unita', il rivale no"),
		URTInspectLibrary::IsInspectable(URTHudViewModel::BuildObserverKnowledgeView(TM, Units, 0), Unit->StableUnitId)
		&& !URTInspectLibrary::IsInspectable(URTHudViewModel::BuildObserverKnowledgeView(TM, Units, RivalTeam), Unit->StableUnitId)))
	{
		return false;
	}

	const auto MakeCard = [&](ARTPlayerController* PC) -> URTInspectCardWidget*
	{
		URTInspectCardWidget* Card = CreateWidget<URTInspectCardWidget>(World, URTInspectCardWidget::StaticClass());
		if (Card == nullptr) { return nullptr; }
		Card->CardRoot = NewObject<UBorder>(Card);
		Card->LinesBox = NewObject<UVerticalBox>(Card);
		Card->LineClass = URTInspectLineWidget::StaticClass();
		Card->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0); // la cache del widget dice 0, per entrambe
		Card->SetCommandControllerForTest(PC);
		Card->SetInspectedUnitForTest(Unit);
		Card->SetContextForTest(ERTPointerContext::Planning);
		PC->PinInspectionForTest(Unit);
		return Card;
	};
	URTInspectCardWidget* OwnCard = MakeCard(Own);
	URTInspectCardWidget* RivalCard = MakeCard(Rival);
	if (!TestTrue(TEXT("le due schede esistono"), OwnCard && RivalCard)) { return false; }

	OwnCard->RefreshLook(true);
	TestTrue(TEXT("il controller di squadra 0 vede la scheda della sua unita' (percorso vero, nessuna scheda iniettata)"), OwnCard->IsCardShown());
	TestTrue(TEXT("e le sue righe sono costruite"), OwnCard->GetLineWidgets().Num() > 0);

	RivalCard->RefreshLook(true);
	TestFalse(TEXT("il controller del rivale non conosce quell'unita': nessuna scheda, anche con la cache del widget a 0"), RivalCard->IsCardShown());
	TestEqual(TEXT("e nessuna riga costruita"), RivalCard->GetLineWidgets().Num(), 0);
	return true;
}

/**
 * La scheda arriva composta dal percorso vero (`ARTHUD::BuildInspectCard`): il cancello di conoscenza, il soggetto pubblico, il verso che
 * segue la targa. Su uno scenario con un manager vero, per ogni unita' la scheda c'e' se e solo se la vista del velo la dichiara `Live`.
 *
 * ⚠️ Lo scenario dice quali nemici sono `Live`: se nessuno lo e', il ramo «velata» di questo test non gira, e il test lo scrive nel
 * referto. Quel ramo e' provato a parte, sulla vista costruita a mano (`InspectUnobservedEnemyReadsAsAnEmptyCell`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardComposedFromTheObserverViewTest,
	"RefactorTactics.ScreenHud.InspectCardIsComposedFromTheObserverView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardComposedFromTheObserverViewTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	FString ReportDir;
	const FRTTestResult Result = URTScenarioRunner::RunById(World, TEXT("Spec.Hud.MatchWithTwoAllies"), ReportDir);
	if (!TestEqual(TEXT("lo scenario gira"), static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass)))
	{
		AddError(FString::Printf(TEXT("esito scenario: %s"), *Result.ErrorMessage));
		return false;
	}

	ARTTurnManager* TM = nullptr;
	for (TActorIterator<ARTTurnManager> It(World); It; ++It) { TM = *It; break; }
	TArray<ARTUnit*> Units;
	for (TActorIterator<ARTUnit> It(World); It; ++It) { Units.Add(*It); }
	if (!TestTrue(TEXT("lo scenario lascia un manager e delle unita'"), TM != nullptr && Units.Num() > 0)) { return false; }

	constexpr int32 ObserverTeam = 0;
	const FRTKnowledgeView View = URTHudViewModel::BuildObserverKnowledgeView(TM, Units, ObserverTeam);
	int32 Composed = 0;
	int32 Veiled = 0;
	for (const ARTUnit* Unit : Units)
	{
		const FRTInspectView Card = ARTHUD::BuildInspectCard(TM, Unit, Units, ObserverTeam);
		const bool bLive = URTInspectLibrary::IsInspectable(View, Unit->StableUnitId);
		TestEqual(*FString::Printf(TEXT("%s: la scheda c'e' se e solo se la vista la dichiara Live"), *Unit->GetName()),
			Card.Lines.Num() > 0, bLive);
		(bLive ? Composed : Veiled) += 1;

		// Il verso segue la targa: acceso fuori dalla risoluzione, spento in risoluzione (P12).
		if (bLive)
		{
			bool bFacingLine = false;
			for (const FString& Line : Card.Lines) { bFacingLine |= Line.StartsWith(TEXT("Guarda verso")); }
			TestEqual(*FString::Printf(TEXT("%s: la riga del verso c'e' se e solo se non si sta risolvendo"), *Unit->GetName()),
				bFacingLine, !TM->IsResolving());
		}
	}
	AddInfo(FString::Printf(TEXT("unita' con scheda composta: %d, unita' velate: %d"), Composed, Veiled));
	TestTrue(TEXT("anti-vacuita': almeno una scheda composta dal percorso vero"), Composed > 0);

	// Lo scenario non vela nessuno, quindi il cancello chiuso si prova con un'unita' che la vista non conosce: nemica, spawnata dopo e
	// fuori dall'elenco da cui si costruisce la vista, quindi senza voce. Il cancello e' «una voce Live», e qui non c'e' nessuna voce.
	FRTCellId UnknownCell; UnknownCell.X = 3; UnknownCell.Y = 3;
	ARTUnit* Unknown = CardTestSpawnUnit(World, 1, 1, UnknownCell);
	if (TestNotNull(TEXT("l'unita' ignota alla vista esiste"), Unknown))
	{
		// ⚠️ `StableUnitId` nasce a 0 e lo scenario ne assegna dallo 0: un'unita' spawnata a mano senza un id suo prende quello di un'altra,
		// che ha una voce Live, e il cancello (che chiude per id) le darebbe la scheda di un'altra. Un id che nessuna voce possiede.
		int32 FreshId = 1000;
		for (const ARTUnit* Unit : Units) { FreshId = FMath::Max(FreshId, Unit->StableUnitId + 1000); }
		Unknown->StableUnitId = FreshId;
		TestTrue(TEXT("premessa: la vista non ha nessuna voce per quell'id"), URTKnowledgeViewLibrary::FindEntry(View, FreshId) == nullptr);
		TestEqual(TEXT("un'unita' senza voce nella vista non ha scheda"),
			ARTHUD::BuildInspectCard(TM, Unknown, Units, ObserverTeam).Lines.Num(), 0);
	}

	// Senza manager o senza unita' la scheda e' vuota, non un crash.
	TestEqual(TEXT("senza manager: vuota"), ARTHUD::BuildInspectCard(nullptr, Units[0], Units, ObserverTeam).Lines.Num(), 0);
	TestEqual(TEXT("senza unita': vuota"), ARTHUD::BuildInspectCard(TM, nullptr, Units, ObserverTeam).Lines.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
