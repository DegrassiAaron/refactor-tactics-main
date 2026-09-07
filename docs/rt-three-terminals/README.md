# Refactor Tactics — RT Three Terminal Roles

Configurazione locale VS Code per separare sviluppo, validazione Unreal ed Editor senza imporre un numero fisso di finestre terminale.

> `rt-three-terminals` indica **tre ruoli operativi**, non “esattamente tre terminali”.

## I tre ruoli

- **DEV — verde**: codice, test authoring, review, Git, tooling statico/headless. Non deve occupare Unreal.
- **VALIDATION — giallo**: build, targeted Unreal tests, Scenario Harness, full suite. Usa Unreal solo durante una Validation Window deliberata.
- **EDITOR — blu**: Unreal Editor, PIE, MCP/editor automation e acceptance visuale. Nessuna suite/build concorrente.

La modalità globale della macchina è separata dal ruolo di ogni terminale. Aprire o chiudere terminali non modifica lo stato globale.

## Ruolo e wave

Ci sono due livelli. Non confonderli.

| Livello | Domanda | Documenti |
|---|---|---|
| **Ruolo** | Cosa può occupare questo terminale, e con chi confligge | [`prompts/TERMINAL_DEV.md`](prompts/TERMINAL_DEV.md) · [`prompts/TERMINAL_VALIDATION.md`](prompts/TERMINAL_VALIDATION.md) · [`prompts/TERMINAL_EDITOR.md`](prompts/TERMINAL_EDITOR.md) |
| **Wave** | Come si esegue e si consegna un lavoro attraverso i ruoli | [`prompts/RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md) · [`prompts/WAVE_DEV_LEAD.md`](prompts/WAVE_DEV_LEAD.md) · [`prompts/WAVE_DEV_MAIN.md`](prompts/WAVE_DEV_MAIN.md) · [`prompts/WAVE_DEV_TEST.md`](prompts/WAVE_DEV_TEST.md) · [`prompts/WAVE_EDITOR.md`](prompts/WAVE_EDITOR.md) · [`prompts/WAVE_VALIDATION.md`](prompts/WAVE_VALIDATION.md) |

Un prompt di wave presuppone il prompt di ruolo. Non lo sostituisce.

I prompt di wave si incollano **uno per sessione**. Dichiarano identità mutuamente esclusive: incollarne due insieme dà a un agente due ruoli contraddittori.

Formato degli handoff, verdetti e matrice canonica: [`prompts/RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md).
Esempio compilato: [`prompts/RT3_EXAMPLE.md`](prompts/RT3_EXAMPLE.md).
Handoff persistiti: [`waves/`](waves/README.md).

### DEV-LEAD

Per una wave, una singola istanza DEV possiede l'integrazione: consolida il lavoro degli altri DEV ed emette l'handoff RT3 di ingresso verso EDITOR.

`DEV-LEAD` è un ruolo **di wave**, non un quarto ruolo di terminale: `rtmode` e le regole di concorrenza restano quelle DEV.

Se per una wave DEV-LEAD non è designato, la wave non ha ingresso.

Prompt: [`prompts/WAVE_DEV_LEAD.md`](prompts/WAVE_DEV_LEAD.md).

### DEV-MAIN e DEV-TEST

Le altre istanze DEV della wave. Sono ruoli **di wave**, non ruoli di terminale: valgono le regole di concorrenza di `TERMINAL_DEV.md` e nessuna delle due occupa Unreal.

- **DEV-MAIN** implementa il comportamento di produzione dentro lo scope assegnato — [`prompts/WAVE_DEV_MAIN.md`](prompts/WAVE_DEV_MAIN.md);
- **DEV-TEST** scrive Automation Test, scenari e validator, e dichiara i comandi che VALIDATION eseguirà — [`prompts/WAVE_DEV_TEST.md`](prompts/WAVE_DEV_TEST.md).

Nessuna delle due emette un handoff RT3: i tre punti fissi della catena restano DEV-LEAD, EDITOR, VALIDATION. Consegnano un **contributo** in [`waves/<feature-slug>/contrib/`](waves/README.md), che DEV-LEAD consolida in un unico `RT3-DEVLEAD-<sha7>.md`.

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

## Identità del workspace

Il ruolo appartiene alla **sessione**. L'identità appartiene alla **directory**, ed è un'altra cosa:

```text
MAIN                 ospita l'unico bridge MCP della macchina
DEV                  sviluppo
TECHNICAL_DESIGNER   design tecnico
```

⛔ `MAIN` **non** è il branch `main`. È il checkout che ospita il bridge; l'authoring avviene lì, e
su un **branch di task**.

L'identità non si deduce dal nome della cartella: la registra l'installer nel registro per macchina
sotto `%LOCALAPPDATA%\RefactorTactics\RT3\`, e `.vscode/rt-workspace-id.txt` è un promemoria locale.

```powershell
rtws -Action status     # identità di questo checkout + registro macchina
rtws -Action verify     # marker e registro concordano?
```

Due workspace `MAIN` per lo stesso progetto vengono rifiutati: sarebbero due bridge che si
contendono un Editor. Spostare `MAIN` richiede `-Force` esplicito, e una directory spostata non
promuove automaticamente nessun'altra.

## Lease del motore

Unreal è **uno** e lo condividono tutti i checkout. Il permesso di occuparlo vive quindi fuori dal
checkout, accanto al registro.

```powershell
rtlease -Action status
rtlease -Action acquire -Operation SUITE -TaskId 1234
rtlease -Action release
```

⛔ **`acquire` e `release` si eseguono dal terminale RT**, non da un task o da una
shell qualunque. L'owner del lease e' la **sessione**, che sopravvive ai comandi:
un processo effimero che lo acquisisse morirebbe un istante dopo, lasciando un
lease immediatamente `STALE`. Senza l'identita' della sessione i due comandi
falliscono con `RT_SESSION_REQUIRED`.

`status` resta invocabile ovunque: e' sola lettura. Per questo il task VS Code
«RT: Engine lease status» esiste, e i task acquire/release **no**.

- **aprire un terminale non acquisisce niente**: il lease si prende just-in-time, prima di Editor,
  PIE, build, commandlet o di una chiamata MCP che richieda l'Editor vivo;
- non è preemptive: chi trova la risorsa presa vede `BUSY` con owner, task e workspace, e attende;
- un processo motore vivo che nessun lease rivendica **blocca** l'acquisizione;
- il rilascio fallisce finché un processo avviato dalla sessione è ancora vivo.

### La build passa dal lease

```powershell
rtbuild -TaskId 2529
rtbuild -Target RefactorTactics -Configuration Shipping -TaskId 2395
```

⛔ **Non invocare `Build.bat` a mano.** Era il solo passo del gate che tocca il motore senza passare
da nessun guard: nel gate di una wave la build **precede** la suite, e ricompilare le DLL sotto la
full suite di un altro checkout ne rende `NON VALIDA` la misura — per l'invariante «binario» che
`rt-suite.ps1` legge prima e dopo la run. Misurato il 2026-09-06, [#2529](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2529).

`rt-build.ps1` non ha un guard proprio e non deve averlo: chiede il lease per `BUILD`, e la regola
su «il motore è libero?» resta in una sede sola. Esce `2` se non lo ottiene, propaga il codice di
`Build.bat` altrimenti, e rilascia il lease **anche quando la build fallisce**.

⚠️ **Non attende**, per decisione: la richiesta di #2529 è che una build in condizione di contesa sia
**fermata**, non sconsigliata. Chi vuole aspettare guarda `rtlease -Action status`.

⚠️ `rtmode` esiste ancora, ed è **solo informativo**. Il suo file vive per-checkout mentre il motore
è per-macchina: il finding `parsecell-arity/1-F13` ha misurato che la sua lettura era *anticorrelata
con la verità* — con sei checkout attivi, l'unico che dichiarava `VALIDATION` era quello che non
stava usando il motore.

Lo stato locale resta in `.vscode/rt-engine-mode.txt`; `.vscode/` è ignorata da Git.

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

## Task routing

Chi lavora su un task attraverso più ruoli non deve ricordare a chi l'ha già passato.

```text
Terminal -> Run Task -> RT: Open next task terminal
TaskId: 2330
```

Il router legge chi tocca adesso e apre **quel** ruolo. Se tocca a te — un giudizio
visivo, il feel, una decisione di design — non apre un ruolo falso: te lo dice.

```powershell
rttask list                     # tutti i task, con stato e prossimo actor
rttask status -TaskId 2330      # chi ha fatto cosa, e chi tocca
```

Le decisioni di routing le scrive il **RT Coordinator** (`claude --agent rt-coordinator`,
oppure `RT: Open COORDINATOR`), che non è una quarta figura RT3: non imposta
`RT_TERMINAL_ROLE`, non acquisisce Unreal, non esegue suite, non emette verdetti.

Un worker deposita un risultato e basta:

```powershell
rttask report -TaskId 2330 -Status DONE -Summary "..." -NextActorRecommended VALIDATION
```

⛔ `NEXT_ACTOR_RECOMMENDED` è una **raccomandazione**. `next_actor` lo imposta solo
il Coordinator.

Semantica completa — schema, comandi, lifecycle, mismatch, `USER_REQUIRED`, rapporto
con le wave RT3 e cosa **non** è source of truth: [`TASK_ROUTING.md`](TASK_ROUTING.md).

Un terminale aperto senza `TaskId` si comporta esattamente come prima: il router è
un'estensione, non un passaggio obbligato.

## Suite protetta

Da un terminale con ruolo VALIDATION:

```powershell
rtlease -Action acquire -Operation SUITE -TaskId 1234
rtsuite
rtlease -Action release
```

`rtsuite` passa da `scripts/rt-suite-safe.ps1`, che rifiuta l’avvio se:

- il terminale non ha ruolo VALIDATION;
- non esiste un lease vivo;
- il lease appartiene a un'altra sessione;
- il lease è per un'operazione diversa da `SUITE` o `BUILD` — un lease preso per aprire l'Editor non
  autorizza una suite, e usarlo così invaliderebbe la misura che il primo stava proteggendo;
- il lease è su un altro checkout.

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

## Il bridge parte solo in MAIN

Il bridge MCP e' **uno per macchina**, ospitato da `MAIN`. Un Editor aperto in un altro checkout, con
`bAutoStartServer=True`, ne fa partire un **secondo**: nessuno lo usa - i client puntano tutti
all'endpoint di MAIN - ma e' raggiungibile.

⛔ Dietro `call_tool` ci sono 56 toolset, fra cui `AutomationTestToolset` con `RunTests` e
`StopTests`. Una chiamata puo' **avviare una suite** senza passare da `rt-suite.ps1`, dal suo mutex e
dal lease - oppure **fermare** quella di un'altra sessione. Un bridge non governato e' quel canale
lasciato aperto.

```powershell
.\scripts\rt-mcp-server.ps1 -RepoRoot (Get-Location).Path -AutoStart Status
.\scripts\rt-mcp-server.ps1 -RepoRoot (Get-Location).Path -AutoStart Off
```

L'installer lo applica da se': `On` in MAIN, `Off` altrove.

⚠️ Il setting vive in `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`, che e' **per
utente e non versionato**: e' una leva di **macchina**, e un `git pull` non la porta. Va riapplicata su
ogni checkout, ed e' il motivo per cui l'installer la include.

⚠️ **L'Editor riscrive quel file alla chiusura.** Modificarlo con un Editor aperto perde la
modifica in silenzio: lo script rifiuta di procedere e lo dice.

## Enforcement reale

Vale la pena dirlo con precisione, perché la differenza conta.

| Livello | Cosa fa | Cosa non fa |
|---|---|---|
| `.mcp.json`, versionato e installato | decide **se** questo checkout vede il bridge | non distingue le chiamate |
| `rtmcp` (preflight) | verifica ruolo, workspace, branch, task, write-set e lease **prima** di agire | **non intercetta**: il trasporto è HTTP diretto |
| Lease + attribuzione di processo | impedisce che due sessioni occupino il motore insieme | non impedisce a chi salta il preflight di chiamare il bridge |

⛔ Nessuno di questi è una barriera contro chi la voglia aggirare: ruolo e workspace arrivano da
variabili che il chiamante può scrivere. Impediscono l'**errore**, non l'abuso — e chiamarli
sicurezza farebbe smettere di cercare la barriera vera.

### Misurato, non ipotizzato

Il **2026-09-06**, con il bridge vivo, un `initialize` JSON-RPC su `127.0.0.1:8765` da un checkout
qualunque ha risposto **HTTP 200** con una sessione valida: nessuna autenticazione, nessun preflight,
nessuna nozione di quale directory stia chiamando.

Dietro `call_tool` ci sono **56 toolset**, di cui **55 non sono nostri**: fra questi `AssetTools`
(`write_file`, `delete`, `move`, `save_assets`), `AutomationTestToolset` (`RunTests`, `StopTests`) e
`ProgrammaticToolset` (`execute_tool_script`, che esegue Python).

🔴 Conseguenza pratica per chi lavora qui: **una chiamata MCP puo' avviare o fermare una suite**
senza passare da `rt-suite.ps1`, dal lease e dal mutex - cioe' puo' rendere `NON VALIDA` la misura di
un'altra sessione. Finche' il bridge e' acceso, questo canale esiste.

La mitigazione realmente efficace non e' il preflight: e' **non far partire il server** dove non serve
(`bAutoStartServer` e' `EditorPerProjectUserSettings`, per utente), oppure non caricare i toolset
mutanti, oppure interporre un proxy che occupi la porta. Le prime due sono disponibili oggi; la terza
e' codice da scrivere.

## Installazione

Serve **pwsh 7**: `scripts/rt-suite.ps1` è UTF-8 senza BOM e Windows PowerShell 5.1 lo legge come
Windows-1252, producendo 26 errori di parsing. Misurato il 2026-09-06.

Una volta per directory, con l'identità esplicita:

```powershell
# Refactor Tactics Main
.\docs\rt-three-terminals\install-rt-terminals.ps1 -RepoRoot (Get-Location).Path -WorkspaceId MAIN

# Refactor Tactic Dev
.\docs\rt-three-terminals\install-rt-terminals.ps1 -RepoRoot (Get-Location).Path -WorkspaceId DEV

# Refactor Tactics Technical Designer
.\docs\rt-three-terminals\install-rt-terminals.ps1 -RepoRoot (Get-Location).Path -WorkspaceId TECHNICAL_DESIGNER
```

`-WorkspaceId` è obbligatorio: non esiste un default, e il nome della cartella non lo sostituisce.

Opzioni: `-McpEndpoint <url>` per un bridge su porta diversa, `-NoMcp` per non generare `.mcp.json`
in questo checkout, `-Force` per spostare `MAIN`.

### Migrazione da un'installazione precedente

Le versioni precedenti non avevano identità di workspace né lease, e il bridge MCP partiva su **ogni**
checkout. `.mcp.json` era versionato allora ed è versionato oggi: non è cambiato, e non è la leva.

1. `git pull` sul checkout;
2. rieseguire l'installer con `-WorkspaceId`, una volta per directory;
3. verificare con `rtws -Action status` che il registro elenchi **un solo** `MAIN`;
4. `rtlease -Action status` deve dire `LIBERO` con nessun processo motore vivo;
5. `rt-mcp-server.ps1 -AutoStart Status` deve dire `True` **solo** in `MAIN`.

L'installer fa il backup con timestamp di ogni file che sovrascrive, `.mcp.json` incluso. Ripeterlo
è idempotente.

### Rollback

1. ripristinare i `.bak` con il timestamp scelto (`settings.json`, `tasks.json`, gli script, `.mcp.json`);
2. rimuovere `%LOCALAPPDATA%\RefactorTactics\RT3\` se si vuole azzerare registro e lease;
3. `.mcp.json` è versionato: un rollback lo riporta alla versione del commit, e l'installer lo riscrive solo se il contenuto cambia davvero.

L’installer:
- verifica prima che tutto il payload richiesto esista;
- fa backup con timestamp dei file locali esistenti;
- copia i template VS Code locali e gli script.

I template versionati stanno sotto `payload/vscode/`, non `payload/.vscode/`: `.vscode/` è ignorata dal repository e quindi non è una sorgente versionata affidabile del bundle.
