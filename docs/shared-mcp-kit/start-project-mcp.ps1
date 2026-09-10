<#
  start-project-mcp.ps1 - one shared Serena per repo/worktree root.

  Corrections applied vs. the original kit script (all evidence-based, see MIGRATION-REPORT.md):
   1. Launch serena.exe directly instead of wrapping it in powershell.exe
      (the wrapper cost one extra process per root and hid the exit code).
   2. Pass --enable-web-dashboard false --enable-gui-log-window false explicitly,
      so a shared server can never pop a browser tab or a pywebview window.
   3. Register at Claude 'local' scope, NOT 'project' scope: 'project' writes into the
      repo's tracked .mcp.json, which would dirty git status in every root.
   4. Poll for the listening socket instead of a fixed sleep.
   5. Idempotent: an already-listening port is reused, never double-started.

  Corrections 2026-09-10 (this pass):
   6. MACHINE-WIDE LOCK. "Check the port, then start" is a race: two terminals opening the same
      root at the same moment both see a free port, both spawn Serena, one loses the bind and
      leaves a dead process behind. The check-and-start is now inside a named mutex per port, and
      the port is re-checked after the lock is taken (double-checked locking). The mutex is
      Local\ (per logon session), which is the scope Serena itself is shared at - no elevation.
   7. NO ORPHAN PID RECORD. The .pid file used to be written before the port was known to be up;
      a failed start left a record claiming a live service. The record is now written only after
      the socket is confirmed, and a failed spawn is torn down instead of being left running.
#>
param(
    [string]$ProjectRoot = (Get-Location).Path,
    [string]$SettingsFileName = ".mcp-shared.json",
    [switch]$NoRegister,
    # -Detached: spawn Serena with NO stream redirection, so it inherits none of the caller's handles.
    # Required when the caller's stdout is a pipe somebody waits on (a Claude Code hook, a command
    # substitution). Measured 2026-09-10: with redirection, Start-Process uses UseShellExecute=false
    # and bInheritHandles=TRUE, so the long-lived Serena inherits the caller's stdout pipe and holds
    # it open for its entire life -- Claude Code waited on that pipe and the session never started.
    # Cost: no serena-<id>-<port>.out/err.log. Serena keeps its own logs under ~/.serena/logs/<date>/.
    [switch]$Detached
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$settingsPath = Join-Path $ProjectRoot $SettingsFileName
if (-not (Test-Path $settingsPath)) { throw "Missing $SettingsFileName in project root: $ProjectRoot" }

$cfg = Get-Content $settingsPath -Raw | ConvertFrom-Json
if (-not $cfg.serena.enabled) { Write-Host "Serena disabled for $($cfg.projectId)"; exit 0 }

$port = [int]$cfg.serena.port
$id   = $cfg.projectId
$serena = Join-Path $HOME ".local/bin/serena.exe"
if (-not (Test-Path $serena)) { throw "serena.exe not found at $serena (install: uv tool install -p 3.13 serena-agent)" }

$logDir = Join-Path $HOME ".mcp-shared/logs"
$pidDir = Join-Path $HOME ".mcp-shared/pids"
New-Item -ItemType Directory -Force -Path $logDir, $pidDir | Out-Null
$pidFile = Join-Path $pidDir "serena-$id-$port.pid"

function Test-Listening([int]$Port) {
    return [bool](Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue)
}

# ---- correction 6: serialise check-and-start across every terminal on this logon session ------
$mutex = New-Object System.Threading.Mutex($false, "Local\mcp-shared-serena-$port")
$held = $false
try {
    try { $held = $mutex.WaitOne([TimeSpan]::FromSeconds(90)) }
    catch [System.Threading.AbandonedMutexException] { $held = $true }   # previous holder died: we own it now
    if (-not $held) { throw "Timed out waiting for the start lock on Serena port $port (another terminal is starting it)." }

    if (Test-Listening $port) {
        Write-Host "Serena already listening on $port (reused)"
    } else {
        $a = @('start-mcp-server', '--project', $ProjectRoot, '--context', 'claude-code',
               '--transport', 'streamable-http', '--host', '127.0.0.1', '--port', "$port",
               '--enable-web-dashboard', 'false', '--enable-gui-log-window', 'false')
        if ($Detached) {
            $p = Start-Process -FilePath $serena -ArgumentList $a -WindowStyle Hidden -PassThru
        } else {
            $p = Start-Process -FilePath $serena -ArgumentList $a -WindowStyle Hidden `
                 -RedirectStandardOutput (Join-Path $logDir "serena-$id-$port.out.log") `
                 -RedirectStandardError  (Join-Path $logDir "serena-$id-$port.err.log") -PassThru
        }

        $ok = $false
        for ($i = 0; $i -lt 40; $i++) {
            Start-Sleep -Milliseconds 1500
            if (Test-Listening $port) { $ok = $true; break }
            if ($p.HasExited) { break }
        }

        # ---- correction 7: never leave a record, or a process, behind a failed start ----------
        if (-not $ok) {
            Remove-Item $pidFile -Force -ErrorAction SilentlyContinue
            if (-not $p.HasExited) {
                Get-CimInstance Win32_Process |
                    Where-Object { [int]$_.ParentProcessId -eq [int]$p.Id } |
                    ForEach-Object { Stop-Process -Id ([int]$_.ProcessId) -Force -ErrorAction SilentlyContinue }
                Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
            }
            throw "Serena failed to listen on $port; the partial start was torn down. See $logDir/serena-$id-$port.err.log"
        }
        Set-Content -Path $pidFile -Value $p.Id
    }
}
finally {
    if ($held) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}

if (-not $NoRegister) {
    Push-Location $ProjectRoot
    try {
        claude mcp remove serena --scope local 2>$null | Out-Null
        claude mcp add --transport http --scope local serena "http://127.0.0.1:$port/mcp" | Out-Null
    } finally { Pop-Location }
}
Write-Host "Serena shared for ${id}: http://127.0.0.1:$port/mcp"
