#include "Misc/AutomationTest.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h" // TAG_Status_Unbalanced: lo stato che nega lo scatto a budget (#3555)
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexLibrary.h"
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

	/**
	 * La voce dell'azione principale nella timeline posata, cercata per la SUA fase, o `nullptr`.
	 * ⏱️ *Fino a [D-470] si cercava `Attack` per ogni azione: `MakePlanPreview` la etichettava sempre cosi'.*
	 */
	const FRTPhasePreviewEntry* AimOriginMainEntry(const FAimOriginBench& B, ERTResolutionPhase Phase)
	{
		for (const FRTPhasePreviewEntry& Voce : B.MapActor->GetPlanPreview().Phases)
		{
			if (Voce.Phase == Phase)
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
	// 🔴 **Il soggetto e' il RIFIUTO dallo scatto, e serve un attacco che la linea la CHIEDA.** Da D-490 (#3608)
	// l'attacco base mira al buio — `BlindAimDirect`: oltre il muro non si rifiuta piu', il colpo parte e il muro
	// lo ferma. Il banco dichiara quindi `Required` sulla PROPRIA copia di `PressureJet` (`MakeMuiren` crea dati
	// nuovi a ogni chiamata, il kit spedito non cambia): e' l'unica differenza, e la premessa sotto la misura.
	// ⌫ *Fino a #3608 il test usava l'attacco base cosi' com'e'.*
	B.Mine->Abilities[B.Getto]->Def.LineOfSightPolicy = ERTLineOfSightPolicy::Required;
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
	if (const FRTPhasePreviewEntry* Colpo = AimOriginMainEntry(B, ERTResolutionPhase::Attack);
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
	if (const FRTPhasePreviewEntry* Colpo = AimOriginMainEntry(B, ERTResolutionPhase::Environment);
		TestNotNull(TEXT("timeline: c'e' la voce dell'azione, nella sua fase"), Colpo))
	{
		TestEqual(TEXT("timeline: parte dalla cella corrente"), Colpo->PreviewOrigin, GAimOriginQui);
		// [D-470]: il ghost sta dove sara' l'unita' quando l'azione risolve. Nessun Move nel banco: dopo lo scatto.
		TestEqual(TEXT("timeline: e il ghost sta dove lo scatto la lascia"), Colpo->PreviewDestination,
			GAimOriginScatto);
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
	// ⚠️ **La voce Dash resta**, come per ogni scatto pianificato che non si applica: la decide `bDashPlanned`, che
	// non e' un lettore di `PlannedDashApplies()`, e `MakePlanPreview` la vuole visibile. Nasconderla direbbe che lo
	// scatto non e' stato pianificato.
	bool bVoceScatto = false;
	for (const FRTPhasePreviewEntry& Voce : B.MapActor->GetPlanPreview().Phases)
	{
		bVoceScatto |= Voce.Phase == ERTResolutionPhase::FastMovement;
	}
	TestTrue(TEXT("e la voce Dash della timeline resta: lo scatto e' pianificato"), bVoceScatto);

	// CONTROLLO 2 — lo slancio LINEARE, con lo stato, sposta ancora: [D-319] nega la corsa, non lo slancio.
	Scia->Def.MovementStyle = ERTMovementStyle::LinearDash;
	TestTrue(TEXT("controllo: uno scatto lineare con Unbalanced sposta"), B.Mine->PlannedDashMoves());
	TestEqual(TEXT("controllo: e la mira parte dallo scatto"), B.Mine->AimOriginFor(ERTResolutionPhase::Attack),
		GAimOriginScatto);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * **Il canale del Fumo mai esplorato, CHIUSO PER IL GIOCATORE — `#3270`, [D-465], rovesciato da `#2794`.**
 *
 * La decisione d'autore del 2026-10-06 era la via 3: *«si accetta il canale dichiarandolo»*, a termine, finche'
 * `#2794` non portava a click e portata la vista ottimistica di `FindPath`. Questo test lo MISURAVA aperto
 * (verde perche' i due mondi differivano); da `#2794` lo misura **chiuso**: due mondi che differiscono per UNA
 * cella di `Smoke` sulla traiettoria, mai esplorata, escono IDENTICI dai due lettori del giocatore —
 *
 * - il rifiuto per distanza — `DescribeCellTargetRefusal`, `Refusal` e `EffectiveRange`, il numero che
 *   `ARTHUD::RefusalText` scrive a schermo;
 * - la portata viola — `TargetableRangeCells`.
 *
 * **Quella del giocatore e' la mappa che gli arriva da `ARTTurnManager::PlanningMapFor`**, la stessa copia di
 * `FindPath`, e non una mappa costruita qui: il test passa dal punto in cui click e portata la prendono. Dove
 * il Fumo e' invece reale — la mappa autorevole del resolver e della validazione — il cap resta, ed e' il
 * controllo che dice che il Fumo c'e'.
 *
 * 🔑 **L'origine si chiede a `ARTUnit::AimOriginFor(Phase)`**, la funzione che usano click, slot e portata
 * (D-464 (6)), e non a `Unit->Cell`. Con uno scatto pianificato il Fumo sta sulla traiettoria che parte
 * dall'ARRIVO dello scatto, e dalla cella corrente la stessa cella di Fumo non sta sulla linea: lo dice la
 * premessa, ed e' cio' che rende il test capace di cadere sull'origine.
 *
 * ⚠️ **Il Fumo gia' ESPLORATO resta letto** (`Q4`, [D-533]): se la squadra l'ha visto, il cap c'e' anche per il
 * giocatore. E' il controllo positivo senza il quale «uguali» potrebbe voler dire «il Fumo non cappa piu' niente».
 *
 * ⛔ **Mutazione**: far restituire `PlanningMapFor` la mappa autorevole deve rendere ROSSO questo test (i due
 * mondi tornano a differire); ignorare il cap del Fumo in `EffectiveTargetingRange` rende rossi i controlli.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginUnexploredSmokeChannelTest,
	"RefactorTactics.BlindActions.UnexploredSmokeIsReadByClickAndRangeFromTheDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginUnexploredSmokeChannelTest::RunTest(const FString&)
{
	FAimOriginBench B;
	if (!SetUpAimOriginBench(*this, B, FRTCellId(4, 0, 0), FRTCellId(-2, 0, 0)))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const URTActionData* Getto = B.Mine->GetAbility(B.Getto);
	const FRTCellId Qui = B.Mine->AimOriginFor(ERTResolutionPhase::Environment);
	const FRTCellId Scatto = B.Mine->AimOriginFor(ERTResolutionPhase::Attack);
	const FRTCellId Bersaglio(4, -3, 0);
	if (!TestEqual(TEXT("premessa: l'origine di mira di un Attack e' l'arrivo dello scatto"), Scatto,
			GAimOriginScatto)
		|| !TestEqual(TEXT("premessa: e quella corrente no"), Qui, GAimOriginQui))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	ARTTurnManager* TM = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(B.World, ARTTurnManager::StaticClass()));
	if (!TestNotNull(TEXT("premessa: il turn manager del banco"), TM))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}

	// La cella di Fumo e' la PRIMA intermedia della linea dallo scatto: letta dal produttore della linea, non
	// scritta a mano, perche' e' la stessa che il cap scorre.
	const TArray<FRTCellId> LineaDalloScatto = URTHexLibrary::HexLine(Scatto, Bersaglio);
	const TArray<FRTCellId> LineaDaQui = URTHexLibrary::HexLine(Qui, Bersaglio);
	if (!TestEqual(TEXT("premessa: il bersaglio e' a distanza 3 dallo scatto"), LineaDalloScatto.Num(), 4))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	const FRTCellId CellaDiFumo = LineaDalloScatto[1];
	TestFalse(TEXT("premessa: la cella di Fumo NON sta sulla linea dalla cella corrente"),
		LineaDaQui.Contains(CellaDiFumo));

	URTHexMapAsset* Chiaro = URTMatchSetupLibrary::MakeTestArena(B.World);
	URTHexMapAsset* ConFumo = URTMatchSetupLibrary::MakeTestArena(B.World);
	FRTHexCellData Fumo(CellaDiFumo);
	Fumo.Surface = ERTHexSurface::Smoke;
	ConFumo->AddOrUpdateCell(Fumo);
	ConFumo->SortCells();

	const int32 Portata = Getto->RangeCells;
	const ERTLineOfSightPolicy Policy = Getto->Def.LineOfSightPolicy;
	struct FMisura
	{
		FRTCellTargetRefusal Dallo;
		FRTCellTargetRefusal DaQui;
		TArray<FRTCellId> Viola;
	};
	const auto Misura = [&](const URTHexMapAsset* Mappa)
	{
		FMisura M;
		M.Dallo = URTCombatLibrary::DescribeCellTargetRefusal(Mappa, Scatto, Bersaglio, Portata, Policy);
		M.DaQui = URTCombatLibrary::DescribeCellTargetRefusal(Mappa, Qui, Bersaglio, Portata, Policy);
		M.Viola = URTCombatLibrary::TargetableRangeCells(Mappa, Scatto, Portata, Policy);
		return M;
	};

	// LA CONOSCENZA: tutto esplorato tranne la cella di Fumo. Dichiarata, non derivata dalla vista di un eroe.
	const auto Conoscenza = [&](bool bEsploraIlFumo)
	{
		FRTTeamKnowledge K;
		K.TeamId = B.Mine->TeamId;
		K.TurnNumber = 1;
		for (const FRTHexCellData& Cell : ConFumo->Cells)
		{
			if (!bEsploraIlFumo && Cell.Id == CellaDiFumo) { continue; }
			K.ExploredCells.Add(Cell.Id);
		}
		K.VisibleCells.Add(Qui);
		K.ExploredCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
		return K;
	};

	// CONTROLLO — la mappa AUTOREVOLE (resolver e validazione): il Fumo c'e' e cappa. Le premesse del confronto.
	const FMisura Autorevole = Misura(ConFumo);
	const FMisura AutorevoleChiara = Misura(Chiaro);
	TestEqual(TEXT("premessa: senza Fumo dallo scatto non c'e' rifiuto"), AutorevoleChiara.Dallo.Refusal,
		ERTTargetRefusal::None);
	TestEqual(TEXT("premessa: e la portata applicata e' la dichiarata"), AutorevoleChiara.Dallo.EffectiveRange, Portata);
	TestEqual(TEXT("controllo: sulla mappa autorevole il Fumo ignoto rifiuta per distanza"),
		Autorevole.Dallo.Refusal, ERTTargetRefusal::Range);
	TestEqual(TEXT("controllo: col numero del cap"), Autorevole.Dallo.EffectiveRange, 2);

	// IL CANALE CHIUSO — la vista del giocatore, dal punto da cui click e portata la prendono.
	TM->SetTeamKnowledgeForTest(Conoscenza(/*bEsploraIlFumo=*/ false));
	const URTHexMapAsset* VistaChiara = TM->PlanningMapFor(B.Mine->TeamId, Chiaro);
	if (!TestTrue(TEXT("premessa: il giocatore riceve una vista ottimistica, non la mappa autorevole"),
			VistaChiara != nullptr && VistaChiara != Chiaro))
	{
		RTWorldFixtures::DestroyWorld(B.World);
		return false;
	}
	// ⚠️ La cache tiene UNA copia: si misura prima di chiedere la seconda, che la sostituisce.
	const FMisura GiocatoreChiaro = Misura(VistaChiara);
	const URTHexMapAsset* VistaFumo = TM->PlanningMapFor(B.Mine->TeamId, ConFumo);
	TestTrue(TEXT("premessa: anche il mondo con Fumo riceve una vista ottimistica"),
		VistaFumo != nullptr && VistaFumo != ConFumo);
	const FMisura GiocatoreFumo = Misura(VistaFumo);

	TestEqual(TEXT("chiuso: il click dallo scatto non rivela il Fumo ignoto — stesso rifiuto"),
		GiocatoreFumo.Dallo.Refusal, GiocatoreChiaro.Dallo.Refusal);
	TestEqual(TEXT("chiuso: e nessun rifiuto"), GiocatoreFumo.Dallo.Refusal, ERTTargetRefusal::None);
	TestEqual(TEXT("chiuso: ne' il numero mostrato, che e' la portata dichiarata"),
		GiocatoreFumo.Dallo.EffectiveRange, GiocatoreChiaro.Dallo.EffectiveRange);
	TestEqual(TEXT("chiuso: e vale la portata dichiarata"), GiocatoreFumo.Dallo.EffectiveRange, Portata);
	TestEqual(TEXT("controllo: dalla cella corrente il numero non si muove"),
		GiocatoreFumo.DaQui.EffectiveRange, GiocatoreChiaro.DaQui.EffectiveRange);
	TestTrue(TEXT("chiuso: il bersaglio resta nella portata viola"), GiocatoreFumo.Viola.Contains(Bersaglio));
	TestEqual(TEXT("chiuso: e le due portate viola coincidono"), GiocatoreFumo.Viola, GiocatoreChiaro.Viola);

	// CONTROLLO POSITIVO — il Fumo gia' ESPLORATO resta letto (`Q4`): senza questo, «uguali» potrebbe voler dire
	// che l'ottimismo ha tolto il cap a tutti.
	TM->SetTeamKnowledgeForTest(Conoscenza(/*bEsploraIlFumo=*/ true));
	const FMisura GiocatoreEsplorato = Misura(TM->PlanningMapFor(B.Mine->TeamId, ConFumo));
	TestEqual(TEXT("controllo: un Fumo gia' esplorato cappa anche per il giocatore"),
		GiocatoreEsplorato.Dallo.Refusal, ERTTargetRefusal::Range);
	TestEqual(TEXT("controllo: col numero del cap"), GiocatoreEsplorato.Dallo.EffectiveRange, 2);
	TestFalse(TEXT("controllo: e il bersaglio esce dalla portata viola"), GiocatoreEsplorato.Viola.Contains(Bersaglio));

	// Senza conoscenza non c'e' niente da giudicare ottimisticamente: si resta sulla mappa autorevole.
	TM->SetTeamKnowledgeForTest(FRTTeamKnowledge());
	TestTrue(TEXT("conoscenza vuota: la mappa autorevole, come prima"),
		TM->PlanningMapFor(B.Mine->TeamId, ConFumo) == ConFumo);

	RTWorldFixtures::DestroyWorld(B.World);
	return true;
}

/**
 * **Il canale del Fumo mai esplorato, dal CONTROLLER** — `#2794`, [D-465] punto (3).
 *
 * Il test di sopra misura `PlanningMapFor` e i due lettori puri; questo misura che il controller li alimenti davvero
 * con quella mappa. Sono due difetti diversi: un `PlanningMapFor` corretto non serve a niente se la portata viola e il
 * click continuano a leggere `GetHexContext`. Un nemico noto a distanza 3 dallo scatto, con una cella di Fumo sulla
 * linea:
 *
 * - **Fumo ignoto** (la squadra non l'ha mai esplorato): il nemico sta nella portata viola e il click pianifica
 *   l'attacco — il canale e' chiuso;
 * - **Fumo esplorato**: il nemico esce dalla portata e il click non pianifica — il controllo positivo, senza il quale
 *   «dentro la portata» potrebbe voler dire che nessuno legge piu' il Fumo;
 * - **nessuna conoscenza** (sessione senza `ARTGameMode`): come il Fumo esplorato, perche' non c'e' niente da
 *   giudicare ottimisticamente.
 *
 * Verifica di mutazione: togliere `PlayerAimMap` dal ricalcolo dell'anteprima (`RefreshPlanningPreview`) rende rossa
 * la portata; dal click su unita' (`HandleClickOnUnit`) rende rosso il click.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAimOriginUnexploredSmokeThroughTheControllerTest,
	"RefactorTactics.BlindActions.UnexploredSmokeDoesNotRefuseTheSlotOrTheClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAimOriginUnexploredSmokeThroughTheControllerTest::RunTest(const FString&)
{
	struct FEsito
	{
		bool bBanco = false;
		bool bInPortata = false;
		bool bClickPianifica = false;
	};
	// Lo stesso bersaglio del test di sopra: a distanza 3 dallo scatto, con la PRIMA cella intermedia al Fumo.
	const FRTCellId Nemico(4, -3, 0);
	const FRTCellId CellaDiFumo = URTHexLibrary::HexLine(GAimOriginScatto, Nemico)[1];

	// 0 = nessuna conoscenza; 1 = Fumo ignoto; 2 = Fumo esplorato; 3 = nessun Fumo (la premessa del confronto).
	const auto Prova = [&](int32 Conoscenza) -> FEsito
	{
		FEsito Out;
		FAimOriginBench B;
		if (!SetUpAimOriginBench(*this, B, Nemico, FRTCellId(-2, 0, 0)))
		{
			RTWorldFixtures::DestroyWorld(B.World);
			return Out;
		}
		if (Conoscenza != 3)
		{
			const FRTHexCellData* Esistente = B.Map->FindCell(CellaDiFumo);
			FRTHexCellData Fumo = Esistente ? *Esistente : FRTHexCellData(CellaDiFumo);
			Fumo.Surface = ERTHexSurface::Smoke;
			B.Map->AddOrUpdateCell(Fumo);
			B.Map->SortCells();
		}

		ARTTurnManager* TM = Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(B.World, ARTTurnManager::StaticClass()));
		if (Conoscenza > 0 && Conoscenza < 3 && TM)
		{
			FRTTeamKnowledge K;
			K.TeamId = B.Mine->TeamId;
			K.TurnNumber = 1;
			for (const FRTHexCellData& Cell : B.Map->Cells)
			{
				if (Conoscenza == 1 && Cell.Id == CellaDiFumo) { continue; }
				K.ExploredCells.Add(Cell.Id);
			}
			K.VisibleCells.Add(GAimOriginQui);
			K.ExploredCells.Sort([](const FRTCellId& L, const FRTCellId& R) { return URTHexLibrary::StableLess(L, R); });
			TM->SetTeamKnowledgeForTest(K);
		}

		B.PC->ArmKitAbility(B.Getto);
		Out.bBanco = B.Mine->SelectedAbilityIndex == B.Getto
			&& URTHexLibrary::HexDistance(B.Mine->AimOriginFor(ERTResolutionPhase::Attack), Nemico) == 3;
		Out.bInPortata = B.MapActor->GetPreviewRangeCells().Contains(Nemico);
		B.PC->HandleClickOnUnitForTest(B.Primo);
		Out.bClickPianifica = B.Mine->PlannedAttackTarget.Get() == B.Primo;
		RTWorldFixtures::DestroyWorld(B.World);
		return Out;
	};

	const FEsito SenzaConoscenza = Prova(0);
	const FEsito Ignoto = Prova(1);
	const FEsito Esplorato = Prova(2);
	const FEsito SenzaFumo = Prova(3);
	if (!TestTrue(TEXT("premessa: il banco regge e il nemico e' a distanza 3 dallo scatto"),
			SenzaConoscenza.bBanco && Ignoto.bBanco && Esplorato.bBanco && SenzaFumo.bBanco))
	{
		return false;
	}
	// La premessa del confronto: senza il Fumo il nemico sta in portata e si puo' attaccare. Cio' che lo toglie, sopra,
	// e' il Fumo e nient'altro.
	TestTrue(TEXT("premessa: senza Fumo il nemico e' nella portata viola"), SenzaFumo.bInPortata);
	TestTrue(TEXT("premessa: e il click pianifica l'attacco"), SenzaFumo.bClickPianifica);

	TestFalse(TEXT("controllo: senza conoscenza il Fumo cappa e il nemico esce dalla portata"), SenzaConoscenza.bInPortata);
	TestFalse(TEXT("controllo: e il click non pianifica"), SenzaConoscenza.bClickPianifica);
	TestFalse(TEXT("controllo: con il Fumo esplorato il nemico esce dalla portata"), Esplorato.bInPortata);
	TestFalse(TEXT("controllo: e il click non pianifica"), Esplorato.bClickPianifica);

	TestTrue(TEXT("chiuso: con il Fumo ignoto il nemico resta nella portata viola"), Ignoto.bInPortata);
	TestTrue(TEXT("chiuso: e il click pianifica l'attacco"), Ignoto.bClickPianifica);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
