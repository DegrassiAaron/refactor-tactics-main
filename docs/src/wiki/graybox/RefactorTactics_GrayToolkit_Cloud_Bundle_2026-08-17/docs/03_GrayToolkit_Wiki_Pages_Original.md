# REFACTORTACTICS — Wiki Pack: Gray Toolkit, Asset Roadmap e Asset Contract
## Handoff per Claude Code / Claude Cloud
**Data:** 2026-08-17  
**Scopo:** creare o aggiornare le pagine Wiki dedicate al Gray Toolkit, alla roadmap asset e alle regole di import/scala/placement, usando le immagini già generate e riconciliando i contenuti con la repository live.

---

# 0. Regole operative

Prima di creare o modificare pagine Wiki:

1. verificare branch/worktree/HEAD;
2. leggere istruzioni repository (`CLAUDE.md`, `AGENTS.md`, ecc.);
3. trovare la posizione canonica della Wiki/documentazione;
4. cercare pagine esistenti con contenuti equivalenti;
5. cercare Decision Log / ADR / Feature Registry / Roadmap / Asset lane;
6. riusare naming, front matter, link relativi e stile già adottati;
7. non creare pagine duplicate;
8. non trattare questo file come nuova source of truth: la repository live prevale.

Se la Wiki è generata da sorgenti Markdown, modificare le sorgenti e rigenerare le viste invece di editare output derivati.

---

# 1. Immagini da integrare

## Infografica
File locale fornito:

`/mnt/data/RT_GrayToolkit_AssetRoadmap_Infographic_v1.png`

Titolo/caption consigliato:

> **Gray Toolkit & Asset Roadmap** — scala, placement, regole asset e maturità dal graybox alla v1.0.

## UML
File locale fornito:

`/mnt/data/RT_GrayToolkit_UML_v1.png`

Titolo/caption consigliato:

> **Gray Toolkit UML** — relazioni tra stato gameplay autorevole, asset contract, placement class, prefab e roadmap art.

## Audit prima della pubblicazione
La UML corrente richiede verifica/correzione di:
- milestone `v0.9` → deve risultare chiaramente `Production Graybox`;
- relazione `CellPlacementVolume -> Asset Contract` come `validates footprint & scale`;
- naming coerente tra `Gray Toolkit`, `Graybox Toolkit`, `Graybox Kit`.

L’infografica è sostanzialmente corretta; uniformare solo naming/terminologia con il repository.

---

# 2. Struttura Wiki proposta

Creare o aggiornare queste pagine, usando i nomi canonici della Wiki se esistono già:

1. `Gray Toolkit`
2. `Asset Roadmap`
3. `Gray Toolkit UML`
4. `Asset Rules & Import Contract`
5. `Character & Environment Art Roadmap`

Aggiungere cross-link fra le cinque pagine.

---

# 3. Pagina Wiki — Gray Toolkit

## Titolo
`Gray Toolkit`

## Scopo
Definire il kit graybox 3D usato per costruire, leggere e validare le mappe di RefactorTactics dalla v0.1 fino alla sostituzione con art finale.

## Hero image
Usare:

`RT_GrayToolkit_AssetRoadmap_Infographic_v1.png`

Esempio Markdown, da adattare alla Wiki:

```md
![Gray Toolkit & Asset Roadmap](../images/RT_GrayToolkit_AssetRoadmap_Infographic_v1.png)
```

## Testo pronto

```md
# Gray Toolkit

Il **Gray Toolkit** è il linguaggio visivo e di authoring usato per costruire e validare le mappe di RefactorTactics prima dell’art finale.

Il kit non è una collezione di asset temporanei casuali. Deve funzionare come:

- linguaggio di debug tattico;
- contratto di authoring;
- test bed di leggibilità;
- base per il futuro art replacement;
- riferimento di scala e footprint.

## Principi

La grammatica visuale segue queste regole:

- **Geometria 3D = che cosa è l’oggetto**
- **Colore / accent = stato o famiglia funzionale**
- **Trasformazione fisica = stato meccanico**
- **Overlay / VFX = stato ambientale**
- **Mesh e collisione graybox non sono authority gameplay**

Lo stato logico rimane nel `MapState`, nelle transizioni e nei servizi di simulazione. La mesh mostra lo stato; non lo decide.

## World scale baseline

Baseline di scala:

| Parametro | Valore |
|---|---:|
| 1 Unreal Unit | 1 cm |
| Lato esagono | 150 cm |
| Flat-to-flat | ~260 cm |
| Vertice-vertice | 300 cm |
| Reference Human | 180 cm |
| Unit visual footprint | ~70–80 cm |

La mesh di una unità può estendersi oltre il suo footprint visivo con arma/equipaggiamento, ma la logica di occupazione non deve essere derivata automaticamente dai bounds della mesh.

## Cell Placement Volume

La v0.1 introduce un **Cell Placement Volume** Editor/debug: un prisma esagonale usato come riferimento dimensionale per gli asset `CellBound`.

Baseline:

- Outer Cell Volume = 100%
- Safe Placement Volume ≈ 90%
- Standard Prop Envelope ≈ 120–160 cm
- guide verticali:
  - `LOW = 0.28C`
  - `STANDARD = 0.55C`
  - `STRUCTURAL = 0.85C`
  - `MAX = 1.00C`

Questi valori sono guide di modellazione e authoring; non sostituiscono le regole competitive di LOS, cover o traversal.

## Placement taxonomy

### CellBound
Asset contenuti in una singola cella.

Esempi:
- Unit
- Generator
- Hazard Tank
- Relay
- Valve
- piccoli device

### EdgeBound
Asset ancorati a segmenti, bordi o transizioni.

Esempi:
- Wall
- CoverLow
- CoverHigh
- Door
- Barrier

### SurfaceBound
Renderer o overlay applicati alla superficie della cella.

Esempi:
- Water
- Ice
- Wet
- future Burning / Electrified / Steam / Smoke

### EditorOnly
Guide e marker non necessari nella presentazione runtime.

Esempi:
- CellPlacementVolume
- SpawnMarker
- debug anchors

### MultiCell
Categoria prevista per asset che occupano più celle. Fuori scope della v0.1.

## Kit v0.1

Il catalogo iniziale comprende:

1. Cell
2. CellPlacementVolume
3. Unit
4. Floor
5. Wall
6. CoverLow
7. CoverHigh
8. Door
9. Rubble
10. WallBroken
11. Ramp
12. Platform
13. Water
14. Ice
15. Valve
16. Generator
17. HazardTank
18. Relay
19. SpawnMarker

## Cover visual grammar

Baseline graybox:

- `CoverLow` = parallelepipedo basso;
- `CoverHigh` = stessa famiglia, silhouette più alta;
- `Intact` = stato neutro;
- `Damaged` = giallo + marker non cromatico;
- `Critical` = arancione + marker più forte;
- `Destroyed` = geometria cambiata / rubble.

Open, closed, moved e rotated devono essere leggibili tramite trasformazione fisica, non soltanto dal colore.

## Regola di sostituzione

Un asset finale può sostituire un graybox solo se rispetta il contratto definito per:

- footprint;
- pivot;
- snap;
- placement class;
- orientamento;
- stato visuale;
- binding logico.

Il gameplay non deve cambiare perché cambia la mesh.
```

## Link consigliati
- `Asset Roadmap`
- `Asset Rules & Import Contract`
- `Gray Toolkit UML`
- `Map Editor`
- `MapState / Tactical Map`

---

# 4. Pagina Wiki — Asset Roadmap

## Titolo
`Asset Roadmap`

## Hero image
Usare l’infografica.

## Testo pronto

```md
# Asset Roadmap

La roadmap asset descrive la maturità prevista degli asset di RefactorTactics dalla prima rappresentazione graybox fino all’art replacement stabile della v1.0.

La sequenza è una **roadmap di maturità** e deve essere riconciliata con le release canoniche del repository senza creare una seconda roadmap parallela.

## Milestone di maturità

| Versione | Focus asset |
|---|---|
| v0.1 | Core Map |
| v0.2 | Environment |
| v0.3 | 3D Map / Verticality |
| v0.4 | Interactive Map |
| v0.5 | Tactical Devices |
| v0.6 | Destruction & Debris |
| v0.7 | Perception & Info Debug |
| v0.8 | Objective Kit |
| v0.9 | Production Graybox |
| v1.0 | Asset Contract Freeze |

## v0.1 — Core Map

Obiettivo: bloccare scala, placement e grammatica visuale.

Asset principali:

- Cell
- CellPlacementVolume
- Unit
- Floor
- Wall
- CoverLow
- CoverHigh
- Door
- Rubble
- WallBroken
- Ramp
- Platform
- Water
- Ice
- Valve
- Generator
- HazardTank
- Relay
- SpawnMarker

## v0.2 — Environment

Aggiunge proxy e render per:

- Pipe
- Drain
- Floodgate
- PowerNode
- ElectricalPanel
- Fire
- Wet
- Electric
- Steam
- Smoke
- Debris variants

## v0.3 — 3D Map / Verticality

Aggiunge:

- Stairs
- Ladder
- Bridge
- Catwalk
- Tunnel
- Upper Floor
- Roof
- Hatch
- Elevator

## v0.4 — Interactive Map

Aggiunge:

- Terminal
- Switch
- Button
- Gate
- movable cover
- rotatable cover
- Camera
- Sensor
- Alarm
- Beacon

## v0.5 — Tactical Devices

Aggiunge proxy per:

- Mine
- Trap
- Turret
- Shield Node
- Drone Station
- Deployable Base
- Remote Device
- Auxiliary proxy

## v0.6 — Destruction & Debris

Aggiunge:

- Fragile Wall
- Structural Pillar
- Breach variants
- Debris S/M/L
- Debris Pile
- Collapsed Structure

L’autorità rimane logica: Chaos o la fisica visuale non decidono l’esito competitivo.

## v0.7 — Perception & Info Debug

Prevalentemente debug visualization:

- Noise Source
- Noise Propagation
- Acoustic Mask
- Visibility Volume
- Detection Area
- Scanner
- Radar
- Surveillance Node

## v0.8 — Objective Kit

Aggiunge:

- Control Node
- Control Zone
- Payload proxy
- Payload path
- Extraction Zone
- Dynamic Objective
- Objective Barrier

## v0.9 — Production Graybox

Consolida il construction kit:

- size variants;
- corners;
- T-junctions;
- cross-junctions;
- end caps;
- modular snapping;
- orientation grammar;
- prefab compositions.

## v1.0 — Asset Contract Freeze

Alla v1.0 il Gray Toolkit diventa un contratto stabile per l’art replacement.

Freeze minimo:

- footprint;
- pivot;
- snap;
- placement class;
- scale class;
- state contract;
- material family;
- stable identity.

L’art finale può cambiare estetica e silhouette entro i limiti del contratto, ma non deve modificare la simulazione.
```

## Nota Claude
Verificare la roadmap canonica e mappare questi cluster senza alterare indebitamente le milestone globali.

---

# 5. Pagina Wiki — Gray Toolkit UML

## Titolo
`Gray Toolkit UML`

## Hero image
Usare:

`RT_GrayToolkit_UML_v1.png`

## Testo pronto

```md
# Gray Toolkit UML

Questo diagramma descrive le relazioni concettuali fra lo stato autorevole della simulazione, il toolkit graybox e il contratto degli asset.

## Relazioni principali

### MapState / Gameplay State → Gray Toolkit
Il gameplay state decide:

- traversal;
- cover;
- LOS;
- trajectory;
- interaction;
- surface state.

Il Gray Toolkit visualizza questi stati.

**La mesh graybox non è la fonte di verità del gameplay.**

### CellPlacementVolume → Asset Contract
Il Cell Placement Volume valida:

- footprint;
- scala;
- envelope;
- guide di altezza.

### PlacementClass → Asset Contract
Ogni asset dichiara una placement class:

- CellBound;
- EdgeBound;
- SurfaceBound;
- EditorOnly;
- MultiCell futuro.

### Asset Contract → Prefab Assets
Il contratto definisce ciò che ogni prefab deve rispettare:

- ScaleClass
- PlacementClass
- Footprint
- Height
- Pivot
- SnapPolicy
- MaterialFamily
- StateSet

### Roadmap → Prefab
La roadmap aumenta progressivamente profondità e varietà degli asset senza cambiare il contratto base.

### Character / Environment Art → Asset Contract
Le lane art devono rispettare lo stesso contratto usato dal graybox.

L’obiettivo è evitare che l’introduzione dell’art finale costringa a modificare MapState, pathfinding, cover, LOS o resolver.
```

## Nota grafica
Prima della pubblicazione verificare che:
- il nodo v0.9 sia corretto;
- la freccia `CellPlacementVolume -> Asset Contract` sia chiara;
- il naming del toolkit sia uniforme.

---

# 6. Pagina Wiki — Asset Rules & Import Contract

## Titolo
`Asset Rules & Import Contract`

## Testo pronto

```md
# Asset Rules & Import Contract

Questa pagina definisce le regole minime per importare e usare modelli 3D in RefactorTactics.

## World scale

- `1 UU = 1 cm`
- lato esagono = `150 cm`
- reference human = `180 cm`

Gli asset devono essere preparati nella scala del mondo, non corretti con scale arbitrarie sugli Actor.

## Runtime scale

Baseline:

```text
Import scale = 1.0
Actor scale  = (1,1,1)
```

Se un asset marketplace arriva con scala errata, deve essere normalizzato nella pipeline asset.

## Pivot

### CellBound
Pivot:

```text
bottom-center
```

### EdgeBound
Pivot:

```text
segment-center / base
```

### Elementi mobili
Il pivot logico dell’Actor resta stabile.
Pannelli, porte e parti mobili usano componenti child con pivot/hinge dedicato.

## Placement

Ogni asset dichiara una `PlacementClass`.

Il placement contract non determina automaticamente:

- traversal;
- cover;
- LOS;
- trajectory.

Questi rimangono dati della simulazione.

## Stable art replacement

Un asset finale deve poter sostituire il graybox senza cambiare:

- logical ID;
- footprint;
- pivot;
- snap;
- anchor;
- stato supportato;
- binding gameplay.

## Asset esterni / Marketplace

Pipeline consigliata:

```text
IMPORT
  ↓
CHECK SCALE
  ↓
CHECK PIVOT
  ↓
CHECK FOOTPRINT
  ↓
CHECK MATERIALS
  ↓
NORMALIZE
  ↓
RT ASSET
```

Evitare asset permanenti usati con Actor Scale arbitrario.

## Texture e materiali

La scala geometrica non determina da sola la risoluzione delle texture.

Il progetto deve introdurre una metrica separata di **Texel Density**.

Il valore definitivo va calibrato su:

- camera gameplay;
- target 1080p;
- distanza tipica;
- memory budget;
- performance.

La roadmap deve prevedere un `Texel Density Calibration` prima del freeze di produzione.
```

---

# 7. Pagina Wiki — Character & Environment Art Roadmap

## Titolo
`Character & Environment Art Roadmap`

## Testo pronto

```md
# Character & Environment Art Roadmap

Le lane art misurano la maturità degli asset con stati espliciti invece di percentuali generiche.

## Character Art

| Stato | Significato |
|---|---|
| C0 | Cylinder |
| C1 | Silhouette Proxy |
| C2 | Gameplay Proxy |
| C3 | Production Blockout |
| C4 | Production Mesh |
| C5 | Textured |
| C6 | Final Art |

### C0 — Cylinder
Placeholder tattico puro.

### C1 — Silhouette Proxy
Prima silhouette distinta per personaggio.

### C2 — Gameplay Proxy
Proporzioni, arma/equipaggiamento e lettura tattica sufficienti al gameplay.

### C3 — Production Blockout
Blockout con proporzioni finali e struttura pronta alla produzione.

### C4 — Production Mesh
Mesh di produzione con topologia e materiali definiti.

### C5 — Textured
Asset con texture/materiali di produzione.

### C6 — Final Art
Asset finale approvato.

## Environment / Prop Art

| Stato | Significato |
|---|---|
| E0 | Primitive |
| E1 | Gameplay Graybox |
| E2 | Art Blockout |
| E3 | Production Mesh |
| E4 | Textured |
| E5 | Final |

### E0 — Primitive
Cube, cylinder, plane o composizioni minime.

### E1 — Gameplay Graybox
Forma e proporzioni già affidabili per test e gameplay.

### E2 — Art Blockout
Prima interpretazione artistica mantenendo il contratto.

### E3 — Production Mesh
Mesh definitiva per produzione.

### E4 — Textured
Materiali e texture di produzione.

### E5 — Final
Asset approvato e pronto al rilascio.

## Vincolo comune

Tutti gli step di art devono rispettare l’`Asset Contract`.

Nessun passaggio da proxy a final art deve cambiare:

- footprint gameplay;
- pivot canonico;
- snap;
- placement class;
- stato logico;
- comportamento competitivo.
```

---

# 8. Navigation / cross-link consigliato

Inserire una sezione finale `See also` o equivalente.

## Gray Toolkit
Link a:
- Asset Roadmap
- Asset Rules & Import Contract
- Gray Toolkit UML
- Character & Environment Art Roadmap
- Map Editor
- Tactical Map / MapState

## Asset Roadmap
Link a:
- Gray Toolkit
- Feature Registry
- Project Roadmap
- Character & Environment Art Roadmap

## Asset Rules
Link a:
- Gray Toolkit
- Content Pipeline / Data Validation
- Asset Manager
- Map Editor

---

# 9. Epic / Issue integration dalla Wiki

La Wiki non deve essere un tracker parallelo.

Aggiungere alle pagine, se lo stile repository lo permette, una sezione:

```md
## Tracking

Owner:
- Epic: <link>
- Lane: <link>
- Feature: <link>

Open work:
- <issue>
- <issue>
```

Claude deve popolare questi link usando owner live reali, non numeri inventati.

---

# 10. Source references da collegare

Dove utile, collegare la Wiki alle specifiche tecniche canoniche su:

- architettura UE5;
- mappa/pathfinding;
- simulazione deterministica;
- UI/accessibilità;
- pipeline contenuti / Data Assets;
- roadmap QA;
- Rubble / Debris;
- cover / map interactions.

Non copiare grandi sezioni dei PDR nella Wiki: usare sintesi + link.

---

# 11. Definition of Done Wiki

- [ ] nessuna pagina duplicata;
- [ ] naming uniforme;
- [ ] immagini copiate nella posizione canonica;
- [ ] immagini embeddate e visibili;
- [ ] correzioni UML applicate;
- [ ] world scale coerente;
- [ ] CellPlacementVolume documentato;
- [ ] placement taxonomy documentata;
- [ ] Asset Contract documentato;
- [ ] roadmap asset collegata alla roadmap canonica;
- [ ] lane C0–C6 ed E0–E5 presenti;
- [ ] link verso Epic/Issue/Feature reali;
- [ ] link incrociati fra pagine;
- [ ] eventuali viste generate rigenerate;
- [ ] markdown lint / docs build / link check verdi se disponibili.

---

# 12. Output finale richiesto a Claude

Restituire:

```text
GRAY TOOLKIT WIKI UPDATE REPORT
```

con:

- pagine trovate;
- pagine aggiornate;
- pagine create;
- pagine accorpate;
- immagini copiate;
- correzioni grafiche applicate;
- link Epic/Issue aggiunti;
- decisioni non consolidate;
- docs build / link check;
- commit / PR;
- prossimo task raccomandato.
