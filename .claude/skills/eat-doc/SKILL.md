---
name: eat-doc
description: Reads one instruction document, executes it end-to-end, may take exclusive ownership of the RefactorTactics Unreal Editor/MCP when asset or editor validation is required, then consumes the document and shuts Unreal Editor down.
argument-hint: "[file]"
arguments:
  - file
disable-model-invocation: true
---

# RefactorTactics — Eat Doc

Consume one instruction document and execute it end-to-end.

Invocation:

`/eat-doc $file`

`$file` is the path or repository-relative name of exactly one file.

The document is an ephemeral work order: read it completely, follow its instructions, validate the resulting work as required, delete the consumed document, and shut Unreal Editor down before this command finishes so the Unreal/MCP resource is released for other workers.

## Core invariants

1. Read `AGENTS.md` and `CLAUDE.md` before making project changes.
2. Resolve `$file` to exactly one existing regular file. Do not accept a directory or wildcard expansion that resolves to multiple files.
3. Read the entire document before executing it. The document supplies the task-specific instructions for this run.
4. The document does not override higher-priority user/system instructions, repository safety rules unrelated to the explicit Unreal exclusivity exception below, or destructive-operation safeguards.
5. Execute the document rather than merely summarizing or proposing its instructions.
6. Preserve unrelated local changes. Do not use reset/checkout/clean/stash as a shortcut for a dirty worktree.
7. If the document requires modifying or testing Unreal assets, levels, Blueprint behavior, PIE/editor behavior, or other editor-owned state, prefer the connected Unreal/Epic MCP over filesystem manipulation or improvised automation.
8. This command has priority for the RefactorTactics Unreal Editor/MCP resource. It may establish exclusive ownership as defined below.
9. Never hand-edit `.uasset` or `.umap` files.
10. Always close Unreal Editor at the end of this command, including after failures once useful diagnostics and intentional saves are secured.
11. Delete the consumed document only after it has been read completely. By default, delete it during final cleanup after its instructions have been executed or the run has reached a terminal failure state.
12. Report only checks that were actually run. Anything not run is `NOT RUN`.

---

# Phase 0 — Resolve and preflight

Normalize `$file` relative to the repository root unless it is already an explicit valid path inside the working tree.

Determine:
- repository root;
- current branch/worktree;
- current `HEAD`;
- current Git status;
- resolved document path;
- whether the document is tracked, untracked or ignored;
- available Unreal/Epic MCP tools;
- Unreal processes currently associated with this RefactorTactics project.

Reject the invocation safely if:
- `$file` is missing;
- it resolves to zero or more than one file;
- it is a directory;
- it cannot be read completely;
- resolving it would target a path outside the repository/worktree unless the user explicitly supplied and authorized that external path.

Do not delete the document during preflight.

---

# Phase 1 — Read and internalize the work order

Read the complete resolved document before making task changes.

Extract internally:
- requested outcome;
- explicit constraints;
- files/assets/systems mentioned;
- required tests or scenarios;
- required documentation updates;
- required Git/GitHub operations, if any;
- completion criteria;
- cleanup requirements.

Then inspect only the repository context necessary to execute those instructions correctly.

Use the repository rule:

`SEARCH -> REUSE / UPDATE -> CREATE only for a real gap`

Do not replace the document's concrete instructions with a generic workflow when they are already actionable.

If the document conflicts with current repository reality, prefer the current canonical owner and implementation unless the work order explicitly instructs a migration/change. Report unresolved contradictions instead of guessing.

---

# Phase 2 — Execute the document

Implement the requested work directly.

Follow the RefactorTactics architecture and current pinned Unreal version from `CLAUDE.md`/`AGENTS.md`.

When relevant:
- C++ remains authority for simulation, networking, serialization, pathfinding, validation and competitive rules;
- Blueprint/data remain configuration and presentation where appropriate;
- simulation state remains separate from presentation;
- deterministic ordering, stable IDs and privacy boundaries must not be weakened;
- planning data must never be leaked to opponents;
- assets under `/Game/RT/` are changed through Unreal-supported tooling, not by editing binary files directly.

If the document requires a destructive action beyond consuming `$file` or taking the scoped Unreal resource ownership defined below, do not infer permission merely from this skill. Follow the normal repository/user authorization rules.

---

# Phase 3 — Exclusive Unreal/Epic MCP ownership

Use this phase whenever the work order requires asset modification, Editor-only validation, PIE, map inspection, Blueprint changes, asset creation, or another operation best performed through Unreal/Epic MCP.

## Priority rule

`/eat-doc` owns the RefactorTactics Unreal Editor/MCP resource for the duration of this phase.

The normal rule that preserves a pre-existing same-project Editor instance is explicitly overridden for this skill when that instance prevents exclusive MCP control or reliable validation.

Scope the exception narrowly:
- only Unreal Editor / UnrealEditor-Cmd / directly associated RefactorTactics Unreal processes that contend for this project's Editor/MCP resource;
- never terminate unrelated applications;
- never terminate Unreal processes for a different project unless they are demonstrably the process holding the same required MCP/editor resource;
- do not kill build tools or unrelated worker processes merely because they are running.

## Acquire ownership

Before using the MCP:

1. Discover the Unreal/Epic MCP tools actually available. Do not invent tool names.
2. Detect active Unreal processes associated with RefactorTactics.
3. If a competing same-project Editor is running, first request a normal shutdown when a safe mechanism is available.
4. If it does not exit and it blocks exclusive ownership, terminate the competing RefactorTactics Unreal process.
5. Confirm the competing process is gone before launching/reusing the Editor for this run.
6. Launch or connect the RefactorTactics Editor through the supported MCP/tooling path.
7. Treat this run as the exclusive Editor owner until cleanup.

Before force termination, make a best effort to preserve already-saved project state. Do not pretend unsaved state was preserved if it cannot be proven.

## Use MCP first

When an editor-capable MCP operation can perform the required asset/test action, prefer it to:
- raw filesystem edits of Unreal assets;
- ad-hoc binary manipulation;
- manual guessing about Editor state.

Use the smallest relevant scenario/test set that proves the requested behavior.

If the work order changes multiplayer/privacy behavior, prefer validation with the required server/client topology rather than a single visual Editor check.

---

# Phase 4 — Validate

Run the checks required by the consumed document plus the repository checks needed for the changed surface.

Prefer existing project entry points and scenario infrastructure.

When applicable, validation can include:
- targeted compilation;
- `./scripts/rt-suite.ps1` with the relevant filter;
- Automation Tests;
- data validators;
- deterministic/replay checks;
- network/privacy checks;
- Unreal/Epic MCP scenario execution;
- PIE or editor verification;
- packaged verification only when explicitly required or necessary for the task's Definition of Done.

Do not claim a validation passed unless it actually ran successfully against the final relevant state.

If a validation failure is caused by this work, fix it when within scope.

If blocked by an unrelated/external failure, record the exact blocker and continue only where doing so will not make the repository state misleading.

---

# Phase 5 — Consume the document

After the document has been read completely and the run has reached either:
- successful completion of its instructions; or
- a terminal failure/blocker with useful diagnostics preserved,

delete the resolved `$file`.

Rules:
- delete only the exact resolved input document;
- never broaden deletion to sibling files/directories;
- if it is tracked, the deletion is an intentional Git change from this command;
- if it is untracked/ignored, remove only that file;
- do not skip consumption merely because execution failed after the file was read, unless deleting it would destroy the only available diagnostic/source needed to recover from a repository/tool failure. In that exceptional case, preserve it and clearly report why it was not consumed.

After deletion, verify that the exact file no longer exists.

---

# Phase 6 — Mandatory Unreal cleanup

Always perform this phase, whether the work order succeeded or failed.

If Unreal Editor was used or is running for RefactorTactics at cleanup time:

1. save only intentional asset/project changes required by the work order;
2. finish or stop active PIE/scenario execution;
3. request normal Unreal Editor shutdown;
4. confirm the Editor process terminated;
5. if normal shutdown fails, terminate the RefactorTactics Unreal Editor process;
6. verify no RefactorTactics Unreal Editor instance owned/used by this command remains running.

For this skill, unlike the default project rule, a pre-existing RefactorTactics Editor that was taken over as part of the exclusive ownership phase is not left running at the end.

The final state required by `/eat-doc` is:

`RefactorTactics Unreal Editor: CLOSED`

This releases the Editor/MCP resource for other workers/processes.

---

# Failure protocol

On an unrecoverable failure:

1. stop unrelated edits;
2. preserve useful logs/diagnostics;
3. keep intentional changes only when they are coherent and useful;
4. consume the input document according to Phase 5 unless the documented recovery exception applies;
5. run mandatory Unreal cleanup;
6. report the exact failing step, resulting repository state and recommended next action.

Never leave Unreal Editor running because the command failed midway.

---

# Completion gate

The command is complete only after all applicable items are true:

- [ ] `$file` resolved to exactly one document.
- [ ] The complete document was read.
- [ ] Its actionable instructions were executed as far as possible.
- [ ] Repository guardrails were preserved.
- [ ] Unreal/Epic MCP was preferred for relevant asset/editor work.
- [ ] Exclusive same-project Unreal ownership was acquired when required.
- [ ] Relevant validations were actually run and recorded.
- [ ] The exact input document was deleted, or a recovery exception was explicitly reported.
- [ ] RefactorTactics Unreal Editor is closed.
- [ ] No unrelated processes/files were terminated or deleted.
- [ ] Final output distinguishes PASS, FAIL and NOT RUN.

## Final output

Report concisely:

### Risultato
What the document requested and what was actually completed.

### File / asset
Files/assets created, modified or deleted, including the consumed document.

### Verifiche
Checks and scenarios actually executed with PASS/FAIL.

### NOT RUN
Relevant checks not executed.

### Unreal lifecycle
Whether exclusive ownership was needed, competing same-project Unreal processes handled, MCP/editor usage, and confirmation that the Editor is closed.

### Rischi / aperti
Blockers, partial work, unsaved-state risk from forced process termination, or follow-ups.

### Prossimo passo
One recommended next action, only if one remains.
