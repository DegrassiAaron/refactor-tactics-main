```text
=== RT3 HANDOFF ===

FROM:          EDITOR
TO:            VALIDATION
FEATURE:       counter-attack-record
WAVE_ID:       counter-attack-record/1

BRANCH:        refactor/2587-counter-attack-record
PARENT_BRANCH: main
BASE_SHA:      0eeb5c10
PRODUCED_SHA:  0eeb5c10

WRITE_SET:     Source/RefactorTactics/Turn/RTReactionPassResult.h
               Source/RefactorTactics/Turn/RTTurnManager.cpp
               Source/RefactorTactics/Tests/RTDefensiveReactionTests.cpp
               Source/RefactorTactics/Tests/RTReactionTests.cpp
BINARY_ASSETS: nessuno

STATUS: READY
```

`BASE_SHA` è il commit ereditato in ingresso — il `PRODUCED_SHA` del DEV-LEAD — non `ee71f3e3`,
che è la base della PR. `PRODUCED_SHA = BASE_SHA`: questo ruolo non ha scritto codice né binari.

**Misurato su `HEAD = 12342a91`**, che porta in più solo commit di documentazione. Il predicato di
validità del work order è stato eseguito e passa su tutti e quattro i controlli: `0eeb5c10` è
antenato di HEAD, il write-set è **byte-identico** fra `0eeb5c10` e HEAD, l'handoff di ingresso
esiste, e la PR [#2594](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2594) è `OPEN`
con `headRefOid = 12342a91`.

`STATUS: READY` significa che ogni sistema in scope porta un verdetto ben formato. Non significa
che la wave sia verificata: cinque delle otto voci sono `NOT RUN` e appartengono a VALIDATION.

---

## MATRICE

Sistemi in scope ereditati dallo scoping §8 del DEV-LEAD, più `ASSETS` e `BLUEPRINT` che entrano
per §8 punto 3: rimuovere sei simboli è una modifica a valle della quale un riferimento da asset
si romperebbe. È la domanda che il work order assegna a questo ruolo.

| # | Sistema | Tetto §7 | Verdetto | Campi |
|---:|---|---|---|---|
| 2 | ARCHITECTURE | `OBSERVED` | `OBSERVED` | `EVIDENCE_REF:` evidence/RT3-EDITOR-reflection-0eeb5c1.md §4 |
| 3 | BUILD | `OBSERVED` | `NOT RUN` | `REASON:` nessuna compilazione eseguita; il ruolo non ha occupato il motore |
| 4 | ASSETS | `PASS` | `PASS` | `EVIDENCE_REF:` evidence/RT3-EDITOR-reflection-0eeb5c1.md §1-§3 |
| 5 | BLUEPRINT | `PASS` | `PASS` | `EVIDENCE_REF:` evidence/RT3-EDITOR-reflection-0eeb5c1.md §2-§4 |
| 18 | DAMAGE | `PASS` | `NOT RUN` | `REASON:` provarlo richiede esecuzione (suite o PIE); nessuna eseguita |
| 21 | REACTIONS | `PASS` | `NOT RUN` | `REASON:` idem — è il sistema diretto della wave e il suo gate è la suite |
| 27 | COMBAT LOG | `PASS` | `NOT RUN` | `REASON:` idem; la voce `Combat` si legge da un TurnLog prodotto a runtime |
| 28 | TURNLOG/REPLAY | `OBSERVED` | `NOT RUN` | `REASON:` nessuna traccia prodotta né confrontata |
| 29 | DETERMINISM | `OBSERVED` | `NOT RUN` | `REASON:` nessuna ripetizione eseguita |
| 32 | AUTOMATION/SCENARIO | `OBSERVED` | `NOT RUN` | `REASON:` `rt-suite` occupa il motore — vietata in finestra EDITOR, dominio VALIDATION |

⚠️ `DAMAGE`, `REACTIONS` e `COMBAT LOG` hanno tetto `PASS` per questo ruolo, e restano `NOT RUN`.
Il tetto dice cosa un ruolo **potrebbe** emettere avendo l'evidenza, non cosa deve emettere senza.
Un `PASS` qui sarebbe malformato per §6 e si leggerebbe `BLOCKED`.

### Cosa questo ruolo ha effettivamente provato

Le tre domande del work order §4, tutte e tre con esito **negativo atteso**:

1. **`FRTCounterAttack` non è riflesso.** Nessun `USTRUCT`, `GENERATED_BODY` o `UPROPERTY` in
   `RTReactionPassResult.h`; l'header lo dichiara a riga 38.
2. **I sei simboli rimossi non sono referenziati da alcun asset.** `CounterAttackSrc`,
   `CounterActionId`, `CounterBaseActionId`, `CounterPriority`, `CounterAttackActors` e
   `CounterAttacks` → 0 su 123 asset tracciati, match esatto e sottostringa.
3. **Nessun Blueprint tocca `FRTReactionPassResult`.** Il tipo e tutti i suoi membri → 0.

Nessuno dei tre risulta falso: **nessun Finding**, e la premessa `5 BLUEPRINT — fuori scope` del
DEV-LEAD è confermata da misura invece che da inferenza.

## FINDINGS

Nessuno. Nessuna regressione osservabile da questo ruolo, e i tre controlli sopra sono tutti
conformi al contratto comportamentale dell'handoff in ingresso.

## EVIDENCE

```text
asset:  docs/rt-three-terminals/waves/counter-attack-record/evidence/RT3-EDITOR-reflection-0eeb5c1.md@12342a91
asset:  Source/RefactorTactics/Turn/RTReactionPassResult.h@0eeb5c10  (0 GENERATED_BODY, 0 UPROPERTY)
suite:  git diff --quiet 0eeb5c10 12342a91 -- <write-set> -> exit 0 (write-set invariato)
suite:  gh pr view 2594 --json state,headRefName,headRefOid -> OPEN, 12342a918a3e…
```

## USER_REQUIRED

Nessuno, e la ragione è misurata, non assunta: il write-set non tocca `.uasset`, `.umap`,
Blueprint, UMG, animazione o UI — verificato per path esplicito su tutti i commit del branch
(`git diff --name-only ee71f3e3..HEAD | grep -iE '\.uasset|\.umap'` → vuoto). Non esiste una
domanda che solo una persona davanti al gioco possa rispondere. Inventare un check visuale qui
produrrebbe evidenza che non risponde a nulla.

---

## OSSERVAZIONI — non sono verdetti

Registrate perché il ruolo successivo le incontrerà, non perché siano difetti del prodotto.

**1. Il work order dichiarava impossibile la misura che questo handoff porta.** «La ricerca sulla
name table dei `.uasset` è stata scartata in calibrazione — cinque simboli certamente usati hanno
risposto zero, quindi lo strumento non discrimina». La calibrazione non era omologa al bersaglio:
misurava nomi di package e di classe, che vivono nella import table, per concludere su nomi di
funzione invocata. Calibrando sulla specie giusta lo strumento discrimina — 67 UFUNCTION su 455 e
111 `CallFunc_*` — e la domanda si chiude senza aprire l'Editor. Dettaglio in evidence §0.

**2. Il branch porta un commit fuori write-set con messaggio non convenzionale.**
`0c06a891 "rt-mcp"` aggiunge `scripts/rt-mcp-server.ps1` (141 righe) fra l'handoff DEV-LEAD e
questo. Non tocca il write-set e non invalida il predicato, ma non è dichiarato da nessun handoff
della wave e non segue `<type>(<scope>): <description>`. Owner da attribuire.

**3. HEAD si è mosso durante la sessione**, `0c06a891` → `12342a91`, e per un momento il working
tree ha contenuto un `RT3-VALIDATION-0c06a89.md` untracked poi sparito. Il working tree è
condiviso e un'altra sessione lavora questa wave in parallelo. La misura qui riportata è
interamente contenuta in `12342a91` — HEAD riletto prima e dopo la finestra, invariato — quindi è
valida per §5. Una misura di VALIDATION che attraversi un cambio di HEAD sarebbe invece
`NON VALIDA`, non `FAIL`.

**4. Avvertenza sulla base, propagata dal DEV-LEAD.** `ee71f3e3` contiene
`f29dd374 wip(2554): … NON COMPILATO` su `SRTAnimPreviewViewport.cpp`, modulo Editor. Se la build
fallisce, il primo sospetto non è questo write-set: compilare `ee71f3e3` da solo separa le cause.
Non verificato qui — `BUILD` è `NOT RUN`.

## PROSSIMO RUOLO

`VALIDATION`. Restano da provare i cinque sistemi `NOT RUN` e l'anti-vacuità del test nuovo
`RefactorTactics.Reactions.Counter.TwoCountersKeepTheirOwnOrigin`, che il DEV-LEAD ha specificato:
scambiare i `SourceCell` dei due record e attendersi `CoppiaA` e `CoppiaB` a zero con
`VociContrattacco` invariato a `2`. Un test nuovo che non ha mai fallito non ha ancora dimostrato
di discriminare.
