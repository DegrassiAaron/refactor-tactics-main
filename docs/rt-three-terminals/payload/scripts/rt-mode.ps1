param(
    [ValidateSet("DEV", "VALIDATION", "EDITOR", "STATUS")]
    [string]$Mode = "STATUS",

    [string]$WorkspaceRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"
$WorkspaceRoot = [System.IO.Path]::GetFullPath($WorkspaceRoot)
$StateDir = Join-Path $WorkspaceRoot ".vscode"
$StateFile = Join-Path $StateDir "rt-engine-mode.txt"

function Get-RTMode {
    if (-not (Test-Path $StateFile)) {
        return "DEV"
    }

    $value = (Get-Content $StateFile -Raw).Trim().ToUpperInvariant()
    if ($value -notin @("DEV", "VALIDATION", "EDITOR")) {
        return "DEV"
    }

    return $value
}

function Write-Mode([string]$Value) {
    $color = switch ($Value) {
        "DEV"        { "Green" }
        "VALIDATION" { "Yellow" }
        "EDITOR"     { "Blue" }
        default      { "Gray" }
    }

    Write-Host "RT ENGINE MODE: " -NoNewline
    Write-Host $Value -ForegroundColor $color
}

if ($Mode -eq "STATUS") {
    $current = Get-RTMode
    Write-Mode $current
    exit 0
}

New-Item -ItemType Directory -Force -Path $StateDir | Out-Null
Set-Content -Path $StateFile -Value $Mode -Encoding ASCII

Write-Mode $Mode

switch ($Mode) {
    "DEV" {
        Write-Host "Unreal deve restare libero. Niente suite/build/commandlet avviati dagli agenti." -ForegroundColor DarkGreen
    }
    "VALIDATION" {
        Write-Host "Unreal appartiene alla coda di validazione: build -> targeted -> scenario -> full suite x1 per batch." -ForegroundColor DarkYellow
    }
    "EDITOR" {
        Write-Host "Unreal appartiene all'Editor/PIE/MCP/utente. Niente suite/build concorrenti." -ForegroundColor DarkCyan
    }
}
