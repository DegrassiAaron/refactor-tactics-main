<#
.SYNOPSIS
    Lease della risorsa Unreal. UNO PER MACCHINA, non per checkout.

.DESCRIPTION
    Il motore e' uno (`D:\EpicGames\UE_5.8`) e lo condividono tutti i checkout. Il
    guard precedente viveva in `<root>/.vscode/rt-engine-mode.txt`, cioe' PER
    WORKSPACE ROOT, ed e' il difetto misurato dal finding `parsecell-arity/1-F13`:
    con sei checkout attivi l'unico che dichiarava VALIDATION era quello che NON
    stava usando il motore, e tutti quelli che lo usavano leggevano DEV. Testuale:
    "la lettura del guard e' anticorrelata con la verita'".

    Qui lo stato esce dal workspace root e diventa unico per macchina, sotto
    %LOCALAPPDATA%\RefactorTactics\RT3\, come il REQUIRED_FIX di quel finding chiede.

    (!) **Non e' un confine di sicurezza.** Ruolo e workspace arrivano da variabili
    d'ambiente che il chiamante puo' scrivere. Questo script impedisce la COLLISIONE
    fra sessioni che collaborano, non l'aggiramento da parte di chi lo voglia
    aggirare. La barriera vera e' il processo: vedi `Assert-EngineAvailable`.

.NOTES
    Il guard di processo NON e' riscritto qui: viene importato da `rt-suite.ps1`,
    che e' il suo unico owner. Vedi `Get-EngineGuardSource`.
#>
param(
    [Parameter(Mandatory)]
    [ValidateSet('status', 'acquire', 'release')]
    [string] $Action,

    # Cosa la sessione sta per fare col motore. Determina se serve un TaskId e se il
    # motore deve essere libero all'acquisizione.
    [ValidateSet('BUILD', 'SUITE', 'EDITOR', 'PIE', 'COMMANDLET', 'MCP_EDITOR_QUERY', 'MCP_ASSET_WRITE')]
    [string] $Operation,

    [string] $TaskId,

    [string] $WorkspaceRoot,

    # PID dell'Editor, quando la sessione lo conosce gia'. Puo' essere dichiarato
    # dopo l'acquisizione con una seconda `acquire` dello stesso owner.
    [int] $EditorPid = 0,

    # Recupera metadata STALE. Non forza un lease vivo: se l'owner e' vivo, o se
    # esiste un processo motore non attribuibile, fallisce lo stesso.
    [switch] $ReclaimStale
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$SchemaVersion = 1

# Operazioni che modificano qualcosa. Richiedono TaskId: senza, un evento di log non
# e' riconducibile a un lavoro e il lease non e' attribuibile a una decisione.
$MutatingOperations = @('EDITOR', 'PIE', 'COMMANDLET', 'MCP_ASSET_WRITE')

# ---------------------------------------------------------------------------
# Store per macchina
# ---------------------------------------------------------------------------

function Get-StoreDir {
    $localAppData = $env:LOCALAPPDATA
    if ([string]::IsNullOrWhiteSpace($localAppData)) {
        throw "LOCALAPPDATA non definito: lo store per macchina non e' localizzabile."
    }
    $dir = Join-Path (Join-Path $localAppData 'RefactorTactics') 'RT3'
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    return $dir
}

function Get-LeasePath     { return Join-Path (Get-StoreDir) 'lease.json' }
function Get-LockPath      { return Join-Path (Get-StoreDir) 'lease.lock' }
function Get-EventLogPath  { return Join-Path (Get-StoreDir) 'events.jsonl' }

# ---------------------------------------------------------------------------
# Guard di processo: importato, non riscritto
# ---------------------------------------------------------------------------

# (!!) `rt-suite.ps1` e' l'owner della regola "il motore e' libero". Duplicarla qui
# creerebbe una seconda source of truth che diverge al primo fix applicato a una
# sola delle due - ed e' gia' successo: la regola era scritta in due punti che non
# concordavano su cosa fare quando l'enumerazione fallisce.
#
# Le funzioni vengono estratte dall'AST invece che con dot-source, perche'
# `rt-suite.ps1` ha un `param()` obbligatorio e side effect all'esecuzione:
# eseguirlo per leggerne quattro funzioni avvierebbe una misura.
# (!) Restituisce il SORGENTE, non definisce le funzioni: un dot-source eseguito
# dentro una funzione le definisce nello scope della funzione, che muore al return.
# Misurato: `Get-EngineProcessEntries is not recognized` subito dopo un import
# apparentemente riuscito. Il dot-source vive al livello dello script, in fondo.
function Get-EngineGuardSource {
    param([Parameter(Mandatory)] [string] $SuitePath)

    if (-not (Test-Path $SuitePath)) {
        throw "ENGINE_GUARD_UNAVAILABLE: $SuitePath non trovato."
    }

    $tokens = $null
    $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($SuitePath, [ref]$tokens, [ref]$errors)
    if ($errors -and $errors.Count -gt 0) {
        throw "ENGINE_GUARD_UNAVAILABLE: $SuitePath non e' parsabile ($($errors.Count) errori)."
    }

    $wanted = @('Get-EngineProcessEntries', 'ConvertTo-EngineEntry', 'Resolve-EngineState', 'Add-EngineCommandLines', 'Test-EngineFree', 'Get-EngineOrigin')
    $found = @{}
    $functions = $ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $true)
    foreach ($fn in $functions) {
        if ($wanted -contains $fn.Name) { $found[$fn.Name] = $fn.Extent.Text }
    }

    # Fail-closed: un rename a monte non deve degradare in "motore libero".
    $missing = @($wanted | Where-Object { -not $found.ContainsKey($_) })
    if ($missing.Count -gt 0) {
        throw "ENGINE_GUARD_UNAVAILABLE: $SuitePath non espone piu' $($missing -join ', '). Il guard non e' importabile e nessuna acquisizione e' sicura."
    }

    $sb = New-Object System.Text.StringBuilder
    foreach ($name in $wanted) { [void]$sb.AppendLine($found[$name]) }
    return $sb.ToString()
}

# ---------------------------------------------------------------------------
# Contesto della sessione
# ---------------------------------------------------------------------------

function Resolve-WorkspaceRoot {
    param([string] $Explicit)

    $root = $Explicit
    if ([string]::IsNullOrWhiteSpace($root)) { $root = $env:RT_WORKSPACE_ROOT }
    if ([string]::IsNullOrWhiteSpace($root)) { $root = (Get-Location).Path }
    return [System.IO.Path]::GetFullPath($root)
}

function Get-SessionContext {
    param([Parameter(Mandatory)] [string] $Root)

    $projectPath = Join-Path $Root 'RefactorTactics.uproject'
    if (-not (Test-Path $projectPath)) {
        throw "WORKSPACE_NOT_A_PROJECT: $Root non contiene RefactorTactics.uproject."
    }

    $branch = ''
    $head = ''
    Push-Location $Root
    try {
        $branch = (& git rev-parse --abbrev-ref HEAD 2>$null)
        $head = (& git rev-parse HEAD 2>$null)
    } catch {
        # Un git non leggibile non impedisce il lease: impedisce di dichiarare
        # branch e SHA, che restano vuoti e visibili come tali.
    } finally {
        Pop-Location
    }

    $instance = $env:RT_TERMINAL_INSTANCE
    if ([string]::IsNullOrWhiteSpace($instance)) { $instance = "$PID" }

    return [pscustomobject]@{
        Role            = $env:RT_TERMINAL_ROLE
        TerminalInstance = $instance
        WorkspaceId     = $env:RT_WORKSPACE_ID
        WorkspaceRoot   = $Root
        ProjectPath     = [System.IO.Path]::GetFullPath($projectPath)
        Branch          = [string]$branch
        HeadSha         = [string]$head
        OwnerPid        = $PID
    }
}

# ---------------------------------------------------------------------------
# Lease: lettura, scrittura atomica, vitalita'
# ---------------------------------------------------------------------------

function Read-Lease {
    $path = Get-LeasePath
    if (-not (Test-Path $path)) { return $null }
    $raw = Get-Content $path -Raw
    if ([string]::IsNullOrWhiteSpace($raw)) { return $null }
    try {
        return ConvertFrom-Json $raw
    } catch {
        # Un lease illeggibile non e' un lease assente: sarebbe l'invariante che
        # fallisce aperta. Si tratta come vivo e non attribuibile.
        throw "LEASE_CORRUPT: $path non e' JSON valido. Ispezionalo a mano prima di procedere."
    }
}

function Write-LeaseAtomic {
    param([Parameter(Mandatory)] [AllowNull()] $Lease)

    $path = Get-LeasePath
    if ($null -eq $Lease) {
        if (Test-Path $path) { Remove-Item $path -Force }
        return
    }

    $tmp = "$path.tmp"
    $json = ConvertTo-Json $Lease -Depth 6
    Set-Content -Path $tmp -Value $json -Encoding UTF8
    Move-Item -Path $tmp -Destination $path -Force
}

# Un PID puo' essere riciclato: da solo non prova che l'owner sia lo stesso. Il
# confronto include l'istante di avvio, che il riciclo non riproduce.
function Test-OwnerAlive {
    param([Parameter(Mandatory)] $Lease)

    $ownerPid = 0
    if ($Lease.PSObject.Properties.Name -contains 'owner_pid') { $ownerPid = [int]$Lease.owner_pid }
    if ($ownerPid -le 0) { return $false }

    $proc = Get-Process -Id $ownerPid -ErrorAction SilentlyContinue
    if ($null -eq $proc) { return $false }

    if ($Lease.PSObject.Properties.Name -contains 'owner_started_at_utc') {
        $declared = [string]$Lease.owner_started_at_utc
        if (-not [string]::IsNullOrWhiteSpace($declared)) {
            try {
                $actual = $proc.StartTime.ToUniversalTime().ToString('o')
                if ($actual -ne $declared) { return $false }
            } catch {
                # StartTime non leggibile: si sbaglia dalla parte che NON libera.
                return $true
            }
        }
    }
    return $true
}

function Get-OwnStartedAtUtc {
    try {
        return (Get-Process -Id $PID).StartTime.ToUniversalTime().ToString('o')
    } catch {
        return ''
    }
}

# ---------------------------------------------------------------------------
# Log JSONL
# ---------------------------------------------------------------------------

# Nessun token, nessuna credenziale, nessun payload: solo cio' che serve a
# ricostruire chi ha preso il motore, per quale lavoro e con quale esito.
function Write-LeaseEvent {
    param(
        [Parameter(Mandatory)] [string] $Event,
        [Parameter(Mandatory)] [AllowNull()] $Context,
        [string] $LeaseId = '',
        [string] $Operation = '',
        [string] $Result = '',
        [string] $ErrorCode = '',
        [string] $TargetSummary = ''
    )

    $record = [ordered]@{
        timestamp_utc     = (Get-Date).ToUniversalTime().ToString('o')
        event             = $Event
        task_id           = ''
        role              = ''
        terminal_instance = ''
        workspace_id      = ''
        workspace_root    = ''
        branch            = ''
        head_sha          = ''
        lease_id          = $LeaseId
        operation_class   = $Operation
        target_summary    = $TargetSummary
        result            = $Result
        error_code        = $ErrorCode
    }

    if ($null -ne $Context) {
        $record.role = [string]$Context.Role
        $record.terminal_instance = [string]$Context.TerminalInstance
        $record.workspace_id = [string]$Context.WorkspaceId
        $record.workspace_root = [string]$Context.WorkspaceRoot
        $record.branch = [string]$Context.Branch
        $record.head_sha = [string]$Context.HeadSha
    }
    if (-not [string]::IsNullOrWhiteSpace($script:EffectiveTaskId)) {
        $record.task_id = $script:EffectiveTaskId
    }

    $line = ConvertTo-Json $record -Depth 4 -Compress

    # Append concorrente-safe: piu' terminali scrivono lo stesso file. Un fallimento
    # del log non deve trasformare un'operazione fallita in un successo, ne'
    # viceversa: si segnala e si prosegue.
    for ($attempt = 0; $attempt -lt 5; $attempt++) {
        try {
            $fs = [System.IO.File]::Open((Get-EventLogPath), 'Append', 'Write', 'Read')
            try {
                $bytes = [System.Text.Encoding]::UTF8.GetBytes($line + [Environment]::NewLine)
                $fs.Write($bytes, 0, $bytes.Length)
            } finally {
                $fs.Dispose()
            }
            return
        } catch {
            Start-Sleep -Milliseconds 40
        }
    }
    Write-Host "WARN: evento di lease non registrato (log occupato)." -ForegroundColor DarkYellow
}

# ---------------------------------------------------------------------------
# Stato del motore
# ---------------------------------------------------------------------------

function Get-EngineSnapshot {
    param([Parameter(Mandatory)] [string] $OwnProjectPath)

    $raw = Get-EngineProcessEntries
    $state = Resolve-EngineState -Entries $raw.Entries -EnumError $raw.EnumError

    # L'attribuzione al checkout e' il punto di tutta la diagnostica: senza la riga
    # di comando ogni processo risulta 'provenienza ignota' e il blocco non e'
    # riconducibile a nessuno. Si paga solo quando c'e' almeno un processo, come
    # `rt-suite.ps1` documenta.
    $state = Add-EngineCommandLines -State $state

    $rows = @()
    foreach ($e in @($state.Engines)) {
        $origin = 'provenienza ignota'
        if ($e.PSObject.Properties.Name -contains 'CommandLine') {
            $origin = Get-EngineOrigin -CommandLine $e.CommandLine -OwnProjectPath $OwnProjectPath
        }
        $rows += [pscustomobject]@{ ProcessId = $e.ProcessId; Origin = $origin }
    }

    return [pscustomobject]@{
        State = $state
        Free  = (Test-EngineFree $state)
        Rows  = $rows
    }
}

# Il motore deve essere libero, oppure gia' occupato da noi. Un processo che esiste
# senza che nessun lease lo rivendichi blocca: e' il caso "Unreal vivo senza owner
# verificabile", e concederlo significherebbe promettere esclusivita' che non c'e'.
function Assert-EngineAvailable {
    param(
        [Parameter(Mandatory)] $Snapshot,
        [Parameter(Mandatory)] [AllowNull()] $ExistingLease
    )

    if ($Snapshot.State.EngineError) {
        return "ENGINE_STATE_UNKNOWN: enumerazione dei processi fallita ($($Snapshot.State.EngineError)). Un errore non e' un motore libero."
    }
    if ($Snapshot.Free) { return $null }

    # Motore vivo. E' nostro solo se un lease vivo lo rivendica e l'owner siamo noi.
    if ($null -ne $ExistingLease -and (Test-OwnerAlive $ExistingLease) -and [int]$ExistingLease.owner_pid -eq $PID) {
        return $null
    }

    return "ENGINE_BUSY_UNATTRIBUTED: processi motore vivi che nessun lease di questa sessione rivendica."
}

# ---------------------------------------------------------------------------
# Presentazione
# ---------------------------------------------------------------------------

function Show-Lease {
    param([Parameter(Mandatory)] [AllowNull()] $Lease, [Parameter(Mandatory)] $Alive)

    if ($null -eq $Lease) {
        Write-Host "ENGINE LEASE: " -NoNewline
        Write-Host "LIBERO" -ForegroundColor Green
        return
    }

    $stato = 'VIVO'
    $colore = 'Yellow'
    if (-not $Alive) {
        $stato = 'STALE (owner non piu' + "' vivo)"
        $colore = 'DarkYellow'
    }

    Write-Host "ENGINE LEASE: " -NoNewline
    Write-Host $stato -ForegroundColor $colore
    Write-Host ("  lease_id       : {0}" -f $Lease.lease_id)
    Write-Host ("  operation      : {0}" -f $Lease.operation)
    Write-Host ("  role           : {0}" -f $Lease.role)
    Write-Host ("  workspace_id   : {0}" -f $Lease.workspace_id)
    Write-Host ("  workspace_root : {0}" -f $Lease.workspace_root)
    Write-Host ("  task_id        : {0}" -f $Lease.task_id)
    Write-Host ("  branch         : {0}" -f $Lease.branch)
    Write-Host ("  head_sha       : {0}" -f $Lease.head_sha)
    Write-Host ("  owner_pid      : {0}" -f $Lease.owner_pid)
    Write-Host ("  editor_pid     : {0}" -f $Lease.editor_pid)
    Write-Host ("  mcp_endpoint   : {0}" -f $Lease.mcp_endpoint)
    Write-Host ("  acquired_at_utc: {0}" -f $Lease.acquired_at_utc)
}

function Show-Engine {
    param([Parameter(Mandatory)] $Snapshot)

    Write-Host ("ENGINE       : {0} (caso: {1})" -f $(if ($Snapshot.Free) { 'libero' } else { 'occupato' }), $Snapshot.State.Case)
    foreach ($row in @($Snapshot.Rows)) {
        Write-Host ("  pid {0,-6} [{1}]" -f $row.ProcessId, $row.Origin) -ForegroundColor DarkGray
    }
}

# ---------------------------------------------------------------------------
# Azioni
# ---------------------------------------------------------------------------

function Invoke-Status {
    param([Parameter(Mandatory)] $Context, [Parameter(Mandatory)] $Snapshot)

    $lease = Read-Lease
    $alive = $false
    if ($null -ne $lease) { $alive = Test-OwnerAlive $lease }

    Show-Lease -Lease $lease -Alive $alive
    Show-Engine -Snapshot $Snapshot

    if ($null -ne $lease -and -not $alive) {
        Write-Host ""
        Write-Host "Metadata stale. Recupero: rt-lease.ps1 -Action acquire -ReclaimStale ..." -ForegroundColor DarkYellow
        Write-Host "Il recupero fallisce comunque se un processo motore non attribuibile e' vivo." -ForegroundColor DarkGray
    }
    return 0
}

function Invoke-Acquire {
    param([Parameter(Mandatory)] $Context, [Parameter(Mandatory)] $Snapshot)

    if ([string]::IsNullOrWhiteSpace($Operation)) {
        Write-Host "BLOCKED: -Operation e' obbligatorio per acquire." -ForegroundColor Red
        Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Result 'DENIED' -ErrorCode 'OPERATION_MISSING'
        return 2
    }

    if ($MutatingOperations -contains $Operation -and [string]::IsNullOrWhiteSpace($script:EffectiveTaskId)) {
        Write-Host "BLOCKED: TASK_CONTEXT_MISSING - l'operazione $Operation richiede -TaskId (o RT_TASK_ID)." -ForegroundColor Red
        Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'DENIED' -ErrorCode 'TASK_CONTEXT_MISSING'
        return 2
    }

    $lockPath = Get-LockPath
    $lock = $null
    try {
        $lock = [System.IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None')
    } catch {
        Write-Host "BUSY: un'altra sessione sta modificando il lease in questo istante. Riprova." -ForegroundColor Yellow
        Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'BUSY' -ErrorCode 'LEASE_LOCK_CONTENDED'
        return 3
    }

    try {
        $existing = Read-Lease
        $alive = $false
        if ($null -ne $existing) { $alive = Test-OwnerAlive $existing }

        if ($null -ne $existing -and $alive -and [int]$existing.owner_pid -ne $PID) {
            Write-Host "BUSY: il motore e' gia' preso." -ForegroundColor Yellow
            Show-Lease -Lease $existing -Alive $true
            Write-Host ""
            Write-Host "Il lease non e' preemptive: attendi il rilascio o coordinati con l'owner." -ForegroundColor DarkYellow
            Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'BUSY' -ErrorCode 'ENGINE_LEASE_BUSY'
            return 3
        }

        if ($null -ne $existing -and -not $alive -and -not $ReclaimStale) {
            Write-Host "BLOCKED: esiste un lease STALE. Rileggilo e conferma con -ReclaimStale." -ForegroundColor Red
            Show-Lease -Lease $existing -Alive $false
            Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'DENIED' -ErrorCode 'LEASE_STALE_UNCONFIRMED'
            return 2
        }

        # Il recupero di metadata stale non basta: se un processo motore vive e non
        # e' attribuibile, la risorsa NON e' libera, per quanto il file dica altro.
        $engineProblem = Assert-EngineAvailable -Snapshot $Snapshot -ExistingLease $existing
        if ($null -ne $engineProblem) {
            Write-Host "BLOCKED: $engineProblem" -ForegroundColor Red
            Show-Engine -Snapshot $Snapshot
            $code = 'ENGINE_BUSY_UNATTRIBUTED'
            if ($engineProblem -like 'ENGINE_STATE_UNKNOWN*') { $code = 'ENGINE_STATE_UNKNOWN' }
            Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'DENIED' -ErrorCode $code
            return 2
        }

        $leaseId = [guid]::NewGuid().ToString('n').Substring(0, 12)
        $editor = $EditorPid
        if ($null -ne $existing -and $alive -and [int]$existing.owner_pid -eq $PID) {
            # Ri-acquisizione dello stesso owner: conserva l'identita' e arricchisce.
            $leaseId = [string]$existing.lease_id
            if ($editor -le 0 -and $existing.PSObject.Properties.Name -contains 'editor_pid') {
                $editor = [int]$existing.editor_pid
            }
        }

        $endpoint = $env:RT_MCP_ENDPOINT
        if ([string]::IsNullOrWhiteSpace($endpoint)) { $endpoint = '' }

        $lease = [ordered]@{
            schema_version       = $SchemaVersion
            lease_id             = $leaseId
            role                 = [string]$Context.Role
            terminal_instance    = [string]$Context.TerminalInstance
            workspace_id         = [string]$Context.WorkspaceId
            workspace_root       = [string]$Context.WorkspaceRoot
            project_path         = [string]$Context.ProjectPath
            task_id              = [string]$script:EffectiveTaskId
            operation            = $Operation
            owner_pid            = $Context.OwnerPid
            owner_started_at_utc = (Get-OwnStartedAtUtc)
            editor_pid           = $editor
            mcp_endpoint         = $endpoint
            branch               = [string]$Context.Branch
            head_sha             = [string]$Context.HeadSha
            acquired_at_utc      = (Get-Date).ToUniversalTime().ToString('o')
        }

        Write-LeaseAtomic -Lease ([pscustomobject]$lease)
        Write-Host "ACQUIRED" -ForegroundColor Green
        Show-Lease -Lease ([pscustomobject]$lease) -Alive $true
        Write-LeaseEvent -Event 'lease_acquire' -Context $Context -LeaseId $leaseId -Operation $Operation -Result 'ACQUIRED'
        return 0
    } finally {
        if ($null -ne $lock) { $lock.Dispose() }
    }
}

function Invoke-Release {
    param([Parameter(Mandatory)] $Context, [Parameter(Mandatory)] $Snapshot)

    $lockPath = Get-LockPath
    $lock = $null
    try {
        $lock = [System.IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None')
    } catch {
        Write-Host "BUSY: un'altra sessione sta modificando il lease in questo istante. Riprova." -ForegroundColor Yellow
        return 3
    }

    try {
        $existing = Read-Lease
        if ($null -eq $existing) {
            Write-Host "Nessun lease da rilasciare." -ForegroundColor DarkGray
            return 0
        }

        if ([int]$existing.owner_pid -ne $PID) {
            Write-Host "BLOCKED: il lease appartiene a un'altra sessione. Nessun force unlock cancella ownership viva." -ForegroundColor Red
            Show-Lease -Lease $existing -Alive (Test-OwnerAlive $existing)
            Write-LeaseEvent -Event 'lease_release' -Context $Context -LeaseId ([string]$existing.lease_id) -Result 'DENIED' -ErrorCode 'LEASE_NOT_OWNED'
            return 2
        }

        # (!!) Il rilascio e' una DICHIARAZIONE che la risorsa e' libera. Se un processo
        # motore e' ancora vivo, dichiararlo sarebbe falso e la prossima sessione
        # troverebbe un motore occupato con lease libero: esattamente lo stato che
        # rende non attribuibile un blocco.
        if (-not $Snapshot.Free) {
            Write-Host "BLOCKED: processi motore ancora vivi. Il lease NON viene rilasciato." -ForegroundColor Red
            Show-Engine -Snapshot $Snapshot
            Write-Host "Chiudi Editor/PIE/commandlet avviati da questa sessione, poi rilascia." -ForegroundColor Yellow
            Write-LeaseEvent -Event 'lease_release' -Context $Context -LeaseId ([string]$existing.lease_id) -Result 'DENIED' -ErrorCode 'ENGINE_STILL_ALIVE'
            return 2
        }

        Write-LeaseAtomic -Lease $null
        Write-Host "RELEASED" -ForegroundColor Green
        Write-LeaseEvent -Event 'lease_release' -Context $Context -LeaseId ([string]$existing.lease_id) -Operation ([string]$existing.operation) -Result 'RELEASED'
        return 0
    } finally {
        if ($null -ne $lock) { $lock.Dispose() }
    }
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

$script:EffectiveTaskId = $TaskId
if ([string]::IsNullOrWhiteSpace($script:EffectiveTaskId)) { $script:EffectiveTaskId = $env:RT_TASK_ID }
if ($null -eq $script:EffectiveTaskId) { $script:EffectiveTaskId = '' }

$root = Resolve-WorkspaceRoot -Explicit $WorkspaceRoot
$context = Get-SessionContext -Root $root

. ([scriptblock]::Create((Get-EngineGuardSource -SuitePath (Join-Path $root (Join-Path 'scripts' 'rt-suite.ps1')))))
$snapshot = Get-EngineSnapshot -OwnProjectPath $context.ProjectPath

switch ($Action) {
    'status'  { exit (Invoke-Status  -Context $context -Snapshot $snapshot) }
    'acquire' { exit (Invoke-Acquire -Context $context -Snapshot $snapshot) }
    'release' { exit (Invoke-Release -Context $context -Snapshot $snapshot) }
}
