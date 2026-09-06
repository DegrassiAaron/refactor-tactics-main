```text
=== RT3 HANDOFF ===

FROM:          VALIDATION
TO:            EDITOR
FEATURE:       counter-attack-record
WAVE_ID:       counter-attack-record/1

BRANCH:        refactor/2587-counter-attack-record
PARENT_BRANCH: main
BASE_SHA:      ee71f3e3
OBSERVED_HEAD: 0c06a891
PRODUCED_SHA:  0c06a891   (VALIDATION non committa: invariato)

STATUS: BLOCKED
REASON: MISSING_INPUT — EXPECTED_SHA e INPUT_HANDOFF_EDITOR non risolti
UNBLOCK: (a) EDITOR emette RT3-EDITOR-<sha7>.md, oppure
         (b) decisione §9 che autorizzi DEV-LEAD -> VALIDATION per questa wave
```

Issue: [#2587](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2587) — wave 1 di [#2586](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2586). PR #2594.

---

## PREFLIGHT — fail-closed, due mismatch

**B1 — `EXPECTED_SHA` non risolvibile.** `INPUT_HANDOFF_EDITOR` punta a
`RT3-EDITOR-<sha7>.md`: `<sha7>` è un placeholder e il file non esiste. La cartella di wave
contiene il solo `RT3-DEVLEAD-0eeb5c1.md`. Per `RT3_CONTRACT.md` §4 un `INPUT_HANDOFF` deve
essere «path a un file esistente»: è `MISSING_INPUT`.

Non è una lacuna di forma. `RT3-DEVLEAD-0eeb5c1.md` §PROSSIMO RUOLO lo dichiara esplicitamente:

> «Non ho trovato nel contratto una regola che consenta di saltarlo: se per questa wave si vuole
> `DEV-LEAD → VALIDATION`, è una decisione da prendere in §9, **non un'inferenza**.»

Concludere che EDITOR sia saltabile perché `BINARY_ASSETS: nessuno` sarebbe esattamente
l'inferenza che il ruolo a monte vieta. VALIDATION non possiede quella decisione.

**B2 — `HEAD` non dichiarato.** L'INPUT dichiara `0eeb5c10` (codice) e `ec370024` (handoff).
`HEAD` è `0c06a891` — «rt-mcp», che tocca `scripts/rt-mcp-server.ps1`, fuori dal `WRITE_SET`.

Sostanzialmente inerte per i gate (un solo `.ps1`, nessun C++, nessun asset), formalmente non
dichiarato: la cosa misurabile non coincide con la cosa dichiarata. Va dichiarato o rimosso
dal perimetro, non ignorato.

---

## GATE UNREAL — nessuno eseguito

`RT3_CONTRACT.md` §4 su `MISSING_INPUT`: «Poi fermati. […] Non avviare Unreal.»
Nessun lease acquisito, nessuna build, nessuna suite.

| gate | verdetto | motivo |
|---|---|---|
| baseline `ee71f3e3` da solo | `NOT RUN` | preflight fail-closed |
| BUILD `0eeb5c10` | `NOT RUN` | idem |
| AUTOMATION `RefactorTactics.Reactions` | `NOT RUN` | idem |
| anti-vacuità (scambio `SourceCell`) | `NOT RUN` | idem |
| TURNLOG/REPLAY, DETERMINISM | `NOT RUN` | idem |

⚠️ La misura §1 del mandato (compilare `ee71f3e3` da solo per separare le cause da
`f29dd374 — NON COMPILATO`) **resta da fare** e non dipende da B1/B2: è il primo atto del
prossimo giro, chiunque lo esegua.

---

## MISURATO — statico, attribuibile a SHA dichiarati

Nessuna di queste è un `PASS` di §7: sono controlli statici sul diff, non prove di comportamento.

| controllo | esito | evidenza |
|---|---|---|
| `WRITE_SET` = dichiarato | conforme, 4 file | `git show --stat 0eeb5c10` |
| contratto §4.4 — nessun `TMap`/`TSet` introdotto | conforme | zero righe `+` con `TMap`/`TSet` nei due file di produzione |
| contratto §4.1 — ordine di produzione | strutturalmente coerente | i sei `Append` paralleli diventano un unico `for (const FRTCounterAttack& Counter : ...)` che accoda in sequenza |
| contratto §4.2 — origine/identità/autore per record | strutturalmente presente | `FRTCounterAttack{ …, SourceCell, ActionId, BaseActionId, Priority }` |
| `ec370024` non tocca il write-set | confermato | tocca solo `docs/` |

⚠️ §4.1 e §4.2 sono verificati **per costruzione**, non per esecuzione. Restano `NOT RUN`
come verdetti: l'ordine lo prova la suite, non la lettura del ciclo.

---

## FINDINGS

```text
FINDING_ID:   counter-attack-record/1-F1
SEVERITY:     BLOCCANTE
EVIDENCE_REF: docs/rt-three-terminals/waves/counter-attack-record/ @ 0c06a891 — nessun RT3-EDITOR-*.md
ROOT_CAUSE:   catena canonica interrotta; EXPECTED_SHA deriva da un handoff mai emesso
OWNER:        EDITOR (o decisione §9)
REQUIRED_FIX: emettere l'handoff EDITOR, anche di soli verdetti OBSERVED; oppure decidere in §9
REGRESSION:   n/a
ATTEMPT:      1
```

```text
FINDING_ID:   counter-attack-record/1-F2
SEVERITY:     MINORE
EVIDENCE_REF: git log ee71f3e3..HEAD @ 0c06a891
ROOT_CAUSE:   0c06a891 «rt-mcp» è su HEAD del branch di wave ma non è dichiarato nell'INPUT RT3
OWNER:        DEV-LEAD
REQUIRED_FIX: dichiararlo nel perimetro oppure spostarlo fuori dal branch di wave
REGRESSION:   nessuna attesa (solo scripts/rt-mcp-server.ps1)
ATTEMPT:      1
```

---

## USER_REQUIRED

1. Decidere B1: far girare EDITOR, oppure registrare in §9 la catena `DEV-LEAD → VALIDATION`
   per le wave con `BINARY_ASSETS: nessuno`.
2. Dichiarare o rimuovere `0c06a891`.
3. ⚠️ La working directory condivisa è volatile: `HEAD` si è mosso tre volte durante questa
   sessione (`9da33c59 → 1b0c3223 → ee71f3e3 → 0c06a891`) e l'albero è stato sporco sui file
   di WAVE 2 mentre misuravo. I gate Unreal del prossimo giro vanno eseguiti in un worktree
   `--detach` su SHA pinnato, o la misura nasce `NON VALIDA`.

## EVIDENCE

Questo file non è committato. Nessun artefatto in `evidence/`: nessun gate Unreal eseguito.

---

RISULTATO: BLOCKED
NEXT_WAVE_AUTHORIZED: no
