<#
.SYNOPSIS
    CLI del control plane RT3: sessioni, eventi e mailbox fra i terminali RT3.

.DESCRIPTION
    Wrapper sottile sul pacchetto Python `tools/rt3`. Non contiene logica: risolve
    l'interprete, mette `tools/rt3` sul PYTHONPATH e passa gli argomenti.

    Perche' il motore e' in Python e non qui, come gli altri script RT: la persistenza
    del control plane DEVE essere SQLite (requisito della milestone), e PowerShell non
    ne ha uno. Servirebbe PSSQLite o System.Data.SQLite - una dipendenza da installare
    su tre workstation - mentre `sqlite3` e' nella libreria standard di Python, che su
    questa macchina c'e' gia' ed e' gia' usato da `tools/`. AGENTS.md §9 vieta di
    introdurre package manager e build step senza una decisione esplicita: questa e' la
    via che non ne introduce nessuno.

    ⚠️ Lo stato NON vive nel repository. Vive in
    `%LOCALAPPDATA%\RefactorTactics\RT3\runtime.db`, come il registro dei workspace di
    `rt-workspace.ps1` e i task di `rt-task-router.ps1`. E' per macchina apposta: i tre
    workspace permanenti sono tre CLONI distinti, e un database dentro il checkout ne
    produrrebbe tre che non si vedono - cioe' il contrario di un control plane.

    Il ruolo, la lane e il workspace group sono DICHIARATI da chi apre la sessione. Non
    si deducono dal nome della directory ne' da quello della finestra VS Code: e' la
    regola di AGENTS.md §11, e vale qui come altrove.

.EXAMPLE
    rt3.ps1 daemon start
    rt3.ps1 session start -id DEV-1 -role DEV -lane DEV -workspace-group DEV -task 2272
    rt3.ps1 status
    rt3.ps1 inbox list

.EXAMPLE
    # Verifica che l'ambiente sia in grado di eseguire il control plane.
    rt3.ps1 -SelfTest
#>
[CmdletBinding()]
param(
    # Esegue i test automatici del pacchetto invece della CLI. E' il gate che dice se
    # questo workspace ha un control plane funzionante - la domanda che si pone dopo
    # aver propagato la modifica agli altri due checkout.
    [switch] $SelfTest,

    # Tutto il resto va alla CLI Python cosi' com'e'.
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $Rt3Args
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

# ---------------------------------------------------------------------------
# Interprete
# ---------------------------------------------------------------------------

function Resolve-Python {
    <#
        Ordine deliberato: il launcher `py -3` per primo perche' su Windows sceglie
        l'installazione registrata anche quando `python` e' lo stub del Microsoft Store,
        che esce 9009 e apre lo Store invece di eseguire qualcosa.
    #>
    $candidates = @(
        @{ Exe = 'py';      Args = @('-3') },
        @{ Exe = 'python3'; Args = @() },
        @{ Exe = 'python';  Args = @() }
    )
    foreach ($candidate in $candidates) {
        $cmd = Get-Command $candidate.Exe -ErrorAction SilentlyContinue
        if ($null -eq $cmd) { continue }
        try {
            $probe = & $candidate.Exe @($candidate.Args + @('-c', 'import sqlite3,sys; print(sys.version_info[0])')) 2>$null
        } catch {
            continue
        }
        if ($LASTEXITCODE -eq 0 -and $probe -match '^3') {
            return $candidate
        }
    }
    return $null
}

$RepoRoot = Split-Path -Parent $PSScriptRoot
$PackageRoot = Join-Path (Join-Path $RepoRoot 'tools') 'rt3'

if (-not (Test-Path (Join-Path $PackageRoot 'rt3\__init__.py'))) {
    Write-Host "RT3_NOT_INSTALLED: il pacchetto non e' in $PackageRoot." -ForegroundColor Red
    Write-Host "Questo workspace non ha il control plane RT3: sincronizzarlo dal checkout che lo porta." -ForegroundColor DarkYellow
    exit 5
}

$python = Resolve-Python
if ($null -eq $python) {
    Write-Host "RT3_PYTHON_MISSING: nessun Python 3 con sqlite3 trovato sul PATH." -ForegroundColor Red
    Write-Host "Il control plane RT3 usa la sola libreria standard: serve un Python 3 qualunque." -ForegroundColor DarkYellow
    exit 5
}

# PYTHONPATH e non un'installazione: il pacchetto vive nel repository, e cosi' la
# versione eseguita e' sempre quella del checkout da cui si lancia il comando. E'
# proprio la proprieta' che serve per accorgersi che due workspace sono disallineati.
$env:PYTHONPATH = if ($env:PYTHONPATH) { "$PackageRoot$([IO.Path]::PathSeparator)$env:PYTHONPATH" } else { $PackageRoot }
$env:PYTHONIOENCODING = 'utf-8'

if ($SelfTest) {
    Push-Location $PackageRoot
    try {
        & $python.Exe @($python.Args + @('-m', 'unittest', 'discover', '-s', 'tests', '-t', '.'))
        exit $LASTEXITCODE
    } finally {
        Pop-Location
    }
}

if (-not $Rt3Args -or $Rt3Args.Count -eq 0) {
    $Rt3Args = @('status')
}

& $python.Exe @($python.Args + @('-m', 'rt3') + $Rt3Args)
exit $LASTEXITCODE
