# DIR-C HANDOFF — QA / Scenario / Bot / Autobattle

> `CURRENT` · **Data**: 2026-08-28 · **Lane**: DIR-C (QA, Scenario Harness, Bot v0.1, Autobattle,
> corpus deterministico).
>
> **Vincoli**: nessun `.uasset` / `.umap` / `Content/**` toccato, nessuna PIE, nessun packaging.
> ⚠️ **`UnrealEditor-Cmd` è stato avviato**, per automation headless: §1 e §16 lo vietano a DIR-C, e il
> committente ha autorizzato la deroga il 2026-08-28. Il dettaglio è in §6.3 — un vincolo aggirato senza
> traccia è peggio di un vincolo che non c'era.
>
> **Cosa è**: la misura dello stato reale delle lane QA/Bot/Autobattle su `HEAD`, più il lavoro di test
> consegnato. **Cosa non è**: una roadmap QA nuova — non ne è stata creata nessuna, e nessun tracker o
> registry rimosso è stato reintrodotto.

---

## HEAD

| | |
|---|---|
| **HEAD iniziale** | `01aac418` — **spostato sotto i piedi durante il pre-flight** |
| **HEAD misurato** | `ad7f212b` (`origin/main`, allineato) |
| **HEAD finale DIR-C** | `test/dir-c-qa-scenario-bot-autobattle` — elenco commit in §5 |
| **Worktree** | `D:/Repositories/wt-dir-c-qa` |

🔴 **Il clone è condiviso, e questo va detto prima di ogni altra cosa.** Fra il primo e il secondo comando
del pre-flight un'altra sessione ha eseguito `pull --tags origin main` sullo stesso `.git`, portando `HEAD`
da `01aac418` a `ad7f212b`. Più tardi la stessa mano ha **cancellato da disco** i worktree `wt-1095` e
`wt-dirc` — entrambi risultavano `prunable` mentre DIR-C ci stava leggendo dentro. La lane è stata ricreata
su `D:/Repositories/wt-dir-c-qa`. **Chi lavora in parallelo su questo clone lo faccia da un worktree e non
cancelli quelli altrui**: un worktree rimosso a metà lettura somiglia a un repository corrotto e non lo è.

---

## 1. Misure, non conteggi copiati

Tutto quanto segue è misurato su `ad7f212b`, non ripreso da documenti.

| Cosa | Valore misurato | Nota |
|---|---|---|
| Scenari spediti | **86** `.json` (`Scenarios/**`, escluso `_redirects.json`) | 5 free-run, 8 turni per lo showcase |
| File di test | **143** in `Source/RefactorTactics/Tests/` | 141 su `ad7f212b`, più il probe e il suo test |
| Test dichiarati | **1291** nomi unici in `Tests/`, **1298** con Editor e plugin | riconciliati con gli eseguiti in §7.4: **1298 su 1298** |
| Corpus golden | **6** scenari, **10** file `.rttl` | organizzato per **categorie di log**, non per scenario |
| Categorie coperte dal golden | soglia **≥ 8**, due scoperte **dichiarate** (`Fallback`, `ReactionClash`) | `Simulation.GoldenCorpusCoversItsCategories` |

⚠️ **`docs/roadmap/parallel-batch.yaml` non esiste.** La direttiva lo elenca sia fra le letture di pre-flight
sia fra i file da non modificare. Nel repository non c'è: esistono `execution-graph.yaml` e
`editor-sessions.yaml`. Non è stato creato — §2 vieta di reintrodurre registry rimossi.

---

## 2. Autobattle — root cause

> La direttiva descrive «match reale/default → 12 round → 0 combat → pareggio» come *difetto vivo da
> riverificare*. **Riverificato: è chiuso, due volte, e la premessa del titolo era falsa.**

### 2.1 Prima causa — stato assorbente dal bonus di quota (#1088, PR #1274 `f3d0ffa3`)

`URTHexBotLibrary::ScorePlan` applicava `WElevation × Layer` come termine **assoluto** di posizione,
incassato ogni turno anche stando fermi.

| piano | `WElevation·Layer` | `WApproach·dist` | totale |
|---|---|---|---|
| **restare** `(2,-1,L1)` | +20 | −40 | **−20** ← vinceva |
| scendere `(1,0,L0)` | 0 | −30 | −30 |

**Fix su `HEAD`, verificato**: `WElevation` **20 → 4** (`RTHexBotLibrary.h:156`), più il ramo mancante del
kiter. L'invariante `WElevation × MaxLayer < WApproach` è **presidiata a runtime**, non solo commentata:
`RTTurnManager.cpp:327` calcola `MaxLayer` dalle celle reali della mappa e denuncia la violazione. Questo
chiude il *«`MaxLayer` è dichiarato, non garantito»* che il consuntivo di #1088 lasciava aperto.

⚠️ **Il titolo di #1088 resta falsificato**, e chi legge la direttiva va avvertito: i bot **non** «non
attaccano mai». Ingaggiano presto — misurato allora: primo `Combat` al turno 2, 23 voci `Combat`. Il difetto
era «ingaggiano e non concludono».

### 2.2 Seconda causa — nessun termine «da qui posso ingaggiare» (#1287 → #1296 → #1300, `D-185`)

Due difetti distinti sulla stessa lacuna, e **non sono lo stesso difetto con due nomi**:

| | forma | firma misurata su `L_HexArena` |
|---|---|---|
| #1287 | filtro sul dominio delle candidate | **oscillazione di periodo due** — 8 alternanze in 12 turni; 37 in 40 sullo scenario `AutoBattle.ArenaV01` |
| #1296 | nessun filtro, avvicinamento in passi sul grafo | **parcheggio cieco** — 7 turni fermi senza sparare |

**Chiuso da `D-185`**, e implementato su `HEAD`: `Score += max(0, WEngage − WEngageDecay × IdleTurns)` sulle
celle che vedono, nei piani senza attacco. `WEngage = 15`, `WEngageDecay = 5`
(`RTHexBotLibrary.cpp:411-419`), con `IdleTurns` propagato da `RTTurnManager.cpp:628` e pinnato da
`HexBot.EngageBonusFadesWithIdleTurns`.

∴ **La causa è nominata e misurata a monte. DIR-C non ha trovato un difetto attivo del bot da correggere**,
e non ne ha inventato uno per avere qualcosa da consegnare.

---

## 3. Il difetto che DIR-C ha trovato: l'oracolo della configurazione spedita era cieco a metà

Lo stato assorbente ha **due firme**, e fino a oggi erano coperte in due posti diversi e **non simmetrici**:

| firma | oracolo | mappa |
|---|---|---|
| punto fisso (parcheggio) | `Match.Autobattle.NobodyParksOnTheAuthoredMap` | mappa d'autore |
| punto fisso (parcheggio) | `Match.Autobattle.EngagesOnTheGeneratedTestArena` | **configurazione spedita** |
| ritorno di periodo due (oscillazione) | `Match.Autobattle.NobodyOscillatesOnTheAuthoredMap` | mappa d'autore |
| ritorno di periodo due | 🔴 **nessuno** | **configurazione spedita** |

`EngagesOnTheGeneratedTestArena` è l'**unico** test che gira su `MapSource = GeneratedTestArena`, cioè su
ciò che la partita non presidiata carica dopo #1069 — e il suo contatore misura i turni **consecutivi sulla
stessa cella**, che un'alternanza `A→B→A→B` azzera a ogni turno. Il difetto era **dichiarato nel commento
del test e non chiuso**.

Non è teorico: è precisamente la forma con cui #1287 è passato.

**Consegnato**:

- `Source/RefactorTactics/Tests/RTOrbitProbeForTest.h` — il rilevatore `Cell[t] == Cell[t-2]` con
  `Cell[t] != Cell[t-1]`, **estratto** dall'oracolo della mappa d'autore invece che riscritto. Una
  implementazione sola: due copie della stessa firma sarebbero divergute in silenzio.
- `EngagesOnTheGeneratedTestArena` — asserzione anti-oscillazione con **soglia e premessa proprie**,
  derivate dai turni *giocati* e non dal `RoundLimit`. Condividere la soglia del parcheggio avrebbe fatto
  ritarare l'una muovendo l'altra — è la ragione per cui `MaxLegitimateStillTurns` e `FirstBloodDeadline`
  sono già due nomi distinti in quel file.
- **Correzione di un `else` senza graffe** che si legava allo statement sbagliato: saltava la *premessa*
  sugli `StableUnitId` invece dell'asserzione anti-parcheggio che dichiarava di saltare. ⚠️ **Latente**: la
  condizione è vera solo per `RoundLimit ≤ 3` e il formato spedito ne porta 12, quindi sulla configurazione
  reale il ramo preso non cambia.

🔴 **Limite dichiarato, e sta nella sonda**: si vede il periodo **due**, non il tre. Un'orbita `A→B→C→A` non
la coglie nessuno dei due oracoli. Non è un'omissione da riparare di lato — chiede una storia per unità e una
soglia propria, e nessun difetto misurato l'ha ancora prodotta.

---

## 4. Audit per area

### Scenario / Complete match (§7 della direttiva)

✅ **Esiste già, e non ne è stato creato un duplicato.** `AutoBattle.ArenaV01`
(`Scenarios/AutoBattle/ArenaV01.json`) è lo scenario «dall'avvio alla vittoria»: mappa multilivello,
spawn dichiarati, terminazione **per eliminazione al turno 19**, con
`Scenario.FreeRun.ArenaV01ReachesAWinner` che esclude il pareggio **per nome**. Il compagno è
`FreeRun.ShippedOpenFieldReachesAWinner`. `FreeRun.CapReachedIsFailNotPass` rende il tetto un `Fail`, quindi
un ciclo che non decide **non** passa per esaurimento.

⚠️ Il segmento `objective` della catena chiesta da §7 **non è dimostrabile oggi** — vedi sotto.

### Showcase (§8)

Misurato, non assunto. `RT_Showcase_Relay_v01` dichiara **8 turni** e ne completa **5**:

| turno | `requires` | esito |
|---|---|---|
| T1 · T3 · T7 | — | esegue |
| T2 | `PredictiveAction` | ✅ disponibile |
| T4 | `DecisionBoundary` | ✅ disponibile (uscita dalle non disponibili con #512 fase B) |
| T5 | `ReactionPlanning` | ✅ disponibile |
| **T6** | `InterceptRevalidation` | 🔴 **BLOCKED** — owner **#1060** |
| T8 | `PredictiveAction` + `Objective` | 🔴 **BLOCKED: Objective** — owner **#75** |

`expect` dichiara `TurnsCompleted = 5`: coerente con la misura. Risposte alle domande di §8:

- **prediction che può whiffare** — ✅ `LogEventCount Predictive/PredictionWhiffed = 1`, più
  `Spec/Predictive/WhiffOnEmptyCell` **con golden**.
- **Overwatch HOLD / FIRE** — ✅ `Spec/Overwatch/HoldThenFire` (con golden),
  `Overwatch.HoldKeepsArmed`, `Overwatch.TimeoutIsHold`.
- **movement truncation** — ✅ `Overwatch.FireTruncatesFutureMovement`.
- **redirect / interposition** — 🔴 **BLOCKED: InterceptRevalidation**. La feature esiste ed è chiusa
  (#200, tre test la pinnano); ciò che manca è il **vocabolario di assertion** — `OriginalTargetEquals` /
  `EffectiveTargetEquals` non esistono, e `ERTReactionOutcome` non distingue il redirect. **Non è stato
  simulato.**
- **environment interaction** — ✅ disponibile.
- **objective finale** — 🔴 **BLOCKED: Objective**.

⚠️ `docs/roadmap/plans/showcase-v01-audit.md` è **stale**: dichiara «432 test unici in 65 file» (2026-08-08)
contro i **1298 in 143** misurati oggi, e dà `Structures` PARTIAL / `Overwatch` MISSING quando entrambe sono
disponibili. Va aggiornato o marcato storico — non è stato toccato perché non è materiale DIR-C.

### Reaction (§9)

Coperto, con i nomi che il repository usa davvero: `Overwatch.TimeoutIsHold`, `HoldKeepsArmed`,
`HoldResumesSameMovementState`, `FireTruncatesFutureMovement`, `SecondFireOnDownedTargetLogsNoDamage`,
`SimultaneousTargetsSingleOpportunity`, `OrderIsDeterministic`, `PromptsAreCapped`,
`OpportunityLeaksNoFuture`. Il replay «registrato FIRE/HOLD → nessun nuovo prompt» è
`Overwatch.DecisionIsReplayable`, e lo scenario è `Scenario.OverwatchHoldThenFireConsumesBothDecisions`.

**Nessun DTO inventato**: `Outcome Preview` non è stato toccato.

### Objective (§10)

🔴 **BLOCKED: Objective** (owner **#75**). La capability è dichiarata **non disponibile**, e i due scenari
che la chiedono — `Spec/Objective/PointSurvivesKO` e `AutoBattle/Objective` — sono BLOCKED per costruzione.
Il giudice esiste (`Match.EndsOnObjective`, `ERTMatchOutcome::Objective`); **la fonte no**.

`Objective_Uncontested` / `Objective_Contested` / `Objective_MatchEnd` **non sono stati creati**: scriverli
oggi produrrebbe tre file BLOCKED, cioè scenari che nessuno esegue e che sembrano copertura. Vanno aperti
insieme a #75, da chi sposta la capability.

### Determinismo (§11)

Il corpus è organizzato per **categorie di log**, non per scenario, e lo dichiara. `SameSeedGivesSameResult`
è onesto sul proprio stato: *«oggi è vero per costruzione»* — il runtime **non ha alcun RNG**, e
`FRTTestScenario::Seed` è «dichiarato ma non consumato». Il suo valore è il giorno in cui smette di esserlo.
Non è stato aggiunto nulla: aggiungere `Repeat`/`Permutation` dove la proprietà è strutturale
gonfierebbe la suite senza aumentare ciò che può cadere.

### Autobattle come modalità (§12)

`DeterminismIsIndependentOfPlayback` · `DeterminismSurvivesUnitPermutation` ·
`PlanningSecondsNeverStallAnUnattendedMatch` · `AllWaitEndsTheTurnNormally` ·
`NoPathProducesLegalFallback` · `SimultaneousKOFollowsDeclaredPolicy`. La UI dello speed control resta
DIR-A (#1015).

### Fairness (§14)

✅ `HexBotPlay.HiddenEnemyFairness` con il gemello dichiarativo `Spec/Bot/HiddenEnemyFairness`, più il
canary complementare `Spec/Bot/PlansOnPartialKnowledge`. Servono **entrambi**: il primo passerebbe anche con
un bot che ignora tutti, il secondo anche con un bot che varia il piano per ragioni diverse dall'informazione
nascosta. La premessa non vuota (`LogEventCount`) è già asserita. Nulla da aggiungere.

---

## 5. File modificati

```text
A  Source/RefactorTactics/Tests/RTOrbitProbeForTest.h
M  Source/RefactorTactics/Tests/RTMatchAutobattleTests.cpp
M  Source/RefactorTactics/Tests/RTAuthoredMapEngagementTests.cpp
A  docs/roadmap/plans/dir-c-qa-scenario-bot-autobattle-handoff-2026-08-28.md
```

Nessun file di DIR-B (`Bot/` escluso, resolver, reaction core, objective core) e nessuno di DIR-A
(`Content/`, UI, mappe, UMG) è stato toccato. Nessuno scenario `.json` creato o modificato.

**Branch**: `test/dir-c-qa-scenario-bot-autobattle`, **non pushato**. I due commit che portano codice:

| SHA | messaggio |
|---|---|
| `9280b16f` | `test(autobattle): la configurazione spedita non aveva oracolo per l'oscillazione, e il commento lo diceva` |
| `738408c9` | `test(autobattle): la mutazione dice che l'oracolo nuovo falsifica sulla mappa d'autore e non su questa arena` |

Gli altri sono aggiornamenti di questo handoff: `git log --oneline ad7f212b..HEAD` li elenca tutti, e
un'elenco scritto a mano qui si invaliderebbe a ogni correzione — che è il difetto che questo documento
segnala altrove.

⚠️ **`Source/RefactorTactics/Bot/` è byte-identico a `ad7f212b`**: verificato con
`git diff ad7f212b -- Source/RefactorTactics/Bot/`, vuoto. Le mutazioni di §6.4 non sono sopravvissute.

---|---|
| `9280b16f` | `test(autobattle): la configurazione spedita non aveva oracolo per l'oscillazione, e il commento lo diceva` |
| `f689f217` | `docs(dir-c): l'handoff della lane QA, e il difetto vivo che la direttiva cercava era gia' chiuso due volte` |
| `03d4157b` | `docs(dir-c): l'handoff diceva «nessun test eseguito», e nel frattempo ne aveva eseguiti sei` |

---

## 6. Eseguito senza Editor

§1 vieta `UnrealEditor.exe` e `UnrealEditor-Cmd`; **non vieta la compilazione**, che passa da UBT. Quindi:

### 6.1 Compilazione — ✅ eseguita

```
Build.bat RefactorTacticsEditor Win64 Development -Project="D:/Repositories/wt-dir-c-qa/RefactorTactics.uproject" -WaitMutex
→ Result: Succeeded · 0 errori · 292 s · 41 azioni
→ UnrealEditor-RefactorTactics.dll ricostruita (15 unity chunk del modulo di gioco)
```

**Il codice consegnato compila e linka.** DIR-A non eredita un errore di sintassi.

### 6.2 Gate Node — eseguiti tutti e cinque, più la suite

| gate | esito |
|---|---|
| `node tools/radar/generate.ts --check` | ✅ 4/4 eroi · 20 abilità · radar allineati |
| `node tools/radar/catalog-code.ts` | ✅ le quattro fonti concordano su tutti i campi |
| `node tools/radar/doc-tables.ts --check` | ✅ 1605 tabelle in 265 documenti — **incluse quelle di questo handoff** |
| `node --test` (da dentro `tools/radar/`) | ✅ **82/82** |
| `node tools/radar/doc-links.ts --check` | 🔴 **exit 1** — vedi sotto |

🔴 **`doc-links` è rosso, e non per colpa di questo lavoro.** 3698 link in 265 documenti, **1** non risolve:

```
docs/research/design/hud/mock-elementi-hud-correzioni.md
  → docs/research/handoff/RefactorTactics_ActionPhases_Dodge_Guard_Brace_Overwatch_Epics_v1.0_2026-08-26.md
```

Verificato che il bersaglio **non esiste già su `ad7f212b`**: il gate era rosso prima di DIR-C. Il documento
che lo cita risale a `e9c900bc`. Non è materiale DIR-C e non è stato toccato — **va consegnato a chi possiede
`docs/research/`**, perché un gate rosso per un motivo vecchio nasconde il prossimo rosso vero.

### 6.3 Automation — ✅ eseguita **in deroga esplicita a §1**

⚠️ **Deroga autorizzata dal committente il 2026-08-28.** §1 e §16 vietano a DIR-C `UnrealEditor-Cmd`; la
lane l'ha eseguito su richiesta esplicita. È registrato qui perché un vincolo aggirato senza traccia è
peggio di un vincolo che non c'era.

```
UnrealEditor-Cmd.exe "<worktree>/RefactorTactics.uproject" ^
  -ExecCmds="Automation RunTests RefactorTactics.Match.Autobattle; Quit" ^
  -unattended -nopause -nosplash -nullrhi -log -abslog=<log>
→ Found 21 automation tests · 21 Success · **** TEST COMPLETE. EXIT CODE: 0 ****
```

**Baseline, i quattro oracoli dello stato assorbente:**

| oracolo | misura | limite |
|---|---|---|
| `EngagesOnTheGeneratedTestArena` · parcheggio | **4** turni fermi | 4 — ⚠️ **margine zero** |
| `EngagesOnTheGeneratedTestArena` · orbite | 1 ritorno | 3 (su 11 turni) |
| `NobodyParksOnTheAuthoredMap` | 3 turni fermi | 4 |
| `NobodyOscillatesOnTheAuthoredMap` | 1 ritorno | 4 |

⚠️ **La sequenza ferma sulla configurazione spedita è esattamente al limite (4 su 4).** Un turno in più e
l'oracolo di #1088 va rosso. Non è un difetto oggi; è un margine che nessuno sta sorvegliando, e un
ritocco ai pesi del bot lo consuma senza preavviso.

### 6.4 Verifica di mutazione — ✅ eseguita, e **il risultato è parziale**

Tre mutazioni cumulative che ricostruiscono lo stato pre-#1296, ognuna con build e run completa:

| mutazione | arena generata | mappa d'autore |
|---|---|---|
| nessuna (baseline) | 1 su lim. 3 | 1 su lim. 4 |
| + filtro sul dominio di #1287 | 2 su lim. 3 | 3 su lim. 4 |
| + `WEngage = 0` (pre-`D-185`) | 2 su lim. 4 | 2 su lim. 4 |
| + avvicinamento in linea d'aria (pre-#1296) | **0** su lim. 4 | **7** su lim. 4 → 🔴 **Fail** |

✅ **Il rilevatore falsifica**: la terza riga fa cadere `NobodyOscillatesOnTheAuthoredMap`
(`EXIT CODE: -1`) riproducendo la misura storica di #1287 — *«otto alternanze in dodici turni»*.

⚠️ **Ma nessuna mutazione del BOT fa cadere l'asserzione sull'arena generata**, e sotto la più forte il
contatore va a **zero**: su quella geometria non si è trovato un comportamento del bot che produca
un'orbita di periodo due.

### 6.5 Mutazione del RILEVATORE — ✅ e corregge in meglio la riga qui sopra

Togliendo `*Prev != Cell` da `FRTOrbitProbe::Observe` — cioè facendo contare anche lo stare fermo — cadono
**tre** test insieme:

```
Meta.OrbitProbeIgnoresStandingStill        <- il test unitario, come previsto
Match.Autobattle.NobodyOscillatesOnTheAuthoredMap
Match.Autobattle.EngagesOnTheGeneratedTestArena   <- la configurazione SPEDITA
```

∴ **L'asserzione sull'arena generata NON è vacua.** La sua soglia è esercitata da dati reali con margine
reale: su quella board ci sono unità abbastanza ferme da superare il limite, se le si contasse male. Ciò
che resta non dimostrato è un **percorso di comportamento** — un difetto del bot che, lì, produca
davvero un'orbita. Un verde dice *«su questa board il bot non orbita»*, non *«il bot non può orbitare»*.

⚠️ **Tutte le mutazioni rimosse**, ripristino verificato: `git diff ad7f212b -- Source/RefactorTactics/Bot/`
vuoto, `RTOrbitProbeForTest.h` senza residui, e **suite intera 1298 Success / 0 Fail**.

### 6.6 Il test della sonda — ✅ 5/5

`RTOrbitProbeTests.cpp`, sotto `RefactorTactics.Meta.*`: sequenze sintetiche, nessun bot e nessuna board.

| test | cosa pinna |
|---|---|
| `OrbitProbeCountsTheAlternation` | `A B A B A B` = **4** ritorni — il numero esatto, non «> 0» |
| `OrbitProbeIgnoresStandingStill` | il parcheggio ha il suo oracolo, e non è un'orbita |
| `OrbitProbeIgnoresAPathThatComesBack` | `A B C D` = 0, e `A B C A` = 0 — il **limite dichiarato** del periodo tre diventa una proprietà misurata |
| `OrbitProbeKeepsUnitsApart` | una chiave condivisa **fabbrica** un'orbita da due unità ferme |
| `OrbitProbeThresholdFollowsTheTurns` | la soglia scala coi turni, e la premessa le lascia spazio |

🔴 **E ha corretto una nota della sonda che diceva il falso.** Sosteneva che con una chiave condivisa
l'oscillante *«non farebbe crescere nessun contatore»*. Misurato: è il contrario, ed è peggio — due unità
**ferme** su celle diverse che scrivono la stessa chiave producono `A B A B`, cioè un'oscillazione perfetta
in cui nessuno si è mosso. La guardia sugli `StableUnitId` evita un falso **positivo**, non un falso
negativo.

### 6.7 Il margine, reso udibile

Sulla configurazione spedita la sequenza ferma è **4 su soglia 4**. Aggiunto un `AddWarning` quando il
margine scende a ≤ 1, e in run spara:

```
anti-parcheggio sul filo: sequenza 4 su soglia 4, margine 0
  — il prossimo ritocco ai pesi del bot lo fa passare rosso
```

⚠️ **Warning e non asserzione, deliberatamente**: un margine sottile non è un difetto, e asserirlo
introdurrebbe di lato una soglia che `D-184` non ha deciso.

### 6.5 Cosa resta misura statica

Inventario di scenari, test e golden; registro delle capability; verifica che il fix di #1088 e quello di
`D-185` siano su `HEAD`; ancestralità di `f3d0ffa3`.

⛔ **Nessun gate di release è dichiarato verde da DIR-C, e nessuna automation è stata eseguita.**

---

## 7. Esecuzione — ✅ fatta, in deroga a §1

⚠️ Questa sezione era *«da eseguire in DIR-A»*. Il committente ha autorizzato la deroga, quindi DIR-C ha
eseguito. Ciò che resta a DIR-A è in §7.5.

### 7.1 Compilazione — ✅ `Result: Succeeded`, 0 errori (§6.1)

### 7.2 I quattro oracoli dello stato assorbente — ✅ 21/21

```
Automation RunTests RefactorTactics.Match.Autobattle
→ Found 21 automation tests · 21 Success · EXIT CODE: 0
```

Numeri e margini in §6.3. **`EngagesOnTheGeneratedTestArena` è verde con la nuova asserzione**, e
`NobodyOscillatesOnTheAuthoredMap` è verde **dopo** la migrazione al probe condiviso: l'estrazione non ha
cambiato semantica.

### 7.3 Verifica di mutazione — ✅ eseguita, esito **parziale** (§6.4)

Il rilevatore falsifica sulla mappa d'autore (1 → **7** su limite 4, `EXIT CODE: -1`). Sull'arena generata
nessuna delle tre mutazioni lo fa cadere. **Il limite è scritto nel test**, non solo qui.

### 7.4 Suite completa — ✅ 1298 Success, 0 Fail

```
Automation RunTests RefactorTactics
→ Found 1298 automation tests · 1298 Success · 0 Fail · EXIT CODE: 0
```

**«N eseguiti su M dichiarati», riconciliato a mano** — è il confronto che `feature_registry.py` faceva e
che `D-181` ha portato via:

| | |
|---|---|
| nomi `RefactorTactics.*` estratti da `Source/` e `Plugins/` | 1300 |
| eseguiti e riportati dal log | **1298** |
| differenza, nominata | `RefactorTactics.h` (una stringa di `#include`) e `RefactorTactics.Probe.LatestRunDirectory` (uno **scenario ID** dentro `FRTScenarioLatestRunIsTheMostRecentTest`, non un test) |

∴ **1298 su 1298. Nessun test dichiarato è rimasto fuori dalla run.** (1293 prima dei cinque di §6.6.)

### 7.5 Cosa resta comunque a DIR-A

- **PIE / packaged**: `PIE-HEXPLAY-*`, `G10`, `G13`, `PIE-V01-PLAYSPEED`. L'automation headless non li tocca
  — sono verifiche umane, e restano tali.
- **#1069**: il `BP_GameMode` spedito e il suo `MatchFormat` rotto. Vive in un `.uasset`: nessuna run C++ lo
  riproduce, e resta il residuo dichiarato di `EngagesOnTheGeneratedTestArena`.
- **Decidere se il branch si mergia**: la suite è verde, ma §6.4 dice esattamente quanto la nuova asserzione
  è provata e quanto no.
## 8. Consegnato a DIR-B — nessun bug core

DIR-C **non ha trovato difetti nel resolver, nella reaction core o nell'objective core** da consegnare, e non
ne ha costruiti per riempire la sezione. Una sola **osservazione**, che è una decisione e non un difetto:

🔵 **I due oracoli di parcheggio rispondono in modo opposto alla stessa domanda.**
`NobodyParksOnTheAuthoredMap` conta un turno solo se l'unità è **inerte** — ferma *e* senza aver colpito
(`URTTurnLogLibrary::IsDamageInflictedByActor`). `EngagesOnTheGeneratedTestArena` **rifiuta esplicitamente**
quella stessa esenzione, con misura scritta: *«il difetto di #1088 è esattamente "sta ferma e spara"»*, e
l'esenzione lo rendeva cieco (9 turni → 2, verde).

Entrambe le scelte portano evidenza di mutazione. **DIR-C non le ha unificate**: sarebbe una decisione sul
significato di «stallo», non un refactor, e distruggerebbe la prova che uno dei due porta. Chi la prende
guardi entrambi i commenti prima di toccarne uno.

---

## 9. Dipendenze verso DIR-A

- **Esecuzione di tutto §7** — DIR-C non può eseguire automation.
- **`Objective`** (#75) e **`InterceptRevalidation`** (#1060) sbloccano rispettivamente il T8 e il T6 dello
  showcase, e con `Objective` diventano scrivibili i tre scenari di §10.
- **#1069** — il `BP_GameMode` spedito serializza un `MatchFormat` rotto verso un `.uasset` non versionato.
  È un difetto che vive in un binario: nessun test C++ lo riproduce, e resta il residuo dichiarato di
  `EngagesOnTheGeneratedTestArena`, che istanzia `ARTGameMode` e non il Blueprint.
- **`showcase-v01-audit.md`** è stale di venti giorni (§4).
- **`doc-links.ts --check` è rosso su `main`** per un link preesistente in `docs/research/` (§6.2). Non è
  DIR-C, ma finché resta rosso il gate non sa più segnalare il prossimo link rotto davvero nuovo.

---

## 10. Gate v0.1 — stato per la lane DIR-C

⚠️ **«Eseguito» qui significa: automation headless verde su questo worktree, il 2026-08-28.** Non significa
PIE, non significa packaged, e non chiude nessun gate che chieda una verifica umana.

| criterio §17 | oracolo | eseguito |
|---|---|---|
| bot produce intenti legali | candidate da `ReachableCells`; `Plan.MovementMainAndReactionAreLegal` | ✅ dentro i 1298 |
| bot non usa hidden state | `HexBotPlay.HiddenEnemyFairness` + `Spec/Bot/*` | ✅ dentro i 1298 |
| autobattle non stalla per un bug noto | 4 oracoli, di cui **1 aggiunto oggi**, più 5 test della sonda | ✅ 26/26 — potere discriminante in §6.4/§6.5 |
| complete match termina | `FreeRun.ArenaV01ReachesAWinner` | ✅ dentro i 1298 |
| reaction FIRE/HOLD riproducibili | `Overwatch.DecisionIsReplayable`, golden `HoldThenFire` | ✅ dentro i 1298 |
| objective conclude la partita | 🔴 **BLOCKED: Objective** (#75) | — non esiste da eseguire |
| showcase usa pipeline reale | 5/8 turni, BLOCKED dichiarato al T6 | ✅ per quanto esiste |
| same seed → same hashes | `SameSeedGivesSameResult` — vero **per costruzione**, nessun RNG | ✅ ma la proprietà è strutturale |
| replay non richiede nuove decisioni | `Overwatch.DecisionIsReplayable` | ✅ dentro i 1298 |

⛔ **Restano fuori, e restano di DIR-A**: `PIE-HEXPLAY-*`, `G10`, `G13`, `PIE-V01-PLAYSPEED`. Una suite
headless verde non è una partita guardata da qualcuno.
