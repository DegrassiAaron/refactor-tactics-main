# RT3 Control Plane

Coordinamento fra le sessioni RT3 aperte in terminali diversi, **senza copiare a mano**
TaskId, SessionId, branch, HEAD, esiti dei gate o note da una finestra all'altra.

Questo documento descrive il control plane. Non sostituisce
[`RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md), che possiede il contratto operativo dei
ruoli, né il task router (`scripts/rt-task-router.ps1`), che possiede il routing dei
**task** — chi deve lavorare adesso su un dato TaskId.

---

## 1. Che cos'è, e che cosa non è

```text
                    ┌────────────────────┐
                    │        rt3d        │
                    │  coordinator local │
                    │                    │
                    │  Sessions          │
                    │  Tasks             │
                    │  Events            │
                    │  Mailboxes         │
                    │  Routing           │
                    │  SQLite            │
                    └─────────┬──────────┘
                              │  HTTP su 127.0.0.1, porta effimera
              ┌───────────────┼───────────────┐
              ▼               ▼               ▼
           DEV-1        EDITOR-DEV      VALIDATOR-DEV
        (terminale)     (terminale)      (terminale)
```

Le sessioni **non si parlano fra loro**. Ogni terminale parla con `rt3d`, e `rt3d` è
l'unico processo che apre il database.

### Control plane

Sessioni, ruoli, lane, task, messaggi, eventi, routing, acknowledgement, metadati dei
candidate e dei lease.

### Data plane — resta fuori

Git, worktree, branch, HEAD, file, Unreal, build, asset.

Il control plane **memorizza riferimenti** al data plane (il branch e lo sha che una
sessione dichiara), ma non lo sostituisce e non lo muta: nessun comando di `rt3` esegue
un'operazione Git che cambi il repository.

### Quattro confini distinti

RT3 tiene separati quattro concetti che si somigliano e non sono la stessa cosa:

| Concetto | Valori | Chi lo possiede |
|---|---|---|
| **Ruolo sessione** | `DEV`, `EDITOR`, `VALIDATION` | dichiarato a `rt3 session start` |
| **Workspace group** | `MAIN`, `DEV`, `DESIGNER` | quale checkout ospita il terminale |
| **Lane** | `MAIN`, `DEV`, `DESIGNER` | su quale corsia viaggia il lavoro |
| **Task routing** | chi deve lavorare adesso | `scripts/rt-task-router.ps1` |

⛔ **Non assumere mai** `WorkspaceGroup == Lane`, né `WorkspaceGroup == Role`, né che la
directory chiamata `Main` sia sul branch `main`. Un `EDITOR` aperto nel checkout MAIN può
lavorare sulla lane DEV, ed è lo scenario normale dell'integrazione.

---

## 2. Avviare `rt3d`

```powershell
scripts\rt3.ps1 daemon start
```

Il daemon parte **staccato** dal terminale che lo lancia: chiudere quella finestra non lo
ferma, altrimenti il primo dei tre VS Code che si chiude porterebbe via il coordinator
degli altri due.

Ascolta su `127.0.0.1` e su una **porta effimera**, che pubblica in `daemon.json`. Una
porta fissa sarebbe più comoda da documentare e sbagliata da usare: su questa workstation
sono già in ascolto tre VS Code, un Unreal e il ponte MCP.

```powershell
scripts\rt3.ps1 daemon status     # è vivo? su quale porta? con quali versioni?
scripts\rt3.ps1 daemon stop
scripts\rt3.ps1 daemon restart
scripts\rt3.ps1 daemon run        # in primo piano, per vedere un errore d'avvio
```

Ne basta **uno per macchina**. `daemon start` lanciato una seconda volta trova quello già
attivo e lo dice, invece di aprirne un secondo.

---

## 3. Registrare una sessione

Ogni terminale dichiara **esplicitamente** chi è. Il nome della finestra VS Code, la
directory e il branch non determinano ruolo né lane — è la regola di `AGENTS.md` §11.

```powershell
scripts\rt3.ps1 session start `
    --id DEV-1 `
    --role DEV `
    --lane DEV `
    --workspace-group DEV `
    --task 2272
```

Branch, HEAD, worktree e repository vengono letti da **Git**, non dedotti:

```text
sessione DEV-1 registrata.
  role/lane      : DEV / DEV
  workspaceGroup : DEV
  worktree       : D:\Repositories\refactor-tactict-dev
  repoRoot       : D:\Repositories\refactor-tactict-dev
  branch @ HEAD  : feat/rt3-task-router @ bbf9a3d1
  task           : 2272
  writeMode      : READ_ONLY
  binding        : C:\Users\...\RT3\bindings\console-33484.json
```

Un **detached HEAD** è uno stato legittimo: `branch` resta vuoto e la CLI stampa
`(detached)`. Una directory che non è un repository Git è altrettanto legittima: i quattro
campi restano vuoti e la registrazione riesce comunque.

⚠️ **I metadati vengono dalla directory corrente, non da dove sta lo script.** È giusto
così — un terminale RT3 sta dentro il proprio workspace — ma rende possibile un errore
silenzioso: invocare `D:\...\refactor-tactics-main\scripts\rt3.ps1` mentre la shell sta in
un altro checkout registra il branch e lo sha di *quel* checkout sotto il nome dell'altro.
È successo durante la verifica incrociata dei tre workspace: tre sessioni registrate da
tre script diversi hanno riportato tutte lo stesso branch, perché la shell non si era mai
spostata. Nessun comando era sbagliato, e il referto lo era. `rt3.ps1` ora avvisa quando la
cwd è fuori dal proprio repository; `--worktree <path>` indica esplicitamente un'altra
directory.

### Quale sessione rappresenta questo terminale

La CLI risolve l'identità in questo ordine:

1. `--session <id>` — argomento esplicito;
2. `RT3_SESSION_ID` — variabile d'ambiente;
3. **binding per console** — file scritto da `session start`, indicizzato dal PID della
   shell che ha invocato il comando;
4. errore `RT3_SESSION_UNBOUND`, con l'istruzione per uscirne.

Il livello 3 esiste perché un processo figlio non può scrivere una variabile d'ambiente
nel padre: `rt3 session start` non può esportare `RT3_SESSION_ID` nella shell che lo ha
lanciato, ma può scrivere un file legato al PID di quella shell. Vive quanto il terminale,
ed è diverso per ogni terminale — anche per **due terminali nella stessa directory**, che
è precisamente ciò che la current working directory non distingue.

⚠️ I PID vengono riusati dal sistema operativo. Una shell nuova che ricevesse il PID di
una vecchia erediterebbe il suo binding. Il caso è raro (`session stop` rimuove il file) e
non è silenzioso: `rt3 status` stampa sempre la sessione risolta **e da dove viene**. La
via d'uscita è `RT3_SESSION_ID`, che ha precedenza.

```powershell
scripts\rt3.ps1 session status
scripts\rt3.ps1 session set --task 2272 --refresh-git
scripts\rt3.ps1 session stop
scripts\rt3.ps1 sessions list
```

---

## 4. Pubblicare un evento

```powershell
scripts\rt3.ps1 event publish --type TASK_READY --with-git --note "risolutore pronto"
```

`--with-git` aggiunge branch, HEAD e worktree al payload: sono i tre dati che altrimenti
si ricopiano a mano da un terminale all'altro.

Destinatario esplicito, quando serve scavalcare il routing automatico:

```powershell
scripts\rt3.ps1 event publish --type QUESTION --to-session DEV-1 --note "quale sha apro?"
scripts\rt3.ps1 event publish --type ANSWER --to-role EDITOR --to-lane DESIGNER
```

Tipi supportati: `SESSION_STARTED`, `SESSION_STOPPED`, `TASK_STARTED`, `TASK_BLOCKED`,
`TASK_READY`, `QUESTION`, `ANSWER`, `REVIEW_REQUESTED`, `REVIEW_APPROVED`,
`REVIEW_REJECTED`, `CANDIDATE_CREATED`, `VALIDATION_REQUESTED`, `VALIDATION_PASSED`,
`VALIDATION_FAILED`, `INTEGRATION_REQUESTED`, `INTEGRATION_ACCEPTED`,
`INTEGRATION_REJECTED`, `LEASE_REQUESTED`, `LEASE_GRANTED`, `LEASE_RELEASED`,
`UNREAL_LEASE_REQUESTED`, `UNREAL_LEASE_GRANTED`, `UNREAL_LEASE_RELEASED`.

⚠️ Il control plane **valida, salva, instrada e mostra** questi eventi. Non implementa la
logica di business di ciascuno: `LEASE_GRANTED` non concede alcun lease, lo annuncia. Il
lease del motore resta di `rt-lease.ps1`.

---

## 5. Leggere la mailbox

```powershell
scripts\rt3.ps1 inbox list                 # solo i pendenti per me
scripts\rt3.ps1 inbox list --all           # anche quelli già presi
scripts\rt3.ps1 inbox show ev_3de9fc4aaf48
scripts\rt3.ps1 inbox ack  ev_3de9fc4aaf48 --note "preso in carico"
```

`show` e `ack` accettano indifferentemente l'**event-id** o il **delivery-id**: chiedere di
ricopiare un secondo identificatore sarebbe esattamente il travaso che il control plane
esiste per togliere.

### Semantica delle consegne

Una consegna è indirizzata **o** a una sessione **o** a una coppia (ruolo, lane) — mai a
entrambi, e il database lo impedisce con un `CHECK`.

- **A una sessione** (`--to-session`): la prende quella e nessun'altra.
- **A un ruolo su una lane** (routing automatico, o `--to-role`): la prende la **prima
  sessione compatibile che fa ack**.

Una sessione vede solo le consegne indirizzate a lei per nome, oppure al **suo ruolo sulla
sua lane**. Un `EDITOR` della lane `DESIGNER` non vede le consegne dirette a
`(EDITOR, DEV)`: non può nemmeno tentare l'ack, e riceve `RT3_NOT_AUTHORIZED` se ci prova.

Fra sessioni **compatibili** — due `EDITOR` sulla stessa lane — la consegna è invece
condivisa e vince chi fa ack per primo. È voluto: la consegna è indirizzata a un *ruolo*,
non a un *processo*, e un secondo EDITOR sulla stessa lane è per definizione un sostituto
legittimo. Il perdente riceve `RT3_DELIVERY_CONFLICT` **con il nome di chi ha preso il
messaggio**, non un silenzio.

L'ack è atomico a livello di database:

```sql
UPDATE deliveries SET state='ACKED', ... WHERE delivery_id=? AND state='PENDING'
```

Nessun controllo-poi-scrivi, nessuna finestra fra la lettura e la scrittura.

### Chi fa ack adotta il task

Prendere in carico un evento significa prendere in carico il suo **task**. Un `EDITOR` che
raccoglie un `TASK_READY` del task `2272` eredita quel task, e da lì in poi i suoi eventi
ci si agganciano da soli, senza ripassare `--task 2272` a mano.

⚠️ L'adozione avviene **solo** se la sessione non ha già un task proprio: spostare una
sessione su un lavoro che non ha scelto sarebbe peggio del problema che risolve.

---

## 6. Routing

| Mittente | Evento | Destinatario | Regola |
|---|---|---|---|
| `DEV` lane *X* | `TASK_READY` | `EDITOR` lane *X* | `DEV_TASK_READY_TO_EDITOR_SAME_LANE` |
| `EDITOR` lane *X* | `VALIDATION_REQUESTED` | `VALIDATION` lane *X* | `EDITOR_VALIDATION_REQUESTED_TO_VALIDATION_SAME_LANE` |
| `VALIDATION` lane *X* | `VALIDATION_PASSED` / `VALIDATION_FAILED` | `EDITOR` lane *X* | `VALIDATION_VERDICT_TO_EDITOR_SAME_LANE` |
| `EDITOR` lane `DEV` o `DESIGNER` | `INTEGRATION_REQUESTED` | `EDITOR` lane `MAIN` | `EDITOR_INTEGRATION_REQUESTED_TO_EDITOR_MAIN` |

Precedenza: `--to-session` › `--to-role` › regola automatica › **nessun destinatario**.

Un destinatario esplicito **sovrascrive** la regola automatica: la regola copre il caso
normale, e chi nomina un destinatario sta dicendo che questo caso non è normale.

Due casi deliberati:

- **`QUESTION` e `ANSWER` esigono un destinatario esplicito.** Non esiste una regola che
  indovini l'interlocutore di una conversazione, e registrare un `QUESTION` che nessuno
  leggerà sarebbe il modo peggiore di fallire.
- **`INTEGRATION_REQUESTED` da un `EDITOR` già sulla lane `MAIN` non ha destinatario.**
  Instradarlo a `(EDITOR, MAIN)` creerebbe una consegna che il mittente stesso può
  raccogliere: un promemoria travestito da handoff.

Quando nessuna regola fa match, l'evento è **registrato nell'event log e non consegnato**,
e la CLI lo dice a voce alta:

```text
  ! NESSUN DESTINATARIO: evento registrato, non consegnato.
    Nessuna regola automatica per REVIEW_APPROVED da EDITOR sulla lane DEV.
```

⛔ Il routing instrada un messaggio, **non giudica il lavoro**. La catena canonica
`DEV-LEAD → EDITOR → VALIDATION` resta una regola di contratto
([`RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md)): nulla qui impedisce di pubblicare un
evento fuori sequenza, il control plane lo registra e lo mostra.

---

## 7. Il database: dove sta e perché

```text
%LOCALAPPDATA%\RefactorTactics\RT3\
    runtime.db          SQLite: sessions, tasks, events, deliveries, candidates, leases
    daemon.json         endpoint pubblicato da rt3d: host, porta, pid, versioni
    bindings\           un file per terminale legato
    rt3d.log            stdout/stderr del daemon staccato
```

🔴 **È per macchina, non per repository, e questa è la decisione più importante del
sistema.**

La specifica proponeva `.rt3/runtime.db` dentro il checkout. Non funziona qui: i tre
workspace permanenti sono **tre cloni distinti** dello stesso remote, ciascuno col proprio
`.git`. Un database dentro il checkout ne produrrebbe tre, e tre control plane che non si
vedono sono l'esatto contrario di ciò che serve — DEV pubblicherebbe `TASK_READY` nel
proprio database e l'EDITOR aprirebbe il proprio, vuoto, concludendo che non c'è lavoro.

È la stessa radice già usata da `rt-workspace.ps1` (registro dei workspace) e
`rt-task-router.ps1` (task), e non è un caso: sono tutti stato per macchina, per la stessa
ragione.

`RT3_HOME` sovrascrive la radice. Serve ai test e alla diagnosi; in esercizio non va
passata, e puntarla dentro un checkout ricrea il difetto qui sopra. Il `.gitignore` porta
comunque `.rt3/` come rete di sicurezza.

⛔ **Nulla di tutto questo va versionato o propagato fra i workspace**: `runtime.db`, il
pid, la porta, i binding di sessione e i log sono lo stato di *una* macchina in *un*
momento. Si condivide il **software** RT3, non il suo stato runtime.

---

## 8. Recovery dopo un riavvio

La persistenza sopravvive alla chiusura del terminale, al riavvio della CLI e al riavvio
di `rt3d`. Gli eventi restano; le mailbox restano; gli ack restano.

Un evento indirizzato a un **ruolo su una lane** è recuperabile anche se al momento della
pubblicazione **nessuna sessione di quel ruolo esisteva**. È lo scenario normale: DEV
dichiara pronto il lavoro prima che l'EDITOR apra il proprio terminale.

Casi previsti e loro risposta:

| Situazione | Cosa succede |
|---|---|
| `rt3d` non avviato | `RT3_DAEMON_UNAVAILABLE`, con l'istruzione per avviarlo |
| `daemon.json` stantio dopo un crash | `daemon status` lo segnala; `daemon start` lo rimuove e riparte |
| SessionId duplicato e ancora attivo | `RT3_SESSION_EXISTS`; si riusa con `--replace` o dopo `session stop` |
| Sessione morta senza cleanup | `session start --replace`, oppure `session stop --session <id>` |
| Evento invalido | `RT3_INVALID_EVENT`, con l'elenco dei valori ammessi |
| Destinatario nominato inesistente | rifiutato subito: resterebbe pending per sempre |
| Detached HEAD | `branch` vuoto, registrazione riuscita |
| Nessun repository Git | i quattro campi restano vuoti, registrazione riuscita |
| Database corrotto | `RT3_STORE_UNAVAILABLE`; fermare `rt3d`, spostare il file, riavviare |

Gli errori escono come `CODICE: messaggio` con exit code dedicato. **Un traceback compare
solo per un imprevisto vero**, che è esattamente l'informazione che serve quando compare.

---

## 9. Versioni, e i tre workspace allineati

**Tre** versioni esplicite, che **non** sono la stessa cosa:

- **`PROTOCOL_VERSION`** — il contratto client ↔ daemon. Si alza quando un client vecchio
  non può più essere servito correttamente da un daemon nuovo, o viceversa.
- **`SCHEMA_VERSION`** — la forma del database. Si alza a ogni migrazione.
- **`ROADMAP_SCHEMA_VERSION`** — la forma del documento roadmap versionato. Si alza quando
  un file scritto per una versione precedente non si legge più allo stesso modo.

Separarle conta: una migrazione che aggiunge una colonna con default non rompe nessun
client, e alzare il protocollo in quel caso costringerebbe tre workspace ad aggiornare per
niente. Allo stesso modo una roadmap non diventa illeggibile perché il database ha una
tabella in più.

⚠️ Il numero identico che portano oggi è una coincidenza di calendario, non un vincolo: si
muovono per ragioni diverse e divergeranno. Non scrivere codice che assuma
`PROTOCOL_VERSION == SCHEMA_VERSION`.

Un checkout non aggiornato si riconosce dalla **terza** versione mancante:

```text
WORKSPACE BRANCH                            HEAD      PROT SCHE ROAD
MAIN      feat/rt3-roadmap-orchestration    77233ea3     2    2    1
DEV       feat/rt3-task-router              0cdebf34     1    1    -     <-- da aggiornare
```

```powershell
scripts\rt3.ps1 version
scripts\rt3.ps1 status     # mostra client e daemon affiancati
```

Il **fail-fast** scatta in tre punti:

1. il client legge `protocolVersion` da `daemon.json` prima di connettersi;
2. il daemon confronta l'header `X-RT3-Protocol` di ogni richiesta **prima di leggerne il
   corpo** — interpretare un corpo di forma ignota sarebbe la degradazione silenziosa che
   la versione esiste per impedire;
3. all'avvio, `rt3d` rifiuta un database con schema **più nuovo** del proprio: quel daemon
   è più vecchio di chi ha scritto il database, e va aggiornato il checkout, non degradato
   il database.

Il messaggio dice sempre quale delle due parti aggiornare:

```text
RT3_PROTOCOL_MISMATCH: rt3d in esecuzione parla protocollo RT3 v1, questo client parla v2.
I tre workspace devono eseguire la stessa versione del control plane: allineare i
checkout, poi `rt3 daemon restart`.
```

⚠️ Il pacchetto viene eseguito **dal checkout da cui si lancia il comando** (`rt3.ps1` mette
`tools/rt3` sul `PYTHONPATH`, non installa nulla). È deliberato: è proprio la proprietà che
fa emergere un disallineamento invece di nasconderlo.

---

## 10. Usarlo da più terminali

I tre workspace permanenti sono cloni distinti che parlano con **un solo** `rt3d`:

```text
D:\Repositories\refactor-tactics-main                                    lane MAIN
D:\Repositories\refactor-tactict-dev                                     lane DEV
D:\Repositories\refactor-tactics-technical-designer\refactor-tactics-main lane DESIGNER
```

Ogni terminale, dalla propria directory:

```powershell
scripts\rt3.ps1 daemon start        # solo il primo lo avvia davvero
scripts\rt3.ps1 session start --id EDITOR-MAIN --role EDITOR --lane MAIN --workspace-group MAIN
scripts\rt3.ps1 status
```

Da quel momento `sessions list` mostra le sessioni di **tutti e tre**, e un evento
pubblicato in uno compare nella mailbox dell'altro senza che nessuno ricopi nulla.

---

## 11. Verificare l'installazione

```powershell
scripts\rt3.ps1 -SelfTest              # i test automatici del pacchetto
python tools\rt3\smoke.py              # smoke end-to-end su RT3_HOME temporanea
python tools\rt3\smoke_multi.py        # smoke sui TRE checkout reali
```

Lo smoke avvia un `rt3d` vero e invoca la CLI vera con tre `RT3_SESSION_ID` diversi — cioè
ciò che accade con tre finestre aperte. Gira su una `RT3_HOME` usa e getta: toccare il
control plane reale lo renderebbe non ripetibile e lascerebbe sessioni finte nell'elenco
che le sessioni vere consultano.

`smoke_multi.py` fa un passo in più: i tre terminali stanno in tre **cloni distinti**, e
ognuno invoca il `rt3` del proprio checkout. È la sola prova che il control plane resti
uno mentre i checkout sono tre — proprietà che nessun test unitario può dare, perché in un
processo «un solo control plane» è vero per costruzione.

---

## 12. Roadmap Orchestration

Il control plane sa **chi sta lavorando**; la roadmap gli dice **cosa c'è da fare**.

### Quattro piani distinti

```text
PLAN        cosa c'è da fare e cosa dipende da cosa   roadmap.yaml, versionato
SCHEDULE    cosa è eseguibile adesso                  derivato, mai salvato
ASSIGNMENT  chi lo prende                             derivato, mai salvato
RUNTIME     cosa è stato fatto davvero                database per macchina
```

🔴 **Nel file versionato non c'è stato.** `state`, `progress`, `assignee` e `candidate`
sono chiavi **rifiutate** dal parser. Se lo stato vivesse nel file, avanzare una issue
sarebbe un commit, due sessioni che avanzano due issue sarebbero un conflitto di merge, e
il control plane diventerebbe una seconda autorità accanto a Git.

Per la stessa ragione **`READY` e `BLOCKED` non sono stati persistiti**: sono derivati dal
grafo più l'avanzamento, e li calcola il planner a ogni richiesta. Gli unici stati salvati
sono i quattro che nessuno può dedurre — `PENDING`, `IN_PROGRESS`, `VALIDATED`, `DONE`.

### La modalità: dove una issue viene lavorata

Accanto all'avanzamento il RUNTIME registra **una** cosa in più, e serve a chiudere un
difetto reale: `mode` ∈ `PERMANENT_WRITER | TEMPORARY_WORKTREE`.

🔴 Il piano è derivato e non viene salvato. Prima che questo campo esistesse, il planner
suggeriva `TEMPORARY_WORKTREE_SUGGESTED`, qualcuno eseguiva il suggerimento, e al giro
successivo il planner rileggeva soltanto `IN_PROGRESS` — contando quell'item come writer
**permanente**. Con `writerCapacity: 1` si arrivava a `used 2 / capacity 1`: il vincolo
sforato eseguendo il piano che quel vincolo doveva rispettare, e nessuno che lo dicesse.

```powershell
scripts\rt3.ps1 roadmap state set EPIC-B/B4 --progress IN_PROGRESS --mode TEMPORARY_WORKTREE
```

⚠️ **Omettere `--mode` significa `PERMANENT_WRITER`**, che è la lettura conservativa:
occupa la risorsa più scarsa. Chi lavora su un worktree temporaneo **deve** dichiararlo,
altrimenti il piano conta un writer permanente che non è occupato.

⚠️ `mode` **non** è conservata quando non viene ridichiarata: appartiene alla presa in
carico corrente. Tenerla in vita quando una issue esce da `IN_PROGRESS` e ci rientra
altrove significherebbe contare la risorsa sbagliata.

`SUGGESTED` non esiste fra i valori salvabili: il planner **propone**, il RUNTIME registra
ciò che è stato **fatto**. Conflaterli renderebbe indistinguibile un consiglio da un fatto.

### `overCommitted`: lo stato impossibile si dichiara

`roadmap plan` porta sempre il campo `overCommitted`, anche vuoto. Elenca le risorse il cui
uso **supera** la capacità dichiarata — cosa che il planner non produce mai da sé, ma che
accade quando due sessioni si dichiarano `IN_PROGRESS` sullo stesso gruppo senza passare di
qui.

⛔ Va dichiarato, non corretto in silenzio: un piano che nasconde uno stato impossibile è
peggio di uno che lo espone, perché chi legge crede che il vincolo regga.

### Comandi

```powershell
scripts\rt3.ps1 roadmap validate --file docs\rt-three-terminals\roadmaps\rt3-smoke-multi-epic.yaml
scripts\rt3.ps1 roadmap load     --file <path>     # valida e carica nel control plane
scripts\rt3.ps1 roadmap graph                      # dipendenze, con i gate sugli archi
scripts\rt3.ps1 roadmap critical-path              # cammino critico e slack
scripts\rt3.ps1 roadmap ready                      # READY/BLOCKED, e chi si aspetta
scripts\rt3.ps1 roadmap plan                       # assegnazioni, rimandi, capacità
scripts\rt3.ps1 roadmap state set EPIC-B/B2 --progress VALIDATED --candidate cand_...
```

`roadmap validate` è l'unico che **non richiede `rt3d`**: è la cosa che si vuole poter fare
mentre si scrive il file, prima che esista un control plane a cui darlo.

### Capacità, e la differenza fra BLOCKED e rimandato

Una issue `READY` può non essere pianificata. Non è la stessa cosa di `BLOCKED`, e la
correzione è diversa:

| Esito | Significa | Si corregge |
|---|---|---|
| `BLOCKED` | una dipendenza non ha passato il gate | validando la dipendenza |
| rimandato `WRITER_CAPACITY` | il writer permanente del workspace è occupato | liberandolo, o alzando la capacità |
| rimandato `TEMPORARY_WORKTREE_CAPACITY` | anche i worktree temporanei sono finiti | chiudendone uno |
| rimandato `UNREAL_LEASE_CAPACITY` | l'Editor è già impegnato | rilasciando il lease |
| rimandato `WIP_PER_WORKSPACE` / `WIP_GLOBAL` | limite di lavoro in corso | chiudendo qualcosa |

⛔ Il planner **suggerisce** il worktree temporaneo, non lo crea: la creazione è data plane,
e questo pacchetto non tocca il data plane.

### Il parser YAML

`tools/rt3/rt3/yamlmini.py` legge **un sottoinsieme** di YAML e rifiuta tutto il resto —
anchor, alias, tag, block scalar, documenti multipli, chiavi duplicate — nominando la riga.
Non è una limitazione da aggirare: un'anchor ignorata in silenzio produrrebbe una roadmap
valida e con dipendenze mancanti, che nessun gate vedrebbe.

È scritto in casa perché il pacchetto non ha dipendenze (§*Vincoli* di
[`tools/rt3/README.md`](../../tools/rt3/README.md)). Dove PyYAML è installato, i test
confrontano i due risultati sugli stessi documenti — inclusa la roadmap vera.

### Determinismo del piano

Due workspace che chiedono un piano sullo stesso stato ottengono lo **stesso** piano.
L'ordine è totale e non ha tie-break casuali:

1. **slack crescente** — il cammino critico per primo;
2. **earliest start crescente** — ciò che può partire prima;
3. **ordine di dichiarazione** — l'unica priorità che qualcuno ha scritto.

---

## 13. Propagare una modifica fra i workspace

Il control plane coordina tre workspace; prima o poi una modifica al control plane stesso
deve raggiungerli. La strada si sceglie **dai fatti**, e i fatti sono uno solo:

```bash
git -C <dir> rev-parse --git-common-dir     # uguale = stesso repo; diverso = cloni
```

| Caso misurato | Strategia | Perché |
|---|---|---|
| `SAME_REPO` — worktree dello stesso repository | **nessun trasporto** | gli oggetti sono già condivisi: il commit è visibile subito. Integrare è un merge o un cherry-pick **locale** |
| `DISTINCT_CLONES` — cloni distinti | `fetch` da **path locale** + materializzazione | il trasporto resta su disco. GitHub serve all'**integrazione** (PR, review, main), non al trasporto |
| `NOT_A_REPO` — Git non utilizzabile | copia controllata, **ultima risorsa** | va autorizzata esplicitamente, e dichiara i file trasferiti |

```powershell
python tools\rt3\distribute.py --to <path> --commit <sha> --paths tools/rt3 --dry-run
```

⚠️ **Il nome della directory non decide.** Su questa macchina
`refactor-tactics-technical-designer` *contiene* un clone senza esserlo: misurato dà
`NOT_A_REPO`, mentre il `refactor-tactics-main` annidato dentro dà `DISTINCT_CLONES`.

⛔ **La copia non parte se il bersaglio è un repository**, nemmeno con `--allow-copy`.
Senza questo rifiuto «copia come fallback» passerebbe sempre, e nessun test potrebbe
distinguere l'ultima risorsa dall'abitudine.

Lo strumento **non integra**: niente merge, niente checkout, `HEAD` e indice non si
muovono. In un working tree condiviso spostare `HEAD` significa spostarlo sotto un'altra
sessione, e l'integrazione appartiene a chi possiede il ramo.

⚠️ Materializza con `git archive`, non con `git checkout <sha> -- <path>`: il secondo
scrive nell'**indice** del bersaglio, e un `git commit` di un'altra sessione assorbirebbe
quei file nel proprio commit.

---

## 14. Limiti della v1

Reali, misurati, non ipotetici:

- **Il `WriteMode` non è imposto.** La colonna esiste, `1 worktree = 1 WRITER` non è
  verificato da nessuno. Il modello dati è pronto; l'enforcement è della milestone
  successiva.
- **I lease sono metadati.** La tabella `leases` e gli eventi `*_LEASE_*` annunciano, non
  concedono. Il lease del motore resta di `rt-lease.ps1`, e `LEASE_GRANTED` pubblicato qui
  non dà accesso a Unreal.
- **Il write-set non è verificato.** Una sessione può dichiarare qualunque cosa; nulla
  confronta la dichiarazione con ciò che tocca davvero.
- **Nessuna autenticazione.** È localhost single-user. Il bind è su `127.0.0.1` e non su
  `0.0.0.0`, che è la differenza fra "senza autenticazione sulla mia macchina" e "senza
  autenticazione sulla rete". Il ruolo dichiarato **non è un confine di sicurezza**:
  impedisce l'errore, non l'abuso — come il registro dei workspace e come il lease.
- **Nessun heartbeat.** `LastSeenAt` si aggiorna quando la sessione fa qualcosa. Una
  sessione il cui terminale è stato chiuso resta `ACTIVE` finché qualcuno non la ferma o
  ne riusa l'id con `--replace`.
- **Nessuna notifica push.** Una sessione scopre di avere posta chiamando `inbox list` o
  `status`. Non c'è nulla che avvisi il terminale.
- **Il binding per console si appoggia al PID della shell** (§3), coi limiti detti lì.
- **Il control plane non tocca Git.** Niente creazione di worktree, merge, rebase,
  enforcement del write-set, automazione Unreal o MCP: sono esplicitamente fuori dalla
  milestone.
- **Nessuna TUI, nessuna dashboard, nessun server remoto.**

Della Roadmap Orchestration in particolare:

- **Il worktree temporaneo è un suggerimento.** Il planner dice che serve; nessuno lo crea.
  Chi lo usa lo **dichiara** con `--mode TEMPORARY_WORKTREE`: il control plane registra il
  fatto, non lo verifica. Nessuno controlla che quel worktree esista davvero.
- **L'assignment non nomina una sessione.** Dice *quale workspace* e *con che modalità*, non
  *quale SessionId*: legare un item a una sessione richiederebbe di sapere quando quella
  sessione muore, e non c'è heartbeat.
- **Il gate `requires` non è verificato contro la realtà.** `VALIDATED` significa «qualcuno
  ha dichiarato VALIDATED», non «la suite è passata». Il legame con l'evidenza passa dal
  `candidate`, ed è una convenzione, non un vincolo.
- **Le capacità sono dichiarate nella roadmap, non misurate.** `writerCapacity: 1` non
  impedisce a due sessioni di scrivere nello stesso checkout: lo dice il piano, non il
  filesystem. Stesso limite del `WriteMode` qui sopra.
- **`estimate` è adimensionale.** Il cammino critico è in unità di stima, non in ore.

---

## Vedi anche

- [`README.md`](README.md) — i tre terminali RT3
- [`RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md) — contratto operativo dei ruoli
- `scripts/rt-task-router.ps1` — routing dei task: quale ruolo deve lavorare adesso
