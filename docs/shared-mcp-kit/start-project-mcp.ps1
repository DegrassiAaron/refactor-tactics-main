param(
    [string]$ProjectRoot = (Get-Location).Path,
    [string]$SettingsFileName = ".mcp-shared.json"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$settingsPath = Join-Path $ProjectRoot $SettingsFileName

if (-not (Test-Path $settingsPath)) {
    throw "Missing $SettingsFileName in project root: $ProjectRoot"
}

$cfg = Get-Content $settingsPath -Raw | ConvertFrom-Json
if (-not $cfg.serena.enabled) {
    Write-Host "Serena disabled for project $($cfg.projectId)"
    exit 0
}

$port = [int]$cfg.serena.port

$existing = Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue
if (-not $existing) {
    $homeDir = Join-Path $HOME ".mcp-shared"
    $logDir = Join-Path $homeDir "logs"
    $pidDir = Join-Path $homeDir "pids"
    New-Item -ItemType Directory -Force -Path $logDir, $pidDir | Out-Null

    $safeRoot = $ProjectRoot.Replace("'", "''")
    $cmd = "serena start-mcp-server --project '$safeRoot' --context claude-code --transport streamable-http --host 127.0.0.1 --port $port"
    $stdout = Join-Path $logDir "serena-$($cfg.projectId)-$port.out.log"
    $stderr = Join-Path $logDir "serena-$($cfg.projectId)-$port.err.log"

    $p = Start-Process -FilePath "powershell.exe" `
        -ArgumentList @("-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", $cmd) `
        -WindowStyle Hidden `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -PassThru

    Set-Content -Path (Join-Path $pidDir "serena-$($cfg.projectId)-$port.pid") -Value $p.Id
    Start-Sleep -Seconds 3
}

if (-not (Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue)) {
    throw "Serena failed to listen on port $port. Check ~/.mcp-shared/logs."
}

Push-Location $ProjectRoot
try {
    # Add/update Serena as PROJECT scoped MCP.
    # Remove first only if the existing entry is named exactly 'serena'.
    try { claude mcp remove serena --scope project 2>$null | Out-Null } catch {}
    claude mcp add --transport http --scope project serena "http://127.0.0.1:$port/mcp"
}
finally {
    Pop-Location
}

Write-Host "Serena shared for $($cfg.projectId): http://127.0.0.1:$port/mcp"
