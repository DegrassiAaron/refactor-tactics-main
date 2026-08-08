#include "ScenarioHarness/RTScenarioSession.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

namespace
{
	/**
	 * Arena esagonale piena di raggio N, piu' le celle modificate dallo scenario.
	 *
	 * RIUSA l'actor mappa gia' presente se c'e': lo stesso codice gira in un mondo vuoto (test) e in una PIE
	 * dove il GameMode ha gia' allestito. Spawnarne un secondo darebbe due griglie sovrapposte.
	 */
	URTHexMapAsset* BuildScenarioArena(UWorld* World, int32 Radius, const TArray<FRTScenarioCell>& Overrides)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			Map->AddOrUpdateCell(FRTHexCellData(Id));
		}

		// Le modifiche DOPO l'arena piena: una cella elencata due volte vince l'ultima, e l'esito non dipende
		// dall'ordine di generazione.
		for (const FRTScenarioCell& Spec : Overrides)
		{
			FRTHexCellData Cell(Spec.Cell);
			Cell.bBlocksMovement = Spec.bBlocksMovement;
			Cell.bBlocksLineOfSight = Spec.bBlocksLineOfSight;
			if (Spec.MoveCost > 0)
			{
				Cell.MoveCost = Spec.MoveCost;
			}
			Map->AddOrUpdateCell(Cell);
		}
		Map->SortCells();

		ARTHexMapActor* Actor = ARTHexMapActor::FindInWorld(World);
		if (!Actor)
		{
			Actor = World->SpawnActor<ARTHexMapActor>();
		}
		if (!Actor)
		{
			return nullptr;
		}
		Actor->MapAsset = Map;
		Actor->RebuildInstances(); // la vista ISM segue l'asset: senza, in PIE resterebbe la mappa precedente
		return Map;
	}

	/** L'eroe del catalogo con quell'ID stabile, o nullptr. Il roster e' la fonte: nessun elenco duplicato. */
	URTHeroData* FindScenarioHero(FName HeroId)
	{
		for (URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Hero && Hero->HeroId == HeroId)
			{
				return Hero;
			}
		}
		return nullptr;
	}
}

bool FRTScenarioSession::Start(UWorld* InWorld, const FRTTestScenario& InScenario)
{
	Scenario = InScenario;
	Result = FRTTestResult();
	Result.ScenarioId = Scenario.ScenarioId;
	Result.Seed = Scenario.Seed;

	auto Fail = [this](const FString& Reason) -> bool
	{
		// Tutto cio' che va storto qui e' ERROR, non FAIL: non si e' potuto eseguire, quindi il difetto e' nel
		// test o nell'ambiente, non nel gioco. Confonderli fa cercare una regressione che non esiste.
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = Reason;
		State = EState::Finished;
		return false;
	};

	if (!InWorld)
	{
		return Fail(TEXT("nessun mondo in cui eseguire lo scenario"));
	}
	World = InWorld;

	FString ValidationError;
	if (!URTScenarioLoader::Validate(Scenario, ValidationError))
	{
		return Fail(FString::Printf(TEXT("scenario non valido: %s"), *ValidationError));
	}

	Map = BuildScenarioArena(InWorld, Scenario.MapRadius, Scenario.Cells);
	if (!Map)
	{
		return Fail(TEXT("impossibile creare l'arena esagonale"));
	}

	for (const FRTScenarioUnit& Spec : Scenario.Units)
	{
		URTHeroData* Hero = FindScenarioHero(Spec.HeroId);
		if (!Hero)
		{
			// Validate() lo esclude gia', ma la sessione non si fida di un invariante altrui.
			return Fail(FString::Printf(TEXT("eroe '%s' non nel catalogo"), *Spec.HeroId.ToString()));
		}

		ARTUnit* Unit = InWorld->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit)
		{
			return Fail(FString::Printf(TEXT("spawn fallito per l'unita' '%s'"), *Spec.Id));
		}
		Unit->TeamId = Spec.TeamId;
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		// Le unita' dello scenario NON sono bot: gli intent li decide il file, non l'utility scoring.
		Unit->bIsBotControlled = false;
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(Spec.Cell, FVector::ZeroVector, Map->HexSize, Map->LayerHeight);

		UnitsById.Add(Spec.Id, Unit);
	}

	ARTTurnManager* TM = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(InWorld, ARTTurnManager::StaticClass()));
	if (!TM)
	{
		TM = InWorld->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	}
	if (!TM)
	{
		return Fail(TEXT("impossibile creare il turn manager"));
	}
	TurnManager = TM;

	// Il timer del turn manager va SPENTO: e' la sessione a decidere quando si chiude la pianificazione. Se
	// scadesse per conto suo risolverebbe un turno che la sessione non ha preparato — ed e' esattamente il
	// difetto dei turni fantasma, visto in PIE prima che questa riga esistesse.
	TM->SetPlanningSeconds(0.f);

	if (Scenario.Turns.Num() == 0)
	{
		// Uno scenario senza turni e' legittimo: verifica solo lo stato iniziale.
		Finish();
		return true;
	}

	State = EState::PauseBeforeTurn;
	PauseElapsed = 0.f;
	TurnIndex = 0;
	return true;
}

void FRTScenarioSession::BeginTurn()
{
	ARTTurnManager* TM = TurnManager.Get();
	if (!TM || !Scenario.Turns.IsValidIndex(TurnIndex))
	{
		Finish();
		return;
	}

	// Tutte ferme per default: un'unita' senza intent nel turno NON eredita il piano del turno prima.
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		if (ARTUnit* U = Pair.Value.Get())
		{
			U->PlannedCell = U->Cell;
			U->PlannedPath.Reset();
			U->PlannedWaypoints.Reset();
		}
	}

	for (const FRTScenarioIntent& Intent : Scenario.Turns[TurnIndex].Intents)
	{
		TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Intent.UnitId);
		ARTUnit* Unit = Found ? Found->Get() : nullptr;
		if (!Unit || !Unit->IsAlive() || Intent.Move.Num() == 0)
		{
			continue;
		}

		// Stessa strada del controller: i waypoint diventano un percorso composito calcolato sullo snapshot
		// AUTOREVOLE. Percorso non valido (budget, blocchi, occupanti) -> l'unita' resta ferma e l'assertion
		// lo mostra: e' il comportamento del gioco, non un caso speciale del test.
		TArray<ARTUnit*> SnapshotUnits;
		const FRTHexSnapshot Snapshot = TM->MakeCurrentSnapshot(SnapshotUnits);
		const int32 UnitId = SnapshotUnits.IndexOfByKey(Unit);
		if (UnitId == INDEX_NONE)
		{
			continue;
		}

		const FRTHexPathResult Path = URTHexSimLibrary::BuildCompositeHexPath(Snapshot, UnitId, Intent.Move);
		if (Path.Path.Num() >= 2)
		{
			Unit->PlannedWaypoints = Intent.Move;
			Unit->PlannedPath = Path.Path;
			Unit->PlannedCell = Path.Path.Last();
		}
		else
		{
			UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: percorso rifiutato per '%s' (l'unita' resta ferma)"),
				*Scenario.ScenarioId, *Intent.UnitId);
		}
	}

	TM->LockInAndResolve();
	State = EState::Resolving;
	ResolveTicks = 0;
}

void FRTScenarioSession::Step(float DeltaSeconds, bool bPumpTurnManager)
{
	ARTTurnManager* TM = TurnManager.Get();
	if (State == EState::Finished || !TM)
	{
		return;
	}

	switch (State)
	{
	case EState::PauseBeforeTurn:
	{
		// La partita puo' essersi decisa prima della fine dello scenario: i turni restanti non esistono.
		if (TM->GetPhase() == ERTMatchPhase::MatchEnded)
		{
			Finish();
			return;
		}
		PauseElapsed += DeltaSeconds;
		if (PauseElapsed >= TurnPauseSeconds)
		{
			BeginTurn();
		}
		break;
	}

	case EState::Resolving:
	{
		// In gioco e' il mondo a ticcare il turn manager: pomparlo anche qui lo farebbe correre al doppio
		// della velocita', e il playback che si vuole GUARDARE passerebbe in meta' del tempo.
		if (bPumpTurnManager)
		{
			TM->Tick(DeltaSeconds);
		}

		if (!TM->IsResolving())
		{
			++TurnIndex;
			Result.TurnsPlayed = TurnIndex;
			if (TurnIndex >= Scenario.Turns.Num())
			{
				Finish();
			}
			else
			{
				State = EState::PauseBeforeTurn;
				PauseElapsed = 0.f;
			}
			break;
		}

		// Tetto di sicurezza: una risoluzione che non finisce deve FALLIRE, non girare all'infinito. Senza,
		// un test appeso somiglierebbe a un test lento, e la differenza si scoprirebbe solo aspettando.
		if (++ResolveTicks > URTScenarioRunner::MaxResolveTicks)
		{
			Result.Outcome = ERTTestOutcome::Error;
			Result.ErrorMessage = FString::Printf(
				TEXT("il turno %d non ha finito di risolvere entro %d passi"),
				TurnIndex + 1, URTScenarioRunner::MaxResolveTicks);
			State = EState::Finished;
		}
		break;
	}

	default:
		break;
	}
}

void FRTScenarioSession::Finish()
{
	// Piani AZZERATI: se restassero appesi, qualunque cosa risolvesse un turno dopo lo scenario li
	// ri-eseguirebbe. E' successo davvero, e in PIE sembrava lo scenario stesso.
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		if (ARTUnit* U = Pair.Value.Get())
		{
			U->PlannedCell = U->Cell;
			U->PlannedPath.Reset();
			U->PlannedWaypoints.Reset();
		}
	}

	// Digest dello stato finale (FNV-1a, stesso idioma di `URTTurnLogLibrary::HashTurnLog`). Le unita' si
	// ordinano per ID di scenario: l'ID viene dal file ed e' stabile, l'ordine di `TMap` no.
	{
		TArray<FString> Ids;
		UnitsById.GetKeys(Ids);
		Ids.Sort();

		uint32 Hash = 2166136261u;
		auto Mix = [&Hash](uint32 V) { Hash ^= V; Hash *= 16777619u; };
		for (const FString& Id : Ids)
		{
			const ARTUnit* Unit = UnitsById[Id].Get();
			if (!Unit)
			{
				continue;
			}
			for (const TCHAR Ch : Id)
			{
				Mix(static_cast<uint32>(Ch));
			}
			Mix(static_cast<uint32>(Unit->Cell.X));
			Mix(static_cast<uint32>(Unit->Cell.Y));
			Mix(static_cast<uint32>(Unit->Cell.Layer));
			Mix(static_cast<uint32>(Unit->Health));
			Mix(static_cast<uint32>(Unit->Shield));
			Mix(static_cast<uint32>(Unit->Energy));
			Mix(Unit->IsAlive() ? 1u : 0u);
		}
		Result.StateHash = Hash;
	}

	for (const FRTTestExpectation& Exp : Scenario.Expect)
	{
		FRTAssertionResult A;
		A.Kind = Exp.Kind;
		A.Turn = Result.TurnsPlayed;

		switch (Exp.Kind)
		{
		case ERTAssertionKind::UnitAtCell:
		{
			A.Description = FString::Printf(TEXT("UnitAtCell(%s)"), *Exp.UnitId);
			A.Expected = Exp.Cell.ToString();

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			if (!Unit)
			{
				A.Actual = TEXT("unita' assente");
				A.bPassed = false;
			}
			else
			{
				A.Actual = Unit->Cell.ToString();
				A.bPassed = (Unit->Cell == Exp.Cell);
			}
			break;
		}
		case ERTAssertionKind::TurnsCompleted:
		{
			A.Description = TEXT("TurnsCompleted");
			A.Expected = FString::Printf(TEXT(">= %d"), Exp.Value);
			A.Actual = FString::FromInt(Result.TurnsPlayed);
			A.bPassed = (Result.TurnsPlayed >= Exp.Value);
			break;
		}
		default:
			A.Description = TEXT("assertion non implementata");
			A.bPassed = false;
			break;
		}

		Result.Assertions.Add(A);
	}

	Result.Outcome = (Result.FailedCount() == 0) ? ERTTestOutcome::Pass : ERTTestOutcome::Fail;
	State = EState::Finished;

	UE_LOG(LogRT, Log, TEXT("[RT-Test] %s: %s (%d/%d assertion, %d turni)"),
		*Result.ScenarioId, *Result.OutcomeString(),
		Result.PassedCount(), Result.Assertions.Num(), Result.TurnsPlayed);
}
