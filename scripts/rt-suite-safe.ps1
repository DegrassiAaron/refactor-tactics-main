<#
.SYNOPSIS
    Guard sottile attorno a scripts/rt-suite.ps1.

.DESCRIPTION
    NON sostituisce rt-suite e non tocca la sua semantica di validita' della misura.
    La serializzazione dei job Unreal resta del mutex/percorso canonico di rt-suite.

    Cosa e' cambiato rispetto alla versione precedente: il permesso non si legge piu'
    da `.vscode/rt-engine-mode.txt`. Quel file e' per-checkout, il motore e' uno per
    macchina, e il finding `parsecell-arity/1-F13` ha misurato che la sua lettura e'
    anticorrelata con la verita'. Ora serve un lease vivo e posseduto.
#>

# -CheckOnly interroga il guard e riporta il verdetto SENZA eseguire la suite.
# Serve a chi deve sapere se puo' misurare - e ai test, che altrimenti per
# verificare il guard dovrebbero occupare il motore per quaranta minuti.
$CheckOnly = $false
$Forward = @()
foreach ($a in $args) {
    if ("$a" -eq '-CheckOnly') { $CheckOnly = $true } else { $Forward += $a }
}

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Contratto di ownership: importato, non riscritto
# ---------------------------------------------------------------------------

# (!!) Il predicato "questo lease e' mio" viveva in TRE posti e in DUE varianti:
# `rt-lease.ps1` confrontava solo il PID, mentre questo script e `rt-mcp-guard.ps1`
# accettavano anche `terminal_instance`. Due script riconoscevano un proprietario
# che il terzo rifiutava.
#
# Ora la regola ha una sede sola e viene importata dall'AST di `rt-lease.ps1`,
# come gia' si fa per il guard del motore. Un rename a monte da' un errore
# esplicito, non un permesso concesso per sbaglio.
function Import-OwnershipContract {
    param([Parameter(Mandatory)] [string] $LeaseScript)

    if (-not (Test-Path $LeaseScript)) {
        throw "OWNERSHIP_CONTRACT_UNAVAILABLE: $LeaseScript non trovato."
    }
    $tokens = $null
    $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($LeaseScript, [ref]$tokens, [ref]$errors)
    if ($errors -and $errors.Count -gt 0) {
        throw "OWNERSHIP_CONTRACT_UNAVAILABLE: $LeaseScript non e' parsabile ($($errors.Count) errori)."
    }

    $wanted = @('Resolve-SessionIdentity', 'Test-LeaseOwnedBy')
    $found = @{}
    foreach ($fn in $ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $true)) {
        if ($wanted -contains $fn.Name) { $found[$fn.Name] = $fn.Extent.Text }
    }
    $missing = @($wanted | Where-Object { -not $found.ContainsKey($_) })
    if ($missing.Count -gt 0) {
        throw "OWNERSHIP_CONTRACT_UNAVAILABLE: $LeaseScript non espone piu' $($missing -join ', ')."
    }

    $sb = New-Object System.Text.StringBuilder
    foreach ($n in $wanted) { [void]$sb.AppendLine($found[$n]) }
    return $sb.ToString()
}

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

# Il lease e' per macchina. Si legge dove vive, non nel checkout.
$localAppData = $env:LOCALAPPDATA
if ([string]::IsNullOrWhiteSpace($localAppData)) {
    Write-Host "BLOCKED: LOCALAPPDATA non definito: il lease non e' localizzabile." -ForegroundColor Red
    exit 2
}
$leasePath = Join-Path (Join-Path (Join-Path $localAppData "RefactorTactics") "RT3") "lease.json"

if (-not (Test-Path $leasePath)) {
    Write-Host "BLOCKED: ENGINE_LEASE_REQUIRED - nessun lease attivo." -ForegroundColor Red
    Write-Host "Acquisiscilo: scripts\rt-lease.ps1 -Action acquire -Operation SUITE" -ForegroundColor Yellow
    exit 2
}

try {
    $lease = Get-Content $leasePath -Raw | ConvertFrom-Json
} catch {
    Write-Host "BLOCKED: LEASE_CORRUPT - $leasePath non e' JSON valido." -ForegroundColor Red
    exit 2
}

# Il lease deve essere di QUESTA sessione: un lease altrui vivo significa che il
# motore e' di qualcun altro, ed e' esattamente il caso che questo guard esiste per
# fermare.
. ([scriptblock]::Create((Import-OwnershipContract -LeaseScript (Join-Path (Join-Path $workspaceRoot "scripts") "rt-lease.ps1"))))

$identity = Resolve-SessionIdentity
if (-not $identity.Ok) {
    Write-Host ("BLOCKED: {0} - la suite si esegue da un terminale RT, non da un processo effimero." -f $identity.ErrorCode) -ForegroundColor Red
    exit 2
}

$owned = Test-LeaseOwnedBy -Lease $lease -Identity $identity
if (-not $owned) {
    Write-Host "BLOCKED: il lease appartiene a un'altra sessione." -ForegroundColor Red
    Write-Host ("  owner_pid    : {0}" -f $lease.owner_pid)
    Write-Host ("  workspace    : {0}" -f $lease.workspace_root)
    Write-Host ("  operation    : {0}" -f $lease.operation)
    Write-Host ("  task_id      : {0}" -f $lease.task_id)
    exit 2
}

# Un lease preso per aprire l'Editor non autorizza una suite: sono due usi diversi
# della stessa risorsa, e il secondo invaliderebbe il primo.
$allowed = @("SUITE", "BUILD")
if ($allowed -notcontains [string]$lease.operation) {
    Write-Host ("BLOCKED: il lease e' per '{0}', non per SUITE/BUILD." -f $lease.operation) -ForegroundColor Red
    Write-Host "Rilascia e riacquisisci: scripts\rt-lease.ps1 -Action acquire -Operation SUITE" -ForegroundColor Yellow
    exit 2
}

# Il lease e' su un checkout preciso. Eseguire la suite di un ALTRO checkout con
# questo lease misurerebbe un albero che il lease non descrive.
$ownProject = [System.IO.Path]::GetFullPath((Join-Path $workspaceRoot "RefactorTactics.uproject"))
if (-not [string]::Equals([string]$lease.project_path, $ownProject, [System.StringComparison]::OrdinalIgnoreCase)) {
    Write-Host "BLOCKED: MCP_CONTEXT_MISMATCH - il lease e' su un altro progetto." -ForegroundColor Red
    Write-Host ("  lease  : {0}" -f $lease.project_path)
    Write-Host ("  questo : {0}" -f $ownProject)
    exit 2
}

$suite = Join-Path (Join-Path $workspaceRoot "scripts") "rt-suite.ps1"
if (-not (Test-Path $suite)) {
    Write-Host "ERROR: scripts\rt-suite.ps1 non trovato." -ForegroundColor Red
    exit 2
}

$instance = $env:RT_TERMINAL_INSTANCE
if ([string]::IsNullOrWhiteSpace($instance)) { $instance = "unknown" }

if ($CheckOnly) {
    Write-Host ("OK: la sessione possiede il lease {0} (task {1}). Suite NON eseguita: -CheckOnly." -f $lease.lease_id, $lease.task_id) -ForegroundColor Green
    exit 0
}

Write-Host ("VALIDATION WINDOW attiva da VALIDATION:{0} (lease {1}, task {2})." -f $instance, $lease.lease_id, $lease.task_id) -ForegroundColor Yellow
Write-Host "La serializzazione dei job Unreal resta affidata al mutex canonico di rt-suite." -ForegroundColor DarkYellow
& $suite @Forward
exit $LASTEXITCODE
