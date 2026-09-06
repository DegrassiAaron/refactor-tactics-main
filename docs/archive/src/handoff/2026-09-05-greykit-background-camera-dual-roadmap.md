# Refactor Tactics — GreyKit Background Geometry & Camera Boundary
## Claude Code Handoff — Dual Roadmap: Architecture/Implementation + Unreal Editor Validation

> Scope: giant unreachable geometric structures used around the GreyKit map as background masses and as visual occluders, with camera movement constrained so the player cannot expose invalid/out-of-map space.
>
> Goal: implement the smallest robust version that preserves a strict separation between visual background geometry, gameplay/playable bounds, and camera bounds, then validate it as thoroughly as possible in Unreal Editor using MCP where available and user-assisted checks where MCP cannot verify behavior.

---

# 0. OPERATING RULES FOR CLAUDE

1. **Audit before editing.**
   - Inspect the existing Refactor Tactics repository.
   - Identify current camera controller / pawn / spectator logic.
   - Identify current grid / hex / map bounds ownership.
   - Identify pathfinding and navigation responsibilities.
   - Identify collision channels and profiles already in use.
   - Identify any GreyKit/greybox map actors, data assets, tags, map-building utilities, debug tooling, automation tests, and Unreal MCP integration.
   - Do not invent parallel systems if the repository already has a suitable abstraction.

2. **Do not merge three different concepts into one system:**
   - Background visual geometry.
   - Playable/gameplay bounds.
   - Camera bounds.

3. **Prefer the minimum system that can be tested.**
   - No production art pipeline.
   - No complex procedural world generation.
   - No new navigation cost system just to block inaccessible scenery.
   - No replicated background actors unless a real gameplay requirement exists.

4. **Preserve current behavior outside this feature.**
   - Existing maps must continue to work.
   - Existing camera movement must not regress.
   - Existing pathfinding must not begin considering non-playable scenery.
   - Existing callers/configuration should retain reasonable defaults.

5. **Every architectural decision must be justified against an actual requirement.**

6. **At the end of each implementation phase, run automated checks.**
   The final phase must include an extensive Unreal Editor validation pass.

7. **Use Unreal MCP for editor work when available.**
   If a required check cannot be performed through MCP, create a precise user-assisted checklist instead of pretending it was verified.

8. **Track uncertainty explicitly using:**
   - FACT
   - INFERENCE
   - PROPOSAL
   - QUESTION

---

# 1. SERIOUS DESIGN / TECHNICAL CRITIQUE

## 1.1 What is good about the idea

The concept has real value for the GreyKit stage because giant unreachable structures can:

- prevent the camera from exposing empty or unfinished world space;
- visually communicate that the tactical board is part of a larger world;
- reduce the need to build hexes or gameplay geometry in areas that will never be reachable;
- provide large-scale landmarks that help orientation;
- serve as low-cost placeholders for later forests, ruins, cliffs, towers, industrial structures, or Paragon-derived scenery;
- let the team test composition and camera framing before committing to final art.

This is a strong prototype technique.

---

## 1.2 Critical architecture warning

The biggest failure mode is treating a giant background mesh as simultaneously:

1. visual scenery;
2. pathfinding blocker;
3. playable-map boundary;
4. camera collision object.

That coupling is cheap initially but expensive later.

If an artist moves or scales a background block, camera behavior or gameplay rules could change accidentally.

### Required separation

The implementation should aim for three independently editable concepts:

### A. Background Geometry
Purely visual/occlusion-oriented scenery.

### B. Playable Bounds
Defines where tactical gameplay can exist:
- valid hexes;
- units;
- objectives;
- pathfinding domain;
- other authoritative gameplay placement.

### C. Camera Bounds
Defines where the local tactical camera may move / pivot / reveal.

They may visually overlap, but they must not be the same source of truth by default.

---

## 1.3 Gameplay/readability risk — false affordance

A visually interesting structure can look reachable even when it is not.

This is dangerous in a tactical game because players interpret geometry as possible tactical space.

The GreyKit should test a visual rule similar to:

> No readable tactical surface / no valid hex presentation = not playable.

Do not rely only on invisible collision to teach this rule.

The player must understand the distinction from presentation.

---

## 1.4 Camera collision alone is not sufficient

A physical blocking volume may stop the camera root while still allowing:

- camera boom / spring arm penetration;
- camera rotation to expose void space;
- near-plane clipping;
- zoom transitions through geometry;
- camera target/pivot crossing invalid space;
- high-angle views over the blocker;
- edge cases at corners between multiple volumes.

Therefore the system must be validated against the actual camera model used by Refactor Tactics, not merely against generic pawn collision.

---

## 1.5 Pathfinding risk

Do **not** represent unreachable background as large areas of high-cost or blocked gameplay cells unless the grid architecture specifically requires it.

Preferred behavior:

- no gameplay hexes are generated there;
- pathfinding domain ends before the scenery;
- background geometry does not participate in semantic path scoring;
- camera-only blockers do not leak into tactical navigation.

This reduces complexity and prevents false interactions.

---

## 1.6 Networking risk

Background visuals and local camera blockers are probably local/non-authoritative concerns.

However, playable bounds are gameplay state.

Therefore:

- do not replicate purely visual background actors unless necessary;
- do not make server gameplay depend on client-only camera blockers;
- if playable bounds are authoritative, ensure server/client derive or receive the same bounds;
- test multiplayer PIE to ensure background/camera setup cannot create divergent gameplay state.

---

## 1.7 Production risk

Do not overbuild a full “background biome system” now.

The first implementation only needs enough primitives and controls to answer:

- Do giant forms frame the map well?
- Can the camera remain inside a visually safe volume?
- Can designers compose the space quickly?
- Does gameplay remain isolated from background scenery?
- Can future art replace the GreyKit forms without rewriting camera/gameplay code?

If these cannot be proven, more features are premature.

---

# 2. TARGET MODEL

The desired conceptual model is:

```text
Visual Background Layer
    └─ Giant unreachable meshes / GreyKit forms
         └─ visual, occlusion, optional shadow

Camera Constraint Layer
    └─ camera bounds / blockers / constraint volumes
         └─ local camera behavior only

Gameplay Layer
    └─ playable bounds / hex generation / pathfinding domain
         └─ authoritative tactical space
```

Do not collapse these layers unless repository analysis proves the current architecture already provides a safe unified abstraction.

---

# 3. ROADMAP A — CODE / ARCHITECTURE / IMPLEMENTATION

## A0 — Repository audit
**Priority: CRITICAL**

Before changing code, produce an audit note containing:

- camera classes and ownership;
- how camera movement is constrained today;
- map/hex bounds representation;
- grid generation entry points;
- semantic pathfinding entry points;
- navigation or collision integration;
- collision channels/profiles;
- map setup actors / level scripts / world subsystems;
- networking authority relevant to map bounds;
- existing tests;
- existing debug visualization;
- Unreal MCP capabilities available in this repo.

### Exit criteria
Claude can explain where each of the following currently lives:
- camera constraints;
- playable space;
- hex generation;
- pathfinding domain;
- level/environment setup.

If any is unclear, document the ambiguity before implementation.

---

## A1 — Define responsibilities
**Priority: CRITICAL**

Create or reuse the smallest abstractions needed to represent:

### Background visual
Must not become a gameplay authority.

Potential representation:
- actors;
- static mesh actors;
- GreyKit background blueprint;
- tagged level actors.

Use the repository's existing conventions.

### Camera constraint
Must be editable separately from visuals.

Potential representation:
- box/volume actors;
- bounds component;
- camera constraint data;
- camera blocker actors.

Again: prefer existing architecture.

### Playable bounds
Do not create a new system if one already exists.

The implementation must identify the actual source of truth for:
- valid tactical cells;
- movement;
- placement.

### Deliverable
Add a short architecture note to the repository explaining ownership and dependency direction.

---

## A2 — Collision model
**Priority: HIGH**

Audit current collision profiles first.

Create the minimum collision behavior required for the camera.

Requirements:

- background visuals should not accidentally block gameplay traces;
- camera-specific blocking should use a dedicated or existing suitable channel/profile;
- unit movement should not rely on camera blockers;
- pathfinding traces should not see camera blockers unless explicitly required;
- projectile / targeting / LOS traces must not be altered accidentally by scenery whose purpose is only background framing.

### Explicit tests
Verify collision responses for at least:
- camera;
- unit/pawn;
- visibility/LOS traces;
- tactical targeting traces if present;
- pathfinding/navigation traces if present.

---

## A3 — GreyKit background primitives
**Priority: MEDIUM**

Implement only a small useful set.

Suggested minimum family, only if it fits project conventions:

- block / monolith;
- wall;
- tower / pillar;
- wedge / cliff;
- optional arch or grouped composition.

Prefer parameterized scale/transform rather than many unique assets.

Requirements:

- clearly non-playable;
- fast to place;
- cheap;
- replaceable by future final assets;
- no gameplay logic embedded in them.

Avoid building a large art framework.

---

## A4 — Camera constraint implementation
**Priority: CRITICAL**

Implement or extend camera constraints based on the real existing camera architecture.

Must explicitly handle, where relevant:

- pan;
- orbit;
- rotation;
- zoom;
- spring arm;
- camera target/pivot;
- interpolation;
- camera bounds corners;
- multiple overlapping blockers;
- high-angle view;
- low-angle view;
- transitions between camera states;
- focus-on-unit / focus-on-position;
- cinematic or resolution-phase camera states if they exist.

### Important
The test is not merely “camera actor cannot enter the volume.”

The test is:

> Can the player expose invalid world space or penetrate visual masses through any supported camera interaction?

---

## A5 — Gameplay isolation
**Priority: CRITICAL**

Prove that background/camera systems do not modify tactical simulation.

Checks:

- no extra hexes generated for background regions;
- no path nodes generated purely to represent unreachable scenery;
- unit valid-move queries unchanged;
- objectives unchanged;
- LOS rules unchanged unless explicitly intended;
- targeting unchanged;
- resolution simulation unchanged;
- deterministic gameplay state unaffected by camera position.

If a dependency is found, document and remove it unless required.

---

## A6 — Debug visualization
**Priority: HIGH**

Add or reuse debug visualization for:

- playable bounds;
- camera bounds;
- background actor category;
- optionally camera pivot/target;
- camera collision contact or clamped position.

Goal:
a designer must be able to see immediately which system owns which boundary.

Do not depend on visual guesswork.

---

## A7 — Automated tests
**Priority: HIGH**

Add the smallest deterministic tests possible.

Potential tests depending on repository infrastructure:

### Bounds separation
Changing a background mesh transform does not change playable bounds.

### Camera data separation
Changing camera bounds does not alter valid movement / pathfinding results.

### Navigation isolation
Background-only actors do not appear in pathfinding domain.

### Collision profile test
Camera blocker collision profile does not block gameplay channels.

### Existing-map compatibility
Maps without the new feature continue to initialize correctly.

### Multiplayer/determinism
If playable bounds are replicated/authoritative:
- server/client agree on playable bounds;
- camera state does not enter authoritative state hashes or simulation outcomes.

### Regression tests
Existing camera and grid tests remain green.

---

## A8 — Performance sanity
**Priority: MEDIUM**

For GreyKit, avoid premature optimization, but check:

- number of draw calls / actors;
- shadows from giant objects;
- collision complexity;
- unnecessary tick;
- unnecessary replication;
- unnecessary navigation generation;
- large-volume overlap cost.

Prefer:
- no Tick where not needed;
- simple collision;
- no replication for local visuals/camera blockers;
- no navigation impact.

---

## A9 — Implementation gate
**Priority: CRITICAL**

Before editor validation begins:

- project compiles;
- automated tests pass;
- no new warnings attributable to this feature;
- architecture note exists;
- debug visualization works;
- test map or existing GreyKit map contains a representative setup.

Only then start Roadmap B.

---

# 4. ROADMAP B — UNREAL EDITOR / MCP / USER VALIDATION

This roadmap is deliberately extensive.

Purpose:
perform as many checks as possible in the Unreal Editor after implementation.

Use Unreal MCP wherever reliable.

When MCP cannot observe or manipulate the required state, provide the exact user action and expected result.

---

## B0 — Editor startup validation
**Priority: CRITICAL**

Via MCP if possible:

- open the project;
- verify no module/plugin load failure;
- inspect Output Log;
- inspect Message Log;
- open the target GreyKit map;
- verify no broken references;
- verify no missing classes/assets caused by the change.

Record all errors/warnings.

---

## B1 — Structure inspection
**Priority: HIGH**

Inspect the map hierarchy / World Outliner.

Verify:

- background visual actors are identifiable;
- camera constraint actors/components are identifiable;
- playable bounds remain separately identifiable;
- no accidental duplicate managers;
- no unintended level-script coupling;
- transforms and ownership are understandable.

If actor naming/tagging conventions exist, verify compliance.

---

## B2 — Visual separation check
**Priority: HIGH**

In editor viewport:

- enable playable-bound debug;
- enable camera-bound debug;
- identify background geometry;
- visually verify they are distinct concepts.

Required evidence:
a screenshot or documented observation demonstrating all three layers.

If MCP can capture editor screenshots, use it.
Otherwise ask the user for screenshots.

---

## B3 — Hex/grid generation validation
**Priority: CRITICAL**

Verify:

- no hexes appear behind/in unreachable background purely because the scenery exists;
- boundary hexes remain valid;
- no malformed partial cells;
- no unexpected gaps caused by camera volumes;
- moving a background mesh does not regenerate or alter the playable grid.

Where possible:
- regenerate/rebuild grid;
- reload level;
- compare behavior.

---

## B4 — Pathfinding validation
**Priority: CRITICAL**

Run representative movement/path queries near map edges.

Test:

1. path parallel to background boundary;
2. path toward edge;
3. path near a corner;
4. path between two edge-adjacent cells;
5. path after moving a background visual actor;
6. path after moving a camera blocker without changing playable bounds.

Expected:
only playable-space data affects tactical movement.

If a debug path renderer exists, capture evidence.

---

## B5 — Camera pan tests
**Priority: CRITICAL**

Test all supported pan directions at:

- center;
- each map edge;
- each corner;
- near concave boundary if present;
- near giant background geometry.

Expected:

- no visual void is exposed;
- no camera penetration;
- no jitter loop;
- no stuck camera;
- no excessive “dead zone” where the camera stops too early.

---

## B6 — Camera rotation/orbit tests
**Priority: CRITICAL**

At several edge positions:

- rotate through full supported yaw;
- test pitch limits;
- test orbit if supported;
- test camera pivot behavior;
- test fast repeated direction changes.

Check:

- spring arm does not tunnel;
- camera does not clip through giant forms;
- invalid world space remains hidden;
- framing remains readable.

---

## B7 — Zoom tests
**Priority: CRITICAL**

Test:

- minimum zoom;
- maximum zoom;
- rapid zoom;
- zoom while panning;
- zoom while rotating;
- zoom near every boundary type.

Check:

- no clipping;
- no sudden camera jumps;
- no interpolation overshoot beyond bounds;
- no invalid scenery exposure.

---

## B8 — Focus / camera command tests
**Priority: HIGH**

If the game has commands such as:
- focus selected unit;
- focus event;
- center on target;
- resolution camera;
- spectate unit;
- jump to location;

run them near boundaries.

Expected:
camera constraint is respected after automatic repositioning.

This is a common hidden failure mode.

---

## B9 — Resolution-phase test
**Priority: HIGH**

Because Refactor Tactics uses simultaneous-turn resolution, test camera behavior during actual resolution.

Scenario:
- place units near multiple edges;
- plan movement/actions;
- execute resolution;
- observe camera transitions while units move near the new background geometry.

Check:

- camera constraints remain active;
- no background geometry interferes with simulation;
- no collision affects unit motion;
- camera interpolation cannot escape bounds.

---

## B10 — LOS / targeting regression
**Priority: CRITICAL**

If background geometry is not supposed to participate in tactical LOS:

- fire/target near map edge;
- inspect LOS traces;
- inspect targeting previews;
- verify background meshes and camera blockers do not create false obstruction.

If background geometry *is* intentionally meant to occlude gameplay LOS, stop and document this as a design decision because it changes the scope substantially.

---

## B11 — Collision validation
**Priority: CRITICAL**

Use Unreal collision visualization / collision analyzer if available.

Verify for each relevant actor class:

- camera channel response;
- pawn/unit channel response;
- visibility;
- weapon/targeting;
- navigation;
- overlap events;
- physics.

Expected:
only explicitly intended channels interact.

---

## B12 — Navigation visualization
**Priority: HIGH**

If UE NavMesh or another nav representation is used anywhere:

- display navigation;
- verify camera-only or background-only geometry does not expand/block nav unexpectedly;
- verify no large unnecessary navigation rebuild region.

If tactical pathfinding is entirely custom and does not use UE NavMesh, document that and skip NavMesh-specific checks.

---

## B13 — Debug bounds stress test
**Priority: HIGH**

Temporarily transform background geometry independently from camera bounds:

### Test A
Move visual block inward/outward.

Expected:
playable bounds unchanged.

### Test B
Move camera blocker.

Expected:
camera behavior changes;
playable grid/pathfinding does not.

### Test C
Move playable boundary through the supported editor workflow.

Expected:
gameplay domain changes;
background visual and camera bounds do not silently rewrite themselves unless an explicit authoring link exists.

Undo temporary modifications afterwards.

---

## B14 — Save / reload / PIE persistence
**Priority: HIGH**

Verify:

- save level;
- close/reopen;
- PIE;
- stop PIE;
- reopen level;
- standalone if practical.

Check that:
- transforms persist;
- references persist;
- no runtime-spawn duplicate actors;
- no stale bounds caches.

---

## B15 — Multi-client PIE
**Priority: HIGH**

Run at least 2 clients if the current project supports multiplayer PIE.

Verify:

- each client has correct background scenery;
- each local camera respects its own constraints;
- camera state is not incorrectly replicated;
- gameplay bounds are consistent;
- server simulation is independent of camera state;
- no replication warnings.

If dedicated server mode exists and is routinely supported, include one dedicated-server pass.

---

## B16 — Disconnect / reconnect sanity
**Priority: MEDIUM**

Only if current project supports reconnect flows.

Verify camera initialization and map bounds after reconnect.

Do not build reconnect infrastructure for this feature.

---

## B17 — Editor transform ergonomics
**Priority: MEDIUM**

As a level designer:

- move;
- rotate;
- scale;
- duplicate;
- delete;
- undo/redo;

the giant background pieces and camera blockers.

Check:

- no unexpected construction-script side effects;
- no massive editor hitch;
- no hidden dependency on actor order;
- bounds remain understandable.

---

## B18 — Extreme transforms / robustness
**Priority: MEDIUM**

Try intentionally bad values:

- very large scale;
- tiny scale;
- negative scale if technically allowed;
- overlapping blockers;
- blocker outside map;
- missing visual mesh;
- missing blocker;
- background actor deleted.

Expected:
safe failure or clearly debuggable behavior.

Do not add complex validation unless actual failures justify it.

---

## B19 — Performance editor sanity
**Priority: MEDIUM**

Inspect:

- stat unit;
- stat game;
- stat gpu if relevant;
- collision overhead;
- actor count;
- shadow cost;
- nav rebuild behavior;
- Tick usage.

This is not a full optimization pass.

Goal:
detect obvious accidental cost.

---

## B20 — Packaging / cook sanity
**Priority: HIGH**

If feasible in current workflow:

- compile development build;
- cook/package representative target;
- ensure no missing references;
- ensure editor-only debug code is guarded appropriately;
- ensure background assets are included where required.

Do not block the feature on unrelated packaging failures; clearly isolate them.

---

## B21 — Final editor regression matrix
**Priority: CRITICAL**

Run a consolidated pass covering:

| Area | Check | Expected |
|---|---|---|
| Camera | Pan at all edges | No escape / no void |
| Camera | Rotate at edge | No clipping |
| Camera | Zoom at edge | No overshoot |
| Camera | Focus command | Bounds respected |
| Camera | Resolution phase | Bounds respected |
| Grid | Background moved | Grid unchanged |
| Grid | Camera blocker moved | Grid unchanged |
| Pathfinding | Edge paths | Valid and predictable |
| LOS | Edge targeting | No false blockers |
| Collision | Unit vs blocker | No gameplay collision |
| Collision | Camera vs blocker | Intended collision/constraint |
| Multiplayer | 2 clients | Local camera, shared gameplay |
| Persistence | Save/reload | Stable |
| Packaging | Build/cook | No feature-specific failure |
| Performance | Idle + camera movement | No obvious regression |

---

# 5. MCP VS USER-ASSISTED EXECUTION RULE

For every Roadmap B item, mark one of:

- `MCP VERIFIED`
- `AUTOMATION VERIFIED`
- `USER VERIFIED`
- `NOT VERIFIED`

Never mark something verified based only on code inspection if the item requires runtime/editor behavior.

If user action is needed, provide exact steps:

```text
USER CHECK
1. Open <map>.
2. Enable <debug command>.
3. Move camera to <edge>.
4. Rotate clockwise through full supported range.
5. Expected: ...
6. Failure evidence to capture: screenshot/video/log.
```

Keep user steps short and deterministic.

---

# 6. ISSUE / TASK ORGANIZATION

If this work is too large for one change, create a parent roadmap/issue and smaller implementation issues.

Suggested grouping:

### Parent
`GreyKit Background Geometry & Camera Boundary Separation`

### Child 1
`Audit camera, playable bounds, grid and collision ownership`

### Child 2
`Implement independent camera boundary representation`

### Child 3
`Add GreyKit giant background primitives`

### Child 4
`Add debug visualization and collision isolation`

### Child 5
`Add automated regression tests`

### Child 6
`Run Unreal Editor / MCP validation matrix`

Do not split further unless the repository structure genuinely requires it.

---

# 7. DEFINITION OF DONE

The feature is complete only when all of the following are true:

## Architecture
- background visuals are not the authoritative playable boundary;
- camera bounds are independently editable;
- pathfinding does not depend on unreachable background scenery;
- ownership is documented.

## Gameplay
- no new reachable tactical space exists in background regions;
- no false movement/targeting/LOS behavior was introduced;
- gameplay simulation does not depend on camera state.

## Camera
- supported pan/zoom/rotation/focus flows cannot expose invalid map space under tested configurations;
- edge/corner behavior is stable and readable.

## Networking
- local camera behavior is not incorrectly authoritative/replicated;
- shared playable-space state remains consistent.

## Editor
- designers can place and edit the system predictably;
- debug visualization clearly distinguishes visual, camera, and gameplay bounds;
- editor validation matrix is completed.

## Testing
- automated tests pass;
- final Unreal Editor pass is documented;
- every unverified item is explicitly listed.

---

# 8. FINAL REPORT FORMAT FOR CLAUDE

At completion, return:

## IMPLEMENTED
Exact files/classes/assets changed.

## ARCHITECTURE
How background, camera bounds, and playable bounds are separated.

## AUTOMATED TESTS
Tests added and results.

## EDITOR CHECKS
Table:

| Check | Method | Result | Evidence |
|---|---|---|---|
| ... | MCP / Automation / User | PASS / FAIL / NOT VERIFIED | ... |

## REGRESSIONS FOUND
Any unrelated or related failures.

## OPEN QUESTIONS
Only unresolved information genuinely needed.

## FOLLOW-UP
Only items that are justified by evidence from the prototype.

---

# 9. NON-GOALS

Do not include unless separately requested:

- final Paragon environment art;
- complete biome/background generation;
- procedural scenery;
- production LOD pipeline;
- large modding framework;
- camera cinematics redesign;
- new LOS system;
- new pathfinding algorithm;
- new multiplayer architecture;
- automatic coupling of art meshes to tactical bounds.

---

# 10. SUCCESS CRITERION

The implementation succeeds if a designer can surround a GreyKit tactical map with huge unreachable geometric masses, constrain the camera so invalid world space is not exposed, and freely modify those visual masses without silently changing tactical movement, pathfinding, or authoritative gameplay space.
