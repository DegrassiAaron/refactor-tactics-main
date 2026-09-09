param(
    [string]$SettingsPath = (Join-Path $PSScriptRoot "shared-mcp.settings.json")
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $SettingsPath)) {
    throw "Settings file not found: $SettingsPath"
}

$cfg = Get-Content $SettingsPath -Raw | ConvertFrom-Json
$homeDir = Join-Path $HOME ".mcp-shared"
$logDir = Join-Path $homeDir "logs"
$pidDir = Join-Path $homeDir "pids"
New-Item -ItemType Directory -Force -Path $logDir, $pidDir | Out-Null

function Test-Port([int]$Port) {
    return [bool](Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue)
}

function Start-LoggedPowerShellProcess([string]$Name, [string]$Command, [int]$Port) {
    if (Test-Port $Port) {
        Write-Host "$Name already listening on port $Port"
        return
    }

    $stdout = Join-Path $logDir "$Name.out.log"
    $stderr = Join-Path $logDir "$Name.err.log"

    $p = Start-Process -FilePath "powershell.exe" `
        -ArgumentList @("-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", $Command) `
        -WindowStyle Hidden `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -PassThru

    Set-Content -Path (Join-Path $pidDir "$Name.pid") -Value $p.Id
    Start-Sleep -Seconds 2

    if (Test-Port $Port) {
        Write-Host "$Name started on port $Port (PID $($p.Id))"
    } else {
        Write-Warning "$Name did not start listening on port $Port. Check $stderr"
    }
}

if ($cfg.playwright.enabled) {
    $pwPort = [int]$cfg.playwright.port
    $pwCmd = "npx -y @playwright/mcp@latest --port $pwPort"
    Start-LoggedPowerShellProcess "playwright-shared" $pwCmd $pwPort
}

# Configure the gateway registry idempotently.
# Existing entries are removed before being re-added; failures on missing entries are ignored.
try { mcp-gway remove playwright 2>$null | Out-Null } catch {}
if ($cfg.playwright.enabled) {
    mcp-gway add playwright --type remote --url $cfg.playwright.endpoint
}

# Episodic Memory backend is intentionally only registered here.
# DO NOT disable the Claude plugin MCP until its hooks/MCP split has been audited.
if ($cfg.episodicMemory.enabled -and (Get-Command $cfg.episodicMemory.command -ErrorAction SilentlyContinue)) {
    try { mcp-gway remove $cfg.episodicMemory.gatewayBackendName 2>$null | Out-Null } catch {}
    mcp-gway add $cfg.episodicMemory.gatewayBackendName --type local --command $cfg.episodicMemory.command
} elseif ($cfg.episodicMemory.enabled) {
    Write-Warning "Episodic Memory MCP command not found; gateway will start without it."
}

$gwPort = [int]$cfg.gatewayPort
$gwHost = [string]$cfg.bindHost
$gwCmd = "mcp-gway serve --host $gwHost --port $gwPort"
Start-LoggedPowerShellProcess "mcp-gateway" $gwCmd $gwPort

Write-Host ""
Write-Host "Shared MCP endpoint: http://$gwHost`:$gwPort/mcp"
Write-Host "Dashboard (if supported by installed mcp-gway): http://$gwHost`:$gwPort/dashboard"
