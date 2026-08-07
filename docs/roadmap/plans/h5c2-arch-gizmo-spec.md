# Spec H5c.2 — Tool "Transizioni" con gizmo (Add-only) nell'Editor Mode hex

> **Stato**: Approvata (design) · **Data**: 2026-08-04 · **Branch**: `feat/hex-grid`
> **Fonte**: milestone H5 di [`hex-map-roadmap.md`](../hex-map-roadmap.md) (riga H5c+, "strumenti archi con gizmo");
> prosecuzione di [`h5c-paint-tool-spec.md`](h5c-paint-tool-spec.md); [`adr-0002-griglia-esagonale.md`](../../decisions/adr-0002-griglia-esagonale.md).
> Non tocca il quadrato (`feat/skeletal-units`, base di rollback).

## 1. Contesto e obiettivo

Dopo H5c.1 l'Editor Mode **Hex Map** ha i tool **Select** (lettura) e **Paint** (paint/erase a click). Le **transizioni
verticali/speciali** (`FRTHexEdge`: scale, rampe, ponti, tunnel, ascensori — collegano celle di layer diversi, altrimenti
non adiacenti) si creano ancora **solo** dal Details dell'`ARTHexMapActor` (`AddVerticalTransition` CallInEditor, con
`TransitionFrom`/`TransitionTo` digitati a mano). Inoltre **gli archi non sono visualizzati**: `RebuildInstances`
disegna solo le celle (ISM), quindi una transizione creata è invisibile nel viewport.

**Obiettivo di questa fetta**: creare una transizione **nel viewport** — click sulla cella `From`, poi un **gizmo di
traslazione** per scegliere `To` (con snap esagonale, anche su un altro layer), Commit — e **vedere** le transizioni
esistenti disegnate. Toglie la dipendenza dal Details per gli archi e introduce la visualizzazione degli archi.

## 2. Decisioni di design (fissate col dev)

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | **Gizmo dall'inizio** (non split "connect a due click prima") | Scelta esplicita del dev: la fetta H5c.2 include il gizmo (`UCombinedTransformGizmo` + `UTransformProxy`), occasione didattica UE. |
| D2 | Interazione **click From → gizmo To → Commit** | Un solo gizmo, un click, una transazione per commit; più piccola/robusta del due-handle. |
| D3 | **Add-only** in questa fetta | Remove/edit di archi esistenti via tool = follow-up (YAGNI). Il Remove resta sul `RemoveVerticalTransition` CallInEditor esistente. |
| D4 | **Rendering degli archi nel tool** (`Render`), visibile mentre il tool è attivo | Prerequisito minimo per vedere ciò che si crea; il rendering persistente (sempre visibile) è follow-up. |
| D5 | **Riuso via metodo runtime** `AddTransitionData` (estratto da `AddVerticalTransition`) | CLAUDE.md «non duplicare»: il tool scrive via l'API dell'actor, come `PaintCellData` in H5c.1 (invariante #1). |

## 3. Architettura

### 3.1 Nuovo tool (modulo editor)

`Source/RefactorTacticsEditor/Private/Tools/RTHexArchTool.{h,cpp}`:

- **`URTHexArchToolBuilder : UInteractiveToolBuilder`** — factory (come Select/Paint).
- **`URTHexArchToolProperties : UInteractiveToolPropertySet`** — palette:
  - `ERTHexTransitionKind Kind = Stair`, `int32 Cost = 2` (ClampMin 0), `bool bBidirectional = true`.
  - readout `VisibleAnywhere`: `FRTCellId From`, `bool bHasFrom`, `FRTCellId To`, `bool bToValid`.
  - due `UFUNCTION(CallInEditor)`: **`Commit()`** e **`ClearArch()`** (bottoni nel pannello del tool). Il property set
    tiene un `TWeakObjectPtr<URTHexArchTool> WeakTool`, impostato in `Setup()` (`Properties->WeakTool = this`) **prima**
    che i bottoni funzionino (altrimenti sono no-op silenziosi) — pattern dei Modeling tools.
- **`URTHexArchTool : USingleClickTool`** — `OnClicked` imposta `From` e spawna il gizmo; il gizmo pilota `To`;
  `Render` disegna archi esistenti + preview; `Commit`/`ClearArch` chiamati dai bottoni.

### 3.2 Gizmo (UE 5.8 — API verificata contro gli header)

- **Nessuna registrazione extra** (correzione da review): i default gizmo sono già registrati d'ufficio dal tools
  context di `UEdMode` (`RegisterDefaultGizmos()` via `EdModeInteractiveToolsContext`), che è la precondizione di
  `CreateCustomTransformGizmo`. **Non** serve `RegisterTransformGizmoContextObject` (percorso indipendente) → nessun
  override `Exit()` per la registrazione.
- Su **From pick** (`OnClicked`): se esiste già un gizmo pendente, **distruggerlo prima** (no duplicati); poi
  `UTransformProxy* Proxy = NewObject<UTransformProxy>(this)`; `Proxy->SetTransform` al centro-mondo di `From`;
  `UCombinedTransformGizmo* Gizmo = GetToolManager()->GetPairedGizmoManager()->CreateCustomTransformGizmo(
  ETransformGizmoSubElements::TranslateAllAxes, this)`; `Gizmo->SetActiveTarget(Proxy, nullptr)` (nullptr = GizmoManager
  come transaction provider); bind `Proxy->OnTransformChanged` (firma **due parametri** `(UTransformProxy*, FTransform)`)
  → `OnGizmoMoved`.
- **`OnGizmoMoved(UTransformProxy*, FTransform)`**: dal world del proxy → `Layer = URTHexLibrary::WorldToLayer(worldZ,
  OriginZ, LayerHeight)` e `To = URTHexLibrary::WorldToAxial(worldXY, Origin, HexSize, Layer)`; **ri-snappo** il proxy al
  centro-mondo di `To`. Poiché `UTransformProxy::SetTransform` **ri-emette** `OnTransformChanged`, serve la guardia di
  re-entrancy `bSnapping`. *(Alternativa valutata: snap a fine drag via `UTransformProxy::OnEndTransformEdit` +
  `UCombinedTransformGizmo::ReinitializeGizmoTransform`, oppure snap "solo preview" col gizmo libero; il comportamento
  di snap durante il drag va verificato a mano nello smoke test.)*
- **Teardown (obbligatorio)**: `URTHexArchTool::Shutdown(EToolShutdownType)` chiama `GetToolManager()
  ->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this)` e azzera proxy/stato; anche Commit/ClearArch distruggono il
  gizmo pendente. Evita gizmo orfani al cambio tool o all'uscita dal mode.

### 3.3 Rendering degli archi

`URTHexArchTool::Render`: per ogni `FRTHexEdge` dell'asset disegna `AxialToWorld(From)`→`AxialToWorld(To)` come linea
colorata per `Kind` (mappa `Kind`→`FColor`) con una piccola freccia verso `To`; più marker `From` (verde) e `To` (blu)
e la linea di preview durante il drag del gizmo. *(Visibile mentre il tool è attivo; rendering persistente = follow-up.)*

### 3.4 Refactor runtime (DRY)

`Source/RefactorTactics/Map/RTHexMapActor.{h,cpp}`:

- `void AddTransitionData(const FRTCellId& From, const FRTCellId& To, int32 Cost, ERTHexTransitionKind Kind, bool bBidirectional)`
  — corpo estratto da `AddVerticalTransition` (validazione `ContainsCell(From)&&ContainsCell(To)`; `FScopedTransaction`
  `#if WITH_EDITOR` + `Modify` + `AddTransition` + `MarkPackageDirty` + `RebuildInstances`). Ritorna presto con warning
  se una cella non esiste. `AddVerticalTransition()` diventa wrapper.

### 3.5 Logica pura estratta (test headless)

`Source/RefactorTactics/Map/RTHexLibrary.{h,cpp}`:

- `static int32 WorldToLayer(double WorldZ, double OriginZ, float LayerHeight)` — `LayerHeight<=0 → 0`; altrimenti
  `FMath::RoundToInt((WorldZ−OriginZ)/LayerHeight)`. Pura, deterministica, testabile. Tie-break esplicito:
  `RoundToInt` = `floor(x+0.5)` (i .5 arrotondano verso +∞: `+0.5→+1`, `−0.5→0`, `−1.5→−1`).

### 3.6 Registrazione tool

- `RTHexEditorModeCommands.{h,cpp}`: comando `ArchTool` (ToggleButton) nella palette `NAME_Default`.
- `RTHexEditorMode.cpp` `Enter()`: `RegisterTool(Commands.ArchTool, TEXT("RTHexArchTool"), NewObject<URTHexArchToolBuilder>(this))`.
  Select resta il tool attivo di default. *(Nessuna registrazione gizmo manuale — vedi §3.2.)*

## 4. Flusso (creazione di una transizione)

1. Tool **Arch** attivo → `Render` mostra gli archi esistenti.
2. **Click** su una cella → `From` (via `ResolveClickedCell`), spawn gizmo al centro di From.
3. **Drag** gizmo → `OnGizmoMoved` → `WorldToLayer` + `WorldToAxial` → `To` (snap a cella, anche altro layer) + preview.
4. **Commit** (bottone) → `Actor->AddTransitionData(From, To, Cost, Kind, bBidirectional)` → arco creato e disegnato;
   `ClearArch`/Commit rimuovono il gizmo e azzerano lo stato pendente.
5. **Undo/Redo**: una `FScopedTransaction` per commit (dentro `AddTransitionData`), coerente con H2/H4/H5c.1.

## 5. Milestone (piccole e compilabili; la fetta include il gizmo)

| ID | Contenuto | Done quando |
|----|-----------|-------------|
| **H5c.2a** Render + refactor | `ARTHexMapActor::AddTransitionData` (+ `AddVerticalTransition` wrapper); `URTHexLibrary::WorldToLayer` (+ test `RefactorTactics.Hex.WorldToLayer`); tool "guscio" `URTHexArchTool` con **render degli archi** (nessun gizmo) + comando/registrazione | Build Editor+Game verdi; test hex verdi (incl. nuovo); nel viewport si vedono le transizioni esistenti |
| **H5c.2b** Gizmo | `OnClicked`→From+gizmo; `OnGizmoMoved`→snap To+preview; bottoni Commit/ClearArch → `AddTransitionData`; registrazione gizmo context nel mode | Build Editor verde; nel viewport: click From → gizmo → drag su To (anche altro layer) → Commit crea la transizione (visibile); Undo coerente |

## 6. Testing e Definition of Done

- **Logica pura → test headless**: `RefactorTactics.Hex.WorldToLayer` in `Source/RefactorTactics/Tests/RTHexTests.cpp`
  (in `RTHexTests.cpp`, dove vivono i test di `URTHexLibrary`): `LayerHeight<=0→0`; Z al centro di un layer → quel
  layer; **Z a metà tra due** → tie-break `floor(x+0.5)` fissato esplicitamente (es. `Z=OriginZ+1.5*LayerHeight → 2`,
  `Z=OriginZ−0.5*LayerHeight → 0`); Z negativo → layer negativo. `AddTransition`/validator già coperti da H4.
- **Editor-bound (non headless, dichiarato)**: gizmo/tool/render → nuove voci manuali in
  [`test-manuali-pie.md`](../../technical/test-manuali-pie.md):
  - `PIE-HEX-MODE-E` — crea transizione via gizmo (click From, drag su To anche su altro layer, Commit → arco creato e disegnato; Undo lo rimuove);
  - `PIE-HEX-MODE-F` — render archi esistenti + `ClearArch` annulla l'arco pendente senza scrivere.
- **DoD applicabile**: build Editor+Game verdi; nessun nuovo warning non spiegato; nessuna dip. editor nel runtime
  packaged; roadmap H5 aggiornata; limiti dichiarati; invarianti #1 (editor scrive solo dati d'asset) e #4
  (determinismo: `WorldToAxial`/`WorldToLayer` deterministici su interi) verificati.

## 7. Invarianti e vincoli rispettati

- **Editor non decide gameplay** (#1): il tool scrive solo dati d'asset via `AddTransitionData`.
- **Determinismo** (#4): nessuna logica di turno toccata; snap deterministico (`WorldToLayer`/`WorldToAxial` a interi).
- **Undo/Redo**: ogni modifica passa da `Modify()`/`FScopedTransaction`.
- **Separazione runtime/editor**: gizmo/ITF confinati nel modulo `RefactorTacticsEditor`; il refactor runtime
  (`AddTransitionData`) non aggiunge dip. editor oltre la `FScopedTransaction` già `#if WITH_EDITOR`.
- **No duplicazione**: pipeline click→cella condivisa (`RTHexEditor`); scrittura archi condivisa
  (`AddTransitionData`) tra tool e `AddVerticalTransition`.

## 8. Fuori scope (YAGNI)

Remove/edit di archi esistenti via tool (resta il `RemoveVerticalTransition` CallInEditor); rendering persistente
(sempre visibile fuori dal tool); due-handle gizmo (From e To entrambi trascinabili); hit-test/selezione di un arco
esistente; snap avanzato/vincoli sul tipo di transizione; costi per-profilo.

**Limite cosmetico dichiarato**: `AxialToWorld` posa marker/linee/gizmo sul **piano del layer**, non sulla "cima" della
cella rialzata (l'`Height` per-cella è aggiunto solo alla resa ISM in `RebuildInstances`). Accettabile per l'authoring.

## 9. Rischi

- **Ciclo di vita del gizmo** (rischio n.1 dopo la review): i default gizmo sono **auto-registrati** dal tools context
  di `UEdMode` (verificato sugli header 5.8), quindi `CreateCustomTransformGizmo` funziona senza registrazione manuale;
  il rischio residuo è il ciclo di vita — **teardown** su `Shutdown` (`DestroyAllGizmosByOwner(this)`) e **no duplicati**
  su re-click. Primo step di H5c.2b = **smoke test** ("il gizmo compare, si muove, sparisce al cambio tool") prima di
  cablare snap/commit.
- **Loop di feedback** nel ri-snap del proxy dentro `OnTransformChanged` (`SetTransform` ri-emette il delegato):
  mitigato dalla guardia `bSnapping`; alternativa a fine-drag valutata in §3.2.
- **Bottoni del property set → tool** (`TWeakObjectPtr` + `CallInEditor`): pattern pinnato sui Modeling tools in fase di piano.
- **API ITF/gizmo version-specific**: pinnate sui doc/scaffold UE 5.8 (`Engine/Source/.../InteractiveToolsFramework/BaseGizmos`,
  `Engine/Plugins/Editor/ModelingToolsEditorMode`) prima di scrivere codice.
- **Modulo editor modificato**: rebuild a editor chiuso (Live Coding non basta).
