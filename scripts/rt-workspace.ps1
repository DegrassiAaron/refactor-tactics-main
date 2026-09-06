<#
.SYNOPSIS
    Identita' del workspace: MAIN | DEV | TECHNICAL_DESIGNER.

.DESCRIPTION
    L'identita' del WORKSPACE non e' il ruolo della SESSIONE e non e' il branch git.
    Sono tre cose diverse che si confondono facilmente:

        ruolo sessione   DEV | EDITOR | VALIDATION    -> RT_TERMINAL_ROLE
        identita' ws     MAIN | DEV | TECHNICAL_...   -> RT_WORKSPACE_ID
        branch git       qualunque                    -> git rev-parse

    MAIN non e' il branch `main`: e' il checkout che ospita l'unico bridge MCP della
    macchina. L'authoring asset avviene la' e su un branch di task, mai su `main`.

    (!!) L'autorita' e' il REGISTRO PER MACCHINA, non il marker locale. Il marker
    `.vscode/rt-workspace-id.txt` e' comodo e modificabile da chiunque: da solo
    dichiara un'intenzione, non un fatto. Chi vuole sapere se questa root e' MAIN
    chiede `-Action verify`, che confronta le due fonti e fallisce se divergono.

    (!) Non e' un confine di sicurezza. Il registro sta in %LOCALAPPDATA% e chi puo'
    eseguire questo script puo' anche riscriverlo. Impedisce l'errore, non l'abuso.
#>
param(
    [Parameter(Mandatory)]
    [ValidateSet('status', 'register', 'verify')]
    [string] $Action,

    [ValidateSet('MAIN', 'DEV', 'TECHNICAL_DESIGNER')]
    [string] $WorkspaceId,

    [string] $WorkspaceRoot,

    # Consente di spostare l'identita' MAIN su un'altra root. Richiesto apposta:
    # senza, una seconda registrazione MAIN viene rifiutata invece di promuovere in
    # silenzio una cartella qualsiasi.
    [switch] $Force
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$SchemaVersion = 1

function Get-StoreDir {
    $localAppData = $env:LOCALAPPDATA
    if ([string]::IsNullOrWhiteSpace($localAppData)) {
        throw "LOCALAPPDATA non definito: il registro per macchina non e' localizzabile."
    }
    $dir = Join-Path (Join-Path $localAppData 'RefactorTactics') 'RT3'
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    return $dir
}

function Get-RegistryPath { return Join-Path (Get-StoreDir) 'workspaces.json' }
function Get-LockPath     { return Join-Path (Get-StoreDir) 'workspaces.lock' }

function Resolve-Root {
    param([string] $Explicit)
    $root = $Explicit
    if ([string]::IsNullOrWhiteSpace($root)) { $root = $env:RT_WORKSPACE_ROOT }
    if ([string]::IsNullOrWhiteSpace($root)) { $root = (Get-Location).Path }
    return [System.IO.Path]::GetFullPath($root)
}

function Get-ProjectPath {
    param([Parameter(Mandatory)] [string] $Root)
    $p = Join-Path $Root 'RefactorTactics.uproject'
    if (-not (Test-Path $p)) {
        throw "WORKSPACE_NOT_A_PROJECT: $Root non contiene RefactorTactics.uproject."
    }
    return [System.IO.Path]::GetFullPath($p)
}

function Get-MarkerPath {
    param([Parameter(Mandatory)] [string] $Root)
    return Join-Path (Join-Path $Root '.vscode') 'rt-workspace-id.txt'
}

function Read-Marker {
    param([Parameter(Mandatory)] [string] $Root)
    $p = Get-MarkerPath -Root $Root
    if (-not (Test-Path $p)) { return '' }
    $v = (Get-Content $p -Raw).Trim().ToUpperInvariant()
    if ($v -notin @('MAIN', 'DEV', 'TECHNICAL_DESIGNER')) { return '' }
    return $v
}

function Write-Marker {
    param([Parameter(Mandatory)] [string] $Root, [Parameter(Mandatory)] [string] $Id)
    $p = Get-MarkerPath -Root $Root
    $dir = Split-Path -Parent $p
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    Set-Content -Path $p -Value $Id -Encoding ASCII
}

function Read-Registry {
    $p = Get-RegistryPath
    if (-not (Test-Path $p)) {
        return [pscustomobject]@{ schema_version = $SchemaVersion; workspaces = @() }
    }
    $raw = Get-Content $p -Raw
    if ([string]::IsNullOrWhiteSpace($raw)) {
        return [pscustomobject]@{ schema_version = $SchemaVersion; workspaces = @() }
    }
    try {
        $obj = ConvertFrom-Json $raw
    } catch {
        throw "WORKSPACE_REGISTRY_CORRUPT: $p non e' JSON valido. Ispezionalo a mano."
    }
    if ($obj.PSObject.Properties.Name -notcontains 'workspaces') {
        throw "WORKSPACE_REGISTRY_CORRUPT: $p non ha il campo 'workspaces'."
    }
    return $obj
}

function Write-RegistryAtomic {
    param([Parameter(Mandatory)] $Registry)
    $p = Get-RegistryPath
    $tmp = "$p.tmp"
    Set-Content -Path $tmp -Value (ConvertTo-Json $Registry -Depth 6) -Encoding UTF8
    Move-Item -Path $tmp -Destination $p -Force
}

function Get-Entries {
    param([Parameter(Mandatory)] $Registry)
    $list = @()
    # Il filtro sul null non e' difensivo a caso: una property che contiene un array
    # vuoto si legge come $null, e `@($null)` e' un array di UN elemento nullo, non
    # un array vuoto. Senza il filtro, un registro vuoto veniva iterato una volta e
    # il chiamante leggeva `workspace_root` su niente.
    foreach ($w in @($Registry.workspaces)) { if ($null -ne $w) { $list += $w } }
    # (!) Niente virgola davanti a $list. `return ,$list` AVVOLGE l'array, e in
    # contesto `@(Get-Entries ...)` un array vuoto avvolto conta come UN elemento:
    # misurato, `@(Get-Entries).Count` dava 1 su registro vuoto e il chiamante
    # leggeva `workspace_root` sull'array. Tutti i chiamanti usano gia' `@(...)`,
    # che normalizza il caso vuoto senza bisogno di avvolgere.
    return $list
}

function Find-EntryByRoot {
    param([Parameter(Mandatory)] $Registry, [Parameter(Mandatory)] [string] $Root)
    foreach ($w in @(Get-Entries -Registry $Registry)) {
        if ([string]::Equals([string]$w.workspace_root, $Root, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $w
        }
    }
    return $null
}

# Le voci il cui path non esiste piu'. Una directory spostata NON promuove
# automaticamente un'altra cartella: resta registrata e visibile come mancante,
# perche' la promozione silenziosa e' il modo in cui MAIN cambia senza che nessuno
# lo decida.
function Test-EntryPresent {
    param([Parameter(Mandatory)] $Entry)
    return (Test-Path ([string]$Entry.workspace_root))
}

function Invoke-Register {
    param([Parameter(Mandatory)] [string] $Root)

    if ([string]::IsNullOrWhiteSpace($WorkspaceId)) {
        Write-Host "BLOCKED: WORKSPACE_ID_MISSING - -WorkspaceId e' obbligatorio per register." -ForegroundColor Red
        return 2
    }

    $projectPath = Get-ProjectPath -Root $Root

    $lock = $null
    try {
        $lock = [System.IO.File]::Open((Get-LockPath), 'OpenOrCreate', 'ReadWrite', 'None')
    } catch {
        Write-Host "BUSY: un'altra registrazione e' in corso. Riprova." -ForegroundColor Yellow
        return 3
    }

    try {
        $registry = Read-Registry
        $entries = @(Get-Entries -Registry $registry)

        # Un secondo MAIN per lo stesso progetto e' il difetto centrale: due checkout
        # che credono entrambi di ospitare il bridge. Si rifiuta, e lo spostamento
        # richiede -Force esplicito.
        if ($WorkspaceId -eq 'MAIN') {
            $existingMain = $null
            foreach ($w in $entries) {
                if ([string]$w.workspace_id -eq 'MAIN' -and -not [string]::Equals([string]$w.workspace_root, $Root, [System.StringComparison]::OrdinalIgnoreCase)) {
                    $existingMain = $w
                    break
                }
            }
            if ($null -ne $existingMain -and -not $Force) {
                Write-Host "BLOCKED: WORKSPACE_MAIN_ALREADY_REGISTERED" -ForegroundColor Red
                Write-Host ("  MAIN corrente : {0}" -f $existingMain.workspace_root)
                Write-Host ("  presente      : {0}" -f (Test-EntryPresent -Entry $existingMain))
                Write-Host ""
                Write-Host "Due workspace MAIN per lo stesso progetto significano due bridge MCP che si contendono un Editor." -ForegroundColor Yellow
                Write-Host "Per spostare MAIN qui, ripeti con -Force." -ForegroundColor Yellow
                return 2
            }
        }

        $kept = @()
        foreach ($w in $entries) {
            $sameRoot = [string]::Equals([string]$w.workspace_root, $Root, [System.StringComparison]::OrdinalIgnoreCase)
            $losesMain = ($WorkspaceId -eq 'MAIN' -and [string]$w.workspace_id -eq 'MAIN')
            if ($sameRoot) { continue }
            if ($losesMain) {
                Write-Host ("MAIN spostato da: {0}" -f $w.workspace_root) -ForegroundColor DarkYellow
                continue
            }
            $kept += $w
        }

        $kept += [pscustomobject]@{
            workspace_id     = $WorkspaceId
            workspace_root   = $Root
            project_path     = $projectPath
            registered_at_utc = (Get-Date).ToUniversalTime().ToString('o')
        }

        $registry = [pscustomobject]@{ schema_version = $SchemaVersion; workspaces = $kept }
        Write-RegistryAtomic -Registry $registry
        Write-Marker -Root $Root -Id $WorkspaceId

        Write-Host "REGISTERED" -ForegroundColor Green
        Write-Host ("  workspace_id  : {0}" -f $WorkspaceId)
        Write-Host ("  workspace_root: {0}" -f $Root)
        Write-Host ("  project_path  : {0}" -f $projectPath)
        Write-Host ("  registro      : {0}" -f (Get-RegistryPath)) -ForegroundColor DarkGray
        Write-Host ("  marker locale : {0}" -f (Get-MarkerPath -Root $Root)) -ForegroundColor DarkGray
        return 0
    } finally {
        if ($null -ne $lock) { $lock.Dispose() }
    }
}

# Il verdetto che gli altri script consumano. Confronta le DUE fonti e non accetta
# che una sola parli: un marker senza registro e' un'intenzione, un registro senza
# marker e' una configurazione che il checkout non conosce.
function Get-WorkspaceVerdict {
    param([Parameter(Mandatory)] [string] $Root)

    $registry = Read-Registry
    $entry = Find-EntryByRoot -Registry $registry -Root $Root
    $marker = Read-Marker -Root $Root
    $envId = $env:RT_WORKSPACE_ID
    if ($null -eq $envId) { $envId = '' }

    $registered = ''
    if ($null -ne $entry) { $registered = [string]$entry.workspace_id }

    $code = ''
    $ok = $false

    if ([string]::IsNullOrWhiteSpace($registered)) {
        $code = 'WORKSPACE_NOT_REGISTERED'
    } elseif ([string]::IsNullOrWhiteSpace($marker)) {
        $code = 'WORKSPACE_MARKER_MISSING'
    } elseif ($marker -ne $registered) {
        $code = 'WORKSPACE_IDENTITY_MISMATCH'
    } elseif (-not [string]::IsNullOrWhiteSpace($envId) -and $envId -ne $registered) {
        $code = 'WORKSPACE_IDENTITY_MISMATCH'
    } else {
        $ok = $true
    }

    return [pscustomobject]@{
        Ok            = $ok
        ErrorCode     = $code
        WorkspaceId   = $registered
        Marker        = $marker
        EnvId         = $envId
        WorkspaceRoot = $Root
        IsMain        = ($ok -and $registered -eq 'MAIN')
    }
}

function Invoke-Status {
    param([Parameter(Mandatory)] [string] $Root)

    $registry = Read-Registry
    $verdict = Get-WorkspaceVerdict -Root $Root

    Write-Host "WORKSPACE (questo checkout)"
    Write-Host ("  root          : {0}" -f $Root)
    Write-Host ("  registrato    : {0}" -f $(if ($verdict.WorkspaceId) { $verdict.WorkspaceId } else { '<nessuno>' }))
    Write-Host ("  marker locale : {0}" -f $(if ($verdict.Marker) { $verdict.Marker } else { '<assente>' }))
    Write-Host ("  RT_WORKSPACE_ID: {0}" -f $(if ($verdict.EnvId) { $verdict.EnvId } else { '<non impostata>' }))
    if ($verdict.Ok) {
        Write-Host "  verdetto      : OK" -ForegroundColor Green
    } else {
        Write-Host ("  verdetto      : {0}" -f $verdict.ErrorCode) -ForegroundColor Red
    }

    Write-Host ""
    Write-Host "REGISTRO MACCHINA"
    $entries = @(Get-Entries -Registry $registry)
    if ($entries.Count -eq 0) {
        Write-Host "  <vuoto>" -ForegroundColor DarkGray
    }
    foreach ($w in $entries) {
        $present = Test-EntryPresent -Entry $w
        $flag = ''
        if (-not $present) { $flag = '  [PATH MANCANTE]' }
        Write-Host ("  {0,-19} {1}{2}" -f $w.workspace_id, $w.workspace_root, $flag)
    }
    return 0
}

function Invoke-Verify {
    param([Parameter(Mandatory)] [string] $Root)

    $verdict = Get-WorkspaceVerdict -Root $Root
    if ($verdict.Ok) {
        Write-Host ("OK: workspace {0}" -f $verdict.WorkspaceId) -ForegroundColor Green
        return 0
    }

    Write-Host ("BLOCKED: {0}" -f $verdict.ErrorCode) -ForegroundColor Red
    Write-Host ("  registrato    : {0}" -f $(if ($verdict.WorkspaceId) { $verdict.WorkspaceId } else { '<nessuno>' }))
    Write-Host ("  marker locale : {0}" -f $(if ($verdict.Marker) { $verdict.Marker } else { '<assente>' }))
    Write-Host ("  RT_WORKSPACE_ID: {0}" -f $(if ($verdict.EnvId) { $verdict.EnvId } else { '<non impostata>' }))
    Write-Host ""
    Write-Host "Registra con: scripts\rt-workspace.ps1 -Action register -WorkspaceId <MAIN|DEV|TECHNICAL_DESIGNER>" -ForegroundColor Yellow
    return 2
}

$root = Resolve-Root -Explicit $WorkspaceRoot

switch ($Action) {
    'status'   { exit (Invoke-Status   -Root $root) }
    'register' { exit (Invoke-Register -Root $root) }
    'verify'   { exit (Invoke-Verify   -Root $root) }
}
