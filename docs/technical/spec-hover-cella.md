# Spec — CP 1.4 Evidenziazione cella sotto il cursore (hover)

> ⚠️ **Superato dal pivot esagonale** ([ADR-0002](../decisions/adr-0002-griglia-esagonale.md)) — **riferimento storico, non normativo.**
> Descrive il substrato **quadrato**, rimosso dal codice al **CP 7.2** (`Grid/`, `URTGridLibrary`, `FRTGridCoord`, resolver e bot quadrati). `ARTGridActor` e `URTGridLibrary` non esistono più; l'hover vive sul percorso `WorldToCellId` del controller.
> Conservato per provenienza e come comportamento di riferimento della parità hex (M6). Punto di ritorno: tag `pre-hex-only`.

> Brainstorming del **2026-08-03**. Chiude il checkpoint **CP 1.4** (M1 polish, rimasto ⏳) e la verifica **PIE-CP1.4**.
> Ancorata al codice (`RTPlayerController`, `RTGridActor`, `RTGridLibrary`), al canone (invariante #1: presentazione,
> non tocca la logica; la griglia logica resta autoritativa). **Documentale: questo file non modifica il codice.**

---

## 1. Obiettivo & scope

Evidenziare in tempo reale la **cella sotto il cursore** del mouse (hover). **Presentazione** (invariante #1).
**Fuori scope**: highlight di raggiungibilità/percorso (già esistono: preview movimento, waypoint).

## 2. Stato attuale (verificato)

| Fatto | Evidenza |
|---|---|
| Il click ottiene la cella: `GetHitResultUnderCursor(ECC_Visibility,…)` → `WorldToCell` | `RTPlayerController.cpp:171,240` |
| `bShowMouseCursor = true`; input via Enhanced Input; **nessun `Tick`/`PlayerTick`** | `RTPlayerController.cpp:107,118-142` |
| Griglia visuale = ISM (`Cells` root `PlaneMesh`, `Obstacles`, `BridgeCells`); **nessun highlight** | `RTGridActor.cpp:11-45`, `RTGridActor.h:100-113` |
| `URTGridLibrary`: `CellToWorld`/`CellToWorldElevated`/`WorldToCell`; **manca `IsInBounds`** | `RTGridLibrary.h:20,28,40` |

## 3. Componenti (`ARTGridActor`)

- Nuovo `TObjectPtr<UStaticMeshComponent> HoverHighlight`: `PlaneMesh` engine, `SetupAttachment(Cells)`,
  **NoCollision** (non deve intercettare il raycast del cursore), MID colorato (giallo tenue) da `TerrainMaterial`
  (già = `M_Unit`, param `"Color"`), inizialmente nascosto. Scala = `CellSize/100 * 0.95` (a `BeginPlay`).
- `void SetHoveredCell(const FRTGridCoord& Cell, bool bValid)`: `!bValid` → `SetVisibility(false)`; altrimenti
  `SetWorldLocation(CellToWorldElevated(Cell, GetActorLocation(), CellSize, HoverZ≈2, LayerHeight))` (leggermente
  sopra la cella, no z-fighting) + `SetVisibility(true)`.

## 4. Tracking (`RTPlayerController`)

- `virtual void PlayerTick(float DeltaTime) override` (idiomatico per il controller). Ogni frame:
  `GetHitResultUnderCursor(ECC_Visibility,false,Hit)` → se hit valido → `Cell = WorldToCell(Hit.Location,…)` →
  `Grid->SetHoveredCell(Cell, URTGridLibrary::IsInBounds(Cell, Grid->Width, Grid->Height))`; altrimenti nascondi.
- `Grid` via `GetActorOfClass(ARTGridActor)` (come già fa `OnSelect`).

## 5. Logica pura (testabile)

- `static bool URTGridLibrary::IsInBounds(const FRTGridCoord& Cell, int32 Width, int32 Height)` →
  `X in [0,Width)` e `Y in [0,Height)`. **Unico pezzo automatizzabile headless**; l'highlight visibile = PIE-CP1.4.

## 6. Fallback & invarianti

- `IsInBounds` non tocca la logica di gioco (solo per l'hover visivo). Il resto è presentazione (invariante #1).
- Additivo: i **71 test** restano verdi. Nessun cambio al click/selezione esistente.

**Requisiti (SMART):**
- **`FR-HOVER-01`** — `IsInBounds` corretta ai bordi (0, Width-1 dentro; -1, Width fuori). *Verifica: test.*
- **`FR-HOVER-02`** — in PIE l'highlight segue la cella sotto il cursore e sparisce fuori griglia. *Verifica: PIE-CP1.4.*

## 7. File

- **Modificati**: `Grid/RTGridLibrary.h/.cpp` (`IsInBounds`), `Grid/RTGridActor.h/.cpp` (`HoverHighlight`, `SetHoveredCell`),
  `Player/RTPlayerController.h/.cpp` (`PlayerTick`), `Tests/RTGridTests.cpp` (test `IsInBounds`).

## 8. Decisioni

- **D-HOVER-1** — disco pieno (Plane engine) colorato, non bordo/outline (semplice, zero nuovi asset: riuso `M_Unit`).
- **D-HOVER-2** — tracking in `PlayerTick` (ogni frame, leggero); highlight posseduto da `ARTGridActor` (è griglia visuale).
- **D-HOVER-3** — `IsInBounds` in `URTGridLibrary` (riusabile, testabile), non inline nel controller.

## 9. Riferimenti

- Roadmap: [`roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md) CP 1.4; [`test-manuali-pie.md`](test-manuali-pie.md) PIE-CP1.4.
- Codice: `RTPlayerController.cpp` (OnSelect), `RTGridActor.cpp` (costruttore/BuildGrid), `RTGridLibrary.h`.
