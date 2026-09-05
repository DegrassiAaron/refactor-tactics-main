# Thin local guard around the repository's existing scripts/rt-suite.ps1.
# It does NOT replace rt-suite and does not change its measurement-validity semantics.

$ErrorActionPreference = "Stop"

$workspaceRoot = $env:RT_WORKSPACE_ROOT
if ([string]::IsNullOrWhiteSpace($workspaceRoot)) {
    $workspaceRoot = (Get-Location).Path
}

$workspaceRoot = [System.IO.Path]::GetFullPath($workspaceRoot)
$role = $env:RT_TERMINAL_ROLE

if ($role -ne "VALIDATION") {
    Write-Host "BLOCKED: rt-suite-safe richiede RT_TERMINAL_ROLE=VALIDATION." -ForegroundColor Red
    exit 2
}

$stateFile = Join-Path $workspaceRoot ".vscode\rt-engine-mode.txt"
$mode = "DEV"

if (Test-Path $stateFile) {
    $readMode = (Get-Content $stateFile -Raw).Trim().ToUpperInvariant()
    if ($readMode -in @("DEV", "VALIDATION", "EDITOR")) {
        $mode = $readMode
    }
}

if ($mode -ne "VALIDATION") {
    Write-Host "BLOCKED: ENGINE MODE e' $mode, non VALIDATION." -ForegroundColor Red
    Write-Host "Esegui: rtmode VALIDATION" -ForegroundColor Yellow
    exit 2
}

$suite = Join-Path $workspaceRoot "scripts\rt-suite.ps1"
if (-not (Test-Path $suite)) {
    Write-Host "ERROR: scripts\rt-suite.ps1 non trovato." -ForegroundColor Red
    exit 2
}

Write-Host "VALIDATION WINDOW attiva. Avvio rt-suite con gli argomenti richiesti..." -ForegroundColor Yellow
& $suite @args
exit $LASTEXITCODE
