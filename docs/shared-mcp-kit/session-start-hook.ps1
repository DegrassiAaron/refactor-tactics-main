<#
  session-start-hook.ps1 — SessionStart hook: bring up the MCP servers this session needs.

   1. Shared Playwright on 8931 (machine-wide). If it is down, the logon Scheduled Task is asked to
      start it. Going through the task is deliberate: Task Scheduler owns the process, so nothing is
      inherited from this hook (see the handle note below).
   2. This root's shared Serena, if the root carries a .mcp-shared.json.

  Design notes:
   * Reads the session cwd from the hook's stdin JSON, falling back to the process cwd.
     The hook payload is authoritative; the process cwd is not guaranteed to be the project root.
   * NO-OP for any root without a .mcp-shared.json. Most repos on this machine have none, and a
     SessionStart hook that errored there would be noise on every unrelated session.
   * Fast path first: an already-listening port costs one check. That is the common case — the
     second, third, ... terminal in the same root, and every session after the first.
   * -Detached is MANDATORY, not an optimisation. Measured 2026-09-10: with the starter's normal
     stream redirection, Start-Process uses UseShellExecute=false and bInheritHandles=TRUE, so the
     long-lived Serena inherits this hook's stdout PIPE and holds it open for its whole life. Claude
     Code waits for that pipe to close, so the session never finished starting — a real `claude -p`
     run sat for 240 s with no output while Serena itself came up fine. -Detached spawns with no
     redirection (UseShellExecute=true), which inherits nothing. After the fix the same run finished
     in 51 s and the session reached mcp__serena__list_memories.
   * -NoRegister: the `serena` entry is already persisted in ~/.claude.json at local scope. Shelling
     out to `claude mcp add` from inside a starting Claude session would be slow and re-entrant;
     this hook only has to guarantee the SERVER is up.
   * Never throws and never writes to stdout. A failing SessionStart hook must not break the session;
     a printing one would inject text into the session context.
#>
$ErrorActionPreference = 'SilentlyContinue'

function Test-Listening([int]$Port) {
    return [bool](Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue)
}

function Wait-Listening([int]$Port, [int]$Seconds) {
    for ($i = 0; $i -lt $Seconds; $i++) {
        if (Test-Listening $Port) { return $true }
        Start-Sleep -Milliseconds 1000
    }
    return (Test-Listening $Port)
}

try {
    # ---- 1. shared Playwright ------------------------------------------------
    if (-not (Test-Listening 8931)) {
        Start-ScheduledTask -TaskName 'MCP Shared Services (user)' -ErrorAction SilentlyContinue
        Wait-Listening 8931 25 | Out-Null
    }

    # ---- 2. this root's Serena ----------------------------------------------
    $root = $null
    $raw = [Console]::In.ReadToEnd()
    if ($raw) { try { $root = (ConvertFrom-Json $raw).cwd } catch { } }
    if (-not $root) { $root = (Get-Location).Path }

    $cfgPath = Join-Path $root '.mcp-shared.json'
    if (-not (Test-Path $cfgPath)) { exit 0 }

    $port = [int]((Get-Content $cfgPath -Raw | ConvertFrom-Json).serena.port)
    if ($port -le 0 -or (Test-Listening $port)) { exit 0 }

    $starter = Join-Path $HOME '.mcp-shared\bin\start-project-mcp.ps1'
    if (-not (Test-Path $starter)) { exit 0 }

    & $starter -ProjectRoot $root -NoRegister -Detached *> $null
} catch {
    # deliberately silent: see header
}
exit 0
