<#
  stop-shared-mcp.ps1 - stop services this kit started.

  Correction vs. the original kit script: a recorded PID is only the LAUNCHER. Measured on this
  machine, one shared Serena is serena.exe -> python.exe -> python.exe and the listening socket
  belongs to the deepest child; killing the recorded PID alone orphans the server and leaves the
  port held. This version walks the process tree from each recorded PID.

  Correction 2026-09-10 (this pass): the previous version's header claimed it "additionally matches
  by full command line" - the code did not. It killed whatever tree hung off the recorded PID. A
  .pid file outlives its process, and Windows reuses PIDs, so a stale file could point at an
  unrelated process and take its whole tree down with it. Three guards were added:

   1. IDENTITY. The recorded PID is killed only if its command line matches the pattern implied by
      the .pid file name (playwright-shared -> @playwright/mcp; serena-<id>-<port> -> serena.exe
      with that exact --port). A mismatch is a STALE record: nothing is killed, the file is pruned.
      Once the root is positively identified, its descendants are its own by construction - that is
      what makes killing the tree safe (a Playwright child Chrome does not carry the parent's
      command line and must still be stopped).
   2. SELF-PRESERVATION. The current process, its whole ancestor chain, and any claude.exe /
      Code.exe are never killed, whatever the tree says.
   3. -Prune drops stale .pid files without stopping anything.
#>
param([switch]$IncludeSerena, [switch]$WhatIf, [switch]$Prune)
$ErrorActionPreference = "Stop"
$pidDir = Join-Path $HOME ".mcp-shared\pids"
$all = Get-CimInstance Win32_Process | Select-Object ProcessId, ParentProcessId, Name, CommandLine
$byPid = @{}; $all | ForEach-Object { $byPid[[int]$_.ProcessId] = $_ }

# ---- guard 2: never kill ourselves, our ancestors, or a Claude/VS Code host -------------------
$protected = @{}
$cur = $PID
while ($byPid.ContainsKey($cur)) { $protected[$cur] = $true; $cur = [int]$byPid[$cur].ParentProcessId }
$all | Where-Object { $_.Name -in 'claude.exe', 'Code.exe' } | ForEach-Object { $protected[[int]$_.ProcessId] = $true }

# ---- guard 1: what command line must a given .pid file's process carry? -----------------------
function Get-ExpectedPattern([string]$Name) {
    if ($Name -eq 'playwright-shared') { return '@playwright.mcp' }
    if ($Name -match '^serena-.+-(\d+)$') { return ('serena.*--port\s+' + $Matches[1] + '\b') }
    return $null
}

function Get-Descendants([int]$RootPid) {
    $acc = New-Object System.Collections.Generic.List[object]
    $queue = New-Object System.Collections.Generic.Queue[int]
    $queue.Enqueue($RootPid)
    $seen = @{}
    while ($queue.Count -gt 0) {
        $c = $queue.Dequeue()
        if ($seen.ContainsKey($c)) { continue }
        $seen[$c] = $true
        if ($byPid.ContainsKey($c)) { $acc.Add($byPid[$c]) }
        foreach ($k in ($all | Where-Object { [int]$_.ParentProcessId -eq $c })) { $queue.Enqueue([int]$k.ProcessId) }
    }
    return $acc
}

if (-not (Test-Path $pidDir)) { Write-Host "No PID directory."; exit 0 }

foreach ($f in (Get-ChildItem $pidDir -Filter *.pid)) {
    $name = $f.BaseName
    if ($name -like 'serena-*' -and -not $IncludeSerena -and -not $Prune) { Write-Host "skip   $name (use -IncludeSerena)"; continue }

    $txt = (Get-Content $f.FullName -ErrorAction SilentlyContinue | Select-Object -First 1)
    if ($txt -notmatch '^\d+$') {
        Write-Host "prune  $name (unreadable PID file)"
        if (-not $WhatIf) { Remove-Item $f.FullName -Force -ErrorAction SilentlyContinue }
        continue
    }
    $rootPid = [int]$txt
    $root = $byPid[$rootPid]

    if (-not $root) {
        Write-Host "prune  $name (PID $rootPid no longer exists)"
        if (-not $WhatIf) { Remove-Item $f.FullName -Force -ErrorAction SilentlyContinue }
        continue
    }

    $pattern = Get-ExpectedPattern $name
    if ($pattern -and ($root.CommandLine -notmatch $pattern)) {
        Write-Warning "STALE  $name : PID $rootPid is now '$($root.Name)', which does not match /$pattern/. Nothing stopped; pruning the record."
        if (-not $WhatIf) { Remove-Item $f.FullName -Force -ErrorAction SilentlyContinue }
        continue
    }
    if (-not $pattern) {
        Write-Warning "SKIP   $name : no identity pattern known for this record. Refusing to stop PID $rootPid unverified."
        continue
    }
    if ($Prune) { Write-Host "ok     $name (PID $rootPid verified, left running)"; continue }

    # root positively identified -> its descendants are its own by construction
    $victims = Get-Descendants $rootPid
    foreach ($v in ($victims | Sort-Object { [int]$_.ProcessId } -Descending)) {
        $vpid = [int]$v.ProcessId
        if ($protected.ContainsKey($vpid)) { Write-Warning "protect $vpid $($v.Name) - not stopped"; continue }
        $cl = ($v.CommandLine -replace '\s+', ' ')
        if ($cl.Length -gt 90) { $cl = $cl.Substring(0, 90) }
        Write-Host ("stop   {0,-7} {1,-12} {2}" -f $vpid, $v.Name, $cl)
        if (-not $WhatIf) { Stop-Process -Id $vpid -Force -ErrorAction SilentlyContinue }
    }
    if (-not $WhatIf) { Remove-Item $f.FullName -Force -ErrorAction SilentlyContinue }
}
