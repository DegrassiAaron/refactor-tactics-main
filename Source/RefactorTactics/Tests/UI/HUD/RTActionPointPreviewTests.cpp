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
#include "Components/Overlay.h"      // HUD-AP.5a: la radice del banco dello slot
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
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
	/**
	 * Il significato con cui il ventaglio disegna `Cella`: da E38 CP 38.16 il ventaglio ha due aree, il pagato (`Movement`) e il tratto che
	 * il residuo non paga (`MovementOverBudget`, [D-628] (3)), e il colore e' della cella, non dell'insieme.
	 */
	ERTOverlayMeaning ApPreviewMeaningOf(const ARTHexMapActor* Map, const FRTCellId& Cella)
	{
		return Map->IsPreviewOverBudgetCell(Cella) ? ERTOverlayMeaning::MovementOverBudget : ERTOverlayMeaning::Movement;
	}

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
	TestEqual(TEXT("2: il ventaglio e' Movement"), static_cast<int32>(ApPreviewMeaningOf(B.Map, Cella)), static_cast<int32>(ERTOverlayMeaning::Movement));

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
		static_cast<int32>(ApPreviewMeaningOf(B.Map, Cella)), static_cast<int32>(ERTOverlayMeaning::MovementOverBudget));
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
		static_cast<int32>(ApPreviewMeaningOf(B.Map, Cella)), static_cast<int32>(ERTOverlayMeaning::Movement));
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
	const auto Ventaglio = [&B, &Cella]() { return static_cast<int32>(ApPreviewMeaningOf(B.Map, Cella)); };
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

/**
 * E38 CP 38.16, [D-625] (1), [D-628] (2) — **un waypoint oltre il tetto dello Sprint apre un tratto, e il tratto e' una voce Move da 2.**
 *
 * Gadget ha movimento base 5: il tetto nudo e' 10 MP, il secondo tratto arriva a 15. Il banco e' piano, quindi una cella costa 1 e il
 * costo e' la distanza. Il click a 12 celle si posa a budget 6 e il piano spende 4; la stessa cella a budget 3 non si posa, e una a 10
 * si' (controllo positivo): il confine e' il tetto in MP, non la distanza del raggio. Il budget del resolver e' quello dei due tratti.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAWaypointBeyondTheSprintCapOpensALegTest,
	"RefactorTactics.PlayerInput.AWaypointBeyondTheSprintCapOpensALeg",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAWaypointBeyondTheSprintCapOpensALegTest::RunTest(const FString&)
{
	const auto VociDiMovimento = [](const TArray<FRTPlannedAction>& Piano)
	{
		TArray<FRTPlannedAction> Voci;
		for (const FRTPlannedAction& E : Piano)
		{
			if (!E.Def.MovementProfileId.IsNone()) { Voci.Add(E); }
		}
		return Voci;
	};

	// --- Il tratto pagato --------------------------------------------------------------------------------------------
	{
		FApPreviewBench B = MakeApPreviewBench();
		if (!TestTrue(TEXT("il banco esiste"), B.IsValid())
			|| !TestEqual(TEXT("premessa: Gadget ha movimento base 5"), B.Unit->GetEffectiveMoveRange(), 5))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		B.Unit->SetActionPointBudget(6);
		B.PC->HandleClickOnCellForTest(FRTCellId(12, 0, 0));
		TestEqual(TEXT("il waypoint a 12 celle si posa"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
		TestEqual(TEXT("il piano porta il costo del percorso"), B.Unit->GetPlan().PlannedPathCost(), 12);
		TestEqual(TEXT("due tratti"), B.Unit->PlannedMoveLegs(), 2);

		const TArray<FRTPlannedAction> Voci = VociDiMovimento(URTPlanValidationLibrary::MakePlanFor(B.Unit));
		if (TestEqual(TEXT("due voci di movimento"), Voci.Num(), 2))
		{
			TestEqual(TEXT("la prima e' la banda, letta dalla distanza: Sprint"), Voci[0].Def.MovementProfileId,
				URTMovementProfileLibrary::ProfileSprint);
			TestEqual(TEXT("la seconda e' un Move"), Voci[1].Def.ActionId, FName(TEXT("Action.Move")));
			TestEqual(TEXT("di profilo Move"), Voci[1].Def.MovementProfileId, URTMovementProfileLibrary::ProfileMove);
			TestEqual(TEXT("con la dichiarazione del percorso"), Voci[1].DeclarationIndex, Voci[0].DeclarationIndex);
		}
		TestEqual(TEXT("il piano spende 4"), B.Spent(), 4);
		TestTrue(TEXT("ed e' legale"), URTPlanValidationLibrary::ValidatePlan(ARTTurnManager::MakeSimUnit(INDEX_NONE, B.Unit),
			URTPlanValidationLibrary::MakePlanFor(B.Unit)).bLegal);
		TestEqual(TEXT("il resolver esegue col budget dei due tratti: 10 + 5"),
			ARTTurnManager::MakeSimUnit(INDEX_NONE, B.Unit).MoveBudget, 15);

		// Controllo positivo del budget: lo stesso percorso senza il costo dichiarato e' il budget della banda, e un tratto solo.
		const TArray<FRTCellId> Percorso = B.Unit->GetPlan().PlannedPath();
		B.Unit->SetPlannedRoute(Percorso);
		TestEqual(TEXT("controllo: senza costo un tratto"), B.Unit->PlannedMoveLegs(), 1);
		TestEqual(TEXT("controllo: e il budget della banda Sprint"), ARTTurnManager::MakeSimUnit(INDEX_NONE, B.Unit).MoveBudget, 10);
		DestroyApPreviewBench(B);
	}

	// --- Il tratto che il residuo non paga -----------------------------------------------------------------------------
	{
		FApPreviewBench B = MakeApPreviewBench();
		if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		// Budget 6 e non un budget basso: a budget 6 il turno pagherebbe tre tratti a piano vuoto, quindi la pianificazione compone il
		// percorso fino a 20 MP e il rifiuto qui sotto lo da' il RESIDUO, non il tetto di pianificazione. ⏱️ *La prima stesura usava budget
		// 3*, dove il tetto di pianificazione e' gia' un tratto solo: il click era rifiutato dal pathfinding, e la mutazione che toglie il
		// controllo degli AP del click sopravviveva (M45 della 38.16b).
		B.Unit->SetActionPointBudget(6);
		B.Unit->DeclarePlannedMainAction(B.Attacco);
		B.PC->SelectAbilityForCurrentForTest(B.Reazione);
		B.Unit->SelectAbility(INDEX_NONE);
		if (!TestEqual(TEXT("premessa: attacco base e reazione spendono 4, il residuo paga un tratto"), B.Spent(), 4))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		B.PC->HandleClickOnCellForTest(FRTCellId(12, 0, 0));
		TestEqual(TEXT("a residuo 2 il waypoint a 12 celle non si posa"), B.Unit->GetPlan().PlannedWaypoints().Num(), 0);
		TestEqual(TEXT("e il piano spende ancora 4"), B.Spent(), 4);

		B.PC->HandleClickOnCellForTest(FRTCellId(10, 0, 0));
		TestEqual(TEXT("controllo: quello a 10, sul tetto, si posa"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
		TestEqual(TEXT("controllo: e il piano spende 6"), B.Spent(), 6);

		// Allungare di una cella oltre il tetto apre il secondo tratto, che il residuo 0 non paga.
		B.PC->HandleClickOnCellForTest(FRTCellId(11, 0, 0));
		TestEqual(TEXT("il waypoint che apre il secondo tratto non si posa"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
		TestEqual(TEXT("e il piano spende ancora 6"), B.Spent(), 6);
		DestroyApPreviewBench(B);
	}
	return true;
}

/**
 * E38 CP 38.16, [D-628] (3) e la lettura (f) di [D-625] — **il ventaglio e' verde nei tratti che il residuo paga e rosso cupo nel
 * successivo, e oltre niente; l'hover costa 0 nel tratto pagato e 2 per il tratto che apre.**
 *
 * Gadget, tetto 10, tratto 5. A budget 2 il residuo paga un tratto: 10 e' verde, 11 rosso cupo. A budget 0 non paga neppure il primo:
 * 10 e' rosso cupo e 11 e' fuori dal ventaglio. A budget 4 ne paga due. Con un waypoint a 3 gia' pianificato, una cella a 8 sta nel
 * primo tratto (costo 8) e non costa niente, una a 11 lo supera (costo 11) e costa 2.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFanPaysTheLegsTheResidualCoversTest,
	"RefactorTactics.ScreenHud.FanPaysTheLegsTheResidualCovers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFanPaysTheLegsTheResidualCoversTest::RunTest(const FString&)
{
	FApPreviewBench B = MakeApPreviewBench();
	if (!TestTrue(TEXT("il banco esiste"), B.IsValid())
		|| !TestEqual(TEXT("premessa: Gadget ha movimento base 5"), B.Unit->GetEffectiveMoveRange(), 5))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	const auto Verde = [&B](const FRTCellId& C) { return B.Map->IsPreviewReachableCell(C) && !B.Map->IsPreviewOverBudgetCell(C); };
	const auto Rossa = [&B](const FRTCellId& C) { return B.Map->IsPreviewOverBudgetCell(C); };
	const auto Fuori = [&B](const FRTCellId& C) { return !B.Map->IsPreviewReachableCell(C); };
	const auto Anteprima = [&B](const FRTCellId& C)
	{
		B.Map->SetHoveredCell(C, /*bValid=*/ true);
		const int32 Dopo = B.PC->ActionPointsAfterGestureNow(B.Unit);
		return Dopo == INDEX_NONE ? INDEX_NONE : Dopo - B.Spent();
	};
	const FRTCellId Dieci(10, 0, 0);
	const FRTCellId Undici(11, 0, 0);

	// --- Budget 2: un tratto pagato ---------------------------------------------------------------------------------------
	B.Unit->SetActionPointBudget(2);
	B.PC->RefreshPlanningOverlay(B.Unit);
	TestTrue(TEXT("budget 2: la cella a 10 e' verde"), Verde(Dieci));
	TestTrue(TEXT("budget 2: quella a 11 e' rosso cupo"), Rossa(Undici));
	TestEqual(TEXT("budget 2: il ventaglio sa il costo fino a 11"), B.Map->GetPreviewMoveCost(Undici), 11);
	TestEqual(TEXT("budget 2: l'hover su una cella del primo tratto costa 2"), Anteprima(FRTCellId(5, 0, 0)), 2);
	TestEqual(TEXT("budget 2: e non ha un rifiuto"), static_cast<int32>(B.PC->HoveredMoveRefusal()),
		static_cast<int32>(ERTActionInvalidReason::None));
	TestEqual(TEXT("budget 2: l'hover sul tratto successivo costa 4"), Anteprima(Undici), 4);
	TestEqual(TEXT("budget 2: e il rifiuto e' OverBudget"), static_cast<int32>(B.PC->HoveredMoveRefusal()),
		static_cast<int32>(ERTActionInvalidReason::OverBudget));

	// --- Budget 0: nessun tratto pagato; il ventaglio arriva al primo e si ferma -------------------------------------------
	B.Unit->SetActionPointBudget(0);
	B.PC->UpdatePlanningOverlay(B.Map); // la pagabilita' e' un ingresso dell'arbitro: nessun ricalcolo forzato
	TestTrue(TEXT("budget 0: la cella a 10 e' rosso cupo"), Rossa(Dieci));
	TestTrue(TEXT("budget 0: quella a 11 e' fuori dal ventaglio"), Fuori(Undici));
	TestEqual(TEXT("budget 0: nessuna cella verde"), B.Map->NumPlanningPreviewCells(ERTOverlayMeaning::Movement), 0);

	// --- Budget 4: due tratti pagati ------------------------------------------------------------------------------------------
	B.Unit->SetActionPointBudget(4);
	B.PC->UpdatePlanningOverlay(B.Map);
	TestTrue(TEXT("budget 4: la cella a 11 e' verde"), Verde(Undici));
	TestEqual(TEXT("budget 4: e l'hover costa i due tratti"), Anteprima(Undici), 4);

	// --- Un percorso gia' pianificato: 0 nel tratto pagato, 2 per il tratto che apre -------------------------------------------
	B.PC->HandleClickOnCellForTest(FRTCellId(3, 0, 0));
	if (TestEqual(TEXT("premessa: il waypoint a 3 si posa e spende 2"), B.Spent(), 2))
	{
		TestEqual(TEXT("una cella a 8 resta nel primo tratto: costa 0"), Anteprima(FRTCellId(8, 0, 0)), 0);
		TestEqual(TEXT("una cella a 11 apre il secondo: costa 2"), Anteprima(Undici), 2);
		TestEqual(TEXT("il ventaglio somma il percorso posato"), B.Map->GetPreviewMoveCost(Undici), 11);
	}
	DestroyApPreviewBench(B);
	return true;
}

/**
 * E38 CP 38.16, [D-626] (1) — **sotto `Sneak` il tetto vale sull'intero movimento: il ventaglio non ha un tratto successivo.**
 *
 * Gadget sguscia a 2 (50% di 5, per difetto). A budget 6 il ventaglio arriva a 2 e non ha celle rosso cupo; a budget 0 la cella a 2 e'
 * rosso cupo e quella a 3 fuori. Il controllo positivo e' lo stesso budget 0 senza `Sneak`: la cella a 3 e' nel ventaglio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSneakFanHasNoNextLegTest,
	"RefactorTactics.ScreenHud.SneakFanHasNoNextLeg",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSneakFanHasNoNextLegTest::RunTest(const FString&)
{
	FApPreviewBench B = MakeApPreviewBench();
	if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	const FRTCellId Due(2, 0, 0);
	const FRTCellId Tre(3, 0, 0);
	B.Unit->SetPlannedMovementProfile(URTMovementProfileLibrary::ProfileSneak);
	B.Unit->SetActionPointBudget(6);
	B.PC->RefreshPlanningOverlay(B.Unit);
	TestTrue(TEXT("Sneak, budget 6: la cella a 2 e' nel ventaglio"), B.Map->IsPreviewReachableCell(Due));
	TestFalse(TEXT("Sneak, budget 6: quella a 3 no"), B.Map->IsPreviewReachableCell(Tre));
	TestEqual(TEXT("Sneak, budget 6: nessuna cella rosso cupo"), B.Map->GetPreviewOverBudgetCells().Num(), 0);

	B.Unit->SetActionPointBudget(0);
	B.PC->UpdatePlanningOverlay(B.Map);
	TestTrue(TEXT("Sneak, budget 0: la cella a 2 e' rosso cupo"), B.Map->IsPreviewOverBudgetCell(Due));
	TestFalse(TEXT("Sneak, budget 0: quella a 3 e' fuori"), B.Map->IsPreviewReachableCell(Tre));

	B.Unit->SetPlannedMovementProfile(NAME_None);
	B.PC->RefreshPlanningOverlay(B.Unit);
	TestTrue(TEXT("controllo: senza Sneak la cella a 3 e' nel ventaglio"), B.Map->IsPreviewOverBudgetCell(Tre));
	DestroyApPreviewBench(B);
	return true;
}

/**
 * E38 CP 38.16, [D-626] (1), revisione della 38.16b — **armare l'Overwatch non conta i tratti che il suo `Withdraw` toglie.**
 *
 * Budget 4 e un percorso di 12 celle, con un primo waypoint a una cella: due tratti, 4 AP, residuo 0. L'Overwatch riserva il `Withdraw`, che vale sull'intero movimento,
 * quindi armarlo riporta il percorso a un tratto: la spesa dopo il gesto e' quella di un tratto piu' l'Overwatch, e il gesto si paga.
 * Il controllo e' la reazione del banco, che non riserva niente: con lo stesso piano non si paga. E l'armo vero spende quanto previsto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmingAReserveDropsTheExtraLegsTest,
	"RefactorTactics.PlayerInput.ArmingAReserveDropsTheExtraLegs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmingAReserveDropsTheExtraLegsTest::RunTest(const FString&)
{
	FApPreviewBench B = MakeApPreviewBench();
	const int32 Sentinella = B.IsValid() ? RTAbilityFixtures::AddCoreAbilityInSlot(B.Unit, TEXT("Action.Overwatch"), B.Guardia) : INDEX_NONE;
	if (!TestTrue(TEXT("il banco esiste"), B.IsValid()) || !TestTrue(TEXT("l'Overwatch e' nel kit"), Sentinella != INDEX_NONE))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	B.Unit->SetActionPointBudget(4);
	// Il primo waypoint a una cella: il `Withdraw` di Gadget vale 1 MP (25% di 5), e il taglio e' per waypoint interi ([D-420]). Con il solo
	// waypoint a 12 il taglio toglierebbe tutto il Move, il «caso stretto» di `ActionPointsAfterArming`, e il conto non sarebbe quello dei tratti.
	B.PC->HandleClickOnCellForTest(FRTCellId(1, 0, 0));
	B.PC->HandleClickOnCellForTest(FRTCellId(12, 0, 0));
	if (!TestEqual(TEXT("premessa: due tratti"), B.Unit->PlannedMoveLegs(), 2) || !TestEqual(TEXT("premessa: il piano spende 4"), B.Spent(), 4))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	const int32 CostoSentinella = B.Unit->GetAbility(Sentinella)->Def.ActionPointCost;
	TestEqual(TEXT("la spesa dopo l'armo e' un tratto piu' l'Overwatch"), B.Unit->ActionPointsAfterArming(Sentinella), 2 + CostoSentinella);
	TestTrue(TEXT("e l'armo si paga"), B.Unit->CanAffordArming(Sentinella));
	TestFalse(TEXT("controllo: la reazione, che non riserva niente, non si paga"), B.Unit->CanAffordArming(B.Reazione));

	const int32 Prevista = B.Unit->ActionPointsAfterArming(Sentinella);
	B.PC->SelectAbilityForCurrentForTest(Sentinella);
	B.Unit->SelectAbility(INDEX_NONE);
	if (TestTrue(TEXT("l'Overwatch e' nel piano"), B.Unit->IsAbilityInPlan(Sentinella)))
	{
		TestEqual(TEXT("e il piano spende quanto previsto"), B.Spent(), Prevista);
		TestEqual(TEXT("con un tratto solo"), B.Unit->PlannedMoveLegs(), 1);
	}
	DestroyApPreviewBench(B);
	return true;
}

/**
 * E38 CP 38.16, revisione della 38.16b — **un gesto che ricompone il percorso non lo porta oltre il residuo: i waypoint che non si pagano
 * cadono dalla fine.** Due gesti: lo scatto pianificato dopo il percorso ([D-628] (4)) e l'annullamento dello `Sneak` che restituisce i
 * waypoint tagliati ([D-444]).
 *
 * Gadget, tetto 10, tratto 5, budget 6. Waypoint a (5,0,0) e (10,0,0): 10 MP, un tratto.
 *  - Lo scatto: con la reazione (2) e il percorso (2) il piano spende 4; lo scatto (2) a (-1,0,0) sposta la partenza di una cella, e il
 *    percorso ricomposto costerebbe 11 MP, due tratti, 8 AP. Cade (10,0,0); resta (5,0,0), 6 MP, e il piano spende 6.
 *  - Lo `Sneak`: con l'attacco base (2) e il percorso, lo `Sneak` lo taglia a zero; armata la reazione (2), annullarlo restituirebbe i due
 *    waypoint, 10 MP e un tratto: 6 AP, che si pagano, quindi tornano entrambi. Controllo del taglio: col percorso a (12,0,0) al posto di
 *    (10,0,0) servirebbero due tratti, 8 AP, e (12,0,0) cade.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRecomposingDropsTheUnpaidWaypointsTest,
	"RefactorTactics.PlayerInput.RecomposingDropsTheUnpaidWaypoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRecomposingDropsTheUnpaidWaypointsTest::RunTest(const FString&)
{
	// --- Lo scatto dopo il percorso ---------------------------------------------------------------------------------
	{
		FApPreviewBench B = MakeApPreviewBench();
		if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		B.Unit->SetActionPointBudget(6);
		B.PC->SelectAbilityForCurrentForTest(B.Reazione);
		B.Unit->SelectAbility(INDEX_NONE);
		B.PC->HandleClickOnCellForTest(FRTCellId(5, 0, 0));
		B.PC->HandleClickOnCellForTest(FRTCellId(10, 0, 0));
		if (!TestEqual(TEXT("premessa: due waypoint"), B.Unit->GetPlan().PlannedWaypoints().Num(), 2)
			|| !TestEqual(TEXT("premessa: reazione e percorso spendono 4"), B.Spent(), 4))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		B.Unit->SelectAbility(B.Scatto);
		B.PC->HandleClickOnCellForTest(FRTCellId(-1, 0, 0));
		B.Unit->SelectAbility(INDEX_NONE);
		if (TestEqual(TEXT("premessa: lo scatto e' pianificato"), B.Unit->GetPlan().PlannedDashCell(), FRTCellId(-1, 0, 0)))
		{
			TestEqual(TEXT("scatto: il waypoint che apriva il secondo tratto cade"), B.Unit->GetPlan().PlannedWaypoints().Num(), 1);
			TestEqual(TEXT("scatto: resta il primo"), B.Unit->GetPlan().PlannedWaypoints().Last(), FRTCellId(5, 0, 0));
			TestEqual(TEXT("scatto: e il piano spende 6, nel budget"), B.Spent(), 6);
		}
		DestroyApPreviewBench(B);
	}

	// --- Lo Sneak annullato ---------------------------------------------------------------------------------------------
	for (const int32 Lontano : { 10, 12 })
	{
		FApPreviewBench B = MakeApPreviewBench();
		if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		const bool bSiPaga = Lontano == 10;
		B.Unit->SetActionPointBudget(6);
		B.Unit->DeclarePlannedMainAction(B.Attacco);
		B.Unit->SelectAbility(INDEX_NONE);
		B.PC->HandleClickOnCellForTest(FRTCellId(5, 0, 0));
		B.PC->HandleClickOnCellForTest(FRTCellId(Lontano, 0, 0));
		B.PC->ToggleSneakForTest();
		TestEqual(*FString::Printf(TEXT("Sneak (%d): il tetto taglia il percorso"), Lontano), B.Unit->GetPlan().PlannedWaypoints().Num(), 0);
		B.PC->SelectAbilityForCurrentForTest(B.Reazione);
		B.Unit->SelectAbility(INDEX_NONE);
		if (!TestEqual(*FString::Printf(TEXT("premessa (%d): attacco e reazione spendono 4"), Lontano), B.Spent(), 4))
		{
			DestroyApPreviewBench(B);
			return false;
		}
		B.PC->ToggleSneakForTest();
		TestEqual(*FString::Printf(TEXT("Sneak annullato (%d): tornano i waypoint che si pagano"), Lontano),
			B.Unit->GetPlan().PlannedWaypoints().Num(), bSiPaga ? 2 : 1);
		TestEqual(*FString::Printf(TEXT("Sneak annullato (%d): e il piano sta nel budget"), Lontano), B.Spent(), 6);
		DestroyApPreviewBench(B);
	}
	return true;
}

/**
 * HUD-AP.5a (#4159), criterio 1 — **lo stato `Planned` conta le copie nel piano, in ogni classe, e lo slot mostra il numero da 2 in su.**
 *
 * L'attacco base e' la principale, Guard sta due volte fra le azioni di Prep in piu' (cooldown 0, quindi legale). Fino a HUD-AP.5a lo
 * stato guardava la prima voce di tre classi, e Guard come voce di Prep non risultava pianificata: e' il controllo di cio' che cambia.
 * Lo slot si monta su una radice `Overlay`, come quella dell'asset: il contatore lo costruisce il C++ quando l'asset non lo dichiara.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlannedCountsEveryCopyTest,
	"RefactorTactics.HudViewModel.PlannedCountsEveryCopy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlannedCountsEveryCopyTest::RunTest(const FString&)
{
	FApPreviewBench B = MakeApPreviewBench();
	if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	B.Unit->DeclarePlannedMainAction(B.Attacco);
	FRTPlanEntry Guardia;
	Guardia.Class = ERTPlanEntryClass::Prep;
	Guardia.AbilityIndex = B.Guardia;
	TestTrue(TEXT("premessa: la prima Guard entra"), B.Unit->AddPlanEntry(Guardia));
	TestTrue(TEXT("premessa: la seconda Guard entra"), B.Unit->AddPlanEntry(Guardia));

	const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(B.Unit);
	if (!TestTrue(TEXT("le righe del kit"), Righe.IsValidIndex(B.Guardia) && Righe.IsValidIndex(B.Attacco) && Righe.IsValidIndex(B.Reazione)))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	TestEqual(TEXT("Guard: due copie"), Righe[B.Guardia].PlannedCount, 2);
	TestTrue(TEXT("Guard: pianificata, anche come voce di Prep"), Righe[B.Guardia].bPlanned);
	TestEqual(TEXT("l'attacco base: una copia"), Righe[B.Attacco].PlannedCount, 1);
	TestEqual(TEXT("la reazione: nessuna"), Righe[B.Reazione].PlannedCount, 0);
	TestFalse(TEXT("la reazione non e' pianificata"), Righe[B.Reazione].bPlanned);
	TestEqual(TEXT("Guard e' Planned"), static_cast<int32>(URTHudViewModel::ResolveSlotState(Righe[B.Guardia], /*bArmed=*/ false)),
		static_cast<int32>(ERTActionSlotState::Planned));

	// Lo slot: il contatore, costruito nella radice `Overlay`.
	URTActionSlotWidget* Slot = NewObject<URTActionSlotWidget>(B.World);
	Slot->WidgetTree = NewObject<UWidgetTree>(Slot, TEXT("WidgetTree"), RF_Transient);
	Slot->WidgetTree->RootWidget = Slot->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Radice"));
	Slot->SetAction(Righe[B.Guardia], /*bInArmed=*/ false);
	if (TestNotNull(TEXT("lo slot ha il contatore"), Slot->PlannedCountText.Get()))
	{
		TestNotNull(TEXT("ed e' montato nella radice"), Slot->PlannedCountText->GetParent());
		TestEqual(TEXT("e dice x2"), Slot->PlannedCountText->GetText().ToString(), FString(TEXT("\u00d72")));
		TestEqual(TEXT("acceso"), Slot->PlannedCountText->GetVisibility(), ESlateVisibility::HitTestInvisible);
		Slot->SetAction(Righe[B.Attacco], /*bInArmed=*/ false);
		TestEqual(TEXT("con una copia sola e' spento"), Slot->PlannedCountText->GetVisibility(), ESlateVisibility::Collapsed);
	}
	DestroyApPreviewBench(B);
	return true;
}

/**
 * HUD-AP.5a (#4159), criterio 2 — **un piano illegale per la regola della copia nomina l'occorrenza colpevole**, nell'avviso `Critical` e
 * nel tooltip dello slot.
 *
 * Counter ha una ricarica: due copie sono `RepeatedWithCooldown` ([D-614]), e il validatore accusa la seconda nell'ordine di dichiarazione
 * (`Plan.VerdictNamesTheOccurrence`). Il controllo e' lo stesso piano con una copia sola: legale, e nessun numero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusalNamesTheOccurrenceTest,
	"RefactorTactics.HudViewModel.RefusalNamesTheOccurrence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusalNamesTheOccurrenceTest::RunTest(const FString&)
{
	FApPreviewBench B = MakeApPreviewBench();
	if (!TestTrue(TEXT("il banco esiste"), B.IsValid()))
	{
		DestroyApPreviewBench(B);
		return false;
	}
	// La principale per prima, in posizione 0: le copie della reazione stanno in 1 e 2, e il numero atteso (2) non coincide con la loro
	// posizione nella lista (revisione di #4159). Budget 10: la principale e tre copie stanno nel budget, e il motivo resta la copia.
	B.Unit->SetActionPointBudget(10);
	B.Unit->DeclarePlannedMainAction(B.Attacco);
	B.PC->SelectAbilityForCurrentForTest(B.Reazione);
	B.Unit->SelectAbility(INDEX_NONE);
	const auto Righe = [&B]() { return URTHudViewModel::BuildAbilityCooldowns(B.Unit); };
	TestEqual(TEXT("controllo: una copia sola, nessuna occorrenza"), Righe()[B.Reazione].PlanInvalidOccurrence, 0);

	TestTrue(TEXT("premessa: la seconda copia entra nel piano"), B.Unit->AddPlannedReaction(B.Reazione));
	const FRTPlanValidation Verdetto = URTPlanValidationLibrary::ValidatePlan(ARTTurnManager::MakeSimUnit(INDEX_NONE, B.Unit),
		URTPlanValidationLibrary::MakePlanFor(B.Unit));
	if (!TestEqual(TEXT("premessa: la copia vietata"), static_cast<int32>(Verdetto.Reason),
		static_cast<int32>(ERTActionInvalidReason::RepeatedWithCooldown)))
	{
		DestroyApPreviewBench(B);
		return false;
	}

	const FRTAbilityCooldownView Riga = Righe()[B.Reazione];
	TestEqual(TEXT("la riga ha due copie"), Riga.PlannedCount, 2);
	TestTrue(TEXT("ed e' Invalid"), Riga.bPlanInvalid);
	TestEqual(TEXT("l'occorrenza colpevole e' la seconda"), Riga.PlanInvalidOccurrence, 2);
	const FString Tooltip = URTHudViewModel::BuildActionTooltip(Riga, /*bArmed=*/ false).Reason.ToString();
	TestTrue(*FString::Printf(TEXT("il tooltip nomina la seconda copia: %s"), *Tooltip), Tooltip.Contains(TEXT("2\u00aa copia")));

	// Una terza copia: il validatore accusa ancora la seconda, e il numero non e' il conto delle copie.
	TestTrue(TEXT("premessa: la terza copia entra nel piano"), B.Unit->AddPlannedReaction(B.Reazione));
	TestEqual(TEXT("con tre copie l'occorrenza colpevole resta la seconda"), Righe()[B.Reazione].PlanInvalidOccurrence, 2);
	TestEqual(TEXT("e il contatore dice tre"), Righe()[B.Reazione].PlannedCount, 3);

	const TArray<FRTPlanWarningView> Avvisi = URTHudViewModel::BuildPlanWarnings(B.Unit, { B.Unit });
	const FRTPlanWarningView* Critico = Avvisi.FindByPredicate([](const FRTPlanWarningView& A) { return A.Level == ERTPlanWarningLevel::Critical; });
	if (TestNotNull(TEXT("l'avviso Critical"), Critico))
	{
		TestTrue(*FString::Printf(TEXT("l'avviso nomina la seconda copia: %s"), *Critico->What.ToString()),
			Critico->What.ToString().Contains(TEXT("2\u00aa copia")));
	}
	DestroyApPreviewBench(B);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
