<#
  status-shared-mcp.ps1 - what is actually running, and whether it is what we think it is.

  Correction vs. the original kit script: 8080 is NOT ours (com.docker.backend owns it on this
  machine) and the per-root Serena ports must come from the port registry, not be hard-coded.

  Corrections 2026-09-10 (this pass):
   * A listening port proves a socket is held, not that OUR service holds it. Each port is now
      matched against the command line the registry implies, and reported OK / FOREIGN / DOWN.
   * `Group-Object Name` counted processes, which is the number that misled everyone: one shared
      Serena is THREE processes (serena.exe -> python -> python) and only the deepest one listens.
      The inventory now separates SERVERS from the processes that implement them, and splits
      per-session stdio servers from machine-wide shared ones.
   * Stale .pid records are surfaced here rather than being discovered by the stop script.
#>
$reg = Join-Path $HOME ".mcp-shared\ports.json"
$pidDir = Join-Path $HOME ".mcp-shared\pids"
$all = Get-CimInstance Win32_Process | Select-Object ProcessId, ParentProcessId, Name, CommandLine
$byPid = @{}; $all | ForEach-Object { $byPid[[int]$_.ProcessId] = $_ }

# ---------- 1. ports, with identity ----------------------------------------------------------
$expect = @{ 8931 = @{ Label = 'playwright (shared)'; Pattern = '@playwright.mcp' } }
if (Test-Path $reg) {
    foreach ($r in ((Get-Content $reg -Raw | ConvertFrom-Json).roots)) {
        $expect[[int]$r.port] = @{ Label = "serena ($($r.projectId), $($r.status))"; Pattern = ('serena.*--port\s+' + [int]$r.port + '\b') }
    }
}

Write-Host "== Shared MCP ports (identity-checked) =="
$expect.Keys | Sort-Object | ForEach-Object {
    $port = $_
    $row = Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $row) {
        [pscustomobject]@{ Port = $port; Bind = ''; PID = ''; Process = ''; Identity = 'DOWN'; Service = $expect[$port].Label }
    } else {
        $pr = $byPid[[int]$row.OwningProcess]
        $ok = $pr -and ($pr.CommandLine -match $expect[$port].Pattern)
        [pscustomobject]@{
            Port = $port; Bind = $row.LocalAddress; PID = $row.OwningProcess
            Process  = $(if ($pr) { $pr.Name } else { '?' })
            Identity = $(if ($ok) { 'OK' } else { 'FOREIGN' })
            Service  = $expect[$port].Label
        }
    }
} | Format-Table -AutoSize

# ---------- 2. servers, not processes --------------------------------------------------------
# Each row is ONE MCP server. Procs = how many OS processes implement it.
$servers = New-Object System.Collections.Generic.List[object]

foreach ($s in ($all | Where-Object { $_.Name -eq 'serena.exe' })) {
    $port = if ($s.CommandLine -match '--port\s+(\d+)') { $Matches[1] } else { '?' }
    $proj = if ($s.CommandLine -match '--project\s+(\S+)') { Split-Path $Matches[1] -Leaf } else { '?' }
    $kids = @($all | Where-Object { [int]$_.ParentProcessId -eq [int]$s.ProcessId })
    $gkids = @($all | Where-Object { $kids.ProcessId -contains $_.ParentProcessId })
    $servers.Add([pscustomobject]@{ Family = 'serena'; Scope = "root:$proj"; Transport = "http:$port"; RootPID = $s.ProcessId; Procs = 1 + $kids.Count + $gkids.Count })
}
foreach ($s in ($all | Where-Object { $_.CommandLine -match '@playwright.mcp' -and $_.Name -eq 'node.exe' })) {
    $port = if ($s.CommandLine -match '--port\s+(\d+)') { $Matches[1] } else { '?' }
    $kids = @($all | Where-Object { [int]$_.ParentProcessId -eq [int]$s.ProcessId })
    $servers.Add([pscustomobject]@{ Family = 'playwright'; Scope = 'machine'; Transport = "http:$port"; RootPID = $s.ProcessId; Procs = 1 + $kids.Count })
}
# stdio servers: one per Claude session, parented by claude.exe
$claude = @($all | Where-Object { $_.Name -eq 'claude.exe' })
foreach ($c in $claude) {
    foreach ($k in ($all | Where-Object { [int]$_.ParentProcessId -eq [int]$c.ProcessId })) {
        $fam = $null
        if ($k.CommandLine -match 'superpowers-chrome') { $fam = 'superpowers-chrome' }
        elseif ($k.CommandLine -match 'episodic-memory') { $fam = 'episodic-memory' }
        if (-not $fam) { continue }
        $kids = @($all | Where-Object { [int]$_.ParentProcessId -eq [int]$k.ProcessId })
        $servers.Add([pscustomobject]@{ Family = $fam; Scope = "session:$($c.ProcessId)"; Transport = 'stdio'; RootPID = $k.ProcessId; Procs = 1 + $kids.Count })
    }
}

Write-Host ""
Write-Host "== MCP servers (one row = one server) =="
$servers | Sort-Object Family, Scope | Format-Table -AutoSize

Write-Host ""
Write-Host "== Totals =="
[pscustomobject]@{
    ClaudeSessions = $claude.Count
    SharedServers  = @($servers | Where-Object { $_.Transport -like 'http:*' }).Count
    StdioServers   = @($servers | Where-Object { $_.Transport -eq 'stdio' }).Count
    ServerProcs    = ($servers | Measure-Object Procs -Sum).Sum
} | Format-List

# ---------- 3. stale PID records -------------------------------------------------------------
if (Test-Path $pidDir) {
    $stale = @(Get-ChildItem $pidDir -Filter *.pid | ForEach-Object {
        $t = (Get-Content $_.FullName -ErrorAction SilentlyContinue | Select-Object -First 1)
        $pat = if ($_.BaseName -eq 'playwright-shared') { '@playwright.mcp' }
               elseif ($_.BaseName -match '^serena-.+-(\d+)$') { 'serena.*--port\s+' + $Matches[1] + '\b' } else { $null }
        $pr = if ($t -match '^\d+$') { $byPid[[int]$t] } else { $null }
        if (-not $pr -or ($pat -and $pr.CommandLine -notmatch $pat)) {
            [pscustomobject]@{ Record = $_.BaseName; RecordedPID = $t; NowRunning = $(if ($pr) { $pr.Name } else { '<gone>' }) }
        }
    })
    if ($stale) {
        Write-Host ""
        Write-Host "== STALE PID records (run stop-shared-mcp.ps1 -Prune) =="
        $stale | Format-Table -AutoSize
    }
}
