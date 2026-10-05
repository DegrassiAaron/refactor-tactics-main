// La posa durante il playback (`#2167`, [D-462] punti 2 e 3): a ogni passo la mesh guarda l'ultimo passo compiuto,
// come la regola `FacingAtMicroStep`; arrivata, si gira sul posto verso il verso finale con un pivot animato.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "RTWorldFixtures.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool StepFacingYawUguale(float A, float B, float Tolleranza = 0.5f)
	{
		return FMath::Abs(FMath::FindDeltaAngleDegrees(A, B)) <= Tolleranza;
	}
}

/**
 * 🔴 **A OGNI CONFINE LO YAW E' L'ULTIMO PASSO, E DENTRO IL PASSO SI GIRA VERSO IL NUOVO** (`#2167`).
 *
 * Un percorso a L: un passo a 0°, poi due a 90°. I punti si scrivono a mano, cosi' gli yaw attesi non escono dalla
 * funzione che si prova.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStepYawFollowsTheLastStepTest,
	"RefactorTactics.Playback.StepYawFollowsTheLastCompletedStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStepYawFollowsTheLastStepTest::RunTest(const FString&)
{
	const TArray<FVector> L = { FVector(0, 0, 0), FVector(100, 0, 0), FVector(100, 100, 0), FVector(100, 200, 0) };
	const float Entrata = 45.f;
	const float Giro = 0.35f;
	auto Yaw = [&](float Alpha) { return URTPlaybackLibrary::StepYawAtAlpha(L, Alpha, Entrata, Giro); };

	TestTrue(TEXT("alla partenza vale lo yaw d'entrata"), StepFacingYawUguale(Yaw(0.f), Entrata));
	TestTrue(TEXT("al primo confine guarda il primo passo (0°)"), StepFacingYawUguale(Yaw(1.f / 3.f), 0.f));
	TestTrue(TEXT("al secondo confine guarda il secondo passo (90°)"), StepFacingYawUguale(Yaw(2.f / 3.f), 90.f));
	TestTrue(TEXT("all'arrivo guarda l'ultimo passo (90°)"), StepFacingYawUguale(Yaw(1.f), 90.f));

	// Dentro il primo segmento: prima del giro sta fra l'entrata e il passo, dopo il giro e' sul passo.
	const float Presto = Yaw(0.1f / 3.f);
	TestTrue(*FString::Printf(TEXT("all'inizio del passo sta girando (%.1f fra 0 e 45)"), Presto),
		Presto > 0.5f && Presto < 44.5f);
	TestTrue(TEXT("a meta' passo il giro e' finito"), StepFacingYawUguale(Yaw(0.5f / 3.f), 0.f));

	// Dopo il primo confine si gira verso il secondo passo, e non prima.
	const float Svolta = Yaw(1.1f / 3.f);
	TestTrue(*FString::Printf(TEXT("subito dopo il confine sta svoltando (%.1f fra 0 e 90)"), Svolta),
		Svolta > 0.5f && Svolta < 89.5f);

	// Un segmento degenere non ha direzione: vale come il passo prima, e non produce uno yaw arbitrario.
	const TArray<FVector> Fermo = { FVector(0, 0, 0), FVector(100, 0, 0), FVector(100, 0, 0) };
	TestTrue(TEXT("un passo fermo tiene lo yaw del passo prima"),
		StepFacingYawUguale(URTPlaybackLibrary::StepYawAtAlpha(Fermo, 1.f, Entrata, Giro), 0.f));
	return true;
}

/** 🔴 **IL PIVOT PRENDE L'ARCO PIU' CORTO** (`#2167`): da 170 a -170 sono venti gradi, passando per 180. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPivotYawShortArcTest,
	"RefactorTactics.Playback.PivotYawTakesTheShortArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPivotYawShortArcTest::RunTest(const FString&)
{
	TestTrue(TEXT("all'inizio e' la partenza"), StepFacingYawUguale(URTPlaybackLibrary::PivotYaw(170.f, -170.f, 0.f), 170.f));
	TestTrue(TEXT("a meta' passa per 180, non per 0"), StepFacingYawUguale(URTPlaybackLibrary::PivotYaw(170.f, -170.f, 0.5f), 180.f));
	TestTrue(TEXT("alla fine e' l'arrivo"), StepFacingYawUguale(URTPlaybackLibrary::PivotYaw(170.f, -170.f, 1.f), -170.f));
	return true;
}

namespace
{
	/** Fa scorrere la sessione finche' il playback si ferma (al confine di uno `Step`) o finisce. */
	int32 StepFacingAvanzaFinoAlConfine(FRTScenarioSession& Session, const ARTTurnManager* TM)
	{
		for (int32 I = 0; I < 400; ++I)
		{
			if (TM->IsPlaybackPaused() || !TM->IsResolving()) { return I; }
			Session.Step(0.05f, /*bPumpTurnManager=*/ true);
		}
		return -1;
	}
}

/**
 * 🔴 **IN PARTITA: LA MESH GUARDA OGNI PASSO AL SUO CONFINE, E ARRIVATA SI GIRA CON UN PIVOT, NON DI SCATTO** (`#2167`).
 *
 * Percorre `Visual.Facing.StepsThenPivots` un micro-step alla volta, come la voce PIE-FACING chiede di guardarlo.
 * Alla fine naturale il pivot e' ancora in corso, quindi la rotazione e' animata; finito il pivot, la mesh guarda il
 * facing LOGICO. Una seconda partita salta il playback, e li' la mesh e' gia' sul facing logico, senza pivot.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMeshFacesEachStepThenPivotsTest,
	"RefactorTactics.Playback.MeshFacesEachStepThenPivotsAfterArrival",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMeshFacesEachStepThenPivotsTest::RunTest(const FString&)
{
	FString Errore;
	const FString Percorso = URTScenarioIndex::ResolvePath(TEXT("Visual.Facing.StepsThenPivots"), Errore);
	FRTTestScenario Scenario;
	if (!TestTrue(TEXT("lo scenario si carica"), !Percorso.IsEmpty() && URTScenarioLoader::LoadFromFile(Percorso, Scenario, Errore)))
	{
		AddError(Errore);
		return false;
	}
	const FRTScenarioUnit* DichA1 = Scenario.Units.FindByPredicate([](const FRTScenarioUnit& U) { return U.Id == TEXT("A1"); });
	const FRTScenarioIntent* Cammino = Scenario.Turns.Num() > 0
		? Scenario.Turns[0].Intents.FindByPredicate([](const FRTScenarioIntent& I) { return I.UnitId == TEXT("A1"); })
		: nullptr;
	if (!TestTrue(TEXT("il file dichiara A1 e il suo cammino"), DichA1 && Cammino && Cammino->Move.Num() >= 3))
	{
		return false;
	}
	TArray<FRTCellId> Pose;
	Pose.Add(DichA1->Cell);
	Pose.Append(Cammino->Move);

	for (const bool bSalta : { false, true })
	{
		const TCHAR* Giro = bSalta ? TEXT("salto") : TEXT("a passi");
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo"), World)) { return false; }
		FRTScenarioSession Session;
		Session.TurnPauseSeconds = 0.f;
		if (!TestTrue(*FString::Printf(TEXT("%s: la sessione parte"), Giro), Session.Start(World, Scenario)))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}

		[&]()
		{
			ARTTurnManager* TM = nullptr;
			for (TActorIterator<ARTTurnManager> It(World); It; ++It) { TM = *It; break; }
			ARTUnit* A1 = RTWorldFixtures::FirstUnitOfTeam(World, DichA1->TeamId);
			const ARTHexMapActor* Mappa = ARTHexMapActor::FindInWorld(World);
			if (!TestTrue(*FString::Printf(TEXT("%s: turn manager, A1 e mappa"), Giro), TM && A1 && Mappa))
			{
				return;
			}
			FVector Origin; float HexSize; float LayerH;
			Mappa->GetHexContext(Origin, HexSize, LayerH);
			auto YawFra = [&](const FRTCellId& Da, const FRTCellId& A)
			{
				return URTPlaybackLibrary::DirectionYaw(A1->WorldForCell(Da, Origin, HexSize, LayerH),
					A1->WorldForCell(A, Origin, HexSize, LayerH));
			};

			TM->SetPlaybackControlsEnabled(true);
			TM->SetStartPlaybackPaused(true);
			for (int32 I = 0; I < 400 && !(TM->IsResolving() && TM->IsPlaybackPaused()) && !Session.IsFinished(); ++I)
			{
				Session.Step(0.05f, /*bPumpTurnManager=*/ true);
			}
			if (!TestTrue(*FString::Printf(TEXT("%s: il playback parte fermo"), Giro), TM->IsResolving() && TM->IsPlaybackPaused()))
			{
				return;
			}

			if (bSalta)
			{
				TM->SkipPlayback();
				TestFalse(TEXT("salto: nessun pivot in corso"), TM->IsPresentationPivotRunning());
				TestTrue(TEXT("salto: la mesh e' gia' sul facing logico"),
					StepFacingYawUguale(A1->GetActorRotation().Yaw, YawFra(A1->Cell, URTHexLibrary::Neighbor(A1->Cell, A1->Facing))));
				return;
			}

			// I confini intermedi: l'ultimo chiude la fase, e a fine playback la posa la decide il pivot.
			for (int32 K = 1; K < Pose.Num() - 1; ++K)
			{
				TM->StepMicroStep();
				if (StepFacingAvanzaFinoAlConfine(Session, TM) < 0 || !TM->IsResolving())
				{
					AddError(FString::Printf(TEXT("passo %d: il playback non si ferma al confine"), K));
					return;
				}
				const float Atteso = YawFra(Pose[K - 1], Pose[K]);
				TestTrue(*FString::Printf(TEXT("confine %d: la mesh guarda l'ultimo passo (%.1f, atteso %.1f)"), K,
					A1->GetActorRotation().Yaw, Atteso), StepFacingYawUguale(A1->GetActorRotation().Yaw, Atteso));
			}

			// La premessa del pivot: il verso finale non e' l'ultimo passo, altrimenti non ci sarebbe niente da girare.
			const float UltimoPasso = YawFra(Pose[Pose.Num() - 2], Pose.Last());
			TM->ResumePlayback();
			for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
			{
				Session.Step(0.05f, /*bPumpTurnManager=*/ true);
			}
			const float Finale = YawFra(A1->Cell, URTHexLibrary::Neighbor(A1->Cell, A1->Facing));
			if (!TestFalse(TEXT("premessa: il verso finale non e' l'ultimo passo"), StepFacingYawUguale(Finale, UltimoPasso)))
			{
				return;
			}
			TestTrue(TEXT("alla fine naturale il pivot e' ancora in corso: animato, non uno scatto"),
				TM->IsPresentationPivotRunning());
			for (int32 I = 0; I < 40 && TM->IsPresentationPivotRunning(); ++I)
			{
				TM->Tick(0.05f);
			}
			TestFalse(TEXT("il pivot finisce"), TM->IsPresentationPivotRunning());
			TestTrue(*FString::Printf(TEXT("e la mesh guarda il facing logico (%.1f, atteso %.1f)"),
				A1->GetActorRotation().Yaw, Finale), StepFacingYawUguale(A1->GetActorRotation().Yaw, Finale));
		}();

		Session.TearDown();
		RTWorldFixtures::DestroyWorld(World);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
