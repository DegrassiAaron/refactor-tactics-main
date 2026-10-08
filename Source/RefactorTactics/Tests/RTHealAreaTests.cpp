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
		TestEqual(TEXT("la morta resta morta"), Morta->Health, 0);
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
		TestTrue(TEXT("e il cooldown NON e' pagato: l'azione non e' partita"), Curatrice->CanUseAbility(Idx));
		TestTrue(TEXT("e nessuna attivazione"), AttivazioneDi(TM, Curatrice) == nullptr);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
