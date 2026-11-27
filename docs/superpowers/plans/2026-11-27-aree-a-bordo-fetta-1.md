# Aree a bordo e bordi a contesto (fetta 1 di #4191) — piano di esecuzione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** in pianificazione le aree si disegnano come bordo luminoso con una famiglia di istanze, il bordo del singolo esagono compare solo a contesto, la griglia è spenta di norma sulle celle in vista e un tasto tenuto premuto accende tutti i bordi ([#4192](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4192)).

**Architecture:** il giudizio sta in librerie pure nuove (lati accesi di un'area, composizione delle istanze, tabella degli slot, maschera della griglia); una componente a istanze nuova le posa; un materiale nuovo, costruito da una ricetta col suo commandlet, accende i lati indicati dai dati per istanza. `ARTHexMapActor` perde i blocchi che disegnavano i contorni a segmenti e guadagna solo le chiamate verso il pezzo nuovo. Le linee (percorso, mira, tiro interrotto, confine della vista) restano dove sono.

**Tech Stack:** Unreal Engine 5.8 (`D:\EpicGames\UE_5.8`), C++, Automation Test Framework, `tools/suite/esegui.py`, `node tools/radar/*.ts`, `python tools/architettura/mappa-cartelle.py`.

**Spec:** [`docs/superpowers/specs/2026-11-27-pianificazione-aree-ed-effetto-design.md`](../specs/2026-11-27-pianificazione-aree-ed-effetto-design.md), §5.1, §3 e §6. Decisione: `D-635`.

**Ramo e worktree:** `feat/4192-aree-a-bordo`, worktree `.claude/worktrees/4192-aree-a-bordo`, base `origin/main` dopo il merge della PR dei documenti.

> **Revisione.** Un revisore indipendente ha letto la prima stesura contro il codice (2026-11-27). Ne vengono: le due famiglie (dietro e davanti alle unità), la regola del lato condiviso, lo strato del cursore, la regola "mappa senza velo", i filtri di test corretti, la tabella degli slot fuori dall'attore, e la rinuncia al velo dentro le celle colpite.

## Struttura dei file

Regola d'autore del 2026-11-27: "struttura il codice in classi e directory, non creare classi con troppe righe". Il codice nuovo nasce in una cartella sua; nelle classi grandi entra solo la chiamata.

**Nuovi, runtime — `Source/RefactorTactics/Map/Overlay/`** (cartella nuova)

| File | Responsabilità |
|---|---|
| `RTAreaBorderTypes.h` | i tipi: richiesta, istanza, strato, slot, canali dei dati per istanza, percorsi dei materiali |
| `RTAreaEdgeLibrary.h/.cpp` | pura: da lati esposti a maschere per cella |
| `RTAreaBorderComposer.h/.cpp` | pura: dalle richieste alle istanze |
| `RTAreaBorderSlots.h/.cpp` | pura: la tabella "slot → come si disegna" |
| `RTAreaBorderMesh.h/.cpp` | la mesh transient del piano esagonale, con le UV |
| `RTAreaBorderComponent.h/.cpp` | una famiglia di istanze: posa trasformate e dati per istanza |
| `RTAreaBorderPresenter.h/.cpp` | tiene le richieste per slot, ricompone quando cambiano, chiama le due famiglie |
| `RTGridVisibility.h` | pura, solo header: la maschera dell'anello della griglia |

**Nuovi, Editor — `Source/RefactorTacticsEditor/Private/Content/Materials/`**: `RTHexAreaBorderMaterial.h/.cpp` (la ricetta) e `RTBuildHexAreaBorderMaterialCommandlet.h/.cpp`. ⚠️ Non in `Content/Build/`: lì i file C++ sono a un passo dal tetto di `mappa-cartelle.py`. Un commandlet si trova per nome di classe, ovunque stia.

**Nuovi, test — un file per soggetto:** in `Source/RefactorTactics/Tests/Map/Overlay/`: `RTAreaEdgeLibraryTests.cpp`, `RTAreaBorderComposerTests.cpp`, `RTAreaBorderSlotsTests.cpp`, `RTAreaBorderMeshTests.cpp`, `RTAreaBorderComponentTests.cpp`, `RTAreaBorderPlanningTests.cpp`, `RTGridVisibilityTests.cpp`. In `Source/RefactorTacticsEditor/Private/Tests/Content/`: `RTHexAreaBorderMaterialTests.cpp`.

**Nuovi, asset:** `Content/RT/Core/Grid/M_HexAreaBorder.uasset` e `M_HexAreaBorderFront.uasset`, ciascuno con la sua riga `!` in `.gitignore`.

**Modificati, solo chiamate sottili:** `Map/Actor/RTHexMapActor.h/.cpp`, `Map/Actor/RTMapVisuals.h`, `Map/Actor/RTOverlayPalette.cpp`, `Player/RTPlayerController.h/.cpp`. Più i test che fissano la resa di oggi (Task 6, 8 e 9) e i documenti (Task 11).

## Global Constraints

- **Struttura.** `Source/RefactorTactics/Map/Actor/RTHexMapActor.cpp` a fine lavoro non ha più righe della base (annotale al Task 5, confrontale al Task 12). Nessun file nuovo supera le poche centinaia di righe: se cresce, si divide per responsabilità.
- **Il giudizio è puro.** Lati accesi, composizione, tabella degli slot e maschera della griglia stanno in funzioni statiche testabili senza mondo. L'attore e la componente posano.
- **Nessun significato disegnato due volte.** Quando un significato passa alla famiglia nuova, il suo blocco a segmenti si toglie nello stesso commit.
- **Il modello non cambia.** I membri `Preview*Area`, i getter `GetPreview*` e `NumPlanningPreviewCells` restano come sono: li leggono molti test e il controller. Cambia solo chi disegna.
- **Dietro o davanti alle unità lo dice la palette.** Un'istanza va nella famiglia "davanti" (materiale senza test di profondità) se `URTOverlayPalette::DrawsThroughUnits(Meaning)` è vero, altrimenti in quella "dietro". È il contratto di oggi per `Attack`, `FriendlyFire`, `Hover`, le portate e `AttackOriginAim`, e per l'arco di vista ([D-521](../../decisions/RT_PDR_00_Decision_Log.md)).
- **Niente ricalcolo a ogni frame.** Le famiglie si ricompongono quando cambia una richiesta. Un'anteprima fatta di sole aree non accende il Tick dell'attore.
- **Numerazione dei lati.** Il bit `i` di una maschera è il lato `URTHexLibrary::EdgeIndexForDirection(D)`. Esistono due numerazioni (`Map/RTHexLibrary.h`): non usarne una a mano.
- **Dati per istanza**, cinque float: `0..2` colore lineare; `3` maschera dei lati luminosi, più `64` se sono tratteggiati; `4` maschera dei lati sottili.
- **Determinismo e privacy.** Nessuna dipendenza dall'ordine di `TMap`/`TSet`: le uscite sono ordinate con `URTHexLibrary::StableLess`. Le famiglie disegnano solo ciò che i setter di oggi già ricevono: nessuna lettura nuova di unità o mappa.
- **Unity build.** Gli helper nei namespace anonimi dei test hanno nomi unici per file. Un test nuovo non ha un nome che sia prefisso di un altro.
- **Filtri di test.** I test dell'attore della mappa stanno in tre famiglie: `RefactorTactics.HexMapActor`, `RefactorTactics.HexMap` (qui i test di `RTLayerPresentationTests.cpp`) e `RefactorTactics.AreaOverlay` (qui `DrawingMutatesNothing` e `PreviewAddsNoActorPerCell`). Nominale tutte e tre.
- **Gate già rossi sulla base.** `python tools/architettura/mappa-cartelle.py --verifica` esce 1 per `Tests/Shared`, e `node tools/radar/pie-verdict-age.ts --check` segnala celle sue: si confronta l'uscita con quella della base, non il codice d'uscita.
- **Motore:** uno per macchina. Prima di compilare o lanciare test leggi la `CommandLine` dei processi `UnrealEditor%` (`Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'"`); se è in uso da un'altra sessione aspetta o dichiara `NOT RUN`.
  - Build: `D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat RefactorTacticsEditor Win64 Development -Project=<worktree>\RefactorTactics.uproject -WaitMutex`, in PowerShell leggendo `$LASTEXITCODE`.
  - Test: `python tools/suite/esegui.py <filtro> --log <FILE nello scratchpad>`; la prima suite di un worktree nuovo può metterci molti minuti ad avviarsi (`--timeout 15` o più), e l'esito si legge dal log.
- **Commit** `<type>(4192): <descrizione>`, percorsi espliciti in `git add`, riga finale `Co-Authored-By: Claude Fable 5.2 <noreply@anthropic.com>`.

## Review Focus

1. **Due piani sulla stessa colonna.** Un'area con celle `(q, r, 0)` e `(q, r, 1)`: i lati si decidono piano per piano, e ogni istanza sta alla quota del suo piano (Task 1 e Task 5).
2. **Una cella che la mappa non ha.** Una richiesta con una cella fuori mappa non produce istanza e non fa cadere niente (Task 5).
3. **Azzerare.** Dopo `ClearPlanningPreview` le famiglie non hanno istanze di pianificazione; il cursore, se c'è, resta (Task 6).
4. **Ricostruzione e vista strategica.** Dopo `RebuildInstances` e dopo il cambio di vista le istanze ci sono ancora, alla quota nuova (Task 5).
5. **Tasto rilasciato senza evento.** Se l'azione viene annullata invece che completata, i bordi si spengono lo stesso (Task 10).

---

### Task 1: Dai lati esposti alle maschere

**Files:** crea `Map/Overlay/RTAreaBorderTypes.h` (per ora solo `FRTAreaEdgeCell`), `Map/Overlay/RTAreaEdgeLibrary.h/.cpp`, `Tests/Map/Overlay/RTAreaEdgeLibraryTests.cpp`.

La regola "un lato è acceso se il vicino sullo stesso piano non sta nell'insieme" esiste già ed è testata: `URTVisibilityBorderLibrary::ExposedEdges(const TArray<FRTCellId>&)` (`Perception/Veil/RTVisibilityBorder.h`). Non si riscrive: si riusa.

**Interfaces — Produces:**

```cpp
struct FRTAreaEdgeCell { FRTCellId Cell; uint8 Mask = 0; };

class URTAreaEdgeLibrary : public UBlueprintFunctionLibrary
{
    /** I lati dati uno per uno (cella, direzione), raggruppati per cella: bit EdgeIndexForDirection(D). Ordine StableLess. */
    static TArray<FRTAreaEdgeCell> MasksFromExposedEdges(const TArray<FRTExposedEdge>& Edges);
    /** Una voce per cella distinta dell'area, in ordine StableLess: le maschere di ExposedEdges(Cells); le celle interne hanno 0. */
    static TArray<FRTAreaEdgeCell> PerimeterMasks(const TArray<FRTCellId>& Cells);
};
```

- [ ] **Step 1:** Test `RefactorTactics.AreaBorder.Edges.SingleCellLightsAllSix`: una cella sola dà maschera `63`. È anche il caso del bordo di mappa: la libreria non conosce la mappa.
- [ ] **Step 2:** Test `...Edges.SharedSideIsDark`: `(0,0,0)` e `(1,0,0)`: il bit di `EdgeIndexForDirection(E)` è spento sulla prima, quello di `W` sulla seconda, gli altri cinque accesi su entrambe.
- [ ] **Step 3:** Test `...Edges.HoleLightsTheInnerSides`: l'anello dei sei vicini di `(0,0,0)` senza il centro: ogni cella ha acceso il lato verso il centro.
- [ ] **Step 4:** Test `...Edges.LayersDoNotTouch` (Review Focus 1): `(0,0,0)` e `(0,0,1)` nella stessa area danno `63` ciascuna.
- [ ] **Step 5:** Test `...Edges.InteriorCellsStayInTheOutput`: il centro più i sei vicini: sette voci, quella del centro con maschera `0`; ordine `StableLess`; un doppione nell'ingresso non cambia l'uscita.
- [ ] **Step 6:** Test `...Edges.ExposedEdgesBecomeMasks`: tre `FRTExposedEdge` (due sulla stessa cella) danno due voci, con i bit giusti.
- [ ] **Step 7:** Implementa; build; `python tools/suite/esegui.py RefactorTactics.AreaBorder` verde.
- [ ] **Step 8:** Commit `feat(4192): dai lati esposti alle maschere per cella`.

### Task 2: La composizione delle istanze

**Files:** completa `Map/Overlay/RTAreaBorderTypes.h`; crea `Map/Overlay/RTAreaBorderComposer.h/.cpp`, `Tests/Map/Overlay/RTAreaBorderComposerTests.cpp`.

**Interfaces — Consumes:** `URTAreaEdgeLibrary`, `FRTOverlayArea`, `URTOverlayPalette::ColorFor`, `PriorityFor`, `DrawsThroughUnits`. **Produces:**

```cpp
enum class ERTAreaBorderMode : uint8 { Perimeter, EveryCell, PerimeterAndCells };
enum class ERTAreaBorderLayer : uint8 { Base, Cells, Cursor };   // quota crescente

struct FRTAreaBorderRequest
{
    FRTOverlayArea Area;
    ERTAreaBorderMode Mode = ERTAreaBorderMode::Perimeter;
    ERTAreaBorderLayer Layer = ERTAreaBorderLayer::Base;
    bool bDashed = false;              // lati luminosi tratteggiati
    TArray<FRTExposedEdge> Edges;      // se non vuoto, i lati luminosi sono questi (arco di vista)
    TOptional<FColor> ColorOverride;   // se assente, il colore della palette per Area.Meaning
    TOptional<bool> FrontOverride;     // se assente, DrawsThroughUnits(Area.Meaning)
};

struct FRTAreaBorderInstance
{
    FRTCellId Cell;
    FLinearColor Color;
    uint8 GlowMask = 0;
    uint8 ThinMask = 0;
    bool bDashed = false;
    ERTAreaBorderLayer Layer = ERTAreaBorderLayer::Base;
    bool bFront = false;               // famiglia senza test di profondita'
};

class URTAreaBorderComposer : public UBlueprintFunctionLibrary
{
    static TArray<FRTAreaBorderInstance> Compose(const TArray<FRTAreaBorderRequest>& Requests, bool bShowEveryCell);
};
```

Regole, da fissare coi test:
- **Un vincitore per cella e per strato.** In uno strato, su una cella contesa vince la richiesta con `PriorityFor(Meaning)` più alta; a parità, quella che viene prima nell'array.
- **Lato condiviso** (spec §5.1, "vince la più interna"). Il lato di una cella il cui vicino è vinto, nello stesso strato, da un'altra richiesta si accende solo se la richiesta della cella batte quella del vicino con lo stesso criterio (priorità più alta; a parità, prima nell'array). Così il confine fra due aree adiacenti si disegna una volta sola, dal lato dell'area che viene prima.
- **Modi.** `Perimeter`: `GlowMask` = perimetro; `ThinMask` = `63` se `bShowEveryCell`, altrimenti `0`. `EveryCell`: `GlowMask` `0`, `ThinMask` `63`. `PerimeterAndCells`: `GlowMask` = perimetro, `ThinMask` `63`. Con `Edges` non vuoto: `GlowMask` dai lati dati, `ThinMask` come `Perimeter`.
- Un'istanza con le due maschere a `0` non esce.
- `bFront` = `FrontOverride` se c'è, altrimenti `DrawsThroughUnits(Meaning)`. Colore = `ColorOverride` se c'è, altrimenti `ColorFor(Meaning)`, convertito con `FLinearColor::FromSRGBColor`.
- Uscita ordinata per strato (`Base`, `Cells`, `Cursor`) e dentro lo strato con `StableLess`.

- [ ] **Step 1:** Test `RefactorTactics.AreaBorder.Compose.PerimeterAreaHasNoInnerInstances`: sette celle (centro e sei vicini), `Perimeter`, senza tasto: sei istanze, nessuna per il centro, nel colore di `Movement`, `bFront` falso.
- [ ] **Step 2:** Test `...Compose.KeyAddsThinSidesEverywhere`: la stessa richiesta con `bShowEveryCell`: sette istanze, `ThinMask == 63` su tutte, `GlowMask` invariata sulle sei.
- [ ] **Step 3:** Test `...Compose.HigherPriorityWinsTheCell`: `AbilityRange` su sette celle e `Attack` (`Perimeter`, strato `Base`) su una di bordo: quella cella ha una sola istanza `Base`, nel colore di `Attack`.
- [ ] **Step 4:** Test `...Compose.SharedSideIsDrawnOnce`: due richieste `Movement` e `MovementOverBudget` (pari priorità) su `(0,0,0)` e `(1,0,0)`: il lato comune è acceso sulla prima (bit di `E`) e spento sulla seconda (bit di `W`); gli altri cinque lati accesi su entrambe.
- [ ] **Step 5:** Test `...Compose.LayersDoNotEraseEachOther`: sulla stessa cella `Movement` (`Perimeter`, `Base`), `Attack` (`PerimeterAndCells`, `Cells`) e `Hover` (`EveryCell`, `Cursor`): tre istanze, una per strato, ciascuna col suo colore. Il cursore non cancella la cella colpita.
- [ ] **Step 6:** Test `...Compose.FrontFollowsThePalette`: un'istanza di `Movement` ha `bFront` falso, una di `Attack` vero; `FrontOverride` vince su entrambe.
- [ ] **Step 7:** Test `...Compose.DashedAndOverridesAreCarried` e `...Compose.EmptyRequestsGiveNothing`.
- [ ] **Step 8:** Implementa; build; `RefactorTactics.AreaBorder` verde.
- [ ] **Step 9:** Commit `feat(4192): composizione delle aree in istanze, per strato`.

### Task 3: La mesh del piano esagonale

**Files:** crea `Map/Overlay/RTAreaBorderMesh.h/.cpp`, `Tests/Map/Overlay/RTAreaBorderMeshTests.cpp`; aggiungi la mesh all'elenco di `Tests/Map/RTHexMapMeshBoundsTests.cpp`.

**Interfaces — Produces:** `namespace RTAreaBorderMesh { UStaticMesh* Get(); constexpr float Radius = 50.f; }`. Una mesh transient per processo, tenuta con `TStrongObjectPtr` come `ARTHexMapActor::GetCellBorderMesh`: un vertice al centro e sei agli angoli di `URTHexLibrary::HexCorners(FVector::ZeroVector, Radius)`, sei triangoli, normale verso l'alto, UV0 = posizione locale `XY / Radius`. Lo slot del materiale si inizializza come nelle altre mesh procedurali dell'attore (`MaterialSlotName = RTProceduralMeshSlotName`, `UVChannelData = FMeshUVChannelInfo(1.f)`: #1665).

- [ ] **Step 1:** Test `RefactorTactics.AreaBorder.Mesh.IsAHexagonWithLocalUVs`: sette vertici e sei triangoli; per ogni angolo `i` la UV coincide con `HexCorners(ZeroVector, 1)[i]`; due chiamate a `Get()` danno lo stesso oggetto.
- [ ] **Step 2:** Implementa; aggiungi la mesh all'elenco del test dei limiti; build; `RefactorTactics.AreaBorder` verde e il test dei limiti delle mesh procedurali verde.
- [ ] **Step 3:** Commit `feat(4192): la mesh del piano esagonale con le UV locali`.

### Task 4: I due materiali dei lati

**Files:** crea `RefactorTacticsEditor/Private/Content/Materials/RTHexAreaBorderMaterial.h/.cpp` e `RTBuildHexAreaBorderMaterialCommandlet.h/.cpp`, `RefactorTacticsEditor/Private/Tests/Content/RTHexAreaBorderMaterialTests.cpp`; aggiungi in `RTAreaBorderTypes.h`:

```cpp
namespace RTAreaBorder
{
    inline constexpr TCHAR MaterialPath[]      = TEXT("/Game/RT/Core/Grid/M_HexAreaBorder.M_HexAreaBorder");
    inline constexpr TCHAR FrontMaterialPath[] = TEXT("/Game/RT/Core/Grid/M_HexAreaBorderFront.M_HexAreaBorderFront");
    inline constexpr int32 NumCustomDataFloats = 5;
}
```

Modifica `.gitignore`, `docs/technical/tooling/asset-map.md` e le liste che nominano i materiali di `Core/Grid` (`git grep -n "M_HexVolume" -- tools docs`).

**Modello da seguire:** `RTHexVolumeMaterial.cpp` (`BuildGraph`, `Build(bool bDryRun)`, nodo `UMaterialExpressionCustom`, `RTMaterialRecipe::ClearGraph`, `MATUSAGE_InstancedStaticMeshes`, `UseConstant = false`, `PostEditChange()`), il suo commandlet e `RefactorTactics.HexVolume.MaterialMatchesTheRecipe`.

**Che cosa fanno i materiali:** `Unlit`, `BLEND_Translucent`. La ricetta è una, con un parametro: `M_HexAreaBorder` ha il test di profondità, `M_HexAreaBorderFront` no (`bDisableDepthTest`). Un nodo `Custom` riceve la UV0 e i cinque dati per istanza e calcola, per ogni lato `i`, la distanza `d = apotema - dot(uv, n_i)`:
- lato luminoso (bit `i` del canale 3): una riga piena larga `0.07` più un bagliore che sfuma verso l'interno fino a `0.34`;
- lati tratteggiati (canale 3 maggiore o uguale a `64`): la stessa riga, spenta a tratti lungo il lato;
- lato sottile (bit `i` del canale 4): una riga larga `0.035`, senza bagliore.

Emissive = colore dei canali `0..2`; opacità = il massimo dei termini, saturato. Le normali `n_i` non si scrivono a mano: la ricetta le calcola dalla libreria e le scrive come costanti nel codice del nodo. Larghezze e forma del bagliore sono valori di partenza: li giudica l'autore sulla tavola del Task 12.

⚠️ **Dove si lancia il commandlet.** La spec (§3) dice che un asset si autora nel clone principale. La ragione è che un worktree non ha i file ignorati da git, e salvare un asset coi riferimenti rotti li azzera. Questi due materiali nascono nuovi e non referenziano asset del progetto: il commandlet si lancia nel worktree, e lo Step 5 verifica che i riferimenti dei due pacchetti siano solo del motore. *Lettura dell'assistente, dichiarata.*

- [ ] **Step 1:** Due righe `!Content/RT/Core/Grid/M_HexAreaBorder.uasset` e `...Front.uasset` in `.gitignore`, accanto a quella di `M_HexVolume`; `git check-ignore -q <percorso>` esce 1 per entrambi.
- [ ] **Step 2:** Test `RefactorTactics.HexAreaBorder.MaterialMatchesTheRecipe`, sul modello di quello di `M_HexVolume`, per entrambi gli asset: proprietà e grafo sono quelli della ricetta, e i due differiscono solo per il test di profondità.
- [ ] **Step 3:** Test `RefactorTactics.HexAreaBorder.EdgeNormalsFollowTheLibrary`: per ognuna delle sei direzioni `D`, la normale che la ricetta scrive all'indice `EdgeIndexForDirection(D)` è parallela e concorde a `EdgeMidpointWorld(C, D, …) - AxialToWorld(C, …)` per una cella `C` qualunque.
- [ ] **Step 4:** Implementa ricetta e commandlet (`-run=RTBuildHexAreaBorderMaterial [-DryRun]`, costruisce entrambi); build.
- [ ] **Step 5:** Lancia il commandlet nel worktree: `UnrealEditor-Cmd.exe <worktree>\RefactorTactics.uproject -run=RTBuildHexAreaBorderMaterial -unattended -nopause -nosplash "-abslog=<scratchpad>\build-hexareaborder.log"`. Atteso: exit 0 e i due `.uasset` sul disco. `node tools/asset-refs/check.ts` e `node tools/asset-provenance/check.ts` verdi.
- [ ] **Step 6:** `RefactorTactics.HexAreaBorder` verde; documenti e liste aggiornati.
- [ ] **Step 7:** Commit `feat(4192): materiali dei lati luminosi, con ricetta e commandlet` (include i due `.uasset` e `.gitignore`).

### Task 5: Le due famiglie e il presentatore

**Files:** crea `Map/Overlay/RTAreaBorderComponent.h/.cpp`, `Map/Overlay/RTAreaBorderPresenter.h/.cpp`, `Map/Overlay/RTAreaBorderSlots.h/.cpp`, `Tests/Map/Overlay/RTAreaBorderComponentTests.cpp`, `Tests/Map/Overlay/RTAreaBorderSlotsTests.cpp`; aggiungi a `RTAreaBorderTypes.h` `ERTAreaBorderSlot` e `FRTAreaBorderPoseFn`; modifica `Map/Actor/RTMapVisuals.h` (quota) e `Map/Actor/RTHexMapActor.h/.cpp` (creazione nel costruttore, mesh e materiali accanto a quelli di `CellBorders`, riposa dopo `RebuildInstances` e dopo `SyncStrategicPresentation`).

**Interfaces — Consumes:** `URTAreaBorderComposer::Compose`, `RTAreaBorderMesh::Get`, `RTAreaBorder::MaterialPath`. **Produces:**

```cpp
enum class ERTAreaBorderSlot : uint8
{
    Reachable, OverBudget, FocusNormal, FocusRun, SkillFree, SkillBlocked, SkillUnverdicted, Range,
    PathCells, AttackOrigin, SightBlock, Hit, AllyHit, EnemyArc, Hover, Count
};

/** Trasformata di una cella alla quota dello strato; false se la mappa non ha quella cella. */
using FRTAreaBorderPoseFn = TFunction<bool(const FRTCellId&, ERTAreaBorderLayer, FTransform&)>;

namespace RTAreaBorderSlots
{
    /** La tabella: per uno slot, come si disegna la sua area (modo, strato, tratteggio). L'ordine degli slot e' l'ordine dell'array di Compose. */
    FRTAreaBorderRequest RequestFor(ERTAreaBorderSlot Slot, const FRTOverlayArea& Area);
}

UCLASS()
class URTAreaBorderComponent : public UInstancedStaticMeshComponent
{
    /** Sostituisce tutte le istanze. Un solo MarkRenderStateDirty. Le celle senza trasformata si saltano. */
    void Show(const TArray<FRTAreaBorderInstance>& Instances, const FRTAreaBorderPoseFn& PoseOf);
    const TArray<FRTAreaBorderInstance>& GetShown() const;   // per i test
};

class FRTAreaBorderPresenter
{
    void Bind(URTAreaBorderComponent* Behind, URTAreaBorderComponent* Front, FRTAreaBorderPoseFn InPoseOf);
    /** true se la richiesta dello slot e' cambiata; in quel caso ricompone e posa. */
    bool SetSlot(ERTAreaBorderSlot Slot, const FRTAreaBorderRequest& Request);
    bool SetSlotArea(ERTAreaBorderSlot Slot, const FRTOverlayArea& Area);   // passa da RTAreaBorderSlots::RequestFor
    bool ClearSlot(ERTAreaBorderSlot Slot);
    void SetShowEveryCell(bool bShow);
    void Repose();                                            // stesse istanze, trasformate nuove
};
```

La tabella di `RTAreaBorderSlots::RequestFor`:

| Slot | Modo | Strato | Tratteggio |
|---|---|---|---|
| `Reachable`, `OverBudget`, `Range`, `SkillFree`, `SkillBlocked` | `Perimeter` | `Base` | no |
| `SkillUnverdicted`, `FocusNormal`, `FocusRun` | `Perimeter` | `Base` | sì (oggi sono tratteggiati) |
| `Hit`, `AllyHit` | `PerimeterAndCells` | `Cells` | no |
| `PathCells`, `AttackOrigin`, `SightBlock` | `EveryCell` | `Cells` | no |
| `Hover` | `EveryCell` | `Cursor` | no |
| `EnemyArc` | `Perimeter` con `Edges` | `Base` | no |

Nell'attore: due `URTAreaBorderComponent` (`AreaBorders`, `AreaBordersFront`) creati nel costruttore come sottocomponenti di default, con `NoCollision`, risposta `ECR_Ignore` su tutti i canali, `CastShadow = false`, `NumCustomDataFloats = RTAreaBorder::NumCustomDataFloats`; un `FRTAreaBorderPresenter` legato dopo che mesh e materiali sono assegnati. La quota è una costante nuova in `RTMapVisuals.h`, `RTLiftAreaBorder = RTLiftCellBorder + 0.4f`; gli strati `Cells` e `Cursor` stanno `0.2f` e `0.4f` più in alto. La scala planare è `HexSize / RTAreaBorderMesh::Radius`.

- [ ] **Step 0:** Annota nel messaggio del commit `wc -l Source/RefactorTactics/Map/Actor/RTHexMapActor.cpp` della base.
- [ ] **Step 1:** Test `RefactorTactics.AreaBorder.Slots.TableMatchesTheSpec`: per ogni slot, `RequestFor` dà modo, strato e tratteggio della tabella qui sopra.
- [ ] **Step 2:** Test `RefactorTactics.AreaBorder.Component.ShowsOneInstancePerGivenCell`: in un mondo di test con una mappa di raggio 2, tre istanze date a `Show` diventano tre istanze, coi cinque dati per istanza scritti come dicono i Global Constraints (compreso `+64` per il tratteggio).
- [ ] **Step 3:** Test `...Component.SkipsCellsTheMapDoesNotHave` (Review Focus 2).
- [ ] **Step 4:** Test `...Component.LayersSitAtTheirOwnHeight` (Review Focus 1): su una mappa a due piani le istanze di `(0,0,0)` e `(0,0,1)` hanno le quote dei rispettivi piani; sulla stessa cella `Base` sta sotto `Cells`, che sta sotto `Cursor`.
- [ ] **Step 5:** Test `...Component.FrontInstancesGoToTheFrontFamily`: tre istanze, due con `bFront`: la famiglia dietro ne ha una, quella davanti due.
- [ ] **Step 6:** Test `...Component.SurvivesRebuildAndStrategicView` (Review Focus 4).
- [ ] **Step 7:** Test `...Presenter.UnchangedSlotDoesNotRecompose`: `SetSlot` due volte con la stessa richiesta: la seconda risponde `false`.
- [ ] **Step 8:** Implementa; build; `RefactorTactics.AreaBorder+RefactorTactics.HexMapActor+RefactorTactics.HexMap+RefactorTactics.AreaOverlay` verdi (fra questi `AreaOverlay.DrawingMutatesNothing`, `AreaOverlay.PreviewAddsNoActorPerCell` e il test di `RTHexMapTests.cpp` che vuole un solo ISM collidibile).
- [ ] **Step 9:** Commit `feat(4192): le due famiglie di istanze delle aree e il loro presentatore`.

### Task 6: Le aree di pianificazione passano alle famiglie nuove

**Files:** modifica `Map/Actor/RTHexMapActor.cpp` (i setter `SetPreviewReachableCells`, `SetPreviewRangeCells`, `SetPreviewFocusAreas`, `SetPreviewSkillRange`, `SetPreviewHitCells`, `SetPreviewPath`, `SetPreviewAttack`, `SetPreviewSightBlock`, `ClearPlanningPreview`, `DrawPlanningPreview`, `HasAnythingToDraw`); riscrivi `Preview.DrawsWithDebugDrawingOff` (`Tests/Map/Actor/RTPreviewLineBatcherTests.cpp`); crea `Tests/Map/Overlay/RTAreaBorderPlanningTests.cpp`.

**Interfaces — Consumes:** `FRTAreaBorderPresenter::SetSlotArea`.

Ogni setter aggiunge una riga: `AreaBorderPresenter.SetSlotArea(<slot>, <area>)`. Da `DrawPlanningPreview` escono i blocchi che disegnavano i contorni per cella di: raggiungibili e oltre il residuo, i due fuochi, i tre stati della portata, la portata armata, le celle del percorso, la cella d'origine dell'attacco, la cella che blocca il tiro, le celle colpite e quelle alleate. Restano: la polilinea del percorso, la linea di mira, la linea del tiro interrotto, e tutto il playback (impronta, strutture, traiettorie, segni).

`HasAnythingToDraw` perde **solo** i termini delle aree migrate. Restano `bCellOverlay`, `VeilCellsInTransition > 0`, le linee rimaste e i canali del playback: la dissolvenza del velo e il playback non devono fermarsi.

- [ ] **Step 1:** Test `RefactorTactics.AreaBorder.Planning.AreasBecomeInstancesNotLines`: per ciascuno di `SetPreviewReachableCells`, `SetPreviewRangeCells`, `SetPreviewFocusAreas`, `SetPreviewSkillRange` e `SetPreviewHitCells`, chiamato da solo con celle note: le famiglie hanno le istanze attese dalla tabella del Task 5, e i due line batcher del mondo (normale e in primo piano) ricevono **zero** segmenti dopo un Tick. Conteggio dei segmenti come in `RTPreviewLineBatcherTests.cpp`.
- [ ] **Step 2:** Test `...Planning.LinesThatStayAreCountedExactly`: `SetPreviewPath` con `N` celle dà `N - 1` segmenti (la sola polilinea) e `N` istanze nello strato `Cells`; `SetPreviewAttack` valido dà i soli segmenti della linea di mira (uno se l'origine è certa, otto se è prevista) e un'istanza per la cella d'origine; `SetPreviewSightBlock` dà la sola linea del tiro e un'istanza per la cella che blocca.
- [ ] **Step 3:** Test `...Planning.AreasAloneDoNotTurnTheTickOn`: con le sole raggiungibili impostate `IsActorTickEnabled()` è falso; con la dissolvenza del velo in corso resta vero.
- [ ] **Step 4:** Test `...Planning.ClearLeavesNoPlanningInstances` (Review Focus 3): dopo alcuni setter e `ClearPlanningPreview` le famiglie non hanno istanze di pianificazione.
- [ ] **Step 5:** Riscrivi `Preview.DrawsWithDebugDrawingOff` sul contratto nuovo: col disegno di debug spento arrivano ancora la polilinea del percorso e la linea di mira, e le famiglie hanno le loro istanze.
- [ ] **Step 6:** Implementa; build; `RefactorTactics.AreaBorder+RefactorTactics.Preview+RefactorTactics.HexMapActor+RefactorTactics.HexMap+RefactorTactics.AreaOverlay+RefactorTactics.Playback` verdi.
- [ ] **Step 7:** Commit `feat(4192): le aree di pianificazione si disegnano a istanze, non a segmenti`.

### Task 7: Il cursore

**Files:** `Map/Actor/RTHexMapActor.cpp` (`SetHoveredCell`, blocco dell'hover in `DrawPlanningPreview`), `Map/Actor/RTOverlayPalette.cpp` (`ColorFor(Hover)`), test in `RTAreaBorderPlanningTests.cpp`.

Il cursore diventa lo slot `Hover`. Il suo colore passa dal giallo a un neutro: prova `FColor(255, 255, 255)`; se `RefactorTactics.AreaOverlay.PaletteIsDistinguishable` lo rifiuta, scegli il neutro più vicino che passa e scrivi nel commit la distanza dal più vicino. Il giallo si libera per la fascia del silenzioso (fetta 2). `SetHoveredCell` gira a ogni frame: `SetSlot` non ricompone se la cella non cambia.

⚠️ **La linea del percorso resta nel colore di `PathTrace`.** Il mockup la mostrava bianca, ma un bianco coinciderebbe col cursore e il gate della palette ([D-364](../../decisions/RT_PDR_00_Decision_Log.md)) non ammette due significati con lo stesso colore. La tavola del Task 12 la mostra all'autore. *Lettura dell'assistente.*

- [ ] **Step 1:** Test `RefactorTactics.AreaBorder.Hover.CursorCellGetsAThinNeutralBorder`: `SetHoveredCell(Cell, true)` dà un'istanza nello strato `Cursor`, davanti, con `ThinMask == 63` nel colore di `Hover`; `SetHoveredCell(Cell, false)` la toglie; chiamarlo due volte con la stessa cella non ricompone.
- [ ] **Step 2:** Test `...Hover.CursorDoesNotEraseAHitCell`: con una cella colpita impostata e il cursore sulla stessa cella, le famiglie hanno sia l'istanza della cella colpita sia quella del cursore.
- [ ] **Step 3:** Implementa; togli il blocco giallo dell'hover da `DrawPlanningPreview`; build; `RefactorTactics.AreaBorder+RefactorTactics.AreaOverlay+RefactorTactics.Pointer+RefactorTactics.PlayerInput` verdi.
- [ ] **Step 4:** Commit `feat(4192): il cursore ha il bordo di cella neutro`.

### Task 8: L'arco di vista del nemico

**Files:** `Map/Actor/RTHexMapActor.cpp` (il blocco dell'arco nel `Tick`), `Tests/Perception/RTEnemyArcTests.cpp`.

L'arco arriva come `FRTEnemyArcView { TArray<FRTExposedEdge> Edges; FColor Color; }`. Diventa lo slot `EnemyArc`: `Edges` = i lati dell'arco più i sei lati della cella del nemico, `ColorOverride` = `Color`, `FrontOverride` = `true`, `Area.Meaning = Vision`, `Area.Cells` = le celle distinte di quei lati. Si aggiorna quando l'arco cambia, non a ogni Tick. Resta in primo piano, come vuole D-521; cambia la sua resa, da linea a bordo luminoso, ed è scritto in `D-635`.

- [ ] **Step 1:** Riscrivi `EnemyArc.FocusChoosesWhichArcsAreDrawn` e `EnemyArc.OwnerCellIsOutlinedInTheArcColor`: oggi contano le linee rosse nel batcher in primo piano; sul contratto nuovo contano le istanze della famiglia davanti, coi bit dei lati e il colore dell'arco. Tolto l'arco, zero istanze.
- [ ] **Step 2:** Implementa; il `Tick` non disegna più l'arco a linee; build; `RefactorTactics.EnemyArc+RefactorTactics.AreaBorder` verdi.
- [ ] **Step 3:** Commit `feat(4192): l'arco di vista del nemico passa alla famiglia davanti`.

### Task 9: La griglia spenta di norma

**Files:** crea `Map/Overlay/RTGridVisibility.h`, `Tests/Map/Overlay/RTGridVisibilityTests.cpp`; modifica `Map/Actor/RTHexMapActor.h/.cpp`; adegua i test che fissano la resa di oggi.

**Interfaces — Produces:**

```cpp
namespace RTGridVisibility
{
    /**
     * Canale 3 dell'anello della griglia: 1 lo nasconde, 0 lo mostra.
     * bVeiled: la board ha un velo di conoscenza applicato. ObservedFactor: 1 cella in vista, 0 ricordata.
     */
    inline float RingMaskFor(bool bVeiled, float ObservedFactor, bool bShowAll)
    { return (!bVeiled || bShowAll) ? 0.f : FMath::Clamp(ObservedFactor, 0.f, 1.f); }
}
// ARTHexMapActor
void SetGridShownEverywhere(bool bShow);
bool IsGridShownEverywhere() const;
```

`CellBorders` usa `M_HexCell`, il cui canale 3 è la trasparenza (`OpacityMask = Dither(1 - x)`, `Map/Actor/RTLayerPresentation.h`; che l'ingresso sia collegato lo misura `HexMap.CellMaterialCompilesTheInputsItHasConnected`).

- **Mappa senza velo → griglia accesa.** Una board mai velata (la modalità Hex Map dell'Editor, molti test) non ha celle "in vista" o "ricordate": lì la griglia resta com'è oggi. In partita il velo c'è sempre.
- **Uno scrittore, cinque momenti.** Un solo metodo privato dell'attore scrive il canale 3 di un'istanza di `CellBorders`, chiamando `RingMaskFor`. Lo chiamano: la nascita dell'istanza in `RebuildInstances`; lo scatto del velo (`VeilInstances`); il passo della dissolvenza (`AdvanceVeilFamily`); `ApplyLayerPresentation`, che smette di azzerare il canale 3 sui bordi e continua ad azzerare il 4 e il 5; `SetGridShownEverywhere`, che riscrive tutte le istanze con un solo `MarkRenderStateDirty`.
- `BorderDisplayFactor` si svuota a ogni ricostruzione e nasce a `-1`: lo scrittore tratta "indice assente" e "valore negativo" come cella non ancora giudicata dal velo, cioè `bVeiled` falso per quell'istanza.
- Il colore dell'anello non cambia: la pianta di D-520 resta, e il codice di piano di D-508 si vede quando il tasto accende tutto. `SetCellBordersVisible` resta com'è.

- [ ] **Step 1:** Test `RefactorTactics.Grid.RingMaskFollowsVeilObservationAndKey`: senza velo `0` qualunque sia il fattore; con velo `RingMaskFor(true, 1, false) == 1`, `(true, 0, false) == 0`, `(true, 0.5, false) == 0.5`, `(true, 1, true) == 0`; fattori fuori intervallo chiusi in `[0, 1]`.
- [ ] **Step 2:** Test `RefactorTactics.HexMapActor.GridRingIsMaskedOnObservedCellsUnderAVeil`: board con un velo applicato che osserva tutte le celle: le istanze di `CellBorders` coprono ancora ogni cella e il canale 3 vale `1` su tutte; dopo `SetGridShownEverywhere(true)` vale `0`; dopo `false` torna `1`. Dopo `RebuildInstances` vale lo stesso.
- [ ] **Step 3:** `HexMapActor.GridIsOnByDefaultAndCoversEveryCell` resta valido su una board senza velo: aggiungi l'asserto che il canale 3 è `0`.
- [ ] **Step 4:** Adegua `Veil.RememberedCellIsAPlan`, `PlanDissolvesBetweenObservedAndRemembered` e `RememberedRingStaysAPlanWhenTheLayerChanges` (`RTPlanVeilTests.cpp`): una cella ricordata ha l'anello nel colore della pianta e canale 3 a `0`; una in vista ha canale 3 a `1`. Nel test di `RTLayerPresentationTests.cpp` che asserisce i canali `3..5` a zero sui bordi, il 3 diventa la maschera attesa e il 4 e il 5 restano a zero. Per ogni test rinominato: `git grep -n "<vecchio nome>" -- docs tools` e `python tools/bot-competence/check.py --check`.
- [ ] **Step 5:** Implementa; build; `RefactorTactics.Grid+RefactorTactics.HexMapActor+RefactorTactics.HexMap+RefactorTactics.Veil+RefactorTactics.Visibility+RefactorTactics.AreaOverlay` verdi.
- [ ] **Step 6:** Commit `feat(4192): la griglia e' spenta di norma sulle celle in vista`.

### Task 10: Il tasto che accende tutti i bordi

**Files:** `Player/RTPlayerController.h/.cpp` (una `UInputAction` nuova in `BuildInputMappings`, il binding in `BindInputActions`, due gestori di una riga); `Map/Actor/RTHexMapActor.h/.cpp` (`SetAllHexBordersShown(bool)`: chiama `SetGridShownEverywhere` e `AreaBorderPresenter.SetShowEveryCell`); test in `Tests/Player/Input/RTInputBindingTests.cpp`, che ha già l'helper per inviare `Canceled`.

Modello: `CameraModifierAction` su `LeftControl` e `RightControl`, con `Started` verso il gestore di pressione e `Completed` più `Canceled` verso quello di rilascio. Il tasto è `EKeys::LeftShift` (libero in `Source/` e in `Config/`; `Alt` è escluso perché `Alt+Invio` passa a schermo intero e `Invio` è Conferma).

- [ ] **Step 1:** Test `RefactorTactics.PlayerInput.ShiftShowsEveryHexBorderWhileHeld`: l'evento `Started` porta `IsGridShownEverywhere()` a vero e la composizione a `bShowEveryCell`; `Completed` li riporta a falso.
- [ ] **Step 2:** Test `...PlayerInput.CanceledGridKeyAlsoReleases` (Review Focus 5): dopo `Started`, l'evento `Canceled` riporta a falso.
- [ ] **Step 3:** Implementa; build; `RefactorTactics.PlayerInput` verde, `HotkeysDoNotCollide` compreso.
- [ ] **Step 4:** Commit `feat(4192): tenendo premuto Maiusc compaiono tutti i bordi`.

### Task 11: Documenti e registro PIE

**Files:** `docs/technical/test-manuali-pie.md`, `docs/technical/systems/progettazione-hud.md`, `docs/technical/tooling/asset-map.md`.

- [ ] **Step 1:** Registro PIE. Riscrivi la parte di resa delle voci che descrivono ciò che è cambiato: `PIE-GRID-CONFINE`, `PIE-PREVIEW-AREA`, `PIE-PREVIEW-PERSIST`, `PIE-HUD-FUOCO-AREE`, `PIE-HUD-SKILL-PORTATA`, `PIE-FOG-ARCO`, `PIE-FOG-PIANTA`, `PIE-CP1.4`, `PIE-HEXPLAY-1`, `PIE-CAM-PIANI-ATTIVO`. Cerca le altre con `grep -nE "contorno|griglia|bordo (nero|bianco|viola)|anello" docs/technical/test-manuali-pie.md`. Chi aveva un verdetto sulla resa vecchia torna ⏳ con la data e il motivo. Aggiungi la voce `PIE-MAP-AREE-BORDO`, con la domanda del design: "senza tasto, capisci dove finisce ogni area? col tasto, conti le celle?". Aggiorna "Stato in numeri" in testa al registro.
- [ ] **Step 2:** `progettazione-hud.md`: nota datata in §3 (la griglia) e in §4.2 (le aree a bordo, i bordi a contesto, il tasto), con il rimando a `D-635` e a questa issue. Non riscrivere le sezioni: la nota dice che cosa è cambiato e dove.
- [ ] **Step 3:** `node tools/radar/doc-links.ts --check`, `doc-tables.ts --check`, `doc-coherence.ts --check`, `issue-refs.ts --check`: exit 0. `pie-verdict-age.ts --check`: stessa uscita della base, più le voci tornate ⏳ in questo task.
- [ ] **Step 4:** Commit `docs(4192): registro PIE e spec dell'HUD dopo le aree a bordo`.

### Task 12: Suite intera, mutazioni, tavola

- [ ] **Step 1:** `wc -l Source/RefactorTactics/Map/Actor/RTHexMapActor.cpp`: non più righe di quelle annotate al Task 5. `python tools/architettura/mappa-cartelle.py --verifica`: la stessa uscita della base, senza cartelle nuove sopra il tetto.
- [ ] **Step 2:** Suite `RefactorTactics` intera sulla testa del ramo, con il binario costruito da quell'albero: `PASS`, o ogni rosso spiegato e confrontato con `main`.
- [ ] **Step 3:** Mutazioni, una alla volta; ciascuna deve far diventare rosso almeno un test, poi si ripristina: (a) `MasksFromExposedEdges` usa l'indice della direzione invece di `EdgeIndexForDirection`; (b) in `Compose` vince la priorità più bassa; (c) in `Compose` il lato condiviso si accende su entrambe le celle; (d) `RingMaskFor` ignora `bShowAll`; (e) `RingMaskFor` ignora `bVeiled`; (f) un setter non chiama più `SetSlotArea`; (g) la ricetta scrive le normali nell'ordine delle direzioni. Annota quale test prende ciascuna.
- [ ] **Step 4:** Tavole per l'autore, da una finestra di gioco su banchi esistenti: (A) ventaglio senza tasto; (B) la stessa scena col tasto; (C) portata a tre stati e celle colpite di una skill, con un alleato nell'area; (D) le aree di movimento di un nemico a fuoco e il suo arco di vista; (E) una cella ricordata accanto a celle in vista, con la griglia spenta; (F) una mappa a più piani, senza e col tasto. Catture e nota di seduta in `docs/technical/evidence/4192/`. La voce `PIE-MAP-AREE-BORDO` resta ⏳ finché l'autore non giudica.
- [ ] **Step 5:** Commit `test(4192): suite, mutazioni e tavole delle aree a bordo`.
