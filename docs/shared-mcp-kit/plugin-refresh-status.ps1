<#
  plugin-refresh-status.ps1 - quali sessioni Claude aperte stanno ancora sulla configurazione
  plugin vecchia, e quali no.

  Perche' serve: `claude plugin disable` scrive in ~/.claude/settings.json, ma una sessione gia'
  aperta ha gia' costruito il proprio inventario di skill e agent all'avvio e non lo rilegge. Il
  risparmio si vede solo nelle sessioni nuove. Questo script rende visibile la differenza invece
  di lasciarla dedurre dal conteggio dei processi.

  NON TERMINA NIENTE. Chiudere un terminale Claude fa perdere la conversazione viva, ed e' una
  decisione di chi ci sta lavorando. Lo script dice cosa riavviare; il riavvio si fa a mano.

  Metodo:
   * la root di una sessione si ricava dalla connessione TCP verso la Serena di quella root
     (una porta per root, da ~/.mcp-shared/ports.json). E' l'unico legame osservabile dall'esterno
     fra un claude.exe e la directory da cui e' partito;
   * una sessione avviata PRIMA dell'ultima modifica di settings.json e' per definizione sulla
     configurazione vecchia;
   * la sessione corrente viene marcata e mai proposta per il riavvio.
#>
param([switch]$Quiet)

$settings = Join-Path $HOME '.claude\settings.json'
$registry = Join-Path $HOME '.mcp-shared\ports.json'

if (-not (Test-Path $settings)) { throw "settings.json non trovato: $settings" }
$configTime = (Get-Item $settings).LastWriteTime

# ---- porta -> root -------------------------------------------------------------------------
$portToRoot = @{}
if (Test-Path $registry) {
    foreach ($r in ((Get-Content $registry -Raw | ConvertFrom-Json).roots)) {
        $portToRoot[[int]$r.port] = $r.projectId
    }
}

$procs = Get-CimInstance Win32_Process -Filter "Name='claude.exe'"
if (-not $procs) { if (-not $Quiet) { Write-Host "Nessuna sessione Claude attiva." }; return }

# ---- sessione corrente ---------------------------------------------------------------------
$byPid = @{}; Get-CimInstance Win32_Process | ForEach-Object { $byPid[[int]$_.ProcessId] = $_ }
$selfSession = $null
$cur = $PID
while ($byPid.ContainsKey($cur)) {
    if ($byPid[$cur].Name -eq 'claude.exe') { $selfSession = $cur; break }
    $cur = [int]$byPid[$cur].ParentProcessId
}

# ---- root per sessione, via connessione alla Serena ------------------------------------------
$sessionRoot = @{}
Get-NetTCPConnection -State Established -ErrorAction SilentlyContinue |
    Where-Object { $portToRoot.ContainsKey([int]$_.RemotePort) } |
    ForEach-Object { $sessionRoot[[int]$_.OwningProcess] = $portToRoot[[int]$_.RemotePort] }

$rows = foreach ($p in ($procs | Sort-Object CreationDate)) {
    $sp = [int]$p.ProcessId
    $stale = $p.CreationDate -lt $configTime
    [pscustomobject]@{
        PID     = $sp
        Root    = $(if ($sessionRoot.ContainsKey($sp)) { $sessionRoot[$sp] } else { '(root non osservabile)' })
        Avviata = $p.CreationDate.ToString('HH:mm:ss')
        Config  = $(if ($stale) { 'VECCHIA' } else { 'aggiornata' })
        Nota    = $(if ($sp -eq $selfSession) { 'QUESTA sessione - non riavviare da qui' } elseif ($stale) { 'riavvia per applicare' } else { '' })
    }
}
$rows | Format-Table -AutoSize

if ($rows | Where-Object Root -like '*non osservabile*') {
    Write-Host "  (root non osservabile) = la sessione non ha una connessione Serena aperta IN QUESTO"
    Write-Host "  istante; le connessioni HTTP sono transitorie. Non significa che sia fuori dalle root"
    Write-Host "  configurate. Per identificarla: cerca in ~/.claude/projects/ la directory il cui"
    Write-Host "  .jsonl piu' recente combacia con l'orario di avvio della sessione."
}

$toRestart = @($rows | Where-Object { $_.Config -eq 'VECCHIA' -and $_.PID -ne $selfSession })
Write-Host ""
Write-Host "settings.json modificato: $($configTime.ToString('yyyy-MM-dd HH:mm:ss'))"
if ($toRestart.Count -eq 0) {
    Write-Host "Tutte le sessioni sono sulla configurazione corrente."
} else {
    Write-Host "Sessioni da riavviare (oltre a questa): $($toRestart.Count)"
    Write-Host ""
    Write-Host "Nel terminale di ciascuna: /exit  (oppure Ctrl+C due volte), poi dalla stessa directory:"
    Write-Host "    claude --continue        # riprende la conversazione dove era"
    Write-Host ""
    Write-Host "La conversazione non si perde: e' gia' su disco in ~/.claude/projects/<root>/*.jsonl."
}

# ---- la configurazione e' ancora quella che abbiamo scritto? ---------------------------------
$ep = (Get-Content $settings -Raw | ConvertFrom-Json).enabledPlugins
$attesi = @(
    'superpowers@superpowers-marketplace', 'superpowers-dev@superpowers-marketplace',
    'feature-dev@claude-plugins-official', 'frontend-design@claude-code-plugins',
    'security-guidance@claude-code-plugins', 'serena@claude-plugins-official',
    'playwright@claude-plugins-official'
)
$riaccesi = @($attesi | Where-Object { $ep.$_ -ne $false })
Write-Host ""
if ($riaccesi.Count -eq 0) {
    Write-Host "Potatura intatta: tutti i plugin disabilitati lo sono ancora."
} else {
    Write-Warning "RIACCESI da qualcuno (una sessione vecchia puo' riscrivere settings.json all'uscita):"
    $riaccesi | ForEach-Object { Write-Warning "    $_" }
    Write-Host "Ripristina con: claude plugin disable <chiave> --scope user"
}
