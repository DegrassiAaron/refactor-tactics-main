<#
  start-shared-mcp.ps1 - machine-wide shared MCP services.

  Corrections vs. the original kit script (evidence in ~/.mcp-shared/MIGRATION-REPORT.md):

   1. --isolated is REQUIRED. Without it @playwright/mcp refuses a second concurrent client:
      "Browser is already in use for ...ms-playwright-mcp/mcp-chrome-<id>, use --isolated".
      Measured 2026-09-10: two concurrent clients -> one served, one hard error.
   2. NEUTRAL working directory. browser_snapshot writes .playwright-mcp/page-*.yml relative to
      the server cwd; started from a repo, it pollutes that repo.
   3. Register Claude against http://localhost:8931/mcp, NOT 127.0.0.1: @playwright/mcp enforces a
      Host check and answers 403 "Access is only allowed at localhost:8931" to a 127.0.0.1 Host
      header. The listening socket still binds to 127.0.0.1 only.
   4. No mcp-gway gateway. Port 8080 is held by com.docker.backend on this machine, and after the
      Episodic Memory audit no backend was left that a gateway would help; fronting Playwright
      through it would only rename its tools. Deferred, not deleted - see the report.
   5. Launch node on the GLOBALLY installed cli.js instead of `npx -y @playwright/mcp@latest`.
      Measured 2026-09-10: under a Scheduled Task the npx resolution hung indefinitely (npx-cli.js
      alive, port never bound, logs empty) while the same command from an interactive shell bound
      in ~3s. A fixed path also drops the cmd.exe + npx-cli.js wrappers: 1 process instead of 5.
      It also pins the version: `@latest` would re-resolve on every logon, so a broken upstream
      release would land silently at boot instead of when someone chose to upgrade.

  Corrections 2026-09-10 (this pass):
   6. MACHINE-WIDE LOCK, as in start-project-mcp.ps1. The Scheduled Task fires at logon and the
      SessionStart hook can fire at the same moment; both used to check the port and both could
      spawn. Check-and-start now happens under a named mutex, with the port re-checked inside it.
   7. NO ORPHAN PID RECORD. The .pid file is written only once the socket is confirmed, and a
      failed spawn is torn down rather than left running.
#>
param([int]$PlaywrightPort = 8931)
$ErrorActionPreference = "Stop"

$root    = Join-Path $HOME ".mcp-shared"
$logDir  = Join-Path $root "logs"
$pidDir  = Join-Path $root "pids"
$workDir = Join-Path $root "playwright-workdir"
New-Item -ItemType Directory -Force -Path $logDir, $pidDir, $workDir | Out-Null
$pidFile = Join-Path $pidDir "playwright-shared.pid"

function Test-Listening([int]$Port) {
    return [bool](Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue)
}

$cli = Join-Path $env:APPDATA "npm/node_modules/@playwright/mcp/cli.js"
if (-not (Test-Path $cli)) { throw "@playwright/mcp is not installed globally. Run: npm install -g @playwright/mcp" }

$mutex = New-Object System.Threading.Mutex($false, "Local\mcp-shared-playwright-$PlaywrightPort")
$held = $false
try {
    try { $held = $mutex.WaitOne([TimeSpan]::FromSeconds(90)) }
    catch [System.Threading.AbandonedMutexException] { $held = $true }
    if (-not $held) { throw "Timed out waiting for the start lock on Playwright port $PlaywrightPort." }

    if (Test-Listening $PlaywrightPort) {
        Write-Host "playwright-shared already listening on $PlaywrightPort"
        exit 0
    }

    $p = Start-Process -FilePath "node" `
         -ArgumentList @($cli, '--port', "$PlaywrightPort", '--host', '127.0.0.1', '--isolated') `
         -WorkingDirectory $workDir -WindowStyle Hidden `
         -RedirectStandardOutput (Join-Path $logDir "playwright-shared.out.log") `
         -RedirectStandardError  (Join-Path $logDir "playwright-shared.err.log") -PassThru

    $ok = $false
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 1500
        if (Test-Listening $PlaywrightPort) { $ok = $true; break }
        if ($p.HasExited) { break }
    }
    if (-not $ok) {
        Remove-Item $pidFile -Force -ErrorAction SilentlyContinue
        if (-not $p.HasExited) {
            Get-CimInstance Win32_Process |
                Where-Object { [int]$_.ParentProcessId -eq [int]$p.Id } |
                ForEach-Object { Stop-Process -Id ([int]$_.ProcessId) -Force -ErrorAction SilentlyContinue }
            Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        }
        throw "playwright-shared failed to listen on $PlaywrightPort; the partial start was torn down. See $logDir/playwright-shared.err.log"
    }
    Set-Content -Path $pidFile -Value $p.Id
    Write-Host "playwright-shared started on $PlaywrightPort (PID $($p.Id))"
}
finally {
    if ($held) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
