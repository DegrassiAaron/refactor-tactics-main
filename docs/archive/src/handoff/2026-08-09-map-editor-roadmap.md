# RefactorTactics — Map Editor Roadmap & Consolidation Brief for Claude

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> ## 🗄️ `HISTORICAL` — SORGENTE REVISIONATO, NON APPLICATO
>
> **Archiviato il 2026-08-09**, dopo la revisione in
> [`../../../roadmap/plans/map-editor-brief-spec-panel-2026-08-09.md`](../../../roadmap/plans/map-editor-brief-spec-panel-2026-08-09.md).
> Era in `todo/RefactorTactics_MapEditor_Roadmap_Consolidation_Claude.md`, untracked.
>
> **Non è una specifica.** Il testo qui sotto è la fotografia di un modello proposto il 2026-08-09 da chi non
> aveva il repository sotto gli occhi: contiene **9 duplicati** di cose che hanno già un owner e un formato
> serializzato, e **5 conflitti** con decisioni accettate. In particolare:
>
> | Qui il brief dice | Ma il canone dice |
> |---|---|
> | §1.3, §4 — i muri sono geometria world-space e derivano occupazione, transizione e LOS | **E23.1** (v0.2): «la logica di transizione **non legge la mesh**: legge archi e stati» |
> | §5 — porte `Open` / `Closed` | `ERTHexDoorState{Open, Closed, Locked, Destroyed}` + `DoorId` — [`spec-porte-cp93.md`](../../../gameplay/spec-porte-cp93.md) |
> | §3 — terreni `Concrete / Metal / Dirt / Grass / Water / Ice / Rubble` | Le otto superfici di CP 8.1 — [`spec-terreni-e8.md`](../../../gameplay/spec-terreni-e8.md) |
> | §11 — `MovementProfileId ∈ {Standard, Heavy, Agile}` | `Move / Sprint / Charge`, profilo dell'**azione** ([D-015](../../../decisions/RT_PDR_00_Decision_Log.md)) |
> | §16, §26, §28 — `RTMapDefinition`, 5 epic, 29 feature entry, ScenarioId `MAP-ED-*` | `URTHexMapAsset` a `FormatVersion=4`; epic a numerazione unica; `feature-registry.yaml` come unico registro |
>
> **Il testo originale non è stato riscritto** — è la convenzione di questa cartella. Quello che il brief
> coglie davvero, e che il repository non ha, è **una sola cosa**: la sonda di movimento nell'editor
> (§9, §12, §13). Vive nella §6 e §9 della revisione.

## Purpose

Use this document as an **implementation/consolidation brief** for the RefactorTactics repository.

The goal is to consolidate the Map Editor work discussed so far into the project’s canonical artifacts:

- product/technical documentation;
- Wiki;
- Roadmap;
- Feature Map / Feature Registry;
- Scenario Map;
- backlog;
- Epic and Issue hierarchy;
- validation/test plan.

Do **not** create a parallel roadmap or duplicate feature definitions. Search the repository first, identify the existing canonical locations, then **merge, update, rename, or supersede** existing material where appropriate.

The Map Editor must remain a tool over the **same runtime core used by the game**. Do not implement editor-only versions of pathfinding, LOS, targeting, environment rules, or movement costs.

---

# 1. Design principles to preserve

## 1.1 Same systems in Editor and Game

Hard requirement:

```text
Map Editor
      |
      v
same MapState
same Graph
same Pathfinding
same LOS
same Targeting
same Environment Resolver
      |
      v
Game
```

If the editor says a unit can reach a cell and the runtime game says it cannot, the architecture is wrong.

Editor tools are visual/debug clients of the authoritative simulation services, not alternative implementations.

---

## 1.2 Compact tactical map

The map remains a compact tactical graph.

- `FRTCellId = X, Y, Layer`.
- X/Y represent the logical hex plane.
- Layer represents overlapping levels.
- Cells are compact data, not one Actor per cell.
- Graph edges/transitions are first-class data.
- Actors are used only when useful for rendering, visible collision, authoring handles, or interactive representation.
- Pathfinding, LOS, targeting, trajectory, cover, and environment remain separate services.
- Graph changes increment revisions and invalidate dependent caches.

---

## 1.3 Hex grid versus architecture

Important Map Editor decision:

**Walls are not constrained to the edges of the hex grid.**

The hex grid describes tactical positions and graph nodes.

Walls represent real architectural geometry and may:

- cross an hex partially;
- be aligned independently from the six hex directions;
- form 90-degree architectural corners;
- cut a transition between two otherwise valid cells;
- block LOS;
- make a tactical anchor/cell unusable if the unit clearance/footprint conflicts with the wall.

Therefore do not model every wall as simply "one closed side of an hex".

---

## 1.4 Doors/openings

Gameplay openings are discrete.

For v0.1:

```text
OPEN
CLOSED
```

No percentage-based logical opening.

A door may physically be 0.8 m, 0.9 m, 1.0 m, 1.5 m, etc., but the gameplay transition is determined by explicit rules/profile clearance and state, not by an arbitrary "62% open" value.

Doors should normally be authored as an opening/interactive associated with wall geometry and graph transitions.

---

## 1.5 Data-driven content

C++ defines:

- what properties exist;
- legal invariants;
- simulation rules;
- graph effects;
- validation.

Data Assets / definitions configure:

- terrain variants;
- cover variants;
- interactive variants;
- movement profiles;
- editor palette entries;
- presentation.

Use stable IDs, versions, governed Gameplay Tags, validation, and indirect references.

Do not introduce public modding in this milestone.

---

# 2. Map Editor v0.1 — Navigation Sandbox

## Goal

The first useful editor must answer:

> **"Can this map be traversed the way the level designer intends?"**

It must be possible to create a small tactical arena from scratch and immediately test how terrain, walls, doors, cover, and interactives affect navigation.

## Core toolbar

Initial modes:

```text
Select
Terrain
Wall
Cover
Interactive
Unit Probe
Debug
```

---

# 3. Terrain Tool — v0.1

Terrain is authored primarily per tactical cell.

Suggested basic operations:

- Paint
- Erase/reset
- Fill
- Pick
- Brush size

Initial definitions may include:

```text
Terrain.Default
Terrain.Concrete
Terrain.Metal
Terrain.Dirt
Terrain.Grass
Terrain.Water
Terrain.Ice
Terrain.Rubble
```

Exact roster is data-driven and should match the current project vocabulary.

Do not force designers to manually enter every numeric property on every cell. A terrain definition supplies defaults such as:

- base physical movement cost;
- surface tags;
- traversal restrictions;
- sound/material modifiers later;
- environment interaction capabilities.

## Critical data-model distinction

Do **not** combine permanent/base terrain with transient environment state.

Good:

```text
Base Surface = Metal

Runtime state:
Wet
Electrified
Burning
Frozen
```

Bad:

```text
MetalWet
MetalWetElectrified
MetalBurning
MetalFrozen
...
```

Avoid combinatorial terrain enums/assets.

---

# 4. Wall Tool — v0.1

Walls are authored as world-space architectural geometry.

Expected workflow:

```text
click start
-> move cursor
-> preview segment
-> click end
```

Support at least:

- segment creation;
- polyline/chained wall creation;
- endpoint snapping;
- useful angular snapping;
- architectural 90-degree corners;
- delete/edit/move segment;
- minimal Undo/Redo.

Possible snapping modes:

```text
Free
Angle increments
Architectural 90 degrees
Existing wall endpoint
```

Do not make hex-edge snapping the only or authoritative wall representation.

## Derived logical effects

A wall can influence three separate systems.

### A. Tactical occupancy / clearance

A cell can become non-standable if the unit's valid footprint/clearance at its tactical anchor intersects the wall.

### B. Graph transition

A transition between two cells can be blocked because the traversal segment/portal crosses the wall.

Both cells may remain individually standable.

### C. LOS

The wall can block or modify visibility independently from movement.

These three effects must not be collapsed into one Boolean.

---

# 5. Doors — v0.1

Doors are interactive map elements attached to architectural openings/transitions.

Workflow target:

```text
create wall
-> choose Door tool/interactive
-> place opening on wall
-> editor binds logical transition(s)
```

Initial state:

```text
Open
Closed
```

Effects can include:

```text
Open   -> transition enabled / LOS profile updated
Closed -> transition disabled / LOS profile updated
```

Any graph-affecting state change increments the appropriate revision and invalidates cached queries.

---

# 6. Cover Tool — v0.1

Cover is separate from Wall.

Do not infer traversal from "has cover".

Examples:

```text
Low Cover
  movement: may be passable according to definition
  cover: yes
  LOS: partial/profile-driven

Large Crate
  movement: blocked
  cover: yes
  LOS: profile-driven

Wall
  movement transition: blocked
  cover: not necessarily represented as tactical cover
  LOS: blocked

Barrier
  movement: definition-driven
  cover: yes
  LOS: definition-driven
```

Initial authoring should support:

- low cover;
- high cover;
- fragile/destructible cover metadata;
- orientation/directional coverage;
- whole cover element placement.

Do not subdivide one hex side into arbitrary thirds or percentage-open portions for v0.1.

---

# 7. Interactive Tool — v0.1

Create a data-driven palette.

Initial categories/examples:

```text
Architecture
- Door

Environment
- Water Valve
- Generator
- Flammable Tank

Objective
- Relay
```

These align with the existing demo/map concepts and can be expanded later.

Conceptual placement record:

```cpp
FRTMapInteractivePlacement
{
    StableId
    DefinitionId
    CellId
    WorldTransform
    InitialState
}
```

Where relevant, an interactive may reference:

```text
AffectedCells
AffectedEdges
AffectedWalls
AffectedNetworks
```

Avoid one giant hard-coded enum containing every future interactive type.

Prefer stable definition IDs, for example:

```text
Interactive.Door.Standard
Interactive.Valve.Water
Interactive.Generator.Electric
Interactive.Tank.Flammable
Interactive.Objective.Relay
```

C++ may expose controlled behavior families/interfaces.

---

# 8. Graph-affecting interactives

A key v0.1 foundation is that interactives can cause logical map changes.

Examples:

```text
Door
-> Edge enabled/disabled

Water Valve
-> Water source/flow state

Generator
-> Electrical network/state

Flammable Tank
-> Destruction/event/environment state
```

Expected architecture:

```text
Interactive action/state change
        |
        v
Map State Change
        |
        v
Graph / Environment Revision
        |
        v
Cache invalidation
        |
        v
Path / LOS / other dependent query recompute
```

Interactives must not directly contain unrelated A* or UI logic.

---

# 9. Unit / Player Movement Probe — v0.1

This is a **required v0.1 editor feature**, not optional polish.

## Purpose

Allow the level designer to place a player/unit probe on a cell and immediately inspect:

- where it can move;
- all reachable cells;
- movement cost to each cell;
- best path to a hovered/selected destination;
- why a cell/path is blocked.

This lets the editor validate terrain, walls, doors, graph transitions, and movement profiles without launching a complete match.

## Basic UI

Example:

```text
UNIT PROBE

Unit / Movement Profile: [ Standard ]

Movement Points: [ 6 ]

Start:
(X,Y,Layer)

[x] Reachable Area
[x] Movement Cost
[x] Best Path
[x] Blocked Reasons
[x] Graph Edges
```

Later, when character definitions exist, allow selecting actual units:

```text
Gadget
Phase
Riktor
Wraith
```

Do not hard-code these names in the pathfinding service.

---

# 10. Movement range algorithm

Do not run A* separately toward every cell.

Use two concepts:

## Point-to-point path

For:

> "What is the best path from A to B?"

Use the project's authoritative A* query.

## Reachable-area query

For:

> "Where can I go with MovementBudget = N?"

Use a bounded-cost Uniform Cost / Dijkstra-style flood over the same tactical graph and movement-cost rules.

Conceptual output:

```cpp
FRTMovementRangeResult
{
    ReachableCells
    CostByCell
    ParentByCell
}
```

`ParentByCell` allows efficient path reconstruction when the designer hovers/selects a reachable destination.

The query must use the same:

- map state;
- graph;
- edge validity;
- physical movement cost;
- movement profile;
- revisions

used by actual gameplay.

---

# 11. Movement profiles

Do not design the probe around only:

```text
Start + MovementPoints
```

Introduce a minimal movement profile concept early enough to avoid rework.

Example conceptual query:

```cpp
struct FRTMovementRangeQuery
{
    FRTCellId Start;
    FName MovementProfileId;
    int32 MovementBudget;
};
```

Potential initial profiles:

```text
Standard
Heavy
Agile
```

Actual values may be deferred/data-driven.

Future rules can therefore express cases such as:

```text
DeepWater:
Standard -> high cost
Aquatic unit -> low cost
Heavy -> invalid
```

without changing map data.

---

# 12. Movement visualization

Reachable cells should not be represented only as "green".

Useful overlays:

- reachable;
- total minimum movement cost;
- selected best path;
- remaining movement points;
- blocked edge;
- blocked cell;
- block reason.

Hover example:

```text
Cell: (5,-1,0)
Reachable: Yes
Total Cost: 4

Cost breakdown:
Concrete +1
Water    +2
Concrete +1
Total     4
```

Blocked example:

```text
UNREACHABLE

Reason:
Wall blocks transition

Edge:
(4,-2,0) -> (5,-2,0)

Source:
Wall_013
```

Other reason codes should include:

```text
MovementBudgetExceeded
CellBlocked
TransitionBlocked
DoorClosed
ProfileRestriction
InvalidCell
NoPath
```

Use stable reason codes where possible, not only localized strings.

---

# 13. Live recomputation in the editor

The Unit Probe should update when map authoring changes relevant state.

Examples:

```text
paint higher-cost terrain
-> reachable area changes

place wall
-> paths recompute

close door
-> graph revision changes
-> cache invalidates
-> reachable area changes

reopen door
-> paths return
```

This is a central editor acceptance path.

---

# 14. Debug overlay — v0.1

Required toggles should include:

```text
Cell IDs
Cell anchors
Standable / blocked cells
Terrain type
Physical movement cost
Graph edges
Blocked graph edges
Graph revision
Reachable cells
Movement total cost
Selected path
Block reason
Cover direction
LOS blocker markers
Interactive IDs
```

Do not require every debug visualization to be production-quality UMG. Efficient editor/debug drawing is acceptable, but it must be useful and explainable.

---

# 15. Map validation — v0.1

Validation should run explicitly and preferably integrate with save/build validation where practical.

Minimum checks:

```text
ERROR: invalid/missing CellId reference
ERROR: graph edge references nonexistent cell
ERROR: invalid layer
ERROR: cover orientation/direction invalid
ERROR: door not associated with a valid opening/transition
ERROR: duplicate stable ID
ERROR: missing interactive definition
ERROR: missing terrain definition

WARNING: standable cell with no exits
WARNING: unreachable interactive/objective
WARNING: near-duplicate/overlapping wall endpoints
WARNING: wall/clearance unexpectedly invalidates a tactical anchor
```

Ensure messages include enough context to locate the problem in the editor.

---

# 16. Map data / saving

The `.umap` must not be the only source of competitive truth.

The project should converge on a versioned map definition, e.g.:

```text
RTMapDefinition
|
+-- GridDefinition
+-- Cells[]
+-- Walls[]
+-- Covers[]
+-- Interactives[]
+-- Spawn/Marker data
+-- Metadata
+-- Version
```

Exact serialization format can follow current repository architecture.

The competitive map definition should support:

- stable ID;
- version;
- validation;
- deterministic materialization into runtime `MapState`;
- content manifest/hash integration.

---

# 17. Map Editor v0.1 — proposed scope lock

Include:

```text
Hex/grid generation
Terrain painting
Terrain physical movement costs
Wall segment/polyline authoring
Walls independent from hex edges
Tactical cell clearance validation
Automatic graph transition blocking
LOS blocker data
Doors/openings
Low/high/directional cover
Destructible cover metadata
Interactive palette
Door
Water Valve
Generator
Flammable Tank
Relay / objective marker
Unit/Player placement
Movement Profile
Movement Budget
Reachable-area query
A* hover/selected path
Movement cost overlay
Blocked reason overlay
Graph debug overlay
Graph revision debug
Basic map validator
Minimal Undo/Redo
```

Explicitly defer:

```text
Full multiplayer simulation
Enemy AI
Full combat
Complete environment resolver
Public modding
Procedural map generation
Production prefab system
Advanced map analytics
Complex multilayer/tunnels/elevators
```

---

# 18. Map Editor v0.2 — Tactical Geometry

## Goal

Answer:

> **"Does the map geometry create the intended tactical decisions?"**

Add:

### Facing Probe

Support six logical hex facings:

```text
N / NE / SE / S / SW / NW
```

Use the actual canonical facing model adopted by the game.

### LOS Probe

Place/select source and target and expose:

```text
Visible
Blocked
Partial/obscured if supported
Blocker ID
Reason code
```

### Visibility Area

From a unit position/facing, visualize visible cells.

Allow overlays such as:

```text
Movement
Visibility
Movement + Visibility
```

### Cover Probe

Select attacker and defender.

Display:

```text
incoming direction
cover level
cover source
cover-facing relationship
```

### Overwatch Geometry Preview

Do **not** duplicate the full reaction system.

Use only the real geometric services needed to display:

```text
Origin
Facing
Arc/cone
Range
LOS-valid threatened cells
```

This is a geometry/debug tool for directional reactions.

### First multilayer support

Introduce a deliberately limited first step:

```text
Layer 0: Ground
Layer 1: Upper
```

Initial special transitions:

```text
Stairs
Ramp
Drop/Jump point if rules are already defined
```

Editor layer filters:

```text
All
Current
Ground
Upper
```

Defer complex elevators, tunnels, timed platforms, and dynamic bridges unless separately approved.

---

# 19. Map Editor v0.3 — Environment Sandbox

## Goal

Answer:

> **"How does the map change when its environment is activated?"**

Add an environment simulation/debug mode using the real environment rules.

## Environmental overlays

Examples:

```text
Terrain
Water
Fire
Electricity
Ice
Steam
Visibility
Noise
Movement
```

## Interactive state preview

Allow controlled editor/debug state changes such as:

```text
Door Open/Closed
Valve Open/Closed
Generator On/Off
```

Then visualize the resulting legal map state.

## Apply Effect tool

Example:

```text
Apply Effect:
Electricity

Target:
Cell(...)
```

Then run the actual environment resolver/service and show affected cells/events.

Use this to validate system interactions such as:

```text
Water + Electricity
Fire + Water -> Steam
Water -> Freeze
Fire -> Melt/evaporate according to current rules
```

Do not invent new environment rules here; consume the canonical ones.

---

# 20. Noise Probe — target v0.3

Add a noise source probe over the tactical graph.

Designer chooses:

```text
origin
noise type
intensity
```

Editor visualizes received acoustic intensity / detection-relevant values across cells.

The tool must use graph-based acoustic propagation rather than a simple world-space sphere overlap.

Useful diagnostics:

```text
source intensity
distance loss
wall attenuation
door attenuation
surface modifiers
ambient masking later
result at cell
```

This becomes the primary sandbox for validating acoustic map design.

---

# 21. Map Editor v0.4 — Combat Sandbox

## Goal

Answer:

> **"Do actual characters and abilities behave correctly and interestingly on this map?"**

Add real character definitions to probes.

Initial expected roster should follow the canonical current project roster, not stale PDR placeholders.

Provide:

### Character Probe

- place unit;
- select character/profile;
- movement overlay;
- facing;
- vision;
- cover.

### Ability Probe

Select an actual ability definition and display through canonical services:

```text
valid/invalid targets
range
LOS requirement
trajectory
AoE
affected cells
potential environment interactions
reason for invalid target
```

### Dash / special movement Probe

Use the actual movement/ability policy.

Do not assume Dash equals Move with more points.

### Team Probe

Place multiple units by team and filter overlays by:

```text
unit
team
selected unit
enemy threat
```

### Choke-point analysis — initial

Introduce a simple structural analysis, not an AI solver.

Given map markers such as:

```text
Team A Spawn
Team B Spawn
Objective
```

sample legal shortest/low-cost routes and highlight highly reused cells/edges.

Use this for detecting:

- accidental single chokepoints;
- dominant corridors;
- unreachable flanks;
- obvious spawn-objective imbalance.

Clearly label analytics as designer assistance, not authoritative balance proof.

---

# 22. Map Editor v0.5 — Production Authoring

## Goal

Answer:

> **"Can designers produce and maintain real maps quickly, consistently, and safely?"**

Focus on productivity rather than adding gameplay.

Potential features:

### Prefabs / authoring assemblies

Examples:

```text
Room
Corridor
Door frame
Cover cluster
Generator room
Bridge segment
```

Ensure prefab use still resolves into canonical competitive map data.

### Transform/edit productivity

- duplicate;
- mirror;
- rotate;
- multi-select;
- copy/paste;
- batch property edit;
- palette favorites;
- search/filter.

For hex-aware content, useful transforms may include multiples of 60 degrees where logically valid.

Architectural geometry may also require normal 90-degree workflows.

Do not assume the same transform constraints for every object type.

### Advanced map statistics

Potential report:

```text
total cells
walkable cells
blocked cells
surface distribution
average/median movement cost
walls
doors
cover distribution
interactives
spawn-to-objective minimum costs
route diversity
symmetry deltas
```

### Advanced validation

Split report into:

```text
Technical
Tactical
Performance
```

Examples:

```text
TECHNICAL
Door references invalid edge

TACTICAL
Team A minimum objective cost = 14
Team B minimum objective cost = 17

Large reachable area has no cover

Objective has only one approach

PERFORMANCE
Excessive render/editor components
Excessive unique assets/materials
```

Tactical validation should provide warnings, not pretend to prove that a map is balanced.

---

# 23. Version roadmap summary

Consolidate the roadmap around this progression unless repository state requires slightly different numbering.

```text
MAP EDITOR v0.1
NAVIGATION SANDBOX
|
+-- Grid
+-- Terrain
+-- Walls
+-- Cover
+-- Interactives
+-- Doors
+-- Unit Movement Probe
+-- Reachable Area
+-- Path
+-- Costs
+-- Block Reasons
+-- Graph Debug
      |
      v
MAP EDITOR v0.2
TACTICAL GEOMETRY
|
+-- Facing
+-- LOS
+-- Visibility
+-- Cover Probe
+-- Overwatch Geometry
+-- First Multi-Layer
      |
      v
MAP EDITOR v0.3
ENVIRONMENT SANDBOX
|
+-- Water
+-- Fire
+-- Electricity
+-- Ice
+-- Steam
+-- Noise
+-- Propagation
+-- Interactive State Preview
      |
      v
MAP EDITOR v0.4
COMBAT SANDBOX
|
+-- Real Characters
+-- Abilities
+-- Targeting
+-- Dash
+-- AoE
+-- Team Probe
+-- Choke Analysis
      |
      v
MAP EDITOR v0.5
PRODUCTION AUTHORING
|
+-- Prefabs
+-- Duplicate/Mirror/Rotate
+-- Batch Editing
+-- Map Statistics
+-- Advanced Validation
+-- Performance Diagnostics
```

---

# 24. Required scenario-map updates

Add/update scenario definitions that validate editor capabilities.

The Scenario Map must reference the relevant Feature IDs/Epics where the project convention supports this.

At minimum create/consolidate scenarios equivalent to the following.

## MAP-ED-001 — Terrain movement-cost test

Setup:

```text
Unit Probe
MovementBudget = configurable
multiple surface costs
```

Assert:

- reachable-area boundary matches accumulated physical cost;
- hover path total equals displayed cost;
- repeated query produces identical result.

## MAP-ED-002 — Wall transition block

Setup:

```text
two standable adjacent cells
wall intersects their transition
```

Assert:

- both cells remain valid when clearance allows;
- direct transition is blocked;
- alternative path is used if one exists;
- reason identifies the wall/transition.

## MAP-ED-003 — Cell clearance versus wall

Setup wall geometry partially crossing an hex.

Assert:

- tactical anchor remains standable if clearance permits;
- becomes blocked only when actual footprint/clearance rule fails;
- no simplistic "wall touches hex => block entire cell" rule.

## MAP-ED-004 — Door revision

Setup:

```text
door open
unit movement probe active
```

Sequence:

```text
open -> closed -> open
```

Assert:

- edge state changes;
- GraphRevision changes;
- movement-range cache invalidates;
- reachable area recomputes correctly.

## MAP-ED-005 — Cover does not imply movement block

Place cover variants.

Assert movement/LOS/cover behavior independently according to definition.

## MAP-ED-006 — Interactive references

Place Valve, Generator, Tank, Relay.

Assert:

- stable IDs valid;
- definitions resolve;
- references target valid cells/edges;
- validator catches invalid references.

## MAP-ED-007 — Deterministic reachable area

Run the same range query repeatedly and with adjacency insertion order permuted.

Assert:

- same reachable set;
- same minimum costs;
- same parent/path result under explicit tie-break rules.

## MAP-ED-008 — LOS wall test [v0.2]

Validate wall geometry against the canonical LOS service.

## MAP-ED-009 — Directional cover probe [v0.2]

Move attacker around defender.

Assert cover changes by attack direction correctly.

## MAP-ED-010 — Multilayer traversal [v0.2]

Validate Ground -> Stairs/Ramp -> Upper path and invalid paths without transition.

## MAP-ED-011 — Environmental propagation [v0.3]

Validate the canonical water/fire/electric/ice interaction rules once implemented.

## MAP-ED-012 — Noise propagation [v0.3]

Validate attenuation through terrain/walls/doors over the tactical graph.

## MAP-ED-013 — Ability targeting sandbox [v0.4]

Ensure Editor Ability Probe and runtime targeting service return identical validity/affected cells.

## MAP-ED-014 — Choke analysis sanity [v0.4]

Known fixture with multiple routes must produce stable route-usage diagnostics.

## MAP-ED-015 — Map authoring validation report [v0.5]

Known broken fixture must produce expected Technical/Tactical/Performance findings.

---

# 25. Required automated tests

Create/consolidate tests around the core rather than testing only UI.

## Core automation

At minimum:

```text
CellId equality/hash
world <-> grid mapping
graph neighbor validity
wall-derived transition blocking
movement cost lookup
movement profile restrictions
bounded reachable-area query
A* point-to-point path
stable tie-break
GraphRevision invalidation
door state transition
map validation
```

## Golden tests

Maintain deterministic fixtures for:

```text
MapDefinition
+ MovementProfile
+ Start
+ Budget
=
Reachable set + costs + selected canonical paths
```

## Functional/editor tests

Where Unreal automation support makes sense:

```text
authoring operation
-> rebuild/validate map
-> run query
-> assert overlay/view-model data
```

Do not make screenshot comparison the primary correctness mechanism.

---

# 26. Suggested Epic structure

Before creating anything, search the repository for existing Map/Editor/Level Design epics and merge where appropriate.

If the backlog does not already have an equivalent hierarchy, propose/create something like:

## EPIC — Map Editor v0.1: Navigation Sandbox

Candidate issues:

1. Map Editor shell/tool modes
2. Terrain definition + terrain paint tool
3. Wall data model + segment/polyline authoring
4. Wall-to-cell-clearance derivation
5. Wall-to-graph-transition derivation
6. Door/opening authoring + initial state
7. Cover definition + directional placement
8. Interactive definition/placement framework
9. Initial Door/Valve/Generator/Tank/Relay definitions
10. Movement Profile data model
11. Bounded reachable-area query
12. Unit Movement Probe
13. Movement cost overlay
14. A* hover path preview
15. Block reason diagnostics
16. Graph/revision debug overlay
17. MapDefinition save/materialization pipeline
18. Map validator v0.1
19. Automation/golden tests
20. Functional scenario fixtures
21. Packaged/editor verification and documentation

## EPIC — Map Editor v0.2: Tactical Geometry

Candidate issues:

1. Facing Probe
2. LOS Probe
3. Visibility Area overlay
4. Cover attacker/defender probe
5. Overwatch geometry preview
6. Layer filter UI
7. Ground/Upper support
8. Stair/Ramp transition authoring
9. Multilayer path tests
10. LOS/cover golden scenarios

## EPIC — Map Editor v0.3: Environment Sandbox

Candidate issues:

1. Environment overlay framework
2. Interactive state preview
3. Apply Environment Effect probe
4. Water debug propagation
5. Fire debug propagation
6. Electricity debug propagation
7. Ice/Steam visualization
8. Noise source probe
9. Acoustic propagation overlay
10. Environment scenario/golden tests

## EPIC — Map Editor v0.4: Combat Sandbox

Candidate issues:

1. Character Probe
2. Ability Probe
3. Target-validity overlay
4. AoE/trajectory visualization
5. Dash/special-movement probe
6. Team Probe
7. Spawn/objective markers
8. Choke structural analysis
9. Editor/runtime parity tests

## EPIC — Map Editor v0.5: Production Authoring

Candidate issues:

1. Prefab/assembly workflow
2. Multi-select/batch edit
3. Duplicate/mirror/rotate
4. Authoring search/favorites
5. Map statistics report
6. Spawn/objective path parity metrics
7. Advanced Technical validation
8. Tactical validation warnings
9. Performance diagnostics
10. Production-map authoring documentation

---

# 27. Epic/Issue requirements

For every Epic and Issue created or modified:

- use the repository's existing ID/naming convention;
- link Feature IDs;
- link Scenario IDs;
- link milestone/version;
- add dependencies;
- add acceptance criteria;
- add test requirements;
- include relevant documentation/wiki links;
- avoid duplicate tasks already present elsewhere;
- mark speculative later-version work appropriately;
- do not make later-version items block v0.1 unless technically required.

Each implementation issue should state:

```text
Why
Scope
Out of scope
Technical approach
Data/API impact
Editor setup
Acceptance criteria
Tests
Debug/telemetry
Dependencies
Documentation updates
Definition of Done
```

For core map logic, include deterministic and packaged validation expectations where applicable.

---

# 28. Feature Map / Feature Registry updates

Create or consolidate feature entries for at least:

```text
Map Editor Shell
Terrain Authoring
Wall Authoring
Door Authoring
Cover Authoring
Interactive Authoring
Map Definition
Map Validation
Map Debug Overlay
Movement Profiles
Movement Range Query
Movement Probe
Path Preview
Blocked Reason Diagnostics
Facing Probe
LOS Probe
Visibility Probe
Cover Probe
Overwatch Geometry Preview
Multilayer Authoring
Environment Sandbox
Noise Probe
Character Probe
Ability Probe
Team Probe
Choke Analysis
Production Authoring
Map Statistics
Advanced Map Validation
```

Each feature entry should include the project's existing fields, plus where supported:

```text
Status
Target Version
Epic
Dependencies
Scenario coverage
Wiki page
Owner/module
DoD
```

Do not create a second Feature Registry if one already exists.

---

# 29. Wiki updates

Find existing map/editor/pathfinding pages and consolidate instead of duplicating.

Suggested Wiki structure, adjusted to repository convention:

```text
Map System
|- Tactical Graph Model
|- Hex Coordinates / FRTCellId
|- Cells and Transitions
|- Terrain
|- Walls and Openings
|- Cover
|- Interactive Map Elements
|- Movement Profiles
|- Pathfinding
|- Reachable Area
|- Graph Revisions

Map Editor
|- Overview
|- v0.1 Navigation Sandbox
|- Terrain Tool
|- Wall Tool
|- Cover Tool
|- Interactive Tool
|- Unit Movement Probe
|- Debug Overlays
|- Map Validation
|- Scenario Testing
|- Roadmap v0.2-v0.5
```

Important Wiki callouts:

- walls are not hex edges;
- walls, movement blockers, cover, and LOS blockers are separate concepts;
- terrain is not transient environment state;
- editor uses the same core services as runtime;
- reachable-area query differs from point-to-point A*;
- doors are graph-affecting interactives;
- GraphRevision/cache invalidation is visible/testable;
- v0.2+ roadmap is planned but not v0.1 scope.

---

# 30. Roadmap consolidation

Update the main roadmap rather than keeping an isolated Map Editor roadmap.

The previously planned "Fondazioni" work already includes:

```text
FRTCellId
grid
graph
A*
visible path
debug
```

Extend/consolidate it so that v0.1 Map Editor authoring and Movement Probe become explicit deliverables.

Important nuance:

We are intentionally bringing **authoring/model-data foundations** for walls, cover, doors, and interactives earlier.

We are **not** pulling every advanced simulation behavior into the same milestone.

For example:

```text
v0.1:
author wall
derive blocked edge
door open/closed
show movement consequences

later:
full multiplayer/environment/reaction implications
```

Reflect this distinction in the roadmap to prevent scope creep.

---

# 31. Documentation/PDR consolidation

Search the current docs for overlapping or stale statements.

At minimum reconcile Map Editor decisions with documents covering:

- UE5 architecture;
- map/pathfinding;
- deterministic simulation;
- UI/debug;
- content definitions/validation;
- roadmap/QA;
- environment;
- Overwatch/reactions;
- noise/perception.

Do not silently overwrite a contradictory project decision.

If conflicts exist:

1. list the conflicting statements;
2. apply the project's precedence rules;
3. update the canonical source;
4. mark old text superseded where useful;
5. record the decision/change in changelog/ADR/decision log if the repository uses one.

---

# 32. Scenario Map consolidation

The Scenario Map must become a way to launch/test features, not just a prose list.

For Map Editor scenarios, capture where possible:

```text
ScenarioId
Title
Target Version
Feature IDs
Epic/Issue refs
Map/fixture
Initial state
Actions
Expected result
Automation status
Manual validation steps
Debug overlays to enable
```

Link Wiki pages back to relevant scenarios so a developer/designer reading a feature page can launch its demonstration/test scenario.

---

# 33. Definition of Done for Map Editor features

A Map Editor feature is not Done merely because the widget/tool appears.

Minimum DoD:

1. Uses canonical runtime/core map services.
2. Produces valid canonical map data.
3. Does not duplicate competitive rules in editor-only code.
4. Has debug/explainability output.
5. Has validation/reason codes for invalid states where relevant.
6. Has Automation/Functional Test coverage appropriate to the feature.
7. Has at least one Scenario Map fixture.
8. Is documented in Wiki/Feature Map.
9. Is linked to the correct Roadmap milestone.
10. Is verified in the project's expected Editor/packaged workflow where applicable.
11. Does not break deterministic map/query results.
12. Has focused Git history/commit(s).

---

# 34. Suggested Git commit sequence

Adapt to actual repository state; do not create commits for nonexistent work.

Possible sequence:

```text
docs(map-editor): consolidate map editor roadmap and architecture

feat(map): add terrain definitions and authoring data

feat(map-editor): add wall and opening authoring

feat(map): derive cell and edge blockers from wall geometry

feat(map-editor): add directional cover and interactive placement

feat(path): add bounded movement-range query

feat(map-editor): add unit movement probe and path diagnostics

feat(map): add map validation and graph revision diagnostics

test(map): add navigation sandbox golden scenarios

docs(map): link wiki feature registry roadmap and scenario map
```

Keep commits focused.

---

# 35. Required final output from Claude

After making repository changes, return a consolidation report containing:

## A. Files changed

List every modified/created/deleted file and why.

## B. Decisions consolidated

List the final canonical decisions, especially:

```text
walls independent from hex edges
terrain vs environment state
cover vs blocking separation
door graph semantics
movement-range algorithm
same-runtime-core editor principle
Map Editor v0.1-v0.5 roadmap
```

## C. Roadmap changes

Show what existing milestones changed and why.

## D. Feature Map changes

List new/modified Feature IDs.

## E. Scenario Map changes

List new/modified Scenario IDs.

## F. Epics and Issues

List:

```text
Epic ID
Issue ID
Title
Target version
Dependencies
Status
```

Explicitly state which existing tasks were reused/consolidated rather than duplicated.

## G. Documentation/Wiki changes

List pages/docs updated and links/references added.

## H. Test changes

List automated/manual tests and scenario coverage.

## I. Conflicts / unresolved decisions

Do not guess.

Raise any real unresolved product/design/technical choice.

## J. Recommended next implementation slice

Default recommendation:

> Implement Map Editor v0.1 Navigation Sandbox through a deterministic Unit Movement Probe over terrain + walls + door state + graph revision before moving into LOS/facing v0.2.

---

# 36. Scope guard

Do not expand this consolidation into:

- public modding;
- matchmaking;
- progression;
- complete GAS integration;
- production dedicated-server work;
- full combat AI;
- procedural generation;
- final art workflow;
- complex 3D/multilevel simulation beyond the planned version.

The intent is to establish a strong Map Editor evolution path while keeping **v0.1 small, testable, and directly useful**.

---

# 37. Canonical roadmap statement to add

Use wording equivalent to:

> **Map Editor evolves as a validation surface over RefactorTactics' canonical tactical simulation: v0.1 Navigation Sandbox, v0.2 Tactical Geometry, v0.3 Environment Sandbox, v0.4 Combat Sandbox, v0.5 Production Authoring. Each version adds editor capabilities by exposing existing or planned runtime services, never by creating a second set of gameplay rules.**

This statement should be linked from the Roadmap, Feature Map, and Map Editor Wiki overview.

