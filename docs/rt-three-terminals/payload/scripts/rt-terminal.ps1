param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("DEV", "VALIDATION", "EDITOR")]
    [string]$Role,

    [string]$WorkspaceRoot = (Get-Location).Path
)

$WorkspaceRoot = [System.IO.Path]::GetFullPath($WorkspaceRoot)

$global:RTTerminalRole = $Role
$global:RTWorkspaceRoot = $WorkspaceRoot
$env:RT_TERMINAL_ROLE = $Role
$env:RT_WORKSPACE_ROOT = $WorkspaceRoot

Set-Location $WorkspaceRoot

function global:Get-RTEngineMode {
    $stateFile = Join-Path $global:RTWorkspaceRoot ".vscode\rt-engine-mode.txt"
    if (-not (Test-Path $stateFile)) {
        return "DEV"
    }

    $mode = (Get-Content $stateFile -Raw).Trim().ToUpperInvariant()
    if ($mode -notin @("DEV", "VALIDATION", "EDITOR")) {
        return "DEV"
    }

    return $mode
}

function global:rtmode {
    param(
        [ValidateSet("DEV", "VALIDATION", "EDITOR", "STATUS")]
        [string]$Mode = "STATUS"
    )

    $script = Join-Path $global:RTWorkspaceRoot "scripts\rt-mode.ps1"
    & $script -Mode $Mode -WorkspaceRoot $global:RTWorkspaceRoot
}

function global:rtstatus {
    $mode = Get-RTEngineMode
    Write-Host "Terminal role : " -NoNewline
    $roleColor = switch ($global:RTTerminalRole) {
        "DEV"        { "Green" }
        "VALIDATION" { "Yellow" }
        "EDITOR"     { "Blue" }
    }
    Write-Host $global:RTTerminalRole -ForegroundColor $roleColor

    Write-Host "Engine mode   : " -NoNewline
    $modeColor = switch ($mode) {
        "DEV"        { "Green" }
        "VALIDATION" { "Yellow" }
        "EDITOR"     { "Blue" }
    }
    Write-Host $mode -ForegroundColor $modeColor
}

function global:rtsuite {
    if ($global:RTTerminalRole -ne "VALIDATION") {
        Write-Host "BLOCKED: rtsuite si esegue solo dal terminale VALIDATION." -ForegroundColor Red
        return
    }

    $safeScript = Join-Path $global:RTWorkspaceRoot "scripts\rt-suite-safe.ps1"
    & $safeScript @args
}

function global:prompt {
    $mode = Get-RTEngineMode

    $roleColor = switch ($global:RTTerminalRole) {
        "DEV"        { "Green" }
        "VALIDATION" { "Yellow" }
        "EDITOR"     { "Blue" }
    }

    $modeColor = switch ($mode) {
        "DEV"        { "Green" }
        "VALIDATION" { "Yellow" }
        "EDITOR"     { "Blue" }
        default      { "Gray" }
    }

    Write-Host "[" -NoNewline -ForegroundColor DarkGray
    Write-Host $global:RTTerminalRole -NoNewline -ForegroundColor $roleColor
    Write-Host "] " -NoNewline -ForegroundColor DarkGray

    Write-Host "[ENGINE:" -NoNewline -ForegroundColor DarkGray
    Write-Host $mode -NoNewline -ForegroundColor $modeColor
    Write-Host "] " -NoNewline -ForegroundColor DarkGray

    $cwd = (Get-Location).Path
    Write-Host $cwd -ForegroundColor DarkGray

    return "> "
}

Clear-Host
Write-Host "Refactor Tactics terminal" -ForegroundColor White
Write-Host "ROLE: " -NoNewline

switch ($Role) {
    "DEV" {
        Write-Host "DEV" -ForegroundColor Green
        Write-Host "Consentito: codice, test authoring, review, git, tooling statico/headless." -ForegroundColor Green
        Write-Host "Non avviare UnrealEditor, UnrealEditor-Cmd, rt-suite, packaging o build che richiedono Unreal libero." -ForegroundColor DarkGreen
    }
    "VALIDATION" {
        Write-Host "VALIDATION" -ForegroundColor Yellow
        Write-Host "Usa rtmode VALIDATION prima delle validazioni Unreal." -ForegroundColor Yellow
        Write-Host "Ordine: static -> build -> targeted -> scenario -> full suite x1 per batch." -ForegroundColor DarkYellow
        Write-Host "Comando protetto: rtsuite <argomenti di rt-suite.ps1>" -ForegroundColor DarkYellow
    }
    "EDITOR" {
        Write-Host "EDITOR" -ForegroundColor Blue
        Write-Host "Usa rtmode EDITOR quando Editor/PIE/MCP appartengono all'utente o alla sessione editor." -ForegroundColor Blue
        Write-Host "Non avviare suite/build/commandlet concorrenti. Save -> Stop PIE -> Close Editor -> VALIDATION." -ForegroundColor DarkCyan
    }
}

Write-Host ""
rtstatus
Write-Host ""
Write-Host "Comandi: rtstatus | rtmode DEV | rtmode VALIDATION | rtmode EDITOR | rtsuite ..." -ForegroundColor Gray
