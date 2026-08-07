# Spec H5 — Editor Mode dedicato per la mappa esagonale

> **Stato**: Approvata (design) · **Data**: 2026-08-03 · **Branch**: `feat/hex-grid`
> **Fonte**: milestone H5 di [`hex-map-roadmap.md`](../hex-map-roadmap.md); spec sorgente §9–§10 (`docs/guides/Implementazione Griglia Esagonale ed Editor Mappa.docx`); [`adr-0002-griglia-esagonale.md`](../../decisions/adr-0002-griglia-esagonale.md).
> Naturale prosecuzione di H4 (multilivello). Non tocca il quadrato (`feat/skeletal-units`, base di rollback).

## 1. Contesto e obiettivo

Dopo H0–H4 la mappa esagonale è autorevole e multilivello, ma l'authoring passa **solo** dal pannello Details
dell'`ARTHexMapActor` (`CallInEditor` + digitazione manuale delle coordinate). Il criterio **Done** di H5:

> «il workflow non dipende più principalmente dal pannello Details».

**Prima fetta prioritaria** (decisa con l'utente): **selezione a click nel viewport** (spec sorgente §10) — click →
raycast → cella sul layer attivo. È il dolore quotidiano n.1 e la base su cui poggiano tutti gli strumenti successivi.

**Vincolo di fondazione** (deciso con l'utente): **via moderna UE** — `UEdMode` + Interactive Tools Framework (ITF)
in un **modulo editor dedicato**, coerente con l'obiettivo didattico del progetto (imparare la strada corretta e a
prova di futuro), accettando un costo iniziale maggiore.

## 2. Decisioni di design

| # | Decisione | Motivo |
|---|-----------|--------|
| D1 | **Modulo editor-only dedicato** `RefactorTacticsEditor` | Le dipendenze editor (ITF/Slate/UnrealEd) non entrano nel packaged; il Build.cs runtime già annota che «il modulo Editor dedicato è rimandato a H5». |
| D2 | **`UEdMode` + ITF** (non `FEdMode` legacy, non hit-proxy manuale) | Strada moderna e a prova di futuro: mode nella toolbar, tool con input-behavior nel viewport, gizmo nativi per gli archi (fette successive). |
| D3 | Il mode **usa** il runtime, non lo duplica | Scrive dati solo via l'API esistente di `URTHexMapAsset` (`Modify`/transaction/`AddOrUpdateCell`/`AddTransition`). L'editor non decide gameplay (invariante #1). |
| D4 | **Split per fette** H5a→H5b→H5c+ | «Milestone piccole e compilabili» (metodo di progetto): il guscio prima, poi la selezione, poi il tooling ricco. |

## 3. Architettura

Nuovo modulo `Source/RefactorTacticsEditor/` (Type `Editor`), che espone:

- **`URTHexEditorMode : UEdMode`** — registra il mode (icona + `FEditorModeInfo`), gestisce `Enter()`/`Exit()`,
  possiede lo **stato di editing condiviso**: layer attivo, selezione corrente (`TArray<FRTCellId>`), asset bersaglio.
  In `Enter()` registra i tool ITF (nelle fette in cui esistono).
- **`FRTHexEditorModeToolkit : FModeToolkit`** — il pannello Slate del mode: campo *layer attivo*, readout della
  cella selezionata (coord/superficie/costo/blocco), pulsanti azione. È qui che il workflow smette di dipendere dal
  Details dell'actor.
- **Tool ITF**, uno per capability. Il primo è il tool di **selezione a click**.
- **Registrazione** nel `StartupModule()`/`ShutdownModule()` del modulo editor (mode registry; eventuale stile/comandi
  del toolkit).

L'`ARTHexMapActor` resta il visualizzatore (ISM). **Actor bersaglio** (regola esplicita): il mode opera
sull'`ARTHexMapActor` **selezionato** nel livello; se nessuno è selezionato ma ne esiste **uno solo**, usa quello; se
ce ne sono più di uno e nessuno è selezionato, il toolkit chiede di selezionarne uno (nessuna azione a vuoto).

### 3.1 Flusso della selezione a click (H5b)

1. Click nel viewport → `USingleClickInputBehavior` del tool di selezione.
2. Ray dal cursore → **raycast** contro l'ISM di `ARTHexMapActor` (già collisione `QueryOnly`/`Block`, colpibile);
   **fallback** su piano matematico alla quota del layer attivo se il ray non colpisce l'ISM.
3. `URTHexLibrary::WorldToAxial(HitLocation, Origin, HexSize, ActiveLayer)` → cella candidata (conversione **pura,
   già testata** in H0, arrotondamento cubico).
4. Lookup nell'asset → aggiorna selezione + highlight + readout nel toolkit.

## 4. Struttura file e dipendenze

- `RefactorTactics.uproject`: aggiungere il modulo `{ "Name": "RefactorTacticsEditor", "Type": "Editor",
  "LoadingPhase": "Default" }` (fase esatta verificata sui doc UE 5.8).
- `Source/RefactorTacticsEditor/RefactorTacticsEditor.Build.cs`: dipendenze editor (set esatto verificato sui doc
  5.8; attese: `Core`, `CoreUObject`, `Engine`, `InputCore`, `Slate`, `SlateCore`, `UnrealEd`, `EditorFramework`,
  `LevelEditor`, `InteractiveToolsFramework`, `EditorInteractiveToolsFramework`; + dip. sul modulo `RefactorTactics`).
- `RefactorTacticsEditor.h/.cpp` (IModuleInterface), `URTHexEditorMode.{h,cpp}`, `FRTHexEditorModeToolkit.{h,cpp}`,
  tool di selezione `.{h,cpp}`.
- Il Build.cs runtime resta invariato (mantiene la dip. `UnrealEd` solo-editor per i `CallInEditor` di H2/H4, che
  restano validi come scorciatoia finché i tool non li sostituiscono — nessuna rimozione ora, YAGNI).

## 5. Milestone (piccole e compilabili)

| ID | Contenuto | Done quando |
|----|-----------|-------------|
| **H5a** Guscio | Modulo editor `RefactorTacticsEditor`; `URTHexEditorMode` registrato; `FRTHexEditorModeToolkit` con pannello base (layer attivo, readout selezione vuota) | Compila (Editor); il mode appare nella toolbar del Level Editor e si attiva; nessun tool ancora; runtime packaged privo di dip. editor |
| **H5b** Selezione a click | Tool ITF: click viewport → raycast → `WorldToAxial` sul layer attivo → selezione + highlight + dati nel toolkit | Si seleziona una cella cliccando (senza digitare coordinate nel Details); il layer attivo del mode determina la cella; Undo/Redo coerente dove si modifica |
| **H5c+** Tooling | Paint/brush + palette · shape/fill · strumenti archi con gizmo (bridge/scala) · copia/incolla · overlay debug layer | *(fuori dalla prima consegna; fette successive con propri criteri)* |

## 6. Testing e Definition of Done

- **Logica pura**: `WorldToAxial` per-layer è già coperta da Automation (H0). Dove H5 introduce nuova logica pura
  (es. mapping hit→cella con scelta del layer, filtro selezione) si aggiungono Automation test headless.
- **Interattivo**: il grosso di H5 (mode/tool/toolkit/Slate) **non è unit-testabile headless** — verifiche in editor,
  nuove voci `PIE-HEX-MODE-*` in [`test-manuali-pie.md`](../../technical/test-manuali-pie.md). Dichiarato esplicitamente (niente
  «dovrebbe funzionare»).
- **DoD applicabile**: compila (Editor + Game); nessun nuovo warning non spiegato; nessuna dip. editor nel runtime
  packaged; roadmap aggiornata; limiti dichiarati; invarianti verificati.

## 7. Invarianti e vincoli rispettati

- **Regole in C++, editor non decide gameplay** (invariante #1): il mode scrive solo dati d'asset.
- **Determinismo** (#4): nessuna logica di turno toccata; la conversione hit→cella è deterministica (arrotondamento
  cubico su interi).
- **Undo/Redo**: ogni modifica ai dati passa da `Modify()`/transaction (come H2/H4).
- **Separazione runtime/editor**: dip. editor confinate nel nuovo modulo editor-only.

## 8. Fuori scope (YAGNI)

Editor standalone; minimappa; multi-viewport; import/export; procedural avanzato; e ogni componente H5c+ (brush,
palette, gizmo archi, copia/incolla, shape/fill) finché la prima consegna (H5a+H5b) non è verificata in editor.
La selezione multipla/raycast avanzata (Shift/Ctrl-click) oltre il click singolo è H5c.

## 9. Rischi

- **API version-specific** (registrazione mode/toolkit, set dipendenze modulo, classi ITF): pinnate sui **doc
  ufficiali UE 5.8** in fase di writing-plans, prima di scrivere codice.
- **Raycast ai bordi ISM**: mitigato dal fallback su piano del layer attivo.
- **Ampiezza di H5**: mitigata dallo split H5a/H5b/H5c+.
