// Il CICLO DI VITA a runtime di uno scenario, separato da chi decide di eseguirlo.
//
// I due casi che questo file pinna sono i due che si perdono quando la firma e' un `bool`: «nessuno ha
// chiesto niente» e «qualcuno ha chiesto una cosa che non esiste» hanno effetti opposti sull'allestimento,
// e confonderli significa far partire una partita che nessuno ha chiesto — o non farne partire nessuna.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioCoordinator.h"
#include "Tests/RTWorldFixtures.h"
#include "Tests/RTUnitClassProbeForTest.h"
#include "Unit/RTUnit.h"
#include "EngineUtils.h" // TActorIterator: quale classe l'harness ha posato (#3586)

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `ScenarioId` vuoto: il coordinatore non tocca niente e lo DICHIARA.
 *
 * ⚠️ **Il valore di ritorno e' meta' del test.** Un coordinatore che tornasse `false` sarebbe
 * indistinguibile da uno che ha fallito il caricamento, e il chiamante non saprebbe se allestire la partita
 * normale — che e' la differenza fra «premo Play e gioco» e «premo Play e non succede niente».
 *
 * L'altra meta' e' che `Tick` sia sicuro senza sessione: il GameMode lo chiama a ogni fotogramma, e nel
 * caso normale — nessuno scenario — non c'e' niente da far avanzare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCoordinatorEmptyRequestTest,
	"RefactorTactics.Scenario.CoordinatorEmptyRequestIsIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCoordinatorEmptyRequestTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	FRTScenarioCoordinator Coordinator;
	const ERTScenarioStart Esito = Coordinator.Start(World, FString(), TEXT("test"), 0.f, {});

	TestTrue(TEXT("nessuna richiesta -> il chiamante allestisce la partita normale"),
		Esito == ERTScenarioStart::NotRequested);
	TestFalse(TEXT("e non c'e' nessuna sessione che gira"), Coordinator.IsRunning());
	TestTrue(TEXT("ne' un esito da dichiarare"), Coordinator.OutcomeString().IsEmpty());

	// Il GameMode lo chiama a ogni fotogramma anche in partita normale: senza sessione non deve fare nulla
	// e soprattutto non deve toccare un puntatore che non c'e'.
	Coordinator.Tick(0.05f);
	TestFalse(TEXT("un Tick senza sessione resta senza sessione"), Coordinator.IsRunning());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **Uno scenario chiesto e non caricabile e' FAIL-CLOSED**: non si ripiega sulla partita normale.
 *
 * E' la ragione per cui l'esito ha tre valori. Chi ha scritto un ID sbagliato — un refuso nella tendina, una
 * console variable rimasta da ieri — si ritroverebbe altrimenti a giocare una partita normale credendo di
 * guardare uno scenario, e la sola traccia sarebbe una riga di Output Log che non si ha motivo di aprire.
 * E' lo stesso difetto che `GetScenarioBannerText` esiste per rendere visibile, preso dall'altro lato.
 *
 * ⚠️ **E nessuna sessione resta appesa**: un coordinatore che avesse costruito la sessione prima di
 * accorgersi del caricamento fallito la lascerebbe li', e il `Tick` successivo la farebbe avanzare su uno
 * scenario vuoto — turni fantasma, che e' precisamente il difetto che la sessione a passi ha chiuso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCoordinatorMissingFailsClosedTest,
	"RefactorTactics.Scenario.CoordinatorMissingScenarioFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCoordinatorMissingFailsClosedTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// L'errore e' il comportamento voluto, quindi va dichiarato: un `Error` non atteso fa cadere il test, e
	// tacerlo qui renderebbe il fail-closed indistinguibile da un silenzio.
	AddExpectedError(TEXT("non caricabile"), EAutomationExpectedErrorFlags::Contains, 1);

	FRTScenarioCoordinator Coordinator;
	const ERTScenarioStart Esito = Coordinator.Start(
		World, TEXT("Scenario.CheNonEsiste.RTTest"), TEXT("test"), 0.f, {});

	TestTrue(TEXT("chiesto e non caricabile -> NotLoadable, non NotRequested"),
		Esito == ERTScenarioStart::NotLoadable);
	TestFalse(TEXT("nessuna sessione e' rimasta appesa"), Coordinator.IsRunning());
	TestTrue(TEXT("e nessun esito viene inventato"), Coordinator.OutcomeString().IsEmpty());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * Il caso che rende non vacui gli altri due: uno scenario REALE parte, e il coordinatore lo dichiara.
 *
 * ⚠️ Senza questo, i due test sopra passerebbero anche con un coordinatore che non avvia mai niente — e la
 * distinzione a tre valori sarebbe verificata su due soli rami.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCoordinatorStartsRealScenarioTest,
	"RefactorTactics.Scenario.CoordinatorStartsAShippedScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCoordinatorStartsRealScenarioTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	{
		// Il coordinatore vive in un blocco proprio: la sessione va distrutta PRIMA del mondo, altrimenti il
		// suo distruttore sbinderebbe un decisore su un turn manager gia' andato.
		FRTScenarioCoordinator Coordinator;
		const ERTScenarioStart Esito = Coordinator.Start(World, TEXT("Movement.Basic"), TEXT("test"), 0.f, {});

		TestTrue(TEXT("uno scenario spedito parte"), Esito == ERTScenarioStart::Started);
		TestTrue(TEXT("e la sessione sta girando"), Coordinator.IsRunning());
		TestTrue(TEXT("senza esito finche' non finisce"), Coordinator.OutcomeString().IsEmpty());
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Lo scenario posa la classe che il chiamante gli da', e il cilindro senza** (`#3586`).
 *
 * 🔑 In PIE il GameMode passa al coordinatore le sue `HeroUnitClasses` — i `BP_Unit_*` con la mesh — e uno
 * scenario `Visual.*` mostra le clip sul personaggio. ⛔ L'automation headless non le passa: il cilindro resta
 * il default, ed e' la seconda meta' di questo test. Senza di essa il ripiego potrebbe sparire senza un rosso.
 *
 * ⚠️ La sonda fa le veci della classe con mesh: un `BP_Unit_*` in un worktree non si carica.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioCoordinatorPosesHeroClassTest,
	"RefactorTactics.Scenario.CoordinatorPosesTheHeroClassItIsGiven",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioCoordinatorPosesHeroClassTest::RunTest(const FString&)
{
	// `Movement.Basic`: A1 e' Aevik, B1 e' Branth. Solo Aevik riceve una classe.
	for (const bool bConClassi : { true, false })
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

		{
			// Il coordinatore vive in un blocco proprio: la sessione va distrutta PRIMA del mondo.
			FRTScenarioCoordinator Coordinator;
			TMap<FName, TSubclassOf<ARTUnit>> Classi;
			if (bConClassi)
			{
				Classi.Add(FName(TEXT("Hero.Aevik")), ARTUnitClassProbeForTest::StaticClass());
			}
			const ERTScenarioStart Esito = Coordinator.Start(World, TEXT("Movement.Basic"), TEXT("test"), 0.f, Classi);
			TestTrue(TEXT("premessa: lo scenario parte"), Esito == ERTScenarioStart::Started);
			// ⚠️ `Started` arriva anche quando la sessione fallisce: e' `IsRunning` a dire che e' partita davvero.
			TestTrue(TEXT("premessa: e la sessione sta girando"), Coordinator.IsRunning());

			int32 Aevik = 0;
			int32 Branth = 0;
			for (TActorIterator<ARTUnit> It(World); It; ++It)
			{
				const ARTUnit* U = *It;
				if (U->HeroId == FName(TEXT("Hero.Aevik")))
				{
					++Aevik;
					if (bConClassi)
					{
						TestTrue(TEXT("con le classi, Aevik e' posato con la SUA classe"),
							U->IsA<ARTUnitClassProbeForTest>());
					}
					else
					{
						TestTrue(TEXT("senza classi, Aevik e' il cilindro"), U->GetClass() == ARTUnit::StaticClass());
					}
				}
				else if (U->HeroId == FName(TEXT("Hero.Branth")))
				{
					++Branth;
					TestTrue(TEXT("Branth non ha una classe: resta il cilindro"), U->GetClass() == ARTUnit::StaticClass());
				}
			}
			TestEqual(TEXT("premessa: c'e' un Aevik"), Aevik, 1);
			TestEqual(TEXT("premessa: c'e' un Branth"), Branth, 1);
		}

		RTWorldFixtures::DestroyWorld(World);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
