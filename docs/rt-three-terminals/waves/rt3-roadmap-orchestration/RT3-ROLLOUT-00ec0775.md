# RT3 Roadmap Orchestration — rollout e smoke multi-workspace

**SourceCommit** `00ec07753479bcc6d02e0f68f65e7b6a97824c49` · branch `feat/rt3-roadmap-orchestration`
**Data** 2026-09-06 · **Verdetto** `RT3 MULTI-WORKSPACE SMOKE TEST: PASS`

**Chiusura** — PR [#2635](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2635)
**MERGED** il 2026-09-06T17:24:50Z in `fix/79-blocked-move-turnlog`, merge commit `9fe49e91`.
Mergiata via `gh api -X PUT`, non `gh pr merge`: il working tree di MAIN è condiviso con
un'altra sessione e il checkout che `gh pr merge` esegue dopo il merge avrebbe spostato
`HEAD` sotto di lei. Ref remoto cancellato a parte (l'API di merge non lo fa).

Smoke rieseguito **dopo** il merge: `PASS`, 89 asserzioni, exit 0 — 194/194 nei tre
workspace. Nessuna issue GitHub corrisponde a questo lavoro (ricerca su «rt3 roadmap
orchestration» e «control plane»: zero aperte), quindi non c'è stato né DoD da aggiornare.

⚠️ **Questo referto arriva in `main` prima del codice che descrive.** Il commit `00ec0775`
è mergiato in `fix/79-blocked-move-turnlog`, e in `main` entrerà con la PR
[#2631](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2631), ancora aperta.
Finché quella non passa, in `main` il control plane è alla **v1**: i file citati qui —
`yamlmini.py`, `roadmap.py`, `graph.py`, `planner.py` — non ci sono. È detto perché non
venga scoperto cercandoli.

---

## 1. Workspace Matrix

| Workspace | ActualPath | GitCommonDir | Branch | HEAD | Prot | Schema | Roadmap |
|---|---|---|---|---|---|---|---|
| MAIN | `D:\Repositories\refactor-tactics-main` | `.../refactor-tactics-main/.git` | `fix/79-blocked-move-turnlog` | `16e95ca8` | 2 | 2 | 1 |
| DEV | `D:\Repositories\refactor-tactict-dev` | `.../refactor-tactict-dev/.git` | `feat/rt3-task-router` | `4d14d489` | 2 | 2 | 1 |
| DESIGNER | `...\refactor-tactics-technical-designer\refactor-tactics-main` | `.../.git` | `issue/2596-enemy-tactical-query` | `6111ad00` | 2 | 2 | 1 |

**Caso B — tre repository distinti.** Non sono worktree: ogni `GitCommonDir` è il `.git`
del proprio checkout. Condividono `origin`, non gli oggetti.

⚠️ `refactor-tactics-technical-designer` **non è un checkout**: è una cartella di lavoro che
*contiene* il clone in `refactor-tactics-main/`. Il workspace DESIGNER è quello annidato —
la cartella esterna non ha né `.git` né `.uproject`.

⚠️ L'HEAD di MAIN si è mosso quattro volte durante la sessione (`19902759` → `6c6bfddc` →
`77233ea3` → `6110d593` → `16e95ca8`): un'altra sessione lavora sullo stesso working tree.
Il commit RT3 è stato costruito con **plumbing** su un ref nuovo, senza spostare `HEAD`.

---

## 2. Files Changed

22 file nel commit: 10 modificati, 12 nuovi.

**Nuovi — motore**
`tools/rt3/rt3/yamlmini.py` · `roadmap.py` · `graph.py` · `planner.py`

**Nuovi — prova**
`tools/rt3/smoke_multi.py` · `tests/test_yamlmini.py` · `test_roadmap.py` · `test_graph.py`
· `test_planner.py` · `test_roadmap_store.py` · `test_cli_surface.py`

**Nuovi — piano**
`docs/rt-three-terminals/roadmaps/rt3-smoke-multi-epic.yaml`

**Modificati**
`rt3/__init__.py` (terza versione) · `cli.py` · `daemon.py` · `errors.py` · `model.py` ·
`store.py` (schema v2) · `tests/harness.py` · `tests/test_daemon.py` ·
`tools/rt3/README.md` · `docs/rt-three-terminals/RT3_CONTROL_PLANE.md`

---

## 3. Distribution

**Metodo: `git fetch` del ref + `git archive` dei path.** Nessun merge, nessun cherry-pick,
nessuna copia fuori da Git.

```bash
git -C <dest> fetch <main> feat/rt3-roadmap-orchestration:feat/rt3-roadmap-orchestration
git -C <main> archive <SourceCommit> tools/rt3 docs/.../roadmaps docs/.../RT3_CONTROL_PLANE.md | tar -x -C <dest>
```

| Workspace | Metodo | HEAD toccato | Branch cambiato |
|---|---|---|---|
| MAIN | commit via plumbing (`commit-tree` + `branch`) | no | no |
| DEV | `fetch` ref + `archive` path | no | no |
| DESIGNER | `fetch` ref + `archive` path | no | no |

**Perché non cherry-pick.** Avrebbe creato un commit RT3 dentro il branch di un'altra wave
(`feat/rt3-task-router`, `issue/2596-...`), e in DEV c'era lavoro non committato di
un'altra sessione: un cherry-pick interrotto avrebbe lasciato `CHERRY_PICK_HEAD` in un
working tree occupato.

**Perché non `checkout -- <path>`.** Avrebbe scritto nell'**indice** condiviso: un
`git commit` dell'altra sessione avrebbe assorbito i miei file. `git archive` scrive solo
il working tree.

Il ref è presente in tutti e tre i repository: nulla di quanto materializzato è
irrecuperabile.

Verifica: **34/34 file identici** byte a byte (normalizzati LF) fra i tre checkout.

---

## 4. Automated Tests

| Workspace | Via Python | Via `scripts\rt3.ps1 -SelfTest` |
|---|---|---|
| MAIN | 194 PASS, 0 FAIL, 0 skip | 194 PASS |
| DEV | 194 PASS, 0 FAIL, 0 skip | 194 PASS |
| DESIGNER | 194 PASS, 0 FAIL, 0 skip | 194 PASS |

Da 74 (preesistenti) a 194. Copertura richiesta: registrazione sessione, SessionId
duplicato, pubblicazione, mailbox persistente, ack, routing ruolo/lane, destinatario
offline, restart, parsing e validazione roadmap, cicli, ready/block, gate, critical path,
capacità risorse, WIP, worktree temporaneo, lease Unreal, piano deterministico, ciclo di
vita del candidate.

### Verifica di mutazione

Il verde non è una prova. Sei mutazioni, e **due hanno scoperto una lacuna** invece di
confermare una copertura:

| Mutazione | Test caduti | Esito |
|---|---|---|
| gate `VALIDATED` soddisfatto anche da `IN_PROGRESS` | 1 accessorio | 🔴 **lacuna** |
| lease Unreal verificato *dopo* il writer | `test_il_lease_unreal_non_consuma_un_writer_se_non_parte` | ok |
| `yamlmini` accetta le anchor | `test_anchor`, `test_yaml_malformato_riporta_la_riga` | ok |
| critical path pesa i nodi 1 ciascuno | 4 test di cammino critico | ok |
| chiavi runtime non più rifiutate | i 3 test `RuntimeInPlanTest` | ok |
| elenco rimesso sotto `roadmap list` | i 3 test `test_cli_surface` | ok |

**Lacuna 1.** Nessun test usava `IN_PROGRESS` come stato di una *dipendenza*: far
soddisfare un gate `VALIDATED` dal lavoro appena cominciato lasciava la suite verde.
Chiusa con `test_una_dipendenza_iniziata_non_sblocca_chi_la_aspetta` (quattro stati
predecessore) e la gemella cross-Epic. Rimisurata: la mutazione ora uccide 3 test.

**Lacuna 2.** La forma della CLI non era provata: un comando finito nel ramo sbagliato
dell'albero dei sottocomandi è stato scoperto dallo **smoke a tre checkout**, il gate più
lento. `test_cli_surface.py` lo fa cadere in un decimo di secondo.

---

## 5. Messaging Smoke Test

Store reale (`%LOCALAPPDATA%\RefactorTactics\RT3\runtime.db`):

```text
DEV-1 (DEV, lane DEV, checkout refactor-tactict-dev)
  │  TASK_READY            ev_2b901a491a14   regola DEV_TASK_READY_TO_EDITOR_SAME_LANE
  ▼
EDITOR-DEV
  │  VALIDATION_REQUESTED  ev_a0a5dc0bd20a   regola EDITOR_VALIDATION_REQUESTED_TO_VALIDATION_SAME_LANE
  ▼
VALIDATOR-DEV
  │  VALIDATION_PASSED     ev_47b23b42555a   regola VALIDATION_VERDICT_TO_EDITOR_SAME_LANE
  ▼
EDITOR-DEV                                   giro chiuso
```

**Chi NON ha ricevuto** — la metà che conta: `EDITOR-DESIGNER` e `VALIDATOR-DESIGNER`
hanno mailbox vuota a ogni passo. La lane DESIGNER è un'altra corsia.

**Offline** — `ev_3fbb1567f1b1` pubblicato verso `EDITOR/DESIGNER` con la sessione
**fermata**; `rt3d` riavviato nel frattempo; riaperta la sessione l'evento era pending, ack
eseguito, sparito dai pendenti. La consegna aspetta un **ruolo**, non un processo.

---

## 6. Roadmap Smoke Test

`rt3-smoke-multi-epic.yaml` — hash `sha256:ce5946b6971cff11`, 3 Epic, 10 issue, 7 archi.

**Critical path** `EPIC-B/B1 → EPIC-B/B2 → EPIC-C/C2 → EPIC-C/C3`, durata **14**.
Attraversa due Epic: un cammino chiuso in un Epic non proverebbe che il grafo sia unico.

**Readiness iniziale** — READY: `A1`, `B1`, `C1`. BLOCKED: le altre sette. ✅ atteso

**Cross-Epic unblock**

| Passo | Effetto |
|---|---|
| `B1 → VALIDATED` | `B2` READY; `C2` **ancora** BLOCKED (aspetta B2, non B1) |
| `B2 → VALIDATED` | `B3` READY, `B4` READY, **`C2` READY** ← confine fra Epic |
| | `C3` resta BLOCKED (aspetta C2); EPIC-A intatto |

**Temporary capacity** — DEV `writerCapacity 1`, `temporaryWorktrees 1`:

```text
EPIC-B/B3  DEV  PERMANENT_WRITER
EPIC-B/B4  DEV  TEMPORARY_WORKTREE_SUGGESTED   ← nessun worktree creato
EPIC-C/C1  —    rimandata: WIP_GLOBAL (4)
```

**Unreal scheduling** — `leaseCapacity 1`, `A3` e `C3` entrambe READY e entrambe
`UNREAL_EDITOR`:

```text
EPIC-C/C3  DESIGNER  PERMANENT_WRITER  UNREAL_EDITOR
EPIC-A/A3  MAIN      rimandata: UNREAL_LEASE_CAPACITY
unrealEditor 1/1
```

L'Editor non è stato avviato: si prova lo scheduler, non Unreal.

**Candidate** — due candidate sullo **stesso branch** `feat/rt3-smoke-b1`, commit diversi:

| Candidate | HEAD | Esito |
|---|---|---|
| `cand_e23f41226926` | `4d14d489` | FAILED |
| `cand_a7144102a4d7` | `6110d593` | PASSED |

La issue `EPIC-B/B1` registra **quale** candidate l'ha portata a VALIDATED; il fallito resta
fallito. L'esito sta sul candidate, non sul branch.

---

## 7. Restart Test

**PASS.** `daemon stop` → `daemon start`, pid **diverso** (restart vero, non riconnessione).
Identici prima e dopo: event log, roadmap, stati, candidate, **piano**, mailbox storica.
Lo schema resta migrato a v2.

---

## 8. Problems Found

1. 🔴 **Lacuna di copertura sul gate `IN_PROGRESS`** — trovata mutando, chiusa. Vedi §4.
2. 🔴 **Superficie CLI non provata** — un comando nel ramo sbagliato è passato al commit ed
   è stato scoperto dallo smoke. Chiusa con `test_cli_surface.py`.
3. ⚠️ **La versione non copre ogni divergenza.** Aggiungere un comando senza alzare
   `PROTOCOL_VERSION` lascia i tre workspace a `(2,2,1)` pur eseguendo codice diverso: è
   successo in questa sessione. Le versioni proteggono il **contratto dichiarato**, non
   l'identità del codice. Chi distribuisce deve ridistribuire, non fidarsi del numero.
4. ⚠️ **Un workspace non aggiornato dava `KeyError`** invece di un messaggio. Corretto: ora
   lo smoke dice «questo checkout ha un rt3 precedente alla Roadmap Orchestration».
5. ⚠️ **`git status --porcelain` mente sui file appena materializzati** — stat-cache
   stantia: 18 file segnalati modificati, `git diff` vuoto. Risolto con
   `git update-index --refresh`; il conto reale è 10 modificati + 12 nuovi.
6. ⚠️ **Il working tree di MAIN è condiviso e attivo.** HEAD si è mosso quattro volte.

### Non eseguito, e perché

- **Push remoto**: `NOT RUN` — deliberato. Il ref vive nei tre repository locali.
- **PR, merge, chiusura issue**: `N/A` — fuori dal mandato di questo rollout.
- **Unreal Editor, MCP, worktree automatici, agent**: `N/A` — esclusi dalla fase 21.
- **Compile UE / Automation / PIE / packaged**: `N/A` — nessun sorgente C++ toccato.

---

## 9. Final Verdict

```text
RT3 MULTI-WORKSPACE SMOKE TEST: PASS
```

Eseguito **due volte**: su `RT3_HOME` temporanea e sullo **store reale di macchina**
(`runtime.db` non esisteva: nessuno stato preesistente distrutto). Entrambe exit 0.
Sullo store reale: 89 asserzioni, un solo `rt3d` (pid 48860) visto dai tre checkout.

Le sessioni di prova sono state fermate a fine corsa; eventi, roadmap e candidate restano
come evidenza.

### Definition of Done

| Criterio | Esito |
|---|---|
| MAIN contiene RT3 funzionante | PASS |
| DEV versione compatibile | PASS |
| DESIGNER versione compatibile | PASS |
| un solo `rt3d` per tutti e tre | PASS |
| sessioni dai tre workspace | PASS — 7 sessioni, branch e HEAD letti da Git |
| routing per lane | PASS |
| routing errato non avviene | PASS — lane DESIGNER mai raggiunta |
| mailbox offline | PASS |
| restart | PASS |
| roadmap multi-Epic valida | PASS |
| dependency graph | PASS |
| cross-Epic dependency | PASS |
| critical path | PASS |
| ready/block | PASS |
| WIP | PASS |
| temporary capacity suggerita | PASS |
| Unreal capacity rispettata | PASS |
| candidate lifecycle | PASS |
| test automatici | PASS — 194/194 nei tre workspace |

---

## Evidenze

- [`evidence/smoke-multi-real-home-00ec0775.log`](evidence/smoke-multi-real-home-00ec0775.log) — smoke sullo store di macchina
- [`evidence/smoke-multi-temp-home-00ec0775.log`](evidence/smoke-multi-temp-home-00ec0775.log) — smoke su store temporanea
- [`evidence/smoke-multi-post-merge-9fe49e91.log`](evidence/smoke-multi-post-merge-9fe49e91.log) — smoke rieseguito dopo il merge
- [`evidence/suite-main-00ec0775.log`](evidence/suite-main-00ec0775.log) — 194 test, verbose

⚠️ Questo referto e le evidenze sono **untracked**: non fanno parte del commit `00ec0775`,
che porta solo motore, test e roadmap. Se vanno versionati, servono un commit e una PR a
parte — non li ho infilati in quella del codice.
