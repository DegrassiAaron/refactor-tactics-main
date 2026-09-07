# RT3 — Work3, Editor singleton e contratto temporale

> Sede canonica di quattro gruppi di decisioni che prima non ne avevano una:
> come i tre worktree si dividono il lavoro, come un task resta di chi lo possiede,
> come l'unico Unreal viene serializzato, e che cosa significano i timestamp.
>
> Il control plane e i suoi comandi stanno in [`RT3_CONTROL_PLANE.md`](RT3_CONTROL_PLANE.md).
> Il contratto di wave sta in [`prompts/RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md).
> Il routing dei task su file sta in [`TASK_ROUTING.md`](TASK_ROUTING.md) — è un
> sistema **diverso** da questo, e §12 qui sotto dice in che cosa.

Ogni sezione dichiara che cosa è **implementato** e che cosa è solo **deciso**. La
distinzione non è cosmetica: una decisione scritta come se fosse codice manda chi legge
a cercare una funzione che non esiste.

```text
IMPLEMENTATO       il codice lo fa, e un test lo dimostra
DECISO, NON IMPLEMENTATO   la regola vale, il codice non la esegue ancora
DECISO, NON TESTATO        il codice lo fa, nessun test lo pinna
```

---

## 1. Il modello Work3

Tre worktree permanenti, **un solo** Unreal Editor.

```text
MAIN       ──┐
DEV        ──┼──> tre lavorazioni in parallelo
DESIGNER   ──┘

                 ... e una sola coda verso Unreal
```

🔴 **I tre worktree sono capacità, non fasi.** Non esiste nessuna pipeline
`MAIN → DEV → DESIGNER` da percorrere per ogni feature. Il default è che ogni lane
lavori una Epic diversa:

```text
MAIN      → Epic X
DEV       → Epic Y
DESIGNER  → Epic Z
```

Una Epic si distribuisce su più worker **solo** quando contiene issue realmente
indipendenti — cioè quando il grafo delle dipendenze le mostra parallelizzabili nello
stesso momento. Non prima, e non per abitudine.

Chi lo decide non è una convenzione: è il planner, che legge `requires` e le capacità
dichiarate nella roadmap. La catena `DEV-LEAD → EDITOR → VALIDATION` del contratto di
wave descrive una **sequenza di verifica** su una singola lavorazione, non un percorso
obbligato attraverso i tre checkout.

**Stato: IMPLEMENTATO.** `writerCapacity` per gruppo, `temporaryWorktrees.capacity` e
`wip` vivono nella roadmap; il planner li rispetta. Vedi
[`RT3_CONTROL_PLANE.md` §12](RT3_CONTROL_PLANE.md).

---

## 2. Ownership di una feature

Un task ha **un** proprietario, e lo mantiene.

```text
DEV-GRID-1 possiede GRID-2303
```

Se `GRID-2303` ha bisogno dell'Editor, la proprietà **non si sposta**: l'esecuzione
dentro Unreal è un servizio che qualcun altro rende, non un passaggio di consegne.

```text
Editor Job ownership  !=  Feature ownership
```

Dopo la validazione o l'esecuzione in Editor, il task torna sempre al proprio
proprietario. Il ruolo EDITOR **non** diventa titolare della feature per il fatto di
averla aperta nell'Editor.

**Stato: PARZIALE.** L'ownership è rappresentata oggi da due dati distinti:

- `sessions.task_id` — su quale task una sessione dichiara di lavorare;
- `Assignment.ownerSessionId` — a quale sessione il planner assegna una issue, quando
  la risorsa che serve è già di qualcuno.

Non esiste invece un campo che leghi un task al suo proprietario **attraverso** una
esecuzione in Editor, perché non esiste ancora l'EditorJob (§5). Finché non c'è, la
proprietà si conserva perché nessuno la sposta, non perché qualcosa lo impedisca.

---

## 3. Unreal è uno solo

L'Editor è una risorsa esclusiva della macchina, non del ruolo.

```text
MAIN      ──┐
DEV       ──┼──>  coda  ──>  UN Unreal Editor
DESIGNER  ──┘
```

```text
ruolo EDITOR  !=  proprietà permanente di Unreal
```

Chi tiene l'Editor lo tiene per il tempo di un lavoro, e poi lo rilascia. Un terminale
EDITOR aperto non possiede Unreal: aprire un terminale non acquisisce niente.

### Il lease che esiste davvero

**Stato: IMPLEMENTATO**, tabella `leases` (schema v4):

| Colonna | Significato |
|---|---|
| `lease_id` | identità del lease |
| `resource_type` | `GIT_WRITER` oppure `UNREAL_EDITOR` |
| `resource_key` | per `GIT_WRITER` il path canonico dell'albero |
| `owner_session_id` | chi lo tiene |
| `acquired_at` | quando è stato preso |
| `last_seen_at` | ultimo segno di vita del proprietario |
| `released_at` · `released_by` | quando, e per mano di chi |
| `state` | `ACTIVE` \| `RELEASED` |
| `note` | testo libero |

L'esclusività non è codice applicativo, è un indice:

```sql
CREATE UNIQUE INDEX idx_lease_exclusive
    ON leases(resource_type, resource_key) WHERE state='ACTIVE'
```

`stale` **non è una colonna**: è derivato, ed è vero quando il lease è `ACTIVE` ma la
sessione proprietaria non lo è più.

### Che cosa il lease non porta, e perché

La specifica di questa milestone elencava `OwnerTask`, `OwnerWorktree`, `Branch`,
`Commit`, `Purpose`. Nessuno di questi è una colonna di `leases`, e **non deve
diventarlo per duplicazione**: sono già sulla sessione proprietaria
(`sessions.task_id`, `worktree_path`, `branch`, `head`), che il lease nomina con
`owner_session_id`. Copiarli qui creerebbe due verità sullo stesso fatto, e la seconda
invecchierebbe.

Restano genuinamente assenti due dati:

- `RequestedAt` — quando il lease è stato **chiesto**, distinto da quando è stato
  ottenuto. Oggi esiste solo `acquired_at`, quindi l'attesa non è misurabile.
- `ExpiresAt` — nessuna scadenza: un lease non scade da solo. Vedi §8.

⛔ **RT3 non ruba mai un lease**, nemmeno stale. Liberarlo da soli significherebbe
toglierlo a una sessione che magari sta solo tacendo.

⚠️ Il lease del control plane impedisce a due **sessioni RT3** di dichiararsi
proprietarie della stessa risorsa. Non impedisce a un essere umano di aprire Unreal
fuori da RT3: quello resta il compito di `rt-lease.ps1`, e i due meccanismi convivono.

---

## 4. `WAITING_EDITOR` non blocca il worker

🔴 La decisione che rende utile la coda.

```text
WAITING_EDITOR  !=  WORKER_BLOCKED
```

Un task in attesa dell'Editor non ferma chi lo possiede:

```text
DEV-GRID-1
    GRID-2303   WAITING_EDITOR     <- in coda, non abbandonato
    GRID-2304   ACTIVE             <- il worker intanto lavora questa
    GRID-2305   READY
```

Quando un task entra in attesa:

1. la richiesta viene registrata;
2. proprietario, branch e commit restano legati al task;
3. il task passa a `WAITING_EDITOR` / `EDITOR_QUEUED`;
4. **lo slot di esecuzione si libera**;
5. il worker può prendere altro lavoro `READY`;
6. la proprietà del task non cambia.

Due situazioni che sembrano uguali e non lo sono:

```text
IDLE_NO_WORK        il worker non ha nulla da fare
BLOCKED_ON_EDITOR   il worker aspetta, e potrebbe non doverlo fare
```

Nel funzionamento normale il secondo non deve esistere: se un worker è fermo mentre
aspetta l'Editor **e** ha altro lavoro pronto, è un difetto. Niente attesa attiva.

**Stato: DECISO, NON IMPLEMENTATO.** Non esiste `WAITING_EDITOR` nel codice, né una
coda. Oggi una sessione che vuole l'Editor chiama `lease.acquire` e o lo ottiene o
riceve `RT3_UNREAL_ALREADY_OWNED`: non c'è un posto dove mettersi in fila.

Conseguenza già valida però in §11: `RESOURCE_UNREAL` come `BlockingReason` descrive
**la sessione**, e va usato solo quando la sessione non ha davvero altro da fare — non
per il solo fatto che un suo task sia in coda.

---

## 5. EditorJob

Il lavoro che serializza sull'Editor è un oggetto, non un messaggio.

```text
JobId · TaskId · EpicId · OwnerSession · OwnerWorktree
SourceBranch · CommitSHA
Purpose · Type · RequiredChecks · Priority
Status
CreatedAt · QueuedAt · StartedAt · CompletedAt
LeaseRequestedAt · LeaseAcquiredAt · LeaseReleasedAt
Result · Evidence
```

Stati:

```text
QUEUED → RUNNING → PASS | FAIL | CANCELLED
```

⚠️ Questi stati sono **dell'EditorJob**, e non vanno confusi con `TASK_STATUSES`
(`ACTIVE` `BLOCKED` `DONE`) né con `CANDIDATE_STATUSES` (`PENDING` `PASSED` `FAILED`).
Un job che finisce `PASS` non rende `DONE` il task: dice che quella esecuzione è
riuscita.

**Stato: DECISO, NON IMPLEMENTATO.** Nessuna tabella, nessun tipo, nessun comando.
Introdurlo richiederà una migrazione di schema — vedi §10.

### Classificazione operativa

```text
E1  VALIDATION              far girare qualcosa e guardarne l'esito
E2  ASSET_MUTATION          toccare .uasset / .umap dentro l'Editor
E3  INTEGRATION_PACKAGING   integrare o impacchettare
```

⛔ È una **classificazione**, non una pipeline: un task non deve attraversare E1, E2 ed
E3 in ordine. Serve a dire che tipo di lavoro chiede l'Editor, perché i tre hanno costi
e rischi diversi.

---

## 6. Workspace di integrazione

Il problema reale: Unreal aperto alternativamente su tre alberi che divergono vede tre
stati diversi dello stesso progetto, e ciò che valida su uno non dice nulla degli altri.

```text
MAIN      ──┐
DEV       ──┼──>  stato di integrazione noto  ──>  Unreal
DESIGNER  ──┘
```

**Stato: DECISO, NON IMPLEMENTATO.** Oggi l'Editor si apre sul clone principale, e la
regola vive in [`prompts/RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md): l'authoring asset
via MCP è consentito **solo** dal workspace `MAIN`, perché il bridge MCP è uno solo e
vive lì.

⛔ **Non si crea una quarta copia del progetto** per aderire a questo schema. Una copia
completa di un progetto Unreal costa spazio e sincronizzazione, e la decisione su come
ottenere uno stato di integrazione noto — quarto albero, branch dedicato, o merge
esplicito prima della seduta — non è ancora presa.

---

## 7. Contratto temporale

Ogni timestamp persistito o destinato a una macchina è **UTC ISO-8601 con suffisso `Z`**.

```text
2026-09-07T07:42:18Z
```

Un evento non ancora avvenuto vale `null`, mai stringa vuota e mai una data finta.

🔴 **Tre istanti che non vanno collassati**, perché misurano tre attese diverse:

```text
QueuedAt           il job entra in coda
LeaseRequestedAt   si chiede la risorsa
LeaseAcquiredAt    si ottiene la risorsa
```

Fonderli renderebbe impossibile distinguere «la coda era lunga» da «la risorsa era
occupata», che sono due problemi con due rimedi diversi.

### Vocabolario

| Istante | Dove vive oggi | Stato |
|---|---|---|
| `GeneratedAt` | `status.generatedAt` | IMPLEMENTATO |
| `StateChangedAt` | — | DECISO, NON IMPLEMENTATO |
| `SessionStartedAt` | `sessions.started_at` | IMPLEMENTATO |
| `TaskStartedAt` | — (esiste `tasks.created_at`) | PARZIALE |
| `RequestCreatedAt` · `RequestQueuedAt` | — | DECISO, NON IMPLEMENTATO |
| `LeaseRequestedAt` | — | DECISO, NON IMPLEMENTATO |
| `LeaseAcquiredAt` | `leases.acquired_at` | IMPLEMENTATO |
| `LeaseReleasedAt` | `leases.released_at` | IMPLEMENTATO |
| `LastHeartbeatAt` | `leases.last_seen_at`, `sessions.last_seen_at` | IMPLEMENTATO (nome diverso) |
| `LeaseExpiresAt` | — | DECISO, NON IMPLEMENTATO |
| `JobStartedAt` · `JobCompletedAt` | — | DECISO, NON IMPLEMENTATO |

⚠️ **Risoluzione al secondo, non al millisecondo.** `now_iso()` tronca i microsecondi:
i timestamp escono come `2026-09-07T07:42:18Z`. Le durate derivate hanno quindi un
errore fino a un secondo. Va bene per un'attesa in coda, non per misurare qualcosa che
dura meno di un secondo. Portare la precisione ai millisecondi cambierebbe il formato di
dati già scritti: è un cambiamento da decidere, non da fare di passaggio.

---

## 8. Durate

🔴 Le durate sono **derivate**, mai persistite. Salvare una durata significa avere due
verità — gli istanti e il loro intervallo — di cui una invecchia a ogni secondo.

```text
SessionAge · TaskAge · RequestAge
QueueWaitDuration · LeaseWaitDuration · LeaseHeldDuration
WaitingEditorDuration
```

```text
macchina   millisecondi interi
umano      "8m 16s", solo nel renderer
```

**Stato: DECISO, NON IMPLEMENTATO.** Nessuna di queste durate è oggi calcolata o
mostrata. Gli istanti che ne sarebbero gli operandi esistono solo in parte (§7).

---

## 9. `StateRevision` e il tempo che passa

```text
il tempo che passa  !=  un cambiamento di stato
```

`GeneratedAt` cambia a ogni lettura; `StateRevision` no. Un'attesa che passa da `04m12s`
a `04m13s` **non produce diff**, altrimenti lo status diventerebbe un orologio e
smetterebbe di essere letto.

`StateRevision` cambia sulle transizioni vere:

```text
AVAILABLE → REQUESTED → OWNED → RELEASED
OWNED → STALE
```

e sui cambi semantici di proprietario, task, candidate, appartenenza o ordine in coda,
ragione di blocco, azione richiesta, stato di una risorsa.

**Stato: IMPLEMENTATO.** `state_revision()` calcola l'hash dei soli campi semantici,
escludendo `generatedAt`, `stateRevision` e `lastSeenAt`. `tests/test_status.py` lo
verifica su ognuno dei 15 campi significativi, non solo su uno.

⚠️ **Il confronto fra due letture successive non è cablato in nessun comando.** Le
funzioni `diff`, `has_significant_diff` e `render_diff` esistono e sono provate, ma
nulla persiste l'ultima `stateRevision` vista, quindi nessun comando sopprime da solo le
ripetizioni. Il non-spam è una proprietà del modulo, non ancora del prodotto.

---

## 10. Heartbeat e `STALE`

🔴 **Un lease non diventa stale perché è vecchio.** L'età non dice niente: una sessione
può tenere legittimamente l'Editor per un'ora.

```text
STALE  =  nessun segno di vita  +  verifica che il proprietario non ci sia più
```

Un heartbeat che arriva è un fatto senza conseguenze semantiche:

```text
heartbeat ricevuto   →  nessun cambio di StateRevision, nessun diff
transizione a STALE  →  StateRevision cambia
```

**Stato: PARZIALE.**

- `last_seen_at` esiste su `sessions` e su `leases`, e si aggiorna quando la sessione
  fa qualcosa. **Non c'è un heartbeat periodico**: una sessione silenziosa ma viva
  invecchia come una morta.
- `stale` è derivato da `owner_status != 'ACTIVE'`, cioè dallo **stato dichiarato**
  della sessione, non da una verifica del processo. Questo soddisfa già la regola
  «più dell'età», ma non la verifica: una sessione il cui terminale è stato chiuso
  senza `session stop` resta `ACTIVE`, e il suo lease non risulta stale.
- `sessions.client_pid` è registrato: **il dato per verificare il processo esiste**, e
  nessuno lo interroga.
- `STALE` non è un valore di `LEASE_STATES` (`ACTIVE` \| `RELEASED`) né di
  `UNREAL_LEASE_STATES` (`NONE` \| `REQUESTED` \| `OWNED`). Renderlo uno stato
  persistito richiede una migrazione.

---

## 11. Che cosa blocca davvero una sessione

`BlockingReason` descrive **la sessione**, non il singolo task:

```text
DEPENDENCY · RESOURCE_WRITER · RESOURCE_UNREAL
WAITING_REVIEW · WAITING_VALIDATION
WORKTREE_MISMATCH · PROTOCOL_MISMATCH · ROADMAP_REVISION
```

🔴 **`RESOURCE_UNREAL` non va usato quando il worker può fare altro.** Un task in coda
per l'Editor non è una sessione bloccata: se quella sessione ha altro lavoro `READY`,
lo stato è `STATUS`, non `BLOCKED`. Marcarla bloccata insegnerebbe a ignorare i blocchi.

**Stato: IMPLEMENTATO** come vocabolario (tutti e otto i valori esistono e sono
validati); **DECISO, NON TESTATO** per la regola qui sopra, che oggi nessun test pinna
perché non esiste ancora una coda che la renda osservabile.

### Azioni richieste

Implementate:

```text
IMPLEMENT_ISSUE · WAIT_REVIEW · REVIEW_CANDIDATE · VALIDATE_CANDIDATE
OPEN_ADDITIONAL_DEV · CREATE_TEMP_WORKTREE · RESOLVE_WRITER_CONFLICT · NONE
```

Decise e assenti, perché descrivono la coda che non c'è:

```text
REQUEST_EDITOR · WAIT_EDITOR · RESUME_AFTER_EDITOR · RESOLVE_UNREAL_CONFLICT
```

---

## 12. Due spazi di stati che si somigliano

⚠️ Questa sezione esiste per un conflitto apparente, e lo scioglie.

[`TASK_ROUTING.md`](TASK_ROUTING.md) vieta esplicitamente `WAITING_DEV`,
`WAITING_EDITOR` e `WAITING_VALIDATION`, perché in **quel** modello l'informazione la
porta già `next_actor`, e uno stato duplicato diverge.

Il divieto resta valido dov'è scritto. `WAITING_EDITOR` di §4 non lo viola, a due
condizioni che qui si dichiarano:

- appartiene allo **stato del lavoro rispetto alla coda Editor**, non a
  `TASK_STATUSES` del control plane, che restano `ACTIVE` `BLOCKED` `DONE`;
- non esiste un `next_actor` nel control plane RT3 che lo renderebbe ridondante:
  `tasks` ha `status`, `lane`, `created_at`, `updated_at` e nient'altro.

Se un giorno il control plane adottasse un `next_actor`, questa decisione andrebbe
riaperta: sarebbe la stessa duplicazione che `TASK_ROUTING.md` rifiuta.

---

## 13. Piano di test

I test RT3 vivono in `tools/rt3/tests/` e si eseguono con:

```powershell
scripts\rt3.ps1 -SelfTest
```

Stato reale delle proprietà che questa milestone dichiara:

| Proprietà | Stato |
|---|---|
| Snapshot DEV / EDITOR / VALIDATION | IMPLEMENTATO — `test_status.py::RenderTest` |
| stesso stato semantico → nessun diff | IMPLEMENTATO — `NoSpamTest` |
| solo il timestamp → nessun diff | IMPLEMENTATO — `NoSpamTest` |
| `Inbox 0→1` → diff | IMPLEMENTATO — `NoSpamTest` |
| ogni campo significativo produce diff | IMPLEMENTATO — controllo positivo su 15 campi |
| `WRITER_ALREADY_OWNED` | IMPLEMENTATO — `test_leases.py` |
| `UNREAL_ALREADY_OWNED` | IMPLEMENTATO — `test_leases.py` |
| `PROTOCOL_MISMATCH` | IMPLEMENTATO — `test_daemon.py` |
| `WORKTREE_MISMATCH` | DECISO, NON TESTATO — è un `BlockingReason`, non un rifiuto di `session start` |
| Epic con una sola issue READY → 3 terminali | IMPLEMENTATO — `test_bootstrap.py::InitialPlanTest` |
| DEV-2 `REQUIRED` e non `ACTIVE` | IMPLEMENTATO — `DynamicSecondDevTest` |
| isolamento di lane | IMPLEMENTATO — `LaneIsolationTest` |
| riuso del writer permanente posseduto | IMPLEMENTATO — `test_planner.py::PermanentOwnerReuseTest` |
| ordinamento temporale, durate non negative | DECISO, NON TESTATO |
| timestamp invalidi rifiutati | DECISO, NON TESTATO |
| stale richiede più dell'età | PARZIALE — deriva dallo stato della sessione, nessun test dedicato |
| transizione reale a STALE incrementa la revisione | DECISO, NON IMPLEMENTATO |
| serializzazione Editor: A possiede, B attende, C continua | DECISO, NON IMPLEMENTATO |
| mai più di un proprietario di Unreal | IMPLEMENTATO — indice unico parziale |
| il worker resta fermo solo se non ha altro lavoro | DECISO, NON IMPLEMENTATO |

⛔ Nessuna riga di questa tabella è stata dedotta dalla documentazione: ognuna è stata
verificata contro `tools/rt3/`.

---

## Vedi anche

- [`RT3_CONTROL_PLANE.md`](RT3_CONTROL_PLANE.md) — comandi, database, lease, roadmap
- [`prompts/RT3_CONTRACT.md`](prompts/RT3_CONTRACT.md) — contratto di wave, §16 status
- [`TASK_ROUTING.md`](TASK_ROUTING.md) — il router su file, e il suo spazio di stati
- [`prompts/TERMINAL_EDITOR.md`](prompts/TERMINAL_EDITOR.md) — il ruolo EDITOR
