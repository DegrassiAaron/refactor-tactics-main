// Il CABLAGGIO fra conduttore e GameMode — la metà che le porte finte non possono vedere.
//
// 🔴 **Questo file esiste per un difetto che i gate della conduzione non prendevano.** Quelli usano
// porte finte, e il difetto viveva nella porta vera: `PrimaryActorTick.bStartWithTickEnabled = false` e
// l'unico `SetActorTickEnabled(true)` stava nel ramo `Started` del `BeginPlay`. Una seduta si apre da
// console **dopo** il Play, quando quel ramo e' gia' passato senza scenario — quindi la sessione partiva
// e non avanzava di un frame, e nessun verdetto veniva mai chiesto. Trovato in code review il
// 2026-09-20, dopo che la suite era verde.

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "PieSession/RTPieSessionSubsystem.h"
#include "RTGameMode.h"
#include "EngineUtils.h"                  // TActorIterator: quale classe la porta ha posato (#3586)
#include "Tests/RTUnitClassProbeForTest.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mondo con una `UGameInstance` vera: senza, `GetSubsystem` risponde sempre null e il test non prova niente. */
	UWorld* PieIntegrationMakeWorld(UGameInstance*& OutGameInstance)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (!World || !GEngine)
		{
			return nullptr;
		}

		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(World);

		// ⚠️ `NewObject<UGameInstance>` da solo NON crea la collezione dei subsystem: `GetSubsystem`
		// risponderebbe null e il test proverebbe di non poter chiedere, invece della precedenza.
		// `InitializeStandalone` e' la via che li inizializza — misurato il 2026-09-20, il primo
		// tentativo senza falliva su «il conduttore esiste sulla GameInstance».
		OutGameInstance = NewObject<UGameInstance>(GEngine);
		OutGameInstance->InitializeStandalone();

		Ctx.OwningGameInstance = OutGameInstance;
		World->SetGameInstance(OutGameInstance);
		return World;
	}

	void PieIntegrationDestroyWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionLaunchEnablesTickTest,
	"RefactorTactics.PieSession.LaunchingAStepEnablesTheTickThatAdvancesIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionLaunchEnablesTickTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	UWorld* World = PieIntegrationMakeWorld(GI);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	URTPieSessionSubsystem* Conduttore = GI ? GI->GetSubsystem<URTPieSessionSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("il conduttore esiste sulla GameInstance"), Conduttore))
	{
		PieIntegrationDestroyWorld(World);
		return false;
	}

	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("game mode"), GameMode))
	{
		PieIntegrationDestroyWorld(World);
		return false;
	}

	// Lo stato di partenza e' quello di una partita normale gia' avviata: tick SPENTO.
	GameMode->SetActorTickEnabled(false);
	TestFalse(TEXT("si parte col tick spento, come dopo un Play senza scenario"),
		GameMode->IsActorTickEnabled());

	// Le porte vere, quelle che il `BeginPlay` installa.
	GameMode->InstallPieSessionPorts();
	if (!TestTrue(TEXT("le porte sono installate"), Conduttore->HasPorts()))
	{
		PieIntegrationDestroyWorld(World);
		return false;
	}

	// ⚠️ Cio' che si misura e' che la porta accenda il tick, **non** che la scena funzioni: lo scenario
	// puo' avviarsi o morire all'avvio, e in entrambi i casi il tick dev'essere acceso — se non lo fosse,
	// una sessione viva non avanzerebbe di un frame.
	//
	// ⛔ Qui NON si dichiarano errori attesi: con `Occurrences = 0` Automation pretende che l'errore
	// AVVENGA, e un'attesa non soddisfatta fallisce da sola. Misurato il 2026-09-20: lo scenario si
	// avvia pulito, quindi dichiarare un errore atteso rendeva rosso un test che passava.

	Conduttore->Begin({ [] {
		FRTPieSessionStep S;
		S.PieItem = TEXT("PIE-VIS-SIGHTWALL");
		S.ScenarioId = TEXT("Visual.Map.SightWallIsWalkable");
		return S;
	}() });

	TestTrue(TEXT("dopo il lancio il tick e' acceso: senza, la sessione non avanza di un frame"),
		GameMode->IsActorTickEnabled());

	PieIntegrationDestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionBeatsTheConsoleCVarTest,
	"RefactorTactics.PieSession.AConductingSessionBeatsTheConsoleCVarOnTheRealPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionBeatsTheConsoleCVarTest::RunTest(const FString&)
{
	// La quarta sorgente sul percorso VERO, non sulla funzione pura: qui si verificano anche le tre
	// righe dentro `ResolveScenarioToRun`, che il gate della funzione pura non attraversa.
	UGameInstance* GI = nullptr;
	UWorld* World = PieIntegrationMakeWorld(GI);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	URTPieSessionSubsystem* Conduttore = GI ? GI->GetSubsystem<URTPieSessionSubsystem>() : nullptr;
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!Conduttore || !GameMode)
	{
		AddError(TEXT("allestimento fallito: conduttore o game mode assenti"));
		PieIntegrationDestroyWorld(World);
		return false;
	}

	GameMode->ScenarioToRun = TEXT("Movement.Basic");
	TestEqual(TEXT("senza seduta vale la proprieta'"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Basic")));


	FRTPieSessionPorts Porte;
	Porte.Launch = [](const FString&) { return FRTPieLaunchOutcome::Avviato(); };
	Porte.TearDown = []() {};
	Conduttore->Begin({ [] {
		FRTPieSessionStep S;
		S.PieItem = TEXT("PIE-A");
		S.ScenarioId = TEXT("Scen.DellaSeduta");
		return S;
	}() }, Porte);

	TestEqual(TEXT("con una seduta in corso vince il suo passo"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Scen.DellaSeduta")));

	Conduttore->Abort();
	TestEqual(TEXT("e finita la seduta la proprieta' torna a valere"),
		GameMode->ResolveScenarioToRun(), FString(TEXT("Movement.Basic")));

	PieIntegrationDestroyWorld(World);
	return true;
}

/**
 * **La porta del conduttore posa le classi CORRENTI del GameMode** (`#3586`).
 *
 * 🔑 E' il collegamento che rende vere le mesh in PIE: il GameMode passa le proprie `HeroUnitClasses` al
 * coordinatore a ogni lancio. Il test di `FRTScenarioCoordinator` prova l'inoltro; questo prova che il GameMode
 * lo FA — togliendo l'argomento dalla lambda della porta, Aevik torna cilindro e la riga cade.
 *
 * ⚠️ La mappa si cambia DOPO lo spawn del GameMode e PRIMA del lancio: e' cio' che distingue «letta a ogni
 * lancio» da «copiata una volta». La sonda fa le veci del `BP_Unit_*`, che in un worktree non si carica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionPortPosesHeroClassesTest,
	"RefactorTactics.PieSession.ThePortPosesTheGameModeHeroClasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionPortPosesHeroClassesTest::RunTest(const FString&)
{
	UGameInstance* GI = nullptr;
	UWorld* World = PieIntegrationMakeWorld(GI);
	if (!TestNotNull(TEXT("mondo"), World)) { return false; }

	URTPieSessionSubsystem* Conduttore = GI ? GI->GetSubsystem<URTPieSessionSubsystem>() : nullptr;
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("il conduttore esiste sulla GameInstance"), Conduttore)
		|| !TestNotNull(TEXT("game mode"), GameMode))
	{
		PieIntegrationDestroyWorld(World);
		return false;
	}

	// Solo Aevik ha una classe; Branth, assente dalla mappa, deve restare il cilindro.
	GameMode->HeroUnitClasses.Empty();
	GameMode->HeroUnitClasses.Add(FName(TEXT("Hero.Aevik")), ARTUnitClassProbeForTest::StaticClass());

	GameMode->InstallPieSessionPorts();
	Conduttore->Begin({ [] {
		FRTPieSessionStep S;
		S.PieItem = TEXT("PIE-VIS-SIGHTWALL");
		S.ScenarioId = TEXT("Visual.Map.SightWallIsWalkable"); // A1 Aevik, B1 Branth
		return S;
	}() });

	int32 Aevik = 0;
	int32 Branth = 0;
	for (TActorIterator<ARTUnit> It(World); It; ++It)
	{
		const ARTUnit* U = *It;
		if (U->HeroId == FName(TEXT("Hero.Aevik")))
		{
			++Aevik;
			TestTrue(TEXT("Aevik e' posato con la classe che il GameMode ha ADESSO"), U->IsA<ARTUnitClassProbeForTest>());
		}
		else if (U->HeroId == FName(TEXT("Hero.Branth")))
		{
			++Branth;
			TestTrue(TEXT("Branth, senza classe, resta il cilindro"), U->GetClass() == ARTUnit::StaticClass());
		}
	}
	TestEqual(TEXT("premessa: la porta ha posato Aevik"), Aevik, 1);
	TestEqual(TEXT("premessa: la porta ha posato Branth"), Branth, 1);

	PieIntegrationDestroyWorld(World);
	return true;
}

#endif
