#include "Misc/AutomationTest.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h" // TAG_Status_Unbalanced: lo stato che nega lo scatto a budget (#3555)
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHUD.h" // ComputePlannedHitMarks: i segni di colpo sulle unita'
#include "UI/RTHudViewModel.h"
#include "Turn/RTPlanPreview.h"
#include "UI/RTScreenHudWidgets.h"
#include "Unit/RTUnit.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

// =====================================================================================================
// `#3509`, [D-464] — L'ORIGINE DI MIRA E' UNA SOLA, E DIPENDE DALLA FASE.
//
// Con uno scatto pianificato e poi un'azione, giudicavano dalla cella corrente i due click, lo stato `Invalid` dello
// slot, l'anteprima del piano e la portata viola; l'area colpita e il `Warning` dalla cella dello scatto. Sulla
// stessa unita' la portata diceva si', lo slot `Warning` e l'anteprima del piano ok.
//
// 🔑 **Gli esiti attesi sono celle SCRITTE A MANO**, quelle di E10 ed E11 del referto
// (`raggio-di-mira-spec-panel-2026-10-06.md` §8), non risposte del classificatore che il produttore stesso chiama:
// confrontare il produttore con se stesso resterebbe verde con l'origine sbagliata. Il classificatore entra solo
// nelle PREMESSE, per dire che dalla cella corrente gli esiti si invertono — e' cio' che rende questi test capaci
// di cadere.
// =====================================================================================================

namespace
{
	/** Tutte le fasi, scritte a mano: un valore nuovo dell'enum fa cadere il controllo di completezza qui sotto. */
	const TArray<ERTResolutionPhase>& AimOriginAllPhases()
	{
		static const TArray<ERTResolutionPhase> Fasi = {
			ERTResolutionPhase::Snapshot, ERTResolutionPhase::Preparation, ERTResolutionPhase::FastMovement,
			ERTResolutionPhase::NormalMovement, ERTResolutionPhase::Control, ERTResolutionPhase::Attack,
			ERTResolutionPhase::Environment, ERTResolutionPhase::Cleanup };
		return Fasi;
	}

	/** Le sole fasi che [D-464] fa mirare dallo scatto. Scritte a mano, non derivate da `MapResolutionPhase`. */
	bool AimOriginMovesWithTheDash(ERTResolutionPhase Fase)
	{
		return Fase == ERTResolutionPhase::Attack || Fase == ERTResolutionPhase::Control;
	}

	FString AimOriginPhaseName(ERTResolutionPhase Fase)
	{
		return UEnum::GetValueAsString(Fase);
	}

	FString AimOriginCellName(const FRTCellId& C)
	{
		return FString::Printf(TEXT("(%d,%d,L%d)"), C.X, C.Y, C.Layer);
	}

	// ── Il banco degli esempi E10/E11: `MakeTestArena`, Muiren col loadout di default, lo scatto gia' pianificato.

	/** La cella di Muiren, e dove la porta `FluidTrail`: allineata, a distanza 3, attraverso il muro alla vista. */
	const FRTCellId GAimOriginQui(-1, 0, 0);
	const FRTCellId GAimOriginScatto(2, 0, 0);

	struct FAimOriginBench
	{
		UWorld* World = nullptr;
		URTHexMapAsset* Map = nullptr;
		ARTHexMapActor* MapActor = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Mine = nullptr;
		ARTUnit* Primo = nullptr;
		ARTUnit* Secondo = nullptr;
		URTActionDockWidget* Dock = nullptr;
		int32 Scatto = INDEX_NONE;
		int32 Getto = INDEX_NONE;
		int32 Irrigatore = INDEX_NONE;
	};

	/** Nomi distinti per file: il progetto usa unity build. */
	ARTUnit* SpawnAimOriginUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell,
		bool bLoadout)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		// 🔑 **Il loadout, come lo spawn di partita** (`RTMatchBootstrapper`): senza, `PressureJet` ha portata 5 e
		// non 4 (`Weapon.Impact` toglie una cella) e il kit non porta l'irrigatore. Gli esempi del referto lo
		// presumono, e il banco del Warning (`RTActionSlotRefusalTests.cpp`) non lo monta.
		if (bLoadout)
		{
			U->EquipLoadout(URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId));
		}
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	int32 AimOriginKitIndex(const ARTUnit* Unit, const TCHAR* ActionId)
	{
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* A = Unit->GetAbility(I);
			if (A && A->Def.ActionId == FName(ActionId))
			{
				return I;
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Allestisce il banco e PIANIFICA lo scatto, come lo lascerebbe il click: `PlannedDashAbility` e
	 * `PlannedDashCell`, lo stesso stato che pinna `HudViewModel.DegradedPlanWarnsOnlyOnAKnownTarget`. I due nemici
	 * noti stanno dove ciascun esempio li vuole.
	 */
	bool SetUpAimOriginBench(FAutomationTestBase& Test, FAimOriginBench& B, const FRTCellId& PrimoCell,
		const FRTCellId& SecondoCell)
	{
		B.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("banco: mondo"), B.World)) { return false; }
		B.Map = URTMatchSetupLibrary::MakeTestArena(B.World);
		B.MapActor = B.World->SpawnActor<ARTHexMapActor>();
		if (!Test.TestNotNull(TEXT("banco: mappa"), B.MapActor)) { return false; }
		B.MapActor->MapAsset = B.Map;
		B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		B.Mine = SpawnAimOriginUnit(B.World, 0, URTHeroCatalogLibrary::MakeMuiren(), GAimOriginQui, true);
		B.Primo = SpawnAimOriginUnit(B.World, 1, URTHeroCatalogLibrary::MakeAevik(), PrimoCell, false);
		B.Secondo = SpawnAimOriginUnit(B.World, 1, URTHeroCatalogLibrary::MakeIvrin(), SecondoCell, false);
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		B.Dock = NewObject<URTActionDockWidget>(B.World);
		if (!Test.TestNotNull(TEXT("banco: Muiren"), B.Mine) || !Test.TestNotNull(TEXT("banco: primo nemico"), B.Primo)
			|| !Test.TestNotNull(TEXT("banco: secondo nemico"), B.Secondo) || !Test.TestNotNull(TEXT("banco: PC"), B.PC)
			|| !Test.TestNotNull(TEXT("banco: dock"), B.Dock))
		{
			return false;
		}
		B.Primo->SetKnownToObserver(true);
		B.Secondo->SetKnownToObserver(true);
		B.PC->SelectUnit(B.Mine, /*bRecordAsPlayerInput=*/ false);
		B.Dock->SetCommandControllerForTest(B.PC);
		B.Dock->SetSelectedUnitForTest(B.Mine);

		B.Scatto = AimOriginKitIndex(B.Mine, TEXT("Hero.Muiren.FluidTrail"));
		B.Getto = AimOriginKitIndex(B.Mine, TEXT("Hero.Muiren.PressureJet"));
		B.Irrigatore = AimOriginKitIndex(B.Mine, TEXT("Gadget.Sprinkler"));
		if (!Test.TestTrue(TEXT("banco: il kit ha FluidTrail"), B.Scatto != INDEX_NONE)
			|| !Test.TestTrue(TEXT("banco: il kit ha PressureJet"), B.Getto != INDEX_NONE)
			|| !Test.TestTrue(TEXT("banco: il loadout ha portato l'irrigatore"), B.Irrigatore != INDEX_NONE))
		{
			return false;
		}

		B.Mine->PlannedDashAbility = B.Scatto;
		B.Mine->PlannedDashCell = GAimOriginScatto;
		return Test.TestTrue(TEXT("banco: lo scatto pianificato si applica"), B.Mine->PlannedDashApplies())
			&& Test.TestFalse(TEXT("banco: e non e' una carica"), B.Mine->PlannedDashIsCharge());
	}

	/** La riga del kit che la dock consegna allo slot: la stessa strada del gioco. */
	FRTAbilityCooldownView AimOriginDockRow(const FAimOriginBench& B, int32 Index)
	{
		const TArray<FRTAbilityCooldownView> Righe = B.Dock->GetActions();
		return Righe.IsValidIndex(Index) ? Righe[Index] : FRTAbilityCooldownView();
	}

	/** La voce del Blast nella timeline posata (`MakePlanPreview` la etichetta sempre `Attack`), o `nullptr`. */
	const FRTPhasePreviewEntry* AimOriginBlastEntry(const FAimOriginBench& B)
	{
		for (const FRTPhasePreviewEntry& Voce : B.MapActor->GetPlanPreview().Phases)
		{
			if (Voce.Phase == ERTResolutionPhase::Attack)
			{
				return &Voce;
			}
		}
		return nullptr;
	}

	/** «Lo slot accenderebbe `Invalid` col puntatore su `Cell`?» — la domanda della lettura A di [D-459]. */
	bool AimOriginSlotRefuses(const FAimOriginBench& B, int32 Index, const FRTCellId& Cell)
	{
		B.MapActor->SetHoveredCell(Cell, /*bValid=*/ true);
		return AimOriginDockRow(B, Index).bTargetRefused;
	}
}

/**
 * **La regola, per fase ([D-464] punto 1)** — `AimOriginCell` su ogni valore di `ERTResolutionPhase`.
 *
 * Con lo scatto che si applica mirano dallo scatto `Attack` e `Control`, e nessun'altra; senza scatto, con uno
 * scatto che non sposta o con una carica, tutte mirano dalla cella corrente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginFollowsThePhaseTest,
	"RefactorTactics.AimOrigin.FollowsTheResolutionPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginFollowsThePhaseTest::RunTest(const FString&)
{
	// COMPLETEZZA — l'elenco a mano copre l'enum. Un valore aggiunto domani cade qui, invece di passare inosservato.
	const UEnum* Enum = StaticEnum<ERTResolutionPhase>();
	if (!TestNotNull(TEXT("l'enum delle fasi e' riflesso"), Enum)
		|| !TestEqual(TEXT("l'elenco delle fasi del test copre l'enum"), AimOriginAllPhases().Num(),
			static_cast<int32>(Enum->NumEnums() - 1)))
	{
		return false;
	}

	const FRTCellId Qui(-1, 0, 0);
	const FRTCellId Scatto(2, 0, 0);
	for (const ERTResolutionPhase Fase : AimOriginAllPhases())
	{
		const FString Nome = AimOriginPhaseName(Fase);
		const FRTCellId Attesa = AimOriginMovesWithTheDash(Fase) ? Scatto : Qui;
		TestEqual(*FString::Printf(TEXT("%s: con lo scatto che si applica"), *Nome),
			URTHexCombatLibrary::AimOriginCell(Fase, Qui, /*bDashResolves=*/ true, /*bDashIsCharge=*/ false, Scatto),
			Attesa);
		TestEqual(*FString::Printf(TEXT("%s: senza scatto, dalla cella corrente"), *Nome),
			URTHexCombatLibrary::AimOriginCell(Fase, Qui, /*bDashResolves=*/ false, /*bDashIsCharge=*/ false, Scatto),
			Qui);
		TestEqual(*FString::Printf(TEXT("%s: uno scatto che non sposta non e' uno scatto"), *Nome),
			URTHexCombatLibrary::AimOriginCell(Fase, Qui, /*bDashResolves=*/ true, /*bDashIsCharge=*/ false, Qui),
			Qui);
		// ⛔ [D-464] punto 5: la `PlannedDashCell` di una carica e' la cella del bersaglio, e da li' non si mira.
		TestEqual(*FString::Printf(TEXT("%s: la carica ne resta fuori"), *Nome),
			URTHexCombatLibrary::AimOriginCell(Fase, Qui, /*bDashResolves=*/ true, /*bDashIsCharge=*/ true, Scatto),
			Qui);
	}
	return true;
}

/**
 * **`BlastOriginCell` ne e' il caso della fase, non un secondo contratto ([D-464] punto 2).**
 *
 * ⚠️ E il piano composto SENZA fase resta del Blast: e' cio' che tiene verdi `Preview.OriginIsPlannedDashCell` e
 * `Preview.PhaseOriginsFollowTheResolutionOrder`, che pinnano l'origine dello scatto senza dichiarare una fase.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginBlastIsItsCaseTest,
	"RefactorTactics.AimOrigin.BlastOriginCellIsThePhaseCase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginBlastIsItsCaseTest::RunTest(const FString&)
{
	const FRTCellId Qui(-1, 0, 0);
	const FRTCellId Scatto(2, 0, 0);
	FRTHexCombatUnit Attaccante;
	Attaccante.UnitId = 0;
	Attaccante.Cell = Qui;
	const TArray<FRTHexCombatUnit> Unita = { Attaccante };

	FRTBlastPreviewPlan Piano;
	Piano.AttackerId = 0;
	Piano.bDashResolves = true;
	Piano.PlannedDashCell = Scatto;
	TestEqual(TEXT("un piano senza fase dichiarata mira dallo scatto, come prima di #3509"),
		URTHexCombatLibrary::BlastOriginCell(Piano, Unita), Scatto);

	for (const ERTResolutionPhase Fase : AimOriginAllPhases())
	{
		for (const bool bCarica : { false, true })
		{
			Piano.Phase = Fase;
			Piano.bDashIsCharge = bCarica;
			TestEqual(*FString::Printf(TEXT("%s%s: BlastOriginCell risponde come AimOriginCell"),
					*AimOriginPhaseName(Fase), bCarica ? TEXT(", carica") : TEXT("")),
				URTHexCombatLibrary::BlastOriginCell(Piano, Unita),
				URTHexCombatLibrary::AimOriginCell(Fase, Qui, true, bCarica, Scatto));
		}
	}
	return true;
}

/**
 * **E10 — scatto, poi un `Attack`: click, slot, portata, anteprima e Warning mirano tutti dallo scatto.**
 *
 * Muiren in (-1,0,0) pianifica `FluidTrail` fino a (2,0,0), poi arma `PressureJet` (portata 4 col loadout). Dallo
 * scatto (4,0,0) e (4,-1,0) sono a distanza 2; (-2,0,0) e' a distanza 4 ma oltre il muro alla vista in q=0. Dalla
 * cella corrente tutto si invertirebbe: le prime due a distanza 5, la terza adiacente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginAttackAfterDashTest,
	"RefactorTactics.AimOrigin.AttackAfterADashAimsFromTheDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginAttackAfterDashTest::RunTest(const FString&)
{
	const FRTCellId Lontano(4, 0, 0);
	const FRTCellId Obliquo(4, -1, 0);
	const FRTCellId DietroIlMuro(-2, 0, 0);

	FAimOriginBench B;
	if (!SetUpAimOriginBench(*this, B, Lontano, DietroIlMuro))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const URTActionData* Getto = B.Mine->GetAbility(B.Getto);

	// PREMESSE — la fase, la portata del loadout, e l'INVERSIONE dalla cella corrente. Senza quest'ultima un verde
	// non distinguerebbe l'origine dello scatto da quella di prima.
	TestEqual(TEXT("premessa: PressureJet e' un Attack"), Getto->Def.ResolutionPhase, ERTResolutionPhase::Attack);
	TestEqual(TEXT("premessa: col loadout la portata e' 4"), Getto->RangeCells, 4);
	const auto DaQui = [&B, Getto](const FRTCellId& C) {
		return URTCombatLibrary::ClassifyHexTargeting(B.Map, GAimOriginQui, C, Getto->RangeCells,
			Getto->Def.LineOfSightPolicy);
	};
	TestEqual(TEXT("premessa: dalla cella corrente (4,0,0) e' fuori portata"), DaQui(Lontano),
		ERTHexTargetReason::OutOfRange);
	TestEqual(TEXT("premessa: dalla cella corrente (-2,0,0) e' colpibile"), DaQui(DietroIlMuro),
		ERTHexTargetReason::Ok);

	B.PC->ArmKitAbility(B.Getto);
	if (!TestEqual(TEXT("premessa: PressureJet e' armato"), B.Mine->SelectedAbilityIndex, B.Getto))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// LA PORTATA — dallo scatto.
	const TArray<FRTCellId>& Portata = B.MapActor->GetPreviewRangeCells();
	TestTrue(TEXT("portata: (4,0,0) e' dentro"), Portata.Contains(Lontano));
	TestTrue(TEXT("portata: (4,-1,0) e' dentro"), Portata.Contains(Obliquo));

	// LO SLOT — `Invalid` sotto il puntatore, dalla stessa origine.
	TestFalse(TEXT("slot: col puntatore sul nemico in (4,0,0) non rifiuta"),
		AimOriginSlotRefuses(B, B.Getto, Lontano));
	TestTrue(TEXT("slot: col puntatore sul nemico oltre il muro rifiuta"),
		AimOriginSlotRefuses(B, B.Getto, DietroIlMuro));

	// IL CLICK — rifiuta il nemico che la cella corrente avrebbe accettato...
	B.PC->HandleClickOnUnitForTest(B.Secondo);
	TestTrue(TEXT("click: il nemico in (-2,0,0) e' rifiutato, perche' dallo scatto c'e' il muro"),
		B.Mine->PlannedAttackTarget.Get() != B.Secondo);
	// ...e accetta quello che avrebbe rifiutato.
	B.PC->HandleClickOnUnitForTest(B.Primo);
	if (!TestTrue(TEXT("click: il nemico in (4,0,0) e' accettato"),
			B.Mine->PlannedAttackTarget.Get() == B.Primo && B.Mine->PlannedAbilityIndex == B.Getto))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// L'ANTEPRIMA — l'area colpita parte dallo scatto. `PressureJet` e' una `Line`: le sue celle si spostano con
	// l'origine, quindi lo dicono anche senza leggere il marcatore.
	TestTrue(TEXT("anteprima: e' accesa"), B.MapActor->IsPreviewAttackValid());
	TestEqual(TEXT("anteprima: l'origine e' la cella dello scatto"), B.MapActor->GetPreviewAttackOrigin(),
		GAimOriginScatto);
	TestTrue(TEXT("anteprima: la linea arriva sul bersaglio"), B.MapActor->IsPreviewHitCell(Lontano));
	TestFalse(TEXT("anteprima: e non passa dietro lo scatto"), B.MapActor->IsPreviewHitCell(FRTCellId(1, 0, 0)));

	// LA TIMELINE DEL PIANO (#172) — la voce del Blast parte dallo scatto, e il suo rifiuto e' quello del click.
	if (const FRTPhasePreviewEntry* Colpo = AimOriginBlastEntry(B);
		TestNotNull(TEXT("timeline: c'e' la voce del Blast"), Colpo))
	{
		TestEqual(TEXT("timeline: parte dallo scatto"), Colpo->PreviewOrigin, GAimOriginScatto);
		TestEqual(TEXT("timeline: e non rifiuta il bersaglio che il click ha accettato"), Colpo->TargetRefusal,
			ERTTargetRefusal::None);
	}

	// I SEGNI DI COLPO SULLE UNITA' (`ARTHUD::ComputePlannedHitMarks`) — la stessa retta dell'area colpita.
	TSet<FRTCellId> Segnate;
	TSet<FRTCellId> AlleateSegnate;
	ARTHUD::ComputePlannedHitMarks({ B.Mine, B.Primo, B.Secondo }, /*PlayerTeamId=*/ 0, Segnate, AlleateSegnate);
	TestTrue(TEXT("segni: la retta arriva sul bersaglio"), Segnate.Contains(Lontano));
	TestFalse(TEXT("segni: e non passa dietro lo scatto"), Segnate.Contains(FRTCellId(1, 0, 0)));

	// IL WARNING — nessuno: il click ha accettato dalla stessa origine da cui il Blast partira'.
	const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(B.Mine);
	if (TestTrue(TEXT("warning: la riga di PressureJet c'e'"), Righe.IsValidIndex(B.Getto)))
	{
		TestFalse(TEXT("warning: il piano accettato dallo scatto non e' degradato"), Righe[B.Getto].bPlanDegraded);
	}

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * **E11 — lo stesso scatto, poi un `Environment`: tutto mira dalla cella corrente, per scelta dichiarata.**
 *
 * L'irrigatore (`Gadget.Sprinkler`, portata 4) risolve dopo il Move, ma il resolver non ne ricontrolla la portata:
 * il click e' l'unica regola, e [D-464] lo fa giudicare da qui. Da (-1,0,0) il nemico in (-4,0,0) e' a distanza 3, e
 * quello in (4,-1,0) a distanza 5. Dallo scatto si invertirebbe: 6 e 2.
 *
 * 🔑 **L'anteprima e' il caso che cambia**: prima di #3509 `BlastOriginCell` non conosceva la fase, e partiva dallo
 * scatto per qualunque azione pianificata. L'irrigatore e' `Single`, quindi la sua area non si sposta con l'origine:
 * lo dice solo il marcatore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginEnvironmentAfterDashTest,
	"RefactorTactics.AimOrigin.EnvironmentAfterADashAimsFromHere",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginEnvironmentAfterDashTest::RunTest(const FString&)
{
	const FRTCellId Vicino(-4, 0, 0);
	const FRTCellId Obliquo(4, -1, 0);

	FAimOriginBench B;
	if (!SetUpAimOriginBench(*this, B, Obliquo, Vicino))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const URTActionData* Irrigatore = B.Mine->GetAbility(B.Irrigatore);

	TestEqual(TEXT("premessa: l'irrigatore e' un Environment"), Irrigatore->Def.ResolutionPhase,
		ERTResolutionPhase::Environment);
	TestEqual(TEXT("premessa: con portata 4"), Irrigatore->RangeCells, 4);
	const auto DalloScatto = [&B, Irrigatore](const FRTCellId& C) {
		return URTCombatLibrary::ClassifyHexTargeting(B.Map, GAimOriginScatto, C, Irrigatore->RangeCells,
			Irrigatore->Def.LineOfSightPolicy);
	};
	TestEqual(TEXT("premessa: dallo scatto (4,-1,0) sarebbe colpibile"), DalloScatto(Obliquo),
		ERTHexTargetReason::Ok);
	TestEqual(TEXT("premessa: dallo scatto (-4,0,0) sarebbe fuori portata"), DalloScatto(Vicino),
		ERTHexTargetReason::OutOfRange);

	B.PC->ArmKitAbility(B.Irrigatore);
	if (!TestEqual(TEXT("premessa: l'irrigatore e' armato"), B.Mine->SelectedAbilityIndex, B.Irrigatore))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	const TArray<FRTCellId>& Portata = B.MapActor->GetPreviewRangeCells();
	TestTrue(TEXT("portata: (-4,0,0) e' dentro"), Portata.Contains(Vicino));
	TestFalse(TEXT("portata: (4,-1,0) e' fuori"), Portata.Contains(Obliquo));

	TestFalse(TEXT("slot: col puntatore sul nemico in (-4,0,0) non rifiuta"),
		AimOriginSlotRefuses(B, B.Irrigatore, Vicino));
	TestTrue(TEXT("slot: col puntatore sul nemico in (4,-1,0) rifiuta"),
		AimOriginSlotRefuses(B, B.Irrigatore, Obliquo));

	B.PC->HandleClickOnUnitForTest(B.Primo);
	TestTrue(TEXT("click: il nemico in (4,-1,0) e' rifiutato"), B.Mine->PlannedAttackTarget.Get() != B.Primo);
	B.PC->HandleClickOnUnitForTest(B.Secondo);
	if (!TestTrue(TEXT("click: il nemico in (-4,0,0) e' accettato"),
			B.Mine->PlannedAttackTarget.Get() == B.Secondo && B.Mine->PlannedAbilityIndex == B.Irrigatore))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	TestTrue(TEXT("anteprima: e' accesa"), B.MapActor->IsPreviewAttackValid());
	TestEqual(TEXT("anteprima: l'origine e' la cella corrente, non lo scatto"), B.MapActor->GetPreviewAttackOrigin(),
		GAimOriginQui);
	if (const FRTPhasePreviewEntry* Colpo = AimOriginBlastEntry(B);
		TestNotNull(TEXT("timeline: c'e' la voce dell'azione"), Colpo))
	{
		TestEqual(TEXT("timeline: parte dalla cella corrente"), Colpo->PreviewOrigin, GAimOriginQui);
		TestEqual(TEXT("timeline: e non rifiuta il bersaglio che il click ha accettato"), Colpo->TargetRefusal,
			ERTTargetRefusal::None);
	}

	const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(B.Mine);
	if (TestTrue(TEXT("warning: la riga dell'irrigatore c'e'"), Righe.IsValidIndex(B.Irrigatore)))
	{
		TestFalse(TEXT("warning: il piano accettato da qui non e' degradato"), Righe[B.Irrigatore].bPlanDegraded);
	}

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * **E10 su una CELLA — lo stesso scatto, poi `CircularTide`, un `Attack` d'area: il click su cella e lo slot.**
 *
 * I due esempi qui sopra mirano un'unita' e passano da `HandleClickOnUnit`; un'azione d'area mira una cella e passa
 * da `HandleTargetCell` e dal ramo `Cell` di `RefusalUnderPointerForArmed`, che hanno la propria chiamata. Dallo
 * scatto (4,0,0) e' a distanza 2 e (-3,0,0) a 5; dalla cella corrente il contrario: 5 e 2.
 *
 * I nemici stanno lontani, in (-4,4,0) e (4,-4,0): il click su una cella non guarda chi la occupa, e il test non deve
 * poterlo far credere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginCellAfterDashTest,
	"RefactorTactics.AimOrigin.CellTargetAfterADashAimsFromTheDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginCellAfterDashTest::RunTest(const FString&)
{
	const FRTCellId DalloScatto(4, 0, 0);
	const FRTCellId DaQui(-3, 0, 0);

	FAimOriginBench B;
	if (!SetUpAimOriginBench(*this, B, FRTCellId(-4, 4, 0), FRTCellId(4, -4, 0)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const int32 Marea = AimOriginKitIndex(B.Mine, TEXT("Hero.Muiren.CircularTide"));
	const URTActionData* Azione = B.Mine->GetAbility(Marea);
	if (!TestNotNull(TEXT("premessa: il kit ha CircularTide"), Azione))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	TestEqual(TEXT("premessa: e' un Attack"), Azione->Def.ResolutionPhase, ERTResolutionPhase::Attack);
	const auto Da = [&B, Azione](const FRTCellId& O, const FRTCellId& C) {
		return URTCombatLibrary::DescribeCellTargetRefusal(B.Map, O, C, Azione->RangeCells,
			Azione->Def.LineOfSightPolicy).Refusal;
	};
	TestEqual(TEXT("premessa: dalla cella corrente (4,0,0) e' rifiutata"), Da(GAimOriginQui, DalloScatto),
		ERTTargetRefusal::Range);
	TestEqual(TEXT("premessa: dalla cella corrente (-3,0,0) e' accettata"), Da(GAimOriginQui, DaQui),
		ERTTargetRefusal::None);

	B.PC->ArmKitAbility(Marea);
	if (!TestEqual(TEXT("premessa: CircularTide e' armata"), B.Mine->SelectedAbilityIndex, Marea)
		|| !TestEqual(TEXT("premessa: e punta una cella"), B.PC->GetPointerTargetKind(), ERTPointerTargetKind::Cell))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	TestFalse(TEXT("slot: col puntatore su (4,0,0) non rifiuta"), AimOriginSlotRefuses(B, Marea, DalloScatto));
	TestTrue(TEXT("slot: col puntatore su (-3,0,0) rifiuta"), AimOriginSlotRefuses(B, Marea, DaQui));

	B.PC->HandleClickOnCellForTest(DaQui);
	TestFalse(TEXT("click: (-3,0,0) non pianifica niente"),
		B.Mine->PlannedAbilityIndex == Marea && B.Mine->bAttackTargetsCell && B.Mine->PlannedAttackCell == DaQui);
	B.PC->HandleClickOnCellForTest(DalloScatto);
	TestTrue(TEXT("click: (4,0,0) pianifica l'area"),
		B.Mine->PlannedAbilityIndex == Marea && B.Mine->bAttackTargetsCell && B.Mine->PlannedAttackCell == DalloScatto);
	TestEqual(TEXT("anteprima: l'origine e' la cella dello scatto"), B.MapActor->GetPreviewAttackOrigin(),
		GAimOriginScatto);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * **La carica resta fuori ([D-464] punto 5)** — con `Hero.Branth.Ram` pianificata, un `Attack` mira dalla cella
 * corrente e non da `PlannedDashCell`, che per una carica e' la cella del bersaglio ([D-296]).
 *
 * ⚠️ Il CONTROLLO e' uno scatto che non e' una carica — `FluidTrail` di Muiren, perche' Branth non ne ha altri —:
 * senza, «mira da qui» non distinguerebbe l'esclusione della carica da uno scatto che non si applica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginChargeStaysOutTest,
	"RefactorTactics.AimOrigin.ChargeStaysOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginChargeStaysOutTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }
	URTMatchSetupLibrary::MakeTestArena(World);

	const FRTCellId QuiBranth(-3, 1, 0);
	const FRTCellId Bersaglio(-3, -1, 0);
	ARTUnit* Branth = SpawnAimOriginUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), QuiBranth, false);
	ARTUnit* Muiren = SpawnAimOriginUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), GAimOriginQui, false);
	if (!TestNotNull(TEXT("Branth"), Branth) || !TestNotNull(TEXT("Muiren"), Muiren))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const int32 Ariete = AimOriginKitIndex(Branth, TEXT("Hero.Branth.Ram"));
	const int32 Scia = AimOriginKitIndex(Muiren, TEXT("Hero.Muiren.FluidTrail"));
	if (!TestTrue(TEXT("premessa: Branth ha l'ariete"), Ariete != INDEX_NONE)
		|| !TestTrue(TEXT("premessa: Muiren ha FluidTrail"), Scia != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// CONTROLLO — uno scatto che non e' una carica sposta l'origine di un Attack.
	Muiren->PlannedDashAbility = Scia;
	Muiren->PlannedDashCell = GAimOriginScatto;
	TestTrue(TEXT("controllo: lo scatto di Muiren si applica"), Muiren->PlannedDashApplies());
	TestFalse(TEXT("controllo: e non e' una carica"), Muiren->PlannedDashIsCharge());
	TestEqual(TEXT("controllo: un Attack di Muiren mira da dove lo scatto arriva"),
		Muiren->AimOriginFor(ERTResolutionPhase::Attack), GAimOriginScatto);

	// IL CUORE — la carica: si applica, e da li' non si mira.
	Branth->PlannedDashAbility = Ariete;
	Branth->PlannedDashCell = Bersaglio;
	TestTrue(TEXT("la carica di Branth si applica"), Branth->PlannedDashApplies());
	TestTrue(TEXT("ed e' riconosciuta come carica"), Branth->PlannedDashIsCharge());
	TestEqual(TEXT("ma un Attack di Branth mira dalla cella corrente"),
		Branth->AimOriginFor(ERTResolutionPhase::Attack), QuiBranth);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Uno scatto a BUDGET che lo stato nega non sposta la mira** ([D-471], #3555).
 *
 * `ResolveDash` rifiuta una mobilita' rapida non lineare a chi e' `Unbalanced` ([D-319]), mentre fino a [D-471] la
 * mira chiedeva solo `PlannedDashApplies()`: con lo scatto negato, il click mirava da una cella che l'unita' non
 * avrebbe raggiunto. Ora tutti chiedono `PlannedDashMoves()`.
 *
 * ⚠️ **L'azione si COSTRUISCE**: nessuna mobilita' rapida spedita e' a budget, quindi `FluidTrail` diventa a budget
 * solo qui. I due controlli sono lo stesso scatto senza lo stato, e uno lineare con lo stato: entrambi spostano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginUnbalancedBudgetDashTest,
	"RefactorTactics.AimOrigin.UnbalancedBudgetDashDoesNotMoveTheAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginUnbalancedBudgetDashTest::RunTest(const FString&)
{
	const FRTCellId Lontano(4, 0, 0);

	FAimOriginBench B;
	if (!SetUpAimOriginBench(*this, B, FRTCellId(-4, 4, 0), FRTCellId(4, -4, 0)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	URTActionData* Scia = B.Mine->GetAbility(B.Scatto);
	Scia->Def.MovementStyle = ERTMovementStyle::Budget;
	// Un attacco su una cella e una reazione armata: sono i due lettori di `bDashResolves` dell'anteprima, l'area
	// colpita e la timeline, che altrimenti nessuna riga di questo test guarderebbe.
	const int32 Reazione = AimOriginKitIndex(B.Mine, TEXT("Reaction.HazardEscape"));
	if (!TestTrue(TEXT("premessa: il loadout ha portato la reazione"), Reazione != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	B.Mine->PlannedReactionAbility = Reazione;
	B.Mine->PlannedAbilityIndex = B.Getto;
	B.Mine->bAttackTargetsCell = true;
	B.Mine->PlannedAttackCell = FRTCellId(-3, 0, 0);
	B.PC->SelectAbilityForCurrentForTest(B.Getto);
	// La corsa va da (-1,0,0) a (2,0,0), quindi l'ultimo passo e' verso E. A fine corsa a budget il cono del verso
	// e' di `MoveEndPivotMaxSteps` passi (Muiren: 2) attorno a E, e W, l'opposto, ne resta fuori; da fermo ogni verso
	// e' legale ([D-367]). E' il quarto lettore della domanda, attraverso `PlannedMovementForFacing`.
	const ERTHexDirection Indietro = ERTHexDirection::W;

	// CONTROLLO — senza lo stato lo scatto a budget sposta: mira, portata e verso partono dallo scatto.
	TestTrue(TEXT("controllo: senza Unbalanced lo scatto a budget sposta"), B.Mine->PlannedDashMoves());
	TestEqual(TEXT("controllo: un Attack mira dallo scatto"), B.Mine->AimOriginFor(ERTResolutionPhase::Attack),
		GAimOriginScatto);
	TestTrue(TEXT("controllo: la portata arriva a (4,0,0)"), B.MapActor->GetPreviewRangeCells().Contains(Lontano));
	TestEqual(TEXT("controllo: il verso si dichiara dalla cella dello scatto"), B.PC->FacingCellFor(B.Mine),
		GAimOriginScatto);
	TestEqual(TEXT("controllo: l'area colpita parte dallo scatto"), B.MapActor->GetPreviewAttackOrigin(),
		GAimOriginScatto);
	TestEqual(TEXT("controllo: la reazione guarda dallo scatto"), B.MapActor->GetPlanPreview().Reaction.WatchOrigin,
		GAimOriginScatto);
	TestFalse(TEXT("controllo: a fine corsa a budget il verso opposto alla corsa non e' legale"),
		B.PC->IsFacingLegalForPlan(B.Mine, Indietro));

	// IL CUORE — lo stato nega lo scatto: la regola del catalogo dice ancora si', quella che sposta no.
	B.Mine->ApplyStatus(TAG_Status_Unbalanced, URTCombatLibrary::UnbalancedDurationTurns);
	B.PC->SelectAbilityForCurrentForTest(B.Getto); // la riconferma ridisegna l'anteprima
	TestTrue(TEXT("premessa: per la regola del catalogo lo scatto si applica ancora"), B.Mine->PlannedDashApplies());
	TestFalse(TEXT("ma lo stato lo nega: lo scatto non sposta"), B.Mine->PlannedDashMoves());
	TestEqual(TEXT("e un Attack mira dalla cella corrente"), B.Mine->AimOriginFor(ERTResolutionPhase::Attack),
		GAimOriginQui);
	TestFalse(TEXT("e la portata non arriva piu' a (4,0,0)"), B.MapActor->GetPreviewRangeCells().Contains(Lontano));
	TestEqual(TEXT("e il verso si dichiara da dove l'unita' resta"), B.PC->FacingCellFor(B.Mine), GAimOriginQui);
	TestEqual(TEXT("e l'area colpita parte dalla cella corrente"), B.MapActor->GetPreviewAttackOrigin(), GAimOriginQui);
	TestEqual(TEXT("e la reazione guarda dalla cella corrente"), B.MapActor->GetPlanPreview().Reaction.WatchOrigin,
		GAimOriginQui);
	TestTrue(TEXT("e il verso si giudica da fermo: anche l'opposto alla corsa e' legale"),
		B.PC->IsFacingLegalForPlan(B.Mine, Indietro));

	// CONTROLLO 2 — lo slancio LINEARE, con lo stato, sposta ancora: [D-319] nega la corsa, non lo slancio.
	Scia->Def.MovementStyle = ERTMovementStyle::LinearDash;
	TestTrue(TEXT("controllo: uno scatto lineare con Unbalanced sposta"), B.Mine->PlannedDashMoves());
	TestEqual(TEXT("controllo: e la mira parte dallo scatto"), B.Mine->AimOriginFor(ERTResolutionPhase::Attack),
		GAimOriginScatto);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
