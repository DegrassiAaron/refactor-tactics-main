param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("DEV", "VALIDATION", "EDITOR")]
    [string]$Role,

    [string]$WorkspaceRoot = (Get-Location).Path,

    # Solo identificativo visuale. Non crea isolamento o un worktree.
    [string]$InstanceId = "",

    # Task/issue su cui questa sessione lavora. Non obbligatorio per aprire un
    # terminale: lo diventa quando si acquisisce il motore per un'operazione mutante.
    [string]$TaskId = ""
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

# (!!) Identita' OS della sessione RT, ereditata dai child.
#
# Il lease appartiene a QUESTO terminale, che sopravvive ai comandi lanciati al suo
# interno. Prima, `rt-lease.ps1` scriveva come owner il PID del proprio processo:
# effimero, morto un istante dopo l'acquire, e il lease nasceva STALE.
#
# (!) Queste variabili sono SEPARATE da RT_TERMINAL_INSTANCE, che resta l'id logico
# scelto con -InstanceId. Confonderle rendeva -InstanceId inutile e faceva comparire
# un numero di processo nel prompt al posto del nome scelto.
#
# Lo start time non e' ridondante: un PID si ricicla, e senza di esso un processo
# nuovo che ne riusa il numero verrebbe scambiato per questa sessione.
$env:RT_TERMINAL_OWNER_PID = "$PID"
try {
    $env:RT_TERMINAL_OWNER_STARTED_AT = (Get-Process -Id $PID).StartTime.ToUniversalTime().ToString('o')
} catch {
    # Senza start time l'identita' e' incompleta e acquire/release falliscono
    # closed: e' il comportamento voluto, non una svista.
    $env:RT_TERMINAL_OWNER_STARTED_AT = ''
}

if (-not [string]::IsNullOrWhiteSpace($TaskId)) {
    $env:RT_TASK_ID = $TaskId
}

# L'identita' del WORKSPACE non e' il ruolo della SESSIONE. Si legge dal marker
# locale, che e' un promemoria: l'autorita' e' il registro per macchina, e chi deve
# decidere qualcosa chiede a `rt-workspace.ps1 -Action verify`.
$MarkerPath = Join-Path (Join-Path $WorkspaceRoot ".vscode") "rt-workspace-id.txt"
if (Test-Path $MarkerPath) {
    $markerValue = (Get-Content $MarkerPath -Raw).Trim().ToUpperInvariant()
    if ($markerValue -in @("MAIN", "DEV", "TECHNICAL_DESIGNER")) {
        $env:RT_WORKSPACE_ID = $markerValue
    }
}

Set-Location $WorkspaceRoot

# ---------------------------------------------------------------------------
# Shell degli script RT
# ---------------------------------------------------------------------------

# (!!) `scripts\rt-suite.ps1` e' UTF-8 SENZA BOM e contiene 1080 byte non-ASCII.
# Windows PowerShell 5.1 legge i file senza BOM come Windows-1252, e su quel file
# produce 26 errori di parsing: la suite NON parte da 5.1. Misurato il 2026-09-06.
# Gli script che la invocano o ne importano il guard vogliono quindi pwsh 7.
function global:Get-RTShell {
    $pwsh = (Get-Command pwsh -ErrorAction SilentlyContinue)
    if ($null -ne $pwsh) { return $pwsh.Source }
    return $null
}

function global:Invoke-RTScript {
    param(
        [Parameter(Mandatory)] [string] $Script,
        [Parameter(ValueFromRemainingArguments = $true)] $Rest
    )

    $path = Join-Path (Join-Path $global:RTWorkspaceRoot "scripts") $Script
    if (-not (Test-Path $path)) {
        Write-Host "ERROR: $path non trovato. Reinstalla il bundle rt-three-terminals." -ForegroundColor Red
        return
    }

    $shell = Get-RTShell
    if ($null -eq $shell) {
        Write-Host "BLOCKED: pwsh 7 non trovato sul PATH." -ForegroundColor Red
        Write-Host "scripts\rt-suite.ps1 non e' parsabile da Windows PowerShell 5.1 (UTF-8 senza BOM)." -ForegroundColor Yellow
        return
    }

    & $shell -NoLogo -NoProfile -File $path @Rest
}

# ---------------------------------------------------------------------------
# Comandi di sessione
# ---------------------------------------------------------------------------

function global:rtws {
    param(
        [ValidateSet("status", "register", "verify")] [string] $Action = "status",
        [ValidateSet("MAIN", "DEV", "TECHNICAL_DESIGNER")] [string] $WorkspaceId = "",
        [switch] $Force
    )
    $args2 = @("-Action", $Action, "-WorkspaceRoot", $global:RTWorkspaceRoot)
    if (-not [string]::IsNullOrWhiteSpace($WorkspaceId)) { $args2 += @("-WorkspaceId", $WorkspaceId) }
    if ($Force) { $args2 += "-Force" }
    Invoke-RTScript -Script "rt-workspace.ps1" @args2
}

function global:rtlease {
    param(
        [ValidateSet("status", "acquire", "release")] [string] $Action = "status",
        [ValidateSet("BUILD", "SUITE", "EDITOR", "PIE", "COMMANDLET", "MCP_EDITOR_QUERY", "MCP_ASSET_WRITE")] [string] $Operation = "",
        [string] $TaskId = "",
        [switch] $ReclaimStale
    )
    $args2 = @("-Action", $Action, "-WorkspaceRoot", $global:RTWorkspaceRoot)
    if (-not [string]::IsNullOrWhiteSpace($Operation)) { $args2 += @("-Operation", $Operation) }
    if (-not [string]::IsNullOrWhiteSpace($TaskId)) { $args2 += @("-TaskId", $TaskId) }
    if ($ReclaimStale) { $args2 += "-ReclaimStale" }
    Invoke-RTScript -Script "rt-lease.ps1" @args2
}

function global:rtbuild {
    <#
        Compila DENTRO il lease (#2529). `Build.bat` era il solo passo del gate che
        tocca il motore senza passare da nessun guard: ricompilare sotto la suite di
        un altro checkout rende NON VALIDA la sua misura.

        Non attende: se il motore e' occupato la build e' FERMATA, non sconsigliata.
    #>
    param(
        [ValidateSet("RefactorTacticsEditor", "RefactorTactics")] [string] $Target = "RefactorTacticsEditor",
        [ValidateSet("Development", "DebugGame", "Shipping")] [string] $Configuration = "Development",
        [string] $TaskId = "",
        [string[]] $ExtraArgs = @()
    )
    $args2 = @("-Target", $Target, "-Configuration", $Configuration)
    if (-not [string]::IsNullOrWhiteSpace($TaskId)) { $args2 += @("-TaskId", $TaskId) }
    if ($ExtraArgs.Count -gt 0) { $args2 += @("-ExtraArgs"); $args2 += $ExtraArgs }
    Invoke-RTScript -Script "rt-build.ps1" @args2
}

function global:rtmcp {
    param(
        [ValidateSet("MCP_EDITOR_QUERY", "MCP_ASSET_WRITE")] [string] $Operation = "MCP_ASSET_WRITE",
        [string] $TaskId = "",
        [string[]] $AssetWriteSet = @()
    )
    $args2 = @("-Action", "check", "-Operation", $Operation, "-WorkspaceRoot", $global:RTWorkspaceRoot)
    if (-not [string]::IsNullOrWhiteSpace($TaskId)) { $args2 += @("-TaskId", $TaskId) }
    if ($AssetWriteSet.Count -gt 0) { $args2 += @("-AssetWriteSet"); $args2 += $AssetWriteSet }
    Invoke-RTScript -Script "rt-mcp-guard.ps1" @args2
}

function global:rtstatus {
    $roleColor = switch ($global:RTTerminalRole) {
        "DEV"        { "Green" }
        "VALIDATION" { "Yellow" }
        "EDITOR"     { "Blue" }
    }

    Write-Host "Terminal role : " -NoNewline
    Write-Host $global:RTTerminalRole -ForegroundColor $roleColor

    Write-Host "Terminal id   : " -NoNewline
    Write-Host "$($global:RTTerminalRole):$($global:RTTerminalInstance)" -ForegroundColor $roleColor

    $ws = $env:RT_WORKSPACE_ID
    if ([string]::IsNullOrWhiteSpace($ws)) { $ws = "<non dichiarato>" }
    Write-Host "Workspace id  : " -NoNewline
    Write-Host $ws -ForegroundColor Cyan

    $task = $env:RT_TASK_ID
    if ([string]::IsNullOrWhiteSpace($task)) { $task = "<nessuno>" }
    Write-Host "Task id       : $task"

    Write-Host "Workspace     : $($global:RTWorkspaceRoot)" -ForegroundColor DarkGray
    Write-Host ""
    rtlease -Action status
}

function global:rtsuite {
    if ($global:RTTerminalRole -ne "VALIDATION") {
        Write-Host "BLOCKED: rtsuite si esegue solo da un terminale con ruolo VALIDATION." -ForegroundColor Red
        return
    }
    Invoke-RTScript -Script "rt-suite-safe.ps1" @args
}

function global:prompt {
    $roleColor = switch ($global:RTTerminalRole) {
        "DEV"        { "Green" }
        "VALIDATION" { "Yellow" }
        "EDITOR"     { "Blue" }
    }

    Write-Host "[" -NoNewline -ForegroundColor DarkGray
    Write-Host "$($global:RTTerminalRole):$($global:RTTerminalInstance)" -NoNewline -ForegroundColor $roleColor
    Write-Host "] " -NoNewline -ForegroundColor DarkGray

    $ws = $env:RT_WORKSPACE_ID
    if ([string]::IsNullOrWhiteSpace($ws)) { $ws = "?" }
    Write-Host "[WS:" -NoNewline -ForegroundColor DarkGray
    Write-Host $ws -NoNewline -ForegroundColor Cyan
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
        Write-Host "Possono esistere piu' terminali VALIDATION, ma un solo job Unreal alla volta." -ForegroundColor Yellow
        Write-Host "Prima di occupare il motore: rtlease -Action acquire -Operation SUITE" -ForegroundColor Yellow
        Write-Host "Ordine: static -> build -> targeted -> scenario -> full suite x1 per batch." -ForegroundColor DarkYellow
    }
    "EDITOR" {
        Write-Host "EDITOR:$InstanceId" -ForegroundColor Blue
        Write-Host "Il ruolo EDITOR esiste in ogni workspace. L'authoring asset via MCP no: solo da MAIN." -ForegroundColor Blue
        Write-Host "Preparazione e query read-only non richiedono MAIN." -ForegroundColor DarkCyan
        Write-Host "Prima di mutare: rtmcp -Operation MCP_ASSET_WRITE -TaskId <id> -AssetWriteSet <path>" -ForegroundColor DarkCyan
    }
}

Write-Host ""
Write-Host "(!) Aprire questo terminale NON acquisisce Unreal. Il lease si prende just-in-time." -ForegroundColor DarkGray
Write-Host ""
rtstatus
Write-Host ""
Write-Host "Comandi: rtstatus | rtws | rtlease | rtmcp | rtbuild | rtsuite ..." -ForegroundColor Gray
