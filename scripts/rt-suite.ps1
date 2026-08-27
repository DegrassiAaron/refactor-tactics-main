<#
.SYNOPSIS
    Esegue la suite di automation e dichiara se la misura VALE.

.DESCRIPTION
    Il problema che chiude non e' che i test falliscano: e' che passino misurando
    un'altra cosa. Con piu' sessioni sulla stessa working directory una run puo'
    riportare `1233/1233, 0 fail` avendo letto un file che nel frattempo qualcun
    altro ha cambiato, oppure essere morta a meta' senza un solo fallimento.
    Entrambe hanno l'aria di essere verdi.

    Quattro invarianti, lette PRIMA e DOPO la run:

      HEAD        il commit su cui gira il codice
      albero      `git status --porcelain`, cioe' le modifiche non committate
      binario     mtime + dimensione di UnrealEditor-RefactorTactics.dll
      motore      i processi UnrealEditor-Cmd, con il checkout da cui vengono

    Piu' una quinta che si legge dal log:

      copertura   `Test Completed` contro il `Found N automation tests`

    Se una qualsiasi e' cambiata, l'esito NON e' registrabile: non e' rosso e non
    e' verde, e' NON VALIDA. Lo script non impedisce niente e non uccide nessuno
    — completa la run e poi dichiara, perche' il log e' l'unica cosa che permette
    di datare la collisione e di attribuirla.

    ⚠️ PowerShell e non Git Bash: MSYS traduce gli argomenti che iniziano con `/`,
    e una riga di comando con un path di mappa diventa `C:/Program Files/Git/...`.

.PARAMETER Filter
    Filtro di automation. Default `RefactorTactics`, cioe' la suite intera.

.PARAMETER LogName
    Nome del file di log sotto Saved/Logs. Default `rt-suite.log`.

.EXAMPLE
    ./scripts/rt-suite.ps1
    ./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario
#>
[CmdletBinding()]
param(
    [string] $Filter = 'RefactorTactics',
    [string] $LogName = 'rt-suite.log'
)

$ErrorActionPreference = 'Stop'

# La radice del repository si deriva dalla posizione dello script: lo si lancia
# da qualunque directory senza che i path relativi cambino significato.
$RepoRoot = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $RepoRoot 'RefactorTactics.uproject'
$Dll      = Join-Path $RepoRoot 'Binaries/Win64/UnrealEditor-RefactorTactics.dll'
$LogPath  = Join-Path $RepoRoot "Saved/Logs/$LogName"

# L'eseguibile del motore viene da `EngineAssociation` nel .uproject: inciderlo
# qui lo farebbe divergere dal progetto alla prima versione nuova.
if (-not (Test-Path $UProject)) { throw "uproject non trovato: $UProject" }
$EngineVersion = (Get-Content $UProject -Raw | ConvertFrom-Json).EngineAssociation
$EngineCmd = "D:/EpicGames/UE_$EngineVersion/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
if (-not (Test-Path $EngineCmd)) { throw "motore non trovato: $EngineCmd" }

function Get-Snapshot {
    <# Le quattro invarianti in un colpo. Costa millisecondi: si puo' chiamare
       prima e dopo senza pesare sulla run. #>
    Push-Location $RepoRoot
    try {
        $head = (& git rev-parse HEAD 2>$null)
        # L'albero si riassume in un hash: l'elenco intero renderebbe il confronto
        # illeggibile, e cio' che serve sapere e' solo SE e' cambiato.
        $statusRaw = (& git status --porcelain 2>$null) -join "`n"
        $statusHash = if ([string]::IsNullOrEmpty($statusRaw)) { '(pulito)' } else {
            $sha = [System.Security.Cryptography.SHA1]::Create()
            $bytes = $sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($statusRaw))
            (($bytes | ForEach-Object { $_.ToString('x2') }) -join '').Substring(0, 8)
        }
        $statusCount = if ([string]::IsNullOrEmpty($statusRaw)) { 0 } else { ($statusRaw -split "`n").Count }
    }
    finally { Pop-Location }

    $dllStamp = if (Test-Path $Dll) {
        $f = Get-Item $Dll
        '{0:yyyy-MM-dd HH:mm:ss}/{1}' -f $f.LastWriteTime, $f.Length
    } else { '(assente)' }

    # ⚠️ La `CommandLine` e non il solo conteggio: un `UnrealEditor-Cmd` di un
    # ALTRO checkout uccide comunque questa run — il mutex e' globale
    # sull'eseguibile del motore — ma non e' lavoro da terminare, e' di qualcun
    # altro. Distinguere i due casi richiede sapere da dove viene.
    $engines = @()
    try {
        $engines = @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor-Cmd.exe'" -ErrorAction Stop |
            ForEach-Object { $_.CommandLine })
    } catch { $engines = @() }

    [pscustomobject]@{
        Head        = $head
        StatusHash  = $statusHash
        StatusCount = $statusCount
        Dll         = $dllStamp
        Engines     = $engines
        EngineCount = $engines.Count
    }
}

function Write-Line { param([string] $Text) Write-Host "[RT-MEASURE] $Text" }

# ---------------------------------------------------------------- PRIMA
$before = Get-Snapshot
Write-Line "filtro   $Filter"
Write-Line ("HEAD     {0}" -f $before.Head.Substring(0, 8))
Write-Line ("albero   {0}{1}" -f $before.StatusHash, $(if ($before.StatusCount) { " ($($before.StatusCount) file)" } else { '' }))
Write-Line ("binario  {0}" -f $before.Dll)

# Un motore gia' vivo PRIMA e' l'unico caso in cui vale la pena fermarsi subito:
# la run morirebbe a meta' e il log andrebbe perso nella rotazione. Non e' una
# violazione dell'«esegui e poi dichiara» — non c'e' ancora niente da preservare.
if ($before.EngineCount -gt 0) {
    Write-Line 'NON AVVIATA: un UnrealEditor-Cmd e'' gia'' attivo.'
    foreach ($e in $before.Engines) { Write-Line "  $e" }
    Write-Line 'Due run di automation si uccidono a vicenda (il mutex e'' globale sull''eseguibile'
    Write-Line 'del motore, quindi vale anche fra checkout diversi). Se e'' di un altro checkout'
    Write-Line 'NON terminarlo: e'' il lavoro di qualcun altro, si aspetta che finisca.'
    exit 2
}

# ---------------------------------------------------------------- LA RUN
Write-Line 'run in corso...'
$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $EngineCmd $UProject `
    "-ExecCmds=Automation RunTests $Filter;Quit" `
    -unattended -nopause -nosplash -nullrhi -NoLiveCoding "-log=$LogName" | Out-Null
$sw.Stop()

# ---------------------------------------------------------------- DOPO
$after = Get-Snapshot

$problems = New-Object System.Collections.Generic.List[string]
if ($before.Head -ne $after.Head) {
    $problems.Add(("HEAD     {0} -> {1}   cambiato a run iniziata" -f $before.Head.Substring(0,8), $after.Head.Substring(0,8)))
}
if ($before.StatusHash -ne $after.StatusHash) {
    $problems.Add(("albero   {0} -> {1}   file modificati durante la run" -f $before.StatusHash, $after.StatusHash))
}
if ($before.Dll -ne $after.Dll) {
    $problems.Add(("binario  {0} -> {1}   ricompilato durante la run" -f $before.Dll, $after.Dll))
}
if ($after.EngineCount -gt 0) {
    $problems.Add("motore   un UnrealEditor-Cmd e' comparso durante la run: $($after.Engines -join '; ')")
}

# ---------------------------------------------------------------- IL LOG
$found = $null; $completed = 0; $failed = 0; $started = 0
if (Test-Path $LogPath) {
    $log = Get-Content $LogPath -Raw
    if ($log -match 'Found (\d+) automation tests') { $found = [int]$Matches[1] }
    $completed = ([regex]::Matches($log, 'Test Completed\.')).Count
    $started   = ([regex]::Matches($log, 'Test Started\.')).Count
    $failed    = ([regex]::Matches($log, 'Result=\{Fail\}')).Count
} else {
    $problems.Add("log      $LogPath non e' stato prodotto")
}

# 🔴 La troncatura e' la meta' silenziosa del difetto, e non si vede da `Fail`:
# due run sono morte a 641/1175 e 662/1191 con ZERO fallimenti. Il numero che le
# smaschera e' `Test Completed` contro `Found N`, mai il conteggio dei rossi.
if ($null -eq $found) {
    $problems.Add("copertura il log non dichiara «Found N automation tests»: run non partita?")
} elseif ($completed -lt $found) {
    $problems.Add(("copertura {0}/{1} completati: ne mancano {2}, con {3} fallimenti" -f $completed, $found, ($found - $completed), $failed))
}
# Un test avviato e mai concluso e' l'altra forma della stessa cosa, e capita
# anche senza collisioni: l'ultimo test puo' perdere la riga di chiusura nel
# flush di shutdown. Va detto, ma non invalida da solo.
$dangling = $started - $completed

# ---------------------------------------------------------------- VERDETTO
Write-Host ''
if ($problems.Count -gt 0) {
    Write-Line 'NON VALIDA'
    foreach ($p in $problems) { Write-Line "  $p" }
    Write-Line ("  esito    {0}/{1}, {2} fail  -> NON REGISTRABILE" -f $completed, $found, $failed)
    Write-Line '  causa probabile: un''altra sessione sulla stessa working directory (D-178).'
    Write-Line "  log: $LogPath"
    exit 1
}

Write-Line 'VALIDA'
Write-Line ("  HEAD     {0}  albero {1}" -f $after.Head.Substring(0,8), $after.StatusHash)
Write-Line ("  esito    {0}/{1} completati, {2} fallimenti" -f $completed, $found, $failed)
if ($dangling -gt 0) { Write-Line ("  nota     {0} test avviati senza riga di conclusione" -f $dangling) }
Write-Line ("  durata   {0:mm\:ss}" -f $sw.Elapsed)
Write-Line "  log: $LogPath"
exit $(if ($failed -gt 0) { 1 } else { 0 })
