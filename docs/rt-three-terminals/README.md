# Refactor Tactics — 3 terminali VS Code

Configurazione locale per separare chiaramente sviluppo, validazione Unreal ed Editor.

## I tre terminali

- **DEV — verde**: codice, test authoring, review, git, tooling statico/headless. Non deve occupare Unreal.
- **VALIDATION — giallo**: build, targeted Unreal tests, Scenario Harness, full suite. Usa Unreal solo durante una Validation Window.
- **EDITOR — blu**: Unreal Editor, PIE, MCP/editor automation e acceptance visuale. Nessuna suite/build concorrente.

La modalità globale della macchina è separata dal ruolo del terminale. Aprire i tre terminali non modifica lo stato globale.

## Stato globale

Comandi disponibili in ogni terminale:

```powershell
rtstatus
rtmode DEV
rtmode VALIDATION
rtmode EDITOR
```

Lo stato è salvato localmente in `.vscode/rt-engine-mode.txt`. Nel repository attuale `.vscode/` è già ignorata da Git.

## Avvio

Dopo l'installazione:

1. apri Refactor Tactics in VS Code;
2. `Terminal -> Run Task -> RT: Open 3 terminals`, oppure `Ctrl+Shift+B`;
3. vedrai tre terminali con icona/colore verde, giallo e blu.

VS Code supporta profili terminale con `icon` e `color`, e task terminali con icone/colori; questa configurazione usa entrambe le possibilità.

## Suite protetta

Dal terminale VALIDATION:

```powershell
rtmode VALIDATION
rtsuite
```

`rtsuite` passa da `scripts/rt-suite-safe.ps1`, che rifiuta l'avvio se:
- il terminale non è VALIDATION;
- la modalità globale non è VALIDATION.

Il wrapper NON modifica `scripts/rt-suite.ps1` e non altera i suoi codici/verdetti.

## Flusso consigliato

```text
DEV
  implementa + scrivi test
  Unreal non occupato
        |
        v
VALIDATION
  build -> targeted -> scenario -> full suite x1/batch
        |
        v
EDITOR
  PIE/MCP/user acceptance
  Save -> Stop PIE -> Close Editor
        |
        v
VALIDATION (se serve gate finale)
```

Non usare `-WaitMinutes` come comportamento normale in DEV. L'attesa del motore ha senso solo dentro una Validation Window deliberata.

## Installazione

Da PowerShell, dalla cartella estratta:

```powershell
.\install-rt-terminals.ps1 -RepoRoot "C:\percorso\a\refactor-tactics-main"
```

L'installer fa backup con timestamp dei file locali esistenti prima di copiarli.
