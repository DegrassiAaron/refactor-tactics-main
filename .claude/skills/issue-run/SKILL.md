---
name: issue-run
description: Executes one GitHub issue end to end. Consumes the existing Behavior Contract and issue definition, performs only role-owned work, records evidence, and hands off remaining phases through RT3.
argument-hint: "[issue-number]"
arguments:
  - issue
disable-model-invocation: true
---

# RefactorTactics — Issue Run

Issue:

`$issue`

Invocation:

`/issue-run $issue`

This skill executes:

> one issue, as far as this session can take it

It does NOT mean:

> close every issue it touches

An issue is done when it is merged and its DoD is verified, not when this skill returns.

---

# 0 — Scope gate

⚠️ **Execution roles no longer exist.** Until 2026-09-08 a session declared one of `DEV`,
`EDITOR` or `VALIDATION` in `RT_TERMINAL_ROLE`, a task router named the `next_actor`, and this
skill refused to run when the two disagreed. Roles, router and the `scripts/` guards were all
removed (`D-346`, `D-347`). If `RT_TERMINAL_*` or `RT_TASK_ID` are set in the environment, they
are leftovers from a window opened earlier: nothing reads them.

What the gate protected is still real, and is now yours to hold:

- **one issue at a time.** Do not widen to sibling issues because they look adjacent;
- **the machine has one Unreal.** Before Editor, PIE, build or a suite, check that no other
  session is using it — `Get-Process UnrealEditor*, UnrealEditor-Cmd*`;
- **asset authoring belongs to the main clone.** A worktree lacks gitignored files, so hard
  references read `None` and saving **zeroes them** — silently;
- **whoever writes a fix does not sign off on it alone.** Fix and measurement are two moments:
  correct, name the commit, then measure that commit.

---

# 2 — Preflight

Normalize `$issue`.

Read the complete live issue:

- title;
- body;
- comments;
- status;
- labels;
- assignee/claim state;
- parent/Epic;
- dependencies;
- linked PRs;
- Acceptance Criteria;
- Behavior Contract if present;
- existing execution/evidence reports.

Measure:

```text
repository root
branch
HEAD
worktree
git status
origin/main
```

Do not overwrite unrelated local changes.

If a clean isolated worktree is required by the current role/tooling, use the project convention.

---

# 3 — Claim rule

Issue claim governance must follow the accepted live project rule.

If the project still has an unresolved governance decision equivalent to #2633, preserve the existing fail-closed behavior.

Until a canonical decision supersedes it:

| Situation | Action |
|---|---|
| no claim and this is the first authorized actor | claim according to repository convention |
| existing claim by another session | do not steal or rewrite the claim; add your evidence to the issue |
| ambiguous case | `BLOCKED — CLAIM_DECISION_REQUIRED` |

Do NOT create separate issues per kind of work to bypass the claim problem.

Do NOT invent whether claim belongs to session or task.

When a Decision Log/ADR/project owner resolves this, follow that source and remove obsolete workaround behavior.

---

# 4 — Behavior Contract is input

If the issue contains or references a Behavior Contract:

```text
Behavior Contract = INPUT
```

Do not silently change:

- behavior semantics;
- player outcome;
- Acceptance Criteria;
- failure behavior;
- design decisions.

If implementation reality conflicts:

```text
BEHAVIOR_CONTRACT_CONFLICT
```

Record exact conflict, observed evidence, affected AC and smallest decision needed.

Stop the conflicting part and return it to `/feature-behavior` / planning owner.

---

# 5 — Technical definition

`sc:spec-panel` refines implementation.

It does NOT own behavior design.

Run it when:

- the technical DNNN definition is absent/incomplete;
- this role is authorized to define the technical work;
- repository workflow requires it.

Do NOT rerun/rewrite the technical definition merely because the issue moved between sessions or VALIDATION.

If an accepted D001-D010 definition already exists, consume it.

Technical definition may cover:

```text
D001 Scope
D002 Authority / behavior mapping
D003 Data / APIs / state
D004 Dependencies
D005 Expected write-set
D006 Failure / validation rules
D007 Automated tests
D008 Scenario / Editor evidence
D009 Docs / related issues
D010 Definition of Done
```

If `sc:spec-panel` contradicts the Behavior Contract:

```text
BEHAVIOR_CONTRACT_CONFLICT
```

Do not let technical specification silently override design.

---

# 6 — Current-phase work only

Determine exactly what this phase must accomplish.

Record:

```text
CURRENT ROLE
CURRENT PHASE OBJECTIVE
OWNED WRITE-SET
READ-ONLY CONTRACTS
ACCEPTANCE IDS
EXPECTED EVIDENCE
NOT RUN GATES
```

Do not perform work merely because it exists somewhere in the issue.

Perform only work belonging to the current role/phase.

---

# 7 — DEV phase

DEV owns, when in scope:

- C++;
- implementation source;
- automated tests;
- Scenario Harness data/files;
- validators;
- scripts/tooling;
- textual schemas/catalogs;
- documentation;
- Git/GitHub updates.

DEV does NOT:

- open Unreal Editor;
- start PIE;
- author `.uasset/.umap`;
- use MCP asset write;
- run machine-wide Unreal build/suite when live policy assigns that resource to VALIDATION.

Before implementing:

1. reproduce or demonstrate the gap;
2. identify a red test / falsifying baseline when possible;
3. map work to Acceptance Criteria;
4. implement through the canonical runtime path;
5. avoid duplicate resolver/pathfinder/replay authority;
6. preserve determinism/privacy/versioning rules.

Scenario creation by DEV is valid when it is textual Scenario Harness authoring and belongs to the issue.

Do not create a scenario merely because one was proposed; reconcile against the scenario plan first.

---

# 8 — EDITOR phase

EDITOR owns:

- `.uasset/.umap`;
- Blueprint;
- UMG;
- Materials;
- visual authoring;
- asset wiring;
- PIE;
- persistence verification;
- human/mixed visual acceptance.

Opening an EDITOR terminal does NOT automatically acquire Unreal.

Take the machine resource only just-in-time, and verify by hand that nobody else holds it: the lease that enforced this was removed.

Asset writes:

```text
one writer
one authorized write-set
one active Unreal operation
```

If MCP asset authoring is restricted to `MAIN`, verify workspace identity before writing.

Do not invent a translation for workspace taxonomy conflicts.

If current workspace cannot legally author:

```text
BLOCKED — MCP_ASSET_WRITE_WORKSPACE
```

Do not bypass the rule.

For every visual check record:

```text
ACCEPTANCE ID
MAP / SCENARIO / FIXTURE
SETUP
EXPECTED
FALSIFICATION CRITERION
OBSERVED
ORACLE: HUMAN | MIXED
EVIDENCE
RESULT
```

Possible Editor results:

```text
PASS
FAIL
BLOCKED
NOT RUN
N/A
OBSERVED
INVALIDATED
```

`OBSERVED` is not PASS for systems Editor cannot prove.

Editor cannot certify by itself byte determinism, payload privacy, replay equivalence, packaged runtime, dedicated server, network authority or shipping performance.

---

# 9 — VALIDATION phase

VALIDATION is independent.

It may:

- review diff;
- build;
- run Automation;
- run Scenario Harness;
- run mutation/anti-vacuity checks;
- run determinism/replay/privacy/network checks;
- run aggregate suite when required;
- run packaged/multiclient/performance gates when required.

It MUST NOT:

```text
find defect
→ modify product
→ rerun
→ approve own fix
```

Instead:

```text
find defect
→ preserve evidence
→ finding
→ DEV or EDITOR fixes
→ new candidate
→ VALIDATION reruns
```

Validation measures an immutable candidate.

Record:

```text
CANDIDATE_SHA
HEAD BEFORE
HEAD AFTER
DIRTY STATE BEFORE
DIRTY STATE AFTER
BINARIES / BUILD ID
COMMAND
FOUND
PERFORMED
PASSED
FAILED
EXIT CODE
```

If source, binary, fixture, relevant config or candidate SHA changes during the measure:

```text
INVALIDATED
```

`performed = 0` is never successful validation.

---

# 10 — Scenario execution

Scenario Harness execution normally belongs to VALIDATION.

Before running a scenario:

- confirm ScenarioId;
- confirm fixture;
- confirm canonical gameplay path;
- identify supported AC;
- identify machine oracle.

Do not treat scenario PASS as proof of unrelated visual/readability ACs.

For mixed evidence:

```text
Scenario machine assertion → logical state
PIE/Human                  → readability
```

Keep results separate.

---

# 11 — Evidence states

Use only factual states:

```text
PASS
FAIL
BLOCKED
NOT RUN
N/A
OBSERVED
INVALIDATED
```

Never infer PASS from compilation only, MCP `success`, command sent, no visible error, a closed issue or an unrelated scenario.

---

# 12 — Candidate changes

If DEV or EDITOR changes source, config, scenario fixture, binary asset or relevant test data, produce a new candidate identity/SHA according to project convention.

Mark impacted previous evidence:

```text
INVALIDATED
```

Do not combine incompatible candidate evidence into one sign-off.

---

# 13 — Coherence scan

Before handoff, reconcile:

```text
Behavior Contract
Acceptance Criteria
technical DNNN
implementation
tests
scenarios
docs
roadmap references
related issues
TurnLog/replay
privacy/network notes
```

Apply:

```text
SEARCH → REUSE → UPDATE → CREATE
```

for follow-up issues.

A finding does not automatically require a new issue if an existing owner can absorb it.

---

# 14 — Reporting

Report on the **issue itself** — a comment, or the PR body. The task router that collected these
reports was removed with the roles (`D-347`), and a result that lives only in a chat does not
exist for whoever comes next.

State at minimum: status (`DONE` / `PARTIAL` / `BLOCKED` / `FAILED`), what changed, the evidence
with its commit, and what was **NOT RUN**.

`NextActorRecommended` is a recommendation.

The Coordinator decides routing.

Do not assign the next actor yourself from a worker role.

---

# 15 — Issue progress

Completing one phase does NOT automatically close the GitHub issue.

The issue may still require another role, human acceptance, packaged, merge, final validation or a decision.

Update the issue with a factual phase report.

Do not mark the whole issue `Done` unless its live DoD is actually complete.

---

# 16 — Cleanup

Every role cleans up only resources it owns.

DEV:

- no Unreal process should have been opened;
- leave unrelated local work untouched.

EDITOR:

- stop PIE;
- save only intentional assets;
- verify dirty packages;
- release Unreal resource;
- close Editor only if this workflow owns that lifecycle.

VALIDATION:

- release validation/Unreal resource;
- preserve logs/evidence;
- do not leave mutated source from validation probes;
- verify working tree/candidate integrity.

Never close a user-owned or another-session Editor.

---

# 17 — Final output

Always finish with:

```markdown
# ISSUE RUN REPORT

## ISSUE
#...

## TASK
...

## CANDIDATE
...

## ACCEPTANCE IDS
...

## WORK PERFORMED

## FILES / ASSETS CHANGED

## EVIDENCE

| AC | Evidence | Result |
|---|---|---|

## PASS

## FAIL

## BLOCKED

## NOT RUN

## INVALIDATED

## FINDINGS

## RELATED ISSUE UPDATES

## HANDOFF

Recommended next actor:
Reason:

## FINAL PHASE STATUS

DONE
PARTIAL
BLOCKED
FAILED
```

`DONE` here means this phase is done unless the report explicitly states that the full GitHub Issue DoD is also satisfied.
