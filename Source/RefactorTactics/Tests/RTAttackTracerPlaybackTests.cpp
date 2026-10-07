// Il Blast in due battiti per colpo, e il tracer che lo racconta (`#2454`, spec §2.3).
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	ARTUnit* SpawnBattitoUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * Il turno di prova: attaccanti della squadra 0 — quella di chi guarda in un mondo senza player controller
	 * (`ARTPlayerState::TeamIdOf(nullptr)` risponde 0) — contro Ivrin. Con `bDue`, Aevik spara anche lui.
	 */
	ARTTurnManager* SetUpBattitoTurn(UWorld* World, bool bDue)
	{
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 8);
		ARTUnit* Branth = SpawnBattitoUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 2));
		ARTUnit* Bersaglio = SpawnBattitoUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Branth || !Bersaglio) { return nullptr; }
		Branth->PlannedAbilityIndex = 0;
		Branth->PlannedAttackTarget = Bersaglio;
		Branth->PlannedCell = Branth->Cell;
		if (bDue)
		{
			ARTUnit* Aevik = SpawnBattitoUnit(World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(3, 4));
			if (!Aevik) { return nullptr; }
			Aevik->PlannedAbilityIndex = 0;
			Aevik->PlannedAttackTarget = Bersaglio;
			Aevik->PlannedCell = Aevik->Cell;
		}
		TM->bRecordAttackBeatsForTest = true;
		return TM;
	}

	/** Tick piccoli finche' il playback e' nel Blast; falso se la risoluzione finisce prima. */
	bool TickUntilBlast(ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			if (TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast) { return true; }
			TM->Tick(0.02f);
		}
		return TM->IsResolving() && TM->CurrentPlaybackPhaseForTest() == ERTMatchPhase::Blast;
	}

	/** L'indice del tick in cui un battito compare per la prima volta nella traccia, o -1. */
	int32 TickOfBeat(const TArray<TPair<int32, FString>>& Visti, const FString& Battito)
	{
		for (const TPair<int32, FString>& V : Visti) { if (V.Value == Battito) { return V.Key; } }
		return -1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackHitArrivesAfterLaunchTest,
	"RefactorTactics.Playback.HitArrivesAfterTheLaunch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackHitArrivesAfterLaunchTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ false);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->LockInAndResolve();
	TArray<TPair<int32, FString>> Visti;
	for (int32 I = 0; I < 600 && TM->IsResolving(); ++I)
	{
		const int32 Prima = TM->AttackBeatTraceForTest().Num();
		TM->Tick(0.02f);
		for (int32 K = Prima; K < TM->AttackBeatTraceForTest().Num(); ++K)
		{
			Visti.Add(TPair<int32, FString>(I, TM->AttackBeatTraceForTest()[K]));
		}
	}

	const int32 Lancio = TickOfBeat(Visti, TEXT("L0"));
	const int32 Arrivo = TickOfBeat(Visti, TEXT("A0"));
	if (!TestTrue(TEXT("premessa: il colpo e' partito ed e' arrivato"), Lancio >= 0 && Arrivo >= 0)) { return false; }
	// 🔴 **La mutazione dichiarata**: con `TracerFlightSeconds = 0` lancio e arrivo cadono nello stesso tick.
	TestTrue(FString::Printf(TEXT("l'arrivo (tick %d) e' DOPO il lancio (tick %d)"), Arrivo, Lancio), Arrivo > Lancio);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackAttackBeatsOrderedInOneTickTest,
	"RefactorTactics.Playback.AttackBeatsStayOrderedInOneTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackAttackBeatsOrderedInOneTickTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTTurnManager* TM = SetUpBattitoTurn(World, /*bDue=*/ true);
	if (!TestNotNull(TEXT("turno di prova"), TM)) { return false; }

	TM->LockInAndResolve();
	if (!TestTrue(TEXT("premessa: il playback arriva al Blast"), TickUntilBlast(TM))) { return false; }

	// Un tick LUNGO, che copre tutti i battiti del Blast: velocita' x4, o un fotogramma lento.
	TM->Tick(5.f);

	// 🔴 **La mutazione dichiarata**: due cicli separati — prima i lanci, poi gli arrivi — danno
	// `L0, L1, A0, A1`, e `Next Action` fermerebbe sull'arrivo di 0 con 1 gia' in volo.
	const TArray<FString> Attesa = { TEXT("L0"), TEXT("A0"), TEXT("L1"), TEXT("A1") };
	TestEqual(TEXT("i battiti escono L0, A0, L1, A1"),
		FString::Join(TM->AttackBeatTraceForTest(), TEXT(",")), FString::Join(Attesa, TEXT(",")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
