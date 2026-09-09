param(
    [switch]$SkipMemoryInstall
)

$ErrorActionPreference = "Stop"

function Require-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Missing required command: $Name"
    }
}

Write-Host "== Shared MCP prerequisites =="
Require-Command "node"
Require-Command "npx"
Require-Command "python"
Require-Command "pip"
Require-Command "uv"
Require-Command "claude"

Write-Host "Installing/updating Serena..."
uv tool install -p 3.13 serena-agent --force
serena init

Write-Host "Installing/updating MCP gateway..."
python -m pip install --upgrade mcp-gway

if (-not $SkipMemoryInstall) {
    Write-Host "Installing/updating Episodic Memory CLI..."
    npm install -g github:obra/episodic-memory
}

$homeDir = Join-Path $HOME ".mcp-shared"
$logDir = Join-Path $homeDir "logs"
New-Item -ItemType Directory -Force -Path $homeDir, $logDir | Out-Null

Write-Host ""
Write-Host "Installed prerequisites."
Write-Host "Next:"
Write-Host "  1) Run .\start-shared-mcp.ps1"
Write-Host "  2) Run .\status-shared-mcp.ps1"
Write-Host "  3) Do NOT disable existing Claude plugins yet."
