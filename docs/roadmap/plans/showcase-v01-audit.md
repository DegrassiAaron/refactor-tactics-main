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

### `S2-1` — Lo scenario sa riferire una fixture nominata — ✅ **FATTO 2026-08-08**

> Consegnato: campo `fixture`, `URTMatchSetupLibrary::MakeFixtureArena`, esito **`Blocked`** con `requires`
> per turno, e lo scenario versionato `Scenarios/RT_Showcase_Relay_v01.json`. Il turno 1 gira attraverso il
> resolver normale e `result.json` riporta `BLOCKED` + `blockedReason`. Suite: **437 verdi, 0 fallimenti**.

| | |
|---|---|
| **Goal** | Uno scenario può dire `"fixture": "RelayBasin"` invece di ridisegnare la mappa nel JSON |
| **Scope** | Campo `mapId` in `FRTTestScenario`; risoluzione nome → fixture in `URTScenarioLoader`; `mapRadius`/`cells[]` restano validi e si applicano **sopra** la fixture |
| **Non-goals** | Caricare `.umap`; un registry generico di mappe |
| **Dipende da** | ✅ `MakeShowcaseRelayBasinArena` |
| **File** | `ScenarioHarness/RTTestScenario.h`, `RTScenarioLoader.*` |
| **Acceptance** | Uno scenario con `mapId` sconosciuto è **`ERROR`**, non `FAIL`; con `mapId` valido l'arena coincide cella per cella con la fixture |
| **Test** | `Scenario.LoaderResolvesNamedMap`, `Scenario.LoaderRejectsUnknownMapId` |
| **Commit** | `feat(harness): scenari che riferiscono una fixture di mappa per nome` |

### `S2-2` — Gli intent sanno esprimere **tutti gli slot** che un giocatore pianifica

> *Riscritta il 2026-08-08 dopo `/sc:spec-panel`. La prima stesura proponeva un solo campo `ability` con un
> `targetCell`, e non sarebbe bastata: gli otto turni usano quasi solo **bersagli-unità**, e un intent non è
> un'azione — sono **quattro slot indipendenti**.*

| | |
|---|---|
| **Goal** | Un intent esprime ciò che un giocatore dichiara in un turno: movimento, azione principale, dash, reazione |
| **Forma** | `{ "unit", "move": [...], "main": { "actionId", "targetUnit" \| "targetCell" }, "dash": { "actionId", "cell" }, "reaction": { "actionId" } }` |
| **Scope** | Estensione di `FRTScenarioIntent` · risoluzione `actionId → indice` sul **kit dell'eroe** · `URTCatalogLibrary::ValidateActionSlots` applicata agli intent · il runner li instrada nello **stesso** percorso di pianificazione del giocatore |
| **Non-goals** | Reaction **policy** e finestre (`S5-1`/E14) · facing (**E16**, non `S2-3`) · `PlannedCleansePriority` · `preCommitValidation` · reset dei piani nel runner *(vedi nota)* |
| **Dipende da** | `S2-1` ✅ |
| **Acceptance** | 1. `actionId` fuori catalogo **o fuori dal kit dell'eroe** → **`ERROR`** col nome<br>2. azione in **cooldown** → si risolve col `Fallback` dichiarato, **non** `ERROR`: è comportamento del gioco, non difetto dello scenario<br>3. due azioni principali nello stesso turno → **`ERROR`**<br>4. **primaria**: asserzioni sull'**effetto** (danno applicato, stato, voci di TurnLog attese)<br>5. **complemento**: piano da scenario e piano scritto sui campi di `ARTUnit` → stesso `LogHash` |
| **Test** | `Scenario.AbilityIntentAppliesExpectedEffect` · `.UnknownActionIdIsError` · `.ActionNotInHeroKitIsError` · `.CooldownFallsBackNotErrors` · `.TwoMainActionsIsError` · `.AbilityIntentMatchesDirectPlanHash` |
| **Sblocca** | **T3** e **T7** per intero · **T5 solo in parte**: gate e `GraphRevision` sì, la *correzione del piano* no — richiede `preCommitValidation`, che è fuori scope |
| **Commit** | `feat(harness): intent a slot nominati negli scenari` |

**Perché l'acceptance 4 viene prima della 5.** Il confronto di `LogHash` confronta lo scenario con un piano
scritto a mano **nel test**: se entrambi sbagliano allo stesso modo — per esempio saltando una validazione che
il controller fa — resta verde e misura due percorsi ugualmente falsi. Da solo è un test che si guarda allo
specchio. Le asserzioni sull'effetto dicono *cosa deve succedere*; l'hash cattura le derive.

> **Nota — il reset dei piani è fuori scope, e non è più un rischio.** La prima stesura del panel lo dava per
> difetto latente («il runner azzera solo il movimento»). **Falso**: il resolver consuma i piani in otto punti
> e `ARTUnit::PlaceOnCell` riallinea il movimento. L'invariante è ora **pinnata** da
> `Turn.PlansDoNotSurviveTheTurn` e `Turn.DiscardedPlanDoesNotSurviveTheTurn`, verificate con mutazione. Il
> reset nel runner risulta quindi **ridondante, non divergente**: toglierlo è una pulizia a effetto nullo e
> vive come voce a sé (`S2-2b`), non dentro S2-2.

### `S2-2b` — Togliere il reset ridondante dei piani nel runner *(pulizia, non bloccante)*

| | |
|---|---|
| **Goal** | Il runner smette di riazzerare i piani di movimento a ogni turno |
| **Perché** | `PlaceOnCell` lo fa già. Un secondo owner dell'invariante può **mascherare** una regressione del resolver: lo scenario resterebbe verde mentre il gioco vero, in PIE e col giocatore, si comporta male — la stessa famiglia del `SetActorLocation` vietato |
| **Non-goals** | Cambiare la semantica di «turno senza intent», che resta *l'unità non fa nulla* perché è già quella del gioco |
| **Acceptance** | Suite invariata; `Turn.PlansDoNotSurvive*` restano verdi senza il reset |
| **Rischio** | Nullo oggi, misurato: nessun comportamento dipende da quel reset |

### `SLOT-1` — Gli slot per turno sono una regola che nessuno fa rispettare *(non è lavoro di showcase)*

> Aperto il **2026-08-08** verificando se `S2-2` potesse appoggiarsi a `ValidateActionSlots`. Registrato in
> [`../../DOC_CONFLICT_MATRIX.md`](../../DOC_CONFLICT_MATRIX.md) riga 43, stato **`OPEN`**.

| | |
|---|---|
| **Il fatto** | Il catalogo dichiara uno slot per azione e `URTCatalogLibrary::ValidateActionSlots` implementa la regola. **Nessuno la chiama in partita**: solo due test (`RTOffensiveActionTests`) |
| **Chi la viola** | **Entrambi.** `ARTPlayerController` imposta `PlannedDashAbility` e `PlannedAbilityIndex` senza azzerare l'altro, e il bot pianifica scatto + attacco di proposito (`RTTurnManager` nota `#145`) — pur occupando entrambi lo slot `Main` |
| **Perché conta** | Il commento `#145` dice che «scatto + attacco base **domina sempre** la carica» (30 danni e spinta 2 contro 20 e spinta 1, cooldown 0 contro 3). Non è un dettaglio di bilanciamento: è una combinazione che il catalogo vieta e la partita premia |
| **La scelta** | **Applicare** la regola (il controller e il bot azzerano lo slot opposto) **oppure** cambiare gli slot del catalogo perché il Dash non prenda `Main`. Non una terza via: oggi il documento dice una cosa e la partita ne fa un'altra |
| **Effetto su `S2-2`** | Se l'harness applicasse `ValidateActionSlots` agli intent, **sarebbe più severo del gioco**: uno scenario verrebbe rifiutato per una combinazione che in PIE il giocatore può pianificare. È legittimo solo dichiarando che l'harness valida **il documento scenario**, non fa rispettare **una regola di gioco** |
| **Non-goals** | Ribilanciare Dash e Charge: qui si decide *quale regola vale*, non quanto danno fa |

**È la stessa forma dell'altra riga `OPEN`** (34, APNAP a sei gruppi): una regola normativa scritta, corretta,
e senza consumatore runtime. Il difetto ricorrente di questo repository non è la formula sbagliata — è il dato
che nessuno legge.

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
