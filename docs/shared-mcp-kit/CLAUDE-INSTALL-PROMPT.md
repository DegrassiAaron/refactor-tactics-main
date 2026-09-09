# Claude Code handoff — migrate duplicated MCP processes to shared services

You are operating on my Windows development machine. I use many Claude Code terminals at the same time, often in different directories, repositories and Git worktrees. Current process inventory showed heavy MCP duplication, especially Serena, Playwright, Episodic Memory and Superpowers Chrome.

Your job is to INSTALL AND MIGRATE the shared MCP architecture described below. Do the work, verify it, and leave rollback artifacts. Do not merely explain what I should do.

## Desired architecture

### Machine-wide shared services
- Playwright: ONE shared Streamable HTTP MCP server on `127.0.0.1:8931`.
- A local MCP gateway on `127.0.0.1:8080/mcp`.
- Episodic Memory: shared machine-wide search/index if safe. Preserve automatic conversation indexing hooks.
- Superpowers Chrome: DO NOT centralize automatically until you verify that one server/browser session safely supports multiple simultaneous Claude clients. Prefer leaving it disabled or unchanged rather than introducing browser-session contention.

### Per repo/worktree
- Serena: ONE Streamable HTTP instance per repo/worktree.
- All Claude terminals working on the same root connect to the same Serena instance.
- Different repos/worktrees receive different ports.
- Treat distinct Git worktrees as distinct roots even when they belong to the same repository.

## Files supplied

This directory contains:
- `shared-mcp.settings.json`
- `project-mcp.settings.template.json`
- `install-shared-mcp.ps1`
- `start-shared-mcp.ps1`
- `stop-shared-mcp.ps1`
- `status-shared-mcp.ps1`
- `start-project-mcp.ps1`
- `README.md`

Use these as the baseline, but inspect the installed tool versions and correct a script if the locally installed CLI syntax differs. Do not silently replace the architecture.

## Mandatory safety rules

1. BEFORE changing anything, back up:
   - `~/.claude.json` if present
   - `~/.claude/settings.json` if present
   - every project `.mcp.json` that you modify
   - relevant plugin configuration/metadata
2. Put backups under:
   `~/.mcp-shared/backups/<timestamp>/`
3. Do NOT edit files inside `~/.claude/plugins/cache` as the permanent solution.
4. Do NOT kill all `node.exe`, `python.exe`, `uv.exe`, etc.
5. Stop only processes you positively identify by PID + full command line, or PIDs created by our scripts.
6. All shared HTTP MCPs must bind to `127.0.0.1`, never `0.0.0.0`.
7. Preserve existing credentials and secrets. Never print tokens.
8. If a migration step cannot be made safely, keep the existing working configuration and report that step as BLOCKED rather than improvising.

## Phase 1 — Inventory the real current state

Run and save:
- `claude --version`
- `claude mcp list`
- `Get-CimInstance Win32_Process` with PID, parent PID, name and command line for MCP/Claude/Node/Python/uv processes
- installed plugin list/status
- current Serena version
- current Episodic Memory version
- current `mcp-gway` version if installed
- current Node/npm/npx, Python/pip and uv versions

Identify which MCPs come from:
- user scope
- project `.mcp.json`
- local scope
- Claude plugins

Build a BEFORE table with:
`MCP | source | transport | instances | process count | project-aware? | migration action`.

## Phase 2 — Install prerequisites

Use the supplied `install-shared-mcp.ps1`, correcting only version/CLI compatibility problems discovered locally.

For Serena, use the current supported `serena-agent` installation rather than an old marketplace command.

Initialize Serena and verify `serena start-mcp-server --help` exposes Streamable HTTP.

Install/update:
- `mcp-gway`
- global Episodic Memory CLI only if needed for a standalone shared MCP backend

Do not uninstall the working Episodic Memory plugin yet.

## Phase 3 — Shared Playwright + gateway

Start Playwright on:
`http://127.0.0.1:8931/mcp`

Start the gateway on:
`http://127.0.0.1:8080/mcp`

Register the gateway ONCE with Claude Code user scope as:
`mcp-shared`

Verify connection with `claude mcp list`.

Test from TWO separate Claude Code sessions that Playwright tools are reachable through the same shared backend.

Only AFTER that test succeeds:
- remove/disable redundant standalone user/project Playwright registrations that would expose the same tools twice
- do not remove unrelated browser MCPs

## Phase 4 — Episodic Memory audit and migration

This step must be evidence-driven.

Facts to preserve:
- the existing plugin automatically indexes/syncs conversations through hooks
- the memory index is SQLite-based
- sync is intended to be atomic/concurrent-safe/idempotent
- conversations retain project information

Inspect the installed Episodic Memory plugin manifest, hooks, skills and `.mcp.json`.

Goal:
- keep automatic indexing hooks/skills
- expose ONE shared Episodic Memory MCP backend through the gateway
- prevent every Claude session from launching a duplicate stdio memory MCP

First check whether the installed Claude Code/plugin version supports disabling only a plugin's MCP surface while retaining hooks/skills.

If YES:
- use the supported mechanism
- connect the standalone `episodic-memory-mcp-server` to `mcp-gway`
- verify search/read from two Claude sessions
- verify a session-end sync still updates the shared index

If NO:
- DO NOT modify plugin cache
- DO NOT disable the whole plugin
- leave Episodic Memory plugin unchanged
- document this optimization as BLOCKED
- note that memory data remains shared even if MCP server processes are duplicated

Also verify that searches from Refactor Tactics preferentially use results belonging to that project when project/path metadata is available. Do not claim hard project isolation if the current MCP search API does not provide it.

## Phase 5 — Serena per root/worktree

Discover the repo/worktree roots I actively use from currently running Claude processes and/or Git.

For each active root:
1. create `.mcp-shared.json` in the ROOT using `project-mcp.settings.template.json`
2. assign a unique Serena port starting at 9121
3. start ONE Serena Streamable HTTP process for that root
4. register `serena` in that project's Claude MCP scope pointing to:
   `http://127.0.0.1:<port>/mcp`

Example:
- Refactor Tactics main -> 9121
- MeepleAI -> 9122
- InfluencerAI -> 9123
- additional worktrees -> 9124+

Do not assume names: detect the actual root paths.

Verify:
- two Claude sessions in the SAME root use the SAME Serena endpoint/process
- Claude sessions in DIFFERENT roots use DIFFERENT Serena processes
- Serena is querying/editing the correct root in every case

Only then remove old per-terminal stdio Serena registrations/plugins responsible for duplicate processes.

## Phase 6 — Superpowers Chrome

Inspect how it is currently installed and why each Claude session launches it.

Do a concurrency capability check before changing anything.

If one shared server/browser state would allow agent A to alter agent B's active tab/session unpredictably, keep this MCP per-session or disable it where Playwright already covers the use case.

Do not centralize it merely to reduce process count.

Produce a recommendation:
- KEEP PER SESSION
- SHARED
- DISABLE BY DEFAULT / ENABLE ON DEMAND

with evidence.

## Phase 7 — Startup

Once the architecture passes tests, make machine-wide services start automatically in a maintainable Windows-native way.

Preferred:
- a single scheduled task at user logon or equivalent user-level startup mechanism
- no administrator requirement unless genuinely necessary
- start shared gateway/Playwright only once
- project Serena may be started on demand by `start-project-mcp.ps1` unless there is a clear benefit to starting all project instances at logon

Do not start inactive project Serena instances without a reason.

## Phase 8 — Validation

Run the same process inventory used BEFORE.

Produce an AFTER table:
`MCP | before instances/processes | after instances/processes | saved | status`.

Required checks:
- ports listen only on loopback
- `claude mcp list` has no accidental duplicate Playwright exposure
- two same-project terminals share Serena
- different projects do not share Serena
- Episodic Memory search still works
- Episodic Memory session-end indexing still works if its plugin was changed
- no zombie processes introduced
- existing project MCPs unrelated to this migration still work

## Deliverables

Create:
`~/.mcp-shared/MIGRATION-REPORT.md`

It must contain:
- architecture implemented
- BEFORE/AFTER counts
- exact roots and Serena ports
- files/configs changed
- plugins changed
- tests executed and results
- blocked optimizations
- rollback instructions
- remaining risks

Also leave the backup directory intact.

## Definition of done

Do not declare success just because services start.

Success means:
1. shared Playwright works from multiple terminals
2. Serena is one process per root/worktree and correct for that root
3. no duplicated registrations expose identical Playwright tools
4. memory functionality is preserved; MCP centralization is only claimed if the plugin MCP duplication was actually removed
5. process count is measurably lower
6. rollback is documented and tested enough to be credible

Proceed autonomously. Ask me only if an operation requires a destructive choice that cannot be safely inferred; otherwise inspect, implement, test and report.
