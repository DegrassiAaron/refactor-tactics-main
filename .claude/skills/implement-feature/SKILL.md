---
name: implement-feature
description: Reconciles a RefactorTactics Behavior Contract or objective with live repository/GitHub ownership, Epics, Issues, roadmap and verification evidence. Produces a safe execution plan by default; mutations require explicit --apply. Never implements gameplay/code/assets itself.
argument-hint: "[--apply] <Behavior Contract, feature or objective>"
disable-model-invocation: true
---

# RefactorTactics — Implement Feature

Input:

`$ARGUMENTS`

This skill owns the **planning and ownership reconciliation layer**.

Its question is:

> WHERE does this behavior belong in the existing project, WHAT work is genuinely missing, and HOW should that work be executed and verified?

It MUST NOT implement the feature itself.

Normal flow:

```text
/feature-behavior
      ↓
Behavior Contract
      ↓
/implement-feature
      ↓
PLAN
      ↓
review
      ↓
/implement-feature --apply
      ↓
RT Coordinator
      ↓
/issue-run
```

---

# 0 — Mode

Default mode is `PLAN` and is read-only.

Mutation mode requires explicit:

```text
--apply
```

If `--apply` is absent:

```text
ZERO MUTATIONS
```

No issue/Epic/roadmap/task-router write is allowed in PLAN.

---

# 1 — Input contract

If a `BEHAVIOR_READY` contract from `/feature-behavior` exists, use it as authoritative input for:

- intent;
- behavior semantics;
- acceptance criteria;
- failure conditions;
- behavior-level open decisions.

Do not silently redesign it.

If repository evidence contradicts it:

```text
BEHAVIOR_CONTRACT_CONFLICT
```

Return the conflict to `/feature-behavior` / the decision owner.

If substantial gameplay semantics are still undefined, return:

```text
BEHAVIOR_DEFINITION_REQUIRED
NEXT: /feature-behavior
```

---

# 2 — Non-negotiable principles

## P1 — Search before create

Always:

```text
SEARCH → REUSE → UPDATE → CREATE
```

Search live GitHub, open/closed issues, parent Epics, Decision Log/ADR, roadmap, specs, code, tests, Scenario Harness, PIE/manual-test registry and recent PRs.

Primary question:

> Who already owns this outcome?

Not:

> Can I invent a cleaner new hierarchy?

## P2 — Feature is not Epic

A feature may cross several existing owners.

Allowed Epic reconciliation:

```text
USE_EXISTING
UPDATE_EXISTING
NO_EPIC_REQUIRED
EPIC_CANDIDATE
```

A new Epic is never auto-created just for roadmap symmetry, one residual issue, a new name over an existing capability, role separation or cross-domain collection.

`EPIC_CANDIDATE` requires explicit approval before creation.

## P3 — Role is not Issue ownership

Execution roles are responsibilities:

```text
DEV
EDITOR
VALIDATION
```

They are NOT automatically separate issues.

Do NOT create:

```text
Issue A — DEV
Issue B — EDITOR
Issue C — VALIDATION
```

just because one outcome passes through several phases.

Create a separate issue only when it is genuinely independently closable with distinct outcome, owner, DoD and separable work/dependency.

Allowed issue reconciliation:

```text
NO_WORK
REUSE
UPDATE
CREATE
BLOCKED_DECISION
```

## P4 — Acceptance owns evidence

Use:

```text
Acceptance Criterion → Evidence
```

not:

```text
Issue → Scenario
```

## P5 — Plan and Apply are separate

PLAN is an ephemeral derived snapshot, not a source of truth.

Every plan records:

```yaml
package_id:
generated_at:
base_sha:
canonical_sources:
issues_measured:
owners_measured:
decisions_measured:
```

Before APPLY, remeasure live state.

If assumptions changed materially:

```text
STALE_PLAN
ZERO MUTATIONS
```

---

# 3 — Current execution model

Use the accepted live repository model.

Conceptually:

```text
DEV pool 1..N
      │
      ▼
 machine-wide Unreal resource
      │
   ┌──┴───────┐
 EDITOR   VALIDATION
```

Rules:

- three logical roles do NOT mean exactly three fixed terminal windows;
- DEV parallelism only exists when work is really separable;
- Unreal is an exclusive machine resource;
- opening a terminal does not acquire Unreal;
- `.uasset/.umap` have one writer;
- MAIN hosts the machine MCP bridge when current accepted policy says so;
- RT Coordinator routes work but is not a fourth implementation role.

Do not resolve workspace taxonomy conflicts by inference.

---

# 4 — Governance conflicts

Before planning, check current status of live governance owners such as:

```text
#2633 — issue claim across the three execution roles
```

⚠️ `#2647` (RT3 control-plane/workspace taxonomy) described a defect of a system that no
longer exists: the control plane was removed. Do not treat it as a live constraint.

If still open:

- do not invent a solution;
- do not create separate issues per role to bypass #2633;
- preserve current fail-closed behavior.

If closed/superseded by an accepted decision, use that new canonical source.

---

# 5 — Phase 0: snapshot

Measure:

```text
REPOSITORY ROOT
HEAD
ORIGIN/MAIN
BRANCH
WORKTREE
DIRTY STATE
RECENT RELEVANT PRs
```

Read when relevant:

- `AGENTS.md`;
- `CLAUDE.md`;
- Decision Log / ADR;
- roadmap/checkpoint;
- Behavior Contract;
- scenario-map;
- Scenario Index docs;
- PIE/manual-test registry;
- affected specs/code/tests.

Never trust dated counts without remeasuring them.

---

# 6 — Phase 1: owner reconnaissance

Search GitHub open AND closed.

For every relevant object classify:

```text
PRIMARY_OWNER
SUPPORTING_OWNER
DELIVERED_EVIDENCE
DEPENDENCY
OVERLAP
HISTORICAL_ONLY
NOT_RELEVANT
```

Find the owner for each Behavior Contract / AC.

A cross-domain feature may legitimately map to several owners.

Do not force one Epic to own everything.

---

# 7 — Phase 2: Critical Review Gate

Before mutations, report:

## INTENTO
## FATTI
## INFERENZE
## PROBLEMI / AMBIGUITÀ
## FAILURE MODES
## ALTERNATIVA MINIMA

Check at least:

- duplicate ownership;
- fake parallelism;
- shared central write-set;
- binary asset conflict;
- duplicate runtime/editor authority;
- duplicate scenario coverage;
- vacuous tests;
- phantom future scenarios;
- privacy side channel;
- replay/StateHash drift;
- nondeterministic order;
- evidence that cannot falsify the AC;
- feature package becoming a second roadmap.

Parallelization verdict:

```text
SAFE
SAFE_WITH_CONTRACT
LIMITED
BLOCKED
```

If blocked by a decision:

```text
BLOCKED_DECISION
```

Do not fabricate work below it.

---

# 8 — Phase 3: Epic reconciliation

For every relevant Epic:

```text
USE_EXISTING
UPDATE_EXISTING
NO_EPIC_REQUIRED
EPIC_CANDIDATE
```

For `EPIC_CANDIDATE`, explain missing ownership, boundary, outcome, non-goals, initial real children, closure gate, release placement and why existing owners are insufficient.

Do NOT create it during PLAN.

Do NOT create it during APPLY without explicit approval.

---

# 9 — Phase 4: Issue reconciliation

For each independently closable outcome:

```text
NO_WORK
REUSE
UPDATE
CREATE
BLOCKED_DECISION
```

A `CREATE` candidate requires:

```text
measured gap
+ no adequate owner
+ independent outcome
+ falsifiable AC
+ real DoD
```

These are NOT enough:

```text
different execution role
different test method
different terminal
different phase
```

One issue may flow sequentially through multiple execution roles when live routing/claim rules permit it.

---

# 10 — Phase 5: Verification reconciliation

For every Acceptance Criterion decide evidence using actions such as:

```text
REUSE
EXTEND
CREATE_CANDIDATE
DEFER
NOT_APPLICABLE
```

Scenario policy:

- `REUSE`: existing fixture + canonical path + oracle genuinely cover the AC;
- `EXTEND`: same conceptual scenario, added assertion remains coherent;
- `CREATE_CANDIDATE`: capability exists and a reusable Scenario Harness regression is justified;
- `DEFER`: tested capability itself does not exist yet;
- `NOT_APPLICABLE`: another evidence type is better.

Do NOT create scenario files in this skill.

Not every gameplay issue requires Scenario Harness or PIE.

---

# 11 — Phase 6: dependency and resource graph

Do not represent every relationship as a hard DAG edge.

Use:

```text
HARD_DEPENDENCY
SOFT_ORDER
VALIDATION_DEPENDENCY
DECISION
RESOURCE_CONFLICT
RELATED
```

Example:

```yaml
- type: HARD_DEPENDENCY
  from: issue-B
  to: issue-A

- type: RESOURCE_CONFLICT
  resource: UNREAL_MACHINE
  max_parallel: 1
  issues: [issue-C, issue-D]
```

Two tasks needing Unreal are not automatically logically dependent.

---

# 12 — Phase 7: write-set / parallelism

For each proposed work unit record:

```text
OWNER
EXPECTED WRITE-SET
SHARED READ-ONLY CONTRACTS
DEPENDENCIES
ROLE(S) NEEDED
```

If two work units write the same central file/contract:

- serialize them;
- merge them into one owner;
- or first create a real seam if that seam has independent value.

"We will resolve conflicts later" is not a parallelization strategy.

---

# 13 — Phase 8: role routing plan

Determine actual phase sequence per issue.

Examples:

```text
DEV → VALIDATION
EDITOR → VALIDATION
DEV → VALIDATION → EDITOR → VALIDATION
EDITOR → USER
VALIDATION → DEV → VALIDATION
```

There is no mandatory universal sequence.

The current actor is decided from actual work remaining, dependencies, routing state and resource availability.

`/issue-run` performs the assigned current phase.

---

# 14 — PLAN output

Without `--apply`, finish with:

```markdown
# IMPLEMENT FEATURE PLAN

## SNAPSHOT

- Package:
- Generated:
- Base SHA:
- Canonical sources:

## BEHAVIOR INPUT

## CRITICAL REVIEW

## OWNERSHIP

| Contract/AC | Existing owner | Status |
|---|---|---|

## EPIC RECONCILIATION

| Epic | Action | Reason |
|---|---|---|

## ISSUE RECONCILIATION

| Outcome | Issue | Action | Reason |
|---|---|---|---|

## ACCEPTANCE → EVIDENCE

| AC | Evidence | Action | Oracle |
|---|---|---|---|

## SCENARIO / PIE PLAN

## DEPENDENCIES

## RESOURCE CONSTRAINTS

## PARALLELIZATION

## PROPOSED MUTATIONS

## EXECUTION PLAN

| Issue | First/current actor | Later gates |
|---|---|---|

## DECISIONS REQUIRED

## VERDICT

PLAN_READY
BLOCKED_DECISION
BEHAVIOR_CONTRACT_CONFLICT
NO_WORK
```

No mutations.

---

# 15 — APPLY preflight

When invoked with `--apply`, re-run discovery.

Compare:

```text
base SHA
issue states
Epic ownership
Decision Log / ADR
roadmap
recent PRs
duplicates
Behavior Contract
```

If material assumptions differ:

```text
STALE_PLAN
ZERO MUTATIONS
```

Do not partially apply a stale ownership graph.

---

# 16 — APPLY mutations

Only after successful preflight.

Allowed planning mutations:

- update existing owner issue;
- create genuine gap issue;
- update parent/child/back-references;
- update applicable roadmap/checkpoint source;
- create task/routing assignment when current coordinator/tooling contract allows it.

Not allowed:

- gameplay implementation;
- runtime source changes;
- tests implementation;
- scenario file implementation;
- Unreal asset authoring;
- arbitrary refactor;
- automatic creation of an unapproved Epic.

After every mutation, reread the object and verify no duplicate was introduced.

---

# 17 — Success metric

Do not optimize for created objects.

Report owners reused, existing issues updated, duplicates avoided, genuine gaps, ACs with falsifiable evidence, scenarios reused and justified new candidates.

A perfect result may be:

```text
0 Epic created
0 Issues created
0 scenarios created
```

---

# 18 — Final APPLY report

```markdown
# APPLY RESULT

## BASELINE REVALIDATED

## REUSED

## UPDATED

## CREATED

## NOT CREATED — DUPLICATE / UNNECESSARY

## ROADMAP / DOC MUTATIONS

## ROUTING CREATED / UPDATED

## OPEN DECISIONS

## NEXT READY WORK

## NOT RUN

## VERDICT

APPLIED
STALE_PLAN
PARTIAL
BLOCKED_DECISION
NO_WORK
```

Never say `APPLIED` when preflight was stale.
