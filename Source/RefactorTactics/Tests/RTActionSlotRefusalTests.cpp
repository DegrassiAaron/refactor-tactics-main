// Gli stati `Invalid` e `Warning` dello slot e i loro PRODUTTORI ([D-459], #3483).
//
// 🔑 **Il banco e' un mondo vero, non una vista scritta a mano.** Una `FRTAbilityCooldownView` costruita nel test
// con `bPlanDegraded = true` proverebbe solo la precedenza, che ha gia' il suo test in `RTHudViewModelTests.cpp`.
// Qui si misura l'altra meta': che i campi si ACCENDANO quando il click rifiuterebbe, e restino spenti quando
// l'osservatore non sa — cioe' che lo slot rosso non diventi un rilevatore di presenze ([D-225]).
//
// L'arena e' quella di `MakeTestArena`: un muro alla vista lungo `q = 0`, `r = -2..2`. L'attaccante in `(-1,0)`
// vede `(-2,0)` e non vede `(1,0)` — le premesse lo chiedono al classificatore invece di darlo per scontato.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h" // ON_SCOPE_EXIT: il banco si distrugge anche sui return intermedi (#3636)
#include "Components/Border.h"
#include "EngineUtils.h" // TActorIterator: il turn manager del banco si trova, non si ricostruisce (#3622)
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // IsFastMovement: lo scatto che sposta l'origine del Blast
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPointerInteraction.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTPlanValidationLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTHUD.h" // RefusalText: il perche' dell'avviso degradato e' il testo del rifiuto dello slot (#3622)
#include "UI/RTScreenHudWidgets.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"
#include "UI/RTUIPalette.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Tests/RTAbilityFixtures.h" // AddCoreAbility: lo slot di reazione del pannello si prova su un'azione vera (#3636)

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	ARTUnit* SpawnSlotRefusalUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
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

	void MoveSlotRefusalUnit(ARTUnit* U, const FRTCellId& Cell)
	{
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	}

	/** Il mondo minimo in cui «il click rifiuterebbe» ha un senso: mappa, turn manager, controller, dock. */
	struct FSlotRefusalBench
	{
		UWorld* World = nullptr;
		URTHexMapAsset* Map = nullptr;
		ARTHexMapActor* MapActor = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Mine = nullptr;
		ARTUnit* Nemico = nullptr;
		URTActionDockWidget* Dock = nullptr;
	};

	const FRTCellId GSlotRefusalAttaccante(-1, 0, 0);
	const FRTCellId GSlotRefusalInVista(-2, 0, 0);    // adiacente, dalla stessa parte del muro
	const FRTCellId GSlotRefusalDietroIlMuro(1, 0, 0); // oltre il muro alla vista

	/** L'attacco base e' il primo del kit di Muiren, ed e' quello che punta un'unita' (`BlindFire`, test 7). */
	constexpr int32 GSlotRefusalAttacco = 0;

	bool SetUpSlotRefusalBench(FSlotRefusalBench& B)
	{
		B.World = RTWorldFixtures::MakeWorld();
		if (!B.World) { return false; }
		B.Map = URTMatchSetupLibrary::MakeTestArena(B.World);
		B.MapActor = B.World->SpawnActor<ARTHexMapActor>();
		if (!B.MapActor) { return false; }
		B.MapActor->MapAsset = B.Map;
		B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		B.Mine = SpawnSlotRefusalUnit(B.World, 0, URTHeroCatalogLibrary::MakeMuiren(), GSlotRefusalAttaccante);
		B.Nemico = SpawnSlotRefusalUnit(B.World, 1, URTHeroCatalogLibrary::MakeAevik(), GSlotRefusalInVista);
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		B.Dock = NewObject<URTActionDockWidget>(B.World);
		if (!B.Mine || !B.Nemico || !B.PC || !B.Dock) { return false; }
		// 🔴 **Il soggetto e' il RIFIUTO oltre il muro, e serve un attacco che la linea la CHIEDA.** Da D-490 (#3608)
		// l'attacco base mira al buio (`BlindAimDirect`): oltre il muro non si rifiuta piu', il colpo parte e il muro
		// lo ferma. Il banco dichiara `Required` sulla PROPRIA copia (`MakeMuiren` crea dati nuovi a ogni chiamata,
		// il kit spedito non cambia), e `SlotRefusalGeometryHolds` misura che il rifiuto c'e'.
		// ⌫ *Fino a #3608 i banchi usavano l'attacco base cosi' com'e'.*
		if (B.Mine->Abilities.IsValidIndex(GSlotRefusalAttacco) && B.Mine->Abilities[GSlotRefusalAttacco])
		{
			B.Mine->Abilities[GSlotRefusalAttacco]->Def.LineOfSightPolicy = ERTLineOfSightPolicy::Required;
		}
		B.PC->SelectActorForTest(B.Mine);
		B.Dock->SetCommandControllerForTest(B.PC);
		B.Dock->SetSelectedUnitForTest(B.Mine);
		B.Nemico->SetKnownToObserver(true);
		return true;
	}

	/** La riga del kit che la dock consegna allo slot: la stessa strada del gioco, non una vista costruita. */
	FRTAbilityCooldownView SlotRefusalRow(const FSlotRefusalBench& B, int32 Index)
	{
		const TArray<FRTAbilityCooldownView> Righe = B.Dock->GetActions();
		return Righe.IsValidIndex(Index) ? Righe[Index] : FRTAbilityCooldownView();
	}

	/** Le premesse geometriche, chieste al classificatore: senza, un verde potrebbe venire da un'arena diversa. */
	bool SlotRefusalGeometryHolds(FAutomationTestBase& Test, const FSlotRefusalBench& B, const URTActionData* A)
	{
		const ERTHexTargetReason InVista = URTCombatLibrary::ClassifyHexTargeting(B.Map, GSlotRefusalAttaccante,
			GSlotRefusalInVista, A->RangeCells, A->Def.LineOfSightPolicy);
		const ERTHexTargetReason Dietro = URTCombatLibrary::ClassifyHexTargeting(B.Map, GSlotRefusalAttaccante,
			GSlotRefusalDietroIlMuro, A->RangeCells, A->Def.LineOfSightPolicy);
		const bool bInVista = Test.TestEqual(TEXT("premessa: il bersaglio adiacente e' accettato"),
			InVista, ERTHexTargetReason::Ok);
		const bool bDietro = Test.TestTrue(TEXT("premessa: il bersaglio oltre il muro e' rifiutato"),
			Dietro != ERTHexTargetReason::Ok);
		return bInVista && bDietro;
	}
}

/**
 * 🔴 **LETTURA A: L'AZIONE ARMATA DICE `Invalid` QUANDO IL CLICK SOTTO IL PUNTATORE SAREBBE RIFIUTATO** ([D-459]).
 *
 * 🔑 **Il controllo 3 e' la ragione per cui il test esiste.** Lo stesso mondo del caso 2, con la sola
 * differenza che il nemico non e' noto: se lo slot si accendesse, il giocatore saprebbe che sulla cella puntata
 * c'e' qualcuno. ⚠️ Il caso 2 e' il controllo positivo che rende il 3 non vacuo: senza, «non si accende»
 * proverebbe soltanto che la dock non scrive mai il campo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSlotRefusalUnderPointerTest,
	"RefactorTactics.HudViewModel.ArmedSlotTurnsInvalidOnARefusedTargetUnderThePointer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSlotRefusalUnderPointerTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	const URTActionData* Attacco = B.Mine->GetAbility(GSlotRefusalAttacco);
	B.PC->ArmKitAbility(GSlotRefusalAttacco);
	if (!TestNotNull(TEXT("premessa: l'attacco esiste"), Attacco)
		|| !TestEqual(TEXT("premessa: l'attacco e' armato"), B.Mine->SelectedAbilityIndex, GSlotRefusalAttacco)
		|| !TestEqual(TEXT("premessa: e punta un'unita'"), B.PC->GetPointerTargetKind(), ERTPointerTargetKind::Unit)
		|| !SlotRefusalGeometryHolds(*this, B, Attacco))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// --- 1. il bersaglio puntato e' accettato: lo slot armato resta `Selected` -----------------------------
	B.MapActor->SetHoveredCell(GSlotRefusalInVista, /*bValid=*/ true);
	TestFalse(TEXT("1: un bersaglio accettato non accende il rifiuto"),
		SlotRefusalRow(B, GSlotRefusalAttacco).bTargetRefused);
	TestEqual(TEXT("1: e lo slot armato resta Selected"),
		URTHudViewModel::ResolveSlotState(SlotRefusalRow(B, GSlotRefusalAttacco), /*bArmed=*/ true),
		ERTActionSlotState::Selected);

	// --- 2. il nemico NOTO oltre il muro: il click rifiuterebbe, e lo slot lo dice prima --------------------
	MoveSlotRefusalUnit(B.Nemico, GSlotRefusalDietroIlMuro);
	B.MapActor->SetHoveredCell(GSlotRefusalDietroIlMuro, /*bValid=*/ true);
	TestTrue(TEXT("2: un bersaglio noto e rifiutato accende il rifiuto sull'azione armata"),
		SlotRefusalRow(B, GSlotRefusalAttacco).bTargetRefused);
	TestEqual(TEXT("2: e lo slot armato diventa Invalid"),
		URTHudViewModel::ResolveSlotState(SlotRefusalRow(B, GSlotRefusalAttacco), /*bArmed=*/ true),
		ERTActionSlotState::Invalid);
	for (const FRTAbilityCooldownView& Riga : B.Dock->GetActions())
	{
		if (Riga.AbilityIndex != GSlotRefusalAttacco)
		{
			TestFalse(*FString::Printf(TEXT("2: la riga %d, non armata, non porta il rifiuto del puntatore"),
				Riga.AbilityIndex), Riga.bTargetRefused);
		}
	}

	// --- 3. ⛔ lo STESSO mondo, col nemico ignoto: niente si accende -------------------------------------------
	B.Nemico->SetKnownToObserver(false);
	TestFalse(TEXT("3: un'unita' ignota sotto il puntatore non accende niente (D-225)"),
		SlotRefusalRow(B, GSlotRefusalAttacco).bTargetRefused);
	TestEqual(TEXT("3: lo slot resta Selected, come su una cella vuota"),
		URTHudViewModel::ResolveSlotState(SlotRefusalRow(B, GSlotRefusalAttacco), /*bArmed=*/ true),
		ERTActionSlotState::Selected);

	// --- 4. il puntatore fuori dalla mappa: nessuna domanda, nessuna risposta ---------------------------------
	B.Nemico->SetKnownToObserver(true);
	B.MapActor->SetHoveredCell(GSlotRefusalDietroIlMuro, /*bValid=*/ false);
	TestFalse(TEXT("4: senza una cella puntata valida il rifiuto resta spento"),
		SlotRefusalRow(B, GSlotRefusalAttacco).bTargetRefused);

	// --- 5. disarmata: il rifiuto del puntatore non ha a chi appartenere -------------------------------------
	B.MapActor->SetHoveredCell(GSlotRefusalDietroIlMuro, /*bValid=*/ true);
	B.PC->ArmKitAbility(GSlotRefusalAttacco); // ricliccare lo slot armato disarma (D-128)
	if (TestEqual(TEXT("5: premessa — nessuna azione armata"), B.Mine->SelectedAbilityIndex, (int32)INDEX_NONE))
	{
		for (const FRTAbilityCooldownView& Riga : B.Dock->GetActions())
		{
			TestFalse(*FString::Printf(TEXT("5: la riga %d non porta il rifiuto"), Riga.AbilityIndex),
				Riga.bTargetRefused);
		}
	}

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * 🔴 **LETTURA A SU UN'AREA: LA DOMANDA E' QUELLA DEL CLICK SU UNA CELLA** ([D-459], #3483).
 *
 * 🔑 Un'azione ad area si centra su una cella, e il click la giudica con `DescribeCellTargetRefusal`, non
 * con la coppia delle unita'. Le due celle si CERCANO sulla mappa chiedendo a quella porta: il test non
 * dichiara la portata di nessuna azione, segue il dato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSlotRefusalAreaCellTest,
	"RefactorTactics.HudViewModel.ArmedAreaSlotReadsTheCellGateOfTheClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSlotRefusalAreaCellTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	int32 Area = INDEX_NONE;
	for (int32 i = 0; i < B.Mine->NumAbilities(); ++i)
	{
		const URTActionData* A = B.Mine->GetAbility(i);
		if (A && !A->bSelfTarget && A->Shape == ERTAbilityShape::Area && A->Def.StructureOp == ERTStructureOp::None)
		{
			Area = i;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: il kit ha un'azione ad area"), Area != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const URTActionData* A = B.Mine->GetAbility(Area);
	B.PC->ArmKitAbility(Area);
	if (!TestEqual(TEXT("premessa: l'area punta una cella"), B.PC->GetPointerTargetKind(), ERTPointerTargetKind::Cell))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// Le due celle, chieste alla porta del click.
	const FRTCellId* Accettata = nullptr;
	const FRTCellId* Rifiutata = nullptr;
	for (const FRTHexCellData& C : B.Map->Cells)
	{
		const ERTTargetRefusal R = URTCombatLibrary::DescribeCellTargetRefusal(
			B.Map, GSlotRefusalAttaccante, C.Id, A->RangeCells, A->Def.LineOfSightPolicy).Refusal;
		if (!Accettata && R == ERTTargetRefusal::None) { Accettata = &C.Id; }
		if (!Rifiutata && R != ERTTargetRefusal::None && R != ERTTargetRefusal::Nothing) { Rifiutata = &C.Id; }
	}
	if (!TestNotNull(TEXT("premessa: una cella che il click accetta"), Accettata)
		|| !TestNotNull(TEXT("premessa: una cella che il click rifiuta"), Rifiutata))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	B.MapActor->SetHoveredCell(*Accettata, /*bValid=*/ true);
	TestFalse(TEXT("una cella accettata non accende il rifiuto"), SlotRefusalRow(B, Area).bTargetRefused);

	B.MapActor->SetHoveredCell(*Rifiutata, /*bValid=*/ true);
	TestTrue(TEXT("una cella rifiutata lo accende"), SlotRefusalRow(B, Area).bTargetRefused);
	TestEqual(TEXT("e lo slot armato diventa Invalid"),
		URTHudViewModel::ResolveSlotState(SlotRefusalRow(B, Area), /*bArmed=*/ true), ERTActionSlotState::Invalid);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * 🔴 **LETTURA B, WARNING: IL PIANO E' ACCETTATO MA IL BERSAGLIO, ALLO STATO NOTO, PRENDEREBBE IL RIPIEGO**.
 *
 * 🔑 Il bersaglio si pianifica col click vero, quando e' in vista; poi si sposta oltre il muro (caso 2), o
 * resta fermo mentre uno scatto pianificato sposta l'origine del Blast (caso 4). Il caso 4 e' quello che il
 * giocatore incontra davvero: in planning nessuno si muove, ed e' lo scatto a separare «accettato» da
 * «degradato».
 *
 * ⛔ **Il caso 3 e' il presidio di privacy**: lo stesso bersaglio, non piu' noto, non accende niente. Il
 * Warning su un'ombra direbbe «l'hai perso di vista, ed e' finito in un posto da cui non lo colpisci».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSlotRefusalDegradedPlanTest,
	"RefactorTactics.HudViewModel.DegradedPlanWarnsOnlyOnAKnownTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSlotRefusalDegradedPlanTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	const URTActionData* Attacco = B.Mine->GetAbility(GSlotRefusalAttacco);
	B.PC->ArmKitAbility(GSlotRefusalAttacco);
	B.PC->HandleClickOnUnitForTest(B.Nemico);
	if (!TestNotNull(TEXT("premessa: l'attacco esiste"), Attacco)
		|| !TestTrue(TEXT("premessa: il click ha pianificato l'attacco sul nemico"),
			B.Mine->PlannedAttackTarget == B.Nemico && B.Mine->PlannedAbilityIndex == GSlotRefusalAttacco)
		|| !SlotRefusalGeometryHolds(*this, B, Attacco))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	auto Riga = [&B]() {
		const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(B.Mine);
		return Righe.IsValidIndex(GSlotRefusalAttacco) ? Righe[GSlotRefusalAttacco] : FRTAbilityCooldownView();
	};

	// --- 1. il bersaglio e' dove lo si e' pianificato: nessun Warning -----------------------------------
	TestFalse(TEXT("1: un bersaglio in vista non degrada il piano"), Riga().bPlanDegraded);
	TestEqual(TEXT("1: lo slot dice Planned"),
		URTHudViewModel::ResolveSlotState(Riga(), /*bArmed=*/ false), ERTActionSlotState::Planned);

	// --- 2. il bersaglio NOTO e' finito oltre il muro: il piano resta, ma prenderebbe il ripiego ---------
	MoveSlotRefusalUnit(B.Nemico, GSlotRefusalDietroIlMuro);
	TestTrue(TEXT("2: il bersaglio noto oltre il muro degrada il piano"), Riga().bPlanDegraded);
	TestEqual(TEXT("2: lo slot dice Warning"),
		URTHudViewModel::ResolveSlotState(Riga(), /*bArmed=*/ false), ERTActionSlotState::Warning);
	for (const FRTAbilityCooldownView& Altra : URTHudViewModel::BuildAbilityCooldowns(B.Mine))
	{
		if (Altra.AbilityIndex != GSlotRefusalAttacco)
		{
			TestFalse(*FString::Printf(TEXT("2: la riga %d, non pianificata su un'unita', resta senza Warning"),
				Altra.AbilityIndex), Altra.bPlanDegraded);
		}
	}

	// --- 3. ⛔ lo stesso mondo, col bersaglio ignoto: niente -------------------------------------------------
	B.Nemico->SetKnownToObserver(false);
	TestFalse(TEXT("3: un bersaglio ignoto non accende il Warning (D-225)"), Riga().bPlanDegraded);
	TestEqual(TEXT("3: lo slot torna a dire Planned"),
		URTHudViewModel::ResolveSlotState(Riga(), /*bArmed=*/ false), ERTActionSlotState::Planned);

	// --- 4. 🔴 IL CASO DI GIOCO: il bersaglio non si muove, ma lo SCATTO pianificato sposta l'origine -------
	// In planning nessuno si muove: e' il caso che rende il Warning raggiungibile dal giocatore. Il click ha
	// accettato dalla cella corrente; il Blast partira' dalla cella dello scatto (`BlastOriginCell`).
	B.Nemico->SetKnownToObserver(true);
	MoveSlotRefusalUnit(B.Nemico, GSlotRefusalInVista);
	int32 Scatto = INDEX_NONE;
	for (int32 i = 0; i < B.Mine->NumAbilities(); ++i)
	{
		const URTActionData* A = B.Mine->GetAbility(i);
		if (A && URTCatalogLibrary::IsFastMovement(A->Def) && B.Mine->CanUseAbility(i))
		{
			Scatto = i;
			break;
		}
	}
	// Due celle d'arrivo, chieste al classificatore: una da cui il bersaglio resta colpibile, una da cui no.
	const FRTCellId* ArrivoBuono = nullptr;
	const FRTCellId* ArrivoCattivo = nullptr;
	for (const FRTHexCellData& C : B.Map->Cells)
	{
		if (C.Id == GSlotRefusalAttaccante || C.Id == GSlotRefusalInVista) { continue; }
		const ERTHexTargetReason R = URTCombatLibrary::ClassifyHexTargeting(
			B.Map, C.Id, GSlotRefusalInVista, Attacco->RangeCells, Attacco->Def.LineOfSightPolicy);
		if (!ArrivoBuono && R == ERTHexTargetReason::Ok) { ArrivoBuono = &C.Id; }
		if (!ArrivoCattivo && R != ERTHexTargetReason::Ok) { ArrivoCattivo = &C.Id; }
	}
	if (TestTrue(TEXT("4: premessa — il kit ha uno scatto pronto"), Scatto != INDEX_NONE)
		&& TestNotNull(TEXT("4: premessa — un arrivo da cui il bersaglio resta colpibile"), ArrivoBuono)
		&& TestNotNull(TEXT("4: premessa — un arrivo da cui non lo e' piu'"), ArrivoCattivo))
	{
		B.Mine->PlannedDashAbility = Scatto;

		B.Mine->PlannedDashCell = *ArrivoBuono;
		TestTrue(TEXT("4: premessa — lo scatto si applica"), B.Mine->PlannedDashApplies());
		TestFalse(TEXT("4a: uno scatto che lascia il bersaglio colpibile non degrada il piano"),
			Riga().bPlanDegraded);

		B.Mine->PlannedDashCell = *ArrivoCattivo;
		TestTrue(TEXT("4: premessa — lo scatto si applica anche qui"), B.Mine->PlannedDashApplies());
		TestTrue(TEXT("4b: uno scatto che porta fuori tiro degrada il piano accettato dal click"),
			Riga().bPlanDegraded);
		TestEqual(TEXT("4b: lo slot dice Warning"),
			URTHudViewModel::ResolveSlotState(Riga(), /*bArmed=*/ false), ERTActionSlotState::Warning);
	}

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * 🔴 **LETTURA B, INVALID: IL PIANO ILLEGALE ACCENDE SOLO LO SLOT DELLA COLPEVOLE**.
 *
 * 🔑 **Il colpevole lo nomina il validatore, non il test**: `OffendingActionId` e' la testimonianza, e
 * l'asserzione e' che la vista la porti sulla riga con quell'`ActionId` e su nessun'altra. Il controllo 1
 * — lo stesso piano, legale — impedisce che un `bPlanInvalid` sempre vero passi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSlotRefusalIllegalPlanTest,
	"RefactorTactics.HudViewModel.IllegalPlanMarksOnlyTheOffendingSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSlotRefusalIllegalPlanTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// Un'azione che ha una ricarica: e' quella che `ConsumeAbility` puo' rendere illegale nel piano.
	int32 ConRicarica = INDEX_NONE;
	for (int32 i = 0; i < B.Mine->NumAbilities(); ++i)
	{
		const URTActionData* A = B.Mine->GetAbility(i);
		if (A && A->CooldownTurns > 0 && !A->Def.ActionId.IsNone())
		{
			ConRicarica = i;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: il kit ha un'azione con ricarica"), ConRicarica != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const FName Colpevole = B.Mine->GetAbility(ConRicarica)->Def.ActionId;
	B.Mine->PlannedAbilityIndex = ConRicarica;

	// --- 1. controllo: lo stesso piano, legale ------------------------------------------------------------
	const FRTPlanValidation Legale = URTPlanValidationLibrary::ValidatePlan(
		FRTHexSimUnit(), URTPlanValidationLibrary::MakePlanFor(B.Mine));
	TestTrue(TEXT("1: premessa — il piano e' legale"), Legale.bLegal);
	for (const FRTAbilityCooldownView& Riga : URTHudViewModel::BuildAbilityCooldowns(B.Mine))
	{
		TestFalse(*FString::Printf(TEXT("1: la riga %d non e' marcata da un piano legale"), Riga.AbilityIndex),
			Riga.bPlanInvalid);
	}

	// --- 2. la stessa azione, ora in ricarica: il validatore la nomina colpevole --------------------------
	B.Mine->ConsumeAbility(ConRicarica);
	const FRTPlanValidation Illegale = URTPlanValidationLibrary::ValidatePlan(
		FRTHexSimUnit(), URTPlanValidationLibrary::MakePlanFor(B.Mine));
	if (!TestFalse(TEXT("2: premessa — il piano e' illegale"), Illegale.bLegal)
		|| !TestEqual(TEXT("2: premessa — e il validatore nomina quell'azione"), Illegale.OffendingActionId, Colpevole))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	for (const FRTAbilityCooldownView& Riga : URTHudViewModel::BuildAbilityCooldowns(B.Mine))
	{
		const bool bColpevole = Riga.ActionId == Colpevole;
		TestEqual(*FString::Printf(TEXT("2: la riga %d e' marcata se e solo se porta la colpevole"), Riga.AbilityIndex),
			Riga.bPlanInvalid, bColpevole);
		if (bColpevole)
		{
			TestEqual(TEXT("2: e lo slot della colpevole dice Invalid, non Planned ne' Cooldown"),
				URTHudViewModel::ResolveSlotState(Riga, /*bArmed=*/ false), ERTActionSlotState::Invalid);
		}
	}

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * ⛔ **IL TOOLTIP NOMINA IL RIFIUTO DI UN BERSAGLIO DEGRADATO SOLO SE IL BERSAGLIO E' NOTO** — `#3499`, DoD 3, [D-225].
 *
 * 🔑 Lo stesso mondo di `DegradedPlanWarnsOnlyOnAKnownTarget`. Il caso noto e' il controllo positivo che rende
 * l'altro non vacuo: senza, «il tooltip tace» proverebbe soltanto che il motivo non si scrive mai.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSlotRefusalTooltipPrivacyTest,
	"RefactorTactics.HudViewModel.TooltipNamesADegradedTargetOnlyWhenKnown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSlotRefusalTooltipPrivacyTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const URTActionData* Attacco = B.Mine->GetAbility(GSlotRefusalAttacco);
	B.PC->ArmKitAbility(GSlotRefusalAttacco);
	B.PC->HandleClickOnUnitForTest(B.Nemico);
	if (!TestNotNull(TEXT("premessa: l'attacco esiste"), Attacco)
		|| !TestTrue(TEXT("premessa: il click ha pianificato l'attacco sul nemico"),
			B.Mine->PlannedAttackTarget == B.Nemico && B.Mine->PlannedAbilityIndex == GSlotRefusalAttacco)
		|| !SlotRefusalGeometryHolds(*this, B, Attacco))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	auto Riga = [&B]() {
		const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(B.Mine);
		return Righe.IsValidIndex(GSlotRefusalAttacco) ? Righe[GSlotRefusalAttacco] : FRTAbilityCooldownView();
	};

	MoveSlotRefusalUnit(B.Nemico, GSlotRefusalDietroIlMuro);
	const FRTAbilityCooldownView Noto = Riga();
	TestEqual(TEXT("noto oltre il muro: la riga porta il rifiuto"), Noto.PlanDegradedRefusal, ERTTargetRefusal::Cover);
	TestTrue(TEXT("e il tooltip lo nomina"),
		URTHudViewModel::BuildActionTooltip(Noto, /*bArmed=*/ false).Reason.ToString().StartsWith(TEXT("Coperto")));

	B.Nemico->SetKnownToObserver(false);
	const FRTAbilityCooldownView Ignoto = Riga();
	TestEqual(TEXT("ignoto: la riga non porta nessun rifiuto"), Ignoto.PlanDegradedRefusal, ERTTargetRefusal::None);
	TestTrue(TEXT("⛔ e il tooltip non dice niente"),
		URTHudViewModel::BuildActionTooltip(Ignoto, /*bArmed=*/ false).Reason.IsEmpty());

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

namespace
{
	/** La prima azione del kit con una ricarica: pianificata e consumata, rende il piano illegale per il validatore. */
	int32 SlotRefusalCooldownAbility(const ARTUnit* Unit)
	{
		for (int32 i = 0; Unit && i < Unit->NumAbilities(); ++i)
		{
			const URTActionData* A = Unit->GetAbility(i);
			if (A && A->CooldownTurns > 0 && !A->Def.ActionId.IsNone())
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	ARTTurnManager* SlotRefusalTurnManager(UWorld* World)
	{
		for (TActorIterator<ARTTurnManager> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}
}

/**
 * 🔑 **GLI AVVISI DI PIANO RIPORTANO LE FONTI CHE ESISTONO** ([D-480], #3622): il validatore per il `Critical`, il
 * ripiego dello slot per il `Warning`. Lo stesso banco dei test dello slot, cosi' le fonti sono quelle che lo slot
 * gia' legge: un avviso che le ricalcolasse potrebbe dire altro dallo slot.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanWarningsSourcesTest,
	"RefactorTactics.HudViewModel.PlanWarningsReportTheValidatorAndTheFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanWarningsSourcesTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const TArray<ARTUnit*> Units = { B.Mine, B.Nemico };

	const URTActionData* Attacco = B.Mine->GetAbility(GSlotRefusalAttacco);
	B.PC->ArmKitAbility(GSlotRefusalAttacco);
	B.PC->HandleClickOnUnitForTest(B.Nemico);
	if (!TestNotNull(TEXT("premessa: l'attacco esiste"), Attacco)
		|| !TestTrue(TEXT("premessa: il click ha pianificato l'attacco sul nemico"), B.Mine->PlannedAttackTarget == B.Nemico)
		|| !SlotRefusalGeometryHolds(*this, B, Attacco))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// --- 1. Un piano pulito non da' avvisi -----------------------------------------------------------------------
	TestEqual(TEXT("1: un attacco su un bersaglio in vista non da' avvisi"),
		URTHudViewModel::BuildPlanWarnings(B.Mine, Units).Num(), 0);

	// --- 2. Il bersaglio noto oltre il muro: lo slot dice Warning, e l'avviso lo riporta --------------------------
	MoveSlotRefusalUnit(B.Nemico, GSlotRefusalDietroIlMuro);
	{
		const TArray<FRTPlanWarningView> Avvisi = URTHudViewModel::BuildPlanWarnings(B.Mine, Units);
		if (TestEqual(TEXT("2: un avviso"), Avvisi.Num(), 1))
		{
			TestEqual(TEXT("2: e' un Warning"), Avvisi[0].Level, ERTPlanWarningLevel::Warning);
			TestEqual(TEXT("2: attribuito all'attacco"), Avvisi[0].SourceActionId, Attacco->Def.ActionId);
			const FRTAbilityCooldownView Riga = URTHudViewModel::BuildAbilityCooldowns(B.Mine)[GSlotRefusalAttacco];
			TestTrue(TEXT("2: premessa — lo slot e' degradato"), Riga.bPlanDegraded);
			TestEqual(TEXT("2: il perche' e' il testo del rifiuto dello slot"), Avvisi[0].Why.ToString(),
				ARTHUD::RefusalText(Riga.PlanDegradedRefusal, Riga.PlanDegradedRange));
			TestFalse(TEXT("2: il costo e' detto"), Avvisi[0].Cost.IsEmpty());
		}
	}
	MoveSlotRefusalUnit(B.Nemico, GSlotRefusalInVista);

	// --- 3. Un'azione in ricarica pianificata: il validatore lo rifiuta, ed e' un Critical -------------------------
	const int32 ConRicarica = SlotRefusalCooldownAbility(B.Mine);
	if (!TestTrue(TEXT("premessa: il kit ha un'azione con ricarica"), ConRicarica != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	B.Mine->PlannedAbilityIndex = ConRicarica;
	B.Mine->ConsumeAbility(ConRicarica);
	const FRTPlanValidation Verdetto = URTPlanValidationLibrary::ValidatePlan(
		FRTHexSimUnit(), URTPlanValidationLibrary::MakePlanFor(B.Mine));
	if (!TestFalse(TEXT("premessa: il piano e' illegale"), Verdetto.bLegal))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const TArray<FRTPlanWarningView> Avvisi = URTHudViewModel::BuildPlanWarnings(B.Mine, Units);
	const FRTPlanWarningView* Critico = Avvisi.FindByPredicate([](const FRTPlanWarningView& A)
	{
		return A.Level == ERTPlanWarningLevel::Critical;
	});
	if (TestNotNull(TEXT("3: c'e' un Critical"), Critico))
	{
		TestEqual(TEXT("3: attribuito all'azione che il validatore nomina"), Critico->SourceActionId, Verdetto.OffendingActionId);
		TestFalse(TEXT("3: il perche' e' detto"), Critico->Why.IsEmpty());
		TestFalse(TEXT("3: il costo e' detto"), Critico->Cost.IsEmpty());
	}
	TestEqual(TEXT("3: il Critical viene per primo"), Avvisi.Num() > 0 ? Avvisi[0].Level : ERTPlanWarningLevel::Info,
		ERTPlanWarningLevel::Critical);

	// --- 3-bis. Una voce per azione: con il bersaglio dietro il muro l'azione illegale non porta anche il ripiego --
	MoveSlotRefusalUnit(B.Nemico, GSlotRefusalDietroIlMuro);
	TestEqual(TEXT("3-bis: un'azione illegale con il bersaglio dietro il muro da' una voce sola"),
		URTHudViewModel::BuildPlanWarnings(B.Mine, Units).Num(), 1);
	MoveSlotRefusalUnit(B.Nemico, GSlotRefusalInVista);

	// --- 4. Il conteggio per livello -------------------------------------------------------------------------------
	FRTPlanWarningCounts Conteggi;
	URTHudViewModel::AddPlanWarningCounts(Avvisi, Conteggi);
	TestEqual(TEXT("4: un Critical nel conteggio"), Conteggi.Critical, 1);
	TestEqual(TEXT("4: nessun Info"), Conteggi.Info, 0);

	// --- 5. Il conflitto di slot piu' comune, scatto + movimento: il perche' nomina le due azioni per nome ---------
	B.Mine->PlannedAbilityIndex = INDEX_NONE;
	B.Mine->ClearPlannedAttack();
	int32 Scatto = INDEX_NONE;
	for (int32 i = 0; i < B.Mine->NumAbilities(); ++i)
	{
		const URTActionData* A = B.Mine->GetAbility(i);
		if (A && URTCatalogLibrary::IsFastMovement(A->Def) && B.Mine->CanUseAbility(i))
		{
			Scatto = i;
			break;
		}
	}
	if (!TestTrue(TEXT("premessa: il kit ha uno scatto utilizzabile"), Scatto != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	B.Mine->PlannedDashAbility = Scatto;
	B.Mine->PlannedDashCell = FRTCellId(-1, 1, 0);
	// Il movimento nel piano lo dice `HasPlannedNormalMove()`: la cella pianificata, non i waypoint.
	B.Mine->PlannedCell = FRTCellId(0, -1, 0);
	const FRTPlanValidation Conflitto = URTPlanValidationLibrary::ValidatePlan(
		FRTHexSimUnit(), URTPlanValidationLibrary::MakePlanFor(B.Mine));
	if (!TestFalse(TEXT("premessa: scatto + movimento e' illegale"), Conflitto.bLegal)
		|| !TestFalse(TEXT("premessa: e il validatore nomina le due azioni"), Conflitto.HolderActionId.IsNone()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const TArray<FRTPlanWarningView> DiConflitto = URTHudViewModel::BuildPlanWarnings(B.Mine, Units);
	if (TestEqual(TEXT("5: una voce"), DiConflitto.Num(), 1))
	{
		const FString Perche = DiConflitto[0].Why.ToString();
		TestFalse(*FString::Printf(TEXT("5: il perche' non mostra id grezzi (%s)"), *Perche), Perche.Contains(TEXT("Action.")));
		TestFalse(*FString::Printf(TEXT("5: il cosa non mostra id grezzi (%s)"), *DiConflitto[0].What.ToString()),
			DiConflitto[0].What.ToString().Contains(TEXT(".")));
		TestTrue(*FString::Printf(TEXT("5: il perche' nomina lo scatto (%s)"), *Perche),
			Perche.Contains(B.Mine->GetAbility(Scatto)->DisplayName.ToString()));
	}

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * ⛔ **GLI AVVISI SEGUONO IL COMANDO E TACCIONO IN RISOLUZIONE** ([D-480] punti 1 e 5, [D-494], #3622).
 *
 * - Il contatore accanto a `Conferma` somma le unita' COMANDATE: un'avversaria con un piano illegale non entra, e
 *   il test prova che per lei l'avviso esisterebbe.
 * - In Risoluzione il piano e' gia' stato consumato, quindi un controllo «vuoto» sarebbe vacuo. Il test scrive un
 *   piano illegale DURANTE la risoluzione: la vista diretta lo vede, i due widget devono tacere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanWarningsWidgetsTest,
	"RefactorTactics.ScreenHud.PlanWarningsFollowTheCommandAndHideInResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanWarningsWidgetsTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	ARTTurnManager* TM = SlotRefusalTurnManager(B.World);
	URTPlanCommitWidget* Conferma = NewObject<URTPlanCommitWidget>(B.World);
	URTSelectedUnitPanelWidget* Pannello = NewObject<URTSelectedUnitPanelWidget>(B.World);
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("conferma"), Conferma) || !TestNotNull(TEXT("pannello"), Pannello))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	Conferma->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Conferma->SetControlGroupForTest(0);
	Pannello->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Pannello->SetSelectedUnitForTest(B.Mine);
	const TArray<ARTUnit*> Units = { B.Mine, B.Nemico };

	// Due piani illegali, uno per squadra.
	const int32 MiaRicarica = SlotRefusalCooldownAbility(B.Mine);
	const int32 SuaRicarica = SlotRefusalCooldownAbility(B.Nemico);
	if (!TestTrue(TEXT("premessa: entrambi i kit hanno una ricarica"), MiaRicarica != INDEX_NONE && SuaRicarica != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	B.Mine->PlannedAbilityIndex = MiaRicarica;
	B.Mine->ConsumeAbility(MiaRicarica);
	B.Nemico->PlannedAbilityIndex = SuaRicarica;
	B.Nemico->ConsumeAbility(SuaRicarica);

	FRTPlanWarningCounts Sua;
	URTHudViewModel::AddPlanWarningCounts(URTHudViewModel::BuildPlanWarnings(B.Nemico, Units), Sua);
	if (!TestEqual(TEXT("premessa: il piano dell'avversaria darebbe un Critical"), Sua.Critical, 1))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// --- 1. Il comando ---------------------------------------------------------------------------------------------
	TestEqual(TEXT("1: il contatore conta il Critical della comandata, non quello dell'avversaria"),
		Conferma->GetPlanWarningCounts().Critical, 1);
	TestTrue(TEXT("1: il pannello elenca l'avviso dell'unita' comandata"), Pannello->GetPlanWarnings().Num() > 0);

	// --- 2. La risoluzione -----------------------------------------------------------------------------------------
	// Un piano che si risolve davvero: l'attacco sul nemico in vista, cosi' il playback dura qualche tick.
	B.Mine->ClearPlannedAttack();
	B.Mine->PlannedAbilityIndex = GSlotRefusalAttacco;
	B.Mine->PlannedAttackTarget = B.Nemico;
	B.Nemico->PlannedAbilityIndex = INDEX_NONE;
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: il turno e' in risoluzione"), TM->IsResolving()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	// Il piano illegale scritto DURANTE la risoluzione: la vista diretta lo vede.
	B.Mine->PlannedAbilityIndex = MiaRicarica;
	if (!TestTrue(TEXT("premessa: in risoluzione la vista diretta vede un avviso"),
			URTHudViewModel::BuildPlanWarnings(B.Mine, Units).Num() > 0))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	TestEqual(TEXT("2: in risoluzione il pannello non elenca avvisi"), Pannello->GetPlanWarnings().Num(), 0);
	const FRTPlanWarningCounts InRisoluzione = Conferma->GetPlanWarningCounts();
	TestEqual(TEXT("2: in risoluzione il contatore e' a zero"),
		InRisoluzione.Critical + InRisoluzione.Warning + InRisoluzione.Info, 0);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * ⛔ **IN RISOLUZIONE IL RIQUADRO DI CONFERMA SI NASCONDE** ([D-480] punto 1, #3633). Lo stesso banco del test degli
 * avvisi in Risoluzione: un turno vero, che il playback tiene in Risoluzione per qualche tick.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanCommitHidesInResolutionTest,
	"RefactorTactics.ScreenHud.PlanCommitHidesInResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanCommitHidesInResolutionTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	ARTTurnManager* TM = SlotRefusalTurnManager(B.World);
	URTPlanCommitWidget* Conferma = NewObject<URTPlanCommitWidget>(B.World);
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("conferma"), Conferma))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	Conferma->CommitRoot = NewObject<UBorder>(Conferma);
	Conferma->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	Conferma->SetControlGroupForTest(0);

	Conferma->RefreshButtons();
	TestTrue(TEXT("in Pianificazione il riquadro c'e'"),
		Conferma->CommitRoot->GetVisibility() != ESlateVisibility::Collapsed);

	B.Mine->PlannedAbilityIndex = GSlotRefusalAttacco;
	B.Mine->PlannedAttackTarget = B.Nemico;
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: il turno e' in risoluzione"), TM->IsResolving()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	Conferma->RefreshButtons();
	TestEqual(TEXT("in Risoluzione il riquadro e' chiuso"), Conferma->CommitRoot->GetVisibility(), ESlateVisibility::Collapsed);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * 🔑 **I CHIP E GLI AVVISI DEL PANNELLO SEGUONO IL PIANO DELLA COMANDATA, E TACCIONO PER L'ISPEZIONATA** (#3636, T8,
 * [D-480]).
 *
 * Lo stesso banco di `PlanWarningsFollowTheCommandAndHideInResolution`: un'azione con ricarica pianificata mentre e'
 * in ricarica da' un Critical che nomina quell'azione.
 *  1. il chip PRINCIPALE ha il nome dell'azione, la striscia della sua fase e il «!»; gli slot liberi dicono «—»;
 *  2. l'elenco ha una voce per avviso, con la forma del livello;
 *  3. un'unita' ispezionata — che un piano ce l'ha — non porta ne' chip ne' avvisi;
 *  4. in Risoluzione i chip si chiudono.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelChipsAndWarningsTest,
	"RefactorTactics.ScreenHud.PanelChipsAndWarningsFollowThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelChipsAndWarningsTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	ARTTurnManager* TM = SlotRefusalTurnManager(B.World);
	URTSelectedUnitPanelWidget* P = NewObject<URTSelectedUnitPanelWidget>(B.World);
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("pannello"), P))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	P->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	P->SetSelectedUnitForTest(B.Mine);

	const auto Uguale = [](const FText& Testo, const FString& Atteso) { return Testo.ToString().Equals(Atteso, ESearchCase::CaseSensitive); };
	const auto Visibile = [](const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed; };
	const auto Colore = [](ERTUIToken Token) { return URTUIPalette::ColorFor(Token); };
	const auto Cornice = [P]()
	{
		UBorder* B = NewObject<UBorder>(P);
		FSlateBrush Brush = B->Background;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		B->SetBrush(Brush);
		return B;
	};
	P->PlanChipsBox = NewObject<UBorder>(P);
	const auto Porte = [&](TObjectPtr<UBorder>& Frame, TObjectPtr<UBorder>& Strip, TObjectPtr<UTextBlock>& Valore, TObjectPtr<UWidget>& Avviso)
	{
		Frame = Cornice();
		Strip = NewObject<UBorder>(P);
		Valore = NewObject<UTextBlock>(P);
		Avviso = NewObject<UBorder>(P);
	};
	Porte(P->MainChipFrame, P->MainChipStrip, P->MainChipValueText, P->MainChipAlert);
	Porte(P->MovementChipFrame, P->MovementChipStrip, P->MovementChipValueText, P->MovementChipAlert);
	Porte(P->ReactionChipFrame, P->ReactionChipStrip, P->ReactionChipValueText, P->ReactionChipAlert);
	UVerticalBox* Elenco = NewObject<UVerticalBox>(P);
	P->PlanWarningsBox = Elenco;
	P->PlanWarningEntryClass = URTPlanWarningEntryWidget::StaticClass();

	const int32 MiaRicarica = SlotRefusalCooldownAbility(B.Mine);
	const int32 SuaRicarica = SlotRefusalCooldownAbility(B.Nemico);
	if (!TestTrue(TEXT("premessa: entrambi i kit hanno una ricarica"), MiaRicarica != INDEX_NONE && SuaRicarica != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	B.Mine->PlannedAbilityIndex = MiaRicarica;
	B.Mine->ConsumeAbility(MiaRicarica);
	B.Nemico->PlannedAbilityIndex = SuaRicarica;
	B.Nemico->ConsumeAbility(SuaRicarica);
	const URTActionData* Azione = B.Mine->GetAbility(MiaRicarica);

	// --- 1. I chip della comandata ------------------------------------------------------------------------------------
	P->RefreshLook();
	TestTrue(TEXT("1: i chip della comandata ci sono"), Visibile(P->PlanChipsBox));
	TestTrue(TEXT("1: PRINCIPALE dice il nome dell'azione"), Uguale(P->MainChipValueText->GetText(), Azione->DisplayName.ToString()));
	TestTrue(TEXT("1: e ha il «!», perche' un Critical la nomina"), Visibile(P->MainChipAlert));
	TestTrue(TEXT("1: col contorno Amber"),
		P->MainChipFrame->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(Colore(ERTUIToken::Amber)));
	const ERTActionPhaseMark Segno = URTHudViewModel::PhaseMarkFor(Azione->Def);
	TestTrue(TEXT("1: premessa: l'azione ha una fase con un colore"),
		Segno != ERTActionPhaseMark::None && Segno != ERTActionPhaseMark::Cleanup);
	const auto ColoreDiFase = [&Colore](ERTActionPhaseMark M)
	{
		switch (M)
		{
		case ERTActionPhaseMark::Prep:     return Colore(ERTUIToken::Phase_Prep);
		case ERTActionPhaseMark::Dash:     return Colore(ERTUIToken::Phase_Dash);
		case ERTActionPhaseMark::Blast:    return Colore(ERTUIToken::Phase_Blast);
		case ERTActionPhaseMark::Move:     return Colore(ERTUIToken::Phase_Move);
		case ERTActionPhaseMark::Reaction: return Colore(ERTUIToken::Violet);
		default:                           return FLinearColor::Transparent;
		}
	};
	TestTrue(TEXT("1: la striscia ha il colore della SUA fase"), P->MainChipStrip->GetBrushColor().Equals(ColoreDiFase(Segno)));
	TestTrue(TEXT("1: MOVIMENTO, libero, dice «—»"), Uguale(P->MovementChipValueText->GetText(), TEXT("—")));
	TestFalse(TEXT("1: e non ha il «!»"), Visibile(P->MovementChipAlert));
	TestEqual(TEXT("1: e la sua striscia e' trasparente"), P->MovementChipStrip->GetBrushColor().A, 0.f);
	TestTrue(TEXT("1: in Text_Disabled"),
		P->MovementChipValueText->GetColorAndOpacity().GetSpecifiedColor().Equals(Colore(ERTUIToken::Text_Disabled)));
	TestTrue(TEXT("1: REAZIONE, libera, dice «—»"), Uguale(P->ReactionChipValueText->GetText(), TEXT("—")));

	// --- 2. L'elenco degli avvisi --------------------------------------------------------------------------------------
	const TArray<FRTPlanWarningView> Avvisi = P->GetPlanWarnings();
	if (!TestTrue(TEXT("2: premessa: la comandata ha un avviso"), Avvisi.Num() > 0))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	TestTrue(TEXT("2: l'elenco c'e'"), Visibile(Elenco));
	TestEqual(TEXT("2: una voce per avviso"), P->GetWarningEntries().Num(), Avvisi.Num());
	TestEqual(TEXT("2: e tutte nell'elenco"), Elenco->GetChildrenCount(), Avvisi.Num());
	if (P->GetWarningEntries().Num() > 0)
	{
		URTPlanWarningEntryWidget* Voce = P->GetWarningEntries()[0];
		TestEqual(TEXT("2: la prima voce e' il primo avviso"), Voce->GetWarning().SourceActionId, Avvisi[0].SourceActionId);
		TestTrue(TEXT("2: premessa: il primo avviso e' il Critical"), Avvisi[0].Level == ERTPlanWarningLevel::Critical);

		// Le porte della voce, scritte da `SetWarning` (la classe C++ non ha l'albero dell'asset).
		Voce->EntryFrame = Cornice();
		Voce->LevelGlyphText = NewObject<UTextBlock>(P);
		Voce->LevelText = NewObject<UTextBlock>(P);
		Voce->WhatText = NewObject<UTextBlock>(P);
		Voce->SetWarning(Avvisi[0]);
		TestTrue(TEXT("2: un Critical ha la forma del rombo"), Uguale(Voce->LevelGlyphText->GetText(), TEXT("◆")));
		TestTrue(TEXT("2: e il nome del livello in maiuscolo"), Uguale(Voce->LevelText->GetText(), TEXT("CRITICAL")));
		TestTrue(TEXT("2: col contorno Red"),
			Voce->EntryFrame->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(Colore(ERTUIToken::Red)));
		TestTrue(TEXT("2: e il «cosa» dell'avviso"), Voce->WhatText->GetText().EqualTo(Avvisi[0].What));

		// Gli altri due livelli: la forma e il nome cambiano con il livello, non solo il colore.
		FRTPlanWarningView Altro = Avvisi[0];
		Altro.Level = ERTPlanWarningLevel::Warning;
		Voce->SetWarning(Altro);
		TestTrue(TEXT("2: un Warning ha la forma del triangolo"), Uguale(Voce->LevelGlyphText->GetText(), TEXT("▲")));
		TestTrue(TEXT("2: e dice WARNING"), Uguale(Voce->LevelText->GetText(), TEXT("WARNING")));
		TestTrue(TEXT("2: col contorno Amber"),
			Voce->EntryFrame->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(Colore(ERTUIToken::Amber)));
		Altro.Level = ERTPlanWarningLevel::Info;
		Voce->SetWarning(Altro);
		TestTrue(TEXT("2: un Info ha la forma del cerchio"), Uguale(Voce->LevelGlyphText->GetText(), TEXT("●")));
		TestTrue(TEXT("2: e dice INFO"), Uguale(Voce->LevelText->GetText(), TEXT("INFO")));
		TestTrue(TEXT("2: col contorno Cyan"),
			Voce->EntryFrame->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(Colore(ERTUIToken::Cyan)));
	}

	// --- 2b. Un percorso e una reazione: chip occupati, mai l'id grezzo -------------------------------------------------
	const int32 Contro = RTAbilityFixtures::AddCoreAbility(B.Mine, TEXT("Action.Counter"));
	if (!TestTrue(TEXT("2b: premessa: la comandata ha il Counter nel kit"), Contro != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	B.Mine->PlannedWaypoints.Add(GSlotRefusalInVista);
	B.Mine->PlannedReactionAbility = Contro;
	P->RefreshLook();
	const FRTUnitSlotsView ConPercorso = P->GetSlots();
	TestTrue(TEXT("2b: premessa: il movimento e' un percorso, senza azione"),
		ConPercorso.Movement.bOccupied && ConPercorso.Movement.ActionId.IsNone());
	TestTrue(TEXT("2b: MOVIMENTO descrive il percorso come lo slot del pannello"),
		Uguale(P->MovementChipValueText->GetText(), ARTHUD::DescribeMovementSlot(ConPercorso).ToString()));
	TestTrue(TEXT("2b: con la striscia del Move"), P->MovementChipStrip->GetBrushColor().Equals(ColoreDiFase(ERTActionPhaseMark::Move)));
	const FString Reazione = P->ReactionChipValueText->GetText().ToString();
	TestFalse(TEXT("2b: REAZIONE occupata non dice «—»"), Reazione.Equals(TEXT("—"), ESearchCase::CaseSensitive));
	TestFalse(*FString::Printf(TEXT("2b: e non dice l'id grezzo (reso: «%s»)"), *Reazione), Reazione.Contains(TEXT("Action.")));
	TestTrue(TEXT("2b: con la striscia della reazione"), P->ReactionChipStrip->GetBrushColor().Equals(ColoreDiFase(ERTActionPhaseMark::Reaction)));
	B.Mine->PlannedWaypoints.Reset();
	B.Mine->PlannedReactionAbility = INDEX_NONE;

	// --- 3. L'ispezionata: un piano ce l'ha, ma il pannello non ne porta niente ----------------------------------------
	FRTPlanWarningCounts Sua;
	URTHudViewModel::AddPlanWarningCounts(URTHudViewModel::BuildPlanWarnings(B.Nemico, { B.Mine, B.Nemico }), Sua);
	if (!TestEqual(TEXT("3: premessa: il piano dell'avversaria darebbe un Critical"), Sua.Critical, 1))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	P->RefreshLook();
	if (!TestTrue(TEXT("3: premessa: prima dell'ispezione l'elenco della comandata c'e'"), Visibile(Elenco)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	P->SetSelectedUnitForTest(nullptr);
	P->SetInspectedUnitForTest(B.Nemico);
	// Senza riconteggio: l'elenco della comandata di prima non deve restare accanto alla carta dell'ispezionata.
	P->RefreshLook(/*bRecountWarnings=*/ false);
	TestTrue(TEXT("3: premessa: il pannello ha un soggetto"), P->HasSubject());
	TestFalse(TEXT("3: niente chip per l'ispezionata — chiusi, non «—»"), Visibile(P->PlanChipsBox));
	TestFalse(TEXT("3: niente elenco, SUBITO, senza aspettare il riconteggio"), Visibile(Elenco));
	TestEqual(TEXT("3: e nessuna voce"), P->GetWarningEntries().Num(), 0);

	// --- 4. In Risoluzione chip ed elenco si chiudono, anche per la comandata ------------------------------------------
	P->SetInspectedUnitForTest(nullptr);
	P->SetSelectedUnitForTest(B.Mine);
	P->RefreshLook();
	if (!TestTrue(TEXT("4: premessa: in Pianificazione l'elenco della comandata c'e'"), Visibile(Elenco)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	B.Mine->ClearPlannedAttack();
	B.Mine->PlannedAbilityIndex = GSlotRefusalAttacco;
	B.Mine->PlannedAttackTarget = B.Nemico;
	B.Nemico->PlannedAbilityIndex = INDEX_NONE;
	TM->LockInAndResolve();
	if (!TestTrue(TEXT("4: premessa: il turno e' in risoluzione"), TM->IsResolving()))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	B.Mine->PlannedAbilityIndex = MiaRicarica; // la vista diretta vedrebbe un avviso: il pannello deve tacere lo stesso
	P->RefreshLook(/*bRecountWarnings=*/ false);
	TestTrue(TEXT("4: premessa: gli slot della comandata restano autorizzati — la causa e' la Risoluzione"), P->GetSlots().bAuthorized);
	TestFalse(TEXT("4: in Risoluzione i chip si chiudono"), Visibile(P->PlanChipsBox));
	TestFalse(TEXT("4: e l'elenco, SUBITO"), Visibile(Elenco));
	TestEqual(TEXT("4: senza voci"), P->GetWarningEntries().Num(), 0);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * 🔑 **LA CARTA DEL PANNELLO SEGUE IL SOGGETTO, E IL SUO CHIP SOLO IL PIANO DELLA COMANDATA** (#3636).
 *
 * 🔴 **Prima la scriveva il grafo del pannello**, nella variabile Blueprint `Card` di un `WBP_RT_UnitCard` senza classe
 * C++. Riparentata la carta, quella variabile non c'e' piu': senza `SubjectCard` il pannello avrebbe una carta vuota.
 *
 * ⚠️ **L'ispezionata ha un Overwatch pianificato**, come la comandata: e' cio' che rende il chip spento una prova
 * della barriera di `GetSlots()` invece di un'assenza qualunque.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelSubjectCardTest,
	"RefactorTactics.ScreenHud.PanelCardFollowsTheSubject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelSubjectCardTest::RunTest(const FString&)
{
	FSlotRefusalBench B;
	if (!TestTrue(TEXT("banco di prova"), SetUpSlotRefusalBench(B)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(B.World); };
	ARTTurnManager* TM = SlotRefusalTurnManager(B.World);
	URTSelectedUnitPanelWidget* P = NewObject<URTSelectedUnitPanelWidget>(B.World);
	if (!TestNotNull(TEXT("turn manager"), TM) || !TestNotNull(TEXT("pannello"), P)) { return false; }
	P->SetMatchContextForTest(TM, /*PlayerTeamId=*/ 0);
	P->SubjectCard = NewObject<URTUnitCardWidget>(P);

	const int32 MiaGuardia = RTAbilityFixtures::AddCoreAbility(B.Mine, TEXT("Action.Overwatch"));
	const int32 SuaGuardia = RTAbilityFixtures::AddCoreAbility(B.Nemico, TEXT("Action.Overwatch"));
	if (!TestTrue(TEXT("premessa: entrambe hanno l'Overwatch"), MiaGuardia != INDEX_NONE && SuaGuardia != INDEX_NONE)) { return false; }
	B.Mine->PlannedAbilityIndex = MiaGuardia;
	B.Nemico->PlannedAbilityIndex = SuaGuardia;
	// Il piano dell'avversaria ARMEREBBE il chip, se lo si leggesse: senza questa premessa il chip spento del passo 3
	// sarebbe vero per caso.
	if (!TestTrue(TEXT("premessa: il piano dell'avversaria arma una reazione"), URTHudViewModel::BuildUnitSlots(B.Nemico).bReactionArmed))
	{
		return false;
	}

	// --- 2. La comandata: la sua carta, col chip del suo piano ---------------------------------------------------------------
	P->SetSelectedUnitForTest(B.Mine);
	P->RefreshLook();
	if (!TestTrue(TEXT("2: premessa: il piano della comandata arma una reazione"), P->GetSlots().bReactionArmed)) { return false; }
	TestEqual(TEXT("2: la carta e' della comandata"), P->SubjectCard->GetCard().HeroId, B.Mine->HeroId);
	TestTrue(TEXT("2: col nome leggibile della vista"), P->SubjectCard->GetCard().DisplayName.EqualTo(P->GetCard().DisplayName));
	TestTrue(TEXT("2: e il chip REAZ."), P->SubjectCard->IsReactionArmed());

	// --- 3. L'ispezionata: la sua carta, senza il chip anche se il suo piano lo armerebbe ---------------------------------------
	P->SetSelectedUnitForTest(nullptr);
	P->SetInspectedUnitForTest(B.Nemico);
	P->RefreshLook();
	TestEqual(TEXT("3: la carta segue l'ispezionata"), P->SubjectCard->GetCard().HeroId, B.Nemico->HeroId);
	TestFalse(TEXT("3: ed e' un'avversaria"), P->SubjectCard->GetCard().bIsAlly);
	TestFalse(TEXT("3: niente chip: il suo piano non lascia il core"), P->SubjectCard->IsReactionArmed());

	// --- 4. Di nuovo la comandata: il chip torna --------------------------------------------------------------------------
	P->SetInspectedUnitForTest(nullptr);
	P->SetSelectedUnitForTest(B.Mine);
	P->RefreshLook();
	TestEqual(TEXT("4: la carta torna alla comandata"), P->SubjectCard->GetCard().HeroId, B.Mine->HeroId);
	TestTrue(TEXT("4: col chip"), P->SubjectCard->IsReactionArmed());

	// --- 5. Senza soggetto la carta si svuota: non resta quella di prima ----------------------------------------------------
	P->SetSelectedUnitForTest(nullptr);
	P->SetInspectedUnitForTest(nullptr);
	P->RefreshLook();
	if (!TestFalse(TEXT("5: premessa: il pannello non ha un soggetto"), P->HasSubject())) { return false; }
	TestTrue(TEXT("5: la carta si svuota"), P->SubjectCard->GetCard().HeroId.IsNone());
	TestFalse(TEXT("5: e perde il chip"), P->SubjectCard->IsReactionArmed());
	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
