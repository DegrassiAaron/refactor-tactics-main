> 📦 `HISTORICAL` · **Sorgente archiviato il 2026-08-12** · **Revisionato, recepito in parte.**
>
> Il testo originale **non è stato riscritto**: quanto segue è l'esito della revisione, non una correzione
> del sorgente. Referto completo, sezione per sezione:
> [`../../../roadmap/plans/action-economy-consolidamento-2026-08-12.md`](../../../roadmap/plans/action-economy-consolidamento-2026-08-12.md).
>
> | Esito | Sezioni |
> |---|---|
> | ✅ **Recepito** — contributo nuovo | §6, §7, §8 (compatibilità abilità↔movimento e modificatori di percorso) · §24 (ordine di roadmap, con una correzione) · §25 (epic, come **E38** in v0.2) · §35–§36 (domande aperte) |
> | **Già canone** — nessuna azione | §3, §9, §11, §12, §14, §16, §17, §31, §32, §33 |
> | ⚠️ **Contraddice il canone** | §4 e §5 (`ActionCapacity` contro il modello a **slot** di `D-028`; «Slot ≡ Action Points, cap 2» è consolidato dal 2026-08-07) · §15 (il pivot in Movement Point contro il **tetto in step** di ADR-0008 §1) |
> | 🔁 **Già aperto altrove** | §15 è **`FAC-12`**, aperta il 2026-08-10 da un altro kit: non ha ricevuto un ID nuovo |
> | 🔴 **Respinto per iscritto** | §30 (aggiornare `RefactorTactics_Balance_Matrices_v0.1.xlsx`): `docs/balance/README.md` vieta la correzione cella per cella e `D-023` lo declassa a `RESEARCH` |
> | ✂️ **Filtrato** | 14 issue proposte → **5** aperte, 2 già coperte, 1 respinta, 6 assorbite · 12 scenari → **6** `planned`, con `ScenarioId` nella convenzione del repository |
>
> ⚠️ **Due premesse di stato erano false quando il kit è stato scritto**, ed è il motivo per cui va letto
> come sorgente e mai come specifica: i Movement Point che §14 chiede di introdurre **esistono**
> (`FRTActionDef::CostMP`, `MoveBudget`, costi interi), e la domanda di §13 «risorsa condivisa o per
> personaggio» era **già chiusa** — risorsa firma per eroe, cap 4, ricarica 1.

---

# RefactorTactics — Action Economy, Movement Coupling & Facing Costs
## Consolidation handoff for Claude Code

**Date:** 2026-08-12  
**Scope:** consolidate the full discussion about what a unit may plan during Planning, how those choices interact with Prep/Dash/Blast/Move, how movement profiles constrain abilities, how cooldown/resource can regulate action density, and how Facing/Pivot may consume Movement Points.

> This file is a **repository consolidation task**, not a greenfield redesign.
> Claude must audit the current repository first, preserve newer canonical decisions already present, and update existing docs/issues instead of creating duplicates.

---

# 0. Required outcome

Claude must:

1. audit the current repository and current GitHub issues/epics;
2. consolidate the action-economy decisions from this handoff into canonical documentation;
3. update the Wiki/Wikipedia;
4. update Roadmap;
5. update Feature Map / Feature Registry;
6. update Scenario Map;
7. update Editor Map / editor-facing design documentation;
8. update balance references/workbooks where relevant;
9. create or update the relevant GitHub Epic and Issues;
10. link Feature -> Roadmap -> Issue -> Scenario/Test -> Wiki;
11. preserve deterministic/server-authoritative simulation;
12. preserve planning privacy;
13. clearly separate **current decisions**, **prototype candidates**, and **open balance/design choices**;
14. finish with a concise consolidation report and the next recommended implementation issue.

Do **not** implement a new gameplay system in code as part of this task unless an already-open repository issue explicitly includes implementation. The primary task is consolidation, traceability, roadmap/issues, scenarios, and design readiness.

---

# 1. Audit before editing

Start with:

```bash
git status
git branch --show-current
git rev-parse HEAD
```

Locate and read the real repository equivalents of at least:

```text
CLAUDE.md
AGENTS.md
README.md
CONTEXT_INDEX.md

docs/decisions/*
docs/gameplay/*
docs/technical/*
docs/product/*
docs/balance/*
docs/roadmap/*
docs/wiki/*
wiki source, if separate

docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/technical/scenario-map.md
editor-map / map-editor roadmap/spec

Scenarios/
Source/RefactorTactics/Planning/
Source/RefactorTactics/Turn/
Source/RefactorTactics/Map/
Source/RefactorTactics/Tests/
```

Search repository-wide for:

```text
Action Economy
ActionBudget
Tempo
Action Points
Movement Points
Move
Sneak
Sprint
Dash
Prep
Blast
Brace
Guard
Overwatch
Wait
Interact
Basic Attack
Cooldown
Energy
Power
Mana
Resource
Facing
Pivot
MaxPivot
MovementProfile
Mobility
Ghost Timeline
Action Dock
Decision Boundary
Reaction Opportunity
Reposition
```

Also inspect existing GitHub Issues/Epics before creating anything.

**Never create a duplicate Epic/Issue when an existing one can be extended or renamed.**

---

# 2. Precedence rule

When material conflicts, use:

```text
latest ADR / Decision Log in repository
> newer explicit project decisions
> current code + data contracts
> current Feature Registry / Roadmap / Scenario Registry
> this handoff
> older handoff files
> old PDR/research/brainstorming
```

Important known examples:

- current round grammar is `Planning -> Prep -> Dash -> Blast -> Move -> Cleanup`;
- normal Move is the last voluntary movement phase;
- Fast Reaction baseline is 3.0 s, timeout HOLD;
- Brace and Overwatch are distinct;
- Facing is six-directional on the hex grid;
- old generic text such as `one movement + one action` is no longer precise enough by itself.

If a newer repository decision conflicts with this handoff:

1. preserve the newer repository canon;
2. do not overwrite silently;
3. record the conflict;
4. update/open a decision issue if needed;
5. include the conflict in the final report.

Use labels such as:

```text
CURRENT
PROTOTYPE
OPEN_BALANCE
OPEN_DESIGN
SUPERSEDED
CONFLICT
HISTORICAL
```

---

# 3. Canonical round structure

Do not change the macro phases unless a newer ADR already has.

```text
PLANNING
   |
COMMIT / READY
   |
PREP
   |
DASH
   |
BLAST
   |
MOVE
   |
CLEANUP
```

Important:

- `Dash` is not the same thing as normal `Move`.
- `Sprint` is not Dash; Sprint is a movement profile of the normal Move family.
- Reactions are not a fifth macro-phase.
- Fast Action / Fast Reaction occur only at explicit Decision Boundaries.
- presentation timing never changes logical timing.

---

# 4. What a player plans for one unit

The latest discussion converged on a **plan composed from several possible blocks**, constrained by cost, compatibility, movement profile, cooldown/resource, and phase.

Conceptual plan:

```text
UNIT PLAN

1. PREP / PREPARED RESPONSE
   optional
   examples:
   - Guard
   - Brace
   - Overwatch
   - stance / prepared counter profile

2. DASH / SPECIAL MOVEMENT
   optional
   examples:
   - Dash
   - Charge
   - Leap
   - Blink
   - Grapple
   - other character-specific special movement

3. MAIN ACTION
   optional
   examples:
   - Basic Attack
   - Hero Ability
   - Interact
   - Wait

4. NORMAL MOVE
   optional
   Movement Profile:
   - Sneak
   - Move / Normal
   - Sprint

5. FACING RESULT
   not a free independent action by default;
   derives from movement, Pivot, action policies and other legal effects.
```

Target, cell, AoE, direction, path, destination, trigger policy, reaction policy, etc. are **parameters of the plan**, not extra actions.

Do not assume the player automatically gets every block every turn.

The final legal combination is produced by validation rules.

---

# 5. Core direction: a bounded number of things per turn

The user wants each unit to have a **limited capacity to perform actions during a turn**, and the number/quality of things possible must depend on what the player planned.

The system should therefore support a general **Action Capacity / Action Budget** concept.

`Tempo` was discussed as a possible name, but the name and exact numeric baseline are **not locked**.

Use a neutral technical name in docs/data until naming is decided, for example:

```text
ActionCapacity
ActionBudget
TurnActionBudget
```

Do not hard-code `3` as canonical merely because it was used in examples.

The model must support actions with different costs:

```text
light action  -> lower ActionCost
heavy action  -> higher ActionCost
prepared action -> cost according to profile
special movement -> cost according to profile
```

The important design rule is:

> The budget defines how much a unit can attempt in one turn, but cost alone does not determine legality.

Legality also depends on:

- phase;
- movement profile;
- compatibility;
- current state/status;
- resources;
- cooldown;
- targeting/LOS;
- character/ability rules;
- environment;
- Facing requirements.

This avoids a generic "buy any 3 actions" AP system.

---

# 6. Movement and action economy must partially overlap

The key new design direction is that movement is not merely an independent free appendage.

The **type and amount of movement planned can change which abilities are legal and how effective they are**.

Examples discussed:

```text
Stationary:
- precision abilities fully available
- heavy attacks may be possible

Sneak:
- shorter movement
- low noise
- stable enough for many abilities

Normal Move:
- standard movement
- some precision/range penalties may apply

Sprint:
- longer movement
- high noise
- may block precise/heavy abilities
- may shorten range, reduce effect, or change action cost
```

Do not lock all abilities into the same rule.

Ability behavior must be data-driven.

---

# 7. Ability <-> movement compatibility

Add/consolidate an explicit data concept allowing an ability to declare how it behaves under the selected movement plan.

Possible conceptual states discussed:

```text
NORMAL
IMPAIRED
ENHANCED
BLOCKED
```

These are useful UX concepts; exact enum/API naming is not locked.

Examples:

```text
Precision Shot
Stationary -> NORMAL / full range
Sneak      -> NORMAL
Move       -> IMPAIRED
Sprint     -> BLOCKED
```

```text
Mobile Shot
Stationary -> NORMAL
Sneak      -> NORMAL
Move       -> NORMAL
Sprint     -> IMPAIRED
```

```text
Momentum Strike
Stationary -> weaker
Move       -> NORMAL
Sprint     -> ENHANCED, e.g. more push/damage
```

Possible modifiers include:

- enabled/disabled;
- range delta;
- AoE delta;
- damage/effect delta;
- push/displacement delta;
- ActionCost delta;
- resource cost delta;
- Facing/aim-turn limit;
- target policy changes;
- reaction availability.

Avoid a swamp of tiny percentages in the default UI.

Prefer discrete, explainable effects where possible.

---

# 8. Movement path itself may affect an ability

Architecture should allow deterministic modifiers based on path facts, without hard-coding hero logic into TurnManager.

Potential derived inputs:

```text
CellsMoved
MovementProfile
ElevationChanged
TurnCount / direction changes
SurfaceTags crossed
Hazards crossed
DashUsed
FinalFacing
```

Examples discussed:

```text
Charge:
more cells moved before impact
-> stronger push / impact
```

```text
Precision attack:
more movement
-> shorter effective range
```

These are examples, not universal rules.

All must be previewable during Planning using only allowed information.

---

# 9. Dash remains special movement

Dash belongs to the `DASH` phase, not Prep and not the normal Move profile family.

Examples:

```text
Dash
Charge
Leap
Blink
Phase Dash
Grapple
```

Dash may have its own:

- path/trajectory policy;
- collision policy;
- occupancy rules;
- hazard policy;
- Facing policy;
- cost;
- resource cost;
- cooldown;
- compatibility with Prep/Main Action/Move.

A mathematical Action Budget that could fit `Overwatch + Dash + Move` does **not** automatically make that plan legal.

Compatibility still matters.

---

# 10. Important Overwatch compatibility constraint

Do not accidentally supersede the newer Overwatch lifecycle just because the generalized action-economy model can represent more combinations.

Current repository handoffs define the v0.1 Universal Overwatch direction approximately as:

```text
Attack OR Ability OR Overwatch
```

and:

```text
Choose Overwatch
-> no voluntary Dash in same baseline plan
```

During the Move phase Overwatch uses:

```text
Stage A: Watch while stationary
Stage B: limited pre-planned Reposition
```

Therefore:

```text
Overwatch + Dash + full normal Move
```

must remain an **extreme/stress-test example**, not a baseline universal combo.

Character-specific/future exceptions may exist later, but only through explicit data/rules and a new approved design decision.

Also preserve:

```text
Brace != Overwatch
```

Both use the Reaction Framework but remain distinct universal actions.

---

# 11. Guard / Brace / Overwatch

Preserve their different tactical identities.

## Guard

Intent:

> prepare to endure or mitigate an incoming attack.

Can often use an automatic/conditional prepared-response policy to avoid excessive manual prompts.

## Brace

Intent:

> prepare against displacement, push/pull, forced geometry change, stability/facing pressure.

Can generate a Fast Reaction when appropriate.

## Overwatch

Intent:

> prepare directional/area control and react to a future legal trigger.

Uses Reaction Opportunity and bounded response policy.

Do not collapse these into a single generic "defend" action.

---

# 12. Wait

Preserve Wait as a deliberate no-main-action choice.

Wait does not automatically grant:

- armor;
- accuracy;
- stealth;
- free reaction;
- free action;
- resource regeneration.

A future resource rule may reward Wait, but that remains a separate explicit decision.

---

# 13. Cooldown and persistent resource

The discussion introduced a second layer beyond Action Capacity:

```text
persistent resource
+
per-ability cooldown
```

Possible player-facing names discussed:

```text
Power
Energy
Mana
```

`Mana` is probably too fantasy-specific for the whole roster.

Use a neutral provisional technical concept such as:

```text
Energy / Power Resource
```

until naming/character semantics are locked.

Purpose separation:

```text
Action Capacity
= how much can I do THIS TURN?

Movement compatibility
= which combinations are legal/effective with THIS movement plan?

Energy/Power
= how many expensive abilities can I afford across actions/turns?

Cooldown
= how often may I repeat THIS specific ability?
```

This separation is valuable and should be documented.

However:

- exact resource name is OPEN_DESIGN;
- exact max value is OPEN_BALANCE;
- exact regen is OPEN_BALANCE;
- whether every character uses the same resource is OPEN_DESIGN;
- whether Wait regenerates it is OPEN_DESIGN;
- exact action-capacity numbers are OPEN_BALANCE.

Do not silently promote example values like `0..6`, `+1/turn`, or `3 Action Capacity` to final canon.

---

# 14. Movement Points

Movement should have its own deterministic budget concept.

Use a neutral term such as:

```text
MovementPoints / MP
```

Movement Points may pay for:

- movement transitions;
- terrain movement cost;
- specific movement-profile costs;
- Pivot / voluntary Facing changes;
- special transitions when defined.

Do not mix presentation distance with real-time velocity.

Use integer/fixed-point logical costs.

---

# 15. Facing may cost Movement Points

Latest explicit direction:

> A voluntary Facing change / Pivot may consume Movement Points.

This is an important update to the previous shorthand where final Facing could look "free".

Facing is not simply a free fifth slot.

The final Facing is the result of legal operations:

```text
movement-derived facing
+
planned Pivot
+
action-facing policy
+
Dash-facing policy
+
reaction/effect-facing policy
```

## Candidate Pivot model

The existing Facing handoff already supports:

```text
1 Pivot step = one 60-degree hex-side change
```

A prototype candidate was:

```text
Pivot cost per 60° step = 1 Movement Point
```

but the exact cost remains **OPEN_BALANCE**.

Support:

```text
Move 3 cells
vs
Move fewer cells + spend MP to Pivot
```

This creates the intended tactical choice:

> Do I travel farther, or arrive correctly oriented?

---

# 16. Movement-derived Facing vs tactical Pivot

Do not charge separately for every visual turn in a curved path.

Separate:

## Locomotion Facing

A successful movement transition may set Facing to the direction of that transition.

This is the natural logical orientation of movement.

## Tactical Pivot

A deliberate orientation change not provided by movement/action policy.

This can cost MP.

Example:

```text
unit reaches destination facing SE
player wants final facing NE
-> planned Pivot
-> consumes MP according to profile
```

Blocked movement must not grant free rotation.

---

# 17. Actions can change Facing as part of the action

An action may legally set/rotate Facing without separately charging a Pivot if that turn is intrinsic to the action definition.

Examples:

```text
Basic Attack:
FaceAttackDirection according to AimTurn policy

Dash:
FaceMovementDirection

Charge:
FaceChargeDirection

Overwatch:
uses PreparedWatchFacing

Brace:
may PreserveFacing or use an explicitly allowed reaction-facing policy
```

Do not double-charge:

```text
Pivot cost
+
action cost
```

when the action itself explicitly includes legal turning.

Action-facing rotation must be:

- deterministic;
- explicit in ability/action definition;
- recorded in TurnLog when relevant;
- persistent unless an action explicitly restores old Facing.

No automatic snap-back.

---

# 18. Suggested logical data model direction

Do not treat this as final Unreal API; adapt to existing project conventions.

Action/ability definition should be able to express concepts equivalent to:

```text
ActionId
Phase
ActionCost
EnergyCost
Cooldown
Targeting
Requirements

MovementCompatibility
MovementModifiers

FacingPolicy
MaxAimTurnSteps

ReactionPolicy if applicable
```

Movement profile should be able to express concepts equivalent to:

```text
MovementProfileId
MovementBudget
NoiseProfile
TerrainCostPolicy
PivotCostPerStep
MaxPivotSteps

AbilityCompatibilityTags / modifiers if architecture chooses profile-driven lookup
```

A plan validator should be able to answer:

```text
LEGAL
or
INVALID + deterministic reason code
```

Examples of reason codes:

```text
InsufficientActionCapacity
InsufficientMovementPoints
InsufficientResource
AbilityOnCooldown
MovementProfileBlocksAbility
MovementModifierRequiresShorterPath
DashIncompatibleWithPreparedAction
OverwatchDisallowsVoluntaryDash
PivotBudgetExceeded
FacingRequirementNotMet
TargetInvalid
```

Keep reason codes stable enough for TurnLog/UI/tests.

---

# 19. UI / Planning HUD updates

Update the HUD/action planning documentation.

The existing principles remain:

```text
Action Dock:
Universal Actions and Hero Kit visually distinct
but never imply a fake second action economy.

Ghost Timeline:
PREP | DASH | BLAST | MOVE
not a generic unordered queue.
```

Add dynamic planning feedback for the new economy.

When movement/path/profile changes, the Ability Bar/Action Dock should update immediately.

Example:

```text
Player selects Sprint

Precision Shot:
BLOCKED
reason: RequiresStableMovement

Mobile Shot:
IMPAIRED
range 5 -> 3

Momentum Strike:
ENHANCED
push +1
```

The UI should expose at least:

- remaining Action Capacity;
- remaining/used Movement Points;
- Energy/Power if/when enabled;
- cooldown;
- selected movement profile;
- changed effective range/effect;
- incompatible actions;
- final Facing;
- Pivot cost;
- deterministic invalid-plan reason.

Use icons/patterns/text, not color alone.

Planning preview must use only:

- own state;
- team intents;
- public/authorized knowledge.

Never simulate hidden enemy planning on the client to produce warnings.

---

# 20. Ghost Timeline / Action Ghost implications

Update Ghost Timeline so it can represent a composed plan without becoming a generic AP queue.

Keep:

```text
PREP -> DASH -> BLAST -> MOVE
```

Show:

```text
PREP:
prepared action / reaction state

DASH:
special movement endpoint + Facing

BLAST:
attack/ability/interact origin, target, AoE, effective values

MOVE:
path, movement profile, MP cost, Pivot/final Facing
```

If a movement choice changes an earlier/later action's effectiveness, show the changed state in the relevant ghost and Action Details.

Examples:

```text
BLAST ghost
Range 6 -> 4 due to Sprint

MOVE ghost
Pivot 120° costs 2 MP
path shortened by one cell
```

Do not make Ghost Timeline authoritative.

---

# 21. Documentation that must be updated

Find the actual current files; likely candidates include equivalents of:

```text
docs/wiki/game/struttura-del-round.md
docs/wiki/game/azioni-e-movimento.md
docs/wiki/game/reazioni-overwatch-e-previsioni.md
docs/wiki/.../facing.md

docs/gameplay/*
docs/technical/*
docs/balance/*
docs/decisions/*

docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/v0.1-definition-of-done.md

docs/technical/scenario-map.md
editor-map equivalent
```

Also inspect `CLAUDE.md`, `AGENTS.md`, README and context/index files for stale one-line summaries such as:

```text
each unit plans one Move and one Action
```

Replace with a concise reference to the canonical action-economy page rather than duplicating all rules everywhere.

---

# 22. Wiki work

Create/update a canonical Wiki page for this system.

Suggested page/title:

```text
Gameplay / Actions / Action Economy and Movement
```

It should explain, for designers/testers:

1. round phases;
2. unit-plan anatomy;
3. Action Capacity concept;
4. why this is not a generic AP system;
5. Prep / Dash / Main Action / Move;
6. Wait / Guard / Brace / Overwatch;
7. Dash vs Sprint;
8. movement profiles;
9. ability-movement compatibility;
10. blocked/impaired/enhanced examples;
11. Movement Points;
12. Facing/Pivot cost;
13. action-induced Facing;
14. Energy/Power vs cooldown;
15. Overwatch exception/lifecycle;
16. planning UI feedback;
17. deterministic validation/reason codes;
18. links to Facing, Reactions, Overwatch, Noise, Terrain, Pathfinding and Character kits.

Do not duplicate the full Facing or Overwatch specs. Cross-link to their owner pages.

---

# 23. Feature Map / Feature Registry changes

Audit existing IDs first.

Create/update stable features equivalent to:

```text
RT-FEAT-ACTION-GENERIC
Wait / Guard / Brace / Interact / Basic Attack / Overwatch

RT-FEAT-ACTION-BUDGET
Turn Action Capacity / composed action economy

RT-FEAT-ACTION-MOVE-PROFILES
Sneak / Move / Sprint

RT-FEAT-ACTION-MOVEMENT-COMPAT
Ability <-> movement compatibility and modifiers

RT-FEAT-ACTION-DASH-DISPLACEMENT
Dash/special movement separated from normal Move

RT-FEAT-ACTION-COOLDOWNS
Per-ability cooldown

RT-FEAT-ACTION-RESOURCE
Persistent Energy/Power resource model

RT-FEAT-MAP-FACING
Six-direction authoritative Facing

RT-FEAT-MAP-PIVOT-COST
Pivot integrated with Movement Points
```

If an equivalent feature already exists, update/link it instead of adding a duplicate.

For each feature link:

```text
owner spec
roadmap epic/checkpoint
issues
tests
scenarios
wiki
status gates
```

Do not mark implementation `DONE` just because documentation exists.

---

# 24. Roadmap changes

The roadmap should show dependency order rather than attempting to implement every idea at once.

Recommended order:

```text
1. canonical Plan schema + deterministic validator
2. Action Capacity data model
3. Movement Points + Move profiles
4. Facing/Pivot MP integration
5. Ability <-> movement compatibility
6. Planning UI / Ghost Timeline preview
7. TurnLog reason codes + automation tests
8. Overwatch/Brace compatibility integration
9. persistent Energy/Power model after design lock
10. balance tuning and character-specific exceptions
```

Keep the existing milestone boundaries if they are already canonical.

Do not drag future balance/resource complexity into Foundations if current milestone scope is smaller.

---

# 25. GitHub Epic

First inspect existing Epics.

If no existing Epic cleanly owns this work, create:

```text
EPIC — Action Economy, Movement Coupling & Facing Costs
```

Objective:

> Implement and document a deterministic, data-driven turn-plan economy in which a unit can compose multiple phase-appropriate actions within bounded capacity, while movement profile, Movement Points, Facing/Pivot, cooldown and resource state determine legal combinations and effectiveness.

Link this Epic to existing:

- Facing Epic;
- Reaction/Overwatch Epic;
- Movement/Pathfinding Epic;
- Planning/UI Epic;

rather than duplicating their internal work.

---

# 26. Suggested GitHub issues

Only create what is missing after audit.

## AE-001 — Consolidate canonical unit-plan schema

Acceptance:

- canonical Prep/Dash/Main/Move plan structure documented;
- phase ownership explicit;
- Wait/Guard/Brace/Overwatch/Interact covered;
- old `one Move + one Action` shorthand corrected;
- cross-links added.

---

## AE-002 — Add deterministic Action Capacity model

Acceptance:

- neutral ActionCapacity/ActionBudget concept;
- integer costs;
- data-driven per-action cost;
- validator returns deterministic reason;
- exact global default remains data/config, not hard-coded design magic;
- tests for legal/illegal combinations.

---

## AE-003 — Integrate Move profiles with plan validation

Acceptance:

- Sneak/Normal/Sprint represented as Move profiles;
- selected profile available to validator;
- Dash is not treated as Sprint;
- movement profile can affect action legality;
- deterministic tests.

---

## AE-004 — Add ability-movement compatibility and effectiveness modifiers

Acceptance:

- ability can express equivalent of Normal/Impaired/Enhanced/Blocked;
- support range/effect/cost modifiers without hero-specific TurnManager branches;
- Planning preview can retrieve effective values;
- reason codes;
- automation tests.

---

## AE-005 — Integrate Movement Points and planned Pivot cost

Acceptance:

- Move cost and Pivot cost share deterministic MP budget where configured;
- six-direction Pivot;
- exact Pivot cost is data-driven;
- blocked move grants no free rotation;
- final Facing preview available;
- tests.

Coordinate with existing Facing issues instead of duplicating them.

---

## AE-006 — Add action-induced Facing policies

Acceptance:

- actions can legally rotate/set Facing according to explicit policy;
- no double-charge when turning is intrinsic to action;
- no automatic snap-back;
- TurnLog records meaningful Facing changes;
- tests.

Coordinate with existing Facing attack-policy issues.

---

## AE-007 — Define Energy/Power resource contract

This may be a **design/data issue first**, not immediate implementation.

Acceptance:

- separate purpose from Action Capacity and cooldown documented;
- decide shared vs character-specific resource semantics;
- decide naming;
- decide regen sources;
- Wait interaction explicitly decided;
- no example values promoted to canon without playtest.

---

## AE-008 — Update Planning HUD for budget/effectiveness feedback

Acceptance:

- Action Capacity visible;
- MP/Pivot cost visible;
- cooldown/resource visible where applicable;
- blocked/impaired/enhanced states readable;
- changed effective range/effect shown;
- no color-only communication;
- 1080p readability.

---

## AE-009 — Update Ghost Timeline for composed plans

Acceptance:

- still phase-based `PREP | DASH | BLAST | MOVE`;
- does not become generic AP queue;
- shows movement profile;
- shows action modifications caused by movement;
- shows Pivot/final Facing;
- team-only intent privacy preserved.

---

## AE-010 — Add plan-validation reason codes and TurnLog/debug

Acceptance:

- deterministic invalid-plan reason codes;
- debug dump shows costs and modifiers;
- effective plan summary available;
- TurnLog contains relevant resolution-side reasons;
- no presentation re-calculation as authority.

---

## AE-011 — Add action-economy automation/golden tests

Acceptance:

- legal plan combinations;
- capacity overflow;
- movement-profile block;
- impaired/enhanced modifier;
- cooldown block;
- resource block when enabled;
- MP/Pivot overflow;
- action-induced Facing;
- repeat/permutation determinism tests.

---

## AE-012 — Add action-economy functional scenarios

Acceptance:

- scenario selector entries;
- visible setup;
- expected plan-validity state;
- expected TurnLog/reason codes;
- documented expected result.

Use Scenario Map entries below.

---

## AE-013 — Add action-plan privacy tests

Acceptance:

- opponent receives no future path;
- opponent receives no future Pivot/final Facing intent;
- opponent receives no selected ability/action cost details;
- team-only Ghost plan stays team-only;
- packaged canary test when networking milestone applies.

---

## AE-014 — Consolidate balance matrices

Acceptance:

- action-cost fields represented once;
- movement-profile modifiers represented once;
- MP/Pivot cost represented;
- cooldown/resource fields linked;
- workbook does not become source of truth for feature status;
- values marked prototype/open where not locked.

---

# 27. Overwatch Issue update

Locate the current Overwatch Epic/Issues and add explicit cross-reference:

```text
Universal Overwatch v0.1
- remains distinct from Brace;
- competes with normal offensive commitment according to current Overwatch canon;
- no voluntary Dash in same baseline plan;
- stationary Watch stage;
- optional pre-planned limited Reposition afterward;
- generalized Action Budget must not accidentally enable
  Overwatch + Dash + full Move for everyone.
```

A future character-specific exception may be a separate issue, not baseline scope.

---

# 28. Scenario Map additions

Create/update scenarios equivalent to the following.

## AE-S01 — Stationary action density

Purpose:

- stationary plan leaves maximum freedom for compatible actions.

Assert:

- legal costs;
- phase order;
- no movement modifier accidentally applied.

---

## AE-S02 — Normal Move modifies precision action

Plan:

```text
Normal Move
+ precision ability
```

Assert:

- ability remains legal;
- configured range/effect modifier is visible;
- effective value matches resolver/validator.

---

## AE-S03 — Sprint blocks heavy precision ability

Plan:

```text
Sprint
+ stationary/heavy precision ability
```

Assert:

```text
INVALID
MovementProfileBlocksAbility
```

UI shows reason before commit.

---

## AE-S04 — Sprint enhances momentum ability

Plan:

```text
Sprint
+ momentum attack
```

Assert:

- legal;
- configured enhanced effect;
- deterministic TurnLog.

---

## AE-S05 — Dash + attack + normal Move

Purpose:

- validate a character/profile where all three are intentionally legal within budget.

Assert:

- Dash resolves in Dash phase;
- attack resolves in Blast;
- normal Move remains last voluntary move;
- costs and Facing consistent.

---

## AE-S06 — Overwatch + voluntary Dash rejected in v0.1 baseline

Assert:

```text
INVALID
DashIncompatibleWithPreparedAction
or equivalent stable reason
```

Do not change current Overwatch lifecycle.

---

## AE-S07 — Brace + attack + movement

Purpose:

- validate partial overlap when budget/compatibility permits.

Assert:

- Brace remains distinct prepared response;
- later action/movement does not erase it unless defined;
- deterministic reaction eligibility.

---

## AE-S08 — Pivot shortens reachable path

Given fixed MP:

```text
longer path + no Pivot
vs
shorter path + Pivot
```

Assert:

- Pivot consumes MP;
- final Facing correct;
- UI reach preview changes.

---

## AE-S09 — Action-induced Facing avoids duplicate Pivot cost

Plan:

```text
move
+ attack that legally faces target
```

Assert:

- attack Facing policy applies;
- no second Pivot charge for intrinsic turn;
- Facing persists after attack unless explicitly restored.

---

## AE-S10 — Cooldown/resource blocks action despite free Action Capacity

Assert:

- Action Capacity remaining is not enough to make the ability legal;
- distinct reason code for cooldown/resource;
- UI communicates correct cause.

---

## AE-S11 — Path length changes action effectiveness

Plan same ability with:

```text
short movement path
vs
long movement path
```

Assert configured effective range/effect differs deterministically.

---

## AE-S12 — Team privacy

Two teams.

Assert during Planning:

- ally sees composed plan, costs, path, final Facing;
- enemy receives none of those future intents;
- no hidden warning derived from enemy plan.

---

# 29. Editor Map / editor tooling updates

Update the editor-facing map/roadmap so designers can author and debug the system.

Expose or plan support for:

```text
ActionCost
Energy/Power cost
Cooldown
Phase
Movement compatibility
Movement modifiers
MovementProfile
MovementBudget
PivotCostPerStep
MaxPivotSteps
FacingPolicy
MaxAimTurnSteps
Overwatch-specific compatibility/reposition profile
```

Editor/debug UX should be able to show:

```text
selected unit
Action Capacity used/remaining
MP used/remaining
selected Move profile
planned Pivot
effective final Facing
ability effective range/effect after movement
invalid-plan reason
cooldowns/resources
```

Add validation for:

- negative costs;
- missing movement compatibility policy where required;
- impossible Pivot limits;
- unknown movement profile;
- contradictory phase/compatibility rules;
- duplicate IDs;
- invalid cooldown/resource values;
- Overwatch baseline contradictions where the v0.1 profile forbids Dash.

Do not create one Actor per logical cell or move gameplay authority into Editor Blueprint logic.

---

# 30. Balance data / workbook

If the repository contains a balance workbook or generated tables, audit and update them.

Known workbook in project material:

```text
RefactorTactics_Balance_Matrices_v0.1.xlsx
```

Do not blindly overwrite numbers.

Add/align fields only where supported by the repository's data pipeline.

Suggested conceptual columns:

```text
ActionId
ActionCapacityCost
Energy/PowerCost
CooldownTurns
Phase
MovementProfileCompatibility
StationaryModifier
SneakModifier
MoveModifier
SprintModifier
PivotInteraction
FacingPolicy
```

Mark prototype values clearly.

The workbook must not become the source of truth for feature completion/status.

---

# 31. Tests and determinism

Every eventual implementation feature must satisfy:

```text
same snapshot
+ same accepted plan
+ same rules/config/version
+ same seed
= same state/log hashes
```

Action-economy validation must not depend on:

- frame rate;
- animation;
- Tick;
- TMap/TSet iteration order;
- client packet timing;
- visual Actor rotation.

Use integer/fixed-point costs.

Add:

- pure validation tests;
- permutation tests;
- replay/golden tests;
- 30/60/144 FPS presentation independence;
- packaged privacy tests when networking applies.

---

# 32. Networking / privacy

The composed plan is team-private planning data.

Enemy clients must not receive:

```text
selected Prep action
selected Dash
selected Main Action
selected ability
path
destination
movement profile
future Pivot
future final Facing
action costs
remaining Action Capacity
planned Energy expenditure
target/AoE
Overwatch area/reposition
```

unless/when a piece of information becomes public through legitimate gameplay.

Preserve:

```text
CanonicalIntentStore -> server only
Team preview DTO -> authorized team only
Public resolved result -> all authorized clients after resolution
```

Never "hide" enemy planning only at widget level.

---

# 33. Definition of Done impact

For a gameplay feature in this cluster, `DONE` eventually requires:

1. deterministic rule/data contract;
2. authoritative validation;
3. no privacy leak;
4. TurnLog/debug explainability;
5. automatic test;
6. scenario where relevant;
7. UI preview where player must understand the rule;
8. documentation/Wiki;
9. packaged verification according to milestone;
10. Feature Registry gates updated from verified evidence.

Do not mark `DONE` from docs alone.

---

# 34. Decisions considered current from this chat

Treat the following as the latest design direction unless a newer repository decision supersedes them:

- a unit should be able to plan more than a rigid universal `one action + one move`;
- the number of things it can do is bounded;
- different actions can have different costs;
- legality is not determined by cost alone;
- Prep, Dash, Main Action and Move may partially coexist;
- Dash is separate from normal Move;
- Sprint is a normal Move profile, not Dash;
- movement profile can disable, shorten, weaken or enhance abilities;
- movement/action coupling should be data-driven;
- an ability can be Stationary-oriented, Stable, Mobile or Momentum-like in behavior, but exact enum names are open;
- simple discrete UI states like Normal/Impaired/Enhanced/Blocked are preferred to unreadable piles of tiny percentages;
- cooldown and persistent resource solve different problems;
- a neutral Power/Energy concept is preferable to globally assuming fantasy Mana;
- exact Power/Energy rules are not locked;
- Movement Points are distinct from Action Capacity;
- voluntary Pivot/Facing change may consume Movement Points;
- final Facing is a plan result, not automatically a free independent action;
- successful movement may derive Facing;
- blocked movement gives no free rotation;
- actions may legally set Facing as part of their own policy without paying a duplicate Pivot cost;
- legal action rotation persists unless explicitly restored;
- Overwatch + Dash + full Move was an extreme example, not baseline universal behavior;
- current Universal Overwatch v0.1 lifecycle/restrictions must not be silently broken by the generalized economy;
- Brace and Overwatch remain distinct.

---

# 35. Open design/balance questions

Do **not** silently lock these:

```text
final player-facing name for Action Capacity
default Action Capacity per turn
cost of each universal action
whether normal Move directly consumes Action Capacity
whether Sprint consumes extra Action Capacity or only applies restrictions
exact Sneak/Normal/Sprint movement budgets
exact ability penalties/bonuses by movement profile
exact Power/Energy name
shared resource vs character-specific resources
Power/Energy maximum
Power/Energy regeneration
whether Wait restores Power/Energy
exact cooldown values
exact Pivot cost per 60°
exact MaxPivotSteps
character-specific Pivot discounts
which actions include free/intrinsic Facing rotation
character-specific exceptions to Overwatch/Dash compatibility
exact interaction phase/cost for map interactions
```

Create/update a design decision issue for these rather than inventing final numbers.

---

# 36. Suggested decision questions for future playtest

Track these in the appropriate design issue:

1. Does a fixed Action Capacity create enough meaningful trade-offs without feeling like generic AP?
2. Should movement consume Action Capacity directly, or should movement primarily constrain/modify other actions?
3. Is Sprint best balanced by:
   - higher Action Capacity cost,
   - ability restrictions,
   - lower effectiveness,
   - higher noise/risk,
   - or a combination?
4. Does Pivot consuming MP create interesting choices or excessive micromanagement?
5. How many discrete movement-effect states can players understand in a 30-second Planning window?
6. Should Energy/Power be universal or part of each character's identity?
7. Which prepared actions can coexist with Dash/Main/Move?
8. Which exceptions are character-defining rather than baseline rules?

---

# 37. Suggested commit sequence for consolidation

Keep commits focused.

Examples:

```text
docs(actions): consolidate composed turn-plan and action economy
docs(movement): define movement-profile ability coupling
docs(facing): link pivot costs to movement budget
docs(reactions): preserve overwatch compatibility constraints
docs(wiki): document action economy and movement interactions
docs(roadmap): add action-budget and movement-compat features
test(scenarios): register action economy validation scenarios
chore(github): align epics and issues with feature registry
```

If actual repository conventions differ, follow them.

---

# 38. Final Claude execution checklist

## A. Audit
- current branch/HEAD;
- relevant docs;
- Feature Registry;
- Roadmap;
- Scenario Map;
- Editor Map;
- Wiki;
- balance data;
- existing Issues/Epics;
- conflicting older material.

## B. Consolidate docs
- canonical action-economy page;
- movement profile coupling;
- Facing/Pivot MP rule;
- resource/cooldown separation;
- Overwatch compatibility note;
- cross-links.

## C. Wiki
- create/update canonical page;
- update round/movement/reaction/facing links;
- avoid duplicate source-of-truth prose.

## D. Feature Map
- add/update stable FeatureIds;
- owners/dependencies/gates/issues/tests/scenarios/wiki refs.

## E. Roadmap
- order work by dependencies;
- do not pull open balance into premature implementation.

## F. Scenario Map
- add/update AE-S01..AE-S12 or repository-equivalent scenarios.

## G. Editor Map
- add authoring/debug/validation needs for costs, movement modifiers and Pivot.

## H. GitHub
- update existing Epic if possible;
- otherwise create `EPIC — Action Economy, Movement Coupling & Facing Costs`;
- create/update only missing issues;
- link acceptance criteria, features and scenarios;
- avoid duplicates.

## I. Final report

Return:

```text
Repository HEAD
Files created
Files updated
Wiki pages created/updated
Feature Registry changes
Roadmap changes
Scenario Map changes
Editor Map changes
Balance data changes
Epic created/updated
Issues created/updated
Conflicts found/resolved
Open design decisions
Open balance decisions
Next recommended issue
Suggested next commit
```

---

# 39. Short canonical design statement

Use this as the concise cross-reference, unless a newer ADR supersedes it:

> RefactorTactics uses a composed, phase-aware turn plan rather than a rigid universal "one action + one move" rule. A unit has bounded action capacity, while phase, movement profile, Movement Points, Facing/Pivot, cooldown, resource state and ability-specific compatibility determine which combinations are legal and how effective they are. Dash is a special movement phase, Sprint is a normal Move profile, and voluntary Pivot may consume Movement Points. Actions may rotate Facing only through explicit deterministic policies. The system remains server-authoritative, deterministic, data-driven and private by team during Planning.

