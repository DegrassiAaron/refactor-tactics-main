# Shared MCP Kit for Claude Code on Windows

## Target architecture

Machine-wide:
- Playwright: one HTTP server on `127.0.0.1:8931`
- MCP gateway: one HTTP endpoint on `127.0.0.1:8080`
- Episodic Memory: one gateway backend **only after** auditing the currently installed Claude plugin so its hooks are preserved without launching an extra MCP per Claude session
- Superpowers Chrome: disabled in this first migration because sharing one live Chrome-control session between multiple agents can create contention

Per repo/worktree:
- Serena: one Streamable HTTP server per repo/worktree, shared by every Claude Code terminal working in that same root
- Different repos/worktrees MUST use different Serena ports

## Files to put in each project directory

Copy:

`project-mcp.settings.template.json`

to the **root** of each repo/worktree and rename it:

`.mcp-shared.json`

Example:

```text
C:\dev\refactor-tactics-main\
  .git\
  .mcp-shared.json
  CLAUDE.md
  ...

C:\dev\meepleai-monorepo\
  .git\
  .mcp-shared.json
  ...
```

Give each simultaneously active root a unique Serena port:

```text
Refactor Tactics main       9121
MeepleAI                    9122
InfluencerAI                9123
Refactor Tactics worktree A 9124
Refactor Tactics worktree B 9125
```

Do NOT copy the same Serena port to two roots that can be active at the same time.

## Machine installation

Run once:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\install-shared-mcp.ps1
.\start-shared-mcp.ps1
.\status-shared-mcp.ps1
```

Register the shared gateway once at user scope:

```powershell
claude mcp add --transport http --scope user mcp-shared http://127.0.0.1:8080/mcp
```

Do not also keep a second user-scoped `playwright-shared` after the gateway migration has been verified, otherwise Claude will expose Playwright twice.

## Per project

From a repo root containing `.mcp-shared.json`:

```powershell
C:\path\to\shared-mcp-kit\start-project-mcp.ps1
```

This starts the Serena server for that root and registers `serena` at project scope.

## Important: Episodic Memory

Episodic Memory already uses one local SQLite search index and preserves project metadata in indexed conversations. Its `sync` is documented as atomic, concurrent-safe, and idempotent.

However the Claude plugin also bundles:
- lifecycle hooks for automatic indexing
- its own stdio MCP server

Therefore disabling the plugin wholesale would remove useful automatic indexing. Do NOT edit the plugin cache directly.

The migration prompt included in this kit tells Claude to audit the installed plugin and only centralize the MCP process if it can preserve the hooks/skills cleanly. If not, keep the plugin unchanged: memory remains functionally shared even though the MCP process count is not yet optimized.

## Verification

After migration, open two Claude terminals in the SAME repo and verify both see the same Serena endpoint.

Open Claude terminals in DIFFERENT repos and verify each sees a different Serena endpoint/port.

Run:

```powershell
.\status-shared-mcp.ps1
```

Then re-run your process inventory and compare counts.

## Rollback

1. Stop processes created by this kit:

```powershell
.\stop-shared-mcp.ps1
```

2. Restore the backed-up Claude configuration made by the migration procedure.
3. Re-enable any plugin that was changed only after the plugin audit succeeded.
4. Remove project Serena registrations only if required:

```powershell
claude mcp remove serena --scope project
```

## Safety rules

- Bind shared MCP services only to `127.0.0.1`.
- Never kill Node/Python processes by name globally.
- Stop only PIDs created by this kit or processes positively identified by full command line.
- Back up Claude configs before editing.
- Treat separate Git worktrees as separate Serena projects.
