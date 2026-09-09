$pidDir = Join-Path $HOME ".mcp-shared\pids"

if (-not (Test-Path $pidDir)) {
    Write-Host "No shared MCP PID directory found."
    exit 0
}

Get-ChildItem $pidDir -Filter "*.pid" | ForEach-Object {
    $name = $_.BaseName
    $pidText = Get-Content $_.FullName -ErrorAction SilentlyContinue
    if ($pidText -match '^\d+$') {
        $proc = Get-Process -Id ([int]$pidText) -ErrorAction SilentlyContinue
        if ($proc) {
            Write-Host "Stopping $name (PID $pidText)"
            Stop-Process -Id ([int]$pidText) -Force
        }
    }
    Remove-Item $_.FullName -Force -ErrorAction SilentlyContinue
}
