#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTScenarioLoader.h"
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
#include "ScenarioHarness/RTTestReportWriter.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
	/** Errore secco: nessuna assertion valutata, motivo esplicito. Un `Error` non deve somigliare a un `Fail`. */
	FRTTestResult MakeErrorResult(const FRTTestScenario& Scenario, const FString& Reason)
	{
		FRTTestResult Result;
		Result.ScenarioId = Scenario.ScenarioId;
		Result.Seed = Scenario.Seed;
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = Reason;
		return Result;
	}

	/**
	 * Arena esagonale piena di raggio N sul layer 0: mappa da codice, nessun `.umap` da versionare.
	 *
	 * RIUSA l'actor mappa gia' presente se c'e'. Serve a far girare lo stesso runner in due contesti diversi:
	 * un mondo vuoto (test di automazione) e una PIE dove il GameMode ha gia' spawnato mappa, luce e turn
	 * manager. Spawnarne un secondo darebbe due griglie sovrapposte e un raycast ambiguo.
	 */
	URTHexMapAsset* BuildArena(UWorld* World, int32 Radius, const TArray<FRTScenarioCell>& Overrides)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			Map->AddOrUpdateCell(FRTHexCellData(Id));
		}

		// Poi le modifiche dello scenario: ostacoli, muri, terreno costoso. Applicate DOPO l'arena piena, cosi'
		// una cella elencata due volte vince l'ultima e non dipende dall'ordine di generazione.
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

	/** L'eroe del catalogo con quell'ID stabile, o nullptr. Il roster e' la fonte: nessun elenco duplicato qui. */
	URTHeroData* FindHero(FName HeroId)
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

	/** Fa avanzare un turno fino in fondo, come `PlayOneTurn` dei test d'integrazione esistenti. */
	void ResolveOneTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < URTScenarioRunner::MaxResolveTicks && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

FRTTestResult URTScenarioRunner::Run(UWorld* World, const FRTTestScenario& Scenario)
{
	// --- 1. precondizioni: tutto cio' che va storto qui e' ERROR, non FAIL --------------------------------
	if (!World)
	{
		return MakeErrorResult(Scenario, TEXT("nessun mondo in cui eseguire lo scenario"));
	}
	FString ValidationError;
	if (!URTScenarioLoader::Validate(Scenario, ValidationError))
	{
		return MakeErrorResult(Scenario, FString::Printf(TEXT("scenario non valido: %s"), *ValidationError));
	}

	FRTTestResult Result;
	Result.ScenarioId = Scenario.ScenarioId;
	Result.Seed = Scenario.Seed;

	// --- 2. mondo: mappa, unita', turn manager ------------------------------------------------------------
	URTHexMapAsset* Map = BuildArena(World, Scenario.MapRadius, Scenario.Cells);
	if (!Map)
	{
		return MakeErrorResult(Scenario, TEXT("impossibile creare l'arena esagonale"));
	}

	TMap<FString, ARTUnit*> UnitsById;
	for (const FRTScenarioUnit& Spec : Scenario.Units)
	{
		URTHeroData* Hero = FindHero(Spec.HeroId);
		if (!Hero)
		{
			// Validate() lo esclude gia', ma il runner non si fida di un invariante altrui.
			return MakeErrorResult(Scenario, FString::Printf(TEXT("eroe '%s' non nel catalogo"), *Spec.HeroId.ToString()));
		}

		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit)
		{
			return MakeErrorResult(Scenario, FString::Printf(TEXT("spawn fallito per l'unita' '%s'"), *Spec.Id));
		}
		Unit->TeamId = Spec.TeamId;
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		// Le unita' dello scenario NON sono bot: gli intent li decide il file, non l'utility scoring. Il bot
		// resta disponibile per gli scenari «agent» futuri, che dichiareranno una policy invece di un intent.
		Unit->bIsBotControlled = false;
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(Spec.Cell, FVector::ZeroVector, Map->HexSize, Map->LayerHeight);

		UnitsById.Add(Spec.Id, Unit);
	}

	// Come per la mappa: si riusa quello del GameMode se la partita e' gia' avviata (PIE).
	ARTTurnManager* TM = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass()));
	if (!TM)
	{
		TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	}
	if (!TM)
	{
		return MakeErrorResult(Scenario, TEXT("impossibile creare il turn manager"));
	}

	// --- 3. turni: si scrivono i piani e si risolve, esattamente come dopo un lock-in del giocatore --------
	const int32 TurnCount = FMath::Min(Scenario.Turns.Num(), MaxTurnsHardCap);
	for (int32 TurnIndex = 0; TurnIndex < TurnCount; ++TurnIndex)
	{
		if (TM->GetPhase() == ERTMatchPhase::MatchEnded)
		{
			break; // la partita si e' decisa prima della fine dello scenario: i turni restanti non esistono
		}

		// Tutte ferme per default: un'unita' senza intent nel turno NON eredita il piano del turno prima.
		for (const TPair<FString, ARTUnit*>& Pair : UnitsById)
		{
			if (ARTUnit* U = Pair.Value)
			{
				U->PlannedCell = U->Cell;
				U->PlannedPath.Reset();
				U->PlannedWaypoints.Reset();
			}
		}

		for (const FRTScenarioIntent& Intent : Scenario.Turns[TurnIndex].Intents)
		{
			ARTUnit** Found = UnitsById.Find(Intent.UnitId);
			ARTUnit* Unit = Found ? *Found : nullptr;
			if (!Unit || !Unit->IsAlive() || Intent.Move.Num() == 0)
			{
				continue;
			}

			// Stessa strada del controller: i waypoint diventano un percorso composito calcolato sullo
			// snapshot AUTOREVOLE. Se il percorso non e' valido (budget, blocchi, occupanti), l'unita' resta
			// ferma e l'assertion lo mostrera' — che e' esattamente il comportamento del gioco.
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
				UE_LOG(LogRT, Warning,
					TEXT("[RT-Test] %s: percorso rifiutato per '%s' (l'unita' resta ferma)"),
					*Scenario.ScenarioId, *Intent.UnitId);
			}
		}

		ResolveOneTurn(TM);
		++Result.TurnsPlayed;
	}

	// --- 4. assertion -------------------------------------------------------------------------------------
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

			ARTUnit** Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? *Found : nullptr;
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

	UE_LOG(LogRT, Log, TEXT("[RT-Test] %s: %s (%d/%d assertion, %d turni)"),
		*Result.ScenarioId, *Result.OutcomeString(),
		Result.PassedCount(), Result.Assertions.Num(), Result.TurnsPlayed);

	return Result;
}

FRTTestResult URTScenarioRunner::RunById(UWorld* World, const FString& ScenarioId, FString& OutReportDirectory)
{
	OutReportDirectory.Reset();

	FRTTestScenario Scenario;
	FString Error;
	const FString Path = URTScenarioLoader::PathForScenarioId(ScenarioId);
	if (!URTScenarioLoader::LoadFromFile(Path, Scenario, Error))
	{
		// Scenario assente o malformato: ERROR con il percorso cercato, cosi' chi legge sa DOVE guardare.
		FRTTestScenario Stub;
		Stub.ScenarioId = ScenarioId;
		FRTTestResult Result = MakeErrorResult(Stub, FString::Printf(TEXT("%s (%s)"), *Error, *Path));

		FString WriteError;
		URTTestReportWriter::Write(Result, FString(), OutReportDirectory, WriteError);
		return Result;
	}

	FRTTestResult Result = Run(World, Scenario);

	FString WriteError;
	if (!URTTestReportWriter::Write(Result, FString(), OutReportDirectory, WriteError))
	{
		// Il report non scritto non cambia l'esito della simulazione, ma va detto: senza report l'harness
		// perde il suo scopo, e un PASS silenzioso nasconderebbe il problema.
		UE_LOG(LogRT, Error, TEXT("[RT-Test] report non scritto: %s"), *WriteError);
	}
	return Result;
}

TArray<FString> URTScenarioRunner::ListScenarioIds()
{
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *URTScenarioLoader::ScenariosRoot(), TEXT("*.json"),
		/*Files=*/ true, /*Directories=*/ false);

	const FString Root = FPaths::ConvertRelativePathToFull(URTScenarioLoader::ScenariosRoot());
	TArray<FString> Ids;
	for (const FString& File : Files)
	{
		// Il percorso E' l'ID: `Movement/Basic.json` -> `Movement.Basic`. Nessun indice da mantenere.
		FString Relative = FPaths::ConvertRelativePathToFull(File);
		FPaths::MakePathRelativeTo(Relative, *(Root / TEXT("")));
		Relative.RemoveFromEnd(TEXT(".json"));
		Ids.Add(Relative.Replace(TEXT("/"), TEXT(".")).Replace(TEXT("\\"), TEXT(".")));
	}
	Ids.Sort();
	return Ids;
}
