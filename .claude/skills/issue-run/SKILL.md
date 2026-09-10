---
name: issue-run
description: Compatibility entry point for the worktree-issue-runner workflow. Accepts the same arguments and produces the same run; carries them to the canonical workflow and adds nothing of its own.
argument-hint: "<issue...> [flows=N] [editor=auto|yes|no]"
disable-model-invocation: true
---

# RefactorTactics — Issue Run (entry point)

Arguments:

`$ARGUMENTS`

This file is **not** the workflow. It is the short, compatible name for it.

```text
/issue-run  -->  worktree-issue-runner  -->  execution
```

`/issue-run` and `/worktree-issue-runner` accept the same syntax and produce the same run. The workflow
lives in exactly one place, so the two names cannot drift apart.

---

# What to do now

1. **Echo the arguments received**, verbatim and unparsed:

   ```text
   ARGUMENTS RECEIVED : $ARGUMENTS
   ```

   Do not parse, normalise, reorder or drop anything here. Parsing is §1 of the canonical workflow, and
   doing it twice is how the two names start to disagree.

2. **Load the canonical workflow.** Resolve the repository root first, because this file must not carry
   an absolute path:

   ```text
   git rev-parse --show-toplevel
   ```

   then read, in full:

   ```text
   <repository root>/.claude/skills/worktree-issue-runner/SKILL.md
   ```

3. **Execute that file** with the arguments echoed in step 1, exactly as if the user had typed
   `/worktree-issue-runner $ARGUMENTS`.

---

# Fallbacks

| Situation | What to do |
|---|---|
| the canonical file is missing at that path | try `~/.claude/skills/worktree-issue-runner/SKILL.md` |
| neither exists | stop. Say which paths were checked, and ask. ⛔ Do **not** improvise the workflow from this file: it does not contain one |
| `/issue-run` is not defined in some environment | `/worktree-issue-runner` is the direct entry point there and needs no adapter |

⛔ **Do not re-enter this file.** The canonical workflow must never invoke `/issue-run`,
`/worktree-issue-runner` or the `worktree-issue-runner` skill from inside itself: the composition happens
here, once, and `/issue-run -> /worktree-issue-runner -> /issue-run` is a loop, not a delegation.

ℹ️ Historical note: until 2026-09-10 this file carried the whole workflow, and `worktree-issue-runner` did
not exist. Nothing was dropped in the move — the content, including the machine-resource discipline that
[`D-362`](../../../docs/decisions/RT_PDR_00_Decision_Log.md) names at this path, now lives one hop away in
`.claude/skills/worktree-issue-runner/SKILL.md`.
