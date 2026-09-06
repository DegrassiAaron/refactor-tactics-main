<#
.SYNOPSIS
    Task router RT3: chi deve lavorare adesso, e cosa gli e' stato consegnato.

.DESCRIPTION
    Risolve un problema di memoria umana, non di gameplay. Con tre ruoli e piu'
    checkout, chi apre un terminale deve ricordare a chi ha gia' passato il lavoro e
    quale ruolo viene dopo. Il router tiene quello stato fuori dalla testa e fuori
    dalla chat.

    Quattro confini da non confondere, gia' distinti altrove e qui preservati:

        ruolo sessione   DEV | EDITOR | VALIDATION       -> RT_TERMINAL_ROLE
        identita' ws     MAIN | DEV | TECHNICAL_DESIGNER -> rt-workspace.ps1
        lease motore     chi occupa Unreal               -> rt-lease.ps1
        routing task     chi deve lavorare adesso        -> QUESTO script

    Sono quattro cose diverse. Questo script non ne tocca nessun'altra: non legge
    ne' scrive il lease, non guarda `rtmode`, non avvia niente.

    (!!) UN SOLO WRITER. `state.json` lo scrive il COORDINATOR. Un worker produce un
    risultato append-only sotto `results/` e non cambia il routing: una
    raccomandazione non e' una decisione, ed e' il punto per cui questo script
    esiste invece di un file condiviso qualunque.

    (!) Non e' un confine di sicurezza. Il ruolo arriva da una variabile d'ambiente
    che il chiamante puo' scrivere, e lo store sta in %LOCALAPPDATA%. Impedisce
    l'errore, non l'abuso - come il registro dei workspace e come il lease.

    Storage per MACCHINA, perche' i checkout sono molti e il task e' uno:

        %LOCALAPPDATA%\RefactorTactics\RT3\Tasks\<TaskId>\
            state.json
            task.lock
            assignments\0001-DEV.md
            results\0001-DEV-<instance>.md

.EXAMPLE
    rt-task-router.ps1 -Action list
    rt-task-router.ps1 -Action status -TaskId 2330
    rt-task-router.ps1 -Action route  -TaskId 2330 -Role EDITOR
#>
param(
    [ValidateSet('list', 'next', 'status', 'init', 'assign', 'assignment', 'report', 'route', 'close')]
    [string] $Action,

    [string] $TaskId,

    # init
    [string] $Title,

    # assign
    [ValidateSet('DEV', 'EDITOR', 'VALIDATION', 'USER', 'NONE')] [string] $Actor,
    [string] $Objective,
    [string] $Context,
    [string[]] $Inputs = @(),
    [string[]] $Do = @(),
    [string[]] $DoNot = @(),
    [string[]] $ExpectedOutput = @(),
    [ValidateSet('DEV', 'EDITOR', 'VALIDATION', 'USER', 'NONE')] [string] $NextIfPass,

    # Stato del TASK, che non e' lo stato di un RISULTATO: sono due vocabolari, e
    # confonderli e' il modo in cui nascono WAITING_DEV, WAITING_EDITOR e le altre
    # copie di un'informazione che `next_actor` gia' porta.
    [ValidateSet('ACTIVE', 'BLOCKED', 'DONE')] [string] $TaskStatus,

    # assign/report: prosa aggiuntiva, presa da file per non passare testo lungo
    # sulla riga di comando.
    [string] $BodyFile,

    # report
    [ValidateSet('DONE', 'PARTIAL', 'BLOCKED', 'FAILED')] [string] $Status,
    [string] $Summary,
    [string[]] $Changes = @(),
    [string[]] $Evidence = @(),
    [string[]] $NotRun = @(),
    [string] $Blocker,
    [ValidateSet('DEV', 'EDITOR', 'VALIDATION', 'USER', 'NONE')] [string] $NextActorRecommended,

    # route
    [ValidateSet('DEV', 'EDITOR', 'VALIDATION')] [string] $Role,

    # Guardia ottimistica: se passata e diversa dalla sequence corrente, la mutazione
    # e' RIFIUTATA invece di sovrascrivere la decisione di un altro Coordinator.
    [int] $ExpectedSequence = -1,

    [string] $Reason,

    # Accettato per uniformita' con gli altri script RT e per il log: il routing NON
    # dipende dal checkout, ed e' esattamente cio' che lo rende visibile da tutti.
    [string] $WorkspaceRoot,

    # (!) Esiste per il SELF-TEST e per la diagnosi. In esercizio non passarlo: lo
    # store e' per macchina, ed e' quello che rende il task visibile da un altro
    # checkout.
    [string] $StoreRoot,

    [switch] $SelfTest
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$SchemaVersion = 1

$WorkerActors = @('DEV', 'EDITOR', 'VALIDATION')
$AllActors    = @('DEV', 'EDITOR', 'VALIDATION', 'USER', 'NONE')
$TaskStates   = @('ACTIVE', 'BLOCKED', 'DONE')
$MutatingActions = @('init', 'assign', 'close')

# Nomi di device DOS. Non sono un rischio di traversal: sono un fallimento oscuro
# di New-Item, che verrebbe letto come "il router e' rotto".
$ReservedNames = @('CON', 'PRN', 'AUX', 'NUL',
    'COM1', 'COM2', 'COM3', 'COM4', 'COM5', 'COM6', 'COM7', 'COM8', 'COM9',
    'LPT1', 'LPT2', 'LPT3', 'LPT4', 'LPT5', 'LPT6', 'LPT7', 'LPT8', 'LPT9')

# ---------------------------------------------------------------------------
# Regole pure - nessun filesystem, nessun ambiente. Sono quelle che -SelfTest prova.
# ---------------------------------------------------------------------------

<#
    Grammatica del TaskId, piccola e dichiarata: il valore diventa un NOME DI
    DIRECTORY, e un id non validato e' una scrittura fuori dallo store.

        primo carattere   lettera o cifra
        seguito da        lettere, cifre, '.', '_', '-'
        lunghezza         1..64

    Il vincolo sul primo carattere e' cio' che esclude `..` e `.git` senza un caso
    speciale: nessuno dei due comincia con un alfanumerico.
#>
function Test-TaskIdValid {
    param([AllowNull()] [string] $Id)

    if ([string]::IsNullOrWhiteSpace($Id)) { return $false }
    if ($Id.Length -gt 64) { return $false }
    if (-not ($Id -cmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$')) { return $false }
    if ($Id.ToUpperInvariant() -in $script:ReservedNames) { return $false }
    return $true
}

function Test-ActorIsWorker {
    param([AllowNull()] [string] $CandidateActor)
    if ([string]::IsNullOrWhiteSpace($CandidateActor)) { return $false }
    return ($CandidateActor -in $script:WorkerActors)
}

<#
    Una sequence non torna indietro. Serve a rifiutare una scrittura che
    sovrascriverebbe una decisione piu' recente, invece di perderla in silenzio.
#>
function Test-SequenceAdvance {
    param([int] $Current, [int] $Proposed)
    return ($Proposed -gt $Current)
}

<#
    Forma di `state.json`. Uno stato illeggibile non e' uno stato assente: si
    rifiuta, non si ricrea. Ricrearlo cancellerebbe il routing di un task vivo.
    Restituisce il codice d'errore, oppure stringa vuota se la forma e' valida.
#>
function Get-StateShapeError {
    param([AllowNull()] $State)

    if ($null -eq $State) { return 'TASK_STATE_CORRUPT' }

    $required = @('schema_version', 'task_id', 'title', 'status', 'next_actor',
        'assignment_sequence', 'assignment_path', 'last_result_path',
        'created_at', 'updated_at', 'history')

    $present = @()
    if ($null -ne $State.PSObject) { $present = @($State.PSObject.Properties.Name) }
    foreach ($field in $required) {
        if ($present -notcontains $field) { return 'TASK_STATE_CORRUPT' }
    }

    if ([string]$State.status -notin $script:TaskStates) { return 'TASK_STATE_CORRUPT' }
    if ([string]$State.next_actor -notin $script:AllActors) { return 'TASK_STATE_CORRUPT' }
    if (-not (Test-TaskIdValid -Id ([string]$State.task_id))) { return 'TASK_STATE_CORRUPT' }

    $seq = -1
    if (-not [int]::TryParse([string]$State.assignment_sequence, [ref] $seq)) { return 'TASK_STATE_CORRUPT' }
    if ($seq -lt 0) { return 'TASK_STATE_CORRUPT' }

    return ''
}

<#
    Il verdetto che il terminale mostra all'avvio, e che `report` usa per rifiutare
    un risultato proveniente da un ruolo che non e' quello atteso.

    E' PURO: (stato, ruolo) -> verdetto. Nessuna delle sue risposte dipende da cosa
    c'e' sul disco in questo istante, ed e' cio' che lo rende provabile senza
    fabbricare un task vero.
#>
function Get-RouteVerdict {
    param([AllowNull()] $State, [AllowNull()] [string] $SessionRole)

    $actual = ''
    if (-not [string]::IsNullOrWhiteSpace($SessionRole)) { $actual = $SessionRole }

    if ($null -eq $State) {
        return [pscustomobject]@{ Ok = $false; Code = 'TASK_NOT_FOUND'; Expected = ''; Actual = $actual }
    }

    $shape = Get-StateShapeError -State $State
    if ($shape -ne '') {
        return [pscustomobject]@{ Ok = $false; Code = $shape; Expected = ''; Actual = $actual }
    }

    $expected = [string]$State.next_actor

    if ([string]$State.status -eq 'DONE') {
        return [pscustomobject]@{ Ok = $false; Code = 'TASK_ALREADY_DONE'; Expected = $expected; Actual = $actual }
    }
    if ($actual -eq '') {
        return [pscustomobject]@{ Ok = $false; Code = 'RT_SESSION_REQUIRED'; Expected = $expected; Actual = $actual }
    }
    if (-not (Test-ActorIsWorker -CandidateActor $expected)) {
        return [pscustomobject]@{ Ok = $false; Code = 'TASK_ACTOR_NOT_WORKER'; Expected = $expected; Actual = $actual }
    }
    if ($expected -ne $actual) {
        return [pscustomobject]@{ Ok = $false; Code = 'TASK_ROUTE_MISMATCH'; Expected = $expected; Actual = $actual }
    }
    if ([int]$State.assignment_sequence -lt 1 -or [string]::IsNullOrWhiteSpace([string]$State.assignment_path)) {
        return [pscustomobject]@{ Ok = $false; Code = 'TASK_ASSIGNMENT_MISSING'; Expected = $expected; Actual = $actual }
    }

    return [pscustomobject]@{ Ok = $true; Code = ''; Expected = $expected; Actual = $actual }
}

<#
    Chi puo' mutare il routing. Guardrail operativo, NON security boundary: la
    variabile che decide e' scrivibile dal chiamante, e lo diciamo invece di
    lasciarlo credere.

    La distinzione che conta e' l'errore che si vuole impedire: una sessione worker
    che, mentre lavora, si riassegna il task o lo chiude. Il Coordinator gira in una
    sessione SENZA ruolo di terminale, e questo basta a separarli.
#>
function Get-MutationPolicy {
    param([Parameter(Mandatory)] [string] $RequestedAction, [AllowNull()] [string] $SessionRole)

    $isWorkerSession = (Test-ActorIsWorker -CandidateActor $SessionRole)

    if ($RequestedAction -in $script:MutatingActions -and $isWorkerSession) {
        return [pscustomobject]@{ Allowed = $false; Code = 'TASK_MUTATION_ROLE_DENIED' }
    }
    if ($RequestedAction -eq 'report' -and -not $isWorkerSession) {
        return [pscustomobject]@{ Allowed = $false; Code = 'RT_SESSION_REQUIRED' }
    }
    return [pscustomobject]@{ Allowed = $true; Code = '' }
}

function Format-Sequence {
    param([int] $Sequence)
    return ('{0:0000}' -f $Sequence)
}

# ---------------------------------------------------------------------------
# Store
# ---------------------------------------------------------------------------

function Get-TasksRoot {
    if (-not [string]::IsNullOrWhiteSpace($script:StoreRoot)) {
        $dir = [System.IO.Path]::GetFullPath($script:StoreRoot)
    } else {
        $localAppData = $env:LOCALAPPDATA
        if ([string]::IsNullOrWhiteSpace($localAppData)) {
            throw "TASK_STORE_UNAVAILABLE: LOCALAPPDATA non definito, lo store per macchina non e' localizzabile."
        }
        $dir = Join-Path (Join-Path (Join-Path $localAppData 'RefactorTactics') 'RT3') 'Tasks'
    }
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    return $dir
}

function Get-TaskDir {
    param([Parameter(Mandatory)] [string] $Id)
    return Join-Path (Get-TasksRoot) $Id
}

function Get-StatePath      { param([Parameter(Mandatory)] [string] $Dir) return (Join-Path $Dir 'state.json') }
function Get-TaskLockPath   { param([Parameter(Mandatory)] [string] $Dir) return (Join-Path $Dir 'task.lock') }
function Get-AssignmentsDir { param([Parameter(Mandatory)] [string] $Dir) return (Join-Path $Dir 'assignments') }
function Get-ResultsDir     { param([Parameter(Mandatory)] [string] $Dir) return (Join-Path $Dir 'results') }

function Read-State {
    param([Parameter(Mandatory)] [string] $Dir)

    $path = Get-StatePath -Dir $Dir
    if (-not (Test-Path $path)) { return $null }

    $raw = Get-Content $path -Raw
    if ([string]::IsNullOrWhiteSpace($raw)) {
        throw "TASK_STATE_CORRUPT: $path e' vuoto. Ispezionalo a mano."
    }
    $obj = $null
    try {
        $obj = ConvertFrom-Json $raw
    } catch {
        throw "TASK_STATE_CORRUPT: $path non e' JSON valido. Ispezionalo a mano."
    }

    $shape = Get-StateShapeError -State $obj
    if ($shape -ne '') {
        throw "${shape}: $path non ha la forma attesa. Ispezionalo a mano."
    }
    return $obj
}

<#
    Scrittura atomica: temp -> chiusura -> rename.

    Riscrivere `state.json` sul posto significa che un'interruzione lascia un JSON
    troncato, e un JSON troncato non e' distinguibile da un task che non esiste.
    Il rename e' l'unico istante in cui il file cambia, e non ha stati intermedi.
#>
function Write-StateAtomic {
    param([Parameter(Mandatory)] [string] $Dir, [Parameter(Mandatory)] $State)

    $path = Get-StatePath -Dir $Dir
    $tmp = "$path.tmp"
    Set-Content -Path $tmp -Value (ConvertTo-Json $State -Depth 8) -Encoding UTF8
    Move-Item -Path $tmp -Destination $path -Force
}

function Write-TextAtomic {
    param([Parameter(Mandatory)] [string] $Path, [Parameter(Mandatory)] [string] $Text)
    $tmp = "$Path.tmp"
    Set-Content -Path $tmp -Value $Text -Encoding UTF8
    Move-Item -Path $tmp -Destination $Path -Force
}

function Get-UtcNow { return (Get-Date).ToUniversalTime().ToString('o') }

<#
    Mutua esclusione fra due Coordinator. Non e' un sistema distribuito: e' un file
    aperto senza condivisione, come il registro dei workspace. Chi lo trova preso
    riceve BUSY e riprova, invece di scrivere sopra.
#>
function Invoke-WithTaskLock {
    param([Parameter(Mandatory)] [string] $Dir, [Parameter(Mandatory)] [scriptblock] $Body)

    if (-not (Test-Path $Dir)) { New-Item -ItemType Directory -Force -Path $Dir | Out-Null }

    $lock = $null
    try {
        $lock = [System.IO.File]::Open((Get-TaskLockPath -Dir $Dir), 'OpenOrCreate', 'ReadWrite', 'None')
    } catch {
        Write-Host "BUSY: un'altra mutazione di questo task e' in corso. Riprova." -ForegroundColor Yellow
        return 3
    }
    try {
        return (& $Body)
    } finally {
        if ($null -ne $lock) { $lock.Dispose() }
    }
}

<#
    L'ultimo risultato non e' uno stato in piu' da tenere allineato: si DERIVA dai
    file presenti. I nomi cominciano con la sequence a quattro cifre, quindi
    l'ordine lessicografico e' l'ordine cronologico del routing.
#>
function Get-LastResultRelative {
    param([Parameter(Mandatory)] [string] $Dir)

    $resultsDir = Get-ResultsDir -Dir $Dir
    if (-not (Test-Path $resultsDir)) { return '' }
    $files = @(Get-ChildItem -Path $resultsDir -Filter '*.md' -File | Sort-Object Name)
    if ($files.Count -eq 0) { return '' }
    return ('results/' + $files[$files.Count - 1].Name)
}

function New-HistoryEntry {
    param(
        [Parameter(Mandatory)] [int] $Sequence,
        [Parameter(Mandatory)] [string] $EventName,
        [string] $EntryActor = '',
        [string] $Detail = ''
    )
    return [pscustomobject]@{
        sequence = $Sequence
        event    = $EventName
        actor    = $EntryActor
        at       = (Get-UtcNow)
        detail   = $Detail
    }
}

function Get-HistoryList {
    param([Parameter(Mandatory)] $State)
    $list = @()
    foreach ($h in @($State.history)) { if ($null -ne $h) { $list += $h } }
    return $list
}

function Get-SessionRole {
    $role = $env:RT_TERMINAL_ROLE
    if ($null -eq $role) { return '' }
    return $role.Trim().ToUpperInvariant()
}

function Get-SessionInstance {
    $instance = $env:RT_TERMINAL_INSTANCE
    if ([string]::IsNullOrWhiteSpace($instance)) { return 'unknown' }
    # Diventa parte di un nome di file: la stessa grammatica del TaskId, senza il
    # vincolo sul primo carattere perche' qui non e' una directory.
    return (($instance -replace '[^A-Za-z0-9._-]', '_'))
}

# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------

function Format-Bullets {
    param([AllowNull()] [string[]] $Items, [string] $EmptyText = '(nessuno)')
    $clean = @()
    foreach ($i in @($Items)) { if (-not [string]::IsNullOrWhiteSpace($i)) { $clean += $i } }
    if ($clean.Count -eq 0) { return "- $EmptyText" }
    return (($clean | ForEach-Object { "- $_" }) -join [Environment]::NewLine)
}

function Get-BodyText {
    param([AllowNull()] [string] $Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { return '' }
    if (-not (Test-Path $Path)) { throw "BODY_FILE_NOT_FOUND: $Path" }
    return (Get-Content $Path -Raw)
}

function New-AssignmentText {
    param(
        [Parameter(Mandatory)] [string] $Id,
        [Parameter(Mandatory)] [string] $TargetActor,
        [Parameter(Mandatory)] [int] $Sequence,
        [Parameter(Mandatory)] [string] $From,
        [Parameter(Mandatory)] [string] $TheObjective,
        [string] $TheContext,
        [string[]] $TheInputs,
        [string[]] $TheDo,
        [string[]] $TheDoNot,
        [string[]] $TheExpectedOutput,
        [string] $TheNextIfPass,
        [string] $Notes
    )

    $lines = @()
    $lines += '=== RT3 TASK ASSIGNMENT ==='
    $lines += ''
    $lines += ("TASK:     {0}" -f $Id)
    $lines += ("ACTOR:    {0}" -f $TargetActor)
    $lines += ("SEQUENCE: {0}" -f (Format-Sequence -Sequence $Sequence))
    $lines += ("FROM:     {0}" -f $From)
    $lines += ("ISSUED:   {0}" -f (Get-UtcNow))
    $lines += ''
    $lines += '## OBJECTIVE'
    $lines += ''
    $lines += $TheObjective
    $lines += ''
    $lines += '## CONTEXT'
    $lines += ''
    if ([string]::IsNullOrWhiteSpace($TheContext)) { $lines += '(nessuno)' } else { $lines += $TheContext }
    $lines += ''
    $lines += '## INPUTS'
    $lines += ''
    $lines += (Format-Bullets -Items $TheInputs)
    $lines += ''
    $lines += '## DO'
    $lines += ''
    $lines += (Format-Bullets -Items $TheDo)
    $lines += ''
    $lines += '## DO_NOT'
    $lines += ''
    $lines += (Format-Bullets -Items $TheDoNot)
    $lines += ''
    $lines += '## EXPECTED_OUTPUT'
    $lines += ''
    $lines += (Format-Bullets -Items $TheExpectedOutput)
    $lines += ''
    $lines += '## NEXT_IF_PASS'
    $lines += ''
    if ([string]::IsNullOrWhiteSpace($TheNextIfPass)) {
        $lines += '(non dichiarato - lo decide il Coordinator sul risultato)'
    } else {
        $lines += ("{0} - informativo: la decisione resta del Coordinator." -f $TheNextIfPass)
    }
    if (-not [string]::IsNullOrWhiteSpace($Notes)) {
        $lines += ''
        $lines += '## NOTES'
        $lines += ''
        $lines += $Notes
    }
    $lines += ''
    return ($lines -join [Environment]::NewLine)
}

function New-ResultText {
    param(
        [Parameter(Mandatory)] [string] $Id,
        [Parameter(Mandatory)] [string] $ReportingRole,
        [Parameter(Mandatory)] [int] $Sequence,
        [Parameter(Mandatory)] [string] $ResultStatus,
        [Parameter(Mandatory)] [string] $TheSummary,
        [string[]] $TheChanges,
        [string[]] $TheEvidence,
        [string[]] $TheNotRun,
        [string] $TheBlocker,
        [string] $TheRecommendation,
        [string] $Instance,
        [string] $Notes
    )

    $lines = @()
    $lines += '=== RT3 TASK RESULT ==='
    $lines += ''
    $lines += ("TASK:                {0}" -f $Id)
    $lines += ("ROLE:                {0}" -f $ReportingRole)
    $lines += ("INSTANCE:            {0}" -f $Instance)
    $lines += ("ASSIGNMENT_SEQUENCE: {0}" -f (Format-Sequence -Sequence $Sequence))
    $lines += ("STATUS:              {0}" -f $ResultStatus)
    $lines += ("REPORTED:            {0}" -f (Get-UtcNow))
    $lines += ''
    $lines += '## SUMMARY'
    $lines += ''
    $lines += $TheSummary
    $lines += ''
    $lines += '## CHANGES'
    $lines += ''
    $lines += (Format-Bullets -Items $TheChanges -EmptyText 'nessuna modifica')
    $lines += ''
    $lines += '## EVIDENCE'
    $lines += ''
    $lines += (Format-Bullets -Items $TheEvidence -EmptyText 'nessuna')
    $lines += ''
    $lines += '## NOT_RUN'
    $lines += ''
    $lines += (Format-Bullets -Items $TheNotRun -EmptyText 'nessuno dichiarato')
    $lines += ''
    $lines += '## BLOCKER'
    $lines += ''
    if ([string]::IsNullOrWhiteSpace($TheBlocker)) { $lines += '(nessuno)' } else { $lines += $TheBlocker }
    $lines += ''
    $lines += '## NEXT_ACTOR_RECOMMENDED'
    $lines += ''
    if ([string]::IsNullOrWhiteSpace($TheRecommendation)) {
        $lines += '(nessuna raccomandazione)'
    } else {
        $lines += ("{0} - RACCOMANDAZIONE, non una decisione di routing." -f $TheRecommendation)
    }
    if (-not [string]::IsNullOrWhiteSpace($Notes)) {
        $lines += ''
        $lines += '## NOTES'
        $lines += ''
        $lines += $Notes
    }
    $lines += ''
    return ($lines -join [Environment]::NewLine)
}

function Write-Refusal {
    param([Parameter(Mandatory)] [string] $Code, [string] $Detail = '')
    Write-Host ("BLOCKED: {0}" -f $Code) -ForegroundColor Red
    if (-not [string]::IsNullOrWhiteSpace($Detail)) { Write-Host ("  {0}" -f $Detail) }
    return 2
}

# ---------------------------------------------------------------------------
# Azioni
# ---------------------------------------------------------------------------

function Invoke-Init {
    param([Parameter(Mandatory)] [string] $Id)

    if ([string]::IsNullOrWhiteSpace($script:Title)) {
        return (Write-Refusal 'TASK_TITLE_MISSING' '-Title e'' obbligatorio: un task senza titolo e'' illeggibile in `rttask list`.')
    }

    $dir = Get-TaskDir -Id $Id
    $initialActor = 'NONE'
    if (-not [string]::IsNullOrWhiteSpace($script:Actor)) { $initialActor = $script:Actor }
    $initialStatus = 'ACTIVE'
    if (-not [string]::IsNullOrWhiteSpace($script:TaskStatus)) { $initialStatus = $script:TaskStatus }

    return (Invoke-WithTaskLock -Dir $dir -Body {
        if (Test-Path (Get-StatePath -Dir $dir)) {
            return (Write-Refusal 'TASK_ALREADY_EXISTS' ("Il task {0} esiste gia'. Usa -Action status o -Action assign." -f $Id))
        }

        $now = Get-UtcNow
        $state = [pscustomobject]@{
            schema_version      = $script:SchemaVersion
            task_id             = $Id
            title               = $script:Title
            status              = $initialStatus
            next_actor          = $initialActor
            assignment_sequence = 0
            assignment_path     = ''
            last_result_path    = ''
            created_at          = $now
            updated_at          = $now
            history             = @((New-HistoryEntry -Sequence 0 -EventName 'init' -EntryActor $initialActor -Detail $script:Title))
        }

        New-Item -ItemType Directory -Force -Path (Get-AssignmentsDir -Dir $dir) | Out-Null
        New-Item -ItemType Directory -Force -Path (Get-ResultsDir -Dir $dir) | Out-Null
        Write-StateAtomic -Dir $dir -State $state

        Write-Host "TASK CREATED" -ForegroundColor Green
        Write-Host ("  task       : {0}" -f $Id)
        Write-Host ("  title      : {0}" -f $script:Title)
        Write-Host ("  status     : {0}" -f $initialStatus)
        Write-Host ("  next actor : {0}" -f $initialActor)
        Write-Host ("  store      : {0}" -f $dir) -ForegroundColor DarkGray
        return 0
    })
}

function Invoke-Assign {
    param([Parameter(Mandatory)] [string] $Id)

    if ([string]::IsNullOrWhiteSpace($script:Actor)) {
        return (Write-Refusal 'TASK_ACTOR_MISSING' '-Actor e'' obbligatorio per assign.')
    }

    $dir = Get-TaskDir -Id $Id
    if (-not (Test-Path (Get-StatePath -Dir $dir))) {
        return (Write-Refusal 'TASK_NOT_FOUND' ("Nessun task {0} nello store." -f $Id))
    }

    return (Invoke-WithTaskLock -Dir $dir -Body {
        $state = Read-State -Dir $dir

        if ([string]$state.status -eq 'DONE') {
            return (Write-Refusal 'TASK_ALREADY_DONE' 'Un task chiuso non riceve assignment. Aprine uno nuovo.')
        }

        $current = [int]$state.assignment_sequence

        # Guardia ottimistica. Non e' un lock: e' il rifiuto di scrivere sopra una
        # decisione che questo Coordinator non aveva letto.
        if ($script:ExpectedSequence -ge 0 -and $script:ExpectedSequence -ne $current) {
            return (Write-Refusal 'TASK_SEQUENCE_CONFLICT' ("Sequence attesa {0}, sul disco {1}. Rileggi lo stato prima di riassegnare." -f $script:ExpectedSequence, $current))
        }

        $next = $current + 1
        if (-not (Test-SequenceAdvance -Current $current -Proposed $next)) {
            return (Write-Refusal 'TASK_SEQUENCE_CONFLICT' 'La sequence non avanza.')
        }

        $needsAssignmentFile = ($script:Actor -ne 'NONE')
        if ($needsAssignmentFile) {
            $missing = @()
            if ([string]::IsNullOrWhiteSpace($script:Objective)) { $missing += '-Objective' }
            if (@($script:Do | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count -eq 0) { $missing += '-Do' }
            if (@($script:ExpectedOutput | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count -eq 0) { $missing += '-ExpectedOutput' }
            if ($missing.Count -gt 0) {
                return (Write-Refusal 'TASK_ASSIGNMENT_BODY_MISSING' ("Mancano: {0}. Un assignment senza obiettivo e senza output atteso non e'' una consegna." -f ($missing -join ', ')))
            }
        }

        # FROM e' il ruolo che ha lavorato PRIMA, non chi ha battuto il comando: e'
        # l'informazione che serve a chi riceve. Al primo giro non c'e' nessuno.
        $from = 'COORDINATOR'
        $previous = [string]$state.next_actor
        if ($current -ge 1 -and $previous -ne 'NONE') { $from = $previous }

        $assignmentRelative = ''
        if ($needsAssignmentFile) {
            $fileName = ("{0}-{1}.md" -f (Format-Sequence -Sequence $next), $script:Actor)
            $assignmentsDir = Get-AssignmentsDir -Dir $dir
            New-Item -ItemType Directory -Force -Path $assignmentsDir | Out-Null
            $fullPath = Join-Path $assignmentsDir $fileName

            # Un assignment emesso non si modifica: una correzione e' una sequence
            # nuova, altrimenti chi l'ha gia' letto sta lavorando su un testo che
            # non esiste piu'.
            if (Test-Path $fullPath) {
                return (Write-Refusal 'TASK_ASSIGNMENT_IMMUTABLE' ("{0} esiste gia'. Una correzione produce una sequence nuova." -f $fullPath))
            }

            $notes = Get-BodyText -Path $script:BodyFile
            $text = New-AssignmentText -Id $Id -TargetActor $script:Actor -Sequence $next -From $from `
                -TheObjective $script:Objective -TheContext $script:Context -TheInputs $script:Inputs `
                -TheDo $script:Do -TheDoNot $script:DoNot -TheExpectedOutput $script:ExpectedOutput `
                -TheNextIfPass $script:NextIfPass -Notes $notes
            Write-TextAtomic -Path $fullPath -Text $text
            $assignmentRelative = 'assignments/' + $fileName
        }

        $newStatus = 'ACTIVE'
        if (-not [string]::IsNullOrWhiteSpace($script:TaskStatus)) { $newStatus = $script:TaskStatus }

        $history = @(Get-HistoryList -State $state)
        $history += (New-HistoryEntry -Sequence $next -EventName 'assign' -EntryActor $script:Actor -Detail $script:Objective)

        $state.status              = $newStatus
        $state.next_actor          = $script:Actor
        $state.assignment_sequence = $next
        $state.assignment_path     = $assignmentRelative
        $state.last_result_path    = (Get-LastResultRelative -Dir $dir)
        $state.updated_at          = (Get-UtcNow)
        $state.history             = $history

        Write-StateAtomic -Dir $dir -State $state

        Write-Host "ASSIGNED" -ForegroundColor Green
        Write-Host ("  task       : {0}" -f $Id)
        Write-Host ("  actor      : {0}" -f $script:Actor)
        Write-Host ("  sequence   : {0}" -f (Format-Sequence -Sequence $next))
        Write-Host ("  from       : {0}" -f $from)
        if ($assignmentRelative -ne '') {
            Write-Host ("  assignment : {0}" -f (Join-Path $dir ($assignmentRelative -replace '/', '\')))
        } else {
            Write-Host "  assignment : nessuno (actor NONE)" -ForegroundColor DarkGray
        }
        return 0
    })
}

function Invoke-Close {
    param([Parameter(Mandatory)] [string] $Id)

    $dir = Get-TaskDir -Id $Id
    if (-not (Test-Path (Get-StatePath -Dir $dir))) {
        return (Write-Refusal 'TASK_NOT_FOUND' ("Nessun task {0} nello store." -f $Id))
    }

    return (Invoke-WithTaskLock -Dir $dir -Body {
        $state = Read-State -Dir $dir

        if ([string]$state.status -eq 'DONE') {
            return (Write-Refusal 'TASK_ALREADY_DONE' 'Il task era gia'' chiuso.')
        }

        $current = [int]$state.assignment_sequence
        if ($script:ExpectedSequence -ge 0 -and $script:ExpectedSequence -ne $current) {
            return (Write-Refusal 'TASK_SEQUENCE_CONFLICT' ("Sequence attesa {0}, sul disco {1}." -f $script:ExpectedSequence, $current))
        }

        $history = @(Get-HistoryList -State $state)
        $history += (New-HistoryEntry -Sequence $current -EventName 'close' -EntryActor 'NONE' -Detail $script:Reason)

        $state.status           = 'DONE'
        $state.next_actor       = 'NONE'
        $state.last_result_path = (Get-LastResultRelative -Dir $dir)
        $state.updated_at       = (Get-UtcNow)
        $state.history          = $history

        Write-StateAtomic -Dir $dir -State $state

        Write-Host "TASK CLOSED" -ForegroundColor Green
        Write-Host ("  task   : {0}" -f $Id)
        if (-not [string]::IsNullOrWhiteSpace($script:Reason)) { Write-Host ("  reason : {0}" -f $script:Reason) }
        return 0
    })
}

<#
    Il worker deposita un risultato. NON tocca `state.json`: e' l'invariante che
    tiene una sola autorita' sul routing, ed e' provata da un caso di -SelfTest.
#>
function Invoke-Report {
    param([Parameter(Mandatory)] [string] $Id)

    $role = Get-SessionRole

    if ([string]::IsNullOrWhiteSpace($script:Status)) {
        return (Write-Refusal 'RESULT_STATUS_MISSING' '-Status e'' obbligatorio (DONE | PARTIAL | BLOCKED | FAILED).')
    }
    if ([string]::IsNullOrWhiteSpace($script:Summary)) {
        return (Write-Refusal 'RESULT_SUMMARY_MISSING' '-Summary e'' obbligatorio: un risultato senza sintesi non e'' leggibile dal Coordinator.')
    }

    $dir = Get-TaskDir -Id $Id
    $state = $null
    if (Test-Path (Get-StatePath -Dir $dir)) { $state = Read-State -Dir $dir }

    $verdict = Get-RouteVerdict -State $state -SessionRole $role
    if (-not $verdict.Ok) {
        $detail = ("atteso {0}, questo terminale {1}" -f $verdict.Expected, $verdict.Actual)
        return (Write-Refusal $verdict.Code $detail)
    }

    $sequence = [int]$state.assignment_sequence
    $instance = Get-SessionInstance
    $resultsDir = Get-ResultsDir -Dir $dir
    New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null

    # Append-only: un risultato gia' depositato non viene sovrascritto. Un secondo
    # invio dello stesso ruolo sulla stessa sequence e' un fatto, e resta leggibile.
    $base = ("{0}-{1}-{2}" -f (Format-Sequence -Sequence $sequence), $role, $instance)
    $fileName = "$base.md"
    $attempt = 1
    while (Test-Path (Join-Path $resultsDir $fileName)) {
        $attempt++
        $fileName = ("{0}-{1}.md" -f $base, $attempt)
    }
    $fullPath = Join-Path $resultsDir $fileName

    $notes = Get-BodyText -Path $script:BodyFile
    $text = New-ResultText -Id $Id -ReportingRole $role -Sequence $sequence -ResultStatus $script:Status `
        -TheSummary $script:Summary -TheChanges $script:Changes -TheEvidence $script:Evidence `
        -TheNotRun $script:NotRun -TheBlocker $script:Blocker -TheRecommendation $script:NextActorRecommended `
        -Instance $instance -Notes $notes
    Write-TextAtomic -Path $fullPath -Text $text

    Write-Host "RESULT RECORDED" -ForegroundColor Green
    Write-Host ("  task     : {0}" -f $Id)
    Write-Host ("  role     : {0}" -f $role)
    Write-Host ("  sequence : {0}" -f (Format-Sequence -Sequence $sequence))
    Write-Host ("  file     : {0}" -f $fullPath)
    Write-Host ""
    Write-Host "RETURN TO RT COORDINATOR" -ForegroundColor Cyan
    Write-Host "Il routing NON e' cambiato: NEXT_ACTOR_RECOMMENDED e' una raccomandazione." -ForegroundColor DarkGray
    return 0
}

function Get-AssignmentFrom {
    param([Parameter(Mandatory)] [string] $Dir, [AllowNull()] [string] $Relative)
    if ([string]::IsNullOrWhiteSpace($Relative)) { return '' }
    $path = Join-Path $Dir ($Relative -replace '/', '\')
    if (-not (Test-Path $path)) { return '' }
    foreach ($line in (Get-Content $path)) {
        if ($line -match '^FROM:\s*(.+)$') { return $Matches[1].Trim() }
    }
    return ''
}

<#
    Stato di un attore, DERIVATO. Non e' un campo in piu' da tenere allineato:
    esiste gia' tutto nei file, e uno stato duplicato e' uno stato che diverge.
#>
function Get-ActorProgress {
    param([Parameter(Mandatory)] [string] $Dir, [Parameter(Mandatory)] $State, [Parameter(Mandatory)] [string] $TargetActor)

    $assignmentsDir = Get-AssignmentsDir -Dir $Dir
    if (-not (Test-Path $assignmentsDir)) { return 'NOT ASSIGNED' }

    $mine = @(Get-ChildItem -Path $assignmentsDir -Filter ("*-{0}.md" -f $TargetActor) -File | Sort-Object Name)
    if ($mine.Count -eq 0) { return 'NOT ASSIGNED' }

    $lastSeq = ($mine[$mine.Count - 1].Name -split '-')[0]

    $resultsDir = Get-ResultsDir -Dir $Dir
    if (Test-Path $resultsDir) {
        $results = @(Get-ChildItem -Path $resultsDir -Filter ("{0}-{1}-*.md" -f $lastSeq, $TargetActor) -File | Sort-Object Name)
        if ($results.Count -gt 0) {
            foreach ($line in (Get-Content $results[$results.Count - 1].FullName)) {
                if ($line -match '^STATUS:\s*(\S+)') { return $Matches[1] }
            }
            return 'REPORTED'
        }
    }
    return 'IN PROGRESS'
}

<#
    L'unico accessore MACCHINA del router: scrive il prossimo actor su stdout e
    nient'altro.

    Esiste perche' `rt-open-task.ps1` deve sapere quale ruolo aprire, e l'unica
    alternativa sarebbe leggere l'output umano di `status` con una regex - un
    formato che nessuno si e' impegnato a non cambiare.

    (!) Usa [Console]::Out e non Write-Output: il valore di ritorno della funzione
    e' l'exit code, e i due finirebbero nello stesso stream.
#>
function Invoke-Next {
    param([Parameter(Mandatory)] [string] $Id)

    $dir = Get-TaskDir -Id $Id
    if (-not (Test-Path (Get-StatePath -Dir $dir))) {
        return (Write-Refusal 'TASK_NOT_FOUND' ("Nessun task {0} nello store." -f $Id))
    }
    $state = Read-State -Dir $dir

    if ([string]$state.status -eq 'DONE') {
        return (Write-Refusal 'TASK_ALREADY_DONE' 'Il task e'' chiuso: non c''e'' un prossimo actor.')
    }

    [Console]::Out.WriteLine([string]$state.next_actor)
    return 0
}

function Invoke-Status {
    param([Parameter(Mandatory)] [string] $Id)

    $dir = Get-TaskDir -Id $Id
    if (-not (Test-Path (Get-StatePath -Dir $dir))) {
        return (Write-Refusal 'TASK_NOT_FOUND' ("Nessun task {0} nello store {1}." -f $Id, (Get-TasksRoot)))
    }
    $state = Read-State -Dir $dir

    Write-Host ("TASK: #{0}" -f $state.task_id) -ForegroundColor White
    Write-Host ("Title  : {0}" -f $state.title)
    Write-Host ("Status : {0}" -f $state.status)
    Write-Host ""
    foreach ($a in $script:WorkerActors) {
        Write-Host ("{0,-11}: {1}" -f $a, (Get-ActorProgress -Dir $dir -State $state -TargetActor $a))
    }
    Write-Host ""
    Write-Host ("NEXT: {0}" -f $state.next_actor) -ForegroundColor Cyan
    Write-Host ""
    Write-Host ("Assignment  : {0}" -f $(if ($state.assignment_path) { $state.assignment_path } else { '<nessuno>' }))
    Write-Host ("Sequence    : {0}" -f (Format-Sequence -Sequence ([int]$state.assignment_sequence)))
    $lastResult = Get-LastResultRelative -Dir $dir
    Write-Host ("Last result : {0}" -f $(if ($lastResult) { $lastResult } else { '<nessuno>' }))
    Write-Host ("Store       : {0}" -f $dir) -ForegroundColor DarkGray

    $history = @(Get-HistoryList -State $state)
    if ($history.Count -gt 0) {
        Write-Host ""
        Write-Host "HISTORY (ultime 5)" -ForegroundColor DarkGray
        $tail = $history
        if ($history.Count -gt 5) { $tail = $history[($history.Count - 5)..($history.Count - 1)] }
        foreach ($h in $tail) {
            Write-Host ("  {0}  {1,-7} {2,-11} {3}" -f (Format-Sequence -Sequence ([int]$h.sequence)), $h.event, $h.actor, $h.at) -ForegroundColor DarkGray
        }
    }
    return 0
}

function Invoke-List {
    $root = Get-TasksRoot
    $dirs = @(Get-ChildItem -Path $root -Directory -ErrorAction SilentlyContinue | Sort-Object Name)

    if ($dirs.Count -eq 0) {
        Write-Host "Nessun task nello store." -ForegroundColor DarkGray
        Write-Host ("  {0}" -f $root) -ForegroundColor DarkGray
        return 0
    }

    foreach ($d in $dirs) {
        $state = $null
        $broken = ''
        try { $state = Read-State -Dir $d.FullName } catch { $broken = 'TASK_STATE_CORRUPT' }

        if ($broken -ne '') {
            Write-Host ("{0,-8} {1,-8} {2,-11} {3}" -f $d.Name, 'CORRUPT', '-', $broken) -ForegroundColor Red
            continue
        }
        if ($null -eq $state) { continue }

        $color = switch ([string]$state.status) {
            'ACTIVE'  { 'White' }
            'BLOCKED' { 'Yellow' }
            'DONE'    { 'DarkGray' }
            default   { 'White' }
        }
        Write-Host ("{0,-8} {1,-8} {2,-11} {3}" -f $state.task_id, $state.status, $state.next_actor, $state.title) -ForegroundColor $color
    }
    return 0
}

function Invoke-ShowAssignment {
    param([Parameter(Mandatory)] [string] $Id)

    $dir = Get-TaskDir -Id $Id
    if (-not (Test-Path (Get-StatePath -Dir $dir))) {
        return (Write-Refusal 'TASK_NOT_FOUND' ("Nessun task {0} nello store." -f $Id))
    }
    $state = Read-State -Dir $dir

    $relative = [string]$state.assignment_path
    if ([string]::IsNullOrWhiteSpace($relative)) {
        return (Write-Refusal 'TASK_ASSIGNMENT_MISSING' 'Nessun assignment emesso per questo task.')
    }
    $path = Join-Path $dir ($relative -replace '/', '\')
    if (-not (Test-Path $path)) {
        return (Write-Refusal 'TASK_ASSIGNMENT_MISSING' ("{0} non esiste: lo stato lo cita ma il file non c'e''." -f $path))
    }
    Write-Host (Get-Content $path -Raw)
    return 0
}

<#
    Il banner di avvio del terminale. Sola lettura: un mismatch NON cambia niente,
    perche' la sessione sbagliata non deve poter riscrivere il routing per
    "sistemarlo".
#>
function Invoke-Route {
    param([Parameter(Mandatory)] [string] $Id)

    $sessionRole = $script:Role
    if ([string]::IsNullOrWhiteSpace($sessionRole)) { $sessionRole = Get-SessionRole }

    $dir = Get-TaskDir -Id $Id
    $state = $null
    if (Test-Path (Get-StatePath -Dir $dir)) {
        try { $state = Read-State -Dir $dir } catch { $state = $null }
    }

    $verdict = Get-RouteVerdict -State $state -SessionRole $sessionRole
    $bar = ('=' * 52)

    if ($verdict.Ok) {
        $sequence = Format-Sequence -Sequence ([int]$state.assignment_sequence)
        $from = Get-AssignmentFrom -Dir $dir -Relative ([string]$state.assignment_path)
        if ([string]::IsNullOrWhiteSpace($from)) { $from = 'COORDINATOR' }

        Write-Host $bar -ForegroundColor Cyan
        Write-Host (" RT3 {0}" -f $sessionRole) -ForegroundColor Cyan
        Write-Host $bar -ForegroundColor Cyan
        Write-Host (" TASK       : {0}" -f $state.task_id)
        Write-Host (" TITLE      : {0}" -f $state.title)
        Write-Host (" ASSIGNMENT : {0}" -f $sequence)
        Write-Host (" EXPECTED   : {0}" -f $verdict.Expected)
        Write-Host (" THIS ROLE  : {0}" -f $verdict.Actual)
        Write-Host " ROUTING    : ASSIGNED" -ForegroundColor Green
        Write-Host (" FROM       : {0}" -f $from)
        Write-Host (" NEXT IF PASS: {0}" -f (Get-NextIfPassLine -Dir $dir -Relative ([string]$state.assignment_path)))
        Write-Host $bar -ForegroundColor Cyan
        Write-Host ""
        Write-Host ("Leggi l'assignment: rttask assignment -TaskId {0}" -f $Id) -ForegroundColor DarkGray
        return 0
    }

    if ($verdict.Code -eq 'TASK_ROUTE_MISMATCH') {
        Write-Host ""
        Write-Host "TASK ROUTING MISMATCH" -ForegroundColor Red
        Write-Host ""
        Write-Host ("Current terminal: {0}" -f $verdict.Actual)
        Write-Host ("Expected actor: {0}" -f $verdict.Expected)
        Write-Host ("Task: {0}" -f $Id)
        Write-Host ""
        Write-Host "DO NOT EXECUTE THIS ASSIGNMENT." -ForegroundColor Red
        Write-Host "Nessuno stato e' stato modificato." -ForegroundColor DarkGray
        return 2
    }

    Write-Host ""
    Write-Host ("TASK ROUTING: {0}" -f $verdict.Code) -ForegroundColor Yellow
    Write-Host ("Task: {0}" -f $Id)
    if ($verdict.Expected -ne '') { Write-Host ("Expected actor: {0}" -f $verdict.Expected) }

    switch ($verdict.Code) {
        'TASK_NOT_FOUND'        { Write-Host "Il task non esiste nello store per macchina. Crealo dal Coordinator." }
        'TASK_ALREADY_DONE'     { Write-Host "Il task e' chiuso: non c'e' lavoro da eseguire." }
        'TASK_ASSIGNMENT_MISSING' { Write-Host "Nessun assignment emesso: torna al Coordinator." }
        'TASK_ACTOR_NOT_WORKER' {
            if ($verdict.Expected -eq 'USER') {
                Write-Host "Tocca a te, non a un ruolo RT3: serve un giudizio umano." -ForegroundColor Cyan
                Write-Host ("Leggi cosa: rttask assignment -TaskId {0}" -f $Id) -ForegroundColor Cyan
            } else {
                Write-Host "Nessun actor assegnato. Torna al Coordinator."
            }
        }
        'RT_SESSION_REQUIRED'   { Write-Host "Questa sessione non dichiara un ruolo RT3." }
        default                 { Write-Host "Stato non utilizzabile: ispeziona lo store." }
    }
    Write-Host "Nessuno stato e' stato modificato." -ForegroundColor DarkGray
    return 2
}

function Get-NextIfPassLine {
    param([Parameter(Mandatory)] [string] $Dir, [AllowNull()] [string] $Relative)
    if ([string]::IsNullOrWhiteSpace($Relative)) { return '(non dichiarato)' }
    $path = Join-Path $Dir ($Relative -replace '/', '\')
    if (-not (Test-Path $path)) { return '(non dichiarato)' }
    $lines = @(Get-Content $path)
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -eq '## NEXT_IF_PASS') {
            for ($j = $i + 1; $j -lt $lines.Count; $j++) {
                if (-not [string]::IsNullOrWhiteSpace($lines[$j])) { return $lines[$j] }
            }
        }
    }
    return '(non dichiarato)'
}

# ---------------------------------------------------------------------------
# Self-test
#
# Due strati, e la distinzione conta.
#
#   PURO          le regole - id, sequence, forma, routing, policy. Non toccano
#                 il disco, e sono quelle che si possono provare senza fabbricare
#                 un task vero.
#   INTEGRAZIONE  il router come lo usa un umano, in uno store TEMPORANEO passato
#                 con -StoreRoot. Invoca questo stesso script come processo figlio,
#                 percio' misura anche il binding dei parametri e gli exit code -
#                 che una chiamata in-process non vedrebbe.
#
# (!!) Non avvia Unreal, non tocca il lease, non tocca `rtmode`. Due casi lo
# PROVANO invece di dichiararlo: sono #19 e #20.
# ---------------------------------------------------------------------------

function Invoke-SelfTest {
    $script:stCount = 0
    $script:stFail = 0

    function Check {
        param([string] $Name, [bool] $Actual, [bool] $Expected = $true)
        $script:stCount++
        if ($Actual -eq $Expected) {
            Write-Host ("  PASS  {0}" -f $Name) -ForegroundColor Green
        } else {
            $script:stFail++
            Write-Host ("  FAIL  {0}  (atteso {1}, ottenuto {2})" -f $Name, $Expected, $Actual) -ForegroundColor Red
        }
    }

    # -----------------------------------------------------------------------
    Write-Host "Test-TaskIdValid (puro)" -ForegroundColor White
    Check 'id numerico'                 (Test-TaskIdValid -Id '2330')
    Check 'id alfanumerico con trattini' (Test-TaskIdValid -Id 'rt3-task-router.v2')
    Check 'vuoto'                       (Test-TaskIdValid -Id '')            $false
    Check 'solo spazi'                  (Test-TaskIdValid -Id '   ')         $false
    Check 'null'                        (Test-TaskIdValid -Id $null)         $false
    Check 'slash'                       (Test-TaskIdValid -Id 'a/b')         $false
    Check 'backslash'                   (Test-TaskIdValid -Id 'a\b')         $false
    Check 'dot dot'                     (Test-TaskIdValid -Id '..')          $false
    Check 'traversal relativo'          (Test-TaskIdValid -Id '../evil')     $false
    Check 'path assoluto'               (Test-TaskIdValid -Id 'C:\Windows')  $false
    Check 'inizia con punto'            (Test-TaskIdValid -Id '.git')        $false
    Check 'inizia con trattino'         (Test-TaskIdValid -Id '-Action')     $false
    Check 'due punti'                   (Test-TaskIdValid -Id 'a:b')         $false
    Check 'spazio interno'              (Test-TaskIdValid -Id 'a b')         $false
    Check 'wildcard'                    (Test-TaskIdValid -Id 'a*')          $false
    Check 'device DOS'                  (Test-TaskIdValid -Id 'CON')         $false
    Check 'device DOS minuscolo'        (Test-TaskIdValid -Id 'nul')         $false
    Check '64 caratteri'                (Test-TaskIdValid -Id ('a' * 64))
    Check '65 caratteri'                (Test-TaskIdValid -Id ('a' * 65))    $false

    Write-Host "Test-SequenceAdvance (puro)" -ForegroundColor White
    Check 'avanza'      (Test-SequenceAdvance -Current 1 -Proposed 2)
    Check 'ferma'       (Test-SequenceAdvance -Current 2 -Proposed 2) $false
    Check 'arretra'     (Test-SequenceAdvance -Current 3 -Proposed 2) $false

    Write-Host "Get-MutationPolicy (puro)" -ForegroundColor White
    Check 'coordinator puo assign'     (Get-MutationPolicy -RequestedAction 'assign' -SessionRole '').Allowed
    Check 'coordinator puo init'       (Get-MutationPolicy -RequestedAction 'init'   -SessionRole '').Allowed
    Check 'coordinator puo close'      (Get-MutationPolicy -RequestedAction 'close'  -SessionRole '').Allowed
    Check 'DEV non puo assign'         (Get-MutationPolicy -RequestedAction 'assign' -SessionRole 'DEV').Allowed    $false
    Check 'EDITOR non puo init'        (Get-MutationPolicy -RequestedAction 'init'   -SessionRole 'EDITOR').Allowed $false
    Check 'VALIDATION non puo close'   (Get-MutationPolicy -RequestedAction 'close'  -SessionRole 'VALIDATION').Allowed $false
    Check 'codice di rifiuto stabile'  ((Get-MutationPolicy -RequestedAction 'assign' -SessionRole 'DEV').Code -eq 'TASK_MUTATION_ROLE_DENIED')
    Check 'DEV puo report'             (Get-MutationPolicy -RequestedAction 'report' -SessionRole 'DEV').Allowed
    Check 'coordinator NON puo report' (Get-MutationPolicy -RequestedAction 'report' -SessionRole '').Allowed $false
    Check 'status sempre leggibile'    (Get-MutationPolicy -RequestedAction 'status' -SessionRole 'DEV').Allowed
    Check 'route sempre leggibile'     (Get-MutationPolicy -RequestedAction 'route'  -SessionRole 'EDITOR').Allowed

    Write-Host "Get-StateShapeError (puro)" -ForegroundColor White
    $good = [pscustomobject]@{
        schema_version = 1; task_id = '2330'; title = 't'; status = 'ACTIVE'; next_actor = 'DEV'
        assignment_sequence = 1; assignment_path = 'assignments/0001-DEV.md'; last_result_path = ''
        created_at = 'T0'; updated_at = 'T0'; history = @()
    }
    Check 'stato valido'          ((Get-StateShapeError -State $good) -eq '')
    Check 'null e corrotto'       ((Get-StateShapeError -State $null) -eq 'TASK_STATE_CORRUPT')
    $noField = $good.PSObject.Copy(); $noField.PSObject.Properties.Remove('next_actor')
    Check 'campo mancante'        ((Get-StateShapeError -State $noField) -eq 'TASK_STATE_CORRUPT')
    $badStatus = $good.PSObject.Copy(); $badStatus.status = 'WAITING_EDITOR'
    Check 'status inventato'      ((Get-StateShapeError -State $badStatus) -eq 'TASK_STATE_CORRUPT')
    $badActor = $good.PSObject.Copy(); $badActor.next_actor = 'DEV-LEAD'
    Check 'actor fuori dai cinque' ((Get-StateShapeError -State $badActor) -eq 'TASK_STATE_CORRUPT')
    $badSeq = $good.PSObject.Copy(); $badSeq.assignment_sequence = -1
    Check 'sequence negativa'     ((Get-StateShapeError -State $badSeq) -eq 'TASK_STATE_CORRUPT')

    Write-Host "Get-RouteVerdict (puro)" -ForegroundColor White
    Check 'ruolo atteso'             (Get-RouteVerdict -State $good -SessionRole 'DEV').Ok
    Check 'ruolo diverso'            (Get-RouteVerdict -State $good -SessionRole 'EDITOR').Ok $false
    Check 'mismatch tipizzato'       ((Get-RouteVerdict -State $good -SessionRole 'EDITOR').Code -eq 'TASK_ROUTE_MISMATCH')
    Check 'task assente'             ((Get-RouteVerdict -State $null -SessionRole 'DEV').Code -eq 'TASK_NOT_FOUND')
    Check 'sessione senza ruolo'     ((Get-RouteVerdict -State $good -SessionRole '').Code -eq 'RT_SESSION_REQUIRED')
    $done = $good.PSObject.Copy(); $done.status = 'DONE'
    Check 'task chiuso'              ((Get-RouteVerdict -State $done -SessionRole 'DEV').Code -eq 'TASK_ALREADY_DONE')
    $userNext = $good.PSObject.Copy(); $userNext.next_actor = 'USER'
    Check 'tocca allo USER'          ((Get-RouteVerdict -State $userNext -SessionRole 'DEV').Code -eq 'TASK_ACTOR_NOT_WORKER')
    $noneNext = $good.PSObject.Copy(); $noneNext.next_actor = 'NONE'
    Check 'nessun actor'             ((Get-RouteVerdict -State $noneNext -SessionRole 'DEV').Code -eq 'TASK_ACTOR_NOT_WORKER')
    $noAssign = $good.PSObject.Copy(); $noAssign.assignment_sequence = 0; $noAssign.assignment_path = ''
    Check 'assignment mancante'      ((Get-RouteVerdict -State $noAssign -SessionRole 'DEV').Code -eq 'TASK_ASSIGNMENT_MISSING')

    # -----------------------------------------------------------------------
    # Integrazione
    # -----------------------------------------------------------------------
    Write-Host "Integrazione (store temporaneo)" -ForegroundColor White

    $hostPath = [System.Diagnostics.Process]::GetCurrentProcess().MainModule.FileName
    $selfPath = $PSCommandPath
    # (!!) Lo store di prova sta DENTRO una cartella privata, non direttamente in
    # %TEMP%. Il caso "nessuna directory fuori dallo store" guarda il PADRE dello
    # store: se quel padre fosse %TEMP%, una sola esecuzione con la grammatica del
    # TaskId rotta ci lascerebbe una cartella `evil` -- e da quel momento il caso
    # fallirebbe per sempre, anche a codice riparato. Misurato il 2026-09-06
    # mutando `Test-TaskIdValid`: la cartella e' stata creata davvero.
    $sandbox = Join-Path ([System.IO.Path]::GetTempPath()) ("rt-task-router-selftest-" + [System.Guid]::NewGuid().ToString('N'))
    $tmpRoot = Join-Path $sandbox 'store'
    New-Item -ItemType Directory -Force -Path $tmpRoot | Out-Null

    # Cio' che questo self-test non deve toccare. Lo misuriamo prima e dopo.
    $rt3Dir = ''
    if (-not [string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) {
        $rt3Dir = Join-Path (Join-Path $env:LOCALAPPDATA 'RefactorTactics') 'RT3'
    }
    function Get-Fingerprint([string] $Path) {
        if ([string]::IsNullOrWhiteSpace($Path)) { return 'NO-PATH' }
        if (-not (Test-Path $Path)) { return 'ASSENTE' }
        return (Get-FileHash -Path $Path -Algorithm SHA256).Hash
    }
    $leaseBefore  = Get-Fingerprint (Join-Path $rt3Dir 'lease.json')
    $eventsBefore = Get-Fingerprint (Join-Path $rt3Dir 'events.jsonl')
    $modeFile = ''
    if (-not [string]::IsNullOrWhiteSpace($script:WorkspaceRoot)) {
        $modeFile = Join-Path (Join-Path $script:WorkspaceRoot '.vscode') 'rt-engine-mode.txt'
    }
    $modeBefore = Get-Fingerprint $modeFile

    # (!!) "Nessun processo Unreal" NON si misura contando i processi della macchina.
    # Misurato il 2026-09-06: durante una singola esecuzione di questo self-test
    # l'insieme e' cambiato due volte, perche' un'altra sessione stava eseguendo una
    # suite. Un test scritto cosi' diventa rosso per il lavoro di qualcun altro, e
    # al terzo falso positivo lo si smette di guardare.
    #
    # L'invariante che riguarda DAVVERO questo script e' statica: la sua meta'
    # operativa non nomina il motore, il lease o rtmode. La verifica gira sui TOKEN
    # -- non sul testo -- perche' i commenti quei nomi li contengono apposta, per
    # dire che non vengono usati.
    function Test-NoEngineReference {
        param([Parameter(Mandatory)] [string] $Path)
        $tk = $null; $er = $null
        $null = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref] $tk, [ref] $er)
        $cut = ($tk | Where-Object { $_.Text -eq 'Invoke-SelfTest' } | Select-Object -First 1)
        if ($null -eq $cut) { return $false }
        $code = (@($tk |
            Where-Object { $_.Kind -ne 'Comment' -and $_.Extent.StartOffset -lt $cut.Extent.StartOffset } |
            ForEach-Object { $_.Text }) -join ' ')
        return (-not ($code -match 'UnrealEditor|Build\.bat|rt-suite|rt-lease|rt-mode|rtmode|rtlease|rtsuite'))
    }

    $savedRole = $env:RT_TERMINAL_ROLE
    $savedInstance = $env:RT_TERMINAL_INSTANCE

    function Invoke-Router {
        param([string] $AsRole, [string[]] $RouterArgs)
        if ([string]::IsNullOrWhiteSpace($AsRole)) {
            Remove-Item Env:\RT_TERMINAL_ROLE -ErrorAction SilentlyContinue
        } else {
            $env:RT_TERMINAL_ROLE = $AsRole
        }
        $all = @($RouterArgs) + @('-StoreRoot', $script:tmpRootShared)
        $text = (& $script:hostPathShared -NoLogo -NoProfile -File $script:selfPathShared @all 2>&1 | Out-String)
        return [pscustomobject]@{ Code = $LASTEXITCODE; Text = $text }
    }

    $script:hostPathShared = $hostPath
    $script:selfPathShared = $selfPath
    $script:tmpRootShared = $tmpRoot
    $env:RT_TERMINAL_INSTANCE = 'selftest'

    try {
        # 1 - init
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'init', '-TaskId', '2330', '-Title', 'GrayKit Arena Door')
        Check '1 init esce 0' ($r.Code -eq 0)
        $stateFile = Join-Path (Join-Path $tmpRoot '2330') 'state.json'
        Check '1 state.json creato' (Test-Path $stateFile)
        $parsed = $null
        try { $parsed = Get-Content $stateFile -Raw | ConvertFrom-Json } catch { $parsed = $null }
        Check '1 state.json e JSON valido' ($null -ne $parsed)
        Check '1 schema versionato' ($null -ne $parsed -and [int]$parsed.schema_version -eq 1)
        Check '1 nasce senza actor' ($null -ne $parsed -and [string]$parsed.next_actor -eq 'NONE')

        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'init', '-TaskId', '2330', '-Title', 'doppione')
        Check '1 init ripetuto rifiutato' ($r.Code -eq 2 -and $r.Text -match 'TASK_ALREADY_EXISTS')

        # 2 - task id illegali
        foreach ($bad in @('../evil', 'a/b', '..', '.git', 'CON', ('a' * 65))) {
            $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'init', '-TaskId', $bad, '-Title', 'x')
            Check ("2 id illegale rifiutato: {0}" -f $bad) ($r.Code -eq 2 -and $r.Text -match 'TASK_ID_INVALID')
        }
        Check '2 nessuna directory fuori dallo store' (-not (Test-Path (Join-Path (Split-Path -Parent $tmpRoot) 'evil')))

        # 3 - status
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'status', '-TaskId', '2330')
        Check '3 status esce 0' ($r.Code -eq 0)
        Check '3 status mostra NEXT' ($r.Text -match 'NEXT: NONE')
        Check '3 status mostra i tre ruoli' ($r.Text -match 'DEV' -and $r.Text -match 'EDITOR' -and $r.Text -match 'VALIDATION')
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'status', '-TaskId', '9999')
        Check '3 status di un task assente' ($r.Code -eq 2 -and $r.Text -match 'TASK_NOT_FOUND')

        # 4 - due task contemporanei
        Invoke-Router -AsRole '' -RouterArgs @('-Action', 'init', '-TaskId', '2501', '-Title', 'Move decay') | Out-Null
        Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '2501', '-Actor', 'DEV',
            '-Objective', 'ridurre il decay', '-Do', 'implementa', '-ExpectedOutput', 'test verde') | Out-Null
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'list')
        Check '4 list mostra 2330' ($r.Text -match '2330')
        Check '4 list mostra 2501' ($r.Text -match '2501')
        Check '4 i due task hanno actor distinti' ($r.Text -match '2330\s+ACTIVE\s+NONE' -and $r.Text -match '2501\s+ACTIVE\s+DEV')

        # 5 - assignment DEV
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '2330', '-Actor', 'DEV',
            '-Objective', 'aggiungere il contratto della porta', '-Context', 'wave graykit',
            '-Inputs', 'docs/spec.md', '-Do', 'scrivi il test', '-DoNot', 'non aprire Unreal',
            '-ExpectedOutput', 'Automation verde', '-NextIfPass', 'VALIDATION')
        Check '5 assign esce 0' ($r.Code -eq 0)
        $assignmentFile = Join-Path (Join-Path (Join-Path $tmpRoot '2330') 'assignments') '0001-DEV.md'
        Check '5 assignment scritto' (Test-Path $assignmentFile)
        $aText = Get-Content $assignmentFile -Raw
        foreach ($section in @('TASK:', 'ACTOR:', 'SEQUENCE:', 'FROM:', '## OBJECTIVE', '## CONTEXT',
                '## INPUTS', '## DO', '## DO_NOT', '## EXPECTED_OUTPUT', '## NEXT_IF_PASS')) {
            Check ("5 assignment contiene {0}" -f $section) ($aText -match [regex]::Escape($section))
        }
        Check '5 NEXT_IF_PASS e informativo' ($aText -match 'la decisione resta del Coordinator')

        # 6 - role match
        $r = Invoke-Router -AsRole 'DEV' -RouterArgs @('-Action', 'route', '-TaskId', '2330')
        Check '6 route DEV esce 0' ($r.Code -eq 0)
        Check '6 route dice ASSIGNED' ($r.Text -match 'ROUTING\s+: ASSIGNED')
        Check '6 route mostra la sequence' ($r.Text -match 'ASSIGNMENT : 0001')

        # 7 - role mismatch, e nessuna scrittura
        $hashBefore = (Get-FileHash $stateFile -Algorithm SHA256).Hash
        $r = Invoke-Router -AsRole 'EDITOR' -RouterArgs @('-Action', 'route', '-TaskId', '2330')
        Check '7 mismatch esce 2' ($r.Code -eq 2)
        Check '7 mismatch e esplicito' ($r.Text -match 'TASK ROUTING MISMATCH')
        Check '7 mismatch dice di non eseguire' ($r.Text -match 'DO NOT EXECUTE THIS ASSIGNMENT')
        Check '7 mismatch non tocca lo stato' ((Get-FileHash $stateFile -Algorithm SHA256).Hash -eq $hashBefore)

        # 8 - result append-only
        $r = Invoke-Router -AsRole 'DEV' -RouterArgs @('-Action', 'report', '-TaskId', '2330',
            '-Status', 'DONE', '-Summary', 'contratto scritto', '-Evidence', 'suite: exit 0',
            '-NextActorRecommended', 'VALIDATION')
        Check '8 report esce 0' ($r.Code -eq 0)
        $resultsDir = Join-Path (Join-Path $tmpRoot '2330') 'results'
        $firstResult = Join-Path $resultsDir '0001-DEV-selftest.md'
        Check '8 result scritto' (Test-Path $firstResult)
        $firstHash = (Get-FileHash $firstResult -Algorithm SHA256).Hash
        $rText = Get-Content $firstResult -Raw
        foreach ($section in @('TASK:', 'ROLE:', 'ASSIGNMENT_SEQUENCE:', 'STATUS:', '## SUMMARY',
                '## CHANGES', '## EVIDENCE', '## NOT_RUN', '## BLOCKER', '## NEXT_ACTOR_RECOMMENDED')) {
            Check ("8 result contiene {0}" -f $section) ($rText -match [regex]::Escape($section))
        }
        Check '8 la raccomandazione si dichiara tale' ($rText -match 'RACCOMANDAZIONE, non una decisione')

        $r = Invoke-Router -AsRole 'DEV' -RouterArgs @('-Action', 'report', '-TaskId', '2330',
            '-Status', 'PARTIAL', '-Summary', 'secondo invio')
        Check '8 secondo report accettato' ($r.Code -eq 0)
        Check '8 il primo result non e sovrascritto' ((Get-FileHash $firstResult -Algorithm SHA256).Hash -eq $firstHash)
        Check '8 il secondo result e un file nuovo' (Test-Path (Join-Path $resultsDir '0001-DEV-selftest-2.md'))

        # 9 - il worker non cambia il routing
        $hashBefore = (Get-FileHash $stateFile -Algorithm SHA256).Hash
        $r = Invoke-Router -AsRole 'DEV' -RouterArgs @('-Action', 'report', '-TaskId', '2330',
            '-Status', 'DONE', '-Summary', 'terzo invio')
        Check '9 report non tocca state.json' ((Get-FileHash $stateFile -Algorithm SHA256).Hash -eq $hashBefore)
        $r = Invoke-Router -AsRole 'DEV' -RouterArgs @('-Action', 'assign', '-TaskId', '2330', '-Actor', 'EDITOR',
            '-Objective', 'x', '-Do', 'y', '-ExpectedOutput', 'z')
        Check '9 un worker non puo assign' ($r.Code -eq 2 -and $r.Text -match 'TASK_MUTATION_ROLE_DENIED')
        $r = Invoke-Router -AsRole 'VALIDATION' -RouterArgs @('-Action', 'close', '-TaskId', '2330')
        Check '9 un worker non puo chiudere' ($r.Code -eq 2 -and $r.Text -match 'TASK_MUTATION_ROLE_DENIED')
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'report', '-TaskId', '2330', '-Status', 'DONE', '-Summary', 'x')
        Check '9 il coordinator non puo riportare' ($r.Code -eq 2 -and $r.Text -match 'RT_SESSION_REQUIRED')
        $r = Invoke-Router -AsRole 'EDITOR' -RouterArgs @('-Action', 'report', '-TaskId', '2330', '-Status', 'DONE', '-Summary', 'x')
        Check '9 il ruolo sbagliato non puo riportare' ($r.Code -eq 2 -and $r.Text -match 'TASK_ROUTE_MISMATCH')

        # 10 - il coordinator consuma il result e instrada
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '2330', '-Actor', 'EDITOR',
            '-Objective', 'creare la porta', '-Do', 'authoring', '-ExpectedOutput', 'uasset salvato')
        Check '10 assign successivo esce 0' ($r.Code -eq 0)
        $parsed = Get-Content $stateFile -Raw | ConvertFrom-Json
        Check '10 next_actor e EDITOR' ([string]$parsed.next_actor -eq 'EDITOR')
        Check '10 last_result_path popolato' ([string]$parsed.last_result_path -match '^results/')
        Check '10 FROM registra il ruolo precedente' ((Get-Content (Join-Path (Join-Path (Join-Path $tmpRoot '2330') 'assignments') '0002-EDITOR.md') -Raw) -match 'FROM:\s+DEV')

        # 11 - sequence monotona
        Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '2330', '-Actor', 'VALIDATION',
            '-Objective', 'gate', '-Do', 'suite', '-ExpectedOutput', 'referto') | Out-Null
        $parsed = Get-Content $stateFile -Raw | ConvertFrom-Json
        Check '11 sequence a 3' ([int]$parsed.assignment_sequence -eq 3)
        $names = @(Get-ChildItem (Join-Path (Join-Path $tmpRoot '2330') 'assignments') -Filter '*.md' | Sort-Object Name | ForEach-Object { $_.Name })
        Check '11 i file sono 0001..0003' (($names -join ',') -eq '0001-DEV.md,0002-EDITOR.md,0003-VALIDATION.md')
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '2330', '-Actor', 'DEV',
            '-Objective', 'x', '-Do', 'y', '-ExpectedOutput', 'z', '-ExpectedSequence', '1')
        Check '11 sequence stantia rifiutata' ($r.Code -eq 2 -and $r.Text -match 'TASK_SEQUENCE_CONFLICT')
        $parsed = Get-Content $stateFile -Raw | ConvertFrom-Json
        Check '11 il rifiuto non ha avanzato la sequence' ([int]$parsed.assignment_sequence -eq 3)
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '2330', '-Actor', 'DEV',
            '-Objective', 'x', '-Do', 'y', '-ExpectedOutput', 'z', '-ExpectedSequence', '3')
        Check '11 sequence corretta accettata' ($r.Code -eq 0)

        # 12 - scrittura atomica
        $tmpGarbage = "$stateFile.tmp"
        Set-Content -Path $tmpGarbage -Value '{ "schema_version": 1, "task_id":' -Encoding UTF8
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'status', '-TaskId', '2330')
        Check '12 un .tmp abbandonato non e lo stato' ($r.Code -eq 0)
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '2330', '-Actor', 'EDITOR',
            '-Objective', 'x', '-Do', 'y', '-ExpectedOutput', 'z')
        Check '12 la mutazione successiva riesce' ($r.Code -eq 0)
        Check '12 state.json resta JSON valido' ($null -ne (Get-Content $stateFile -Raw | ConvertFrom-Json))
        Check '12 il .tmp non sopravvive alla scrittura' (-not (Test-Path $tmpGarbage))

        # 13 - stato corrotto: fail closed
        Invoke-Router -AsRole '' -RouterArgs @('-Action', 'init', '-TaskId', 'rotto', '-Title', 'stato corrotto') | Out-Null
        $brokenState = Join-Path (Join-Path $tmpRoot 'rotto') 'state.json'
        Set-Content -Path $brokenState -Value '{ questo non e JSON' -Encoding UTF8
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'status', '-TaskId', 'rotto')
        Check '13 stato corrotto rifiutato' ($r.Code -eq 2 -and $r.Text -match 'TASK_STATE_CORRUPT')
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', 'rotto', '-Actor', 'DEV',
            '-Objective', 'x', '-Do', 'y', '-ExpectedOutput', 'z')
        Check '13 non si muta uno stato corrotto' ($r.Code -eq 2 -and $r.Text -match 'TASK_STATE_CORRUPT')
        Check '13 lo stato corrotto non e stato ricreato' ((Get-Content $brokenState -Raw).Trim() -eq '{ questo non e JSON')
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'list')
        Check '13 list segnala CORRUPT senza fermarsi' ($r.Code -eq 0 -and $r.Text -match 'CORRUPT' -and $r.Text -match '2330')

        # 14 - assignment mancante
        Invoke-Router -AsRole '' -RouterArgs @('-Action', 'init', '-TaskId', 'nudo', '-Title', 'senza assignment') | Out-Null
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assignment', '-TaskId', 'nudo')
        Check '14 assignment assente' ($r.Code -eq 2 -and $r.Text -match 'TASK_ASSIGNMENT_MISSING')
        $r = Invoke-Router -AsRole 'DEV' -RouterArgs @('-Action', 'route', '-TaskId', 'nudo')
        Check '14 route senza assignment' ($r.Code -eq 2 -and $r.Text -match 'TASK_ACTOR_NOT_WORKER')

        # 15 - task DONE
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'close', '-TaskId', '2501', '-Reason', 'mergiato')
        Check '15 close esce 0' ($r.Code -eq 0)
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'status', '-TaskId', '2501')
        Check '15 status dice DONE' ($r.Text -match 'Status : DONE' -and $r.Text -match 'NEXT: NONE')
        $r = Invoke-Router -AsRole 'DEV' -RouterArgs @('-Action', 'route', '-TaskId', '2501')
        Check '15 route su task chiuso' ($r.Code -eq 2 -and $r.Text -match 'TASK_ALREADY_DONE')
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '2501', '-Actor', 'DEV',
            '-Objective', 'x', '-Do', 'y', '-ExpectedOutput', 'z')
        Check '15 assign su task chiuso' ($r.Code -eq 2 -and $r.Text -match 'TASK_ALREADY_DONE')
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'close', '-TaskId', '2501')
        Check '15 close ripetuto rifiutato' ($r.Code -eq 2 -and $r.Text -match 'TASK_ALREADY_DONE')

        # 16 - next_actor = USER
        Invoke-Router -AsRole '' -RouterArgs @('-Action', 'init', '-TaskId', '288', '-Title', 'Defeat timing') | Out-Null
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '288', '-Actor', 'USER',
            '-Objective', 'guarda se la sconfitta si legge in tempo',
            '-Do', 'apri il PIE e guarda', '-ExpectedOutput', 'un giudizio, non una misura')
        Check '16 assign a USER esce 0' ($r.Code -eq 0)
        Check '16 USER riceve un assignment' (Test-Path (Join-Path (Join-Path (Join-Path $tmpRoot '288') 'assignments') '0001-USER.md'))
        $r = Invoke-Router -AsRole 'EDITOR' -RouterArgs @('-Action', 'route', '-TaskId', '288')
        Check '16 nessun ruolo RT3 la esegue' ($r.Code -eq 2 -and $r.Text -match 'TASK_ACTOR_NOT_WORKER')
        Check '16 il banner nomina lo USER' ($r.Text -match 'giudizio umano')
        $r = Invoke-Router -AsRole 'EDITOR' -RouterArgs @('-Action', 'report', '-TaskId', '288', '-Status', 'DONE', '-Summary', 'x')
        Check '16 USER_REQUIRED non diventa un report' ($r.Code -eq 2 -and $r.Text -match 'TASK_ACTOR_NOT_WORKER')

        # 17 - next_actor = NONE
        $r = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'assign', '-TaskId', '288', '-Actor', 'NONE',
            '-TaskStatus', 'BLOCKED', '-Reason', 'aspetta una decisione')
        Check '17 parcheggio su NONE esce 0' ($r.Code -eq 0)
        $parsed = Get-Content (Join-Path (Join-Path $tmpRoot '288') 'state.json') -Raw | ConvertFrom-Json
        Check '17 lo stato e BLOCKED' ([string]$parsed.status -eq 'BLOCKED' -and [string]$parsed.next_actor -eq 'NONE')
        Check '17 NONE non produce assignment' ([string]$parsed.assignment_path -eq '')
        $r = Invoke-Router -AsRole 'DEV' -RouterArgs @('-Action', 'route', '-TaskId', '288')
        Check '17 route su NONE non apre un ruolo falso' ($r.Code -eq 2 -and $r.Text -match 'TASK_ACTOR_NOT_WORKER')

        # 18 - lo stesso task da due WorkspaceRoot
        $rootA = Join-Path $tmpRoot 'ws-a'
        $rootB = Join-Path $tmpRoot 'ws-b'
        New-Item -ItemType Directory -Force -Path $rootA, $rootB | Out-Null
        $a = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'status', '-TaskId', '2330', '-WorkspaceRoot', $rootA)
        $b = Invoke-Router -AsRole '' -RouterArgs @('-Action', 'status', '-TaskId', '2330', '-WorkspaceRoot', $rootB)
        Check '18 visibile dal workspace A' ($a.Code -eq 0 -and $a.Text -match 'TASK: #2330')
        Check '18 visibile dal workspace B' ($b.Code -eq 0 -and $b.Text -match 'TASK: #2330')
        Check '18 i due leggono lo stesso next_actor' (
            ([regex]::Match($a.Text, 'NEXT: (\w+)').Groups[1].Value) -eq ([regex]::Match($b.Text, 'NEXT: (\w+)').Groups[1].Value))

        # 19 / 20 - niente effetti collaterali sul motore
        Check '19 lease.json non toccato'   ((Get-Fingerprint (Join-Path $rt3Dir 'lease.json')) -eq $leaseBefore)
        Check '19 events.jsonl non toccato' ((Get-Fingerprint (Join-Path $rt3Dir 'events.jsonl')) -eq $eventsBefore)
        Check '20 rt-engine-mode non toccato' ((Get-Fingerprint $modeFile) -eq $modeBefore)
        Check '20 la meta operativa non nomina il motore' (Test-NoEngineReference -Path $selfPath)
    } finally {
        if ($null -eq $savedRole) {
            Remove-Item Env:\RT_TERMINAL_ROLE -ErrorAction SilentlyContinue
        } else { $env:RT_TERMINAL_ROLE = $savedRole }
        if ($null -eq $savedInstance) {
            Remove-Item Env:\RT_TERMINAL_INSTANCE -ErrorAction SilentlyContinue
        } else { $env:RT_TERMINAL_INSTANCE = $savedInstance }
        Remove-Item -Recurse -Force $sandbox -ErrorAction SilentlyContinue
    }

    Write-Host ""
    if ($script:stFail -eq 0) {
        Write-Host ("SELF-TEST: {0} PASS, 0 FAIL" -f $script:stCount) -ForegroundColor Green
        return 0
    }
    Write-Host ("SELF-TEST: {0} FAIL su {1}" -f $script:stFail, $script:stCount) -ForegroundColor Red
    return 1
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

function Get-ExitCode {
    param($Value)
    if ($Value -is [array]) {
        if ($Value.Count -eq 0) { return 0 }
        return [int]$Value[$Value.Count - 1]
    }
    return [int]$Value
}

if ($SelfTest) { exit (Get-ExitCode (Invoke-SelfTest)) }

if ([string]::IsNullOrWhiteSpace($Action)) {
    Write-Host "BLOCKED: -Action e' obbligatorio (list | next | status | init | assign | assignment | report | route | close), oppure usa -SelfTest." -ForegroundColor Red
    exit 2
}

if ($Action -ne 'list') {
    if (-not (Test-TaskIdValid -Id $TaskId)) {
        exit (Get-ExitCode (Write-Refusal 'TASK_ID_INVALID' 'Ammessi: lettera o cifra iniziale, poi lettere, cifre, ".", "_", "-". Massimo 64 caratteri.'))
    }
}

$policy = Get-MutationPolicy -RequestedAction $Action -SessionRole (Get-SessionRole)
if (-not $policy.Allowed) {
    $detail = switch ($policy.Code) {
        'TASK_MUTATION_ROLE_DENIED' { "Questa sessione dichiara RT_TERMINAL_ROLE=$(Get-SessionRole). Il routing lo scrive il Coordinator, che gira senza ruolo di terminale." }
        'RT_SESSION_REQUIRED'       { "report si esegue da un terminale RT con un ruolo (DEV | EDITOR | VALIDATION)." }
        default                     { '' }
    }
    exit (Get-ExitCode (Write-Refusal $policy.Code $detail))
}

try {
    switch ($Action) {
        'list'       { exit (Get-ExitCode (Invoke-List)) }
        'status'     { exit (Get-ExitCode (Invoke-Status         -Id $TaskId)) }
        'init'       { exit (Get-ExitCode (Invoke-Init           -Id $TaskId)) }
        'assign'     { exit (Get-ExitCode (Invoke-Assign         -Id $TaskId)) }
        'assignment' { exit (Get-ExitCode (Invoke-ShowAssignment -Id $TaskId)) }
        'report'     { exit (Get-ExitCode (Invoke-Report         -Id $TaskId)) }
        'next'       { exit (Get-ExitCode (Invoke-Next           -Id $TaskId)) }
        'route'      { exit (Get-ExitCode (Invoke-Route          -Id $TaskId)) }
        'close'      { exit (Get-ExitCode (Invoke-Close          -Id $TaskId)) }
    }
} catch {
    # Un codice stabile resta un codice stabile anche quando arriva da un throw:
    # altrimenti uno stato corrotto uscirebbe con 1 e un messaggio di stack, che
    # nessuno script chiamante sa distinguere da un crash.
    $message = $_.Exception.Message
    if ($message -cmatch '^([A-Z_]+):') {
        exit (Get-ExitCode (Write-Refusal $Matches[1] $message))
    }
    throw
}
