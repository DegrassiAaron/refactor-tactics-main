> 📅 **PIANIFICATO per la v0.2** il 2026-08-08 — **non** è materiale della v0.1.
> Diventa **E23** in [`../../roadmap/roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md): separazione
> geometria/logica, porta come oggetto logico unico, Stable ID e binding, interaction graph, leggibilità
> (§12.4: mai il solo colore). Gli Stable ID si decidono una volta — cambiarli dopo il primo cook invalida
> scenari, golden replay e mappe salvate.

# RefactorTactics — Consolidamento muri, porte, interazioni e validazione
## Handoff operativo per Claude Code
**Data:** 2026-08-08  
**Scope:** architettura della mappa hex, muri/porte/interazioni, documentazione, roadmap, issue e scenari di validazione

---

# 0. MISSIONE

Stai lavorando nella repository **RefactorTactics**.

Devi consolidare nel progetto le decisioni di design e architettura riportate in questo documento.

Il lavoro richiesto NON è solo documentale.

Devi:

1. analizzare lo stato reale della repository;
2. confrontare queste decisioni con documentazione, ADR, Decision Log, roadmap, issue e codice corrente;
3. aggiornare la documentazione canonica esistente;
4. creare o aggiornare ADR/Decision Log quando necessario;
5. creare le issue tecniche mancanti, evitando duplicati;
6. integrare le issue nella roadmap reale con dipendenze ed exit gate;
7. creare scenari di validazione automatici e visuali;
8. aggiornare il piano di test;
9. segnalare conflitti o decisioni ancora aperte;
10. NON implementare feature fuori dalla milestone corrente se la roadmap non le prevede ancora.

La repository corrente è la fonte di verità tecnica.

Prima di modificare qualsiasi file:

- leggi `CLAUDE.md`, `AGENTS.md` e istruzioni equivalenti;
- identifica la versione UE realmente bloccata;
- leggi Decision Log e ADR correnti;
- leggi i documenti attivi sotto `docs/` relativi a:
  - map/grid;
  - hex grid;
  - pathfinding;
  - LOS;
  - cover;
  - environment;
  - interaction;
  - action economy / phase sequence;
  - UI/UX;
  - planning/team coordination;
  - deterministic simulation;
  - TurnLog;
  - networking/privacy;
  - test harness;
  - roadmap;
  - showcase v0.1;
- controlla issue già esistenti prima di crearne di nuove;
- controlla il codice corrente prima di proporre nuove classi o tipi.

Non inventare API Unreal.
Non duplicare sistemi già implementati.
Non reintrodurre concetti square-grid superati.

---

# 1. DECISIONE PRINCIPALE — GRIGLIA TATTICA ≠ ARCHITETTURA FISICA

## 1.1 Scala

Baseline di design:

```text
lato esagono = circa 1.5 m
```

Questa scala serve a dare una dimensione tattica leggibile alle celle.

IMPORTANTE:

> L'esagono rappresenta una posizione tattica discreta, non una mattonella architettonica che deve coincidere fisicamente con muri, porte e geometria del livello.

Non assumere:

```text
1 wall segment = 1 hex edge fisico
```

Non assumere:

```text
door width = numero di lati hex concatenati geometricamente
```

Questo porterebbe muri e porte a zig-zag o angolari.

---

# 2. MURI — MODELLO CONSOLIDATO

## 2.1 Geometria e logica devono essere separate

Usare tre livelli concettuali:

```text
TACTICAL GRID
    ↓
posizioni discrete / FRTCellId / neighbor graph

ARCHITECTURE
    ↓
muri, porte, finestre, cancelli, barriere, brecce

LOGICAL GRAPH BINDING
    ↓
quali transition/edge vengono bloccati, aperti, modificati
```

Il muro fisico non deve essere l'autorità del pathfinding.

La collisione Unreal e la mesh rappresentano la geometria.

Il `MapState` / graph state deve contenere esplicitamente la verità competitiva.

---

## 2.2 Direzioni preferite

Per il layout standard, i muri rettilinei devono essere preferibilmente allineati alle **tre direttrici naturali della griglia esagonale**.

Concettualmente:

```text
          /
         / 60°
--------+-------- 0°
         \
          \ 120°
```

Le convenzioni numeriche/coordinate precise dipendono dall'orientamento hex realmente usato nella repository.

NON hard-codificare `0/60/120` senza verificare pointy-top / flat-top e convenzioni locali.

Principio:

> La griglia offre tre famiglie di direttrici architettoniche principali; un muro può estendersi in linea retta lungo una di queste direttrici per qualunque lunghezza.

Quindi un muro lungo NON deve seguire il perimetro a zig-zag degli esagoni.

---

## 2.3 Angoli

Baseline:

- angoli coerenti con le direttrici hex: preferiti;
- 60° / 120°: naturali;
- altre angolazioni, incluso 90°: possibili come eccezione;
- free-angle: possibile solo con validator/editor constraints adeguati.

Obiettivo:

> Consentire edifici credibili senza creare celle tatticamente ambigue.

---

## 2.4 Curvature / archi

Muri curvi o spline sono **compatibili con l'architettura**, ma NON sono richiesti per la prima implementazione.

Modello futuro:

```text
Wall Geometry / Spline
    ↓
binding/bake
    ↓
AffectedGraphTransitions[]
```

Il resolver non deve sapere se il muro è dritto o curvo.

Deve conoscere solamente lo stato logico delle transizioni rilevanti.

---

# 3. GRAPH BINDING — ELEMENTO CENTRALE

Ogni struttura architettonica che influenza il gameplay deve poter dichiarare o produrre una relazione con il grafo tattico.

Concetto:

```text
Structure
    ↓
AffectedTransitions / AffectedEdges
```

Esempio:

```text
Wall W01
AffectedTransitions:
  E_A_B
  E_C_D
  E_E_F
```

Effetti possibili:

```text
MovementBlocked
LOSBlocked
Opacity
DirectionalCover
ProjectileBlocking
AcousticOcclusion
EnvironmentPropagationBlocking
```

Usare i campi/tipi reali già presenti nella repository.

NON aggiungere automaticamente tutti questi domini nella prima issue se non esistono ancora.

La relazione deve essere estensibile.

---

# 4. AUTHORING VS RUNTIME

Valutare la soluzione più semplice e robusta per la repository corrente.

Possibili approcci:

## A. Binding esplicito authored

Il level/map author assegna esplicitamente gli edge interessati.

Vantaggi:

- deterministico;
- facile da validare;
- nessuna dipendenza runtime dalla fisica.

## B. Bake/editor derivato dalla geometria

Tool editor:

```text
Wall Geometry
→ intersection test
→ candidate transitions
→ author confirms / validator
→ save AffectedEdges
```

Runtime usa soltanto il dato salvato.

## C. Runtime geometric intersection

NON preferito come autorità competitiva.

Può essere usato come debug/validation, non come fonte finale dei risultati.

Claude deve scegliere in base al codice reale e documentare la decisione.

Baseline raccomandata:

> dati logici espliciti a runtime; eventuale derivazione geometrica avviene in editor/bake.

---

# 5. REGOLE DI VALIDAZIONE GEOMETRICA

Creare o aggiornare validator per impedire layout ambigui.

Come minimo valutare:

## 5.1 Cell anchor clearance

Un muro non deve attraversare troppo vicino al punto/area di occupazione tattica di una cella valida.

Esempio warning:

```text
Wall W05 overlaps playable Cell H17
Clearance below configured minimum
```

La soglia esatta deve essere data-driven o definita nella spec reale.

NON fissare ora valori arbitrari se la repository non ne ha.

## 5.2 Ambiguous transition

Errore se una struttura sembra bloccare geometricamente una transizione ma il binding logico non la blocca, o viceversa, quando la policy richiede coerenza.

## 5.3 Duplicate / conflicting binding

Errore o warning se due strutture incompatibili governano lo stesso edge senza una policy dichiarata.

## 5.4 Invalid target edge

Errore se un `AffectedEdge`:

- non esiste;
- appartiene a layer incompatibile;
- è degenerato;
- non è coerente con la mappa corrente.

## 5.5 Door aperture consistency

Una porta deve governare tutte e sole le transizioni dichiarate per la sua apertura logica.

## 5.6 Cook / stable IDs

Gli elementi competitivi devono usare ID stabili e validabili secondo la pipeline contenuti corrente.

---

# 6. PORTE — MODELLO CONSOLIDATO

## 6.1 Porta come oggetto logico unico

Una porta è un elemento architettonico/interattivo che può controllare **una o più transizioni**.

NON modellare necessariamente:

```text
porta da 3 m = due edge geometrici consecutivi
```

perché due lati consecutivi della tessellazione hex formano un angolo.

Modello corretto:

```text
Door D01
PhysicalWidth = 300 cm

AffectedTransitions:
  A <-> B
  C <-> D
```

Il giocatore vede **una porta rettilinea da 3 m**.

Il resolver vede un singolo oggetto con N transizioni governate.

---

## 6.2 Larghezze

Baseline concettuale:

```text
porta stretta / standard
~1–1.5 m
→ tipicamente 1 passaggio tattico

porta doppia
~3 m
→ può governare più passaggi tattici

gate / hangar
larghezza superiore
→ più transition possibili
```

NON derivare rigidamente il numero di transition da `WidthCm`.

La larghezza fisica serve a:

- rendering;
- authoring;
- editor validation;
- UX;
- eventuali regole future.

Il gameplay competitivo usa il binding esplicito alle transition.

---

## 6.3 Stati

Usare gli stati già esistenti se presenti.

Modello concettuale:

```text
Open
Closed
Locked
Disabled
Destroyed
```

Non introdurre stati non necessari alla milestone.

Per ogni stato definire esplicitamente gli effetti su:

- traversabilità;
- LOS;
- cover;
- eventuali propagation domains.

---

## 6.4 Porta come gruppo atomico

Se una porta larga governa più transition, lato gameplay deve restare un singolo oggetto:

```text
Door D03
State = Open
AffectedEdges = [E1, E2]
```

L'azione:

```text
Open D03
```

modifica coerentemente tutte le transition governate, salvo design esplicito contrario.

Il giocatore NON deve vedere `D03A`, `D03B` se semanticamente è una sola porta.

---

# 7. MURI DISTRUTTIBILI / BREACH

Lo stesso modello deve supportare:

```text
Wall / Barrier
    ↓
Destroyed / Breached
    ↓
affected transition state changes
```

Esempio concettuale:

```text
Wall W07
Integrity = ...
AffectedEdges = [...]
```

Breach:

```text
StructureChanged / EnvironmentChanged
W07: Intact -> Destroyed
GraphRevision++
```

Conseguenze logiche possibili:

- movimento consentito;
- LOS aggiornata;
- cover aggiornata;
- cache invalidate.

Usare il sistema cover/structure già presente, senza duplicarlo.

---

# 8. PORTE, INTERRUTTORI E DISPOSITIVI

## 8.1 Interaction graph

Separare:

```text
INTERACTION SOURCE
    ↓
INTERACTION
    ↓
TARGET(S)
```

Esempi di source:

- wall switch;
- floor plate;
- terminal;
- lever;
- generator;
- security console;
- remote device.

Esempi di target:

- door;
- gate;
- bridge;
- lift;
- blast shield;
- barrier;
- light/visibility system;
- hazard controller;
- future environmental actuator.

---

## 8.2 Stable IDs

Usare Stable ID governati.

Esempio concettuale:

```text
Switch.S01
Door.D03
Gate.G02
```

Display/tactical label separata dall'ID tecnico:

```text
StableId: Map.Foundry.Door.003
TacticalLabel: D3
DisplayName: Porta laboratorio
```

La forma precisa deve seguire le convenzioni reali della repository.

---

## 8.3 Cardinalità

Supportare concettualmente:

```text
1 source -> 1 target
1 source -> N targets
N sources -> 1 target
```

Esempio:

```text
S1 -> D1

S2 -> D2 + D3

S3 + S4 -> Gate G1
```

NON implementare logiche complesse AND/OR se non richieste dalla milestone.

Il data model non deve però impedirle in futuro.

---

## 8.4 Operazioni

Possibili operation:

```text
Open
Close
Toggle
Unlock
Lock
Enable
Disable
Activate
Deactivate
```

Usare solo quelle necessarie al vertical slice / roadmap corrente.

Non creare una mega-enum preventiva.

---

# 9. INTERACT E SEQUENZA DI RISOLUZIONE

Questa chat ha usato come esempio:

```text
Interact switch
→ door changes state
→ later actions exploit new state
```

Claude deve verificare la **sequenza di turno canonica corrente** e la phase reale dell'azione `Interact` / `Activate`.

NON sovrascrivere una decisione più recente della repository.

Se la phase attuale consente il pattern:

```text
ally opens door
→ later Blast sees open LOS
→ later Move traverses open passage
```

documentarlo.

Se non lo consente:

- segnalare il conflitto;
- proporre la modifica minima;
- aggiornare l'ADR/Decision Log solo se coerente con le decisioni correnti.

Non introdurre sequenze arbitrarie `Move -> Attack`.

Mantenere il modello di fase corrente del progetto.

---

# 10. MUTAZIONE LOGICA, REVISIONI E CACHE

Quando una struttura cambia:

```text
logical state mutation
    ↓
graph / cover / structure revision
    ↓
invalidate relevant caches
    ↓
revalidate path / LOS / targeting when required
    ↓
TurnLog event
    ↓
presentation update
```

NON:

```text
move mesh
→ hope pathfinding follows
```

Usare revisioni locali/chunk se il sistema corrente le supporta.

Aggiornare:

- graph revision;
- cover revision;
- LOS/path cache invalidation;
- targeting revalidation;
- preview invalidation.

Secondo le strutture reali esistenti.

---

# 11. TURNLOG ED EXPLAINABILITY

Non creare un event system parallelo.

Usare il `TurnLog` canonico.

Eventi concettuali da supportare tramite i tipi reali presenti:

```text
InteractionDeclared
InteractionResolved
StructureChanged
EnvironmentChanged
GraphRevisionChanged
MoveBlocked
LOSValidated / targeting reason
AbilityFizzled
```

Non aggiungere tutti i tipi se non servono.

La cosa importante è poter spiegare:

```text
Drift attempted S2
→ interaction failed

D3 remained Closed

Nyx attempted Move through D3
→ MoveBlocked
→ Reason = DoorClosed / RequiredInteractionFailed
```

I reason code devono essere logici e machine-readable.

La UI non deve ricalcolare da sola il perché.

---

# 12. UX — RELAZIONE "QUESTO PULSANTE APRE QUESTA PORTA"

Questa parte è una feature di leggibilità importante.

## 12.1 Tactical labels

Gli elementi principali devono poter avere label brevi:

```text
S1
D1
S2
D2
```

La label deve essere leggibile in mappa senza trasformare il battlefield in una tabella.

---

## 12.2 Hover source -> targets

Hover / focus su `S1`:

```text
S1 — Door Control
Controls:
  D1
```

La mappa evidenzia:

```text
S1 ==========> D1
```

La linea è un overlay UI, non necessariamente un cavo fisico.

---

## 12.3 Hover target -> known controllers

Hover su `D1`:

```text
D1 — Laboratory Door
State: Closed

Controlled by:
  S1
```

Azione UI:

```text
Focus S1
```

se coerente con il sistema di input corrente.

---

## 12.4 Non usare solo colore

Usare combinazioni di:

- tactical ID;
- icona source/target;
- linea;
- pattern;
- testo;
- highlight;
- shape.

Coerente con la grammar UI corrente.

---

# 13. KNOWLEDGE / FOG OF WAR / PRIVACY

La relazione device -> target può essere parte della conoscenza tattica.

Non assumere che tutte le connessioni siano sempre note.

Possibili stati:

```text
D3
Controller = Unknown
```

oppure:

```text
S2
Known targets:
  D3
  ???
```

La source of truth server può contenere il graph completo.

Il client deve ricevere solo relazioni autorizzate dal modello di knowledge corrente.

NON:

```text
replicate all interaction links
→ hide unknown links in UI
```

Applicare gli stessi principi di privacy architetturale del planning.

Se il sistema di knowledge non è ancora implementato:

- documentare il requirement;
- inserirlo nella milestone corretta;
- non bloccare la prima demo locale se non necessario.

---

# 14. TEAM PLANNING — DIPENDENZE TRA AZIONI

Scenario:

```text
Ally A:
Interact S1 -> Open D1

Ally B:
Move / Attack through D1
```

La UI deve poter mostrare che il piano di B dipende dal successo dell'azione di A.

Usare la grammar corrente:

```text
Confirmed
Predicted
Uncertain
```

o i nomi effettivi in repository.

Baseline:

- stato attuale porta chiusa = Confirmed;
- porta prevista aperta grazie a intento alleato = Predicted;
- esito dipendente da evento avversario/interrupt = Uncertain dove applicabile.

Warning/plan health possibile:

```text
Depends On Ally
```

Non creare leak da planning nemico.

---

# 15. PORTE E OVERWATCH / REACTION

Il sistema deve restare sistemico.

Esempio:

```text
Door opens
    ↓
LOS becomes valid
    ↓
existing reaction rules inspect current logical state
    ↓
Overwatch Opportunity if legal
```

NON creare:

```text
if DoorOpened:
    special-case Overwatch
```

La porta modifica stato.

I sistemi successivi reagiscono normalmente secondo le phase/decision boundary correnti.

Questo è coerente con la meccanica generale `OPEN -> FIRE -> SEAL`.

---

# 16. SCOPE V0.1 RACCOMANDATO

Per non esplodere lo scope, la prima iterazione dovrebbe dimostrare:

1. **Straight Wall**
   - una direttrice hex;
   - blocco movement;
   - blocco LOS/cover secondo sistema corrente.

2. **60°/120° Corner**
   - due segmenti rettilinei;
   - nessuna cell ambiguity.

3. **Standard Door**
   - singolo oggetto;
   - stato Open/Closed;
   - almeno una transition.

4. **Double Door ~3 m**
   - singolo oggetto logico;
   - più transition;
   - mesh/apertura rettilinea.

5. **Switch -> Door**
   - interaction source;
   - tactical label;
   - mapping esplicito;
   - UI relation overlay minima.

6. **Breachable Wall / Dynamic Barrier**
   - se il sistema structure/cover corrente è già pronto;
   - altrimenti roadmap issue separata.

NON richiedere subito:

- spline walls runtime-authoritative;
- editor completo tipo CAD;
- porte con fisica dinamica;
- elaborate security networks;
- circuiti logici complessi;
- decine di operation;
- procedural building generation.

---

# 17. DOCUMENTAZIONE DA CONSOLIDARE

Claude deve identificare i documenti canonici reali.

Non creare duplicati se una fonte attiva esiste già.

Come minimo controllare e aggiornare documenti equivalenti a:

## Map / Hex / Spatial

- hex coordinate model;
- map graph;
- graph transition;
- wall/edge/structure model;
- doors;
- multilayer map;
- pathfinding;
- graph revision;
- LOS;
- cover.

## Gameplay / Environment

- Interact / Activate;
- environment interactions;
- dynamic cover;
- breach;
- OPEN -> FIRE -> SEAL;
- reaction/Overwatch interactions.

## UI/UX

- hover inspector;
- tactical labels;
- structure state;
- relationship overlay;
- Confirmed / Predicted / Uncertain;
- `Depends On Ally`;
- team planning.

## Networking / Knowledge

- authorized map knowledge;
- interaction relation visibility;
- no hidden-state replication.

## Simulation / TurnLog

- StructureChanged / EnvironmentChanged;
- revision;
- deterministic state mutation;
- reason codes.

## Data / Validation

- stable structure IDs;
- map validators;
- structure-target references;
- duplicate/conflicting bindings.

## Roadmap / QA

- feature placement;
- dependencies;
- automated scenarios;
- packaged / visual tests.

Se esiste una vecchia regola del tipo:

```text
wall = hex edge mesh
```

che confligge con questa decisione, correggerla esplicitamente e registrare il cambio.

---

# 18. ADR / DECISION LOG

Verificare se serve una nuova decisione architetturale.

Possibile titolo:

```text
ADR — Separation of Hex Tactical Grid and Architectural Geometry
```

Decisione da registrare:

```text
Hex cells define tactical positions.
Architectural geometry is not constrained to hex perimeters.
Walls are preferably aligned to the three principal hex-grid directions.
Structures bind explicitly to graph transitions.
Runtime competitive state uses the logical graph binding, not physics overlap.
Doors are logical structures that may govern one or more transitions.
```

Se un ADR esistente copre già questo tema, aggiornarlo o linkarlo invece di crearne uno nuovo.

---

# 19. ISSUE — STRATEGIA

Prima:

```text
gh issue list / repo search
```

o equivalente disponibile.

NON creare duplicate.

Se una issue esistente copre almeno il 70–80% dello scope:

- aggiorna quella;
- aggiungi acceptance criteria;
- collega dipendenze.

Altrimenti crea issue focalizzate.

---

# 20. ISSUE PROPOSTE

I titoli seguenti sono indicativi.

Adattali alle convenzioni della repository.

## ISSUE A — Architectural Structure to Hex Graph Binding

### Goal

Introdurre il contratto generico:

```text
Structure -> AffectedGraphTransitions
```

### Scope

- stable structure ID;
- affected transition refs;
- movement/LOS/cover consequences usando tipi esistenti;
- deterministic logical state;
- map validation;
- debug visualization.

### Acceptance

- straight wall blocca le transition previste;
- map state contiene il binding;
- nessuna decisione competitiva dipende da runtime mesh collision;
- golden test;
- debug overlay;
- docs.

### Roadmap

Fondazione necessaria a F3 / milestone map systems corrente.
Se parte del supporto structure/cover esiste già, trasformarla in hardening/migration issue.

---

## ISSUE B — Straight Wall Authoring on Hex Principal Axes

### Goal

Permettere muri rettilinei lunghi allineati alle direttrici della griglia senza zig-zag.

### Scope

- authoring graybox;
- 3 direction families;
- 60/120 corners;
- graph binding;
- editor/debug preview.

### Acceptance

- almeno 3 wall orientations;
- segmenti lunghi realmente rettilinei;
- nessuna cell ambiguity;
- deterministic binding;
- visual validation scenario.

### Roadmap

F3 / map authoring.

---

## ISSUE C — Structure Geometry Validator

### Goal

Rilevare mappe tatticamente ambigue o binding incoerenti.

### Checks

- cell anchor clearance;
- invalid affected edge;
- conflicting binding;
- geometry/logical mismatch dove verificabile;
- duplicate structure ID;
- door aperture consistency.

### Acceptance

- validator command/editor;
- almeno fixture valide e invalide;
- machine-readable reason code;
- CI/content validation se infrastruttura già presente.

### Roadmap

F3, hardening F4/F6.

---

## ISSUE D — Door Structure with Multi-Transition Aperture

### Goal

Una porta è un singolo oggetto che controlla 1..N transizioni.

### Scope

- stable ID;
- Open/Closed minimo;
- affected transitions;
- standard door;
- double door ~3 m;
- path/LOS revalidation;
- TurnLog.

### Acceptance

- D1 1-transition;
- D2 multi-transition;
- Open/Close atomico;
- path changes;
- LOS changes;
- graph revision;
- deterministic replay.

### Roadmap

F3 — doors already belong here in the current high-level roadmap.

---

## ISSUE E — Map Interaction Source -> Target Graph

### Goal

Supportare switch/console/device che operano su porte o strutture.

### Scope

- SourceId;
- target stable IDs;
- one-to-one;
- one-to-many;
- Interact integration;
- validation;
- TurnLog.

### Acceptance

- S1 opens D1;
- S2 opens D2+D3;
- invalid target rejected;
- deterministic operation order;
- no Blueprint hard-reference used as competitive source of truth if current architecture uses IDs/data.

### Roadmap

F3/F4 depending current action framework.

---

## ISSUE F — Interaction Relationship Tactical UI

### Goal

Rendere immediatamente chiaro:

```text
which switch controls which door
```

### Scope

- tactical labels;
- source hover -> targets;
- target hover -> known sources;
- temporary connecting overlay;
- state label Open/Closed;
- accessibility.

### Acceptance

- no color-only encoding;
- S1/D1 readable;
- overlay disappears out of focus;
- supports 1->N;
- uses authorized knowledge only.

### Roadmap

F4 UI, with minimal debug version in F3.

---

## ISSUE G — Team Plan Dependency on Interactive Structures

### Goal

Rappresentare:

```text
Ally A opens D1
Ally B depends on D1 being open
```

### Scope

- derived plan dependency;
- Predicted state;
- `Depends On Ally` warning;
- stale/revalidation behavior;
- no client authoritative outcome.

### Acceptance

- planning shows dependency;
- ally changes plan -> dependent preview refreshes;
- resolution may still fail;
- reason visible through TurnLog;
- no enemy intent leak.

### Roadmap

F4 team planning/UI.
Networking hardening in F1/F5 as appropriate to current roadmap.

---

## ISSUE H — Door/Structure Network Privacy and Knowledge

### Goal

Non replicare relazioni interaction/structure che la squadra non deve conoscere.

### Acceptance

- server can retain canonical interaction graph;
- client receives authorized subset;
- unknown controller remains unknown;
- canary test for hidden relationship if/when FoW/knowledge active;
- packaged network test.

### Roadmap

Integrate with F1 privacy foundations + F4 knowledge/FoW, following actual repo milestones.

Do not block local v0.1 if knowledge system is not yet active.

---

## ISSUE I — Dynamic Structure Mutation / Breach Integration

### Goal

Integrare porte, wall breach e cover mutation nello stesso state/revision pipeline.

### Acceptance

```text
state mutation
-> revision
-> cache invalidation
-> LOS/path revalidation
-> TurnLog
-> presentation
```

- no mesh-authoritative logic;
- breach changes traversal;
- cover state changes correctly;
- golden test.

### Roadmap

F2/F3 depending current structure/ability implementation.

---

## ISSUE J — Structure Interaction Scenario Test Pack

### Goal

Aggiungere scenari automated/visual per tutti i casi sotto.

### Acceptance

- works with existing Scenario Test Harness;
- result machine-readable;
- TurnLog inspected;
- deterministic repeat where applicable;
- visual map scenario.

### Roadmap

Create alongside implementation issues, not at the very end.

---

# 21. ROADMAP — CONSOLIDAMENTO

Non creare una nuova roadmap parallela.

Aggiornare la roadmap reale.

Baseline di collocazione:

| Milestone | Integrazione |
|---|---|
| F0 / Foundations | mantenere solo i contratti già necessari: stable IDs, graph edges, deterministic MapState, TurnLog, scenario harness |
| F1 / Private Network | privacy rules applicabili anche a interaction/knowledge DTO se già introdotti |
| F2 / Abilities | ability/effect può modificare structure/cover/edge senza diventare autorità del grafo |
| F3 / Map Systems / Multilevel | wall geometry binding, door multi-transition, interaction source->target, revisions, validators, debug |
| F4 / Vertical Slice | tactical UI, ally dependencies, showcase scenarios, interactability polish, bot understanding |
| F5 / Dedicated | packaged privacy/replay/soak per dynamic structures |
| F6 / Beta | authoring hardening, accessibility, content validation, complex structures if needed |
| Future | spline walls, complex security networks, procedural architecture |

La roadmap reale può avere milestone/numerazione diversa.

Usare quella reale.

---

# 22. DIPENDENZE RACCOMANDATE

Concettualmente:

```text
Structure Graph Binding
    |
    +--> Straight Wall Authoring
    |
    +--> Door Multi-Transition
    |       |
    |       +--> Interaction Source -> Target
    |               |
    |               +--> Tactical UI
    |               +--> Team Plan Dependency
    |
    +--> Dynamic Mutation / Breach
    |
    +--> Validators

Scenario Test Pack depends incrementally on each feature.
```

Non aspettare che tutte le feature siano finite per aggiungere test.

Ogni issue implementativa deve portare il proprio test.

---

# 23. SCENARI DI VALIDAZIONE

Usare il Test Harness reale se disponibile.

Se il formato scenario corrente è diverso dagli esempi, usare quello reale.

Non creare un secondo framework.

---

## SCENARIO 1 — Straight Wall Direction 0

ID indicativo:

```text
Structure.Wall.Straight.Axis0
```

Setup:

- wall segment lungo;
- allineato a una direttrice principale;
- più transition affected.

Assert:

- tutte le transition dichiarate bloccate;
- nessuna transition laterale bloccata;
- LOS coerente;
- debug geometry/logical binding coincide;
- same StateHash/LogHash on repeat.

---

## SCENARIO 2 — Straight Wall Direction 1

```text
Structure.Wall.Straight.Axis1
```

Stessi assert per seconda direttrice.

---

## SCENARIO 3 — Straight Wall Direction 2

```text
Structure.Wall.Straight.Axis2
```

Stessi assert per terza direttrice.

---

## SCENARIO 4 — 60/120 Corner

```text
Structure.Wall.Corner.HexAligned
```

Assert:

- nessuna cell anchor invalida;
- edge binding corretto;
- path aggira il corner;
- LOS non passa illegalmente attraverso l'angolo.

---

## SCENARIO 5 — Invalid Free-Angle Wall

```text
Structure.Wall.Validation.AmbiguousCell
```

Setup:

- muro posizionato in modo da tagliare una cella/anchor oltre la policy.

Expected:

```text
VALIDATION FAIL
```

Assert:

- reason code chiaro;
- struttura identificata;
- cella identificata;
- scenario non false-passa.

---

## SCENARIO 6 — Standard Door Closed/Open

```text
Structure.Door.Standard.Toggle
```

Setup:

```text
D1 controls one traversal transition
```

Closed:

- movement blocked;
- LOS state secondo spec.

Open:

- movement enabled;
- LOS updated;
- revision updated.

Assert TurnLog.

---

## SCENARIO 7 — Double Door 3 m Multi-Transition

```text
Structure.Door.Double.MultiTransition
```

Setup:

```text
D2
Width ≈ 300 cm
AffectedTransitions = 2 or N according to map fixture
```

Assert:

- single logical DoorId;
- multiple affected transitions;
- state change is atomic;
- all transitions react consistently;
- UI shows one door `D2`, not sub-doors.

---

## SCENARIO 8 — Switch Opens Door

```text
Interaction.Switch.OpenDoor
```

Setup:

```text
S1 -> D1
```

Planning:

```text
Unit A Interact S1
```

Expected:

```text
S1 interaction resolves
D1 Closed -> Open
```

Assert:

- source and target stable IDs;
- TurnLog;
- graph revision;
- path/LOS changed as expected.

---

## SCENARIO 9 — One Switch Controls Multiple Doors

```text
Interaction.Switch.MultiTarget
```

Setup:

```text
S2 -> D2 + D3
```

Assert:

- deterministic target iteration order;
- both targets change;
- TurnLog stable;
- no dependency on TMap/TSet order.

---

## SCENARIO 10 — Door Reverse Lookup UX

```text
UI.Interaction.DoorToController
```

Visual/functional test.

Hover/focus D1.

Expected:

```text
Controlled by S1
```

Hover/focus S1.

Expected:

```text
Controls D1
```

Assert accessibility representation where automated UI tests allow it.

At minimum produce screenshot/manual visual acceptance if automated UMG assertion is premature.

---

## SCENARIO 11 — Ally Opens Door, Ally Moves Through

```text
Planning.Dependency.OpenThenMove
```

Planning:

```text
A -> Interact S1 -> D1
B -> Move path through D1
```

Expected planning:

```text
B path/result marked Predicted / Depends On Ally
```

Expected resolution if A succeeds:

```text
D1 Open
B Move succeeds
```

Assert:

- preview does not decide outcome;
- resolver uses actual state.

---

## SCENARIO 12 — Interaction Fails, Dependent Move Blocks

```text
Planning.Dependency.OpenFailsMoveBlocks
```

Cause:

- A is interrupted;
- interaction invalidated;
- or another legal deterministic cause from current systems.

Expected:

```text
D1 remains Closed
B reaches D1
MoveBlocked
Reason = DoorClosed / DependencyFailed
```

Use real reason-code taxonomy.

---

## SCENARIO 13 — Open -> Fire -> Close

Reuse/integrate existing Cover Window scenario if present.

```text
Interaction.Door.OpenFireClose
```

Sequence:

```text
A opens D1
B fires through new LOS
C closes D1
```

Assert:

- phase ordering;
- LOS revalidation;
- attack valid only while door state allows it;
- final state Closed;
- TurnLog explainable.

Do not duplicate an existing scenario with the same purpose.

---

## SCENARIO 14 — Door Open Triggers Overwatch

```text
Interaction.Door.OpenTriggersOverwatch
```

If reaction infrastructure supports it.

Expected:

```text
D1 opens
→ current LOS changes
→ reaction condition becomes valid
→ Opportunity
→ test policy chooses Commit/Hold
```

Assert:

- no special-case Door->Overwatch code;
- decision boundary uses existing reaction system.

---

## SCENARIO 15 — Breach Wall Opens Route

```text
Structure.Wall.Breach.OpensPath
```

If dynamic structure destruction exists.

Before:

```text
path unavailable / longer
LOS blocked
```

After breach:

```text
transition becomes valid
LOS updated
revision incremented
```

Assert no runtime physics authority.

---

## SCENARIO 16 — Destroyed Door / Wall Cannot Be "Closed" Normally

```text
Structure.Destroyed.NoNormalClose
```

If state model includes Destroyed.

Assert:

- closing/toggling destroyed element does not recreate structure unless explicit repair/rebuild effect exists;
- reason code clear.

---

## SCENARIO 17 — Determinism Repeat

```text
Structure.Interaction.Determinism
```

Repeat representative scenario N times using existing harness.

Assert:

- StateHash identical;
- LogHash identical;
- event order identical.

Use project-standard repeat count.

---

## SCENARIO 18 — Permutation Test

Create fixture with source->multiple targets inserted in different container order.

Assert:

- same event order;
- same final state;
- same hashes.

---

## SCENARIO 19 — Network Privacy: Hidden Controller Relation

Only when knowledge/FoW applies.

```text
Interaction.Network.HiddenRelation
```

Server canonical:

```text
S9 -> D9
```

Team B has not discovered S9.

Assert Team B does NOT receive:

- S9 ID;
- link S9->D9;
- target array;
- hidden labels.

Use canary technique already used by networking tests.

Packaged test where required by current Definition of Done.

---

## SCENARIO 20 — Showcase Visuale

Create a small dedicated area inside current dev/test map or scenario map.

Must visibly demonstrate in <= 1–2 turns:

```text
straight wall
+
wide 3 m door
+
switch S1
+
door D1
+
ally planned dependency
+
door opens
+
unit passes / shot passes
+
optional door closes
```

The observer should understand without reading logs:

> "S1 controls D1."

Required visual cues:

- `S1`;
- `D1`;
- hover/focus link;
- current door state;
- predicted ally dependency.

Then TurnLog can explain details.

---

# 24. AUTOMATION TESTS / PURE TESTS

Oltre agli scenario functional tests, aggiungere pure tests dove appropriato.

## Graph binding

- structure with N edge IDs;
- invalid edge;
- duplicate edge;
- stable sorting.

## Door state

- Closed -> Open;
- Open -> Closed;
- multi-edge atomic update.

## Revision

- relevant state mutation increments expected revision;
- non-gameplay visual change does NOT mutate logical revision.

## Serialization/hash

- stable structure IDs;
- affected edge array normalized in deterministic order;
- different insertion order -> same normalized result/hash.

## Validation

- ambiguous wall;
- invalid controller target;
- duplicate tactical label policy if one exists.

---

# 25. DEBUG TOOLING

Aggiungere/aggiornare overlay debug per mostrare:

```text
StructureId
TacticalLabel
AffectedEdges
Door State
Interaction Targets
GraphRevision
```

Possibile view:

```text
[W03]
Affected:
 E12
 E13
 E14

[D1 CLOSED]
Affected:
 E21
 E22

[S1]
Targets:
 D1
```

Usare il sistema debug reale già presente.

---

# 26. EDITOR EXPERIENCE

Obiettivo futuro/minimo:

Level designer deve poter:

1. piazzare/definire un muro rettilineo;
2. scegliere/snap alle direttrici preferite;
3. vedere le transition influenzate;
4. inserire una porta nel muro;
5. assegnare un tactical label;
6. collegare un switch alla porta;
7. eseguire Validate;
8. premere Play;
9. vedere lo scenario.

NON costruire un editor sofisticato prima che il data model sia stabile.

---

# 27. DATA-DRIVEN

Non hard-codificare:

```cpp
if (SwitchId == "S1")
{
    OpenDoor("D1");
}
```

Il mapping deve essere definito da dati/versioni/ID correnti.

Rispettare:

- stable IDs;
- versioning;
- catalog/asset manager se applicabile;
- validator;
- content hash;
- replay determinism.

---

# 28. NETWORKING

Quando la feature raggiunge networking:

Client propone:

```text
Interact SourceId
```

Server:

- valida ownership dell'unità;
- phase;
- range;
- source known/usable;
- operation legal;
- target graph canonical;
- applies logical result.

Il client NON deve scegliere arbitrariamente i target interni del source se la mappa dice:

```text
S1 -> D1
```

Il server risolve il mapping canonico.

Planning/team preview segue le policy privacy correnti.

---

# 29. DEFINITION OF DONE

Per ogni issue:

1. funziona nel modello server/client previsto per quella milestone;
2. non espone dati non autorizzati;
3. produce debug/log sufficienti;
4. usa Stable IDs;
5. non dipende da frame rate o collision physics per esiti competitivi;
6. include test automatico appropriato;
7. include scenario visuale quando la feature è spaziale;
8. passa validation/content checks;
9. è verificata packaged quando richiesto dalla milestone;
10. documentazione e roadmap aggiornate;
11. commit focalizzato.

---

# 30. OUTPUT RICHIESTO A CLAUDE

Alla fine del lavoro produrre un report Markdown, ad esempio:

```text
Docs/Reports/RT_WallsDoorsInteractions_Consolidation_Report.md
```

con:

## A. Audit

```text
File
Status before
Action
Status after
```

## B. Decisioni consolidate

Elenco breve delle nuove regole canoniche.

## C. Conflitti trovati

Per ogni conflitto:

```text
old source
new source
resolution
```

## D. Issue

Per ogni issue:

```text
Issue number
Title
Milestone
Dependencies
Acceptance criteria
```

Se GitHub non è disponibile:

- creare issue drafts versionabili;
- produrre i comandi `gh issue create` pronti;
- NON fingere di aver creato issue remote.

## E. Roadmap diff

Mostrare cosa è stato inserito/spostato e perché.

## F. Test/scenarios

Tabella:

```text
Scenario
Mode
Feature
Assertions
Current status
Roadmap checkpoint
```

## G. File modificati

Elenco completo.

## H. Test eseguiti

Comandi + risultato.

## I. Remaining risks / follow-up

Solo problemi reali ancora aperti.

---

# 31. COMMIT STRATEGY

Preferire commit focalizzati.

Esempio:

```text
docs(map): define architectural wall and graph binding model
docs(ui): define door-control relationship visualization
roadmap(map): add wall door interaction work and validation gates
test(map): add wall and multi-transition door scenarios
```

Se vengono create issue via GitHub, riferire gli ID nei commit/documenti secondo le convenzioni correnti.

---

# 32. GUARDRAIL FINALI

Non fare queste cose:

- non trasformare ogni cella in Actor;
- non usare runtime collision come autorità del graph;
- non costringere tutti i muri a seguire il perimetro degli hex;
- non modellare una porta da 3 m come due lati consecutivi se geometricamente crea un angolo;
- non dividere una porta larga in due oggetti visibili al giocatore se semanticamente è una sola porta;
- non introdurre pathfinding alternativo per le porte;
- non duplicare cover/structure systems;
- non creare una seconda interaction pipeline solo per i test;
- non replicare l'intero interaction graph ai client e poi nasconderlo;
- non creare issue duplicate;
- non riscrivere la roadmap da zero;
- non implementare spline/free-angle completo nella v0.1;
- non inventare API Unreal o tipi che la repository non richiede.

---

# 33. RISULTATO ATTESO

Dopo il consolidamento, la repository deve comunicare in modo inequivocabile:

```text
Hex grid = tactical positions

Architecture = independent geometry

Walls =
straight structures preferably aligned
to the 3 hex-grid direction families

Graph =
explicit deterministic transitions

Door =
one logical structure
controlling 1..N transitions

3 m double door =
straight physical opening
with multi-transition logical binding

Switch =
interaction source
linked by stable IDs to door/structure targets

UI =
clear S1 -> D1 relationship
without color-only encoding

Simulation =
logical mutation
-> revision
-> cache/LOS/path revalidation
-> TurnLog
-> presentation

Tests =
pure + functional + visual + determinism
+ network privacy when applicable
```

Questa è la decisione da consolidare.
