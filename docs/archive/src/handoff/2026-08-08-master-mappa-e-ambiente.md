> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md).
>
> **Recepito da** [`spec-interazioni-mappa-cp101.md`](../../../gameplay/spec-interazioni-mappa-cp101.md), owner
> della grammatica delle interazioni (E10 · CP 10.1): l'elemento dichiara i verbi, l'unita' le capability, e la
> legalita' esce da tre filtri indipendenti.
>
> ⚠️ **Non applicare** §11 cosi' com'e': `D-046` ha reso `Hero.Gadget.ConductiveNode` un `Action.Electrify`, e la
> conduzione e' della **cella** — non dello stato dell'unita' ([`D2` di `spec-propagazione-elettrica-cp83.md`](../../../gameplay/spec-propagazione-elettrica-cp83.md)).
> Le sigle nuove che propone (`ENV-001`, `MAP-001`) duplicano regole che hanno gia' un identificatore.

# RefactorTactics — Map & Environment Master Consolidation v0.1

**Data consolidamento:** 2026-08-09  
**Scope:** grafo tattico, architettura fisica, muri/porte/transizioni, cover, superfici, acqua/elettricità/fuoco, interazioni, capability, rumore/percezione, privacy, map scale e scenario coverage.  
**Stato:** master di consolidamento per cleanup. La repository/Decision Log corrente resta la fonte tecnica finale.

---

# 0. Principio

> La mappa non è uno sfondo. È un sistema strategico attivo.

La mappa deve produrre decisioni su:
- percorso;
- quota;
- cover;
- LOS;
- facing;
- rumore;
- visibilità/detection;
- superfici;
- hazard;
- porte/ponti/tunnel/ascensori;
- controllo di dispositivi;
- interazioni character-specific tramite capability;
- reaction e choke point.

Il modello competitivo è data-driven, deterministico e server-authoritative.

---

# 1. Modello spaziale canonico

La mappa è un **grafo tattico 3D**.

```text
Node = posizione tattica valida
Edge = transizione valida
Layer = livello sovrapposto
```

`FRTCellId` resta il riferimento logico di cella:

```text
X
Y
Layer
```

Il Layer distingue almeno concettualmente:
- ground;
- bridge;
- roof;
- tunnel;
- altri livelli sovrapposti.

Le celle e gli archi sono dati compatti centralizzati.

NON:
- un Actor per ogni cella;
- NavMesh come autorità competitiva;
- pathfinding derivato dalla mesh durante la Resolution.

Actor/mesh/collisioni servono a presentazione, authoring e interazioni visibili.

---

# 2. Griglia tattica ≠ architettura fisica

Decisione importante consolidata:

> l'esagono rappresenta una posizione tattica discreta, non una mattonella architettonica.

Baseline di design emersa:
```text
lato esagono ≈ 1,5 m
```

Questa è una scala tattica/authoring, non una regola che obbliga muri e porte a seguire a zig-zag il bordo degli esagoni.

Separare:

```text
TACTICAL GRID
    ↓
FRTCellId / neighbor graph

ARCHITECTURE
    ↓
walls / doors / windows / gates / barriers / breaches

LOGICAL GRAPH BINDING
    ↓
AffectedTransitions / AffectedEdges
```

La geometria fisica non è l'autorità del resolver.

---

# 3. Graph Binding

Ogni struttura competitiva che influenza la mappa deve poter dichiarare:

```text
AffectedTransitions[]
AffectedCells[]
AffectedCoverArcs[]
```

secondo ciò che esiste davvero nel modello corrente.

Possibili effetti, senza obbligare il primo MVP a implementarli tutti:

```text
MovementBlocked
LOSBlocked
Opacity
DirectionalCover
ProjectileBlocking
AcousticOcclusion
EnvironmentPropagationBlocking
```

Runtime preferito:

> dati logici espliciti e validati; eventuale derivazione geometrica avviene in editor/bake.

Runtime geometric intersection può essere debug/validation, non source of truth.

---

# 4. Muri

I muri sono strutture architettoniche collegate logicamente a celle/transizioni.

Linee guida:
- preferire le direttrici naturali della griglia hex per muri rettilinei;
- 60°/120° sono naturali;
- 90° o free-angle possono esistere con validator adeguati;
- curve/spline sono compatibili in futuro;
- il resolver deve vedere solo i binding logici, non la forma della spline.

Validator minimi:
- wall non attraversa una playable anchor oltre soglia;
- binding geometrico/logico non ambiguo;
- edge esistente e layer coerente;
- nessun conflicting binding non dichiarato;
- Stable ID valido.

---

# 5. Porte

La porta è **un singolo elemento logico** che può governare una o più transizioni.

Esempio:

```text
Door D03
State = Closed
AffectedEdges = [E1, E2]
```

Una porta larga non va necessariamente spezzata in `D03A`, `D03B`.

Stati concettuali:
```text
Open
Closed
Locked
Disabled
Destroyed
```

Usare solo quelli richiesti dal progetto reale.

Ogni stato deve dichiarare effetti separati su:
- traversabilità;
- LOS;
- cover;
- eventuale propagation.

Esempio:

```text
Door.Open
 -> enable relevant transitions
 -> update graph revision
 -> invalidate path cache
 -> update LOS/targeting if required
 -> TurnLog
```

Non:
```text
open animation -> hope gameplay follows
```

---

# 6. Ponti, tunnel, ascensori e transizioni speciali

Gli archi sono dati di prima classe.

## Bridge

Può:
- creare/disabilitare una transizione;
- connettere Layer diversi;
- essere strutturale/distruttibile;
- essere conduttivo quando dichiarato;
- cambiare GraphRevision.

## Tunnel

Può:
- collegare percorsi sovrapposti;
- modificare visibilità;
- modificare propagazione acustica;
- creare choke/alternative route.

## Elevator

È una transizione multilivello con stato/logica esplicita.

Non legare la disponibilità dell'ascensore al semplice stato dell'animazione.

---

# 7. Cover

Cover resta un sistema distinto da pathfinding e LOS.

La cover può essere:
- direzionale;
- bassa/alta secondo il catalogo reale;
- distruttibile;
- ruotabile/modificabile;
- creata da abilità/strutture.

Una struttura può influire contemporaneamente su:
```text
movement
LOS
cover
trajectory
```
ma questi domini non devono diventare lo stesso booleano.

Esempi utili:

```text
Transparent force field:
Movement = blocked
LOS = allowed

Smoke:
Movement = allowed
LOS = degraded

Low cover:
Movement = allowed
LOS = allowed
Cover = directional
```

---

# 8. High Ground / quota

Quota e posizionamento sono parte del sistema tattico.

Vincolo corrente da preservare:

> v0.1 non assegna automaticamente un bonus numerico di visione all'High Ground finché non esiste una decisione approvata.

La quota può influenzare:
- geometria LOS;
- cover efficace;
- targeting;
- range/ability requirement quando dichiarato.

Non introdurre genericamente:
```text
HighGround = +X damage
HighGround = +Y sight
```
senza spec corrente.

---

# 9. Superfici e stati ambientali

Famiglie già emerse:

```text
Dry
ShallowWater / Wet terrain
Conductive
Burning / Fire
Electrified
Rough
Ice
Smoke / Steam
Mud
```

IMPORTANTE: non tutte devono essere considerate già v0.1 implementate solo perché compaiono in infografiche o brainstorming.

Classificare ogni elemento nel repository:

```text
CURRENT
PARTIAL
DESIGNED
FUTURE
RESEARCH
```

---

# 10. Wet ≠ conduttività

Decisione corrente importante:

```text
Wet(unit) != Conductive(cell/edge)
```

## Wet

`ShallowWater` applica Wet mentre l'unità è sostenuta dalla cella.

Phase può applicare Wet anche fuori dall'acqua; nella guida corrente la durata adottata è 1 turno.

Interazioni v0.1 documentate:
- Wet rimuove Burning;
- `Hero.Gadget.LinearDischarge` ottiene +8 danni contro un bersaglio Wet.

Questi numeri devono restare nel catalogo/spec normativa, non sparsi nel codice.

## Conduttività

Le celle `ShallowWater` e `Conductive` possono condurre elettricità.

La propagazione segue il **grafo conduttivo**, non:
- un semplice raggio;
- una catena di unità Wet.

> Un'unità Wet su terreno asciutto non diventa un ponte elettrico.

---

# 11. Propagazione elettrica

La guida corrente documenta per `Action.Electrify`:

```text
20 danni bersaglio iniziale
12 danni unità raggiunte
max 3 passi nel grafo conduttivo
ogni cella visitata una sola volta per evento
```

Stato importante:

> il motore della propagazione risulta implementato/testato, ma nessuno dei quattro eroi v0.1 possiede normalmente `Action.Electrify` come skill standard.

Quindi non scrivere nelle pagine hero che Gadget “possiede Electrify” se il catalogo non lo dichiara.

Gadget ha invece una sinergia corrente con Wet tramite `LinearDischarge`.

Ponti possono dichiarare `bConductsElectricity`; se inattivi/distrutti interrompono la catena.

---

# 12. Fuoco, acqua, vapore

Principio sistemico da mantenere:

```text
Water + Fire
 -> extinguish / transform according to current environment rules
 -> possible Steam/Smoke if implemented
```

Le infografiche e i brief trattano:
- fuoco che si propaga;
- acqua che spegne;
- vapore/nebbia che riduce visibilità.

Prima di promuovere ogni combinazione a CURRENT:
- verificare spec ambientale più recente;
- verificare resolver/data;
- verificare scenario automatico.

Niente ricette hard-coded per coppia di eroi.

Regola:

> le abilità appartengono ai personaggi; le interazioni appartengono ai sistemi.

---

# 13. Ghiaccio, fango e terrain avanzato

Il materiale recente indica:
- slide base su ghiaccio da preservare dove già implementato;
- momentum/traction/prone/cracked ice/termica avanzata come futuro;
- Mud/Rough come terrain con costi/rischi da verificare contro il catalogo reale.

Non trasformare le infografiche in source of truth dei numeri.

---

# 14. Catalogo elementi interattivi

Famiglie iniziali:

```text
Access & Transitions
Power
Fluid
Structural
Hazard
Control
Information
Tactical Device
```

Esempi:

## Access & Transitions
- door
- gate
- hatch
- bridge
- stair
- elevator
- moving platform
- tunnel access
- sealed passage

## Power
- generator
- relay
- electrical panel
- transformer
- battery
- capacitor
- power node

## Fluid
- valve
- pump
- pipe
- floodgate
- sprinkler
- tank
- drain

## Structural
- fragile wall
- reinforced wall
- barricade
- directional cover
- movable/rotatable cover
- pillar
- support
- panel

## Hazard
- explosive barrel
- inflammable tank
- gas pipe
- exposed conductor
- industrial machine
- vent

## Control
- switch
- button
- terminal
- console
- remote control
- security panel

## Information
- camera
- sensor
- radar
- scanner
- alarm
- beacon
- surveillance node

## Tactical Device
- turret
- trap
- drone station
- shield node
- mine controller

Catalogo ampio ≠ scope di implementazione immediato.

---

# 15. Element Interaction Model

NON:

```text
Door -> Interact
Generator -> Interact
Valve -> Interact
```

e NON:

```text
ThisDoorCanBeUsedByFlux = true
```

Modello:

```text
Map Element
 ├─ State
 ├─ Capabilities
 ├─ Available Interaction Verbs
 ├─ Requirements
 └─ Effects
```

Esempio Door:

```text
Open
Close
ForceOpen
Override
Overload
Reinforce
Seal
Destroy
```

I verb legali dipendono da:
- unit capability;
- team/ownership;
- world state;
- range/position;
- status;
- environment;
- ability/build.

---

# 16. Capability, ownership, world state

Separare tre dimensioni.

## Unit Capability

Esempi concettuali:
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

Mapping hero discussi solo come esempi da verificare:
```text
Gadget -> Electric / Tech
Phase -> Fluid
Riktor -> Engineering / Force
Wraith -> Precision / Sensor
```

Non renderli canon automaticamente.

## Team / Ownership

Possibili stati:
```text
Neutral
TeamA
TeamB
Contested
Locked
Disabled
```

## World State

Esempio Generator:
```text
Off
Online
Overloaded
Damaged
Destroyed
```

Disponibilità delle interaction = funzione di tutte e tre le dimensioni.

---

# 17. State machine degli elementi

Ogni elemento importante può dichiarare stati/transizioni.

Esempio:

```text
Generator

Off
 -> Start -> Online
 -> Sabotage -> Damaged

Online
 -> Shutdown -> Off
 -> RedirectPower -> Online
 -> Overload -> Overloaded
 -> Destroy -> Destroyed

Overloaded
 -> Stabilize -> Online
 -> Disconnect -> Off
 -> Failure -> Destroyed
```

Le transizioni devono essere:
- deterministiche;
- serializzabili;
- validabili;
- loggabili;
- snapshot/replay-safe;
- data-driven.

---

# 18. Universal, Capability e Ability-driven interactions

Distinguere:

## Common
```text
Open
Close
Activate
Deactivate
Use
Push
Destroy
```

## Capability
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

## Ability-driven
Esempi concettuali:
```text
Gadget + generator/electric system
Phase + water/valve system
Riktor + structural cover
Wraith + sensor/trajectory system
```

Questi esempi NON autorizzano branch hard-coded per nome eroe.

---

# 19. Regola delle tre soluzioni

Guideline di level/system design:

> un ostacolo/affordance tatticamente importante dovrebbe offrire almeno tre modi sensati di essere affrontato.

Esempio Armored Door:
1. aprire/sbloccare;
2. specialist interaction;
3. force/destroy/bypass;
4. eventuale team combo.

Non è una legge assoluta: le eccezioni devono essere deliberate.

Scopo:
- evitare personaggi obbligatori;
- evitare dead content;
- favorire trade-off/counterplay.

---

# 20. Planning / Resolution delle interazioni

Le interazioni ambientali passano dallo stesso modello degli altri intent.

```text
Select Unit
 -> Select Map Element
 -> Query Legal Interactions
 -> Select Verb
 -> Preview
 -> Intent
 -> Commit
 -> Snapshot
 -> Resolver
 -> State/Graph/Environment change
 -> TurnLog
 -> Presentation
```

Non usare input real-time `press E during resolution` come gameplay competitivo standard.

Fast Action/Reaction sono eccezioni esplicite gestite dal framework appropriato.

---

# 21. Fasi

Ordine da preservare:

```text
Planning
-> Prep
-> Dash
-> Blast
-> Move
-> Cleanup
```

Possibili mapping solo da verificare contro le spec correnti:
```text
OpenDoor      -> Prep
HackTerminal  -> Prep
DeployCover   -> Prep
BreachDoor    -> Dash
Overload      -> Blast
CrossDoor     -> Move
UseElevator   -> transition policy
```

Non canonizzare automaticamente questi esempi.

---

# 22. Graph mutation

Qualunque interaction che modifica traversabilità:

```text
logical mutation
 -> graph/structure revision
 -> cache invalidation
 -> revalidate path/LOS/targeting when required
 -> TurnLog
 -> presentation
```

Esempi:

```text
Door.Close -> edge disabled
Door.Open -> edge enabled
Bridge.Deployed -> transition enabled
Bridge.Retracted -> transition disabled
CollapsedWall -> blocker removed / possible new edge
Elevator state -> layer transition enabled/disabled
```

---

# 23. Interaction graph source -> target

Separare:

```text
SOURCE
 -> OPERATION
 -> TARGET(S)
```

Source:
- switch;
- floor plate;
- terminal;
- lever;
- generator;
- security console.

Target:
- door;
- gate;
- bridge;
- lift;
- blast shield;
- hazard controller.

Supportare concettualmente:
```text
1 -> 1
1 -> N
N -> 1
```

Non implementare logica AND/OR complessa se non richiesta.

---

# 24. Tactical labels e UX

Gli elementi importanti possono avere label brevi:

```text
S1
D1
G2
```

Hover source:
```text
S1 — Door Control
Controls: D1
```

Hover target:
```text
D1 — Laboratory Door
State: Closed
Controlled by: S1
```

Usare:
- label;
- icona;
- linea;
- pattern;
- testo;
- highlight.

Non solo colore.

La relazione source->target può essere knowledge-sensitive: il client non deve ricevere collegamenti nascosti solo per poi nasconderli nel widget.

---

# 25. Rumore come sistema di mappa/percezione

Rumore ≠ semplice debuff.

Modello:

```text
Simulation
 -> Noise Event
 -> Acoustic Propagation
 -> Unit Perception
 -> Team Merge
 -> Team Knowledge
 -> Sanitized UI
```

`FRTNoiseEvent` concettuale:
```text
SourceUnitId
OriginCell
NoiseType
Intensity
TurnIndex
MicroStepIndex
```

Evitare campi non consumati nell'MVP.

---

# 26. Propagazione acustica

Usare il grafo tattico/multilivello.

NON usare `SphereOverlap` come autorità.

Formula concettuale:

```text
Received =
  SourceIntensity
  - DistanceCost
  - AcousticOcclusion
  - AmbientMask
  + SurfaceModifiers
```

Tutto intero/fixed-point.

Percezione base:
```text
ReceivedNoise >= HearingThreshold
```

Non:
```text
65% chance to hear
```

---

# 27. Rumore ambientale e interazioni

Fonti possibili:
- door;
- forced door;
- generator;
- alarm;
- elevator;
- bridge movement;
- valve break;
- fire;
- electricity;
- explosion;
- industrial machinery;
- waterfall;
- tunnel echo.

Valori sono data-driven.

La specifica rumore propone baseline come:
```text
Move 2
Sprint 5
Dash 6
door open 2
forced door 7
rifle 7
explosion 10
```

Sono **baseline di balance**, non costanti da incidere nel TurnManager.

---

# 28. Acoustic Mask e memoria sonora

Design da preservare:

```text
loud ambient/source noise
 -> may mask weaker noise
```

Una squadra può mantenere ghost intel:

```text
LastHeardTurn
LastHeardMicroStep
LastHeardArea
NoiseType
Confidence
```

Questa memoria non deve seguire segretamente la sorgente dopo la perdita di contatto.

---

# 29. Perception / Team Knowledge

Distinguere almeno:

```text
Visible
Detected
Identified
Last Known
Acoustic Contact
Unknown
```

Nota corrente:
- la mappa statica è principalmente nota;
- l'informazione incompleta riguarda soprattutto unità/eventi/stati knowledge-sensitive;
- non usare “Fog of War” per implicare che tutta la geometria statica sia ignota.

Il client riceve solo conoscenza autorizzata.

---

# 30. Rumore + Reaction

Un evento acustico può generare una Reaction Opportunity **solo** se una reaction/profile lo dichiara.

Non esiste una Fast Reaction acustica globale automatica.

Esempio futuro:

```text
NoiseDetected >= threshold
 -> valid reaction trigger
 -> sanitized opportunity
```

Il client può sapere:
```text
movement detected NE
```

senza ricevere:
- enemy path;
- destination;
- exact identity;
- future intent.

---

# 31. Privacy

Stato di un elemento può essere:

```text
Public
TeamKnown
OwnerKnown
Observed
Hidden
Unknown
```

Regola:
- server può conoscere il grafo/elemento completo;
- client riceve solo DTO/knowledge autorizzati;
- UI non è una misura di sicurezza.

Vale per:
- stato dispositivi;
- controller/target links;
- trap;
- sensor;
- noise;
- reaction;
- enemy interaction intent.

---

# 32. Temporal Map Size

Non dimensionare la mappa solo in metri/esagoni.

Metrica primaria:

> quanti round/Move servono per raggiungere zone tatticamente rilevanti?

Una mappa buona:
- crea decisioni dal round 1;
- contatto/contest significativo entro ~1–2 round nel formato Standard da playtestare;
- ha 2–3 macro-route realmente diverse;
- distingue route veloce, sicura, silenziosa, coperta;
- attraversamento completo sensibilmente più costoso.

Per Skirmish/vertical slice sono stati proposti ~3–4 Move normali per attraversamento completo: baseline di playtest, non numero definitivo.

---

# 33. Overwatch e level design

Overwatch direzionale richiede:
- choke;
- porte;
- ponti;
- corridoi;
- tunnel;
- route alternative;
- flank.

Level-design requirement:

> evitare una singola posizione da cui una Overwatch controlli sistematicamente tutte le rotte principali senza counterplay.

---

# 34. Scope per milestone

Usare la roadmap reale, senza crearne una parallela.

## F0 Fondazioni
Solo il minimo:
- cell/graph;
- selezione;
- interaction semplice locale;
- Door Open/Close;
- edge enable/disable;
- GraphRevision;
- TurnLog;
- Automation Test.

## F1 Rete privata
- authoritative interaction request;
- ownership validation;
- safe DTO;
- public/team-only state;
- privacy tests.

## F2 Abilities
- capability-driven interaction;
- ability/environment integration;
- requirement validation;
- hero profiles dove realmente necessari.

## F3 Map multilayer/environment
- doors;
- bridges;
- tunnels;
- elevators;
- remote switches;
- graph mutation;
- water/electric;
- structures;
- acoustic substrate secondo roadmap corrente.

## F4 Vertical Slice
- subset completo richiesto dalla mappa showcase;
- objective interactions;
- UI;
- scenario composito;
- explainability;
- noise/reaction solo se realmente verdi nello scope.

## F5/F6
- production networking/privacy/replay/telemetry secondo roadmap corrente;
- perception/noise completo nel punto realmente deciso dal repository.

Nota: alcuni handoff recenti spostano Team Knowledge/Noise a una milestone successiva rispetto al PDR iniziale. Non rinumerare automaticamente: consolidare per dipendenza, non per numero storico.

---

# 35. Feature Registry — collegamenti

Feature già emerse da riusare, non duplicare:

```text
RT-FEAT-MAP-HEXGRAPH
RT-FEAT-MAP-PATHFINDING
RT-FEAT-MAP-LOS
RT-FEAT-MAP-FACING
RT-FEAT-MAP-COVER
RT-FEAT-MAP-DYNAMIC-COVER
RT-FEAT-MAP-INTERACTIVE-EDGES
RT-FEAT-MAP-SPECIAL-TRANSITIONS
RT-FEAT-ENV-*
RT-FEAT-PERCEPTION-*
```

Per interactive elements, prima cercare feature equivalenti.

Catena desiderata:

```text
Wiki
 -> Feature
 -> Epic
 -> Issue
 -> Roadmap
 -> Scenario
 -> Test
```

---

# 36. Scenario Registry iniziale

## MAP-001 — Door Open/Close
Apertura/chiusura modifica edge e GraphRevision.

## MAP-002 — Wide Door Atomic
Una porta logica governa più transition in modo atomico.

## MAP-003 — Wall Breach
Distruzione struttura aggiorna movimento/LOS/cover secondo dati.

## MAP-004 — Switch Controls Door
Source->target interaction e TurnLog.

## MAP-005 — Hidden Controller Privacy
Relazione non nota non viene replicata al client non autorizzato.

## MAP-006 — Bridge Layer Transition
Bridge abilita/disabilita collegamento multilivello.

## MAP-007 — Conductive Bridge
Propagazione elettrica segue bridge conduttivo solo se attivo.

## ENV-001 — Wet vs Conductive
Unità Wet su terreno asciutto non crea ponte elettrico.

## ENV-002 — Water Electric Graph
Propagazione segue celle conduttive, non distanza geometrica.

## ENV-003 — Wet Removes Burning
Validare la regola corrente.

## ENV-004 — Gadget Wet Bonus
`LinearDischarge` usa il modifier del catalogo corrente.

## ENV-005 — Fire Water Interaction
Solo quando la spec ambientale corrente la marca implementata.

## ENV-006 — Ice Slide
Preservare baseline implementata se confermata dal repository.

## INTERACT-001 — Capability Query
Due unità con capability diverse vedono verb diversi sullo stesso elemento.

## INTERACT-002 — Unauthorized Verb
Il server rifiuta capability/state invalido.

## INTERACT-003 — Three Solutions
Scenario playtest per affordance critica con più approcci.

## NOISE-001 — Distance
Propagazione deterministica.

## NOISE-002 — Wall Occlusion
Rumore ricevuto con muro < senza muro.

## NOISE-003 — Ambient Mask
Masking secondo policy corrente.

## NOISE-004 — Privacy
Team non autorizzato non riceve source/origin.

## NOISE-005 — Acoustic Memory
Ghost intel non segue la sorgente dopo perdita contatto.

---

# 37. Test automatici minimi

Core:
- StableId element;
- state transition validity;
- capability query;
- wrong capability rejected;
- correct capability accepted;
- wrong world state rejected;
- graph edge mutation;
- GraphRevision increment;
- cache invalidation;
- deterministic simultaneous interaction ordering;
- TurnLog reason codes;
- replay determinism;
- conductive graph traversal;
- Wet != Conductive invariant;
- noise propagation pure tests;
- privacy canary.

Functional:
- Door;
- Bridge;
- Switch;
- Water/Electric;
- Cover/Breach;
- interaction capability;
- noise/perception quando in scope.

Packaged:
- map state mutation;
- privacy;
- replay/hash;
- scenario smoke.

---

# 38. Debug

Riutilizzare/estendere tooling esistente.

Candidate, verificando naming reali:

```text
rt.Map.Debug
rt.Map.Interactions
rt.Map.DumpElement
rt.Map.DebugCapabilities
rt.Map.DebugGraphChanges
rt.Map.DebugAcoustics
```

Overlay utili:
- CellId/Layer;
- GraphRevision;
- edge state;
- cover arcs;
- surface;
- conductive graph;
- acoustic received values;
- source->target controller links autorizzati.

---

# 39. Conflitti / rischi trovati durante cleanup

## MAP-SCOPE-01 — Infografica vs implementazione
Infografiche mostrano molte superfici/combos come linguaggio di gioco, ma non provano che siano tutte CURRENT.

Azione:
classificare ogni voce CURRENT/PARTIAL/DESIGNED/FUTURE.

## NOISE-SCOPE-01
La specifica Noise è molto dettagliata, ma handoff showcase recenti indicano che non deve bloccare la v0.1 base se non promosso esplicitamente.

Azione:
preservare design, non dichiararlo automaticamente v0.1 Done.

## INTERACTION-CAPABILITY-01
I mapping Gadget/Phase/Riktor/Wraith -> capability sono esempi concettuali nel prompt interazioni.

Azione:
non canonizzarli finché Hero Catalog/Decision Log non li approva.

## HIGHGROUND-01
Nessun bonus numerico generale di sight approvato.

Azione:
lasciare OPEN.

## ENV-NUMBERS-01
Numeri di rumore e environment presenti nei brief sono baseline/data-driven.

Azione:
non duplicarli nel TurnManager o in Wiki non normativa.

---

# 40. Chat cleanup

Dopo integrazione canonica:

Candidate ad Archive/Delete:
```text
Interazioni mappa giocatore
```

Da mantenere come documenti specialistici o archiviare dopo merge:
```text
RefactorTactics_Interactive_Map_Elements_Claude_Consolidation.md
RefactorTactics_Walls_Doors_Interactions_Consolidation_Claude_2026-08-08.md
RefactorTactics_Rumore_Claude.md
```

Il documento Rumore può restare specialistico se il dominio Perception merita una spec separata; il Master Map deve solo possederne le dipendenze e lo stato.

PDR Map/Pathfinding resta storico/architetturale utile, ma non deve sovrascrivere decisioni più recenti del repository.

---

# 41. Epic suggerite

## Interactive Map Elements
- element data/state;
- interaction verbs;
- capability query;
- ownership/world state;
- TurnLog;
- privacy.

## Dynamic Graph & Structures
- walls;
- doors;
- bridge;
- breach;
- revision/cache invalidation;
- validator.

## Environmental Systems
- surfaces;
- Wet/Burning/Conductive;
- water/electric;
- fire/water;
- ice/rough;
- deterministic propagation.

## Perception & Noise
- noise events;
- acoustic propagation;
- hearing;
- Team Knowledge;
- sound overlay;
- privacy;
- reaction hooks.

## Map Authoring & Validation
- graph binding;
- geometry/bake;
- Stable IDs;
- ambiguous edge validator;
- debug overlay.

---

# 42. Exit criteria

Il cluster Map & Environment è consolidato quando:

1. esiste una sola tassonomia current per cell/edge/structure/device;
2. griglia tattica e architettura fisica sono separate;
3. Door/Bridge/Breach modificano il grafo via stato logico;
4. interaction verbs/capability sostituiscono branch per eroe;
5. Wet e Conductive sono documentati come concetti distinti;
6. water/electric usa il grafo conduttivo;
7. feature ambientali sono classificate per stato reale;
8. noise/perception ha scope di milestone esplicito;
9. privacy di device links/noise è documentata e testata;
10. Scenario Registry contiene MAP/ENV/INTERACT/NOISE rilevanti;
11. Wiki, Feature Registry e Roadmap puntano agli stessi owner;
12. la chat “Interazioni mappa giocatore” può uscire dal CORE senza perdita.
