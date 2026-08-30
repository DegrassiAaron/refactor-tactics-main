# RefactorTactics — Context Index

> `CURRENT` · **Tipo**: indice di navigazione per assistenti/agent · **Ultimo aggiornamento**: 2026-08-17
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
- `docs/research/` salvo richiesta esplicita di vision/north-star/provenienza — è la ex `docs/src/`,
  svuotata il 2026-08-19 ([#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165));
- `docs/archive/` salvo richiesta storica;
- handoff/prompt temporanei, inclusi file root del tipo `*_Claude_*.md`, salvo che il task chieda proprio di
  consolidarli. ✅ **Dal 2026-08-30 la radice non ne ha nessuno**: i sei accumulati fra il 2026-08-08 e il
  2026-08-28 sono stati consumati da [D-246](decisions/RT_PDR_00_Decision_Log.md), e la radice contiene i
  soli `AGENTS.md`, `CLAUDE.md` e `README.md`. La riga resta perché il posto si riempie di nuovo: è la
  casella di posta dell'autore, e nessun gate la guarda;
- [`product/lore-e-worldbuilding.md`](product/lore-e-worldbuilding.md) — è `PROPOSED` e **non normativo**:
  si carica per scrivere di narrativa, mai per decidere una regola. Nessun termine che contiene è nel canone;
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
- Conoscenza parziale: vista/udito alimentano `Team Knowledge`; implementazione è E13. ⚠️ **La geometria non osservata si nasconde** da [D-225](decisions/RT_PDR_00_Decision_Log.md) (2026-08-28) — questa riga diceva «mappa statica nota».
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

- `PushResistance` **ha** un consumer dal 2026-08-08 — `RTTurnManager.cpp`, pass degli effetti del Blast —
  ed è una **soglia**, non una sottrazione ([D-038](decisions/RT_PDR_00_Decision_Log.md), che dichiara di
  correggere [`#241`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/241), chiusa lo stesso
  giorno). È però **dormiente**: [D-075](decisions/RT_PDR_00_Decision_Log.md) l'ha portata a `0` su tutto il
  roster, quindi nessuna spinta la incontra sopra zero. *Dormiente e assente non sono la stessa cosa.*
- La combo documentata `Phase PressureJet → Gadget` nello stesso Blast non funziona come descritto per il timing degli snapshot/fasi; lo scenario end-to-end funzionante usa acqua già disponibile prima del Blast (`#242`).

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

Due **viste** aiutano a orientarsi e non sostituiscono gli owner qui sopra: `docs/roadmap/roadmap-v0.1-v1.0.md`
(la traiettoria v0.1 → v1.0 in una pagina, con le quattro soglie che separano le release) e
`docs/roadmap/roadmap-main-v0.1.md` (il focus v0.1 come tre lane e sei wave, con le premesse del mandato
verificate contro il repository). Nessuna delle due porta numeri propri: se una diverge da un owner, ha
ragione l'owner.

⛔ **Non esiste più un grafo del progetto in forma macchina.** `project-graph.json`,
`feature-registry.json` e il Project Control Center che li leggeva sono usciti il 2026-08-21
(**D-181**), insieme al Feature Registry che li generava. Per orientarti restano **gli owner qui
sopra**, che erano già l'autorità: il grafo diceva *dove guardare*, non *cosa è vero*.

### Le due toolchain

⛔ **Nessuna delle due fa più da gate.** `scripts/` è stata rimossa il 2026-08-21 (**D-182**) con i suoi
nove script e i due file di test: cinque gate documentali, due controlli sui dati di gioco, due generatori.
Il Python versionato è tornato — tre file — ma nessuno di essi verifica niente.

| | dove | cosa |
|---|---|---|
| ~~**Python**~~ | ~~`scripts/`~~ | ⛔ **rimossa con D-182** il 2026-08-21: i cinque gate documentali, i due controlli sui dati di gioco e i due generatori |
| **Node 22** | `tools/radar/` | rubrica dei rating e generatore SVG dei radar di personaggio — **gli unici controlli vivi** |
| **Python 3.11** | `tools/icons-downloader/` | un downloader di icone Paragon, non un gate |
| **Python 3.11** | `tools/decision-log/` | vista HTML del Decision Log e raccoglitore del suo stato GitHub — **nessun `--check`**, non falliscono mai |

> 🔴 **Fino al 2026-08-19 questa tabella elencava i gate Python per nome, e ne conosceva tre su sei.**
> Mancavano `check-docs-naming.py`, `check-capability-owners.py` e `check-equipment-defaults.py`, più
> `docs_inventory.py`, che è nato dopo. È lo stesso difetto che `AGENTS.md` ha già pagato e corretto il
> 2026-08-16, con la stessa causa: **un elenco scritto a mano dentro un documento non ha modo di
> accorgersi di un file nuovo**, e chi aggiunge un gate non passa di qui. La forma `check-*` si aggiorna
> da sé; i tre che non la seguono restano nominati, perché lì l'elenco è l'unica via.

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

`tools/decision-log/` legge questo registro — [`decisions/RT_PDR_00_Decision_Log.md`](decisions/RT_PDR_00_Decision_Log.md),
che resta l'**owner** — e ne genera una pagina unica dove ogni `#nnnn` e ogni `D-nnn` sono cliccabili:

```sh
python3 tools/decision-log/fetch_github_cache.py                        # stato e titoli da GitHub (serve `gh`)
python3 tools/decision-log/build_decision_view.py --out build/decision-log.html
```

⚠️ **L'HTML è un artefatto rigenerabile e non si versiona** (`build/` è in `.gitignore`): la vista non è
una seconda verità, si ricostruisce dal Markdown. Ciò che **è** versionato è `github-cache.json`, e va
saputo per cos'è: un'**istantanea datata** dello stato delle issue, non un dato vivo — una pagina statica
non può interrogare GitHub. La pagina dichiara la data in cui è stata presa; chi la usa per decidere cosa
è ancora aperto rilancia prima il primo comando. Sola dipendenza esterna del repository: la CLI `gh`, e la
usa solo quel comando.

Gli ID condivisi del Decision Log — `D-nnn` — si scelgono leggendo l'ultimo assegnato e si
**riverificano prima del merge** ([D-178](decisions/RT_PDR_00_Decision_Log.md)):

```sh
git fetch --prune origin
git grep -oh "D-[0-9]\+" origin/main -- docs/decisions/RT_PDR_00_Decision_Log.md | sort -V | tail -1
gh pr list --state open      # una PR in volo puo' aver gia' rivendicato il numero
```

⚠️ **Questo è un controllo a vista, e il limite è dichiarato.** Fino al 2026-08-20 l'assegnazione era
automatica — `scripts/rt_shared_id.py`, un allocatore con lock nel git common dir — perché il progetto
aveva pagato **sedici** collisioni scegliendo i numeri a mano. Lo strumento è stato rimosso con
[D-178](decisions/RT_PDR_00_Decision_Log.md) insieme al resto del sistema di lavoro parallelo, sulla
premessa che «con una sola sessione per volta la finestra di race si chiude quasi tutta».

⛔ **Quella premessa è falsa, misurata il 2026-08-27** ([D-222](decisions/RT_PDR_00_Decision_Log.md)):
**101** checkout di `HEAD` in 24 ore, **6 sessioni** distinte a committare, **4 nella stessa finestra di
6 minuti**. La finestra di race non si chiude affatto — e il progetto aveva pagato **sedici** collisioni
proprio con il metodo a mano, sotto questo stesso regime. Il controllo a vista resta l'unica difesa e
**ora si sa che è più esposto di quanto questa riga dichiarasse**: `git fetch --prune origin`, poi
`gh pr list --state open`, prima di ogni merge. Se una PR aperta rivendica lo stesso ID con una **tesi
diversa**, rinumera la seconda.

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
- `docs/technical/systems/spec-cover-placement-intra-hex.md` — **posa nella cella, copertura selezionabile e
  geometria intra-hex** (`D-285`, 2026-08-30). Owner di: regioni di posa, `CoverSource`/`CoverOption`/
  `CoverSide`, traversata `SideA ↔ SideB`, e l'invariante **uno slot di occupancy per `FRTCellId`**.
  ⚠️ **Corregge `spec-hex-geometry-authoring.md` §5 e §6**: la soglia dei settori e il contatto col centro
  **non decidono più la calpestabilità** — la decide l'esistenza di un placement per il footprint
- `docs/technical/architecture/spec-mappa-multilivello.md`
- `docs/technical/architecture/spec-pathfinding-pf3-pf4.md`
- `docs/gameplay/spec-copertura-cp91.md` e successive spec CP9.x
- `Source/RefactorTactics/Map/`
- `Source/RefactorTactics/Pathfinding/`
- test `RTHex*`

### Strumenti d'authoring — mappa, scenari, abilità

Owner del **concetto e del confine**, dal 2026-08-17 ([D-154](decisions/RT_PDR_00_Decision_Log.md)):

- `docs/technical/tooling/spec-tactical-designer.md` — cosa uno strumento d'editor ha il diritto di decidere, e cosa
  deve invece chiedere al gioco. Risponde a una domanda sola: *se l'editor e il runtime possono divergere,
  lo strumento ha perso il suo valore*. Contiene la scala di maturità `TD 0.1 … TD 1.0`, che è **maturità di
  uno strumento e non una release** — `TD 0.7` non ha niente a che vedere con `v0.7`
- `docs/technical/test-manuali-pie.md` — quale seduta d'editor fare, e con quale esito. ⛔ La vista che ne ordinava la coda — `editormap.shortlist.md` — è uscita con **D-181** il 2026-08-21: l'ordine oggi si legge in `docs/roadmap/editor-sessions.yaml`, che ne è la sorgente, e nessuna vista lo aggrega
- `docs/technical/tooling/scenario-map.md` — chi verifica cosa, fra macchina e persona
- `Source/RefactorTacticsEditor/` — il mode e i cinque tool. ⚠️ **Ha test dal 2026-08-16** (`#993`,
  `Private/Tests/`): la frase «quel modulo non ha test», che vive ancora in due punti di
  `spec-hex-geometry-authoring.md`, è **superata**
- `Source/RefactorTactics/ScenarioHarness/` — il formato scenario canonico che ogni authoring deve produrre

### Eroi / azioni / bilanciamento

Owner numerici:

- `docs/balance/RT_ActionCatalog_v0.1.md`
- `docs/balance/RT_HeroCatalog_v0.1.md`
- `docs/balance/RT_TerrainCatalog_v0.1.md`
- `docs/balance/RT_EquipmentCatalog_v0.1.md`

Il workbook XLSX è **research**, non owner dei numeri vigenti.

### Scenari / showcase / test automatici

Carica:

- `docs/technical/tooling/test-automatico-unreal.md`
- `docs/product/showcase-v0.1.md`
- `Scenarios/`
- `Source/RefactorTactics/ScenarioHarness/`
- `Source/RefactorTactics/Tests/RTScenario*.cpp`
- `Source/RefactorTactics/Tests/RTShowcaseScenarioTests.cpp`

Uno scenario `BLOCKED` è valido come specifica anticipata; `FAIL` indica un difetto del gioco; `ERROR` un problema dello scenario/harness.

### UI / planning visuale

Carica:

- `docs/technical/systems/progettazione-hud.md`
- `docs/technical/systems/brief-planning-visuale.md`
- `docs/gameplay/brief-conoscenza-parziale.md`
- `Source/RefactorTactics/UI/`
- `Source/RefactorTactics/Player/`

### Camera e presentazione della mappa

Carica:

- `docs/technical/systems/spec-tactical-camera.md` — stato, input, zoom, focus, multilayer, picking,
  occlusione. ⚠️ Separa ovunque ✅ *misurato nel codice* da ⏳ *prescritto e non scritto*: leggere il
  simbolo prima della riga
- `docs/technical/systems/spec-domini-spaziali-mappa.md` — i quattro domini (`PlayableMapBounds`,
  `ScenicBufferArea`, `CameraTravelBounds`, `VisualBackgroundBounds`), i limiti viewport-aware, il
  linguaggio del bordo
- `Source/RefactorTactics/Camera/RTCameraPawn.h` — l'unico posto in cui la camera è già decisa
- `Source/RefactorTactics/Tests/RTCameraPawnTests.cpp`

La camera è **presentation-only** (`D-143`): non decide LOS, targeting, cover, pathfinding né validità di
cella. Il triage `docs/roadmap/plans/camera-roadmap-v1-triage-2026-08-14.md` è **storia**, non owner
(`D-254`).

### Content / asset / Blueprint

Carica:

- `docs/technical/tooling/convenzioni-contenuti-ue.md`
- `docs/technical/tooling/asset-map.md` — quali asset servono e quanti ne mancano (registro)
- `docs/technical/systems/spec-graybox-placement-contract.md` — quanto spazio occupa un asset di mappa, dov'è il
  suo pivot, come si legge il suo stato. ⚠️ Non è il clearance: *quanto grande posso modellare* non è
  *dove un'unità ci sta in piedi* (CP 23.6)
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
