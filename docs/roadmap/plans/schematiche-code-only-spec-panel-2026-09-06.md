# «Roadmap Schematiche: Code-Only» — spec panel

> `CURRENT` · **Stato**: revisione chiusa. Il sorgente è **recensito, non applicato**: nessuna delle
> quindici voci ha prodotto una issue · **Data**: 2026-09-06
> **HEAD della misura**: `2a08f4c2`, **riallineato a `1b4ff858`** — `origin/main` si è mosso durante la
> run, e il branch del checkout condiviso è cambiato **due volte** sotto di essa
> (`feat/2491-icone-branth` → `fix/2519-icone-action-dash-orfane`): la scrittura è avvenuta nel worktree
> isolato `D:/rt-wt-2533`, mai nel working tree condiviso.
> **Oggetto**: il work order esterno *«RefactorTactics — Roadmap Schematiche: Code-Only»*, fornito in chat
> **senza sha né data**, che chiede di coprire con test headless quindici contratti `S01…S15` attraverso
> undici checkpoint `C0…C10`, su sette branch con una PR per checkpoint.
> **Panel**: Wiegers (lead) · Fowler · Nygard · Crispin · Adzic · Cockburn
> **Modo**: critique · **Focus**: requirements, architecture, testing
> **Tracker**: [#2533](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2533), che porta
> l'anti-duplicazione voce per voce e resta l'owner di quella misura.
> ⚠️ **Il documento non è nel repository e non entra**: è una consegna effimera — [`AGENTS.md`](../../../AGENTS.md) §8.
> 🔑 **Nessun numero qui è ricordato.** Ogni conteggio porta il comando che l'ha prodotto, in §3.
> 👥 **Gemello**: questo kit è la metà `CODE` della coppia la cui metà `EDITOR` è già recensita in
> [`schematiche-editor-execution-spec-panel-2026-09-06.md`](schematiche-editor-execution-spec-panel-2026-09-06.md).
> Ciò che quel referto ha già stabilito **non si ripete qui**: si cita e si estende — §2.

---

## 1. Il verdetto in una riga

> **Il kit è disciplinato e chiede lavoro che è già stato fatto.** La sua tesi — *la simulazione decide, il
> test lo dimostra senza Editor* — è corretta e il repository la applica da tempo: **2110** automation test
> la presidiano. Ma delle quindici voci nessuna giustifica una issue nuova, un intero checkpoint è
> implementato **mutazione compresa**, due dei blocchi che dichiara sono decisi da giorni, e la tassonomia
> `S01…S15` su cui fonda il proprio DoD non è del repository.

| | Rilievo | Gravità |
|---|---|---|
| **R1** | **`C8` non è lavoro: è lavoro fatto.** Le nove voci del §13 hanno tutte un owner, una porta lo **stesso nome** (`Overwatch.OpportunityLeaksNoFuture`), e la «mutation obbligatoria» del kit esiste già come **test permanente** | 🔴 |
| **R2** | **Due blocchi fantasma.** `MOV-4` è chiusa dal 2026-09-04 (`D-325`) e la semantica `Deflect` dal 2026-09-01 ([#1918](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1918)). Un blocco che non esiste non è inerte: fa evitare lavoro legittimo | 🔴 |
| **R3** | **Il §12 vieta una tassonomia che è già canone.** `ERTIntentCertainty { Confirmed, Predicted, Uncertain }` esiste in `Turn/RTIntentPrivacyLibrary.h:21`, popolato da `ClassifyPlan` e pinnato da un test | 🔴 |
| **R4** | **Il §20 chiede sette branch paralleli** e non nomina mai `scripts/rt-suite.ps1` né la validità della misura: [`AGENTS.md`](../../../AGENTS.md) §11 dice che *«un worktree separato non elimina il mutex globale Unreal/Live Coding»* | 🟠 |
| **R5** | **Il §15 ha una riga che contraddice la propria intestazione** — sotto *«Deve cadere»*, `ordine input movement` → `nessuna divergenza` — e lascia il commit `test(determinism)` **senza artefatto di prova** | 🟠 |
| **R6** | **Il DoD è auto-verificante in due punti**: *«S01–S13 e S15 hanno test dove il contratto è deciso»* si soddisfa dichiarando non deciso ciò che non ha test; `C0` chiede di *«misurare test dichiarati/eseguiti»* senza soglia | 🟠 |
| **R7** | **`S05` ha il suo test, e quel test pinna l'arretrato.** `Action.Sprint` risolve **pre-Blast** per debito dichiarato — la divergenza che il gemello ha misurato in `R4` e per cui propone `Oracle: CANON \| OBSERVED` | 🟠 |
| **R8** | Il contratto di fase del §6 elenca **`Blink`**, che nel codice non esiste | 🔵 |
| **R9** | Il §1 vieta i `.uasset` e il §18 dice *«nessun `.uasset`»*: ambiguo fra *non modificarli* e *non dipenderne*, mentre test headless esistenti li **leggono** | 🔵 |

**Cosa si salva**: tre scelte corrette, che vanno conservate in qualunque riscrittura — §5.

**Raccomandazione del panel**: **non creare le quindici issue.** Il §4 non è un mapping ma un indice di
lettura: va riagganciato agli owner reali, che esistono tutti.

---

## 2. Cosa il gemello ha già stabilito

Il referto della metà `EDITOR` è stato scritto sullo **stesso** `2a08f4c2` e mergiato alle 01:18 di oggi.
Quattro dei suoi rilievi valgono per entrambe le metà e **non si rimisurano qui**:

| Suo | Cosa ha stabilito | Effetto su questa metà |
|---|---|---|
| `R3` | l'oggetto non esiste: zero file `*schemati*` e zero `*code-only*` in entrambi i checkout | il §4 e il §18 di questo kit non sono verificabili da nessuno |
| `R7` | **gli owner delle `S` sono issue**, e quattro su cinque sono chiuse — `#2277` (`S01`,`S05`) · `#1918` (`S08`) · `#589` (`S15`) · `MOV-4` (`S03`,`S09`), contro il solo [#2148](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2148) ancora `OPEN` | `R2` di questo referto è la stessa misura vista dal lato codice |
| `R8` | `S01…S15` è un **terzo spazio di identificatori** accanto a 48 sedute `U*` e 226 voci `PIE-*`, senza una mappa fra i tre | qui il difetto è più stretto: nessuna `S` dichiara quale **test** pinni |
| `R4` | `Action.Sprint` è profilo di `Move` per il canone e risolve **pre-Blast** nel codice — arretrato `#641`/`D-116` | è `R7` di questo referto |

🔑 **La coppia si conferma da sola.** Due kit della stessa forma, consegnati insieme, misurati sullo stesso
sha da due run indipendenti, producono la stessa lista di premesse decadute. Il difetto non è di una
stesura: è del **canale**, che è precisamente ciò che
[`gov4-contratto-work-order-esterni-2026-09-05.md`](gov4-contratto-work-order-esterni-2026-09-05.md) ha
deciso il giorno prima con `D-336`.

---

## 3. Come è stato misurato

```bash
# la tassonomia
grep -rliE "schematich|schematic" --include=*.md --include=*.txt --include=*.json --include=*.yaml \
  --exclude-dir=.git --exclude-dir=Intermediate --exclude-dir=Saved --exclude-dir=DerivedDataCache \
  --exclude-dir=Binaries --exclude-dir=Content D:/Repositories/                        # -> 0

# le issue: corpi scaricati e cercati in locale, non `--search` (indice asincrono)
gh issue list --state all --limit 3000 --json number,title,body,state                  # -> 951
#   pattern \bS(0[1-9]|1[0-5])\b nei titoli                                            # -> 0
#   nei corpi                                                                          # -> 2, entrambi altri corpus

# la baseline dei test, su origin/main e non sul branch del checkout condiviso
git grep -c "IMPLEMENT_SIMPLE_AUTOMATION_TEST" origin/main -- "Source/*"               # -> 2110
git ls-tree -r --name-only origin/main -- Source/RefactorTactics/Tests/ | grep -c '\.cpp$'   # -> 207
git ls-tree -r --name-only origin/main -- Scenarios/ | grep -c '\.json$'               # -> 133
```

⚠️ **Il primo conteggio dei test era 2049**, misurato nel working tree condiviso: quel branch era **indietro
di 1101 righe** rispetto a `origin/main`, test compresi. La misura di una copertura non si fa sul ramo che
si ha sotto mano — è lo stesso difetto di metodo che il gemello si è corretto in `R8`.

### I due corpus `Sxx` che esistono, e non sono questo

- [#2190](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2190) — *«Corpus scenari del replay
  viewer»*, `S1…S14`. Lì `S1` è *«due unità sulla stessa cella»*; qui `S01` è *«state machine e ordine
  fasi»*. Numerazione scorrelata.
- I tre file `issue-run-NNN-Sx-*.md` del workspace `technical-designer`, che citano `S6` e `S9` **di quel**
  corpus.

La corrispondenza più stretta è **per contenuto, non per numero**: il §4 ricalca i diagrammi dell'*UML
Architecture Atlas* §IV — `State 02 — Turn State Machine`, `Activity 04 — Reaction Window`,
`State 06 — Team-only Network Planning`, e il *«Show Confirmed / Predicted / Uncertain feedback»*
dell'`Activity 01`. ⛔ Ma quell'Atlas dichiara di sé *«All diagrams are SUPPORTING views… never a parallel
normative hierarchy»*: anche trovandoli, non possono reggere un DoD.

---

## 4. L'anti-duplicazione

La tabella completa — quindici righe, owner e verdetto — vive in
[#2533](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2533) e non si duplica qui. L'esito:

| | |
|---|---|
| Voci che richiedono una issue nuova | **0 / 15** |
| Voci `REUSE` | 13 |
| Voci `EXTEND` su una issue esistente | 2 — `S07` e `S11`, entrambe su [#2492](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2492) |
| Righe ad evidenza **alta** | 15 / 15 |

### `C8` è implementato, mutazione compresa — `R1`

Le nove voci del §13 hanno un owner; una porta lo stesso nome della richiesta.

| Voce del kit | Sede |
|---|---|
| `FilterForTeam` | `URTIntentPrivacyLibrary::FilterForTeam` |
| reflection guard · replicated carrier · RPC carrier | `Core/RTServerOnlyGuard.h`, `ERTLeakRoute` a tre valori |
| server-only metadata | `Privacy.ServerOnlyTypesAreNotReplicated` |
| positive violating fixture | `Privacy.GuardDetectsAPlantedLeak` + `RTServerOnlyGuardFixturesForTest.h` |
| enemy cannot see unrevealed intent | `Combat.IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed` · `Reactions.IntentNotVisibleToEnemy` |
| reaction opportunity leaks no future | **`Overwatch.OpportunityLeaksNoFuture`** |

🔑 **La «mutation obbligatoria» del §13 è già un test permanente, e l'header dice perché**: *«Uno sweep che
non può fallire è indistinguibile da un checker rotto»*. Lo stesso vale per `C6`:
`Simulation.ChecksumUnitFieldListIsComplete` enumera i campi di `FRTUnitStateDigest` con `TFieldIterator` e
li confronta con la lista coperta — otto, fra cui tutti e tre gli stati che il §11 chiede di auditare
(`AbilityCooldowns`, `Statuses`, `Facing`).

⚠️ **La disciplina che il §15 propone come metodo è già pratica scritta.** Dal commento di
`Simulation.ChecksumDiscriminatesFacing`: *«La mutazione è del SOLO facing, ed è ciò che rende il test una
misura invece di una descrizione»*, con il verso opposto verificato perché *«un hash che cambiasse a ogni
chiamata passerebbe la prima asserzione senza discriminare niente»*.

---

## 5. Cosa si salva

Tre scelte corrette, nessuna delle quali richiede una roadmap nuova per essere adottata.

**C1 — `C7` arriva tardi di proposito, e la ragione è scritta.** *«L'explainability deve proiettare il
simulatore e il TurnLog già definiti, non diventare un secondo simulatore.»* È la stessa disciplina degli
invarianti del §3 del kit, e il suo owner naturale è
[#1937](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1937).

**C2 — La distinzione fra `BLOCKED-DECISION` e `BLOCKED-PACKAGED`**, con il rifiuto esplicito di chiudere
`M10.3` con test headless. ✅ La frase va tenuta: una simulazione locale non prova la privacy di rete. ⚠️ Ma
il canary ha già un owner — [#784](https://github.com/DegrassiAaron/refactor-tactics-main/issues/784) — e
il gate strutturale che il kit ignora esiste: è `RTServerOnlyGuard`.

**C3 — `REUSE → EXTEND → CREATE` del §14.** È la regola giusta, ed è quella che applicata al kit stesso
avrebbe evitato `R1`.

---

## 6. Cosa il kit chiedeva e non è stato fatto

| Richiesta | Esito |
|---|---|
| `C0`…`C10` come checkpoint con branch e PR | ❌ **non eseguiti**: il lavoro che descrivono esiste |
| Quindici issue per `S01…S15` | ❌ **non create** — §4 |
| Sette branch `test/schematic-*` | ❌ **non creati** — `R4` |
| Commit `test(schematics): establish code-only baseline for S01-S15` e i dieci successivi | ❌ conseguenza delle righe sopra |
| Repro per [#2148](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2148) | ⏸️ **non scritto**: resta legittimo e senza owner, ed è l'unico blocco del kit che regge |

⛔ Nessun `D-nnn`, `Enn`, milestone o label nuova è stato assegnato: il kit non li porta, e
[`AGENTS.md`](../../../AGENTS.md) §8 vieta di dedurli.

---

## 7. Riconciliazione

| Voce | Esito |
|---|---|
| Tracker della misura | **creato**: [#2533](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2533), label `documentation`, senza milestone |
| Referto | **questo file** |
| Roadmap code-only | **non creata** |
| Issue per le quindici `S` | **non create** |
| [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | **non toccato**: nessuna decisione nuova, e `MOV-4` era già barrata |

### NOT RUN

- Nessuna build, nessuna suite, nessun PIE. I conteggi vengono da `git grep`/`git ls-tree` e dall'API
  GitHub, non da una run: **questo referto non afferma che i 2110 test passino**, afferma che esistono.
- Delle quindici righe, `S05` e `S12` sono state verificate leggendo il **corpo** dei test; per le altre la
  misura si ferma al nome e all'owner. Un nome non è un oracolo.
- Lo stato delle issue è del 2026-09-06 e decade in ore — che è esattamente il difetto qui misurato.
