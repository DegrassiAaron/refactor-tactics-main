---
name: rt-terminal
description: Initialize the current Claude Code session with the Refactor Tactics terminal role contract.
argument-hint: "[dev|test|validation|editor]"
arguments:
  - role
disable-model-invocation: true
---

# Refactor Tactics terminal initializer

Initialize this Claude Code session for terminal type: `$role`.

## Accepted values

Normalize the first argument case-insensitively:

- `dev` -> `DEV`
- `test` -> `VALIDATION`
- `validation` -> `VALIDATION`
- `editor` -> `EDITOR`

If `$role` is empty or does not match one of the accepted values, stop immediately and print only:

`Usage: /rt-terminal <dev|test|validation|editor>`

Do not guess a role.

## Initialization procedure

1. Read `docs/rt-three-terminals/README.md`.
2. Read exactly ONE role prompt:
   - `DEV` -> `docs/rt-three-terminals/prompts/TERMINAL_DEV.md`
   - `VALIDATION` -> `docs/rt-three-terminals/prompts/TERMINAL_VALIDATION.md`
   - `EDITOR` -> `docs/rt-three-terminals/prompts/TERMINAL_EDITOR.md`
3. Do not load the other role prompts unless the user explicitly asks to compare roles.
4. Treat the selected prompt as the operational contract for the remainder of the current Claude Code session.
5. Also continue to obey repository-level instructions such as `AGENTS.md`, `CLAUDE.md`, applicable ADRs, issue requirements, and higher-priority instructions.
6. The selected role does NOT automatically change the machine-wide engine mode.
7. Do not call `rtmode` merely because this skill was invoked.
8. Do not claim that the VS Code terminal profile/color or parent PowerShell environment changed. This skill initializes Claude's operational role only.
9. If the environment exposes an existing `RT_TERMINAL_ROLE`, you may report a mismatch with the requested role, but do not silently override or reinterpret the requested role.
10. If the selected role is `VALIDATION` or `EDITOR`, respect the README requirement to verify the global mode before occupying Unreal.

## Result

After initialization, respond concisely with:

- `RT ROLE: <DEV|VALIDATION|EDITOR>`
- the role prompt file loaded;
- current engine mode if it can be determined safely without changing it;
- one short sentence describing what is allowed in this role.

Do not start builds, tests, Unreal Editor, PIE, suites, or other work merely because the role was initialized. Wait for the user's next task.
