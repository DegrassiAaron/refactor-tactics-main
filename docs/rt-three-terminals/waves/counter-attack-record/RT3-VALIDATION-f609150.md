# RT3 VALIDATION — wave `counter-attack-record/1`

```text
ROLE:            VALIDATION
TERMINAL:        66468            (RT_TERMINAL_OWNER_PID 66468, started 2026-09-07T06:12:47Z)
WORKSPACE_ID:    DEV              (D:\Repositories\refactor-tactict-dev)
WAVE_ID:         counter-attack-record/1
ISSUE:           2587             — CLOSED
BASE_SHA:        ee71f3e3
EXPECTED_SHA:    0eeb5c10         — antenato di HEAD: sì (exit 0)
MEASURED_AT:     2026-09-07 ~06:50Z
```

> ⛔ **Nessun gate Unreal è stato eseguito.** Ogni verdetto qui sotto è `NOT RUN`. Le
> osservazioni statiche non sono convertite in `PASS`: `NOT RUN` non equivale a `PASS`.

---

## 0. Il mandato ricevuto era decaduto su due punti

Il work order di sessione descriveva uno stato del mondo che, all'ora in cui l'ho letto, era
già passato. Entrambe le divergenze sono state verificate, non dedotte.

### a. Il lease non è stale: è vivo, ed è di un altro

Il mandato prescriveva `-ReclaimStale` su un lease del pid `4520`
(`refactor-tactics-technical-designer`, preso alle `05:37:43Z`, processo morto), e aggiungeva:
«Se invece trovi un owner vivo, fermati: non è tuo».

Trovato:

```text
ENGINE LEASE: VIVO
  lease_id       : 25d2dc0c9b5f
  operation      : EDITOR
  terminal       : eatdoc-hero-profile
  workspace_id   : TECHNICAL_DESIGNER
  workspace_root : D:\Repositories\rt-wt-heroprofile
  task_id        : hero-profile
  owner_pid      : 10156          -> pwsh, vivo, avviato 08:18:59 locale
  acquired_at_utc: 2026-09-07T06:42:14Z
ENGINE       : occupato (caso: vivo)
  pid 29356  [altro checkout]     -> UnrealEditor.exe, vivo, avviato 08:42:14 locale
```

Non è il lease del mandato: è un **lease nuovo**, preso 8 minuti prima della mia lettura, da
un'altra figura, per un altro task. Il ramo `-ReclaimStale` non era applicabile.
**Fermato, come il mandato stesso prescrive.**

### b. I due bersagli sono collassati in uno

Il mandato separava:

| # | albero | commit | cosa doveva provare |
|---|--------|--------|---------------------|
| A | worktree `D:\rt-build-main` | `cefcd66e` | il fix F5 — che il test veda tutti e cinque i satelliti |
| B | `origin/main` | `f2c72…` | il sign-off che F6 aspetta |

Stato reale:

```text
$ git rev-parse origin/main
f6091507f94c7fa2d1750b14a54880121123dbfd     # non f2c720ef: tre merge più avanti

$ git merge-base --is-ancestor cefcd66e origin/main
exit 0                                        # A E' DENTRO B

$ git diff --stat cefcd66e origin/main -- \
    Source/RefactorTactics/Tests/RTDefensiveReactionTests.cpp \
    Source/RefactorTactics/Turn/RTTurnManager.cpp
(vuoto)                                       # sui file in scope, A e B sono LO STESSO CODICE
```

`f2c720ef` (#2642 lab-panel) era il tip quando il mandato è stato scritto. Sopra sono atterrati
`adde7c44` (#2609 — **proprio il fix F5**) e `f6091507` (#2644 map-template). L'intera differenza
`A -> B` sono 13 file di `Map/` e `RefactorTacticsEditor/`, estranei alla wave.

∴ **Misurare A e B separatamente non distinguerebbe più nulla**: stesso sorgente, contorno diverso.
La struttura a due bersagli del mandato non è eseguibile come scritta.

---

## 1. Il sign-off era già stato scavalcato — due volte

Il work order su disco chiude con: «⛔ Il merge della PR **#2594** avviene dopo il tuo sign-off,
non prima». Il work order di sessione ripete l'avvertimento per #2609.

```text
PR #2594   MERGED  2026-09-06T13:54:15Z   head 28bc2837
PR #2609   MERGED  2026-09-07T06:25:08Z   head cefcd66e   -> base main
issue 2587 CLOSED
```

Il merge di #2609 è avvenuto **25 minuti prima** che questa sessione leggesse il lease, e
**48 minuti dopo** lo snapshot del lease che il mandato citava come attuale.

∴ Questo referto **non è il sign-off pre-merge** che il contratto prevede: quel punto è passato.
Al più può essere una verifica di regressione post-merge — e nemmeno quella è stata eseguibile.

---

## 2. Matrice — sistemi in scope

Tetto `PASS` per tutti; nessuno raggiunto, per indisponibilità del motore.

| # | sistema | verdetto | perché |
|---|---------|----------|--------|
| 2 | ARCHITECTURE | `NOT RUN` | nessuna compilazione; vedi §3 per l'osservazione statica |
| 3 | BUILD | `NOT RUN` | `rt-build.ps1` non lanciato: editor vivo su altro checkout, mutex Live Coding globale |
| 18 | DAMAGE | `NOT RUN` | richiede Automation |
| 21 | REACTIONS | `NOT RUN` | richiede Automation |
| 27 | COMBAT LOG | `NOT RUN` | richiede Automation |
| 28 | TURNLOG/REPLAY | `NOT RUN` | richiede Automation |
| 29 | DETERMINISM | `NOT RUN` | richiede Automation |
| 32 | AUTOMATION/SCENARIO | `NOT RUN` | lease altrui, vivo |

Fuori scope, non ampliati: 30 NETWORK AUTHORITY, 31 PRIVACY, 36 PACKAGED, 5 BLUEPRINT.

**Anti-vacuità (§5 del mandato): `NOT RUN`.** Le tre mutazioni — scambio di `SourceCell`,
`ActionId`, `Actor` fra i due record — richiedono ricompilazione ed esecuzione. Non eseguite,
quindi la capacità di discriminare del test **resta un'affermazione**, esattamente come il
mandato avverte.

---

## 3. Osservazione statica — non è un gate

Riportata perché utile al prossimo giro, **non** come sostituto di una misura.

Il produttore, `RTTurnManager.cpp:4971`, è un `Add` unico con i sei campi posizionali
(`FRTAttack`, `Unit->Cell`, `ActionId`, `BaseActionId`, `Priority`, `Unit`) — la forma che
l'issue chiedeva al posto dei sei array paralleli.

`RTDefensiveReactionTests.cpp:400` asserisce i cinque satelliti **uno per riga** su entrambe le
voci, più quattro `TestNotEqual` che impongono la divergenza delle due identità, più due guardie
di premessa (id assegnati, due reazioni attivate). L'ancora di selezione è `TgtCell`, che deriva
dal colpo e non dai satelliti.

∴ Per lettura, le tre mutazioni di §5 avrebbero un'assertion che le intercetta. **Questo non prova
che diventino rosse**: è precisamente la classe di affermazione che solo la mutazione falsifica.

---

## FINDINGS

```text
FINDING_ID:   counter-attack-record/1-F7
SEVERITY:     BLOCCANTE (per questa sessione)
EVIDENCE_REF: rt-lease.ps1 -Action status @ 2026-09-07 ~06:50Z; Get-Process 10156, 29356
ROOT_CAUSE:   il mandato dichiarava il lease STALE (pid 4520, 05:37:43Z) e prescriveva
              -ReclaimStale. Alle 06:42:14Z un'altra figura (eatdoc-hero-profile,
              TECHNICAL_DESIGNER, task hero-profile) ha acquisito un lease EDITOR vivo.
              Nessun gate Unreal è eseguibile senza sottrarre il motore a una sessione attiva.
OWNER:        processo / USER
REQUIRED_FIX: riemettere il mandato quando il motore è libero, oppure autorizzare l'attesa
              (rt-suite.ps1 -WaitMinutes N). Dopo un'attesa lunga la ricompilazione è
              obbligatoria: il DLL in memoria è quello dell'altra sessione.
REGRESSION:   n/a
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F8
SEVERITY:     MAGGIORE (processo, non codice)
EVIDENCE_REF: git merge-base --is-ancestor cefcd66e origin/main -> 0;
              git diff cefcd66e origin/main -- <write-set> -> vuoto;
              origin/main = f6091507, non f2c720ef
ROOT_CAUSE:   il mandato ancorava il bersaglio B a f2c72…, ma #2609 (cefcd66e) e #2644 sono
              atterrati su main prima dell'esecuzione. A è antenato di B e i file in scope
              sono identici: i due bersagli non distinguono più nulla.
OWNER:        processo / USER
REQUIRED_FIX: un solo bersaglio — origin/main a SHA pinnato, in worktree --detach. La domanda
              «il fix funziona» e «main è firmabile» ora hanno la stessa risposta.
REGRESSION:   n/a
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F9   (conferma e chiude F6 come fatto compiuto)
SEVERITY:     MAGGIORE (processo, non codice)
EVIDENCE_REF: gh pr view — #2594 MERGED 2026-09-06T13:54:15Z; #2609 MERGED 2026-09-07T06:25:08Z;
              issue 2587 CLOSED
ROOT_CAUSE:   entrambi i work order vincolavano il merge al sign-off VALIDATION. Entrambe le PR
              sono state mergiate senza. F6 non è più prevenibile: è accaduto due volte.
OWNER:        USER
REQUIRED_FIX: decisione — o il sign-off VALIDATION è un gate reale sul merge, o il contratto
              va riscritto per dire che è una verifica post-merge di regressione. Oggi il
              documento promette la prima e il repository esegue la seconda.
REGRESSION:   il codice della wave è in main non verificato da gate Unreal
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F10
SEVERITY:     MINORE (correzione operativa al mandato)
EVIDENCE_REF: Get-Process UnrealEditor -> pid 29356, vivo, avviato 08:42:14 locale
ROOT_CAUSE:   il mandato annota «-NoHotReloadFromIDE ... Adesso non ce n'è» riferendosi a editor
              vivi. Al momento dell'esecuzione ce n'è uno, di un altro checkout. Il mutex Live
              Coding è globale sull'eseguibile: il flag è necessario, non condizionale.
OWNER:        chi riemette il mandato
REQUIRED_FIX: rendere -NoHotReloadFromIDE incondizionato nella riga di build, o far derivare la
              nota da una misura al momento dell'uso invece che dalla stesura.
REGRESSION:   una build lanciata senza il flag, con quell'editor vivo, nasce NON VALIDA
ATTEMPT:      1
```

---

## P0:
- Nessuno **sul codice**: nessun gate eseguito, quindi nessun difetto di codice osservato né escluso.
- **F7** blocca l'intera misura.

## P1:
- **F9** — il sign-off è stato scavalcato due volte; il codice della wave è in `main` senza gate Unreal.
- **F8** — la struttura del mandato non è più eseguibile come scritta.

## P2:
- **F10** — `-NoHotReloadFromIDE` da rendere incondizionato.

## P3:
- L'anti-vacuità di §5 resta il debito aperto della wave: il test è scritto per discriminare, ma
  nessuno lo ha mai visto rosso sotto mutazione.

## USER_REQUIRED:
1. **Decidere se attendere il motore.** Il lease `25d2dc0c9b5f` è vivo e non mio. Non l'ho
   reclamato. Autorizzare l'attesa, o riemettere il mandato a motore libero.
2. **Decidere cosa significa questo sign-off dopo il merge** (F9): gate reale, o verifica di
   regressione. Oggi i due documenti dicono cose diverse dai fatti.
3. **Bersaglio unico su SHA pinnato in worktree `--detach`** per il prossimo giro (F8, e F2 che
   resta valido: qui gli untracked non sopravvivono).
4. Questo referto è **untracked** nel checkout condiviso, che è su `feat/rt3-task-router` con 45
   file sporchi di un'altra sessione. Non l'ho committato: non è il mio branch. È esattamente la
   condizione che ha distrutto la prima stesura in F2.

## EVIDENCE:
Nessun artefatto in `evidence/`: **nessun gate Unreal eseguito**. Tutta l'evidenza di questo
referto è di sola lettura — `rt-lease.ps1 -Action status`, `Get-Process`, `git merge-base`,
`git diff`, `gh pr view`, `gh issue view` — ed è citata inline al punto d'uso.

Nessuna mutazione effettuata: nessun commit, nessun push, nessun merge, nessun `checkout` nel
working tree condiviso, nessun `fetch` (su richiesta esplicita: `main` non va avanzato).

---

RISULTATO: BLOCKED
NEXT_WAVE_AUTHORIZED: no

---

> ⚠️ **Superato sui gate, non sui finding.** Il motore si è liberato alle 08:08Z, il lease è stato
> acquisito (`9bde7243de40`) e la misura è stata eseguita: vedi
> [`RT3-VALIDATION-cefcd66.md`](RT3-VALIDATION-cefcd66.md) — anti-vacuità `PASS`, più F11/F12/F13/F14.
> I finding di processo di questo referto (F7 escluso, che era la sola indisponibilità del motore)
> **restano aperti**: F8 bersagli collassati, F9 merge senza sign-off, F10 flag di build.
