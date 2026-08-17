#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 18.2 — `Wraith.InterceptShot` come **Predictive Action**, verificata sul PERCORSO REALE.
 *
 * Questi tre test girano attraverso `ARTTurnManager::LockInAndResolve`, non sulla libreria pura. La ragione
 * e' [D-036](../../../docs/decisions/RT_PDR_00_Decision_Log.md): un test che non attraversa il percorso reale
 * puo' restare verde per sempre mentre la regola non esiste — era successo con il bonus Wet, che passava
 * senza mai passare dal `TurnManager`. La libreria pura ha gia' i suoi quattro test (CP 18.1); questi
 * verificano che qualcuno la CHIAMI.
 *
 * Prefissi `Pred*` negli helper: unity build, i namespace anonimi si fondono fra file.
 */
namespace
{
	UWorld* MakePredWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPredWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnPredMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
	}

	ARTUnit* SpawnPredUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}

	/**
	 * Arma `Wraith.InterceptShot` sulla cella dichiarata. L'abilita' entra nello slot 3 e non in coda:
	 * `AbilityCooldowns` e' dimensionato sul conteggio originale e non si allarga da solo.
	 */
	bool ArmPredIntercept(ARTUnit* Shooter, const FRTCellId& LockedCell, int32 SlotIndex = 3)
	{
		URTHeroData* Wraith = URTHeroCatalogLibrary::MakeWraith();
		if (!Shooter || !Wraith || !Shooter->Abilities.IsValidIndex(SlotIndex)) { return false; }

		Shooter->Abilities[SlotIndex] = Wraith->Actions[1]; // InterceptShot
		Shooter->PlannedAbilityIndex = SlotIndex;
		Shooter->bAttackTargetsCell = true;   // mira a una CELLA, non a un'unita': e' il punto
		Shooter->PlannedAttackCell = LockedCell;
		Shooter->PlannedAttackTarget = nullptr;
		return true;
	}

	void RunPredTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** La prima voce predittiva del TurnLog, o nullptr. */
	const FRTTurnLogEntry* FindPredEntry(const ARTTurnManager* TM)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Predictive) { return &E; }
		}
		return nullptr;
	}

	constexpr int32 PredInterceptDamage = 16; // dal catalogo eroi, non un numero di questo file
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveInterceptHitTest,
	"RefactorTactics.Predictive.InterceptCellHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveInterceptHitTest::RunTest(const FString&)
{
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);

	ARTUnit* Runner = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(-1, 0));
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 1)); // adiacente alla cella bloccata
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Runner"), Runner) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("TM"), TM) || !TestTrue(TEXT("intercetto armato"), ArmPredIntercept(Shooter, Locked)))
	{
		DestroyPredWorld(World);
		return false;
	}

	// Il Runner ATTRAVERSA la cella bloccata e vorrebbe proseguire fino a (1,0).
	Runner->PlannedCell = FRTCellId(1, 0);
	const int32 Before = Runner->Health;

	RunPredTurn(TM);

	TestEqual(TEXT("chi e' entrato incassa i danni del catalogo"), Before - Runner->Health, PredInterceptDamage);
	// E si FERMA sulla cella bloccata: il troncamento e' meta' dell'azione, e senza di esso la previsione
	// azzeccata costerebbe solo qualche punto vita a chi passa comunque.
	TestTrue(TEXT("e si ferma SULLA cella bloccata, non oltre"), Runner->Cell == Locked);

	const FRTTurnLogEntry* Entry = FindPredEntry(TM);
	if (TestNotNull(TEXT("il TurnLog registra la previsione"), Entry))
	{
		TestEqual(TEXT("come TriggerMatched"), Entry->Outcome,
			static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched));
		TestTrue(TEXT("con la cella su cui si e' scommesso"), Entry->TgtCell == Locked);
		TestEqual(TEXT("e l'identita' dell'azione"), Entry->ActionId, FName(TEXT("Hero.Wraith.InterceptShot")));
	}

	DestroyPredWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveInterceptMissTest,
	"RefactorTactics.Predictive.InterceptCellMiss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveInterceptMissTest::RunTest(const FString&)
{
	// Il gemello del test sopra, ed e' quello che conta: un'implementazione che al boundary cercasse il
	// bersaglio piu' vicino passerebbe `InterceptCellHit` e fallirebbe QUESTO. E' la stessa ragione per cui
	// `Spec.Predictive.WhiffOnEmptyCell` e' stato scritto prima dell'implementazione.
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);
	const FRTCellId Detour(-1, 1);

	ARTUnit* Runner = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(-1, 0));
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Runner"), Runner) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("TM"), TM) || !TestTrue(TEXT("intercetto armato"), ArmPredIntercept(Shooter, Locked)))
	{
		DestroyPredWorld(World);
		return false;
	}

	// Il Runner DEVIA: non passa mai dalla cella prevista.
	Runner->PlannedCell = Detour;
	const int32 Before = Runner->Health;

	RunPredTurn(TM);

	TestEqual(TEXT("chi ha deviato e' illeso"), Runner->Health, Before);
	TestTrue(TEXT("e arriva dove voleva: il whiff non tronca niente"), Runner->Cell == Detour);

	const FRTTurnLogEntry* Entry = FindPredEntry(TM);
	if (TestNotNull(TEXT("il whiff e' registrato, non taciuto"), Entry))
	{
		TestEqual(TEXT("come PredictionWhiffed"), Entry->Outcome,
			static_cast<uint8>(ERTPredictiveOutcome::PredictionWhiffed));
		// La cella c'e' anche quando non e' successo niente: e' l'informazione che spiega il turno.
		TestTrue(TEXT("con la cella su cui si era scommesso"), Entry->TgtCell == Locked);
		TestEqual(TEXT("e nessun danno"), Entry->Amount, 0);
	}

	DestroyPredWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveCrossingNotPresenceTest,
	"RefactorTactics.Predictive.CrossingIsNotPresence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveCrossingNotPresenceTest::RunTest(const FString&)
{
	// La distinzione che il boundary esiste per fare: il colpo prende chi **entra**, non chi **c'e' gia'**.
	// Un'unita' ferma sulla cella prevista non viene colta — non e' entrata, e la previsione riguardava un
	// movimento che non e' avvenuto. Senza questa proprieta' `InterceptShot` diventerebbe un colpo ad area
	// ritardato, cioe' un'altra azione.
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);

	ARTUnit* Sitter = SpawnPredUnit(World, /*Team*/ 0, Locked); // gia' li', e non si muove
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Sitter"), Sitter) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("TM"), TM) || !TestTrue(TEXT("intercetto armato"), ArmPredIntercept(Shooter, Locked)))
	{
		DestroyPredWorld(World);
		return false;
	}

	Sitter->PlannedCell = Locked; // resta fermo
	const int32 Before = Sitter->Health;

	RunPredTurn(TM);

	TestEqual(TEXT("chi era gia' sulla cella NON viene colto"), Sitter->Health, Before);
	TestTrue(TEXT("e resta dov'era"), Sitter->Cell == Locked);

	const FRTTurnLogEntry* Entry = FindPredEntry(TM);
	if (TestNotNull(TEXT("la previsione ha comunque un esito"), Entry))
	{
		// Va a vuoto: la presenza non e' un'entrata, ed e' il whiff a dirlo. Un colpo che scattasse qui
		// renderebbe indistinguibili «ti ho previsto» e «eri li'».
		TestEqual(TEXT("presenza non e' entrata: whiff"), Entry->Outcome,
			static_cast<uint8>(ERTPredictiveOutcome::PredictionWhiffed));
	}

	DestroyPredWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
