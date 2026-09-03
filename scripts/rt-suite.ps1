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
      copertura   `Test Started` contro il `Found N` dichiarato in testa — quanti
                  test sono PARTITI, non quanti sono arrivati in fondo. ⚠️ La
                  differenza e' deliberata e misurata: l'ULTIMO test di una suite
                  intera perde regolarmente la riga di conclusione nel flush di
                  shutdown (`clean-baseline.log`: 1232 avviati, 1231 conclusi, run
                  sana), quindi invalidare sui conclusi renderebbe NON VALIDA ogni
                  suite completa. La ragione per esteso sta sul controllo stesso

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

    Exit code di una MISURA, e sono QUATTRO perche' tanti sono gli stati in cui
    puo' finire:
      0  VALIDA, nessun fallimento
      1  VALIDA, ma dei test falliscono   -> difetto del gioco
      3  NON VALIDA                       -> esito non registrabile, non e' rosso
      2  NON AVVIATA                      -> un processo del motore era VIVO
         (con `-WaitMinutes N` aspetta che si liberi invece di uscire subito)

    ⚠️ Una VOCE RESIDUALE non e' un motore occupato (#2130): un processo puo'
    lasciare dietro di se' una voce che sopravvive alla sua morte, e che nessuno
    puo' terminare. La suite parte lo stesso, e la diagnostica nomina il caso visto
    — `vivo` · `zombie-solo` · `misto` · `query-fallita`.

    ⚠️ Il caso `zombie-solo` presuppone che la voce residuale sia ancora ENUMERATA
    fra i processi: e' cio' che #2130 ha misurato sul campo (`Get-Process -Name` la
    elencava con `HasExited = True`, mentre `Get-Process -Id` non la trovava). Se una
    voce non fosse enumerata affatto, il caso sarebbe `libero` — e la suite
    partirebbe ugualmente, che e' l'esito voluto: cambia l'etichetta, non il verdetto.

    ⚠️ `-SelfTest` NON e' una misura, e i suoi due codici vanno letti in quel modo:
      0  il classificatore dello stato del motore e' conforme
      1  non lo e'  -> il difetto e' in QUESTO script, non nella suite
    Non tocca ne' il progetto ne' il motore, e non richiede che siano installati.

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

.PARAMETER NoIssueRefs
    Salta il promemoria `issue-refs` stampato dopo un verdetto `VALIDA`.

    Quel controllo confronta i percorsi citati dalle issue APERTE con l'albero, e
    sta qui perche' il difetto che chiude non nasce da un commit: nasce dal tempo
    che passa fra la rimozione di un file e la issue che nessuno riapre. Il
    2026-08-31 ne sono state corrette 63, trovate a mano dieci giorni dopo.

    🔴 **Non concorre al verdetto, ed e' una scelta.** Legge GitHub, che cambia
    mentre la suite gira: in una run da quaranta minuti puo' passare all'avvio e
    fallire alla fine. Farlo entrare nelle invarianti renderebbe `NON VALIDA` una
    misura sana per una issue che ha modificato qualcun altro — cioe' il difetto
    che questo script esiste per impedire. Stampa, e l'exit code resta quello dei
    test. Serve rete e `gh`: senza, dichiara `NOT RUN` e non blocca niente.

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
    [int] $PollSeconds = 30,

    # Salta il promemoria `issue-refs` in coda al verdetto.
    #
    # Quel controllo legge GitHub, non l'albero: e' l'unica cosa in questo script
    # che dipende da una fonte che nessuno qui controlla, e per questo NON entra
    # nelle invarianti e NON cambia l'esito. Vedi §PROMEMORIA in fondo.
    [switch] $NoIssueRefs,

    # Verifica il classificatore dello stato del motore su casi fabbricati, e esce
    # senza toccare il motore.
    #
    # 🔴 **Esiste perche' uno zombie WMI non si fabbrica** (#2130): la voce di DoD
    # «con solo una voce residuale la suite parte» non e' provabile a comando
    # finche' la decisione vive dentro la funzione che interroga il sistema.
    # `Resolve-EngineState` e' pura proprio per questo, e qui le si danno le
    # collezioni che il sistema non sa produrre su richiesta.
    #
    # ⚠️ Non e' un framework di test: non ce n'e' uno nel repository, e AGENTS.md §9
    # vieta di introdurre build step senza una decisione. Sta in questo file perche'
    # la regola non deve avere una seconda sede — che e' lo stesso motivo per cui
    # `Test-EngineFree` esiste.
    [switch] $SelfTest
)

$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $RepoRoot 'RefactorTactics.uproject'
$LogPath  = Join-Path $RepoRoot "Saved/Logs/$LogName"
$Dlls     = @(
    (Join-Path $RepoRoot 'Binaries/Win64/UnrealEditor-RefactorTactics.dll'),
    (Join-Path $RepoRoot 'Binaries/Win64/UnrealEditor-RefactorTacticsEditor.dll')
)

# ⚠️ **Le precondizioni non valgono per `-SelfTest`.** Quel modo prova una funzione
# PURA e non tocca ne' il progetto ne' il motore: farlo dipendere da un motore
# installato al percorso cablato qui sotto significa che su un clone fresco —— o
# sulla macchina di chi rilegge la PR —— il classificatore risulterebbe ROTTO
# (`exit 1`) mentre non ha niente che non va. Un test di una funzione pura non si
# fa cadere da un'installazione.
if (-not $SelfTest) {
    if (-not (Test-Path $UProject)) { throw "uproject non trovato: $UProject" }
    $EngineVersion = (Get-Content $UProject -Raw | ConvertFrom-Json).EngineAssociation
    $EngineCmd = "D:/EpicGames/UE_$EngineVersion/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
    if (-not (Test-Path $EngineCmd)) { throw "motore non trovato: $EngineCmd" }
}

function Invoke-Git {
    param([Parameter(Mandatory)] [string[]] $GitArgs)

    # 🔴 **`$ErrorActionPreference = 'Stop'` trasforma ogni riga di stderr di un comando nativo in un
    # `ErrorRecord` che LANCIA, anche quando il comando riesce** — e `2>$null` non protegge, perche' il
    # record nasce prima che la redirezione lo scarti. Un solo file tracciato scritto con terminatori LF
    # basta: `git diff` stampa `warning: LF will be replaced by CRLF`, esce **0**, e lo script muore.
    #
    # Misurato il 2026-09-01 su `#1964`: basta aggiungere a un `.cpp` tracciato una riga che finisca
    # con il solo LF, e la run muore con
    # `NativeCommandError` a `rt-suite.ps1:247`, **nessun verdetto**, mentre `Saved/Logs/` conserva il log
    # della run PRECEDENTE — un verdetto vecchio con l'aria di essere quello nuovo. E il blocco che chiama
    # git si riesegue a ogni giro d'attesa, quindi una run con `-WaitMinutes` ha decine di occasioni di
    # morire, non una.
    #
    # ⚠️ La preferenza si abbassa SOLO attorno all'invocazione e si rialza in `finally`: il resto dello
    # script continua a fallire chiuso. Lo stderr non viene soppresso, viene **separato** — le righe
    # d'avviso escono dal valore di ritorno, quindi non entrano nel digest dell'albero.
    #
    # ⛔ Non si silenzia il warning lato git (`core.autocrlf`, `core.safecrlf`): sposterebbe una
    # configurazione di repository per aggirare un difetto di script, e lascerebbe scoperto ogni altro
    # avviso che git puo' emettere — `detached HEAD`, `index.lock`, refname ambiguo.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $combined = & git @GitArgs 2>&1
        $exit = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previous
    }

    # 🔑 `$LASTEXITCODE` si riscrive DOPO il filtro: i cmdlet qui sotto non lo toccano oggi, ma il
    # `throw` di ogni chiamante lo legge, e un fail-closed che dipende dall'ordine delle righe non e' un
    # fail-closed. Il codice tornato e' quello di git, non quello della pipeline.
    $lines = $combined |
        Where-Object { $_ -isnot [System.Management.Automation.ErrorRecord] } |
        ForEach-Object { [string]$_ }
    $global:LASTEXITCODE = $exit

    # Una riga sola torna come stringa e non come array di uno, che e' cio' che facevano le invocazioni
    # dirette: i chiamanti la passano a `[string]::IsNullOrWhiteSpace` e a `-join`.
    return $lines
}

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
# 🔴 **La fonte del CONTEGGIO e' `Get-Process`, non WMI** (#2130). Una voce di
# processo puo' sopravvivere alla morte del processo: WMI la elenca ancora — con
# `WorkingSetSize` e tutto — mentre `Get-Process -Id` non la trova e `taskkill`
# non ha nulla da terminare. Misurato il 2026-09-02: una voce cosi', ferma da
# dodici ore col log troncato a meta' riga, ha tenuto ferma OGNI run della
# macchina per otto ore. L'unico discriminante e' `HasExited`, che non e' una
# proprieta' CIM — vive su `System.Diagnostics.Process`.
#
# ⚠️ **E il verso conta.** Partire da WMI e chiedere poi a `Get-Process` chi e'
# vivo classificherebbe come RESIDUALE un motore appena nato, visibile a WMI e non
# ancora nella tabella dei processi: un fail-OPEN, cioe' due run che si uccidono.
# Con `Get-Process` per primo, cio' che non e' ancora nella tabella non viene
# contato affatto, e quella finestra resta coperta dall'acquire atomico piu' sotto.
#
# ⚠️ **WMI resta, ma non decide piu' niente**: serve la `CommandLine`, che dice DI
# CHI e' il processo, e la usa solo la diagnostica. Si paga quindi solo quando c'e'
# almeno un processo da raccontare — misurato su questa macchina, dieci giri a
# motore libero: `Get-Process` 8 ms, la CIM filtrata 117 ms. Il caso comune di ogni
# giro d'attesa e' «nessun processo», e ora non costa piu' la query grossa.
#
# 🔴 Un fallimento dell'enumerazione NON e' «nessun processo»: sarebbe
# un'invariante che fallisce APERTA, indistinguibile dal caso sano.
function Get-EngineProcessEntries {
    <#
    .SYNOPSIS
        Enumera i processi del motore con il loro stato di vita. IMPURA: e' il solo
        punto di questo script che interroga il sistema sui processi.
    #>
    try {
        # 🔴 **`-ErrorAction Stop`, e NON `SilentlyContinue`.** La soppressione
        # rendeva questo `catch` irraggiungibile: un cmdlet che sopprime i propri
        # errori non terminanti torna un array VUOTO, e un array vuoto qui significa
        # «motore libero» — l'invariante che fallisce APERTA, cioe' il difetto che
        # questo guard esiste per prevenire.
        #
        # ⚠️ E la soppressione non serviva nemmeno: misurato, un **wildcard** che non
        # corrisponde a niente (`'ZzNoSuchProc*'`) torna zero elementi **senza
        # errore**. Solo un nome ESATTO senza match solleva `ProcessCommandException`,
        # e qui il nome e' sempre un wildcard.
        $procs = @(Get-Process -Name 'UnrealEditor*' -ErrorAction Stop)
    } catch {
        return [pscustomobject]@{ Entries = @(); EnumError = $_.Exception.Message }
    }

    return [pscustomobject]@{ Entries = @($procs | ForEach-Object { ConvertTo-EngineEntry $_ }); EnumError = $null }
}

# Da un processo alla sua voce. Presa a parte perche' la regola «uno stato non
# leggibile vale VIVO» vive qui, e dentro l'enumeratore non era provabile: e' una
# funzione di UN oggetto, e un oggetto il cui `HasExited` solleva si fabbrica.
function ConvertTo-EngineEntry {
    param($Process)
    $probe = 'ok'
    $live = $true

    # 🔴 **Il discriminante e' il TIPO del valore letto, non un `catch`.** Misurato:
    # PowerShell NON propaga le eccezioni sollevate da un getter di proprieta' — ne'
    # da una `ScriptProperty` ne' da un getter .NET vero. L'accesso restituisce
    # `$null` e l'eccezione finisce in `$Error`. Un `try { -not $p.HasExited } catch`
    # e' quindi codice MORTO: il `catch` non scatta mai, `-not $null` vale `$true`, e
    # il valore giusto uscirebbe per caso — con `LiveProbe` che dichiara `ok` una
    # lettura mai avvenuta.
    #
    # 🔴 **Non leggibile ⇒ VIVO.** Una sessione non elevata puo' non poter leggere lo
    # stato di un motore avviato da un altro utente, e concludere «morto» da un
    # accesso negato e' l'invariante che fallisce aperta.
    $raw = $null
    try { $raw = $Process.HasExited } catch { $raw = $null }
    if ($raw -is [bool]) {
        $live = -not $raw
    }
    else {
        $live = $true
        $probe = 'non-leggibile'
    }

    return [pscustomobject]@{
        ProcessId   = $Process.Id
        CommandLine = $null   # si riempie solo se qualcuno deve STAMPARLA: vedi Add-EngineCommandLines
        IsLive      = $live
        LiveProbe   = $probe
    }
}

# La `CommandLine` dice DI CHI e' il processo, e la legge solo la diagnostica.
#
# 🔴 **Si paga qui, e non nell'enumerazione, perche' il ciclo d'attesa e' il
# percorso CALDO**: mentre si aspetta c'e' per definizione almeno un processo,
# quindi arricchire dentro `Get-EngineState` avrebbe pagato WMI a ogni giro —
# misurato 184 ms contro i 172 ms della versione che questo cambiamento doveva
# rendere piu' economica. Peggio di prima, per una stringa che l'heartbeat non
# stampa. Da qui: 15 ms per giro, e WMI una volta sola quando c'e' da raccontare.
#
# ⚠️ Un errore qui degrada la riga di spiegazione, non il verdetto: il conteggio e'
# gia' certo. Trattarlo come l'errore dell'enumerazione reintrodurrebbe #2130 con
# un nome nuovo.
function Add-EngineCommandLines {
    param($State)
    if ($State.Engines.Count -eq 0) { return $State }
    $cmdByPid = @{}
    try {
        Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" -ErrorAction Stop |
            ForEach-Object { $cmdByPid[[int]$_.ProcessId] = $_.CommandLine }
    } catch {
        $State.DetailError = $_.Exception.Message
        return $State
    }
    foreach ($e in $State.Engines) {
        if ($cmdByPid.ContainsKey($e.ProcessId)) { $e.CommandLine = $cmdByPid[$e.ProcessId] }
    }
    return $State
}

# Il verdetto, da una collezione GIA' letta. PURA, ed e' cio' che rende il
# predicato verificabile: uno zombie WMI non si fabbrica, ma una collezione che ne
# contiene uno si', e `-SelfTest` fa esattamente quello.
function Resolve-EngineState {
    param(
        $Entries,
        [string] $EnumError
    )

    $all = @($Entries)
    # 🔴 **`-ne $false`, non `{ $_.IsLive }`.** Il filtro per verita' conta come NON
    # viva una voce che il campo non ce l'ha — assente o rinominato vale `$null`,
    # `$null` e' falso, e il verdetto diventa «libero» mentre un motore gira: la
    # stessa forma del difetto che il commento qui sotto racconta, su un campo nuovo.
    # Cosi' invece una voce malformata conta come VIVA, e si sbaglia dalla parte che
    # ferma la run invece di quella che ne uccide due.
    $live = @($all | Where-Object { $_.IsLive -ne $false })

    # I quattro casi hanno un nome perche' la riga di diagnostica deve dire QUALE ha
    # visto: senza, chi legge rifa' a mano le cinque misure che #2130 ha gia' fatto.
    if ($EnumError)                     { $case = 'query-fallita' }
    elseif ($all.Count -eq 0)           { $case = 'libero' }
    elseif ($live.Count -eq 0)          { $case = 'zombie-solo' }
    elseif ($live.Count -eq $all.Count) { $case = 'vivo' }
    else                                { $case = 'misto' }

    # ⚠️ **Gli stessi nomi di `Get-Snapshot`**, e non e' cosmesi: `Test-EngineFree`
    # riceve indifferentemente l'uno o l'altro, e con due vocabolari — `Error`
    # qui, `EngineError` la' — leggeva un campo inesistente. Sotto
    # `Set-StrictMode` LANCIA; senza, la proprieta' assente vale `$null`, la
    # guardia sull'errore risulta sempre passata e la funzione decide sul solo
    # conteggio: sbagliata **in silenzio**, che e' il modo peggiore.
    return [pscustomobject]@{
        Engines     = $all
        LiveCount   = $live.Count
        EngineError = $EnumError
        # Vuoto finche' qualcuno non chiede le righe di comando: le riempie
        # `Add-EngineCommandLines`, e solo lei puo' fallire senza toccare il verdetto.
        DetailError = $null
        Case        = $case
    }
}

function Get-EngineState {
    $raw = Get-EngineProcessEntries
    return Resolve-EngineState -Entries $raw.Entries -EnumError $raw.EnumError
}

# Una riga per processo, e la sede e' una sola: la stampano il preambolo
# d'attesa, la diagnostica di NON AVVIATA e il controllo di fine run.
function Format-EngineEntry {
    param($Entry)
    if ($Entry.LiveProbe -ne 'ok') { $stato = 'stato non leggibile' }
    elseif ($Entry.IsLive)         { $stato = 'vivo' }
    else                           { $stato = 'residuale' }
    $cmd = $(if ($null -ne $Entry.CommandLine) { $Entry.CommandLine } else { '(riga di comando non disponibile)' })
    return ("[{0,-19}] pid {1,-6} {2}" -f $stato, $Entry.ProcessId, $cmd)
}

# «Il motore e' libero» secondo uno stato gia' letto — mai `EngineCount -eq 0` da
# solo, che e' vero anche quando la QUERY e' fallita. La regola sta in un posto
# perche' era scritta in due, e i due punti gia' non concordavano su cosa fare
# quando l'errore c'e'.
#
# 🔴 **`LiveCount` e non `Engines.Count`** (#2130): con una sola voce residuale
# `Engines` non e' vuoto e il motore e' libero lo stesso. E' la riga che decide se
# lo zombie ferma la macchina.
function Test-EngineFree {
    param($State)
    return (-not $State.EngineError) -and ($State.LiveCount -eq 0)
}

function Get-Snapshot {
    Push-Location $RepoRoot
    try {
        $head = Invoke-Git @('rev-parse', 'HEAD')
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
        $tracked = (Invoke-Git @('diff', 'HEAD')) -join "`n"
        if ($LASTEXITCODE -ne 0) { throw "git diff HEAD fallito (exit $LASTEXITCODE): albero non leggibile" }

        # 🔴 Degli untracked serve il CONTENUTO, non l'elenco dei path, ed e' lo
        # stesso argomento del blocco qui sopra applicato all'altra meta'.
        # Misurato il 2026-08-28 lavorando su `#166`: un `.cpp` nuovo non ancora
        # `git add`-ato e' stato MUTATO fra due run e ricompilato, e il digest e'
        # rimasto `b8e81adc` in entrambe — due misure, una verde e una rossa,
        # dichiarate VALIDE con lo stesso identificatore d'albero. L'invariante
        # era cieca proprio sul file che si sta scrivendo in quel momento.
        $untrackedPaths = @(Invoke-Git @('ls-files', '--others', '--exclude-standard'))
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

        $paths = Invoke-Git @('status', '--porcelain')
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
        # 🔴 `EngineCount` conta le VOCI, `LiveCount` i processi che esistono davvero
        # (#2130). Chi decide se partire legge il secondo: il primo comprende anche
        # le voci residuali, che non tengono nessun mutex.
        LiveCount   = $state.LiveCount
        EngineCase  = $state.Case
        DetailError = $state.DetailError
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
        Write-Information ("[RT-MEASURE] ...attesa: {0:N0}s trascorsi, {1:N0}s residui, {2} processo/i vivo/i ({3})" -f `
            $Waited.Elapsed.TotalSeconds, [Math]::Max(0.0, $BudgetSeconds - $Waited.Elapsed.TotalSeconds), `
            $engineState.LiveCount, $engineState.Case) -InformationAction Continue
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

# ------------------------------------------------------------- SELF-TEST
# 🔴 **Una voce residuale non si fabbrica**, e senza poterla fabbricare la voce di
# DoD «con solo uno zombie la suite parte» si chiude su un aneddoto (#2130).
# `Resolve-EngineState` e' pura per questo: qui le si passano le collezioni che il
# sistema non sa produrre a comando, e si controlla il verdetto.
#
# ⚠️ **Prova il predicato, non l'occorrenza.** Che una voce residuale esista
# davvero e che `HasExited` la marchi resta osservazione sul campo — questo blocco
# dice che, DATA una voce cosi', la suite parte.
#
# ⚠️ Esce prima di toccare il motore, ma DOPO i controlli di testa su uproject e
# percorso del motore: serve comunque un checkout valido.
if ($SelfTest) {
    $failures = 0
    function Assert-Case {
        param([string] $Name, $Entries, [string] $EnumError, [bool] $ExpectFree, [string] $ExpectCase)
        $state = Resolve-EngineState -Entries $Entries -EnumError $EnumError
        $free = Test-EngineFree $state
        $ok = ($free -eq $ExpectFree) -and ($state.Case -eq $ExpectCase)
        if (-not $ok) { $script:failures++ }
        Say ("{0}  {1,-14} caso={2,-14} libero={3,-5} (atteso caso={4}, libero={5})" -f `
            $(if ($ok) { 'ok  ' } else { 'ROTTO' }), $Name, $state.Case, $free, $ExpectCase, $ExpectFree)
    }

    # ⚠️ `$ProcessId` e non `$Pid`: `$PID` e' una variabile automatica di PowerShell
    # — il processo corrente — e un parametro con quel nome la ombreggia.
    function New-Entry {
        param([int] $ProcessId, [bool] $Live, [string] $Probe = 'ok')
        return [pscustomobject]@{ ProcessId = $ProcessId; CommandLine = "UnrealEditor-Cmd.exe (finto $ProcessId)"; IsLive = $Live; LiveProbe = $Probe }
    }

    Say 'self-test del classificatore dello stato del motore (#2130)'
    Assert-Case 'libero'        @()                                                  $null   $true  'libero'
    Assert-Case 'vivo'          @((New-Entry 101 $true))                             $null   $false 'vivo'
    Assert-Case 'zombie-solo'   @((New-Entry 102 $false))                            $null   $true  'zombie-solo'
    Assert-Case 'misto'         @((New-Entry 103 $true), (New-Entry 104 $false))     $null   $false 'misto'
    Assert-Case 'query-fallita' @()                                                  'WMI ko' $false 'query-fallita'
    # 🔴 **Una voce MALFORMATA vale VIVA**: e' il filtro `-ne $false` sopra, e senza
    # questo caso un `Where-Object { $_.IsLive }` tornerebbe senza che nulla lo dica.
    Assert-Case 'campo assente' @([pscustomobject]@{ ProcessId = 106 })              $null   $false 'vivo'

    # ⚠️ **Questi due non passano dal classificatore, e devono esserci lo stesso.**
    # La regola «uno stato non leggibile vale VIVO» sta in `ConvertTo-EngineEntry`, e
    # una prima stesura la «provava» costruendo a mano una voce con `IsLive = $true`:
    # una tautologia — invertendo la regola in produzione il self-test restava verde.
    # Un getter che non risponde si fabbrica, e allora la regola si prova davvero.
    $nonLeggibile = New-Object PSObject
    $nonLeggibile | Add-Member -MemberType NoteProperty -Name Id -Value 107
    $nonLeggibile | Add-Member -MemberType ScriptProperty -Name HasExited -Value { throw [System.ComponentModel.Win32Exception]::new(5) }
    $e1 = ConvertTo-EngineEntry $nonLeggibile
    $ok1 = ($e1.IsLive -eq $true) -and ($e1.LiveProbe -eq 'non-leggibile')
    if (-not $ok1) { $failures++ }
    Say ("{0}  {1,-14} IsLive={2} LiveProbe={3} (atteso IsLive=True, probe non-leggibile)" -f `
        $(if ($ok1) { 'ok  ' } else { 'ROTTO' }), 'non-leggibile', $e1.IsLive, $e1.LiveProbe)

    $uscito = New-Object PSObject
    $uscito | Add-Member -MemberType NoteProperty -Name Id -Value 108
    $uscito | Add-Member -MemberType ScriptProperty -Name HasExited -Value { $true }
    $e2 = ConvertTo-EngineEntry $uscito
    $ok2 = ($e2.IsLive -eq $false) -and ($e2.LiveProbe -eq 'ok')
    if (-not $ok2) { $failures++ }
    Say ("{0}  {1,-14} IsLive={2} LiveProbe={3} (atteso IsLive=False, probe ok)" -f `
        $(if ($ok2) { 'ok  ' } else { 'ROTTO' }), 'probe-uscito', $e2.IsLive, $e2.LiveProbe)

    if ($failures -gt 0) {
        Say ("self-test ROSSO: {0} caso/i non conforme/i" -f $failures)
        exit 1
    }
    Say 'self-test verde: otto casi su otto'
    exit 0
}

$before = Get-Snapshot
Say-Preamble $before

# 🔴 **Inizializzata, e non e' pedanteria.** Un `Set-StrictMode -Version Latest`
# nel profilo di chi lancia si propaga nello scope dello script, e leggere una
# variabile mai impostata LANCIA — con `ErrorActionPreference = 'Stop'` il
# processo esce `1`, che in §OUTPUTS significa «la suite e' rossa, dei test
# falliscono». Per una run che il motore occupato non ha nemmeno avviato.
$script:WaitElapsed = $null

if ($before.LiveCount -gt 0 -and $WaitMinutes -gt 0) {
    $waited = [System.Diagnostics.Stopwatch]::StartNew()
    Say ("in attesa: il motore e' occupato, ricontrollo ogni {0}s per al massimo {1} min" -f $PollSeconds, $WaitMinutes)
    # Le righe di comando si chiedono a WMI QUI, una volta, perche' qui si stampano:
    # dentro il ciclo che segue costerebbero a ogni giro senza che nessuno le legga.
    $null = Add-EngineCommandLines $before
    foreach ($e in $before.Engines) { Say ("  " + (Format-EngineEntry $e)) }

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
            # ⚠️ **Gli stessi UNDICI campi di `Get-Snapshot`, coi suoi nomi esatti**
            # — erano otto prima di #2130, e il numero e' la somma di controllo: chi
            # aggiunge un campo la' e non qui lo scopre solo quando questo ramo
            # LANCIA, cioe' nel giorno peggiore. La
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
                LiveCount   = 0
                EngineCase  = 'query-fallita'
                DetailError = $null
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

# ⚠️ **L'errore va PRIMA del conteggio**: `LiveCount` e' `0` anche quando
# l'enumerazione e' FALLITA, e senza questo ordine il referto diceva «motore libero:
# si parte» e subito dopo «NON AVVIATA: la query e' fallita» — due righe che si
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
if ($before.LiveCount -gt 0) {
    Say ("NON AVVIATA: un processo del motore e' gia' attivo (caso: {0})." -f $before.EngineCase)
    # ⚠️ I secondi TRASCORSI, non il parametro: la prima stesura stampava «dopo 40
    # minuti di attesa» per un'attesa di 437s, e quel numero e' l'unico che
    # permette di attribuire il blocco a chi lo teneva. `$null` quando non si e'
    # atteso affatto — inizializzata in testa, perche' sotto `Set-StrictMode`
    # leggere una variabile mai impostata fa uscire lo script con un codice che
    # significa «test falliti».
    if ($null -ne $script:WaitElapsed) { Say ("  (dopo {0:N0}s di attesa)" -f $script:WaitElapsed) }
    $null = Add-EngineCommandLines $before
    foreach ($e in $before.Engines) { Say ("  " + (Format-EngineEntry $e)) }
    # La provenienza si perde solo se WMI non risponde, e allora va detto: senza
    # questa riga la lista sembra incompleta per un difetto dello script.
    if ($before.DetailError) {
        Say "  (riga di comando non disponibile: la query WMI e' fallita — $($before.DetailError))"
        Say '  Il verdetto qui sopra NON dipende da quella query: il conteggio e'' gia'' certo.'
    }
    Say 'Due run di automation si uccidono a vicenda: il mutex e'' globale sull''eseguibile del'
    Say 'motore, quindi vale anche fra checkout diversi. Se e'' di un ALTRO checkout non'
    Say 'terminarlo — e'' il lavoro di qualcun altro.'
    # ⚠️ **Qui c'era un consiglio, ed e' diventato codice** (#2130). Diceva di
    # guardare a mano l'mtime del log per capire se il processo fosse uno zombie:
    # euristica, manuale, e nel caso misurato nemmeno conclusiva — l'ultima riga era
    # troncata a meta', non `Engine exit requested`. Ora la distinzione la fa
    # `HasExited` sopra, e una voce residuale non ferma piu' nessuno. Resta da dire
    # cosa fare quando il processo e' VIVO davvero, che e' il solo caso che arriva
    # fin qui.
    # ⚠️ La riga dipende dal CASO: con `misto` l'elenco qui sopra contiene anche voci
    # marcate `residuale`, e dire «e' un processo vivo» contraddirebbe cio' che si
    # legge due righe piu' su — in un referto il cui unico scopo e' attribuire il blocco.
    if ($before.EngineCase -eq 'misto') {
        Say ("Fermano la suite i {0} processo/i VIVO/I dell'elenco: le voci `residuale` no," -f $before.LiveCount)
        Say 'e da sole non l''avrebbero fermata.'
    }
    else {
        Say 'E'' un processo VIVO, non una voce residuale: quelle non fermano piu'' la suite.'
    }
    Say 'Aspetta che finisca — oppure, se e'' di QUESTO checkout e sai di poterlo perdere,'
    Say 'chiudilo tu: da un altro checkout non toccarlo, e'' il lavoro di qualcun altro.'
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
    # 🔴 **`EngineCount` e non `LiveCount`, e qui la differenza e' opposta a quella
    # del guard d'avvio.** All'avvio una voce residuale non prova nulla e non deve
    # fermare la run; a fine run prova che un processo del motore **e' esistito**
    # mentre misuravamo, ed e' esattamente cio' che invalida la misura: il mutex e'
    # globale sull'eseguibile, e una collisione avvenuta non diventa innocua perche'
    # l'altro e' gia' uscito.
    #
    # ⚠️ Una stesura di questo cambiamento filtrava anche qui sulle sole voci vive,
    # per non far accusare la run del residuo del PROPRIO motore. Ma lo script non
    # cattura il pid del figlio (`& $EngineCmd` non e' `Start-Process -PassThru`),
    # quindi non sa distinguere il proprio residuo da quello di un ALTRO checkout: il
    # filtro toglieva anche il secondo, e una collisione vera passava per `VALIDA`.
    # Un falso allarme si legge; una collisione taciuta no. Distinguere davvero
    # richiede il pid del figlio, che e' lavoro suo — e non di #2130.
    $problems.Add("motore    un processo del motore e' comparso durante la run:")
    $null = Add-EngineCommandLines $after
    foreach ($e in $after.Engines) { $problems.Add("          " + (Format-EngineEntry $e)) }
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

# ------------------------------------------------------------- PROMEMORIA
# `issue-refs` confronta i percorsi citati dalle issue APERTE con l'albero. Sta
# qui perche' il difetto che chiude non nasce da un commit: nasce dal tempo che
# passa fra la rimozione di un file e la issue che nessuno riapre. Il 2026-08-31
# ne sono state corrette 63, trovate a mano dieci giorni dopo la rimozione.
#
# 🔴 **Non concorre al verdetto, e non e' una svista.** §9 di AGENTS.md: una
# misura vale solo se osserva lo STESSO HEAD, albero, binario e motore
# dall'inizio alla fine. Questo controllo legge GitHub, che cambia mentre la
# suite gira — in una run da 40 minuti puo' passare all'avvio e fallire alla
# fine. Farlo entrare in `$problems` renderebbe NON VALIDA una misura sana per
# una issue che qualcun altro ha modificato nel frattempo, ed e' esattamente il
# difetto che questo script esiste per impedire. Percio': stampa e basta.
# L'esito resta quello dei test.
if (-not $NoIssueRefs) {
    $gate = Join-Path $RepoRoot 'tools/radar/issue-refs.ts'
    $haveNode = [bool] (Get-Command node -ErrorAction SilentlyContinue)
    $haveGh   = [bool] (Get-Command gh   -ErrorAction SilentlyContinue)

    if (-not (Test-Path $gate)) {
        Say '  issue-refs NOT RUN: tools/radar/issue-refs.ts non presente'
    } elseif (-not $haveNode -or -not $haveGh) {
        Say ("  issue-refs NOT RUN: {0} non disponibile" -f $(if (-not $haveNode) { 'node' } else { 'gh' }))
    } else {
        # `2>&1` perche' il gate scrive la copertura su stderr, come gli altri
        # radar. `--check` per avere l'exit code, che qui si LEGGE e non si
        # propaga.
        #
        # ⚠️ Il `try` non e' difensivita' generica, copre un caso preciso: con
        # `$PSNativeCommandUseErrorActionPreference = $true` — oggi `False` su
        # questa macchina, ma e' una preferenza che si puo' attivare, e in PS 7.4+
        # esiste apposta — un comando nativo che esce non-zero diventa un errore
        # TERMINANTE sotto `ErrorActionPreference = 'Stop'`. Senza `try`, un
        # promemoria informativo farebbe abortire una suite sana **dopo** che i
        # test sono passati, e il referto non uscirebbe affatto.
        $gateExit = 0
        $out = $null
        try {
            $out = & node $gate --check 2>&1
            $gateExit = $LASTEXITCODE
        } catch {
            $gateExit = -1
        }

        $sintesi = ($out | Where-Object { $_ -match 'riferimenti a percorsi RIMOSSI|nessuna issue cita|NOT RUN' } | Select-Object -First 1)
        if ($gateExit -eq 0) {
            Say ("  issue-refs {0}" -f $(if ($sintesi) { $sintesi } else { 'verde' }))
        } elseif ($gateExit -lt 0) {
            Say '  issue-refs NOT RUN: il gate non ha potuto girare (nessun effetto su questo verdetto)'
        } else {
            Say ("  issue-refs 🔴 {0}" -f $(if ($sintesi) { $sintesi } else { "exit $gateExit" }))
            Say '             non tocca l''esito di questa suite: misura GitHub, non l''albero.'
            Say '             per il dettaglio: node tools/radar/issue-refs.ts --check'
        }
    }
}

exit $(if ($failed -gt 0) { 1 } else { 0 })

}
finally {
    # `exit` dentro un `try` esegue comunque il `finally`, quindi il rilascio sta
    # in un posto solo invece che a ognuno dei tre punti d'uscita — che e' il modo
    # in cui un lucchetto sopravvive a un ramo che qualcuno aggiunge domani.
    if ($holdsMutex) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}
