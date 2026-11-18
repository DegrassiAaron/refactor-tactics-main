// #4155 (HUD-AP.3) — il costo del gesto in corso nei pip degli Action Point, e la cella che il residuo non paga ([D-625] (2)).
//
// 🔑 **L'oracolo della spesa e' il compositore del validatore**, non un numero scritto qui: `ActionPointsAfterMoving` deve dire la spesa
// che `ActionPointsSpent(MakePlanFor(Unit))` dara' DOPO aver posato davvero il waypoint col click. Un costo del Move ricopiato nel test
// passerebbe anche se la voce di movimento cambiasse prezzo.
//
// ⚠️ **Ogni rifiuto ha il suo controllo positivo nello stesso piano**: un click che non posa niente passerebbe anche se il click non
// arrivasse mai al ramo del movimento.

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPointerInteraction.h" // ERTPointerContext
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTMovementProfileLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexLibrary.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/Actor/RTOverlayArea.h"
#include "Turn/RTTurnManager.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Actions/RTPlanValidationLibrary.h"
#include "Turn/Actions/RTActionFallbackLibrary.h"
#include "Turn/Log/RTTurnLogLibrary.h"
#include "UI/HUD/RTHudViewModel.h"
#include "UI/HUD/RTHUD.h"
#include "UI/HUD/RTScreenHudWidgets.h"
#include "UI/HUD/RTUIPalette.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Tests/Shared/RTAbilityFixtures.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Il banco: un esagono piano senza ostacoli, il manager del turno, un controller e Gadget al centro, selezionata. */
	struct FApPreviewBench
	{
		UWorld* World = nullptr;
		ARTHexMapActor* Map = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Unit = nullptr;
		int32 Attacco = INDEX_NONE;
		int32 Reazione = INDEX_NONE;
		int32 Scatto = INDEX_NONE;
		int32 Guardia = INDEX_NONE;

		bool IsValid() const
		{
			return World && Map && PC && Unit && Attacco != INDEX_NONE && Reazione != INDEX_NONE && Scatto != INDEX_NONE
				&& Guardia != INDEX_NONE;
		}

		int32 Spent() const
		{
			return URTPlanValidationLibrary::ActionPointsSpent(URTPlanValidationLibrary::MakePlanFor(Unit));
		}
	};

	FApPreviewBench MakeApPreviewBench()
	{
		FApPreviewBench B;
		B.World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (!B.World || !GEngine)
		{
			return B;
		}
		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(B.World);

		// Senza ostacoli: un muro farebbe rifiutare il waypoint dal pathfinding prima che la regola del budget abbia voce.
		B.Map = B.World->SpawnActor<ARTHexMapActor>();
		if (B.Map)
		{
			B.Map->MapAsset = URTMatchSetupLibrary::MakeFlatArena(B.World, /*Radius=*/ 12);
		}
		B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

		ARTUnit* U = B.World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (U)
		{
			U->TeamId = 0;
			U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeGadget());
			UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
			U->bIsBotControlled = false;
			U->DispatchBeginPlay();
			U->PlaceOnCell(FRTCellId(0, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		}
		B.Unit = U;
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		if (!B.Unit || !B.PC)
		{
			return B;
		}
		B.PC->SelectActorForTest(B.Unit);

		const FName Base(TEXT("Action.BasicAttack"));
		for (int32 i = 0; i < B.Unit->NumAbilities(); ++i)
		{
			const URTActionData* A = B.Unit->GetAbility(i);
			if (A && (A->Def.ActionId == Base || A->Def.BaseActionId == Base)) { B.Attacco = i; break; }
		}
		// Reazione, scatto e Guard nelle ultime posizioni del kit, lontano dall'attacco base, come in `UnaffordableActionDoesNotArm`.
		TArray<int32> Libere;
		for (int32 i = B.Unit->NumAbilities() - 1; i >= 0 && Libere.Num() < 3; --i)
		{
			if (i != B.Attacco) { Libere.Add(i); }
		}
		if (Libere.Num() == 3)
		{
			B.Reazione = RTAbilityFixtures::AddCoreAbilityInSlot(B.Unit, TEXT("Action.Counter"), Libere[0]);
			B.Scatto = RTAbilityFixtures::AddCoreAbilityInSlot(B.Unit, TEXT("Action.Dodge"), Libere[1]);
			B.Guardia = RTAbilityFixtures::AddCoreAbilityInSlot(B.Unit, TEXT("Action.Guard"), Libere[2]);
		}
		return B;
	}

	void DestroyApPreviewBench(FApPreviewBench& B)
	{
		if (B.World && GEngine)
		{
			GEngine->DestroyWorldContext(B.World);
			B.World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
		B = FApPreviewBench();
	}

	/** I pip uno a uno: `TestEqual` non confronta due `TArray` di enum. */
	void ApPipsEqual(FAutomationTestBase& Test, const TCHAR* Caso, const TArray<ERTActionPointPip>& Avuti, const TArray<ERTActionPointPip>& Attesi)
	{
		if (!Test.TestEqual(*FString::Printf(TEXT("%s: quanti pip"), Caso), Avuti.Num(), Attesi.Num()))
		{
			return;
		}
		for (int32 i = 0; i < Attesi.Num(); ++i)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: pip %d"), Caso, i), static_cast<int32>(Avuti[i]), static_cast<int32>(Attesi[i]));
		}
	}

	/** La voce di movimento del piano composto, o `nullptr`. */
	const FRTPlannedAction* ApPreviewMoveEntry(const TArray<FRTPlannedAction>& Plan)
	{
		return Plan.FindByPredicate([](const FRTPlannedAction& E) { return !E.Def.MovementProfileId.IsNone(); });
	}
}

/**
 * #4155, criterio 1 — **la spesa dopo un waypoint e' quella che il piano avra' DOPO il click, in ogni banda di [D-425].**
 *
 * Il piano vuoto paga la voce Move; un percorso gia' pianificato si allunga a costo 0 (la lettura (f) di [D-625] sul tratto pagato). La
 * cella della banda Sprint e' il caso che una costante «2» ricopiata qui non distinguerebbe da una lettura: la banda la sceglie la distanza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovePreviewIsTheComposedPlanSpendTest,
	"RefactorTactics.HudViewModel.MovePreviewIsTheComposedPlanSpend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovePreviewIsTheComposedPlanSpendTest::RunTest(const FString&)
{
	for (const bool bSprint : { false, true })
	{
		FApPreviewBench B = MakeApPreviewBench();
		if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		const TCHAR* Banda = bSprint ? TEXT("Sprint") : TEXT("Move");
		const int32 Raggio = B.Unit->GetEffectiveMoveRange();
		const FRTCellId Meta(bSprint ? Raggio + 1 : 1, 0, 0);
		TestEqual(*FString::Printf(TEXT("premessa (%s): la cella e' a %d passi"), Banda, bSprint ? Raggio + 1 : 1),
			URTHexLibrary::HexDistance(B.Unit->Cell, Meta), bSprint ? Raggio + 1 : 1);

		TestEqual(*FString::Printf(TEXT("premessa (%s): il piano vuoto non spende"), Banda), B.Spent(), 0);
		const int32 Prevista = B.Unit->ActionPointsAfterMoving();

		B.PC->HandleClickOnCellForTest(Meta);
		const TArray<FRTPlannedAction> Piano = URTPlanValidationLibrary::MakePlanFor(B.Unit);
		const FRTPlannedAction* Voce = ApPreviewMoveEntry(Piano);
		if (TestNotNull(*FString::Printf(TEXT("premessa (%s): il click ha pianificato il movimento"), Banda), Voce))
		{
			TestEqual(*FString::Printf(TEXT("premessa: la banda e' %s"), Banda), Voce->Def.MovementProfileId,
				bSprint ? URTMovementProfileLibrary::ProfileSprint : URTMovementProfileLibrary::ProfileMove);
		}
		TestEqual(*FString::Printf(TEXT("1 (%s): la spesa prevista e' quella del piano dopo il click"), Banda),
			Prevista, URTPlanValidationLibrary::ActionPointsSpent(Piano));
		TestTrue(*FString::Printf(TEXT("1 (%s): e il movimento costa qualcosa"), Banda), Prevista > 0);

		// Con il percorso nel piano, un waypoint in piu' lo allunga: la spesa e' quella di adesso.
		TestEqual(*FString::Printf(TEXT("1 (%s): allungare un percorso pianificato costa 0"), Banda),
			B.Unit->ActionPointsAfterMoving(), B.Spent());
		DestroyApPreviewBench(B);
	}
	return true;
}

/** #4155 — **i pip dicono lo stato DOPO il gesto**: `Preview` fra impegnati e liberi, `Over` oltre il budget, un gesto che libera AP. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionPointPipsShowTheGestureTest,
	"RefactorTactics.HudViewModel.ActionPointPipsShowTheGesture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTActionPointPipsShowTheGestureTest::RunTest(const FString&)
{
	using P = ERTActionPointPip;
	const auto Vista = [](int32 Budget, int32 Committed, int32 Preview)
	{
		FRTActionPointView V;
		V.bAuthorized = true;
		V.Budget = Budget;
		V.Committed = Committed;
		V.Preview = Preview;
		return V;
	};
	ApPipsEqual(*this, TEXT("senza gesto: la fila di #4150"), URTHudViewModel::ActionPointPips(Vista(6, 4, 0)),
		TArray<P>({ P::Committed, P::Committed, P::Committed, P::Committed, P::Free, P::Free }));
	ApPipsEqual(*this, TEXT("un gesto da 2 entro il budget: due Preview fra gli impegnati e i liberi"),
		URTHudViewModel::ActionPointPips(Vista(6, 2, 2)),
		TArray<P>({ P::Committed, P::Committed, P::Preview, P::Preview, P::Free, P::Free }));
	ApPipsEqual(*this, TEXT("un gesto da 2 su un residuo 0: i suoi AP sono Over, fuori dalla cornice"),
		URTHudViewModel::ActionPointPips(Vista(4, 4, 2)),
		TArray<P>({ P::Committed, P::Committed, P::Committed, P::Committed, P::Over, P::Over }));
	ApPipsEqual(*this, TEXT("un gesto che sfora a meta': cio' che sta nel budget e' Preview, il resto Over"),
		URTHudViewModel::ActionPointPips(Vista(3, 2, 2)),
		TArray<P>({ P::Committed, P::Committed, P::Preview, P::Over }));
	ApPipsEqual(*this, TEXT("un gesto che libera 1 AP: l'ultimo impegnato appare libero"),
		URTHudViewModel::ActionPointPips(Vista(6, 4, -1)),
		TArray<P>({ P::Committed, P::Committed, P::Committed, P::Free, P::Free, P::Free }));
	TestEqual(TEXT("una vista non autorizzata non ha pip, anche con un Preview"),
		URTHudViewModel::ActionPointPips([&Vista]() { FRTActionPointView V = Vista(6, 2, 2); V.bAuthorized = false; return V; }()).Num(), 0);
	return true;
}

/**
 * #4155, criterio 3 — **[D-625] (2): un click su una cella che il residuo non paga non pianifica; allungare un percorso pagato si'.**
 *
 * Due piani nello stesso banco. Nel primo il movimento si aggiunge a un piano da 6 AP senza movimento (attacco base, reazione, scatto):
 * a budget 6 il waypoint non si posa, a budget 8 si' (controllo positivo). Nel secondo il movimento e' gia' nel piano, e il waypoint
 * che lo allunga si posa anche a residuo 0 e a piano sforato, perche' la spesa non cambia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnaffordableMoveDoesNotPlanTest,
	"RefactorTactics.PlayerInput.UnaffordableMoveDoesNotPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnaffordableMoveDoesNotPlanTest::RunTest(const FString&)
{
	// --- Il movimento che si AGGIUNGE --------------------------------------------------------------------------------
	{
		FApPreviewBench B = MakeApPreviewBench();
		if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		B.Unit->SetActionPointBudget(6);
		B.Unit->DeclarePlannedMainAction(B.Attacco);
		B.PC->SelectAbilityForCurrentForTest(B.Reazione);
		B.Unit->SetPlannedDash(B.Scatto, FRTCellId(0, 1, 0));
		if (!TestEqual(TEXT("premessa: attacco base, reazione e scatto spendono 6"), B.Spent(), 6))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		B.Unit->SelectAbility(INDEX_NONE); // il click e' sulla mappa, non su un bersaglio dell'azione armata

		B.PC->HandleClickOnCellForTest(FRTCellId(1, 0, 0));
		TestEqual(TEXT("3: a budget 6 il waypoint non si posa"), B.Unit->GetPlan().PlannedWaypoints().Num(), 0);
		TestEqual(TEXT("3: e il piano spende ancora 6"), B.Spent(), 6);

		B.Unit->SetActionPointBudget(8);
		B.PC->HandleClickOnCellForTest(FRTCellId(1, 0, 0));
		TestEqual(TEXT("3: controllo positivo, a budget 8 il waypoint si posa"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
		TestEqual(TEXT("3: e il piano spende 8"), B.Spent(), 8);

		// Il piano ora e' oltre un budget di 6 (il budget e' sceso dopo): allungarlo non peggiora la spesa, e passa.
		B.Unit->SetActionPointBudget(6);
		B.PC->HandleClickOnCellForTest(FRTCellId(2, 0, 0));
		TestEqual(TEXT("3: a piano sforato il percorso si allunga"), B.Unit->GetPlan().PlannedWaypoints().Num(), 2);
		TestEqual(TEXT("3: e la spesa non cambia"), B.Spent(), 8);
		DestroyApPreviewBench(B);
	}

	// --- Il movimento che si ALLUNGA a residuo 0 ------------------------------------------------------------------------
	{
		FApPreviewBench B = MakeApPreviewBench();
		if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		B.Unit->SetActionPointBudget(6);
		B.Unit->DeclarePlannedMainAction(B.Attacco);
		B.PC->SelectAbilityForCurrentForTest(B.Reazione);
		B.Unit->SelectAbility(INDEX_NONE);
		TestEqual(TEXT("premessa: attacco base e reazione spendono 4"), B.Spent(), 4);

		B.PC->HandleClickOnCellForTest(FRTCellId(1, 0, 0));
		TestEqual(TEXT("3: il movimento porta il piano a 6 su 6, e si posa"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
		TestEqual(TEXT("premessa: il piano spende 6"), B.Spent(), 6);
		B.PC->HandleClickOnCellForTest(FRTCellId(2, 0, 0));
		TestEqual(TEXT("3: a residuo 0 il percorso pianificato si allunga"), B.Unit->GetPlan().PlannedWaypoints().Num(), 2);
		DestroyApPreviewBench(B);
	}
	return true;
}

/**
 * #4155, criteri 2, 4 e 5 — **la dock mostra il costo del gesto sotto il cursore, e la cella che il residuo non paga si mostra rifiutata.**
 *
 * La dock legge il controller (`ActionPointsAfterGestureNow`) come in partita: controller e unita' sono iniettati, il ventaglio e' quello
 * che l'arbitro calcola, la cella sotto il cursore e' quella della mappa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDockShowsTheGestureCostTest,
	"RefactorTactics.ScreenHud.DockShowsTheGestureCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDockShowsTheGestureCostTest::RunTest(const FString&)
{
	FApPreviewBench B = MakeApPreviewBench();
	URTActionDockWidget* Dock = B.World ? NewObject<URTActionDockWidget>(B.World) : nullptr;
	if (!TestTrue(TEXT("il banco esiste"), B.IsValid()) || !TestNotNull(TEXT("dock"), Dock))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	Dock->SetSelectedUnitForTest(B.Unit);
	Dock->SetCommandControllerForTest(B.PC);

	const FRTCellId Cella(1, 0, 0);
	B.Unit->SetActionPointBudget(6);
	B.Unit->DeclarePlannedMainAction(B.Attacco);
	B.Unit->SelectAbility(INDEX_NONE);
	B.PC->RefreshPlanningOverlay(B.Unit);
	if (!TestTrue(TEXT("premessa: la cella e' nel ventaglio"), B.Map->IsPreviewReachableCell(Cella))
		|| !TestEqual(TEXT("premessa: il piano spende 2"), B.Spent(), 2))
	{
		DestroyApPreviewBench(B);
		return false;
	}

	// Nessun gesto: la vista e' quella di #4150.
	B.Map->SetHoveredCell(FRTCellId(), /*bValid=*/ false);
	TestEqual(TEXT("senza gesto: Preview 0"), Dock->GetActionPoints().Preview, 0);
	TestEqual(TEXT("senza gesto: Residual = budget - impegnati"), Dock->GetActionPoints().Residual, 4);

	// --- 2. L'hover su una cella del ventaglio ----------------------------------------------------------------------------
	B.Map->SetHoveredCell(Cella, /*bValid=*/ true);
	const FRTActionPointView ConCella = Dock->GetActionPoints();
	TestEqual(TEXT("2: l'hover sulla cella porta Preview 2"), ConCella.Preview, 2);
	TestEqual(TEXT("2: e il Preview e' la spesa dopo il waypoint, meno l'impegnato"), ConCella.Preview, B.Unit->ActionPointsAfterMoving() - B.Spent());
	TestEqual(TEXT("2: Residual = budget - impegnati - 2"), ConCella.Residual, 2);
	using P = ERTActionPointPip;
	ApPipsEqual(*this, TEXT("2: i pip mostrano due Preview"), URTHudViewModel::ActionPointPips(ConCella),
		TArray<P>({ P::Committed, P::Committed, P::Preview, P::Preview, P::Free, P::Free }));
	TestEqual(TEXT("2: nessun motivo di rifiuto"), static_cast<int32>(B.PC->HoveredMoveRefusal()), static_cast<int32>(ERTActionInvalidReason::None));
	TestEqual(TEXT("2: il ventaglio e' Movement"), static_cast<int32>(B.Map->GetPreviewReachableMeaning()), static_cast<int32>(ERTOverlayMeaning::Movement));

	Dock->EnsureActionPointPips();
	Dock->RefreshActionPoints();
	if (TestEqual(TEXT("2: la dock dipinge sei pip"), Dock->ActionPointPips->GetChildrenCount(), 6))
	{
		const UBorder* Terzo = Cast<UBorder>(Dock->ActionPointPips->GetChildAt(2));
		if (TestNotNull(TEXT("2: il terzo pip e' un bordo"), Terzo))
		{
			TestEqual(TEXT("2: il pip del gesto e' dipinto in bianco"), Terzo->GetBrushColor(), URTUIPalette::ColorFor(ERTUIToken::White));
		}
	}

	// Una cella fuori dal ventaglio non e' un gesto.
	B.Map->SetHoveredCell(FRTCellId(30, 0, 0), /*bValid=*/ true);
	TestEqual(TEXT("2: una cella fuori dal ventaglio non ha anteprima"), Dock->GetActionPoints().Preview, 0);

	// --- 5. L'hover su uno slot della dock ---------------------------------------------------------------------------------
	B.Map->SetHoveredCell(FRTCellId(), /*bValid=*/ false);
	B.PC->SetSkillHover(B.Unit, B.Reazione);
	TestEqual(TEXT("5: lo slot della reazione porta il delta di ActionPointsAfterArming"),
		Dock->GetActionPoints().Preview, B.Unit->ActionPointsAfterArming(B.Reazione) - B.Spent());
	TestEqual(TEXT("5: che per la reazione e' 2"), Dock->GetActionPoints().Preview, 2);
	B.PC->ClearSkillHover(B.Unit, B.Reazione);
	B.PC->SetSkillHover(B.Unit, B.Attacco);
	TestEqual(TEXT("5: lo slot gia' nel piano non ha anteprima"), Dock->GetActionPoints().Preview, 0);
	B.PC->ClearSkillHover(B.Unit, B.Attacco);
	TestEqual(TEXT("5: senza hover lo slot non e' piu' il gesto"), Dock->GetActionPoints().Preview, 0);

	// La principale sostituisce se stessa, quindi lo 0 qui sopra non distingue «gia' nel piano» da «costa 0». Guard, aggiunta come voce
	// di Prep con la principale presa, lo distingue: chiederle il costo di un'altra copia direbbe 1 (revisione di #4153).
	B.PC->SelectAbilityForCurrentForTest(B.Guardia);
	B.Unit->SelectAbility(INDEX_NONE);
	TestTrue(TEXT("premessa: Guard e' nel piano"), B.Unit->IsAbilityInPlan(B.Guardia));
	TestEqual(TEXT("premessa: attacco base e Guard spendono 3"), B.Spent(), 3);
	TestEqual(TEXT("premessa: un'altra copia di Guard costerebbe 1"), B.Unit->ActionPointsAfterArming(B.Guardia) - B.Spent(), 1);
	B.PC->SetSkillHover(B.Unit, B.Guardia);
	TestEqual(TEXT("5: la voce di Prep gia' nel piano non ha anteprima"), Dock->GetActionPoints().Preview, 0);
	B.PC->ClearSkillHover(B.Unit, B.Guardia);

	// --- 4. Il residuo non paga il Move ---------------------------------------------------------------------------------
	B.PC->SelectAbilityForCurrentForTest(B.Reazione);
	B.Unit->SelectAbility(INDEX_NONE);
	B.Unit->SetActionPointBudget(5);
	TestEqual(TEXT("premessa: attacco base, Guard e reazione spendono 5 su 5"), B.Spent(), 5);
	// L'aggiornamento di ogni fotogramma, non il ricalcolo forzato: la pagabilita' e' un ingresso dell'arbitro.
	B.PC->UpdatePlanningOverlay(B.Map);
	TestEqual(TEXT("4: il ventaglio prende MovementOverBudget senza un ricalcolo forzato"),
		static_cast<int32>(B.Map->GetPreviewReachableMeaning()), static_cast<int32>(ERTOverlayMeaning::MovementOverBudget));
	TestTrue(TEXT("4: e le sue celle restano visibili"), B.Map->IsPreviewReachableCell(Cella));
	TestEqual(TEXT("4: nessuna cella conta come Movement"), B.Map->NumPlanningPreviewCells(ERTOverlayMeaning::Movement), 0);
	TestTrue(TEXT("4: tutte contano come MovementOverBudget"), B.Map->NumPlanningPreviewCells(ERTOverlayMeaning::MovementOverBudget) > 0);

	B.Map->SetHoveredCell(Cella, /*bValid=*/ true);
	ApPipsEqual(*this, TEXT("4: i pip dell'hover sono Over"), URTHudViewModel::ActionPointPips(Dock->GetActionPoints()),
		TArray<P>({ P::Committed, P::Committed, P::Committed, P::Committed, P::Committed, P::Over, P::Over }));
	TestEqual(TEXT("4: il motivo all'hover e' OverBudget"), static_cast<int32>(B.PC->HoveredMoveRefusal()), static_cast<int32>(ERTActionInvalidReason::OverBudget));
	const FString Testo = ARTHUD::MoveRefusalText(B.PC->HoveredMoveRefusal());
	TestTrue(*FString::Printf(TEXT("4: il testo nomina il motivo: %s"), *Testo),
		Testo.Contains(URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason::OverBudget)));
	TestTrue(TEXT("4: senza motivo non c'e' testo"), ARTHUD::MoveRefusalText(ERTActionInvalidReason::None).IsEmpty());

	// Controllo positivo: con un budget che paga, il ventaglio torna Movement e il motivo sparisce.
	B.Unit->SetActionPointBudget(7);
	B.PC->UpdatePlanningOverlay(B.Map);
	TestEqual(TEXT("4: controllo positivo, il ventaglio torna Movement"),
		static_cast<int32>(B.Map->GetPreviewReachableMeaning()), static_cast<int32>(ERTOverlayMeaning::Movement));
	TestEqual(TEXT("4: e il motivo sparisce"), static_cast<int32>(B.PC->HoveredMoveRefusal()), static_cast<int32>(ERTActionInvalidReason::None));

	DestroyApPreviewBench(B);
	return true;
}

/**
 * #4155, revisione — **niente anteprima e niente rifiuto dove il click non posa un waypoint**: con uno scatto armato la cella e' la sua
 * meta, con il verso dichiarato il movimento e' chiuso (`ARTUnit::WaypointClickIsOpen`).
 *
 * In entrambi i casi il piano non paga il Move, quindi senza la regola l'hover direbbe «Non si muove» e il ventaglio sarebbe rosso cupo.
 * Il controllo positivo e' lo stesso piano senza quella condizione: il rifiuto torna. E il click vero, accanto, mostra che la regola
 * dice cio' che il click fa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoMovePreviewWhereTheClickDoesNotMoveTest,
	"RefactorTactics.ScreenHud.NoMovePreviewWhereTheClickDoesNotMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoMovePreviewWhereTheClickDoesNotMoveTest::RunTest(const FString&)
{
	FApPreviewBench B = MakeApPreviewBench();
	if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	const FRTCellId Cella(1, 0, 0);
	const FRTCellId MetaDelloScatto(0, 1, 0);
	B.Unit->DeclarePlannedMainAction(B.Attacco);
	B.PC->SelectAbilityForCurrentForTest(B.Reazione);
	B.Unit->SetPlannedDash(B.Scatto, MetaDelloScatto);
	B.Unit->SelectAbility(INDEX_NONE);
	B.Unit->SetActionPointBudget(6);
	if (!TestEqual(TEXT("premessa: attacco base, reazione e scatto spendono 6 su 6"), B.Spent(), 6))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	B.PC->RefreshPlanningOverlay(B.Unit);
	B.Map->SetHoveredCell(Cella, /*bValid=*/ true);
	const auto Rifiuto = [&B]() { return static_cast<int32>(B.PC->HoveredMoveRefusal()); };
	const auto Ventaglio = [&B]() { return static_cast<int32>(B.Map->GetPreviewReachableMeaning()); };
	const int32 Nessuno = static_cast<int32>(ERTActionInvalidReason::None);
	const int32 OverBudget = static_cast<int32>(ERTActionInvalidReason::OverBudget);

	// Controllo positivo: senza scatto armato e senza verso, il Move non si paga e l'hover lo dice.
	if (!TestTrue(TEXT("premessa: la cella e' nel ventaglio"), B.Map->IsPreviewReachableCell(Cella)))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	TestEqual(TEXT("controllo: il rifiuto c'e'"), Rifiuto(), OverBudget);
	TestEqual(TEXT("controllo: il ventaglio e' rosso cupo"), Ventaglio(), static_cast<int32>(ERTOverlayMeaning::MovementOverBudget));
	TestTrue(TEXT("controllo: e c'e' un'anteprima"), B.PC->ActionPointsAfterGestureNow(B.Unit) != INDEX_NONE);

	// --- Lo scatto pianificato, riarmato: il click sulla cella e' la sua meta ------------------------------------------
	B.Unit->SelectAbility(B.Scatto);
	B.PC->UpdatePlanningOverlay(B.Map);
	TestEqual(TEXT("premessa: con lo scatto armato il contesto e' Planning"),
		static_cast<int32>(B.PC->GetPointerContext()), static_cast<int32>(ERTPointerContext::Planning));
	TestFalse(TEXT("scatto armato: il click non posa un waypoint"), B.Unit->WaypointClickIsOpen());
	TestEqual(TEXT("scatto armato: nessun rifiuto all'hover"), Rifiuto(), Nessuno);
	TestEqual(TEXT("scatto armato: nessuna anteprima"), B.PC->ActionPointsAfterGestureNow(B.Unit), (int32)INDEX_NONE);
	TestEqual(TEXT("scatto armato: il ventaglio non e' rosso cupo"), Ventaglio(), static_cast<int32>(ERTOverlayMeaning::Movement));
	B.PC->HandleClickOnCellForTest(Cella);
	TestEqual(TEXT("scatto armato: il click non posa un waypoint"), B.Unit->GetPlan().PlannedWaypoints().Num(), 0);
	TestEqual(TEXT("scatto armato: il click sposta la meta dello scatto"), B.Unit->GetPlan().PlannedDashCell(), Cella);
	// La meta torna dov'era: sulla cella d'arrivo il click apre il verso, e il resto del test la vuole una cella di movimento.
	B.Unit->SetPlannedDash(B.Scatto, MetaDelloScatto);
	B.Unit->SelectAbility(INDEX_NONE);

	// --- Il verso dichiarato: il movimento e' chiuso ---------------------------------------------------------------------
	B.Unit->DeclarePlannedFacing(ERTHexDirection::E);
	B.PC->UpdatePlanningOverlay(B.Map);
	TestFalse(TEXT("verso dichiarato: il click non posa un waypoint"), B.Unit->WaypointClickIsOpen());
	TestEqual(TEXT("verso dichiarato: nessun rifiuto all'hover"), Rifiuto(), Nessuno);
	TestEqual(TEXT("verso dichiarato: nessuna anteprima"), B.PC->ActionPointsAfterGestureNow(B.Unit), (int32)INDEX_NONE);
	TestEqual(TEXT("verso dichiarato: il ventaglio non e' rosso cupo"), Ventaglio(), static_cast<int32>(ERTOverlayMeaning::Movement));
	B.Unit->SetActionPointBudget(8); // il budget non c'entra: il verso chiude il movimento anche quando il Move si paga
	B.PC->HandleClickOnCellForTest(Cella);
	TestEqual(TEXT("verso dichiarato: il click non posa un waypoint"), B.Unit->GetPlan().PlannedWaypoints().Num(), 0);

	// Ritirato il verso, a budget 6, il rifiuto torna: e' la condizione, non lo stato del banco, che lo spegneva.
	B.Unit->RetractPlannedFacing();
	B.Unit->SetActionPointBudget(6);
	B.PC->UpdatePlanningOverlay(B.Map);
	TestEqual(TEXT("ritirato il verso il rifiuto torna"), Rifiuto(), OverBudget);

	DestroyApPreviewBench(B);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
