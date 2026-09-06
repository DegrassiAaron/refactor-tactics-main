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
    [switch] $ReclaimStale,

    # Verifica su casi fabbricati le regole PURE di questo script - il predicato di
    # ownership e la vitalita' dell'owner - ed esce senza toccare lease o motore.
    #
    # Esiste perche' un owner morto non si fabbrica a comando, ma uno STATO che lo
    # descrive si'. E' lo stesso motivo per cui `rt-suite.ps1 -SelfTest` esiste, e
    # la stessa sede: il repository non ha un framework di test PowerShell, e
    # AGENTS.md sezione 9 vieta di introdurne uno senza una decisione.
    [switch] $SelfTest
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

    $identity = Resolve-SessionIdentity

    return [pscustomobject]@{
        Role             = [string]$identity.Role
        TerminalInstance = [string]$identity.Instance
        # Provvisorio: e' il promemoria letto dall'ambiente. `Invoke-Acquire` lo
        # sostituisce col valore del REGISTRO prima di scrivere il lease.
        WorkspaceId      = $env:RT_WORKSPACE_ID
        WorkspaceRoot    = $Root
        ProjectPath      = [System.IO.Path]::GetFullPath($projectPath)
        Branch           = [string]$branch
        HeadSha          = [string]$head
        Identity         = $identity
    }
}

# ---------------------------------------------------------------------------
# Identita' della sessione RT - il cuore della hotfix
# ---------------------------------------------------------------------------

# (!!) L'OWNER DEL LEASE E' LA SESSIONE RT, NON IL PROCESSO CHE SCRIVE IL FILE.
#
# Prima di questa correzione `owner_pid` era `$PID`, cioe' il PID del processo
# che eseguiva questo script. Quel processo e' effimero: `rt-lease.ps1 -Action
# acquire` termina subito dopo aver scritto il file, e lo `status` successivo
# leggeva un owner gia' morto. Misurato: ACQUIRED con owner_pid 39516, e la
# lettura immediatamente dopo diceva STALE.
#
# Il lease descrive CHI TIENE IL MOTORE. Chi tiene il motore e' il terminale RT,
# che sopravvive ai comandi che ci si lanciano dentro. Il processo che scrive il
# file e' solo il messaggero.
#
# (!) DUE identita' distinte, e non vanno confuse:
#
#   RT_TERMINAL_INSTANCE          id LOGICO del terminale - etichetta, prompt, log.
#                                 Lo sceglie chi apre il terminale (-InstanceId).
#   RT_TERMINAL_OWNER_PID         id OS del processo terminale persistente.
#   RT_TERMINAL_OWNER_STARTED_AT  istante di avvio di quel processo, UTC ISO-8601.
#   RT_TERMINAL_ROLE              ruolo della sessione.
#
# La vitalita' e la proprieta' del lease si decidono sull'identita' OS. L'id logico
# non ci entra: due terminali possono avere etichette qualunque, e restano sessioni
# diverse perche' hanno processi diversi.
#
# Tenerle nello stesso campo aveva un costo concreto: sovrascrivere
# RT_TERMINAL_INSTANCE col PID rendeva -InstanceId inutile, e il prompt mostrava un
# numero di processo al posto del nome scelto.
#
# Lo start time non e' decorativo: un PID si ricicla, e senza di esso un processo
# nuovo che riusa il numero verrebbe scambiato per l'owner.
function Resolve-SessionIdentity {
    $instance  = $env:RT_TERMINAL_INSTANCE
    $ownerText = $env:RT_TERMINAL_OWNER_PID
    $started   = $env:RT_TERMINAL_OWNER_STARTED_AT
    $role      = $env:RT_TERMINAL_ROLE

    $ok = $true
    $code = ''

    if ([string]::IsNullOrWhiteSpace($ownerText)) {
        $ok = $false; $code = 'RT_SESSION_REQUIRED'
    } elseif (-not ($ownerText -match '^[0-9]+$')) {
        $ok = $false; $code = 'RT_SESSION_MALFORMED'
    } elseif ([string]::IsNullOrWhiteSpace($started)) {
        $ok = $false; $code = 'RT_SESSION_STARTED_AT_MISSING'
    }

    $ownerPid = 0
    if ($ok) { $ownerPid = [int]$ownerText }

    # L'id logico e' informativo: se manca, si ripiega sul PID solo per avere
    # un'etichetta. Non partecipa mai alla decisione di ownership.
    if ([string]::IsNullOrWhiteSpace($instance)) { $instance = $ownerText }

    return [pscustomobject]@{
        Ok        = $ok
        ErrorCode = $code
        Instance  = [string]$instance
        OwnerPid  = $ownerPid
        StartedAt = [string]$started
        Role      = [string]$role
    }
}

# (!!) `ConvertFrom-Json` NON restituisce le stringhe ISO-8601 come stringhe:
# le converte in [datetime]. Un valore scritto come "2026-09-06T11:27:38.3809232Z"
# torna dalla rilettura come 06/09/2026 11:27:38, e il confronto testuale con
# l'istante ricalcolato da `Get-Process` falliva SEMPRE.
#
# L'effetto era esattamente il difetto che questa hotfix corregge, spostato di un
# passo: l'owner era quello giusto, ma ogni lease risultava STALE appena riletto.
# Misurato sulla batteria di lifecycle: T2, T3, T4 e T5 rossi con owner_pid corretto.
#
# Qui ogni istante viene riportato a una grafia sola, qualunque tipo arrivi.
function ConvertTo-CanonicalUtc {
    param([Parameter(Mandatory)] [AllowNull()] [object] $Value)

    if ($null -eq $Value) { return '' }
    if ($Value -is [datetime]) {
        return ([datetime]$Value).ToUniversalTime().ToString('o')
    }

    $s = [string]$Value
    if ([string]::IsNullOrWhiteSpace($s)) { return '' }

    $parsed = [datetime]::MinValue
    $styles = [System.Globalization.DateTimeStyles]::AdjustToUniversal -bor [System.Globalization.DateTimeStyles]::AssumeUniversal
    if ([datetime]::TryParse($s, [System.Globalization.CultureInfo]::InvariantCulture, $styles, [ref]$parsed)) {
        return $parsed.ToUniversalTime().ToString('o')
    }
    return $s
}

# PURA: nessuna lettura di processi, nessun filesystem. E' il predicato "questo
# lease e' mio", ed esiste in UN SOLO posto perche' prima ne esistevano TRE, in
# due varianti diverse: `rt-lease.ps1` confrontava solo il PID, mentre
# `rt-suite-safe.ps1` e `rt-mcp-guard.ps1` accettavano anche `terminal_instance`.
# Due script riconoscevano un proprietario che il terzo rifiutava.
#
# Esportata a quei due tramite import dell'AST, come gia' si fa per il guard del
# motore: la regola ha una sede sola.
function Test-LeaseOwnedBy {
    param(
        [Parameter(Mandatory)] [AllowNull()] $Lease,
        [Parameter(Mandatory)] [AllowNull()] $Identity
    )

    if ($null -eq $Lease -or $null -eq $Identity) { return $false }
    if (-not $Identity.Ok) { return $false }

    # (!) Si confronta l'identita' OS, MAI `terminal_instance`: quello e' un'etichetta
    # scelta da chi apre il terminale, e due sessioni potrebbero portare la stessa.
    $leasePid = 0
    if ($Lease.PSObject.Properties.Name -contains 'owner_pid') { $leasePid = [int]$Lease.owner_pid }
    if ($leasePid -le 0 -or $leasePid -ne [int]$Identity.OwnerPid) { return $false }

    # Lo start time deve combaciare quando entrambi lo dichiarano. Un lease scritto
    # senza start time appartiene a un formato precedente e non si riconosce come
    # proprio: si sbaglia dalla parte che NON concede il motore.
    $leaseStarted = ''
    if ($Lease.PSObject.Properties.Name -contains 'owner_started_at_utc') {
        $leaseStarted = ConvertTo-CanonicalUtc $Lease.owner_started_at_utc
    }
    if ([string]::IsNullOrWhiteSpace($leaseStarted)) { return $false }

    return ($leaseStarted -eq (ConvertTo-CanonicalUtc $Identity.StartedAt))
}

# PURA: decide la vitalita' da uno stato GIA' letto. L'impurita' - interrogare il
# sistema sui processi - resta fuori, in `Test-OwnerAlive`.
function Resolve-OwnerLiveness {
    param(
        [Parameter(Mandatory)] [AllowNull()] $Lease,
        [Parameter(Mandatory)] [AllowNull()] [object] $ActualStartedAt,
        [Parameter(Mandatory)] [bool] $ProcessExists,
        [Parameter(Mandatory)] [bool] $StartTimeReadable
    )

    if ($null -eq $Lease) { return $false }
    $ownerPid = 0
    if ($Lease.PSObject.Properties.Name -contains 'owner_pid') { $ownerPid = [int]$Lease.owner_pid }
    if ($ownerPid -le 0) { return $false }
    if (-not $ProcessExists) { return $false }

    # Processo vivo ma start time illeggibile: si sbaglia dalla parte che NON
    # libera la risorsa, perche' liberarla concederebbe il motore a due sessioni.
    if (-not $StartTimeReadable) { return $true }

    $declared = ''
    if ($Lease.PSObject.Properties.Name -contains 'owner_started_at_utc') {
        $declared = ConvertTo-CanonicalUtc $Lease.owner_started_at_utc
    }
    if ([string]::IsNullOrWhiteSpace($declared)) { return $true }

    return ((ConvertTo-CanonicalUtc $ActualStartedAt) -eq $declared)
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

# IMPURA: interroga il sistema sui processi, e delega la decisione alla funzione
# pura `Resolve-OwnerLiveness`. La separazione esiste perche' un owner morto non
# si fabbrica a comando, ma uno stato che lo descrive si'.
function Test-OwnerAlive {
    param([Parameter(Mandatory)] [AllowNull()] $Lease)

    if ($null -eq $Lease) { return $false }
    $ownerPid = 0
    if ($Lease.PSObject.Properties.Name -contains 'owner_pid') { $ownerPid = [int]$Lease.owner_pid }
    if ($ownerPid -le 0) { return $false }

    $proc = Get-Process -Id $ownerPid -ErrorAction SilentlyContinue
    $exists = ($null -ne $proc)

    $actual = ''
    $readable = $false
    if ($exists) {
        try {
            $actual = $proc.StartTime.ToUniversalTime().ToString('o')
            $readable = $true
        } catch {
            $readable = $false
        }
    }

    return (Resolve-OwnerLiveness -Lease $Lease -ActualStartedAt $actual -ProcessExists $exists -StartTimeReadable $readable)
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
        [Parameter(Mandatory)] [AllowNull()] $ExistingLease,
        [Parameter(Mandatory)] [AllowNull()] $Identity
    )

    if ($Snapshot.State.EngineError) {
        return "ENGINE_STATE_UNKNOWN: enumerazione dei processi fallita ($($Snapshot.State.EngineError)). Un errore non e' un motore libero."
    }
    if ($Snapshot.Free) { return $null }

    # Motore vivo. E' nostro solo se un lease vivo lo rivendica e l'owner siamo noi.
    if ($null -ne $ExistingLease -and (Test-OwnerAlive $ExistingLease) -and (Test-LeaseOwnedBy -Lease $ExistingLease -Identity $Identity)) {
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
    # L'id logico serve proprio qui: quando due terminali si contendono il motore,
    # "owner_pid 52060" non dice a chi chiedere, "term-validation-3" si'.
    Write-Host ("  terminal       : {0}" -f $Lease.terminal_instance)
    Write-Host ("  workspace_id   : {0}" -f $Lease.workspace_id)
    Write-Host ("  workspace_root : {0}" -f $Lease.workspace_root)
    Write-Host ("  task_id        : {0}" -f $Lease.task_id)
    Write-Host ("  branch         : {0}" -f $Lease.branch)
    Write-Host ("  head_sha       : {0}" -f $Lease.head_sha)
    Write-Host ("  owner_pid      : {0}" -f $Lease.owner_pid)
    Write-Host ("  editor_pid     : {0}" -f $Lease.editor_pid)
    Write-Host ("  mcp_endpoint   : {0}" -f $Lease.mcp_endpoint)
    # normalizzato anche in stampa: riletto da JSON e' un [datetime], e verrebbe
    # mostrato nel formato locale accanto a campi che sono UTC ISO.
    Write-Host ("  acquired_at_utc: {0}" -f (ConvertTo-CanonicalUtc $Lease.acquired_at_utc))
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

    # (!!) FAIL-CLOSED: senza una sessione RT persistente non c'e' un owner che
    # possa sopravvivere a questo comando. Un lease scritto qui nascerebbe gia'
    # STALE - il difetto che questa versione corregge - e peggio: sembrerebbe
    # acquisito. Meglio non concederlo affatto.
    $identity = $Context.Identity
    if (-not $identity.Ok) {
        Write-Host ("BLOCKED: {0} - acquire richiede una sessione RT persistente." -f $identity.ErrorCode) -ForegroundColor Red
        Write-Host "Aprilo con: Terminal -> Run Task -> 'RT: Open <RUOLO> terminal', poi usa rtlease." -ForegroundColor Yellow
        Write-Host "Un processo effimero non puo' possedere il motore: terminerebbe subito dopo averlo preso." -ForegroundColor DarkGray
        Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'DENIED' -ErrorCode $identity.ErrorCode
        return 2
    }

    if ([string]::IsNullOrWhiteSpace($Context.Role)) {
        Write-Host "BLOCKED: RT_SESSION_ROLE_MISSING - la sessione non dichiara un ruolo." -ForegroundColor Red
        Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'DENIED' -ErrorCode 'RT_SESSION_ROLE_MISSING'
        return 2
    }

    # L'identita' del workspace si valida contro il REGISTRO di macchina, non
    # contro la variabile d'ambiente: quella e' un promemoria, e un lease che
    # dichiara un workspace non registrato non e' attribuibile a nessun checkout.
    $ws = Get-WorkspaceVerdictOrNull -Root $Context.WorkspaceRoot
    if (-not $ws.Available) {
        Write-Host "BLOCKED: WORKSPACE_CONTRACT_UNAVAILABLE - l'identita' del workspace non e' verificabile." -ForegroundColor Red
        Write-Host ("  motivo: {0}" -f $ws.Reason) -ForegroundColor DarkGray
        Write-Host "Non aver potuto verificare non e' aver verificato: l'acquire si ferma qui." -ForegroundColor Yellow
        Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'DENIED' -ErrorCode 'WORKSPACE_CONTRACT_UNAVAILABLE'
        return 2
    }
    if (-not $ws.Verdict.Ok) {
        Write-Host ("BLOCKED: {0} - identita' del workspace non verificabile." -f $ws.Verdict.ErrorCode) -ForegroundColor Red
        Write-Host "Registra il checkout: scripts\rt-workspace.ps1 -Action register -WorkspaceId <MAIN|DEV|TECHNICAL_DESIGNER>" -ForegroundColor Yellow
        Write-LeaseEvent -Event 'lease_acquire' -Context $Context -Operation $Operation -Result 'DENIED' -ErrorCode ([string]$ws.Verdict.ErrorCode)
        return 2
    }

    # (!) Il lease dichiara il workspace che il REGISTRO conferma, non quello che
    # l'ambiente afferma.
    #
    # `RT_WORKSPACE_ID` e' un promemoria - lo dice `rt-workspace.ps1`, che tratta il
    # registro come unica autorita' - e puo' essere assente o divergente. Un lease
    # che copiasse la variabile registrerebbe un workspace che nessuno ha verificato,
    # e nel caso peggiore uno diverso da quello appena validato: il campo diventa
    # inutile proprio per la cosa a cui serve, attribuire il motore a un checkout.
    #
    # Misurato sullo smoke reale: con la variabile non impostata il lease usciva con
    # `workspace_id` VUOTO mentre il verdetto, nella riga sopra, aveva gia' confermato
    # MAIN dal registro.
    $Context.WorkspaceId = [string]$ws.Verdict.WorkspaceId

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

        if ($null -ne $existing -and $alive -and -not (Test-LeaseOwnedBy -Lease $existing -Identity $identity)) {
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
        $engineProblem = Assert-EngineAvailable -Snapshot $Snapshot -ExistingLease $existing -Identity $identity
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
        if ($null -ne $existing -and $alive -and (Test-LeaseOwnedBy -Lease $existing -Identity $identity)) {
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
            terminal_instance    = [string]$Context.TerminalInstance   # id LOGICO, non decide l'ownership
            workspace_id         = [string]$Context.WorkspaceId
            workspace_root       = [string]$Context.WorkspaceRoot
            project_path         = [string]$Context.ProjectPath
            task_id              = [string]$script:EffectiveTaskId
            operation            = $Operation
            owner_pid            = $identity.OwnerPid
            owner_started_at_utc = [string]$identity.StartedAt
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

    # Stesso fail-closed dell'acquire: senza sessione RT non si puo' dimostrare di
    # essere il proprietario, e rilasciare il lease di un altro e' peggio che non
    # rilasciarlo.
    if (-not $Context.Identity.Ok) {
        Write-Host ("BLOCKED: {0} - release richiede la sessione RT che possiede il lease." -f $Context.Identity.ErrorCode) -ForegroundColor Red
        Write-LeaseEvent -Event 'lease_release' -Context $Context -Result 'DENIED' -ErrorCode ([string]$Context.Identity.ErrorCode)
        return 2
    }

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

        # Il lease porta il workspace verificato all'acquire: per il log vale quello,
        # non il promemoria d'ambiente di questa invocazione.
        if ($existing.PSObject.Properties.Name -contains 'workspace_id' -and
            -not [string]::IsNullOrWhiteSpace([string]$existing.workspace_id)) {
            $Context.WorkspaceId = [string]$existing.workspace_id
        }

        if (-not (Test-LeaseOwnedBy -Lease $existing -Identity $Context.Identity)) {
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
# Identita' del workspace: si chiede al suo owner, non si reimplementa
# ---------------------------------------------------------------------------

# (!!) FAIL-CLOSED, e la versione precedente non lo era.
#
# Restituiva $null quando il contratto workspace non era disponibile, e il
# chiamante bloccava solo su un verdetto NEGATIVO: contratto assente o rotto
# significava quindi "acquire consentito". Bastava cancellare o corrompere
# `rt-workspace.ps1` per aggirare la validazione dell'identita' del workspace.
#
# Ora l'esito e' sempre tipizzato, e i tre casi sono distinti:
#
#   Available = $true,  Verdict.Ok = $true    -> si procede
#   Available = $true,  Verdict.Ok = $false   -> BLOCKED col codice del verdetto
#   Available = $false                        -> BLOCKED, WORKSPACE_CONTRACT_UNAVAILABLE
#
# "Non ho potuto verificare" non e' "ho verificato e va bene".
function Get-WorkspaceVerdictOrNull {
    param([Parameter(Mandatory)] [string] $Root)

    function New-Unavailable([string] $Why) {
        return [pscustomobject]@{ Available = $false; Reason = $Why; Verdict = $null }
    }

    $wsScript = Join-Path (Join-Path $Root 'scripts') 'rt-workspace.ps1'
    if (-not (Test-Path $wsScript)) { return (New-Unavailable "$wsScript non trovato") }

    try {
        $tokens = $null
        $errors = $null
        $ast = [System.Management.Automation.Language.Parser]::ParseFile($wsScript, [ref]$tokens, [ref]$errors)
        if ($errors -and $errors.Count -gt 0) {
            return (New-Unavailable "$wsScript non e' parsabile ($($errors.Count) errori)")
        }

        $wanted = @('Get-StoreDir', 'Get-RegistryPath', 'Get-MarkerPath', 'Read-Marker',
                    'Read-Registry', 'Get-Entries', 'Find-EntryByRoot', 'Get-WorkspaceVerdict')
        $found = @{}
        foreach ($fn in $ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $true)) {
            if ($wanted -contains $fn.Name) { $found[$fn.Name] = $fn.Extent.Text }
        }
        $missing = @($wanted | Where-Object { -not $found.ContainsKey($_) })
        if ($missing.Count -gt 0) {
            return (New-Unavailable "$wsScript non espone piu' $($missing -join ', ')")
        }

        $sb = New-Object System.Text.StringBuilder
        foreach ($n in $wanted) { [void]$sb.AppendLine($found[$n]) }
        . ([scriptblock]::Create($sb.ToString()))

        $v = Get-WorkspaceVerdict -Root $Root
        if ($null -eq $v) { return (New-Unavailable 'Get-WorkspaceVerdict non ha prodotto un verdetto') }
        return [pscustomobject]@{ Available = $true; Reason = ''; Verdict = $v }
    } catch {
        return (New-Unavailable $_.Exception.Message)
    }
}

# ---------------------------------------------------------------------------
# Self-test delle regole pure
# ---------------------------------------------------------------------------

function Invoke-SelfTest {
    $failures = 0
    $count = 0

    function Check([string] $Name, [bool] $Actual, [bool] $Expected) {
        $script:count++
        if ($Actual -eq $Expected) {
            Write-Host ("  PASS  {0}" -f $Name) -ForegroundColor Green
        } else {
            $script:failures++
            Write-Host ("  FAIL  {0}  (atteso {1}, ottenuto {2})" -f $Name, $Expected, $Actual) -ForegroundColor Red
        }
    }

    $script:count = 0
    $script:failures = 0

    $idOk    = [pscustomobject]@{ Ok = $true;  ErrorCode = ''; Instance = '1234'; OwnerPid = 1234; StartedAt = 'T0'; Role = 'DEV' }
    $idOther = [pscustomobject]@{ Ok = $true;  ErrorCode = ''; Instance = '9999'; OwnerPid = 9999; StartedAt = 'T9'; Role = 'DEV' }
    $idNone  = [pscustomobject]@{ Ok = $false; ErrorCode = 'RT_SESSION_REQUIRED'; Instance = ''; OwnerPid = 0; StartedAt = ''; Role = '' }

    $mine     = [pscustomobject]@{ owner_pid = 1234; owner_started_at_utc = 'T0' }
    $recycled = [pscustomobject]@{ owner_pid = 1234; owner_started_at_utc = 'T-OLD' }
    $legacy   = [pscustomobject]@{ owner_pid = 1234 }
    $foreign  = [pscustomobject]@{ owner_pid = 4321; owner_started_at_utc = 'T0' }

    Write-Host "Test-LeaseOwnedBy (puro)" -ForegroundColor White
    Check 'lease della mia sessione'                (Test-LeaseOwnedBy -Lease $mine     -Identity $idOk)    $true
    Check 'lease di un altro terminale'             (Test-LeaseOwnedBy -Lease $foreign  -Identity $idOk)    $false
    Check 'stesso PID ma start time diverso'        (Test-LeaseOwnedBy -Lease $recycled -Identity $idOk)    $false
    Check 'lease senza start time (formato vecchio)' (Test-LeaseOwnedBy -Lease $legacy  -Identity $idOk)    $false
    Check 'identita assente'                        (Test-LeaseOwnedBy -Lease $mine     -Identity $idNone)  $false
    Check 'lease assente'                           (Test-LeaseOwnedBy -Lease $null     -Identity $idOk)    $false
    Check 'identita di un altro terminale'          (Test-LeaseOwnedBy -Lease $mine     -Identity $idOther) $false

    Write-Host "identita' OS separata dall'id logico (puro)" -ForegroundColor White
    # stesso processo, etichette logiche diverse: resta la stessa sessione
    $idLabelA = [pscustomobject]@{ Ok = $true; ErrorCode = ''; Instance = 'dev-1'; OwnerPid = 1234; StartedAt = 'T0'; Role = 'DEV' }
    $idLabelB = [pscustomobject]@{ Ok = $true; ErrorCode = ''; Instance = 'altro-nome'; OwnerPid = 1234; StartedAt = 'T0'; Role = 'DEV' }
    # processi diversi con la STESSA etichetta: sessioni diverse
    $idSameLabel = [pscustomobject]@{ Ok = $true; ErrorCode = ''; Instance = 'dev-1'; OwnerPid = 5678; StartedAt = 'T5'; Role = 'DEV' }
    $leaseLabelled = [pscustomobject]@{ owner_pid = 1234; owner_started_at_utc = 'T0'; terminal_instance = 'dev-1' }

    Check 'id logico diverso, stesso processo: e mio'   (Test-LeaseOwnedBy -Lease $leaseLabelled -Identity $idLabelB)    $true
    Check 'id logico uguale, processo diverso: NON mio' (Test-LeaseOwnedBy -Lease $leaseLabelled -Identity $idSameLabel) $false
    Check 'id logico uguale e stesso processo: e mio'   (Test-LeaseOwnedBy -Lease $leaseLabelled -Identity $idLabelA)    $true

    Write-Host "ConvertTo-CanonicalUtc + round-trip JSON (puro)" -ForegroundColor White
    $isoText = '2026-09-06T11:27:38.3809232Z'
    $asDate  = [datetime]::Parse($isoText, [System.Globalization.CultureInfo]::InvariantCulture,
                   ([System.Globalization.DateTimeStyles]::AdjustToUniversal -bor [System.Globalization.DateTimeStyles]::AssumeUniversal))
    $idIso   = [pscustomobject]@{ Ok = $true; ErrorCode = ''; Instance = '1234'; OwnerPid = 1234; StartedAt = $isoText; Role = 'DEV' }
    # e' cio' che ConvertFrom-Json restituisce davvero: un [datetime], non una stringa
    $fromJson = [pscustomobject]@{ owner_pid = 1234; owner_started_at_utc = $asDate }
    Check 'stringa e datetime denotano lo stesso istante' ((ConvertTo-CanonicalUtc $isoText) -eq (ConvertTo-CanonicalUtc $asDate)) $true
    Check 'lease riletto da JSON resta di questa sessione' (Test-LeaseOwnedBy -Lease $fromJson -Identity $idIso) $true
    Check 'liveness sopravvive al round-trip JSON'          (Resolve-OwnerLiveness -Lease $fromJson -ActualStartedAt $isoText -ProcessExists $true -StartTimeReadable $true) $true

    Write-Host "Resolve-OwnerLiveness (puro)" -ForegroundColor White
    Check 'processo vivo, start combacia'   (Resolve-OwnerLiveness -Lease $mine -ActualStartedAt 'T0'    -ProcessExists $true  -StartTimeReadable $true)  $true
    Check 'processo assente'                (Resolve-OwnerLiveness -Lease $mine -ActualStartedAt ''      -ProcessExists $false -StartTimeReadable $false) $false
    Check 'PID riciclato: start diverso'    (Resolve-OwnerLiveness -Lease $mine -ActualStartedAt 'T-NEW' -ProcessExists $true  -StartTimeReadable $true)  $false
    Check 'start non leggibile: NON libera' (Resolve-OwnerLiveness -Lease $mine -ActualStartedAt ''      -ProcessExists $true  -StartTimeReadable $false) $true
    Check 'lease assente'                   (Resolve-OwnerLiveness -Lease $null -ActualStartedAt 'T0'    -ProcessExists $true  -StartTimeReadable $true)  $false

    Write-Host ""
    if ($script:failures -eq 0) {
        Write-Host ("SELF-TEST: {0} PASS, 0 FAIL" -f $script:count) -ForegroundColor Green
        return 0
    }
    Write-Host ("SELF-TEST: {0} FAIL su {1}" -f $script:failures, $script:count) -ForegroundColor Red
    return 1
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

if ($SelfTest) { exit (Invoke-SelfTest) }

if ([string]::IsNullOrWhiteSpace($Action)) {
    Write-Host "BLOCKED: -Action e' obbligatorio (status | acquire | release), oppure usa -SelfTest." -ForegroundColor Red
    exit 2
}

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
