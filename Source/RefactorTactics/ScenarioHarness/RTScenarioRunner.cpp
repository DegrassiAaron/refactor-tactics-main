#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTScenarioIndex.h"
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
#include "EngineUtils.h" // TActorIterator: verificare che il mondo sia vuoto fra due varianti
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h" // rileggere le tracce per confrontarle senza le voci dell'unita' spostata
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

namespace
{
	/**
	 * Lo stesso scenario con le celle di UNA variante gia' applicate, e senza le varianti: la copia si gioca
	 * una volta sola, come qualunque altro scenario.
	 */
	FRTTestScenario MakeVariantScenario(const FRTTestScenario& Base, const FRTScenarioVariant& Variant)
	{
		FRTTestScenario Out = Base;
		Out.Variants.Reset();
		Out.bExpectSameAcrossVariants = false;
		for (const FRTScenarioVariantUnit& Moved : Variant.Units)
		{
			for (FRTScenarioUnit& Unit : Out.Units)
			{
				if (Unit.Id == Moved.Id)
				{
					Unit.Cell = Moved.Cell;
					break;
				}
			}
		}
		return Out;
	}

	/** Quante unita' ci sono nel mondo: serve a verificare che il teardown fra due varianti sia avvenuto. */
	int32 CountUnitsInWorld(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<ARTUnit> It(World); It; ++It)
		{
			if (IsValid(*It)) { ++Count; }
		}
		return Count;
	}

	/**
	 * Le tracce di una variante senza le voci EMESSE dalle unita' che la variante sposta, ri-serializzate in
	 * forma canonica.
	 *
	 * ⚠️ **Il filtro e' necessario e non e' un'indulgenza.** `BuildMoveLog` scrive una voce per OGNI unita',
	 * anche per chi resta fermo (`Move/Stayed`), e la chiave della voce e' la cella di partenza: l'unita' che
	 * la variante sposta produce quindi una voce diversa **per costruzione**. Senza il filtro il confronto
	 * sarebbe rosso sempre, e per il solo fatto che la variante ha fatto il suo mestiere.
	 *
	 * ⚠️ **Cosa il filtro NON nasconde**, che e' la ragione per cui e' sicuro: si escludono le voci EMESSE
	 * dall'unita' spostata (`SrcCell`), non quelle che la riguardano. Se il bot la bersagliasse, la voce di
	 * combattimento avrebbe come `SrcCell` la cella del BOT e resterebbe nel confronto; se si muovesse
	 * diversamente per causa sua, a differire sarebbe la voce di movimento del bot. I due modi in cui
	 * l'informazione nascosta puo' entrare nell'esito restano entrambi osservabili.
	 *
	 * Si ri-serializza invece di confrontare campo per campo: la forma canonica esiste gia' ed e' ordinata,
	 * mentre un confronto scritto a mano qui divergerebbe dal formato appena qualcuno aggiunge un campo.
	 */
	bool FilterTracesByEmitter(const TArray<FRTTurnTrace>& Traces, const TSet<FRTCellId>& ExcludedSources,
		TArray<TArray<uint8>>& OutFiltered, FString& OutError)
	{
		OutFiltered.Reset();
		for (const FRTTurnTrace& Trace : Traces)
		{
			TArray<FRTTurnLogEntry> Entries;
			if (!URTTurnLogLibrary::DeserializeTurnLog(Trace.Bytes, Entries))
			{
				OutError = TEXT("traccia non rileggibile: il confronto fra varianti non e' producibile");
				return false;
			}
			Entries.RemoveAll([&ExcludedSources](const FRTTurnLogEntry& Entry)
			{
				return ExcludedSources.Contains(Entry.SrcCell);
			});

			// `UnitId` si azzera, ed e' l'unica normalizzazione che il confronto applica.
			//
			// Non e' un'indulgenza: quell'intero e' l'INDICE dell'unita' nello snapshot, e lo snapshot ordina
			// per cella («ordine stabile: UnitId, poi cella»). Spostare l'unita' nascosta da un lato all'altro
			// della mappa la fa scavalcare le altre nell'ordinamento, e gli indici di TUTTE scalano di uno.
			// Misurato sul canary: la sola voce che differiva era `unit=2` contro `unit=1` sulla stessa cella,
			// con lo stesso esito e la stessa azione.
			//
			// L'identita' stabile di una voce e' la cella di partenza — lo dichiara `BuildMoveLog`: «chiave
			// stabile dell'unita' nel turno: la sua cella di PARTENZA, mai un pointer». `SrcCell` resta nel
			// confronto, quindi non si perde nessuna attribuzione: un bot che colpisse un altro bersaglio
			// cambierebbe `TgtCell`, uno che si muovesse altrove cambierebbe `TgtCell` e `Amount`.
			for (FRTTurnLogEntry& Entry : Entries)
			{
				Entry.UnitId = 0;
			}
			OutFiltered.Add(URTTurnLogLibrary::SerializeTurnLog(Entries, ERTLogTopology::Hex));
		}
		return true;
	}

	/** Le celle da cui parte cio' che la variante ha spostato: sono le sorgenti da escludere dal confronto. */
	TSet<FRTCellId> MovedCellsOf(const FRTScenarioVariant& Variant)
	{
		TSet<FRTCellId> Cells;
		for (const FRTScenarioVariantUnit& Moved : Variant.Units)
		{
			Cells.Add(Moved.Cell);
		}
		return Cells;
	}

	/**
	 * Una voce in forma leggibile, con TUTTI i campi che la serializzazione scrive.
	 *
	 * ⚠️ Elencarne un sottoinsieme sembra innocuo e non lo e': la prima stesura ometteva `UnitId`, `Phase` e
	 * `TurnNumber`, e davanti a due tracce che differivano stampava **zero righe** — una diagnostica che
	 * dichiarava identiche due cose che il confronto aveva appena giudicato diverse. Chi aggiunge un campo al
	 * TurnLog deve aggiungerlo anche qui, e il modo per accorgersene e' esattamente questo: un diff vuoto
	 * accanto a un rosso.
	 */
	FString DescribeEntry(const FRTTurnLogEntry& Entry)
	{
		const UEnum* CategoryEnum = StaticEnum<ERTLogCategory>();
		const FString Category = CategoryEnum
			? CategoryEnum->GetNameStringByValue(static_cast<int64>(Entry.Category))
			: FString::FromInt(static_cast<int32>(Entry.Category));

		return FString::Printf(
			TEXT("t%d unit=%d phase=%d %s/%d src=%s tgt=%s amount=%d action=%s base=%s"),
			Entry.TurnNumber, Entry.UnitId, static_cast<int32>(Entry.Phase),
			*Category, Entry.Outcome, *Entry.SrcCell.ToString(), *Entry.TgtCell.ToString(),
			Entry.Amount, *Entry.ActionId.ToString(), *Entry.BaseActionId.ToString());
	}

	/**
	 * Le voci presenti in A e non in B, e viceversa, gia' formattate.
	 *
	 * Un canary che dice soltanto «i due hash non coincidono» costringe chi lo legge a rifare a mano il
	 * confronto che il runner ha appena fatto — e a indovinare, perche' i byte non si leggono. Le righe che
	 * differiscono sono l'unica informazione che rende il rosso azionabile: dicono se a muoversi e' stato il
	 * bot (difetto del gioco) o qualcosa che la variante cambia per costruzione (difetto dello scenario).
	 */
	TArray<FString> DiffEntries(const TArray<uint8>& A, const TArray<uint8>& B)
	{
		TArray<FString> Lines;
		TArray<FRTTurnLogEntry> EntriesA;
		TArray<FRTTurnLogEntry> EntriesB;
		if (!URTTurnLogLibrary::DeserializeTurnLog(A, EntriesA)
			|| !URTTurnLogLibrary::DeserializeTurnLog(B, EntriesB))
		{
			return Lines;
		}

		// Le due liste arrivano in forma CANONICA (la serializzazione ordina), quindi il confronto e'
		// posizionale: e' l'unico che non puo' dichiarare identiche due tracce con lo stesso insieme di voci
		// ma un numero diverso di duplicati — cosa che un confronto per appartenenza lascia passare.
		TArray<FString> TextA;
		TArray<FString> TextB;
		for (const FRTTurnLogEntry& Entry : EntriesA) { TextA.Add(DescribeEntry(Entry)); }
		for (const FRTTurnLogEntry& Entry : EntriesB) { TextB.Add(DescribeEntry(Entry)); }

		if (TextA.Num() != TextB.Num())
		{
			Lines.Add(FString::Printf(TEXT("numero di voci diverso: %d contro %d"), TextA.Num(), TextB.Num()));
		}
		for (int32 I = 0; I < FMath::Max(TextA.Num(), TextB.Num()); ++I)
		{
			const FString A_ = TextA.IsValidIndex(I) ? TextA[I] : TEXT("(assente)");
			const FString B_ = TextB.IsValidIndex(I) ? TextB[I] : TEXT("(assente)");
			if (A_ != B_)
			{
				Lines.Add(FString::Printf(TEXT("voce %d: %s  ||  %s"), I, *A_, *B_));
			}
		}
		return Lines;
	}
}

FRTTestResult URTScenarioRunner::RunSingle(UWorld* World, const FRTTestScenario& Scenario,
	bool bTearDownAfter)
{
	// Ciclo stretto sopra la STESSA sessione che il gioco fa avanzare un passo per frame. Non e' una seconda
	// implementazione: se lo fosse, un test verde non direbbe piu' niente su quel che si vede a schermo.
	FRTScenarioSession Session;
	Session.TurnPauseSeconds = 0.f; // headless non c'e' nessuno a guardare: nessuna pausa da rispettare

	if (!Session.Start(World, Scenario))
	{
		const FRTTestResult Failed = Session.GetResult();
		if (bTearDownAfter) { Session.TearDown(); }
		return Failed;
	}

	// Tetto complessivo: la sessione ha gia' il suo per turno, questo protegge dal caso in cui non avanzi
	// affatto. Un test appeso somiglia a un test lento, e la differenza si scopre solo aspettando.
	const int32 MaxSteps = MaxResolveTicks * (Scenario.Turns.Num() + 2);
	for (int32 I = 0; I < MaxSteps && !Session.IsFinished(); ++I)
	{
		Session.Step(0.05f, /*bPumpTurnManager=*/ true);
	}

	const FRTTestResult Result = Session.GetResult();
	if (bTearDownAfter)
	{
		Session.TearDown();
	}
	return Result;
}

FRTTestResult URTScenarioRunner::Run(UWorld* World, const FRTTestScenario& Scenario)
{
	if (Scenario.Variants.Num() == 0)
	{
		return RunSingle(World, Scenario, /*bTearDownAfter=*/ false);
	}

	FRTTestResult Aggregate;
	Aggregate.ScenarioId = Scenario.ScenarioId;
	Aggregate.Seed = Scenario.Seed;
	Aggregate.Outcome = ERTTestOutcome::Pass;

	TArray<TArray<FRTTurnTrace>> TracesByVariant;
	TracesByVariant.Reserve(Scenario.Variants.Num());

	for (int32 I = 0; I < Scenario.Variants.Num(); ++I)
	{
		const FRTScenarioVariant& Variant = Scenario.Variants[I];

		// Il mondo dev'essere vuoto PRIMA di ogni variante. Non e' una precauzione formale: un residuo qui
		// produrrebbe due partite diverse per una ragione che non e' la variante, e il canary attribuirebbe
		// la differenza — o l'uguaglianza — all'ingresso sbagliato. Meglio un `Error` che lo dice.
		if (const int32 Residue = CountUnitsInWorld(World))
		{
			return MakeErrorResult(Scenario, FString::Printf(
				TEXT("variante '%s': %d unita' sono rimaste nel mondo dalla variante precedente"),
				*Variant.Name, Residue));
		}

		const FRTTestScenario VariantScenario = MakeVariantScenario(Scenario, Variant);
		const FRTTestResult VariantResult = RunSingle(World, VariantScenario, /*bTearDownAfter=*/ true);

		// Le assertion di tutte le varianti finiscono nel report, col nome davanti: senza, un rosso direbbe
		// «UnitHpEquals(BAIT)» senza dire in quale delle due partite.
		for (const FRTAssertionResult& Assertion : VariantResult.Assertions)
		{
			FRTAssertionResult Renamed = Assertion;
			Renamed.Description = FString::Printf(TEXT("[%s] %s"), *Variant.Name, *Assertion.Description);
			Aggregate.Assertions.Add(Renamed);
		}
		for (const FString& Note : VariantResult.Notes)
		{
			Aggregate.Notes.Add(FString::Printf(TEXT("[%s] %s"), *Variant.Name, *Note));
		}
		Aggregate.TurnsPlayed = FMath::Max(Aggregate.TurnsPlayed, VariantResult.TurnsPlayed);

		// Un `Error` e un `Blocked` fermano tutto: le varianti restanti direbbero la stessa cosa, e un
		// confronto fra tracce incomplete non significa niente.
		if (VariantResult.Outcome == ERTTestOutcome::Error)
		{
			Aggregate.Outcome = ERTTestOutcome::Error;
			Aggregate.ErrorMessage = FString::Printf(TEXT("variante '%s': %s"), *Variant.Name, *VariantResult.ErrorMessage);
			return Aggregate;
		}
		if (VariantResult.Outcome == ERTTestOutcome::Blocked)
		{
			Aggregate.Outcome = ERTTestOutcome::Blocked;
			Aggregate.BlockedReason = FString::Printf(TEXT("variante '%s': %s"), *Variant.Name, *VariantResult.BlockedReason);
			return Aggregate;
		}
		if (VariantResult.Outcome == ERTTestOutcome::Fail)
		{
			Aggregate.Outcome = ERTTestOutcome::Fail;
		}

		// L'hash dello stato finale non si aggrega: le varianti hanno per costruzione stati finali diversi, e
		// riportarne uno solo darebbe al report un numero che non descrive niente.
		TracesByVariant.Add(VariantResult.TurnTraces);
	}

	if (Scenario.bExpectSameAcrossVariants)
	{
		auto Digest = [](const TArray<TArray<uint8>>& Traces)
		{
			uint32 Crc = 0;
			for (const TArray<uint8>& Bytes : Traces)
			{
				Crc = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num(), Crc);
			}
			return Crc;
		};

		TArray<TArray<uint8>> First;
		FString FilterError;
		if (!FilterTracesByEmitter(TracesByVariant[0], MovedCellsOf(Scenario.Variants[0]), First, FilterError))
		{
			return MakeErrorResult(Scenario, FilterError);
		}
		const FString FirstName = Scenario.Variants[0].Name;

		for (int32 I = 1; I < TracesByVariant.Num(); ++I)
		{
			TArray<TArray<uint8>> Other;
			if (!FilterTracesByEmitter(TracesByVariant[I], MovedCellsOf(Scenario.Variants[I]), Other, FilterError))
			{
				return MakeErrorResult(Scenario, FilterError);
			}

			bool bSame = (Other.Num() == First.Num());
			for (int32 T = 0; bSame && T < First.Num(); ++T)
			{
				bSame = (Other[T] == First[T]);
			}

			// Il CRC serve solo a rendere leggibile il report: il verdetto e' il confronto byte a byte sopra,
			// perche' un hash che collide farebbe passare due partite diverse.
			FRTAssertionResult Same;
			Same.Description = FString::Printf(TEXT("SameTurnLogAcrossVariants(%s vs %s)"),
				*FirstName, *Scenario.Variants[I].Name);
			Same.bPassed = bSame;
			Same.Expected = FString::Printf(TEXT("%u"), Digest(First));
			Same.Actual = FString::Printf(TEXT("%u"), Digest(Other));
			Same.Turn = Aggregate.TurnsPlayed;
			Aggregate.Assertions.Add(Same);

			if (!bSame)
			{
				Aggregate.Outcome = ERTTestOutcome::Fail;

				// Le righe che differiscono, turno per turno: senza, il rosso e' un numero contro un numero.
				// Anche a log, e non solo nelle Notes: il corpus stampa le assertion, non le note del risultato,
				// e una diagnostica che non raggiunge chi legge il fallimento non e' stata scritta.
				for (int32 T = 0; T < FMath::Min(First.Num(), Other.Num()); ++T)
				{
					for (const FString& Line : DiffEntries(First[T], Other[T]))
					{
						const FString Note = FString::Printf(TEXT("turno %d, %s"), T + 1, *Line);
						Aggregate.Notes.Add(Note);
						UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *Note);
					}
				}
			}
		}
	}

	return Aggregate;
}

FRTTestResult URTScenarioRunner::RunById(UWorld* World, const FString& ScenarioId, FString& OutReportDirectory)
{
	OutReportDirectory.Reset();

	FRTTestScenario Scenario;
	FString Error;
	const FString Path = URTScenarioIndex::ResolvePath(ScenarioId, Error);
	if (Path.IsEmpty() || !URTScenarioLoader::LoadFromFile(Path, Scenario, Error))
	{
		// Scenario assente o malformato: ERROR con il percorso cercato, cosi' chi legge sa DOVE guardare.
		// Con l'indice il percorso puo' essere vuoto — l'ID non e' stato risolto — e in quel caso il motivo
		// e' gia' dentro `Error`, che nomina l'ID e dice quanti scenari sono stati esaminati.
		FRTTestScenario Stub;
		Stub.ScenarioId = ScenarioId;
		FRTTestResult Result = MakeErrorResult(Stub, Path.IsEmpty()
			? Error
			: FString::Printf(TEXT("%s (%s)"), *Error, *Path));

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
	// L'ID non si deduce piu' dal percorso: lo dichiara il file, e l'indice lo legge. Cosi' spostare uno
	// scenario in un'altra cartella non ne cambia l'identita' — ed e' cio' che permette di raggrupparlo
	// per piu' criteri insieme invece che per la sola cartella in cui sta.
	return URTScenarioIndex::ListIds(FString(), FString());
}
