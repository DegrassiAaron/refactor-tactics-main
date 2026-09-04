#include "ScenarioHarness/RTScenarioRunner.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioSession.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
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
	// ⛔ **SENZA CHIAMANTI, misurato il 2026-09-05.** `git grep HashFinalState -- Source/` da' questa
	// definizione e una citazione in un commento di test: nient'altro. Lo `StateHash` che gli scenari
	// pubblicano viene da `URTMatchStateHashLibrary::HashMatchState` via `RTScenarioSession.cpp`, e
	// `RTScenarioRunner` lo propaga da `RunResult.StateHash`. ⚠️ `D-284` e `D-324` contano questo fra i **due
	// siti di `Mix`** da tenere allineati: uno dei due e' questa funzione morta, e la nota va letta sapendolo.
	// La rimozione non e' fatta qui — sarebbe una pulizia che le due voci non prevedono.
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

	/**
	 * Riversa il risultato di UNA esecuzione nell'aggregato: assertion e note col loro `[etichetta]` davanti,
	 * il massimo dei turni, e le quattro grandezze delle decisioni di finestra (`#512`).
	 *
	 * 🔴 **Esiste perche' la seconda copia aveva gia' perso un campo.** Questo blocco viveva solo dentro il
	 * ciclo delle varianti; `RunRepeated` lo ha ricopiato e ha omesso `LastScriptedResponse`, quindi uno
	 * scenario con `repeatCount` che scripta una decisione riportava `lastScriptedResponse: ""` accanto a un
	 * `scriptedDecisionsApplied` diverso da zero — due campi dello stesso referto che si contraddicono. Il
	 * commento della versione originale lo diceva gia': senza queste righe il `result.json` «direbbe il falso
	 * proprio nel file per cui i campi sono stati aggiunti». Da qui in poi c'e' un posto solo da aggiornare
	 * quando nasce un campo nuovo.
	 */
	void AppendRunInto(FRTTestResult& Aggregate, const FRTTestResult& RunResult, const FString& Label)
	{
		for (const FRTAssertionResult& Assertion : RunResult.Assertions)
		{
			FRTAssertionResult Renamed = Assertion;
			Renamed.Description = FString::Printf(TEXT("[%s] %s"), *Label, *Assertion.Description);
			Aggregate.Assertions.Add(Renamed);
		}
		for (const FString& Note : RunResult.Notes)
		{
			Aggregate.Notes.Add(FString::Printf(TEXT("[%s] %s"), *Label, *Note));
		}
		Aggregate.TurnsPlayed = FMath::Max(Aggregate.TurnsPlayed, RunResult.TurnsPlayed);

		// Le durate si SOMMANO dove i turni si massimizzano, ed e' voluto: `TurnsPlayed` risponde a «quanto e'
		// andata avanti la partita piu' lunga», le durate a «quanto e' costato tutto questo». Un massimo qui
		// direbbe che due varianti costano quanto la piu' lenta, che e' falso di ogni run ripetuta.
		Aggregate.SimulationSeconds += RunResult.SimulationSeconds;
		Aggregate.WallClockSeconds += RunResult.WallClockSeconds;

		Aggregate.ScriptedDecisionsApplied += RunResult.ScriptedDecisionsApplied;
		Aggregate.ScriptedDecisionsUnused += RunResult.ScriptedDecisionsUnused;
		if (RunResult.DecisionSource != TEXT("none"))
		{
			Aggregate.DecisionSource = RunResult.DecisionSource;
		}
		if (!RunResult.LastScriptedResponse.IsEmpty())
		{
			Aggregate.LastScriptedResponse = RunResult.LastScriptedResponse;
		}
	}

	/**
	 * `repeatCount` (CP 47.4): N esecuzioni **identiche** dello stesso scenario, confrontate fra loro.
	 *
	 * E' il veicolo del corpus di determinismo (E47.5) e la domanda che pone e' l'opposto di quella delle
	 * varianti: li' si cambia un ingresso per vedere se l'esito si muove, qui non si cambia niente per vedere
	 * se sta fermo. Il confronto e' su **due** grandezze e non su una:
	 *
	 *   · il **TurnLog** serializzato, turno per turno — cio' che e' SUCCESSO, byte per byte;
	 *   · lo **StateHash** finale — cio' che ne e' RIMASTO.
	 *
	 * ⚠️ Nessuna delle due basta da sola, ed e' misurato: un TurnLog identico non prova che lo stato finale lo
	 * sia (il log non registra tutto cio' che un digest copre), e lo StateHash da solo *«sarebbe rimasto verde
	 * su #990»* — dove la divergenza compariva al turno 2 e lo stato finale tornava lo stesso.
	 */
	FRTTestResult RunRepeated(UWorld* World, const FRTTestScenario& Scenario)
	{
		// Ogni giro e' una esecuzione SINGOLA: senza questo azzeramento `Run` rientrerebbe qui per sempre.
		FRTTestScenario Once = Scenario;
		Once.RepeatCount = 1;

		FRTTestResult Aggregate;
		Aggregate.ScenarioId = Scenario.ScenarioId;
		Aggregate.Seed = Scenario.Seed;
		Aggregate.Outcome = ERTTestOutcome::Pass;

		TArray<TArray<FRTTurnTrace>> TracesByRun;
		TArray<uint32> HashByRun;
		TracesByRun.Reserve(Scenario.RepeatCount);
		HashByRun.Reserve(Scenario.RepeatCount);

		for (int32 I = 0; I < Scenario.RepeatCount; ++I)
		{
			// Stessa precauzione delle varianti, e per la stessa ragione: un residuo qui produrrebbe due
			// partite diverse per un motivo che non e' la ripetizione, e il confronto attribuirebbe la
			// differenza — o l'uguaglianza — alla cosa sbagliata.
			if (const int32 Residue = CountUnitsInWorld(World))
			{
				return MakeErrorResult(Scenario, FString::Printf(
					TEXT("ripetizione %d: %d unita' sono rimaste nel mondo dall'esecuzione precedente"),
					I + 1, Residue));
			}

			const FRTTestResult RunResult = URTScenarioRunner::RunSingle(World, Once, /*bTearDownAfter=*/ true);

			AppendRunInto(Aggregate, RunResult, FString::Printf(TEXT("run %d"), I + 1));

			if (RunResult.Outcome == ERTTestOutcome::Error)
			{
				Aggregate.Outcome = ERTTestOutcome::Error;
				Aggregate.ErrorMessage = FString::Printf(TEXT("ripetizione %d: %s"), I + 1, *RunResult.ErrorMessage);
				return Aggregate;
			}
			if (RunResult.Outcome == ERTTestOutcome::Blocked)
			{
				Aggregate.Outcome = ERTTestOutcome::Blocked;
				Aggregate.BlockedReason = FString::Printf(TEXT("ripetizione %d: %s"), I + 1, *RunResult.BlockedReason);
				return Aggregate;
			}
			if (RunResult.Outcome == ERTTestOutcome::Fail)
			{
				Aggregate.Outcome = ERTTestOutcome::Fail;
			}

			TracesByRun.Add(RunResult.TurnTraces);
			HashByRun.Add(RunResult.StateHash);
		}

		// Le tracce e l'hash della PRIMA esecuzione finiscono nel report: a differenza delle varianti, qui
		// sono le stesse di tutte le altre — o il confronto qui sotto e' rosso, e il report dice quale voce
		// differisce.
		Aggregate.TurnTraces = TracesByRun[0];
		Aggregate.StateHash = HashByRun[0];

		for (int32 I = 1; I < TracesByRun.Num(); ++I)
		{
			bool bSameLog = (TracesByRun[I].Num() == TracesByRun[0].Num());
			for (int32 T = 0; bSameLog && T < TracesByRun[0].Num(); ++T)
			{
				bSameLog = (TracesByRun[I][T].Bytes == TracesByRun[0][T].Bytes);
			}

			FRTAssertionResult SameLog;
			SameLog.Description = FString::Printf(TEXT("SameTurnLogAcrossRuns(1 vs %d)"), I + 1);
			SameLog.Expected = FString::Printf(TEXT("%d turni identici byte per byte"), TracesByRun[0].Num());
			SameLog.Actual = bSameLog
				? FString::Printf(TEXT("%d turni identici"), TracesByRun[I].Num())
				: FString::Printf(TEXT("%d turni, almeno uno diverso"), TracesByRun[I].Num());
			SameLog.bPassed = bSameLog;
			SameLog.Turn = Aggregate.TurnsPlayed;
			Aggregate.Assertions.Add(SameLog);

			if (!bSameLog)
			{
				Aggregate.Outcome = ERTTestOutcome::Fail;
				// Le righe che differiscono, turno per turno: senza, il rosso e' un numero contro un numero.
				for (int32 T = 0; T < FMath::Min(TracesByRun[0].Num(), TracesByRun[I].Num()); ++T)
				{
					for (const FString& Line : DiffEntries(TracesByRun[0][T].Bytes, TracesByRun[I][T].Bytes))
					{
						const FString Note = FString::Printf(TEXT("run %d, turno %d, %s"), I + 1, T + 1, *Line);
						Aggregate.Notes.Add(Note);
						UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *Note);
					}
				}
			}

			FRTAssertionResult SameHash;
			SameHash.Description = FString::Printf(TEXT("SameStateHashAcrossRuns(1 vs %d)"), I + 1);
			SameHash.Expected = FString::Printf(TEXT("%u"), HashByRun[0]);
			SameHash.Actual = FString::Printf(TEXT("%u"), HashByRun[I]);
			SameHash.bPassed = (HashByRun[I] == HashByRun[0]);
			SameHash.Turn = Aggregate.TurnsPlayed;
			Aggregate.Assertions.Add(SameHash);
			if (!SameHash.bPassed)
			{
				Aggregate.Outcome = ERTTestOutcome::Fail;
			}
		}

		return Aggregate;
	}
}

FRTTestResult URTScenarioRunner::RunSingle(UWorld* World, const FRTTestScenario& Scenario,
	bool bTearDownAfter, const FRTWorkbenchVariant& Variant)
{
	// Ciclo stretto sopra la STESSA sessione che il gioco fa avanzare un passo per frame. Non e' una seconda
	// implementazione: se lo fosse, un test verde non direbbe piu' niente su quel che si vede a schermo.
	FRTScenarioSession Session;
	Session.TurnPauseSeconds = 0.f; // headless non c'e' nessuno a guardare: nessuna pausa da rispettare
	// La variante prima di `Start`, che e' dove la sessione la applica: passarla dopo sarebbe un'ora tardi.
	Session.WorkbenchVariant = Variant;

	// Il tempo di parete parte da QUI e non dal primo step: `Start` allestisce mondo, unita' e mappa, e su
	// uno scenario grande e' una fetta reale del costo. Escluderlo darebbe una durata che non corrisponde a
	// nulla che si possa aspettare guardando la run.
	const double WallClockStart = FPlatformTime::Seconds();

	if (!Session.Start(World, Scenario))
	{
		FRTTestResult Failed = Session.GetResult();
		Failed.WallClockSeconds = static_cast<float>(FPlatformTime::Seconds() - WallClockStart);
		if (bTearDownAfter) { Session.TearDown(); }
		return Failed;
	}

	// Tetto complessivo: la sessione ha gia' il suo per turno, questo protegge dal caso in cui non avanzi
	// affatto. Un test appeso somiglia a un test lento, e la differenza si scopre solo aspettando.
	//
	// In free-run i turni non sono enumerati e `Turns.Num()` e' zero: senza `MaxTurns` questo tetto varrebbe
	// due turni, e la partita verrebbe troncata dal RUNNER prima che la sessione possa dire se e' finita —
	// un `Fail` su «ancora in corso» che non parlerebbe del gioco ma di questa riga.
	const int32 PlannedTurns = Scenario.bFreeRun ? Scenario.MaxTurns : Scenario.Turns.Num();
	const int32 MaxSteps = MaxResolveTicks * (PlannedTurns + 2);

	// Il passo del ciclo in una costante e non ripetuto due volte: e' il fattore che converte gli step in
	// `SimulationSeconds`, e due letterali `0.05f` che devono restare uguali sono due letterali che prima o
	// poi divergono — a quel punto la durata riportata sarebbe di una simulazione che non e' avvenuta.
	constexpr float StepSeconds = 0.05f;
	int32 StepsTaken = 0;
	for (int32 I = 0; I < MaxSteps && !Session.IsFinished(); ++I)
	{
		Session.Step(StepSeconds, /*bPumpTurnManager=*/ true);
		++StepsTaken;
	}

	FRTTestResult Result = Session.GetResult();
	// Le due durate si riempiono QUI e non dentro la sessione: la sessione avanza un passo per volta e non sa
	// quanti gliene chiederanno: e' il runner a possedere il ciclo, quindi e' il runner a poterlo contare.
	Result.SimulationSeconds = StepsTaken * StepSeconds;
	Result.WallClockSeconds = static_cast<float>(FPlatformTime::Seconds() - WallClockStart);
	if (bTearDownAfter)
	{
		Session.TearDown();
	}
	return Result;
}

FRTTestResult URTScenarioRunner::Run(UWorld* World, const FRTTestScenario& Scenario)
{
	// `repeatCount` viene PRIMA delle varianti e non ci si combina: il loader rifiuta i due insieme, e questa
	// riga e' il lato eseguibile di quel rifiuto. Le due domande sono opposte — le varianti cambiano un
	// ingresso per vedere se l'esito si muove, le ripetizioni non cambiano niente per vedere se sta fermo — e
	// un ciclo annidato produrrebbe tracce che differiscono per costruzione: il confronto non risponderebbe a
	// nessuna delle due.
	if (Scenario.RepeatCount > 1)
	{
		return RunRepeated(World, Scenario);
	}

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
		// «UnitHpEquals(BAIT)» senza dire in quale delle due partite. Le quattro grandezze delle decisioni di
		// finestra (#512) si sommano nello stesso passaggio — vedi `AppendRunInto`, che e' l'unico posto in cui
		// vive questa aggregazione da quando la sua seconda copia ha perso un campo.
		AppendRunInto(Aggregate, VariantResult, Variant.Name);

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
