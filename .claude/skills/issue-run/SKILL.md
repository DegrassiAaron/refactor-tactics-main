---
name: issue-run
description: Executes one GitHub issue as far as a single session can take it. Consumes the existing Behavior Contract and issue definition, does only the work the current step needs, records evidence with the commit it was measured on, and states plainly what was left NOT RUN.
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

Do NOT rerun/rewrite the technical definition merely because the issue moved between sessions.

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
CURRENT STEP OBJECTIVE
OWNED WRITE-SET
READ-ONLY CONTRACTS
ACCEPTANCE IDS
EXPECTED EVIDENCE
NOT RUN GATES
```

Do not perform work merely because it exists somewhere in the issue.

The three headings below are **kinds of work with different technical constraints**, not roles
assigned to terminals: one session may cross all three. What does not change is that a gate you
did not run is `NOT RUN`, never `PASS`.

---

# 7 — Code and tooling work

This kind of work does not need Unreal Editor. It covers, when in scope:

- C++;
- implementation source;
- automated tests;
- Scenario Harness data/files;
- validators;
- scripts/tooling;
- textual schemas/catalogs;
- documentation;
- Git/GitHub updates.

From here you cannot claim what only the Editor can show: `PIE`, visual evidence and asset
integration are not observable without opening it. Claiming them is the one failure this section
exists to prevent.

Unreal itself — build or suite — is a **machine** resource, not a session one: before starting
one, check by hand that no other session is running it. Two measurements in the same window
invalidate each other, and nothing stops them any more (`D-347`).

Before implementing:

1. reproduce or demonstrate the gap;
2. identify a red test / falsifying baseline when possible;
3. map work to Acceptance Criteria;
4. implement through the canonical runtime path;
5. avoid duplicate resolver/pathfinder/replay authority;
6. preserve determinism/privacy/versioning rules.

Creating a scenario is code-and-tooling work when it is textual Scenario Harness authoring and belongs to the issue.

Do not create a scenario merely because one was proposed; reconcile against the scenario plan first.

---

# 8 — Work that requires Unreal Editor

This kind of work needs the Editor open:

- `.uasset/.umap`;
- Blueprint;
- UMG;
- Materials;
- visual authoring;
- asset wiring;
- PIE;
- persistence verification;
- human/mixed visual acceptance.

Opening a terminal does NOT acquire Unreal.

Take the machine resource only just-in-time, and verify by hand that nobody else holds it: the lease that enforced this was removed.

⚠️ **PIE sessions and MCP asset authoring want the main clone.** A worktree does not carry the
gitignored files: without the packs, hard references read `None`, and **saving zeroes them**. This
is not a protocol rule — it is a property of the checkout, and it survived the removal.

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

# 9 — Independent verification

Verification is independent of whoever wrote the code — that is what the word means.

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
→ fix it
→ new candidate, commit declared
→ measure again on that commit
```

Verification measures an immutable candidate.

⚠️ The same session may do both steps — no role forbids it any more. What it must not do is
collapse them: fixing and measuring are **two moments**, and if one session did both, say so
next to the result. Whoever wrote the fix knows the case they had in mind, not the one they broke.

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

Running the Scenario Harness is a measurement: it counts as evidence under the independence rule of §9.

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

If source, config, scenario fixture, binary asset or relevant test data change, produce a new candidate identity/SHA according to project convention — an earlier measurement does not carry over to it.

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

If the issue needs work this session cannot do — an Editor seance, a packaged build, a
measurement someone else must take independently — name it in the report as remaining work.
Nothing routes it automatically any more: the Coordinator and the task router were removed
with the roles (`D-347`), so an unstated remainder is simply lost.

---

# 15 — Issue progress

Completing one phase does NOT automatically close the GitHub issue.

The issue may still require another role, human acceptance, packaged, merge, final validation or a decision.

Update the issue with a factual phase report.

Do not mark the whole issue `Done` unless its live DoD is actually complete.

---

# 16 — Cleanup

Clean up only what this session opened.

After code and tooling work:

- no Unreal process should have been left running;
- leave unrelated local work untouched.

After work in the Editor:

- stop PIE;
- save only intentional assets;
- verify dirty packages;
- close the Editor only if this session opened it.

After a verification run:

- preserve logs/evidence;
- do not leave mutated source from probes — a mutation gate that stops early leaves the
  **binary** mutated too, which `git checkout --` does not undo: rebuild before any other
  measurement;
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
