<#
.SYNOPSIS
    Esegue la suite di automation e dichiara se la misura VALE.

.DESCRIPTION
    Il problema che chiude non e' che i test falliscano: e' che passino misurando
    un'altra cosa. Con piu' sessioni sulla stessa working directory una run puo'
    riportare `1233/1233, 0 fail` avendo letto un file che nel frattempo qualcun
    altro ha cambiato, oppure essere morta a meta' senza un solo fallimento.
    Entrambe hanno l'aria di essere verdi.

    Invarianti lette PRIMA e DOPO la run:

      HEAD        il commit su cui gira il codice
      albero      `git diff HEAD` piu' gli untracked con il loro `hash-object`:
                  e' un digest del CONTENUTO, non di `git status --porcelain` —
                  quello elenca i path e resta identico se cambia cio' che c'e'
                  DENTRO a un file gia' modificato, che e' il caso piu' comune.
                  ⚠️ Fino al 2026-08-28 degli untracked prendeva i soli PATH, e
                  un file nuovo poteva essere riscritto e ricompilato fra due run
                  senza che il digest cambiasse: due misure, una verde e una
                  rossa, dichiarate VALIDE con lo stesso identificatore
      binario     mtime + dimensione di ENTRAMBI i moduli, `-RefactorTactics` e
                  `-RefactorTacticsEditor`: una build editor-only tocca il secondo.
                  ⛔ **Copre il binario che cambia DURANTE la run, non un binario
                  gia' stantio all'avvio.** Un DLL compilato da un commit diverso
                  da quello su disco resta identico dall'inizio alla fine, quindi
                  passa. I due criteri praticabili senza toccare la build sono
                  entrambi inaffidabili — l'ora dell'ultimo commit su `Source/` e'
                  sempre posteriore alla build che l'ha preceduta (build, test,
                  commit), e l'mtime dei sorgenti viene riscritto da ogni
                  `checkout` senza che il contenuto cambi. Servirebbe un marker
                  scritto dalla build, che e' fuori dal perimetro di questo script.
                  Il limite e' dichiarato, non risolto
      motore      i processi `UnrealEditor*`, con il checkout da cui vengono

    Piu' due controlli sul referto:

      freschezza  il log e' stato scritto DOPO l'avvio? Un log stantio di una run
                  precedente, letto come se fosse di questa, e' verde su nulla
      copertura   `Test Completed` contro il `Found N` dichiarato in testa

    Se una qualsiasi cade, l'esito NON e' registrabile: non e' rosso e non e'
    verde, e' NON VALIDA. Lo script non impedisce niente e non uccide nessuno —
    completa la run e poi dichiara, perche' il log e' l'unica cosa che permette di
    datare la collisione e di attribuirla.

    ⚠️ PowerShell e non Git Bash: MSYS traduce gli argomenti che iniziano con `/`,
    e una riga di comando con un path di mappa diventa `C:/Program Files/Git/...`.

.OUTPUTS
    Le righe di verdetto vanno sullo stream di SUCCESSO (`Write-Output`), quindi
    `./scripts/rt-suite.ps1 > referto.txt` e `$v = ./scripts/rt-suite.ps1`
    funzionano. Con `Write-Host` sarebbero finite sullo stream 6 e la cattura
    avrebbe prodotto un file vuoto — cioe' avrebbe perso proprio la dichiarazione
    per cui lo script esiste.

    Exit code, e sono TRE perche' gli stati sono tre:
      0  VALIDA, nessun fallimento
      1  VALIDA, ma dei test falliscono   -> difetto del gioco
      3  NON VALIDA                       -> esito non registrabile, non e' rosso
      2  NON AVVIATA                      -> il motore era gia' occupato
         (con `-WaitMinutes N` aspetta che si liberi invece di uscire subito)

.PARAMETER Filter
    Filtro di automation. Default `RefactorTactics`, cioe' la suite intera.

.PARAMETER LogName
    Nome del file di log sotto Saved/Logs. Default `rt-suite.log`.

.PARAMETER WaitMinutes
    Minuti di attesa se il motore e' occupato da un'altra sessione; `0` (default)
    esce subito con `2`. L'attesa rifa' lo snapshot a ogni giro, quindi la run
    parte da cio' che c'e' quando si libera — non da cio' che c'era mezz'ora
    prima. Non termina mai nessun processo: se e' di un altro checkout e' lavoro
    di qualcun altro, e questo script non lo tocca.

.PARAMETER PollSeconds
    Ogni quanto ricontrollare durante l'attesa. Default 30.

.EXAMPLE
    ./scripts/rt-suite.ps1
    ./scripts/rt-suite.ps1 -Filter RefactorTactics.Scenario
    ./scripts/rt-suite.ps1 -WaitMinutes 40      # parte da sola quando il motore si libera
#>
[CmdletBinding()]
param(
    [string] $Filter = 'RefactorTactics',
    [string] $LogName = 'rt-suite.log',

    # Minuti di attesa se il motore e' occupato da un'altra sessione. `0` = non
    # attende ed esce `2`, che resta il default: un'attesa implicita in uno
    # script lanciato a mano lo farebbe sembrare appeso.
    #
    # ⚠️ **Esiste perche' l'attesa la scrivevano tutti a mano, ogni volta
    # diversa.** In una sola sessione del 2026-08-28 lo stesso
    # `while (Get-Process …) { Start-Sleep }` e' stato riscritto CINQUE volte, e
    # la prima aspettava il solo `UnrealEditor-Cmd`: un editor interattivo
    # dell'altro checkout l'ha attraversata, e la run e' uscita `2` lo stesso
    # dopo aver atteso per niente. La condizione di rilascio e' la stessa che
    # questo script gia' controlla — tenerla in due posti significa che uno dei
    # due e' sbagliato.
    [int] $WaitMinutes = 0,

    # Ogni quanto ricontrollare, mentre attende. Trenta secondi e' il compromesso
    # fra «riparte presto» e «non interroga WMI di continuo per mezz'ora».
    [int] $PollSeconds = 30
)

$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $RepoRoot 'RefactorTactics.uproject'
$LogPath  = Join-Path $RepoRoot "Saved/Logs/$LogName"
$Dlls     = @(
    (Join-Path $RepoRoot 'Binaries/Win64/UnrealEditor-RefactorTactics.dll'),
    (Join-Path $RepoRoot 'Binaries/Win64/UnrealEditor-RefactorTacticsEditor.dll')
)

if (-not (Test-Path $UProject)) { throw "uproject non trovato: $UProject" }
$EngineVersion = (Get-Content $UProject -Raw | ConvertFrom-Json).EngineAssociation
$EngineCmd = "D:/EpicGames/UE_$EngineVersion/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
if (-not (Test-Path $EngineCmd)) { throw "motore non trovato: $EngineCmd" }

function Get-ShortHash {
    param([string] $Text)
    if ([string]::IsNullOrEmpty($Text)) { return '(pulito)' }
    $sha = [System.Security.Cryptography.SHA1]::Create()
    $bytes = $sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($Text))
    return (($bytes | ForEach-Object { $_.ToString('x2') }) -join '').Substring(0, 8)
}

function Get-Snapshot {
    Push-Location $RepoRoot
    try {
        $head = (& git rev-parse HEAD 2>$null)
        # 🔴 Un `git` che fallisce non deve produrre un `$null` che esplode piu'
        # avanti con «You cannot call a method on a null-valued expression»: il
        # motivo va detto qui, dove si sa qual e'.
        if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($head)) {
            throw "HEAD non leggibile in $RepoRoot (git assente, repo corrotto, o non e' un work tree)"
        }

        # Il CONTENUTO, non l'elenco dei path. `git status --porcelain` resta
        # identico quando cambia cio' che sta DENTRO a un file gia' modificato —
        # misurato: due `echo >> CLAUDE.md` di fila danno lo stesso ` M CLAUDE.md`.
        # Con quel digest l'invariante piu' importante sarebbe stata cieca proprio
        # sul caso piu' comune.
        $tracked = (& git diff HEAD 2>$null) -join "`n"

        # 🔴 Degli untracked serve il CONTENUTO, non l'elenco dei path, ed e' lo
        # stesso argomento del blocco qui sopra applicato all'altra meta'.
        # Misurato il 2026-08-28 lavorando su `#166`: un `.cpp` nuovo non ancora
        # `git add`-ato e' stato MUTATO fra due run e ricompilato, e il digest e'
        # rimasto `b8e81adc` in entrambe — due misure, una verde e una rossa,
        # dichiarate VALIDE con lo stesso identificatore d'albero. L'invariante
        # era cieca proprio sul file che si sta scrivendo in quel momento.
        #
        # `git hash-object` e non una lettura diretta: gestisce i binari, non
        # carica il file in memoria, e una sola invocazione con `--stdin-paths`
        # copre l'intero elenco.
        $untrackedPaths = @(& git ls-files --others --exclude-standard 2>$null)
        if ($untrackedPaths.Count -gt 0) {
            $hashes = @($untrackedPaths | git hash-object --stdin-paths 2>$null)
            # ⚠️ Se `hash-object` non risponde per TUTTI — file sparito fra
            # l'elenco e la lettura, permesso negato — si ripiega sui soli path
            # invece di appaiare hash sbagliati a file sbagliati: un digest
            # disallineato direbbe «cambiato» a ogni run e renderebbe il gate
            # rumore da ignorare, che e' il modo in cui un gate muore.
            $untracked = if ($hashes.Count -eq $untrackedPaths.Count) {
                (0..($untrackedPaths.Count - 1) | ForEach-Object { "$($untrackedPaths[$_]) $($hashes[$_])" }) -join "`n"
            } else {
                ($untrackedPaths -join "`n")
            }
        } else {
            $untracked = ''
        }

        $paths = (& git status --porcelain 2>$null)
    }
    finally { Pop-Location }

    $dllStamps = foreach ($d in $Dlls) {
        if (Test-Path $d) {
            $f = Get-Item $d
            '{0}={1:yyyy-MM-dd HH:mm:ss}/{2}' -f $f.BaseName.Replace('UnrealEditor-', ''), $f.LastWriteTime, $f.Length
        } else { '{0}=(assente)' -f (Split-Path $d -Leaf) }
    }

    # ⚠️ `UnrealEditor*`, non il solo `-Cmd`: il mutex Live Coding e' globale
    # sull'eseguibile del motore, e un editor interattivo lo tiene quanto una run
    # headless. La `CommandLine` e non il conteggio, perche' un processo di un
    # ALTRO checkout uccide comunque questa run ma non e' lavoro da terminare.
    # 🔴 Un fallimento della query NON e' «nessun processo»: sarebbe un'invariante
    # che fallisce APERTA, indistinguibile dal caso sano.
    $engines = @(); $engineError = $null
    try {
        $engines = @(Get-CimInstance Win32_Process -ErrorAction Stop |
            Where-Object { $_.Name -like 'UnrealEditor*.exe' } |
            ForEach-Object { $_.CommandLine })
    } catch { $engineError = $_.Exception.Message }

    [pscustomobject]@{
        Head        = $head
        TreeHash    = Get-ShortHash ($tracked + "`n--untracked--`n" + $untracked)
        Paths       = @($paths)
        PathCount   = @($paths).Count
        Dlls        = ($dllStamps -join ' ')
        Engines     = $engines
        EngineCount = $engines.Count
        EngineError = $engineError
    }
}

function Say { param([string] $Text) Write-Output "[RT-MEASURE] $Text" }

# ---------------------------------------------------------------- PRIMA
$before = Get-Snapshot
Say ("filtro   {0}" -f $Filter)
Say ("HEAD     {0}" -f $before.Head.Substring(0, 8))
Say ("albero   {0}{1}" -f $before.TreeHash, $(if ($before.PathCount) { " ($($before.PathCount) file)" } else { '' }))
Say ("binario  {0}" -f $before.Dlls)

if ($before.EngineError) {
    Say "NON AVVIATA: la query sui processi e' fallita — $($before.EngineError)"
    Say 'Senza quel dato non si puo'' sapere se il motore e'' libero, e partire alla cieca'
    Say 'significa rischiare che le due run si uccidano a vicenda.'
    exit 2
}

# Un motore gia' vivo PRIMA e' l'unico caso in cui vale la pena non partire: la
# run morirebbe a meta' e il log andrebbe perso nella rotazione. Non contraddice
# l'«esegui e poi dichiara» — non c'e' ancora niente da preservare.
if ($before.EngineCount -gt 0 -and $WaitMinutes -gt 0) {
    $waitUntil = (Get-Date).AddMinutes($WaitMinutes)
    # ⚠️ Doppi apici: qui `''` NON e' un escape e finirebbe a schermo come due
    # apostrofi. L'escape raddoppiato vale nelle stringhe ad apice singolo, che
    # sono la maggioranza delle altre righe di questo script.
    Say ("in attesa: il motore e' occupato, ricontrollo ogni {0}s fino a un massimo di {1} min" -f $PollSeconds, $WaitMinutes)
    foreach ($e in $before.Engines) { Say "  $e" }

    while ($before.EngineCount -gt 0 -and (Get-Date) -lt $waitUntil) {
        Start-Sleep -Seconds $PollSeconds
        $before = Get-Snapshot
        # 🔴 Lo snapshot si RIFA' per intero, non solo la query sui processi. Chi
        # attende mezz'ora attende in un albero che le altre sessioni muovono: se
        # `HEAD` o l'albero cambiano mentre si aspetta, la run deve partire da
        # cio' che c'e' ADESSO, e i controlli di fine run confrontarsi con quello.
        # Aggiornare il solo conteggio dei processi avrebbe misurato la fine
        # contro un inizio che non esisteva piu'.
        if ($before.EngineError) { break }
    }

    if ($before.EngineCount -eq 0) {
        Say ("motore libero: si parte (HEAD {0}, albero {1})" -f $before.Head.Substring(0, 8), $before.TreeHash)
    }
}

if ($before.EngineError) {
    Say "NON AVVIATA: la query sui processi e' fallita durante l'attesa — $($before.EngineError)"
    exit 2
}

if ($before.EngineCount -gt 0) {
    Say 'NON AVVIATA: un processo del motore e'' gia'' attivo.'
    if ($WaitMinutes -gt 0) { Say ("  (dopo {0} minuti di attesa)" -f $WaitMinutes) }
    foreach ($e in $before.Engines) { Say "  $e" }
    Say 'Due run di automation si uccidono a vicenda: il mutex e'' globale sull''eseguibile del'
    Say 'motore, quindi vale anche fra checkout diversi. Se e'' di un ALTRO checkout non'
    Say 'terminarlo — e'' il lavoro di qualcun altro.'
    # ⚠️ Un processo che ha FINITO puo' restare appeso in shutdown tenendosi il
    # mutex, e ne' i thread ne' il working set lo distinguono da una run viva. Il
    # criterio che funziona e' l'mtime del suo log contro l'ora corrente: fermo da
    # minuti piu' ultima riga `RequestExit` significa che non c'e' niente da
    # perdere. Senza questa riga si aspetta un processo morto all'infinito.
    Say 'Se e'' di QUESTO checkout e sospetti sia uno zombie: guarda l''mtime del suo log —'
    Say 'fermo da minuti + ultima riga `Engine exit requested` ⇒ non c''e'' nulla da perdere.'
    if ($WaitMinutes -le 0) {
        Say 'Per attendere che si liberi e partire da sola: -WaitMinutes 40'
    }
    exit 2
}

# ---------------------------------------------------------------- LA RUN
# 🔴 Il log si rimuove PRIMA. Se il motore muore senza arrivare a inizializzare il
# log — cosa documentata, capita dopo una build Shipping — il file precedente
# resterebbe sul disco e verrebbe letto come referto di QUESTA run: verde su una
# run che non e' mai partita.
if (Test-Path $LogPath) { Remove-Item $LogPath -Force }

$startedAt = Get-Date
Say 'run in corso...'
$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $EngineCmd $UProject `
    "-ExecCmds=Automation RunTests $Filter;Quit" `
    -unattended -nopause -nosplash -nullrhi -NoLiveCoding "-log=$LogName" | Out-Null
$engineExit = $LASTEXITCODE
$sw.Stop()

# ---------------------------------------------------------------- DOPO
$after = Get-Snapshot
$problems = New-Object System.Collections.Generic.List[string]

if ($before.Head -ne $after.Head) {
    $problems.Add(("HEAD      {0} -> {1}   cambiato a run iniziata" -f $before.Head.Substring(0,8), $after.Head.Substring(0,8)))
}
if ($before.TreeHash -ne $after.TreeHash) {
    # I path che sono comparsi o spariti: e' cio' che serve per ATTRIBUIRE la
    # collisione, ed e' gia' in memoria. Due hash soli manderebbero a riderivare a
    # mano quello che lo script sapeva.
    $diff = Compare-Object -ReferenceObject $before.Paths -DifferenceObject $after.Paths -ErrorAction SilentlyContinue
    $problems.Add(("albero    {0} -> {1}   il contenuto e' cambiato durante la run" -f $before.TreeHash, $after.TreeHash))
    if ($diff) {
        foreach ($d in $diff) {
            $segno = if ($d.SideIndicator -eq '=>') { 'comparso' } else { 'sparito ' }
            $problems.Add(("          {0}  {1}" -f $segno, $d.InputObject.Trim()))
        }
    } else {
        $problems.Add("          gli stessi path, contenuto diverso: qualcuno ha riscritto un file gia' modificato")
    }
}
if ($before.Dlls -ne $after.Dlls) {
    $problems.Add(("binario   {0}" -f $before.Dlls))
    $problems.Add(("       -> {0}   ricompilato durante la run" -f $after.Dlls))
}
if ($after.EngineError) {
    $problems.Add("motore    query fallita a fine run ($($after.EngineError)): impossibile escludere una collisione")
} elseif ($after.EngineCount -gt 0) {
    $problems.Add("motore    un processo del motore e' comparso durante la run:")
    foreach ($e in $after.Engines) { $problems.Add("          $e") }
}

# ---------------------------------------------------------------- IL REFERTO
$found = $null; $completed = 0; $failed = 0; $started = 0
if (-not (Test-Path $LogPath)) {
    $problems.Add("log       non prodotto: il motore e' uscito con codice $engineExit senza scrivere")
} else {
    # Freschezza: il log dev'essere di QUESTA run. Il `Remove-Item` sopra copre il
    # caso normale, ma un file ricomparso con un mtime anteriore all'avvio non e'
    # nostro comunque.
    $logFile = Get-Item $LogPath
    if ($logFile.LastWriteTime -lt $startedAt) {
        $problems.Add(("log       stantio: scritto alle {0:HH:mm:ss}, la run e' partita alle {1:HH:mm:ss}" -f $logFile.LastWriteTime, $startedAt))
    }
    $log = Get-Content $LogPath -Raw
    if ($log -match 'Found (\d+) automation tests') { $found = [int]$Matches[1] }
    $completed = ([regex]::Matches($log, 'Test Completed\.')).Count
    $started   = ([regex]::Matches($log, 'Test Started\.')).Count
    $failed    = ([regex]::Matches($log, 'Result=\{Fail\}')).Count

    if ($null -eq $found) {
        # ⚠️ Misurato: con un filtro che non corrisponde a nessun test UE **non
        # scrive affatto** la riga `Found N` — non scrive `Found 0`. Quindi questo
        # ramo copre due casi diversi, e vanno nominati entrambi: chi ha sbagliato
        # il filtro non deve cercare un difetto della run.
        $problems.Add("copertura il log non dichiara «Found N automation tests»: filtro '$Filter' senza corrispondenze, o run mai partita")
    } elseif ($completed -lt $found) {
        # 🔴 La troncatura e' la meta' silenziosa del difetto, e non si vede dai
        # fallimenti: due run sono morte a 641/1175 e 662/1191 con ZERO rossi.
        #
        # ⚠️ **La soglia e' `$started`, non `$found`, e la differenza e' misurata**:
        # su una suite intera l'ULTIMO test perde regolarmente la riga di
        # conclusione nel flush di shutdown — `clean-baseline.log` riporta 1232
        # avviati e 1231 conclusi, run perfettamente sana. Invalidare su `$found`
        # avrebbe dichiarato NON VALIDA ogni suite completa con quella coda, cioe'
        # avrebbe reso lo script inutile proprio nel caso per cui esiste.
        # Cio' che conta e' quanti test sono PARTITI: se non sono partiti tutti, la
        # run e' stata troncata.
        if ($started -lt $found) {
            $problems.Add(("copertura {0}/{1} avviati: la run e' stata troncata ({2} non partiti, {3} fallimenti)" -f $started, $found, ($found - $started), $failed))
        }
    }
}

$dangling = $started - $completed

# ---------------------------------------------------------------- VERDETTO
Write-Output ''
if ($problems.Count -gt 0) {
    Say 'NON VALIDA'
    foreach ($p in $problems) { Say "  $p" }
    Say ("  esito     {0}/{1}, {2} fail  -> NON REGISTRABILE" -f $completed, $(if ($null -eq $found) { '?' } else { $found }), $failed)
    Say '  Non e'' rosso e non e'' verde: la misura non vale, e si rifa''. Il regime di piu'''
    Say '  sessioni sulla stessa working directory e'' dichiarato in D-222.'
    Say "  log: $LogPath"
    exit 3
}

Say 'VALIDA'
Say ("  HEAD      {0}  albero {1}" -f $after.Head.Substring(0,8), $after.TreeHash)
Say ("  esito     {0}/{1} completati, {2} fallimenti" -f $completed, $found, $failed)
if ($dangling -gt 0) {
    Say ("  nota      {0} test avviati senza riga di conclusione (coda di shutdown, non una troncatura)" -f $dangling)
}
Say ("  durata    {0:mm\:ss}" -f $sw.Elapsed)
Say "  log: $LogPath"
exit $(if ($failed -gt 0) { 1 } else { 0 })
