<#
.SYNOPSIS
    Apre il terminale RT3 che il task richiede adesso, senza chiedere il ruolo.

.DESCRIPTION
    Toglie all'utente l'unica decisione che sbagliava: quale dei tre terminali
    aprire. Il ruolo lo dice il router, non la memoria di chi apre la finestra.

        rt-open-task.ps1 -TaskId 2330
            -> chiede al router chi e' il prossimo actor
            -> se e' DEV | EDITOR | VALIDATION, diventa quel terminale
            -> se e' USER o NONE, NON finge un ruolo: dice perche'

    (!!) Il ruolo si assume caricando `rt-terminal.ps1` con il dot-source, non
    lanciandolo come processo figlio: un processo figlio riceverebbe il ruolo e poi
    morirebbe, lasciando questa finestra senza `rtstatus`, senza prompt e - cio' che
    conta - senza l'identita' OS che il lease usa come owner.

    Non acquisisce Unreal, non muta il routing, non scrive lo stato del task.
#>
param(
    [Parameter(Mandatory = $true)]
    [string] $TaskId,

    [string] $WorkspaceRoot = (Get-Location).Path,

    [string] $InstanceId = ""
)

$WorkspaceRoot = [System.IO.Path]::GetFullPath($WorkspaceRoot)

$ScriptsDir = Join-Path $WorkspaceRoot "scripts"
$Router = Join-Path $ScriptsDir "rt-task-router.ps1"
$Terminal = Join-Path $ScriptsDir "rt-terminal.ps1"

foreach ($required in @($Router, $Terminal)) {
    if (-not (Test-Path $required)) {
        Write-Host "ERROR: $required non trovato. Reinstalla il bundle rt-three-terminals." -ForegroundColor Red
        return
    }
}

# `rt-suite.ps1` non e' parsabile da Windows PowerShell 5.1, e gli script RT che lo
# toccano vogliono pwsh 7. Qui vale la stessa regola, per non aprire un terminale in
# cui meta' dei comandi fallirebbe.
$Pwsh = (Get-Command pwsh -ErrorAction SilentlyContinue)
if ($null -eq $Pwsh) {
    Write-Host "BLOCKED: pwsh 7 non trovato sul PATH." -ForegroundColor Red
    return
}

# ---------------------------------------------------------------------------
# Chi tocca adesso
# ---------------------------------------------------------------------------

# (!!) Lo stdout del router va in $RouterSays, diagnosi COMPRESA: e' un processo
# figlio, quindi anche cio' che scrive con Write-Host finisce catturato invece che a
# schermo. Misurato il 2026-09-06 provando gli argv del task VS Code: su un task
# inesistente l'utente leggeva "nessun actor" e non `TASK_NOT_FOUND`, cioe' perdeva
# il codice che la tabella di recupero gli dice di guardare.
$RouterSays = (& $Pwsh.Source -NoLogo -NoProfile -File $Router -Action next -TaskId $TaskId -WorkspaceRoot $WorkspaceRoot 2>&1)
$RouterExit = $LASTEXITCODE

if ($RouterExit -ne 0) {
    Write-Host ""
    foreach ($line in @($RouterSays)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$line)) { Write-Host ([string]$line) -ForegroundColor Red }
    }
    Write-Host ""
    Write-Host "Nessun terminale aperto: il router non ha un actor per questo task." -ForegroundColor Yellow
    Write-Host "Torna al RT Coordinator." -ForegroundColor Yellow
    return
}

# L'ultima riga non vuota e' l'actor: il router lo scrive da solo, ma un warning di
# PowerShell potrebbe precederlo.
$NextActor = ''
foreach ($line in @($RouterSays)) {
    if (-not [string]::IsNullOrWhiteSpace([string]$line)) { $NextActor = [string]$line }
}

$NextActor = ([string]$NextActor).Trim().ToUpperInvariant()

if ($NextActor -in @("DEV", "EDITOR", "VALIDATION")) {
    Write-Host ("Task {0}: tocca a {1}. Apro il terminale." -f $TaskId, $NextActor) -ForegroundColor Cyan
    Write-Host ""

    # Dot-source: il ruolo, le funzioni rt* e il prompt restano in QUESTA sessione.
    if ([string]::IsNullOrWhiteSpace($InstanceId)) {
        . $Terminal -Role $NextActor -WorkspaceRoot $WorkspaceRoot -TaskId $TaskId
    } else {
        . $Terminal -Role $NextActor -WorkspaceRoot $WorkspaceRoot -TaskId $TaskId -InstanceId $InstanceId
    }
    return
}

# ---------------------------------------------------------------------------
# USER e NONE: nessun ruolo RT3 puo' eseguirli
# ---------------------------------------------------------------------------

Write-Host ""
if ($NextActor -eq "USER") {
    Write-Host "TASK ROUTING: USER_REQUIRED" -ForegroundColor Cyan
    Write-Host ""
    Write-Host ("Il task {0} aspetta TE, non un ruolo RT3." -f $TaskId)
    Write-Host "Nessuno strumento puo' rispondere al posto tuo: giudizio visivo, feel, o una decisione di design."
    Write-Host ""
    & $Pwsh.Source -NoLogo -NoProfile -File $Router -Action assignment -TaskId $TaskId -WorkspaceRoot $WorkspaceRoot
    Write-Host ""
    Write-Host "Quando hai risposto, torna al RT Coordinator: registrera' l'esito e decidera' il prossimo actor." -ForegroundColor Cyan
    Write-Host "Una risposta umana non diventa PASS da sola." -ForegroundColor DarkGray
} else {
    Write-Host "TASK ROUTING: NESSUN ACTOR" -ForegroundColor Yellow
    Write-Host ""
    Write-Host ("Il task {0} non ha un actor assegnato (next_actor = {1})." -f $TaskId, $NextActor)
    Write-Host ""
    & $Pwsh.Source -NoLogo -NoProfile -File $Router -Action status -TaskId $TaskId -WorkspaceRoot $WorkspaceRoot
    Write-Host ""
    Write-Host "Apri il RT Coordinator e assegna il prossimo actor." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Questa finestra e' una shell normale: nessun ruolo RT3 e' stato assunto." -ForegroundColor DarkGray
