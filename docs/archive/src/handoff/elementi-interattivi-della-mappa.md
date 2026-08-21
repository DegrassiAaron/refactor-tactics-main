> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked. E' il *kit* di dettaglio
> del master Mappa & Ambiente, e precede l'intero lavoro di governance.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md).
>
> **Recepito da** [`spec-interazioni-mappa-cp101.md`](../../../gameplay/spec-interazioni-mappa-cp101.md) — §2–§13,
> filtrate contro il canone. Restano aperte `INT-1`, `INT-2` e `INT-4` in
> [`OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md); `INT-3` e' stata giudicata mal posta.
>
> ⚠️ **Non applicare**: le 15 feature `MAP-INTERACTION-*` di §25 e i 12 `MAP-INTERACT-0xx` di §23 usano
> convenzioni di ID che non sono quelle del progetto — il registro e' unico
> (`feature-registry.yaml`) e gli ScenarioId seguono
> [`scenario-index-e-tag.md`](../../../technical/tooling/scenario-index-e-tag.md). La mappatura capability→eroe di §3
> contraddice [`ADR-0006`](../../../decisions/adr-0006-ownership-abilita-sinergie.md): il requisito e' la
> capability, mai il nome dell'eroe.

# RefactorTactics — Catalogo Elementi Interattivi della Mappa
## Prompt operativo per Claude Code: consolidamento design, documentazione, Wiki, Roadmap, Feature Map, Scenario Map, Epic e Issue

## 0. Scopo

Usa questo documento come **input di consolidamento del progetto RefactorTactics**.

L'obiettivo non è creare una nuova specifica isolata, ma integrare il sistema degli **elementi interattivi della mappa** nel repository e nelle fonti di verità già esistenti.

Devi:

1. analizzare la documentazione esistente;
2. identificare feature, epic, issue, scenari e sezioni Wiki già correlate;
3. evitare duplicati;
4. consolidare i concetti descritti qui nelle specifiche esistenti;
5. aggiornare la documentazione tecnica e di game design;
6. aggiornare la Wiki;
7. aggiornare la Roadmap;
8. aggiornare la Feature Map;
9. aggiornare la Scenario Map;
10. creare o aggiornare Epic e Issue necessarie;
11. collegare Epic, Issue, Feature, Scenario, Wiki e Roadmap;
12. aggiungere test e acceptance criteria;
13. mantenere il sistema data-driven, deterministico, server-authoritative e compatibile con il modello di privacy di RefactorTactics.

Non creare documenti paralleli se esiste già una fonte canonica adatta.

---

# 1. Contesto RefactorTactics da preservare

RefactorTactics è un tattico competitivo PC-first in Unreal Engine 5 basato su:

- planning simultaneo;
- snapshot immutabile;
- resolution deterministica;
- stato logico separato dalla presentazione;
- grafo tattico 3D;
- celle e archi come dati compatti;
- pathfinding autorevole;
- LOS, targeting e trajectory come servizi separati;
- ambiente sistemico;
- turn log autorevole;
- gameplay data-driven;
- Gameplay Tags governati;
- Stable ID e versioning;
- server authoritative;
- privacy degli intenti;
- UI distinta tra **Confermato**, **Previsto** e **Incerto**.

Principio fondamentale:

> La mappa non è uno sfondo. È un sistema strategico attivo.

Il sistema qui descritto deve diventare uno dei pilastri della mappa.

---

# 2. Decisione di design principale

NON modellare la mappa come un insieme di oggetti con una sola azione del tipo:

```text
Door -> Interact
Generator -> Interact
Valve -> Interact
```

e NON modellare:

```text
ThisDoorCanBeUsedByFlux = true
```

Il modello richiesto è:

```text
Map Element
    |
    +-- State
    |
    +-- Capabilities
    |
    +-- Available Interaction Verbs
    |
    +-- Requirements
    |
    +-- Effects
```

Un elemento della mappa espone un insieme di **interaction verbs**.

Le unità possono utilizzare solo i verb per cui soddisfano i requisiti.

Esempio:

```text
Door
 ├─ Open
 ├─ Close
 ├─ ForceOpen
 ├─ Override
 ├─ Overload
 ├─ Reinforce
 ├─ Seal
 └─ Destroy
```

Lo stesso elemento deve quindi produrre gameplay differente in base a:

- personaggio;
- ruolo;
- build;
- equipaggiamento;
- stato;
- squadra;
- controllo dell'elemento;
- effetti ambientali;
- abilità;
- condizioni della mappa.

---

# 3. Non usare il personaggio come chiave primaria

Evitare requisiti hard-coded come:

```text
CanUseOnlyCharacter = Gadget
```

Preferire un sistema a capability/tag.

Esempio iniziale:

```text
Interaction.Tech
Interaction.Engineering
Interaction.Force
Interaction.Fluid
Interaction.Electric
Interaction.Precision
Interaction.Remote
Interaction.Demolition
Interaction.Sensor
Interaction.Security
Interaction.Mechanical
```

Un personaggio può possedere più capability.

Esempio concettuale iniziale:

```text
Gadget
- Interaction.Electric
- Interaction.Tech

Phase
- Interaction.Fluid

Riktor
- Interaction.Engineering
- Interaction.Force

Wraith
- Interaction.Precision
- Interaction.Sensor
```

Questi mapping devono essere verificati contro la definizione canonica attuale dei personaggi.

Non trasformare questi esempi in canon senza confronto con la documentazione corrente.

Le interazioni realmente character-exclusive devono restare eccezioni deliberate.

---

# 4. Le tre dimensioni dell'accesso

Separare esplicitamente:

## 4.1 Unit Capability

Dipende dalla unità.

Esempi:

```text
Engineer -> Repair
Hacker -> Override
Heavy -> ForceOpen
Electric specialist -> Overload
```

---

## 4.2 Team / Ownership

Dipende dalla squadra o dal controllo dell'elemento.

Possibili stati:

```text
Neutral
TeamA
TeamB
Contested
Locked
Disabled
```

Un elemento controllato può offrire verb differenti a:

- owner;
- alleato;
- nemico;
- unità neutrale;
- spettatore: nessuna interazione.

---

## 4.3 World State

Dipende dallo stato corrente dell'elemento e dell'ambiente.

Esempio:

```text
Generator.Off
Generator.Online
Generator.Overloaded
Generator.Damaged
Generator.Destroyed
```

Le interaction disponibili devono essere calcolate dallo stato corrente.

Non confondere capability, ownership e world state.

---

# 5. Gli elementi interattivi sono state machine

Ogni elemento rilevante deve poter dichiarare stati e transizioni.

Esempio:

```text
Generator_A

Off
 ├─ Start -> Online
 ├─ Repair -> Off
 └─ Sabotage -> Damaged

Online
 ├─ Shutdown -> Off
 ├─ RedirectPower -> Online
 ├─ Overload -> Overloaded
 └─ Destroy -> Destroyed

Overloaded
 ├─ Stabilize -> Online
 ├─ Disconnect -> Off
 └─ Failure -> Destroyed

Damaged
 ├─ Repair -> Off
 └─ Destroy -> Destroyed
```

Le transizioni devono essere:

- deterministiche;
- serializzabili;
- loggabili;
- validabili;
- data-driven;
- riproducibili da snapshot.

---

# 6. Catalogo iniziale delle famiglie di elementi

Creare un catalogo iniziale organizzato almeno nelle seguenti famiglie.

## 6.1 Access & Transitions

Elementi:

- porta;
- portone;
- botola;
- ponte;
- ponte mobile;
- scala;
- ascensore;
- piattaforma mobile;
- tunnel access;
- passaggio sigillato.

Sistemi coinvolti:

- pathfinding;
- graph edges;
- layer;
- choke point;
- LOS;
- facing;
- noise.

---

## 6.2 Power

Elementi:

- generatore;
- relay;
- quadro elettrico;
- trasformatore;
- batteria;
- capacitore;
- power node;
- junction box.

Sistemi coinvolti:

- elettricità;
- controllo remoto;
- alimentazione di dispositivi;
- hazard;
- objective;
- information system.

---

## 6.3 Fluid

Elementi:

- valvola;
- pompa;
- tubo;
- condotta;
- chiusa;
- floodgate;
- sprinkler;
- cisterna;
- serbatoio acqua;
- scarico.

Sistemi coinvolti:

- acqua;
- fuoco;
- pressione;
- propagazione;
- elettricità;
- superficie;
- movement cost.

---

## 6.4 Structural

Elementi:

- muro fragile;
- muro rinforzato;
- barricata;
- cover direzionale;
- cover mobile;
- cover ruotabile;
- pilastro;
- sostegno;
- parapetto;
- pannello.

Sistemi coinvolti:

- LOS;
- cover;
- trajectory;
- graph;
- destruction;
- displacement.

---

## 6.5 Hazard

Elementi:

- barile;
- serbatoio infiammabile;
- tubo gas;
- conduttore esposto;
- area elettrica;
- macchina industriale;
- vent;
- materiale esplosivo.

Sistemi coinvolti:

- fuoco;
- esplosione;
- shock;
- knockback;
- noise;
- chain reaction.

---

## 6.6 Control

Elementi:

- switch;
- pulsante;
- pannello;
- terminale;
- console;
- control station;
- remote switch;
- security panel.

Sistemi coinvolti:

- controllo remoto;
- ownership;
- activation chain;
- doors;
- bridges;
- elevators;
- traps;
- objectives.

---

## 6.7 Information

Elementi:

- telecamera;
- sensore;
- radar;
- scanner;
- relay informazioni;
- allarme;
- beacon;
- surveillance node.

Sistemi coinvolti:

- visibility;
- detection;
- sound;
- team knowledge;
- Fog of War;
- warning;
- reaction.

---

## 6.8 Tactical Device

Elementi:

- torretta;
- emplacement;
- trappola;
- drone station;
- shield node;
- mine controller;
- deployable station.

Sistemi coinvolti:

- reaction;
- control space;
- targeting;
- LOS;
- threat;
- ownership.

---

# 7. Elementi già presenti o già coerenti con il progetto

Consolidare prioritariamente elementi già emersi nella documentazione e negli scenari:

```text
Relay
Generator
Water Valve
Inflammable Tank
Directional Cover
Water
Doors
Bridges
Tunnels
Elevators
```

Non duplicare feature esistenti.

Verificare:

- nomenclatura canonica;
- issue esistenti;
- epic esistenti;
- scenario esistenti;
- feature registry;
- Wiki;
- roadmap.

---

# 8. Primo catalogo v0.1

Creare una matrice catalogo con almeno:

```text
ElementId
Category
DisplayName
PhysicalType
PlacementType
InitialState
PossibleStates
InteractionVerbs
CapabilitiesRequired
AffectedSystems
Destructible
Repairable
Hackable
RemoteControllable
OwnershipPolicy
KnowledgePolicy
NoiseProfile
GraphMutation
Tags
Version
RoadmapMilestone
FeatureId
ScenarioIds
EpicId
IssueIds
```

`PhysicalType` deve almeno distinguere:

```text
Cell
Edge
Area
Device
Structure
```

Esempi:

- Door = Edge/Structure
- Water = Cell/Area
- Generator = Device
- Bridge = Edge + Structure
- Turret = Device
- Cover = Edge/Structure

Non forzare una singola categoria se il modello attuale del progetto gestisce meglio composizione o componenti.

---

# 9. Interaction Definition

Ogni interaction verb deve avere una definizione esplicita.

Schema concettuale:

```text
InteractionId
DisplayName
Tags

SourceRequirements
TargetRequirements

RequiredCapabilities
ForbiddenStates
RequiredStates

Range
TargetingPolicy
Phase
Priority

Cost
Cooldown
Duration

Effects

GraphEffects
EnvironmentEffects
InformationEffects
NoiseEffects

FailurePolicy
InterruptPolicy
ReactionPolicy

VisibilityPolicy
KnowledgePolicy

Version
```

Non introdurre tutti questi campi immediatamente nel codice se non necessari.

Usare la struttura come modello di dominio e introdurre solo ciò che serve per la milestone corrente.

---

# 10. Azioni base e azioni specialistiche

Distinguere almeno:

## Universal / Common

Esempi:

```text
Open
Close
Activate
Deactivate
Use
Push
Destroy
```

Disponibili a molte unità se soddisfano i requisiti fisici.

## Capability interaction

Esempi:

```text
Hack
Repair
ForceOpen
Override
Overload
RedirectPower
RegulateFlow
DisableTrap
Reinforce
```

## Ability-driven interaction

Esempi:

```text
Gadget electrifies generator
Phase manipulates water system
Riktor moves/rotates structural cover
Wraith interacts with sensor/trajectory system
```

## Reaction interaction

Possibili casi futuri:

```text
Door opens during movement
Trap detects crossing
Sensor detects noise
Turret gains target
Generator overload reaches critical state
```

Questi casi devono integrarsi con il sistema Reaction/Fast Reaction senza diventare interrupt arbitrari.

---

# 11. Interazioni incrociate con ambiente e abilità

Un elemento non deve reagire soltanto a `Interact`.

Esempio:

```text
Generator
 + Water
 = WetGenerator

WetGenerator
 + Electricity
 = ShortCircuit / Arc / Overload

Generator
 + Fire
 = Damage / Critical

Generator
 + Force
 = PhysicalDamage

Generator
 + Tech
 = Shutdown / RedirectPower
```

Esempio valvola:

```text
Generic unit
 -> Open
 -> Close

Fluid capability
 -> RegulateFlow
 -> ReverseFlow
 -> IncreasePressure

Force capability
 -> BreakValve

Electric capability
 -> ElectrifyMechanism

Destroyed Valve
 -> UncontrolledLeak
```

La mappa deve favorire **compositional gameplay**.

---

# 12. Design rule: almeno tre soluzioni

Per gli elementi tatticamente importanti applicare come linea guida:

> Ogni ostacolo o affordance importante dovrebbe offrire almeno tre modi sensati di essere affrontato.

Esempio:

```text
Armored Door

1. Open / unlock normally
2. Specialist interaction
3. Force / destroy / bypass
4. Optional team combo
```

Scopo:

- evitare personaggi obbligatori;
- evitare dead content;
- favorire composizione squadra;
- creare trade-off;
- supportare counterplay.

Questa è una guideline, non una regola assoluta.

Le eccezioni devono essere deliberate e documentate.

---

# 13. Planning e Resolution

Le interazioni ambientali devono entrare nello stesso modello delle altre azioni.

Flusso:

```text
Select Unit
    |
Select Map Element
    |
Query Legal Interactions
    |
Select Interaction
    |
Preview
    |
Create Intent
    |
Commit
    |
Snapshot
    |
Resolver
    |
EnvironmentChanged / GraphChanged / etc.
    |
TurnLog
    |
Presentation
```

Non creare gameplay competitivo basato su:

```text
player presses E during resolution
```

salvo Fast Action / Fast Reaction esplicitamente progettate.

---

# 14. Fasi della Resolution

Ogni interaction deve poter dichiarare la sua fase logica.

Preservare l'ordine macro consolidato del progetto:

```text
Decision / Planning
        ↓
Prep
        ↓
Dash
        ↓
Blast
        ↓
Move
```

Il normale `Move` resta l'ultima azione volontaria.

Esempi da valutare:

```text
OpenDoor        -> Prep
HackTerminal    -> Prep
DeployCover     -> Prep

BreachDoor      -> Dash
ForcePassage    -> Dash

Overload        -> Blast
DestroyObject   -> Blast

CrossDoor       -> Move
CrossBridge     -> Move
UseElevator     -> Move / transition policy
```

Non fissare automaticamente questi mapping senza confrontarli con le regole canoniche attuali.

Aggiornare la documentazione della resolution quando necessario.

---

# 15. Grafo tattico

Gli elementi che modificano movimento devono agire sul grafo logico.

Esempi:

```text
Door.Close
 -> Edge.Enabled = false

Door.Open
 -> Edge.Enabled = true

Bridge.Deployed
 -> Create/Enable transition

Bridge.Retracted
 -> Disable transition

Elevator.AtLayer1
 -> enable transition L1

CollapsedWall
 -> disable blocker
 -> possible new edge
```

Ogni mutazione deve:

- aggiornare revisioni;
- invalidare cache;
- essere loggata;
- essere deterministica;
- produrre reason code;
- essere testabile.

Pathfinding non deve dipendere dalla mesh.

---

# 16. LOS, Cover e Targeting

Gli elementi possono influenzare:

```text
Movement
LOS
Cover
Opacity
Trajectory
Targeting
Noise
Detection
Environment
Objective
```

Non accoppiare i sistemi.

Esempio:

```text
Door open:
Movement = allowed
LOS = allowed

Transparent force field:
Movement = blocked
LOS = allowed

Smoke gate:
Movement = allowed
LOS = degraded

Low cover:
Movement = allowed
LOS = allowed
Cover = directional
```

Aggiornare il modello dati senza trasformare il pathfinding nel contenitore di tutte le regole.

---

# 17. Rumore

Integrare gli elementi interattivi con il sistema rumore.

Esempi:

```text
Open door          Noise 2
Close door         Noise 2
Force door         Noise 7
Generator online   persistent ambient noise
Alarm              high persistent noise
Explosion          Noise 10
Elevator           mechanical noise
Bridge movement    mechanical noise
Valve break        mechanical + water noise
```

I valori sono data-driven.

Il rumore deve entrare nel normale:

```text
Simulation
 -> Noise Event
 -> Sound Propagation
 -> Team Knowledge
 -> UI
```

Non replicare direttamente eventi globali ai client non autorizzati.

---

# 18. Informazione e privacy

Lo stato di un elemento può essere:

```text
Public
TeamKnown
OwnerKnown
Observed
Hidden
Unknown
```

Esempio:

Un terminale nemico potrebbe avere uno stato interno server-side non noto agli avversari.

La UI deve usare solo Team Knowledge autorizzata.

Nessun planning o stato nascosto deve essere accidentalmente esposto tramite:

- replicated Actor globale;
- GameState;
- debug UI shipping;
- tooltip;
- preview;
- warning;
- TurnLog prematuro.

Integrare con:

```text
Confermato
Previsto
Incerto
```

---

# 19. UI / UX

Quando il giocatore seleziona un elemento:

```text
DOOR A-17

State: CLOSED

AVAILABLE
[OPEN]

SPECIALIST
[OVERRIDE]       Tech
[FORCE OPEN]     Force
[REINFORCE]      Engineering

UNAVAILABLE
[REMOTE OPEN]    Remote connection required
```

La UI deve spiegare perché una interaction non è disponibile.

Reason codes esempi:

```text
MissingCapability
WrongState
OutOfRange
NoLineOfInteraction
NotOwner
Blocked
Disabled
Destroyed
InsufficientResource
WrongPhase
HiddenInformation
```

Non mostrare reason che rivelano informazioni private nemiche.

---

# 20. Data-driven e Primary Data Assets

Allineare il sistema con la pipeline esistente:

```text
Stable ID
Definition Version
Gameplay Tags
Primary Data Assets
Catalog
Validator
Manifest
Hash
```

Valutare tipi come:

```text
URTMapElementDefinition
URTInteractionDefinition
URTMapElementCatalog
```

NON introdurre nuove classi se il repository contiene già definizioni equivalenti.

Prima:

1. cercare;
2. consolidare;
3. estendere.

Poi eventualmente creare.

---

# 21. Validator richiesti

Integrare validator per almeno:

```text
duplicate ElementId
duplicate InteractionId
unknown Gameplay Tag
invalid state transition
interaction references nonexistent state
missing capability definition
invalid graph mutation
missing target element
cyclic dependency
invalid remote-control reference
interaction with impossible requirements
destructible element without valid destroyed state
repair interaction without repairable state
ranked content with unrestricted custom execution
```

Aggiungere validator progressivamente secondo milestone.

---

# 22. TurnLog

Aggiungere o estendere event type / reason code per:

```text
InteractionDeclared
InteractionValidated
InteractionFailed
InteractionStarted
InteractionResolved

MapElementStateChanged
MapElementDamaged
MapElementDestroyed
MapElementRepaired

GraphEdgeEnabled
GraphEdgeDisabled
GraphTransitionChanged

RemoteControlActivated
OwnershipChanged

EnvironmentChanged
NoiseGenerated
```

Non creare eventi ridondanti se l'event model esistente li copre già.

Ogni evento deve essere:

- ordinabile;
- serializzabile;
- replayable;
- explainable.

---

# 23. Scenari richiesti

Aggiornare la Scenario Map.

Creare scenari piccoli e focalizzati prima di uno scenario complesso.

## Scenario MAP-INTERACT-001 — Door Basics

Dimostra:

- open;
- close;
- path change;
- graph revision;
- path invalidation;
- TurnLog.

---

## Scenario MAP-INTERACT-002 — Specialist Door

Unità diverse davanti alla stessa porta.

Dimostra:

```text
Unit A -> Open
Tech -> Override
Heavy -> ForceOpen
Engineer -> Reinforce
```

Verifica che le opzioni disponibili dipendano dalla capability.

---

## Scenario MAP-INTERACT-003 — Remote Switch

```text
Switch A
    |
    +-> Door B
```

Dimostra:

- controllo remoto;
- relazione visibile UI;
- ownership;
- path update.

---

## Scenario MAP-INTERACT-004 — Generator

Dimostra:

```text
Start
Shutdown
Overload
Repair
Destroy
```

e state machine.

---

## Scenario MAP-INTERACT-005 — Water Valve

Dimostra:

```text
Open Valve
 -> Water enters cells
 -> movement/noise/environment updates
```

---

## Scenario MAP-INTERACT-006 — Water + Electricity

Dimostra:

```text
Valve
 -> Water

Gadget / electrical source
 -> Electricity

Water + Electricity
 -> conductive hazard
```

---

## Scenario MAP-INTERACT-007 — Force vs Tech

Stesso obiettivo raggiunto:

```text
Route A -> specialist
Route B -> force
Route C -> detour
```

Dimostra il principio delle 3 soluzioni.

---

## Scenario MAP-INTERACT-008 — Cover Manipulation

Dimostra:

- directional cover;
- rotate;
- destroy;
- repair;
- LOS recalc;
- trajectory recalc.

---

## Scenario MAP-INTERACT-009 — Bridge

Dimostra:

```text
Deploy
Retract
Disable
Repair
Cross
```

e transizioni multilivello / edge mutation.

---

## Scenario MAP-INTERACT-010 — Hidden Control State

Dimostra:

- state server-authoritative;
- Team Knowledge;
- Confermato/Previsto/Incerto;
- nessun leak al team nemico.

---

## Scenario MAP-INTERACT-011 — Noise Interaction

Dimostra:

```text
Open door quietly
vs
Force door
vs
Explosion
```

e propagazione sonora.

---

## Scenario MAP-INTERACT-012 — Reaction Trigger

Esempio:

```text
unit crosses controlled door
 -> sensor / trap / overwatch opportunity
```

Verificare che il reaction system utilizzi snapshot e stato autorizzato senza leak.

---

# 24. Scenario composito vertical slice

Creare successivamente uno scenario integrato che contenga almeno:

```text
Door
Generator
Valve
Water
Relay
Directional Cover
Inflammable Tank
Bridge or controlled transition
```

Obiettivo:

mostrare che lo stesso elemento può:

- essere usato;
- essere manipolato da specialisti;
- essere distrutto;
- essere usato in combo;
- modificare path/LOS;
- produrre rumore;
- modificare informazioni;
- creare nuove decisioni di squadra.

Non usare lo scenario composito per sostituire i test unitari/focalizzati.

---

# 25. Feature Map

Individuare nel registry attuale feature analoghe.

Se non esistono, proporre una gerarchia come:

```text
MAP-INTERACTION
│
├─ MAP-INTERACTION-CATALOG
├─ MAP-INTERACTION-VERBS
├─ MAP-INTERACTION-CAPABILITIES
├─ MAP-INTERACTION-STATES
├─ MAP-INTERACTION-OWNERSHIP
├─ MAP-INTERACTION-GRAPH
├─ MAP-INTERACTION-ENVIRONMENT
├─ MAP-INTERACTION-REMOTE
├─ MAP-INTERACTION-INFORMATION
├─ MAP-INTERACTION-NOISE
├─ MAP-INTERACTION-REACTIONS
├─ MAP-INTERACTION-UI
├─ MAP-INTERACTION-LOG
├─ MAP-INTERACTION-VALIDATION
└─ MAP-INTERACTION-SCENARIOS
```

NON usare questi ID se confliggono con la tassonomia esistente.

Regola:

> estendere il Feature Registry canonico, non crearne uno parallelo.

Ogni feature deve avere:

```text
FeatureId
Title
Status
Milestone
Epic
Issue
Docs
Wiki
Scenarios
Tests
Dependencies
Acceptance Criteria
```

---

# 26. Epic proposta

Cercare prima se esiste già un'epic mappa/interaction/environment.

Se esiste:

- consolidare questa feature dentro l'epic esistente;
- creare sub-issues;
- aggiungere relation.

Se non esiste, creare:

```text
EPIC — Interactive Map Elements & Affordance System
```

Scopo:

> Implementare una grammatica data-driven per gli elementi della mappa, con state machine, capability-based interactions, mutazioni del grafo, integrazione ambientale, UI, privacy, log, test e scenari.

---

# 27. Issue candidate

Consolidare con issue esistenti prima di creare.

Candidate:

### Core model

```text
[MapInteraction] Define stable IDs and map element definition
[MapInteraction] Define interaction verbs and requirements
[MapInteraction] Define capability taxonomy
[MapInteraction] Define map element runtime state machine
[MapInteraction] Add interaction availability query service
```

### Graph

```text
[MapInteraction] Support edge-state changes from interactive elements
[MapInteraction] Increment graph revision on topology mutation
[MapInteraction] Invalidate path cache after map interaction
```

### Environment

```text
[MapInteraction] Integrate water valve interactions
[MapInteraction] Integrate generator interactions
[MapInteraction] Integrate destructible cover
[MapInteraction] Integrate doors
[MapInteraction] Integrate bridge transitions
```

### Information / networking

```text
[MapInteraction] Define map element knowledge policy
[MapInteraction] Add team-sanitized interactive state DTO
[MapInteraction] Add anti-leak tests for hidden map states
```

### UI

```text
[MapInteraction] Show selectable interactive map elements
[MapInteraction] Show available/locked interaction verbs
[MapInteraction] Add interaction preview
[MapInteraction] Add safe reason codes
```

### Resolver

```text
[MapInteraction] Resolve committed map interactions
[MapInteraction] Emit map interaction TurnLog events
[MapInteraction] Add deterministic ordering for simultaneous interactions
```

### Noise / reactions

```text
[MapInteraction] Emit noise from map interactions
[MapInteraction] Support interaction-triggered reaction opportunities
```

### Data

```text
[MapInteraction] Create initial interactive element catalog
[MapInteraction] Add map element validators
[MapInteraction] Add interaction definition validators
```

### Scenarios/tests

```text
[Scenario] Door basic interaction
[Scenario] Specialist door interaction
[Scenario] Generator state machine
[Scenario] Valve and water propagation
[Scenario] Water/electric interaction
[Scenario] Cover manipulation
[Scenario] Bridge transition
[Scenario] Remote switch
[Scenario] Hidden map state privacy
[Scenario] Interaction noise
[Scenario] Interaction reaction trigger
```

---

# 28. Issue relation

Le issue non devono essere una lista piatta.

Creare relazioni esplicite.

Esempio:

```text
Catalog
   ↓
Interaction Definitions
   ↓
Capability Query
   ↓
State Machine
   ↓
Resolver
   ↓
TurnLog
   ↓
UI
   ↓
Scenario
   ↓
Packaged Test
```

Dipendenze specifiche:

```text
Door
 -> Graph Edge Mutation
 -> Graph Revision
 -> Path Cache Invalidation

Generator
 -> Environment State
 -> Electric system
 -> Noise

Valve
 -> Water system
 -> Environment propagation

Hidden Control
 -> Team Knowledge
 -> Network Sanitization

Reaction Trigger
 -> Reaction Opportunity System
```

Registrare queste dipendenze nella Feature Map/Roadmap se il repository le supporta.

---

# 29. Roadmap

Non inventare una roadmap parallela.

Aggiornare quella esistente.

Indicazione iniziale:

## F0 Fondazioni

Solo:

- identificazione elemento;
- selezione;
- interaction semplice locale;
- Door Open/Close;
- edge enable/disable;
- graph revision;
- TurnLog;
- Automation Test.

NON introdurre il catalogo completo nel codice.

---

## F1 Rete privata

Aggiungere:

- authoritative interaction requests;
- ownership validation;
- team/public element state;
- privacy tests;
- safe DTO;
- anti-leak.

---

## F2 Abilities

Aggiungere:

- capability interaction;
- ability-driven interaction;
- interaction requirements;
- GAS/UI mirror se necessario;
- Gadget/Phase/Riktor/Wraith integration.

---

## F3 Mappa multilivello

Aggiungere:

- doors;
- bridges;
- tunnels;
- elevators;
- remote switches;
- graph mutation;
- water/electric environment;
- structural elements.

Questa milestone è il punto naturale per il catalogo ambientale più ampio.

---

## F4 Vertical Slice

Aggiungere:

- catalogo v0.1 completo richiesto dalla mappa;
- UI completa;
- scenario composito;
- objective interactions;
- reaction integration;
- noise integration;
- explainability.

---

## F5 Dedicated

Aggiungere:

- packaged privacy tests;
- replication/DTO hardening;
- replay audit;
- telemetry;
- soak tests.

---

# 30. Acceptance Criteria globali

Il sistema è accettabile solo se:

1. due unità con capability diverse vedono interaction diverse sullo stesso elemento;
2. la validazione è server-authoritative;
3. il client non può forzare interaction non autorizzate;
4. map state e graph state sono serializzabili;
5. la stessa snapshot produce lo stesso risultato;
6. graph revision cambia correttamente;
7. path cache viene invalidata;
8. TurnLog spiega l'interazione;
9. UI espone reason code sicuri;
10. stato nascosto non viene replicato agli avversari;
11. scenario automatico verifica ogni comportamento;
12. packaged test passa;
13. Feature Registry, Wiki, Roadmap e Scenario Map sono collegati.

---

# 31. Automation Test minimi

Aggiungere test per:

```text
MapElement StableId
State transition validity
Capability query
Wrong capability rejected
Correct capability accepted
Wrong state rejected
Graph edge mutation
Graph revision increment
Path cache invalidation
Simultaneous interaction ordering
Interaction TurnLog
Replay determinism
Hidden state privacy
Noise event generation
```

Dove appropriato aggiungere:

```text
Functional Test
Network Test
Packaged Test
Golden Test
```

---

# 32. Debug

Aggiungere o estendere debug tooling.

Possibili comandi:

```text
rt.Map.Interactions
rt.Map.DumpElement <ElementId>
rt.Map.DumpInteractions <ElementId>
rt.Map.DebugCapabilities
rt.Map.DebugOwnership
rt.Map.DebugGraphChanges
```

Verificare naming e console command esistenti prima di aggiungerli.

Debug overlay possibile:

```text
ElementId
State
Owner
Available interactions
Required capabilities
Graph revision
Affected edges
```

Non mostrare dati server-only nei client shipping.

---

# 33. Wiki

Aggiornare la Wiki canonica.

Creare/aggiornare almeno pagine o sezioni equivalenti a:

```text
Map
Interactive Map Elements
Interaction System
Map Element Catalog
Capabilities
Map States
Doors
Generators
Valves
Structural Elements
Environmental Interactions
Remote Controls
Information Devices
Noise
Map Interaction Scenarios
```

Ogni pagina feature deve includere, quando il sistema Wiki lo supporta:

```text
Feature ID
Roadmap status
Milestone
Epic
Issues
Scenarios
Tests
Dependencies
Last update
```

La Wiki non deve diventare una copia della specifica tecnica.

Separare:

- player-facing explanation;
- design rule;
- implementation details;
- test/scenario references.

---

# 34. Documentation

Consolidare nei documenti esistenti relativi a:

```text
Architecture
Map / Pathfinding
Deterministic Simulation
Networking / Privacy
Abilities / Characters
UI / UX
Data / Validation
Roadmap / QA
Noise / Perception
Reaction System
```

Aggiornare solo sezioni rilevanti.

Non riscrivere PDR interi senza motivo.

Mantenere la regola di prevalenza del progetto:

```text
decisioni esplicite del progetto
>
requisiti consolidati
>
proposte
>
ricerca esterna
```

---

# 35. Scenario Map

La Scenario Map deve permettere di rispondere a:

```text
Quale scenario dimostra questa feature?
Quale feature viene dimostrata da questo scenario?
Quale issue implementa lo scenario?
Quale milestone deve farlo passare?
```

Creare relazioni bidirezionali:

```text
Feature -> Scenario
Scenario -> Feature
Issue -> Scenario
Scenario -> Test
Wiki -> Scenario
```

Non lasciare scenari senza feature owner.

---

# 36. Feature-to-Scenario matrix iniziale

Creare o consolidare una tabella equivalente:

| Feature | Scenario |
|---|---|
| Interaction State Machine | MAP-INTERACT-004 |
| Capability Query | MAP-INTERACT-002 |
| Graph Mutation | MAP-INTERACT-001 / 009 |
| Remote Control | MAP-INTERACT-003 |
| Water | MAP-INTERACT-005 |
| Water + Electric | MAP-INTERACT-006 |
| Multi-solution Design | MAP-INTERACT-007 |
| Cover Manipulation | MAP-INTERACT-008 |
| Hidden State | MAP-INTERACT-010 |
| Noise | MAP-INTERACT-011 |
| Reaction Integration | MAP-INTERACT-012 |

Adattare gli ID allo standard esistente.

---

# 37. Definition of Done

Una feature non è Done perché "funziona in Editor".

Done richiede:

```text
design consolidato
implementation
server validation
network/privacy behavior
debug/log
Automation/Functional Test
scenario
Wiki
Feature Map
Roadmap
packaged validation
```

Applicare la Definition of Done già esistente nel progetto.

---

# 38. Procedura operativa richiesta a Claude

Eseguire in questo ordine.

## Step 1 — Discovery

Cercare nel repository:

```text
roadmap
feature registry / feature map
scenario registry / scenario map
wiki
PDR
architecture docs
map docs
interaction docs
environment docs
GitHub issue references
epic references
```

Produrre una breve mappa:

```text
Canonical source
Existing feature
Existing epic
Existing issue
Existing scenario
Needed change
```

---

## Step 2 — Deduplication

Per ogni elemento di questo documento classificare:

```text
EXISTS
PARTIAL
MISSING
CONFLICT
```

Non creare duplicati.

---

## Step 3 — Canonical design update

Integrare:

- interaction verbs;
- capability system;
- state machine;
- ownership;
- world state;
- graph mutation;
- environment integration;
- information policy;
- data model;
- validator.

---

## Step 4 — Feature Map

Aggiornare il Feature Registry canonico.

Aggiungere status e dependency.

---

## Step 5 — Scenario Map

Aggiungere gli scenari necessari.

Collegarli alle feature.

---

## Step 6 — Roadmap

Inserire le feature nella milestone corretta.

Non spostare feature consolidate senza motivazione.

---

## Step 7 — Epic / Issue

Cercare Epic e Issue esistenti.

Poi:

```text
update existing
OR
create missing
```

Collegare:

```text
Epic
 ↕
Feature
 ↕
Issue
 ↕
Scenario
 ↕
Test
 ↕
Wiki
 ↕
Roadmap
```

---

## Step 8 — Wiki

Aggiornare pagine e backlink.

---

## Step 9 — Validation

Eseguire validator/document consistency check se disponibili.

Controllare:

- link rotti;
- ID duplicati;
- milestone incoerenti;
- feature senza scenario;
- issue senza feature;
- scenario senza acceptance criteria;
- feature Done senza test.

---

## Step 10 — Report finale

Restituire:

```text
1. File modificati
2. File creati
3. Feature create/modificate
4. Scenario create/modificati
5. Epic create/modificate
6. Issue create/modificate
7. Roadmap changes
8. Wiki changes
9. Test aggiunti/proposti
10. Conflitti trovati
11. Decisioni da approvare
12. Prossimo passo consigliato
```

---

# 39. Vincoli

NON:

- creare una seconda roadmap;
- creare una seconda Feature Map;
- creare una seconda Scenario Map;
- duplicare Epic;
- duplicare Issue;
- creare Actor per ogni cella;
- rendere Blueprint autorità competitiva;
- usare UI come validatore finale;
- usare character name come unico requisito;
- replicare stato nascosto;
- usare NavMesh come autorità;
- introdurre RNG non deterministico;
- legare outcome alle animazioni;
- introdurre decine di classi UE prima che il dominio sia consolidato.

---

# 40. Output desiderato dal lavoro di consolidamento

Alla fine il repository deve permettere di seguire una feature così:

```text
Wiki:
Door Interaction
        |
        v
Feature:
MAP-INTERACTION-DOOR
        |
        v
Epic:
Interactive Map Elements
        |
        v
Issues:
Core / Graph / UI / Test
        |
        v
Roadmap:
F0 -> F1 -> F3
        |
        v
Scenarios:
Door Basics
Specialist Door
Remote Switch
        |
        v
Tests:
Automation
Functional
Network Privacy
Packaged
```

Lo stesso principio deve essere applicabile a:

```text
generator
valve
bridge
cover
sensor
hazard
terminal
turret
```

---

# 41. Principio finale

Il risultato da ottenere non è:

> "Abbiamo un catalogo di props interattivi."

Il risultato è:

> "RefactorTactics possiede una grammatica sistemica e data-driven con cui la stessa mappa offre possibilità tattiche diverse a personaggi e squadre differenti, senza rendere obbligatorio un singolo personaggio e mantenendo determinismo, leggibilità, privacy e counterplay."

Usare questo principio per valutare ogni decisione di consolidamento.
