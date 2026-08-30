# CLAUDE — RefactorTactics
# Tactical Grid Overlay — Issue-Driven Execution Plan

## Missione

Esegui end-to-end la feature **Tactical Grid Overlay** di RefactorTactics, includendo:

- griglia esagonale semi-trasparente sopra la mappa;
- griglia ON di default e disattivabile/riattivabile;
- highlight della cella sotto il mouse;
- selezione cella coerente con hover;
- debug `FRTCellId` (`X`, `Y`, `Layer`);
- nessun Actor per cella;
- nessuna mutazione dello stato competitivo;
- compatibilità con futura mappa multilivello;
- test automatici, PIE, packaged build e verifica performance;
- creazione, riuso e modifica delle **GitHub Issue** necessarie;
- aggiornamento della roadmap corrente;
- uso di **GitHub MCP** e **Epic Unreal MCP / Unreal Editor MCP** quando disponibili e utili.

Questa attività deve essere gestita in modalità **issue-driven**: prima allinea tracking e ownership, poi implementa una issue alla volta, aggiornando stato, evidenze e roadmap durante il lavoro.

---

# 0. Regole non negoziabili

1. **SEARCH BEFORE CREATE**: prima di creare una nuova issue cerca issue OPEN e CLOSED equivalenti o parzialmente sovrapposte.
2. **REUSE BEFORE DUPLICATE**: se una issue esistente possiede già il lavoro, aggiornala invece di crearne una nuova.
3. **MEASURE BEFORE CLOSE**: non chiudere issue senza evidenza reale da codice, test, Editor o packaged build.
4. Non inventare numeri issue, Epic, milestone, label, branch, file o asset.
5. Non inventare funzioni MCP: usa solo tool realmente esposti nella sessione.
6. Non creare una nuova Epic se una Epic corrente possiede già correttamente il dominio.
7. Non creare Actor per ogni esagono.
8. Non duplicare algoritmi world↔hex se esiste già un servizio canonico (`URTHexLibrary` o equivalente).
9. Hover e toggle sono presentation-only e non devono mutare `FRTMapState`, snapshot, seed, path graph o TurnLog.
10. Non usare mai dati di planning nemico per grid, hover o warning.
11. Non bypassare il Pointer Interaction Contract corrente.
12. Ogni issue implementata deve avere Definition of Done verificabile.
13. Se un gate non viene eseguito, scrivere `NOT RUN` con motivazione. Mai dichiararlo verde.
14. Se il working tree è sporco, non sovrascrivere modifiche non correlate.
15. Prima di commit/push rispetta le policy del repository e le istruzioni locali (`AGENTS.md`, `CLAUDE.md`, ecc.).

---

# 1. Preflight repository

Prima di fare qualsiasi mutazione GitHub o codice:

```text
REPO ROOT
REMOTE
DEFAULT BRANCH
CURRENT BRANCH
HEAD SHA
WORKTREE STATUS
UE EngineAssociation / versione UE
```

Leggi obbligatoriamente, se presenti:

```text
AGENTS.md
CLAUDE.md
README.md
ROADMAP.md / docs roadmap correnti
Decision Log / ADR
Definition of Done
Pointer Interaction spec
Map/Grid architecture docs
QA / Automation docs
```

Trova gli owner reali di:

```text
FRTCellId
world <-> hex conversion
map asset / cell enumeration
map renderer
pointer hover
LMB selection
Enhanced Input
debug HUD / debug overlay
L_DevSandbox
Automation tests
packaged validation
```

Path storicamente plausibili, **solo come hint**:

```text
Source/RefactorTactics/Map/RTCellId.h
Source/RefactorTactics/Map/RTHexCellData.h
Source/RefactorTactics/Map/RTHexLibrary.*
Source/RefactorTactics/Map/RTHexMapAsset.*
Source/RefactorTactics/Map/RTHexMapActor.*
Source/RefactorTactics/Player/RTPlayerController.*
Source/RefactorTactics/Tests/*
```

Verifica i path reali prima di modificare.

---

# 2. Audit GitHub obbligatorio

Usa GitHub MCP per cercare:

```text
grid
hex grid
tactical grid
grid overlay
hover
cell hover
cell selection
pointer
pointer interaction
FRTCellId
debug cell
map readability
presentation
L_DevSandbox
multilayer
```

Cerca sia **OPEN** sia **CLOSED**.

Per ogni risultato rilevante leggi:

```text
title
body
state
labels
milestone
assignee
parent/epic relationship se disponibile
comments recenti
dipendenze
Definition of Done
```

Classifica ogni issue:

```text
REUSE
UPDATE
PARTIAL_OVERLAP
DUPLICATE
STALE
CLOSED_BUT_REOPEN_REQUIRED
UNRELATED
```

Non creare nulla prima di questa classificazione.

---

# 3. Decisione Epic

Verifica se esiste già una Epic che possiede una o più di queste aree:

```text
Map Presentation
UI / Readability
Pointer Interaction
Tactical World Overlay
Vertical Slice Foundations
```

Riferimenti storici eventualmente presenti nel repository, come Epic/issue numerate, sono solo indizi: **verifica sempre live**.

## Policy

### Caso A — Epic esistente corretta

Usala. Non crearne una nuova.

### Caso B — ownership divisa tra due Epic esistenti

Preferisci:

```text
Grid visual layer -> Presentation / Readability owner
Pointer hover/selection -> Pointer Interaction owner
```

Collega le issue con dipendenze/documentazione secondo la convenzione live.

### Caso C — nessuna Epic possiede il dominio

Solo in questo caso crea una Epic con outcome:

```text
In L_DevSandbox il giocatore vede una griglia hex semi-trasparente,
identifica chiaramente la cella sotto il mouse, la seleziona tramite
il pointer contract e può attivare/disattivare la griglia senza
alterare gameplay, networking o privacy.
```

---

# 4. Piano issue consigliato

Non creare micro-issue inutili. Target: **2–4 issue reali** complessive, riusando quelle esistenti quando possibile.

## Issue A — Tactical Grid Visual Layer

Creare solo se non esiste un owner adeguato.

### Scope

```text
- semi-transparent hex fill
- hex outline
- instanced rendering
- elevation/lift sopra la surface
- ON by default
- toggle visibility
- material/material instance setup
- no Actor per cella
```

### Out of scope

```text
- pathfinding rules
- targeting rules
- LOS
- networking intent
- GAS
- objective rules
```

### DoD

- [ ] grid visibile in `L_DevSandbox`;
- [ ] fill semi-trasparente;
- [ ] terreno leggibile sotto la grid;
- [ ] outline leggibile;
- [ ] niente z-fighting evidente;
- [ ] rendering aggregato ISM/HISM o equivalente;
- [ ] nessun Actor per cella;
- [ ] toggle ON/OFF;
- [ ] toggle non muta MapState;
- [ ] performance sanity check;
- [ ] packaged Development verificato.

---

## Issue B — Pointer Cell Hover + Selection

Prima cerca un owner Pointer Interaction esistente. Preferisci aggiornare quello.

### Scope

```text
- mouse hover -> logical FRTCellId
- highlight singola cella
- no full grid rebuild su mouse move
- LMB usa current pointer contract
- selected cell == hovered cell quando valido
- debug hovered/selected FRTCellId
- HUD/world input priority rispettata
```

### DoD

- [ ] hover evidenzia una sola cella;
- [ ] hover non committa gameplay;
- [ ] uscita dalla mappa pulisce hover;
- [ ] stesso `FRTCellId` per hover e click;
- [ ] debug mostra `(X,Y,Layer)`;
- [ ] nessun hidden enemy target può diventare hover target se il pointer contract lo vieta;
- [ ] Automation pointer pertinente verde;
- [ ] PIE verificato.

---

## Issue C — Multilayer + QA + Packaged

Crearla solo se il repository separa chiaramente QA/integration work. Altrimenti inserire questi gate nelle issue A/B.

### Scope

```text
- same X/Y, different Layer safety
- active/visible layer picking
- toggle regression
- automation integration
- shader complexity / overdraw sanity
- packaged Development
```

### DoD

- [ ] due celle con stesso X/Y e Layer diverso restano distinguibili;
- [ ] layer nascosti/non attivi non sono pickati accidentalmente;
- [ ] nessuna adiacenza verticale implicita introdotta;
- [ ] Automation verde;
- [ ] PIE verde;
- [ ] packaged verde;
- [ ] privacy regression verde.

---

# 5. Creazione / modifica issue

Usa GitHub MCP per:

```text
create issue
update issue body/title
add/remove labels
assign milestone
add assignee se previsto
reopen issue se necessario
close duplicate con reason
aggiungere commenti di evidenza
```

Dopo ogni mutazione, rifai fetch dell'issue e verifica lo stato reale.

## Template issue

```markdown
# <Titolo>

**Epic/Owner:** #...
**Release/Milestone:** v0.1 o corrente
**Priority:** secondo convenzione live

## Outcome
...

## Why
...

## Current measured state
...

## Scope
...

## Out of scope
...

## Existing systems reused
- `FRTCellId`
- current map renderer
- current world↔cell conversion
- current pointer contract
- current debug HUD

## Dependencies
...

## Implementation notes
...

## Epic Unreal MCP / Editor work
...

## Automation
- [ ] ...

## PIE
- [ ] ...

## Performance
- [ ] ...

## Packaged
- [ ] ...

## Privacy impact
Presentation-only. Nessun hidden enemy planning data consumato o replicato.

## Determinism impact
Nessuna mutazione di snapshot/hash/ruleset/MapState.

## Definition of Done
- [ ] ...

## Evidence
Da compilare durante l'implementazione.
```

---

# 6. Roadmap

Trova la roadmap canonica corrente e **modifica quella**. Non crearne una seconda.

Inserisci la feature nell'ordine reale delle dipendenze:

```text
0. Contract + tracking audit
1. Semi-transparent tactical grid renderer
2. Cell hover
3. Cell selection + FRTCellId debug
4. Toggle + multilayer safety
5. Automation + PIE + perf + packaged
```

Per ogni step registra:

```text
owner issue
status
dependency
exit gate
```

Se la roadmap usa checkbox, milestone o tabelle, segui lo stile esistente.

---

# 7. Audit Unreal con Epic MCP

Quando disponibile, usa Epic Unreal MCP / Unreal Editor MCP ufficiale per misurare prima di creare asset:

```text
- map renderer corrente
- mesh hex esistenti
- grid materials
- hover materials
- selected materials
- Material Instances
- Blueprint owner
- Enhanced Input actions
- current key bindings
- L_DevSandbox
- compile errors
- PIE behavior
```

Non duplicare asset equivalenti.

Se `G` è già occupato, non sovrascrivere il binding: usa la convenzione corrente e aggiorna issue/spec.

---

# 8. Architettura tecnica preferita

Dopo audit, scegli la soluzione minima.

## Preferenza 1 — estendere owner map/presentation esistente

```text
Existing Map Presentation Owner
    ├─ GridFillInstances
    ├─ GridOutlineInstances
    ├─ Hover primitive/instance
    └─ Selected primitive/instance
```

## Preferenza 2 — nuovo component solo se necessario

```text
URTTacticalGridOverlayComponent
attached to existing map presentation owner
```

Non creare un Actor globale duplicato se il renderer corrente può possedere la feature.

### Mapping

Garantire:

```text
Visual Instance Index -> FRTCellId
```

oppure usare il lookup canonico esistente.

---

# 9. Visual baseline

Default iniziali:

```text
GridLiftCm               = 2.0
HoverAdditionalLiftCm    = 0.5
GridFillOpacity          = 0.12
GridOutlineOpacity       = 0.40
HoverFillOpacity         = 0.28
SelectedFillOpacity      = 0.22
```

Sono default di presentazione e devono essere configurabili.

### Material baseline

Grid fill:

```text
Unlit
Translucent
Depth Test ON
parameterized opacity
```

Outline:

```text
Unlit
Masked o Translucent secondo renderer/perf
```

Hover/Selected:

```text
Unlit
Translucent
più leggibile della base grid
```

Non usare `Disable Depth Test` in modo da vedere grid attraverso muri/occluder.

---

# 10. Implementazione hover

Flusso:

```text
pointer update
    ↓
HUD consumes pointer? -> stop world handling
    ↓
resolve logical FRTCellId
    ↓
cell changed?
  no -> no-op
  yes -> move/show single hover visual
    ↓
update debug state
```

Regole:

- una sola cella hover;
- niente rebuild di tutte le istanze;
- niente modifica MapState;
- niente commit;
- se non esiste cella valida: clear hover.

---

# 11. Implementazione click / selection

Non creare un nuovo sistema parallelo.

Usa il current Pointer Interaction Contract.

Acceptance minima in stato neutro:

```text
HoveredCell == SelectedCell after valid LMB
```

Se il current context assegna a LMB un'altra azione (target/destination), passa dal resolver esistente.

Debug Development:

```text
Hovered Cell: (X,Y,L)
Selected Cell: (X,Y,L)
Grid: ON/OFF
```

Riusa l'overlay/debug HUD corrente.

---

# 12. Toggle

La griglia deve essere ON di default.

Toggle OFF:

```text
hide fill
hide outline
hide hover/selected grid visuals
```

Toggle ON:

```text
restore grid presentation
```

Non distruggere/ribuildare asset ad ogni toggle.

Non mutare:

```text
FRTMapState
graph revision
path cache
snapshot
TurnLog
network state
```

---

# 13. Multilayer safety

`FRTCellId` è almeno:

```text
X
Y
Layer
```

Due celle con stesso `X/Y` e Layer diverso devono restare distinte.

Policy:

```text
render visible layers
pick only valid/active/focused layers
never infer vertical adjacency from overlapping X/Y
```

Se il sistema layer attuale non è ancora completo, implementa la feature senza assumere `Layer == 0` in modo hard-coded.

---

# 14. Automation tests

Cerca prima nomi/test equivalenti.

Coverage richiesta, adattando i namespace reali:

```text
GridOverlay.InstanceCountMatchesVisibleCells
GridOverlay.InstanceToCellIdIsStable
GridOverlay.ToggleDoesNotMutateMapState
GridOverlay.HoverChangesOnlyOnCellChange
PlayerInput.CellHoverResolvesCellId
PlayerInput.LMBSelectsHoveredCell
```

Se esistono già test come:

```text
PlayerInput.HoverNeverCommits
PlayerInput.HUDConsumesPointerBeforeWorld
PlayerInput.HiddenEnemyCannotBecomeHoverTarget
```

mantienili e assicurati che continuino a passare.

---

# 15. Compilazione / validation

Ordine minimo:

```text
git diff --check
Development Editor build
test mirati grid/pointer
suite Automation pertinente
PIE L_DevSandbox
shader complexity / overdraw sanity
packaged Development
```

Target client: 60 FPS.

Verifica che il mouse hover non causi rebuild O(N) della griglia.

---

# 16. Workflow issue-driven durante l'implementazione

Per ogni issue:

## Prima di iniziare

1. fetch issue live;
2. verifica dipendenze;
3. commenta `Implementation started` solo se questa convenzione esiste già nel repo;
4. registra branch/HEAD iniziale nel report locale.

## Durante il lavoro

Aggiorna l'issue quando emerge:

```text
- un blocker reale;
- una dipendenza non documentata;
- un cambio di scope necessario;
- un asset/owner differente da quanto previsto;
```

Non aggiungere rumore per ogni micro-step.

## Quando il lavoro è pronto

Aggiungi evidence nell'issue:

```text
Files changed
Assets changed
Tests run
PIE result
Packaged result
Performance result
Screenshots/evidence se il workflow lo supporta
Commit/PR
Known limitations
```

Chiudi l'issue solo quando tutti i DoD applicabili sono verdi.

---

# 17. Duplicate handling

Se trovi due issue equivalenti:

1. scegli l'owner più completo/recente/coerente con la roadmap;
2. trasferisci nel body/commento dell'owner eventuale scope utile mancante;
3. aggiungi link reciproco;
4. chiudi il duplicato con reason `duplicate` se GitHub MCP lo consente;
5. verifica che milestone/roadmap puntino all'owner corretto.

Non perdere acceptance criteria durante la deduplicazione.

---

# 18. Reopen policy

Riapri una issue chiusa quando:

```text
- il DoD dichiarato non è più vero sul main corrente;
- la feature era solo parziale;
- il packaged/PIE gate non era mai stato eseguito;
- il nuovo requisito semi-transparent grid rientra chiaramente nello scope originale.
```

Non riaprire issue storiche se è più corretto creare una child issue moderna con ownership chiara.

---

# 19. Branch / commit / PR

Segui la strategia Git del repository.

Commit suggeriti, solo se coerenti con lo stato reale:

```text
feat(ui-map): add translucent tactical hex grid overlay
feat(input): connect hex hover and cell selection
feat(debug): show hovered and selected cell ids
test(ui-map): cover grid overlay pointer behavior
```

Se il repository usa PR:

- collega le issue con sintassi live (`Closes #...`, `Refs #...`);
- non chiudere manualmente una issue se la policy delega la chiusura al merge;
- aggiorna roadmap dopo merge/verification.

---

# 20. Consistency check finale

Dopo il batch GitHub + implementazione:

```text
1. refetch tutte le issue toccate
2. verify state
3. verify labels
4. verify milestone
5. verify Epic/owner
6. verify dependencies
7. verify roadmap references
8. verify duplicate search
9. verify code/assets/test state
10. verify no issue claims unsupported evidence
```

---

# 21. Output finale obbligatorio

Restituisci un report con questo formato:

```text
REPOSITORY
Repo:
Branch:
HEAD before:
HEAD after:
UE version:

EPIC DECISION
New Epic required: YES/NO
Owner Epic(s):
Reason:

ISSUE AUDIT
Reused:
Updated:
Created:
Reopened:
Closed as duplicate:
Deferred:

ROADMAP
File/owner:
Changes:
Current execution order:

UNREAL MCP AUDIT
Renderer:
Materials:
Meshes:
Input:
L_DevSandbox:

IMPLEMENTATION
Grid fill:
Outline:
Hover:
Selection:
CellId debug:
Toggle:
Multilayer:

FILES CHANGED
...

ASSETS CHANGED
...

TESTS
Build:
Automation:
PIE:
Packaged:
Performance:

PRIVACY
...

DETERMINISM
...

GITHUB EVIDENCE
Issue comments/updates:
PR/commits:

BLOCKERS
...

NEXT P1 ISSUE
#... — <title>
```

---

# 22. Condizione di completamento globale

La feature può essere considerata completata solo quando:

- [ ] tracking GitHub è coerente e senza duplicati rilevanti;
- [ ] roadmap canonica è aggiornata;
- [ ] grid semi-trasparente è visibile in `L_DevSandbox`;
- [ ] grid è ON di default;
- [ ] toggle funziona;
- [ ] hover cella funziona;
- [ ] selection è coerente con hover e pointer contract;
- [ ] debug `FRTCellId` mostra X/Y/Layer;
- [ ] nessun Actor per cella;
- [ ] nessuna mutazione di MapState durante hover/toggle;
- [ ] nessun hidden enemy data leak;
- [ ] multilayer non è hard-coded a Layer 0;
- [ ] Automation pertinente è verde;
- [ ] PIE è verificato;
- [ ] packaged Development è verificato;
- [ ] performance/shader sanity è verificata;
- [ ] issue implementate contengono evidence reale;
- [ ] issue completate sono chiuse/risolte secondo la policy del repository.

---

# Principio operativo finale

```text
SEARCH BEFORE CREATE
REUSE BEFORE DUPLICATE
ISSUE BEFORE IMPLEMENTATION
MEASURE BEFORE CLOSE
REFACTOR BEFORE PARALLEL SYSTEM
GRID IS PRESENTATION
POINTER IS A CONTRACT
NO ACTOR PER CELL
NO HIDDEN DATA LEAK
```
