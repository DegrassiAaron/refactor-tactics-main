// I due bersagli dell'azione principale sono ESCLUSIVI (`#2884`).
//
// `PlannedAttackTarget` e la coppia `bAttackTargetsCell`/`PlannedAttackCell` descrivono la stessa scelta in
// due modi, e il resolver ne legge uno solo: `Blast` guarda PRIMA il flag e, quando e' acceso, **ignora** il
// bersaglio-unita'. Finche' l'esclusivita' e' stata una convenzione invece che una funzione, nessuno l'ha
// applicata — due scrittori del flag, quattro lettori, zero azzeramenti.
//
// 🔴 **Questi test attraversano DUE turni e il percorso di input reale, e nessuno dei due e' un dettaglio.**
// Il difetto e' sopravvissuto proprio perche' la suite costruiva un turno solo, o scriveva i campi `Planned*`
// a mano: un test che non cambia idea non puo' vedere un piano che non si ritira.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTCellId.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Banco condiviso: arena piatta, un'unita' propria e un nemico ADIACENTE. Nomi distinti per unity build. */
	struct FPlanBench
	{
		UWorld* World = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Mine = nullptr;
		ARTUnit* Foe = nullptr;
		int32 AreaIdx = INDEX_NONE;
	};

	ARTUnit* SpawnPlanUnit(UWorld* World, int32 Team, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = Team;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	bool SetUpPlanBench(FPlanBench& B)
	{
		B.World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (!B.World) { return false; }
		if (GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(B.World);
		}

		URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeFlatArena(B.World, 5);
		ARTHexMapActor* MapActor = B.World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = Arena;
		B.TM = B.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

		B.Mine = SpawnPlanUnit(B.World, 0, URTHeroCatalogLibrary::MakeAevik(), FRTCellId(0, 0, 0));
		B.Foe = SpawnPlanUnit(B.World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(1, 0, 0));
		B.PC = B.World->SpawnActor<ARTPlayerController>();
		if (!B.TM || !B.Mine || !B.Foe || !B.PC) { return false; }

		// L'azione ad area si cerca per proprieta', non per indice: sopravvive a un riordino del kit.
		for (int32 i = 0; i < B.Mine->NumAbilities(); ++i)
		{
			const URTActionData* A = B.Mine->GetAbility(i);
			if (A && A->Shape == ERTAbilityShape::Area && !A->bSelfTarget) { B.AreaIdx = i; break; }
		}
		B.PC->SelectActorForTest(B.Mine);
		return B.AreaIdx != INDEX_NONE;
	}

	void TearDownPlanBench(FPlanBench& B)
	{
		if (B.World && GEngine)
		{
			GEngine->DestroyWorldContext(B.World);
			B.World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void ResolvePlanTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	/** Quanto ha incassato, scudo compreso: e' l'oracolo che non dipende da come il danno si ripartisce. */
	int32 DamageTaken(const ARTUnit* U, int32 HealthBefore, int32 ShieldBefore)
	{
		return (HealthBefore - U->Health) + (ShieldBefore - U->Shield);
	}
}

/**
 * **IL CASO CHE DECIDE.** Nello stesso turno: prima si punta una cella, poi si CAMBIA IDEA e si punta un
 * nemico. Il colpo deve arrivare al nemico.
 *
 * 🔴 **E' il caso che l'uscita «azzera al consumo del piano» non avrebbe visto**, e la ragione per cui
 * l'esclusivita' vive dove si DICHIARA e non dove si consuma. Misurato prima della correzione: il nemico
 * puntato incassava `0` — il Blast risolveva sulla cella di prima, in silenzio.
 *
 * ⛔ **Verifica di mutazione**: togliere `bAttackTargetsCell = false` da `ARTUnit::DeclareAttackOnUnit` deve
 * rendere ROSSO questo test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanMindChangeTest,
	"RefactorTactics.Plan.MindChangeRetiresTheOtherTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanMindChangeTest::RunTest(const FString&)
{
	FPlanBench B;
	if (!TestTrue(TEXT("banco di prova con un'area nel kit"), SetUpPlanBench(B)))
	{
		TearDownPlanBench(B); return false;
	}

	// 1. Si punta una cella LONTANA dal nemico: se il piano restasse quello, il nemico non verrebbe sfiorato.
	B.Mine->SelectAbility(B.AreaIdx);
	const FRTCellId Lontana(-3, 0, 0);
	if (!TestTrue(TEXT("premessa: l'area a cella e' accettata"), B.PC->HandleTargetCell(Lontana)))
	{
		TearDownPlanBench(B); return false;
	}
	TestTrue(TEXT("premessa: il piano dichiara una cella"), B.Mine->bAttackTargetsCell);

	// 2. Cambio d'idea, dallo stesso percorso che usa un giocatore.
	B.Mine->SelectAbility(0); // attacco base: punta un'UNITA'
	B.PC->HandleClickOnUnitForTest(B.Foe);

	// 🔑 L'asserto sullo STATO, che nomina il difetto: la dichiarazione opposta e' stata ritirata.
	TestFalse(TEXT("la dichiarazione a cella e' stata ritirata"), B.Mine->bAttackTargetsCell);
	TestTrue(TEXT("e il bersaglio-unita' e' quello cliccato"), B.Mine->PlannedAttackTarget == B.Foe);

	// 🔑 E l'asserto sull'ESITO, che e' quello che il giocatore vede: il colpo arriva.
	const int32 HP = B.Foe->Health;
	const int32 Scudo = B.Foe->Shield;
	ResolvePlanTurn(B.TM);
	TestTrue(TEXT("il nemico puntato incassa il colpo"), DamageTaken(B.Foe, HP, Scudo) > 0);

	TearDownPlanBench(B);
	return true;
}

/**
 * Il piano non sopravvive al proprio turno: dopo la risoluzione nessuna delle due forme resta dichiarata.
 *
 * ⚠️ **Due turni, ed e' la parte che nessun test copriva.** Il difetto viveva qui: `bAttackTargetsCell`
 * restava acceso e `PlannedAttackCell` conservava la cella del turno prima, quindi il primo attacco del
 * turno seguente partiva verso di essa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanDoesNotSurviveItsTurnTest,
	"RefactorTactics.Plan.PlanDoesNotSurviveItsTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanDoesNotSurviveItsTurnTest::RunTest(const FString&)
{
	FPlanBench B;
	if (!TestTrue(TEXT("banco di prova con un'area nel kit"), SetUpPlanBench(B)))
	{
		TearDownPlanBench(B); return false;
	}

	// Turno 1: area su una cella lontana dal nemico.
	B.Mine->SelectAbility(B.AreaIdx);
	if (!TestTrue(TEXT("premessa: l'area a cella e' accettata"), B.PC->HandleTargetCell(FRTCellId(-3, 0, 0))))
	{
		TearDownPlanBench(B); return false;
	}
	ResolvePlanTurn(B.TM);

	TestFalse(TEXT("dopo la risoluzione la cella non e' piu' dichiarata"), B.Mine->bAttackTargetsCell);
	TestNull(TEXT("e nemmeno un bersaglio-unita'"), (void*)B.Mine->PlannedAttackTarget.Get());

	// Turno 2: attacco base sul nemico adiacente. Deve arrivare a segno.
	B.Mine->SelectAbility(0);
	B.PC->HandleClickOnUnitForTest(B.Foe);
	const int32 HP = B.Foe->Health;
	const int32 Scudo = B.Foe->Shield;
	ResolvePlanTurn(B.TM);
	TestTrue(TEXT("il nemico puntato al turno 2 incassa il colpo"), DamageTaken(B.Foe, HP, Scudo) > 0);

	TearDownPlanBench(B);
	return true;
}

/**
 * ✅ **IL CONTROLLO POSITIVO, e senza di lui i due test qui sopra si soddisfano rompendo tutto.**
 *
 * Un'implementazione che azzerasse `bAttackTargetsCell` *troppo presto* — per esempio in cima al ciclo del
 * Blast, dove il piano si consuma — li farebbe passare entrambi e toglierebbe al gioco il bersaglio a cella,
 * cioe' esattamente cio' che `#2870` ha appena reso raggiungibile. E' il difetto opposto, e ha lo stesso
 * aspetto se si guardano solo gli asserti sul nemico.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanCellTargetStillResolvesTest,
	"RefactorTactics.Plan.CellTargetStillResolvesOnTheCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanCellTargetStillResolvesTest::RunTest(const FString&)
{
	FPlanBench B;
	if (!TestTrue(TEXT("banco di prova con un'area nel kit"), SetUpPlanBench(B)))
	{
		TearDownPlanBench(B); return false;
	}

	// L'area si centra sulla cella del nemico ADIACENTE: nessun bersaglio-unita' dichiarato, e il colpo deve
	// comunque arrivargli addosso — e' la prova che il ramo a cella regge fino al resolver.
	B.Mine->SelectAbility(B.AreaIdx);
	if (!TestTrue(TEXT("premessa: l'area sulla cella del nemico e' accettata"),
		B.PC->HandleTargetCell(B.Foe->Cell)))
	{
		TearDownPlanBench(B); return false;
	}
	TestNull(TEXT("nessun bersaglio-unita' dichiarato"), (void*)B.Mine->PlannedAttackTarget.Get());

	const int32 HP = B.Foe->Health;
	const int32 Scudo = B.Foe->Shield;
	ResolvePlanTurn(B.TM);
	TestTrue(TEXT("chi sta sulla cella mirata incassa comunque"), DamageTaken(B.Foe, HP, Scudo) > 0);

	TearDownPlanBench(B);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
