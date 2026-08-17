# REFACTORTACTICS — Graybox Kit, Cover Visual Grammar e Cell Placement Volume

> `HISTORICAL` · **Materiale NON autorevole**, archiviato il **2026-08-17** dopo il consolidamento.
> **Non si applica**: si legge per sapere da dove viene una decisione. Le fonti autorevoli restano
> [`RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md),
> [`spec-graybox-placement-contract.md`](../../technical/spec-graybox-placement-contract.md) — l'owner nato
> da qui — e [`feature-registry.yaml`](../../roadmap/feature-registry.yaml).
>
> **Recepito da**: `D-152` (contratto di ingombro, pivot e presentazione) · `D-153` (innesto sulla release
> ladder canonica, nessuna epic nuova) · feature `RT-FEAT-UI-GRAYBOX-KIT` · `GBX-1`…`GBX-4` in
> [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), issue
> [#1094](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1094) · seduta **U25** in
> [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), issue
> [#1095](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1095) · voci PIE
> [#1096](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1096) · epic **E21**
> [#286](https://github.com/DegrassiAaron/refactor-tactics-main/issues/286) e **E45**
> [#778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/778) aggiornate.
>
> 🔴 **Tre prescrizioni NON sono entrate, perché descrivono un repository che non esiste.**
> **(§8)** «rotation step = 30°»: la grammatica canonica non ha uno step di rotazione — è
> `ERTTacticalAxis` più offset **interi** in tre famiglie, e ammette configurazioni a **90°**. I dodici
> settori da 30° esistono ma sono un **righello** che misura quanto una cella è invasa, e la spec dichiara
> che *«non definiscono vicini, non definiscono facing, non definiscono su quale lato sta una copertura»*.
> **(§3.2)** La cover come oggetto della cella: è direzionale **per bordo** dal formato mappa v3 (`E9.1`).
> **(§3.1)** «Temporary / Energy Cover»: `ERTHexCoverType` ha `None · Low · High`, e il commento sopra
> l'enum dichiara il criterio — *«inventare oggi un valore che nessuna regola sa applicare»* sarebbe
> l'errore. L'owner è
> [`spec-coperture-temporanee-cp95.md`](../../gameplay/spec-coperture-temporanee-cp95.md).
>
> ⚠️ **Una quarta la respinge il repository, non il filtro.** La **valvola** sta fra i diciannove elementi
> «v0.1» della §9, mentre
> [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) §11 la dichiara fuori
> scope con motivazione registrata: *«l'acqua ha un produttore nel roster (`D-046`,
> `Hero.Phase.FluidTrail` **è** `Action.CreateWater`) e non serve un secondo modello per crearla»*.
>
> 🔵 **La ladder di maturità della §14 è arretrata rispetto al progetto, non in anticipo.** Mette
> Environment in v0.2, Perception in v0.7 e Objectives in v0.8: sul repository sono tutti e tre lavoro
> della **v0.1**. Seguirla avrebbe rinviato alle release future gli asset di sistemi che la release già
> possiede. ⚠️ *Il grado differisce, e la prima stesura di questo banner lo appiattiva in «in gran parte
> `INTEGRATED`»: vale per **E8** (8 feature su 8), mentre **E13** ed **E10** non ne hanno nessuna
> `INTEGRATED`. Corretto in code review — la conclusione regge, la prova era più forte del vero.* Delle
> dieci righe **una sola** coincide esattamente — il contract freeze in v1.0; Core map è a cavallo, con la
> coda di muri e porte in v0.2. La tabella di riconciliazione vive in
> [`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md).
>
> ✅ **Due cose che questo documento non sapeva, e che l'audit ha trovato misurando.**
> `ERTHexDoorState` ha **quattro** stati e la §3.4 ne conosce tre: `Locked` e `Closed` negano entrambi il
> passaggio e sono **geometricamente identici**, quindi distinguerli col solo colore violerebbe `D-146` al
> primo asset prodotto — è `GBX-2`, e resta aperta. E gli elementi interattivi **non si esauriscono
> nell'`Online/Offline` della §11**: l'esempio di CP 10.1 §5 ne mostra cinque
> (`Off · Online · Overloaded · Damaged · Destroyed`), dove `Overloaded` e `Damaged` hanno transizioni
> d'uscita diverse e fonderli toglierebbe al giocatore l'informazione che decide l'azione successiva.
> ⚠️ **Cinque non è il numero canonico**: quel blocco è etichettato *«Esempio, non catalogo»* e ogni
> elemento dichiara i propri stati. Il contratto impone la ridondanza per **qualunque** cardinalità.
>
> ⚠️ **Sette dei diciannove elementi sono `DEFER`**, e si dividono per ragione: **tre** per dipendenza da
> feature `IDEA` su release `future`, **due** fuori scope v0.1 dichiarato, **due** proxy senza
> produttore. Il kit «completo» non è raggiungibile in v0.1, e non è un ritardo:
> sette voci su diciannove descrivono sistemi che il progetto non ha ancora deciso di costruire. Il conto
> completo è `REUSE 2 · UPDATE 8 · CREATE 2 · DEFER 7`, e la classificazione voce per voce è in
> [`spec-graybox-placement-contract.md`](../../technical/spec-graybox-placement-contract.md) §8.

## Handoff operativo per Claude Code / Claude Cloud — consolidamento + roadmap + issue

**Data:** 17 agosto 2026  
**Tipo documento:** handoff operativo da consumare e consolidare nella repository, **NON nuova source of truth**.  
**Scopo:** consolidare le decisioni prese nella sessione su **cover graybox, kit graybox 3D, Cell Placement Volume, regole dimensionali/pivot/snap, roadmap asset fino alla v1.0**, riconciliarle con lo stato reale di `main`, aggiornare tracking/documentazione e creare o aggiornare le issue necessarie.

---

# 0. Regola assoluta: repository first

Prima di modificare qualsiasi cosa:

1. `git fetch --prune`;
2. verificare branch/worktree e HEAD;
3. leggere `CLAUDE.md`, `AGENTS.md` e istruzioni equivalenti;
4. verificare la versione Unreal realmente bloccata nel repository;
5. leggere Decision Log / ADR / Open Decisions;
6. leggere roadmap v0.1 → v1.0 corrente;
7. leggere Feature Registry;
8. leggere Lane/Issue Map, Scenario Map, Editor Map, Asset tracker, QA/Test tracker;
9. cercare issue **aperte e chiuse** su:
   - graybox;
   - cover;
   - wall / low wall;
   - door;
   - map authoring;
   - geometry grammar;
   - asset readability;
   - cell footprint / placement;
   - editor/debug volume;
   - rubble/debris;
   - environment states;
10. verificare codice e Content già esistenti;
11. classificare ogni richiesta di questo handoff come:

```text
AS_BUILT
PARTIAL
PLANNED
BLOCKED
SUPERSEDED
MISSING
```

12. per ogni elemento decidere:

```text
REUSE
UPDATE
CREATE
DEFER
```

**Non creare sistemi, Epic, tracker o roadmap parallele.**

La repository live vince sempre su questo handoff in caso di conflitto.

---

# 1. Baseline nota da riconfermare

Da documentazione recente, la baseline attesa è:

- Unreal Engine 5.8.1 o patch UE 5.8.x realmente bloccata su `main`;
- v0.1 = vertical slice deterministico 2v2 offline vs bot;
- mappa tattica hex come substrato logico;
- geometria architettonica separata dalla griglia tattica;
- MapState / dati canonici decidono traversal, cover, LOS, trajectory;
- mesh/collisione graybox = presentazione, authoring e debug, **non authority competitiva**;
- Asset lane già esistente con una voce tipo `AST-001 Graybox kit`;
- Editor lane già include graybox authoring / `L_DevSandbox`;
- E23 / issue GitHub equivalente possiede muri, porte e interaction graph in v0.2, se ancora canonico;
- la roadmap corrente arriva già fino a v1.0;
- non assegnare nuovi ID globali o numeri Epic/issue a memoria.

Verificare tutto sul live repository prima di scrivere.

---

# 2. Decisioni consolidate della sessione

## 2.1 Graybox 3D, non simboli piatti

Decisione:

> La rappresentazione semplificata della mappa deve usare **oggetti 3D graybox semplici**, non una grammatica basata principalmente su simboli 2D.

Principio:

```text
GEOMETRIA 3D
    = che cosa è / silhouette / ingombro

COLORE + ACCENT
    = stato o famiglia funzionale

TRASFORMAZIONE FISICA
    = stato meccanico reale

OVERLAY / VFX MINIMALE
    = stato ambientale
```

Non trasformare il Graybox Kit in produzione artistica.

---

## 2.2 Pochi master mesh, molti prefab/configurazioni

Obiettivo:

- pochi master mesh;
- scaling;
- rotazione;
- composizione;
- Material Instance / parametri;
- Blueprint/prefab riutilizzabili.

Baseline proposta da consolidare:

```text
SM_GB_Box
SM_GB_Cylinder
SM_GB_Plane
SM_GB_Hex
SM_GB_Wedge
SM_GB_Pipe
SM_GB_Stair
SM_GB_Rubble
```

Questa lista è **concettuale**: se la repository possiede già primitive equivalenti, riutilizzarle.

Non creare duplicati solo per ottenere naming identico.

---

# 3. Cover — visual grammar consolidata

## 3.1 Forma

La cover graybox è un **oggetto 3D direzionale**.

Baseline:

```text
Low Cover
= parallelepipedo basso

High Cover
= parallelepipedo alto

Temporary / Energy Cover
= pannello sottile verticale
  solo quando realmente richiesto dallo scope
```

Low e High Cover devono appartenere chiaramente alla stessa famiglia visiva.

---

## 3.2 Cover non riempie la cella

La cover non è un terreno.

È un elemento spaziale/strutturale, normalmente:

```text
EdgeBound
```

e si aggancia alla geometria/segmento rilevante.

Non codificare `Cell.HasCover = true` come unico modello concettuale se il sistema corrente usa segmenti/bake/directional cover.

---

## 3.3 Stato visuale della cover

Baseline approvata:

```text
INTACT
- geometria normale
- grigio/neutro

DAMAGED
- stessa geometria
- giallo
- almeno un marker/pattern geometrico o fascia diagonale

CRITICAL
- stessa geometria
- arancione
- pattern più evidente / doppia fascia

DESTROYED
- la cover non resta semplicemente rossa
- geometria principale rimossa/collassata
- compare rubble/debris basso
```

Regola:

> Non affidarsi solo al colore.

---

## 3.4 Open / Closed / Rotated / Moved

Per elementi meccanici:

```text
OPEN
CLOSED
ROTATED
MOVED
RAISED
LOWERED
```

la geometria deve mostrare realmente lo stato.

Esempio:

```text
Closed door
-> pannello nel passaggio

Open door
-> pannello ruotato/spostato

Destroyed door
-> pannello rimosso + residuo/rubble se previsto
```

Non usare:

```text
porta ciano = aperta
```

come informazione primaria.

---

# 4. Cell Placement Volume — nuova decisione v0.1

Aggiungere alla v0.1 una guida/volume di authoring per definire quanto spazio può occupare un asset legato a una singola cella.

Nome di lavoro:

```text
BP_GB_CellPlacementVolume
```

Il nome reale deve rispettare le convenzioni del repository.

---

## 4.1 Forma

Il volume è un **prisma esagonale 3D** derivato dalla cella logica.

Visualizzazione Editor/debug:

```text
HEX CELL VOLUME
+ Safe Placement Volume inset
+ guide verticali
```

Default proposto:

```text
Outer footprint = 100% della cella
Safe footprint  ≈ 90% della cella
```

Il `90%` è una **baseline di design da validare visivamente**, non un numero competitivo sacro.

Se il sistema corrente ha metriche/clearance già canoniche, usare quelle.

---

## 4.2 Purpose

Serve a:

- modellare asset proporzionati;
- evitare che un `SingleCell / CellBound` invada la cella adiacente;
- definire una silhouette coerente;
- supportare futuro asset replacement;
- validare pivot e footprint;
- preparare deployable, devices, objective, unit placeholder;
- diventare contratto dimensionale per art production.

Il volume è:

```text
Editor/debug only
```

e non deve diventare un Actor autorevole della simulazione.

---

# 5. Placement taxonomy v0.1

Consolidare una tassonomia minima.

## `CellBound`

Asset contenuto nella cella / Safe Placement Volume.

Esempi:

```text
Unit
Generator
Hazard Tank
Relay
Valve
small interactive
Rubble single-cell
```

Regola:

> La geometria gameplay-significativa di un asset `CellBound` deve rispettare il Safe Placement Volume.

---

## `EdgeBound`

Asset ancorato a un segmento/bordo/transizione/struttura.

Esempi:

```text
Wall
Low Cover
High Cover
Door
Barrier
```

Non forzarli dentro il Safe Placement Volume `CellBound`.

---

## `SurfaceBound`

Overlay o stato visuale praticamente coincidente con la superficie della cella.

Esempi:

```text
Water
Ice
Wet
future fire/electric/steam/smoke renderers
```

---

## `EditorOnly`

Marker e guide.

Esempi:

```text
CellPlacementVolume
SpawnMarker
debug anchors
coverage/footprint guides
```

---

## `MultiCell`

**Previsto ma fuori scope v0.1**.

Entrerà quando servono davvero asset grossi, payload, macchinari 2x1/2x2 o strutture equivalenti.

Non implementare prematuramente.

---

# 6. Dimension grammar v0.1

Non assumere centimetri assoluti finché il repository non li blocca.

Usare una metrica relativa.

Definizioni concettuali:

```text
C = distanza centro-centro fra due celle adiacenti
E = lato/segment metric coerente con la geometry grammar corrente
Z=0 = piano di appoggio
```

Guide verticali proposte nel `CellPlacementVolume`:

```text
1.00 C  MAX GUIDE
0.85 C  STRUCTURAL
0.55 C  STANDARD
0.28 C  LOW
0.00 C  FLOOR
```

Queste sono **guide di modellazione graybox**, non categorie di targeting/LOS automatiche.

Se il repository ha già HeightClass/standing eye height/cover height canonici, non duplicarli: mappare le guide visuali ai dati esistenti.

---

# 7. Pivot contract

## CellBound

```text
Pivot = bottom-center del footprint
```

Obiettivo:

```text
ActorLocation = CellWorldAnchor
```

senza offset manuali arbitrari.

---

## EdgeBound

```text
Pivot = centro del segmento, alla base
+X = lungo il segmento
+Y = lato/frontale convenzionale
+Z = alto
```

La semantica reale dell'asse va adattata alle convenzioni già presenti.

---

## Elementi mobili

L'Actor mantiene l'anchor/pivot logico.

La parte mobile usa un child component / sub-pivot.

Esempio:

```text
Door Actor
 ├─ Frame
 └─ Panel
      └─ hinge / translation pivot
```

Non spostare l'identità logica della porta per animarla.

---

# 8. Snap / rotation

Baseline di design richiesta:

```text
rotation step = 30°
```

per supportare una grammar architettonica che non sia limitata al solo bordo hex e possa includere configurazioni ortogonali/bisettrici già discusse nel progetto.

**ATTENZIONE:** verificare la geometry grammar reale su `main`.

Se il runtime/authoring usa già una quantizzazione diversa, consolidare lì invece di introdurre un secondo sistema.

Modalità concettuali:

```text
Cell Snap
Edge Snap
Free Architectural / Quantized Snap
```

---

# 9. Catalogo v0.1 richiesto

Il catalogo graybox v0.1 deciso nella sessione comprende **19 elementi**.

L'implementazione reale deve riutilizzare prefab/asset già esistenti quando possibile.

| # | Elemento di lavoro | Placement | Scopo |
|---:|---|---|---|
| 1 | `GB_Cell` | Editor/debug | lettura cella / grid |
| 2 | `GB_CellPlacementVolume` | EditorOnly | safe footprint + height guide |
| 3 | `GB_Unit` | CellBound | placeholder cilindrico + facing |
| 4 | `GB_Floor` | architectural | pavimento continuo |
| 5 | `GB_Wall` | EdgeBound | struttura alta |
| 6 | `GB_CoverLow` | EdgeBound | low directional cover |
| 7 | `GB_CoverHigh` | EdgeBound | high/full cover visual |
| 8 | `GB_Door` | EdgeBound | accesso open/closed |
| 9 | `GB_Rubble` | CellBound | distruzione/debris baseline |
| 10 | `GB_WallBroken` | EdgeBound | breach leggibile |
| 11 | `GB_Ramp` | transition preview | preparazione verticalità |
| 12 | `GB_Platform` | architectural | platform/upper support |
| 13 | `GB_SurfaceState_Water` | SurfaceBound | water readability |
| 14 | `GB_SurfaceState_Ice` | SurfaceBound | ice readability |
| 15 | `GB_Valve` | CellBound | Fluid interactive |
| 16 | `GB_Generator` | CellBound | Power interactive |
| 17 | `GB_HazardTank` | CellBound | hazard/explosive proxy |
| 18 | `GB_Relay` | CellBound | objective proxy |
| 19 | `GB_SpawnMarker` | EditorOnly | spawn/team authoring |

---

# 10. Dimensioni relative proposte per v0.1

Sono **baseline visuali da validare**, non authority gameplay.

## Unit

```text
diametro ≈ 0.23 C
altezza  ≈ 0.52 C
```

Decisione importante:

> i cilindri-unità devono essere visivamente più piccoli di quanto sono oggi se oggi invadono troppo la cella.

Devono lasciare spazio leggibile per:

- cover;
- path;
- facing;
- hazard;
- surface;
- interaction markers.

---

## Wall

```text
lunghezza ≈ modulo/segmento canonico
spessore  ≈ 0.10–0.12 C
altezza   ≈ 0.85 C
```

Non legare la lunghezza del muro a un lato hex se la geometry grammar corrente non lo fa.

---

## Low Cover

```text
lunghezza ≈ 0.85–0.95 del segmento utile
spessore  ≈ 0.10 C
altezza   ≈ 0.28 C
```

---

## High Cover

```text
lunghezza ≈ 0.85–0.95 del segmento utile
spessore  ≈ 0.10 C
altezza   ≈ 0.68–0.70 C
```

---

## Rubble

```text
footprint max ≈ 0.65 C
altezza       ≈ 0.10–0.15 C
```

---

## Generator

```text
footprint ≈ 0.35 C x 0.30 C
altezza   ≈ 0.45 C
```

---

## Hazard Tank

```text
diametro ≈ 0.22–0.25 C
altezza  ≈ 0.48–0.52 C
```

---

## Relay

```text
footprint ≈ 0.25–0.30 C
altezza   ≈ 0.58 C
```

---

## Valve

```text
footprint ≤ 0.25 C
altezza   ≈ 0.25–0.35 C
```

---

## Ramp / Platform

Trattare come graybox di authoring finché il multilayer runtime non è owner della transizione.

Non far decidere la transizione alla mesh.

---

# 11. Palette / visual language

Separare **corpo strutturale**, **accent funzionale** e **stato**.

## Corpo

```text
Structural / neutral = gray
```

---

## Accent funzionale

Baseline:

```text
Fluid       = Blue
Power       = Cyan
Hazard      = Orange
Objective   = Purple / Magenta
Interactive = Amber (solo se utile e non conflittuale)
```

Non colorare necessariamente l'intero oggetto.

Preferire:

```text
body neutral
+ functional strip/ring/emissive accent
```

---

## Integrità

```text
Intact    = neutral
Damaged   = Yellow + non-color marker
Critical  = Orange + stronger marker
Destroyed = geometry change / rubble
```

---

## Stati funzionali

```text
Online/Active = accent on
Offline       = accent off
Disabled      = desaturated / marker
Open          = geometry transformed
Closed        = geometry closed
```

Non riutilizzare lo stesso colore per due significati incompatibili se la forma/pattern non basta a disambiguare.

---

# 12. Regola fondamentale — visual ≠ authority

Inserire o consolidare esplicitamente nella documentazione:

> La collisione della mesh graybox e la mesh stessa non sono authority delle regole competitive.

Il gameplay deve leggere dati canonici:

```text
MapState
TransitionState
Cover/bake data
LOS inputs
Trajectory blockers
Interaction state
Surface/environment state
```

e non:

```text
"se la mesh è lì allora blocca"
```

La presentazione deve seguire lo stato logico, non crearlo.

---

# 13. Validation map / scenario visuale v0.1

Creare o aggiornare una scena di validazione minimalista, preferibilmente dentro il workflow già esistente di `L_DevSandbox` / Scenario Harness / Editor Map.

Baseline proposta:

```text
5x5 circa
2 Units
1 Low Cover
1 High Cover
1 Wall
1 Broken Wall
1 Closed Door
1 Open Door
1 Water Cell
1 Ice Cell
1 Valve
1 Generator
1 Hazard Tank
1 Relay
1 Ramp
1 Platform
```

Testarla a:

```text
near zoom
normal gameplay zoom
far tactical zoom
```

Acceptance visuale:

senza selezione e senza HUD dettagliato deve essere possibile distinguere:

```text
Unit
Low vs High Cover
Wall vs Broken Wall
Door Open vs Closed
Water vs Ice
Fluid device
Power device
Hazard
Objective
```

Se non è leggibile, modificare la grammar **prima** di aggiungere altri asset.

---

# 14. Roadmap asset — intento di maturità deciso nella sessione

La sessione ha definito questa sequenza concettuale:

```text
v0.1  CORE MAP / GRAYBOX CONTRACT
v0.2  ENVIRONMENT
v0.3  3D MAP / VERTICALITY
v0.4  INTERACTIVE MAP
v0.5  TACTICAL DEVICES / AUXILIARY
v0.6  DESTRUCTION / DEBRIS
v0.7  PERCEPTION / INFORMATION DEBUG
v0.8  OBJECTIVE KIT
v0.9  MODULARIZATION / PRODUCTION GRAYBOX
v1.0  ART-REPLACEMENT CONTRACT FREEZE
```

**IMPORTANTE:** la roadmap repository corrente può avere release theme diversi.

Claude deve:

1. preservare questa **sequenza di maturità del Graybox Kit**;
2. NON creare una roadmap parallela;
3. mappare ciascun cluster sulla **release canonica attuale**;
4. se la release mapping della sessione confligge con `main`, usare `main` e registrare il delta;
5. aggiornare Asset lane / Editor lane / roadmap owner esistenti.

---

# 15. Mapping asset desiderato per cluster

## Core Map / v0.1 intent

```text
Cell
CellPlacementVolume
Unit
Floor
Wall
WallBroken
LowCover
HighCover
Rubble
Door
Ramp
Platform
Water
Ice
Valve
Generator
HazardTank
Relay
SpawnMarker
```

---

## Environment cluster

```text
Pipe
Drain
Floodgate
PowerNode
ElectricalPanel
Fire overlay/VFX
Wet overlay
Electric overlay/VFX
Steam
Smoke
Debris visual variants
```

---

## 3D Map / Verticality cluster

```text
Stairs
Ladder
Bridge
Catwalk
TunnelEntrance
TunnelPortal
UpperFloor
Roof
Hatch
ElevatorPlatform
ElevatorShaft
```

---

## Interactive Map cluster

```text
Terminal
Switch
Button
Gate
MovableCover
RotatableCover
Camera
Sensor
Alarm
Beacon
```

---

## Tactical Devices cluster

```text
Mine
Trap
Turret
ShieldNode
DroneStation
DeployableBase
RemoteDevice
```

---

## Destruction / Debris cluster

```text
FragileWall
StructuralPillar
BreachSmall
BreachLarge
DebrisSmall
DebrisMedium
DebrisLarge
DebrisPile
CollapsedStructure
```

Non duplicare il sistema Rubble/Debris già consolidato altrove.

---

## Perception / Information cluster

Prevalentemente debug/overlay:

```text
NoiseSource
NoisePropagation
AcousticMask
VisibilityVolume
DetectionArea
Scanner
Radar
SurveillanceNode
```

---

## Objective cluster

```text
ControlNode
ControlZone marker
Payload proxy
PayloadPath marker
ExtractionZone
DynamicObjective
ObjectiveBarrier
```

---

## Production Graybox / modularization cluster

Consolidare:

```text
Short / Standard / Long variants
30° / 60° / 90° / ... quantized orientations
InsideCorner
OutsideCorner
T-Junction
CrossJunction
EndCap
VerticalJunction
Prefab compositions
snapping contract
pivot contract
footprint contract
```

Solo se compatibile con la geometry grammar canonica.

---

## v1.0 contract freeze

Alla v1.0 il Graybox Kit deve funzionare come contratto per l'asset replacement.

Freeze su:

```text
Footprint
Pivot
Snap
Placement class
Height/reference class
State presentation contract
Gameplay binding contract
Stable logical identity
```

L'art finale può cambiare completamente estetica, ma non deve cambiare le regole competitive.

---

# 16. Issue strategy — NON duplicare

Prima cercare ownership reale.

Possibili owner da verificare:

```text
AST-001 Graybox Kit                 (planning key / lane item, non GitHub number)
ED-003 Graybox Authoring            (planning key / lane item)
E23 / #324 Walls, Doors, Interaction Graph
Map Editor / Geometry Grammar owner
Rubble/Debris owner
Objective owner
Perception owner
```

Non creare una nuova Epic se un owner esistente può ospitare il lavoro.

---

# 17. Candidate issue decomposition — v0.1

Creare/aggiornare issue solo dopo audit.

Usare slug/titoli; lasciare che GitHub assegni i numeri.

## P0 — audit e ownership

### `graybox-kit-v01-audit-and-owner-reconcile`

Scopo:

- trovare asset/prefab/materiali già esistenti;
- trovare owner Feature/Epic/Issue;
- mappare i 19 elementi;
- identificare duplicati;
- produrre `REUSE / UPDATE / CREATE / DEFER`.

Acceptance:

```text
19 v0.1 items accounted for
owner known
no duplicate Epic
asset/editor/scenario/test links known
```

---

## P0 — Cell Placement Volume

### `graybox-cell-placement-volume-v01`

Scope:

- `CellBound / EdgeBound / SurfaceBound / EditorOnly`;
- prisma esagonale visuale;
- Safe Placement Volume;
- height guides;
- toggle editor/debug;
- no runtime authority;
- document placement contract.

Acceptance:

```text
CellBound example fits Safe Volume
EdgeBound example intentionally not forced inside
volume hidden in normal gameplay
pivot/anchor behavior documented
```

---

## P0 — primitive/master mesh baseline

### `graybox-master-primitives-v01`

Scope:

- audit master meshes;
- reuse engine/project primitives where possible;
- create only missing reusable primitives;
- naming/pivot consistency.

Acceptance:

```text
no redundant mesh zoo
all v0.1 prefab can be composed from approved primitive set
```

---

## P0 — Unit scale + facing readability

### `graybox-unit-placeholder-scale-v01`

Scope:

- cylinder unit;
- reduced scale if current unit is too large;
- bottom-center pivot;
- facing indicator;
- test at tactical zoom.

Acceptance:

```text
unit does not visually dominate cell
facing readable
cover/path/surface remain visible
```

---

## P0 — Wall + Cover family

### `graybox-wall-cover-family-v01`

Scope:

- Wall;
- Low Cover;
- High Cover;
- EdgeBound anchor;
- common visual family;
- 30° or canonical geometry snap;
- no mesh authority.

Acceptance:

```text
Low/High distinguishable without color
wall distinguishable from high cover according to current design
snap is deterministic/consistent
```

---

## P0 — Cover state grammar

### `graybox-cover-damage-state-grammar-v01`

Scope:

```text
Intact
Damaged
Critical
Destroyed
```

- color + non-color marker;
- Destroyed -> rubble/geometry change.

Acceptance:

```text
states distinguishable in grayscale / no color-only dependency
destroyed cannot be confused with critical
```

---

## P0 — Door state presentation

### `graybox-door-open-closed-destroyed-v01`

Preferire update di E23/door issue se già owner.

Scope:

```text
Closed
Open
Disabled if already in scope
Destroyed if already in scope
```

Visual transform must follow logical state.

No second door authority.

---

## P1 — Rubble + broken wall presentation

### `graybox-rubble-broken-structure-v01`

Preferire reuse del Rubble/Debris owner.

Scope:

- baseline rubble primitive/prefab;
- broken wall visual;
- no Chaos authority;
- no advanced debris gameplay dragged into v0.1.

---

## P1 — floor/ramp/platform authoring pieces

### `graybox-architecture-support-pieces-v01`

Scope:

```text
Floor
Ramp
Platform
```

Ramp/Platform can visually prepare verticality without implementing multilayer gameplay prematurely.

---

## P1 — surface state renderers

### `graybox-water-ice-readability-v01`

Scope:

```text
Water
Ice
```

SurfaceBound renderers/materials.

Acceptance:

```text
Water != Ice at normal tactical zoom
no new terrain authority
```

---

## P1 — functional props

### `graybox-functional-props-v01`

May be split only if repo ownership requires.

Scope:

```text
Valve       -> Fluid
Generator   -> Power
HazardTank  -> Hazard
Relay       -> Objective
```

Use neutral body + functional accent.

---

## P1 — spawn/debug markers

### `graybox-spawn-debug-markers-v01`

EditorOnly.

Do not pollute runtime presentation.

---

## P0/P1 — visual validation scene

### `graybox-v01-readability-validation`

Scope:

- 5x5-ish validation composition;
- tactical camera zoom passes;
- screenshot/video acceptance;
- update Editor Map / manual verification tracking;
- scenario link if Scenario Harness owns it.

Acceptance:

```text
all mandatory categories readable without detailed HUD
no CellBound asset violates Safe Placement contract
open/closed and intact/destroyed readable without color only
```

---

# 18. Validator / automation candidate

Verificare prima se esiste già un asset/geometry validator.

Se manca e può essere implementato senza overengineering, creare o pianificare:

### `graybox-placement-contract-validator`

Possibili check:

```text
CellBound footprint inside configured Safe Volume
pivot convention
scale non-zero / sane
placement class present
duplicate Stable/Asset ID
invalid material/state mapping
unsupported snap/orientation
```

Non leggere una mesh renderizzata per decidere gameplay.

Se l'automazione automatica su `.uasset` è prematura, creare un Editor/manual validation step tracciato e rimandare il validator.

---

# 19. Tracking da aggiornare

Dopo audit, aggiornare SOLO le source of truth reali.

Verificare almeno:

```text
Decision Log / ADR
Feature Registry
Roadmap
Lane / Issue Map
Asset tracker
Editor Map / Editor sessions
Scenario Map / Scenario Registry
QA / Test Map
Project Graph / Execution Map / Control Center
Wiki / technical owner docs
Open Decisions
```

Le viste generate:

```text
NON si editano a mano
```

Rigenerarle tramite tooling.

---

# 20. Decision Log — decisioni da registrare se non già presenti

Registrare come decisioni consolidate, se il repository non le contiene già:

1. Graybox map objects are 3D primitives/prefab, not primarily 2D symbols.
2. Shape conveys type/silhouette; color/accent conveys state/family.
3. Mechanical state changes must be geometrically visible where applicable.
4. Cover baseline = Low/High 3D directional family.
5. Destroyed cover becomes rubble / changed geometry, not just recolor.
6. Add Cell Placement Volume in v0.1.
7. Placement taxonomy: CellBound / EdgeBound / SurfaceBound / EditorOnly; MultiCell deferred.
8. CellBound uses Safe Placement Volume inset baseline ~90%, subject to repository metric validation.
9. Pivot contract: bottom-center for CellBound; segment-center/base for EdgeBound.
10. Graybox mesh/collision is never competitive authority.
11. Graybox kit must become art-replacement contract by v1.0.
12. Unit cylinders must remain small enough for tactical readability.

Se un punto è già canonico, aggiornarne il riferimento invece di creare decisione duplicata.

---

# 21. Open Decisions da NON chiudere a intuito

Se non già decisi live, lasciare esplicitamente aperti:

```text
exact centimeters / C metric
exact Safe Placement inset percentage
exact cover height thresholds used by gameplay
exact Low/High naming if canonical vocabulary differs
exact Temporary/Energy Cover inclusion in v0.1
exact wall-vs-cover visual distinction
exact automatic validator implementation
exact MultiCell release
exact material palette values / color hex
```

La sessione ha deciso la grammatica, non necessariamente tutti i numeri finali.

---

# 22. Roadmap reconciliation requirement

La repository recente possiede una release ladder e un'Asset lane già più matura di alcuni handoff storici.

Claude deve produrre una tabella:

| Graybox maturity cluster | Session mapping | Current canonical release | Owner | Action |
|---|---|---|---|---|
| Core map | v0.1 | ? | ? | REUSE/UPDATE |
| Environment | v0.2 | ? | ? | ... |
| 3D Map | v0.3 | ? | ? | ... |
| Interactive Map | v0.4 | ? | ? | ... |
| Tactical Devices | v0.5 | ? | ? | ... |
| Destruction | v0.6 | ? | ? | ... |
| Perception | v0.7 | ? | ? | ... |
| Objectives | v0.8 | ? | ? | ... |
| Modularization | v0.9 | ? | ? | ... |
| Contract Freeze | v1.0 | ? | ? | ... |

Non cambiare il tema delle release globali solo per far coincidere questa tabella.

Preservare l'ordine di maturità, ma adattarlo alla roadmap canonica.

---

# 23. Acceptance criteria globali del Graybox Kit v0.1

Il lavoro è Done quando:

- [ ] tutti i 19 elementi sono classificati REUSE / UPDATE / CREATE / DEFER;
- [ ] `CellPlacementVolume` ha owner e implementazione/task tracciata;
- [ ] ogni `CellBound` rispetta il contract scelto;
- [ ] ogni `EdgeBound` usa anchor/snap coerenti;
- [ ] pivot contract documentato;
- [ ] Low vs High Cover distinguibili senza colore;
- [ ] cover Intact/Damaged/Critical/Destroyed leggibili;
- [ ] Open vs Closed leggibile geometricamente;
- [ ] Destroyed leggibile geometricamente;
- [ ] unit placeholder non domina la cella;
- [ ] Water vs Ice leggibili a zoom normale;
- [ ] Fluid/Power/Hazard/Objective identificabili;
- [ ] mesh/collisione non diventano authority gameplay;
- [ ] test/Editor validation map aggiornata;
- [ ] asset/editor/scenario/test tracking collegati;
- [ ] nessuna Epic/issue duplicata;
- [ ] docs owner aggiornate;
- [ ] viste derivate rigenerate;
- [ ] validator/check pertinenti verdi;
- [ ] packaged/editor verification pianificata o eseguita secondo DoD del repository.

---

# 24. Definition of Done per ogni asset/prefab graybox

Ogni elemento deve avere, dove applicabile:

```text
Name / Stable asset identity
PlacementClass
Footprint
Pivot
Snap/Orientation
Height/reference class
Visual family
State presentation
Logical owner/binding
Editor usage
Scenario/Test
Acceptance screenshot/video
```

Inoltre:

- leggibile a camera tattica;
- non dipende solo dal colore;
- non possiede regole competitive tramite mesh;
- non introduce riferimenti hard-coded a hero;
- riusa material/master mesh;
- è sostituibile da art finale.

---

# 25. Output finale obbligatorio di Claude

Restituire:

```text
REFACTORTACTICS GRAYBOX KIT CONSOLIDATION REPORT
```

con:

## Audit

```text
Repository
Branch
Worktree
Base HEAD
Final HEAD
UE version
Source-of-truth files inspected
Existing graybox assets found
Existing owner Epic/issues found
```

## Reconciliation

```text
Decisions already canonical
Decisions added
Decisions superseded
Conflicts found
Open decisions
```

## Issue work

```text
Epics reused
Issues updated
Issues created
Issues closed/superseded
Issue dependencies
Release/milestone assignment
Lane assignment
```

## Asset roadmap

```text
v0.1 actual tracked assets
post-v0.1 mapping to canonical releases
deferred asset clusters
v1.0 contract-freeze mapping
```

## Tracking

```text
Feature Registry changes
Asset tracker changes
Editor Map/session changes
Scenario links
QA/Test links
Project Graph / Control Center regenerated
```

## Documentation

```text
Decision Log / ADR
technical specs
Wiki
roadmap docs
generated views
```

## Tests / validation

```text
Automation
Editor/manual
PIE
Packaged
screenshots/video evidence
validator output
```

## Git

```text
Commits
PR
Remaining blockers
Next executable issue
```

---

# 26. Commit strategy proposta

Non imporre se la repository usa convenzioni differenti.

Possibile sequenza:

```text
docs(graybox): consolidate placement and visual contract

chore(tracking): map graybox kit work to canonical roadmap

feat(graybox): add cell placement volume and base placement classes

feat(graybox): add unit wall and cover readability baseline

feat(graybox): add door rubble and structural state presentation

feat(graybox): add environment and functional prop placeholders

test(graybox): add v0.1 readability and placement validation
```

Tenere i commit focalizzati.

---

# 27. Principio finale

Il Graybox Kit non è art provvisoria casuale.

Deve essere:

```text
TACTICAL DEBUG LANGUAGE
        +
AUTHORING CONTRACT
        +
READABILITY TEST BED
        +
FUTURE ART REPLACEMENT CONTRACT
```

fino alla v1.0.

Il player/developer deve poter guardare una scena e capire, con asset minimi:

```text
dove posso stare
cosa mi copre
cosa blocca
cosa è aperto
cosa è distrutto
cosa è acqua/ghiaccio
cosa appartiene a Fluid/Power/Hazard/Objective
cosa è cambiato durante la Resolution
```

senza trasformare la mesh in authority competitiva e senza richiedere una proliferazione di asset specializzati.
