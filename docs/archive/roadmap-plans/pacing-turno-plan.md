# Sonda di pacing del turno — Implementation Plan

> ## 📦 `DELIVERED PLAN` — PIANO GIA' ESEGUITO, NON NORMATIVO
>
> Il piano di esecuzione, gia' eseguito. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Misurare quanto tempo reale occupa un turno — decisione del giocatore e risoluzione — in un canale separato dal TurnLog, così che `PlanningSeconds` possa essere tarato su un dato invece che su un'ipotesi.

**Architecture:** Uno `struct` puro (`FRTPacingSample`) descrive un turno misurato; una libreria statica pura (`URTPacingLibrary`) calcola percentili, conteggi e righe CSV; `ARTTurnManager` è l'unico punto impuro — cronometra con `FPlatformTime::Seconds()`, accumula e appende il file. `ARTPlayerController` non cronometra: notifica solo il tipo di input. Nessun dato di pacing entra nel TurnLog né nel suo hash.

**Tech Stack:** Unreal Engine 5.8.1, C++, Unreal Automation Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`).

**Spec:** [`spec-pacing-turno.md`](../../gameplay/spec-pacing-turno.md).

## Global Constraints

- Prefissi `RT`/`URT`/`FRT`/`ERT`; PascalCase; commenti e messaggi in **italiano**, identificatori in inglese.
- **Nessun `float` nel campione**: tutto intero, in millisecondi (spec D4). I `float` esistono solo come sorgente (`FPlatformTime::Seconds()`, `PlaybackElapsedTotal`) e si convertono con `FMath::RoundToInt` al momento della cattura.
- **La sonda non ha ritorno verso il gameplay** (spec §4): nessun campo di pacing entra in `FRTTurnLogEntry`, in `HashTurnLog` o in una decisione. Task 5 lo verifica.
- **Additività**: i test esistenti restano verdi **senza modifiche**. Nessun file di test esistente viene toccato.
- **Nessuna modifica a `.Build.cs`**: UBT include automaticamente i nuovi `.h`/`.cpp` sotto `Source/RefactorTactics/`.
- Engine: `D:\EpicGames\UE_5.8` · uproject: `D:\Repositories\refactor-tactics-main\RefactorTactics.uproject`.
- **Build (entrambi i target, sempre)**:
  - `"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTactics Win64 Development -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex`
  - `"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex`
- **Test (a editor chiuso)**: `"D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" "-ExecCmds=Automation RunTests RefactorTactics.Pacing; Quit" -nullrhi -unattended -nopause -nosplash -log`
  Con l'editor aperto il target Editor non si linka (Live Coding tiene i binari): in quel caso compila il target `RefactorTactics` e rimanda l'esecuzione dei test a editor chiuso.
- Branch: `docs/pacing-turno` (o un `feat/` derivato). Nessun push senza richiesta esplicita.

## Struttura dei file

| File | Responsabilità |
|---|---|
| `Source/RefactorTactics/Turn/RTPacing.h` *(nuovo)* | Solo tipi: `ERTLockInSource`, `ERTPlanningInput`, `FRTPacingSample`, `FRTPacingSummary`. Nessuna funzione |
| `Source/RefactorTactics/Turn/RTPacingLibrary.h/.cpp` *(nuovi)* | `URTPacingLibrary`: percentile, sommario, header e riga CSV. Tutto `static`, tutto puro |
| `Source/RefactorTactics/Turn/RTPacingConsole.cpp` *(nuovo)* | Il solo comando `rt.Debug.Pacing`. Isolato perché è l'unico pezzo che dipende da `IConsoleManager` |
| `Source/RefactorTactics/Turn/RTTurnManager.h/.cpp` *(modifica)* | Sei agganci, accumulo, scrittura del file |
| `Source/RefactorTactics/Player/RTPlayerController.cpp` *(modifica)* | Quattro chiamate a `RecordPlanningInput` |
| `Source/RefactorTactics/Tests/RTPacingTests.cpp` *(nuovo)* | Test della libreria pura |
| `Source/RefactorTactics/Tests/RTPacingIntegrationTests.cpp` *(nuovo)* | Test con `UWorld`, con helper propri come fanno già `RTHexBotIntegrationTests.cpp` e `RTHexCombatIntegrationTests.cpp` |
| `docs/design/v0.1-definition-of-done.md` *(modifica)* | Riga KPI predisposta |
| `docs/design/test-manuali-pie.md` *(modifica)* | Voce ⏳ per la verifica in PIE della Task 4 |

---

### Task 1: Tipi e sommario puro

**Files:**
- Create: `Source/RefactorTactics/Turn/RTPacing.h`
- Create: `Source/RefactorTactics/Turn/RTPacingLibrary.h`
- Create: `Source/RefactorTactics/Turn/RTPacingLibrary.cpp`
- Create: `Source/RefactorTactics/Tests/RTPacingTests.cpp`
- Modify: `docs/design/spec-pacing-turno.md` (§5: rinomina di tre campi, vedi Step 7)

**Interfaces:**
- Produces: `enum class ERTLockInSource : uint8 { Input, Timeout }`; `enum class ERTPlanningInput : uint8 { Click, Selection, Order, Undo }`; `struct FRTPacingSample`; `struct FRTPacingSummary`; `URTPacingLibrary::PercentileNearestRank(const TArray<int32>& SortedValues, int32 Percentile) -> int32`; `URTPacingLibrary::SummarizeSamples(const TArray<FRTPacingSample>& Samples, int32 CutoffWindowMs) -> FRTPacingSummary`.

- [ ] **Step 1: Creare `Source/RefactorTactics/Turn/RTPacing.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "RTPacing.generated.h"

/** Chi ha chiuso la pianificazione: il giocatore o il timer. */
UENUM(BlueprintType)
enum class ERTLockInSource : uint8
{
	Input,      // il giocatore ha premuto il lock-in
	Timeout     // l'ha chiusa lo scadere del timer
};

/** Tipo di input di pianificazione, per distinguere "sto pensando" da "sto litigando con l'interfaccia". */
UENUM(BlueprintType)
enum class ERTPlanningInput : uint8
{
	Click,      // attivita' generica: aggiorna solo i tempi, non incrementa nulla
	Selection,  // il giocatore ha selezionato un'unita'
	Order,      // il giocatore ha impartito un ordine (abilita' o destinazione)
	Undo        // waypoint annullato
};

/**
 * Un turno misurato. TELEMETRIA: non entra nel TurnLog, non entra nel suo hash, non influenza nessuna
 * decisione di gioco (spec-pacing-turno.md §4). E' l'unica ragione per cui puo' permettersi di non essere
 * deterministico.
 *
 * Tutti interi in millisecondi: un `float` in CSV con locale italiano stampa la virgola decimale, che
 * collide col separatore, e renderebbe i test a tolleranza invece che esatti.
 */
USTRUCT(BlueprintType)
struct FRTPacingSample
{
	GENERATED_BODY()

	/** Numero del turno misurato. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 TurnNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 UnitsAliveTeam0 = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 UnitsAliveTeam1 = 0;

	/** Azioni utilizzabili dalle unita' vive della squadra misurata (cooldown/energia escludono). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 ActionsAvailable = 0;

	/** Millisecondi dall'inizio della pianificazione al primo input; = MsToLockIn se non c'e' stato input. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsToFirstInput = 0;

	/** Quante volte il giocatore ha selezionato un'unita' in questo turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 SelectionCount = 0;

	/** Quanti ordini (abilita' o destinazione) ha impartito in questo turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 OrderCount = 0;

	/** Quanti waypoint ha annullato in questo turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 UndoCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsToLockIn = 0;

	/**
	 * Millisecondi dall'ultimo input al lock-in; = MsToLockIn se non c'e' stato NESSUN input.
	 * E' il campo che distingue un timer che TAGLIA (valore basso) da un timer che scade A VUOTO (alto):
	 * due patologie con cure opposte, indistinguibili contando solo i timeout.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsSinceLastInput = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	ERTLockInSource LockInSource = ERTLockInSource::Input;

	/** Durata effettiva del playback della risoluzione (0 se non c'e' stato playback). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MsPlayback = 0;

	/** Vero se il giocatore ha saltato il playback. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	bool bPlaybackSkipped = false;
};

/** Sommario di una sessione di campioni. Prodotto da URTPacingLibrary::SummarizeSamples. */
USTRUCT(BlueprintType)
struct FRTPacingSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 SampleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MedianMsToLockIn = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 P90MsToLockIn = 0;

	/** Timeout con input recente: il timer ha tagliato una decisione in corso. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 TrueCutoffs = 0;

	/** Timeout senza input recente: il timer e' scaduto a vuoto. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 IdleTimeouts = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 SkippedPlaybacks = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Pacing")
	int32 MedianMsPlayback = 0;
};
```

- [ ] **Step 2: Creare `Source/RefactorTactics/Turn/RTPacingLibrary.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTPacing.h"
#include "RTPacingLibrary.generated.h"

/** Calcoli puri sulla telemetria di pacing (nessun Actor, nessun file, testabili headless). */
UCLASS()
class REFACTORTACTICS_API URTPacingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Percentile con metodo NEAREST-RANK su un array GIA' ORDINATO in modo crescente: nessuna
	 * interpolazione, quindi il risultato e' sempre un valore realmente osservato e il test si scrive a mano.
	 * Rango 1-based = ceil(Percentile/100 * N). Array vuoto -> 0.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static int32 PercentileNearestRank(const TArray<int32>& SortedValues, int32 Percentile);

	/**
	 * Sommario dei campioni. `CutoffWindowMs` e' la soglia che separa un TAGLIO (timeout con input piu'
	 * recente della soglia) da un'ATTESA A VUOTO: e' un parametro esplicito e non una costante sepolta,
	 * perche' e' una decisione di design ritarabile. Array vuoto -> sommario tutto a zero (fail-closed).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FRTPacingSummary SummarizeSamples(const TArray<FRTPacingSample>& Samples, int32 CutoffWindowMs);

	/** Intestazione del CSV: tredici colonne, nello stesso ordine di CsvRow. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FString CsvHeader();

	/** Una riga CSV: tutti interi con %d, quindi nessuna virgola decimale introdotta dal locale. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Pacing")
	static FString CsvRow(const FRTPacingSample& Sample);
};
```

- [ ] **Step 3: Scrivere i test che falliscono — `Source/RefactorTactics/Tests/RTPacingTests.cpp`**

```cpp
#include "Misc/AutomationTest.h"
#include "Turn/RTPacing.h"
#include "Turn/RTPacingLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Campione minimo: solo i campi che il test in questione guarda. */
	FRTPacingSample MakeSample(int32 MsToLockIn, ERTLockInSource Source, int32 MsSinceLastInput,
		int32 MsPlayback = 0, bool bSkipped = false)
	{
		FRTPacingSample S;
		S.MsToLockIn = MsToLockIn;
		S.LockInSource = Source;
		S.MsSinceLastInput = MsSinceLastInput;
		S.MsPlayback = MsPlayback;
		S.bPlaybackSkipped = bSkipped;
		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingPercentileTest,
	"RefactorTactics.Pacing.PercentileNearestRank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingPercentileTest::RunTest(const FString&)
{
	// Sette valori: il nearest-rank restituisce sempre un valore OSSERVATO, mai una media.
	// p50 -> rango ceil(0.50*7) = 4 -> quarto valore = 400. p90 -> ceil(0.90*7) = 7 -> settimo = 700.
	const TArray<int32> Sorted = { 100, 200, 300, 400, 500, 600, 700 };
	TestEqual(TEXT("p50 = quarto valore"), URTPacingLibrary::PercentileNearestRank(Sorted, 50), 400);
	TestEqual(TEXT("p90 = settimo valore"), URTPacingLibrary::PercentileNearestRank(Sorted, 90), 700);
	TestEqual(TEXT("p100 = ultimo valore"), URTPacingLibrary::PercentileNearestRank(Sorted, 100), 700);

	// Un solo campione: ogni percentile e' quel campione.
	const TArray<int32> One = { 42 };
	TestEqual(TEXT("p90 di un solo valore"), URTPacingLibrary::PercentileNearestRank(One, 90), 42);

	// Array vuoto: 0, nessun accesso fuori range.
	const TArray<int32> Empty;
	TestEqual(TEXT("array vuoto -> 0"), URTPacingLibrary::PercentileNearestRank(Empty, 50), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingEmptySummaryTest,
	"RefactorTactics.Pacing.SummaryOfEmptySampleIsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingEmptySummaryTest::RunTest(const FString&)
{
	// Fail-closed: nessun campione -> tutto zero, nessuna divisione per zero, nessun indice fuori range.
	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(TArray<FRTPacingSample>(), 3000);
	TestEqual(TEXT("nessun campione"), S.SampleCount, 0);
	TestEqual(TEXT("mediana 0"), S.MedianMsToLockIn, 0);
	TestEqual(TEXT("p90 0"), S.P90MsToLockIn, 0);
	TestEqual(TEXT("nessun taglio"), S.TrueCutoffs, 0);
	TestEqual(TEXT("nessuna attesa"), S.IdleTimeouts, 0);
	TestEqual(TEXT("nessun playback saltato"), S.SkippedPlaybacks, 0);
	TestEqual(TEXT("mediana playback 0"), S.MedianMsPlayback, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingCutoffTest,
	"RefactorTactics.Pacing.CutoffVsIdleTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingCutoffTest::RunTest(const FString&)
{
	TArray<FRTPacingSample> Samples;
	// Timeout con input a 500 ms dalla fine: il timer ha TAGLIATO una decisione in corso.
	Samples.Add(MakeSample(30000, ERTLockInSource::Timeout, 500));
	// Timeout con ultimo input 12 s prima: il giocatore aveva finito, il timer e' scaduto A VUOTO.
	Samples.Add(MakeSample(30000, ERTLockInSource::Timeout, 12000));
	// Confine ESATTO: uguale alla soglia conta come attesa, non come taglio.
	Samples.Add(MakeSample(30000, ERTLockInSource::Timeout, 3000));
	// Lock-in manuale: non e' ne' l'uno ne' l'altro.
	Samples.Add(MakeSample(8000, ERTLockInSource::Input, 200, 4000, /*bSkipped=*/ true));

	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(Samples, /*CutoffWindowMs=*/ 3000);
	TestEqual(TEXT("quattro campioni"), S.SampleCount, 4);
	TestEqual(TEXT("un solo taglio vero"), S.TrueCutoffs, 1);
	TestEqual(TEXT("due attese a vuoto (12 s e il confine esatto)"), S.IdleTimeouts, 2);
	TestEqual(TEXT("un playback saltato"), S.SkippedPlaybacks, 1);
	// Ordinati: 8000, 30000, 30000, 30000 -> p50 = rango ceil(0.50*4) = 2 -> 30000.
	TestEqual(TEXT("mediana del lock-in"), S.MedianMsToLockIn, 30000);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 4: Compilare per vedere il fallimento**

Run: `"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex`

Expected: **errore di link** su `URTPacingLibrary::PercentileNearestRank` e `SummarizeSamples` — dichiarate nell'header, non ancora definite. È il fallimento atteso: senza implementazione i test non possono nemmeno linkare.

- [ ] **Step 5: Implementare `Source/RefactorTactics/Turn/RTPacingLibrary.cpp`**

```cpp
#include "Turn/RTPacingLibrary.h"

int32 URTPacingLibrary::PercentileNearestRank(const TArray<int32>& SortedValues, int32 Percentile)
{
	if (SortedValues.Num() == 0)
	{
		return 0;
	}
	const int32 P = FMath::Clamp(Percentile, 1, 100);
	// Rango 1-based = ceil(P/100 * N), su interi per non passare mai da un float.
	const int32 Rank = FMath::DivideAndRoundUp(P * SortedValues.Num(), 100);
	return SortedValues[FMath::Clamp(Rank - 1, 0, SortedValues.Num() - 1)];
}

FRTPacingSummary URTPacingLibrary::SummarizeSamples(const TArray<FRTPacingSample>& Samples, int32 CutoffWindowMs)
{
	FRTPacingSummary Out;
	Out.SampleCount = Samples.Num();
	if (Samples.Num() == 0)
	{
		return Out; // fail-closed: nessun campione -> tutto zero
	}

	TArray<int32> LockIn;
	TArray<int32> Playback;
	LockIn.Reserve(Samples.Num());
	Playback.Reserve(Samples.Num());

	for (const FRTPacingSample& S : Samples)
	{
		LockIn.Add(S.MsToLockIn);
		Playback.Add(S.MsPlayback);

		if (S.LockInSource == ERTLockInSource::Timeout)
		{
			// Sotto soglia = il giocatore stava ancora agendo -> taglio. Dalla soglia in su -> attesa a vuoto.
			(S.MsSinceLastInput < CutoffWindowMs ? Out.TrueCutoffs : Out.IdleTimeouts)++;
		}
		if (S.bPlaybackSkipped)
		{
			++Out.SkippedPlaybacks;
		}
	}

	LockIn.Sort();
	Playback.Sort();
	Out.MedianMsToLockIn = PercentileNearestRank(LockIn, 50);
	Out.P90MsToLockIn = PercentileNearestRank(LockIn, 90);
	Out.MedianMsPlayback = PercentileNearestRank(Playback, 50);
	return Out;
}

FString URTPacingLibrary::CsvHeader()
{
	return TEXT("Turn,AliveT0,AliveT1,ActionsAvailable,MsToFirstInput,SelectionCount,OrderCount,")
		   TEXT("UndoCount,MsToLockIn,MsSinceLastInput,LockInSource,MsPlayback,PlaybackSkipped");
}

FString URTPacingLibrary::CsvRow(const FRTPacingSample& Sample)
{
	// Tutti %d: nessun float, quindi nessuna virgola decimale da locale che spezzi le colonne.
	return FString::Printf(TEXT("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"),
		Sample.TurnNumber,
		Sample.UnitsAliveTeam0,
		Sample.UnitsAliveTeam1,
		Sample.ActionsAvailable,
		Sample.MsToFirstInput,
		Sample.SelectionCount,
		Sample.OrderCount,
		Sample.UndoCount,
		Sample.MsToLockIn,
		Sample.MsSinceLastInput,
		static_cast<int32>(Sample.LockInSource),
		Sample.MsPlayback,
		Sample.bPlaybackSkipped ? 1 : 0);
}
```

- [ ] **Step 6: Compilare ed eseguire i test**

Run (build, entrambi i target):
```
"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTactics Win64 Development -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex
"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex
```
Run (test, a editor chiuso):
```
"D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" "-ExecCmds=Automation RunTests RefactorTactics.Pacing; Quit" -nullrhi -unattended -nopause -nosplash -log
```
Expected: 3 test eseguiti, 3 passati.

- [ ] **Step 7: Allineare la spec ai nomi reali dei campi**

In `docs/design/spec-pacing-turno.md` §5, la tabella delle definizioni dice `SelectionChanges` e `OrderChanges` con semantica «la prima non conta». Contare «il primo ordine **per unità** non conta» richiede tracciamento per unità e non aggiunge informazione: due unità e otto ordini è churn tanto quanto sei cambi. Sostituire le due righe con:

```markdown
| `SelectionCount` | Quante volte il giocatore ha selezionato un'unità nel turno |
| `OrderCount` | Quanti ordini (abilità o destinazione) ha impartito nel turno |
```

e aggiornare di conseguenza il blocco di §5 e la riga `SelectionChanges`/`OrderChanges` dell'elenco dei campi.

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Turn/RTPacing.h Source/RefactorTactics/Turn/RTPacingLibrary.h Source/RefactorTactics/Turn/RTPacingLibrary.cpp Source/RefactorTactics/Tests/RTPacingTests.cpp docs/design/spec-pacing-turno.md
git commit -m "feat(pacing): il sommario di una sessione, calcolato invece che stimato"
```

---

### Task 2: La riga CSV regge il locale

**Files:**
- Modify: `Source/RefactorTactics/Tests/RTPacingTests.cpp`

**Interfaces:**
- Consumes: `URTPacingLibrary::CsvHeader()`, `URTPacingLibrary::CsvRow(const FRTPacingSample&)` (Task 1).
- Produces: nulla di nuovo.

- [ ] **Step 1: Scrivere il test che falliscono**

Aggiungere in `RTPacingTests.cpp`, **prima** di `#endif // WITH_DEV_AUTOMATION_TESTS`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingCsvTest,
	"RefactorTactics.Pacing.CsvRowMatchesHeader",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingCsvTest::RunTest(const FString&)
{
	FRTPacingSample S;
	S.TurnNumber = 7;
	S.UnitsAliveTeam0 = 2;
	S.UnitsAliveTeam1 = 1;
	S.ActionsAvailable = 6;
	S.MsToFirstInput = 1200;
	S.SelectionCount = 3;
	S.OrderCount = 2;
	S.UndoCount = 1;
	S.MsToLockIn = 18400;
	S.MsSinceLastInput = 900;
	S.LockInSource = ERTLockInSource::Input;
	S.MsPlayback = 5300;
	S.bPlaybackSkipped = false;

	TArray<FString> HeaderCols;
	TArray<FString> RowCols;
	URTPacingLibrary::CsvHeader().ParseIntoArray(HeaderCols, TEXT(","), /*InCullEmpty=*/ false);
	URTPacingLibrary::CsvRow(S).ParseIntoArray(RowCols, TEXT(","), /*InCullEmpty=*/ false);

	TestEqual(TEXT("tredici colonne nell'intestazione"), HeaderCols.Num(), 13);
	TestEqual(TEXT("la riga ha le stesse colonne dell'intestazione"), RowCols.Num(), HeaderCols.Num());

	// Ogni colonna e' un intero: se un float si intrufolasse, con locale italiano stamperebbe una virgola
	// e spezzerebbe la riga in 14 colonne. Il controllo qui sopra lo prende; questo dice PERCHE'.
	for (const FString& Col : RowCols)
	{
		TestTrue(FString::Printf(TEXT("colonna intera: %s"), *Col), Col.IsNumeric() && !Col.Contains(TEXT(".")));
	}

	// I valori finiscono nelle colonne giuste, nell'ordine dichiarato.
	TestEqual(TEXT("prima colonna = turno"), RowCols[0], TEXT("7"));
	TestEqual(TEXT("nona colonna = MsToLockIn"), RowCols[8], TEXT("18400"));
	TestEqual(TEXT("undicesima colonna = LockInSource Input = 0"), RowCols[10], TEXT("0"));
	TestEqual(TEXT("ultima colonna = playback non saltato"), RowCols[12], TEXT("0"));
	return true;
}
```

- [ ] **Step 2: Compilare ed eseguire — deve passare subito**

Run: build (entrambi i target) + `-ExecCmds=Automation RunTests RefactorTactics.Pacing; Quit`
Expected: 4 test, 4 passati.

Se il conteggio delle colonne fallisce, l'errore è in `CsvHeader()`/`CsvRow()` di Task 1: le due liste sono disallineate. Correggere lì, non nel test.

- [ ] **Step 3: Commit**

```bash
git add Source/RefactorTactics/Tests/RTPacingTests.cpp
git commit -m "test(pacing): la riga CSV non si spezza col locale italiano"
```

---

### Task 3: Gli agganci nel TurnManager

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (righe 281, 299, 315, 385, 394, 1211)
- Create: `Source/RefactorTactics/Tests/RTPacingIntegrationTests.cpp`

**Interfaces:**
- Consumes: `FRTPacingSample`, `ERTPlanningInput`, `ERTLockInSource` (Task 1).
- Produces: `ARTTurnManager::RecordPlanningInput(ERTPlanningInput Kind)`; `ARTTurnManager::GetPacingSamples() const -> const TArray<FRTPacingSample>&`; `ARTTurnManager::bRecordPacing` (bool, `EditAnywhere`); `ARTTurnManager::PacingTeamId` (int32, `EditAnywhere`, default 0).

- [ ] **Step 1: Scrivere i test che falliscono — `Source/RefactorTactics/Tests/RTPacingIntegrationTests.cpp`**

```cpp
#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPacing.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Telemetria di pacing con un mondo vero. Helper locali come in RTHexBotIntegrationTests.cpp e
 * RTHexCombatIntegrationTests.cpp: ogni file d'integrazione tiene i propri, sono in namespace anonimo.
 */
namespace
{
	UWorld* MakeHexPacingWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyHexPacingWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnHexPacingMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
	}

	ARTUnit* SpawnHexPacingUnit(UWorld* World, int32 TeamId, ERTArchetype Arch, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureAsArchetype(Arch);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = true;
		U->DispatchBeginPlay(); // senza, i cooldown restano vuoti e ogni abilita' risulta sempre pronta
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * TurnManager pronto a giocare. DispatchBeginPlay e' NECESSARIO: e' BeginPlay a chiamare
	 * StartPlanningTimer, che apre il campione di pacing del PRIMO turno. Senza, i campioni sarebbero
	 * sistematicamente uno in meno dei turni giocati.
	 */
	ARTTurnManager* SpawnHexPacingTurnManager(UWorld* World)
	{
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (TM) { TM->DispatchBeginPlay(); }
		return TM;
	}

	/**
	 * Nome con suffisso `Pacing`: UE compila piu' .cpp in un'unica unita' di traduzione (unity build),
	 * quindi due helper omonimi in namespace anonimo di file diversi collidono appena il raggruppamento
	 * li mette insieme. `RTHexMatchIntegrationTests.cpp` ha gia' un `PlayOneTurn`, e la collisione
	 * compare o sparisce al cambiare dei file del modulo: un build verde non e' garanzia.
	 */
	void PlayOnePacingTurn(ARTTurnManager* TM)
	{
		TM->PlanBotsForTest();
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingSamplePerTurnTest,
	"RefactorTactics.Pacing.EveryTurnProducesOneSample",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingSamplePerTurnTest::RunTest(const FString&)
{
	UWorld* World = MakeHexPacingWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexPacingMap(World, /*Radius=*/ 5);

	ARTUnit* A1 = SpawnHexPacingUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-4, 2));
	ARTUnit* A2 = SpawnHexPacingUnit(World, 0, ERTArchetype::Guardian, FRTCellId(-4, 3));
	ARTUnit* B1 = SpawnHexPacingUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(4, -2));
	ARTUnit* B2 = SpawnHexPacingUnit(World, 1, ERTArchetype::Guardian, FRTCellId(4, -3));
	ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
	if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexPacingWorld(World); return false; }

	int32 TurnsPlayed = 0;
	while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < 40)
	{
		PlayOnePacingTurn(TM);
		++TurnsPlayed;
	}

	// Un campione per turno, ULTIMO COMPRESO. Il turno decisivo passa da ConcludeTurn, che pero' esce
	// prima di incrementare TurnNumber quando la partita finisce: se il campione si chiudesse dopo quel
	// ritorno anticipato, l'unico turno che decide la partita non verrebbe mai misurato.
	TestEqual(TEXT("un campione per turno giocato"), TM->GetPacingSamples().Num(), TurnsPlayed);
	TestTrue(TEXT("la partita si e' decisa"), TM->GetPhase() == ERTMatchPhase::MatchEnded);

	// I numeri di turno sono progressivi da 1: nessun buco, nessun doppione.
	for (int32 I = 0; I < TM->GetPacingSamples().Num(); ++I)
	{
		TestEqual(FString::Printf(TEXT("campione %d e' del turno %d"), I, I + 1),
			TM->GetPacingSamples()[I].TurnNumber, I + 1);
	}

	DestroyHexPacingWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingCompositionTest,
	"RefactorTactics.Pacing.RecordsDecisionComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingCompositionTest::RunTest(const FString&)
{
	UWorld* World = MakeHexPacingWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnHexPacingMap(World, /*Radius=*/ 3);

	ARTUnit* A1 = SpawnHexPacingUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-2, 1));
	ARTUnit* B1 = SpawnHexPacingUnit(World, 1, ERTArchetype::Guardian, FRTCellId(2, -1));
	ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
	if (!TM || !A1 || !B1) { DestroyHexPacingWorld(World); return false; }

	// Il turno 1 e' in pianificazione: simuliamo la mano del giocatore senza passare dal controller.
	TM->RecordPlanningInput(ERTPlanningInput::Selection);
	TM->RecordPlanningInput(ERTPlanningInput::Order);
	TM->RecordPlanningInput(ERTPlanningInput::Order);
	TM->RecordPlanningInput(ERTPlanningInput::Undo);
	TM->RecordPlanningInput(ERTPlanningInput::Click); // attivita' generica: non incrementa nessun contatore

	PlayOnePacingTurn(TM);

	if (!TestTrue(TEXT("almeno un campione"), TM->GetPacingSamples().Num() >= 1))
	{
		DestroyHexPacingWorld(World);
		return false;
	}
	const FRTPacingSample& S = TM->GetPacingSamples()[0];
	TestEqual(TEXT("una selezione"), S.SelectionCount, 1);
	TestEqual(TEXT("due ordini"), S.OrderCount, 2);
	TestEqual(TEXT("un annullamento"), S.UndoCount, 1);
	TestTrue(TEXT("il lock-in manuale non e' un timeout"), S.LockInSource == ERTLockInSource::Input);
	TestEqual(TEXT("due unita' vive, una per squadra"), S.UnitsAliveTeam0 + S.UnitsAliveTeam1, 2);

	DestroyHexPacingWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: Compilare per vedere il fallimento**

Run: `Build.bat RefactorTacticsEditor …`
Expected: errore di compilazione — `ARTTurnManager` non ha `RecordPlanningInput` né `GetPacingSamples`.

- [ ] **Step 3: Dichiarare l'interfaccia in `RTTurnManager.h`**

Aggiungere l'include accanto agli altri in testa al file:

```cpp
#include "Turn/RTPacing.h"
```

Nella sezione `public:`, dopo `GetLastMoveRoutes()` (riga ~76):

```cpp
	// --- Sonda di pacing (TELEMETRIA: nessun ritorno verso il gameplay) --------------------------
	/**
	 * Registra un input di pianificazione. Chiamata dal PlayerController, che NON cronometra: tutto il
	 * tempo vive qui, in un posto solo. Ignorata fuori dalla fase di pianificazione.
	 */
	void RecordPlanningInput(ERTPlanningInput Kind);

	/** Campioni di pacing della sessione corrente (sola lettura; telemetria, non stato di gioco). */
	const TArray<FRTPacingSample>& GetPacingSamples() const { return PacingSamples; }

	/** Se vero, ogni turno appende una riga in Saved/RT/pacing_<sessione>.csv. L'accumulo in memoria e' sempre attivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Pacing")
	bool bRecordPacing = false;

	/** Squadra il cui spazio di decisione si misura in ActionsAvailable (il giocatore umano e' il team 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Pacing")
	int32 PacingTeamId = 0;
```

Nella sezione `private:`, accanto agli altri helper (dopo `float DurationForPlaybackPhase(...) const;`, riga ~181):

```cpp
	// --- Sonda di pacing ------------------------------------------------------------------------
	void BeginPacingSample();
	void ClosePacingSample();
	void AppendPacingRow(const FRTPacingSample& Sample);

	TArray<FRTPacingSample> PacingSamples;
	FRTPacingSample PacingCurrent;
	double PacingPlanningStart = 0.0;  // FPlatformTime::Seconds() all'apertura della pianificazione
	double PacingLastInput = 0.0;
	bool bPacingHadInput = false;
	FString PacingFilePath;            // vuoto finche' non si scrive la prima riga
```

- [ ] **Step 4: Implementare gli agganci in `RTTurnManager.cpp`**

Aggiungere gli include in testa al file, accanto agli altri:

```cpp
#include "Turn/RTPacingLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"
```

**4a.** In `StartPlanningTimer()` (riga 281), subito dopo la guardia su `World` e **prima** di `PlanBots()`:

```cpp
	BeginPacingSample(); // apre il campione: il cronometro parte quando parte la pianificazione
```

**4b.** In `OnPlanningTimeout()` (riga 299), **prima** di `LockInAndResolve()`:

```cpp
	PacingCurrent.LockInSource = ERTLockInSource::Timeout; // non l'ha chiusa il giocatore
```

**4c.** In `LockInAndResolve()` (riga 315), subito **dopo** la guardia `if (Phase != ERTMatchPhase::Planning || bIsResolving) { return; }`:

```cpp
	// Sonda di pacing: chiude i tempi della pianificazione. Telemetria, nessun effetto sul turno.
	{
		const double Now = FPlatformTime::Seconds();
		PacingCurrent.MsToLockIn = FMath::RoundToInt((Now - PacingPlanningStart) * 1000.0);
		// Senza nessun input, "tempo dall'ultimo input" e' l'intera pianificazione: cosi' un turno passato
		// inerte finisce fra le attese a vuoto e non fra i tagli, che e' la classificazione corretta.
		PacingCurrent.MsSinceLastInput = bPacingHadInput
			? FMath::RoundToInt((Now - PacingLastInput) * 1000.0)
			: PacingCurrent.MsToLockIn;
		if (!bPacingHadInput)
		{
			PacingCurrent.MsToFirstInput = PacingCurrent.MsToLockIn;
		}
	}
```

**4d.** Sempre in `LockInAndResolve()` — le righe 318-391 sono **tutte in questa funzione**, non esiste nessuna `ResolveTurn()` — immediatamente **prima** di `if (bEnablePlayback && ResolvedTimeline.Num() > 0)` (riga 385):

```cpp
	// Il playback di QUESTO turno parte da zero anche se non verra' riprodotto: senza, il ramo senza
	// playback lascerebbe il valore del turno precedente e la misura leggerebbe una durata mai avvenuta.
	PlaybackElapsedTotal = 0.f;
```

**4e.** In `SkipPlayback()` (riga 1211), dopo la guardia `if (!bIsResolving) { return; }`:

```cpp
	PacingCurrent.bPlaybackSkipped = true;
```

**4f.** In `ConcludeTurn()` (riga 394), come **primissima istruzione della funzione**, prima di `DestroyDefeatedUnits()`:

```cpp
	// PRIMA di tutto il resto: a partita finita questa funzione esce anticipatamente, e il turno che
	// decide la partita e' proprio quello che non verrebbe mai misurato.
	ClosePacingSample();
```

**4g.** Aggiungere le tre funzioni in fondo al file:

```cpp
void ARTTurnManager::BeginPacingSample()
{
	PacingCurrent = FRTPacingSample();
	PacingCurrent.TurnNumber = TurnNumber;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, ARTUnit::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		const ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive())
		{
			continue;
		}
		(Unit->TeamId == 0 ? PacingCurrent.UnitsAliveTeam0 : PacingCurrent.UnitsAliveTeam1)++;

		if (Unit->TeamId != PacingTeamId)
		{
			continue; // ActionsAvailable misura lo spazio di decisione di CHI decide, non di tutti
		}
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			if (Unit->CanUseAbility(I))
			{
				++PacingCurrent.ActionsAvailable;
			}
		}
	}

	PacingPlanningStart = FPlatformTime::Seconds();
	PacingLastInput = PacingPlanningStart;
	bPacingHadInput = false;
}

void ARTTurnManager::RecordPlanningInput(ERTPlanningInput Kind)
{
	if (Phase != ERTMatchPhase::Planning)
	{
		return; // un input fuori dalla pianificazione non e' una decisione di turno
	}

	const double Now = FPlatformTime::Seconds();
	if (!bPacingHadInput)
	{
		bPacingHadInput = true;
		PacingCurrent.MsToFirstInput = FMath::RoundToInt((Now - PacingPlanningStart) * 1000.0);
	}
	PacingLastInput = Now;

	switch (Kind)
	{
	case ERTPlanningInput::Selection: ++PacingCurrent.SelectionCount; break;
	case ERTPlanningInput::Order:     ++PacingCurrent.OrderCount;     break;
	case ERTPlanningInput::Undo:      ++PacingCurrent.UndoCount;      break;
	case ERTPlanningInput::Click:
	default:
		break; // attivita' generica: aggiorna solo i tempi
	}
}

void ARTTurnManager::ClosePacingSample()
{
	PacingCurrent.MsPlayback = FMath::RoundToInt(PlaybackElapsedTotal * 1000.f);
	PacingSamples.Add(PacingCurrent);
	if (bRecordPacing)
	{
		AppendPacingRow(PacingCurrent);
	}
	PacingCurrent = FRTPacingSample();
}

void ARTTurnManager::AppendPacingRow(const FRTPacingSample& Sample)
{
	if (PacingFilePath.IsEmpty())
	{
		// Un file per esecuzione. Si scrive UNA RIGA PER TURNO: un riavvio della partita non perde nulla.
		const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RT"));
		IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/ true);
		PacingFilePath = FPaths::Combine(Dir,
			FString::Printf(TEXT("pacing_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"))));
		FFileHelper::SaveStringToFile(URTPacingLibrary::CsvHeader() + LINE_TERMINATOR, *PacingFilePath);
	}
	FFileHelper::SaveStringToFile(URTPacingLibrary::CsvRow(Sample) + LINE_TERMINATOR, *PacingFilePath,
		FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}
```

- [ ] **Step 5: Compilare ed eseguire i test**

Run: build (entrambi i target) + `-ExecCmds=Automation RunTests RefactorTactics.Pacing; Quit`
Expected: 6 test, 6 passati.

Se `EveryTurnProducesOneSample` riporta **un campione in meno** dei turni: `ClosePacingSample()` non è la prima istruzione di `ConcludeTurn()` e il turno decisivo si perde nel ritorno anticipato (4f).

- [ ] **Step 6: Verificare che l'intera suite sia ancora verde**

Run: `-ExecCmds=Automation RunTests RefactorTactics; Quit`
Expected: nessun test preesistente fallisce. I nuovi campi hanno tutti un default e nessun test esistente è stato modificato.

- [ ] **Step 7: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Tests/RTPacingIntegrationTests.cpp
git commit -m "feat(pacing): il turno si misura da solo, un campione per turno"
```

---

### Task 4: Il controller dice cosa ha fatto il giocatore

**Files:**
- Modify: `Source/RefactorTactics/Player/RTPlayerController.cpp` (righe ~270, ~320, ~361, ~553 e `SelectAbilityForCurrent`)
- Modify: `docs/design/test-manuali-pie.md`

**Interfaces:**
- Consumes: `ARTTurnManager::RecordPlanningInput(ERTPlanningInput)` (Task 3).
- Produces: nulla per le task successive.

> **Perché nessun test automatico qui**: questa task collega input umano a codice già coperto. Il comportamento dei contatori è verificato headless da `RefactorTactics.Pacing.RecordsDecisionComposition` (Task 3), che chiama `RecordPlanningInput` direttamente. Quello che resta da verificare è il **cablaggio**, e si verifica in PIE — che è la ragione per cui il progetto tiene `docs/design/test-manuali-pie.md`.

- [ ] **Step 1: Aggiungere l'helper di accesso al TurnManager**

In `RTPlayerController.cpp`, nel namespace anonimo in testa al file (o subito prima di `OnSelect`, se non ce n'è uno):

```cpp
namespace
{
	/** TurnManager del livello, o nullptr. La telemetria non deve mai far crashare l'input. */
	ARTTurnManager* PacingTurnManager(const UObject* WorldContext)
	{
		return Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(WorldContext, ARTTurnManager::StaticClass()));
	}
}
```

Se `RTTurnManager.h` non è già incluso in questo file, aggiungerlo agli include.

- [ ] **Step 2: Aggancio generico in `OnSelect`**

In `ARTPlayerController::OnSelect` (riga 270), come **prima istruzione della funzione**, prima di `FHitResult Hit;`:

```cpp
	// Attivita' generica: aggiorna i tempi anche quando il click non produce nulla. Un click a vuoto
	// e' comunque il giocatore che sta lavorando, e serve a non scambiarlo per un giocatore assente.
	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Click);
	}
```

- [ ] **Step 3: Aggancio sull'ordine di attacco**

In `OnSelect`, dentro il ramo che pianifica l'abilità, subito dopo `SelectedUnit->PlannedAttackTarget = ClickedUnit;` (riga ~321) e **prima** dello `UE_LOG` che lo segue:

```cpp
			if (ARTTurnManager* TM = PacingTurnManager(this))
			{
				TM->RecordPlanningInput(ERTPlanningInput::Order);
			}
```

- [ ] **Step 4: Aggancio sulla selezione**

In `OnSelect`, nel ramo di selezione, dentro `if (HitActor != SelectedActor)` (riga ~361), subito dopo `Selectable->OnSelected();`:

```cpp
			if (ARTTurnManager* TM = PacingTurnManager(this))
			{
				TM->RecordPlanningInput(ERTPlanningInput::Selection);
			}
```

- [ ] **Step 5: Aggancio sull'abilità scelta da tastiera**

In `ARTPlayerController::SelectAbilityForCurrent(int32 Index)` (riga 507) — è l'imbuto unico dei tasti 1-4 (righe 541-544), quindi **un solo punto invece di quattro** — subito **dopo `Unit->SelectAbility(Index);`** (riga 514).

Non «in fondo alla funzione»: sotto ci sono due `return` anticipati (`:510` nessuna unità, `:516` nessuna abilità) e il ramo `bSelfTarget` (`:521`), quindi «l'ultima istruzione» non è un punto unico. La riga 514 è invece dove l'intenzione del giocatore è già registrata, ed è attraversata da ogni pressione di tasto che produca un effetto:

```cpp
	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Order);
	}
```

- [ ] **Step 6: Aggancio sull'annullamento**

In `ARTPlayerController::OnUndoWaypoint` (riga 546), subito dopo `RebuildPlannedPath();` (riga 554):

```cpp
	if (ARTTurnManager* TM = PacingTurnManager(this))
	{
		TM->RecordPlanningInput(ERTPlanningInput::Undo);
	}
```

- [ ] **Step 7: Compilare e verificare che la suite resti verde**

Run: build (entrambi i target) + `-ExecCmds=Automation RunTests RefactorTactics; Quit`
Expected: compila; nessuna regressione. I test headless non passano dal controller, quindi il conteggio non cambia.

- [ ] **Step 8: Aggiungere la voce di verifica manuale**

In `docs/design/test-manuali-pie.md`, aggiungere in coda all'elenco:

```markdown
- ⏳ **Pacing — il cablaggio degli input**: in PIE, con `bRecordPacing = true` sul `TurnManager`, giocare
  un turno selezionando due volte un'unità, impartendo un ordine, annullando un waypoint e chiudendo con
  Spazio. Poi `rt.Debug.Pacing`: il sommario deve riportare **1 turno**, `SelectionCount` ≥ 2,
  `OrderCount` ≥ 1, `UndoCount` = 1 e **nessun taglio** (il lock-in è stato manuale). Verificare che
  `Saved/RT/pacing_*.csv` esista, abbia l'intestazione e **una riga per turno giocato**.
```

- [ ] **Step 9: Commit**

```bash
git add Source/RefactorTactics/Player/RTPlayerController.cpp docs/design/test-manuali-pie.md
git commit -m "feat(pacing): il controller dichiara gli input, il cronometro resta uno solo"
```

---

### Task 5: Lettura in gioco e prova dell'invariante

**Files:**
- Create: `Source/RefactorTactics/Turn/RTPacingConsole.cpp`
- Modify: `Source/RefactorTactics/Tests/RTPacingIntegrationTests.cpp`

**Interfaces:**
- Consumes: `URTPacingLibrary::SummarizeSamples` (Task 1), `ARTTurnManager::GetPacingSamples` e `bRecordPacing` (Task 3).
- Produces: comando console `rt.Debug.Pacing`.

- [ ] **Step 1: Scrivere il test dell'invariante**

Aggiungere in `RTPacingIntegrationTests.cpp`, prima di `#endif // WITH_DEV_AUTOMATION_TESTS`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPacingHashInvarianceTest,
	"RefactorTactics.Pacing.DoesNotAffectTurnLogHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPacingHashInvarianceTest::RunTest(const FString&)
{
	// La stessa partita giocata due volte, con la sonda spenta e accesa: gli esiti autoritativi devono
	// essere IDENTICI hash per hash. E' la dimostrazione eseguibile dell'invariante: la telemetria non ha
	// ritorno verso il gameplay. Oggi passa quasi per costruzione; serve per quando qualcuno sara' tentato
	// di far leggere un tempo di parete a una decisione.
	auto PlayMatchHashes = [this](bool bRecord, TArray<uint32>& OutHashes) -> bool
	{
		UWorld* World = MakeHexPacingWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		SpawnHexPacingMap(World, /*Radius=*/ 5);

		ARTUnit* A1 = SpawnHexPacingUnit(World, 0, ERTArchetype::Ranger,   FRTCellId(-4, 2));
		ARTUnit* A2 = SpawnHexPacingUnit(World, 0, ERTArchetype::Guardian, FRTCellId(-4, 3));
		ARTUnit* B1 = SpawnHexPacingUnit(World, 1, ERTArchetype::Ranger,   FRTCellId(4, -2));
		ARTUnit* B2 = SpawnHexPacingUnit(World, 1, ERTArchetype::Guardian, FRTCellId(4, -3));
		ARTTurnManager* TM = SpawnHexPacingTurnManager(World);
		if (!TM || !A1 || !A2 || !B1 || !B2) { DestroyHexPacingWorld(World); return false; }

		TM->bRecordPacing = bRecord;
		int32 TurnsPlayed = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && TurnsPlayed < 40)
		{
			PlayOnePacingTurn(TM);
			++TurnsPlayed;
			OutHashes.Add(URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog()));
		}
		DestroyHexPacingWorld(World);
		return true;
	};

	TArray<uint32> Off;
	TArray<uint32> On;
	if (!PlayMatchHashes(/*bRecord=*/ false, Off)) { return false; }
	if (!PlayMatchHashes(/*bRecord=*/ true,  On))  { return false; }

	TestEqual(TEXT("stesso numero di turni con sonda accesa e spenta"), On.Num(), Off.Num());
	const int32 Count = FMath::Min(On.Num(), Off.Num());
	for (int32 I = 0; I < Count; ++I)
	{
		// TestTrue e non TestEqual: gli hash sono uint32 e le overload di TestEqual sono ambigue su quel tipo.
		TestTrue(FString::Printf(TEXT("turno %d: hash del TurnLog identico"), I + 1), On[I] == Off[I]);
	}
	return true;
}
```

Aggiungere l'include in testa a `RTPacingIntegrationTests.cpp`, accanto agli altri:

```cpp
#include "Turn/RTTurnLogLibrary.h"
```

- [ ] **Step 2: Compilare ed eseguire — deve passare**

Run: build (entrambi i target) + `-ExecCmds=Automation RunTests RefactorTactics.Pacing; Quit`
Expected: 7 test, 7 passati.

Se gli hash divergono, **non è un test da aggiustare**: significa che qualcosa della sonda è finito in una decisione di gioco. Trovare il punto e toglierlo.

- [ ] **Step 3: Creare `Source/RefactorTactics/Turn/RTPacingConsole.cpp`**

```cpp
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Turn/RTPacingLibrary.h"
#include "Turn/RTTurnManager.h"

/**
 * `rt.Debug.Pacing` — sommario della sessione corrente. Sola lettura: non tocca lo stato di gioco.
 * Il prefisso `rt.Debug.*` anticipa il namespace di CP 11.4 (#80): quando quella issue verra' lavorata,
 * questo comando va aggiunto al suo elenco, non duplicato.
 */
static void RTDebugPacingCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (!World)
	{
		Ar.Log(TEXT("[RT] Nessun mondo attivo."));
		return;
	}
	ARTTurnManager* TM = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass()));
	if (!TM)
	{
		Ar.Log(TEXT("[RT] Nessun TurnManager nel livello."));
		return;
	}

	const FRTPacingSummary S = URTPacingLibrary::SummarizeSamples(TM->GetPacingSamples(), /*CutoffWindowMs=*/ 3000);
	Ar.Logf(TEXT("[RT] Pacing su %d turni:"), S.SampleCount);
	Ar.Logf(TEXT("[RT]   lock-in: mediana %d ms, p90 %d ms"), S.MedianMsToLockIn, S.P90MsToLockIn);
	Ar.Logf(TEXT("[RT]   tagli veri: %d | attese a vuoto: %d"), S.TrueCutoffs, S.IdleTimeouts);
	Ar.Logf(TEXT("[RT]   playback: mediana %d ms, saltati %d"), S.MedianMsPlayback, S.SkippedPlaybacks);
	Ar.Logf(TEXT("[RT]   lettura: tagli > 0 -> alza PlanningSeconds; tagli 0 e attese alte -> e' l'interfaccia, non il timer."));
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugPacing(
	TEXT("rt.Debug.Pacing"),
	TEXT("Sommario del pacing della sessione corrente (telemetria: nessun effetto sul gioco)."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugPacingCommand));
```

- [ ] **Step 4: Compilare e verificare la suite completa**

Run: build (entrambi i target) + `-ExecCmds=Automation RunTests RefactorTactics; Quit`
Expected: compila su entrambi i target; suite verde, 7 test `Pacing` inclusi.

- [ ] **Step 5: Verificare che il CSV non finisca nel versionamento**

Run:
```bash
git status --short
```
Expected: **nessun** file sotto `Saved/` compare. `Saved/` è già escluso; se comparisse, è un `.gitignore` da correggere, non un file da aggiungere.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Turn/RTPacingConsole.cpp Source/RefactorTactics/Tests/RTPacingIntegrationTests.cpp
git commit -m "feat(pacing): rt.Debug.Pacing legge la sessione, un test prova che non tocca il gioco"
```

---

### Task 6: Predisporre il KPI e chiudere la documentazione

**Files:**
- Modify: `docs/design/v0.1-definition-of-done.md` (§4, tabella KPI)
- Modify: `docs/design/spec-pacing-turno.md` (§10)

**Interfaces:**
- Consumes: nulla di codice.
- Produces: la riga KPI che il playtest riempirà.

- [ ] **Step 1: Aggiungere la riga KPI**

In `docs/design/v0.1-definition-of-done.md`, tabella §4 (colonne `Budget | Target | Metodo di misura | Stato`), inserire **dopo** la riga `Resolver` — accanto alle altre misure di tempo — e **prima** di `Replay divergence`:

```markdown
| Decisione del giocatore (p90 al lock-in) | nessun taglio | `rt.Debug.Pacing` su ≥ 10 partite in PIE con `bRecordPacing`; nearest-rank su `MsToLockIn`; taglio = timeout con input < 3 s prima | ⏳ **riserva: campione di un solo giocatore, che è l'autore del gioco → il p90 sottostima un giocatore nuovo.** Ri-misurare alla chiusura di E4, E5, E6 |
```

Il target è **«nessun taglio»** e non un numero di secondi: il vincolo della spec §2 è qualitativo e misurabile, e mettere lì un numero rimetterebbe in tabella proprio l'ipotesi che questa fetta serve a eliminare.

Nota: la tabella ha già una riga sul **numero** di round, che è l'issue `#96` — riga diversa, problema diverso
(spec §10). ⚠️ **Aggiornata il 2026-08-07**: era `Turni per partita ≤ 12`, ora è `Round per partita (2v2 v0.1)
10–14`, perché il limite è diventato un parametro di formato
([`spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md) §6). Anche la §4 della DoD ha
tre righe nuove sulla **durata** che useranno lo stesso canale di questa sonda: non duplicarle qui.

- [ ] **Step 2: Aggiornare il riferimento a `#96` nella spec**

`spec-pacing-turno.md` §10 dice che una partita dura «25 contro i 12 del catalogo». Il dato è stato superato: il commento di `RefactorTactics.HexMatch.PlaysToCompletion` riporta ora «MISURATO (2026-08-06): la partita si decide al turno 10, dentro il limite di 12 del catalogo v0.1. Era 25 finché lo scudo delle abilità di supporto non scadeva». Sostituire il punto elenco con:

```markdown
- **Non affronta `#96`**: quello è il **numero** di turni (misurato a 10 dopo la scadenza dello scudo nel
  Cleanup; era 25 prima), questo è il **tempo** di un turno. Confonderli farebbe tarare il pacing su una
  partita che è lunga per un altro motivo.
```

- [ ] **Step 3: Aprire la issue di checkpoint**

```bash
gh issue create --title "Pacing: misurare il tempo di decisione del giocatore" --label "v0.1" --body "Vedi \`docs/design/spec-pacing-turno.md\`.

Non è un'estensione di #41: quella DoD misura la macchina (FPS, path, preview, resolver), questa misura il comportamento umano; #41 può chiudersi subito, questo aspetta una sessione di gioco; la ri-misura va ripetuta a ogni epic che allarga la decisione (E4, E5, E6).

### Definition of Done
- [ ] \`FRTPacingSample\` + \`URTPacingLibrary\` puri e testati
- [ ] Sei agganci nel TurnManager, un campione per turno (ultimo compreso)
- [ ] Il controller notifica gli input, senza cronometrare
- [ ] \`rt.Debug.Pacing\` stampa il sommario; CSV in \`Saved/RT/\`, fuori dal versionamento
- [ ] \`HashTurnLog\` invariato con sonda accesa (\`RefactorTactics.Pacing.DoesNotAffectTurnLogHash\`)
- [ ] Riga KPI predisposta in \`v0.1-definition-of-done.md\` §4 con metodo e riserva sul campione
- [ ] Voce ⏳ in \`test-manuali-pie.md\` per il cablaggio degli input"
```

- [ ] **Step 4: Commit**

```bash
git add docs/design/v0.1-definition-of-done.md docs/design/spec-pacing-turno.md
git commit -m "docs(pacing): la riga KPI che il playtest dovra' riempire"
```

---

## Riepilogo dei test prodotti

| Test | Task |
|---|---|
| `RefactorTactics.Pacing.PercentileNearestRank` | 1 |
| `RefactorTactics.Pacing.SummaryOfEmptySampleIsZero` | 1 |
| `RefactorTactics.Pacing.CutoffVsIdleTimeout` | 1 |
| `RefactorTactics.Pacing.CsvRowMatchesHeader` | 2 |
| `RefactorTactics.Pacing.EveryTurnProducesOneSample` | 3 |
| `RefactorTactics.Pacing.RecordsDecisionComposition` | 3 |
| `RefactorTactics.Pacing.DoesNotAffectTurnLogHash` | 5 |

Sette test: i sei della spec §8 più `RecordsDecisionComposition`, che copre headless i contatori che la
Task 4 si limita a cablare.
