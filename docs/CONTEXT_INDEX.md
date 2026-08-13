# RefactorTactics — Context Index

> `CURRENT` · **Tipo**: indice di navigazione per assistenti/agent · **Ultimo aggiornamento**: 2026-08-10
>
> Questo file dice **quale contesto caricare e in che ordine**. Non è una nuova fonte normativa e non duplica le regole: per ogni concetto rimanda al suo owner.
>
> Repository: `DegrassiAaron/refactor-tactics-main` · branch operativo di riferimento: `main`.

---

## 1. Regola base

Per lavorare sul progetto **non usare la memoria della chat come fonte di verità**.

Prima di una modifica o di un'analisi concreta:

1. verifica `main`/HEAD e il task corrente;
2. leggi `AGENTS.md`;
3. usa questo indice per caricare **solo** gli owner pertinenti;
4. confronta sempre documentazione con codice e test quando la domanda riguarda lo stato reale;
5. se una fonte normativa e il codice divergono, **segnala la deriva**: non correggerla per plausibilità.

### Esclusioni di contesto

Per default **NON fanno parte del contesto autorevole**:

- file allegati in chat o caricati esternamente in conversazioni precedenti;
- copie locali/PDF degli stessi documenti se nel repository esiste una versione Markdown corrente;
- `docs/src/` salvo richiesta esplicita di vision/north-star/provenienza;
- `docs/archive/` salvo richiesta storica;
- handoff/prompt temporanei, inclusi file root del tipo `*_Claude_*.md`, salvo che il task chieda proprio di consolidarli;
- workbook di ricerca non canonici quando esiste un catalogo Markdown owner.

Se l'utente fornisce esplicitamente un nuovo file e chiede di usarlo, quel file entra nel task corrente ma **non sostituisce automaticamente il canone**.

---

## 2. Gerarchia effettiva delle fonti

La gerarchia completa vive in `docs/README.md`. Per lavoro operativo:

| Priorità | Fonte | Uso |
|---|---|---|
| 1 | `docs/product/piano-canonico-mvp.md` | invarianti e decisioni operative |
| 2 | `docs/decisions/RT_PDR_00_Decision_Log.md` + ADR applicabili | decisioni esplicite e motivazioni |
| 3 | `docs/DOC_CONFLICT_MATRIX.md` + `docs/OPEN_DECISIONS.md` | supersessioni, conflitti, decisioni non deducibili |
| 4 | `docs/roadmap/roadmap-checkpoint.md` | stato di esecuzione e DoD per milestone |
| 5 | `docs/roadmap/roadmap-v0.1.md` | scope e stato delle epic della release |
| 6 | `docs/balance/` | numeri vigenti e cataloghi |
| 7 | `docs/gameplay/` + `docs/technical/` | specifiche owner per feature |
| 8 | codice + test del branch corrente | **stato implementativo reale**, non decisioni di prodotto |

Regola di governance: un ADR accettato deve essere recepito nel canone; se non lo è, è un difetto documentale, non un invito a ricostruire a mente una gerarchia alternativa.

---

## 3. Snapshot operativo del progetto

Questa sezione è una **mappa rapida**, non un posto dove mantenere numeri di bilanciamento.

- Engine bloccato: **Unreal Engine 5.8.1**.
- Scope v0.1: **vertical slice 2v2 offline contro bot**.
- Roster v0.1: **Gadget · Phase · Riktor · Wraith**.
- Loop: `Planning → Prep → Dash → Blast → Move → Cleanup`.
- Il `Move` normale resta l'ultima fase volontaria.
- Griglia: **esagonale multilivello**, unico substrato, coordinate `FRTCellId{X=q, Y=r, Layer}`.
- No GAS nella v0.1: azioni/eroi/equipaggiamento sono data-driven con `UPrimaryDataAsset`.
- Authority: separata dal presentation layer anche in offline; rete completa è post-v0.1.
- Privacy intenti: il modello esiste già (`FRTPlannedIntent → FilterForTeam → FRTIntentView`); il gate di rete arriva in M10.
- Reazioni live: modello deciso `Opportunity → Commit`, Fast Reaction baseline **3,0 s**, timeout `HOLD`; implementazione interattiva è E14.
- Conoscenza parziale: mappa statica nota; vista/udito alimentano `Team Knowledge`; implementazione è E13.
- Facing è stato gameplay; implementazione completa è E16.

### Stato recente misurato su HEAD del 2026-08-08

HEAD osservato durante la creazione di questo indice: `e2bb0fc84733b50b47ec2d629d2131222b2e2206`.

- M6: codice della parità hex completato; resta il playtest PIE CP 6.8.
- M7: substrato quadrato rimosso; restano KPI che richiedono editor/rendering.
- M8: regole dei quattro eroi e reazioni base presenti; presentation/Ghost Timeline ancora da completare.
- M9: terreni/stati chiusi; cover bassa/alta, porte e ponti implementati; restano gli ultimi elementi di strutture/editor secondo la roadmap corrente.
- M10 rete/privacy e M11 production readiness: non ancora milestone operative completate.
- Scenario Harness presente sotto `Source/RefactorTactics/ScenarioHarness/` e corpus sotto `Scenarios/`.
- Ultima suite dichiarata dalla roadmap a questo HEAD: **492 test unici in 70 file**; run framework **496 performed, 0 failure**. Questo dato è solo snapshot: **va rimisurato**, mai copiato come costante.
- Il primo run automatico del corpus scenari ha prodotto **28 PASS, 6 BLOCKED, 2 expected-fail**.

Difetti/attenzioni emersi dal corpus più recente:

- `PushResistance` è presente nei dati ma manca ancora un consumer nella spinta (`#241`).
- La combo documentata `Riva PressureJet → Flux` nello stesso Blast non funziona come descritto per il timing degli snapshot/fasi; lo scenario end-to-end funzionante usa acqua già disponibile prima del Blast (`#242`).

Per lo stato aggiornato, prevalgono sempre `docs/roadmap/roadmap-v0.1.md`, `docs/roadmap/roadmap-checkpoint.md`, issue correnti e test/code del HEAD.

---

## 4. Mappa del codice

Runtime: `Source/RefactorTactics/`.

| Dominio | Percorso principale | Cosa cercare |
|---|---|---|
| Azioni / cataloghi | `Ability/` | `FRTActionDef`, `URTActionData`, hero/equipment catalog |
| Mappa hex | `Map/` | `FRTCellId`, cell data, map asset/actor, cover, porte, archi, LOS |
| Pathfinding | `Pathfinding/` | A* e path hex multilivello |
| Turno / resolver | `Turn/` | action queue, fallback, reaction, snapshot/sim, TurnLog, TurnManager |
| Combat | `Combat/` | forme, LOS combat, offensive actions, resolver |
| Terreni | `Terrain/` | superfici, stati, propagazioni |
| Bot | `Bot/` | utility bot su hex |
| Input | `Player/` | selezione, planning, preview, comandi player |
| Unit runtime | `Unit/` | posizione gameplay, statistiche, stato unità |
| UI | `UI/` | HUD e osservabilità runtime |
| Scenario tests | `ScenarioHarness/` | loader/index/runner/session/report e console `rt.Test.*` |
| Automation | `Tests/` | test di dominio e integrazione |

Editor-only: `Source/RefactorTacticsEditor/` — Hex Editor Mode e tool di paint/select/fill/arch.

Asset proprietari: `/Game/RT/` → `Content/RT/`.

Asset Fab/Paragon: terze parti, non fonte di regole e non necessariamente versionati nel repository.

---

## 5. Contesto da caricare per tipo di task

### Product / design / regole

Carica:

1. `docs/product/piano-canonico-mvp.md`
2. Decision Log + ADR pertinenti
3. `docs/DOC_CONFLICT_MATRIX.md`
4. `docs/OPEN_DECISIONS.md`
5. specifica owner in `docs/gameplay/`
6. catalogo pertinente in `docs/balance/`

Non dedurre una decisione aperta da altri documenti.

### Roadmap / feature registry / issue

Carica:

1. `docs/roadmap/roadmap-v0.1.md`
2. `docs/roadmap/roadmap-checkpoint.md`
3. `docs/roadmap/v0.1-definition-of-done.md`
4. issue/PR correnti
5. test e codice che provano lo stato

Non usare un handoff come prova che una feature sia implementata.

**Per orientarti prima di leggere**: `docs/roadmap/project-graph.json` è il grafo del progetto in forma
macchina — diagnostica, gate di release, epic/milestone/checkpoint, sedute in editor, voci PIE, scenari —
generato insieme a `feature-registry.json` da `python scripts/feature_registry.py generate`. Serve a sapere
**dove guardare**; gli owner qui sopra restano l'autorità. La sua vista umana è
`docs/control-center/` (`python -m http.server`, poi `/docs/control-center/`).

⚠️ Entrambi sono **generati**: se il `.yaml` è più recente, sono vecchi. Nel dubbio, `generate --check`.

### Le due toolchain

Il repository ne ha **due**, e nessuna gira in CI: i gate si eseguono **a mano**.

| | dove | cosa |
|---|---|---|
| **Python** | `scripts/` | `check-docs-links.py` · `check-docs-symbols.py` · `feature_registry.py` · `rt_shared_id.py` |
| **Node 22** | `tools/radar/` | rubrica dei rating e generatore SVG dei radar di personaggio |

`tools/radar/` legge i cataloghi di bilanciamento, calcola i rating dei radar e produce gli SVG in
[`characters/radar/`](characters/radar/), che sono **versionati con un gate**:

```sh
node --test "tools/radar/**/*.test.ts"   # 56 test
node tools/radar/generate.ts             # riscrive gli otto SVG
node tools/radar/generate.ts --check     # verifica, exit 1 se divergono
node tools/radar/wiki-alt.ts --wiki-root <clone> --check   # l'alt sulla Wiki ripete i valori: exit 1 se e' rimasto indietro
```

⚠️ **Non è solo documentazione**: siccome i rating si calcolano dai cataloghi
([D-106](decisions/RT_PDR_00_Decision_Log.md)), cambiare `Salute` o un cooldown rende il gate rosso
finché i radar non sono rigenerati. **Chi tocca un catalogo deve poter eseguire il generatore** — è il
prezzo dichiarato di [D-108](decisions/RT_PDR_00_Decision_Log.md).

Nessun `npm install`, nessun build step: Node 22 esegue TypeScript con type stripping e i test usano
`node:test`, quindi `tools/` ha **zero dipendenze**.

`scripts/rt_shared_id.py` è l'allocatore degli ID condivisi del Decision Log
([D-135](decisions/RT_PDR_00_Decision_Log.md)): `D-nnn` **non si sceglie a mano**.

```sh
python scripts/rt_shared_id.py reserve D --reason "#621 bake"   # stampa l'ID da usare
python scripts/rt_shared_id.py status                           # contatore e reservation del clone
python scripts/rt_shared_id.py release D-134                    # cede un ID a chi lo usa gia' (non lo libera)
python scripts/rt_shared_id.py check                            # exit 1 su duplicati o ID malformati
git fetch --prune origin && python scripts/rt_shared_id.py audit-refs   # exit 1 se due rami collidono
python scripts/test_rt_shared_id.py                             # 33 test, uno a venti processi
```

L'atomicità copre tutti i worktree di **questo clone** — lock nel git common dir — e non altri cloni o
altri PC: là `audit-refs` diagnostica prima del merge invece di prevenire. Meccanismo e recovery in
[`technical/workflow-parallel-claude.md`](technical/workflow-parallel-claude.md).

⚠️ **L'allocatore risolve la collisione di numerazione e nient'altro: due worktree possono ancora scrivere
lo stesso file.** Per quello serve il write-set del batch — [`roadmap/parallel-batch.yaml`](roadmap/parallel-batch.yaml),
[D-139](decisions/RT_PDR_00_Decision_Log.md) — con la regola *file non assegnato = STOP* e la **Binary
Asset Lease** sui `.uasset`/`.umap`, che sono human-first ma non human-only.

### Riferimenti a checkpoint

`CP 6.3` **non è risolvibile**: 20 numeri di checkpoint su 22 esistono in due spazi — `6.3` è «input hex»
in **M6** e «Phase» in **E6**. Scrivi sempre la forma prefissata: `M6.3` (owner `roadmap-checkpoint.md`),
`E1.3` (owner `roadmap-v0.1.md` §2.2), `E8` (epic intera), `U13` (seduta in editor).

### Turni / action engine / reazioni

Owner principali:

- `docs/gameplay/spec-sequenza-turno.md`
- `docs/gameplay/spec-economia-del-turno.md` — **quanto può fare un'unità in un turno**: come i quattro budget
  (slot, Movement Point, pivot, cooldown/risorsa) si tengono insieme, e la proposta **aperta** che il profilo
  di movimento cambi anche la legalità delle azioni. I singoli budget restano owner di sé stessi
- `docs/gameplay/spec-motore-azioni-e4.md`
- `docs/gameplay/spec-reazioni-componibili-cp55.md`
- `docs/decisions/adr-0004-finestre-di-reazione.md`
- `docs/gameplay/brief-overwatch-reazioni.md`
- `Source/RefactorTactics/Turn/`
- test `RTAction*`, `RTReaction*`, `RTComposableReaction*`, `RTIntercept*`

### Mappa / path / cover / strutture

Owner principali:

- `docs/decisions/adr-0002-griglia-esagonale.md`
- `docs/gameplay/spec-tassonomia-movimento.md` — **quale famiglia di movimento**, e cosa comporta. Dal
  2026-08-12 ([D-118](decisions/RT_PDR_00_Decision_Log.md)) la partizione è **`Traversal`** (Move · Dash ·
  Forced — percorre lo spazio) contro **`Transfer`** (Leap · Blink · Swap · Recall — cambia posizione senza
  percorrerlo); `Reaction` è una **causa**, non una famiglia. È l'unico documento che le confronta; i
  singoli restano owner di sé stessi
- `docs/technical/spec-mappa-multilivello.md`
- `docs/technical/spec-pathfinding-pf3-pf4.md`
- `docs/gameplay/spec-copertura-cp91.md` e successive spec CP9.x
- `Source/RefactorTactics/Map/`
- `Source/RefactorTactics/Pathfinding/`
- test `RTHex*`

### Eroi / azioni / bilanciamento

Owner numerici:

- `docs/balance/RT_ActionCatalog_v0.1.md`
- `docs/balance/RT_HeroCatalog_v0.1.md`
- `docs/balance/RT_TerrainCatalog_v0.1.md`
- `docs/balance/RT_EquipmentCatalog_v0.1.md`

Il workbook XLSX è **research**, non owner dei numeri vigenti.

### Scenari / showcase / test automatici

Carica:

- `docs/technical/test-automatico-unreal.md`
- `docs/product/showcase-v0.1.md`
- `Scenarios/`
- `Source/RefactorTactics/ScenarioHarness/`
- `Source/RefactorTactics/Tests/RTScenario*.cpp`
- `Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp`

Uno scenario `BLOCKED` è valido come specifica anticipata; `FAIL` indica un difetto del gioco; `ERROR` un problema dello scenario/harness.

### UI / planning visuale

Carica:

- `docs/technical/progettazione-hud.md`
- `docs/technical/brief-planning-visuale.md`
- `docs/gameplay/brief-conoscenza-parziale.md`
- `Source/RefactorTactics/UI/`
- `Source/RefactorTactics/Player/`

### Content / asset / Blueprint

Carica:

- `docs/technical/convenzioni-contenuti-ue.md`
- `docs/technical/asset-map.md` — quali asset servono e quanti ne mancano (registro)
- `Content/RT/`
- eventuali guide feature-specifiche

Non modificare `.uasset`/`.umap` come testo e non spostarli dal filesystem.

---

## 6. Invarianti da tenere sempre in RAM

1. Simulazione decide; presentation mostra.
2. Snapshot + versione/regole + seed + decisioni registrate ⇒ stesso risultato.
3. Ordinamenti competitivi espliciti; niente dipendenza da `TMap`/`TSet`, frame rate o timing animazioni.
4. `FRTCellId` è l'unica coordinata gameplay.
5. `Move` normale dopo `Blast`.
6. Client propone / authority valida; privacy dell'intento per costruzione, non per occultamento UI.
7. C++ definisce possibilità/invarianti; Data Asset/Blueprint scelgono variante/presentazione.
8. No GAS nella v0.1.
9. Una ability ha un solo owner; sinergie emergono da sistemi/stati, non da ability di coppia.
10. Test/scenari devono passare dalla pipeline reale `Intent → Planning → Snapshot → Resolver → TurnLog`.

---

## 7. Freshness protocol per assistenti

Quando una risposta dipende dallo stato attuale del repository:

- controlla HEAD recente;
- non fidarti di conteggi test copiati: rimisura o usa l'ultima misura esplicitamente legata a un commit;
- controlla prima il file owner, poi il codice/test relativo;
- per feature dichiarate “fatte”, cerca almeno una prova: test, file runtime, build/PIE/packaged registrato nel DoD appropriato;
- se un documento è più vecchio di una decisione/commit che lo contraddice, apri `docs/DOC_CONFLICT_MATRIX.md` prima di usarlo.

Questo file va aggiornato **solo** quando cambia la mappa delle fonti, lo scope canonico o un punto di ingresso principale. Non deve diventare una seconda roadmap o un secondo catalogo di bilanciamento.
