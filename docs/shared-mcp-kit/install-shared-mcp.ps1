<#
  install-shared-mcp.ps1 - install the prerequisites. It does NOT start anything.

  Corrections vs. the original kit script:
   1. Install and start are separate. The original installed on every run, so a startup path that
      called it would re-resolve packages at boot.
   2. No `uv tool install --force`. Forcing reinstalls a working Serena on every run and can lose a
      local configuration; the script reports what is present and upgrades only when asked.
   3. No `serena init`. ~/.serena/serena_config.yml may already exist and re-initialising clobbers it.
   4. @playwright/mcp is installed GLOBALLY at a resolved version rather than run through
      `npx -y ...@latest`. Measured 2026-09-10: under a Scheduled Task the npx resolution hung
      indefinitely - npx-cli.js alive, port never bound, logs empty. A pinned global install also
      means an upstream release lands when someone chooses it, not silently at the next logon.
   5. mcp-gway is not installed: the gateway is deferred (see shared-mcp.settings.json).
#>
param([switch]$Upgrade)

$ErrorActionPreference = "Stop"

function Require-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) { throw "Missing required command: $Name" }
}

Write-Host "== Shared MCP prerequisites =="
Require-Command "node"
Require-Command "npm"
Require-Command "uv"
Require-Command "claude"

# ---- Serena -----------------------------------------------------------------------------------
$serenaExe = Join-Path $HOME ".local/bin/serena.exe"
if ((Test-Path $serenaExe) -and -not $Upgrade) {
    Write-Host "Serena present: $serenaExe (re-run with -Upgrade to update)"
} else {
    Write-Host "Installing/updating Serena..."
    uv tool install -p 3.13 serena-agent @(if ($Upgrade) { '--force' })
}

# ---- Playwright MCP ---------------------------------------------------------------------------
$cli = Join-Path $env:APPDATA "npm/node_modules/@playwright/mcp/cli.js"
if ((Test-Path $cli) -and -not $Upgrade) {
    Write-Host "@playwright/mcp present: $cli (re-run with -Upgrade to update)"
} else {
    Write-Host "Installing/updating @playwright/mcp globally..."
    npm install -g @playwright/mcp
}

# ---- state directories ------------------------------------------------------------------------
$homeDir = Join-Path $HOME ".mcp-shared"
New-Item -ItemType Directory -Force -Path `
    $homeDir, (Join-Path $homeDir "logs"), (Join-Path $homeDir "pids"), `
    (Join-Path $homeDir "bin"), (Join-Path $homeDir "backups"), (Join-Path $homeDir "playwright-workdir") | Out-Null

if (-not (Test-Path (Join-Path $homeDir "ports.json"))) {
    Copy-Item (Join-Path $PSScriptRoot "ports.template.json") (Join-Path $homeDir "ports.json")
    Write-Host "Created ~/.mcp-shared/ports.json from the template - edit it before starting anything."
}

Write-Host ""
Write-Host "Prerequisites installed. Nothing is running yet. Next:"
Write-Host "  1) Copy this kit's *.ps1 to ~/.mcp-shared/bin/"
Write-Host "  2) Edit ~/.mcp-shared/ports.json: one port per simultaneously active root"
Write-Host "  3) Drop .mcp-shared.json in each root (see project-mcp.settings.template.json)"
Write-Host "  4) Run start-shared-mcp.ps1, then start-project-mcp.ps1 from inside a root"
Write-Host "  5) Verify with status-shared-mcp.ps1"
