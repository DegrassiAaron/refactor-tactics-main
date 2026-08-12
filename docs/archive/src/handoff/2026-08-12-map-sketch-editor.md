> ## 🗄️ `HISTORICAL` — sorgente archiviato il 2026-08-12
>
> **Revisionato e applicato in parte.** Referto:
> [`../../../roadmap/plans/map-sketch-editor-spec-panel-2026-08-12.md`](../../../roadmap/plans/map-sketch-editor-spec-panel-2026-08-12.md).
>
> Su **32 voci** classificate: 8 `CURRENT`, 8 `DUPLICATE`, 6 `CONFLICT`, 7 `PROPOSED`, 3 `STALE`.
> **Sedici hanno già un padrone** — l'editor mode, i quattro tool, la camera del viewport, la toolbar, il
> validator di mappa e la visualizzazione in editor esistono e sono tracciati altrove.
>
> **Terzo prompt della famiglia map-editor.** I due precedenti —
> [`2026-08-09-map-editor-roadmap.md`](2026-08-09-map-editor-roadmap.md) e
> [`2026-08-10-full-grid-geometry-walls-water.md`](2026-08-10-full-grid-geometry-walls-water.md) — avevano già
> un verdetto sulla stessa tesi centrale (§3: la geometria architettonica non coincide coi lati dell'hex),
> collocata in **E23.1** (v0.2).
>
> **Cosa lo distingue dai due che l'hanno preceduto**: la §4 — l'esagono diviso in dodici settori — è la
> risposta puntuale all'obiezione che aveva fermato il predecessore, cioè che un muro world-space porta
> estremi in virgola mobile dentro l'hash che tiene fermo il KPI *replay divergence = 0*. Una geometria
> enumerabile non ha estremi arbitrari. **Quello è il contributo tecnico del documento.**
>
> **Applicato**, per due decisioni esplicite dell'autore (referto §4): la geometria quantizzata è
> **anticipata in v0.1 come strumento d'editor** (epic [#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324),
> anticipazione dichiarata, non apertura); layout **generato** e geometria **disegnata** convivono perché
> hanno soggetti diversi. Issue aperte: [#619](https://github.com/DegrassiAaron/refactor-tactics-main/issues/619),
> [#620](https://github.com/DegrassiAaron/refactor-tactics-main/issues/620),
> [#621](https://github.com/DegrassiAaron/refactor-tactics-main/issues/621),
> [#622](https://github.com/DegrassiAaron/refactor-tactics-main/issues/622),
> [#623](https://github.com/DegrassiAaron/refactor-tactics-main/issues/623).
>
> **Non applicato**, e il referto dice perché: le priorità `P1.1`…`P2.5` di §27–§28 (quarto asse di
> numerazione); la roadmap editor scritta a mano di §30 (`roadmap-editor.md` è `HISTORICAL` **proprio**
> perché era una vista mantenuta a mano, ed è tornata generata); il `UDeveloperSettings` di §15 (in
> `Source/` non ne esiste nessuno); gli «scenari» `MapSketch_*` di §21 (sono verifiche di **classe C** e
> vanno in `test-manuali-pie.md`); il `LOW WALL` di §12 che non assegna cover (`FRTHexCover{Low}` è già
> esattamente quell'oggetto, sul bordo, serializzato).
>
> **Il testo originale non è stato riscritto.**

---

# PROMPT OPERATIVO — RT Map Sketch Editor v0.1 / Priority One

## Ruolo

Agisci come Senior Unreal Engine 5 Developer, Editor Tools Developer, Gameplay/Spatial Systems Engineer e repository maintainer per **RefactorTactics**.

Questa attività è **Priority One**.

L'obiettivo è arrivare il prima possibile a una modalità di authoring della mappa realmente usabile, sfruttando e migliorando ciò che esiste già nel repository, NON ricominciando da zero.

Il progetto ha già una modalità/strumento per disegnare o generare esagoni. Devi trovarla, capirla, mantenerla e usarla come base.

NON creare un secondo editor parallelo se l'attuale può essere esteso.

---

# 0. Regole operative obbligatorie

Prima di modificare qualsiasi file:

1. leggi `CLAUDE.md`;
2. leggi `AGENTS.md`, `AGENT.md` o equivalenti;
3. leggi `README.md`;
4. individua la versione Unreal Engine realmente bloccata nel repository;
5. individua roadmap, milestone, feature map, scenario map, editor map, wiki e documentazione map/spatial;
6. cerca le issue/epic esistenti relative a:
   - hex grid;
   - map editor;
   - editor mode;
   - graybox;
   - cell drawing;
   - wall/cover geometry;
   - pathfinding;
   - LOS;
   - spatial debug;
   - camera;
   - lighting;
7. trova l'implementazione attuale del disegno/generazione degli esagoni;
8. trova `L_DevSandbox` e gli scenari/editor test esistenti;
9. verifica lo stato Git;
10. lavora esclusivamente in un **Git worktree dedicato**.

Non lavorare direttamente nel worktree principale.

---

# 1. Git worktree — obbligatorio

Prima dell'implementazione:

```text
git status
git branch --show-current
git worktree list
```

Individua il branch base corretto seguendo le convenzioni del repository.

Crea un nuovo branch/worktree dedicato, per esempio:

```text
feature/map-sketch-editor-v01
```

NON usare questo nome se viola le convenzioni esistenti.

Il worktree deve essere esterno al working directory corrente e non deve sovrascrivere worktree già presenti.

Regole:
- nessuna modifica nel worktree principale;
- nessun reset distruttivo;
- nessun force push;
- nessun merge automatico nel branch principale;
- conserva commit piccoli e focalizzati;
- alla fine riporta path del worktree, branch e commit creati.

---

# 2. Contesto di design già deciso

RefactorTactics usa una griglia tattica **esagonale**.

La cella logica usa:

```text
FRTCellId
X
Y
Layer
```

X/Y rappresentano coordinate esagonali assiali.

La mappa runtime è un grafo tattico 3D:

```text
Cell = node
Transition = edge
```

Pathfinding, LOS, Targeting e Trajectory sono servizi separati.

Le celle sono dati compatti, non migliaia di Actor.

---

# 3. Geometria architettonica — nuova direzione

IMPORTANTE:

La geometria architettonica NON coincide necessariamente con i lati degli hex.

La griglia serve a definire:
- posizione tattica;
- movimento;
- layer;
- occupazione;
- facing;
- query spaziali.

La geometria architettonica serve a definire:
- muri;
- muretti;
- edifici;
- cave;
- pareti;
- aperture;
- porte;
- bordi;
- ostacoli solidi;
- cover;
- LOS blockers.

La geometria di gameplay deve essere **quantizzata**.

Sono ammesse:
1. direttrici principali derivate dall'esagono;
2. ortogonali a tali direttrici;
3. segmenti sui lati/perimetro dell'esagono;
4. junction compatibili con questa grammatica.

Non creare geometria tattica arbitraria a qualunque angolo.

---

# 4. 12 settori geometrici della cella

Per il prototipo usare una suddivisione logica dell'hex in **12 settori angolari**, uno ogni 30°.

Questi 12 settori NON sono nuove direzioni di movimento.

Servono al bake/editor per stimare quanta parte della cella è occupata da geometria solida.

Prevedere:

```text
12-bit occupancy mask
+
CoreBlocked
```

Baseline sperimentale iniziale:

```text
0-3 settori occupati
→ Free

4-5
→ Constrained

6+
→ Blocked

Core occupato
→ Blocked
```

QUESTI VALORI SONO DA VALIDARE VISIVAMENTE.

Devono essere configurabili nel debug/editor e NON diventare magic number sparsi nel codice.

---

# 5. Stati della cella

Supportare almeno:

```text
Free
Constrained
Blocked
```

Per v0.1 `Constrained` può essere trattata come traversabile dal pathfinding se non esistono ancora regole specifiche.

In futuro potrà influire su:
- Large units;
- Dash;
- displacement;
- vault;
- facing;
- passaggi stretti;
- path preference.

Non implementare queste estensioni ora.

---

# 6. Separazione fondamentale: Occupancy, Transition, Cover, LOS

NON confondere questi sistemi.

## Cell Occupancy
Deriva dal footprint solido che invade una cella.

Esempi:
- building footprint;
- rock;
- solid cliff;
- void;
- structural volume.

Output:
```text
Free / Constrained / Blocked
```

## Transition
Indica se è possibile passare da una cella a un'altra.

Possibili stati futuri:
```text
Walk
Blocked
Vault
Drop
Jump
Climb
Door
Special
```

## Cover
Dipende dalla geometria e dalla direzione dell'attacco.

Un `LowWall` NON concede automaticamente cover.

La cover sarà affrontata in un'issue successiva usando il sistema geometrico.

## LOS / Trajectory
Un muro pieno può bloccare LOS o una traiettoria.

Questo è distinto dalla cover.

Non implementare ora il sistema finale di cover/LOS se non necessario al debug dell'editor.

---

# 7. Problema attuale dell'Editor

L'attuale modalità di disegno ha almeno tre problemi UX osservati:

## Problema A — Griglia non visibile prima di disegnare
Il designer deve vedere chiaramente la griglia e il piano di lavoro PRIMA di piazzare geometria.

## Problema B — Scena troppo scura
Il graybox/editor deve avere illuminazione leggibile.

## Problema C — Camera scomoda
Navigare mentre si costruisce una mappa deve essere semplice.

---

# 8. Obiettivo immediato — RT Map Sketch Editor v0.1

Estendere la modalità esistente per arrivare a questo workflow:

```text
Open L_DevSandbox
        ↓
Enter RT Map Edit Mode
        ↓
Grid visible immediately
        ↓
Select drawing tool
        ↓
See valid snap directions
        ↓
See ghost preview
        ↓
Commit geometry
        ↓
Live occupancy preview
        ↓
Free / Constrained / Blocked overlay
        ↓
Undo / Redo
        ↓
Save
```

---

# 9. NON ricreare il disegno hex esistente

Questa è una regola esplicita.

Nel repository esiste già una modalità di disegno/generazione degli esagoni.

Prima di creare nuove classi:

```text
SEARCH
↓
UNDERSTAND
↓
REUSE
↓
EXTEND
```

Nel report iniziale indica:

```text
Existing Hex Drawing System:
- classi;
- file;
- asset;
- input;
- ownership;
- dati prodotti;
- limiti;
- cosa verrà mantenuto;
- cosa verrà modificato.
```

---

# 10. Grid Preview sempre disponibile

Quando entra la modalità Map Edit:

```text
Grid = ON
```

La griglia deve essere visibile anche dove ancora non esistono celle create, se questo è coerente con il tool attuale.

Se l'editor distingue:

```text
workspace grid
actual map cells
```

renderizzarle in modo diverso.

Esempio:

```text
Workspace Grid
→ sottile / ghost

Existing Cell
→ linea normale

Hovered Cell
→ evidenziata

Selected Cell
→ evidenziata con stato chiaro
```

---

# 11. Snap geometrico

Quando uno strumento di geometria è attivo, mostrare chiaramente gli assi validi.

Supportare:

```text
Hex axes
Orthogonal axes
Hex perimeter / side anchors
Existing junctions
```

Il designer deve vedere:
- valid snap point;
- valid direction;
- ghost segment;
- invalid placement.

Prima del click.

---

# 12. Strumenti v0.1

Scope obbligatorio:

```text
SELECT
HEX / FLOOR esistente
WALL
LOW WALL
SOLID FOOTPRINT
VOID / CLIFF
ERASE
```

Se la modalità esistente ha già strumenti equivalenti, integrarli.

`LOW WALL` è un tipo geometrico distinto ma NON assegna automaticamente cover.

---

# 13. Disegno di segmenti

Workflow preferito:

```text
First click
→ Anchor A

Move mouse
→ snapped ghost

Second click
→ commit Segment A-B

Continue
→ B diventa anchor successivo

ESC / RMB
→ termina catena
```

Supportare junction continue per costruire rapidamente:
- casa;
- corridoio;
- bordo cava;
- muretto.

---

# 14. Preview Occupancy live

Quando una geometria viene committed:

```text
Geometry
    ↓
Affected Cells
    ↓
12-sector occupancy
    ↓
Core occupancy
    ↓
Cell classification
```

Overlay:

```text
Free
Constrained
Blocked
```

Aggiungere debug opzionale:

```text
Show Occupancy Sectors
```

Info cella:

```text
Cell: (Q,R,L)
Occupied Sectors: 5 / 12
Mask: 001111000100
Core: Free
Classification: Constrained
```

---

# 15. Threshold debug

Aggiungere in Development/Editor controlli centralizzati:

```text
ConstrainedThreshold
BlockedThreshold
CoreBlocksCell
```

La modifica deve aggiornare l'overlay.

La policy va centralizzata in `Ruleset`, `DeveloperSettings` o `MapEditorSettings`, scegliendo la soluzione coerente col repository.

---

# 16. Lighting per L_DevSandbox

Auditare l'attuale lighting.

Creare o correggere un setup semplice:

```text
Directional Light
Sky Light
Sky Atmosphere
```

Requisiti:
- superficie leggibile;
- niente zone quasi nere;
- griglia sempre visibile;
- geometria distinguibile;
- esposizione prevedibile.

---

# 17. Camera Map Editor

Auditare prima il controller attuale.

Target UX iniziale:

```text
MMB drag
→ pan

RMB drag
→ orbit / rotate around map pivot

Mouse Wheel
→ zoom toward cursor

WASD
→ pan

Q / E
→ rotate snapped

F
→ focus selection

Home
→ frame entire editable map

Shift
→ faster movement
```

Per l'editor valutare snap di rotazione a `30°`.

Se confligge con gameplay camera, separare configurazione Map Editor e Gameplay Tactical Camera, condividendo le utility.

---

# 18. Editor HUD / Toolbar

Integrare nel sistema UI/Editor esistente.

MVP desiderato:

```text
RT MAP EDIT

Tool
[Select]
[Hex]
[Wall]
[LowWall]
[Solid]
[Void]
[Erase]

Layer
[L0]

Snap
[x] Grid
[x] 30° Axes
[x] Junction

View
[x] Grid
[x] Occupancy
[ ] Occupancy Sectors
[ ] Graph
[ ] LOS
[ ] Cover

Bake
[Live]

[Validate]
[Rebuild]
```

Non introdurre CommonUI.

---

# 19. Live bake incrementale

NON ricostruire tutta la mappa a ogni movimento del mouse.

Workflow:

```text
Mouse move
→ cheap ghost preview

Commit
→ determine affected cells/chunk
→ rebake affected region
→ update overlay
→ update revision
```

Supportare Undo/Redo correttamente.

---

# 20. Dati runtime vs dati editor

Separare:

```text
Authoring Geometry
        ↓
Bake
        ↓
Runtime Spatial Data
```

Runtime non deve dipendere da Editor-only UObject/classes.

---

# 21. Scenari richiesti

Cercare prima gli scenari già esistenti.

NON creare duplicati.

Se non esistono scenari equivalenti, creare o aggiornare:

## Scenario — MapSketch_House
Verifica:
- grid visible;
- snap corretto;
- angolo 90°;
- footprint solido;
- occupancy;
- Free/Constrained/Blocked;
- Undo/Redo;
- save/reload.

## Scenario — MapSketch_Quarry
Verifica:
- Void/Cliff;
- geometria concava;
- celle edge;
- constrained;
- blocked.

## Scenario — MapSketch_CoverGeometry
Verifica:
- wall;
- low wall;
- junction;
- snap;
- segment types;
- debug visualization.

Non validare ancora la cover finale.

---

# 22. Automation / Functional Tests

Minimo:

```text
SectorMask deterministic
CoreBlocked classification
Threshold classification
Geometry -> affected cells stable
Save/reload
Undo/Redo where testable
```

Fixture:
- single solid segment;
- corner;
- solid footprint;
- void footprint.

---

# 23. Spatial Debug integration

Esiste/è prevista una issue dedicata allo Spatial Debug.

Questa feature deve integrarsi con essa.

Esporre almeno:

```text
Grid
Cell IDs
Occupancy
Occupancy Sectors
Cell Classification
Authoring Segments
Junctions
```

Non duplicare un nuovo sistema debug.

---

# 24. Validazione

Aggiungere validator per:

```text
off-axis tactical geometry
invalid junction
zero-length segment
duplicate segment
invalid layer
solid geometry crossing forbidden core
orphan geometry
invalid occupancy mask
invalid footprint
geometry outside editable bounds
```

---

# 25. Documentazione obbligatoria

Questa issue NON è Done se il codice funziona ma la documentazione resta indietro.

Individua prima i documenti reali presenti nel repository.

Aggiorna e consolida, senza creare duplicati inutili:

```text
CLAUDE.md
AGENTS.md / AGENT.md
README.md
Roadmap
Milestone map
Feature map
Scenario map
Editor map
Wiki
Map / Spatial docs
Map Editor docs
Debug docs
Testing docs
```

La documentazione deve descrivere:
1. modalità Map Edit;
2. controlli camera;
3. strumenti;
4. snap;
5. 12-sector occupancy;
6. Free/Constrained/Blocked;
7. lighting development;
8. scenari;
9. test;
10. limiti v0.1;
11. roadmap successiva.

---

# 26. Issue ed Epic — NON duplicare

Prima cerca issue/epic esistenti.

Per ogni elemento:

```text
FOUND
→ update / link

NOT FOUND
→ create
```

Collegare questa attività agli item esistenti su:
- Hex Grid;
- Map Editor;
- Spatial Debug;
- Pathfinding;
- LOS;
- Cover;
- Map Multilevel;
- Editor UX.

---

# 27. Roadmap Priority One

Costruisci una roadmap consolidata che includa:
- questa issue;
- issue già create rilevanti;
- dipendenze;
- sequenza minima per arrivare a un editor usabile;
- exit gate di ogni passo.

Usa priorità:

```text
P0 = blocker tecnico immediato
P1 = necessario per Map Sketch usable
P2 = necessario per validare gameplay spaziale
P3 = avanzato / production
```

Obiettivo:

> Qual è la sequenza minima di issue per permettere al designer di costruire e verificare una mappa RefactorTactics il prima possibile?

---

# 28. Roadmap proposta da verificare contro le issue reali

NON applicarla ciecamente.

## P0 — Audit + Existing Hex Tool Stabilization
EXIT:
```text
existing hex authoring works
project compiles
```

## P1.1 — Persistent Grid Preview
EXIT:
```text
designer never draws blind
```

## P1.2 — Editor Camera & Lighting
EXIT:
```text
editing map is comfortable
```

## P1.3 — Quantized Segment Authoring
EXIT:
```text
designer can sketch house perimeter
```

## P1.4 — Solid / Void Footprints
EXIT:
```text
designer can sketch building and quarry
```

## P1.5 — Occupancy Bake
EXIT:
```text
geometry produces understandable tactical occupancy
```

## P1.6 — Save / Undo / Redo / Validate
EXIT:
```text
map survives editor restart
all authoring operations transactional
```

## P1.7 — Scenario Pack
EXIT:
```text
three reproducible fixtures validate authoring
```

## P2.1 — Spatial Debug Full Integration
- graph;
- transitions;
- path;
- reason codes.

## P2.2 — Transition Bake
- blocked;
- walk;
- door;
- cliff;
- vault/drop future-ready.

## P2.3 — LOS Structural Bake
- full walls;
- openings;
- dynamic structure revisions.

## P2.4 — Cover Geometry
- low wall;
- wall endpoint;
- angle;
- silhouette;
- None/Partial/Strong.

NON anticipare questa issue dentro P1 salvo hook tecnici.

## P2.5 — A* + Map Editing Feedback
- path live preview;
- invalid path;
- GraphRevision.

## P3
- multilayer;
- bridge;
- tunnel;
- stairs;
- elevator;
- chunking;
- advanced editor mode;
- production optimization.

---

# 29. Priority One definition

Nel report finale, identifica esplicitamente:

```text
NEXT ISSUE
```

e deve essere UNA sola issue concreta.

Scegli il blocker che porta più velocemente a:

```text
visible grid
+
usable camera
+
drawing
+
occupancy feedback
```

---

# 30. Roadmap file

Aggiornare la roadmap esistente.

Se non esiste una roadmap editor consolidata, crearne UNA soltanto, seguendo naming/convenzioni repository.

Deve contenere:

```text
Existing issue ID
Title
Priority
Dependencies
Status
Exit Gate
Scenario coverage
Test coverage
```

---

# 31. Scenario Map / Feature Map / Editor Map

Aggiornare le mappe esistenti con relazioni reali.

Usa il formato reale già adottato nel repository.

---

# 32. Criteri di accettazione v0.1

Questa tranche è Done soltanto quando:

- il progetto compila;
- si lavora da worktree dedicato;
- il sistema esistente di disegno hex è preservato/esteso;
- la griglia è visibile prima del disegno;
- hover e selection sono leggibili;
- la scena graybox è illuminata correttamente;
- la camera è usabile senza combattere contro il pivot;
- Wall funziona;
- LowWall funziona;
- segmenti snap alle direttrici valide;
- ghost preview funziona;
- Solid Footprint funziona;
- Void/Cliff funziona;
- occupancy 12-sector funziona;
- CoreBlocked funziona;
- Free/Constrained/Blocked sono visibili;
- threshold può essere testato;
- Undo/Redo funziona;
- Save/Reload funziona;
- validator trova casi invalidi;
- scenari necessari sono presenti o aggiornati;
- Automation/Functional Test pertinente passa;
- documentazione/wiki/maps sono aggiornate;
- roadmap è consolidata con issue reali;
- viene indicata la singola NEXT ISSUE P1.

---

# 33. Performance

Non fare:

```text
one Actor per hex
one Actor per sector
global full map rebake on every mouse move
heavy Tick for every preview object
```

Preferire:

```text
instancing
affected-cell rebake
dirty region
event-driven refresh
pooled debug rendering
```

---

# 34. Commit suggeriti

Adatta alle convenzioni reali.

```text
fix(map-editor): keep workspace hex grid visible
fix(camera): improve map editor navigation and focus
fix(editor): add readable dev sandbox lighting
feat(map-editor): add quantized wall segment authoring
feat(map-editor): add solid and void footprints
feat(map): add 12-sector occupancy bake
feat(debug): visualize map occupancy classification
test(map-editor): add house quarry authoring fixtures
docs(map-editor): consolidate roadmap wiki and scenario maps
```

---

# 35. Output iniziale richiesto da Claude

Prima di implementare, rispondi con:

```text
REPOSITORY AUDIT

UE VERSION
BASE BRANCH
WORKTREE PLAN

EXISTING HEX DRAWING SYSTEM
EXISTING MAP EDITOR
EXISTING CAMERA
EXISTING LIGHTING

RELEVANT ISSUES / EPICS
RELEVANT ROADMAP ITEMS

FILES TO MODIFY
FILES TO CREATE

SCENARIOS FOUND
SCENARIOS MISSING

P0 / P1 DEPENDENCY PLAN

FIRST IMPLEMENTATION SLICE
```

Poi procedi senza attendere conferma se non trovi un blocco realmente ambiguo o distruttivo.

---

# 36. Output finale richiesto

Alla fine riporta:

```text
WORKTREE
BRANCH
COMMITS

IMPLEMENTED
REUSED
REFACTORED
NOT IMPLEMENTED

BUILD RESULT
TEST RESULT
SCENARIO RESULT
PACKAGED RESULT

DOCUMENTATION UPDATED
WIKI UPDATED
ROADMAP UPDATED
FEATURE MAP UPDATED
SCENARIO MAP UPDATED
EDITOR MAP UPDATED

ISSUES UPDATED
ISSUES CREATED
EPIC LINKS

KNOWN LIMITATIONS

P1 STATUS

NEXT ISSUE:
<single concrete issue>
```

---

# 37. Principio finale

Questa non è ancora la costruzione del Map Editor definitivo.

È il **Map Sketch Editor v0.1**:

> uno strumento rapido, leggibile e testabile per disegnare una mappa RefactorTactics sopra una griglia hex visibile e verificare immediatamente come la geometria quantizzata influenza le celle tattiche.

La priorità assoluta è arrivare al più presto a questo loop:

```text
SEE GRID
→ DRAW
→ SEE RESULT
→ ADJUST
→ SAVE
→ TEST
```

Non espandere lo scope verso:
- cover finale;
- LOS finale;
- GAS;
- multiplayer;
- modding pubblico;
- procedural generation avanzata;
- art pipeline finale;
- editor standalone.

Prepara gli hook, ma completa prima il loop di authoring P1.
