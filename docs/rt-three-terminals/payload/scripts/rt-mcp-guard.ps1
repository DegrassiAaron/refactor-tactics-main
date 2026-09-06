<#
.SYNOPSIS
    Preflight della policy "asset MCP soltanto da MAIN".

.DESCRIPTION
    Verifica, in un colpo solo, tutte le condizioni che la policy richiede prima di
    una mutazione asset via Unreal MCP, e stampa un codice di errore stabile quando
    una cade.

    (!!) **QUESTO SCRIPT NON INTERCETTA LE CHIAMATE MCP, E DIRE IL CONTRARIO
    SAREBBE FALSO.** Il trasporto e' HTTP: il client apre una connessione al bridge
    e invoca i tool direttamente. Nessuno script PowerShell sta su quel percorso.

    Cosa questo script E':
      - un preflight che dice, PRIMA di agire, se la sessione ha diritto di mutare;
      - il posto dove quella decisione e' scritta una volta sola;
      - il produttore dell'evento di log che rende la mutazione attribuibile.

    Cosa NON e':
      - una barriera. Chi salta il preflight raggiunge il bridge lo stesso.

    L'enforcement che regge davvero e' a monte, ed e' di CONFIGURAZIONE:
    `.mcp.json` non e' versionato e viene generato dall'installer solo dove il
    bridge deve essere raggiungibile. Un workspace che non lo ha non vede il server.
    Vedi `docs/rt-three-terminals/README.md`, sezione "Enforcement reale".
#>
param(
    [Parameter(Mandatory)]
    [ValidateSet('check')]
    [string] $Action,

    [Parameter(Mandatory)]
    [ValidateSet('MCP_EDITOR_QUERY', 'MCP_ASSET_WRITE')]
    [string] $Operation,

    [string] $TaskId,

    # Path degli asset che la sessione dichiara di voler toccare. Obbligatorio per
    # MCP_ASSET_WRITE: senza, non esiste un write-set con cui confrontare cio' che
    # e' stato effettivamente modificato.
    [string[]] $AssetWriteSet = @(),

    [string] $WorkspaceRoot
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

# Branch su cui l'authoring e' vietato: MAIN e' un'identita' di workspace, non un
# branch, e la policy chiede esplicitamente un branch di task.
$ProtectedBranches = @('main', 'master', 'HEAD')

function Resolve-Root {
    param([string] $Explicit)
    $root = $Explicit
    if ([string]::IsNullOrWhiteSpace($root)) { $root = $env:RT_WORKSPACE_ROOT }
    if ([string]::IsNullOrWhiteSpace($root)) { $root = (Get-Location).Path }
    return [System.IO.Path]::GetFullPath($root)
}

# Importa funzioni da un altro script senza eseguirlo. Gli script RT hanno `param()`
# obbligatori e side effect: un dot-source li farebbe partire.
# (!) Restituisce il SORGENTE, non definisce: un dot-source dentro una funzione
# definisce nello scope della funzione, che muore al return.
function Get-ScriptFunctionSource {
    param(
        [Parameter(Mandatory)] [string] $Path,
        [Parameter(Mandatory)] [string[]] $Names
    )

    if (-not (Test-Path $Path)) {
        throw "SCRIPT_UNAVAILABLE: $Path non trovato."
    }
    $tokens = $null
    $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$tokens, [ref]$errors)
    if ($errors -and $errors.Count -gt 0) {
        throw "SCRIPT_UNAVAILABLE: $Path non e' parsabile ($($errors.Count) errori)."
    }

    $found = @{}
    $functions = $ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $true)
    foreach ($fn in $functions) {
        if ($Names -contains $fn.Name) { $found[$fn.Name] = $fn.Extent.Text }
    }
    $missing = @($Names | Where-Object { -not $found.ContainsKey($_) })
    if ($missing.Count -gt 0) {
        throw "SCRIPT_UNAVAILABLE: $Path non espone piu' $($missing -join ', ')."
    }
    $sb = New-Object System.Text.StringBuilder
    foreach ($n in $Names) { [void]$sb.AppendLine($found[$n]) }
    return $sb.ToString()
}

function Get-LeasePathLocal {
    $localAppData = $env:LOCALAPPDATA
    if ([string]::IsNullOrWhiteSpace($localAppData)) { return '' }
    return Join-Path (Join-Path (Join-Path $localAppData 'RefactorTactics') 'RT3') 'lease.json'
}

function Read-LeaseLocal {
    $p = Get-LeasePathLocal
    if ([string]::IsNullOrWhiteSpace($p) -or -not (Test-Path $p)) { return $null }
    $raw = Get-Content $p -Raw
    if ([string]::IsNullOrWhiteSpace($raw)) { return $null }
    try { return ConvertFrom-Json $raw } catch { throw "LEASE_CORRUPT: $p non e' JSON valido." }
}

function Get-Branch {
    param([Parameter(Mandatory)] [string] $Root)
    Push-Location $Root
    try {
        $b = (& git rev-parse --abbrev-ref HEAD 2>$null)
        return [string]$b
    } catch {
        return ''
    } finally {
        Pop-Location
    }
}

# Ogni condizione produce un codice stabile. L'elenco vive qui e nella policy
# documentale, e i due devono restare la stessa lista.
function Test-Policy {
    param([Parameter(Mandatory)] [string] $Root)

    $failures = @()

    $role = $env:RT_TERMINAL_ROLE
    if ($role -ne 'EDITOR') {
        $failures += [pscustomobject]@{ Code = 'ASSET_WRITE_ROLE_DENIED'; Detail = "RT_TERMINAL_ROLE = '$role', atteso EDITOR." }
    }

    $verdict = Get-WorkspaceVerdict -Root $Root
    if (-not $verdict.Ok) {
        $failures += [pscustomobject]@{ Code = $verdict.ErrorCode; Detail = "identita' del workspace non verificabile (registrato='$($verdict.WorkspaceId)', marker='$($verdict.Marker)')." }
    } elseif (-not $verdict.IsMain) {
        $failures += [pscustomobject]@{ Code = 'ASSET_WRITE_WRONG_WORKSPACE'; Detail = "workspace = $($verdict.WorkspaceId), la mutazione asset e' consentita solo da MAIN." }
    }

    $branch = Get-Branch -Root $Root
    if ([string]::IsNullOrWhiteSpace($branch)) {
        $failures += [pscustomobject]@{ Code = 'TASK_CONTEXT_MISSING'; Detail = 'branch non leggibile.' }
    } elseif ($ProtectedBranches -contains $branch) {
        $failures += [pscustomobject]@{ Code = 'PROTECTED_BRANCH_DENIED'; Detail = "branch = '$branch'. L'authoring vuole un branch di task." }
    }

    $task = $TaskId
    if ([string]::IsNullOrWhiteSpace($task)) { $task = $env:RT_TASK_ID }
    if ([string]::IsNullOrWhiteSpace($task)) {
        $failures += [pscustomobject]@{ Code = 'TASK_CONTEXT_MISSING'; Detail = 'nessun -TaskId e nessuna RT_TASK_ID.' }
    }

    if ($AssetWriteSet.Count -eq 0) {
        $failures += [pscustomobject]@{ Code = 'ASSET_WRITESET_CONFLICT'; Detail = 'write-set asset non dichiarato: non c e nulla con cui confrontare il risultato.' }
    }

    $lease = Read-LeaseLocal
    if ($null -eq $lease) {
        $failures += [pscustomobject]@{ Code = 'ENGINE_LEASE_REQUIRED'; Detail = 'nessun lease attivo. Acquisiscilo con rt-lease.ps1.' }
    } else {
        if ([int]$lease.owner_pid -ne $PID -and [string]$lease.terminal_instance -ne [string]$env:RT_TERMINAL_INSTANCE) {
            $failures += [pscustomobject]@{ Code = 'ENGINE_LEASE_REQUIRED'; Detail = "il lease appartiene a owner_pid $($lease.owner_pid), non a questa sessione." }
        }
        if ([string]$lease.operation -ne $Operation) {
            $failures += [pscustomobject]@{ Code = 'ENGINE_LEASE_REQUIRED'; Detail = "il lease e' per '$($lease.operation)', non per '$Operation'." }
        }
        $ownProject = [System.IO.Path]::GetFullPath((Join-Path $Root 'RefactorTactics.uproject'))
        if (-not [string]::Equals([string]$lease.project_path, $ownProject, [System.StringComparison]::OrdinalIgnoreCase)) {
            $failures += [pscustomobject]@{ Code = 'MCP_CONTEXT_MISMATCH'; Detail = "il lease e' su '$($lease.project_path)', questa sessione su '$ownProject'." }
        }
        if (-not [string]::IsNullOrWhiteSpace($branch) -and -not [string]::IsNullOrWhiteSpace([string]$lease.branch) -and [string]$lease.branch -ne $branch) {
            $failures += [pscustomobject]@{ Code = 'MCP_CONTEXT_MISMATCH'; Detail = "il lease e' stato preso su '$($lease.branch)', ora il branch e' '$branch'." }
        }
    }

    return $failures
}

$root = Resolve-Root -Explicit $WorkspaceRoot
$SchemaVersion = 1
. ([scriptblock]::Create((Get-ScriptFunctionSource -Path (Join-Path $root (Join-Path 'scripts' 'rt-workspace.ps1')) `
    -Names @('Get-StoreDir', 'Get-RegistryPath', 'Get-MarkerPath', 'Read-Marker', 'Read-Registry', 'Find-EntryByRoot', 'Get-Entries', 'Get-WorkspaceVerdict'))))

# MCP_EDITOR_QUERY non muta: non richiede MAIN, ne' task, ne' write-set. Richiede
# solo che l'Editor a cui si parla sia quello che il lease dichiara.
if ($Operation -eq 'MCP_EDITOR_QUERY') {
    $lease = Read-LeaseLocal
    if ($null -eq $lease) {
        Write-Host "BLOCKED: ENGINE_LEASE_REQUIRED - una query che richiede l'Editor vivo vuole un lease." -ForegroundColor Red
        exit 2
    }
    Write-Host "OK: MCP_EDITOR_QUERY consentita (lease $($lease.lease_id), workspace $($lease.workspace_id))." -ForegroundColor Green
    Write-Host "Ricorda: una risposta vuota non e' un PASS e non e' una capability assente." -ForegroundColor DarkGray
    exit 0
}

$failures = Test-Policy -Root $root

if ($failures.Count -eq 0) {
    Write-Host "OK: MCP_ASSET_WRITE consentita." -ForegroundColor Green
    Write-Host ("  workspace : MAIN ({0})" -f $root)
    Write-Host ("  branch    : {0}" -f (Get-Branch -Root $root))
    Write-Host ("  task      : {0}" -f $(if ($TaskId) { $TaskId } else { $env:RT_TASK_ID }))
    Write-Host ("  write-set : {0}" -f ($AssetWriteSet -join ', '))
    Write-Host ""
    Write-Host "(!) Questo preflight non intercetta la chiamata MCP: la autorizza." -ForegroundColor DarkYellow
    Write-Host "    Dopo la chiamata verifica la postcondizione e confronta gli asset toccati col write-set." -ForegroundColor DarkGray
    exit 0
}

Write-Host "BLOCKED: MCP_ASSET_WRITE negata." -ForegroundColor Red
foreach ($f in $failures) {
    Write-Host ("  {0,-32} {1}" -f $f.Code, $f.Detail) -ForegroundColor Red
}
exit 2
