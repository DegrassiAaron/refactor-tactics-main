<#
.SYNOPSIS
  Esegue la META' MECCANICA del giro di `G16` senza una persona davanti, e raccoglie l'evidenza.

.DESCRIPTION
  Il giro di `G16` ha cinque passi, e tre sono percettivi: nessuno script giudica se una cosa si
  LEGGE a schermo. Ma le loro PRECONDIZIONI non sono percettive, e a mano costano una seduta:

    - il feed del Turn Log si popola solo DOPO una risoluzione, e il TurnLog tiene un turno solo
      (`TurnLog.Reset()` a ogni risoluzione): a mano la finestra utile va indovinata;
    - l'archivio replay del passo 5 vale solo se la partita CHIUDE con un esito terminale, e una
      partita abbandonata lascia `Outcome: 0`;
    - gli esiti dal log sono gli stessi sei di `G13`.

  Questo script le verifica da se', in autobattle, e lascia alla persona i soli giudizi percettivi.

  🔑 Il meccanismo che lo rende possibile e' nel codice, non inventato qui: `rt.Debug.ScreenHud`
  accetta un RITARDO in secondi (`RTScreenHudConsole.cpp`), arma un `SetTimer` e scrive il rapporto
  come `Display` — con il commento che dice *«il rapporto differito finisce in un file di log che
  qualcuno legge dopo»*. E' la via d'automazione che il comando si e' data.

.PARAMETER Ritardo
  Secondi di gioco dopo i quali il rapporto del feed viene emesso. Va scelto perche' cada DOPO la
  prima risoluzione: in autobattle un turno si risolve in pochi secondi, 30 e' prudente.

.PARAMETER Minuti
  Tetto al tempo di gioco. Oltre, il processo viene chiuso e la run dichiarata TRONCATA — non valida.

.NOTES
  ⛔ Cosa NON copre, dichiarato perche' non venga scoperto dopo:
    - i passi 1-2 (Ability / Hero Lab): il modulo `RefactorTacticsEditor` e' di tipo `Editor`, quindi
      i Lab NON esistono nel pacchetto. Restano una seduta in Editor;
    - il passo 3, e la metà percettiva del 4: che l'esito si LEGGA a schermo. Questo script prova che
      il feed ha righe, non che siano comprensibili;
    - `G16` nel suo complesso: il criterio chiede le cinque superfici «nello stesso giro», e un giro
      meccanico sul pacchetto non e' quel giro. Questo script PREPARA la seduta, non la sostituisce.
#>
param(
  [int]    $Ritardo = 30,
  [int]    $Minuti  = 12,
  [string] $Mappa   = 'L_HexArena',
  [string] $Scratch = $env:TEMP
)

$ErrorActionPreference = 'Stop'
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
# ⛔ **`ProjectSavedDir()` di un PACCHETTO non e' il `Saved/` del repository**: e' quello dentro lo
# staged. Queste due righe erano scritte indipendentemente -- l'exe sotto `StagedBuilds`, gli archivi sotto
# `$Repo\Saved\Replays` -- e per questo il passo 5 dichiarava `[FAIL]` su partite che avevano registrato
# correttamente (`#3463`, dove l'ipotesi «il pacchetto non registra» e' caduta sulla misura). Ora derivano
# da UNA radice: non possono piu' divergere.
$Staged = Join-Path $Repo 'Saved\StagedBuilds\Windows\RefactorTactics'
$Exe    = Join-Path $Staged 'Binaries\Win64\RefactorTactics.exe'
$Log    = Join-Path $Scratch ('giro-meccanico-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')
$Repl   = Join-Path $Staged 'Saved\Replays'

function Riga($t) { Write-Host $t }

# --- 0. preflight: il motore e' uno, e una misura in finestra sporca non vale ------------------
$occupato = Get-CimInstance Win32_Process |
  Where-Object { $_.Name -match 'UnrealEditor|UnrealPak|AutomationTool|UnrealBuildTool|^cl\.exe$' }
if ($occupato) {
  Riga '[STOP] il motore o la toolchain sono occupati:'
  $occupato | Select-Object ProcessId, Name | Format-Table -AutoSize | Out-String | Write-Host
  Riga '       una run lanciata ora condivide CPU e disco: NON VALIDA. Aspetta.'
  exit 2
}
if (-not (Test-Path $Exe)) {
  Riga "[STOP] pacchetto assente: $Exe"
  Riga '       cuocilo dal clone principale, e conta i .uasset di Content/FabAsset PRIMA.'
  exit 2
}

# Fotografia di cio' che c'era PRIMA: senza, un archivio preesistente si legge come proprio.
$prima = @(Get-ChildItem $Repl -Directory -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name)
Riga ("archivi replay prima della run: " + $prima.Count + "  in " + $Repl)

# --- 1. la run ---------------------------------------------------------------------------------
# -ExecCmds gira DOPO l'allestimento, ed e' esattamente cio' che serve: il comando deve trovare un
# mondo con un TurnManager. Il ritardo interno al comando sposta il rapporto dopo la prima risoluzione.
$argomenti = @(
  $Mappa,
  '-RTAutobattle',
  '-dpcvars=rt.Map.Source=LevelAsset',
  '-windowed', '-ResX=1280', '-ResY=720',
  ('-ExecCmds="rt.Debug.ScreenHud {0}"' -f $Ritardo),
  ('-abslog="{0}"' -f $Log)
)
Riga ''
Riga 'lancio:'
Riga ("  " + $Exe)
Riga ("  " + ($argomenti -join ' '))
Riga ''

$p = Start-Process -FilePath $Exe -ArgumentList $argomenti -PassThru

# Si ATTENDE SUL LOG, non sull'uscita del processo, ed e' il pattern che `tools/suite/esegui.py`
# documenta per la stessa ragione: niente in questo gioco chiude il processo a partita finita --
# resta sulla schermata di Result. La prima stesura di questo script usava WaitForExit, e la run
# veniva TRONCATA dopo che aveva gia' prodotto tutto: il tetto chiudeva un processo che aveva
# finito il lavoro, e dichiarava non valida una misura buona.
$scadenza = (Get-Date).AddMinutes($Minuti)
$finita   = $false
while ((Get-Date) -lt $scadenza) {
  if ($p.HasExited) { break }
  if (Test-Path $Log) {
    # `Partita finita:` e' il testimone della SIMULAZIONE: dice che un esito e' stato deciso.
    if (Select-String -Path $Log -Pattern 'Partita finita:' -Quiet -ErrorAction SilentlyContinue) {
      $finita = $true
      break
    }
  }
  Start-Sleep -Seconds 3
}

if ($finita) {
  # Il rapporto differito puo' cadere dopo la fine: si concede un margine prima di chiudere.
  Start-Sleep -Seconds 5
  Riga 'partita finita: il log porta `Partita finita:`. Chiudo il processo, che da se non esce.'
  try { $p.Kill($true) } catch {}
} elseif ($p.HasExited) {
  Riga ("processo uscito da se con codice " + $p.ExitCode + " -- il verdetto si legge dai conteggi")
} else {
  Riga ("[STOP] oltre il tetto di {0} min SENZA che il log porti ``Partita finita:``." -f $Minuti)
  Riga '       run TRONCATA e non valida: chiudo il processo.'
  try { $p.Kill($true) } catch {}
  exit 3
}

# --- 2. gli esiti dal log ----------------------------------------------------------------------
if (-not (Test-Path $Log)) { Riga "[STOP] nessun log: -abslog si e' perso (virgolette?)"; exit 3 }
$testo = Get-Content -Raw -Encoding UTF8 $Log

function Conta($pat) { ([regex]::Matches($testo, $pat, 'IgnoreCase')).Count }

$esiti = [ordered]@{}
# il CANCELLO: senza, i sei zeri sotto sono un'assenza e non una misura
$esiti['CANCELLO allestimento']   = @{ v = Conta 'Board 2v2'                  ; atteso = '>=1' }
$esiti['CANCELLO fine partita']   = @{ v = Conta 'Fine partita al round'      ; atteso = '>=1' }
$esiti['CANCELLO esito deciso']   = @{ v = Conta 'Partita finita:'            ; atteso = '>=1' }
# il passo 4: il rapporto differito, e il feed con righe
$esiti['rapporto differito']      = @{ v = Conta 'rapporto differito'         ; atteso = '>=1' }
$esiti['feed: righe dopo filtro'] = @{ v = Conta 'righe dopo filtro=[1-9]'    ; atteso = '>=1' }
$esiti['feed: TurnLog vuoto']     = @{ v = Conta "TurnLog e' vuoto"           ; atteso = '0' }
# le tre reti dei crash, perche' la prima e' cieca ai GPU crash (cella G2)
$esiti['crash: rete storica']     = @{ v = Conta 'Fatal error|Assertion failed|Critical error' ; atteso = '0' }
$esiti['crash: rete larga']       = @{ v = Conta 'Error:'                     ; atteso = '0' }
$esiti['crash: GPU']              = @{ v = Conta 'TerminateOnGPUCrash|DXGI_ERROR_DEVICE_REMOVED' ; atteso = '0' }
$esiti['asset mancanti']          = @{ v = Conta 'Failed to find object'      ; atteso = '0' }
$esiti['travel failure']          = @{ v = Conta 'Travel Failure'             ; atteso = '0' }

Riga ''
Riga 'ESITI DAL LOG'
foreach ($k in $esiti.Keys) {
  $e = $esiti[$k]
  $ok = if ($e.atteso -eq '0') { $e.v -eq 0 } else { $e.v -ge 1 }
  Riga ("  {0,-26} {1,6}   atteso {2,-4}  {3}" -f $k, $e.v, $e.atteso, $(if ($ok) {'PASS'} else {'FAIL'}))
}

# --- 3. l'archivio del passo 5, correlato per MatchId ------------------------------------------
$dopo  = @(Get-ChildItem $Repl -Directory -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name)
$nuovi = @($dopo | Where-Object { $prima -notcontains $_ })
Riga ''
Riga ('ARCHIVIO DEL PASSO 5 -- cartelle nuove: ' + $nuovi.Count)
if ($nuovi.Count -eq 0) {
  Riga '  [FAIL] nessun archivio nuovo: il passo 5 non ha nulla da riaprire.'
} else {
  foreach ($n in $nuovi) {
    $m = Join-Path (Join-Path $Repl $n) 'match.rtmanifest'
    if (-not (Test-Path $m)) { Riga ("  [FAIL] {0}: manifest assente" -f $n); continue }
    $j = Get-Content -Raw -Encoding UTF8 $m | ConvertFrom-Json
    # Outcome e' un intero, non un nome: l'ordine e' quello di ERTMatchOutcome
    $nome = @('InProgress','Team0Wins','Team1Wins','Draw')[[int]$j.Outcome]
    $term = [int]$j.Outcome -ne 0
    Riga ("  {0}" -f $n)
    Riga ("     MatchId   {0}" -f $j.MatchId)
    Riga ("     TurnCount {0}" -f $j.TurnCount)
    Riga ("     Outcome   {0} = {1}   {2}" -f $j.Outcome, $nome,
          $(if ($term) {'PASS -- terminale, riapribile'} else {'FAIL -- partita non finita'}))
    # il nome della cartella E' il MatchId in forma Digits: la conferma che il foglio prescrive
    Riga ("     nome == MatchId: {0}" -f $(if ($n -eq $j.MatchId) {'SI'} else {'NO -- da guardare'}))
  }
}

Riga ''
Riga ('log della run: ' + $Log)
Riga ''
Riga 'RESTA ALLA PERSONA, e nessuno script lo sostituisce:'
Riga '  - passi 1-2 (Ability / Hero Lab): i Lab sono Editor-only, non esistono nel pacchetto;'
Riga '  - passo 3: che l''esito del resolver si LEGGA a schermo;'
Riga '  - la meta percettiva del passo 4: che le righe del feed siano comprensibili, non solo presenti;'
Riga '  - il passo 5 aperto nel Replay Viewer: che si riapra e si scorra.'
Riga ''
Riga 'E `G16` NON si chiude da qui: il criterio chiede le cinque superfici <<nello stesso giro>>,'
Riga 'e questa run prepara quella seduta invece di sostituirla.'
