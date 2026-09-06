# CLAUDE.md — RefactorTactics

Overlay operativo per **Claude Code / SuperClaude**.

> **Prima regola:** leggere [`AGENTS.md`](AGENTS.md).
>
> `CLAUDE.md` non duplica il contratto condiviso. Contiene solo il protocollo Claude-specifico e i rinvii ai suoi owner.

Le sezioni 2–10 conservano la propria numerazione perché altri documenti vi puntano. Dove il contenuto ha un owner altrove, la sezione resta e porta il rinvio.

## 1. Context protocol

Non lavorare dalla memoria del progetto.

### Figura della sessione

Prima di qualunque altra cosa, stabilisci **quale figura** sei fra DEV, EDITOR e VALIDATION. Le tre figure, i loro limiti e le regole di concorrenza sono in [`AGENTS.md`](AGENTS.md) §11.

Il ruolo si **misura**, in quest'ordine:

1. le variabili d'ambiente del terminale, se presenti:

   ```text
   RT_TERMINAL_ROLE       DEV | VALIDATION | EDITOR
   RT_TERMINAL_INSTANCE   identificativo dell'istanza
   RT_WORKSPACE_ROOT      checkout su cui la sessione lavora
   RT_WORKSPACE_ID        MAIN | DEV | TECHNICAL_DESIGNER
   RT_TASK_ID             task/issue, quando dichiarato
   ```

   `RT_WORKSPACE_ID` è l'identità del **workspace**, non la figura della sessione e
   non il branch. Il valore autorevole sta nel registro di macchina: variabile e
   marker locale sono promemoria, e `rtws -Action verify` dice se concordano.

2. `rtstatus`, quando il comando è disponibile: stampa ruolo del terminale, id, engine mode e workspace. Non è definito in ogni shell — se manca, non è un errore, è un dato in meno;
3. la dichiarazione esplicita di chi ha aperto la sessione.

Il ruolo **non si deduce dal nome della directory**: `Main`, `Dev` e `Technical Designer` sono luoghi, non figure.

### Prompt da caricare

Una sessione carica:

- [`docs/rt-three-terminals/prompts/RT3_CONTRACT.md`](docs/rt-three-terminals/prompts/RT3_CONTRACT.md) — il contratto di wave, sempre;
- **un solo** `TERMINAL_*.md`, quello della propria figura;
- **al più un** `WAVE_*.md`, e solo se compatibile con quella figura.

Compatibilità:

| Figura | `TERMINAL_*` | `WAVE_*` ammessi |
|---|---|---|
| DEV | `TERMINAL_DEV.md` | `WAVE_DEV_LEAD.md` · `WAVE_DEV_MAIN.md` · `WAVE_DEV_TEST.md` — uno solo |
| EDITOR | `TERMINAL_EDITOR.md` | `WAVE_EDITOR.md` |
| VALIDATION | `TERMINAL_VALIDATION.md` | `WAVE_VALIDATION.md` |

Due prompt di wave nella stessa sessione danno due identità contraddittorie. Non si sommano.

### Fail-closed

```text
ROLE_MISSING    nessuna delle tre fonti dà un ruolo
ROLE_CONFLICT   le fonti danno ruoli diversi, oppure il prompt caricato
                non appartiene alla figura misurata
```

In entrambi i casi: **fermati e dichiaralo**. Non leggere il repository per indovinare, non avviare Unreal, non scrivere.

Un ruolo dedotto dal contesto è un ruolo inventato, e la figura sbagliata occupa il motore di qualcun altro.

### Prima di modificare

1. misura `git status`;
2. identifica branch;
3. misura `HEAD`;
4. misura `origin/main`;
5. leggi `AGENTS.md`;
6. individua issue/task;
7. individua l'owner documentale;
8. cerca implementazione e test esistenti;
9. dichiara write-set;
10. dichiara rischi e verifiche previste.

Quando pertinenti controlla:

- `docs/product/piano-canonico-mvp.md`
- `docs/decisions/RT_PDR_00_Decision_Log.md`
- ADR applicabili
- `docs/DOC_CONFLICT_MATRIX.md`
- `docs/OPEN_DECISIONS.md`
- roadmap/checkpoint
- spec owner
- codice
- test

Usa search/grep per restringere il contesto prima di aprire documenti lunghi.

### Non usare come autorità implicita

- `docs/research/`
- `docs/archive/`
- PDF
- export
- handoff
- audit
- vecchi snapshot
- numeri copiati

Se due regole sono in conflitto, non riconciliarle a intuito.

Trova l'owner corrente.

## 2. Pin correnti

I pin **non vivono qui**. Copiarli produce due elenchi che divergono in silenzio.

| Cosa | Owner |
|---|---|
| Engine, scope v0.1 e standard, roster, mappa, coordinate, loop, azioni universali, ability system, GAS | [`AGENTS.md`](AGENTS.md) §1 |
| Traversal/Transfer, Sprint, Overwatch, reazioni, Fast Reaction, timeout, thin slice Predictive, High Ground | [`AGENTS.md`](AGENTS.md) §5 |
| Decisioni che hanno fissato un pin | [`docs/decisions/RT_PDR_00_Decision_Log.md`](docs/decisions/RT_PDR_00_Decision_Log.md) |

Il dettaglio vive negli owner gameplay.

Due regole restano Claude-specifiche perché descrivono un errore che un agente commette da solo:

- niente compatibilità implicita con nomi legacy rimossi;
- nessun redirect legacy reintrodotto senza decisione.

## 3. Guardrail architetturali

Gli invarianti sono in [`AGENTS.md`](AGENTS.md) §3, numerati: determinismo e confine simulazione/presentazione (1–6), substrato spaziale unico (7–11), ownership del gameplay (12–17).

Qui resta solo il modo in cui un agente li viola.

Non si rompono per una scelta di architettura. Si rompono in un punto **locale**, dove la scorciatoia è più corta della regola: un tempo reale letto «solo per questo caso», una seconda conversione fra mondo ed esagoni scritta per far funzionare una feature, un ramo su due eroi al posto di uno stato condiviso.

Una scorciatoia che funziona resta un secondo owner.

Prima di introdurre una struttura nuova nel dominio spaziale, nella risoluzione o nelle abilità, verifica quale invariante numerato la copre già. Se la copre, non serve la struttura. Se non la copre, la sede della decisione è la spec dell'owner, non il resolver.

## 4. Privacy

Owner della regola: [`AGENTS.md`](AGENTS.md) §4, che dichiara dove il planning avversario non può stare e quali informazioni UI e warning possono leggere.

Qui resta l'errore che un agente commette da solo: replicare l'informazione e poi **nasconderla in presentazione**.

Un dato che ha raggiunto il client è arrivato. Nessun filtro di UI lo fa tornare indietro, e l'assenza dalla schermata avversaria non è una prova della sua assenza.

Da cui:

- se una feature ha bisogno di un'informazione che il client non deve conoscere, si sposta il calcolo sull'autorità: non si filtra il rendering;
- un warning costruito su un intento privato avversario è un difetto di autorità, non un dettaglio di presentazione;
- il confine si verifica su ciò che viaggia, non su ciò che si vede — ed è per questo che il verdetto su privacy e autorità di rete appartiene a VALIDATION, [`AGENTS.md`](AGENTS.md) §11.

## 5. Unreal asset safety

Asset proprietari:

`/Game/RT/`

Le convenzioni di contenuto, i prefissi, i redirector e il write-set binario sono in [`AGENTS.md`](AGENTS.md) §7. Owner documentale:

[`docs/technical/tooling/convenzioni-contenuti-ue.md`](docs/technical/tooling/convenzioni-contenuti-ue.md)

Non:

- editare `.uasset` a mano;
- editare `.umap` a mano;
- spostare asset Unreal da filesystem;
- modificare binari posseduti da un'altra sessione;
- sovrascrivere un working tree sporco;
- editare viste generate.

Usare:

- Content Browser;
- Unreal/Epic MCP come prima scelta quando serve creare, modificare, analizzare o validare asset, mappe, Blueprint o altro stato Editor-only;
- Fix Up Redirectors;
- Binary Asset Lease;
- write-set esplicito.

Il repository non usa Git LFS.

### Lifecycle Editor / MCP

Non tenere Unreal Editor aperto per default.

Quando il task richiede asset, mappe, Blueprint, PIE o altra verifica Editor-only:

1. scopri gli strumenti Unreal/Epic MCP realmente disponibili;
2. verifica se esiste già un'istanza Editor per RefactorTactics e chi la possiede;
3. avvia l'Editor solo se necessario;
4. preferisci MCP alle manipolazioni filesystem dei binari Unreal;
5. esegui l'operazione o il test minimo che produce evidenza utile;
6. salva solo le modifiche intenzionali;
7. termina PIE/scenari;
8. chiudi l'Editor avviato da questo workflow;
9. verifica che il processo sia terminato.

Il passo 1 è una misura, non una presunzione: la superficie MCP cambia con la sessione e con lo stato del ponte.

⚠️ **Una risposta vuota non è un no.** Con l'Editor in play mode alcune query rispondono `[]` o `False`
**senza errore**, e un elenco vuoto sembra una misura. Prima di dichiarare una capability assente — e di
spostare il lavoro su una persona — ripeti la query fuori da PIE, con il ponte acceso. Se resta assente:
`NOT RUN` con il motivo.

Se un passo è eseguibile via MCP ma il suo oracolo resta umano, restano due cose distinte: MCP esegue, la
persona giudica. Averlo eseguito non promuove il verdetto — la ripartizione è in
[`docs/technical/tooling/scenario-map.md`](docs/technical/tooling/scenario-map.md), e non si riclassifica qui.

Il cleanup vale anche su errore.

Una sessione non deve lasciare un Editor da lei avviato disponibile indefinitamente: dopo l'uso lo rilascia chiudendolo, così la risorsa torna disponibile agli altri processi.

Non terminare un Editor preesistente posseduto da un altro workflow/persona salvo che il comando corrente dichiari esplicitamente ownership esclusiva (per esempio una skill dedicata che lo prevede).

⛔ Occupare il motore è una decisione di figura, non di comodità: mentre l'Editor è aperto nessuna suite gira, e viceversa — [`AGENTS.md`](AGENTS.md) §11.

## 6. Test

Entry point:

```powershell
./scripts/rt-suite.ps1
```

Per build, filtri, attesa e tool Node usa [`AGENTS.md`](AGENTS.md) §9.

Non duplicare qui la lista operativa.

### Regole

Se il motore è occupato:

usa il comportamento dello script.

Non inventare watcher paralleli.

Una suite che vede cambiare:

- `HEAD`;
- working tree;
- binario;
- processi Unreal;

durante la misura è:

**NON VALIDA**.

Non equivale a rosso, e non equivale a verde: è una misura da rifare.

Dopo un'attesa lunga:

**ricompila prima di registrare il verde.**

Se PIE/Editor/packaged non sono stati eseguiti:

**NOT RUN**.

Quando il cambiamento è verificabile in Editor e l'ambiente lo consente, usa Unreal/Epic MCP anche per PIE o scenario validation. Preferiscilo soprattutto per:

- selezione/input/camera e comportamento editor-facing;
- mappe, Blueprint e asset;
- interazioni visuali che richiedono il runtime Editor;
- regressioni che Automation Test da solo non dimostra.

PIE/MCP aggiunge evidenza, non sostituisce build, Automation Test o Scenario Harness richiesti.

Se non è possibile eseguirlo, dichiarare `NOT RUN` con il motivo.

Prima del merge:

verifica che il gate sia stato eseguito sul commit che stai realmente mergiando.

### Chi misura

Occupare il motore appartiene alla figura VALIDATION — [`AGENTS.md`](AGENTS.md) §11. Una sessione DEV prepara il test, esegue ciò che è statico o headless e marca il resto `NOT RUN`.

Una Validation Window aperta prima che la catena sia completa misura una base precedente: è utile, e non è il sign-off finale.

## 7. Lavoro parallelo

Il repository viene lavorato in parallelo. Figure, isolamento e coordinamento sono in [`AGENTS.md`](AGENTS.md) §11.

Non assumere che rimangano stabili:

- branch;
- `HEAD`;
- `origin/main`;
- issue;
- PR;
- binari;
- shared ID.

Prima di creare qualcosa:

**SEARCH → REUSE / UPDATE → CREATE solo per gap reale.**

Non assegnare dalla memoria:

- `D-nnn`;
- Epic `Enn`;
- altri contatori condivisi.

Fetch e riverifica prima del merge.

Nella stessa directory il working tree è condiviso: preferisci `git add` per path espliciti e non assumere che una modifica visibile in `git status` appartenga a questa sessione.

## 8. Git

Branch focalizzati:

```text
feat/
fix/
refactor/
docs/
test/
```

Usare Conventional Commits.

Non fare operazioni remote distruttive senza autorizzazione esplicita.

La PR va aperta sul **branch padre reale**, non su `main` per assunzione. Chiusura delle issue, forma di `Closes #N` e ID condivisi: [`AGENTS.md`](AGENTS.md) §12.

Non confondere:

```text
file modificato
```

con:

```text
build/test/PIE/packaged verificato
```

## 9. Output dopo ogni pass

Riporta:

### Risultato

Cosa è cambiato realmente.

### File

File creati/modificati.

### Decisioni

Owner, ADR o Decision Log coinvolti.

### Verifiche

Build, test e tool effettivamente eseguiti.

### NOT RUN

Verifiche non eseguite.

### Rischi / aperti

Conflitti, limiti, decisioni o follow-up.

### Prossimo passo

Una sola azione consigliata.

Non dichiarare:

- funziona;
- completo;
- production ready;
- sicuro;
- deterministico;

senza evidenza.

Quando la sessione consegna a un'altra figura, l'output di cui sopra non basta da solo: serve l'handoff persistito su file, nella forma di [`docs/rt-three-terminals/prompts/RT3_CONTRACT.md`](docs/rt-three-terminals/prompts/RT3_CONTRACT.md).

## 10. Mappa rapida

La tabella dei percorsi è in [`AGENTS.md`](AGENTS.md) §6.

Mappa dettagliata:

[`docs/technical/architecture/architettura-codice.md`](docs/technical/architecture/architettura-codice.md)

Prompt e contratto delle tre figure:

[`docs/rt-three-terminals/README.md`](docs/rt-three-terminals/README.md)

## 11. Routing rapido

Cosa fa una sessione Claude una volta misurata la propria figura.

### DEV

Codice, test, review, Git/GitHub, tooling statico e headless.

- non avviare UnrealEditor, UnrealEditor-Cmd, `rt-suite`, packaging o build che monopolizzano il motore;
- ciò che richiede Unreal si prepara e si marca `NOT RUN`, con il comando che VALIDATION dovrà eseguire;
- più istanze DEV condividono il working tree: §7.

Se la sessione è il `DEV-LEAD` di una wave, emette l'handoff di ingresso e non emette verdetti: non possiede lo strumento che li prova.

### EDITOR

Editor, PIE, MCP, `.uasset`/`.umap`, Blueprint, authoring, acceptance visuale.

- il ruolo EDITOR esiste in ogni workspace; **l'authoring asset via MCP solo da `MAIN`**, su un branch di task. Preflight: `rtmcp -Operation MCP_ASSET_WRITE -TaskId <id> -AssetWriteSet <path>`;
- lifecycle e cleanup obbligatori, anche su errore: §5;
- `MCP command sent` non è `verified`: serve un oracolo positivo — rilettura della property, riapertura dell'asset, compile esplicito, PIE, test, packaged;
- una risposta vuota non è né un `PASS` né una capability assente: §5;
- privacy, determinismo, autorità di rete, replay e performance restano osservazioni, non verdetti — [`AGENTS.md`](AGENTS.md) §11;
- niente suite o build concorrenti nella stessa finestra.

### VALIDATION

Build, Automation Test mirati, Scenario Harness, suite, packaged.

- il motore si prende just-in-time — `rtlease -Action acquire -Operation SUITE -TaskId <id>` — e si rilascia dopo;
- ordine: controlli statici → build → test mirati → scenari → suite completa una volta per batch integrato;
- validità della misura: §6;
- ricompila invece di fidarti di un binario che non hai prodotto;
- `performed = 0` non è un `PASS`;
- un difetto torna al suo owner: non riparare il codice di produzione e poi approvare sé stessi.
