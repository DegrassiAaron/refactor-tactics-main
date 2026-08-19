# CP 6.1 — Allestimento della partita su mappa hex · Piano di implementazione

> ## 📦 `DELIVERED PLAN` — PIANO GIA' ESEGUITO, NON NORMATIVO
>
> Il piano di esecuzione, gia' eseguito. **Il corpo qui sotto non va aggiornato**: comandi, nomi di branch e percorsi sono quelli di allora, e
> correggerli falsificherebbe la storia invece di renderla utile.
>
> Stato corrente: [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) · indice dei documenti correnti:
> [`../../README.md`](../../README.md). Banner aggiunto il 2026-08-08.

> **Per chi esegue**: i passi usano checkbox (`- [ ]`) per il tracciamento. Ogni task si chiude con build +
> suite verde e un commit. Non passare al task successivo con la suite rossa.

**Obiettivo**: `ARTGameMode` allestisce la partita da un `ARTHexMapActor` + `URTHexMapAsset`, e `ARTUnit`
porta la posizione autorevole in `FRTCellId` — sostituzione di `FRTGridCoord`, non un campo parallelo.

**Architettura**: `FRTCellId` diventa **l'unico** tipo di coordinata del progetto; `FRTGridCoord` viene
eliminato da `Core/RTTypes.h` in un'unica PR (decisione dell'utente del 2026-08-05, vedi § *Decisione e
rischio accettato*). Le librerie hex già esistenti e testate (`URTHexLibrary`, `URTHexPathLibrary`,
`URTHexVisionLibrary`, `URTHexSimLibrary`) non vengono riscritte: CP 6.1 collega ad esse l'allestimento e la
posizione delle unità. La **matematica** dei consumatori quadrati (distanza Manhattan, vicini a 4, LOS
quadrata) resta invariata in questo checkpoint e migra nei CP 6.2–6.4: qui cambia il *tipo*, non le *regole*.

**Stack**: Unreal Engine 5.8.1, C++ autoritativo, Unreal Automation Framework
(`IMPLEMENT_SIMPLE_AUTOMATION_TEST`), Blueprint solo per presentazione.

## Vincoli globali

- Prefissi `RT`/`URT`; risposte e commenti in italiano, identificatori in inglese.
- Invariante #2: la posizione autorevole è la griglia logica; il `FVector` serve solo al rendering.
- Invariante #4 (determinismo): nessun float nelle coordinate o negli hash; nessuna dipendenza dall'ordine
  di container non ordinati; ordine stabile `(Layer, X, Y)` dove serve un ordinamento.
- Invariante #1: la presentazione non decide nulla — `PlaceOnCell`/`WorldForCell` restano derivate.
- Suite di riferimento all'inizio: **172 test, 0 fail** (misurata 2026-08-05 su `main` a `f6adf11`).
- Build: `"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat" RefactorTacticsEditor Win64 Development -project="D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" -waitmutex`
- Test: `"D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Repositories\refactor-tactics-main\RefactorTactics.uproject" "-ExecCmds=Automation RunTests RefactorTactics; Quit" -nullrhi -unattended -nopause -nosplash -log -abslog=<log>`
  → contare `Result={Success}` e `Result={Fail}` nel log. **L'editor UE deve essere chiuso** (Live Coding
  blocca il target Editor).

## Decisione e rischio accettato

La roadmap (M6, rischio *a*) raccomanda di sostituire la coordinata **a fette compilabili**. L'utente ha
scelto invece la **sostituzione totale in una PR**, valutando che il ponte di conversione temporaneo costi
più di quanto protegga. Conseguenza operativa: il Task 1 è grande e atomico — o compila tutto, o non compila
niente. Mitigazione adottata in questo piano:

1. Il Task 1 è **puramente meccanico** (cambio di tipo), nessuna modifica di comportamento: la suite esistente
   è la rete di sicurezza, e deve restare a 172/0.
2. Prima del Task 1 si crea un **tag di ritorno** (`pre-cellid-swap`), così il rollback è un comando solo.
3. I task 2–5 sono incrementali e indipendenti fra loro una volta chiuso il Task 1.

---

## Struttura dei file

| File | Responsabilità dopo CP 6.1 |
|---|---|
| `Core/RTTypes.h` | `FRTTraversalEdge` su `FRTCellId`; **`FRTGridCoord` rimosso** |
| `Map/RTCellId.h` | unico tipo di coordinata del progetto (invariato) |
| `Unit/RTUnit.h/.cpp` | posizione autorevole `FRTCellId Cell`; `PlaceOnCell`/`WorldForCell` su geometria esagonale |
| `RTGameMode.h/.cpp` | allestisce da `ARTHexMapActor` + `URTHexMapAsset`; niente `ARTGridActor` nel flusso di partita |
| `Turn/RTMatchSetupLibrary.h/.cpp` | **nuovo**: funzioni pure di allestimento (celle di partenza, occupazione) — testabili headless |
| `Grid/RTGridLibrary.*`, `Turn/RTMovementResolver.*`, `Bot/RTBotLibrary.*`, `Combat/RTCombatLibrary.*`, `UI/RTHUD.cpp`, `Player/RTPlayerController.cpp`, `Turn/RTTurnManager.*`, `Turn/RTTurnLog.h`, `Turn/RTResolvedEvent.h` | firme migrate a `FRTCellId`, **semantica quadrata invariata** (migra in 6.2–6.4) |
| `Tests/*` | stessi casi, tipo aggiornato |

Estensione misurata: **35 file** contengono `FRTGridCoord`, di cui 11 di test.

---

## Task 1: `FRTCellId` diventa l'unica coordinata

**Files:**
- Modify: `Source/RefactorTactics/Core/RTTypes.h` (rimuove `FRTGridCoord`, porta `FRTTraversalEdge` su `FRTCellId`)
- Modify: gli altri 34 file elencati da `git grep -l FRTGridCoord -- Source`
- Test: nessun test nuovo — la suite esistente (172) è il criterio

**Interfaces:**
- Consuma: `FRTCellId` da `Map/RTCellId.h` (già esistente, con `GetTypeHash`, `operator==`, `ToString()`)
- Produce: ogni firma che prendeva `const FRTGridCoord&` prende `const FRTCellId&`; ogni `TArray<FRTGridCoord>`
  diventa `TArray<FRTCellId>`; `TMap<FRTGridCoord,int32>` diventa `TMap<FRTCellId,int32>`.
  `URTHexSimLibrary::ToLogCoord` sparisce (era la conversione verso il tipo di log): il TurnLog usa già
  direttamente `FRTCellId`.
- **Rename del campo** su `ARTUnit`: `FRTGridCoord GridCell` → `FRTCellId Cell`. Il nome allinea l'unità a
  `FRTHexSimUnit::Cell` e rende evidente in ogni call-site che la coordinata è cambiata (un rename di tipo
  silenzioso lascerebbe compilare codice che *pensa* ancora in quadrato). Gli altri campi mantengono il nome
  e cambiano solo tipo: `PlannedCell`, `PlannedPath`, `PlannedWaypoints`, `PlannedDashCell`.

- [ ] **Step 1: Tag di ritorno prima di toccare il codice**

```bash
git tag -a pre-cellid-swap -m "Ultimo commit con FRTGridCoord ancora presente (prima di CP 6.1)"
```

- [ ] **Step 2: Verificare il punto di partenza (build + suite)**

Build, poi suite. Atteso: `Result: Succeeded`, `Success: 172`, `Fail: 0`. Se qui è già rosso, fermarsi: non
si migra sopra una base rotta.

- [ ] **Step 3: Portare `Core/RTTypes.h` sul nuovo tipo**

`FRTGridCoord` viene eliminato; `FRTTraversalEdge` passa a `FRTCellId`. `RTTypes.h` include `Map/RTCellId.h`.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTTypes.generated.h"

/**
 * Arco di traversata esplicito tra due celle (anche non adiacenti / su layer diversi):
 * scale, rampe, portali, ascensori, salti. Direzionale (per il bidirezionale servono due archi).
 */
USTRUCT(BlueprintType)
struct FRTTraversalEdge
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
    FRTCellId From;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
    FRTCellId To;

    /** Costo di attraversamento dell'arco (>= 0). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Grid")
    int32 Cost = 1;

    FRTTraversalEdge() = default;
    FRTTraversalEdge(const FRTCellId& InFrom, const FRTCellId& InTo, int32 InCost)
        : From(InFrom), To(InTo), Cost(InCost) {}
};
```

- [ ] **Step 4: Sostituzione meccanica nei restanti 34 file**

Sostituire `FRTGridCoord` → `FRTCellId` in tutto `Source/`. Attenzione ai tre punti dove la sostituzione
testuale **non** basta:

1. **Costruttori a 2 argomenti**: `FRTGridCoord(2, 4)` → `FRTCellId(2, 4)` compila (il terzo parametro ha
   default `0`), ma `FRTGridCoord(X, Y)` senza layer significava «piano implicito 0» mentre in `FRTCellId`
   il layer è esplicito. Nessuna modifica necessaria, ma non convertire i letterali a caso: mantenere gli
   stessi valori numerici, così i test esistenti restano confrontabili.
2. **`URTHexSimLibrary::ToLogCoord`** (`Turn/RTHexSimLibrary.h:77`, `.cpp`): esisteva solo per convertire
   `FRTCellId` → tipo di log. Rimuoverla e aggiornare i chiamanti perché passino la cella direttamente.
3. **`Turn/RTTurnLog.h`**: le entry del log cambiano tipo di campo ma **non** il layout binario (3 × `int32`
   in entrambi i casi). Il test `RefactorTactics.TurnLog.SquareBytesUnchanged` deve restare verde: se
   fallisce, la serializzazione è cambiata davvero e va indagata prima di proseguire (non «aggiustare» il test).

- [ ] **Step 5: Documentare il debito semantico dove è nascosto**

`URTGridLibrary` continua a fare matematica **quadrata** (Manhattan, vicini a 4) su un tipo che ora è
**assiale**: finché il flusso di partita non passa alle librerie hex (CP 6.2–6.4) questo è corretto ma
insidioso. Aggiungere in testa a `Grid/RTGridLibrary.h`:

```cpp
/**
 * ATTENZIONE (CP 6.1): questa libreria applica geometria QUADRATA (distanza di Manhattan, 4 vicini) a
 * coordinate ora di tipo FRTCellId, che è ASSIALE. Finche' il flusso di partita non e' migrato a
 * URTHexLibrary/URTHexPathLibrary (CP 6.2-6.4) le due cose convivono: qui e' cambiato il tipo, non le regole.
 * Non usare queste funzioni per logica esagonale nuova — usare le librerie hex. Rimozione pianificata in M7.
 */
```

- [ ] **Step 6: Build**

Atteso: `Result: Succeeded`, zero warning nuovi. Gli errori attesi in questo passo sono di tipo
(`cannot convert`), non di logica: risolverli cambiando il tipo, mai inserendo conversioni silenziose.

- [ ] **Step 7: Suite**

Atteso: `Success: 172`, `Fail: 0`. Un test rosso qui significa che la sostituzione ha cambiato
comportamento: indagare la causa, non adattare l'asserzione.

- [ ] **Step 8: Verificare che il tipo sia davvero sparito**

```bash
git grep -c "FRTGridCoord" -- Source | wc -l
```

Atteso: `0`.

- [ ] **Step 9: Commit**

```bash
git add -A Source
git commit -m "refactor(coord): FRTCellId diventa l'unica coordinata del progetto"
```

---

## Task 2: Libreria pura di allestimento

Il `GameMode` non deve contenere logica non testabile: le decisioni di allestimento (quali celle occupano le
4 unità, come si ricostruisce l'occupazione) vivono in una libreria pura, verificabile headless.

**Files:**
- Create: `Source/RefactorTactics/Turn/RTMatchSetupLibrary.h`
- Create: `Source/RefactorTactics/Turn/RTMatchSetupLibrary.cpp`
- Test: `Source/RefactorTactics/Tests/RTMatchSetupTests.cpp`

**Interfaces:**
- Consuma: `URTHexMapAsset::CellsInLayer`, `URTHexMapAsset::FindCell`, `FRTHexCellData::bBlocksMovement`,
  `URTHexLibrary::StableLess`
- Produce:
  - `static TArray<FRTCellId> URTMatchSetupLibrary::PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer)`
    — ritorna `2 * NumPerTeam` celle: le prime `NumPerTeam` per il team 0, le successive per il team 1.
    Vuoto se la mappa è nulla o non ha abbastanza celle libere.
  - `static TMap<FRTCellId, int32> URTMatchSetupLibrary::BuildOccupancy(const TArray<FRTCellId>& Cells, const TArray<int32>& UnitIds, const TArray<bool>& Alive)`
    — occupazione ricostruita dallo stato delle unità (solo vive). Array di lunghezza diversa → mappa vuota.

- [ ] **Step 1: Scrivere i test che falliscono**

```cpp
#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"

namespace
{
    /** Mappa di prova: esagono di celle sul layer 0, con eventuali celle bloccanti. */
    URTHexMapAsset* MakeTestMap(const TArray<FRTCellId>& Cells, const TArray<FRTCellId>& Blocked)
    {
        URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
        for (const FRTCellId& Id : Cells)
        {
            FRTHexCellData Data;
            Data.Id = Id;
            Data.bBlocksMovement = Blocked.Contains(Id);
            Map->AddOrUpdateCell(Data);
        }
        return Map;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupPickStartCellsTest,
    "RefactorTactics.MatchSetup.PickStartCells",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTMatchSetupPickStartCellsTest::RunTest(const FString&)
{
    URTHexMapAsset* Map = MakeTestMap({ {0,0}, {1,0}, {2,0}, {3,0}, {4,0} }, {});

    const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, 2, 0);
    TestEqual(TEXT("quattro celle per un 2v2"), Start.Num(), 4);
    TestTrue(TEXT("nessuna cella ripetuta"), TSet<FRTCellId>(Start).Num() == 4);

    // Determinismo: la stessa mappa produce sempre lo stesso allestimento.
    const TArray<FRTCellId> Again = URTMatchSetupLibrary::PickStartCells(Map, 2, 0);
    TestTrue(TEXT("allestimento deterministico"), Start == Again);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupSkipsBlockedTest,
    "RefactorTactics.MatchSetup.SkipsBlockedCells",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTMatchSetupSkipsBlockedTest::RunTest(const FString&)
{
    URTHexMapAsset* Map = MakeTestMap({ {0,0}, {1,0}, {2,0}, {3,0}, {4,0} }, { {1,0}, {3,0} });

    const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, 1, 0);
    TestEqual(TEXT("due celle per un 1v1"), Start.Num(), 2);
    TestFalse(TEXT("nessuna cella bloccante"), Start.Contains(FRTCellId(1, 0)));
    TestFalse(TEXT("nessuna cella bloccante"), Start.Contains(FRTCellId(3, 0)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupInsufficientCellsTest,
    "RefactorTactics.MatchSetup.InsufficientCells",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTMatchSetupInsufficientCellsTest::RunTest(const FString&)
{
    URTHexMapAsset* Map = MakeTestMap({ {0,0}, {1,0} }, {});
    TestEqual(TEXT("mappa troppo piccola -> nessun allestimento"),
        URTMatchSetupLibrary::PickStartCells(Map, 2, 0).Num(), 0);
    TestEqual(TEXT("mappa nulla -> nessun allestimento"),
        URTMatchSetupLibrary::PickStartCells(nullptr, 2, 0).Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchSetupOccupancyTest,
    "RefactorTactics.MatchSetup.BuildOccupancy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTMatchSetupOccupancyTest::RunTest(const FString&)
{
    const TArray<FRTCellId> Cells { {0,0}, {1,0}, {2,0} };
    const TArray<int32> Ids { 10, 11, 12 };
    const TArray<bool> Alive { true, false, true };

    const TMap<FRTCellId, int32> Occ = URTMatchSetupLibrary::BuildOccupancy(Cells, Ids, Alive);
    TestEqual(TEXT("le unita' morte non occupano"), Occ.Num(), 2);
    TestEqual(TEXT("cella 0 occupata da 10"), Occ[FRTCellId(0, 0)], 10);
    TestFalse(TEXT("cella dell'unita' morta libera"), Occ.Contains(FRTCellId(1, 0)));

    const TMap<FRTCellId, int32> Mismatch = URTMatchSetupLibrary::BuildOccupancy(Cells, { 10 }, Alive);
    TestEqual(TEXT("array incoerenti -> occupazione vuota"), Mismatch.Num(), 0);
    return true;
}
```

- [ ] **Step 2: Eseguire i test e verificare che falliscano**

Compilazione fallita con `RTMatchSetupLibrary.h` non trovato: è il rosso atteso.

- [ ] **Step 3: Scrivere l'header**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTMatchSetupLibrary.generated.h"

class URTHexMapAsset;

/**
 * Funzioni PURE di allestimento della partita su mappa esagonale: scelta delle celle di partenza e
 * ricostruzione dell'occupazione dallo stato delle unita'. Nessuno stato, nessun Actor, nessun World:
 * il GameMode le chiama, i test le verificano headless.
 */
UCLASS()
class REFACTORTACTICS_API URTMatchSetupLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Celle di partenza per un NumPerTeam vs NumPerTeam sul layer indicato: le prime NumPerTeam al team 0,
     * le successive al team 1. Prende le celle percorribili in ordine STABILE (Layer, X, Y) e assegna i due
     * team dalle due estremita' dell'ordine, cosi' le squadre partono lontane senza dipendere da RNG.
     * Mappa nulla o celle percorribili insufficienti -> array vuoto (il chiamante non allestisce).
     */
    static TArray<FRTCellId> PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer);

    /**
     * Occupazione cella -> UnitId ricostruita dallo stato delle unita': solo le vive occupano.
     * I tre array sono paralleli; lunghezze incoerenti -> mappa vuota (input non fidato, nessun indovinare).
     */
    static TMap<FRTCellId, int32> BuildOccupancy(const TArray<FRTCellId>& Cells,
        const TArray<int32>& UnitIds, const TArray<bool>& Alive);
};
```

- [ ] **Step 4: Scrivere l'implementazione**

```cpp
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"

TArray<FRTCellId> URTMatchSetupLibrary::PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer)
{
    TArray<FRTCellId> Result;
    if (!Map || NumPerTeam <= 0)
    {
        return Result;
    }

    // Celle percorribili del layer, in ordine stabile: CellsInLayer lo garantisce gia' (Layer, X, Y).
    TArray<FRTCellId> Walkable;
    for (const FRTCellId& Id : Map->CellsInLayer(Layer))
    {
        const FRTHexCellData* Data = Map->FindCell(Id);
        if (Data && !Data->bBlocksMovement)
        {
            Walkable.Add(Id);
        }
    }

    if (Walkable.Num() < NumPerTeam * 2)
    {
        return Result;
    }

    // Team 0 dall'inizio dell'ordine, team 1 dalla fine: le squadre partono agli estremi della mappa.
    for (int32 i = 0; i < NumPerTeam; ++i)
    {
        Result.Add(Walkable[i]);
    }
    for (int32 i = 0; i < NumPerTeam; ++i)
    {
        Result.Add(Walkable[Walkable.Num() - 1 - i]);
    }
    return Result;
}

TMap<FRTCellId, int32> URTMatchSetupLibrary::BuildOccupancy(const TArray<FRTCellId>& Cells,
    const TArray<int32>& UnitIds, const TArray<bool>& Alive)
{
    TMap<FRTCellId, int32> Occupancy;
    if (Cells.Num() != UnitIds.Num() || Cells.Num() != Alive.Num())
    {
        return Occupancy;
    }

    for (int32 i = 0; i < Cells.Num(); ++i)
    {
        if (Alive[i])
        {
            Occupancy.Add(Cells[i], UnitIds[i]);
        }
    }
    return Occupancy;
}
```

- [ ] **Step 5: Build + suite**

Atteso: build `Succeeded`; suite `Success: 176`, `Fail: 0` (172 + 4 nuovi).

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Turn/RTMatchSetupLibrary.h Source/RefactorTactics/Turn/RTMatchSetupLibrary.cpp Source/RefactorTactics/Tests/RTMatchSetupTests.cpp
git commit -m "feat(setup): libreria pura di allestimento su mappa esagonale"
```

---

## Task 3: `ARTUnit` si posiziona sui centri esagonali

**Files:**
- Modify: `Source/RefactorTactics/Unit/RTUnit.h` (firme di `PlaceOnCell`/`WorldForCell`)
- Modify: `Source/RefactorTactics/Unit/RTUnit.cpp`
- Test: `Source/RefactorTactics/Tests/RTUnitPlacementTests.cpp` (nuovo)

**Interfaces:**
- Consuma: `URTHexLibrary::AxialToWorld(const FRTCellId&, const FVector& Origin, float HexSize, float LayerHeight)`
- Produce:
  - `void ARTUnit::PlaceOnCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight)`
  - `FVector ARTUnit::WorldForCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight) const`

  Il parametro `CellSize` quadrato diventa `HexSize`; `LayerHeight` perde il default `0.f` perché su mappa
  multilivello è un dato reale della mappa, non un extra opzionale.

- [ ] **Step 1: Scrivere il test che fallisce**

```cpp
#include "Misc/AutomationTest.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitWorldForCellTest,
    "RefactorTactics.Unit.WorldForCellIsHexCenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitWorldForCellTest::RunTest(const FString&)
{
    // WorldForCell e' const e legge solo VisualZOffset: basta il CDO, niente World da creare e distruggere.
    const ARTUnit* Unit = GetDefault<ARTUnit>();
    const FVector Origin(0.f, 0.f, 0.f);
    const float HexSize = 100.f;
    const float LayerHeight = 250.f;
    const FRTCellId Cell(2, -1, 1);

    const FVector Expected = URTHexLibrary::AxialToWorld(Cell, Origin, HexSize, LayerHeight)
        + FVector(0.f, 0.f, Unit->VisualZOffset);

    TestTrue(TEXT("la posizione visiva e' il centro esagonale piu' l'offset di presentazione"),
        Unit->WorldForCell(Cell, Origin, HexSize, LayerHeight).Equals(Expected, 0.01f));
    return true;
}
```

- [ ] **Step 2: Eseguire e verificare il rosso**

Atteso: errore di compilazione sulla firma (il tipo di ritorno e i parametri non corrispondono ancora).

- [ ] **Step 3: Aggiornare le firme in `RTUnit.h`**

```cpp
    /** Posiziona l'unita' al centro-mondo della cella esagonale, con la base appoggiata al piano. */
    void PlaceOnCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight);

    /** Posizione-mondo che PlaceOnCell userebbe per InCell, senza modificare lo stato logico (playback). */
    FVector WorldForCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight) const;
```

- [ ] **Step 4: Implementare in `RTUnit.cpp`**

```cpp
FVector ARTUnit::WorldForCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight) const
{
    // La geometria esagonale sta in URTHexLibrary: qui si aggiunge solo l'offset di PRESENTAZIONE.
    return URTHexLibrary::AxialToWorld(InCell, Origin, HexSize, LayerHeight) + FVector(0.f, 0.f, VisualZOffset);
}

void ARTUnit::PlaceOnCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight)
{
    Cell = InCell;                       // posizione AUTOREVOLE
    PlannedCell = InCell;
    SetActorLocation(WorldForCell(InCell, Origin, HexSize, LayerHeight));  // solo rendering
}
```

- [ ] **Step 5: Build + suite**

Atteso: `Succeeded`; `Success: 177`, `Fail: 0`.

- [ ] **Step 6: Commit**

```bash
git add Source/RefactorTactics/Unit Source/RefactorTactics/Tests/RTUnitPlacementTests.cpp
git commit -m "feat(unit): posizionamento sui centri esagonali via URTHexLibrary"
```

---

## Task 4: `ARTGameMode` allestisce dalla mappa esagonale

**Files:**
- Modify: `Source/RefactorTactics/RTGameMode.h`
- Modify: `Source/RefactorTactics/RTGameMode.cpp`
- Test: `Source/RefactorTactics/Tests/RTMatchSetupWorldTests.cpp` (nuovo, test in `UWorld`)

**Interfaces:**
- Consuma: `URTMatchSetupLibrary::PickStartCells` (Task 2), `ARTUnit::PlaceOnCell` (Task 3),
  `ARTHexMapActor::MapAsset`, `URTHexMapAsset::HexSize`, `URTHexMapAsset::LayerHeight`
- Produce: `ARTUnit* ARTGameMode::SpawnUnit(int32 TeamId, const FRTCellId& InCell, bool bGuardian, const FVector& Origin, float HexSize, float LayerHeight)`

- [ ] **Step 1: Scrivere il test in `UWorld` che fallisce**

Il repo crea i world di test con `UWorld::CreateWorld` + `FWorldContext` (vedi
`Tests/RTBotPlanningTests.cpp:20-38`), **non** con `FAutomationEditorCommonUtils`: seguire quel pattern e
distruggere sempre il world alla fine, anche sui rami di fallimento.

```cpp
#include "Misc/AutomationTest.h"
#include "RTGameMode.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

namespace
{
    /** Come in RTBotPlanningTests: World di gioco minimale, senza tick ne' rendering. */
    UWorld* MakeSetupTestWorld()
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
        if (World && GEngine)
        {
            FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
            Ctx.SetCurrentWorld(World);
        }
        return World;
    }

    void DestroySetupTestWorld(UWorld* World)
    {
        if (World && GEngine)
        {
            GEngine->DestroyWorldContext(World);
            World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGameModeHexSetupTest,
    "RefactorTactics.MatchSetup.GameModeSpawnsOnHexMap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGameModeHexSetupTest::RunTest(const FString&)
{
    UWorld* World = MakeSetupTestWorld();
    if (!TestNotNull(TEXT("world di prova"), World))
    {
        return false;
    }

    // Mappa esagonale di prova: un esagono di raggio 2 sul layer 0.
    URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
    for (int32 q = -2; q <= 2; ++q)
    {
        for (int32 r = -2; r <= 2; ++r)
        {
            if (FMath::Abs(q + r) <= 2)
            {
                FRTHexCellData Data;
                Data.Id = FRTCellId(q, r, 0);
                Map->AddOrUpdateCell(Data);
            }
        }
    }

    ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
    MapActor->MapAsset = Map;

    World->SpawnActor<ARTGameMode>()->DispatchBeginPlay();

    int32 NumUnits = 0;
    TSet<FRTCellId> Occupied;
    for (TActorIterator<ARTUnit> It(World); It; ++It)
    {
        ++NumUnits;
        Occupied.Add(It->Cell);
        TestTrue(TEXT("l'unita' sta su una cella della mappa"), Map->ContainsCell(It->Cell));
    }

    TestEqual(TEXT("board 2v2"), NumUnits, 4);
    TestEqual(TEXT("nessuna sovrapposizione di partenza"), Occupied.Num(), 4);

    DestroySetupTestWorld(World);
    return true;
}
```

- [ ] **Step 2: Eseguire e verificare il rosso**

Atteso: `Cell` non esiste ancora come membro pubblico usato così, oppure 0 unità spawnate (il GameMode cerca
ancora `ARTGridActor`).

- [ ] **Step 3: Sostituire l'allestimento in `RTGameMode.cpp`**

```cpp
    // Mappa esagonale: usa quella presente nel livello, altrimenti ne crea una all'origine (graybox demo).
    ARTHexMapActor* HexMap = Cast<ARTHexMapActor>(
        UGameplayStatics::GetActorOfClass(this, ARTHexMapActor::StaticClass()));
    if (!HexMap)
    {
        HexMap = World->SpawnActor<ARTHexMapActor>(ARTHexMapActor::StaticClass(), FTransform::Identity);
    }

    // ... luce e TurnManager invariati ...

    // Board 2v2 (solo se non ci sono gia' unita' nel livello).
    if (HexMap && !UGameplayStatics::GetActorOfClass(this, ARTUnit::StaticClass()))
    {
        const URTHexMapAsset* Map = HexMap->MapAsset;
        const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Map, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
        if (Start.Num() == 4)
        {
            const FVector Origin = HexMap->GetActorLocation();
            const float HexSize = Map ? Map->HexSize : HexMap->HexSize;
            const float LayerHeight = Map ? Map->LayerHeight : HexMap->LayerHeight;

            // Per squadra: un Ranger (fragile, lunga gittata) e un Guardian (tanky, corta gittata).
            SpawnUnit(0, Start[0], /*bGuardian=*/ false, Origin, HexSize, LayerHeight);
            SpawnUnit(0, Start[1], /*bGuardian=*/ true,  Origin, HexSize, LayerHeight);
            SpawnUnit(1, Start[2], /*bGuardian=*/ false, Origin, HexSize, LayerHeight);
            SpawnUnit(1, Start[3], /*bGuardian=*/ true,  Origin, HexSize, LayerHeight);
            UE_LOG(LogRT, Log, TEXT("[RT] Board 2v2 esagonale avviata su %d celle"),
                Map ? Map->NumCells() : 0);
        }
        else
        {
            UE_LOG(LogRT, Warning,
                TEXT("[RT] Mappa esagonale senza celle percorribili sufficienti: partita non allestita"));
        }
    }
```

- [ ] **Step 4: Build + suite**

Atteso: `Succeeded`; `Success: 178`, `Fail: 0`.

- [ ] **Step 5: Commit**

```bash
git add Source/RefactorTactics/RTGameMode.h Source/RefactorTactics/RTGameMode.cpp Source/RefactorTactics/Tests/RTMatchSetupWorldTests.cpp
git commit -m "feat(match): il GameMode allestisce la partita dalla mappa esagonale"
```

---

## Task 5: Chiusura del checkpoint

**Files:**
- Modify: `docs/design/roadmap-checkpoint.md` (riga di CP 6.1)
- Modify: `docs/design/test-manuali-pie.md` (esito di `PIE-HEXPLAY-1`)

- [ ] **Step 1: Eseguire `PIE-HEXPLAY-1` in editor**

Aprire `L_DevSandbox` (ha già `ARTHexMapActor` + `DA_HexMap_Sandbox`), avviare PIE e verificare: 4 unità
sui centri esagonali, 2 per squadra, nessuna sovrapposizione, nessuna unità fuori dalla mappa. Senza i
`BP_Unit_*` le unità appaiono come **cilindri segnaposto** — è il fallback previsto, non un difetto.

- [ ] **Step 2: Registrare l'esito reale**

Scrivere in `test-manuali-pie.md` l'esito osservato (✅ o 🟡 con la nota). Non dichiarare verde ciò che non
è stato eseguito.

- [ ] **Step 3: Aggiornare la roadmap**

Portare la riga **6.1** a ✅ con la data, e annotare il numero di test della suite.

- [ ] **Step 4: Commit e PR verso `main`**

```bash
git add docs/design/roadmap-checkpoint.md docs/design/test-manuali-pie.md
git commit -m "docs(cp6.1): chiude il checkpoint di allestimento su mappa esagonale"
gh pr create --base main --title "CP 6.1 — Allestimento della partita su mappa hex" --body-file - <<'PR'
Primo checkpoint di M6: la partita si allestisce su griglia esagonale.

- `FRTCellId` e' l'unica coordinata del progetto: `FRTGridCoord` rimosso (35 file).
  Tag di ritorno prima della sostituzione: `pre-cellid-swap`.
- `URTMatchSetupLibrary`: celle di partenza e occupazione come funzioni pure, testabili headless.
- `ARTUnit` si posiziona sui centri esagonali via `URTHexLibrary::AxialToWorld`.
- `ARTGameMode` allestisce da `ARTHexMapActor` + `URTHexMapAsset`, non piu' da `ARTGridActor`.

Limite dichiarato: il flusso di partita usa un solo tipo di coordinata ma ancora la matematica
quadrata di `URTGridLibrary` — migra nei CP 6.2-6.4, la libreria sparisce in M7.

Build Editor: Succeeded. Suite: <N> Success, 0 Fail. PIE-HEXPLAY-1: <esito reale>.
PR
```

---

## Cosa NON è in questo checkpoint

Delimitato apposta, per non trasformare CP 6.1 in tutta M6:

- **Movimento** end-to-end su hex (snapshot → `ResolveHexPaths` → TurnLog) → **CP 6.2**
- **Input/selezione/preview** su celle assiali → **CP 6.3**
- **Combat** (forme, LOS esagonale, status) → **CP 6.4**
- **Dash e knockback** esagonali → **CP 6.5**
- **Bot** su `URTHexBotLibrary` → **CP 6.6**
- Rimozione di `URTGridLibrary` e del resolver quadrato → **M7**

Dopo il Task 1 il flusso di partita usa un solo tipo di coordinata ma ancora la **matematica quadrata**: è
uno stato intermedio dichiarato, non un difetto da correggere fuori piano.
