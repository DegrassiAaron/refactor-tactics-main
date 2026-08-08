# Audit — `RT_Showcase_Relay_v01` «Relay Basin»

> `CURRENT` · **Data**: 2026-08-08 · **HEAD misurato**: merge di `origin/main` (CP 9.3) in
> `docs/consolidamento-non-gameplay` · **Suite**: **432 test unici in 65 file**
> **Origine**: handoff `../../src/RefactorTactics_Claude_Roadmap_Docs_Tests_Showcase_v01.md`
>
> **Cosa è**: la misura dello scarto fra ciò che il repository fa oggi e ciò che serve perché
> `RT_Showcase_Relay_v01` giri per intero. **Cosa non è**: una specifica — quella è
> [`../../product/showcase-v0.1.md`](../../product/showcase-v0.1.md), che resta l'owner unico della showcase.

---

## 1. Il punto di partenza è migliore del previsto

**Tutte e diciotto le abilità nominate dagli 8 turni esistono già a catalogo**: `FluidTrail`, `KineticPanel`,
`ConductiveNode`, `PressureJet`, `Reconfigure`, `InterceptShot`, `LinearDischarge`, `PassingBlade`,
`CircularTide`, `MistVeil`, `Deflection`, `Interposition`, `Ram`, `CreateWater`, `Ignite`, `Electrify`,
`ReactiveCapacitor`, `FlowReaction`.

**Tutte e otto le superfici richieste esistono** in `ERTHexSurface`: `Floor`, `ShallowWater`, `Rough`, `Fire`,
`Conductive`, `Ice`, `Smoke`, `HighGround` (più `Void`).

Lo scarto quindi **non è nel contenuto**. È in quattro sistemi mancanti e in un harness che non sa esprimere
ciò che la showcase gli chiederebbe.

## 2. Stato per area

| Area | Stato | File principali | Test | Gap verso la showcase |
|---|---|---|---|---|
| Coordinate e mappa | **READY** | `Map/RTCellId.h`, `RTHexMapAsset` (formato **v4**) | 85 `Hex*` | — |
| Pathfinding · LOS · targeting | **READY** | `RTHexPathLibrary`, `RTHexVisionLibrary` | *(inclusi sopra)* | — |
| Fasi, snapshot, resolver | **READY** | `RTTurnManager`, `RTHexSimLibrary`, `RTActionQueueLibrary` | 58 `Actions` · 27 `HexSim` | — |
| TurnLog | **READY** | `RTTurnLogLibrary` | 22 | export `turnlog.jsonl` |
| Catalogo eroi e azioni | **READY** | `RTHeroCatalogLibrary`, `RTCatalogLibrary` | 25 `Heroes` · 9 `Catalog` | — |
| Terreni, stati, propagazione | **READY** | `RTTerrainLibrary` | 39 | — |
| Coperture | **READY** | `RTHexCoverLibrary` | 13 `Cover` | — |
| Strutture | **PARTIAL** | `RTHexDoorLibrary` (CP 9.3) | 10 `Structures` | **porte ✅** · ponti ⏳ CP 9.4 · gate = porta ✅ |
| Scenario Harness | **PARTIAL** | `ScenarioHarness/` | 13 `Scenario` | **intent = solo movimento** · **2 assertion** · niente reaction policy, stato iniziale di superfici/strutture, modi |
| Objective | **MISSING** | `ERTMatchOutcome::Objective` | `Match.EndsOnObjective` | **c'è il giudice, non la fonte** |
| Facing | **MISSING** | solo presentazione | — | E16 |
| Overwatch / Decision Boundary | **MISSING** | — | — | E14 (dipende da E13) |
| Predictive Action | **MISSING** | `InterceptShot` a catalogo, `Slot::None`, nessun trigger | — | E18 |

### 2.1 Il gap vero: l'harness

Oggi uno scenario sa dire **chi c'è, dove va, quanti turni**. La showcase gli chiederebbe di dire anche
*quale abilità*, *con quale bersaglio*, *quali superfici all'inizio*, *quali strutture*, *come rispondere a una
finestra di reazione* e *cosa verificare* su venticinque dimensioni invece di due.

| Dimensione | Oggi | Serve |
|---|---|---|
| Intent | `Move` (waypoint) | + abilità con bersaglio, + facing |
| Stato iniziale | celle sovrascritte (`cells[]`) | + superfici, coperture, porte, objective |
| Reaction | — | `Hold`, `CommitFirstValid`, `HoldFirstThenCommit`, `CommitSpecificTarget`, `Timeout` |
| Assertion | `UnitAtCell`, `TurnsCompleted` | ~25 (stati, superfici, bordi, revisione, reazioni, predizione, objective, hash) |
| Report | `result.json` | + `turnlog.jsonl`, `state_initial.json`, `state_final.json` |
| Modi | uno | `VISUAL` · `FAST` · `HEADLESS`, logicamente equivalenti |

---

## 3. Conflitti

### 3.1 Quattro file di input dichiarati **non esistono**

L'handoff §0 dichiara come input, e la §1 mette al **rank #2**, file che non sono nel repository:

| File | Rank | Stato |
|---|---|---|
| `RT_Showcase_Relay_v01_ScenarioSpec_Claude.md` | **2** | **assente** |
| `RT_Showcase_Relay_v01_ScenarioDraft.json` | 3 | **assente** |
| `Testing automatico Cloud.txt` | 6 | **assente** |
| `Revisione sequenza turno.txt` | 5 | **assente** |
| `mappa_tattica_del_bacino_relay.png` · `..._board_im.png` | 9 | **assenti** |
| `RefactorTactics_Overwatch_FastReaction_Claude.md` | 4 | ✅ presente |
| `RefactorTactics_Rumore_Claude.md` | — | ✅ presente |

**Perché si può procedere lo stesso.** L'handoff **riporta per esteso** ciò che i due file assenti più
importanti avrebbero portato: la §7 dà la mappa (45 celle, forma per riga, spawn, Relay a `(0,0,0)`, elenco dei
terreni e degli elementi) e la §8 dà gli 8 turni azione per azione. Gli altri tre sono superati dalla realtà:
la sequenza del turno e l'harness **esistono già** nel repository, e le immagini per stessa dichiarazione della
§0 non prevalgono sul testo.

**Cosa manca davvero**: l'assegnazione delle **celle** ai terreni, e la posizione di coperture, gate e bridge.
Su richiesta dell'autore (2026-08-08) quel layout è stato **autorato in questo passaggio** — è dichiarato tale
in [`../../product/showcase-v0.1.md`](../../product/showcase-v0.1.md) §mappa, **non ereditato da una fonte**.
Se la spec originale riemerge, il layout va confrontato: schema, harness, roadmap e test restano validi.

### 3.2 Il documento della showcase esiste già

L'handoff §16.B chiede `docs/gameplay/showcase-v0.1-relay-basin.md`. Il repository ha già
`docs/product/showcase-v0.1.md` come owner canonico, e la §16.C dello stesso handoff dice di **non** creare una
fonte concorrente. **Risolto verso l'esistente**: la regola «un concetto, un owner» del
[`README`](../../README.md) vale più del path suggerito.

### 3.3 Il «Bridge Edge» non ha un modello su un solo layer

La §7 fissa `Layer = 0` per tutte le 45 celle **e** chiede un `Bridge Edge`. Nel repository `FRTHexEdge` è
riservato per decisione esplicita alle **sole transizioni fra layer**
([D-013](../../decisions/RT_PDR_00_Decision_Log.md)): un ponte fra due celle dello stesso layer non è un arco.

**Risolto** modellandolo come **struttura di bordo**, la stessa famiglia delle porte di CP 9.3 — che è
esattamente ciò che **CP 9.4 (ponti)** sta costruendo. Non è stato inventato un meccanismo nuovo: è stata
riconosciuta una dipendenza. Il bridge della showcase è quindi **bloccato su CP 9.4**, e lo scenario lo
dichiara.

### 3.4 Conflitti storici della §15: **già risolti**

L'handoff chiede di controllare quattro punti. Il consolidamento documentale del 2026-08-08 li ha già chiusi;
qui non c'è nulla da correggere:

| Punto §15 | Stato nel repository |
|---|---|
| Reaction da 5 secondi | ✅ baseline **3,0 s**, `Timeout → HOLD` ([ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md) §8) |
| Roster Aegis/Nyx/Drift/Vex | ✅ solo in righe che li dichiarano storici |
| Sequenza turno arbitraria | ✅ `Planning → Prep → Dash → Blast → Move → Cleanup`, canonica |
| Logica square-grid | ✅ rimossa al CP 7.2, verificata assente in `Source/` |

### 3.5 Determinismo: la progressione parte da 100, non da zero

La §10 chiede `Repeat 10 → 100 → 1000`. **CP 12.1 è già a 100 ripetizioni**
(`Simulation.DeterministicReplay`), con `ChecksumStableAcrossPermutations` e
`GoldenCorpusRejectsFormatMismatch`. La progressione residua è `1000` + equivalenza `Visual/Fast/Headless`.

---

## 4. Decisioni prese in questo passaggio

| # | Decisione | Perché |
|---|---|---|
| 1 | Il layout dei terreni è **autorato**, non ereditato | La spec di rank #2 è assente; l'autore ha autorizzato a procedere |
| 2 | La showcase resta in `product/showcase-v0.1.md` | Un solo owner per concetto |
| 3 | Il **gate del turno 5 è una porta** (CP 9.3) | Il meccanismo esiste già: stato di bordo, blocco del passo, revisione che sale |
| 4 | Il **bridge è bloccato su CP 9.4** | Non si inventa un arco su layer singolo contro D-013 |
| 5 | L'harness si **estende**, non si sostituisce | Il draft JSON è assente e lo schema esistente regge; §6.1 chiede di preferire l'estensione |

## 5. Decisioni ancora bloccate

| # | Domanda | Perché non la decido io |
|---|---|---|
| 1 | La policy per il **moving target** del turno 3 | «secondo la policy reale definita dal catalogo»: va letta dai dati di `LinearDischarge`, e se il catalogo non la dichiara è una decisione di gameplay, non documentale |
| 2 | Come si **contende** il Relay (turno 8, Bastion fallisce «per una causa reale del ruleset») | Il sistema objective non esiste: la causa del fallimento non può essere scelta prima di sapere quali cause il ruleset ammette |
| 3 | Se la **Predictive Action** consuma lo slot principale | Tocca l'action economy, che è materia da decisione esplicita (D-014/D-025) |

---

## 6. Backlog implementativo

Ordinato **secondo la roadmap reale**, non per dominio: ogni voce è una fetta verticale
`scenario → feature → test automatico → risultato visibile`.

### `S2-1` — Lo scenario sa riferire una fixture nominata

| | |
|---|---|
| **Goal** | Uno scenario può dire `"mapId": "ShowcaseRelayBasin"` invece di ridisegnare la mappa nel JSON |
| **Scope** | Campo `mapId` in `FRTTestScenario`; risoluzione nome → fixture in `URTScenarioLoader`; `mapRadius`/`cells[]` restano validi e si applicano **sopra** la fixture |
| **Non-goals** | Caricare `.umap`; un registry generico di mappe |
| **Dipende da** | ✅ `MakeShowcaseRelayBasinArena` |
| **File** | `ScenarioHarness/RTTestScenario.h`, `RTScenarioLoader.*` |
| **Acceptance** | Uno scenario con `mapId` sconosciuto è **`ERROR`**, non `FAIL`; con `mapId` valido l'arena coincide cella per cella con la fixture |
| **Test** | `Scenario.LoaderResolvesNamedMap`, `Scenario.LoaderRejectsUnknownMapId` |
| **Commit** | `feat(harness): scenari che riferiscono una fixture di mappa per nome` |

### `S2-2` — Gli intent sanno esprimere un'abilità

| | |
|---|---|
| **Goal** | `"ability": { "actionId": "...", "targetCell": [...] }` accanto a `"move"` |
| **Scope** | Estensione di `FRTScenarioIntent`; il runner li instrada nello **stesso** percorso di pianificazione del giocatore |
| **Non-goals** | Reaction policy (è `S5-1`); facing (è `S2-3`) |
| **Dipende da** | `S2-1` |
| **Acceptance** | Un `actionId` fuori catalogo è **`ERROR`**; un'abilità legale produce le stesse voci di TurnLog di una pianificata a mano |
| **Test** | `Scenario.AbilityIntentResolvesLikePlanned`, `Scenario.UnknownActionIdIsError` |
| **Commit** | `feat(harness): intent di abilita' negli scenari` |

### `S2-3` — Turno 1 della showcase

| | |
|---|---|
| **Goal** | `RT.Scenario.Showcase.T1` verde attraverso il gameplay reale |
| **Scope** | `Scenarios/Showcase/RelayBasin_T1.json`; assertion `UnitAtCell` sui quattro arrivi |
| **Non-goals** | Facing (E16) e `CreateCover` (CP 9.5): il turno 1 li **dimostra** ma non li richiede per passare |
| **Dipende da** | `S2-1`, `S2-2` |
| **Acceptance** | Nessun `SetActorLocation`; lo `StateHash` è stabile su 10 ripetizioni |
| **Test** | `RT.Scenario.Showcase.T1` |
| **Commit** | `feat(showcase): turno 1 eseguibile dallo scenario` |

### Voci successive, in ordine

| ID | Titolo | Sblocca | Dipende da |
|---|---|---|---|
| `S4-1` | Leggere e registrare la policy di **moving target** dal catalogo | T3 | catalogo |
| `S6-1` | Assertion `EdgeEnabled`/`EdgeDisabled`/`GraphRevisionChanged` | T5 | ✅ CP 9.3 |
| `S8-1` | Assertion su superfici e stati (`SurfaceHasStatus`, `UnitHasStatus`) | T7 | ✅ E8 |
| `S7-1` | **D-017**: `Intercept` rivalida la geometria sul bersaglio effettivo + test discriminante | T6 | E5 |
| `S3-1` | Predictive Action, slice `Vektor.InterceptShot` (**E18**) | T2, T8 | D-016 |
| `S5-1` | `reactionPolicy[]` + Decision Boundary (**E14**) | T4 | E13, E16 |
| `S9-1` | Objective `Relay` contendibile (**E10 CP 10.1/10.2**) | T8, Full | — |
| `S10-1` | `turnlog.jsonl` + campi mancanti di `result.json` | Golden | — |
| `S10-2` | `Repeat 1000` ed equivalenza `Visual`/`Fast`/`Headless` | Golden | S9-1 |

**Definition of Done, per ognuna** — non basta che si veda in PIE:

1. passa dal gameplay/resolver reale; 2. nessun caso speciale dello scenario; 3. deterministica;
4. TurnLog e reason code; 5. test automatico; 6. visualizzazione sufficiente a diagnosticare;
7. compatibile con l'autorità di rete prevista; 8. nessun leak di planning; 9. documentazione aggiornata;
10. packaged test dove il livello della feature lo richiede.
