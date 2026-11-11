# RefactorTactics — Facing System Consolidation Pack for Claude

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> 📦 **Archiviato il 2026-08-10 — recepito, non applicato.** L'esito del triage vive in
> [`roadmap/plans/facing-consolidation-triage-2026-08-10.md`](../../../roadmap/plans/facing-consolidation-triage-2026-08-10.md),
> e le domande che ne restano aperte in [`OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md) come `FAC-11`…`FAC-14`.
> **Questo file non è autorità.** Delle sue 69 sezioni, **38 erano già canoniche** (ADR-0005, ADR-0008,
> D-020, CP 16.1/16.2), 6 riaffermano domande già aperte, 18 sono procedura e **4** sono nuove.
>
> ⚠️ **Tre sezioni rovesciano una decisione presa, e vanno lette sapendolo**: il §4 («nessuna banda globale»)
> contraddice ADR-0005 §4 e ADR-0008 §5, per cui *«l'arco frontale resta `HexCone` e resta **uno solo***»
> (riga **67** di [`DOC_CONFLICT_MATRIX.md`](../../../DOC_CONFLICT_MATRIX.md)); il §10 («1 step di pivot =
> 1 MP») fa del pivot un **prezzo**, mentre ADR-0008 §1 lo tratta come un **tetto** gratuito (riga **68**);
> il §11 (categorie `Heavy/Standard/Agile`) era **già stata respinta** da ADR-0008, che assegna i valori
> per **eroe** e non per archetipo. In tutti e tre i casi prevale l'ADR: un handoff è l'ultima fonte della
> gerarchia.
>
> ⚠️ Il §21 («`Brace` direzionale») ha già un test che pinna l'opposto —
> `Spec.Facing.BraceHoldsFromBehind` — quindi accettarlo significa **cambiare quello scenario**, non
> aggiungerne uno. La domanda resta aperta come `FAC-3`.
>
> Le §53-§54 chiedono un'epic e 14 issue: **`E16` esiste ed è chiusa** (`#175`, con CP 16.1 e CP 16.2), e le
> issue proposte descrivono in maggioranza codice già scritto. Nessuna è stata creata.

## Purpose

This file consolidates **all decisions, rules, open points, implementation guidance, documentation impacts, roadmap work, and GitHub issue work discussed in the current Facing-focused chat** for RefactorTactics.

Claude must use this file as a consolidation/update task, not as a greenfield redesign.

### Required outcome

Claude must:

1. consolidate the Facing system into the project documentation;
2. update the project Wiki/Wikipedia pages;
3. update the technical roadmap and feature roadmap;
4. update scenario/test maps where Facing affects them;
5. create or update the necessary GitHub epics/issues;
6. preserve the deterministic/server-authoritative architecture;
7. preserve planning privacy;
8. add automated tests and debug visibility;
9. avoid reintroducing superseded Facing models;
10. clearly separate **decisions already consolidated** from **balance values still requiring playtest**.

---

# 1. Project context

RefactorTactics is a PC-first competitive tactical game in Unreal Engine 5 with simultaneous turns.

Relevant principles:

- authoritative deterministic simulation;
- immutable turn snapshot;
- `TurnLog` as canonical explanation/playback source;
- client proposes, server validates and applies;
- planning intents are private by team;
- enemy clients must never receive opponent future planning;
- presentation/animation never determines simulation results;
- hexagonal tactical grid;
- pathfinding, LOS, targeting and trajectory are separate services;
- map geometry, cover, walls and Facing are separate concepts;
- abilities and reactions may be directional;
- Overwatch is directional and evaluated during resolution micro-steps;
- the UI distinguishes **Confirmed / Predicted / Uncertain** information.

Baseline documentation currently targets Unreal Engine 5.8. The repository must still lock the exact patch/toolchain used by the milestone.

---

# 2. Core Facing decision

## 2.1 Facing uses the six sides of the hexagon

A unit has exactly **six discrete logical Facing directions**.

Each Facing direction corresponds to **one side of the hexagonal cell**.

Do not use free-angle competitive Facing.

Do not use 8-way cardinal/intercardinal Facing.

Do not use a generic Unreal `FRotator` as authoritative Facing state.

Canonical logical form:

```cpp
uint8 Facing; // 0..5
```

The six directions are discrete and cyclic.

One Facing step corresponds geometrically to one adjacent hex side, i.e. 60 degrees.

The 60-degree value is useful for visualization, but simulation logic should use the six integer directions.

---

# 3. Relative directions around a unit

If the unit's current Facing is considered relative direction `0`, the six relative sides are:

| Relative offset | Semantic name |
|---:|---|
| 0 | Front |
| +1 | FrontRight |
| +2 | RearRight |
| 3 | Rear |
| -2 | RearLeft |
| -1 | FrontLeft |

Equivalent normalized representation:

```text
0 = Front
1 = FrontRight
2 = RearRight
3 = Rear
4 = RearLeft
5 = FrontLeft
```

Example:

```text
                  FRONT
                    0
              /           \
           5                 1
      FRONT-LEFT       FRONT-RIGHT

       REAR-LEFT       REAR-RIGHT
           4                 2
              \           /
                    3
                   REAR
```

These are the six fundamental geometric relations.

---

# 4. Important correction: no global Front/Flank/Rear bands

Earlier brainstorming considered collapsing the six directions into global macro-zones such as:

- Front Arc
- Flank
- Rear

That must **not** become the fundamental Facing model.

The authoritative geometric model is always the six individual sides.

A specific ability may group sides if its own rules need to.

Examples:

```text
Wide Shield:
{FrontLeft, Front, FrontRight}

Narrow Parry:
{Front}

Left-biased Shield:
{FrontLeft, Front}

Rear-sensitive ability:
{Rear}

Flank-like ability:
{RearLeft, RearRight}
```

"Flank" can remain a gameplay/design term, but it is derived from explicitly selected sides, not a primitive global Facing state.

This distinction must be reflected in code, documentation and Wiki terminology.

---

# 5. Facing is persistent authoritative unit state

Facing is part of the logical state of each unit.

It must be included in:

- current authoritative unit state;
- turn snapshot;
- replay-relevant state;
- deterministic state hashing as appropriate;
- `TurnLog` events when changed;
- team/public knowledge according to visibility rules;
- debug output.

The unit does **not** automatically return to some preferred orientation after an action.

Facing persists until a legal event changes it.

---

# 6. Presentation does not decide Facing

The character mesh may:

- rotate smoothly;
- turn head/torso independently;
- aim a weapon;
- interpolate visually;
- play anticipation/recovery animation.

None of these visual rotations changes authoritative Facing unless the resolver emitted the logical Facing change.

Authoritative Facing must never depend on:

- montage timing;
- animation notify timing;
- frame rate;
- client interpolation;
- Actor visual rotation;
- physics timing.

---

# 7. Facing changes during normal movement

## Consolidated rule

Each **successfully completed movement transition** may update the unit Facing to the direction of that transition.

Example:

```text
Start Facing: East

Step 1: move North-East
=> Facing becomes North-East

Step 2: move East
=> Facing becomes East
```

A reaction or event occurring between those two micro-steps observes the Facing valid at that exact logical boundary.

## Failed movement

A failed/blocked attempted transition must **not** automatically rotate the unit merely because it tried to move in that direction.

This avoids free Facing manipulation by intentionally attempting invalid movement.

Any different behavior must be explicitly defined by a movement/ability policy.

---

# 8. Planned Pivot

A Pivot changes Facing without necessarily changing cell.

The general planning concept discussed is:

```text
MoveIntent
    Path
    EndPivot
```

A stationary Pivot can therefore be represented as:

```text
Path = empty
EndPivot != 0
```

This avoids creating a separate special-case "rotate action" unless later gameplay requires one.

---

# 9. Pivot steps

Because Facing follows the six hex sides:

```text
1 Pivot step = 60°
2 Pivot steps = 120°
3 Pivot steps = 180°
```

Direction is signed:

```text
+1 / -1
+2 / -2
3
```

Normalize all results into `[0..5]`.

Example:

```text
Facing 0
Pivot +1 -> Facing 1
Pivot +2 -> Facing 2
Pivot +3 -> Facing 3
Pivot -1 -> Facing 5
```

---

# 10. Pivot budget model discussed

The chat explored and provisionally accepted the following simple scalable model:

```text
Movement Budget
    movement transitions
    +
    Pivot cost
```

Initial candidate rule:

```text
1 Pivot step = 1 Movement Point
```

Thus:

```text
Move 3 cells                         = 3 MP
Move 2 cells + Pivot 60°            = 3 MP
Move 1 cell + Pivot 120°            = 3 MP
Stationary Pivot 180°               = 3 MP
```

This is a **balance/configuration candidate**, not a sacred universal constant.

Claude should document it as the current baseline/prototype rule and keep the values data-driven.

Do not hard-code the balance into generic simulation rules if a movement profile can hold it.

Candidate profile data:

```cpp
struct FRTMovementProfile
{
    int32 MovementPoints;
    int32 PivotCostPerStep = 1;
    int32 MaxPivotSteps = 3;
};
```

---

# 11. Character-specific Pivot limits

The system should support different Pivot freedom by character/movement profile.

Candidate categories discussed:

```text
Heavy    -> MaxPivotSteps = 1
Standard -> MaxPivotSteps = 2
Agile    -> MaxPivotSteps = 3
```

Examples discussed conceptually:

- Riktor-type heavy defender: limited correction / high commitment;
- Gadget-type standard/technical unit: medium correction;
- Phase-type fluid/mobile unit: larger correction;
- Wraith-type agile/predictive unit: very high correction.

These exact values remain balance/playtest material.

The architecture should support them without special-case code.

---

# 12. Planned Pivot is not an automatic desired final Facing

Important deterministic/intent rule:

When the player chooses a final visual orientation, the canonical intent should represent the legal Pivot operation, not a magical instruction such as:

> "Whatever happens during resolution, restore me to North-East."

Example:

```text
Planning:
Facing = East
Player selects North-East
=> PlannedPivot = -1
```

If a previous event changes Facing before the Pivot boundary, the resolver applies the already planned Pivot relative to the state at that boundary according to the defined policy.

The simulator must not silently "repair" the Facing back to the originally desired direction.

---

# 13. No free corrective rotation during resolution

The player cannot arbitrarily change Facing during Resolution because new information appeared.

Example that must not happen:

```text
Riktor gets attacked from the side
-> player freely rotates toward attacker
```

Facing can change only through a legal mechanism such as:

- already planned Pivot;
- movement;
- Dash policy;
- ability;
- Fast Action;
- Fast Reaction;
- reaction-specific Pivot;
- forced rotation/control effect;
- other explicitly defined resolver event.

An incoming attack alone does not create free rotation.

---

# 14. No automatic snap-back

If an attack/reaction/ability legally rotates a unit, the new Facing persists.

Do not implement:

```text
rotate to attack
attack
snap back to previous Facing
```

unless the specific action definition explicitly says to restore its previous Facing.

This persistence is important for tactical consequences.

Example:

```text
Riktor faces North
Riktor rotates North-East to attack
Riktor now remains North-East
A second enemy may exploit the newly exposed side
```

---

# 15. Attack Facing policies

Not every attack should have the same turning freedom.

The system should support an action/ability Facing policy.

Concepts discussed:

```text
KeepFacing
FaceTarget
FaceAttackDirection
LimitedTurn(N)
FreeTurn
RestorePreviousFacing
```

`RestorePreviousFacing` should be explicit and comparatively rare.

Useful per-ability data may include:

```text
MaxAimTurnSteps
FacingPolicy
```

Examples discussed conceptually:

```text
Heavy Cannon:
MaxAimTurnSteps = 0

Rifle:
MaxAimTurnSteps = 1

Pistol:
MaxAimTurnSteps = 2

Highly agile melee:
MaxAimTurnSteps = 3
```

These are design examples, not locked balance numbers.

---

# 16. Attack rotation persists

If an attack legally requires/permits a rotation, the logical Facing becomes the resulting orientation.

Example:

```text
Facing = North
Attack target = North-East
Ability permits +1
=> FacingChanged North -> North-East
=> attack resolves
=> unit remains North-East
```

This can expose different sides of the unit to later attacks/reactions.

That emergent consequence is desired.

---

# 17. Incoming attack direction

Facing interaction is determined from the direction **from which the attack reaches the target**, relative to the target's current Facing.

The system should not simply ask where the attacker is looking.

Conceptually:

```text
AttackBearing = direction from Target toward attack origin / trajectory origin
RelativeDirection = AttackBearing - TargetFacing
```

normalized to the six directions.

For a target whose Facing is side 0:

```text
incoming side 0 -> Front
incoming side 1 -> FrontRight
incoming side 2 -> RearRight
incoming side 3 -> Rear
incoming side 4 -> RearLeft
incoming side 5 -> FrontLeft
```

---

# 18. Impact direction policy

Not every effect should derive its incoming direction in the same way.

Support an explicit policy such as:

```text
FromSource
FromTrajectory
FromImpactCenter
ExplicitDirection
NonDirectional
```

Examples:

- rifle/projectile: `FromTrajectory`;
- melee: often `FromSource`;
- explosion: `FromImpactCenter`;
- persistent fire terrain damage: `NonDirectional`;
- directional wave: likely `FromTrajectory`.

Do not produce absurd Facing effects such as "the burning floor hit me from behind."

---

# 19. No universal rear/backstab damage bonus

Do **not** introduce a global rule like:

```text
Front = normal
Side = +15%
Rear = +30%
```

for all attacks.

The design direction is:

> Facing changes access to directional defenses, reactions, abilities and specific bonuses; it is not automatically a universal damage multiplier.

A particular ability may absolutely have a Rear/side requirement or bonus.

Examples:

```text
Silent Blade:
bonus when RelativeDirection == Rear

Special attack:
valid from RearLeft or RearRight

Heavy frontal breaker:
bonus specifically against Front
```

These are ability-specific rules.

---

# 20. Directional defense

Facing becomes strategically valuable primarily because defensive systems can cover explicit sides.

Examples:

```text
Wide Brace:
{FrontLeft, Front, FrontRight}

Narrow Parry:
{Front}

Asymmetric Shield:
{FrontLeft, Front}

All-direction Dodge:
{all six sides}
```

The ability definition owns the covered set.

Do not hard-code a generic global Front Arc.

---

# 21. Brace and Facing

Brace should respect current Facing and its own directional coverage.

A defender can therefore be manipulated by:

- displacement;
- explicit forced rotation;
- a reaction that changes Facing;
- an attack that caused a legal Facing change;
- movement/repositioning.

If the unit's Facing changes before a later attack, the later attack checks the new orientation.

This enables geometric team combos without arbitrary damage buffs.

---

# 22. Overwatch and Facing

Overwatch is directional.

The unit's Facing plus the Overwatch definition determines the controlled/trigger area.

Do not make:

```text
Unit Facing
```

and

```text
Overwatch Direction
```

two completely unrelated free orientations by default.

Preferred model:

```text
Current/armed Facing
    +
Overwatch arc/shape
    =
controlled area
```

A specific Overwatch/reaction variant may permit a limited reaction Pivot, but this must be explicit data, e.g.:

```text
ReactionPivotSteps = 1
```

No automatic 180-degree snap toward a target entering from behind.

This aligns with the existing reaction architecture:

```text
Reaction Definition
+ Intent
+ Snapshot
+ Reaction Opportunity
+ optional Commit
= Reaction Resolution
```

Overwatch trigger evaluation still occurs at deterministic micro-step boundaries.

---

# 23. Brace/Overwatch lock timing

Planning flow concept:

```text
Facing / setup Pivot
    ->
Brace or Overwatch arms
```

Once armed, its directional coverage is based on its legal Facing state.

If later game rules permit Facing change while the stance remains active, that must be explicitly specified.

Do not allow silent free retargeting of the sector during Resolution.

---

# 24. Normal Move after Overwatch

If Overwatch ends before normal Move, the unit may still perform a Pivot that was already part of its later planned Move/Reposition.

This is not a correction to Overwatch.

It is a distinct already-planned later action.

---

# 25. Wait and Facing

No free universal 180-degree rotation merely because the player selected Wait.

Current baseline:

- Pivot uses the normal movement/Pivot rules;
- a stationary Pivot is possible through the same movement intent mechanism;
- Wait does not automatically grant a special free orientation reset.

Future character/ability variants may override this.

---

# 26. Forced movement and forced Facing are separate

Do not assume that every Push/Knockback automatically turns the target.

Displacement must declare its Facing policy independently.

Possible policies:

```text
PreserveFacing
FaceMovementDirection
FaceSource
FaceAwayFromSource
ExplicitRotation
```

Examples:

```text
Water Push:
Push 1
PreserveFacing
```

```text
Shield Bash:
Push 1
FaceAwayFromSource
```

```text
Spin effect:
No displacement
ExplicitRotation = +2
```

This makes Facing control a first-class crowd-control mechanic.

---

# 27. Facing control as geometric crowd control

RefactorTactics should support control effects that manipulate Facing without necessarily dealing damage or applying classic Stun/Slow.

Examples:

```text
RotateTarget(+1)
RotateTarget(-1)
FaceSource
FaceAwayFromSource
LockFacing
ReducePivot
```

This can disable or degrade:

- Brace coverage;
- shield coverage;
- Overwatch coverage;
- narrow counters;
- future attacks with limited aim-turn;
- directional abilities.

This is desirable because it turns positioning and orientation into system-level tactical resources.

---

# 28. Simultaneous impact rule

Facing must not change accidentally between attacks that belong to the same true simultaneous impact group merely because of iteration order.

Incorrect:

```text
Attack A resolves
-> rotates target
Attack B resolves against new Facing
```

when A and B are logically simultaneous.

Preferred model:

```text
Capture impact boundary state

Evaluate simultaneous attacks/relevant relations
against the same boundary state

Collect outcomes

Apply according to explicit resolver rules
```

Only an explicitly earlier event/reaction/control phase may change Facing before another attack reads it.

Never depend on:

- `TMap` order;
- `TSet` order;
- Actor iteration order;
- packet arrival order;
- frame timing.

---

# 29. Suggested impact-boundary processing

Conceptual pipeline discussed:

```text
1. Capture boundary state
2. Determine attack bearings
3. Determine Facing relations
4. Evaluate eligible reactions
5. Resolve reaction effects
6. Rebuild/re-evaluate if a reaction legally changed geometry/Facing
7. Resolve attack impact group
8. Apply damage/status
9. Apply explicit post-impact Facing-control effects
10. Append canonical TurnLog events
```

Claude must reconcile this with the current resolver phase model rather than inventing a second competing pipeline.

---

# 30. Cover and Facing are separate systems

Very important:

Environmental cover does **not** automatically depend on where the character is looking.

A wall stays in the world regardless of Facing.

Maintain separation:

```text
Map geometry
    -> LOS / trajectory / obstruction

Cover geometry
    -> whether the incoming attack is geometrically covered

Unit Facing
    -> personal directional defenses / reactions / directional abilities
```

A personal shield can be Facing-dependent because it belongs to the unit.

A wall must not disappear defensively because the unit turned away from it.

---

# 31. Facing and RefactorTactics wall geometry

The current map design does not require walls to coincide with the sides of the hex cell.

Walls/architectural geometry follow the project's separate geometry/directrix model.

Therefore:

- unit Facing still uses the six hex sides;
- wall direction is a separate geometric system;
- do not force wall orientation into unit Facing indexing;
- do not redesign wall placement merely to simplify Facing.

This must be preserved in documentation.

---

# 32. LOS, targeting and pathfinding remain separate

Facing must not collapse existing spatial services together.

Keep:

```text
Pathfinding
LOS
Targeting
Trajectory
Facing relation
```

as separate logical concerns.

Examples:

- an edge may be traversable but not visible through;
- a target may be visible but outside an ability's legal Facing turn;
- a wall may block trajectory independently of Facing;
- a directional reaction may require both detection and a valid relative side.

---

# 33. Perception and Facing

Do not automatically make the base visual perception system a Facing cone unless design explicitly decides so later.

However reactions may require conditions such as:

```text
RequiresVisibleSource
RequiresDetectedSource
RequiresSourceInSpecificSides
```

Examples:

- Counter Shot may require visible source and valid covered side;
- instinctive Dodge may not require source identification;
- stealth can prevent an Overwatch/reaction trigger if detection requirements fail.

Keep geometry, detection, visibility and reaction eligibility separate.

---

# 34. Enemy Facing and Fog of War

The enemy's **current observed Facing** may be public knowledge while visible.

But future planned Facing is private planning data.

Rules:

```text
enemy currently visible
-> current Facing may be observed

enemy leaves visibility
-> retain last known Facing if Team Knowledge supports it

hidden enemy rotates later
-> do NOT silently update opposing client
```

The UI may therefore show:

```text
Last known cell
Last known Facing
Observation age/confidence
```

without claiming that the Facing is still current.

Use the project's standard UI semantics:

- Confirmed;
- Predicted;
- Uncertain.

Do not leak future planning.

---

# 35. Team intent Facing preview

Allied planning may include/team-relay:

- planned final Facing;
- planned Pivot;
- Brace directional coverage;
- Overwatch directional coverage;
- directional ability ghost;
- relevant warnings.

These are team-only planning data.

Enemy clients must not receive them.

Do not place future Facing intents on globally replicated GameState/PlayerState data.

Use the existing team-only sanitized planning architecture.

---

# 36. Planning UI

The UI should communicate Facing using the same six-side model.

Do not present Facing as an arbitrary free-angle rotation gizmo for competitive input.

Recommended interaction:

1. select/path the unit;
2. derive arrival Facing from the final successful path transition;
3. show legal Pivot choices around the ghost;
4. highlight only reachable Facing sides;
5. player selects one of the six legal sides;
6. preview shows Pivot cost and final Facing;
7. ability/reaction overlays show the sides/area covered.

Examples of useful labels:

```text
FACE: FrontRight
Pivot: +1
Cost: 1 MP
```

or preferably in absolute map-facing terms in the actual UX if the project already has stable direction names.

The important part is that the visual marker locks to a hex side.

---

# 37. Directional coverage UI

Do not permanently draw every arc for every unit.

Show directional overlays contextually:

- selected unit;
- hover;
- selected ability;
- Brace planning;
- Overwatch planning;
- reaction preview;
- debug mode.

Potential overlays:

```text
Facing side
Ability legal turn range
Brace-covered sides
Overwatch-controlled region
Invalid side / insufficient Pivot
```

Avoid visual overload.

---

# 38. Confirmed / Predicted / Uncertain Facing preview

Use existing UI information classes.

## Confirmed

Use when Facing is determined from current public state and the unit's own deterministic plan without unresolved external dependencies.

## Predicted

Use for team-plan-derived results that may depend on allied coordination.

## Uncertain

Use when future enemy action, displacement, collision or hidden information can alter the predicted Facing.

Never simulate with hidden enemy intents on the client just to produce a more accurate warning.

---

# 39. Pathfinding scope

Do **not** immediately turn authoritative A* into a search over:

```text
(CellId, Facing)
```

Current simplest scalable baseline:

```text
A* produces geometric path
-> successful transitions derive Facing
-> planned Pivot is applied at the legal boundary
-> abilities/reactions validate Facing separately
```

Only move toward orientation-aware path state if playtests show that Facing-at-destination must become a primary pathfinding query.

Do not complicate F0/Fondazioni prematurely.

---

# 40. Suggested logical API

Create a small pure deterministic Facing utility rather than an Actor Component initially.

Suggested file:

```text
Source/RefactorTactics/Public/Core/RTFacing.h
```

Conceptual enums:

```cpp
enum class ERTFacingRelation : uint8
{
    Front = 0,
    FrontRight,
    RearRight,
    Rear,
    RearLeft,
    FrontLeft
};
```

Utility concepts:

```text
NormalizeFacing(Direction)
ApplyPivot(Facing, SignedSteps)
GetRelativeDirection(TargetFacing, IncomingDirection)
```

Use integer operations.

Do not use floating `FRotator` comparisons for competitive relation tests.

---

# 41. Unit state

The logical unit state needs Facing.

Conceptually:

```cpp
struct FRTUnitState
{
    // ...
    uint8 Facing = 0;
};
```

Use an enum or strongly validated integer if preferred by current code style.

Snapshot serialization/hashing must treat it deterministically.

---

# 42. Movement intent

Candidate concept:

```cpp
struct FRTMoveIntent
{
    TArray<FRTCellId> Path;
    int8 EndPivotSteps = 0;
};
```

Exact schema must align with the current intent architecture and network DTO separation.

Do not expose canonical enemy intent fields to enemy clients.

---

# 43. Facing policy data

Potential data-driven concepts needed over time:

```text
MaxPivotSteps
PivotCostPerStep
MaxAimTurnSteps
AttackFacingPolicy
ReactionFacingPolicy
DisplacementFacingPolicy
CoveredRelativeDirections
ImpactDirectionPolicy
FacingLock
```

Avoid creating one giant enum that combines unrelated semantics.

Separate:

- movement Pivot;
- attack aiming;
- reaction turning;
- displacement effects;
- defensive coverage;
- impact-direction interpretation.

---

# 44. TurnLog

Add explicit Facing-related events/reason data.

Suggested event:

```text
FacingChanged
```

Essential fields:

```text
UnitId
FromFacing
ToFacing
Reason
ActionId / SourceId if relevant
```

Candidate reasons:

```text
MovementHeading
PlannedPivot
AttackAim
Reaction
Ability
ForcedEffect
```

Attack/defense explainability should be able to log:

```text
IncomingDirection
TargetFacing
RelativeDirection
```

A failed directional defense should have a reason such as:

```text
OutsideCoveredDirections
```

and identify the required/actual side set when useful.

The client should explain outcomes from the authoritative TurnLog/reason codes, not recalculate hidden logic independently.

---

# 45. Debug tools

Facing debug visualization should support:

- current absolute Facing index/side;
- relative side labels;
- planned allied Facing;
- Pivot steps/cost;
- ability allowed aim-turn;
- defensive covered sides;
- incoming direction;
- computed relative direction;
- reason for invalid action/reaction;
- TurnLog Facing changes.

Possible integration:

```text
rt.Map.Debug
rt.Turn.DumpSnapshot
rt.Turn.DumpLog
```

or a new focused command if project convention warrants it.

Do not create redundant debug systems if existing overlays can be extended.

---

# 46. Automated tests

At minimum create pure tests for direction normalization and relative relations.

For a target with Facing `0`:

```text
incoming 0 -> Front
incoming 1 -> FrontRight
incoming 2 -> RearRight
incoming 3 -> Rear
incoming 4 -> RearLeft
incoming 5 -> FrontLeft
```

Test all six target Facing values as rotations/permutations of this rule.

Test Pivot normalization:

```text
0 +1 -> 1
0 -1 -> 5
5 +1 -> 0
1 +3 -> 4
```

Test illegal Pivot based on profile limits.

Test that a failed movement step does not silently rotate the unit.

Test that an attack with insufficient `MaxAimTurnSteps` is rejected/fizzles according to policy.

Test that legal attack turn changes and persists Facing.

Test that environmental non-directional damage does not produce a fake rear/front relation.

---

# 47. Determinism tests

Add tests that ensure:

- insertion order does not change Facing outcomes;
- simultaneous impact groups read the same boundary Facing where appropriate;
- repeated snapshot+intent inputs produce identical Facing events/log hash;
- frame rate/presentation does not change final Facing;
- replay reproduces the same Facing sequence;
- state/hash serialization includes the same canonical Facing values.

---

# 48. Network/privacy tests

Add/extend privacy tests so that:

- team A future Facing/Pivot does not appear in team B packets during Planning;
- Brace/Overwatch planned direction is team-only if part of private intent;
- current public Facing is replicated only according to the normal visibility/public-state model;
- hidden unit Facing changes do not silently leak via replicated properties;
- late join/spectator/replay behavior respects information classification;
- canary tests cover Pivot/Facing intent fields.

---

# 49. Functional gameplay scenarios

Create/update sandbox scenarios.

## Scenario FACING-01 — Basic Pivot

- unit starts Facing side 0;
- stationary Pivot +1;
- expected final Facing side 1;
- log contains FacingChanged.

## Scenario FACING-02 — Movement derives Facing

- multi-step hex path with direction changes;
- assert Facing after each successful micro-step;
- assert final Pivot.

## Scenario FACING-03 — Blocked move

- attempted transition blocked;
- assert no free rotation from attempted direction.

## Scenario FACING-04 — Narrow directional defense

- defense covers only `Front`;
- attacks from all six relative sides;
- only Front qualifies.

## Scenario FACING-05 — Wide defense

- defense explicitly covers `{FrontLeft, Front, FrontRight}`;
- verify exact three-side set;
- do not call this a universal Front Arc rule in core geometry.

## Scenario FACING-06 — Forced rotation opens defense

- defender arms directional Brace;
- controller rotates defender through an explicit Facing-control effect;
- second attacker checks new relative side;
- no universal flank damage multiplier is used.

## Scenario FACING-07 — Attack aim persists

- attacker rotates legally to attack;
- attack resolves;
- next event observes the new Facing;
- no snap-back.

## Scenario FACING-08 — Overwatch coverage

- arm Overwatch from known Facing;
- target enters covered area -> opportunity;
- target enters uncovered rear side -> no trigger unless reaction policy explicitly permits turning.

## Scenario FACING-09 — Simultaneous attack boundary

- two logically simultaneous impacts;
- target Facing at boundary is shared;
- iteration order permutation produces the same result.

## Scenario FACING-10 — Fog/privacy

- hidden opponent changes Facing;
- enemy client retains only last legally known Facing;
- no live hidden update.

---

# 50. Documentation to update

Claude must locate and update all relevant canonical documentation rather than adding an isolated orphan note.

At minimum review/update:

- deterministic simulation / snapshot / TurnLog documentation;
- map/grid geometry documentation;
- movement/planning documentation;
- ability/targeting documentation;
- Overwatch / Brace / reaction documentation;
- networking/privacy documentation;
- UI/UX planning and ghost-preview documentation;
- character data/profile documentation;
- roadmap and Definition of Done;
- test/scenario documentation;
- balance matrices if Pivot/Facing values are represented there;
- design glossary.

If a document contains older generic phrases such as:

```text
Facing affects Overwatch
```

expand it enough to link to the canonical Facing specification.

If an older document assumes global Front/Flank/Rear bands, replace that with the six relative hex sides.

---

# 51. Wiki / "Wikipedia" work

Create or update a canonical Wiki page dedicated to Facing.

Suggested page:

```text
Gameplay / Positioning / Facing
```

It should explain to designers/testers:

1. what Facing is;
2. the six sides;
3. Front / FrontRight / RearRight / Rear / RearLeft / FrontLeft;
4. movement-derived Facing;
5. Pivot;
6. attack turn;
7. reaction/Brace/Overwatch relationship;
8. no universal backstab bonus;
9. Facing-control effects;
10. cover vs Facing separation;
11. Fog of War / last known Facing;
12. examples;
13. links to related systems.

Also update cross-links from:

- Movement;
- Brace;
- Overwatch;
- Reactions;
- Cover;
- Targeting;
- Ghost Mode;
- Fog of War;
- Turn Resolver;
- Character Profiles;
- Status/Control;
- Map Geometry.

Avoid duplicating authoritative rules across many Wiki pages. Prefer a canonical Facing page plus concise references.

---

# 52. Roadmap changes

Facing is now sufficiently important to deserve explicit roadmap items.

Do not treat it as a visual polish task.

## F0 / Foundations-compatible work

Keep the first implementation minimal:

- logical six-direction Facing;
- Facing in unit state/snapshot;
- movement-derived Facing;
- stationary/end Pivot;
- TurnLog event;
- debug visualization;
- automation tests.

Do not prematurely add every character-specific reaction.

## Later ability/reaction work

Add:

- attack Facing policies;
- directional ability validation;
- Brace covered-side definitions;
- Overwatch direction integration;
- reaction Pivot policies;
- Facing-control effects;
- ability-specific rear/side bonuses.

## Network milestone

Add:

- team-only planned Facing/Pivot relay;
- hidden enemy Facing privacy tests;
- canary fields.

## UI milestone

Add:

- hex-side Facing selector;
- ghost final Facing;
- directional defense/Overwatch overlays;
- certainty styling;
- explainability reason codes.

## QA milestone

Add the functional scenarios defined above and packaged privacy/replay tests.

---

# 53. GitHub epic proposal

Create or update a dedicated Epic if one does not already cover this cleanly.

Suggested title:

```text
EPIC — Tactical Facing & Directional Interaction
```

Suggested objective:

> Implement deterministic six-direction unit Facing aligned to hex sides, including Pivot, movement-derived orientation, directional ability/reaction validation, UI preview, TurnLog explainability, network privacy, and automated tests.

Do not duplicate an existing Facing/Movement epic if one already exists; extend/link it instead.

---

# 54. Suggested GitHub issues

Claude should inspect the existing roadmap/issues first and create/update only what is actually missing.

Suggested issue breakdown:

### FACING-001 — Add six-direction logical Facing state

Acceptance:

- integer/enum 0..5;
- deterministic normalization;
- included in unit state and snapshot;
- no ActorRotation authority;
- pure tests.

### FACING-002 — Derive Facing from successful movement steps

Acceptance:

- each completed transition updates Facing;
- blocked transition does not grant free rotation;
- TurnLog events emitted;
- micro-step tests.

### FACING-003 — Add planned End/Stationary Pivot

Acceptance:

- path may be empty;
- signed Pivot steps;
- movement-profile limits/cost;
- deterministic validation;
- ghost preview support.

### FACING-004 — Add attack Facing validation

Acceptance:

- per-ability aim-turn policy;
- invalid target has reason code;
- legal attack rotation persists;
- no automatic snap-back.

### FACING-005 — Add relative incoming-direction resolver

Acceptance:

- six relative directions;
- impact direction policy;
- pure deterministic tests;
- non-directional effects supported.

### FACING-006 — Add directional defense coverage sets

Acceptance:

- ability defines explicit covered relative directions;
- narrow/wide/asymmetric sets supported;
- no hard-coded global Front Arc.

### FACING-007 — Integrate Brace with Facing

Acceptance:

- Brace uses covered side set;
- explicit Facing changes alter later eligibility;
- log explainability.

### FACING-008 — Integrate Overwatch with Facing

Acceptance:

- Overwatch area anchored to legal Facing;
- no free rear snap;
- optional reaction Pivot is data-driven;
- trigger tests at micro-step boundaries.

### FACING-009 — Add Facing-control effects

Acceptance:

- PreserveFacing / FaceMovementDirection / FaceSource / FaceAwayFromSource / ExplicitRotation;
- displacement and rotation separate;
- deterministic tests.

### FACING-010 — Add Facing UI and ghost preview

Acceptance:

- six-side selector;
- legal Pivot choices;
- cost;
- team ghost;
- contextual directional overlays;
- 1080p readable.

### FACING-011 — Add Facing TurnLog and debug explainability

Acceptance:

- FacingChanged event;
- reason codes;
- incoming/relative direction for relevant events;
- dump/debug visualization.

### FACING-012 — Add Facing privacy/network tests

Acceptance:

- opponent receives no future Pivot/Facing intent;
- hidden current Facing updates do not leak;
- packaged canary test.

### FACING-013 — Add Facing replay/determinism golden tests

Acceptance:

- repeat/permutation tests;
- same state/log hashes;
- playback FPS independent;
- replay sequence stable.

### FACING-014 — Add Facing functional sandbox scenarios

Acceptance:

- implement FACING-01 through FACING-10 or equivalent;
- scenario selector integration;
- expected log/assertions documented.

---

# 55. Feature map updates

Add a Facing feature family with at least:

```text
Facing Core
Movement-derived Facing
Pivot
Stationary Pivot
Attack Aim Turn
Directional Defense
Brace Facing
Overwatch Facing
Reaction Pivot
Facing Control / Forced Rotation
Facing UI
Facing Team Ghost
Facing Fog/Last Known State
Facing TurnLog
Facing Replay
Facing Privacy Tests
```

Link each item to:

- roadmap issue;
- Wiki page;
- scenario/test where applicable;
- milestone.

---

# 56. Scenario map updates

Add/associate the scenarios from section 49.

Ensure scenarios test **rules**, not animation appearance.

Each scenario should include:

- initial cells;
- initial Facing indices;
- accepted intents;
- expected logical boundaries;
- expected `FacingChanged` events;
- expected final Facing;
- expected failure/eligibility reason codes.

---

# 57. Editor map updates

Add manual Editor tasks only where they genuinely require Editor work.

Likely examples:

- Facing debug decal/mesh visualization;
- six-side Pivot selector visual asset;
- Overwatch/Brace directional overlay materials;
- unit arrow/facing marker;
- functional map placements for Facing test scenarios;
- optional mesh alignment validation (visual forward axis vs logical Front).

Do not put pure C++ work into the Editor map.

---

# 58. Balance data updates

If the existing balance workbook has movement/character rows, add appropriate fields rather than hard-coding values in docs.

Candidate fields:

```text
BaseMovementPoints
PivotCostPerStep
MaxPivotSteps
DefaultAttackAimTurn
```

Ability-specific data may include:

```text
MaxAimTurnSteps
CoveredRelativeDirections
ReactionPivotSteps
ImpactDirectionPolicy
```

Do not treat candidate values from this chat as final balance until playtested.

---

# 59. Data validation

Add validators when the relevant Data Assets exist.

Examples:

- Facing index in `[0..5]`;
- Pivot steps valid for supported range;
- coverage set contains only valid relative directions;
- impossible/contradictory Facing policy combinations;
- ability requires direction but defines `NonDirectional` impact incorrectly;
- `RestorePreviousFacing` used only where supported;
- directional reaction missing coverage definition;
- malformed movement profile limits.

Keep validators deterministic and CI-friendly.

---

# 60. Naming guidance

Prefer terms consistent across code/docs/UI.

Recommended canonical terms:

```text
Facing
FacingDirection
RelativeDirection
Front
FrontRight
RearRight
Rear
RearLeft
FrontLeft
Pivot
PivotSteps
CoveredRelativeDirections
IncomingDirection
ImpactDirectionPolicy
FacingChanged
```

Avoid ambiguous core terms such as:

```text
Side
Flank
FrontArc
BackArc
```

unless a particular ability/design concept defines them explicitly.

---

# 61. Source/code structure proposal

Keep the first implementation small and close to current architecture.

Candidate files:

```text
Source/RefactorTactics/Public/Core/RTFacing.h
Source/RefactorTactics/Public/Map/RTMapGeometry.h
Source/RefactorTactics/Private/Map/RTMapGeometry.cpp
Source/RefactorTactics/Public/Unit/RTUnitState.h
Source/RefactorTactics/Public/Planning/RTIntent.h
Source/RefactorTactics/Public/Turn/RTTurnEvent.h
Source/RefactorTactics/Tests/RTFacingTests.cpp
```

Do not create a `UFacingComponent` unless runtime/presentation needs prove that a component adds value.

The canonical resolver state should stay plain/logical.

---

# 62. Unreal implementation constraints

- UE baseline documentation: 5.8; repository must lock actual patch.
- C++ owns simulation, validation and deterministic rules.
- Blueprint can own presentation and configurable content.
- Do not add more architectural complexity than needed for the milestone.
- Facing must compile in headless/server contexts.
- Avoid dependence on `LocalPlayer`, widgets or animation from resolver code.
- No frame/tick-based authoritative orientation.
- Use stable IDs and deterministic ordering.
- Add log/debug/test with every milestone increment.
- Verify in packaged build when the relevant milestone requires it.

---

# 63. Documentation precedence

When consolidating conflicting older material, apply this precedence:

1. explicit latest decisions in this Facing chat;
2. existing consolidated project decisions;
3. older PDR proposals;
4. old research/brainstorming.

Important latest decision that overrides earlier shorthand:

> The unit's Front, Rear and all intermediate Facing directions are aligned to the six sides of the hexagon. The fundamental model is six explicit relative directions, not global Front/Flank/Rear bands.

Do not preserve obsolete wording merely because it exists in an older document.

---

# 64. Decisions considered consolidated from this chat

Treat these as current design decisions unless a newer repository decision explicitly supersedes them:

- six discrete Facing directions;
- Facing aligns to hex sides;
- Front and Rear are opposite hex sides;
- four intermediate relative directions remain individually addressable;
- Facing persists as logical state;
- movement can update Facing on successful transitions;
- blocked movement gives no free rotation;
- no arbitrary correction during Resolution;
- no automatic snap-back;
- attack turning is policy-driven;
- legal attack rotation persists;
- no universal rear/flank damage bonus;
- directional defenses use explicit side sets;
- Overwatch is Facing-directional;
- forced movement and forced rotation are separate;
- Facing manipulation can be crowd control;
- cover geometry is separate from Facing;
- wall geometry is separate from unit Facing;
- six-side relation is deterministic/integer-based;
- future Facing planning is private team data;
- hidden enemy Facing must not leak;
- TurnLog/debug/tests must expose the logical reason for Facing changes.

---

# 65. Items still requiring playtest/design lock

Do not silently mark these values final:

- exact Pivot cost per 60-degree step;
- exact `MaxPivotSteps` per character;
- exact default aim-turn values by weapon/ability type;
- exact Brace side coverage for each character;
- exact Overwatch side/arc shape per character;
- whether specific movement types preserve or derive Facing;
- special Dash facing behavior;
- which abilities receive Rear/side bonuses;
- which reactions allow reaction Pivot;
- whether stationary units receive any future special Pivot discount;
- future perception/FOV interaction with Facing.

Track these as balance/design follow-ups where appropriate.

---

# 66. Claude execution checklist

Claude must perform the following in order:

## A. Repository audit

- locate existing Facing references;
- locate movement/Pivot references;
- locate Brace/Overwatch direction rules;
- locate PDR/Wiki/roadmap/scenario/feature/editor-map files;
- locate existing relevant GitHub epics/issues;
- identify contradictions and duplicates.

## B. Consolidate canonical documentation

- create/update canonical Facing spec;
- update linked PDRs;
- replace obsolete global band wording;
- add diagrams/tables using six explicit hex sides;
- cross-link related systems.

## C. Update Wiki/Wikipedia

- create/update Facing page;
- update related pages;
- avoid duplicated source-of-truth text.

## D. Update roadmap/maps

- roadmap;
- feature map;
- scenario map;
- editor map;
- QA/test roadmap;
- balance data references.

## E. GitHub work

- update existing Epic if appropriate;
- otherwise create `EPIC — Tactical Facing & Directional Interaction`;
- create/update missing issues from the issue breakdown;
- link milestone, Wiki, scenario and acceptance criteria;
- avoid duplicates.

## F. Tests

- pure direction tests;
- movement/Pivot tests;
- simultaneous resolution tests;
- reaction/Overwatch tests;
- privacy tests;
- replay/determinism tests;
- functional scenarios.

## G. Report

At the end, output a concise consolidation report containing:

```text
Files created
Files updated
Wiki pages created/updated
Roadmap changes
Feature-map changes
Scenario-map changes
Editor-map changes
GitHub epic created/updated
GitHub issues created/updated
Tests added/updated
Conflicts resolved
Open design/balance decisions
Next recommended implementation issue
```

---

# 67. Suggested next implementation slice

After documentation/roadmap consolidation, implement the smallest playable/core increment:

```text
FACING-001
    ->
FACING-002
    ->
FACING-003
    ->
FACING-011
    ->
FACING-013
```

Meaning:

1. six-direction logical state;
2. movement-derived Facing;
3. planned Pivot;
4. TurnLog/debug;
5. deterministic automated tests.

Then move to ability/reaction integration:

```text
FACING-004
FACING-005
FACING-006
FACING-007
FACING-008
```

Do not start with complex character abilities before the core Facing state is stable and replayable.

---

# 68. Suggested commits

Keep commits focused.

Examples:

```text
docs(facing): consolidate six-direction hex-facing rules
feat(facing): add deterministic six-direction unit facing
feat(movement): derive facing and support planned pivot
feat(combat): add directional attack and defense policies
feat(reaction): integrate brace and overwatch facing
feat(ui): add facing and directional planning overlays
test(facing): add deterministic and privacy coverage
docs(wiki): cross-link facing with movement reactions and cover
```

---

# 69. Final design statement

The canonical RefactorTactics Facing model is:

> A unit always has one persistent logical Facing aligned to one of the six sides of its current hex cell. Front, Rear and the four intermediate directions are all explicit six-way relative directions. Movement, Pivot, attacks, reactions and control effects may change Facing only through deterministic policies. Abilities may group specific sides for coverage, but the core system never collapses them into universal Front/Flank/Rear zones. Facing affects directional interaction and counterplay, not a universal rear-damage multiplier. Map cover and wall geometry remain separate systems.

This statement should become the short canonical definition referenced by the Wiki, gameplay specs and code comments.
