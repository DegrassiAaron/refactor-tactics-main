<#
.SYNOPSIS
    Compila un target di RefactorTactics DENTRO il lease del motore.

.DESCRIPTION
    Chiude il buco misurato da #2529: `Build.bat` era il solo passo del gate che tocca
    il motore e non passava da nessun guard.

    Nel gate della wave `parsecell-arity/1` la build PRECEDE la suite. Eseguirlo alla
    lettera avrebbe ricompilato `UnrealEditor-RefactorTactics*.dll` sotto la full suite
    pre-merge di un altro checkout: per l'invariante «binario» che `rt-suite.ps1` legge
    prima e dopo la run, quella misura sarebbe diventata NON VALIDA. Un gate di una wave
    avrebbe distrutto la misura di un'altra, senza che nessun guard intervenisse.

    A fermare chi lo eseguiva e' stato guardare i PROCESSI invece del file di mode -
    cioe' un controllo che il gate non prescriveva.

    (!!) **Non ha un guard PROPRIO, e non deve averlo.** Chiede il lease a
    `rt-lease.ps1`, che importa le funzioni di stato del motore da `rt-suite.ps1` per
    estrazione AST, fail-closed. La regola su «il motore e' libero?» ha UNA sede; una
    terza copia divergerebbe dalle altre due, ed e' precisamente il difetto che
    `rt-mode.ps1` ha gia' pagato.

    (!!) **Non attende, per decisione.** `rt-lease.ps1` non ha un'attesa e questo script
    non ne inventa una: la richiesta di #2529 e' che una build in condizione di contesa
    sia FERMATA, non sconsigliata. Chi vuole aspettare guarda `rtlease -Action status`.

    (!!) **Rilascia il lease anche quando la build fallisce.** Il rilascio sta in un
    `finally`, che e' il modo in cui un lucchetto sopravvive a un ramo che qualcuno
    aggiunge domani - stessa forma del mutex di `rt-suite.ps1`.

.OUTPUTS
    Esce con il codice di `Build.bat`, tranne quando non arriva a lanciarlo:

      0      build riuscita
      2      lease non ottenuto: il motore e' occupato, o vivo e non attribuibile
      3      precondizione mancante (uproject, motore, o `rt-lease.ps1` assente)
      altro  il codice di `Build.bat`

.EXAMPLE
    .\scripts\rt-build.ps1 -TaskId 2529

.EXAMPLE
    .\scripts\rt-build.ps1 -Target RefactorTactics -Configuration Shipping -TaskId 2395
#>
param(
    [ValidateSet('RefactorTacticsEditor', 'RefactorTactics')]
    [string] $Target = 'RefactorTacticsEditor',

    [ValidateSet('Development', 'DebugGame', 'Shipping')]
    [string] $Configuration = 'Development',

    [string] $Platform = 'Win64',

    # Issue o task per cui si compila. Il lease lo richiede per le operazioni che
    # mutano, e comparira' negli eventi: una build senza task non e' attribuibile.
    [string] $TaskId = '',

    # Argomenti aggiuntivi per UBT, es. `-NoHotReloadFromIDE`.
    [string[]] $ExtraArgs = @(),

    # Radice del motore. Vuoto = `D:/EpicGames/UE_<EngineAssociation>`, la stessa che
    # `rt-suite.ps1` ricava dall'uproject.
    #
    # (!!) Esplicito per due motivi, e il secondo e' quello che conta: quel percorso e'
    # cablato in tutto il repository e prima o poi qualcuno avra' il motore altrove; e
    # senza di esso il FLUSSO di questo script - lease, build, rilascio - non e'
    # provabile se non occupando il motore vero, che e' esattamente la risorsa contesa
    # che #2529 esiste per proteggere.
    [string] $EngineRoot = '',

    # Verifica su casi fabbricati la regola PURA di questo script - la riga di comando
    # che compone - ed esce senza toccare lease ne' motore. Stessa sede e stesso motivo
    # di `rt-suite.ps1 -SelfTest` e `rt-lease.ps1 -SelfTest`: il repository non ha un
    # framework di test PowerShell, e AGENTS.md sezione 9 vieta di introdurne uno senza
    # una decisione.
    [switch] $SelfTest
)

$ErrorActionPreference = 'Stop'

function Say { param([string] $Text) Write-Output "[RT-BUILD] $Text" }

$RepoRoot    = Split-Path -Parent $PSScriptRoot
$UProject    = Join-Path $RepoRoot 'RefactorTactics.uproject'
$LeaseScript = Join-Path $PSScriptRoot 'rt-lease.ps1'

# ------------------------------------------------------------------ REGOLA PURA
function Resolve-BuildArgs {
    <#
    .SYNOPSIS
        Compone gli argomenti di `Build.bat`. Nessun I/O.

    .DESCRIPTION
        (!!) Restituisce un OGGETTO e non l'array: PowerShell srotola una collezione
        restituita da una funzione, e un array di un elemento tornerebbe come stringa.

        (!!) `-Project` col percorso VIRGOLETTATO e `-WaitMutex` non sono opzionali e
        non si dimenticano: sono la forma documentata in `AGENTS.md` e nella Definition
        of Done, ed e' cio' che questo script esiste per non far riscrivere a mano.
    #>
    param(
        [Parameter(Mandatory)] [string] $Target,
        [Parameter(Mandatory)] [string] $Platform,
        [Parameter(Mandatory)] [string] $Configuration,
        [Parameter(Mandatory)] [string] $UProjectPath,
        [string[]] $ExtraArgs = @()
    )

    $a = @($Target, $Platform, $Configuration, ('-Project="{0}"' -f $UProjectPath), '-WaitMutex')
    foreach ($x in $ExtraArgs) {
        if (-not [string]::IsNullOrWhiteSpace($x)) { $a += $x }
    }
    return [pscustomobject]@{ Args = $a }
}

# ---------------------------------------------------------------------- SELF-TEST
if ($SelfTest) {
    $failures = 0
    $total = 0

    function Assert-Args {
        param([string] $Name, [string[]] $Actual, [string] $MustContain, [bool] $Expect = $true)
        $script:total++
        $hit = [bool]($Actual | Where-Object { $_ -like "*$MustContain*" })
        $ok = ($hit -eq $Expect)
        if (-not $ok) { $script:failures++ }
        Say ("{0}  {1,-24} {2} '{3}'" -f `
            $(if ($ok) { 'ok  ' } else { 'ROTTO' }), $Name, $(if ($Expect) { 'contiene' } else { 'NON contiene' }), $MustContain)
        if (-not $ok) { Say ("        > " + ($Actual -join ' ')) }
    }

    Say 'self-test della riga di comando (#2529)'
    $r = Resolve-BuildArgs -Target 'RefactorTacticsEditor' -Platform 'Win64' -Configuration 'Development' -UProjectPath 'D:\rt wt\RefactorTactics.uproject'

    # Il percorso ha uno SPAZIO apposta: senza virgolette UBT riceve due argomenti e
    # compila il progetto sbagliato, o nessuno.
    Assert-Args 'progetto virgolettato' $r.Args '-Project="D:\rt wt\RefactorTactics.uproject"'
    Assert-Args 'waitmutex'             $r.Args '-WaitMutex'
    Assert-Args 'target'                $r.Args 'RefactorTacticsEditor'
    Assert-Args 'configurazione'        $r.Args 'Development'

    $script:total++
    $okN = ($r.Args.Count -eq 5)
    if (-not $okN) { $script:failures++ }
    Say ("{0}  {1,-24} argomenti={2} (attesi 5: target, piattaforma, configurazione, progetto, waitmutex)" -f `
        $(if ($okN) { 'ok  ' } else { 'ROTTO' }), 'niente in piu', $r.Args.Count)

    # Gli extra si aggiungono in coda, e un vuoto non diventa un argomento vuoto che
    # UBT leggerebbe come un target senza nome.
    $e = Resolve-BuildArgs -Target 'RefactorTactics' -Platform 'Win64' -Configuration 'Shipping' -UProjectPath 'X.uproject' -ExtraArgs @('-NoHotReloadFromIDE', '', '   ')
    Assert-Args 'extra in coda'         $e.Args '-NoHotReloadFromIDE'
    $script:total++
    $okE = ($e.Args.Count -eq 6)
    if (-not $okE) { $script:failures++ }
    Say ("{0}  {1,-24} argomenti={2} (attesi 6: i 5 piu' UN extra; vuoto e spazi scartati)" -f `
        $(if ($okE) { 'ok  ' } else { 'ROTTO' }), 'extra vuoti scartati', $e.Args.Count)

    if ($failures -gt 0) {
        Say ("self-test ROSSO: {0} caso/i non conforme/i su {1}" -f $failures, $total)
        exit 1
    }
    Say ("self-test verde: {0} casi su {0}" -f $total)
    exit 0
}

# ------------------------------------------------------------------ PRECONDIZIONI
if (-not (Test-Path $UProject)) {
    Say "PRECONDIZIONE: uproject non trovato: $UProject"
    exit 3
}
if (-not (Test-Path $LeaseScript)) {
    # Fail-closed, e non un ripiego su `Build.bat` nudo: senza il lease questo script
    # non ha nulla da aggiungere, e girare comunque sarebbe il difetto di #2529.
    Say "PRECONDIZIONE: rt-lease.ps1 non trovato: $LeaseScript"
    Say '  Senza lease questa build non e'' governata: e'' esattamente il caso che #2529 chiude.'
    exit 3
}

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $EngineVersion = (Get-Content $UProject -Raw | ConvertFrom-Json).EngineAssociation
    $EngineRoot = "D:/EpicGames/UE_$EngineVersion"
}
$BuildBat = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
if (-not (Test-Path $BuildBat)) {
    Say "PRECONDIZIONE: Build.bat non trovato: $BuildBat"
    exit 3
}

# ------------------------------------------------------------------------- LEASE
Say ("target    {0} {1} {2}" -f $Target, $Platform, $Configuration)
Say 'lease     richiesta per BUILD...'

# (!!) HASHTABLE e non array. Lo splatting di un ARRAY passa gli elementi
# POSIZIONALMENTE: `@('-Action','acquire',...)` finisce sui parametri nell'ordine in
# cui sono dichiarati, e `-WorkspaceRoot` e' arrivato su `[int] $EditorPid` con un
# errore di conversione. Misurato sul banco di prova, non dedotto.
$leaseArgs = @{
    Action        = 'acquire'
    Operation     = 'BUILD'
    WorkspaceRoot = $RepoRoot
}
if (-not [string]::IsNullOrWhiteSpace($TaskId)) { $leaseArgs['TaskId'] = $TaskId }

& $LeaseScript @leaseArgs
$leaseExit = $LASTEXITCODE
if ($leaseExit -ne 0) {
    Say ''
    Say ("BUILD NON AVVIATA: il lease non e'' stato ottenuto (rt-lease exit {0})." -f $leaseExit)
    Say '  Il motore e'' occupato, oppure vivo e non attribuibile a questa sessione.'
    Say '  Compilare adesso riscriverebbe le DLL sotto la misura di un''altra sessione:'
    Say '  per l''invariante «binario» di rt-suite.ps1 quella misura diventa NON VALIDA.'
    Say '  Stato: scripts\rt-lease.ps1 -Action status'
    exit 2
}

# --------------------------------------------------------------------- LA BUILD
$buildExit = 1
try {
    $composed = Resolve-BuildArgs -Target $Target -Platform $Platform -Configuration $Configuration -UProjectPath $UProject -ExtraArgs $ExtraArgs
    Say ("comando   Build.bat {0}" -f ($composed.Args -join ' '))
    Say 'build in corso...'

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & $BuildBat @($composed.Args)
    $buildExit = $LASTEXITCODE
    $sw.Stop()

    Say ''
    if ($buildExit -eq 0) {
        Say ("BUILD OK      durata {0:mm\:ss}" -f $sw.Elapsed)
    } else {
        Say ("BUILD ROSSA   exit {0}, durata {1:mm\:ss}" -f $buildExit, $sw.Elapsed)
    }
}
finally {
    # Il rilascio sta qui e non ai punti d'uscita: un lease non rilasciato blocca la
    # macchina intera, e il ramo che lo dimenticherebbe e' sempre quello aggiunto dopo.
    Say 'lease     rilascio...'
    & $LeaseScript -Action release -WorkspaceRoot $RepoRoot
}

exit $buildExit
