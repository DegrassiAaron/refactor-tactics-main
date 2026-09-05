# Refactor Tactics — RT Three Terminal Roles

Configurazione locale VS Code per separare sviluppo, validazione Unreal ed Editor senza imporre un numero fisso di finestre terminale.

> `rt-three-terminals` indica **tre ruoli operativi**, non “esattamente tre terminali”.

## I tre ruoli

- **DEV — verde**: codice, test authoring, review, Git, tooling statico/headless. Non deve occupare Unreal.
- **VALIDATION — giallo**: build, targeted Unreal tests, Scenario Harness, full suite. Usa Unreal solo durante una Validation Window deliberata.
- **EDITOR — blu**: Unreal Editor, PIE, MCP/editor automation e acceptance visuale. Nessuna suite/build concorrente.

La modalità globale della macchina è separata dal ruolo di ogni terminale. Aprire o chiudere terminali non modifica lo stato globale.

## Quanti terminali posso aprire?

I ruoli supportano **N terminali nella stessa directory / stesso checkout**.

Configurazione tipica:

```text
DEV-1   codice gameplay/UI
DEV-2   test
DEV-3   review/docs/GitHub
DEV-4   shell manuale

VALIDATION-1   build e test Unreal
EDITOR-1       Unreal Editor / PIE / MCP
```

| Ruolo | Finestre | Uso concorrente |
|---|---:|---|
| DEV | 1..N | Sì, se non scrivono sugli stessi file e non eseguono operazioni Git distruttive sul working tree condiviso |
| VALIDATION | 1..N | Possono esistere più terminali, ma un solo job che occupa Unreal deve essere attivo alla volta; usare `rtsuite` e il mutex canonico |
| EDITOR | 1..N tecnicamente | Normalmente **una sola sessione Editor attiva e un solo writer `.uasset/.umap`** per checkout |

### Regola Git con più DEV

Tutti vedono lo stesso working tree. Quindi:

- concordare ownership dei file prima di scrivere;
- preferire staging per path espliciti (`git add <path>`);
- evitare `git add -A`, `git commit -am`, `reset`, `restore`, `clean`, `switch`, `pull --rebase` mentre un altro terminale DEV ha lavoro non ancora integrato;
- non assumere che una modifica visibile in `git status` appartenga al terminale corrente.

Più terminali DEV danno parallelismo operativo, **non isolamento Git**.

## Identità del terminale

Ogni istanza riceve un ID locale derivato dal processo, per esempio:

```text
[DEV:18452] [ENGINE:DEV]
[DEV:27104] [ENGINE:DEV]
[EDITOR:30112] [ENGINE:EDITOR]
```

L’ID serve a distinguere le finestre; non crea worktree o sandbox separate.

## Stato globale

Comandi disponibili in ogni terminale:

```powershell
rtstatus
rtmode DEV
rtmode VALIDATION
rtmode EDITOR
```

Lo stato è salvato localmente in `.vscode/rt-engine-mode.txt`. Nel repository `.vscode/` è ignorata da Git.

## Avvio

Dopo l’installazione:

1. apri Refactor Tactics in VS Code;
2. usa `Terminal -> Run Task`;
3. scegli:
   - `RT: Open DEV terminal`
   - `RT: Open VALIDATION terminal`
   - `RT: Open EDITOR terminal`
   - `RT: Open role set (DEV + VALIDATION + EDITOR)`
4. riesegui `RT: Open DEV terminal` ogni volta che serve una nuova istanza DEV.

Resta disponibile `RT: Open 3 terminals` come alias di compatibilità per aprire una istanza per ruolo.

I task usano `panel: new` e `instanceLimit > 1`, quindi lo stesso ruolo può essere aperto più volte.

## Suite protetta

Da un terminale con ruolo VALIDATION:

```powershell
rtmode VALIDATION
rtsuite
```

`rtsuite` passa da `scripts/rt-suite-safe.ps1`, che rifiuta l’avvio se:

- il terminale non ha ruolo VALIDATION;
- la modalità globale non è VALIDATION.

Il wrapper non modifica `scripts/rt-suite.ps1` e non altera i suoi codici/verdetti. La serializzazione effettiva dei job Unreal resta responsabilità del mutex/percorso canonico già esistente.

## Flusso consigliato

```text
DEV pool (1..N)
  implementa + scrive test + review
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
VALIDATION (solo se serve gate finale/reload/package)
```

Non usare `-WaitMinutes` come comportamento normale in DEV. L’attesa del motore ha senso solo dentro una Validation Window deliberata.

## Writer binari

Stessa directory NON significa più writer sicuri.

Per `.uasset/.umap`:

```text
un checkout
+ un Editor attivo
+ un writer binario alla volta
```

Prima di modificare un binario: verificare `git status --short`. Dopo il Save: rileggere dirty state e `git status` prima che DEV esegua operazioni sullo stesso path.

## Installazione

Da PowerShell:

```powershell
.\install-rt-terminals.ps1 -RepoRoot "C:\percorso\a\refactor-tactics-main"
```

L’installer:
- verifica prima che tutto il payload richiesto esista;
- fa backup con timestamp dei file locali esistenti;
- copia i template VS Code locali e gli script.

I template versionati stanno sotto `payload/vscode/`, non `payload/.vscode/`: `.vscode/` è ignorata dal repository e quindi non è una sorgente versionata affidabile del bundle.

## Verifiche dopo l'installazione

Smoke test senza occupare Unreal:

1. apri due volte `RT: Open DEV terminal`;
2. verifica che i due prompt mostrino ID diversi (`[DEV:18452]`, `[DEV:27104]`);
3. esegui `rtstatus` in entrambi;
4. apri `RT: Open VALIDATION terminal` e `RT: Open EDITOR terminal`;
5. verifica che `rtsuite` sia rifiutato da DEV e da EDITOR;
6. esegui `rtmode VALIDATION` e verifica che il gate passi solo dal terminale VALIDATION;
7. non avviare Unreal durante questi passi.
