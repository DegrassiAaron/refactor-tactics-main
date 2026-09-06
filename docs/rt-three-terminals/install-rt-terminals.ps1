<#
.SYNOPSIS
    Installa il bundle rt-three-terminals in un checkout di RefactorTactics.

.DESCRIPTION
    Tre ruoli di sessione (DEV, EDITOR, VALIDATION) sono disponibili in OGNI
    directory: nessun checkout e' vincolato a un ruolo, e non esiste un parametro
    che leghi una directory a un ruolo operativo.

    Cio' che invece distingue le directory e' l'IDENTITA' DEL WORKSPACE:

        MAIN                 ospita l'unico bridge MCP della macchina
        DEV                  sviluppo
        TECHNICAL_DESIGNER   design tecnico

    L'identita' e' obbligatoria (-WorkspaceId) e viene registrata nel registro per
    macchina sotto %LOCALAPPDATA%\RefactorTactics\RT3\, non dedotta dal nome della
    cartella.
#>
param(
    [string]$RepoRoot = (Get-Location).Path,

    [Parameter(Mandatory = $true)]
    [ValidateSet("MAIN", "DEV", "TECHNICAL_DESIGNER")]
    [string]$WorkspaceId,

    # Endpoint del bridge MCP. Uno solo per macchina: e' quello ospitato da MAIN, e
    # tutti i workspace che lo usano parlano a QUELL'Editor.
    [string]$McpEndpoint = "http://127.0.0.1:8765/mcp",

    # Non generare .mcp.json in questo checkout. Utile dove il bridge non serve.
    [switch]$NoMcp,

    # Consente di spostare l'identita' MAIN su questa root.
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$RepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Payload = Join-Path $Here "payload"

$UProject = Join-Path $RepoRoot "RefactorTactics.uproject"
if (-not (Test-Path $UProject)) {
    throw "RepoRoot non sembra Refactor Tactics: manca $UProject"
}

# La sorgente VS Code NON vive in payload/.vscode:
# .vscode/ e' ignorata dal repository. I template versionati stanno in payload/vscode/.
$Targets = @(
    @("vscode\settings.json", ".vscode\settings.json"),
    @("vscode\tasks.json", ".vscode\tasks.json"),
    @("scripts\rt-terminal.ps1", "scripts\rt-terminal.ps1"),
    @("scripts\rt-mode.ps1", "scripts\rt-mode.ps1"),
    @("scripts\rt-suite-safe.ps1", "scripts\rt-suite-safe.ps1"),
    @("scripts\rt-lease.ps1", "scripts\rt-lease.ps1"),
    @("scripts\rt-build.ps1", "scripts\rt-build.ps1"),
    @("scripts\rt-task-router.ps1", "scripts\rt-task-router.ps1"),
    @("scripts\rt-open-task.ps1", "scripts\rt-open-task.ps1"),
    @("scripts\rt-workspace.ps1", "scripts\rt-workspace.ps1"),
    @("scripts\rt-mcp-guard.ps1", "scripts\rt-mcp-guard.ps1"),
    @("scripts\rt-mcp-server.ps1", "scripts\rt-mcp-server.ps1")
)

# Preflight atomico: non iniziare una installazione parziale se il bundle e' incompleto.
$Missing = @()
foreach ($Pair in $Targets) {
    $Source = Join-Path $Payload $Pair[0]
    if (-not (Test-Path $Source)) {
        $Missing += $Source
    }
}

if ($Missing.Count -gt 0) {
    $List = ($Missing -join [Environment]::NewLine)
    throw "Bundle rt-three-terminals incompleto. File mancanti:`n$List"
}

$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"

function Backup-IfPresent {
    param([Parameter(Mandatory)] [string] $Target)
    if (Test-Path $Target) {
        $Backup = "$Target.$Stamp.bak"
        Copy-Item $Target $Backup -Force
        Write-Host "Backup: $Backup" -ForegroundColor DarkGray
        return $Backup
    }
    return $null
}

foreach ($Pair in $Targets) {
    $Source = Join-Path $Payload $Pair[0]
    $Target = Join-Path $RepoRoot $Pair[1]
    $TargetDir = Split-Path -Parent $Target

    New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null
    Backup-IfPresent -Target $Target | Out-Null
    Copy-Item $Source $Target -Force
    Write-Host "Installed: $Target" -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# Identita' del workspace
# ---------------------------------------------------------------------------

Write-Host ""
$WsScript = Join-Path (Join-Path $RepoRoot "scripts") "rt-workspace.ps1"
$WsArgs = @("-Action", "register", "-WorkspaceId", $WorkspaceId, "-WorkspaceRoot", $RepoRoot)
if ($Force) { $WsArgs += "-Force" }

$Shell = (Get-Command pwsh -ErrorAction SilentlyContinue)
if ($null -eq $Shell) {
    throw "pwsh 7 non trovato sul PATH: scripts\rt-suite.ps1 non e' parsabile da Windows PowerShell 5.1 e il lease ne importa il guard."
}

& $Shell.Source -NoLogo -NoProfile -File $WsScript @WsArgs
if ($LASTEXITCODE -ne 0) {
    throw "Registrazione del workspace fallita (exit $LASTEXITCODE). L'installazione dei file e' avvenuta; l'identita' no."
}

# ---------------------------------------------------------------------------
# Avvio automatico del bridge: acceso solo in MAIN
# ---------------------------------------------------------------------------

# (!!) Il bridge e' UNO per macchina. Un Editor aperto in un altro checkout, con
# `bAutoStartServer=True`, ne fa partire un SECONDO: nessuno lo usa - i client
# puntano tutti all'endpoint di MAIN - ma e' raggiungibile, e dietro `call_tool`
# espone 56 toolset fra cui `AutomationTestToolset` (`RunTests`, `StopTests`).
#
# Una chiamata a `RunTests` avvia una suite senza passare da `rt-suite.ps1`, dal suo
# mutex e dal lease; `StopTests` ferma quella di un altro. Spegnere l'avvio
# automatico fuori da MAIN chiude quel canale.
#
# Il setting vive in Saved/, che e' per utente e non versionato: e' una leva di
# MACCHINA, e va riapplicata su ogni checkout - non la porta un `git pull`.
$McpServerScript = Join-Path (Join-Path $RepoRoot "scripts") "rt-mcp-server.ps1"
$DesiredAutoStart = if ($WorkspaceId -eq "MAIN") { "On" } else { "Off" }

Write-Host ""
& $Shell.Source -NoLogo -NoProfile -File $McpServerScript -RepoRoot $RepoRoot -AutoStart $DesiredAutoStart
if ($LASTEXITCODE -eq 2) {
    Write-Host "(!) bAutoStartServer NON e' stato applicato. Chiudi l'Editor e riesegui:" -ForegroundColor Yellow
    Write-Host ("    .\scripts\rt-mcp-server.ps1 -RepoRoot `"{0}`" -AutoStart {1}" -f $RepoRoot, $DesiredAutoStart) -ForegroundColor DarkGray
}

# ---------------------------------------------------------------------------
# Configurazione MCP: locale, non versionata
# ---------------------------------------------------------------------------

# (!) `.mcp.json` E' VERSIONATO, ed e' una scelta rivista.
#
# Era stato tolto dal repository sul presupposto che la configurazione MCP variasse
# per macchina. Misurato il 2026-09-06 sui tre checkout: gli endpoint erano BYTE
# IDENTICI, perche' il bridge e' UNO ed e' ospitato da MAIN. Un file che non varia
# non e' una configurazione di macchina, e de-versionarlo aveva un costo concreto:
# ogni `git pull` lo cancellava dal working tree, rompendo il bridge finche' non
# veniva ripristinato a mano.
#
# L'installer lo riscrive solo se il contenuto cambia davvero: cosi' non lascia
# modifiche non committate in un checkout che era gia' allineato.
#
# Un endpoint diverso resta possibile con -McpEndpoint, e in quel caso il file
# risultera' modificato: e' corretto, perche' quella e' una divergenza reale che
# qualcuno deve decidere se committare.
if (-not $NoMcp) {
    $McpTarget = Join-Path $RepoRoot ".mcp.json"

    $McpConfig = [ordered]@{
        mcpServers = [ordered]@{
            'unreal-mcp' = [ordered]@{
                type = 'http'
                url  = $McpEndpoint
            }
        }
    }

    $McpText = ConvertTo-Json $McpConfig -Depth 5

    $Existing = $null
    if (Test-Path $McpTarget) { $Existing = (Get-Content $McpTarget -Raw) }

    if ($null -ne $Existing -and $Existing.Trim() -eq $McpText.Trim()) {
        Write-Host "Unchanged: $McpTarget (endpoint $McpEndpoint)" -ForegroundColor DarkGray
    } else {
        Backup-IfPresent -Target $McpTarget | Out-Null
        $Tmp = "$McpTarget.tmp"
        Set-Content -Path $Tmp -Value $McpText -Encoding UTF8
        Move-Item -Path $Tmp -Destination $McpTarget -Force
        Write-Host "Installed: $McpTarget (endpoint $McpEndpoint)" -ForegroundColor Green
    }
} else {
    Write-Host "MCP: nessun .mcp.json generato (-NoMcp)." -ForegroundColor DarkGray
}

# ---------------------------------------------------------------------------
# Stato locale informativo
# ---------------------------------------------------------------------------

$StateFile = Join-Path $RepoRoot ".vscode\rt-engine-mode.txt"
if (-not (Test-Path $StateFile)) {
    Set-Content -Path $StateFile -Value "DEV" -Encoding ASCII
}

Write-Host ""
Write-Host "Installazione completata." -ForegroundColor Green
Write-Host ("Workspace id : {0}" -f $WorkspaceId) -ForegroundColor White
Write-Host "RT Three Terminals = 3 ruoli di sessione, N terminali, in OGNI directory." -ForegroundColor White
Write-Host ""
Write-Host "In VS Code: Terminal -> Run Task -> 'RT: Open role set (DEV + VALIDATION + EDITOR)'." -ForegroundColor White
Write-Host "Aprire un terminale NON acquisisce Unreal: il lease si prende con 'rtlease -Action acquire'." -ForegroundColor DarkYellow
if ($WorkspaceId -ne "MAIN") {
    Write-Host ""
    Write-Host "(!) Questo workspace non e' MAIN: la mutazione asset via MCP e' negata." -ForegroundColor DarkYellow
    Write-Host "    Le query read-only restano disponibili e parlano all'Editor di MAIN." -ForegroundColor DarkGray
}
