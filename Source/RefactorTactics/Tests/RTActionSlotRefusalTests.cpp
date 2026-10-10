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
#include "UI/RTScreenHudWidgets.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
