# TurnLog + reason codes — Implementation Plan

> `HISTORICAL` · **Piano eseguito, non normativo**
> Lo dichiarava gia' in prosa nella riga sotto; qui acquista il banner che rende la cartella
> leggibile a un criterio meccanico. L'owner del formato e' `ERTTurnLogFormatVersion`
> (`Source/RefactorTactics/Turn/RTTurnLog.h`) e [`../../technical/architecture/spec-turnlog.md`](../../technical/architecture/spec-turnlog.md).
>
> 📦 **Piano di esecuzione consegnato** — **riferimento storico, non normativo.**
> Gli snippet di codice usano API rimosse al **CP 7.2** (`URTGridLibrary`, `FRTGridCoord`). La feature è viva: `URTTurnLogLibrary` è in `main`, con hash permutazione-invariante, serializzazione versionata e checksum.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produrre un TurnLog strutturato, deterministico e permutazione-invariante degli esiti di Movimento e Combat, con reason codes (enum interi), riflessi nel combat log HUD.

**Architecture:** Osservabilità autoritativa separata dalla presentazione. Gli outcome sono esposti da funzioni pure (`ResolvePaths` per il movimento, `URTCombatLibrary::ClassifyCombatOutcome` per il combat); `ARTTurnManager` è solo collettore e ordina il log in modo deterministico. Non riusa `FRTResolvedEvent` (playback).

**Tech Stack:** Unreal Engine 5.8.1, C++, Unreal Automation Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`).

**Spec:** [`spec-turnlog.md`](../../technical/architecture/spec-turnlog.md).

## Global Constraints

- Prefissi classi/tipi **`RT`/`URT`/`FRT`/`ERT`**; PascalCase; risposte/commenti in italiano, identificatori in inglese.
- **Invarianti** (canone §5): #1 le regole decidono / la presentazione riproduce; #3 raccogli-poi-applica; #4 **determinismo — nessun float** nel log/ordinamento; #7 combat math = funzioni pure testate.
- **Chiave unità nel log = cella di partenza del turno** (`SrcCell`), mai pointer/spawn-order.
- **Priorità Combat** (enum a valore singolo): `Lethal` > `ShieldAbsorbed` > `TerrainBonus` > `Hit`; `NoLineOfSight` = ramo distinto (attacco non applicato).
- **Additività**: i test esistenti restano verdi (i campi nuovi hanno default).
- **Ambiente**: l'editor UE è aperto → durante l'implementazione si verifica solo la **compilazione** (`Build.bat RefactorTactics Win64 Development -project=... -waitmutex`, target Game); l'**esecuzione dei test Automation** (`UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Automation RunTests RefactorTactics; Quit" -nullrhi -unattended -nopause -nosplash -log`) va fatta **a editor chiuso**. Ogni step "run test" è perciò: compila ora; esegui a editor chiuso.
- Branch: `feat/skeletal-units` (lavoro isolato in commit propri); nessun push senza richiesta.
- Engine: `D:\EpicGames\UE_5.8`; uproject: `D:\Repositories\refactor-tactics-main\RefactorTactics.uproject`.

---

### Task 1: Movimento — `RTTurnLog.h` + `FRTPathResult.Outcome`

**Files:**
- Create: `Source/RefactorTactics/Turn/RTTurnLog.h`
- Modify: `Source/RefactorTactics/Turn/RTMovementResolver.h` (include `RTTurnLog.h`; `FRTPathResult` += `Outcome`)
- Modify: `Source/RefactorTactics/Turn/RTMovementResolver.cpp` (`ResolvePaths` popola `Outcome`)
- Test: `Source/RefactorTactics/Tests/RTTurnLogTests.cpp` (create)

**Interfaces:**
- Produces: `enum class ERTLogCategory : uint8 { Move, Combat }`; `enum class ERTMoveOutcome : uint8 { Stayed, Moved, BlockedContested, BlockedByUnit }`; `enum class ERTCombatOutcome : uint8 { Hit, ShieldAbsorbed, Lethal, NoLineOfSight, TerrainBonus }`; `struct FRTTurnLogEntry`; `FRTPathResult.Outcome` (default `Stayed`).

- [ ] **Step 1: Create `RTTurnLog.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnRules.h"
#include "RTTurnLog.generated.h"

/** Categoria dell'esito registrato nel TurnLog. */
UENUM(BlueprintType)
enum class ERTLogCategory : uint8 { Move, Combat };

/** Esito del movimento di un'unita' nel turno (dal resolver ResolvePaths). */
UENUM(BlueprintType)
enum class ERTMoveOutcome : uint8
{
	Stayed,           // non pianificava movimento (path < 2 celle)
	Moved,            // raggiunta la destinazione pianificata (scambio incluso)
	BlockedContested, // fermata (o parziale) per destinazione contesa
	BlockedByUnit     // fermata (o parziale) per cella occupata da un'unita' ferma
};

/** Esito di un attacco nel turno. Priorita': Lethal > ShieldAbsorbed > TerrainBonus > Hit. */
UENUM(BlueprintType)
enum class ERTCombatOutcome : uint8
{
	Hit,            // danno inflitto agli HP
	ShieldAbsorbed, // danno assorbito interamente dallo scudo (HP invariati)
	Lethal,         // bersaglio portato a HP <= 0
	NoLineOfSight,  // attacco pianificato scartato per LOS bloccata
	TerrainBonus    // colpo a segno con bonus altura (+danno), non letale
};

/**
 * Voce del TurnLog: un esito autoritativo del turno con il suo reason code. Osservabilita' separata
 * dalla presentazione (non e' FRTResolvedEvent). Deterministica: la chiave dell'unita' e' la sua cella
 * di partenza del turno (max 1 unita'/cella), mai un pointer.
 */
USTRUCT(BlueprintType)
struct FRTTurnLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	ERTMatchPhase Phase = ERTMatchPhase::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	ERTLogCategory Category = ERTLogCategory::Move;

	/** Valore dell'enum di categoria (ERTMoveOutcome se Move, ERTCombatOutcome se Combat). Intero: no float (#4). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	uint8 Outcome = 0;

	/** Chiave stabile: cella di partenza dell'unita' nel turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FRTGridCoord SrcCell;

	/** Bersaglio (Combat) o destinazione (Move); = SrcCell se non applicabile. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FRTGridCoord TgtCell;

	/** Danno effettivo (Combat) o numero di celle percorse (Move). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 Amount = 0;

	FRTTurnLogEntry() = default;
};
```

- [ ] **Step 2: Add `Outcome` to `FRTPathResult`**

In `RTMovementResolver.h`, aggiungi l'include e il campo (dopo `Entered`):

```cpp
#include "Turn/RTTurnLog.h"   // in cima, dopo gli altri include
```
```cpp
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Turn")
	TArray<FRTGridCoord> Entered;

	/** Perche' il movimento e' finito cosi' (default: nessun movimento pianificato). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Turn")
	ERTMoveOutcome Outcome = ERTMoveOutcome::Stayed;
```

- [ ] **Step 3: Write the failing test** in `RTTurnLogTests.cpp`

```cpp
#include "Misc/AutomationTest.h"
#include "Turn/RTMovementResolver.h"
#include "Turn/RTTurnLog.h"
#include "Core/RTTypes.h"
#include "Algo/Reverse.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogMoveOutcomesTest,
	"RefactorTactics.TurnLog.MoveOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogMoveOutcomesTest::RunTest(const FString&)
{
	// Stayed: path < 2 celle.
	{
		const TArray<TArray<FRTGridCoord>> P = { { FRTGridCoord(0,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("stayed"), R[0].Outcome == ERTMoveOutcome::Stayed);
	}
	// Moved: percorso libero fino in fondo.
	{
		const TArray<TArray<FRTGridCoord>> P = { { FRTGridCoord(0,0), FRTGridCoord(1,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("moved"), R[0].Outcome == ERTMoveOutcome::Moved);
	}
	// BlockedContested: due verso la stessa cella.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(2,0), FRTGridCoord(1,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("u0 contested"), R[0].Outcome == ERTMoveOutcome::BlockedContested);
		TestTrue(TEXT("u1 contested"), R[1].Outcome == ERTMoveOutcome::BlockedContested);
	}
	// BlockedByUnit: u1 ferma su (1,0), u0 prova a entrarci.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(1,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("u0 blocked-by-unit"), R[0].Outcome == ERTMoveOutcome::BlockedByUnit);
		TestTrue(TEXT("u1 stayed"), R[1].Outcome == ERTMoveOutcome::Stayed);
	}
	// Scambio -> Moved per entrambi.
	{
		const TArray<TArray<FRTGridCoord>> P = {
			{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
			{ FRTGridCoord(1,0), FRTGridCoord(0,0) } };
		const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
		TestTrue(TEXT("swap u0 moved"), R[0].Outcome == ERTMoveOutcome::Moved);
		TestTrue(TEXT("swap u1 moved"), R[1].Outcome == ERTMoveOutcome::Moved);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTurnLogMoveOrderInvariantTest,
	"RefactorTactics.TurnLog.MoveOutcomeOrderInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTurnLogMoveOrderInvariantTest::RunTest(const FString&)
{
	const TArray<TArray<FRTGridCoord>> P = {
		{ FRTGridCoord(0,0), FRTGridCoord(1,0) },
		{ FRTGridCoord(2,0), FRTGridCoord(1,0) } };   // contesa con u0
	TArray<TArray<FRTGridCoord>> Rev = P; Algo::Reverse(Rev);
	const TArray<FRTPathResult> R = URTMovementResolver::ResolvePaths(P);
	const TArray<FRTPathResult> RR = URTMovementResolver::ResolvePaths(Rev);
	TestTrue(TEXT("outcome invariante"), R[0].Outcome == RR[1].Outcome && R[1].Outcome == RR[0].Outcome);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 4: Run test to verify it fails** (a editor chiuso)

Run: `UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Automation RunTests RefactorTactics.TurnLog; Quit" -nullrhi -unattended -nopause -nosplash -log`
Expected: FAIL (Outcome sempre `Stayed`, non ancora popolato). Con editor aperto: verificare almeno che **compili**.

- [ ] **Step 5: Implement — popola `Outcome` in `ResolvePaths`**

In `RTMovementResolver.cpp`, dentro `ResolvePaths`, aggiungi un array del motivo di blocco e classifica a fine loop.

Dopo `Results[i].Final = Pos[i];` (nel primo for), aggiungi la tracking:
```cpp
	TArray<ERTMoveOutcome> BlockReason; BlockReason.Init(ERTMoveOutcome::BlockedByUnit, N);
```
Nel punto di congelamento, dove oggi c'e' `if (bBlocked) { ToFreeze.Add(i); }`, registra il motivo:
```cpp
			if (bBlocked)
			{
				ToFreeze.Add(i);
				BlockReason[i] = (Contenders >= 2) ? ERTMoveOutcome::BlockedContested : ERTMoveOutcome::BlockedByUnit;
			}
```
Prima di `return Results;`, classifica:
```cpp
	for (int32 i = 0; i < N; ++i)
	{
		if (Paths[i].Num() <= 1) { Results[i].Outcome = ERTMoveOutcome::Stayed; }
		else if (Results[i].Final == Paths[i].Last()) { Results[i].Outcome = ERTMoveOutcome::Moved; }
		else { Results[i].Outcome = BlockReason[i]; }
	}
```

- [ ] **Step 6: Run test to verify it passes** (a editor chiuso)

Run: come Step 4. Expected: PASS (`RefactorTactics.TurnLog.MoveOutcomes`, `...OrderInvariant`). Verifica anche che i test `ResolvePaths*` esistenti restino verdi.

- [ ] **Step 7: Verify compilation now** (editor aperto)

Run: `Build.bat RefactorTactics Win64 Development -project="<uproject>" -waitmutex`
Expected: `Result: Succeeded`.

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnLog.h Source/RefactorTactics/Turn/RTMovementResolver.h Source/RefactorTactics/Turn/RTMovementResolver.cpp Source/RefactorTactics/Tests/RTTurnLogTests.cpp
git commit -m "feat(turnlog): ERTMoveOutcome esposto da ResolvePaths (TL.1)"
```

---

### Task 2: Combat — `URTCombatLibrary::ClassifyCombatOutcome`

**Files:**
- Modify: `Source/RefactorTactics/Combat/RTCombatLibrary.h` (dichiara `ClassifyCombatOutcome`)
- Modify: `Source/RefactorTactics/Combat/RTCombatLibrary.cpp` (implementa)
- Test: `Source/RefactorTactics/Tests/RTCombatLibraryTests.cpp` (aggiungi test)

**Interfaces:**
- Consumes: `ERTCombatOutcome` (Task 1, `RTTurnLog.h`).
- Produces: `static ERTCombatOutcome URTCombatLibrary::ClassifyCombatOutcome(int32 HealthBefore, int32 HealthAfter, int32 AttackerDmgBonus)`.

- [ ] **Step 1: Write the failing test** in `RTCombatLibraryTests.cpp`

Aggiungi (l'include `#include "Turn/RTTurnLog.h"` in cima al file se assente):

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTClassifyCombatOutcomeTest,
	"RefactorTactics.Combat.ClassifyCombatOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTClassifyCombatOutcomeTest::RunTest(const FString&)
{
	using URTCombatLibrary;
	// ShieldAbsorbed: HP invariati (lo scudo ha assorbito tutto).
	TestTrue(TEXT("shield"), URTCombatLibrary::ClassifyCombatOutcome(100, 100, 0) == ERTCombatOutcome::ShieldAbsorbed);
	// Hit: HP calano, nessun bonus, non letale.
	TestTrue(TEXT("hit"), URTCombatLibrary::ClassifyCombatOutcome(100, 70, 0) == ERTCombatOutcome::Hit);
	// TerrainBonus: HP calano, bonus altura > 0, non letale.
	TestTrue(TEXT("terrain"), URTCombatLibrary::ClassifyCombatOutcome(100, 55, 15) == ERTCombatOutcome::TerrainBonus);
	// Lethal: HP a 0 (priorita' su tutto).
	TestTrue(TEXT("lethal"), URTCombatLibrary::ClassifyCombatOutcome(30, 0, 0) == ERTCombatOutcome::Lethal);
	// Lethal ha priorita' sul bonus altura.
	TestTrue(TEXT("lethal>terrain"), URTCombatLibrary::ClassifyCombatOutcome(30, 0, 15) == ERTCombatOutcome::Lethal);
	return true;
}
```
> Nota: rimuovi la riga `using URTCombatLibrary;` (era un refuso) — chiama direttamente `URTCombatLibrary::ClassifyCombatOutcome`.

- [ ] **Step 2: Run test to verify it fails** (a editor chiuso)

Run: `...-ExecCmds="Automation RunTests RefactorTactics.Combat.ClassifyCombatOutcome; Quit"...`
Expected: FAIL ("ClassifyCombatOutcome not declared"). Editor aperto: la build fallisce (simbolo mancante) = atteso.

- [ ] **Step 3: Implement**

In `RTCombatLibrary.h`, aggiungi l'include e la dichiarazione:
```cpp
#include "Turn/RTTurnLog.h"   // per ERTCombatOutcome
```
```cpp
	/**
	 * Classifica l'esito di un colpo a segno secondo la priorita' Lethal > ShieldAbsorbed > TerrainBonus > Hit.
	 * ShieldAbsorbed = HP invariati (assorbito dallo scudo). TerrainBonus = HP calati con bonus altura > 0.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static ERTCombatOutcome ClassifyCombatOutcome(int32 HealthBefore, int32 HealthAfter, int32 AttackerDmgBonus);
```
In `RTCombatLibrary.cpp`:
```cpp
ERTCombatOutcome URTCombatLibrary::ClassifyCombatOutcome(int32 HealthBefore, int32 HealthAfter, int32 AttackerDmgBonus)
{
	if (HealthBefore > 0 && HealthAfter <= 0) { return ERTCombatOutcome::Lethal; }
	if (HealthAfter == HealthBefore)          { return ERTCombatOutcome::ShieldAbsorbed; }
	if (AttackerDmgBonus > 0)                 { return ERTCombatOutcome::TerrainBonus; }
	return ERTCombatOutcome::Hit;
}
```

- [ ] **Step 4: Run test to verify it passes** (a editor chiuso). Expected: PASS.

- [ ] **Step 5: Verify compilation** (editor aperto)

Run: `Build.bat RefactorTactics Win64 Development -project="<uproject>" -waitmutex` → `Result: Succeeded`.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Combat/RTCombatLibrary.h Source/RefactorTactics/Combat/RTCombatLibrary.cpp Source/RefactorTactics/Tests/RTCombatLibraryTests.cpp
git commit -m "feat(turnlog): ClassifyCombatOutcome (Hit/Shield/Lethal/TerrainBonus) (TL.2)"
```

---

### Task 3: Wiring — TurnLog in `ARTTurnManager` + NoLineOfSight + log arricchito

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h` (membro `TurnLog` + getter + include `RTTurnLog.h`)
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp` (popola in `ResolveMovement`/`ResolveCombat`; separa LOS; ordina; `AddLogEvent` arricchito)

**Interfaces:**
- Consumes: `FRTTurnLogEntry`, `ERTLogCategory`, `ERTMoveOutcome`, `ERTCombatOutcome` (Task 1); `URTCombatLibrary::ClassifyCombatOutcome` (Task 2); `FRTPathResult.Outcome` (Task 1).
- Produces: `const TArray<FRTTurnLogEntry>& ARTTurnManager::GetTurnLog() const`.

> **Nota di verifica**: questo task e' **wiring** dell'Actor → non unit-testabile in automation. Verifica = **build Game Succeeded** + **PIE** (il combat log mostra i reason). La logica classificatrice e' gia' coperta dai Task 1/2.

- [ ] **Step 1: Declare TurnLog** in `RTTurnManager.h`

Include in cima: `#include "Turn/RTTurnLog.h"` (gia' incluso `RTResolvedEvent.h`). Nella sezione `public` (vicino a `GetRecentEvents`):
```cpp
	/** Esiti autoritativi dell'ultimo turno risolto (Movimento + Combat), ordinati deterministicamente. */
	const TArray<FRTTurnLogEntry>& GetTurnLog() const { return TurnLog; }
```
Nella sezione `protected` (vicino a `RecentEvents`):
```cpp
	TArray<FRTTurnLogEntry> TurnLog;
```

- [ ] **Step 2: Popola il Movimento** in `RTTurnManager.cpp::ResolveMovement`

Dove oggi c'e' `const TArray<FRTPathResult> Resolved = URTMovementResolver::ResolvePaths(Paths);` (riga ~902), il codice itera gia' su un array parallelo di unita' con i loro `Paths`. Per ogni unita' `Unit` con la sua `FRTPathResult& PR` e la cella di partenza `Start` (= `Paths[i][0]`), dopo l'applicazione aggiungi:
```cpp
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Move;
		E.Category = ERTLogCategory::Move;
		E.Outcome = static_cast<uint8>(PR.Outcome);
		E.SrcCell = Start;
		E.TgtCell = PR.Final;
		E.Amount = PR.Entered.Num();
		TurnLog.Add(E);
		if (PR.Outcome == ERTMoveOutcome::BlockedContested)
			AddLogEvent(FString::Printf(TEXT("%s: fermo (cella contesa)"), *Unit->GetName()));
		else if (PR.Outcome == ERTMoveOutcome::BlockedByUnit)
			AddLogEvent(FString::Printf(TEXT("%s: fermo (cella occupata)"), *Unit->GetName()));
```
> All'inizio di `ResolveMovement`, prima di riempire il log, azzera: `TurnLog.Reset();` (una sola volta per turno — se `ResolveCombat` gira prima, azzera in `LockInAndResolve` o nella prima fase risolta; vedi Step 4).

- [ ] **Step 3: Popola il Combat + `NoLineOfSight`** in `RTTurnManager.cpp::ResolveCombat`

Separa la condizione LOS dal `continue` cumulativo (riga ~612-618) per riconoscere lo scarto per sola LOS:
```cpp
		const bool bBaseValid = Ability && !Ability->bDash && Target && IndexOf.Contains(Target)
			&& Target->TeamId != Unit->TeamId && Unit->CanUseAbility(AbilityIndex)
			&& URTGridLibrary::IsWithinRange(Unit->GridCell, Target->GridCell, Ability->RangeCells);
		if (bBaseValid && !URTGridLibrary::HasLineOfSight(Unit->GridCell, Target->GridCell, Blockers))
		{
			FRTTurnLogEntry E;
			E.Phase = ERTMatchPhase::Blast; E.Category = ERTLogCategory::Combat;
			E.Outcome = static_cast<uint8>(ERTCombatOutcome::NoLineOfSight);
			E.SrcCell = Unit->GridCell; E.TgtCell = Target->GridCell; E.Amount = 0;
			TurnLog.Add(E);
			AddLogEvent(FString::Printf(TEXT("%s -> %s: nessuna linea di tiro"), *Unit->GetName(), *Target->GetName()));
		}
		if (!bBaseValid || !URTGridLibrary::HasLineOfSight(Unit->GridCell, Target->GridCell, Blockers))
		{
			continue;
		}
```
Dopo `ResolveAttacks` (riga ~703), per ogni bersaglio colpito costruisci l'entry con la classificazione (usa `BeforeHP`/`AfterHP` gia' calcolati e il bonus altura dell'attaccante). Per ogni attacco `A` con attaccante `Src` (cella `SrcCell`), bersaglio `Tgt` (indice `Idx`, cella `TgtCell`), bonus `DmgBonus`:
```cpp
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Blast; E.Category = ERTLogCategory::Combat;
		E.Outcome = static_cast<uint8>(URTCombatLibrary::ClassifyCombatOutcome(BeforeHP[Idx], AfterHP[Idx], DmgBonus));
		E.SrcCell = SrcCell; E.TgtCell = TgtCell; E.Amount = BeforeHP[Idx] - AfterHP[Idx];
		TurnLog.Add(E);
```
> Nota di implementazione: mantieni, accanto agli `Attacks`, la cella dell'attaccante e il suo `DmgBonus` (array paralleli a quelli gia' presenti — `Attackers`, `IndexOf`), cosi' da avere `SrcCell`/`DmgBonus` per ogni bersaglio in fase di logging.

- [ ] **Step 4: Azzera e ordina il TurnLog deterministicamente**

Azzera il log all'inizio della risoluzione del turno (in `LockInAndResolve`, prima delle fasi): `TurnLog.Reset();`.
Al termine della risoluzione (dopo l'ultima fase, es. fine di `ResolveMovement`), ordina in modo stabile e deterministico (fase → categoria → SrcCell), perche' `GetAllActorsOfClass` non e' ordinato:
```cpp
	TurnLog.Sort([](const FRTTurnLogEntry& A, const FRTTurnLogEntry& B)
	{
		if (A.Phase != B.Phase) { return A.Phase < B.Phase; }
		if (A.Category != B.Category) { return A.Category < B.Category; }
		if (A.SrcCell.X != B.SrcCell.X) { return A.SrcCell.X < B.SrcCell.X; }
		if (A.SrcCell.Y != B.SrcCell.Y) { return A.SrcCell.Y < B.SrcCell.Y; }
		if (A.SrcCell.Layer != B.SrcCell.Layer) { return A.SrcCell.Layer < B.SrcCell.Layer; }
		return A.TgtCell.X < B.TgtCell.X;
	});
```

- [ ] **Step 5: Verify compilation** (editor aperto)

Run: `Build.bat RefactorTactics Win64 Development -project="<uproject>" -waitmutex` → `Result: Succeeded`.

- [ ] **Step 6: Verify in PIE** (richiede editor / interazione utente)

Avvia PIE, gioca un turno con un movimento bloccato e un attacco senza LOS. Atteso: il combat log a schermo mostra le righe con reason (es. «fermo (cella contesa)», «nessuna linea di tiro»).

- [ ] **Step 7: Full test run** (a editor chiuso)

Run: `UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Automation RunTests RefactorTactics; Quit" -nullrhi -unattended -nopause -nosplash -log`
Expected: tutti i test verdi (esistenti + nuovi TurnLog/Combat). Conta i `Result={Fail}` = 0.

- [ ] **Step 8: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Turn/RTTurnManager.cpp
git commit -m "feat(turnlog): colleziona TurnLog + NoLineOfSight + combat log arricchito (TL.3)"
```

---

## Self-Review (fatto in stesura)

- **Spec coverage**: FR-TURNLOG-01 (reason per enum) → Task 1 (move) + Task 2 (combat) + Task 3 (NoLineOfSight); FR-02 (permutazione) → Task 1 test OrderInvariant + `TurnLog.Sort`; FR-03 (no float) → enum interi, sort su interi; FR-04 (log HUD) → Task 2/3 `AddLogEvent`. ✅
- **Placeholder scan**: nessun TODO/TBD; codice reale in ogni step. Il refuso `using URTCombatLibrary;` nel test e' segnalato e corretto inline.
- **Type consistency**: `ERTMoveOutcome`/`ERTCombatOutcome`/`FRTTurnLogEntry`/`ClassifyCombatOutcome(HealthBefore,HealthAfter,AttackerDmgBonus)` coerenti fra Task 1→2→3.
- **Scope**: singolo slice (TL.1–TL.3), Movimento + Combat; hash/hazard/status fuori.
