# RT3 — Task routing

Owner di questa semantica. Se un altro documento descrive il routing dei task in modo diverso, vince questo; se contraddice `CLAUDE.md` o `AGENTS.md`, vincono quelli.

Implementazione: [`payload/scripts/rt-task-router.ps1`](payload/scripts/rt-task-router.ps1) · [`payload/scripts/rt-open-task.ps1`](payload/scripts/rt-open-task.ps1).

---

## 1. Il problema che chiude

Con tre ruoli e più checkout, lo stato del lavoro viveva nella testa di chi apriva i terminali:

1. apri un tipo di terminale;
2. incolli un prompt;
3. ricevi un risultato;
4. **ricordi** quale ruolo viene dopo;
5. **ricordi** a chi hai già passato il lavoro;
6. apri il terminale sbagliato, o rimandi un task già fatto.

I passi 4 e 5 non sono lavoro: sono memoria, e la memoria è la parte che si perde chiudendo VS Code.

Il router li sposta su disco:

```text
USER
 |
 v
RT COORDINATOR ----> task router per macchina
 |                        |
 |                        v
 |             DEV | EDITOR | VALIDATION | USER
 |                        |
 |                        v
 +<-------------------- result
```

Chi torna al Coordinator e chiede «dove siamo» riceve una risposta che nessuno ha dovuto ricordare.

---

## 2. Quattro concetti, non uno

È il punto in cui si sbaglia più facilmente, perché tutti e quattro parlano di «chi».

| Concetto | Domanda | Autorità | Comando |
|---|---|---|---|
| **Ruolo di sessione** | cosa può occupare questo terminale | `RT_TERMINAL_ROLE` | `rtstatus` |
| **Identità del workspace** | questa directory ospita il bridge MCP? | registro per macchina | `rtws -Action verify` |
| **Lease del motore** | chi occupa Unreal adesso | lease per macchina | `rtlease -Action status` |
| **Routing del task** | chi deve lavorare adesso | store per macchina | `rttask status -TaskId <id>` |

⛔ **Il Coordinator non è nessuno di questi.**

Non è `MAIN`: `MAIN` è l'identità del *workspace* che ospita l'unico bridge MCP della macchina, e non ha rapporto con il routing.

Non è una quarta figura RT3: le figure restano `DEV`, `EDITOR`, `VALIDATION`. Il Coordinator non imposta `RT_TERMINAL_ROLE`, non acquisisce il lease, non apre PIE, non esegue build o suite, non tocca `.uasset`, non emette verdetti.

Non è `DEV-LEAD`: quella è una funzione **di wave** dentro il ruolo DEV, e continua a esistere come prima.

---

## 3. Storage: per macchina, non nel checkout

```text
%LOCALAPPDATA%\RefactorTactics\RT3\Tasks\<TaskId>\
    state.json          il routing. Lo scrive SOLO il Coordinator.
    task.lock           mutua esclusione fra due Coordinator.
    assignments\        consegne immutabili: 0001-DEV.md, 0002-EDITOR.md, ...
    results\            risultati append-only: 0001-DEV-<instance>.md, ...
```

Sta accanto al registro dei workspace e al lease, e per la stessa ragione: **il motore è uno e i checkout sono molti**. Un task instradato da un checkout deve essere visibile dagli altri due, e un file dentro `Saved/` di una sola directory non lo sarebbe.

### TaskId

Diventa un nome di directory, quindi ha una grammatica dichiarata:

```text
primo carattere   lettera o cifra
poi               lettere, cifre, '.', '_', '-'
lunghezza         1..64
```

Rifiutati con `TASK_ID_INVALID`: slash e backslash, `..`, percorsi assoluti, tutto ciò che inizia con `.` o `-`, spazi, wildcard, e i nomi di device DOS (`CON`, `NUL`, `LPT1`, …) — quelli non sono un rischio di traversal, sono un fallimento oscuro di `New-Item`.

Il vincolo sul primo carattere è ciò che esclude `..` e `.git` senza un caso speciale.

---

## 4. Schema di `state.json`

```json
{
  "schema_version": 1,
  "task_id": "2330",
  "title": "GrayKit Arena Door",
  "status": "ACTIVE",
  "next_actor": "EDITOR",
  "assignment_sequence": 2,
  "assignment_path": "assignments/0002-EDITOR.md",
  "last_result_path": "results/0001-DEV-18452.md",
  "created_at": "2026-09-06T15:00:00.0000000Z",
  "updated_at": "2026-09-06T15:20:00.0000000Z",
  "history": [ { "sequence": 1, "event": "assign", "actor": "DEV", "at": "…", "detail": "…" } ]
}
```

| Actor ammessi | Stati ammessi |
|---|---|
| `DEV` `EDITOR` `VALIDATION` `USER` `NONE` | `ACTIVE` `BLOCKED` `DONE` |

⚠️ **Niente `WAITING_DEV`, `WAITING_EDITOR`, `WAITING_VALIDATION`.** Quell'informazione la porta già `next_actor`, e uno stato duplicato è uno stato che diverge.

Per la stessa ragione **non sono campi**, ma valori derivati dai file presenti:

- `last_result_path` — l'ultimo file in `results/`, che l'ordine lessicografico dei nomi rende cronologico;
- lo stato per attore che `status` mostra (`DONE`, `IN PROGRESS`, `NOT ASSIGNED`) — deriva da quali assignment esistono e quali risultati li seguono.

Uno `state.json` che non ha la forma attesa è `TASK_STATE_CORRUPT`: si rifiuta, **non si ricrea**. Ricrearlo cancellerebbe il routing di un task vivo.

---

## 5. Un solo writer

```text
worker recommendation  !=  routing decision
```

Il Coordinator è l'unico che scrive `state.json`. Un worker scrive **solo** un file nuovo sotto `results/`, e può proporre:

```text
NEXT_ACTOR_RECOMMENDED: VALIDATION
```

ma solo il Coordinator imposta `next_actor`.

Il router lo fa rispettare così:

| Comando | Chi | Regola |
|---|---|---|
| `init` `assign` `close` | Coordinator | **rifiutati** se `RT_TERMINAL_ROLE` è `DEV`, `EDITOR` o `VALIDATION` → `TASK_MUTATION_ROLE_DENIED` |
| `report` | worker | **rifiutato** senza un ruolo → `RT_SESSION_REQUIRED`; e se il ruolo non è `next_actor` → `TASK_ROUTE_MISMATCH` |
| `list` `next` `status` `assignment` `route` | chiunque | sola lettura |

⚠️ **È un guardrail operativo, non un confine di sicurezza.** `RT_TERMINAL_ROLE` è una variabile d'ambiente che il chiamante può scrivere, e lo store sta in `%LOCALAPPDATA%`. Impedisce l'**errore** — la sessione worker che si riassegna il task mentre lavora — non l'abuso. Vale la stessa nota del registro dei workspace e del lease.

---

## 6. Comandi

Dentro un terminale RT esiste `rttask`, accanto a `rtstatus`, `rtws`, `rtlease`, `rtmcp`, `rtbuild`, `rtsuite`.

```powershell
rttask list                                  # tutti i task: stato e prossimo actor
rttask status -TaskId 2330                   # un task: chi ha fatto cosa, chi tocca
rttask assignment -TaskId 2330               # la consegna corrente, per esteso
rttask route -TaskId 2330                    # il banner: questo ruolo è quello atteso?
rttask report -TaskId 2330 -Status DONE -Summary "..."   # deposita un risultato
```

Fuori da un terminale RT — per esempio dal Coordinator — si chiama lo script:

```powershell
pwsh -NoLogo -NoProfile -File scripts/rt-task-router.ps1 -Action list
pwsh -NoLogo -NoProfile -File scripts/rt-task-router.ps1 -Action init   -TaskId 2330 -Title "GrayKit Arena Door"
pwsh -NoLogo -NoProfile -File scripts/rt-task-router.ps1 -Action assign -TaskId 2330 -Actor DEV `
    -ExpectedSequence 0 -Objective "..." -Do "..." -ExpectedOutput "..." -NextIfPass VALIDATION
pwsh -NoLogo -NoProfile -File scripts/rt-task-router.ps1 -Action close  -TaskId 2330 -Reason "..."
```

`-Action next` esiste per gli script: stampa **solo** il prossimo actor su stdout. È ciò che [`payload/scripts/rt-open-task.ps1`](payload/scripts/rt-open-task.ps1) usa per sapere quale ruolo aprire, invece di leggere con una regex l'output pensato per gli umani.

### Concorrenza

Due difese, diverse:

- **`task.lock`** — un file aperto senza condivisione attorno a ogni mutazione. Due Coordinator simultanei: il secondo riceve `BUSY` (exit 3) e riprova;
- **`-ExpectedSequence`** — la guardia ottimistica. Se la sequence sul disco non è quella che il Coordinator aveva letto, la mutazione è **rifiutata** con `TASK_SEQUENCE_CONFLICT` invece di sovrascrivere una decisione più recente.

`state.json` non viene mai riscritto sul posto: si scrive un `.tmp`, lo si chiude, e lo si rinomina. Un'interruzione non lascia un JSON troncato, che sarebbe indistinguibile da un task inesistente.

### Codici di rifiuto

Stabili, come quelli del lease e del registro workspace:

```text
TASK_ID_INVALID              TASK_NOT_FOUND
TASK_ALREADY_EXISTS          TASK_ALREADY_DONE
TASK_STATE_CORRUPT           TASK_SEQUENCE_CONFLICT
TASK_ROUTE_MISMATCH          TASK_ASSIGNMENT_MISSING
TASK_ASSIGNMENT_IMMUTABLE    TASK_ASSIGNMENT_BODY_MISSING
TASK_ACTOR_NOT_WORKER        TASK_MUTATION_ROLE_DENIED
TASK_TITLE_MISSING           TASK_ACTOR_MISSING
RESULT_STATUS_MISSING        RESULT_SUMMARY_MISSING
RT_SESSION_REQUIRED          TASK_STORE_UNAVAILABLE
```

Exit code: `0` fatto · `2` rifiutato · `3` occupato · `1` self-test fallito.

---

## 7. Assignment e result

### Assignment — immutabile

```text
assignments/0001-DEV.md
assignments/0002-EDITOR.md
assignments/0003-VALIDATION.md
```

Contiene sempre: `TASK`, `ACTOR`, `SEQUENCE`, `FROM`, `ISSUED`, poi `OBJECTIVE`, `CONTEXT`, `INPUTS`, `DO`, `DO_NOT`, `EXPECTED_OUTPUT`, `NEXT_IF_PASS`.

Un assignment già emesso **non si modifica** (`TASK_ASSIGNMENT_IMMUTABLE`): chi l'ha letto starebbe lavorando su un testo che non esiste più. Una correzione produce una sequence nuova.

`NEXT_IF_PASS` è informativo e lo dice nel testo: chi lo legge non è autorizzato a instradarsi da solo.

Un assignment verso un worker o verso `USER` richiede `-Objective`, `-Do` e `-ExpectedOutput`: senza, è `TASK_ASSIGNMENT_BODY_MISSING`. Una consegna senza obiettivo e senza output atteso non è una consegna.

### Result — append-only

```text
results/0002-EDITOR-30112.md
```

Contiene: `TASK`, `ROLE`, `INSTANCE`, `ASSIGNMENT_SEQUENCE`, `STATUS`, `REPORTED`, poi `SUMMARY`, `CHANGES`, `EVIDENCE`, `NOT_RUN`, `BLOCKER`, `NEXT_ACTOR_RECOMMENDED`.

`STATUS` è uno di `DONE` `PARTIAL` `BLOCKED` `FAILED`.

Un secondo invio dello stesso ruolo sulla stessa sequence **non sovrascrive** il primo: nasce `…-2.md`. Un ripensamento è un fatto, e resta leggibile.

Il worker **non** aggiorna `state.json`.

---

## 8. Avvio del terminale

`rt-terminal.ps1` accetta già `-TaskId`, e non ne esiste un secondo.

Quando è presente, dopo `rtstatus` il terminale chiede il verdetto al router. È **sola lettura**: un mismatch stampa e non corregge.

Corrispondenza:

```text
====================================================
 RT3 EDITOR
====================================================
 TASK       : 2330
 TITLE      : GrayKit Arena Door
 ASSIGNMENT : 0002
 EXPECTED   : EDITOR
 THIS ROLE  : EDITOR
 ROUTING    : ASSIGNED
 FROM       : DEV
 NEXT IF PASS: VALIDATION - informativo: la decisione resta del Coordinator.
====================================================
```

Mismatch:

```text
TASK ROUTING MISMATCH

Current terminal: DEV
Expected actor: EDITOR
Task: 2330

DO NOT EXECUTE THIS ASSIGNMENT.
Nessuno stato e' stato modificato.
```

Il prompt guadagna un segmento, e la finestra un titolo:

```text
[EDITOR:30112] [WS:MAIN] [TASK:2330] D:\Repositories\...
```

⚠️ **Il titolo e il prompt sono UX.** Non sono una fonte: la fonte è lo store, e `rttask status` è ciò che la legge. È la stessa distinzione per cui `rtmode` è informativo e il lease no.

### Aprire il terminale giusto senza sceglierlo

```text
Terminal -> Run Task -> RT: Open next task terminal
TaskId: 2330
```

`rt-open-task.ps1` chiede l'actor al router e **diventa** quel terminale, caricando `rt-terminal.ps1` con il dot-source. Non lo lancia come processo figlio: un figlio riceverebbe il ruolo e poi morirebbe, lasciando la finestra senza `rtstatus`, senza prompt e senza l'identità OS che il lease usa come owner.

Se l'actor è `USER` o `NONE`, **non apre un ruolo falso**: stampa il perché e lascia una shell normale.

Restano invariati: `RT: Open DEV terminal`, `RT: Open VALIDATION terminal`, `RT: Open EDITOR terminal`, `RT: Open role set …`, `RT: Open 3 terminals`. Un terminale aperto senza `TaskId` si comporta esattamente come prima.

Aggiunti: `RT: Task status` (sola lettura) e `RT: Open COORDINATOR`.

---

## 9. Il Coordinator

Agent di progetto: [`.claude/agents/rt-coordinator.md`](../../.claude/agents/rt-coordinator.md).

```powershell
claude --agent rt-coordinator
```

o `Terminal -> Run Task -> RT: Open COORDINATOR`, che esegue lo stesso comando dopo aver rimosso `RT_TERMINAL_ROLE` dall'ambiente — perché il Coordinator non è un terminale di ruolo, e con un ruolo addosso il router gli rifiuterebbe le mutazioni.

⛔ **Non è l'agent di default del progetto.** I terminali DEV/EDITOR/VALIDATION restano normali sessioni Claude Code con il proprio ruolo RT3, e il Coordinator si avvia esplicitamente.

Il suo confine di tool (`Read`, `Glob`, `Grep`, `Bash`, `PowerShell`, `TodoWrite`) esclude `Write` e `Edit`, ma **non è una barriera**: con `Bash` si può scrivere. È dichiarato nell'agent perché sia verificabile da chi legge il suo output.

---

## 10. `USER` come actor

Quando la risposta richiede un occhio umano — leggibilità, feel, una decisione di design, un'approvazione, un dato che nessuno strumento produce — l'actor è `USER`.

L'assignment viene scritto lo stesso: è l'istruzione alla persona, e dice cosa guardare.

⛔ **`USER_REQUIRED` non diventa `PASS`.** Non per silenzio, non per fretta, non perché il ruolo successivo è andato avanti. Il Coordinator registra l'esito che l'utente dà, e solo dopo decide.

`next_actor = NONE` è diverso: significa che nessuno ha il lavoro adesso. Tipicamente accompagna `status = BLOCKED`.

---

## 11. Più task insieme

Non esiste un `ACTIVE_TASK` globale, e non deve esistere: ogni task ha la sua directory.

```text
> rttask list
2330     ACTIVE   EDITOR      GrayKit Arena Door
2501     ACTIVE   DEV         Move decay
288      ACTIVE   USER        Defeat timing
1904     DONE     NONE        Scenario notes drift
```

Ogni terminale porta il proprio `-TaskId`; il routing di uno non tocca quello degli altri.

---

## 12. Il routing non è una pipeline

⛔ **`DEV -> EDITOR -> VALIDATION` non è cablato da nessuna parte**, ed è una scelta.

Forme tutte legittime:

```text
DEV -> VALIDATION                           bug C++ puro
EDITOR -> VALIDATION                        authoring di contenuto
DEV -> VALIDATION -> EDITOR -> VALIDATION   gate headless, poi PIE, poi sign-off
EDITOR -> USER                              check percettivo
VALIDATION -> DEV                           finding di codice emerso validando
```

Il Coordinator decide dal task corrente e dagli output reali.

---

## 13. Rapporto con le wave RT3

Il router **non sostituisce** [`prompts/RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md), né gli handoff persistiti in [`waves/`](waves/README.md).

```text
Task Router      =  chi deve lavorare adesso?
RT3_CONTRACT     =  quale evidenza e quale handoff servono per una wave formale?
```

Sono due domande diverse e nessuna risponde all'altra.

Quando il lavoro è una wave formale:

- gli `INPUTS` di un assignment **puntano** all'handoff (`waves/<slug>/RT3-DEVLEAD-<sha7>.md`);
- l'`EXPECTED_OUTPUT` **nomina** l'handoff che deve nascere;
- i verdetti restano nell'handoff.

⛔ **Non copiare la matrice §7 dentro `state.json`, e non copiare i verdetti RT3 nel router.** Sarebbe una seconda fonte per la stessa affermazione, e le due divergerebbero — che è esattamente il difetto che `NOT RUN != PASS` esiste per impedire.

`STATUS: DONE` di un result **non è** un verdetto `PASS` di §6. Dice che l'actor ha finito il suo assignment, non che un sistema è stato provato.

---

## 14. Guasti e recupero

| Sintomo | Causa | Cosa fare |
|---|---|---|
| `TASK_NOT_FOUND` | il task non è mai stato creato su **questa macchina** | lo store è per macchina, non si sincronizza: ricrealo dal Coordinator |
| `TASK_STATE_CORRUPT` | `state.json` illeggibile o fuori forma | ispezionalo a mano. Il router **non** lo ricrea di proposito |
| `TASK_ROUTE_MISMATCH` | terminale aperto con il ruolo sbagliato | chiudilo e usa `RT: Open next task terminal` |
| `TASK_SEQUENCE_CONFLICT` | un altro Coordinator ha instradato nel frattempo | rileggi `rttask status` e ridecidi. Non forzare |
| `BUSY` (exit 3) | due mutazioni simultanee | riprova |
| `TASK_MUTATION_ROLE_DENIED` | stai instradando da una sessione worker | il routing lo scrive il Coordinator |
| task fermo su `USER` | aspetta te | `rttask assignment` dice cosa guardare |

Cancellare `%LOCALAPPDATA%\RefactorTactics\RT3\Tasks\` azzera il routing di **tutti** i task e non tocca nient'altro: né il repository, né il lease, né il registro dei workspace.

---

## 15. Cosa NON è source of truth

| Non lo è | Perché |
|---|---|
| `Saved/RT3/ACTIVE_TASK.md` | vive in un solo checkout, mentre il lavoro attraversa tutti |
| `.vscode/active-task.*` | idem, e `.vscode/` non è versionata |
| il titolo della finestra e il prompt | UX. Si aggiornano all'avvio e non dopo |
| la cronologia della chat | non sopravvive alla chiusura, e non è leggibile dall'altro ruolo |
| `rtmode` | è informativo, e il finding `parsecell-arity/1-F13` ha misurato che era *anticorrelato con la verità* |
| `state.json` sullo stato del **repository** | dice chi tocca adesso. Cosa esiste lo dicono repository e GitHub |
| `STATUS: DONE` di un result | non è un verdetto `PASS` di `RT3_CONTRACT.md` §6 |

---

## 16. Verifica

```powershell
pwsh -NoLogo -NoProfile -File scripts/rt-task-router.ps1 -SelfTest -WorkspaceRoot (Get-Location).Path
```

150 casi: regole pure (grammatica del TaskId, sequence, forma dello stato, verdetto di routing, policy di mutazione) e integrazione end-to-end in uno store temporaneo, che invoca lo script come processo figlio e misura anche gli exit code.

Non avvia Unreal, non tocca il lease, non tocca `rtmode`. Tre casi lo **provano** invece di dichiararlo: due confrontano le impronte di `lease.json` e `rt-engine-mode.txt` prima e dopo, il terzo verifica sui token — non sul testo, perché i commenti quei nomi li contengono apposta — che la metà operativa dello script non nomini il motore.

⚠️ «Nessun processo Unreal» **non** si misura contando i processi della macchina: durante una singola esecuzione del self-test l'insieme è cambiato due volte, perché un'altra sessione stava eseguendo una suite. Un test scritto così diventa rosso per il lavoro di qualcun altro.
