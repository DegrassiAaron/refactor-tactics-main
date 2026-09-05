param(
    [string]$RepoRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"

$RepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Payload = Join-Path $Here "payload"

$UProject = Join-Path $RepoRoot "RefactorTactics.uproject"
if (-not (Test-Path $UProject)) {
    throw "RepoRoot non sembra Refactor Tactics: manca $UProject"
}

$Targets = @(
    @(".vscode\settings.json", ".vscode\settings.json"),
    @(".vscode\tasks.json", ".vscode\tasks.json"),
    @("scripts\rt-terminal.ps1", "scripts\rt-terminal.ps1"),
    @("scripts\rt-mode.ps1", "scripts\rt-mode.ps1"),
    @("scripts\rt-suite-safe.ps1", "scripts\rt-suite-safe.ps1")
)

$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"

foreach ($Pair in $Targets) {
    $Source = Join-Path $Payload $Pair[0]
    $Target = Join-Path $RepoRoot $Pair[1]
    $TargetDir = Split-Path -Parent $Target

    New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null

    if (Test-Path $Target) {
        $Backup = "$Target.$Stamp.bak"
        Copy-Item $Target $Backup -Force
        Write-Host "Backup: $Backup" -ForegroundColor DarkGray
    }

    Copy-Item $Source $Target -Force
    Write-Host "Installed: $Target" -ForegroundColor Green
}

$StateFile = Join-Path $RepoRoot ".vscode\rt-engine-mode.txt"
if (-not (Test-Path $StateFile)) {
    Set-Content -Path $StateFile -Value "DEV" -Encoding ASCII
    Write-Host "Initialized ENGINE MODE: DEV" -ForegroundColor Green
}

Write-Host ""
Write-Host "Installazione completata." -ForegroundColor Green
Write-Host "In VS Code: Terminal -> Run Task -> 'RT: Open 3 terminals'" -ForegroundColor White
Write-Host "Oppure Ctrl+Shift+B (task build di default)." -ForegroundColor White
