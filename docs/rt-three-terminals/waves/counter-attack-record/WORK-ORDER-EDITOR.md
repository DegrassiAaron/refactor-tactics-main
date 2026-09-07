# Work order EDITOR — wave `counter-attack-record/1`

> ⛔ **Questo file è ancorato a uno SHA. Esegui il predicato qui sotto PRIMA di obbedirgli.**
> Un mandato che parla di un commit che non è più quello consegnato è `STALE`, non «un po' vecchio».

## 0. Predicato di validità — fail-closed

```bash
# 1. il codice della wave è nell'albero che ho davanti?
git merge-base --is-ancestor 0eeb5c10 HEAD   || echo "STALE: 0eeb5c10 non è un antenato di HEAD"

# 2. qualcuno ha toccato il write-set DOPO il commit del codice?
git diff --quiet 0eeb5c10 HEAD -- \
  Source/RefactorTactics/Turn/RTReactionPassResult.h \
  Source/RefactorTactics/Turn/RTTurnManager.cpp \
  Source/RefactorTactics/Tests/RTDefensiveReactionTests.cpp \
  Source/RefactorTactics/Tests/RTReactionTests.cpp \
  || echo "STALE: il write-set è cambiato dopo 0eeb5c10 — questo mandato descrive un altro codice"

# 3. l'handoff in ingresso esiste?
test -f docs/rt-three-terminals/waves/counter-attack-record/RT3-DEVLEAD-0eeb5c1.md \
  || echo "STALE: handoff di ingresso assente"

# 4. la PR è ancora aperta sullo stesso branch?
gh pr view 2594 --json state,headRefName,headRefOid
```

Se **una** riga stampa `STALE`, non procedere: emetti `BLOCKED — STALE MANDATE`, dichiara quale controllo è caduto, e chiedi un mandato riemesso.

⚠️ Il punto 2 è quello che conta, ed è scritto sul **write-set** e non su `HEAD` di proposito: commit di sola documentazione sopra il codice sono normali in questa wave e non invalidano nulla.

**Misurato il 2026-09-06.** Ogni numero qui viene da quel momento.

---

## 1. Input RT3

```text
PROGRAM:        reaction-combat-solid-refactor
CURRENT_WAVE:   1

FEATURE:        counter-attack-record
WAVE_ID:        counter-attack-record/1

BRANCH:         refactor/2587-counter-attack-record
PARENT_BRANCH:  main
BASE_SHA:       ee71f3e3
INPUT_HANDOFF:  docs/rt-three-terminals/waves/counter-attack-record/RT3-DEVLEAD-0eeb5c1.md

ISSUE:          2587   (wave 1 di 2586)
PR:             2594
```

Assegnazione esplicita di ruolo. **Non dedurre il ruolo dal workspace.**

⚠️ **`PRODUCED_SHA` è `0eeb5c10` e NON è `HEAD`.** `HEAD` porta in più i commit di documentazione di questa wave, che non toccano il write-set. È la stessa forma del precedente `parsecell-arity`, dove `RT3-DEVLEAD-022977f.md` dichiara `022977fd` ed è committato in `cdcf1dad`. **Non è il mismatch di §9**: il predicato al punto 2 è il controllo giusto.

## 2. Bootstrap

Leggi integralmente `AGENTS.md`, `CLAUDE.md`, `docs/rt-three-terminals/README.md`, `RT3_CONTRACT.md`, `TERMINAL_EDITOR.md`, `WAVE_EDITOR.md`. Esegui il protocollo di `CLAUDE.md §2` e il preflight fail-closed di RT3.

⚠️ Gli alias `rtstatus` / `rtws` / `rtlease` **non risolvono** in una shell non interattiva. Gli script vivono in `scripts/rt-workspace.ps1`, `scripts/rt-lease.ps1`, `scripts/rt-suite.ps1`: usa i path.

## 3. Cosa questa wave NON ti chiede

`BINARY_ASSETS: nessuno`. Il write-set è quattro file C++.

- ⛔ **Nessun authoring.** Non c'è un asset da toccare, e non va inventato.
- ⛔ **`MCP_ASSET_WRITE` non è autorizzato**: il preflight richiede un write-set asset dichiarato, e non ce n'è uno. Restano consentite ispezione e query read-only.
- ⛔ **Nessun PIE.** Il diff non cambia nulla di osservabile a schermo. Aprire una sessione per cercare qualcosa da guardare produce un'evidenza che non risponde a nessuna domanda.

## 4. Il valore che SOLO tu puoi produrre

DEV ha dichiarato una cosa che **non poteva misurare**, e l'ha scritto nell'handoff: se un Blueprint o un asset dipenda dai simboli toccati. La ricerca sulla name table dei `.uasset` è stata scartata **in calibrazione** — cinque simboli certamente usati hanno risposto zero, quindi lo strumento non discrimina e il suo silenzio non prova niente.

Tu hai l'Editor. Sono le sezioni **A (Reflection C++)** e **D (Content Browser)** di `WAVE_EDITOR.md`, e sono il contributo reale di questo passaggio:

1. **`FRTCounterAttack` non è riflesso.** Non è una `USTRUCT`: non deve comparire in reflection né essere referenziabile da Blueprint. È la premessa su cui la wave ha scelto un value object semplice.
2. **I sei simboli rimossi non sono referenziati da alcun asset** — `CounterAttackSrc`, `CounterActionId`, `CounterBaseActionId`, `CounterPriority`, `CounterAttackActors`, e il vecchio tipo di `CounterAttacks`.
3. **Nessun Blueprint chiama membri di `FRTReactionPassResult`.**

Se uno dei tre risulta falso è un `FINDING` e la wave torna a DEV. **Non ripararlo tu**: §6 vieta di correggere e approvare sé stessi.

## 5. Sistemi in scope e tetto di §7

| # | Sistema | EDITOR max |
|---:|---|---|
| 2 | ARCHITECTURE | `OBSERVED` |
| 3 | BUILD | `OBSERVED` |
| 18 | DAMAGE | `PASS` |
| 21 | REACTIONS | `PASS` |
| 27 | COMBAT LOG | `PASS` |
| 28 | TURNLOG/REPLAY | `OBSERVED` |
| 29 | DETERMINISM | `OBSERVED` |
| 32 | AUTOMATION/SCENARIO | `OBSERVED` |

Un `PASS` senza `EVIDENCE_REF` **non è ben formato**. Se non hai l'evidenza il verdetto è `OBSERVED` o `NOT RUN`, non un `PASS` gentile.

## 6. 🔴 Avvertenza sulla base

`BASE_SHA = ee71f3e3` contiene un commit che il suo autore dichiara non compilato:

```text
f29dd374 wip(2554): l'anteprima ticka il mondo e si inquadra da sola — NON COMPILATO
```

Tocca `SRTAnimPreviewViewport.cpp`, cioè il **modulo Editor**, e non ha rapporto con questo write-set. Se l'Editor non si apre o la build fallisce, registralo come osservazione: **non attribuirlo alla wave**.

## 7. Uscita

Handoff nel formato §9, su file secondo §10:

```text
docs/rt-three-terminals/waves/counter-attack-record/RT3-EDITOR-<sha7>.md
```

`FROM: EDITOR` · `TO: VALIDATION` · `BINARY_ASSETS` per path esplicito (atteso: `nessuno`) · `PRODUCED_SHA = BASE_SHA` se non hai scritto.

Se concludi che la wave non ha materia EDITOR, **scrivilo con l'evidenza dei tre controlli di §4**: è un risultato, non un passaggio a vuoto. Non emettere `STATUS: READY` con verdetti che non hai prodotto.

---

## 8. Se riusi questo file

⛔ **Non copiarlo su un'altra wave modificando due campi.** È così che nasce un mandato con i buchi.

Da riderivare **tutti insieme**, o nessuno:

| campo | dove si legge |
|---|---|
| `FEATURE`, `WAVE_ID` | la cartella della wave |
| `BRANCH`, `PARENT_BRANCH` | `git branch --show-current`, `git config branch.<n>.parent` |
| `BASE_SHA`, `PRODUCED_SHA` | la busta dell'handoff DEV-LEAD della wave |
| `INPUT_HANDOFF` | il file `RT3-DEVLEAD-<sha7>.md` della wave |
| `ISSUE`, `PR` | GitHub, non la memoria |
| il write-set nel predicato §0 | la riga `WRITE_SET` dell'handoff |
| sistemi in scope §5 | riderivati dal write-set con §8, **non ricopiati** |
| l'avvertenza §6 | `git log BASE_SHA` della NUOVA base: quel commit è di oggi, non è una costante |

Se non puoi riderivarne uno, quello è `MISSING_INPUT`. Non sostituirlo per inferenza.
