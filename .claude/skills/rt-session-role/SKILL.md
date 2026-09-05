---
name: rt-session-role
description: Initialize the current Claude Code session with one Refactor Tactics terminal-role contract (DEV, VALIDATION, or EDITOR). Use only when explicitly invoked by the user.
argument-hint: "[dev|test|validation|editor]"
arguments: [role]
disable-model-invocation: true
allowed-tools: Read
---

# Refactor Tactics session role

Initialize this Claude Code session for the requested Refactor Tactics role.

Requested role: `$role`

## Normalize the role

Interpret `$role` case-insensitively:

- `dev` -> `DEV`
- `test` -> `VALIDATION`
- `validation` -> `VALIDATION`
- `editor` -> `EDITOR`

If `$role` is empty or invalid, stop. Reply only with:

`Usage: /rt-session-role <dev|test|validation|editor>`

Do not infer a role.

## Load the contract

Read:

1. `${CLAUDE_PROJECT_DIR}/docs/rt-three-terminals/README.md`
2. Exactly one role prompt:
   - `DEV`: `${CLAUDE_PROJECT_DIR}/docs/rt-three-terminals/prompts/TERMINAL_DEV.md`
   - `VALIDATION`: `${CLAUDE_PROJECT_DIR}/docs/rt-three-terminals/prompts/TERMINAL_VALIDATION.md`
   - `EDITOR`: `${CLAUDE_PROJECT_DIR}/docs/rt-three-terminals/prompts/TERMINAL_EDITOR.md`

Do not read the other role prompts unless the user explicitly asks to compare roles.

Treat the selected role prompt as the active operational contract for the rest of this Claude Code session.

Repository-level and higher-priority instructions remain authoritative, including `AGENTS.md`, `CLAUDE.md`, applicable ADRs, issue requirements, and explicit user instructions.

## Separation of concerns

The Claude session role and the machine-wide Unreal engine mode are separate.

Invoking this skill:
- does not change `rtmode`;
- does not change `RT_TERMINAL_ROLE` in the parent PowerShell process;
- does not change the VS Code terminal profile, name, or color;
- does not acquire Unreal ownership;
- does not start Unreal, PIE, builds, tests, Scenario Harness, or suites.

For `VALIDATION` and `EDITOR`, follow the README/prompt preconditions before any later task occupies Unreal.

If the environment exposes an existing terminal role and it conflicts with the requested role, report the mismatch. Do not silently rewrite either value.

## Completion response

After loading the contract, reply concisely with:

`RT SESSION ROLE: <DEV|VALIDATION|EDITOR>`

Then report:
- the loaded role prompt path;
- the current engine mode only if it can be read safely without changing it;
- one sentence summarizing the role's operating boundary.

Then stop and wait for the user's next instruction.
