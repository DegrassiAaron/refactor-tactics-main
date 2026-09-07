<#
.SYNOPSIS
    Accende o spegne l'avvio automatico del bridge MCP per un checkout.

.DESCRIPTION
    Il bridge MCP e' UNO per macchina, ed e' ospitato dal workspace MAIN. Un Editor
    aperto in un altro checkout, con `bAutoStartServer=True`, fa partire un SECONDO
    bridge: nessuno lo usa - i client puntano tutti all'endpoint di MAIN - ma e'
    raggiungibile, e dietro `call_tool` espone 56 toolset fra cui `AssetTools`
    (`write_file`, `delete`, `move`), `AutomationTestToolset` (`RunTests`,
    `StopTests`) e `ProgrammaticToolset`, che esegue Python.

    (!!) `RunTests` e `StopTests` sono il caso peggiore per questo repository: una
    chiamata MCP puo' avviare una suite senza passare da `rt-suite.ps1`, dal suo
    mutex e dal lease - oppure FERMARE i test che un'altra sessione sta eseguendo.
    Spegnere l'avvio automatico fuori da MAIN chiude quel canale.

    Il setting vive in `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`,
    che e' PER UTENTE e non versionato: e' per questo che la leva e' di macchina e
    non di repository, e va applicata una volta per checkout.

    (!) L'Editor riscrive questo file alla chiusura. Con un Editor aperto sul
    checkout, la modifica verrebbe persa: lo script rifiuta di procedere.
#>
param(
    [Parameter(Mandatory = $true)]
    [string] $RepoRoot,

    [Parameter(Mandatory = $true)]
    [ValidateSet('On', 'Off', 'Status')]
    [string] $AutoStart
)

$ErrorActionPreference = 'Stop'

$RepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
$Ini = Join-Path $RepoRoot 'Saved\Config\WindowsEditor\EditorPerProjectUserSettings.ini'
$DefaultIni = Join-Path $RepoRoot 'Config\DefaultEditorPerProjectUserSettings.ini'
$Section = '[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]'
$Key = 'bAutoStartServer'

if (-not (Test-Path (Join-Path $RepoRoot 'RefactorTactics.uproject'))) {
    Write-Host "BLOCKED: $RepoRoot non contiene RefactorTactics.uproject." -ForegroundColor Red
    exit 2
}

if ($AutoStart -ne 'Status' -and -not (Test-Path $Ini)) {
    Write-Host "NOT RUN: $Ini non esiste." -ForegroundColor Yellow
    Write-Host "Il file nasce alla prima apertura dell'Editor su questo checkout." -ForegroundColor DarkGray
    Write-Host "Fino ad allora vale il default versionato in Config/." -ForegroundColor DarkGray
    exit 3
}

# Legge il valore dentro la sezione giusta: la stessa chiave puo' comparire in
# altre sezioni, e cercarla ovunque leggerebbe quella sbagliata.
function Get-ValueFrom {
    param([Parameter(Mandatory)] [string] $Path)

    if (-not (Test-Path $Path)) { return $null }
    $lines = [System.IO.File]::ReadAllLines($Path)
    $inSection = $false
    foreach ($l in $lines) {
        $t = $l.Trim()
        if ($t.StartsWith('[')) { $inSection = ($t -eq $Section); continue }
        if ($inSection -and $t -like "$Key=*") { return $t.Substring($Key.Length + 1) }
    }
    return $null
}

function Get-CurrentValue { return (Get-ValueFrom -Path $Ini) }

# (!) Il valore che conta e' quello EFFETTIVO, e non e' sempre in Saved/.
#
# Unreal legge `Config/DefaultEditorPerProjectUserSettings.ini` come base e lascia
# che `Saved/` lo sovrascriva. Se la chiave manca in Saved, vale il default
# versionato - e dire "<non impostato>" nasconderebbe il valore che l'Editor usera'
# davvero.
function Get-EffectiveValue {
    $local = Get-ValueFrom -Path $Ini
    if ($null -ne $local) {
        return [pscustomobject]@{ Value = $local; Source = 'Saved (locale)' }
    }
    $fromDefault = Get-ValueFrom -Path $DefaultIni
    if ($null -ne $fromDefault) {
        return [pscustomobject]@{ Value = $fromDefault; Source = 'Config/Default (versionato)' }
    }
    return [pscustomobject]@{ Value = $null; Source = 'nessuno' }
}

$current = Get-CurrentValue

if ($AutoStart -eq 'Status') {
    $eff = Get-EffectiveValue
    $shown = $eff.Value
    if ($null -eq $shown) { $shown = '<non impostato>' }
    Write-Host ("{0,-22} {1,-6} da {2}" -f (Split-Path $RepoRoot -Leaf), $shown, $eff.Source)
    exit 0
}

$desired = if ($AutoStart -eq 'On') { 'True' } else { 'False' }

if ($current -eq $desired) {
    Write-Host ("Unchanged: bAutoStartServer e' gia' {0} in {1}" -f $desired, $RepoRoot) -ForegroundColor DarkGray
    exit 0
}

# (!) L'Editor riscrive l'intero file alla chiusura: modificarlo mentre e' aperto
# significa perdere la modifica senza accorgersene.
$running = @(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    Write-Host "BLOCKED: c'e' un processo Unreal vivo. L'Editor riscrive questo file alla chiusura." -ForegroundColor Red
    foreach ($p in $running) { Write-Host ("  pid {0}" -f $p.Id) -ForegroundColor DarkGray }
    Write-Host "Chiudi l'Editor e ripeti." -ForegroundColor Yellow
    exit 2
}

$lines = [System.IO.File]::ReadAllLines($Ini)
$out = New-Object System.Collections.Generic.List[string]
$inSection = $false
$written = $false

foreach ($l in $lines) {
    $t = $l.Trim()
    if ($t.StartsWith('[')) {
        # uscendo dalla sezione senza aver trovato la chiave, la si aggiunge
        if ($inSection -and -not $written) {
            $out.Add("$Key=$desired") | Out-Null
            $written = $true
        }
        $inSection = ($t -eq $Section)
        $out.Add($l) | Out-Null
        continue
    }
    if ($inSection -and $t -like "$Key=*") {
        $out.Add("$Key=$desired") | Out-Null
        $written = $true
        continue
    }
    $out.Add($l) | Out-Null
}

if ($inSection -and -not $written) {
    $out.Add("$Key=$desired") | Out-Null
    $written = $true
}

if (-not $written) {
    Write-Host "NOT RUN: sezione $Section assente in $Ini." -ForegroundColor Yellow
    Write-Host "Nasce quando il plugin MCP viene usato almeno una volta su questo checkout." -ForegroundColor DarkGray
    exit 3
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
Copy-Item $Ini "$Ini.$stamp.bak" -Force

$tmp = "$Ini.tmp"
[System.IO.File]::WriteAllLines($tmp, $out)
Move-Item -Path $tmp -Destination $Ini -Force

$after = Get-CurrentValue
if ($after -ne $desired) {
    Write-Host "ERROR: la scrittura non ha prodotto il valore atteso (letto: '$after')." -ForegroundColor Red
    exit 1
}

Write-Host ("bAutoStartServer: {0} -> {1}   {2}" -f $current, $desired, $RepoRoot) -ForegroundColor Green
exit 0
