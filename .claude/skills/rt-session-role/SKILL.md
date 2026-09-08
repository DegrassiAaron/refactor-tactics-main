---
name: rt-session-role
description: Initializes the current Claude Code session with exactly one RefactorTactics RT3 logical role contract (DEV, EDITOR or VALIDATION). Does not assign work, acquire Unreal, change workspace identity, or execute an issue.
argument-hint: "[dev|test|validation|editor]"
arguments: [role]
disable-model-invocation: true
allowed-tools: Read, Bash, PowerShell
---

# RefactorTactics — RT Session Role

Requested role:

`$role`

This skill initializes a Claude session with one RT3 **logical role**.

It does NOT create a fixed three-terminal workflow.

Three roles exist:

```text
DEV
EDITOR
VALIDATION
```

The number of active windows/sessions is a runtime concern.

---

# 1 — Normalize role

Case-insensitive mapping:

```text
dev         → DEV
test        → VALIDATION
validation  → VALIDATION
editor      → EDITOR
```

If empty/invalid:

```text
Usage: /rt-session-role <dev|test|validation|editor>
```

Stop.

Do not infer the role.

---

# 2 — Load canonical session contract

Read:

```text
AGENTS.md
CLAUDE.md
```

⚠️ The per-role prompt files and the RT3 shared-operation documents **no longer exist**.
The control plane was removed: what survived of that model now lives in `AGENTS.md` and
`CLAUDE.md`, and nowhere else. Do not look for `docs/rt-three-terminals/` — reading a
path that is gone teaches the session to invent what it cannot find.

The role itself is still real, and the scripts that enforce it are still there:

```text
scripts/rt-terminal.ps1     session identity (role, instance, owner PID + start time)
scripts/rt-lease.ps1        the engine: one Unreal per machine
scripts/rt-suite.ps1        measurements
scripts/rt-task-router.ps1  task routing
```

Do not read other role prompts merely to blend responsibilities.

One role per session.

Higher-priority repository instructions remain authoritative.

---

# 3 — Keep identities separate

Do not collapse these concepts:

```text
SESSION ROLE
DEV | EDITOR | VALIDATION

WORKSPACE IDENTITY
project checkout identity

UNREAL RESOURCE
who currently owns the machine-wide Unreal operation

TASK ROUTING
who should perform the current phase

ISSUE OWNERSHIP / CLAIM
GitHub/project ownership rule
```

They answer different questions.

Invoking `/rt-session-role` does NOT:

- change workspace identity;
- translate workspace taxonomy;
- acquire an Unreal lease;
- start Unreal;
- start PIE;
- start build/suite;
- assign a task;
- claim a GitHub issue;
- create a branch;
- mutate the task router;
- modify repository state.

---

# 4 — Workspace taxonomy safety

Use the workspace identity defined by current canonical repository tooling.

If environment/tooling exposes conflicting names such as:

```text
DESIGNER
TECHNICAL_DESIGNER
```

do not invent a translation.

Report:

```text
WORKSPACE_TAXONOMY_CONFLICT
```

and refer to the live owner/decision.

This skill must not solve control-plane governance.

---

# 5 — Task routing read-only check

Bash/PowerShell use inside this skill is read-only.

If `RT_TASK_ID` exists, query:

```powershell
rttask status -TaskId $env:RT_TASK_ID
rttask assignment -TaskId $env:RT_TASK_ID
```

or the repository-equivalent current command.

Report:

```text
TASK ID
TASK STATUS
NEXT ACTOR
ASSIGNMENT SEQUENCE
OBJECTIVE
```

Compare:

```text
RT_TERMINAL_ROLE
vs
next_actor
```

If mismatch:

```text
TASK_ROUTE_MISMATCH
```

Stop.

Do not mutate the router or fix the assignment.

The Coordinator owns routing mutations.

If `RT_TASK_ID` is absent:

```text
NO TASK ASSIGNED
```

This is valid.

A role session may exist before receiving work.

---

# 6 — Unreal resource

Opening or initializing an EDITOR/VALIDATION session does not acquire Unreal.

A terminal may remain open while another task/role uses the machine resource.

Unreal ownership is:

```text
just-in-time
exclusive
released after the operation
```

The exact lease/resource command comes from current repository tooling.

This skill may read current status when safe.

It does not acquire/release it during initialization.

---

# 7 — Role summary

## DEV

Owns C++, tests, textual Scenario Harness authoring, headless tooling, docs and Git/GitHub.

Does not occupy Unreal unless a later explicit accepted workflow says otherwise.

## EDITOR

Owns Unreal asset authoring, Blueprint/UMG/Material, `.uasset/.umap`, PIE and visual/human acceptance.

Does not automatically own the machine Unreal resource just because the session is open.

## VALIDATION

Owns independent build/test measurement, Scenario Harness execution, determinism/replay/privacy and packaged/performance where required.

Does not repair a defect and approve its own fix.

---

# 8 — MAIN / MCP

When live repository policy says only the `MAIN` workspace hosts the machine MCP bridge:

- treat this as workspace/resource policy;
- do not confuse `MAIN` workspace identity with Git branch `main`;
- an EDITOR role outside the authorized workspace does not gain MCP asset-write permission merely because it is EDITOR.

Do not silently bypass MCP policy.

---

# 9 — Completion output

Reply:

```text
RT SESSION ROLE: <DEV|EDITOR|VALIDATION>
```

Then:

```text
ROLE CONTRACT: <path>

TASK:
<id/status/next actor or NO TASK ASSIGNED>

WORKSPACE:
<measured identity if available>

UNREAL RESOURCE:
<status if safely readable>

BOUNDARY:
<one-sentence role boundary>
```

If any mismatch exists, append exactly the relevant warning:

```text
TASK_ROUTE_MISMATCH
WORKSPACE_TAXONOMY_CONFLICT
ROLE_ENVIRONMENT_MISMATCH
```

Then stop and wait for the next instruction.

Do not automatically invoke `/issue-run`.
