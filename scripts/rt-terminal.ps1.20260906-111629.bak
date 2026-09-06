param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("DEV", "VALIDATION", "EDITOR")]
    [string]$Role,

    [string]$WorkspaceRoot = (Get-Location).Path,

    # Solo identificativo visuale. Non crea isolamento o un worktree.
    [string]$InstanceId = ""
)

$WorkspaceRoot = [System.IO.Path]::GetFullPath($WorkspaceRoot)

if ([string]::IsNullOrWhiteSpace($InstanceId)) {
    $InstanceId = "$PID"
}

$global:RTTerminalRole = $Role
$global:RTTerminalInstance = $InstanceId
$global:RTWorkspaceRoot = $WorkspaceRoot

$env:RT_TERMINAL_ROLE = $Role
$env:RT_TERMINAL_INSTANCE = $InstanceId
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

    Write-Host "Terminal role : " -NoNewline
    Write-Host $global:RTTerminalRole -ForegroundColor $roleColor

    Write-Host "Terminal id   : " -NoNewline
    Write-Host "$($global:RTTerminalRole):$($global:RTTerminalInstance)" -ForegroundColor $roleColor

    Write-Host "Engine mode   : " -NoNewline
    Write-Host $mode -ForegroundColor $modeColor

    Write-Host "Workspace     : " -NoNewline
    Write-Host $global:RTWorkspaceRoot -ForegroundColor DarkGray
}

function global:rtsuite {
    if ($global:RTTerminalRole -ne "VALIDATION") {
        Write-Host "BLOCKED: rtsuite si esegue solo da un terminale con ruolo VALIDATION." -ForegroundColor Red
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
    Write-Host "$($global:RTTerminalRole):$($global:RTTerminalInstance)" -NoNewline -ForegroundColor $roleColor
    Write-Host "] " -NoNewline -ForegroundColor DarkGray

    Write-Host "[ENGINE:" -NoNewline -ForegroundColor DarkGray
    Write-Host $mode -NoNewline -ForegroundColor $modeColor
    Write-Host "] " -NoNewline -ForegroundColor DarkGray

    $cwd = (Get-Location).Path
    Write-Host $cwd -ForegroundColor DarkGray

    return "> "
}

Clear-Host
Write-Host "Refactor Tactics terminal role" -ForegroundColor White
Write-Host "ROLE INSTANCE: " -NoNewline

switch ($Role) {
    "DEV" {
        Write-Host "DEV:$InstanceId" -ForegroundColor Green
        Write-Host "Sono consentite piu' istanze DEV nello stesso checkout." -ForegroundColor Green
        Write-Host "Coordina ownership dei file. Evita git add -A/reset/restore/clean/switch/rebase mentre altri DEV hanno modifiche." -ForegroundColor DarkGreen
        Write-Host "Non avviare UnrealEditor, UnrealEditor-Cmd, rt-suite, packaging o build che monopolizzano Unreal." -ForegroundColor DarkGreen
    }
    "VALIDATION" {
        Write-Host "VALIDATION:$InstanceId" -ForegroundColor Yellow
        Write-Host "Possono esistere piu' terminali VALIDATION, ma un solo job Unreal deve essere attivo alla volta." -ForegroundColor Yellow
        Write-Host "Usa rtmode VALIDATION prima delle validazioni Unreal." -ForegroundColor Yellow
        Write-Host "Ordine: static -> build -> targeted -> scenario -> full suite x1 per batch." -ForegroundColor DarkYellow
        Write-Host "Comando protetto: rtsuite <argomenti di rt-suite.ps1>" -ForegroundColor DarkYellow
    }
    "EDITOR" {
        Write-Host "EDITOR:$InstanceId" -ForegroundColor Blue
        Write-Host "Normalmente una sola sessione Editor attiva e un solo writer .uasset/.umap per checkout." -ForegroundColor Blue
        Write-Host "Usa rtmode EDITOR quando Editor/PIE/MCP appartengono alla sessione editor." -ForegroundColor Blue
        Write-Host "Non avviare suite/build/commandlet concorrenti. Save -> Stop PIE -> Close Editor -> VALIDATION se serve." -ForegroundColor DarkCyan
    }
}

Write-Host ""
rtstatus
Write-Host ""
Write-Host "Comandi: rtstatus | rtmode DEV | rtmode VALIDATION | rtmode EDITOR | rtsuite ..." -ForegroundColor Gray
