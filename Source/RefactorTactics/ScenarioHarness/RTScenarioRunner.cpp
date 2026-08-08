#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioSession.h"
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

	/**
	 * Digest dello stato finale (FNV-1a, stesso idioma di `URTTurnLogLibrary::HashTurnLog`).
	 *
	 * Le unita' si ordinano per **ID di scenario** prima di mescolare: l'ID viene dal file ed e' stabile,
	 * mentre l'ordine di `TMap` non lo e'. Senza l'ordinamento l'hash dipenderebbe dall'iterazione di un
	 * container non ordinato — cioe' esattamente cio' che l'invariante #4 vieta, dentro lo strumento che
	 * dovrebbe verificarlo.
	 */
	uint32 HashFinalState(const TMap<FString, ARTUnit*>& UnitsById)
	{
		TArray<FString> Ids;
		UnitsById.GetKeys(Ids);
		Ids.Sort();

		uint32 Hash = 2166136261u; // FNV-1a offset basis (32 bit)
		auto Mix = [&Hash](uint32 V)
		{
			Hash ^= V;
			Hash *= 16777619u; // FNV-1a prime (32 bit)
		};

		for (const FString& Id : Ids)
		{
			const ARTUnit* Unit = UnitsById[Id];
			if (!Unit)
			{
				continue;
			}
			// L'identita' entra nell'hash: due unita' che si scambiano di posto devono dare un hash DIVERSO
			// da quello in cui sono rimaste ferme, altrimenti uno swap passerebbe per «niente e' successo».
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
		return Hash;
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
	// Ciclo stretto sopra la STESSA sessione che il gioco fa avanzare un passo per frame. Non e' una seconda
	// implementazione: se lo fosse, un test verde non direbbe piu' niente su quel che si vede a schermo.
	FRTScenarioSession Session;
	Session.TurnPauseSeconds = 0.f; // headless non c'e' nessuno a guardare: nessuna pausa da rispettare

	if (!Session.Start(World, Scenario))
	{
		return Session.GetResult();
	}

	// Tetto complessivo: la sessione ha gia' il suo per turno, questo protegge dal caso in cui non avanzi
	// affatto. Un test appeso somiglia a un test lento, e la differenza si scopre solo aspettando.
	const int32 MaxSteps = MaxResolveTicks * (Scenario.Turns.Num() + 2);
	for (int32 I = 0; I < MaxSteps && !Session.IsFinished(); ++I)
	{
		Session.Step(0.05f, /*bPumpTurnManager=*/ true);
	}

	return Session.GetResult();
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
