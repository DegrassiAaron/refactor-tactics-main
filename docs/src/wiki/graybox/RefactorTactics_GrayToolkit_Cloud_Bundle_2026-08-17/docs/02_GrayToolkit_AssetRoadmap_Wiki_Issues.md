# REFACTORTACTICS — Gray Toolkit / Asset Roadmap Consolidation
## Handoff operativo per Claude Code / Claude Cloud
**Data:** 2026-08-17  
**Scopo:** consolidare le decisioni emerse sulla scala, il Graybox/Gray Toolkit, il Cell Placement Volume, la roadmap asset e le regole per import/scaling/pivot; verificare e correggere i materiali grafici; aggiornare issue/epic/documentazione/wiki; usare le immagini allegate nelle pagine wiki.

---

# 0. Materiale grafico da usare

## Immagini correnti
1. **Infografica**
   - File: `/mnt/data/RT_GrayToolkit_AssetRoadmap_Infographic_v1.png`
   - Titolo consigliato: `RT Gray Toolkit & Asset Roadmap`

2. **UML / diagramma concettuale**
   - File: `/mnt/data/RT_GrayToolkit_UML_v1.png`
   - Titolo consigliato: `RT Gray Toolkit UML`

> Prima di pubblicarle/committarle, verifica se nel repository esiste già una cartella canonica per immagini wiki / docs / generated visuals.  
> Se sì, spostale/copiati lì e usa il naming della repo.

---

# 1. Regola assoluta: repository first

Prima di toccare qualunque file o tracker:

1. `git fetch --prune`
2. verifica branch/worktree/HEAD
3. leggi `CLAUDE.md`, `AGENTS.md` e istruzioni repo
4. verifica la versione UE realmente bloccata
5. leggi:
   - Decision Log / ADR
   - Feature Registry
   - Roadmap
   - Asset lane / editor lane / issue map
   - wiki/documentazione esistente
6. cerca issue/epic aperte e chiuse relative a:
   - graybox / gray toolkit
   - scale / import scale / pivots
   - cover / walls / doors
   - cell footprint / placement / volume
   - asset roadmap
   - character art pipeline
   - environment prop pipeline
7. classifica ogni punto di questo handoff come:
   - `AS_BUILT`
   - `PARTIAL`
   - `PLANNED`
   - `BLOCKED`
   - `SUPERSEDED`
   - `MISSING`

La repo live vince sempre se c’è conflitto.

---

# 2. Consolidamento delle decisioni

## 2.1 World scale / metrica base

Baseline da consolidare nella documentazione, salvo conflitti con `main`:

- `1 UU = 1 cm`
- **Hex side = 150 cm**
- `flat-to-flat ≈ 260 cm`
- `vertex-to-vertex = 300 cm`
- **Reference Human = 180 cm**
- **Unit visual footprint = 70–80 cm**
- le unità graybox devono restare **più piccole della cella** e non dominarla visivamente

## 2.2 Principi visuali

Regola da mantenere:

- **Geometria 3D = che cosa è**
- **Colore / accent = stato o famiglia funzionale**
- **Trasformazione fisica = stato meccanico**
- **Overlay = stato ambientale**
- **Mesh/collision graybox ≠ authority gameplay**

## 2.3 Cell Placement Volume

Decisione chiave v0.1:

- introdurre un **Cell Placement Volume** come guida Editor/debug
- forma = **prisma esagonale**
- `Outer Volume = 100%`
- `Safe Volume ≈ 90%`
- `Standard Prop Envelope = 120–160 cm` come envelope consigliato per `CellBound` standard
- guide verticali:
  - `MAX = 1.00C`
  - `STRUCTURAL = 0.85C`
  - `STANDARD = 0.55C`
  - `LOW = 0.28C`

## 2.4 Placement taxonomy

Da consolidare:

- `CellBound`
- `EdgeBound`
- `SurfaceBound`
- `EditorOnly`
- `MultiCell (future)`

## 2.5 Asset rules

Da consolidare:

- `Import scale = 1.0`
- `Actor scale = 1,1,1`
- `Pivot bottom-center` per `CellBound`
- `Pivot segment-center` per `EdgeBound`
- art replacement deve rispettare:
  - footprint
  - pivot
  - snap
- le mesh non diventano authority del gameplay

## 2.6 Kit v0.1

Catalogo v0.1 da mappare nella roadmap e/o issue:

- `Cell`
- `CellPlacementVolume`
- `Unit`
- `Floor`
- `Wall`
- `CoverLow`
- `CoverHigh`
- `Door`
- `Rubble`
- `WallBroken`
- `Ramp`
- `Platform`
- `Water`
- `Ice`
- `Valve`
- `Generator`
- `HazardTank`
- `Relay`
- `SpawnMarker`

## 2.7 Roadmap asset / maturità

Sequenza concettuale da preservare anche se il mapping sulle release reali va riconciliato con la roadmap canonica:

- `v0.1` Core Map
- `v0.2` Environment
- `v0.3` 3D Map / Verticality
- `v0.4` Interactive Map
- `v0.5` Tactical Devices
- `v0.6` Destruction & Debris
- `v0.7` Perception & Info Debug
- `v0.8` Objective Kit
- `v0.9` Production Graybox
- `v1.0` Asset Contract Freeze

## 2.8 Lane art

Da consolidare come tracking leggibile:

### Character Art
- `C0 Cylinder`
- `C1 Silhouette`
- `C2 Gameplay Proxy`
- `C3 Blockout`
- `C4 Production Mesh`
- `C5 Textured`
- `C6 Final Art`

### Environment Art / Prop Art
- `E0 Primitive`
- `E1 Gameplay Graybox`
- `E2 Art Blockout`
- `E3 Production Mesh`
- `E4 Textured`
- `E5 Final`

---

# 3. Audit grafico — controlli e correzioni richieste

## 3.1 Infografica — stato
**Esito:** buona e quasi pronta, con **solo piccoli fix consigliati**.

### Verifiche positive
- la scala di riferimento è coerente con quanto deciso
- il Cell Placement Volume è rappresentato bene
- il kit v0.1 è coerente
- la roadmap asset è coerente
- le lane art sono coerenti
- le regole asset principali sono corrette

### Piccoli fix consigliati
1. **Naming consistency**  
   Scegliere e usare in modo coerente uno solo tra:
   - `Gray Toolkit`
   - `Graybox Toolkit`
   - `Graybox Kit`

   Se la repo/documentazione usa già un termine canonico, uniformare il titolo e i sottotitoli.

2. **SurfaceBound description**  
   La descrizione attuale è utilizzabile ma leggermente generica.  
   Valutare una formulazione più aderente al nostro modello, ad esempio:
   - `Si applica come overlay/stato alla superficie della cella (Water, Ice, Wet, ecc.).`

3. **Formatting numerico**
   - `Actor scale = 1,1,1` può restare, ma se la repo usa notazione tecnica più pulita valutare:
     - `Actor scale = (1,1,1)` oppure
     - `Actor scale = 1.0 / 1.0 / 1.0`

Questi non sono blocchi, sono **micro-fix di rifinitura**.

---

## 3.2 UML — stato
**Esito:** utile e quasi corretta, ma qui ci sono **2 correzioni importanti + 1 rifinitura**.

### Correzione importante #1 — milestone v0.9
Nel blocco `Asset Roadmap`, il nodo **v0.9** sembra riportare ancora `v0.8` nella label del cerchio/titolo del milestone.

**Da correggere in:**
- `v0.9 → Production Graybox`

### Correzione importante #2 — relazione CellPlacementVolume
Nel diagramma la relazione di `CellPlacementVolume` sembra puntare/validare soprattutto i `Prefab Assets v0.1`.

Concettualmente, la versione più corretta è:

- `CellPlacementVolume` **valida il placement contract / footprint contract**
- il `placement/asset contract` viene poi **applicato ai prefab**

Quindi la relazione da privilegiare visivamente è:

```text
CellPlacementVolume
    -> validates footprint & scale
    -> Asset Contract
Asset Contract
    -> applied to
    -> Prefab Assets
```

### Rifinitura consigliata #3 — naming consistency
Anche qui uniformare:
- `Gray Toolkit`
- `Graybox Toolkit`
- `Graybox Kit`

### Altre verifiche
Il resto del diagramma è buono come concetto:
- `MapState / Gameplay State`
- `PlacementClass`
- `Asset Contract`
- `Prefab Assets v0.1`
- `Character Art Roadmap`
- `Environment / Prop Roadmap`

---

# 4. Cosa fare adesso nel repository

## 4.1 Issue / Epic / tracking
Claude deve:

1. **non creare duplicati**
2. trovare gli owner esistenti
3. aggiornare, se esistono, Epic/issue relative a:
   - Graybox Kit / Gray Toolkit
   - Cell Placement Volume
   - Unit scale baseline
   - Cover / Wall / Door readability
   - Asset Roadmap
   - Character Art Roadmap
   - Environment / Prop Roadmap
   - Wiki / documentation for asset rules

### Possibili cluster di issue
Se non esistono owner adeguati, creare o proporre issue del tipo:

- `gray-toolkit-v01-consolidation`
- `cell-placement-volume-v01`
- `unit-scale-baseline-v01`
- `wall-cover-door-readability-contract`
- `asset-rules-import-scale-pivot-contract`
- `asset-roadmap-and-lane-tracking`
- `wiki-gray-toolkit-and-asset-roadmap`
- `docs-embed-gray-toolkit-images`

### Epic / macro owner
Raggruppare sotto Epic/lane esistenti invece di crearne di nuove a caso, per esempio:
- asset/graybox
- map/editor
- docs/wiki
- art pipeline

---

# 5. Documentazione da aggiornare

Aggiornare solo le source of truth reali.

## Da aggiornare / creare se mancanti
- Decision Log / ADR
- Feature Registry
- Roadmap
- Asset lane / asset tracker
- Editor lane / editor map
- Wiki / documentation pages
- eventuali generated views / dashboards / control center

## Decisioni da registrare
Se non già presenti:

1. world scale baseline
2. reference human
3. unit footprint baseline
4. Cell Placement Volume v0.1
5. placement taxonomy
6. asset rules (import scale, actor scale, pivot contract)
7. mesh not gameplay authority
8. lane art maturity model
9. art replacement contract by v1.0

---

# 6. Wiki pages da creare o aggiornare

Claude deve creare/aggiornare pagine wiki che **usino le immagini**.

## Pagina 1 — `Gray Toolkit / Graybox Kit`
Contenuto minimo:
- scopo del kit
- principi visuali
- world scale
- Cell Placement Volume
- placement taxonomy
- kit v0.1
- regole asset
- nota forte: mesh/collision non sono authority gameplay

**Embed consigliato:**
- infografica come hero image o image principale

## Pagina 2 — `Asset Roadmap`
Contenuto minimo:
- roadmap v0.1 → v1.0
- significato dei cluster
- relazione con roadmap canonica
- definizione di `Asset Contract Freeze`

**Embed consigliato:**
- infografica
- eventuale crop/thumbnail dell’area roadmap se la wiki lo supporta

## Pagina 3 — `Gray Toolkit UML`
Contenuto minimo:
- spiegazione del diagramma concettuale
- relazioni fra `MapState`, `Asset Contract`, `PlacementClass`, `CellPlacementVolume`, `Prefab Assets`
- relazione fra roadmap asset e roadmaps art

**Embed consigliato:**
- immagine UML

## Pagina 4 — `Asset Rules & Import Contract`
Contenuto minimo:
- import scale
- actor scale
- pivot rules
- placement class
- art replacement constraints
- note per asset esterni / marketplace

**Embed consigliato:**
- infografica
- eventualmente tabella sintetica presa dalla documentazione

## Pagina 5 — `Character Art & Environment Art Roadmap`
Contenuto minimo:
- lane `C0–C6`
- lane `E0–E5`
- definizione di ogni step
- mapping sulle versioni reali del progetto

---

# 7. Output finale richiesto a Claude

Restituire un report tipo:

## `GRAY TOOLKIT CONSOLIDATION REPORT`

Con almeno:

### Audit repo
- branch / HEAD
- file ispezionati
- roadmap / tracker trovati
- owner issue/epic trovati

### Consolidation
- decisioni già canoniche
- decisioni aggiunte
- conflitti trovati
- open decisions lasciate aperte

### Image audit
- fix applicati all’infografica
- fix applicati all’UML
- path finale delle immagini nel repo/docs

### Issue / Epic work
- issue aggiornate
- issue create
- epic aggiornate
- duplicati evitati
- dipendenze

### Documentation / Wiki
- pagine create o aggiornate
- immagini embeddate
- link interni aggiunti
- eventuali pagine da rigenerare

### Tracking
- roadmap aggiornata
- asset tracker aggiornato
- feature registry aggiornato
- lane/editor/scenario/test tracking allineato

### Git
- commit
- PR
- remaining blockers
- next recommended task

---

# 8. Acceptance criteria

Il lavoro è completo quando:

- [ ] le due immagini sono state verificate e corrette dove necessario
- [ ] la roadmap asset è allineata alla roadmap canonica
- [ ] i cluster asset sono tracciati in issue/epic senza duplicati
- [ ] la documentazione è aggiornata
- [ ] le pagine wiki usano le immagini
- [ ] il Cell Placement Volume è consolidato come concetto/documentazione/issue
- [ ] le regole asset sono documentate
- [ ] il modello `C0–C6` e `E0–E5` è tracciato
- [ ] il report finale indica chiaramente cosa è stato creato/aggiornato

---

# 9. Nota finale

La cosa importante non è solo “salvare due immagini”, ma usarle come **entry point visivi** per:

- consolidare le decisioni,
- creare/aggiornare issue ed epic,
- allineare roadmap e tracker,
- aggiornare la wiki,
- definire un **Asset Contract** stabile fino alla v1.0.
