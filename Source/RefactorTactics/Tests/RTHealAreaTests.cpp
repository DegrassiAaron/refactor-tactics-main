// LA CURA AD AREA (#3593, spec SP5 §2.1): il percorso delle cure (`CollectHealActions`) conosce la forma `Area`.
//
// 🔑 **Cosa misura questo file**: che `Hero.Muiren.CircularTide`, derivata da `Action.Heal`, curi OGNI compagna nel
// raggio del centro dichiarato — chi cura compresa, i nemici mai — con una voce per destinataria; che l'amount venga
// dalla variante attiva; e che un'area senza nessuno parta lo stesso (cooldown e attivazione) lasciando una voce
// `NoEffect` sul centro. ⛔ Non misura il playback ne' il bot: sono altri task del piano.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo nella voce di fallback
#include "Turn/RTResolvedEvent.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"
#include "RTAbilityFixtures.h"
#include "Ability/RTCatalogLibrary.h"
#include "Map/RTHexCoverLibrary.h"  // #3598: il muro alzato dopo il piano
#include "Map/RTHexVisionLibrary.h" // #3598: la premessa «il muro chiude la linea»

#if WITH_DEV_AUTOMATION_TESTS

// ⚠️ Namespace NOMINATO e proprio: due namespace anonimi con la stessa funzione collidono in unity build.
namespace RTHealAreaTestsInternal
{
	constexpr const TCHAR* TideId = TEXT("Hero.Muiren.CircularTide");

	void SpawnHealMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
	}

	ARTUnit* SpawnHealUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, const URTHeroData* Hero)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Il turno sul percorso reale senza la pianificazione dei bot: i piani sono quelli scritti dal test. */
	void RunHealTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** L'indice nel kit dell'abilita' con questo `ActionId`, letto dal CATALOGO dell'unita' (Task 1: l'indice 1 di Muiren). */
	int32 IndiceDi(const ARTUnit* U, const TCHAR* ActionId)
	{
		for (int32 i = 0; U && i < U->NumAbilities(); ++i)
		{
			const URTActionData* A = U->GetAbility(i);
			if (A && A->Def.ActionId == FName(ActionId)) { return i; }
		}
		return INDEX_NONE;
	}

	/** Pianifica CircularTide di `Curatrice` su una cella. */
	int32 PianificaTideSuCella(ARTUnit* Curatrice, const FRTCellId& Centro)
	{
		const int32 Idx = IndiceDi(Curatrice, TideId);
		Curatrice->PlannedAbilityIndex = Idx;
		Curatrice->PlannedCell = Curatrice->Cell;
		// Il setter della coppia cella/flag (`Unit/RTUnit.h`, `DeclareAttackOnCell`): scrivere i due campi a mano
		// salterebbe l'invariante di `#2884`.
		Curatrice->DeclareAttackOnCell(Centro);
		return Idx;
	}

	/** Pianifica CircularTide di `Curatrice` su un'unita'. */
	int32 PianificaTideSuUnita(ARTUnit* Curatrice, ARTUnit* Bersaglio)
	{
		const int32 Idx = IndiceDi(Curatrice, TideId);
		Curatrice->PlannedAbilityIndex = Idx;
		Curatrice->PlannedCell = Curatrice->Cell;
		Curatrice->DeclareAttackOnUnit(Bersaglio);
		return Idx;
	}

	int32 ConteggioVoci(const ARTTurnManager* TM, ERTLogCategory Cat, uint8 Esito)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog()) { if (E.Category == Cat && E.Outcome == Esito) { ++N; } }
		return N;
	}

	const FRTResolvedEvent* AttivazioneDi(const ARTTurnManager* TM, const ARTUnit* Sorgente)
	{
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Sorgente->StableUnitId) { return &Ev; }
		}
		return nullptr;
	}

	int32 ConteggioFallback(const ARTTurnManager* TM, ERTActionInvalidReason Motivo)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Amount == static_cast<int32>(Motivo)) { ++N; }
		}
		return N;
	}
}

using namespace RTHealAreaTestsInternal;

/**
 * #3593, spec SP5 §2.1 punti 1-6: la cura ad area cura OGNI compagna nel raggio, chi cura compresa, con una
 * voce `Healed` ciascuna; il nemico nel raggio e l'alleata fuori non cambiano; la compagna morta produce la
 * voce `TargetDead` che `ApplyPlannedHeals` gia' scrive per un bersaglio morto; un solo `AbilityActivated`,
 * di forma `Area`, sul centro; il cooldown e' pagato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTideHealsAlliesInAreaTest,
	"RefactorTactics.Heroes.TideHealsAlliesInArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTideHealsAlliesInAreaTest::RunTest(const FString&)
{
	// --- 1. Centro su una cella: tutte le compagne nel raggio ------------------------------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnHealMap(World, 6);
		// Centro (1,0): a distanza 1 stanno (0,0) chi cura, (2,0), (1,-1) le compagne, (1,1) il nemico, (0,1) la morta;
		// (4,0) e' a distanza 3, fuori.
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* A1 = SpawnHealUnit(World, 0, FRTCellId(2, 0, 0), URTHeroCatalogLibrary::MakeAevik());
		ARTUnit* A2 = SpawnHealUnit(World, 0, FRTCellId(1, -1, 0), URTHeroCatalogLibrary::MakeBranth());
		ARTUnit* Lontana = SpawnHealUnit(World, 0, FRTCellId(4, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(1, 1, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTUnit* Morta = SpawnHealUnit(World, 0, FRTCellId(0, 1, 0), URTHeroCatalogLibrary::MakeBranth());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Curatrice || !A1 || !A2 || !Lontana || !Nemico || !Morta || !TM) { return false; }
		Curatrice->Health -= 10; A1->Health -= 40; A2->Health -= 40; Lontana->Health -= 40; Nemico->Health -= 40;
		Morta->Health = 0;
		const int32 PrimaC = Curatrice->Health, PrimaA1 = A1->Health, PrimaA2 = A2->Health, PrimaL = Lontana->Health, PrimaN = Nemico->Health;
		const int32 Idx = PianificaTideSuCella(Curatrice, FRTCellId(1, 0, 0));
		if (!TestTrue(TEXT("premessa: Muiren ha CircularTide"), Idx != INDEX_NONE)) { return false; }
		RunHealTurn(TM);
		TestEqual(TEXT("compagna A1 curata di 18"), A1->Health - PrimaA1, 18);
		TestEqual(TEXT("compagna A2 curata di 18"), A2->Health - PrimaA2, 18);
		TestEqual(TEXT("chi cura e' nel raggio: +10, tetto a MaxHealth"), Curatrice->Health - PrimaC, 10);
		TestEqual(TEXT("fuori raggio: invariata"), Lontana->Health, PrimaL);
		TestEqual(TEXT("il nemico nel raggio non guarisce"), Nemico->Health, PrimaN);
		TestEqual(TEXT("tre voci Healed"), ConteggioVoci(TM, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Healed)), 3);
		TestEqual(TEXT("una voce di fallback: la morta"), ConteggioVoci(TM, ERTLogCategory::Fallback, static_cast<uint8>(ERTFallbackOutcome::Cancelled)), 1);
		TestTrue(TEXT("e dice TargetDead"), ConteggioFallback(TM, ERTActionInvalidReason::TargetDead) == 1);
		int32 Attivazioni = 0; const FRTResolvedEvent* Beat = nullptr;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.SourceStableUnitId == Curatrice->StableUnitId) { ++Attivazioni; Beat = &Ev; }
		}
		TestEqual(TEXT("un solo AbilityActivated"), Attivazioni, 1);
		if (Beat)
		{
			TestTrue(TEXT("di forma Area"), Beat->Shape == ERTAbilityShape::Area);
			TestTrue(TEXT("sul centro"), Beat->AimCell == FRTCellId(1, 0, 0));
			TestEqual(TEXT("nessun bersaglio-unita'"), Beat->TargetStableUnitId, 0);
		}
		TestFalse(TEXT("il cooldown e' pagato"), Curatrice->CanUseAbility(Idx));
	}

	// --- 2. Chi cura e' l'unica compagna nel raggio: una voce Healed, nessun fallback -------------------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova (sola curatrice)"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnHealMap(World, 6);
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(-4, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Curatrice || !Nemico || !TM) { return false; }
		Curatrice->Health -= 10;
		const int32 Prima = Curatrice->Health;
		PianificaTideSuCella(Curatrice, FRTCellId(0, 0, 0));
		RunHealTurn(TM);
		TestEqual(TEXT("chi cura, sola nel raggio: +10"), Curatrice->Health - Prima, 10);
		TestEqual(TEXT("una sola voce Healed"), ConteggioVoci(TM, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Healed)), 1);
		TestEqual(TEXT("nessun fallback"), ConteggioVoci(TM, ERTLogCategory::Fallback, static_cast<uint8>(ERTFallbackOutcome::Cancelled)), 0);
	}

	// --- 3. Bersaglio-unita' dichiarato (nessuna cella): il centro e' la sua cella, l'id e' il suo ------
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova (bersaglio-unita')"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnHealMap(World, 6);
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* A1 = SpawnHealUnit(World, 0, FRTCellId(2, 0, 0), URTHeroCatalogLibrary::MakeAevik());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(-4, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Curatrice || !A1 || !Nemico || !TM) { return false; }
		A1->Health -= 40;
		const int32 Prima = A1->Health;
		PianificaTideSuUnita(Curatrice, A1);
		RunHealTurn(TM);
		TestEqual(TEXT("A1 curata di 18"), A1->Health - Prima, 18);
		const FRTResolvedEvent* Beat = AttivazioneDi(TM, Curatrice);
		if (TestNotNull(TEXT("l'attivazione e' emessa"), Beat))
		{
			TestEqual(TEXT("il bersaglio-unita' e' A1"), Beat->TargetStableUnitId, A1->StableUnitId);
			TestTrue(TEXT("e il centro e' la sua cella"), Beat->AimCell == A1->Cell);
		}
	}
	return true;
}

/** #3593, spec §2.1 punto 6: l'amount viene dalla variante attiva (`Healing` 24), altrimenti da `Def`. Review Focus 4: `Impact` cura 10 e non spinge. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTideHealsVariantAmountTest,
	"RefactorTactics.Heroes.TideHealsVariantAmount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTideHealsVariantAmountTest::RunTest(const FString&)
{
	auto Corsa = [this](const TCHAR* Variante, int32 Atteso) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnHealMap(World, 6);
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* A1 = SpawnHealUnit(World, 0, FRTCellId(2, 0, 0), URTHeroCatalogLibrary::MakeAevik());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(3, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Curatrice || !A1 || !Nemico || !TM) { return false; }
		A1->Health -= 40;
		const int32 Prima = A1->Health;
		const FRTCellId CellaNemico = Nemico->Cell;
		Curatrice->ActiveVariantId = FName(Variante);
		PianificaTideSuCella(Curatrice, FRTCellId(2, 0, 0));
		RunHealTurn(TM);
		TestEqual(*FString::Printf(TEXT("variante '%s' cura %d"), Variante, Atteso), A1->Health - Prima, Atteso);
		TestTrue(TEXT("il nemico nel raggio non si sposta (R3: la spinta di Impact non passa dalle cure)"), Nemico->Cell == CellaNemico);
		return true;
	};
	if (!Corsa(TEXT("Hero.Muiren.CircularTide.Healing"), 24)) { return false; }
	if (!Corsa(TEXT("Hero.Muiren.CircularTide.Impact"), 10)) { return false; }
	if (!Corsa(TEXT(""), 18)) { return false; }
	return true;
}

/** #3593, spec §2.1 punto 4 (R2 ribaltato): un'area senza nessuno della squadra PARTE — cooldown pagato, attivazione emessa — e lascia una sola voce `NoEffect` sul centro. Review Focus 3: fuori portata invece NON parte. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTideOnEmptyAreaStillStartsTest,
	"RefactorTactics.Heroes.TideOnEmptyAreaStillStarts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTideOnEmptyAreaStillStartsTest::RunTest(const FString&)
{
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnHealMap(World, 6);
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(-3, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Curatrice || !Nemico || !TM) { return false; }
		const FRTCellId Centro(4, -4, 0); // distanza 4 da (0,0): dentro la portata, nessuno intorno
		const int32 Idx = PianificaTideSuCella(Curatrice, Centro);
		if (!TestTrue(TEXT("premessa: Muiren ha CircularTide"), Idx != INDEX_NONE)) { return false; }
		RunHealTurn(TM);
		TestEqual(TEXT("nessuna Healed"), ConteggioVoci(TM, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Healed)), 0);
		int32 Vuote = 0; bool bSulCentro = false;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Amount == static_cast<int32>(ERTActionInvalidReason::NoEffect)) { ++Vuote; bSulCentro = (E.TgtCell == Centro); }
		}
		TestEqual(TEXT("una voce NoEffect"), Vuote, 1);
		TestTrue(TEXT("con TgtCell = centro, non la cella di chi cura"), bSulCentro);
		TestFalse(TEXT("il cooldown e' pagato: l'azione e' partita"), Curatrice->CanUseAbility(Idx));
		TestNotNull(TEXT("l'attivazione e' emessa"), AttivazioneDi(TM, Curatrice));
	}
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova (fuori portata)"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
		SpawnHealMap(World, 6);
		ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
		ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(-3, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Curatrice || !Nemico || !TM) { return false; }
		const int32 Idx = PianificaTideSuCella(Curatrice, FRTCellId(5, -5, 0)); // distanza 5: fuori portata 4
		RunHealTurn(TM);
		TestEqual(TEXT("fuori portata: una voce OutOfRange"), ConteggioFallback(TM, ERTActionInvalidReason::OutOfRange), 1);
		bool bSulCentroMirato = false;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Fallback && E.Amount == static_cast<int32>(ERTActionInvalidReason::OutOfRange)) { bSulCentroMirato = (E.TgtCell == FRTCellId(5, -5, 0)); }
		}
		TestTrue(TEXT("e la voce dice DOVE mirava: TgtCell = centro dichiarato"), bSulCentroMirato);
		TestTrue(TEXT("e il cooldown NON e' pagato: l'azione non e' partita"), Curatrice->CanUseAbility(Idx));
		TestTrue(TEXT("e nessuna attivazione"), AttivazioneDi(TM, Curatrice) == nullptr);
	}
	return true;
}

/**
 * **Ogni cura SINGOLA dichiara di agganciare, e nessuna cura ad AREA lo fa** — D-493 (#3609).
 *
 * La decisione d'autore: *«aggancia l'unità, come eccezione dichiarata in catalogo. il cura ha un tipo di tiro
 * diverso da un normale shoot»*. La dichiarazione e' `Fallback = AttackTarget` (`FRTActionDef::DeclaresTracking`);
 * un'area agisce invece sulle celle attorno alla mira congelata, come ogni azione ([D-419]).
 *
 * 🔴 **E' il gate che rende coerente il ramo della cura in `CollectAttackIntents`**: una cura singola che NON
 * agganciasse misurerebbe la portata sulla mira congelata e curerebbe l'identita' dovunque sia — due meta' dello
 * stesso gesto decise da due cose diverse. Il catalogo non deve poterla spedire.
 *
 * ⛔ Anti-vacuita': devono esistere almeno una cura singola (il core) e almeno una ad area (`CircularTide`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEverySingleHealDeclaresTrackingTest,
	"RefactorTactics.Heroes.EverySingleHealDeclaresTracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEverySingleHealDeclaresTrackingTest::RunTest(const FString&)
{
	static const FName HealId(TEXT("Action.Heal"));
	int32 Singole = 0;
	int32 AdArea = 0;

	// Il core: `FRTActionDef` non porta una forma, quindi la cura del catalogo e' a bersaglio singolo.
	const FRTActionDef Core = URTCatalogLibrary::FindCoreAction(HealId);
	if (!TestEqual(TEXT("premessa: la cura e' a catalogo"), Core.ActionId, HealId)) { return false; }
	TestTrue(TEXT("la cura del core (singola) dichiara di agganciare"), Core.DeclaresTracking());
	++Singole;

	const TArray<URTHeroData*> Eroi = {
		URTHeroCatalogLibrary::MakeAevik(), URTHeroCatalogLibrary::MakeMuiren(),
		URTHeroCatalogLibrary::MakeBranth(), URTHeroCatalogLibrary::MakeIvrin() };
	for (const URTHeroData* Eroe : Eroi)
	{
		if (!TestNotNull(TEXT("eroe costruito"), Eroe)) { return false; }
		for (const TObjectPtr<URTActionData>& Ptr : Eroe->Actions)
		{
			const URTActionData* A = Ptr.Get();
			if (!A || (A->Def.ActionId != HealId && A->Def.DerivedFromActionId != HealId)) { continue; }
			if (A->Shape == ERTAbilityShape::Area)
			{
				++AdArea;
				TestFalse(FString::Printf(TEXT("%s: la cura ad AREA non aggancia, la sua mira si congela"),
					*A->Def.ActionId.ToString()), A->Def.DeclaresTracking());
			}
			else
			{
				++Singole;
				TestTrue(FString::Printf(TEXT("%s: la cura SINGOLA dichiara di agganciare"),
					*A->Def.ActionId.ToString()), A->Def.DeclaresTracking());
			}
		}
	}
	TestTrue(TEXT("anti-vacuita': c'e' almeno una cura singola"), Singole > 0);
	TestTrue(TEXT("anti-vacuita': c'e' almeno una cura ad area"), AdArea > 0);
	return true;
}

/**
 * **La cura ad AREA puntata su un'unita' resta dove e' stata puntata** — [D-419], D-493 (#3609).
 *
 * L'alleato puntato scatta fuori dal raggio prima del Blast; un secondo alleato sta accanto alla cella dove il primo
 * era al lock-in. Con la mira congelata la Tide cura il secondo e manca il primo; con la mira che insegue — com'era
 * fino a #3609 — avrebbe fatto il contrario. Le due scene danno esiti OPPOSTI, ed e' cio' che rende il test una misura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTideOnAUnitStaysWhereAimedTest,
	"RefactorTactics.Heroes.TideOnAUnitStaysWhereItWasAimed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTideOnAUnitStaysWhereAimedTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnHealMap(World, 6);

	const FRTCellId Puntata(2, 0, 0);
	const FRTCellId Scattata(4, 0, 0);
	const FRTCellId Accanto(2, -1, 0); // a un passo dalla cella puntata, a tre da quella dello scatto
	ARTUnit* Curatrice = SpawnHealUnit(World, 0, FRTCellId(0, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
	ARTUnit* Puntato = SpawnHealUnit(World, 0, Puntata, URTHeroCatalogLibrary::MakeAevik());
	ARTUnit* Vicino = SpawnHealUnit(World, 0, Accanto, URTHeroCatalogLibrary::MakeBranth());
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Curatrice || !Puntato || !Vicino || !TM) { return false; }
	Puntato->Health -= 40;
	Vicino->Health -= 40;
	const int32 PrimaP = Puntato->Health, PrimaV = Vicino->Health;

	const int32 Idx = PianificaTideSuUnita(Curatrice, Puntato);
	if (!TestTrue(TEXT("premessa: Muiren ha CircularTide"), Idx != INDEX_NONE)) { return false; }
	const int32 Scatto = RTAbilityFixtures::AddCoreAbility(Puntato, TEXT("Action.Reposition"));
	Puntato->PlannedDashAbility = Scatto;
	Puntato->PlannedDashCell = Scattata;
	RunHealTurn(TM);

	if (!TestEqual(TEXT("premessa: l'alleato puntato e' davvero scattato via"), Puntato->Cell, Scattata)) { return false; }
	TestEqual(TEXT("🔴 chi sta accanto alla cella puntata e' curato: l'area e' rimasta li'"), Vicino->Health - PrimaV, 18);
	TestEqual(TEXT("🔴 l'alleato scattato via e' uscito dall'area: niente cura"), Puntato->Health, PrimaP);
	return true;
}

/**
 * [D-504], `#3598` — **nel Blast le cure ricontrollano la linea di tiro, come gli attacchi.**
 *
 * 🔑 **Il muro si posa DOPO che il piano e' scritto**, cioe' dove un muro alzato in Prep sta in partita: i piani di
 * questi test non passano dalla validazione della pianificazione, quindi l'unico controllo che la cura incontra e'
 * quello del Blast — che e' cio' che il test misura. Ogni caso ha il suo gemello senza muro: senza, «non curata»
 * non distinguerebbe un muro che ferma da una cura che non parte per un'altra ragione.
 *
 * Due forme, perche' il centro si legge in due modi: la cura SINGOLA verso l'alleato agganciato, l'AREA verso la
 * mira congelata. La cura bloccata e' spesa ([D-200]): cooldown pagato e gesto emesso, voce `NoLineOfSight`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHealLineOfSightIsCheckedInTheBlastTest,
	"RefactorTactics.Actions.Heal.LineOfSightIsCheckedInTheBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHealLineOfSightIsCheckedInTheBlastTest::RunTest(const FString&)
{
	// Chi cura in (0,0), l'alleata ferita in (2,0); il muro alto sta sul bordo fra (1,0) e (2,0).
	const FRTCellId CellaCura(0, 0, 0);
	const FRTCellId CellaFerita(2, 0, 0);

	for (const bool bArea : { false, true })
	{
		for (const bool bMuro : { true, false })
		{
			const FString Caso = FString::Printf(TEXT("%s, %s"), bArea ? TEXT("area") : TEXT("singola"),
				bMuro ? TEXT("muro") : TEXT("senza muro"));
			UWorld* World = RTWorldFixtures::MakeWorld();
			if (!TestNotNull(*FString::Printf(TEXT("mondo di prova (%s)"), *Caso), World)) { return false; }
			ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
			SpawnHealMap(World, 6);
			ARTUnit* Curatrice = SpawnHealUnit(World, 0, CellaCura, URTHeroCatalogLibrary::MakeMuiren());
			ARTUnit* Ferita = SpawnHealUnit(World, 0, CellaFerita, URTHeroCatalogLibrary::MakeAevik());
			ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(-4, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
			ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
			ARTHexMapActor* MapActor = ARTHexMapActor::FindInWorld(World);
			if (!Curatrice || !Ferita || !Nemico || !TM || !MapActor || !MapActor->MapAsset) { return false; }
			Ferita->Health -= 40;
			const int32 Prima = Ferita->Health;

			int32 Idx = INDEX_NONE;
			if (bArea)
			{
				Idx = PianificaTideSuCella(Curatrice, CellaFerita);
			}
			else
			{
				// ⚠️ **In uno SLOT del kit, non appesa**: `AbilityCooldowns` copre solo gli slot dell'eroe, e su
				// un'abilita' appesa il cooldown non si scrive mai (`AddCoreAbilityInSlot`). E la ricarica sale a
				// 2: quella di catalogo (1) si azzera nel Cleanup dello stesso turno, e «pagato» non si vedrebbe.
				Idx = RTAbilityFixtures::AddCoreAbilityInSlot(Curatrice, TEXT("Action.Heal"), 3);
				if (Idx != INDEX_NONE) { Curatrice->Abilities[Idx]->CooldownTurns = 2; }
				Curatrice->PlannedAbilityIndex = Idx;
				Curatrice->PlannedCell = Curatrice->Cell;
				Curatrice->DeclareAttackOnUnit(Ferita);
			}
			if (!TestTrue(*FString::Printf(TEXT("premessa: la cura c'e' (%s)"), *Caso), Idx != INDEX_NONE)) { return false; }
			if (!TestTrue(*FString::Printf(TEXT("premessa: la cura dichiara la linea di tiro (%s)"), *Caso),
					Curatrice->GetAbility(Idx)->Def.LineOfSightPolicy == ERTLineOfSightPolicy::Required))
			{
				return false;
			}

			if (bMuro)
			{
				URTHexCoverLibrary::AddCover(MapActor->MapAsset, CellaFerita, ERTHexDirection::W, ERTHexCoverType::High, 30);
				if (!TestFalse(TEXT("⛔ premessa: il muro chiude la linea"),
						URTHexVisionLibrary::HasLineOfSight(MapActor->MapAsset, CellaCura, CellaFerita)))
				{
					return false;
				}
			}

			RunHealTurn(TM);

			const int32 Bloccate = ConteggioFallback(TM, ERTActionInvalidReason::NoLineOfSight);
			if (bMuro)
			{
				TestEqual(*FString::Printf(TEXT("🔴 il muro ferma la cura (%s)"), *Caso), Ferita->Health, Prima);
				TestEqual(*FString::Printf(TEXT("e lascia una voce NoLineOfSight (%s)"), *Caso), Bloccate, 1);
				TestEqual(*FString::Printf(TEXT("nessuna voce Healed (%s)"), *Caso),
					ConteggioVoci(TM, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Healed)), 0);
			}
			else
			{
				TestTrue(*FString::Printf(TEXT("senza muro la cura arriva (%s)"), *Caso), Ferita->Health > Prima);
				TestEqual(*FString::Printf(TEXT("e non c'e' nessuna voce NoLineOfSight (%s)"), *Caso), Bloccate, 0);
			}
			// Spesa in entrambi i casi ([D-200]): il gesto si vede e il cooldown si paga.
			TestNotNull(*FString::Printf(TEXT("il gesto si emette (%s)"), *Caso), AttivazioneDi(TM, Curatrice));
			TestFalse(*FString::Printf(TEXT("il cooldown e' pagato (%s)"), *Caso), Curatrice->CanUseAbility(Idx));
		}
	}
	return true;
}

/**
 * [D-505], `#3600` — **la cura ha un momento sul curato**: un `Healed` per ogni voce `Healed` del TurnLog, e a
 * playback finito il token del curato dice `+N`.
 *
 * 🔑 **Tre forme in un turno**: una cura singola su una ferita, un'area con due ferite e una caduta. La caduta non
 * produce il momento — non e' stata curata —, le altre si', con l'importo restituito davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHealEmitsItsMomentTest,
	"RefactorTactics.Actions.Heal.EmitsAMomentOnTheHealed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHealEmitsItsMomentTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };
	SpawnHealMap(World, 6);
	// Curatrice singola in (-3,0) su Lontana (-1,0); CircularTide da (3,0) centrata su (2,0): A1 (2,0), A2 (1,0), Morta (2,-1).
	ARTUnit* Singola = SpawnHealUnit(World, 0, FRTCellId(-3, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
	ARTUnit* Lontana = SpawnHealUnit(World, 0, FRTCellId(-1, 0, 0), URTHeroCatalogLibrary::MakeBranth());
	ARTUnit* Tide = SpawnHealUnit(World, 0, FRTCellId(3, 0, 0), URTHeroCatalogLibrary::MakeMuiren());
	ARTUnit* A1 = SpawnHealUnit(World, 0, FRTCellId(2, 0, 0), URTHeroCatalogLibrary::MakeAevik());
	ARTUnit* A2 = SpawnHealUnit(World, 0, FRTCellId(1, 0, 0), URTHeroCatalogLibrary::MakeIvrin());
	ARTUnit* Morta = SpawnHealUnit(World, 0, FRTCellId(2, -1, 0), URTHeroCatalogLibrary::MakeBranth());
	ARTUnit* Nemico = SpawnHealUnit(World, 1, FRTCellId(-5, 3, 0), URTHeroCatalogLibrary::MakeIvrin());
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!Singola || !Lontana || !Tide || !A1 || !A2 || !Morta || !Nemico || !TM) { return false; }
	Lontana->Health -= 40; A1->Health -= 40; A2->Health -= 40; Morta->Health = 0;
	const int32 PrimaL = Lontana->Health, PrimaA1 = A1->Health, PrimaA2 = A2->Health;

	const int32 Cura = RTAbilityFixtures::AddCoreAbility(Singola, TEXT("Action.Heal"));
	Singola->PlannedAbilityIndex = Cura;
	Singola->PlannedCell = Singola->Cell;
	Singola->DeclareAttackOnUnit(Lontana);
	if (!TestTrue(TEXT("premessa: CircularTide c'e'"), PianificaTideSuCella(Tide, FRTCellId(2, 0, 0)) != INDEX_NONE)) { return false; }

	RunHealTurn(TM);

	TArray<const FRTResolvedEvent*> Momenti;
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::Healed) { Momenti.Add(&Ev); }
	}
	TestEqual(TEXT("🔴 un momento per voce Healed del TurnLog"), Momenti.Num(),
		ConteggioVoci(TM, ERTLogCategory::Combat, static_cast<uint8>(ERTCombatOutcome::Healed)));
	auto MomentoDi = [&Momenti](const ARTUnit* U) -> const FRTResolvedEvent*
	{
		for (const FRTResolvedEvent* Ev : Momenti) { if (Ev->TargetStableUnitId == U->StableUnitId) { return Ev; } }
		return nullptr;
	};
	if (const FRTResolvedEvent* M = MomentoDi(Lontana); TestNotNull(TEXT("la ferita della cura singola ha il suo momento"), M))
	{
		TestEqual(TEXT("chi ha curato"), M->SourceStableUnitId, Singola->StableUnitId);
		TestEqual(TEXT("quanto, davvero"), M->Amount, Lontana->Health - PrimaL);
		TestEqual(TEXT("con quale azione"), M->ActionId, FName(TEXT("Action.Heal")));
		TestTrue(TEXT("nel Blast"), M->Phase == ERTMatchPhase::Blast);
	}
	if (const FRTResolvedEvent* M = MomentoDi(A1); TestNotNull(TEXT("A1, nell'area"), M))
	{
		TestEqual(TEXT("A1: quanto, davvero"), M->Amount, A1->Health - PrimaA1);
		TestEqual(TEXT("A1: l'azione della cura ad area"), M->ActionId, FName(TEXT("Hero.Muiren.CircularTide")));
	}
	TestNotNull(TEXT("A2, nell'area"), MomentoDi(A2));
	TestNull(TEXT("⛔ la caduta non e' stata curata: nessun momento"), MomentoDi(Morta));

	// A playback finito il token del curato e' quello della cura.
	TestEqual(TEXT("🔴 il token della ferita dice «+N»"), Lontana->LastDamageToken.Label,
		FString::Printf(TEXT("+%d"), Lontana->Health - PrimaL));
	TestTrue(TEXT("ed e' una cura"), Lontana->LastDamageToken.bIsHeal);
	TestEqual(TEXT("anche nell'area"), A2->LastDamageToken.Label, FString::Printf(TEXT("+%d"), A2->Health - PrimaA2));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
