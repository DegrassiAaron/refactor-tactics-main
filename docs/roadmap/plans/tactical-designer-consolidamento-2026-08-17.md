# Tactical Designer — referto di consolidamento

> `SNAPSHOT` · **Misurato il**: 2026-08-17 su `origin/main` `bd806e4f` · **Owner**: nessuno
> **Cosa è**: la fotografia con cui è stato consumato il sorgente
> [`RefactorTactics_TacticalDesigner_03_Roadmap_v1.0_Scenari_SkillWorkbench_Claude.md`](../../archive/src/RefactorTactics_TacticalDesigner_03_Roadmap_v1.0_Scenari_SkillWorkbench_Claude.md),
> più il criterio con cui ogni sua sezione è stata accolta, ridotta o respinta.
> **Cosa non è**: una fonte di stato. Il modello vive in
> [`../../technical/spec-tactical-designer.md`](../../technical/spec-tactical-designer.md), lo stato nel
> [`../feature-registry.yaml`](../feature-registry.yaml) e nelle issue.

---

## 1. La regola numero zero, applicata

Il sorgente apre dichiarandosi *«fotografia storica aggiornata fino al 2026-08-13»* e chiede di non fidarsene.
Rimisurato: **la catena base che il documento tratta come lavoro da fare è chiusa da quattro giorni.**

| Issue | Stato nel sorgente | Stato misurato 2026-08-17 |
|---|---|---|
| `#554` | «se è aperta, in parallelo» (§13) | ✅ **CLOSED** |
| `#588` | «CLOSED, ma testi stale la citano aperta» (§3) | ✅ **CLOSED** — e il drift non esiste più |
| `#619` | CLOSED | ✅ **CLOSED** |
| `#620` | **OPEN — «è il prossimo lavoro corretto»** (§5, §34) | ✅ **CLOSED** |
| `#621` | «non partire se #620 non è pronta» (§9) | ✅ **CLOSED** |
| `#622` | OPEN | ⬜ **OPEN** |
| `#623` | OPEN | ⬜ **OPEN** |
| `#324` | epic v0.2 | ⬜ **OPEN**, non si apre |

Ne segue che **§5, §6, §7, §8, §9, §10, §12, §25, §26 e §34 sono interamente superate**: descrivono la
progettazione di `#620` e `#621`, che esistono in `main` come `RTGeometryGrammar.{h,cpp}` e
`RTGeometryBake.{h,cpp}` con **13 + 8** test — contati con
`grep -c IMPLEMENT_SIMPLE_AUTOMATION_TEST` sui due file, non ripresi dal registry, che dice «undici» e
«sette» ed è indietro di due e di uno. Il sorgente stesso lo prevede — §34:
*«Se #620 è già chiusa: NON rifarla»* — e questa è l'applicazione di quella riga, non una deviazione.

Anche **§11** è chiusa e il sorgente non poteva saperlo: la domanda sull'invertibilità del bake è `MSE-1`, e
[`D-131`](../../decisions/RT_PDR_00_Decision_Log.md) l'ha risolta il 2026-08-13 con `FRTHexCover::bGenerated`
— provenienza d'authoring che **non entra in `ComputeHash`**. La policy che §11 chiedeva di non inventare
esiste, ed è *source geometry owns the fields it generated*.

---

## 2. Errore di fatto nel sorgente, da non propagare

**§68** elenca come «contesto da verificare» il roster *«Gadget, Phaser, Victor, Wrath»*. È **falso in tre
nomi su quattro**: [`D-120`](../../decisions/RT_PDR_00_Decision_Log.md) fissa il roster e
[`D-130`](../../decisions/RT_PDR_00_Decision_Log.md) i nomi canonici — **Gadget · Phase · Riktor · Wraith**.
La §38 del sorgente ripete `Phaser` dentro un mockup di UI.

Il sorgente chiede di verificare quell'elenco contro `main`, e questa è la verifica: nessun nome del sorgente
entra in nessun documento di questo consolidamento. Registrato qui perché è la classe di errore che
[`piano-migrazione-roster.md`](../../technical/piano-migrazione-roster.md) esiste per prevenire, e un
sorgente archiviato resta leggibile per anni.

---

## 3. Cosa esiste già — l'elenco che rende il sorgente per metà non eseguibile

Il rischio dichiarato dal sorgente è la duplicazione (§24, §35: *«la qualità si misura da ciò che NON viene
duplicato»*). Questo è l'inventario che lo previene.

### 3.1 Editor

`Source/RefactorTacticsEditor/` — `URTHexEditorMode` (UEdMode auto-registrato) e **cinque** tool:

| Tool | File | Copre del sorgente |
|---|---|---|
| Select | `Tools/RTHexSelectTool.h` | §4 |
| Paint | `Tools/RTHexPaintTool.h` | §4 · §30 |
| Fill | `Tools/RTHexFillTool.h` | §4 · §30 |
| Arch | `Tools/RTHexArchTool.h` | §4 · §14 |
| **Geometry** | `Tools/RTHexGeometryTool.{h,cpp}` — 380 righe, `99cd06bd` | **§6 · §25 · §31A.4 (parziale)** |

⚠️ **Il tool Geometry esiste.** Il sorgente lo tratta come lavoro futuro in tre punti; è in `main` con
ghost, snap sugli assi e readout dell'asse agganciato. `#712` resta aperta per la sola **seduta U22**
(leggibilità del ghost, percepibilità dello snap, granularità dell'Undo) — cioè per un giudizio umano, non
per del codice.

### 3.2 Runtime che l'editor chiama, e che non va reimplementato

| Owner | Cosa | Test |
|---|---|---|
| `Map/RTGeometryGrammar.{h,cpp}` | grammatica quantizzata + validator + snap | `RefactorTactics.GeometryGrammar.*` (**13**, due dei quali `Snap*`) |
| `Map/RTGeometryBake.{h,cpp}` | cottura verso `FRTHexCover` / `bBlocksMovement` | `RefactorTactics.GeometryBake.*` (**8**) |
| `Map/RTHexOccupancyLibrary.{h,cpp}` | dodici settori, `Free`/`Constrained`/`Blocked` | `RefactorTactics.HexOccupancy.*` (19) |
| `Pathfinding/` | A\* deterministico, `ReachableCells` | `RefactorTactics.HexSim.*` |
| `Perception/` | LOS esagonale con regola d'elevazione | — |

Il vincolo di collocazione che il sorgente chiede in §8 — *«sposta la logica pura nel modulo runtime»* — è
**già la regola del repository**, ed è stata applicata anche allo snap di `#712`:
`RefactorTactics.GeometryGrammar.Snap*` vive nel runtime.

> 🔴 **La ragione con cui il repository giustifica quella regola è scaduta, e questo referto l'ha ripetuta
> prima di misurarla.** La formulazione in tre punti del Feature Registry e in due spec è *«in
> `Source/RefactorTacticsEditor/` non esiste alcun test»*. Misurato il 2026-08-17: `find
> Source/RefactorTacticsEditor -iname "*test*"` restituisce **due voci** — `Private/Tests/` e
> `Private/Tests/RTHexToolPropertiesTests.cpp`, **due** test arrivati con `#993` il 2026-08-16.
>
> Il file di test nomina il difetto meglio di quanto potrei io: *«Tre issue di fila (#871, #921, #931) hanno
> dichiarato "RefactorTacticsEditor/ non ha test, quindi la verifica è manuale" trattandolo come un dato di
> fatto. Non lo era: [...] I test non erano impossibili: non erano stati scritti.»* Con questo referto la
> ripetizione è alla **quarta** volta.
>
> **La regola resta, la giustificazione cambia**: la logica di gioco non va nel modulo editor non perché
> quel modulo sia intestabile, ma perché una regola di gioco che vive lì è una **seconda risposta** alla
> stessa domanda. I due test dell'editor coprono il proprio dominio — il costo che segue la superficie, il
> readout che non si aggiorna da solo — e nessuno dei due decide un esito di partita.
>
> Corretto in `spec-tactical-designer.md` §3 e nelle tre note del Feature Registry. ⚠️ **Restano due
> occorrenze non corrette** in `docs/technical/spec-hex-geometry-authoring.md` (§ soglie e § collocazione):
> quel file **non è nel `writable` di nessuna track né in `integration_only`**, e per `D-139` è uno
> **STOP**. Non è «una piccola fix»: è il file di un altro. Dichiarato qui perché non resti a nessuno.

### 3.3 Scenario Harness — il formato canonico che il Composer dovrà authorare

`Source/RefactorTactics/ScenarioHarness/` — `RTScenarioIndex`, `RTScenarioLoader`, `RTScenarioRunner`,
`RTScenarioSession`, `RTTestScenario`, `RTTestReportWriter`, `RTTestConsole`.

Il modello dati che il sorgente §37 chiede di «poter rappresentare» **esiste già in gran parte**:

| §37 chiede | `RTTestScenario.h` ha | Gap |
|---|---|---|
| Stable Scenario ID, tag | `FRTTestScenario::ScenarioId` + `Version` di formato + `RTScenarioIndex` (`#209`) | — |
| map, character placement | `FRTScenarioCell`, `FRTScenarioUnit::Cell` | — |
| facing | `FRTScenarioUnit::Facing` con `bDeclaresFacing` | — |
| HP / resources / status | `Health`, `Shield`, `VisionRange`, `Loadout` | **status** non esprimibile |
| environment state | `bBlocksMovement`, `bBlocksLineOfSight`, `MoveCost`, `OccupancySurcharge` | acqua/fuoco/ghiaccio no |
| Timeline / Plans | `FRTScenarioTurn::Intents` → `FRTScenarioIntent` (`UnitId`, `Move`, `Ability`, `Target`) | — |
| **Interactive decisions / reactions** | **`FRTScenarioDecision`** (`Unit`, `Respond` = `FIRE`/`HOLD`, `Target`) | **nessun gap** |
| Expectations | `FRTTestExpectation` + 8 `ERTAssertionKind`, fra cui `LogEventCount` · `LogEventOrder` · `LogEventAmount` | — |
| **Clone / Variant (§40.3)** | **`FRTScenarioVariant`** + `FRTScenarioVariantUnit` | varia **solo le celle**, per scelta dichiarata |
| deterministic seed | **`FRTTestScenario::Seed`** — dichiarato e **non consumato**, con guardiano `RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed` | 🔴 **nessun gap**: vedi sotto |

E l'esito `ERTTestOutcome::Blocked` più `FRTScenarioTurn::Requires` sono già la risposta a §41 e a §54: uno
scenario può essere versionato **prima** che i suoi sistemi esistano, dichiarando quali capability gli
mancano, senza rendere rossa la suite.

> 🔴 **Il sorgente non ha misurato questo file, e la prima stesura di questo referto neppure.**
> §36 dice *«non creare un secondo scenario language se il repository possiede già un formato canonico»* — lo
> possiede, ha **78** file di scenario versionati in `Scenarios/` (`find Scenarios -name '*.json' ! -name '_redirects.json' | wc -l`, 2026-08-17 — `scenario-map.md` dichiara **73** *classificati*, misurati il 2026-08-13: contano cose diverse, e l'owner del conteggio è quello, non questo referto), e copre **due** delle cose che §37 e §40.3 presentano
> come da progettare. Questa tabella diceva «`Interactive decisions → gap, è il seam di D-101`»: falso.
> `D-101`/`#542` è il seam **generale** di chi produce decisioni; le decisioni **di finestra** scriptate sono
> `#512` e sono in `main`. Sono tre cose distinte con lo stesso nome, e `RTTurnManager.h` lo dice per esteso:
> *«sono tre cose in tre release»*.

⚠️ **`FRTScenarioVariant` è il punto d'innesto del Skill Workbench, e porta già il suo trade-off scritto.**
Il commento della struct dichiara la restrizione e il prezzo di allargarla:

> *«Solo le CELLE, e la limitazione è deliberata: una variante che potesse cambiare eroi, squadre o
> condizione iniziale non sarebbe più "lo stesso scenario con un ingresso diverso", e il confronto fra le sue
> tracce non direbbe più quale ingresso ha prodotto la differenza. Il giorno in cui servisse variare altro,
> lo si aggiunge sapendo che si sta allargando ciò che il canary può attribuire.»*

Il *baseline vs candidate* di §43 è esattamente «servisse variare altro». Quindi non nasce un secondo
meccanismo di varianti: si estende questo, e l'estensione deve **dichiarare** cosa smette di essere
attribuibile. È scritto nel DoD della issue di TD 0.3.

### 3.4 Replay, indice, bilanciamento

- `RT-FEAT-REPLAY-ARCHIVE` **INTEGRATED**: recorder, Player, `history.rtindex`. È metà di §52.
  🔴 Questa riga diceva «**26** test», ripreso dal registry (`last_verified: 2026-08-10`) invece che misurato
  — cioè il contrario di quello che il §1 di questo stesso documento fa con «undici/sette». Misurato il
  2026-08-17: `grep -rhoE '"RefactorTactics\.Replay[A-Za-z0-9_.]*"' Source/RefactorTactics/Tests/ | sort -u |
  wc -l` → **48**. Il registry non è stato aggiornato qui: quel campo è di chi possiede la feature, e la
  misura è registrata perché il prossimo che la legge non la riprenda a sua volta.
- `RT-FEAT-UI-SCENARIO-BROWSER` **INTEGRATED**: `ScenarioId` staccato dal percorso, tag, `ResolvePath`. È
  metà di §54.
- `RT-FEAT-TOOL-BALANCE-GROUND` **IMPLEMENTING**: cataloghi + matrice di test. `#543` porta il vincolo di
  [`D-102`](../../decisions/RT_PDR_00_Decision_Log.md) che §53 non nomina — *un risultato bot-vs-bot non è
  evidenza di bilanciamento finché il bot non è certificato sulla capability misurata*.

---

## 4. I gap veri

Quattro, e sono quelli su cui il sorgente aggiunge qualcosa che il repository non ha.

| # | Gap | Perché è un gap misurato |
|---|---|---|
| **G1** | **Scenario Composer** — authoring visuale sopra `FRTScenario*` | Gli scenari si scrivono a mano in JSON. `grep -ril "composer" Source/` → zero. ⚠️ Il gap è la **UI**, non il formato: §3.3 mostra che il modello copre già timeline, decisioni ed expectation |
| **G2** | **Skill Workbench** — candidate ≠ production | ⚠️ vedi sotto: il **nome** esiste, il **concetto** no |
| **G3** | **Probe di targeting/LOS/cover nell'editor** | Esiste solo `#711` (sonda di movimento), aperta. Nessuna issue per LOS/cover/AoE |
| **G4** | **Baseline vs candidate su una suite** | Dipende da G2. Nessun owner |

`G3` **non** diventa una feature nuova: `#711` è già dichiarata in `RT-FEAT-TOOL-MAP-EDITOR`, e il sorgente
avverte da sé (§42) che *«il Feature Registry non deve diventare una lista di ogni singolo toggle UI»*.

> ⚠️ **`G2` è stato quasi dichiarato con la misura sbagliata, e vale la pena scriverlo.**
> `grep -ril "candidate" Source/RefactorTactics --include=*.h` restituisce **cinque** file —
> `RTActionDef.h`, `RTCatalogLibrary.h`, `RTHexBotLibrary.h`, `RTPointerInteraction.h`,
> `RTMovementActionLibrary.h` — e la prima lettura è stata *«il concetto esiste già»*. **Falso**: in tutti e
> cinque `candidate` significa **mossa candidata del bot** — `BuildCandidates`, `ChooseBestPlan`, le
> candidate d'attacco che nascono da `ReachableCells`. È un'alternativa che il bot valuta **dentro un
> turno**, non una configurazione di skill che un designer confronta **fra due esecuzioni**.
> Due spazi semantici sotto la stessa parola: chi cercherà questo concetto lo ritroverà, e deve trovare
> anche il motivo per cui i cinque risultati non sono la risposta. Il gap si misura sul **significato**,
> non sull'occorrenza — e per questo `RT-FEAT-TOOL-SKILL-WORKBENCH` dovrà scegliere un termine che non
> collida: **`Variant`** invece di `Candidate` nei nomi C++, se e quando nascerà.

---

## 5. La scala di maturità, e perché non è una numerazione di release

Il sorgente §46 chiede di mappare `TD v0.1 → v1.0` sulla roadmap reale *«senza rinumerare arbitrariamente il
progetto»*. Applicato alla lettera, e con un precedente:

> 🔴 **Lo stesso errore è già stato respinto una volta.** Il consolidamento del 2026-08-13
> ([archivio](../../archive/src/RefactorTactics_Claude_Consolidamento_Roadmap_v1_0_2026-08-13.md) §9) ha
> dichiarato superata la milestone *«Skill Balance Lab v0.3»* proposta da un sorgente precedente, perché
> `RT-FEAT-TOOL-BALANCE-GROUND` era **già v0.1 `IMPLEMENTING`**. Una scala di maturità di uno *strumento*
> non è una release di *gioco*, e collocarla nella roadmap di release la fa competere con la consegna.

La scala vive in [`spec-tactical-designer.md`](../../technical/spec-tactical-designer.md) §6 come **stadi di
capability** — `TD 0.1` … `TD 1.0` — con una riga in testa che dice ciò che nessun lettore deve dedurre:
**`TD 0.7` non ha niente a che vedere con `RefactorTactics v0.7`**.

Mappatura sugli owner reali:

| Stadio | Owner reale | Stato misurato |
|---|---|---|
| **TD 0.1** Mappa deterministica + fondamenta scenario | `RT-FEAT-TOOL-MAP-EDITOR` · `RT-FEAT-TOOL-MAP-GEOMETRY` · `RT-FEAT-TEST-SCENARIO-HARNESS` · **M9.1** | quasi chiuso — residuo `#622`, `#623`, `#712`, sedute **U21 · U22 · U26** |
| **TD 0.2** Scenario Composer MVP | **G1** → `RT-FEAT-TOOL-SCENARIO-COMPOSER` (nuova) · **M9.4** | non cominciato |
| **TD 0.3** Skill Workbench MVP | **G2** → `RT-FEAT-TOOL-SKILL-WORKBENCH` (nuova) · **M9.4** | non cominciato |
| **TD 0.4** Loop integrato | **G4** — dipende da TD 0.2 + TD 0.3 | non cominciato |
| **TD 0.5** Probe ed explainability | `RT-FEAT-TOOL-MAP-EDITOR` — `#711`, `#695` | parziale |
| **TD 0.6** Registrazione e replay authoring | `RT-FEAT-REPLAY-ARCHIVE` (INTEGRATED) + conversione replay→scenario | metà esistente |
| **TD 0.7** Batch e analytics | `RT-FEAT-TOOL-BALANCE-GROUND` · **E43** (`#776`, v0.8) · `#543` · `#542` | epic aperta, issue `P0` aperte |
| **TD 0.8** Matrice, varianti, regressione | `RT-FEAT-UI-SCENARIO-BROWSER` · `RT-FEAT-TEST-GOLDEN` · [`scenario-map.md`](../../technical/scenario-map.md) | parziale |
| **TD 0.9** Governance e promozione | dipende da TD 0.3 | non cominciato |
| **TD 1.0** — | il DoD di §56, ridotto a ciò che è verificabile | — |

**Nessuna release di gioco è stata rinumerata, e nessuna epic esistente è stata spostata.**

---

## 6. Decisioni di questo consolidamento

**C1 — Una epic nuova, e SENZA numero `E`.**
Il sorgente §72 vieta *«una nuova Epic **se una esistente possiede già il lavoro**»*, e nessuna lo possiede:
`E23` (`#324`) possiede la logica di transizione e ospita l'anticipazione di authoring; `E43` (`#776`)
possiede la misura a lotti. Scenario Composer e Skill Workbench non hanno un padre — e `#622`, `#623`,
`#695`, `#711` oggi non ne hanno **nessuno** (verificato: `.parent` è `null` per tutte e quattro), cioè sono
orfane nella sola vista che il repository usa per il progresso, le **sub-issue**.

> 🔴 **La prima stesura prendeva `E48`, ed era l'errore che questo stesso referto esiste per prevenire.**
> `E48` era libera — massimo misurato `E47`, su `main`, su GitHub e su tutti i ref remoti — ma la
> numerazione `E` **è** la numerazione delle epic di release: `E1`–`E47` mappano tutte su `v0.1`…`v1.0` in
> `roadmap-v0.1.md` e `roadmap-post-v0.1.md`, e `known_roadmap_refs()` le legge da lì. Assegnare `E48` a uno
> strumento dichiarato `out_of_release_scope` lo avrebbe messo nella roadmap di release dalla porta di
> servizio, dopo due pagine spese a dire che non ci va.
>
> Il repository ha già la forma giusta e non l'avevo guardata: **`#839`** *«[EPIC] Lavoro parallelo —
> worktree, write-set e lease binarie»* e **`#422`** *«[EPIC] Wiki Player-First»* sono epic di **processo**,
> entrambe **senza numero `E`** e senza milestone. È lì che il Tactical Designer appartiene. Il numero
> `E48` resta libero per la prossima epic di release.

**C2 — Nessuna issue nuova per `#620`/`#621`/`#554`/`#588`.** Chiuse. Il sorgente le chiede tutte.

**C3 — Due feature nuove, non sei.** `G1` e `G2`. `G3` resta in `RT-FEAT-TOOL-MAP-EDITOR`, `G4` è un DoD di
`G2` e non una feature.

**C4 — Il naming *Tactical Designer* è di prodotto, non di API.** §45 lo chiede e questo consolidamento lo
applica: **zero rename** di classi, moduli o file. `URTHexEditorMode` resta `URTHexEditorMode`.

**C5 — La scala TD non entra in `roadmap-post-v0.1.md`.** Entra in `roadmap-checkpoint.md` come **M9.4**,
che è la vista di esecuzione degli strumenti, coerente con `out_of_release_scope` delle due feature d'editor.

---

## 7. Cosa NON è entrato, e perché

| Sezione | Chiedeva | Perché no |
|---|---|---|
| §5–§10, §25, §26, §34 | implementare `#620` e `#621` | chiuse il 2026-08-13 |
| §11 | policy di invertibilità del bake | chiusa da `D-131` (`bGenerated`) |
| §12 | separare bake logico e render rebuild | `RebuildInstances` completo è una difesa dichiarata; il sorgente stesso dice *«misura prima»*, e non c'è un numero |
| §13, §14 | `#554` reachability + vista | chiusa |
| §31A.5 `Power Budget` | indicatore euristico | nessun consumatore, e `D-102` dice che prima serve il competence gate del bot. Registrato in `spec-tactical-designer.md` §8 come guardrail, non come lavoro |
| §44 impact analysis | dependency graph degli scenari | `project-graph.json` **esiste già** e porta scenari e feature. Il gap è la query, non il grafo — resta un DoD di TD 0.8 |
| §45 | rename verso `RTTacticalDesigner*` | C4 |
| §57 | *«se la roadmap è generated, modificare la sorgente»* | applicato: `feature-registry.yaml` e `editor-sessions.yaml` sono sorgenti, le cinque `*.shortlist.md` e i due `.json` sono rigenerati |
| §74 | quattro branch paralleli | `parallel-batch.yaml` governa già il write-set (`D-139`). Non si aprono track per lavoro non cominciato |

---

## 8. Domande che restano aperte

Nessuna nuova voce in `OPEN_DECISIONS.md`: le due domande che il sorgente solleva hanno già una risposta o
un innesco dichiarato.

- **Invertibilità del bake** (§11) → chiusa, `D-131`.
- **Seed deterministico nello scenario** (§37) → 🔴 **questa riga diceva «il formato non ha un campo
  seed», ed è falso**: `FRTTestScenario::Seed` esiste, ed è documentato come *«dichiarato ma non consumato
  [...] il campo esiste perché il giorno in cui un RNG entrerà nel resolver lo scenario debba già saperlo
  dichiarare»*. Ha anche un **guardiano** — `RefactorTactics.Simulation.SeedIsDeclaredAndUnconsumed`
  verifica che due seed **diversi** diano lo stesso risultato, che è l'unico verso che morde su un progetto
  senza RNG. Chi introducesse casualità non troverebbe un campo mancante: **contraddirebbe un test verde**.
  Il *se* è aperto (`RNG-1`/`RNG-2`, [#960](https://github.com/DegrassiAaron/refactor-tactics-main/issues/960)),
  il *come* è già scritto. Terzo errore di misura di questo referto, e la forma è sempre la stessa: dedurre
  l'assenza invece di cercarla.
- **Reaction choices scriptate** (§37, §40.2) → è `D-101` / `#542`, già aperta e già tracciata.

---

## 9. Cosa è stato scritto su GitHub

| Atto | Dove |
|---|---|
| Epic creata, **senza numero `E`** | [#1105](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1105) — *«Tactical Designer — un solo loop fra mappa, skill e scenario»* |
| Sub-issue collegate | `#622`, `#623`, `#695`, `#711` — le quattro che non avevano padre |
| Sub-issue **non** spostata | `#712` resta di `E23` ([#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324)): un'issue ha un padre solo, e il gesto dell'autore è authoring di geometria prima che Tactical Designer |
| Issue nuova | [#1106](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1106) — il validator del registry non legge i checkpoint di milestone |
| Epic aggiornate | [#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324) (la relazione con `#695` va nel verso opposto all'anticipazione) · [#776](https://github.com/DegrassiAaron/refactor-tactics-main/issues/776) (la metà di misura non si muove, e l'ordine con `D-102` è scritto) |
| Issue **non** create | quelle di `TD 0.2` e `TD 0.3`: si aprono quando `TD 0.1` chiude, o la loro prima riga sarebbe «serve un consumatore che non esiste» |

`#1106` non era nel piano: nasce dall'aver dovuto scrivere `checkpoints: []` su due feature nuove e dall'aver
misurato **perché**. Il precedente è `D-138`, che corresse lo stesso difetto per le epic; il commento che
documenta quella correzione prevede da sé il caso del *terzo owner*, ed è arrivato.

## 10. Provenienza

Sorgente consumato e archiviato con banner in
[`docs/archive/src/`](../../archive/src/RefactorTactics_TacticalDesigner_03_Roadmap_v1.0_Scenari_SkillWorkbench_Claude.md).
Il sorgente cita due handoff precedenti come contesto storico: entrambi sono già in archivio —
[`2026-08-12-level-designer-01-context.md`](../../archive/src/handoff/2026-08-12-level-designer-01-context.md)
e
[`2026-08-12-level-designer-02-implementazione-consolidamento.md`](../../archive/src/handoff/2026-08-12-level-designer-02-implementazione-consolidamento.md).
