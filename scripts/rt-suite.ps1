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
    esce subito con `2`. Al risveglio lo snapshot si rifa' per intero e il
    preambolo si RIDICHIARA, quindi la run parte da cio' che c'e' quando il motore
    si libera — non da cio' che c'era mezz'ora prima. Non termina mai nessun
    processo: se e' di un altro checkout e' lavoro di qualcun altro.

    ⛔ **Attendere AMPLIFICA il punto cieco dichiarato sopra sul binario.**
    Aspettare il rilascio significa partire nell'istante in cui un'ALTRA sessione
    — che stava molto probabilmente compilando e testando un altro `HEAD` —
    libera il motore. Il DLL che lo snapshot cattura al risveglio e' il suo, resta
    identico per tutta la run, e il verdetto e' `VALIDA` su una suite che esegue
    codice compilato da un commit di qualcun altro. E' la terza modalita' di
    fallimento documentata in D-222.

    ⚠️ **L'attesa non finisce alla PRIMA finestra persa.** Se un altro checkout prende
    il motore mentre lo snapshot gira, si torna ad aspettare finche' il budget regge:
    `-WaitMinutes 40` significa «fino a quaranta minuti», non «un tentativo entro
    quaranta minuti». E' cio' che rende il flag utile su una macchina con piu' checkout
    attivi, ed e' anche cio' che tiene occupato il terminale piu' a lungo (#1650).

    ∴ **Dopo un'attesa lunga, ricompila prima di fidarti del verde.** Il flag
    toglie l'attrito dell'attesa, non il dovere di sapere da quale commit viene il
    binario che stai misurando.

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
    # ⚠️ `ValidateRange` e non un `if` piu' avanti: senza, `-PollSeconds -1` fa
    # esplodere `Start-Sleep` sotto `ErrorActionPreference = 'Stop'` e il processo
    # esce con un codice che questo script NON dichiara — chi lo legge secondo il
    # contratto di §OUTPUTS registra «la suite e' rossa» per una run che non ha
    # mai avviato il motore. Un errore di parametro deve fermarsi prima.
    [ValidateRange(0, 1440)]
    [int] $WaitMinutes = 0,

    # Ogni quanto ricontrollare, mentre attende. Trenta secondi e' il compromesso
    # fra «riparte presto» e «non interroga WMI di continuo per mezz'ora».
    # Minimo 1: con `0` il ciclo diventa uno spin che rifa' la query di continuo
    # sulla stessa macchina dove sta girando l'automation di qualcun altro.
    [ValidateRange(1, 3600)]
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

# Solo i processi del motore: e' l'unica domanda che decide se continuare ad
# attendere, e costa una frazione dello snapshot intero.
#
# ⚠️ **`-Filter` WQL e non `Where-Object`**: la versione non filtrata materializza
# OGNI processo della macchina con la sua `CommandLine` — misurato 901 ms contro
# 561 ms. Su un'attesa di 40 minuti sono 80 giri, e ognuno cade sulla stessa
# macchina dove sta girando l'automation di qualcun altro: il costo del mio
# controllo diventa rumore nella misura del vicino.
#
# 🔴 Un fallimento della query NON e' «nessun processo»: sarebbe un'invariante che
# fallisce APERTA, indistinguibile dal caso sano.
function Get-EngineState {
    try {
        $procs = @(Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" -ErrorAction Stop |
            ForEach-Object { $_.CommandLine })
        # ⚠️ **Gli stessi nomi di `Get-Snapshot`**, e non e' cosmesi: `Test-EngineFree`
        # riceve indifferentemente l'uno o l'altro, e con due vocabolari — `Error`
        # qui, `EngineError` la' — leggeva un campo inesistente. Sotto
        # `Set-StrictMode` LANCIA; senza, la proprieta' assente vale `$null`, la
        # guardia sull'errore risulta sempre passata e la funzione decide sul solo
        # conteggio: sbagliata **in silenzio**, che e' il modo peggiore.
        return [pscustomobject]@{ Engines = $procs; EngineError = $null }
    } catch {
        return [pscustomobject]@{ Engines = @(); EngineError = $_.Exception.Message }
    }
}

# «Il motore e' libero» secondo uno stato gia' letto — mai `EngineCount -eq 0` da
# solo, che e' vero anche quando la QUERY e' fallita. La regola sta in un posto
# perche' era scritta in due, e i due punti gia' non concordavano su cosa fare
# quando l'errore c'e'.
function Test-EngineFree {
    param($State)
    return (-not $State.EngineError) -and ($State.Engines.Count -eq 0)
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
        # 🔴 `$LASTEXITCODE` su OGNI comando git, e non solo su `rev-parse`. Un
        # `git` che fallisce restituisce una stringa vuota, che qui e'
        # indistinguibile da «albero pulito»: l'invariante fallirebbe APERTA, che
        # e' precisamente cio' che il commento sulla query dei processi vieta
        # trenta righe piu' sotto. E la causa non e' teorica — un'altra sessione
        # che tiene `.git/index.lock` a meta' di un `checkout` basta, e questo
        # blocco ora gira anche a ogni giro d'attesa.
        $tracked = (& git diff HEAD 2>$null) -join "`n"
        if ($LASTEXITCODE -ne 0) { throw "git diff HEAD fallito (exit $LASTEXITCODE): albero non leggibile" }

        # 🔴 Degli untracked serve il CONTENUTO, non l'elenco dei path, ed e' lo
        # stesso argomento del blocco qui sopra applicato all'altra meta'.
        # Misurato il 2026-08-28 lavorando su `#166`: un `.cpp` nuovo non ancora
        # `git add`-ato e' stato MUTATO fra due run e ricompilato, e il digest e'
        # rimasto `b8e81adc` in entrambe — due misure, una verde e una rossa,
        # dichiarate VALIDE con lo stesso identificatore d'albero. L'invariante
        # era cieca proprio sul file che si sta scrivendo in quel momento.
        $untrackedPaths = @(& git ls-files --others --exclude-standard 2>$null)
        if ($LASTEXITCODE -ne 0) { throw "git ls-files fallito (exit $LASTEXITCODE): untracked non leggibili" }

        $untracked = ''
        if ($untrackedPaths.Count -gt 0) {
            # ⚠️ **`--no-filters`, e non e' un dettaglio.** Senza, `hash-object`
            # applica il clean filter: con `core.autocrlf=true` e `*.cpp text` in
            # `.gitattributes` — entrambi veri in questo repository — un file
            # riscritto con EOL diversi produce lo STESSO hash. Misurato:
            # `riga1\nriga2` e `riga1\r\nriga2` danno entrambi `e30bf437`, e con
            # `--no-filters` danno `e30bf437` e `2e13e382`. Un digest che
            # normalizza il contenuto non e' un digest del contenuto.
            #
            # `hash-object` e non una lettura diretta: gestisce i binari, non
            # carica il file in memoria, e `--stdin-paths` copre l'elenco in una
            # sola invocazione.
            $hashes = @($untrackedPaths | git hash-object --no-filters --stdin-paths 2>$null)

            # 🔴 **Un path non leggibile degrada SE STESSO, non l'intero blocco.**
            # La prima stesura ripiegava sui soli path per TUTTI quando anche un
            # solo `hash-object` falliva — un file sparito fra l'elenco e la
            # lettura, o un `.uasset` tenuto aperto dall'editor. Due difetti in
            # uno: il buco degli untracked si riapriva in silenzio con la run
            # dichiarata VALIDA, e se il ripiego scattava in UNO SOLO dei due
            # snapshot i digest differivano **per forma**, producendo una
            # collisione che non era mai avvenuta e attribuendola a una sessione
            # vicina. Il segnaposto tiene la forma stabile e nomina il file.
            for ($i = 0; $i -lt $untrackedPaths.Count; $i++) {
                $h = if ($i -lt $hashes.Count -and -not [string]::IsNullOrWhiteSpace($hashes[$i])) {
                    $hashes[$i]
                } else {
                    '(non leggibile)'
                }
                $untracked += "$($untrackedPaths[$i]) $h`n"
            }
        }

        $paths = (& git status --porcelain 2>$null)
        if ($LASTEXITCODE -ne 0) { throw "git status fallito (exit $LASTEXITCODE)" }
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
    $state = Get-EngineState
    $engines = $state.Engines
    $engineError = $state.EngineError

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

function Wait-EngineWindow {
    <#
    .SYNOPSIS
        Aspetta che il motore si liberi, e restituisce lo stato dei processi all'uscita.

    .DESCRIPTION
        Torna quando il motore sembra libero, quando la query sui processi fallisce, o
        quando il budget e' finito.

        ⚠️ **Il chiamante legge SOLO `EngineError`**, e poi ridichiara tutto da uno
        snapshot fresco: il conteggio qui dentro e' vecchio di un istante, e su questa
        macchina un istante basta. La docstring lo dice invece di promettere un contratto
        a tre vie che nessuno usa — chi si fidasse di `Engines.Count` leggerebbe una
        lettura scaduta, o `$null` se il budget era gia' finito all'ingresso.

        ⚠️ **Era il corpo del ciclo d'attesa, ed e' diventata una funzione per un motivo
        preciso**: il chiamante deve poterla RICHIAMARE quando perde la finestra durante
        lo snapshot. Inline, quel ritorno avrebbe richiesto un secondo ciclo attorno a
        sessanta righe, e la reindentazione avrebbe reso illeggibile il diff di un file
        il cui unico scopo e' dire se una misura vale.
    #>
    param(
        [Parameter(Mandatory)] [double] $BudgetSeconds,
        [Parameter(Mandatory)] [int] $PollSeconds,
        [Parameter(Mandatory)] [System.Diagnostics.Stopwatch] $Waited
    )

    # 🔴 **Il jitter si RITIRA a ogni ingresso, e la prima stesura lo azzerava.** Serve a
    # sfasare due sessioni rilasciate dalla stessa terza run; ma il ciclo esterno ricrea
    # quell'evento di rilascio a ogni finestra persa, e con un jitter speso una volta sola
    # le due sessioni tornerebbero in lockstep proprio nel percorso aggiunto per gestirle.
    $jitter = Get-Random -Minimum 0 -Maximum ([Math]::Max(1, [int]($PollSeconds / 3)))

    $engineState = $null
    while ($true) {
        # ⚠️ **Il residuo si misura sul CRONOMETRO, non sull'orologio.** `Get-Date` fa un
        # salto di un'ora due volte l'anno, e `-WaitMinutes 1440` — il massimo che
        # `ValidateRange` ammette — attraversa una transizione per costruzione: sul
        # ritorno all'ora solare un'attesa da 60 minuti ne durava 90, tenendo occupato il
        # terminale di chi l'ha lanciata. Un `Stopwatch` e' monotono e non lo sa nemmeno.
        $remaining = $BudgetSeconds - $Waited.Elapsed.TotalSeconds
        if ($remaining -le 0) { break }

        # Il sonno si CLAMPA al residuo: senza, `-WaitMinutes 1 -PollSeconds 600`
        # dorme dieci minuti dopo aver chiesto di aspettarne uno — il deadline
        # sarebbe un suggerimento, non un limite.
        $nap = [Math]::Min($PollSeconds + $Jitter, $remaining)
        Start-Sleep -Seconds ([Math]::Max(1, [int]$nap))
        $Jitter = 0

        # ⚠️ Solo i PROCESSI, non lo snapshot intero. La domanda del ciclo e' una
        # sola — «e' libero?» — e rifare albero e binario a ogni giro costava
        # quattro invocazioni git piu' una query non filtrata su ogni processo
        # della macchina, ottanta volte, buttandone via settantanove. Quel lavoro
        # cade sulla stessa macchina dove gira l'automation che sto aspettando.
        $engineState = Get-EngineState
        if ($engineState.EngineError) { break }

        if (Test-EngineFree $engineState) {
            # 🔴 **Vedere zero processi non basta a smettere di aspettare.** Fra
            # questo istante e il lancio c'e' lo snapshot completo — un paio di
            # secondi — e in quella finestra un'altra sessione che esegue run **in
            # serie** ne fa partire un'altra. Misurato il 2026-08-28: `wt-dir-c-v02`
            # ha chiuso `rt-c3.log` e aperto `rt-c3-mut.log`, e la prima stesura di
            # questo ciclo usciva proprio li' — arrendendosi dopo 437s di un'attesa
            # da 40 minuti, con quasi tutto il tempo ancora disponibile.
            #
            # ⚠️ La conferma e' un secondo `Get-EngineState`, **non** uno snapshot
            # completo. La prima stesura di questa correzione ci metteva
            # `Get-Snapshot`, e riportava dentro il ciclo esattamente il costo che
            # il commento qui sopra spiega di voler evitare — quattro invocazioni
            # git e una seconda query, a ogni giro che sembra libero, sulla
            # macchina dove sta girando l'automation che aspetto. E `Get-Snapshot`
            # **lancia** su un git che fallisce: dentro il ciclo, senza `try`, quel
            # throw sarebbe uscito dallo script con un codice non dichiarato,
            # proprio quando l'altra sessione sta committando.
            Start-Sleep -Milliseconds 750
            $confirm = Get-EngineState

            if ($confirm.EngineError) {
                # Un fallimento della query NON e' «nessun processo», e non e'
                # nemmeno «si e' rioccupato»: e' l'assenza del dato. Si esce come
                # fa il ciclo esterno, e il verdetto lo decide chi legge l'errore.
                $engineState = $confirm
                break
            }

            $engineState = $confirm

            # `Test-EngineFree` e non `Engines.Count -eq 0`: la regola sta in un posto solo
            # perche' era scritta in due, e i due punti non concordavano su cosa fare
            # quando l'errore c'e'. Qui il conteggio nudo sarebbe corretto **oggi** — il
            # ramo `EngineError` esce tre righe sopra — e sbagliato il giorno in cui quella
            # guardia cambia, trattando una query fallita come «libero».
            if (Test-EngineFree $confirm) { break }

            Write-Information ("[RT-MEASURE] ...finestra persa: il motore si e' rioccupato ({0:N0}s trascorsi, {1:N0}s residui)" -f `
                $Waited.Elapsed.TotalSeconds, [Math]::Max(0.0, $BudgetSeconds - $Waited.Elapsed.TotalSeconds)) -InformationAction Continue
            continue
        }

        # Heartbeat: senza, uno script lanciato a mano tace fino a quaranta
        # minuti ed e' indistinguibile da un blocco — che e' esattamente la
        # ragione per cui il default di `-WaitMinutes` e' `0`. Va sullo stream
        # INFORMATION e non su quello di successo: `> referto.txt` cattura il
        # verdetto, e il battito non deve finirci dentro.
        Write-Information ("[RT-MEASURE] ...attesa: {0:N0}s trascorsi, {1:N0}s residui, {2} processo/i" -f `
            $Waited.Elapsed.TotalSeconds, [Math]::Max(0.0, $BudgetSeconds - $Waited.Elapsed.TotalSeconds), `
            $engineState.Engines.Count) -InformationAction Continue
    }

    return $engineState
}

function Say { param([string] $Text) Write-Output "[RT-MEASURE] $Text" }

# ---------------------------------------------------------------- PRIMA
# Le quattro righe del preambolo stanno in una funzione perche' dopo un'attesa
# vanno RIDICHIARATE tutte: la prima stesura ne ristampava due, e il referto
# restava con l'impronta dei DLL di prima dell'attesa. Chi aspetta il rilascio
# aspetta per definizione una sessione che stava compilando, quindi e' proprio il
# caso in cui il binario cambia — e `binario` non compare da nessun'altra parte
# nel referto. D-222 lo nomina fra le quattro invarianti protette.
function Say-Preamble {
    param($Snapshot)
    Say ("filtro   {0}" -f $Filter)
    Say ("HEAD     {0}" -f $Snapshot.Head.Substring(0, 8))
    Say ("albero   {0}{1}" -f $Snapshot.TreeHash, $(if ($Snapshot.PathCount) { " ($($Snapshot.PathCount) file)" } else { '' }))
    Say ("binario  {0}" -f $Snapshot.Dlls)
}

$before = Get-Snapshot
Say-Preamble $before

# 🔴 **Inizializzata, e non e' pedanteria.** Un `Set-StrictMode -Version Latest`
# nel profilo di chi lancia si propaga nello scope dello script, e leggere una
# variabile mai impostata LANCIA — con `ErrorActionPreference = 'Stop'` il
# processo esce `1`, che in §OUTPUTS significa «la suite e' rossa, dei test
# falliscono». Per una run che il motore occupato non ha nemmeno avviato.
$script:WaitElapsed = $null

if ($before.EngineCount -gt 0 -and $WaitMinutes -gt 0) {
    $waited = [System.Diagnostics.Stopwatch]::StartNew()
    Say ("in attesa: il motore e' occupato, ricontrollo ogni {0}s per al massimo {1} min" -f $PollSeconds, $WaitMinutes)
    foreach ($e in $before.Engines) { Say "  $e" }

    # 🔴 **Il jitter non e' cosmetico**, e vive dentro `Wait-EngineWindow` perche' va
    # ritirato a ogni ingresso: due sessioni bloccate dalla stessa terza run partono con
    # fasi quasi identiche, si svegliano nella stessa finestra, vedono entrambe zero
    # processi e lanciano — e il mutex Live Coding e' globale sull'eseguibile, quindi si
    # uccidono a vicenda. Il jitter le sfasa; l'acquire piu' sotto e' cio' che chiude
    # davvero la corsa.

    # 🔴 **Perdere la finestra DURANTE lo snapshot non e' un'attesa scaduta, e per un giro
    # intero questo script ha detto il contrario.** Misurato il 2026-08-29: uscita `2` con
    # «attesa scaduta dopo 95s» mentre il log, trentadue secondi prima, dichiarava `2.337s
    # residui` — trentanove minuti di budget mai usati. La causa e' che `Get-Snapshot` costa
    # quattro invocazioni git piu' una query sui processi, e in quei secondi un altro
    # checkout prende il motore: il codice cadeva nel ramo `else`, che dice «scaduta» perche'
    # e' l'unico caso che il suo autore aveva previsto.
    #
    # ⚠️ **E' LA STESSA correzione gia' applicata dentro il ciclo** — il `continue` sulla
    # «finestra persa» — mancante nel punto immediatamente successivo. Una difesa che copre
    # un istante e non quello dopo e' una difesa che sembra esserci: e' la ragione per cui
    # qui il ciclo e' esterno invece di essere un secondo `if`.
    #
    # 🔴 **E la prima stesura di QUESTO ciclo l'ha rifatto in un terzo posto.** Aveva un ramo
    # dedicato che, su `EngineError`, faceva snapshot e usciva: un singhiozzo transitorio di
    # `Get-CimInstance` — WMI che si riavvia — bruciava l'intera attesa e cadeva nel messaggio
    # «scaduta» con il budget intatto. Trovato in code review. Oggi non c'e' nessun ramo
    # dedicato: l'errore lo decide lo snapshot, che e' l'unica lettura su cui questo script
    # dichiara qualcosa. Transitorio → lo snapshot risponde e si torna ad aspettare;
    # persistente → anche lo snapshot porta `EngineError`, e si esce dicendo quello.
    $budgetSeconds = [double]$WaitMinutes * 60.0

    while ($true) {
        $null = Wait-EngineWindow -BudgetSeconds $budgetSeconds -PollSeconds $PollSeconds -Waited $waited

        # ⚠️ **`Get-Snapshot` LANCIA su un git che fallisce, e ora sta dentro un ciclo.**
        # Prima girava una volta sola; oggi una per finestra persa, e le finestre perse sono
        # causate dalla stessa attivita' vicina che produce `.git/index.lock`. Senza questo
        # `try` il throw uscirebbe dallo script con un codice **non dichiarato** — `1` sotto
        # `pwsh -File`, che in §OUTPUTS significa «la suite e' rossa, dei test falliscono»,
        # per una run che il motore non ha nemmeno avviato. Si degrada invece di lanciare, e
        # il ramo `EngineError` qui sotto lo riporta come cio' che e': assenza del dato.
        try {
            # Lo snapshot si rifa' SEMPRE dopo l'attesa: l'albero e `HEAD` possono essere
            # stati mossi dalle altre sessioni, e la run deve partire da cio' che c'e' adesso.
            $before = Get-Snapshot
        } catch {
            # ⚠️ **Gli stessi otto campi di `Get-Snapshot`, coi suoi nomi esatti.** La
            # prima stesura ne inventava quattro (`Tree`, `TreeCount`, `Bin`): sotto
            # `Set-StrictMode` leggere una proprieta' assente LANCIA, e il ramo scritto per
            # non far uscire lo script con un codice non dichiarato ce lo avrebbe fatto
            # uscire da solo. Trovato confrontando col `return` della funzione, non a mente.
            $before = [pscustomobject]@{
                Head        = $null
                TreeHash    = $null
                Paths       = @()
                PathCount   = 0
                Dlls        = $null
                Engines     = @()
                EngineCount = 0
                EngineError = $_.Exception.Message
            }
            break
        }

        if ((Test-EngineFree $before) -or $before.EngineError) { break }

        # Il motore e' occupato di nuovo. Si torna ad aspettare **solo** se il budget
        # regge: senza questa riga il ciclo girerebbe oltre il deadline, che e'
        # esattamente l'abuso che `-WaitMinutes` deve impedire.
        if (($budgetSeconds - $waited.Elapsed.TotalSeconds) -le 0) { break }

        Write-Information ("[RT-MEASURE] ...finestra persa durante lo snapshot: si torna in attesa ({0:N0}s trascorsi, {1:N0}s residui)" -f `
            $waited.Elapsed.TotalSeconds, [Math]::Max(0.0, $budgetSeconds - $waited.Elapsed.TotalSeconds)) -InformationAction Continue
    }

    # 🔴 **Il tempo atteso si campiona UNA volta.** Il cronometro corre, e leggerlo
    # due volte per due messaggi diversi puo' dare «attesa scaduta dopo 2400s»
    # seguito da «(dopo 2401s di attesa)»: due numeri per la stessa cosa, in un
    # referto il cui unico scopo e' attribuire il blocco a chi lo teneva.
    #
    # ⚠️ **Gli snapshot consumano il budget, ed e' voluto**: sono lavoro fatto per conto
    # dell'attesa, e contarli fuori lascerebbe il ciclo girare oltre `-WaitMinutes`. Il
    # totale puo' quindi superare il budget di **un** ultimo snapshot, non di piu'.
    $waited.Stop()
    $script:WaitElapsed = $waited.Elapsed.TotalSeconds

    if (Test-EngineFree $before) {
        Say ("motore libero dopo {0:N0}s: stato ridichiarato" -f $script:WaitElapsed)
        Say-Preamble $before
    }
    elseif ($before.EngineError) {
        Say ("attesa interrotta dopo {0:N0}s: la query sui processi e' fallita" -f $script:WaitElapsed)
    }
    else {
        # ⚠️ **«Scaduta» si dice solo quando il budget e' finito davvero**, e questo ramo e'
        # ora raggiungibile per quella sola via. Il budget accanto al tempo trascorso e' un
        # auto-controllo: se i due numeri non tornano, la riga si smentisce da se' — che e'
        # cio' che mancava quando lo script dichiarava scaduta un'attesa con mezz'ora avanti.
        Say ("attesa scaduta dopo {0:N0}s (budget {1:N0}s)" -f $script:WaitElapsed, $budgetSeconds)
    }
}

# ⚠️ **L'errore va PRIMA del conteggio**: `EngineCount` e' `0` anche quando la
# query e' FALLITA, e senza questo ordine il referto diceva «motore libero: si
# parte» e subito dopo «NON AVVIATA: la query e' fallita» — due righe che si
# contraddicono, per chi le legge dopo mezz'ora di attesa.
if ($before.EngineError) {
    Say "NON AVVIATA: la query sui processi e' fallita — $($before.EngineError)"
    if ($null -ne $script:WaitElapsed) { Say ("  (dopo {0:N0}s di attesa)" -f $script:WaitElapsed) }
    Say 'Senza quel dato non si puo'' sapere se il motore e'' libero, e partire alla cieca'
    Say 'significa rischiare che le due run si uccidano a vicenda.'
    exit 2
}

# Un motore gia' vivo PRIMA e' l'unico caso in cui vale la pena non partire: la
# run morirebbe a meta' e il log andrebbe perso nella rotazione. Non contraddice
# l'«esegui e poi dichiara» — non c'e' ancora niente da preservare.
if ($before.EngineCount -gt 0) {
    Say 'NON AVVIATA: un processo del motore e'' gia'' attivo.'
    # ⚠️ I secondi TRASCORSI, non il parametro: la prima stesura stampava «dopo 40
    # minuti di attesa» per un'attesa di 437s, e quel numero e' l'unico che
    # permette di attribuire il blocco a chi lo teneva. `$null` quando non si e'
    # atteso affatto — inizializzata in testa, perche' sotto `Set-StrictMode`
    # leggere una variabile mai impostata fa uscire lo script con un codice che
    # significa «test falliti».
    if ($null -ne $script:WaitElapsed) { Say ("  (dopo {0:N0}s di attesa)" -f $script:WaitElapsed) }
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

# ------------------------------------------------------- ACQUIRE ATOMICO
# 🔴 **Vedere il motore libero non basta a esserne il proprietario.** Due sessioni
# che aspettano la stessa run si svegliano nella stessa finestra, leggono
# entrambe zero processi, e lanciano prima che l'`UnrealEditor-Cmd` dell'altra sia
# visibile a WMI: il mutex Live Coding e' globale sull'eseguibile, quindi si
# uccidono a vicenda e dopo mezz'ora l'esito e' NON VALIDA per entrambe — peggio
# del `2` immediato che `-WaitMinutes` sostituisce. Il numero di sessioni in
# attesa e' esattamente cio' che quel flag aumenta.
#
# Il mutex chiude la corsa fra istanze di QUESTO script — non fra rt-suite e un
# editor aperto a mano, che nessun lock puo' coordinare: quel caso resta coperto
# dal controllo sui processi. Named `Global\` per attraversare le sessioni, e
# rilasciato dall'OS se il processo muore, quindi non lascia lucchetti orfani.
$Mutex = New-Object System.Threading.Mutex($false, 'Global\RTSuiteEngineRun')
$holdsMutex = $false
try {
    $holdsMutex = $Mutex.WaitOne(0)
} catch [System.Threading.AbandonedMutexException] {
    # Una sessione morta senza rilasciare: il lucchetto e' nostro, ed e' proprio il
    # caso che l'eccezione segnala invece di lasciare tutti bloccati per sempre.
    $holdsMutex = $true
}

if (-not $holdsMutex) {
    Say 'NON AVVIATA: un''altra rt-suite sta per lanciare il motore (lock condiviso).'
    Say 'Non e'' una collisione: e'' la corsa evitata. Riprova, o usa -WaitMinutes.'
    exit 2
}

try {

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

}
finally {
    # `exit` dentro un `try` esegue comunque il `finally`, quindi il rilascio sta
    # in un posto solo invece che a ognuno dei tre punti d'uscita — che e' il modo
    # in cui un lucchetto sopravvive a un ramo che qualcuno aggiunge domani.
    if ($holdsMutex) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}
