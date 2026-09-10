<#
  kit-drift-check.ps1 - il kit versionato e la copia operativa dicono la stessa cosa?

  Il kit vive in due posti con ruoli diversi:
    docs/shared-mcp-kit/   sorgente versionata, da cui si reinstalla. Non esegue niente.
    ~/.mcp-shared/bin/     copia operativa, quella che lo Scheduled Task e l'hook invocano.

  La duplicazione ha gia' prodotto un drift due volte in un giorno: la macchina girava su script
  corretti mentre il repository era fermo a una stesura smentita dalla misura. Questo script rende
  la divergenza visibile invece di lasciarla scoprire alla prossima reinstallazione.

  ⚠️ CONFRONTA IL CONTENUTO, NON I BYTE. Il primo tentativo usava Get-FileHash sui file cosi' come
  stanno, e su questa macchina segnalava DIVERSO su tutto: `git config core.autocrlf` vale `true`,
  quindi il working copy prende CRLF mentre gli script operativi, scritti da shell POSIX, hanno LF.
  Contenuto identico, hash diversi. Un check che segnala sempre un problema inesistente e' peggio
  di nessun check: si impara a ignorarlo, e quando il drift e' vero non lo vede nessuno.
  Qui i fine riga vengono normalizzati prima di confrontare.

  Exit code: 0 se tutto allineato, 1 se c'e' almeno un DIVERSO o un ASSENTE inatteso.
#>
param(
    [string]$KitPath = $PSScriptRoot,
    [string]$LivePath = (Join-Path $HOME '.mcp-shared/bin')
)

# Script che vivono SOLO nel repository: si installano, non si eseguono da ~/.mcp-shared/bin.
$repoOnly = @('install-shared-mcp.ps1', 'kit-drift-check.ps1')

function Get-ContentHash([string]$Path) {
    $text = [IO.File]::ReadAllText($Path) -replace "`r`n", "`n"
    $bytes = [Text.Encoding]::UTF8.GetBytes($text)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash($bytes)).Replace('-', '') }
    finally { $sha.Dispose() }
}

$rows = foreach ($f in (Get-ChildItem (Join-Path $KitPath '*.ps1') | Sort-Object Name)) {
    $live = Join-Path $LivePath $f.Name
    if ($repoOnly -contains $f.Name) {
        [pscustomobject]@{ Script = $f.Name; Stato = 'solo-repo'; Nota = 'si installa, non si esegue' }
    } elseif (-not (Test-Path $live)) {
        [pscustomobject]@{ Script = $f.Name; Stato = 'MANCANTE'; Nota = "assente in $LivePath - copialo" }
    } elseif ((Get-ContentHash $f.FullName) -eq (Get-ContentHash $live)) {
        [pscustomobject]@{ Script = $f.Name; Stato = 'allineato'; Nota = '' }
    } else {
        [pscustomobject]@{ Script = $f.Name; Stato = 'DIVERSO'; Nota = 'decidi quale delle due e'' giusta' }
    }
}

# Script operativi che il repository non conosce: sono drift anche loro.
foreach ($f in (Get-ChildItem (Join-Path $LivePath '*.ps1') -ErrorAction SilentlyContinue | Sort-Object Name)) {
    if (-not (Test-Path (Join-Path $KitPath $f.Name))) {
        $rows += [pscustomobject]@{ Script = $f.Name; Stato = 'NON VERSIONATO'; Nota = 'esiste solo nella copia operativa' }
    }
}

$rows | Format-Table -AutoSize

$bad = @($rows | Where-Object { $_.Stato -in 'DIVERSO', 'MANCANTE', 'NON VERSIONATO' })
Write-Host ""
if ($bad.Count -eq 0) {
    Write-Host "Kit versionato e copia operativa dicono la stessa cosa."
    exit 0
}
Write-Host "Divergenze: $($bad.Count)"
Write-Host "La copia operativa e' quella che ha superato le misure; la versionata e' quella che"
Write-Host "sopravvive a un reinstall. Decidi quale ha ragione PRIMA di allinearle."
exit 1
