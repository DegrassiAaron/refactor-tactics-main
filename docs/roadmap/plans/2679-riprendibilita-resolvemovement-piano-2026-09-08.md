# Riprendibilità di `ResolveMovement` — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rendere la risoluzione del movimento sospendibile fra un micro-step e l'altro, spostando il suo stato dallo stack di `ResolveMovement` a un contesto membro — **senza cambiare alcun comportamento osservabile**.

**Architecture:** Il livello difficile esiste già. `FRTMovementResolutionState` (`Turn/RTHexSim.h:201`) è dichiarata *«stato di una risoluzione di movimento **SOSPENDIBILE** fra un microstep e l'altro (CP 14.2)»*, con gli input **copiati** apposta perché il chiamante può *«aver aperto una finestra di reazione durata un turno di orologio»*. Ciò che manca è solo che `ARTTurnManager` smetta di tenerla sullo stack. Si estrae un `FRTMovementResolutionContext` che raccoglie lo `State` più le undici locali che il ciclo attraversa, lo si appende al manager, e si spezza `ResolveMovement` in `Begin` / `Advance` / `Finish`. `ResolveMovement` resta, come composizione delle tre: è il gate che tiene il comportamento invariante.

**Tech Stack:** Unreal Engine **5.8** (`D:\EpicGames\UE_5.8`), C++17, Automation Testing framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`).

**Spec:** [#2679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2679) · decisione governante [`D-355`](../../decisions/RT_PDR_00_Decision_Log.md) · referto [`2679-finestra-interlacciata-spec-panel-2026-09-08.md`](2679-finestra-interlacciata-spec-panel-2026-09-08.md)

## Scope: questo è il **primo di tre** piani

#2679 copre tre sottosistemi indipendenti. Ciascuno produce software funzionante e verificabile da solo:

| | Piano | Stato |
|---|---|---|
| **1** | **Riprendibilità della resolution** — questo documento | 🔄 in corso |
| **2** | La finestra come oggetto con ciclo di vita (apertura, attesa, chiusura) | ⏳ da scrivere |
| **3** | Interlacciamento con il playback (`LockInAndResolve` ↔ `TickPlayback`) — `D-355` | ⏳ da scrivere |

⛔ **Questo piano non apre nessuna finestra e non sospende nulla.** Costruisce la *capacità* di sospendere e la dimostra con un test; il punto di sospensione arriva col piano 2. È deliberato: un refactor a comportamento invariante è verificabile dalla suite esistente, una sospensione vera no.

## Global Constraints

Copiati dalla spec e da `CLAUDE.md`. Valgono per **ogni** task di questo piano.

- **Comportamento invariante.** Nessun task di questo piano può cambiare un esito, un TurnLog, un hash o un ordine. Il gate è la suite esistente: **`RefactorTactics` deve restare verde a ogni commit**.
- **Determinismo** (`CLAUDE.md` §7): nessuna dipendenza da frame rate, ordine di iterazione di `TMap`/`TSet`, indirizzi di puntatore, wall-clock. Il contesto va indicizzato per **identità**, mai per ordine di comparsa.
- **Il `Tick` non decide sequencing.** Può essere l'orologio della presentazione; non l'arbitro dell'ordine di risoluzione.
- **`AskReactionDecision` resta `const`** e resta sincrona in questo piano. La sua firma non si tocca.
- **Nessuna finestra si apre in ri-simulazione**: `RecordedDecisions.Num() > 0` continua a prendere il ramo della traccia prima di qualunque attesa (`Turn/RTTurnManager.cpp:6557`).
- **Build**: `D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactics-dev\RefactorTactics.uproject" -WaitMutex` — incrementale ~80 s.
- **Prima di ogni build**: verificare che nessun altro stia usando Unreal (`Get-Process UnrealEditor*,UnrealEditor-Cmd*`). Il mutex Live Coding è **globale sull'eseguibile del motore**: un editor aperto su un altro checkout blocca la build qui.
- **Commit frequenti**, uno per task, con il gate eseguito prima.

---

## File Structure

| File | Responsabilità | Azione |
|---|---|---|
| `Source/RefactorTactics/Turn/RTMovementResolutionContext.h` | La struct del contesto sospendibile e nient'altro. Nessuna logica. | **Create** |
| `Source/RefactorTactics/Turn/RTTurnManager.h` | Dichiarazione del membro `PendingMovement` e delle tre funzioni | **Modify** |
| `Source/RefactorTactics/Turn/RTTurnManager.cpp:7137-7660` | `ResolveMovement` spezzata in `Begin`/`Advance`/`Finish` | **Modify** |
| `Source/RefactorTactics/Tests/RTMovementResumeTests.cpp` | I due test di questo piano: caratterizzazione ed equivalenza | **Create** |

**Perché un header nuovo e non un campo in più su `RTTurnManager.h`**: `#1818` misura quel file a 10.817 righe e dodici responsabilità, e la DoD di #2679 vincola la crescita a **11.000 righe**. Una struct di stato con undici campi e nessuna logica è esattamente ciò che si tiene fuori: sta accanto a `RTHexSim.h`, che ospita già `FRTMovementResolutionState` per la stessa ragione.

---

### Task 1: La rete di caratterizzazione

Prima di spostare una riga, si fissa il comportamento attuale. È un refactor: il test che conta è quello che **fallirebbe se il refactor cambiasse un esito**, e deve esistere **prima**.

**Files:**
- Create: `Source/RefactorTactics/Tests/RTMovementResumeTests.cpp`

**Interfaces:**
- Consumes: `ARTTurnManager::LockInAndResolve()`, `ARTTurnManager::GetTurnLog()`, `URTTurnLogLibrary`
- Produces: gli helper `MakeResumeWorld`, `SpawnResumeUnit`, `DestroyResumeWorld`, usati da Task 4

- [ ] **Step 1: Scrivere il test di caratterizzazione**

Uno scenario con un Overwatch armato che scatta durante il movimento — cioè un turno che **attraversa** `ResolveReactionBoundary`, che è il punto che il refactor tocca.

```cpp
// Source/RefactorTactics/Tests/RTMovementResumeTests.cpp
#include "Misc/AutomationTest.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Unit/RTUnit.h"
#include "Tests/RTAbilityFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTMovementResume
{
    /** Il mondo di prova, con la mappa esagonale. Stesso pattern di `RTReactionTests.cpp`. */
    UWorld* MakeResumeWorld();
    ARTUnit* SpawnResumeUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell);
    void DestroyResumeWorld(UWorld* World);

    /**
     * L'impronta di un turno: cio' che il refactor NON deve cambiare.
     *
     * Non e' l'intero TurnLog — sarebbe fragile a ogni voce nuova — ma la terna che dichiara l'esito
     * del movimento: dove sono finite le unita', quante voci ha prodotto il log, e il digest delle
     * voci di movimento in ordine.
     */
    struct FTurnFingerprint
    {
        TArray<FRTCellId> FinalCells;
        int32 LogEntryCount = 0;
        FString MoveDigest;

        bool operator==(const FTurnFingerprint& O) const
        {
            return FinalCells == O.FinalCells
                && LogEntryCount == O.LogEntryCount
                && MoveDigest == O.MoveDigest;
        }
    };

    FTurnFingerprint Capture(ARTTurnManager* TM, const TArray<ARTUnit*>& Units)
    {
        FTurnFingerprint F;
        for (const ARTUnit* U : Units)
        {
            F.FinalCells.Add(U->Cell);
        }
        const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
        F.LogEntryCount = Log.Num();
        for (const FRTTurnLogEntry& E : Log)
        {
            if (E.Phase == ERTMatchPhase::Move)
            {
                F.MoveDigest += FString::Printf(TEXT("%d:%d,%d,%d;"),
                    E.UnitId, E.TgtCell.X, E.TgtCell.Y, E.TgtCell.Layer);
            }
        }
        return F;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementCharacterizationTest,
    "RefactorTactics.Movement.ResolveMovementFingerprintIsStable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementCharacterizationTest::RunTest(const FString&)
{
    using namespace RTMovementResume;

    UWorld* World = MakeResumeWorld();
    if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

    // Un mover che attraversa la zona, un watcher armato che la controlla: il turno passa
    // per `ResolveReactionBoundary`, che e' il punto che il refactor tocca.
    ARTUnit* Mover = SpawnResumeUnit(World, /*TeamId*/ 0, FRTCellId(0, 0));
    ARTUnit* Watcher = SpawnResumeUnit(World, /*TeamId*/ 1, FRTCellId(3, 0));
    ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
    if (!TestNotNull(TEXT("Mover"), Mover) || !TestNotNull(TEXT("Watcher"), Watcher)
        || !TestNotNull(TEXT("TM"), TM))
    {
        DestroyResumeWorld(World);
        return false;
    }

    Watcher->bIsBotControlled = true; // il decisore sincrono resta quello del bot
    const int32 OverwatchIdx = RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
    Watcher->PlannedReactionAbility = OverwatchIdx;
    Mover->PlannedCell = FRTCellId(4, 0); // entra nella zona controllata

    TM->LockInAndResolve();

    const FTurnFingerprint Fingerprint = Capture(TM, { Mover, Watcher });

    // L'impronta e' non vuota: un test che confronta due nulla passerebbe sempre.
    TestTrue(TEXT("il turno ha prodotto voci di log"), Fingerprint.LogEntryCount > 0);
    TestTrue(TEXT("il movimento ha lasciato traccia"), !Fingerprint.MoveDigest.IsEmpty());
    TestNotEqual(TEXT("il mover si e' mosso"), Fingerprint.FinalCells[0], FRTCellId(0, 0));

    DestroyResumeWorld(World);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 2: Verificare che nessuno stia usando Unreal, poi compilare**

```powershell
Get-Process UnrealEditor*,UnrealEditor-Cmd* -ErrorAction SilentlyContinue
```

Se non elenca nulla, compilare:

```
D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat RefactorTacticsEditor Win64 Development -Project="D:\Repositories\refactor-tactics-dev\RefactorTactics.uproject" -WaitMutex
```

Atteso: **compila**. Se `Unable to build while Live Coding is active` con nessun editor aperto, il mutex è di un processo zombie: aggiungere `-NoHotReloadFromIDE`.

- [ ] **Step 3: Eseguire il test e verificare che PASSA**

```
D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe "D:\Repositories\refactor-tactics-dev\RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.Movement.ResolveMovementFingerprintIsStable;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```

Atteso: **PASS**. Questo test non è "red": è una **rete**, e cattura il comportamento che esiste già. Contare `Result={Success}` nel log più recente sotto `Saved/Logs/`.

⚠️ Se fallisce, il difetto è nel test (helper, fixture, id dell'abilità), non nel codice di produzione: va corretto prima di proseguire. Un refactor senza rete non si fa.

- [ ] **Step 4: Commit**

```bash
git add Source/RefactorTactics/Tests/RTMovementResumeTests.cpp
git commit -m "test(2679): la rete di caratterizzazione, prima di spostare una riga"
```

---

### Task 2: Il contesto, popolato ma non ancora usato

Si crea la struct e la si riempie, lasciando `ResolveMovement` intatta. È un passo che non cambia nulla e isola il rischio: se la suite resta verde, il contesto contiene ciò che serve.

**Files:**
- Create: `Source/RefactorTactics/Turn/RTMovementResolutionContext.h`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h`

**Interfaces:**
- Consumes: `FRTMovementResolutionState` (`Turn/RTHexSim.h:201`), `FRTHexSnapshot`
- Produces: `FRTMovementResolutionContext`, e il membro `TUniquePtr<FRTMovementResolutionContext> ARTTurnManager::PendingMovement` — su cui Task 3 e Task 4 si appoggiano

- [ ] **Step 1: Creare l'header del contesto**

I campi sono **esattamente** le locali che il ciclo dei micro-step attraversa e che il codice dopo il ciclo rilegge — misurate su `RTTurnManager.cpp:7137-7660`, non indovinate.

```cpp
// Source/RefactorTactics/Turn/RTMovementResolutionContext.h
#pragma once

#include "CoreMinimal.h"
#include "Turn/RTHexSim.h" // FRTMovementResolutionState e FRTHexSnapshot: stanno entrambe qui

class ARTUnit;

/**
 * Il contesto di una risoluzione di movimento che puo' SOSPENDERSI fra due micro-step (`#2679`, [D-355]).
 *
 * 🔑 **Non introduce la sospendibilita': la RENDE RAGGIUNGIBILE.** `FRTMovementResolutionState`
 * (`Turn/RTHexSim.h:201`) e' sospendibile dal CP 14.2 e lo dichiara — *«gli input sono COPIATI, non
 * referenziati … un resolver che restituisce il controllo non puo' dipendere dalla vita di variabili
 * locali del chiamante»*. Cio' che mancava era solo che `ARTTurnManager::ResolveMovement` smettesse di
 * tenerla sul proprio stack, insieme alle altre locali che il ciclo attraversa.
 *
 * ⚠️ **`Units` sono TWeakObjectPtr e non puntatori nudi.** Fra due micro-step passa, in prospettiva, una
 * finestra di reazione: un'unita' distrutta nel frattempo lascerebbe un dangling che il compilatore non
 * vede. Con il modello sincrono di oggi non puo' accadere; questa struct esiste per il modello in cui
 * puo'.
 *
 * ⛔ **Non e' una USTRUCT**, per la stessa ragione di `FRTMovementResolutionState`: `TArray<TArray<>>`
 * non e' esponibile a UPROPERTY, e questo e' stato interno di un calcolo — non un dato di gioco che
 * qualcuno debba ispezionare da Blueprint.
 */
struct FRTMovementResolutionContext
{
    /** Lo stato del resolver puro. Gia' sospendibile: e' il cuore che questa struct trasporta. */
    FRTMovementResolutionState State;

    /** Lo snapshot su cui il turno e' stato deciso. Rileggerlo dal mondo darebbe uno stato piu' recente. */
    FRTHexSnapshot Snapshot;

    /** Le unita' del turno, nell'ordine dello snapshot: gli indici di `State` sono indici QUI. */
    TArray<TWeakObjectPtr<ARTUnit>> Units;

    /** I percorsi come il turno li ha pianificati, per `BuildMoveLog`. */
    TArray<TArray<FRTCellId>> Paths;

    /** Chi e' stato accorciato dalla topologia: il resolver non puo' saperlo, questo ciclo si'. */
    TArray<bool> bStoppedByTopology;

    /** Chi ha dichiarato una destinazione e se l'e' vista negare perche' occupata (`#79`). */
    TArray<bool> bDeniedByOccupant;

    /** La destinazione richiesta e negata, per indice. Viaggia accanto al flag, mai dentro `Paths`. */
    TArray<FRTCellId> DeniedDestination;

    /** Quante celle ogni unita' aveva gia' percorso al micro-step precedente: distingue chi ha AVANZATO. */
    TArray<int32> EnteredBefore;

    /** Il contesto esagonale, congelato all'inizio: `GetHexContext` puo' cambiare fra due frame. */
    FVector Origin = FVector::ZeroVector;
    float HexSize = 0.f;
    float LayerHeight = 0.f;

    /** Vero fra `Begin` e `Finish`. Un contesto non attivo non ha campi da leggere. */
    bool bActive = false;
};
```

- [ ] **Step 2: Dichiarare il membro sul manager**

In `Turn/RTTurnManager.h`, accanto agli altri stati della risoluzione (cercare `ArmedOverwatches`, che è lo stato affine, e mettere il campo lì sotto):

```cpp
    /**
     * La risoluzione di movimento in corso, quando esiste (`#2679`, [D-355]).
     *
     * 🔑 **`TUniquePtr` e non un valore**: il contesto porta lo snapshot e i percorsi di ogni unita', e
     * un turno su due non ne ha nessuno. Tenerlo per valore lo farebbe pagare a ogni istanza del manager,
     * compresi quelli dei test che non muovono niente.
     *
     * ⛔ **Non e' un secondo stato canonico**: e' il MEZZO con cui la risoluzione attraversa piu' frame,
     * e vive solo fra `BeginMovementResolution` e `FinishMovementResolution`. Fuori da quella finestra
     * e' nullo, e leggerlo e' un difetto.
     */
    TUniquePtr<FRTMovementResolutionContext> PendingMovement;
```

E l'include in cima al file, accanto agli altri `#include "Turn/..."`:

```cpp
#include "Turn/RTMovementResolutionContext.h"
```

- [ ] **Step 3: Compilare**

Stesso comando del Task 1 Step 2. Atteso: **compila**. Nessun uso ancora: si sta solo verificando che l'header sia autosufficiente e che gli include non siano circolari.

⚠️ Se `RTHexSnapshot.h` non è il path corretto, trovarlo con `grep -rn "struct FRTHexSnapshot" Source/` e correggere l'include — non aggiungere una forward declaration: la struct è usata per valore.

- [ ] **Step 4: Eseguire la suite intera**

```
D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe "D:\Repositories\refactor-tactics-dev\RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```

Atteso: **stesso numero di `Result={Success}` di prima del task**, zero `Result={Fail}`. ~1156 test, ~3 minuti. Registrare il numero: è la baseline dei task successivi.

- [ ] **Step 5: Commit**

```bash
git add Source/RefactorTactics/Turn/RTMovementResolutionContext.h Source/RefactorTactics/Turn/RTTurnManager.h
git commit -m "feat(2679): il contesto sospendibile, con i campi misurati e non indovinati"
```

---

### Task 3: Lo split in `Begin` / `Advance` / `Finish`

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.cpp:7137-7660`
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h`

**Interfaces:**
- Consumes: `FRTMovementResolutionContext`, `PendingMovement` (Task 2)
- Produces:
  - `void ARTTurnManager::BeginMovementResolution()` — popola `PendingMovement`, fino a `BeginHexMovement` incluso
  - `void ARTTurnManager::AdvanceMovementResolution()` — gira il ciclo dei micro-step **fino in fondo** (il passo singolo arriva col Task 4)
  - `void ARTTurnManager::FinishMovementResolution()` — da `FinishHexMovement` alla fine, e rilascia `PendingMovement`
  - `void ARTTurnManager::ResolveMovement()` — **resta**, come `Begin(); Advance(); Finish();`

- [ ] **Step 1: Dichiarare le tre funzioni in `RTTurnManager.h`**

Accanto alla dichiarazione esistente di `ResolveMovement`:

```cpp
    /**
     * La risoluzione del movimento, in tre momenti invece che in uno (`#2679`).
     *
     * 🔑 **`ResolveMovement` resta, e non per compatibilita'**: e' la composizione delle tre, ed e' il
     * GATE che tiene il comportamento invariante. Finche' esiste ed e' l'unico chiamante di produzione,
     * lo split non puo' aver cambiato un esito senza che la suite se ne accorga.
     *
     * ⛔ **Nessuna delle tre sospende ancora nulla.** `Advance` gira il ciclo fino in fondo, come il
     * `while` che sostituisce. Il punto di sospensione arriva col piano 2 di [#2679].
     */
    void BeginMovementResolution();
    void AdvanceMovementResolution();
    void FinishMovementResolution();
```

- [ ] **Step 2: Spezzare la funzione**

Il taglio è meccanico e i confini sono già misurati:

| Da → a | Va in |
|---|---|
| `7139` (`UWorld* World`) → `7302` (`BeginHexMovement`, incluso) | `BeginMovementResolution` |
| `7303` (`{ TArray<int32> EnteredBefore`) → `7320` (chiusura del `while`) | `AdvanceMovementResolution` |
| `7321` (`FinishHexMovement`) → `7660` (fine funzione) | `FinishMovementResolution` |

Le sostituzioni da fare mentre si taglia, in tutte e tre:

- ogni locale del contesto diventa `Ctx.<nome>` — `State` → `Ctx.State`, `Snapshot` → `Ctx.Snapshot`, `Paths` → `Ctx.Paths`, `bStoppedByTopology` → `Ctx.bStoppedByTopology`, `bDeniedByOccupant` → `Ctx.bDeniedByOccupant`, `DeniedDestination` → `Ctx.DeniedDestination`, `Origin` → `Ctx.Origin`, `HexSize` → `Ctx.HexSize`, `LayerH` → `Ctx.LayerHeight`;
- `Units` diventa `Ctx.Units`, che è `TArray<TWeakObjectPtr<ARTUnit>>`: dove il codice esistente passa `const TArray<ARTUnit*>&` (a `ResolveReactionBoundary`, `ResolvePredictiveBoundary`) si materializza una vista locale, **senza cambiare quelle firme**;
- `PlannedMoves` **resta locale a `Begin`**: è consumata da `BeginHexMovement` e mai riletta (misurato: zero usi dopo il ciclo).

`Begin` in testa e `Finish` in coda:

```cpp
void ARTTurnManager::BeginMovementResolution()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    PendingMovement = MakeUnique<FRTMovementResolutionContext>();
    FRTMovementResolutionContext& Ctx = *PendingMovement;
    Ctx.bActive = true;

    GetHexContext(Ctx.Origin, Ctx.HexSize, Ctx.LayerHeight);

    TArray<ARTUnit*> Units;
    Ctx.Snapshot = MakeCurrentSnapshot(Units);
    Ctx.Units.Reserve(Units.Num());
    for (ARTUnit* U : Units)
    {
        Ctx.Units.Add(U);
    }

    // ... il corpo esistente di 7139-7302, con le locali riscritte in `Ctx.<nome>` ...
    // L'ultima riga resta `BeginHexMovement`, che ora scrive nel contesto:
    Ctx.State = URTHexSimLibrary::BeginHexMovement(Ctx.Paths, TArray<int32>(),
        TArray<bool>(), TArray<bool>(), PlannedMoves);

    Ctx.EnteredBefore.Init(0, Ctx.Paths.Num());
}
```

```cpp
void ARTTurnManager::AdvanceMovementResolution()
{
    if (!PendingMovement.IsValid() || !PendingMovement->bActive)
    {
        return; // fail-closed: avanzare una risoluzione che non esiste non e' un no-op da inventare
    }
    FRTMovementResolutionContext& Ctx = *PendingMovement;

    // La vista con puntatori nudi che `ResolveReactionBoundary` richiede. Ricostruita a ogni chiamata e
    // non memorizzata: e' proprio la vita di questi puntatori che il contesto esiste per non assumere.
    TArray<ARTUnit*> Units;
    Units.Reserve(Ctx.Units.Num());
    for (const TWeakObjectPtr<ARTUnit>& U : Ctx.Units)
    {
        Units.Add(U.Get());
    }

    CurrentMicroStepIndex = Ctx.State.MicroStepIndex;
    ON_SCOPE_EXIT{ CurrentMicroStepIndex = INDEX_NONE; };

    while (URTHexSimLibrary::ResolveNextHexMicroStep(Ctx.State))
    {
        TArray<int32> MovedUnitIds;
        for (int32 i = 0; i < Ctx.State.Num(); ++i)
        {
            const int32 EnteredNow = Ctx.State.Results[i].Entered.Num();
            if (EnteredNow > Ctx.EnteredBefore[i])
            {
                MovedUnitIds.Add(i);
            }
            Ctx.EnteredBefore[i] = EnteredNow;
        }

        ResolveReactionBoundary(Ctx.Snapshot.Map, Units, Ctx.State, MovedUnitIds, CurrentMicroStepIndex);
        ++CurrentMicroStepIndex;
    }
}
```

⚠️ **`CurrentMicroStepIndex` si semina da `Ctx.State.MicroStepIndex` e non da `0`.** Oggi le due cose coincidono perché `Advance` gira una volta sola; col Task 4 non coincideranno più, e ripartire da zero rinumererebbe i boundary — cioè cambierebbe le chiavi di `FRTReactionOpportunityKey`, che è il difetto che rompe il replay.

```cpp
void ARTTurnManager::FinishMovementResolution()
{
    if (!PendingMovement.IsValid() || !PendingMovement->bActive)
    {
        return;
    }
    FRTMovementResolutionContext& Ctx = *PendingMovement;

    TArray<ARTUnit*> Units;
    Units.Reserve(Ctx.Units.Num());
    for (const TWeakObjectPtr<ARTUnit>& U : Ctx.Units)
    {
        Units.Add(U.Get());
    }

    TArray<FRTHexMoveResult> Resolved = URTHexSimLibrary::FinishHexMovement(Ctx.State);

    // ... il corpo esistente di 7322-7659, con le locali riscritte in `Ctx.<nome>` ...

    // Il contesto muore QUI e non prima: ogni riga sopra lo legge.
    PendingMovement.Reset();
}
```

```cpp
void ARTTurnManager::ResolveMovement()
{
    BeginMovementResolution();
    AdvanceMovementResolution();
    FinishMovementResolution();
}
```

- [ ] **Step 3: Compilare**

Stesso comando. Atteso: **compila**. Gli errori probabili e cosa significano:

| Errore | Causa | Rimedio |
|---|---|---|
| `no matching function ... TArray<ARTUnit*>` | una chiamata riceve `Ctx.Units` invece della vista | materializzare la vista locale, **non** cambiare la firma chiamata |
| `use of undeclared identifier 'PlannedMoves'` in `Finish` | `PlannedMoves` è locale a `Begin` | è corretto: quel punto non deve usarla — verificare cosa gli serve davvero |
| `'Origin' was not declared` | una riga sfuggita alla riscrittura | `Ctx.Origin` |

- [ ] **Step 4: Eseguire la suite intera e confrontare con la baseline del Task 2**

Stesso comando del Task 2 Step 4.

Atteso: **stesso numero di `Result={Success}`, zero `Result={Fail}`**. Un solo test rosso qui significa che lo split ha cambiato un comportamento: **non si prosegue** e si trova quale locale è stata persa. `Movement.ResolveMovementFingerprintIsStable` è quello che deve gridare per primo.

- [ ] **Step 5: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Turn/RTTurnManager.h
git commit -m "refactor(2679): ResolveMovement in tre momenti, comportamento invariante"
```

---

### Task 4: Un micro-step per chiamata

Ora `Advance` fa **un** passo e dice se ne restano. È la capacità che il piano esiste per costruire: chi chiama può fermarsi in mezzo.

**Files:**
- Modify: `Source/RefactorTactics/Turn/RTTurnManager.h`, `Source/RefactorTactics/Turn/RTTurnManager.cpp`
- Modify: `Source/RefactorTactics/Tests/RTMovementResumeTests.cpp`

**Interfaces:**
- Consumes: tutto il Task 3
- Produces: `enum class ERTMovementAdvanceResult : uint8 { Advanced, Finished };` e la nuova firma `ERTMovementAdvanceResult ARTTurnManager::AdvanceMovementResolution()` — su cui il **piano 2** aggiungerà il terzo valore `Suspended`

- [ ] **Step 1: Scrivere il test di equivalenza (questo è "red")**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementStepwiseMatchesWholeTest,
    "RefactorTactics.Movement.StepwiseResolutionMatchesWhole",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementStepwiseMatchesWholeTest::RunTest(const FString&)
{
    using namespace RTMovementResume;

    // --- Esecuzione A: il turno intero, come oggi ---------------------------------------------------
    UWorld* WorldA = MakeResumeWorld();
    if (!TestNotNull(TEXT("world A"), WorldA)) { return false; }
    ARTUnit* MoverA = SpawnResumeUnit(WorldA, 0, FRTCellId(0, 0));
    ARTUnit* WatcherA = SpawnResumeUnit(WorldA, 1, FRTCellId(3, 0));
    ARTTurnManager* TMA = WorldA->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
    WatcherA->bIsBotControlled = true;
    WatcherA->PlannedReactionAbility =
        RTAbilityFixtures::AddCoreAbilityInSlot(WatcherA, TEXT("Action.Overwatch"), 3);
    MoverA->PlannedCell = FRTCellId(4, 0);
    TMA->LockInAndResolve();
    const FTurnFingerprint Whole = Capture(TMA, { MoverA, WatcherA });
    DestroyResumeWorld(WorldA);

    // --- Esecuzione B: identica, ma un micro-step per chiamata ---------------------------------------
    UWorld* WorldB = MakeResumeWorld();
    if (!TestNotNull(TEXT("world B"), WorldB)) { return false; }
    ARTUnit* MoverB = SpawnResumeUnit(WorldB, 0, FRTCellId(0, 0));
    ARTUnit* WatcherB = SpawnResumeUnit(WorldB, 1, FRTCellId(3, 0));
    ARTTurnManager* TMB = WorldB->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
    WatcherB->bIsBotControlled = true;
    WatcherB->PlannedReactionAbility =
        RTAbilityFixtures::AddCoreAbilityInSlot(WatcherB, TEXT("Action.Overwatch"), 3);
    MoverB->PlannedCell = FRTCellId(4, 0);

    TMB->BeginMovementResolution();
    int32 Passi = 0;
    int32 Guard = 0;
    while (TMB->AdvanceMovementResolution() == ERTMovementAdvanceResult::Advanced && Guard++ < 256)
    {
        ++Passi;
    }
    TMB->FinishMovementResolution();
    const FTurnFingerprint Stepwise = Capture(TMB, { MoverB, WatcherB });
    DestroyResumeWorld(WorldB);

    // Il cuore: due strade, un esito.
    TestTrue(TEXT("un passo per volta produce la stessa impronta del turno intero"), Whole == Stepwise);

    // ⚠️ E il ciclo ha davvero girato piu' volte: senza questo, un `Advance` che facesse tutto in una
    // chiamata passerebbe il confronto qui sopra e il test direbbe di aver verificato il passo singolo.
    TestTrue(TEXT("i micro-step sono stati piu' di uno"), Passi > 1);
    TestTrue(TEXT("il ciclo e' terminato senza toccare la guardia"), Guard < 256);

    return true;
}
```

⚠️ **Il test confronta due mondi, non due esecuzioni nello stesso mondo.** Rieseguire il turno sullo stesso manager userebbe uno stato già mutato dalla prima esecuzione, e il confronto sarebbe fra un turno e il suo seguito.

⚠️ **Esecuzione B non chiama `LockInAndResolve`**: chiama le tre funzioni direttamente. Le fasi non-movimento non entrano nel confronto, e l'impronta le esclude filtrando su `ERTMatchPhase::Move`.

- [ ] **Step 2: Compilare, e verificare che FALLISCE**

Atteso: **errore di compilazione** — `ERTMovementAdvanceResult` non esiste, e `AdvanceMovementResolution` ritorna `void`. È il "red" di questo task.

- [ ] **Step 3: Cambiare `Advance` in passo singolo**

L'enum, in `RTTurnManager.h` sopra la classe:

```cpp
/**
 * L'esito di un passo di risoluzione del movimento (`#2679`).
 *
 * ➕ **Il piano 2 di [#2679] aggiunge `Suspended` IN CODA**, mai in mezzo: chi legge questo enum lo fa
 * con uno `switch` senza `default`, e un valore inserito prima farebbe cambiare significato ai
 * confronti gia' scritti.
 */
enum class ERTMovementAdvanceResult : uint8
{
    /** Un micro-step risolto, ne restano altri. */
    Advanced,
    /** Nessun micro-step da risolvere: la risoluzione e' pronta per `Finish`. */
    Finished,
};
```

E il corpo, che perde il `while`:

```cpp
ERTMovementAdvanceResult ARTTurnManager::AdvanceMovementResolution()
{
    if (!PendingMovement.IsValid() || !PendingMovement->bActive)
    {
        return ERTMovementAdvanceResult::Finished;
    }
    FRTMovementResolutionContext& Ctx = *PendingMovement;

    TArray<ARTUnit*> Units;
    Units.Reserve(Ctx.Units.Num());
    for (const TWeakObjectPtr<ARTUnit>& U : Ctx.Units)
    {
        Units.Add(U.Get());
    }

    // 🔑 **Si semina dallo STATO, non da un contatore di questa funzione.** Fra due chiamate il manager
    // puo' aver fatto altro, e un contatore locale ripartirebbe da zero: le chiavi delle finestre
    // (`FRTReactionOpportunityKey`) si rinumererebbero, ed e' il difetto che rompe il replay.
    CurrentMicroStepIndex = Ctx.State.MicroStepIndex;
    ON_SCOPE_EXIT{ CurrentMicroStepIndex = INDEX_NONE; };

    if (!URTHexSimLibrary::ResolveNextHexMicroStep(Ctx.State))
    {
        return ERTMovementAdvanceResult::Finished;
    }

    TArray<int32> MovedUnitIds;
    for (int32 i = 0; i < Ctx.State.Num(); ++i)
    {
        const int32 EnteredNow = Ctx.State.Results[i].Entered.Num();
        if (EnteredNow > Ctx.EnteredBefore[i])
        {
            MovedUnitIds.Add(i);
        }
        Ctx.EnteredBefore[i] = EnteredNow;
    }

    ResolveReactionBoundary(Ctx.Snapshot.Map, Units, Ctx.State, MovedUnitIds, CurrentMicroStepIndex);

    return ERTMovementAdvanceResult::Advanced;
}
```

E `ResolveMovement` acquista il ciclo che `Advance` ha perso:

```cpp
void ARTTurnManager::ResolveMovement()
{
    BeginMovementResolution();

    // La guardia non e' difensiva: e' il cap che impedisce a un difetto del resolver di appendere
    // l'Editor invece di far fallire un test. `MicroStepsInPath` non supera la lunghezza del percorso
    // piu' lungo, e 256 e' due ordini di grandezza sopra una mappa 2v2.
    int32 Guard = 0;
    while (AdvanceMovementResolution() == ERTMovementAdvanceResult::Advanced && Guard++ < 256)
    {
    }
    ensureMsgf(Guard < 256, TEXT("risoluzione del movimento non terminata in 256 micro-step"));

    FinishMovementResolution();
}
```

- [ ] **Step 4: Compilare ed eseguire il test nuovo**

```
D:/EpicGames/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe "D:\Repositories\refactor-tactics-dev\RefactorTactics.uproject" -ExecCmds="Automation RunTests RefactorTactics.Movement;Quit" -unattended -nopause -nosplash -nullrhi -NoLiveCoding -log
```

Atteso: **PASS** su `StepwiseResolutionMatchesWhole`, `ResolveMovementFingerprintIsStable` e `StepperMatchesBatchResolver`.

- [ ] **Step 5: Eseguire la suite intera**

Atteso: **stesso numero di `Result={Success}` della baseline**, più i due test nuovi. Zero `Result={Fail}`.

⚠️ I test da guardare per primi se qualcosa si rompe, perché sono quelli che attraversano il boundary: `Overwatch.DecisionIsReplayable`, `Overwatch.OrderIsDeterministic`, `Overwatch.TimeoutIsHold`, `Replay.Verifier.ResimulationIsDeterministic`.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Turn/RTTurnManager.h Source/RefactorTactics/Tests/RTMovementResumeTests.cpp
git commit -m "feat(2679): un micro-step per chiamata, e il turno intero resta identico"
```

---

### Task 5: Il bound su `#1818`, e la chiusura del piano

La DoD di #2679 vincola `RTTurnManager.cpp` + `.h` a **11.000 righe**. Questo piano vi aggiunge tre firme e ne toglie un corpo: va misurato, non sperato.

**Files:**
- Modify: nessuno, salvo quanto la misura imponga

- [ ] **Step 1: Misurare**

```bash
wc -l Source/RefactorTactics/Turn/RTTurnManager.cpp Source/RefactorTactics/Turn/RTTurnManager.h
```

Baseline su `a83ea7d9`: **10.817** (8.427 + 2.390). Soglia: **11.000**.

- [ ] **Step 2: Se la misura supera 11.000, spostare `Finish` in un file affine**

`RTTurnManager_Blast.cpp` è il precedente: il progetto già divide questa classe per fase. Un `RTTurnManager_Movement.cpp` che accoglie le tre funzioni è la mossa coerente, e va fatta **qui** — non rimandata al piano 2, che la troverebbe come debito invece che come scelta.

⚠️ Aggiungere il file nuovo non richiede modifiche a `RefactorTactics.Build.cs`: Unreal compila per cartella.

- [ ] **Step 3: Aggiornare la casella nella DoD di #2679**

```bash
gh issue view 2679 --json body -q '.body' > /tmp/2679.md
# spuntare: "- [x] La scelta fra «dentro ARTTurnManager» e «estratta» e' dichiarata e vincolata"
# e annotarvi la misura reale
gh issue edit 2679 --body-file /tmp/2679.md
```

- [ ] **Step 4: Commit e PR**

```bash
git push -u origin feat/2679-riprendibilita-resolvemovement
gh pr create --base main \n  --title "feat(2679): la risoluzione del movimento diventa riprendibile" \n  --body "Fetta 1 di 3 di #2679, governata da D-355. Sposta lo stato della risoluzione del movimento dallo stack di ResolveMovement a un contesto membro, e spezza la funzione in Begin/Advance/Finish. Comportamento invariante: ResolveMovement resta come composizione delle tre, ed e' il gate. Nessuna finestra si apre e Advance non ritorna mai Suspended: sono le fette 2 e 3. Gate: suite RefactorTactics verde, piu' Movement.ResolveMovementFingerprintIsStable e Movement.StepwiseResolutionMatchesWhole."
```

---

## Cosa questo piano NON consegna

Dichiarato perché nessuno lo scopra leggendo la DoD di #2679 e trovandola a metà:

- ⛔ **Nessuna finestra si apre**, e `MakeReactionWindowView` resta senza chiamanti di produzione. È il piano 2.
- ⛔ **`Advance` non ritorna mai `Suspended`**: il valore non esiste ancora nell'enum. È il piano 2.
- ⛔ **Il sito del `Brace` (`RTTurnManager_Blast.cpp:2264`) non e' toccato.** La DoD di #2679 chiede che sospendano **entrambi** i siti di `AskReactionDecision`; questo piano copre il movimento. Il `Brace` vive in `ResolveCombat`, ha un ciclo diverso, e merita la propria fetta invece di essere infilato qui.
- ⛔ **`LockInAndResolve` resta monolitica**: sospendere il movimento senza sospendere il ciclo delle fasi lascerebbe proseguire le fasi successive. È il piano 3, ed è la ragione per cui il piano 2 da solo non basta a far vedere una finestra al giocatore.
- ⛔ **I tre commenti normativi** (`RTTurnManager.h:552`, `:627`, `.cpp:6622`) restano falsi: diventano correggibili quando il comportamento che descrivono cambia davvero, cioè col piano 3.
