# RT3 — Contratto di wave

Contratto condiviso dai prompt di wave `WAVE_EDITOR.md` e `WAVE_VALIDATION.md`.

Owner di questo documento: se una regola qui contraddice `CLAUDE.md` o `AGENTS.md`, vince il documento di repository. Questo file non è autorità su lifecycle Editor, suite o Git: li richiama.

## 1. Due livelli, due documenti

`rt-three-terminals` ha due livelli distinti. Non confonderli.

| Livello | Domanda | Documenti |
|---|---|---|
| Ruolo | Cosa può occupare questo terminale, e con chi confligge | `TERMINAL_DEV.md` · `TERMINAL_VALIDATION.md` · `TERMINAL_EDITOR.md` |
| Wave | Come si esegue e si consegna un lavoro attraverso i ruoli | `RT3_CONTRACT.md` · `WAVE_EDITOR.md` · `WAVE_VALIDATION.md` |

Un prompt di wave presuppone il prompt di ruolo. Non lo sostituisce.

Un prompt di wave si incolla **da solo**. Non incollare due ruoli nella stessa sessione.

## 2. Attori

| Attore | Definizione | Prompt di ruolo |
|---|---|---|
| DEV | Istanza che scrive codice/test senza occupare Unreal. Possono essere N. | `TERMINAL_DEV.md` |
| DEV-LEAD | La singola istanza DEV che, per una wave, possiede l'integrazione: consolida il lavoro dei DEV ed emette l'handoff RT3 di ingresso. È un ruolo di wave, non un quarto ruolo di terminale: le regole di concorrenza restano quelle DEV. | `TERMINAL_DEV.md` |
| EDITOR | Unica istanza che scrive `.uasset`/`.umap` per la wave. | `TERMINAL_EDITOR.md` |
| VALIDATION | Verificatore indipendente. Non ripara, non possiede binari. | `TERMINAL_VALIDATION.md` |

Se per una wave DEV-LEAD non è designato, la wave non ha ingresso. Vedi §4.

## 3. Principi

Tre non-equivalenze. Valgono su ogni sezione di ogni wave.

```text
MCP command sent   != verified
animation success  != simulator correctness
file modificato    != build/test/PIE/packaged verificato
```

Da cui:

```text
performed = 0      != PASS
risposta MCP vuota != capability assente
assenza in UI      != assenza sul client
```

## 4. Preflight — fail-closed

Prima dello step 1, prima di leggere il repository, prima di aprire l'Editor.

Campi obbligatori:

```text
FEATURE
BRANCH
BASE_SHA
INPUT_HANDOFF   path a un file esistente, non testo incollato
```

Se uno qualsiasi è vuoto, è un placeholder non risolto (`[FEATURE]`, `<...>`, `TBD`) o non è risolvibile:

```text
STATUS: BLOCKED
REASON: MISSING_INPUT
FIELDS: <elenco dei campi mancanti>
```

Poi fermati.

Non ispezionare il repository. Non avviare Unreal. Non dedurre il valore mancante dal contesto, dalla cronologia o dal working tree.

Un placeholder risolto per inferenza è un input inventato.

## 5. Precondizioni del repository

Misura, non presumere:

```powershell
git status --short
git rev-parse HEAD
git rev-parse --abbrev-ref HEAD
```

`BLOCKED` se:

- `HEAD` non corrisponde a `BASE_SHA`;
- `HEAD` è detached e `BASE_SHA` non lo prevede esplicitamente;
- il working tree contiene modifiche non dichiarate nel write-set in ingresso;
- `HEAD`, working tree, binari o processi Unreal cambiano durante una finestra di misura — in quel caso la misura è `NON VALIDA`, non `FAIL`.

L'ultimo caso è già normato in `CLAUDE.md` §6. Qui non viene riscritto: viene applicato.

Il working tree è condiviso tra istanze. Una modifica visibile in `git status` non appartiene necessariamente a questa wave: vedi la regola Git per più DEV in `../README.md`.

## 6. Verdetti tipizzati

Un verdetto non è una parola. È un record con campi obbligatori.

| Verdetto | Campi richiesti | Significato |
|---|---|---|
| `PASS` | `EVIDENCE_REF` | Verificato. L'evidenza è rileggibile da terzi. |
| `FAIL` | `EVIDENCE_REF` | Verificato negativo. |
| `BLOCKED` | `REASON`, `UNBLOCK` | Non verificabile ora. `UNBLOCK` dice cosa lo sbloccherebbe. |
| `N/A` | `REASON` | Fuori dal write-set della wave. |
| `OBSERVED` | `EVIDENCE_REF` | Osservazione registrata, **non** un verdetto. Non conta come `PASS`. |
| `NOT RUN` | `REASON` | Non eseguito. Non conta come `PASS`. |

Regole di forma:

- un `PASS` senza `EVIDENCE_REF` è malformato e si legge `BLOCKED`;
- un `N/A` senza `REASON` è malformato e si legge `BLOCKED`;
- `N/A` è giustificato dal write-set, mai dalla difficoltà o dal tempo;
- `OBSERVED` esiste perché alcuni ruoli possono guardare un sistema senza poterlo provare. Vedi §7.

⚠️ **Queste regole valgono per le voci di verdetto.** Un handoff che per §9 non porta payload non ha voci: la loro assenza non è una voce malformata, e non si legge `BLOCKED`.

`EVIDENCE_REF` è un riferimento ad artefatto, non prosa:

```text
log:     Saved/Logs/<file>.log#L<riga>
suite:   <comando> -> exit <n>, found <n>, performed <n>, passed <n>, failed <n>
turnlog: <path dump>
asset:   <path>@<sha7>
shot:    docs/rt-three-terminals/waves/<feature>/evidence/<file>.png
```

Una frase descrittiva non è un `EVIDENCE_REF`.

## 7. Matrice canonica

Una sola tabella per entrambi i ruoli. EDITOR e VALIDATION compilano la stessa lista, ciascuno la propria colonna.

`Verdetto max` limita il verdetto più forte che quel ruolo può emettere per quel sistema.

| # | Sistema | EDITOR max | VALIDATION max |
|---:|---|---|---|
| 1 | PROJECT | `PASS` | `PASS` |
| 2 | ARCHITECTURE | `OBSERVED` | `PASS` |
| 3 | BUILD | `OBSERVED` | `PASS` |
| 4 | ASSETS | `PASS` | `PASS` |
| 5 | BLUEPRINT | `PASS` | `PASS` |
| 6 | DATA | `PASS` | `PASS` |
| 7 | DATA VALIDATORS | `PASS` | `PASS` |
| 8 | MAP | `PASS` | `OBSERVED` |
| 9 | GRID/GRAPH | `PASS` | `PASS` |
| 10 | INPUT | `PASS` | `OBSERVED` |
| 11 | CAMERA | `OBSERVED` | `OBSERVED` |
| 12 | PLANNING | `PASS` | `PASS` |
| 13 | READY/COMMIT | `PASS` | `PASS` |
| 14 | SNAPSHOT | `PASS` | `PASS` |
| 15 | MOVEMENT | `PASS` | `PASS` |
| 16 | TARGETING | `PASS` | `PASS` |
| 17 | LOS/COVER | `PASS` | `PASS` |
| 18 | DAMAGE | `PASS` | `PASS` |
| 19 | STATUS/CONTROL | `PASS` | `PASS` |
| 20 | DISPLACEMENT | `PASS` | `PASS` |
| 21 | REACTIONS | `PASS` | `PASS` |
| 22 | ENVIRONMENT | `PASS` | `PASS` |
| 23 | OBJECTIVES | `PASS` | `PASS` |
| 24 | KO/CLEANUP | `PASS` | `PASS` |
| 25 | UI/HUD | `PASS` | `OBSERVED` |
| 26 | CERTAINTY | `PASS` | `PASS` |
| 27 | COMBAT LOG | `PASS` | `PASS` |
| 28 | TURNLOG/REPLAY | `OBSERVED` | `PASS` |
| 29 | DETERMINISM | `OBSERVED` | `PASS` |
| 30 | NETWORK AUTHORITY | `OBSERVED` | `PASS` |
| 31 | PRIVACY | `OBSERVED` | `PASS` |
| 32 | AUTOMATION/SCENARIO | `OBSERVED` | `PASS` |
| 33 | ERRORS | `PASS` | `PASS` |
| 34 | PERFORMANCE | `OBSERVED` | `PASS` |
| 35 | SAVE/RELOAD | `PASS` | `PASS` |
| 36 | PACKAGED | `N/A` | `PASS` |

Lettura di `OBSERVED` come tetto: quel ruolo può guardare il sistema e registrare cosa vede, ma non possiede lo strumento che lo prova.

Caso guida — `PRIVACY`. EDITOR può constatare che un dato privato non compare nella UI avversaria. Non è una prova: il dato può essere presente sul client. La prova richiede canary lato connessione, che appartiene a VALIDATION. EDITOR emette `OBSERVED`, mai `PASS`.

Disaccordo tra colonne: vince VALIDATION quando il suo verdetto è `FAIL`. Un `PASS` EDITOR contro un `FAIL` VALIDATION apre un Finding, non una media.

⚠️ **Un ruolo assente da questa tabella non ha un tetto basso: non ha lo strumento.** DEV-LEAD non compare in nessuna colonna, ed è da qui che §9 trae la conseguenza — il suo handoff non porta payload di verdetti.

## 8. Scoping dal write-set

La matrice non si compila per intero a ogni wave.

L'handoff in ingresso dichiara `WRITE_SET`. Da lì:

1. i sistemi toccati dal write-set sono **in scope** e richiedono un verdetto verificato;
2. i sistemi non toccati sono `N/A` con `REASON: fuori write-set`;
3. i sistemi non toccati ma a valle di uno toccato sono in scope come regressione.

Il punto 3 non è opzionale. Se il write-set tocca il resolver, `TURNLOG/REPLAY` e `DETERMINISM` sono in scope anche se nessun file di quei sistemi è stato modificato.

Non ampliare lo scope per completezza. Non restringerlo per costo.

## 9. Schema di handoff

Tre punti fissi, non uno. Un ruolo che scrive produce un commit diverso da quello che ha ricevuto.

Un handoff ha due parti. La **busta** identifica e traccia, e la portano tutti. Il **payload** porta i verdetti, e lo porta chi possiede lo strumento per produrli.

### Busta — sempre

```text
=== RT3 HANDOFF ===

FROM:          DEV-LEAD | EDITOR | VALIDATION
TO:            EDITOR | VALIDATION | DEV-LEAD
FEATURE:
WAVE_ID:       <feature-slug>/<n>

BRANCH:
PARENT_BRANCH: base reale della PR, non "main" per default
BASE_SHA:      commit ereditato in ingresso
PRODUCED_SHA:  commit dopo le scritture di questo ruolo
               = BASE_SHA se questo ruolo non ha scritto

WRITE_SET:     path espliciti, testuali e binari
BINARY_ASSETS: .uasset/.umap toccati, oppure "nessuno"

STATUS: READY | PARTIAL | BLOCKED
```

### Payload — se e solo se §7 assegna al ruolo una colonna

```text
## MATRICE
<voci in scope, con verdetto tipizzato e campi richiesti>

## FINDINGS
<Finding ID, severità, owner, evidenza — vedi §12>

## EVIDENCE
<EVIDENCE_REF, uno per riga>

## USER_REQUIRED
<check a oracolo umano, con Result: NOT RUN>
```

La condizione non è un'eccezione da ricordare: si legge in §7. Un ruolo che non compare nella matrice canonica non possiede lo strumento che prova quei sistemi, e §3 lo dice già — `file modificato != build/test/PIE/packaged verificato`.

⚠️ **Assenza di payload non è payload malformato.** §6 legge `BLOCKED` una **voce di verdetto** malformata. Un handoff che per questa sezione non porta payload non ha voci: non c'è nulla da leggere malformato, e non è `BLOCKED` per questo.

**DEV-LEAD è oggi l'unico ruolo in questa condizione.** Il suo handoff porta la busta e, al posto del payload, i **sistemi in scope** derivati dal write-set con §8 — senza verdetto. Lo scoping fatto una volta all'ingresso vale per entrambi i ruoli a valle, che altrimenti lo ripetono.

`PARENT_BRANCH` è obbligatorio: la PR va aperta sul branch padre, non su `main` per default.

Se `PRODUCED_SHA` differisce da `BASE_SHA`, il ruolo successivo verifica `PRODUCED_SHA`. Un mismatch è `BLOCKED`, non un avviso.

## 10. Persistenza

Un handoff che vive solo nella conversazione non esiste per il ruolo successivo.

```text
docs/rt-three-terminals/waves/<feature-slug>/
  RT3-DEVLEAD-<sha7>.md
  RT3-EDITOR-<sha7>.md
  RT3-VALIDATION-<sha7>.md
  contrib/
  evidence/
```

`<sha7>` è il `PRODUCED_SHA` del ruolo che emette.

I tre file `RT3-*` sono gli handoff dei tre punti fissi della catena. `contrib/` raccoglie i **contributi** delle istanze DEV che non sono DEV-LEAD: non sono handoff, non portano verdetti di §7, e la loro identità non deriva dallo SHA — vedi [`../waves/README.md`](../waves/README.md).

Il ruolo che riceve legge il file. Non ricostruisce l'handoff dal contesto della chat.

Vale anche per l'evidenza: uno screenshot descritto a parole non è riverificabile.

## 11. Propagazione di BLOCKED

Un handoff in ingresso con `STATUS: BLOCKED` blocca il ruolo successivo.

```text
STATUS: BLOCKED
REASON: upstream BLOCKED — <WAVE_ID> <FROM>
UNBLOCK: <ciò che il ruolo a monte deve produrre>
```

Non validare sopra una base che il ruolo precedente ha dichiarato inaffidabile.

`STATUS: PARTIAL` non blocca. Ma i sistemi che il ruolo a monte ha lasciato `BLOCKED` o `NOT RUN` non diventano `PASS` a valle per ereditarietà: vanno misurati, oppure restano non provati.

## 12. Defect policy

`P0`/`P1` trovato da VALIDATION: non riparare il codice di produzione e poi approvare sé stessi.

```text
FINDING_ID:   <WAVE_ID>-F<n>
SEVERITY:     P0 | P1 | P2 | P3
EVIDENCE_REF:
ROOT_CAUSE:
OWNER:        DEV-LEAD | EDITOR
REQUIRED_FIX:
REGRESSION:   test che deve esistere prima della richiusura
ATTEMPT:      <n>
```

Poi richiedi un nuovo `PRODUCED_SHA` e rivalida.

Terminazione — il ciclo non è illimitato:

- ogni ripresentazione dello stesso `FINDING_ID` incrementa `ATTEMPT`;
- ad `ATTEMPT = 3` il ciclo si ferma;
- il Finding viene escalato a decisione umana con `STATUS: BLOCKED` e `REASON: defect loop`.

Un `FINDING_ID` è stabile. Ricomparire con un id nuovo per azzerare il contatore è un aggiramento.

## 13. Definition of Done

`DONE` richiede la Definition of Done **viva**, non quella citata da un handoff.

Rileggila alla chiusura. Se è cambiata durante la wave, vale quella corrente.

Nessun verdetto verde senza il campo che lo prova.

## 14. Capability matrix

Ogni operazione appartiene a una classe. La classe dice **cosa serve prima**, e un tool
non classificato non ha una risposta di default permissiva.

| Capability | Requisiti |
|---|---|
| `MCP_READ_ONLY` | figura compatibile; nessun side effect; non richiede lease |
| `WORKTREE_WRITE` | task/issue e write-set dichiarati |
| `EXTERNAL_WRITE` | task, target remoto, ownership, log; idempotenza dove applicabile |
| `UNREAL_USE` | lease vivo e posseduto; contesto Unreal verificato |
| `MCP_ASSET_READ` | contesto progetto verificato; lease se richiede l'Editor vivo |
| `MCP_ASSET_WRITE` | `EDITOR` + workspace `MAIN` + branch di task + task id + write-set asset + lease + binding MCP verificato |
| `VALIDATION_SIGNOFF` | sessione `VALIDATION` indipendente; input immutato; evidenza sufficiente |

⛔ **Un tool MCP sconosciuto o non classificato che potrebbe avere side effect e'
`DENIED_UNCLASSIFIED`.** Il default permissivo e' esattamente il modo in cui una
superficie nuova entra senza che nessuno la valuti.

🔴 **La superficie mutante non e' quella di RefactorTactics, ed e' enorme.**
Misurata sul bridge vivo il **2026-09-06**, parlando al server in JSON-RPC senza
passare da nessuno script:

```text
tools/list      ->  3 meta-tool   (list_toolsets, describe_toolset, call_tool)
list_toolsets   -> 56 toolset     di cui 1 di RefactorTactics e 55 no
```

I cinque tool `RTDeveloperTools` sono tutti read-only per costruzione, e il loro
brief vieta la scrittura asset arbitraria. Gli altri 55 toolset non hanno quel
vincolo. Tre esempi, coi nomi reali dei loro tool:

| Toolset | Tool | Cosa consente |
|---|---|---|
| `editor_toolset.toolsets.asset.AssetTools` | `write_file` `delete` `move` `duplicate` `save_assets` | mutazione asset completa, **e scrittura di file su disco** |
| `AutomationTestToolset` | `RunTests` `RunTestsByFilter` `StopTests` | avviare **e fermare** Automation Test |
| `editor_toolset.toolsets.programmatic.ProgrammaticToolset` | `execute_tool_script` | eseguire **Python** che orchestra tutti i tool sopra in una sola chiamata |

⛔ **`RunTests` e `StopTests` sono il caso peggiore per questo repository**, e non
erano stati nominati da nessuno. Una chiamata MCP puo' far partire una suite senza
passare da `rt-suite.ps1`, dal suo mutex, dal lease e dall'invariante di validita'
della misura - oppure **fermare** i test che un'altra sessione sta eseguendo. E' lo
stesso difetto del finding `parsecell-arity/1-F13`, su un canale che nessun guard
vede.

Con `bEnableToolSearch = true` questi toolset non compaiono in `tools/list`: chi
guarda solo li' conclude che la superficie sia di tre tool. Classificare significa
guardare dietro `call_tool`, non nel plugin di casa.

Codici di rifiuto stabili, gli stessi che gli script stampano:

```text
ASSET_WRITE_WRONG_WORKSPACE    ASSET_WRITE_ROLE_DENIED
TASK_CONTEXT_MISSING           PROTECTED_BRANCH_DENIED
ENGINE_LEASE_REQUIRED          MCP_CONTEXT_MISMATCH
ASSET_WRITESET_CONFLICT        DENIED_UNCLASSIFIED
```

## 15. Lease del motore, e cosa il lease non e'

Il lease e' della **risorsa**, non del ruolo, ed e' unico per macchina. Vive sotto
`%LOCALAPPDATA%\RefactorTactics\RT3\` perche' il motore e' uno e i checkout sono molti.

Metadata:

```text
schema_version · lease_id · role · terminal_instance
workspace_id · workspace_root · project_path
task_id · operation
owner_pid · owner_started_at_utc · editor_pid · mcp_endpoint
branch · head_sha · acquired_at_utc
```

🔴 **`owner_pid` e' il PID del TERMINALE RT, non del processo che scrive il file.**

Il comando che acquisisce il lease e' effimero: termina un istante dopo. Un lease
che dichiarasse quel PID nascerebbe gia' `STALE` — misurato il 2026-09-06 su
`d062ccf0`, con `ACQUIRED` e lo `status` immediatamente successivo che diceva
`STALE (owner non piu' vivo)`.

Chi tiene il motore e' la **sessione**, che sopravvive ai comandi lanciati al suo
interno. L'identita' arriva ai processi figli per variabile d'ambiente:

```text
RT_TERMINAL_INSTANCE          id LOGICO del terminale - etichetta, prompt, log
RT_TERMINAL_OWNER_PID         id OS del processo terminale persistente
RT_TERMINAL_OWNER_STARTED_AT  istante di avvio di quel processo, UTC ISO-8601
RT_TERMINAL_ROLE              ruolo della sessione
```

⚠️ **Le due identita' sono distinte, e non vanno confuse.** L'ownership si decide
sull'identita' **OS**; l'id logico non ci entra mai. Due terminali possono portare
etichette qualunque e restano sessioni diverse perche' hanno processi diversi -
ed e' cio' che consente piu' terminali DEV, EDITOR o VALIDATION nello stesso
workspace.

`acquire` e `release` senza queste variabili falliscono **fail-closed**
(`RT_SESSION_REQUIRED`): un processo effimero non puo' possedere una risorsa che
gli sopravvive. `status` resta invocabile ovunque, perche' e' sola lettura.

`owner_started_at_utc` esiste perche' un PID si ricicla: da solo non prova che
l'owner sia ancora lo stesso processo.

⚠️ Gli istanti si confrontano **normalizzati**. `ConvertFrom-Json` non
restituisce le stringhe ISO-8601 come stringhe: le converte in `[datetime]`, e un
confronto testuale con l'istante ricalcolato falliva sempre dopo la rilettura. Il
difetto sopravvive a chi lo corregge una volta sola: e' coperto da un caso di
`-SelfTest`.

**Nessun heartbeat**: non e' implementato, e descriverne uno sarebbe dichiarare una
proprieta' che nessuno misura. Un lease il cui owner non e' piu' vivo e' `STALE`, e
il recupero e' esplicito — mai automatico, e comunque negato se un processo motore
vivo non e' attribuibile.

### Un solo predicato di ownership

⛔ **La verifica dell'identita' del workspace e' fail-closed.**

`acquire` distingue tre esiti, e due bloccano:

| Esito | Comportamento |
|---|---|
| contratto disponibile, verdetto positivo | si procede |
| contratto disponibile, verdetto negativo | `BLOCKED` col codice del verdetto |
| contratto **non disponibile** | `BLOCKED`, `WORKSPACE_CONTRACT_UNAVAILABLE` |

Il terzo caso e' quello che una prima stesura sbagliava: restituiva «nessun
verdetto» e il chiamante proseguiva. Bastava cancellare o corrompere
`rt-workspace.ps1` per aggirare la validazione. **Non aver potuto verificare non e'
aver verificato.**

⛔ La domanda «questo lease e' mio?» ha **una sola sede**.

Prima ne aveva tre, in due varianti: `rt-lease.ps1` confrontava solo il PID,
mentre `rt-suite-safe.ps1` e `rt-mcp-guard.ps1` accettavano anche
`terminal_instance`. Due script riconoscevano un proprietario che il terzo
rifiutava — e la divergenza non era visibile finche' qualcuno non provava a
rilasciare un lease che la suite aveva appena accettato.

`Test-LeaseOwnedBy` vive in `rt-lease.ps1` ed e' importata dagli altri due
dall'AST. Un rename a monte produce `OWNERSHIP_CONTRACT_UNAVAILABLE`, non un
permesso concesso per sbaglio.

La funzione e' **pura**: `(lease, identita') -> bool`. E' cio' che la rende
verificabile senza occupare il motore, con `rt-lease.ps1 -SelfTest`.

### Log

Ogni acquisizione, rilascio e chiamata con side effect produce una riga JSONL in
`events.jsonl` accanto al lease:

```text
timestamp_utc · event · task_id · role · terminal_instance
workspace_id · workspace_root · branch · head_sha
lease_id · operation_class · target_summary · result · error_code
```

Niente token, credenziali, URL firmati, header, prompt o payload asset. La scrittura
e' in append con retry, e un log che non riesce a scrivere **non** trasforma un
fallimento in successo: stampa un avviso e lascia l'esito dov'era.

Il file cresce e nessuno lo ruota: la pulizia e' manuale, e appartiene a chi possiede
la macchina.

### 🔴 Cosa questo contratto NON puo' garantire

Il trasporto MCP e' HTTP diretto. Un client che salta il preflight raggiunge il
bridge lo stesso: **nessuno script PowerShell sta su quel percorso**.

Il preflight autorizza, non intercetta. L'enforcement che regge davvero e' di
configurazione — il bridge **parte solo in MAIN**, quindi dove non gira non c'e' una
porta da raggiungere — e di disciplina. Chiamarla barriera sarebbe un falso verde, e
un falso verde fa smettere di cercare la barriera vera.

⚠️ **La leva e' `bAutoStartServer`**, per utente in
`Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`: `On` in MAIN, `Off`
altrove, applicato dall'installer. Vive fuori dal versionamento, quindi un `git pull`
non la porta e va riapplicata su ogni checkout.

⛔ **`.mcp.json` non e' quella leva.** E' versionato e byte-identico nei tre checkout
— misurato il 2026-09-06, `E20761402582` in tutti e tre — quindi non discrimina
niente: chi lo leggesse come confine leggerebbe un file uguale ovunque.

---

## Aperti

Due domande sul contratto stesso, **non normative**: registrate perché l'owner le decida, non risolte qui.

Non modificano nessuna regola sopra. Finché restano aperte, vale il testo delle sezioni §1–§13.

| ID | Domanda | Registro |
|---|---|---|
| ~~`GOV-5`~~ | ✅ **Chiusa il 2026-09-05 da [`D-335`](../../decisions/RT_PDR_00_Decision_Log.md).** §9 separa la **busta**, che portano tutti, dal **payload di verdetti**, che si porta se e solo se §7 assegna una colonna. La regola è recepita sopra e non è più aperta. | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) · riga **97** di [`DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) |
| `GOV-6` | La forma del contratto comportamentale (`Given`/`When`/`Then`/`Authority`/`SEED_SOURCE`/…) sale qui, o resta nei prompt di wave DEV? Oggi vive in `WAVE_DEV_LEAD.md` e la usa `WAVE_DEV_MAIN.md`. | [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |

`GOV-6` resta aperta e non blocca: è una questione di sede, non di correttezza. Un prompt che apre un altro prompt per leggere una tabella funziona; è fragile, non rotto.
