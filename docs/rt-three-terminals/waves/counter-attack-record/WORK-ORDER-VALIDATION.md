# Work order VALIDATION — wave `counter-attack-record/1`

> ⛔ **Questo file è ancorato a uno SHA. Esegui il predicato qui sotto PRIMA di obbedirgli.**
> Sei il ruolo che firma: un mandato scaduto qui non fa perdere tempo, fa firmare **la build sbagliata**.

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

# 3. i due handoff in ingresso esistono?
ls docs/rt-three-terminals/waves/counter-attack-record/RT3-DEVLEAD-*.md \
   docs/rt-three-terminals/waves/counter-attack-record/RT3-EDITOR-*.md

# 4. la PR è ancora aperta, e su quale commit punta ORA?
gh pr view 2594 --json state,headRefName,headRefOid
```

Se **una** riga stampa `STALE`, o se `RT3-EDITOR-*.md` non esiste, emetti `BLOCKED — STALE MANDATE` e chiedi un mandato riemesso. Non «adattarlo».

🔴 **Il caso che questo predicato esiste per prendere è il ciclo di rientro**: se hai già emesso un `FINDING`, DEV ha corretto e tu rileggi questo file, il punto 2 è rosso e `EXPECTED_SHA` qui sotto è di un'altra build. Riverificare il commit vecchio produrrebbe un verde su codice non consegnato.

**Misurato il 2026-09-06.** Ogni numero qui viene da quel momento.

---

## 1. Input RT3

```text
PROGRAM:              reaction-combat-solid-refactor
CURRENT_WAVE:         1

FEATURE:              counter-attack-record
WAVE_ID:              counter-attack-record/1

BRANCH:               refactor/2587-counter-attack-record
PARENT_BRANCH:        main
BASE_SHA:             ee71f3e3
EXPECTED_SHA:         <<PRODUCED_SHA dichiarato dall'handoff EDITOR>>

INPUT_HANDOFF_DEV:    docs/rt-three-terminals/waves/counter-attack-record/RT3-DEVLEAD-0eeb5c1.md
INPUT_HANDOFF_EDITOR: docs/rt-three-terminals/waves/counter-attack-record/RT3-EDITOR-<sha7>.md

ISSUE:                2587   (wave 1 di 2586)
PR:                   2594
```

⚠️ `EXPECTED_SHA` è **l'unico campo lasciato aperto**, e va compilato dalla busta dell'handoff EDITOR. Lasciarlo `<<...>>` è `MISSING_INPUT`: non sostituirlo per inferenza e non assumere che valga `0eeb5c10`.

Il codice della wave è `0eeb5c10`. I commit di documentazione sopra non toccano il write-set — il controllo giusto è il punto 2 del predicato, non un confronto con `HEAD`.

## 2. Bootstrap

`AGENTS.md`, `CLAUDE.md`, `README.md`, `RT3_CONTRACT.md`, `TERMINAL_VALIDATION.md`, `WAVE_VALIDATION.md`, integralmente. Protocollo di `CLAUDE.md §2`, preflight fail-closed.

⚠️ Gli alias `rtstatus` / `rtws` / `rtsuite` **non risolvono** in una shell non interattiva: usa `scripts/rt-workspace.ps1` e `scripts/rt-suite.ps1`.

⚠️ Una `rt-suite` è valida solo se `HEAD`, albero, binario e processi sono gli stessi all'inizio e alla fine. Scrivere in `docs/` mentre gira la rende **NON REGISTRABILE**, con zero fallimenti.

## 3. 🔴 Prima misura, prima di ogni altra: separa le due cause

`BASE_SHA` contiene un commit che il suo autore dichiara non compilato:

```text
f29dd374 wip(2554): l'anteprima ticka il mondo e si inquadra da sola — NON COMPILATO
```

Tocca `SRTAnimPreviewViewport.cpp`, **nessun rapporto** con questo write-set.

**Compila `ee71f3e3` da solo prima di compilare la wave.** Senza quella misura un rosso su questo branch non è attribuibile, e attribuirlo alla wave sarebbe un `FINDING` inventato.

## 4. Gate

### B. Build

`0eeb5c10`. **Nessuno l'ha mai compilato**: DEV non occupa Unreal e l'ha dichiarato. La compilabilità per ispezione non è `PASS`.

### C. Automation

Suite intera, più il test nuovo:

```text
RefactorTactics.Reactions.Counter.TwoCountersKeepTheirOwnOrigin
./scripts/rt-suite.ps1 -Filter RefactorTactics.Reactions
```

Atteso **verde**: è caratterizzazione del comportamento corretto, non riproduzione di un difetto attivo.

🔴 **L'anti-vacuità è obbligatoria, e non è una formalità.** Il test è nuovo e non ha mai fallito: la sua capacità di discriminare è un'affermazione di DEV, non una misura.

> Scambia fra loro i `SourceCell` dei due record al momento della costruzione — `RTTurnManager.cpp`, il blocco `Out.CounterAttacks.Add(FRTCounterAttack{...})`.
> **Atteso**: `CoppiaA` e `CoppiaB` **entrambe a zero** mentre `VociContrattacco` resta `2`.

Se sotto quella mutazione il test resta verde è **muto**, e questo è un `FINDING` **indipendente** dal fatto che il refactor sia corretto. La mutazione va disfatta prima della misura finale.

### E. TurnLog / replay

Le voci `Combat` dei contrattacchi devono portare gli stessi campi di prima: `SrcCell`, `ActionId`, `BaseActionId`, `Priority` e il soggetto. **Il corpus golden non deve cambiare.**

### Determinismo

L'invariante della wave è l'**ordine di produzione** dei colpi di ritorno e la loro posizione **in coda** ad `Attacks`. `FirstCounter` continua a marcare l'inizio della coda che la voce direzionale di `#2128` itera: se quella coda cambiasse composizione, le voci direzionali leggerebbero l'origine di un altro colpo.

## 5. Sistemi in scope — colonna VALIDATION di §7

**2 ARCHITECTURE** · **3 BUILD** · **18 DAMAGE** · **21 REACTIONS** · **27 COMBAT LOG** · **28 TURNLOG/REPLAY** · **29 DETERMINISM** · **32 AUTOMATION/SCENARIO** — tutti con tetto `PASS`.

Fuori scope, dichiarati dall'handoff DEV-LEAD e **da non ampliare**: **30 NETWORK AUTHORITY** e **31 PRIVACY** (struct transiente, non serializzata, non replicata; il diff non tocca replica né proiezione degli eventi), **36 PACKAGED** (nessun contenuto distribuito cambia), **5 BLUEPRINT** (verificato da EDITOR — se il suo handoff non lo prova, è `NOT RUN`, non `N/A`).

Non ampliare lo scope per completezza. Non restringerlo per costo.

## 6. Contratto comportamentale da falsificare

1. l'ordine dei contrattacchi in `Attacks` è quello di produzione, e stanno in coda;
2. ogni contrattacco porta la propria origine, identità e autore;
3. nessun valore di danno cambia, nessuno scenario del corpus cambia numero;
4. nessun `TMap` / `TSet` introdotto sul percorso, nessuna `USTRUCT` e nessun `UObject`.

## 7. Difetti e uscita

Contratto §12 integralmente.

⛔ **Non riparare il codice di produzione e poi approvare te stesso.** Un difetto che richiede modifica produce un handoff verso DEV, e tu rivalidi una build **indipendente**. `ATTEMPT`: al terzo ciclo sullo stesso `FINDING_ID` il loop si ferma ed escala.

Matrice finale sui soli sistemi in scope, con verdetti tipizzati di §6. Poi:

```text
## P0:
## P1:
## P2:
## P3:
## USER_REQUIRED:
## EVIDENCE:

RISULTATO: DONE | PARTIAL | BLOCKED | FAILED
```

```text
docs/rt-three-terminals/waves/counter-attack-record/RT3-VALIDATION-<sha7>.md
```

`DONE` richiede la Definition of Done **viva**, riletta alla chiusura — non quella citata dagli handoff. **Nessun verdetto verde senza il campo che lo prova.**

⛔ Il merge della PR **#2594** avviene dopo il tuo sign-off, non prima.

---

## 8. Se riusi questo file

⛔ **Non copiarlo su un'altra wave modificando due campi.** È così che nasce un mandato con i buchi.

Da riderivare **tutti insieme**, o nessuno:

| campo | dove si legge |
|---|---|
| `FEATURE`, `WAVE_ID` | la cartella della wave |
| `BRANCH`, `PARENT_BRANCH` | `git branch --show-current`, `git config branch.<n>.parent` |
| `BASE_SHA`, `EXPECTED_SHA` | le buste dei due handoff in ingresso |
| `INPUT_HANDOFF_DEV`, `INPUT_HANDOFF_EDITOR` | i file `RT3-*.md` della wave |
| `ISSUE`, `PR` | GitHub, non la memoria |
| il write-set nel predicato §0 | la riga `WRITE_SET` dell'handoff |
| la mutazione di anti-vacuità §4 | il test della **nuova** wave: è specifica, non riusabile |
| sistemi in scope §5 | riderivati dal write-set con §8, **non ricopiati** |
| l'avvertenza §3 | `git log BASE_SHA` della NUOVA base: quel commit è di oggi, non è una costante |

Se non puoi riderivarne uno, quello è `MISSING_INPUT`. Non sostituirlo per inferenza.
