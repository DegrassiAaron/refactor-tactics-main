=== RT3 HANDOFF ===

FROM:          EDITOR
TO:            DEV-LEAD
FEATURE:       issue-79-combat-log-blocked-move
WAVE_ID:       issue-79-combat-log-blocked-move/1

BRANCH:        fix/79-blocked-move-turnlog  (locale; assente su `origin`)
PARENT_BRANCH: main  — dal WORK-ORDER, non verificato da questa sessione
BASE_SHA:      a59671c87a7fe3407c1d3158521280267857d20b
               ⚠️ ricevuto come placeholder `<PRODUCED_SHA_DEVLEAD>`; risolto **leggendo il
               WORK-ORDER**, non per inferenza da `HEAD`.
PRODUCED_SHA:  a59671c8 — coincide con `BASE_SHA`: EDITOR non ha scritto codice ne' asset.
               L'unica scrittura e' questo handoff, che resta **untracked**.
HEAD_AT_OPEN:  75ab6287   (sessione aperta su `main`, wave inesistente)
HEAD_AT_CLOSE: 02a48b58   (≠ BASE_SHA — vedi §2)

WRITE_SET:     docs/rt-three-terminals/waves/issue-79-combat-log-blocked-move/RT3-EDITOR-a59671c.md
BINARY_ASSETS: nessuno

STATUS: BLOCKED

---

```text
RT3 INIT

Tipo:            EDITOR
Ruolo:           RT_TERMINAL_ROLE=EDITOR, RT_WORKSPACE_ID=MAIN
Workspace:       OK: workspace MAIN  (rt-workspace.ps1 -Action verify)
Lease Unreal:    ENGINE LEASE: LIBERO — mai acquisito da questa sessione
Base SHA:        a59671c8  (dal WORK-ORDER; placeholder all'apertura)
Input handoff:   NON RISOLTO — `RT3-DEVLEAD-<sha7>.md` non esiste
HEAD:            02a48b58  (75ab6287 -> 586ad594 -> 02a48b58 durante la sessione)
Working tree:    SPORCO e in crescita — 3 file di `Source/` modificati da altra sessione
PIE:             non aperto. Editor non aperto. Nessun asset toccato.
```

⚠️ **Nessuna misura PIE e' stata tentata, e non per prudenza: non esiste un commit su cui
farla.** Il codice della #79 e' in scrittura **adesso**, non committato. Vedi §3.

---

## 1. Preflight §4 — fail-closed

`RT3_CONTRACT.md` §4 e' fail-closed sui campi obbligatori.

| Campo | Ricevuto dal mandato | Esito alla chiusura |
|---|---|---|
| `FEATURE` | `issue-79-combat-log-blocked-move` | risolto |
| `BRANCH` | `fix/79-blocked-move-turnlog` | esiste (all'apertura **non** esisteva) |
| `BASE_SHA` | `<PRODUCED_SHA_DEVLEAD>` | risolto a sessione in corso dal WORK-ORDER: `a59671c8` |
| `INPUT_HANDOFF` | `<PATH_RT3_DEVLEAD>` | 🔴 **non risolto** — il file non esiste |

```text
STATUS: BLOCKED
REASON: MISSING_INPUT
FIELDS: INPUT_HANDOFF
```

### Il WORK-ORDER non e' l'handoff che §10 richiede

Nella directory di wave esiste `WORK-ORDER.md` (310 righe, commit `ef3587c9`), emesso da
DEV-LEAD. **Non sostituisce `RT3-DEVLEAD-<sha7>.md`**: §10 nomina i tre file `RT3-*` come gli
handoff dei tre punti fissi della catena, e §9 impone alla busta campi che il work order non
porta (`PRODUCED_SHA`, `WRITE_SET`, `BINARY_ASSETS`, `STATUS`).

Misurato — sweep su tutto il repository, non solo sulla directory di wave:

```text
find . -name 'RT3-DEVLEAD*'
  -> docs/.../parsecell-arity/RT3-DEVLEAD-022977f.md
  -> docs/.../parsecell-arity/RT3-DEVLEAD-39f3ec9.md
  (nessuno per issue-79-combat-log-blocked-move)
```

Leggere lo scope dal WORK-ORDER al posto dell'handoff sarebbe la sostituzione di fonte che §10
vieta: *«il ruolo che riceve legge il file»*. Il work order **e'** un file, ma non e' quel file.

---

## 2. Precondizioni del repository §5 — HEAD non corrisponde

```text
BASE_SHA       a59671c8
HEAD           02a48b58
```

§5 e' esplicito: `BLOCKED` se `HEAD` non corrisponde a `BASE_SHA`. Qui la divergenza e' di
**tre commit**, tutti di documentazione della wave stessa:

```text
02a48b58  docs(rt3): il referto VALIDATION della wave #79 e' BLOCKED, e dice a quale stadio
586ad594  docs(rt3): toglie gli escape spuri dalle due citazioni del DoD #79
ef3587c9  docs(rt3): work order della wave issue-79-combat-log-blocked-move
c3f18b98  docs(rt3): persiste i contributi DEV-MAIN e DEV-TEST della wave #79
a59671c8  Merge pull request #2580 from DegrassiAaron/issue/2565-cr-balance   <- BASE_SHA
```

⚠️ **`a59671c8` non e' un commit prodotto dalla wave.** E' il merge della PR #2580
(`issue/2565-cr-balance`), estranea alla #79: e' la base da cui il branch e' partito. Il nome
di questo file e del referto VALIDATION lo eredita perche' §10 lega `<sha7>` a `PRODUCED_SHA`,
e nessuno dei due ruoli ha scritto codice — non perche' quel commit contenga la feature.

---

## 3. Il bersaglio si e' mosso durante la misura

`CLAUDE.md` §6 e `RT3_CONTRACT.md` §5 normano il caso: se `HEAD`, il working tree, i binari o i
processi Unreal cambiano durante una finestra di misura, **la misura e' `NON VALIDA`, non
`FAIL`**. Qui il cambiamento non e' un rischio teorico: e' misurato quattro volte.

| # | Momento | `HEAD` | `git status --short` |
|---|---|---|---|
| 1 | apertura sessione | `75ab6287` (su `main`) | pulito — la wave **non esisteva** |
| 2 | dopo `fetch` | `586ad594` | `?? RT3-VALIDATION-a59671c.md` |
| 3 | durante §2 | `02a48b58` | ` M Source/RefactorTactics/Unit/RTUnit.h` |
| 4 | chiusura | `02a48b58` | ` M RTUnit.h` ` M RTUnit.cpp` ` M RTPlayerController.cpp` |

Fra la riga 3 e la riga 4 **non e' passato un commit**: e' cresciuto il working tree. Una terza
sessione sta scrivendo il fix della #79 in questo stesso checkout, ora.

### Il codice della #79 esiste, ma non in un commit

```text
git diff --name-only a59671c8..HEAD            -> 4 file, tutti .md
git diff --name-only a59671c8..HEAD -- Source/ -> (vuoto)
grep -rn "NoteMovePlanRejection" Source/       -> RTUnit.h:408, RTUnit.cpp:1072,
                                                  RTPlayerController.cpp:1636
```

Le due misure non si contraddicono, e insieme dicono il fatto centrale di questo referto:
**il fix e' interamente non committato.** I cinque commit del branch sono `.md`; il codice —
`bMovePlanRejectedByOccupant`, `RejectedMoveDestination`, `NoteMovePlanRejection` — vive solo
nel working tree, condiviso e in scrittura attiva.

Conseguenza per questo ruolo: **non esiste uno SHA su cui aprire PIE.** Un `EVIDENCE_REF` deve
essere rileggibile da terzi (§6); un'evidenza raccolta ora citerebbe uno stato che nessun altro
puo' ricostruire e che cambia mentre lo si scrive. Non e' un `FAIL` della feature: e' l'assenza
del presupposto che rende una misura una misura.

---

## MATRICE

Le colonne assegnate a EDITOR da §7. Nessuna e' `NOT RUN` per mancanza di tempo: tutte sono
`BLOCKED`, perche' `UNBLOCK` esiste ed e' lo stesso per tutte.

| Sistema | Verdetto | REASON | UNBLOCK |
|---|---|---|---|
| PIE — caso A «resta» | `BLOCKED` | nessun commit contiene il fix; working tree in scrittura | commit del fix + `RT3-DEVLEAD-<sha7>.md` |
| PIE — caso B «blocco esplicito» | `BLOCKED` | idem | idem |
| Combat log — riga turno 4 | `BLOCKED` | idem | idem |
| Confronto con TurnLog autoritativo | `BLOCKED` | idem | idem |
| Asset / `.uasset` / `.umap` | `N/A` | write-set della wave: `BINARY_ASSETS: nessuno`, confermato — il branch non tocca binari | — |
| Privacy / network authority | `N/A` | fuori dal tetto di EDITOR per §7; di VALIDATION | — |

Nessun `PASS` e' emesso. Nessun `OBSERVED`: non ho guardato il sistema affatto.

---

## FINDINGS

### F4 — rettifica, con evidenza

Il referto VALIDATION registra:

> `F4 wrapper rtstatus/rtsuite/rtlease non risolvibili nella shell`

**La formulazione e' vera come sintomo e fuorviante come causa, e i tre wrapper non hanno la
stessa causa.** Misurato da questa sessione:

| Wrapper | Script versionato | Esito reale |
|---|---|---|
| `rtlease` | `scripts/rt-lease.ps1` | ✅ esiste — invocato per path: `ENGINE LEASE: LIBERO` |
| `rtsuite` | `scripts/rt-suite.ps1` | ✅ esiste |
| `rtws` | `scripts/rt-workspace.ps1` | ✅ esiste — invocato per path: `OK: workspace MAIN` |
| `rtstatus` | 🔴 **nessuno** | l'unico omonimo e' `Source/RefactorTactics/Tests/RTStatusTests.cpp`, un test C++ |

I primi tre **non sono irrisolvibili**: sono alias del profilo PowerShell interattivo, che una
shell non interattiva non carica. L'errore *«not recognized»* e' identico a quello di uno script
cancellato, ed e' esattamente il falso positivo che porta a concludere «tooling rimosso»
(D-181/D-182). Rimedio: invocare per path.

```powershell
& "D:\Repositories\refactor-tactics-main\scripts\rt-lease.ps1" -Action status
```

`rtstatus` e' l'unico caso in cui l'assenza e' reale, ed e' un difetto di contratto, non di
shell: `CLAUDE.md` §2 prescrive *«esegui `rtstatus` quando disponibile»* all'avvio di ogni
sessione, ma nessuno script con quel nome e' versionato.

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F4R
SEVERITY:     P3
EVIDENCE_REF: shell: rt-lease.ps1 -Action status -> "ENGINE LEASE: LIBERO"
              shell: rt-workspace.ps1 -Action verify -> "OK: workspace MAIN"
              shell: git ls-files scripts/ -> 8 file, nessuno di nome rt-status*
ROOT_CAUSE:   tre alias di profilo non caricati in shell non interattiva (falso sintomo)
              + un wrapper prescritto da CLAUDE.md §2 che non ha script versionato (difetto reale)
OWNER:        DEV-LEAD
REQUIRED_FIX: separare i due casi in F4; per `rtstatus`, decidere se creare lo script o
              togliere la prescrizione da CLAUDE.md §2
REGRESSION:   nessuna misura RT3 deve dedurre l'assenza di tooling da un alias non risolto
ATTEMPT:      1
```

### F5 — la capability EDITOR non e' il vincolo

Il mandato di questa sessione prescriveva, come esito in caso di blocco:

> `REASON: Unreal/Editor capability unavailable`

**Quella reason sarebbe falsa.** Ruolo `EDITOR`, workspace `MAIN` verificato sul registro di
macchina, lease `LIBERO` e acquisibile: l'ambiente e' pronto. Il vincolo e' a monte nella catena
(`INPUT_HANDOFF` assente) e nello stato del branch (fix non committato). Attribuire il blocco
all'ambiente avrebbe mandato DEV-LEAD a riparare una macchina che funziona.

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F5
SEVERITY:     P3
EVIDENCE_REF: shell: rt-workspace.ps1 -Action verify -> "OK: workspace MAIN"
              shell: rt-lease.ps1 -Action status -> "ENGINE LEASE: LIBERO"
ROOT_CAUSE:   il mandato prescrive una reason fissa senza misurare la capability
OWNER:        DEV-LEAD
REQUIRED_FIX: nei mandati, la reason va misurata, non prescritta
REGRESSION:   —
ATTEMPT:      1
```

### F6 — il fix vive solo nel working tree condiviso

```text
FINDING_ID:   issue-79-combat-log-blocked-move/1-F6
SEVERITY:     P2
EVIDENCE_REF: shell: git diff --name-only a59671c8..HEAD -- Source/ -> (vuoto)
              shell: git status --short -> M RTUnit.h, M RTUnit.cpp, M RTPlayerController.cpp
              shell: grep -rn NoteMovePlanRejection Source/ -> 3 hit in Source/
ROOT_CAUSE:   DEV scrive il fix nel checkout MAIN condiviso; nessun commit lo contiene
OWNER:        DEV-LEAD
REQUIRED_FIX: committare il fix e dichiararne lo SHA nell'handoff DEV-LEAD, prima di
              convocare EDITOR o VALIDATION
REGRESSION:   il gate d'ingresso di EDITOR deve verificare che `git diff BASE..HEAD -- Source/`
              non sia vuoto quando la wave dichiara un fix di codice
ATTEMPT:      1
```

⚠️ Corollario per chi committa: questo handoff e' **untracked** in un working tree che contiene
il fix di un'altra sessione. Un `git add -A` lo assorbirebbe nel commit del fix. Committarlo
separatamente, oppure lasciarlo a DEV-LEAD.

---

## EVIDENCE

```text
shell: rt-workspace.ps1 -Action verify            -> "OK: workspace MAIN"
shell: rt-lease.ps1 -Action status                -> "ENGINE LEASE: LIBERO"
shell: git rev-parse --short HEAD                 -> 75ab6287 / 586ad594 / 02a48b58 (tre letture)
shell: git branch --show-current                  -> fix/79-blocked-move-turnlog
shell: git status --short                         -> 4 letture, working tree in crescita (§3)
shell: git diff --name-only a59671c8..HEAD        -> 4 file, tutti .md
shell: git diff --name-only a59671c8..HEAD -- Source/ -> (vuoto)
shell: git log -1 --format=%s a59671c8            -> "Merge pull request #2580 ... issue/2565-cr-balance"
shell: find . -name 'RT3-DEVLEAD*'                -> solo parsecell-arity (2 file)
shell: git ls-files scripts/                      -> 8 file .ps1, nessun rt-status*
shell: grep -rn NoteMovePlanRejection Source/     -> RTUnit.h:408, RTUnit.cpp:1072,
                                                     RTPlayerController.cpp:1636
gh:    gh issue view 79                           -> OPEN, "CP 11.3 — Combat log con reason code
                                                     completi", milestone "v0.1 · Leggibilita'"
```

Nessun `EVIDENCE_REF` di build, suite, PIE o packaged: **non ne esistono**, e questa sessione
non ne ha prodotti.

---

## USER_REQUIRED

Nessun check a oracolo umano viene aggiunto da EDITOR. I tre gia' registrati da VALIDATION
(`PIE-V01-LOG`, `PIE-V01-COLL` clausola (c), riga combat log turno 4) restano `NOT RUN` e
diventano eseguibili quando l'`UNBLOCK` di questo referto e' soddisfatto.

---

## UNBLOCK

Nell'ordine, e tutti necessari:

1. **DEV committa il fix.** `git diff BASE..HEAD -- Source/` deve smettere di essere vuoto.
2. **DEV-LEAD emette `RT3-DEVLEAD-<sha7>.md`** con `<sha7>` = SHA del commit di cui al punto 1,
   busta §9 completa e sistemi in scope derivati dal write-set.
3. **La suite Automation gira su quel commit**, con working tree pulito — altrimenti la misura
   e' `NON VALIDA` per lo stesso motivo di §3.
4. Rilancio di EDITOR con `BASE_SHA` e `INPUT_HANDOFF` **risolti a valori reali**.

Solo allora la finestra PIE e' apribile, e i due casi della #79 (A «resta» / B «blocco
esplicito») diventano osservabili contro un TurnLog che qualcun altro puo' rileggere.
