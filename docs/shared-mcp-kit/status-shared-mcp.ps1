$ports = 8080, 8931
Write-Host "== Shared MCP listening ports =="
foreach ($port in $ports) {
    $rows = Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue
    if ($rows) {
        foreach ($r in $rows) {
            $p = Get-Process -Id $r.OwningProcess -ErrorAction SilentlyContinue
            [pscustomobject]@{
                Port = $port
                PID = $r.OwningProcess
                Process = if ($p) { $p.ProcessName } else { "?" }
                Status = "LISTEN"
            }
        }
    } else {
        [pscustomobject]@{ Port=$port; PID=""; Process=""; Status="DOWN" }
    }
} | Format-Table -AutoSize

Write-Host ""
Write-Host "== Claude MCP configuration =="
claude mcp list

Write-Host ""
Write-Host "== MCP-like process counts =="
Get-CimInstance Win32_Process |
Where-Object { $_.CommandLine -match 'mcp|serena|episodic|playwright|superpowers' } |
Group-Object Name |
Sort-Object Count -Descending |
Select-Object Count, Name |
Format-Table -AutoSize
