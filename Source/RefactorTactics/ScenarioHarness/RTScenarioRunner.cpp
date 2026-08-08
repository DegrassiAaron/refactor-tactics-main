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
#include "Turn/RTMatchSetupLibrary.h"
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
	/**
	 * Applica gli override dello scenario a una mappa gia' costruita, la ordina e la installa nell'actor.
	 *
	 * Estratto da `BuildArena` quando gli scenari hanno imparato a RIFERIRE una fixture: le due strade
	 * differiscono solo per come nasce la mappa, e tutto cio' che viene dopo — override, ordinamento,
	 * wiring dell'actor — deve restare identico, o una fixture si comporterebbe diversamente da un'arena
	 * generata per ragioni che non hanno niente a che vedere con la sua geometria.
	 */
	URTHexMapAsset* InstallArena(UWorld* World, URTHexMapAsset* Map, const TArray<FRTScenarioCell>& Overrides)
	{
		if (!Map)
		{
			return nullptr;
		}

		// Le modifiche dello scenario: ostacoli, muri, terreno costoso. Applicate DOPO l'arena piena, cosi'
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

	/** Arena esagonale piena di raggio N, poi installata come tutte le altre. */
	URTHexMapAsset* BuildArena(UWorld* World, int32 Radius, const TArray<FRTScenarioCell>& Overrides)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			Map->AddOrUpdateCell(FRTHexCellData(Id));
		}
		return InstallArena(World, Map, Overrides);
	}

	/**
	 * Le capability che il gioco possiede **oggi**. Un turno che ne chiede una assente non viene giocato, e
	 * lo scenario si dichiara `Blocked` invece di fallire.
	 *
	 * L'elenco e' qui e non nei dati perche' e' una proprieta' del **codice**: se stesse nello scenario,
	 * dichiarare disponibile una capability inesistente sarebbe una modifica al JSON, e il primo scenario
	 * verde e bugiardo arriverebbe da li'.
	 */
	bool IsCapabilityAvailable(const FString& Capability)
	{
		static const TSet<FString> Available = {
			TEXT("FixtureReference"),  // S2-1: lo scenario riferisce la geometria per nome
			TEXT("Reaction"),          // E5: reazioni componibili, automatiche (AllowedResponses <= 1)
			TEXT("Environment"),       // E8: superfici, stati, propagazione
			TEXT("Cover"),             // E9 CP 9.1/9.2: coperture bassa e alta, distruzione
			TEXT("Structures"),        // E9 CP 9.3: porte come bordo, revisione della mappa
		};
		return Available.Contains(Capability);
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
	// Una fixture RIFERITA per nome batte l'arena generata: la geometria canonica vive in un posto solo.
	URTHexMapAsset* Map = nullptr;
	if (!Scenario.Fixture.IsEmpty())
	{
		Map = URTMatchSetupLibrary::MakeFixtureArena(World, Scenario.Fixture);
		if (!Map)
		{
			// Il nome sbagliato si dice, non si aggira: un'arena vuota darebbe un fallimento che parla di
			// unita' fuori mappa invece che della fixture inesistente.
			return MakeErrorResult(Scenario,
				FString::Printf(TEXT("fixture di mappa sconosciuta: '%s'"), *Scenario.Fixture));
		}
		// Gli override di cella restano validi: si applicano SOPRA la fixture, non al posto suo.
		Map = InstallArena(World, Map, Scenario.Cells);
	}
	else
	{
		Map = BuildArena(World, Scenario.MapRadius, Scenario.Cells);
	}
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

		// Con una fixture riferita per nome la forma non e' un raggio, quindi `Validate()` non puo' piu'
		// controllare che la cella esista: qui si', perche' qui la mappa vera c'e'. Senza questo, un'unita'
		// fuori mappa produrrebbe un FAIL su assertion che parlano di posizioni, invece di dire cos'e'
		// successo davvero.
		if (!Map->ContainsCell(Spec.Cell))
		{
			return MakeErrorResult(Scenario, FString::Printf(
				TEXT("unita' '%s': la cella %s non esiste nella mappa"), *Spec.Id, *Spec.Cell.ToString()));
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
	FString BlockedBy;
	for (int32 TurnIndex = 0; TurnIndex < TurnCount; ++TurnIndex)
	{
		if (TM->GetPhase() == ERTMatchPhase::MatchEnded)
		{
			break; // la partita si e' decisa prima della fine dello scenario: i turni restanti non esistono
		}

		// Il turno chiede qualcosa che il gioco non sa ancora fare? Ci si ferma QUI, dichiarando cosa manca.
		// Non si gioca "quel che si puo'" del turno: un turno a meta' produrrebbe uno stato che non
		// corrisponde ne' al gioco di oggi ne' a quello di domani, e ogni assertion successiva mentirebbe.
		for (const FString& Required : Scenario.Turns[TurnIndex].Requires)
		{
			if (!IsCapabilityAvailable(Required))
			{
				BlockedBy = FString::Printf(TEXT("turno %d: manca la capability '%s'"), TurnIndex + 1, *Required);
				break;
			}
		}
		if (!BlockedBy.IsEmpty())
		{
			break;
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

	// Digest dello stato finale, prima delle assertion: e' cio' che il gate di determinismo confronta fra
	// una ripetizione e l'altra, e vale anche quando qualche assertion fallisce (due FAIL identici devono
	// avere lo stesso hash, altrimenti non si potrebbe dire se una regressione e' la stessa di ieri).
	Result.StateHash = HashFinalState(UnitsById);

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

	// Precedenza: un FAIL vero batte il BLOCKED. Un'assertion caduta PRIMA del punto di blocco riguarda
	// codice che esiste ed e' rotto — nasconderla dietro «non e' ancora pronto» sarebbe il modo piu' comodo
	// di perdere una regressione.
	if (Result.FailedCount() > 0)
	{
		Result.Outcome = ERTTestOutcome::Fail;
	}
	else if (!BlockedBy.IsEmpty())
	{
		Result.Outcome = ERTTestOutcome::Blocked;
		Result.BlockedReason = BlockedBy;
	}
	else
	{
		Result.Outcome = ERTTestOutcome::Pass;
	}

	UE_LOG(LogRT, Log, TEXT("[RT-Test] %s: %s (%d/%d assertion, %d turni)%s"),
		*Result.ScenarioId, *Result.OutcomeString(),
		Result.PassedCount(), Result.Assertions.Num(), Result.TurnsPlayed,
		BlockedBy.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" — %s"), *BlockedBy));

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
