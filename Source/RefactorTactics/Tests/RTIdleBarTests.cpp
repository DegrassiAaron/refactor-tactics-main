// La barra dei comandi SENZA un'unita' comandata ([D-460], #3494): la struttura invece del vuoto.
//
// 🔑 **L'oracolo delle Comuni e del Kit e' la barra di un'unita' VERA**, costruita dallo stesso ViewModel: la
// struttura deve riprodurne la forma — quante Comuni, quale Base, quanti vuoti — senza portarne nessuno stato.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
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
	ARTUnit* SpawnIdleBarUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Quante posizioni del kit di un'unita' si leggono col Kit (una vuota compresa), dalla sua barra vera. */
	int32 IdleBarKitLength(const ARTUnit* Unit)
	{
		int32 N = 0;
		for (const FRTAbilityCooldownView& R : URTHudViewModel::BuildAbilityCooldowns(Unit))
		{
			N += (R.Group == ERTActionGroup::Kit || R.Group == ERTActionGroup::None) ? 1 : 0;
		}
		return N;
	}

	/** Le Comuni di un'unita', nell'ordine di lettura. */
	TArray<FName> IdleBarCommons(const ARTUnit* Unit)
	{
		TArray<FName> Out;
		for (const FRTAbilityCooldownView& R :
			URTHudViewModel::OrderForReading(URTHudViewModel::BuildAbilityCooldowns(Unit)))
		{
			if (R.Group == ERTActionGroup::Common) { Out.Add(R.ActionId); }
		}
		return Out;
	}

	/** Lo stato che il grafo della dock chiederebbe: armato se l'indice e' quello armato, che senza unita' e' `INDEX_NONE`. */
	ERTActionSlotState IdleBarStateWithoutUnit(const FRTAbilityCooldownView& V)
	{
		return URTHudViewModel::ResolveSlotState(V, /*bArmed=*/ V.AbilityIndex == INDEX_NONE);
	}
}

/**
 * 🔴 **LA STRUTTURA: COMUNI SPENTE, UNA BASE VUOTA, TANTI VUOTI QUANTO IL KIT PIU' LUNGO** ([D-460]).
 *
 * ⚠️ **Il controllo sugli armati e' la ragione di `IdleSlotIndex`**: il grafo accende lo slot il cui indice e'
 * uguale a quello armato, che senza unita' vale `INDEX_NONE`. Una struttura con quell'indice sarebbe tutta
 * accesa come `Selected`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIdleBarShapeTest,
	"RefactorTactics.HudViewModel.IdleBarShowsTheCommonsOffAndTheKitEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIdleBarShapeTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Aevik = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTUnit* Muiren = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(1, 0, 0));
	if (!Aevik || !Muiren)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("le due unita' esistono"), false);
	}

	const TArray<FName> Comuni = IdleBarCommons(Aevik); // la prima dell'elenco e' la fonte delle Comuni
	const int32 Kit = FMath::Max(IdleBarKitLength(Aevik), IdleBarKitLength(Muiren));
	if (!TestTrue(TEXT("premessa: ci sono Comuni"), Comuni.Num() > 0)
		|| !TestTrue(TEXT("premessa: c'e' un Kit"), Kit > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const TArray<FRTAbilityCooldownView> Barra = URTHudViewModel::BuildIdleBar({ Aevik, Muiren });
	if (!TestEqual(TEXT("Comuni + una Base + i vuoti del kit piu' lungo"), Barra.Num(), Comuni.Num() + 1 + Kit))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	for (int32 i = 0; i < Barra.Num(); ++i)
	{
		const FRTAbilityCooldownView& V = Barra[i];
		TestEqual(*FString::Printf(TEXT("voce %d: nessun indice di kit"), i), V.AbilityIndex, URTHudViewModel::IdleSlotIndex);
		TestFalse(*FString::Printf(TEXT("voce %d: niente di pianificato"), i), V.bPlanned);
		if (i < Comuni.Num())
		{
			TestEqual(*FString::Printf(TEXT("Comune %d: e' l'azione vera"), i), V.ActionId, Comuni[i]);
			TestEqual(*FString::Printf(TEXT("Comune %d: e' del gruppo Comuni"), i), V.Group, ERTActionGroup::Common);
			TestEqual(*FString::Printf(TEXT("Comune %d: spenta, cioe' Unavailable — non armata"), i),
				IdleBarStateWithoutUnit(V), ERTActionSlotState::Unavailable);
		}
		else
		{
			const bool bBase = i == Comuni.Num();
			TestEqual(*FString::Printf(TEXT("voce %d: il gruppo"), i), V.Group,
				bBase ? ERTActionGroup::Base : ERTActionGroup::Kit);
			TestEqual(*FString::Printf(TEXT("voce %d: vuota — Empty, non armata"), i),
				IdleBarStateWithoutUnit(V), ERTActionSlotState::Empty);
		}
	}
	TestTrue(TEXT("la Base apre un gruppo"), Barra[Comuni.Num()].bGroupBreakBefore);
	TestTrue(TEXT("il Kit apre un gruppo"), Barra[Comuni.Num() + 1].bGroupBreakBefore);

	TestEqual(TEXT("senza unita' nella squadra la barra resta vuota"),
		URTHudViewModel::BuildIdleBar({}).Num(), 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **LA STRUTTURA NON PORTA NIENTE DEL PIANO DELL'UNITA' DA CUI COPIA LE COMUNI** ([D-460]).
 *
 * 🔑 **E' il controllo dell'elenco positivo dei campi**: l'unita' fonte ha una Comune pianificata e un'altra in
 * ricarica, e il piano e' illegale. Una copia per intero della riga — invece che campo per campo — porterebbe
 * nella barra senza unita' l'impegno, il numero di turni e lo stato `Invalid` di un'unita' che nessuno comanda.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIdleBarCarriesNoPlanTest,
	"RefactorTactics.HudViewModel.IdleBarCarriesNothingOfThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIdleBarCarriesNoPlanTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ARTUnit* Fonte = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("l'unita' fonte esiste"), Fonte)) { RTWorldFixtures::DestroyWorld(World); return false; }

	// Una Comune con ricarica: la si pianifica e la si consuma, cosi' il piano e' anche illegale (OnCooldown).
	int32 Comune = INDEX_NONE;
	const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(Fonte);
	for (const FRTAbilityCooldownView& R : Righe)
	{
		const URTActionData* A = Fonte->GetAbility(R.AbilityIndex);
		if (R.Group == ERTActionGroup::Common && A && A->CooldownTurns > 0)
		{
			Comune = R.AbilityIndex;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: una Comune ha una ricarica"), Comune != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	Fonte->PlannedAbilityIndex = Comune;
	Fonte->ConsumeAbility(Comune);
	const TArray<FRTAbilityCooldownView> Prima = URTHudViewModel::BuildAbilityCooldowns(Fonte);
	if (!TestTrue(TEXT("premessa: nella barra dell'unita' la Comune e' pianificata, in ricarica e colpevole"),
		Prima[Comune].bPlanned && Prima[Comune].TurnsRemaining > 0 && Prima[Comune].bPlanInvalid))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	for (const FRTAbilityCooldownView& V : URTHudViewModel::BuildIdleBar({ Fonte }))
	{
		const FString Nome = V.ActionId.IsNone() ? TEXT("(vuota)") : V.ActionId.ToString();
		TestFalse(*FString::Printf(TEXT("%s: non pianificata"), *Nome), V.bPlanned);
		TestEqual(*FString::Printf(TEXT("%s: nessuna ricarica"), *Nome), V.TurnsRemaining, 0);
		TestFalse(*FString::Printf(TEXT("%s: nessuno stato di D-459"), *Nome),
			V.bPlanInvalid || V.bPlanDegraded || V.bTargetRefused);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **LA DOCK CONSEGNA LA STRUTTURA SOLO SENZA UN'UNITA' COMANDATA, E LA CONTA SULLA PROPRIA SQUADRA** ([D-460]).
 *
 * ⛔ **Il controllo B e' quello di privacy**: in campo c'e' un avversario con un kit PIU' LUNGO del proprio. Se
 * entrasse nel conteggio, il numero dei vuoti direbbe al giocatore quante azioni ha un'unita' nemica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIdleBarDockTest,
	"RefactorTactics.ScreenHud.DockShowsTheIdleBarOnlyWithoutACommandedUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIdleBarDockTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* A = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTUnit* B = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(1, 0, 0));
	URTActionDockWidget* Dock = NewObject<URTActionDockWidget>(World);
	if (!A || !B || !Dock)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e dock esistono"), false);
	}
	// La propria e' quella dal kit piu' corto; l'avversaria, quella dal piu' lungo.
	ARTUnit* Propria = IdleBarKitLength(A) <= IdleBarKitLength(B) ? A : B;
	ARTUnit* Avversaria = Propria == A ? B : A;
	Avversaria->TeamId = 1;
	if (!TestTrue(TEXT("premessa: il kit avversario e' piu' lungo del proprio"),
		IdleBarKitLength(Avversaria) > IdleBarKitLength(Propria)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	Dock->SetMatchContextForTest(nullptr, /*InPlayerTeamId=*/ 0);

	// --- A. senza unita': GetActions resta l'identita', la lettura consegna la struttura ------------------
	TestEqual(TEXT("A: GetActions senza unita' e' vuota"), Dock->GetActions().Num(), 0);
	const TArray<FRTAbilityCooldownView> Struttura = Dock->GetActionsInReadingOrder();
	TestTrue(TEXT("A: la lettura senza unita' non e' vuota"), Struttura.Num() > 0);

	// --- B. ⛔ i vuoti del Kit sono quelli della PROPRIA squadra ----------------------------------------------
	int32 VuotiKit = 0;
	for (const FRTAbilityCooldownView& V : Struttura)
	{
		VuotiKit += V.Group == ERTActionGroup::Kit ? 1 : 0;
		TestEqual(TEXT("A: ogni voce porta l'indice della struttura"), V.AbilityIndex, URTHudViewModel::IdleSlotIndex);
	}
	TestEqual(TEXT("B: i vuoti del Kit contano solo la propria squadra"), VuotiKit, IdleBarKitLength(Propria));

	// --- C. con un'unita' comandata, la barra e' la sua --------------------------------------------------------
	Dock->SetSelectedUnitForTest(Propria);
	const TArray<FRTAbilityCooldownView> Sua = Dock->GetActionsInReadingOrder();
	TestEqual(TEXT("C: con l'unita' la lettura ha le sue voci"), Sua.Num(), Dock->GetActions().Num());
	for (const FRTAbilityCooldownView& V : Sua)
	{
		TestTrue(TEXT("C: e ogni voce porta un indice di kit vero"), V.AbilityIndex >= 0);
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **UNO SLOT DELLA STRUTTURA NON ARMA NIENTE** ([D-460]).
 *
 * 🔑 Il controllore ha un'unita' selezionata, cosi' il click arriverebbe davvero al kit: e' il caso del frame in
 * cui la selezione arriva mentre la barra mostra ancora la struttura. Il controllo positivo — lo stesso click
 * con un indice vero arma — impedisce che il verde venga da una porta che non arma mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIdleBarSlotArmsNothingTest,
	"RefactorTactics.ScreenHud.IdleBarSlotArmsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIdleBarSlotArmsNothingTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ARTUnit* Unit = SpawnIdleBarUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>(World);
	if (!Unit || !PC || !Slot)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita', controller e slot esistono"), false);
	}
	PC->SelectActorForTest(Unit);
	Slot->SetArmingControllerForTest(PC);

	FRTAbilityCooldownView Idle;
	Idle.AbilityIndex = URTHudViewModel::IdleSlotIndex;
	Idle.Group = ERTActionGroup::Kit;
	Slot->SetAction(Idle, false);
	Slot->Activate();
	TestEqual(TEXT("il click su uno slot della struttura non arma niente"), Unit->SelectedAbilityIndex, (int32)INDEX_NONE);

	// Controllo positivo: lo stesso slot con un indice vero arma.
	FRTAbilityCooldownView Vera = URTHudViewModel::BuildAbilityCooldowns(Unit)[0];
	Slot->SetAction(Vera, false);
	Slot->Activate();
	TestEqual(TEXT("controllo: con un indice di kit lo stesso click arma"), Unit->SelectedAbilityIndex, Vera.AbilityIndex);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
