# CLAUDE HANDOFF — Create GrayKit Combat Feedback Issues

## Mission

Work in repository:

`DegrassiAaron/refactor-tactics-main`

Create the GitHub issue structure needed to implement **GrayKit Combat Feedback** for Refactor Tactics.

Do **not** implement code in this task.

The deliverable of this task is the **issue hierarchy and tracking only**.

---

# 1. FIRST: inspect before creating anything

Before creating issues, inspect the repository and existing GitHub issues.

At minimum verify:

- #1990 — `[EPIC] Gray Kit Playground — Visual Language & Validation Lab`
- #286 — `[EPIC v0.1] E21 — Presentazione e leggibilità`
- #288 — animation / locomotion / impact presentation owner
- #2245 — `StatusChanged` playback event
- #2274 — status badge composition / ordering
- #2288 — world-space Unit Overlay with HP / shield / energy / status
- #2336 — status icons / icon representation
- #1625 — visual playback / Scenario Composer playback
- #2272 — playback / seek coordinate issue if still relevant
- any open issue created after these that already covers:
  - floating damage numbers
  - hit feedback
  - status appear/disappear animation
  - animation debug overlay
  - event-to-cue mapping

Use `gh issue view`, `gh issue list`, `gh search issues`, repository search, and source inspection as needed.

## STOP condition

If an equivalent issue already exists, **do not create a duplicate**.

Instead:

1. record the existing issue number;
2. reuse it in the new Epic;
3. only create the missing pieces.

---

# 2. Core architectural rule

The system must follow:

```text
Resolver / TurnLog / ResolvedTimeline
            |
            v
      resolved event
        +---+----------------+
        |                    |
        v                    v
 animation cue        combat feedback
```

Presentation consumes already-resolved facts.

Presentation must never become gameplay authority.

Forbidden patterns:

```text
animation finished -> apply damage
widget checks Health delta -> infer attack
widget polls HasStatus -> infer StatusApplied
GrayKit recalculates LOS / targeting / damage / pathfinding
```

The animation / feedback layer must consume canonical data.

---

# 3. Known existing facts

Treat these as starting hypotheses and VERIFY them against current main before writing issue bodies.

## Existing static unit overlay

Issue #2288 already owns the persistent world-space unit overlay:

- name
- HP
- shield
- energy
- status icons

Therefore:

**DO NOT create a new generic “show HP above unit” issue.**

The new work is about **transient change feedback**.

---

## Existing resolved event model

Verify `FRTResolvedEvent` and `ERTResolvedEventType`.

Expected event types include:

- `Move`
- `Attack`
- `HazardDamage`
- `Defeated`
- `AttackFootprint`
- `ReactionResolved`
- `StatusChanged`

Expected useful data includes:

- `SourceStableUnitId`
- `TargetStableUnitId`
- `Amount`
- phase
- hit cells
- origin / aim cell
- status tag / status outcome / duration where applicable

The presentation layer must consume these values instead of recalculating them.

---

# 4. Create ONE cross-release Epic

Create a single Epic unless an equivalent Epic already exists.

Suggested title:

`[EPIC] GrayKit Combat Feedback — animazioni, HP, danni e status (v0.1 → v1.0)`

Labels:

- `epic`
- `enhancement`
- `P2`

Do NOT assign this Epic a game release milestone if the repository convention treats this capability as cross-release / tooling / visual language.

The Epic must explicitly link:

- #1990 Gray Kit Playground
- #286 E21 presentation/readability
- #288 animation owner
- #2288 static Unit Overlay
- #2245 status playback event

---

# 5. Epic intent

The Epic must state that the player should understand:

1. which action/event just activated;
2. who caused it;
3. who received it;
4. how much HP changed;
5. which effect/status appeared or disappeared;
6. what canonical event drove the visual result.

The feature has two layers.

## Player Readability

Primary communication:

- ring / pulse
- arrow / line / trail where appropriate
- impact cue
- floating damage number
- HP bar response
- status icon pulse / fade
- defeat cue

The player-facing layer must remain understandable with technical diagnostics OFF.

## Developer Diagnostics

Optional debug information:

- Stable Unit ID
- `ERTResolvedEventType`
- phase
- action id, only if already canonically available
- animation/montage state, only if already exposed
- playback coordinate / boundary / microstep only from canonical source

If a field is unavailable, display `N/A`.

Do not reconstruct missing facts.

---

# 6. Epic maturity roadmap

The Epic must contain a maturity ladder, but DO NOT automatically create all future leaf issues.

## v0.1

Minimum observable combat feedback:

- activation
- impact
- damage number
- HP response
- status enter/exit
- defeat cue
- optional developer diagnostics

No Niagara requirement.

---

## v0.2

Explain richer effect outcomes only when canonical runtime semantics exist.

Examples:

- reduced damage
- prevented damage
- reaction outcome
- status consequence

Do not invent severity categories in presentation.

---

## v0.3

Solve simultaneous-event readability:

- overlapping numbers
- multiple hits
- event ordering
- stacking
- clutter
- temporal priority

Do not implement speculative aggregation in v0.1.

---

## v0.5

Make event → cue mapping reusable / data-driven where useful.

Still no gameplay authority in the mapping.

---

## v1.0

Same visual grammar across:

- Planning
- Resolution
- Replay / Playback
- Gray Kit Playground
- Scenario tooling where applicable

Include accessibility / non-color-only validation.

---

# 7. Create only the v0.1 leaf issues that are justified today

The default target is **4 leaf issues**.

If current repository inspection proves one already exists, reuse it instead of duplicating it.

---

# ISSUE A — Event → Cue

Suggested title:

`[v0.1][GrayKit] Event → cue — attivazione, impatto e defeat senza gameplay logic`

Purpose:

Create the minimum visual grammar driven directly by resolved events.

Scope:

- activation ring/pulse on source;
- impact marker on target / cell;
- defeat cue distinct from normal hit;
- optional technical event label;
- `ReactionResolved` cue when justified;
- `AttackFootprint` consumes frozen `HitCells/Shape/Origin/AimCell`.

Hard rule:

Do not call targeting / hit-cell generation again from presentation.

Suggested tests:

- pure mapping `ResolvedEvent -> CueDescriptor`;
- Attack source/target;
- Defeated;
- ReactionResolved;
- AttackFootprint uses event cells exactly;
- Stable ID `0` handled as NONE, never Unit 0.

---

# ISSUE B — Floating damage + HP response

Suggested title:

`[v0.1][GrayKit] Damage feedback — floating numbers e risposta HP guidati da ResolvedEvent`

Dependencies:

- #2288

Purpose:

The HP bar shows state, but the player also needs to see the **change**.

Scope:

For `Attack` and `HazardDamage`:

- floating damage token near target;
- brief HP bar emphasis/pulse;
- resulting HP state from canonical view/state;
- never apply damage from UI.

v0.1 policy:

```text
one damage event -> one visual token
```

Do NOT aggregate multi-hit events in v0.1.

Explicitly decide behavior for `Amount == 0`.

Do not leave this accidental.

Possible choices:

- neutral zero/block cue;
- no number but reaction/defense cue.

The issue must require the implementation to choose and document one.

Tests:

- `Amount=17`;
- hazard damage with no source unit;
- `Amount=0`;
- missing target actor;
- replay/seek visual state parity.

---

# ISSUE C — Status lifecycle

Suggested title:

`[v0.1][GrayKit] Status lifecycle — pulse/fade delle icone su StatusChanged`

Dependencies:

- #2245
- #2274
- #2288
- #2336

Purpose:

Persistent icons already show current status.

This issue adds the **visible transition**.

Scope:

On `StatusChanged`:

Birth:

- icon appears / pulses.

Death:

- icon exit / fade before removal.

Diagnostics may show:

- StatusTag
- StatusOutcome

Use the canonical status-birth classification owner.

Do NOT duplicate the logic with an independent local mapping if the repository already has something like:

`URTTurnLogLibrary::IsStatusBirth`

Requirements:

- timed status;
- expired status;
- cell-bound / revoked status;
- no fake countdown for cell-bound status;
- final icon set matches `BuildStatusBadges`.

No polling `HasStatus` to infer transition.

---

# ISSUE D — Animation / Playback Diagnostics

Suggested title:

`[v0.1][GrayKit] Animation diagnostics — overlay evento/animazione separato dal layer player-facing`

Dependencies / related:

- #288
- #1625
- playback/seek owners such as #2272 if still current

Purpose:

Distinguish:

1. gameplay event missing;
2. event exists but consumer not called;
3. wrong animation/cue;
4. wrong playback timing/seek coordinate.

Scope:

Toggleable Developer Diagnostics.

Display only existing canonical data:

- Stable Unit ID
- resolved event type
- phase
- canonical playback coordinate if available
- microstep only from canonical coordinate
- animation/montage state if exposed

Never create a local playback counter.

If MicroStepIndex is part of the canonical coordinate, do not reconstruct playback position from only Turn + Phase.

Requirements:

- diagnostics ON/OFF;
- no gameplay mutation;
- event-driven update where possible;
- no Tick polling unless there is a measured reason;
- diagnostics OFF must not break player readability.

---

# 8. Gray Kit Playground constraint

Do not automatically create a new Station 08 implementation issue if #1990 still states that Stations 02–08 must not be anticipated before the GKP 0.1 gate.

Respect the current repository governance.

The combat feedback system can first become a valid runtime/presentation consumer.

Then GrayKit integration can be opened when #1990's gate allows it.

If the gate has changed on current main, document the current state before creating the integration issue.

---

# 9. Visual design constraints

v0.1 must prefer low-cost primitives.

Allowed examples:

- UMG
- simple material pulse
- mesh / primitive
- decal
- line / arrow
- icon
- floating text

Do not require Niagara for v0.1.

If Niagara becomes desirable later, treat it as presentation polish after readability is proven.

Never use only color as the communication channel.

Examples:

Bad:

- red = damage
- green = heal

Better:

- number + sign
- distinct icon
- geometry
- pulse direction
- text only in diagnostics

---

# 10. Simultaneous turns — important risk

Refactor Tactics uses simultaneous planning/resolution.

The issue bodies must acknowledge that combat feedback can become unreadable when many events resolve close together.

For v0.1:

prefer:

```text
one event -> one signal
```

Avoid clever aggregation.

For v0.3:

explicitly schedule investigation of:

- overlapping damage numbers;
- hit sequencing;
- multi-target AoE;
- reaction + attack chains;
- status + damage in same boundary;
- defeated unit receiving multiple resolved visual events.

---

# 11. Replay / seek invariant

Where applicable, require:

```text
playback lineare at boundary X
==
seek direttamente a boundary X
```

for persistent visual state.

Transient one-shot effects may need a defined replay policy, but must never leave the persistent state inconsistent.

Do not add presentation data to:

- MapState
- snapshot
- TurnLog
- StateHash

solely to make UI easier.

If a canonical event lacks required information, stop and document the missing producer instead of inventing a second truth in presentation.

---

# 12. GitHub operations

Use GitHub CLI.

Expected flow:

```bash
gh auth status
gh issue list --repo DegrassiAaron/refactor-tactics-main
gh issue view 1990 --repo DegrassiAaron/refactor-tactics-main
...
```

Create the Epic first.

Then create missing leaf issues.

Use existing labels only.

Likely labels:

- `enhancement`
- `epic`
- `v0.1`
- `P2`

For v0.1 leaf issues, use milestone:

`v0.1 · Leggibilità`

ONLY if that milestone is still the correct owner after current inspection.

Do not create labels or milestones just because this prompt names them.

---

# 13. Sub-issue relationships

If the GitHub API / `gh` supports formal sub-issues in the repository, add the leaf issues under the Epic.

If formal sub-issue mutation is unavailable:

- link the Epic in every leaf body;
- add a checklist with issue numbers in the Epic;
- make the relationship readable in both directions.

Do not fail the whole task just because the formal sub-issue API is unavailable.

---

# 14. Update existing owner issues

After creating the new Epic, add a concise comment to:

## #1990

Explain:

- new cross-release combat feedback workstream;
- does not duplicate GrayKit ownership;
- does not prematurely fund Station 02–08;
- future GrayKit integration waits for its gate.

## #286

Explain:

- animation/presentation ownership remains there;
- static HP/status remains #2288;
- new Epic owns transient event-driven combat feedback.

Do NOT rewrite large existing Epic bodies unless necessary.

Prefer additive comments.

---

# 15. Do not create speculative issue spam

Critical rule:

Do NOT create:

- one Epic per release;
- one issue for every hypothetical visual effect;
- future Niagara issues;
- future heal/crit issues if canonical semantics do not exist;
- duplicate HP overlay issue;
- duplicate status icon issue;
- duplicate animation runtime issue.

The hierarchy should remain:

```text
GrayKit Combat Feedback Epic
    |
    +-- v0.1 Event → Cue
    +-- v0.1 Damage feedback
    +-- v0.1 Status lifecycle
    +-- v0.1 Animation diagnostics

future maturity
    |
    +-- documented as gates
    +-- leaf issues created only when actionable
```

---

# 16. Final verification

Before declaring success, print a report like:

```text
CREATED
Epic #XXXX
#XXXX Event → Cue
#XXXX Damage Feedback
#XXXX Status Lifecycle
#XXXX Animation Diagnostics

REUSED
#NNNN existing equivalent issue

LINKED
#1990 comment
#286 comment

NOT CREATED
v0.2+ leaf issues — intentionally deferred

RISKS / BLOCKERS
...
```

Then verify each new issue with:

```bash
gh issue view <number> --repo DegrassiAaron/refactor-tactics-main
```

Confirm:

- correct title;
- correct labels;
- correct milestone;
- correct Epic link;
- no obvious duplicate;
- acceptance criteria present;
- architectural invariant present.

---

# 17. Important working rule

If current repository state contradicts this handoff:

**current main wins.**

Do not force this prompt over newer facts.

Classify every deviation in the final report:

- FACT — found in repository;
- INFERENCE — conclusion from current state;
- CHANGE — modification you made;
- BLOCKER — missing data/permission that prevented completion.

Do not implement game code in this task.
