---
name: implement-feature
description: Critically analyzes a RefactorTactics objective, reconciles it with live GitHub issues and repository contracts, creates or updates the minimum set of independent work-unit issues, and generates 3-6 Claude Code terminal prompts that execute those issues through /issue-run in parallel. Exactly one terminal owns final Validation + Unreal Editor/PIE validation.
argument-hint: "<feature or objective description>"
disable-model-invocation: true
---

# RefactorTactics — Implement Feature

Objective supplied by the user:

`$ARGUMENTS`

Invocation example:

`/implement-feature Replay scenario`

This skill is an **orchestrator of issues and Claude Code terminals**.

It MUST NOT implement the requested feature itself.

Its job is:

1. understand the requested outcome;
2. inspect the repository and live GitHub state;
3. **criticize the proposed direction before producing a plan**;
4. find, create, or update the minimum useful set of GitHub issues;
5. make those issues independently claimable by `/issue-run`;
6. generate prompts for 3-6 Claude Code terminals;
7. reserve **exactly one** terminal for combined `VALIDATION_EDITOR`;
8. leave a dependency graph that can actually be executed without two terminals owning the same issue or binary Editor asset.

---

# Non-negotiable invariants

## I1 — Every execution terminal uses `/issue-run`

Every terminal that performs work MUST execute exactly one GitHub issue through:

`/issue-run <issue-number>`

Do not replace `/issue-run` with an ad-hoc implementation prompt.

The issue is the operational source of truth.

The terminal prompt may add context and guardrails, but the actual work starts from `/issue-run`.

## I2 — One terminal, one issue, one owner

`/issue-run` claims its issue.

Therefore:

- NEVER assign the same issue to two terminals;
- NEVER ask two terminals to solve different slices of one claimed issue concurrently;
- if a parent issue is too large for safe parallel work, create or reuse **focused child/work-unit issues**;
- the parent remains the capability/scope owner when that is already the repository model;
- child issues are execution units, not duplicate primary owners.

## I3 — Exactly one Validation + Editor terminal

There MUST be exactly one terminal of type:

`VALIDATION_EDITOR`

Do not create:
- a separate `VALIDATION` terminal;
- a separate `EDITOR` terminal;
- multiple PIE/Editor owners.

This terminal owns the final integration validation and all required Unreal Editor / PIE / visual MCP validation for the feature.

Implementation terminals should use headless automation whenever possible.

If a worker issue needs visual/Editor evidence to be considered complete, define that evidence as delegated to the `VALIDATION_EDITOR` issue and link the dependency explicitly.

## I4 — Critique before plan

Do not create or update issues until the objective has passed a **Critical Review Gate**.

The first output of the skill is analysis, not a task list.

Challenge:
- whether the requested outcome already exists;
- whether an open issue already owns it;
- whether the apparent feature is actually a bug, missing validation, tooling gap, data gap, authoring gap, or unresolved design decision;
- whether parallelization is safe;
- whether the work can be split without overlapping files/contracts;
- whether a scenario is actually needed;
- whether Unreal Editor/MCP is actually needed;
- whether the requested work would create a second authority, second simulator, duplicated feature owner, or unnecessary infrastructure;
- whether a dependency must land first;
- whether the smallest testable solution is smaller than the requested solution.

If evidence says the requested plan is wrong, say so and change the decomposition.

Do not create work merely to fill terminals.

## I5 — Minimum useful issue set

Reuse an existing issue when it already owns the required work.

Create a new issue only when:
- no existing issue owns the work;
- a large existing issue needs independently executable child work;
- a follow-up is genuinely outside the current issue scope;
- final integration/Editor validation needs its own single owner.

Do not create duplicate issues for symmetry.

## I6 — MCP and scenarios are conditional tools

Use GitHub MCP when available for live issue reads/writes.

Use Epic/Unreal MCP only when it adds meaningful evidence.

Use Scenario Harness scenarios when the behavior is best proved end-to-end.

Do not add scenarios merely because the feature is gameplay-related.

Do not add Editor validation merely because Unreal is the engine.

Prefer:
- unit/automation tests for local logic;
- deterministic Scenario Harness coverage for cross-system gameplay;
- Editor/PIE validation for authoring, visualization, asset wiring, input, presentation, or behavior that cannot be proven headlessly;
- packaged/network scenarios only when the requirement depends on those environments.

## I7 — One owner for binary Unreal assets

No two parallel terminals may edit the same:
- `.uasset`
- `.umap`
- other binary Unreal artifact.

All Editor-created or Editor-modified binary assets required by this orchestration belong to `VALIDATION_EDITOR`, unless a dedicated content issue is absolutely necessary.

If a dedicated content issue owns binary assets, `VALIDATION_EDITOR` may inspect/use them but MUST NOT independently recreate or rewrite them unless fixing an integration defect.

## I8 — Preserve RefactorTactics authority boundaries

When relevant:
- runtime/resolver remains gameplay authority;
- Editor/UI is a consumer of canonical runtime/query/DTO surfaces;
- replay consumes canonical traces and does not recalculate gameplay;
- planning privacy is enforced at the data/replication boundary, not just visually;
- deterministic ordering and stable IDs are preserved;
- scenarios exercise the real gameplay path, not shortcuts such as direct teleport/damage unless the tested system explicitly owns that operation;
- presentation/animation timing never determines simulation outcome.

---

# Phase 0 — Normalize the objective

Normalize `$ARGUMENTS` into a short working objective.

Examples:

`Replay scenario`
`GrayKit action fallback`
`Semantic path preview`
`Ready/Commit reconnect flow`

Do not reinterpret a vague objective into a large feature without evidence.

Record:

- `OBJECTIVE`
- `SUCCESS OUTCOME`
- `KNOWN CONSTRAINTS`
- `UNKNOWN / NEEDS EVIDENCE`

Do not ask the user for information that can be discovered from the repository, GitHub, MCP, roadmap, ADRs, tests or scenarios.

---

# Phase 1 — Repository and issue discovery

Before proposing any plan, inspect the current state.

## Repository

Read when present and relevant:

- `AGENTS.md`
- `CLAUDE.md`
- `.claude/skills/issue-run/SKILL.md`
- relevant ADR/PDR/Decision Log entries
- roadmap/checkpoint documents
- related specs
- existing tests
- Scenario Harness infrastructure
- replay/TurnLog contracts
- Editor/tooling surfaces
- asset maps / editor session definitions
- affected runtime code

Search for the objective by:
- feature terminology;
- class/type/function names;
- test names;
- scenario names;
- issue references;
- roadmap references.

## GitHub

Use GitHub MCP first when available.

Search:
- open issues;
- recently closed issues when they may have delivered part of the objective;
- epics/parents;
- dependencies/blockers;
- PRs if relevant;
- roadmap/capability tracking issues.

For each relevant issue classify:

- `OWNER`
- `DEPENDENCY`
- `DUPLICATE / OVERLAP`
- `DELIVERED PART`
- `BLOCKER`
- `NOT RELEVANT`

Do not trust an old handoff over live repository + live GitHub state.

---

# Phase 2 — Critical Review Gate

Produce a concise but serious review BEFORE issue creation/update.

Use this structure:

## INTENTO
What the user is actually trying to achieve.

## FATTI
Only what repository/GitHub evidence proves.

## INFERENZE
Likely consequences, explicitly labeled.

## PROBLEMI / AMBIGUITÀ
Missing ownership, duplicate authority, unclear acceptance criteria, overlap, risky shared files, unresolved decisions.

## FAILURE MODES
What can go wrong if implemented as first imagined.

Check at least:
- fake parallelism caused by workers touching the same files;
- parent issue being claimed by multiple terminals;
- duplicate scenario/editor work;
- binary asset merge conflicts;
- duplicated runtime/editor rules;
- tests that prove implementation details instead of behavior;
- scenario that passes vacuously;
- replay/state hash drift;
- privacy leakage;
- nondeterministic iteration/order;
- validation only at the end when a contract mismatch could have been detected earlier.

## ALTERNATIVE PIÙ PICCOLA
The smallest safe/testable path.

## PARALLELIZATION VERDICT
Choose one:
- `SAFE`
- `SAFE WITH CONTRACT`
- `LIMITED`
- `BLOCKED`

Explain why.

### Stop condition

If `BLOCKED` because of an unresolved author/design decision or missing prerequisite:
- create/update the blocking issue if appropriate;
- do NOT fabricate an implementation plan;
- generate only safe preparatory issue-runs if they have independent value;
- otherwise stop with the blocker.

---

# Phase 3 — Build the issue graph

Only after the Critical Review Gate passes, create/update the execution graph.

Target: **3-6 terminal issues total**, including `VALIDATION_EDITOR`.

Default shapes:

### Small
- Worker 1
- Worker 2
- VALIDATION_EDITOR

### Medium
- Worker 1
- Worker 2
- Worker 3
- VALIDATION_EDITOR

### Large
- Worker 1
- Worker 2
- Worker 3
- Worker 4
- VALIDATION_EDITOR

Do not exceed 6 unless the user explicitly asks.

## Allowed worker types

Choose based on evidence, not a template:

- `CORE_ARCHITECTURE`
- `INTEGRATION_DATA`
- `AUTOMATION_TESTS`
- `SCENARIOS`
- `TOOLING_UI`
- `NETWORKING`
- `CONTENT_PIPELINE`
- `REPLAY_TURNLOG`
- `DOCUMENTATION_MIGRATION`

Do not create a worker just because a category exists.

Tests may stay with implementation when separating them would create overlapping write sets.

## Parent/owner issue

If a live issue already owns the objective:
- keep it as the parent/owner;
- update it with the execution decomposition and dependencies;
- do not run the parent concurrently in multiple terminals.

If the parent itself is a small executable issue and can be one worker:
- one terminal may own it through `/issue-run`;
- other terminals must own distinct dependent issues.

If no owner exists:
- create the minimum correct owner issue or attach the work to the correct existing epic/capability.

---

# Phase 4 — Create or update each issue

Use GitHub MCP first when available; otherwise use the repository's configured GitHub path (`gh` only if already configured).

For every work-unit issue, ensure the issue contains enough information for `/issue-run` to define it further with `sc:spec-panel`.

Do NOT duplicate the entire D001-D010 definition that `/issue-run` will create.

Provide:

## Why
Why this work unit exists.

## Scope
What this terminal owns.

## Out of scope
What belongs to another terminal or owner.

## Dependencies
Explicit issue numbers.

## Contract
Inputs/outputs/shared API that other workers rely on.

## Expected write set
Expected modules/files/assets.

This is a guardrail, not permission to modify unrelated files.

## Acceptance criteria
Behavioral, observable criteria.

## Automation
Required headless tests.

## Scenario
One of:
- required, with what it proves;
- existing scenario to extend/reuse;
- `N/A` with reason.

## Editor / PIE
For every non-validation worker, normally write:

`Delegated to #<VALIDATION_EDITOR_ISSUE>. Do not own or modify Editor/PIE binary validation assets in this issue.`

Use `N/A` only when Editor validation is genuinely irrelevant to the whole feature.

## TurnLog / Replay impact
Required when relevant.

## Privacy / Networking impact
Required when relevant.

## DoD
What `/issue-run` must be able to prove before this worker hands off.

---

# Phase 5 — Create the single VALIDATION_EDITOR issue

There must be exactly one issue dedicated to final integration + Editor validation.

Recommended title pattern:

`[VALIDATION/EDITOR] <objective> — integrazione, PIE e acceptance finale`

It depends on all implementation issues.

Its scope is NOT to reimplement the feature.

It owns:

1. integration acceptance;
2. regression gate;
3. cross-worker contract validation;
4. Unreal Editor / PIE / visual MCP evidence when applicable;
5. binary Editor validation assets not owned elsewhere;
6. final scenario pass when Editor/PIE is required;
7. issue/roadmap coherence after all children land;
8. minimal integration fixes only.

It must not:
- create a second gameplay authority;
- silently redesign worker implementations;
- duplicate headless tests already owned elsewhere;
- close a worker whose evidence is missing;
- mark an Editor check passed when it was not run.

## Preflight responsibility

The issue may be started before the other workers finish only for:
- reading acceptance criteria;
- preparing the validation matrix;
- checking MCP/editor availability;
- identifying existing reusable scenarios/assets;
- detecting contract collisions.

It must not perform final validation against partial code and call it complete.

## Final gate

After all worker commits/PRs are available:
- integrate in dependency order;
- run required builds;
- run targeted + regression automation;
- run Scenario Harness suites;
- use Unreal/Epic MCP and PIE where applicable;
- capture exact evidence;
- reconcile GitHub issues/docs/roadmap;
- report PASS / BLOCKED with reasons.

---

# Phase 6 — File/write-set collision check

Before generating terminal prompts, build a collision table.

Example:

| Issue | Worker | Owned write set | Shared read-only contracts | Depends on |
|---|---|---|---|---|
| #A | CORE_ARCHITECTURE | `Source/.../Core/*` | `RTTurnLog.h` | — |
| #B | SCENARIOS | `Scenarios/...` | APIs from #A | #A contract |
| #C | AUTOMATION_TESTS | `Source/.../Tests/...` | API contract | #A |
| #D | VALIDATION_EDITOR | Editor assets + integration docs | outputs A/B/C | #A #B #C |

Rules:
- same writable file in two issues => redesign;
- same `.uasset`/`.umap` in two issues => redesign immediately;
- one worker changing an API that another worker must compile against is allowed only with an explicit contract;
- if the contract is not stable enough for parallel work, mark `LIMITED` and serialize those issues through dependencies.

Parallel does NOT mean all issues must start at the exact same second.

A valid graph may be:
- A and B parallel;
- C starts after A;
- VALIDATION_EDITOR preflight parallel, final gate after A+B+C.

---

# Phase 7 — Decide scenario and MCP usage critically

For each issue, explicitly decide:

## Scenario Harness
Use when:
- the behavior crosses multiple gameplay systems;
- ordering/resolution matters;
- determinism/replay/state hash matters;
- a regression is best represented as reusable data;
- the existing project already models that behavior through scenarios.

Do NOT use when:
- a pure function/unit test proves the contract better;
- the scenario would only restate a unit test;
- the format cannot express the required behavior yet;
- it would require bypassing the canonical runtime path.

## Epic/Unreal MCP
Use when:
- Editor authoring is part of the acceptance criteria;
- asset references/wiring must be verified;
- viewport visualization/readability matters;
- PIE behavior matters;
- an existing Editor scenario/session is the canonical validation surface.

Do NOT use when:
- behavior is headless and deterministic;
- validation is pure C++;
- the Editor would add no new evidence.

## GitHub MCP
Use for:
- finding owner issues;
- checking claim/status/dependencies;
- creating/updating work-unit issues;
- linking parent/children/dependencies;
- keeping roadmap issue state coherent.

---

# Phase 8 — Generate terminal prompts

Create one self-contained prompt file per issue.

Write generated prompt files under the ignored working area:

`Saved/Claude/ImplementFeature/<objective-slug>/`

Suggested names:

- `terminal-01-core.md`
- `terminal-02-tests.md`
- `terminal-03-scenarios.md`
- `terminal-04-validation-editor.md`

Do not version these generated prompt files unless the user explicitly asks.

Also print the prompts/paths in the command result.

## Implementation terminal prompt template

Each implementation prompt MUST contain:

```md
# Terminal N — <WORKER_TYPE>

Issue: #<number>
Parent/owner: #<number if applicable>
Objective: <slice>

## Execute

/issue-run <number>

## Parallel contract

You own only issue #<number>.

Do not claim or execute sibling issues:
- #...
- #...

Expected writable area:
- ...

Treat these as read-only shared contracts unless /issue-run proves a necessary change:
- ...

Editor/PIE ownership:
- delegated to #<VALIDATION_EDITOR issue>
- do not launch/change shared Editor binary assets unless the issue definition explicitly proves it is required

## Dependencies

- ...

## Handoff required

When /issue-run finishes, report:
- final issue state;
- branch/worktree;
- commit/PR;
- exact tests run;
- scenario evidence if this issue owns a headless scenario;
- changed files;
- contract changes that affect siblings;
- blocker/follow-up issue numbers.
```

The line `/issue-run <number>` is mandatory.

## VALIDATION_EDITOR terminal prompt template

```md
# Terminal N — VALIDATION_EDITOR

Issue: #<number>
Objective: final integration + single Editor/PIE validation owner

## Execute

/issue-run <number>

## Special role

This is the ONLY Validation + Editor terminal for this /implement-feature run.

Do not duplicate worker implementation.

### Preflight
You may immediately:
- inspect all child issues;
- prepare acceptance matrix;
- inspect reusable scenarios;
- inspect Editor/MCP availability;
- detect contract/write-set conflicts.

### Final validation
Do not declare completion until all required worker outputs are available.

Integrate/validate in dependency order and prove:
- build;
- targeted tests;
- regression tests;
- scenario results;
- replay/determinism/privacy gates when applicable;
- Editor/PIE/MCP evidence when applicable;
- documentation/roadmap/issue coherence.

If a semantic integration conflict exists, set/report BLOCKED rather than silently choosing a design.
```

---

# Phase 9 — Output the orchestration report

The `/implement-feature` command must finish with:

# CRITICAL REVIEW
Summary of the critique and chosen minimum approach.

# ISSUE GRAPH
For every issue:
- number;
- title;
- existing / updated / created;
- worker type;
- dependencies;
- parent/owner.

# PARALLEL WAVES
Example:

`Wave 0: VALIDATION_EDITOR preflight`
`Wave 1: #A + #B + #C`
`Wave 2: #D if it depends on #A`
`Wave 3: VALIDATION_EDITOR final gate`

# COLLISION CHECK
Writable overlaps and how they were removed.

# SCENARIO / MCP DECISIONS
What will use:
- headless automation;
- Scenario Harness;
- GitHub MCP;
- Epic/Unreal MCP;
- PIE;
and why.

# TERMINALS
List the generated prompt files in execution order.

# START NOW
Explicitly state which terminals may start immediately.

---

# Example — `/implement-feature Replay scenario`

This is illustrative only. Discover the live repository before using it.

A healthy decomposition might become:

- existing replay/scenario owner issue updated;
- issue A — scenario format/fixture or replay-source contract;
- issue B — deterministic replay/scenario automation;
- issue C — scenario corpus/golden evidence if independently writable;
- issue D — `VALIDATION_EDITOR`, depending on A/B/C.

Possible waves:

`Wave 1: A + B`
`Wave 2: C after contract from A, if needed`
`Wave 3: D final`

But if live evidence shows an existing issue already owns the complete replay scenario and splitting it would create overlapping edits, DO NOT force this example. Use the smallest safe graph.

---

# Anti-patterns — reject these

## Three terminals on one issue

Wrong:

`Terminal 1 -> /issue-run 123`
`Terminal 2 -> /issue-run 123`
`Terminal 3 -> /issue-run 123`

This violates exclusive claim ownership.

## Separate Validation and Editor

Wrong:
- Terminal 4 Validation
- Terminal 5 Editor

Use one `VALIDATION_EDITOR`.

## Create issue for every technical layer

Wrong when all layers touch the same files and API repeatedly.

Parallelism must reduce elapsed work without multiplying integration risk.

## Scenario by default

A scenario is evidence, not ceremony.

## Editor as second simulator

Never reproduce resolver/pathfinding/LOS/targeting logic in Editor code to make validation easier.

## Final validation only by visual inspection

PIE/Editor evidence complements automation. It does not replace deterministic/headless gates.

## Plan before evidence

Never output terminal assignments before the Critical Review Gate.
