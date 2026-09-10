---
name: worktree-issue-runner
description: Executes one or more GitHub issues from the directory it is launched in, planning how many independent flows the work really supports, reserving a single EDITOR flow when the engine is genuinely required, and concentrating build and test into one shared validation queue. Records evidence with the commit it was measured on and states plainly what was left NOT RUN.
argument-hint: "<issue...> [flows=N] [editor=auto|yes|no]"
disable-model-invocation: true
---

# RefactorTactics — Worktree Issue Runner

Arguments:

`$ARGUMENTS`

This skill executes:

> one or more issues, as far as this session can take them

It does NOT mean:

> close every issue it touches

An issue is done when it is merged and its DoD is verified, not when this skill returns.

---

# 0 — Canonical position, and the cycle you must not create

This file **is** the workflow. Everything else is an entry point into it.

```text
/worktree-issue-runner  --+
                          +-->  this file  -->  execution
/issue-run  --------------+
```

`/issue-run` is a compatibility entry point: it carries its arguments here verbatim and adds nothing.
Both names accept the same syntax and produce the same run.

⛔ **From inside this file, do not invoke `/issue-run`, `/worktree-issue-runner`, or the
`worktree-issue-runner` skill.** You are already inside the workflow; re-entering it is a loop, not a
delegation. The composition happens once, at the entry point, and never again.

**If `/issue-run` is not defined in this environment** — a checkout without the adapter, a personal skill
directory that was never installed — nothing is lost: `/worktree-issue-runner` is the direct entry point
and needs no adapter. The reverse also holds: if only `/issue-run` exists, it reaches this file by reading
it, and the run is identical.

---

# 1 — Arguments

Parse `$ARGUMENTS` into exactly three things, then **echo what you parsed before doing anything else**.

| Form | Meaning |
|---|---|
| `123`, `#123` | issue reference |
| `OWNER/REPO#123` | issue reference in another repository |
| `https://github.com/OWNER/REPO/issues/123` | issue reference |
| `flows=N` | maximum total number of flows, `N` a positive integer |
| `editor=auto` / `editor=yes` / `editor=no` | Editor policy |

Defaults:

```text
flows  = 1
editor = auto
```

Rules:

1. **`flows` comes only from `flows=N`.** A bare number is never a flow count.
2. **`editor` comes only from `editor=`.** A bare word is never an Editor policy.
3. `flows=N` and `editor=V` may appear in any position; order does not change meaning.
4. ⛔ **A bare integer that appears after a `key=value` token is not silently promoted to an issue.**
   `#123 flows=4 3` is malformed, not "three issues": stop with
   `BLOCKED — AMBIGUOUS_ARGUMENT`, show the parse, and ask which of the two the trailing number was.
   Adding an issue nobody asked for costs a claim on somebody else's work.
5. An unrecognised token is never guessed. Same treatment: `BLOCKED — AMBIGUOUS_ARGUMENT`.
6. Zero issue references is not a run: ask for one.

Echo, always, before the first read of the first issue:

```text
ISSUES     : #... , #...
FLOWS      : requested N (default 1 when absent)
EDITOR     : auto | yes | no
UNPARSED   : (must be empty)
```

---

# 2 — Where you are, measured

The directory this skill was launched from is the authoritative context. Derive everything from Git,
nothing from the folder's name.

```text
git rev-parse --show-toplevel
git rev-parse --absolute-git-dir
git rev-parse --path-format=absolute --git-common-dir
git rev-parse --abbrev-ref HEAD
git rev-parse HEAD
git status --porcelain
git fetch --quiet origin && git rev-parse origin/main
```

🔑 **`--git-common-dir` is what tells worktrees from clones**, and the two behave differently:

| `--git-common-dir` | What it is | Consequence |
|---|---|---|
| equal to this checkout's `.git` | a **clone** of its own | objects are not shared; nothing you commit is visible to the others without a fetch |
| a **different** path | a **worktree** of that repository | objects are shared; `HEAD` and the working tree are not |

Neither of them isolates machine resources. Unreal, Live Coding, DDC, CPU and disk stay single.

This skill MUST NOT:

- switch worktree or branch to obtain a privilege it does not have where it stands;
- infer a role, an authority or an Editor entitlement from the directory's name;
- create a worktree that was not asked for;
- embed an absolute path;
- assume the clone that owns the Editor is called `MAIN`, `EDITOR`, or anything else.

Local repository policy wins over this file wherever the two disagree. Read `AGENTS.md` and `CLAUDE.md`
from the repository root you just measured, not from memory.

⚠️ **Execution roles no longer exist.** Until 2026-09-08 a session declared one of `DEV`, `EDITOR` or
`VALIDATION` in `RT_TERMINAL_ROLE` and scripts enforced it. Roles, router and the `scripts/` guards were
removed (`D-346`, `D-347`). If `RT_TERMINAL_*` or `RT_TASK_ID` are set, they are leftovers from a window
opened earlier: nothing reads them. What the guards protected is still real, and is now yours to hold.

---

# 3 — Issue intake

For each parsed issue, read the complete live issue:

- title, body, comments;
- status, labels, assignee/claim state;
- parent/Epic, dependencies, linked PRs;
- Acceptance Criteria;
- Behavior Contract if present;
- existing execution/evidence reports.

Then build the **issue matrix**, which is the shared state for the whole run:

| Issue | Needs | Depends on | Write-set | AC ids | Flow | Editor requests | Validation | Status |
|---|---|---|---|---|---|---|---|---|

`Needs` is one or more of: `CODE`, `DOCS`, `DATA`, `EDITOR`, `VERIFY`.

## Claim

Issue claim governance follows the accepted live project rule.

| Situation | Action |
|---|---|
| no claim and this is the first authorized actor | claim according to repository convention |
| existing claim by another session | do not steal or rewrite the claim; add your evidence to the issue |
| ambiguous case | `BLOCKED — CLAIM_DECISION_REQUIRED` |

With more than one issue in the batch, claim them **one at a time, in the order you will work them**, and
re-read each issue immediately after claiming it. If ownership no longer belongs to this run, drop that
issue from the batch and say so — that re-read is the race guard, and a batch multiplies the races.

Do NOT create separate issues per kind of work to bypass the claim problem. Do NOT invent whether the
claim belongs to a session or to a task.

## Behavior Contract is input

If an issue contains or references a Behavior Contract:

```text
Behavior Contract = INPUT
```

Do not silently change behavior semantics, player outcome, Acceptance Criteria, failure behavior or
design decisions. On conflict emit `BEHAVIOR_CONTRACT_CONFLICT` with the exact conflict, the observed
evidence, the affected AC and the smallest decision needed, and return that part to `/feature-behavior`.

## Technical definition

`sc:spec-panel` refines implementation. It does NOT own behavior design.

Run it when the technical `DNNN` definition is absent or incomplete and this run is authorized to define
the technical work. If an accepted `D001`-`D010` definition already exists, consume it — do not rewrite a
definition merely because the issue moved between sessions.

```text
D001 Scope                    D006 Failure / validation rules
D002 Authority / behavior     D007 Automated tests
D003 Data / APIs / state      D008 Scenario / Editor evidence
D004 Dependencies             D009 Docs / related issues
D005 Expected write-set       D010 Definition of Done
```

---

# 4 — Flow planning

A **flow** is an independent line of work with its own write-set. Flows are a way to avoid waiting, not a
target to hit.

## Does this batch need the Editor?

Answer `EDITOR: yes` when at least one issue requires `.uasset`/`.umap`, Blueprint, UMG, Materials, visual
authoring, asset wiring, PIE, a screenshot, persistence verification, or any state observable only inside
the engine.

The `editor=` argument overrides the discovery:

| `editor=` | Effect |
|---|---|
| `auto` | decide from the issues, as above |
| `yes` | reserve the EDITOR flow even if discovery said no |
| `no` | reserve none; every Editor-dependent acceptance in this batch is `NOT RUN`, with the reason |

⛔ **`editor=no` never converts an Editor-only check into a `PASS` obtained some other way.** It removes
the flow, not the requirement.

## How many flows

```text
independent = number of issue groups with disjoint write-sets and no dependency between them
effective   = min(flows, independent)
              of which exactly 1 is the EDITOR flow, when the Editor is needed
```

- the EDITOR flow **counts inside** `flows`. `flows=4` with Editor work means at most **1 EDITOR + 3
  non-EDITOR**; without Editor work, up to **4 non-EDITOR**;
- there is never more than **one** EDITOR flow, whatever `flows` says;
- **do not create more flows than there is independent work.** Two issues that touch the same files are
  one flow. `flows=4` on a single issue is one flow, and saying so is the correct answer;
- with `flows=1` and Editor work needed, the single flow **is** the EDITOR flow: it does its code work
  first, then opens the engine once for the Editor batch.

Echo the plan before starting:

```text
FLOWS REQUESTED : N
FLOWS EFFECTIVE : M   (reason, when M < N)
EDITOR FLOW     : yes | no
FLOW 1 : issues ... — write-set ...
FLOW 2 : ...
```

## Coordinator

Whichever flow you are executing, one role is not distributed: the **coordinator** owns the issue matrix,
the Editor request queue and the validation queue. Only the coordinator starts an expensive operation.

---

# 5 — Flow charter

Every flow, EDITOR included:

- works only inside its own write-set;
- treats every other flow's files as read-only contracts;
- records evidence against the commit it was measured on;
- writes a gate it did not run as `NOT RUN`, never as `PASS`.

Non-EDITOR flows additionally:

- **must not** open Unreal Editor, call Unreal MCP mutating tools, write binary assets, edit maps or
  Blueprints, or start PIE;
- **must not** claim what only the Editor can show. `PIE`, visual evidence and asset integration are not
  observable from here, and claiming them is the one failure this rule exists to prevent;
- send everything engine-shaped to the EDITOR flow as a request (§6).

---

# 6 — The EDITOR flow

Exactly one flow, when reserved, holds the engine. It is the only one authorized to:

- start or drive Unreal Editor;
- use Unreal MCP for anything that mutates;
- modify binary assets;
- modify maps or Blueprints;
- start PIE;
- save Editor-generated assets;
- hold the machine's engine allocation for the duration.

## Taking the engine, without a lease

`D-347` removed the lease and did not replace it. There is no lock file and no marker; the declaration of
possession is the process itself.

**Read who is there, and from where:**

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
```

⛔ **A process count says nothing.** The `CommandLine` carries the `.uproject` — hence **which clone** —
`UnrealEditor.exe` against `UnrealEditor-Cmd.exe` says interactive Editor or headless run, and `-abslog`
says **which session**. Those three decide whether you wait.

**When you take it, make your process readable.** Every headless run passes `-abslog` inside this
session's own scratchpad directory. A process that dies takes its declaration with it; a lock file would
not.

| What is running | You want | |
|---|---|---|
| build or suite in **another** clone | build or suite | **do not wait** |
| **performance** measurement in any clone | anything on the engine | **wait**: CPU contention moves timings |
| anything in **your** clone | anything | **wait**: same `Binaries/` |
| interactive Editor on **your** clone | build | **wait**: it holds the DLL |
| anything | build of an **Engine** target | **wait**, and warn: the argument below lapses |

🔑 **A build in clone A does not invalidate a suite in clone B.** `Binaries/` is per clone and the engine
is an *installed build* (`Engine/Build/InstalledBuild.txt`), so a project target cannot rewrite Engine
modules. `AGENTS.md` §9 carries the measurement and the commands to re-verify it. ⚠️ If
`InstalledBuild.txt` disappears, or an Engine target is compiled, that argument lapses **entirely** and
everything serializes again.

**If you cannot wait, do not measure anyway.** Declare `NOT RUN` with the reason — *"engine held by
`<clone>`"*. An honest `NOT RUN` costs one round; an invalid measurement costs the credibility of every
other one.

## Which checkout may author assets

Asset authoring through MCP belongs to the clone that **hosts the bridge**, and that clone is identified
by measurement, never by name:

```text
1. read the bridge URL from the repository root's .mcp.json
2. find who listens on that port      -> Get-NetTCPConnection -State Listen -LocalPort <port>
3. read that PID's CommandLine        -> which .uproject, hence which checkout
4. compare with the repository root measured in §2
```

If the bridge answers for a different checkout than yours, an asset write from here mutates that
checkout's assets while you read your own `git status`. Emit:

```text
BLOCKED — MCP_ASSET_WRITE_WORKSPACE
```

and name the checkout that holds the bridge. Do not bypass the rule, and do not translate it into a
workspace taxonomy that this repository does not use.

⚠️ **The reason is a property of the checkout, not a protocol.** A worktree does not carry the gitignored
files: hard references read `None`, and **saving zeroes them** — silently, and you notice later.
Preparation, inspection and read-only queries have none of this constraint.

## Request format

A non-EDITOR flow that needs the engine writes a request into the shared queue. One request, one
verifiable outcome:

```text
REQUEST      <id>
ISSUE        #...
OPERATION    what to do, in one line
PROJECT/MAP  .uproject and map or asset path
SETUP        preconditions to establish before the check
ASSETS       assets involved, by path
CHECK        the observation to perform
EXPECTED     the result that would satisfy the AC
FALSIFIES    what would be seen if the AC does NOT hold
EVIDENCE     what must come back: log lines, capture, round-trip read
DEPENDS ON   request ids that must land first, or none
PRIORITY     blocking | needed-for-DoD | opportunistic
```

⛔ **`FALSIFIES` is not optional.** A check that cannot fail is not evidence, and the Editor is the most
expensive place to discover that.

## Returning results

The EDITOR flow writes results back into the issue matrix, one row per request, and never a bare verdict:

```text
REQUEST <id> — ISSUE #... — RESULT <state> — EVIDENCE <what came back> — CANDIDATE <sha>
```

Editor result states:

```text
PASS   FAIL   BLOCKED   NOT RUN   N/A   OBSERVED   INVALIDATED
```

`OBSERVED` is not `PASS` for systems the Editor cannot prove. The Editor cannot certify by itself byte
determinism, payload privacy, replay equivalence, packaged runtime, dedicated server, network authority or
shipping performance.

For every visual check, record `ACCEPTANCE ID`, `MAP / SCENARIO / FIXTURE`, `SETUP`, `EXPECTED`,
`FALSIFICATION CRITERION`, `OBSERVED`, `ORACLE: HUMAN | MIXED`, `EVIDENCE`, `RESULT`.

The verdict goes where its owner looks for it: a `PIE-*` entry in `docs/technical/test-manuali-pie.md`
when the behavior is in game; the owning issue when the check happens in the editor before Play; a
versioned artifact when the seance produces a file. ⛔ If the check **has** a `PIE-*` entry, the registry
owns the verdict and an issue comment does not replace it.

---

# 7 — MCP, used sparingly

Use MCP only when it changes a decision, implements a requirement, or produces evidence that is
required. Do not open the Editor to confirm that a UI, an asset or a scenario exists when the repository
and the logs already answer.

Shape every MCP session as one pass:

```text
1. minimal discovery          enumerate at runtime; do not recall a capability from memory
2. collect the requests       everything queued for this batch
3. operation manifest         one flat list of the mutations to perform
4. group                      by project, map, scenario and asset dependency
5. compatible mutations       apply the group that shares a setup
6. save at coherent bounds    nothing saves itself; ask, then verify on disk
7. one verification pass      per batch, not per request
8. collect logs and evidence  attributed to each issue the batch covered
```

⛔ **Do not issue concurrent MCP calls against the same Editor** unless the toolset's contract explicitly
guarantees it. The bridge is one.

⚠️ **Enumerate, do not remember.** `list_toolsets` then `describe_toolset`: which toolsets are mounted
depends on which plugins loaded in that startup, so the inventory is not a constant and must not be used
as an oracle. A capability absent from the project toolset may exist in an engine toolset, and a
capability absent from both may still be reachable through a commandlet — which is the idiomatic way to
**generate** an asset in this repository, with MCP the way to **wire** it and to verify it by round-trip.

⚠️ **A tool answering `true` is not evidence the change persisted.** Verify by round-trip — read back what
you wrote — and on disk with `git status`, not from the tool's return value.

---

# 8 — One validation queue

Build, UHT, engine tests, PIE, automation, cook and packaging share **one** queue, owned by the
coordinator. Every entry declares which issues it covers:

```text
GATE <name> — COVERS #..., #... — TRIGGER <why now> — CANDIDATE <sha> — RESULT <state>
```

Deduplicate equivalent checks: two issues whose write-sets both land in the same module do not each get
their own suite run.

Concentrate the expensive gates:

- at the end of **one issue**, when it must be validated in isolation;
- at the end of a **compatible group**;
- at the end of the **whole batch**, when a single gate produces evidence valid for all of it.

⛔ **Do not run a full build after every issue.**

An **early checkpoint** is allowed only when one of these changed, because a later failure would otherwise
be impossible to attribute:

```text
reflection / UHT               module boundaries          serialization
public APIs later flows need   Build.cs / Target.cs       replication
plugins                        asset schema               dependencies that blur attribution
```

Respect the exclusion policy between Editor and build: an interactive Editor on this clone holds the DLL.

## What makes a measurement valid

A measurement is valid only if it observes the same `HEAD`, working tree, binary and engine state from
start to finish. If any of them changes:

```text
INVALIDATED
```

⛔ **`git status --porcelain` is not enough, and that is the trap.** It lists status letters and paths:
two *different* modifications of the same file give the identical line ` M file.cpp`. Compare content:

```powershell
git rev-parse HEAD
git diff HEAD                                       # the CONTENT of the modified files
git ls-files -o --exclude-standard | Get-FileHash    # and of the untracked ones
```

⛔ **Untracked files are hashed, not listed** — a fixture rewritten during the run gives the same list of
paths and different hashes. ⚠️ No `-z` in that pipeline: it separates names with NUL, which PowerShell
hands to `Get-FileHash` as one non-existent path, and the command meant to expose the trap falls into it.

⚠️ `Binaries/` is gitignored, so none of the three sees it. Compare it separately: mtime and size of the
editor DLLs.

## Independence of the measure

```text
find defect -> preserve evidence -> finding -> fix it -> new candidate, commit declared -> measure that commit
```

NOT:

```text
find defect -> modify product -> rerun -> approve own fix
```

⚠️ The same session may do both steps — no role forbids it any more. What it must not do is collapse
them: fixing and measuring are **two moments**, and if one session did both, say so next to the result.

Record `CANDIDATE_SHA`, `HEAD BEFORE`, `HEAD AFTER`, `DIRTY STATE BEFORE`, `DIRTY STATE AFTER`,
`BINARIES / BUILD ID`, `COMMAND`, `FOUND`, `PERFORMED`, `PASSED`, `FAILED`, `EXIT CODE`.

`performed = 0` is never successful validation. 🔴 And the process exit code is not a verdict: read the
run's own report.

---

# 9 — Evidence states

Use only factual states:

```text
PASS   FAIL   BLOCKED   NOT RUN   N/A   OBSERVED   INVALIDATED
```

Never infer `PASS` from compilation only, from an MCP `success`, from a command sent, from the absence of
a visible error, from a closed issue or from an unrelated scenario.

If source, config, scenario fixture, binary asset or relevant test data change, produce a new candidate
identity and mark the impacted earlier evidence `INVALIDATED`. Do not combine incompatible candidate
evidence into one sign-off.

---

# 10 — Coherence scan

Before handoff, reconcile — per issue, then across the batch:

```text
Behavior Contract    implementation   scenarios   roadmap references   TurnLog/replay
Acceptance Criteria  tests            docs        related issues       privacy/network notes
technical DNNN
```

Apply `SEARCH -> REUSE -> UPDATE -> CREATE` for follow-ups. A finding does not automatically require a new
issue if an existing owner can absorb it.

⚠️ A batch makes one failure mode much likelier: **two issues editing the same document**. Reconcile the
document once, at the end, rather than twice from two flows.

---

# 11 — Closing the engine

After the last Editor batch, in this order:

1. save only the intended assets, and verify the dirty package list is what you expect;
2. collect logs and evidence, attributed per issue;
3. stop PIE;
4. close Unreal Editor cleanly;
5. release the engine allocation — say in the report that it is free;
6. verify that the processes **this session started** have terminated;
7. use the repository's own procedure for orphaned processes, if one exists.

⛔ **Never terminate an Unreal process by name.** Match the `CommandLine` — the `.uproject` and the
`-abslog` you passed — and leave every process that is not yours alone. An Editor that was already running
before this run stays running.

⚠️ Never force-kill when doing so risks unsaved project data; report the failed clean shutdown instead.

After code and tooling work: no Unreal process should have been left running, and unrelated local work is
untouched. After a verification run: preserve the logs, and do not leave mutated source behind — a
mutation gate that stops early leaves the **binary** mutated too, which `git checkout --` does not undo.
Rebuild before any other measurement.

---

# 12 — Reporting

Report on the **issues themselves** — a comment, or the PR body. A result that lives only in a chat does
not exist for whoever comes next, and nothing routes it automatically since `D-347`.

Per issue, state at minimum: status (`DONE` / `PARTIAL` / `BLOCKED` / `FAILED`), what changed, the
evidence with its commit, and what was **NOT RUN**. When a gate covered several issues, say so in each of
them — an issue whose evidence lives in another issue's comment has no evidence.

⚠️ **Do not write totals that change on their own** — how many issues in a family are open, how many
assets a folder holds, how many milestones exist. Nothing re-measures a number written in prose, so it
goes stale silently and reads as current. Use the **names** when an enumeration follows, or `xx`/`yy` with
the command to count them next to it. `AGENTS.md` §14 has the rule and its three exceptions.
⛔ This is not licence to be vague: a defect stated as a count — *"one non-test reader"*, *"`grep -ci`
answers 0"* — is evidence, and stays written with the command that produced it.

Completing one phase does NOT close the GitHub issue. Do not mark an issue `Done` unless its live DoD is
actually complete.

---

# 13 — Failure protocol

On any unrecoverable failure, in this order:

1. stop making unrelated changes;
2. preserve the diagnostics;
3. reconcile any partial issue or documentation edits so they are not left misleading;
4. update each affected issue with: work completed, the failing step, the exact blocker, the evidence, and
   the recommended next action;
5. set the issue to `Blocked` when that is what it is;
6. run §11 cleanup anyway;
7. close only the Editor instance this run started.

Never hide a partial failure behind a `DONE`.

---

# 14 — Final output

Always finish with:

```markdown
# ISSUE RUN REPORT

## ARGUMENTS
Issues: #...
Flows: requested N — effective M (reason)
Editor flow: yes | no

## WORKSPACE
Repository root / branch / HEAD / worktree-or-clone / origin-main

## ISSUE MATRIX

| Issue | Flow | Needs | Status | Evidence | NOT RUN |
|---|---|---|---|---|---|

## WORK PERFORMED

## FILES / ASSETS CHANGED

## EDITOR REQUESTS

| Request | Issue | Result | Evidence |
|---|---|---|---|

## VALIDATION QUEUE

| Gate | Covers | Candidate | Result |
|---|---|---|---|

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

## ENGINE

Released: yes | no | never taken
Processes started by this run still alive: none | ...

## HANDOFF

Remaining work, per issue:
Reason:

## FINAL PHASE STATUS

DONE | PARTIAL | BLOCKED | FAILED
```

`DONE` here means this phase is done unless the report explicitly states that the full GitHub issue DoD is
also satisfied.
