<#
.SYNOPSIS
    DEPRECATO - stato locale informativo. Non autorizza niente.

.DESCRIPTION
    (!!) Questo file resta per compatibilita' con chi ha `rtmode` nelle dita, e ha
    smesso di essere un guard. La ragione e' misurata, non teorica.

    `rt-engine-mode.txt` vive in `<root>/.vscode/`, cioe' PER WORKSPACE ROOT, mentre
    il motore Unreal e' UNO e lo condividono tutti i checkout. Il finding
    `parsecell-arity/1-F13` lo ha misurato con sei checkout attivi: l'unico che
    dichiarava VALIDATION era quello che NON stava usando il motore, e tutti quelli
    che lo usavano leggevano DEV. "La lettura del guard e' anticorrelata con la
    verita'."

    Un file per-root non puo' descrivere una risorsa per-macchina. Chi vuole sapere
    se puo' occupare il motore chiede al lease:

        scripts\rt-lease.ps1 -Action status
        scripts\rt-lease.ps1 -Action acquire -Operation SUITE

    Il mode locale resta utile a una cosa sola: dire a chi guarda il terminale che
    cosa questa finestra INTENDE fare. Non e' un permesso.
#>
param(
    [ValidateSet("DEV", "VALIDATION", "EDITOR", "STATUS")]
    [string]$Mode = "STATUS",

    [string]$WorkspaceRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"
$WorkspaceRoot = [System.IO.Path]::GetFullPath($WorkspaceRoot)
$StateDir = Join-Path $WorkspaceRoot ".vscode"
$StateFile = Join-Path $StateDir "rt-engine-mode.txt"

function Get-RTMode {
    if (-not (Test-Path $StateFile)) { return "DEV" }
    $value = (Get-Content $StateFile -Raw).Trim().ToUpperInvariant()
    if ($value -notin @("DEV", "VALIDATION", "EDITOR")) { return "DEV" }
    return $value
}

function Write-Mode([string]$Value) {
    $color = switch ($Value) {
        "DEV"        { "Green" }
        "VALIDATION" { "Yellow" }
        "EDITOR"     { "Blue" }
        default      { "Gray" }
    }
    Write-Host "RT ENGINE MODE (informativo): " -NoNewline
    Write-Host $Value -ForegroundColor $color
}

function Write-Deprecation {
    Write-Host ""
    Write-Host "(!) Questo valore NON autorizza l'uso del motore." -ForegroundColor DarkYellow
    Write-Host "    E' locale al checkout; il motore e' uno per macchina." -ForegroundColor DarkGray
    Write-Host "    Il permesso vive nel lease: scripts\rt-lease.ps1 -Action status" -ForegroundColor DarkGray
}

if ($Mode -eq "STATUS") {
    Write-Mode (Get-RTMode)
    Write-Deprecation
    exit 0
}

New-Item -ItemType Directory -Force -Path $StateDir | Out-Null
Set-Content -Path $StateFile -Value $Mode -Encoding ASCII

Write-Mode $Mode
Write-Deprecation
exit 0
