# Shared MCP Kit for Claude Code on Windows

Ogni terminale Claude Code avvia per conto proprio i server MCP dichiarati come `stdio`. Con piu'
terminali aperti sulla stessa macchina, lo stesso server esiste in tante copie quante sono le
sessioni. Questo kit sposta su HTTP i server che **si possono** condividere, e dichiara — con la
misura che lo dimostra — quali non si possono.

> Gli script di questa cartella sono la **sorgente versionata**, allineata alla copia operativa che
> gira su questa macchina da `~/.mcp-shared/bin/`. La prima stesura del kit conteneva scelte poi
> smentite dalla misura (gateway su 8080, `npx @latest`, stop per PID non verificato): sono
> documentate qui sotto come *deviazioni*, con l'evidenza, perche' chi rilegge non le reintroduca.

## Architettura

### Machine-wide

| Servizio | Stato | Endpoint | Perche' |
|---|---|---|---|
| Playwright | **condiviso** | `http://localhost:8931/mcp` — il socket ascolta solo su `127.0.0.1` | un solo server `--isolated` serve tutti i terminali |
| Gateway (`mcp-gway`) | **rinviato, non rimosso** | — | dopo l'audit non restava un backend che il gateway aiutasse; aggiungerebbe un processo e un hop |
| Episodic Memory | **BLOCKED**, invariato | — | i suoi agent e skill citano `mcp__plugin_episodic-memory_*`: riesporlo con un altro nome li rompe in silenzio |
| Superpowers Chrome | **per sessione**, invariato | — | pilota un Chrome vivo via CDP: due client si contendono la stessa scheda |

### Per root

Un Serena Streamable HTTP **per repo/worktree**, condiviso da ogni terminale aperto in quella root.
Root diverse usano porte diverse; il registro e' `~/.mcp-shared/ports.json`, e le righe `reserved`
esistono perche' una root futura non collida.

I worktree Git separati contano come root separate.

## Installazione

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\install-shared-mcp.ps1            # solo prerequisiti, non avvia niente
Copy-Item .\*.ps1 "$HOME\.mcp-shared\bin\"
# poi: edita ~/.mcp-shared/ports.json, una porta per root simultaneamente attiva
```

In ogni root: copia `project-mcp.settings.template.json` come `.mcp-shared.json`, cambia
`projectId` e `port`, e aggiungi `.mcp-shared.json` a `.git/info/exclude` — e' configurazione
locale, non va committata.

```powershell
& "$HOME\.mcp-shared\bin\start-shared-mcp.ps1"                 # Playwright
& "$HOME\.mcp-shared\bin\start-project-mcp.ps1"                # dalla root, Serena + registrazione
& "$HOME\.mcp-shared\bin\status-shared-mcp.ps1"
```

Serena viene registrata a scope **`local`**, non `project`: `project` scriverebbe dentro il
`.mcp.json` tracciato del repo, sporcando `git status` in ogni root.

## Avvio automatico

Due meccanismi, cosi' non resta niente da ricordare:

* **Scheduled Task** con trigger al logon, run level `Limited`, senza admin: avvia
  `start-shared-mcp.ps1`, che esce subito se la porta e' gia' occupata.
* **Hook `SessionStart`** (`session-start-hook.ps1`, registrato in `~/.claude/settings.json`):
  verifica Playwright e, se la root corrente ha un `.mcp-shared.json`, che la sua Serena sia in
  ascolto. In una root senza quel file e' un no-op silenzioso.

⚠️ L'hook **deve** invocare lo starter con `-Detached`. Misurato il 2026-09-10: con la
redirezione degli stream, `Start-Process` usa `UseShellExecute=false` e quindi
`bInheritHandles=TRUE`, e Serena eredita la pipe di stdout dell'hook tenendola aperta per tutta la
sua vita. Claude Code aspetta quella pipe: una `claude -p` reale e' rimasta ferma **240 s** senza
output. Con `-Detached` la stessa run ha chiuso in **51 s**.

## Deviazioni dalla prima stesura, con l'evidenza

1. **Niente gateway su 8080.** Su questa macchina la porta e' di `com.docker.backend`. E il
   gateway non serviva comunque: vedi tabella sopra.
2. **`--isolated` su Playwright e' obbligatorio.** Senza, il secondo client concorrente riceve
   `Browser is already in use for ...mcp-chrome-<id>, use --isolated`: uno servito, uno errore.
3. **Playwright si registra come `localhost`, non `127.0.0.1`.** `@playwright/mcp` applica un
   controllo sull'header `Host` e risponde `403 Access is only allowed at localhost:8931`.
4. **Si lancia `node <cli.js globale>`, non `npx -y @playwright/mcp@latest`.** Sotto Scheduled Task
   la risoluzione npx si e' bloccata a tempo indefinito — `npx-cli.js` vivo, porta mai occupata,
   log vuoti — mentre lo stesso comando da shell interattiva completava in circa 3 s. Il path
   fisso elimina anche i wrapper `cmd.exe` + `npx-cli.js`, e fissa la versione: `@latest`
   ririsolverebbe a ogni logon.
5. **Playwright gira con working directory neutra.** `browser_snapshot` scrive
   `.playwright-mcp/page-*.yml` relativo al cwd del server: avviato da un repo, lo sporca.
6. **`stop-shared-mcp.ps1` cammina l'albero dei processi.** Il PID registrato e' solo il
   *launcher*: un Serena condiviso e' `serena.exe -> python.exe -> python.exe` e il socket
   appartiene al figlio piu' profondo. Uccidere il PID registrato lo orfanizza e lascia la porta
   occupata.
7. **...e verifica l'identita' prima di uccidere.** Un file `.pid` sopravvive al suo processo e
   Windows riusa i PID: senza il controllo sulla command line, un record stantio porta a
   terminare l'albero di un processo estraneo. Il record viene confrontato con il pattern implicato
   dal nome del file; se non corrisponde non si ferma niente e il record viene rimosso.
8. **Il `.pid` si scrive solo dopo che il socket risponde**, e un avvio fallito viene smontato:
   altrimenti resta un record che dichiara vivo un servizio che non c'e'.
9. **Check-and-start sotto mutex.** Due terminali che aprono la stessa root nello stesso istante
   vedevano entrambi la porta libera. Ora la verifica e l'avvio stanno dentro un mutex per porta,
   e la porta viene ricontrollata dopo aver preso il lock.
10. **`status` verifica *chi* tiene la porta.** Una porta in ascolto prova che un socket e'
    occupato, non che sia il nostro servizio: ogni riga riporta `OK` / `FOREIGN` / `DOWN`.
11. **`status` conta i server, non i processi.** E' il numero che aveva depistato tutti: un Serena
    condiviso e' *tre* processi e uno solo ascolta.
12. **`install` non avvia niente e non forza reinstallazioni.** Installazione e avvio erano
    accoppiati; `uv tool install --force` reinstallava un Serena funzionante a ogni giro.

## Le due copie, e come accorgersi che divergono

Gli script esistono in due posti con ruoli diversi:

* `docs/shared-mcp-kit/` (questo) e' la **sorgente versionata**: da qui si installa su una macchina
  nuova e da qui si ripristina. Non esegue niente.
* `~/.mcp-shared/bin/` e' la copia **operativa**: e' quella che lo Scheduled Task e l'hook
  `SessionStart` invocano davvero.

Questa duplicazione ha gia' prodotto un drift una volta: la macchina girava su script corretti
mentre il repo era fermo alla prima stesura, e chi leggeva il repo trovava istruzioni smentite
dalla misura. Dopo ogni modifica, da un lato o dall'altro, confronta:

```powershell
Get-ChildItem .\*.ps1 | ForEach-Object {
  $mine = (Get-FileHash $_.FullName).Hash
  $live = Join-Path $HOME ".mcp-shared\bin\$($_.Name)"
  $theirs = if (Test-Path $live) { (Get-FileHash $live).Hash } else { 'ASSENTE' }
  [pscustomobject]@{ Script=$_.Name; Uguali=($mine -eq $theirs); Operativo=$theirs }
} | Format-Table -AutoSize
```

Se una riga dice `False`, decidi **quale** delle due e' quella giusta prima di allinearle: la copia
operativa e' quella che ha superato le misure, la copia versionata e' quella che sopravvive a un
reinstall.

`install-shared-mcp.ps1` risulta `False`: e' corretto, vive solo qui e non va copiato in
`~/.mcp-shared/bin/`.

## Plugin duplicati

Gli MCP non sono l'unica cosa che si duplica. Piu' plugin possono esporre **lo stesso contenuto
sotto chiavi diverse**, e ogni chiave abilitata paga il proprio inventario in ogni sessione.

Cercare i doppioni per nome non basta: su questa macchina la copia piu' costosa di `superpowers`
era registrata come `superpowers-dev`, con manifest, descrizione e skill identici. Il confronto va
fatto **per contenuto**:

```powershell
claude plugin details <chiave>      # inventario dei componenti + costo always-on stimato
```

Due distinzioni che cambiano cosa conviene fare:

* piu' **installazioni** dello stesso `nome@marketplace` vivono in
  `~/.claude/plugins/installed_plugins.json`, che mappa ogni plugin a una *lista* di
  installazioni. Costano **zero**: `enabledPlugins` ha una sola chiave e il contenuto si carica
  una volta. Sono record ridondanti del registro, non duplicazione;
* lo stesso contenuto sotto **chiavi diverse** in `enabledPlugins` costa davvero, ogni sessione.

Si disabilita con `claude plugin disable <chiave> --scope user` e si rimette con `enable`: la
cache non viene toccata e il plugin resta su disco.

⚠️ Una sessione gia' aperta **non rilegge** `settings.json`: ha costruito il proprio inventario
all'avvio. Il risparmio si vede solo nelle sessioni nuove, ed e' la stessa dinamica degli MCP. Una
sessione vecchia puo' inoltre riscrivere `settings.json` uscendo, riaccendendo quello che avevi
spento. Per vedere chi e' rimasto indietro e se la potatura regge:

```powershell
& "$HOME\.mcp-shared\bin\plugin-refresh-status.ps1"
```

Lo script non termina niente: chiudere un terminale Claude fa perdere la conversazione viva, ed e'
una decisione di chi ci sta lavorando. Il riavvio si fa a mano — `/exit`, poi `claude --continue`
dalla stessa directory.

## Verifica

```powershell
& "$HOME\.mcp-shared\bin\status-shared-mcp.ps1"
```

La prova che la condivisione funziona non e' il conteggio dei processi: sono le sessioni. Apri due
terminali nella **stessa** root e verifica che nessun processo Serena si aggiunga; aprine uno in
una root **diversa** e verifica che `claude mcp get serena` mostri una porta diversa.

⚠️ Una sessione Claude gia' aperta **non rilegge** la configurazione: tiene i suoi server `stdio`
per tutta la vita. Finche' quelle finestre restano aperte il conteggio resta alto, e questo **non**
e' un fallimento della migrazione. Va riletto dopo aver riavviato i terminali.

## Rollback

```powershell
& "$HOME\.mcp-shared\bin\stop-shared-mcp.ps1" -IncludeSerena
Unregister-ScheduledTask -TaskName 'MCP Shared Services (user)' -Confirm:$false
# ripristina ~/.claude.json e ~/.claude/settings.json dal backup timestampato in
# ~/.mcp-shared/backups/<timestamp>/  (chiudi prima le sessioni Claude: le riscrivono all'uscita)
```

Rollback parziali:

```powershell
claude plugin enable serena@claude-plugins-official       # torna il Serena stdio per sessione
claude plugin enable playwright@claude-plugins-official   # torna il Playwright stdio per sessione
claude mcp remove serena --scope local                    # da dentro la root
claude mcp remove playwright --scope user
```

## Regole di sicurezza

* Ogni servizio condiviso ascolta **solo** su `127.0.0.1`.
* Mai terminare processi Node o Python per nome.
* Si ferma solo un processo di cui si sono verificati PID **e** command line completa.
* Backup della configurazione Claude prima di modificarla, in `~/.mcp-shared/backups/<timestamp>/`.
* Non modificare mai `~/.claude/plugins/cache`.
* Worktree Git separati sono progetti Serena separati.
