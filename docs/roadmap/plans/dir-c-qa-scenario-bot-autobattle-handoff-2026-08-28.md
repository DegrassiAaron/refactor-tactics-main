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
A  Source/RefactorTactics/Tests/RTOrbitProbeTests.cpp
M  Source/RefactorTactics/Tests/RTMatchAutobattleTests.cpp
M  Source/RefactorTactics/Tests/RTAuthoredMapEngagementTests.cpp
A  docs/roadmap/plans/dir-c-qa-scenario-bot-autobattle-handoff-2026-08-28.md
```

Nessun file di DIR-B (`Bot/` escluso, resolver, reaction core, objective core) e nessuno di DIR-A
(`Content/`, UI, mappe, UMG) è stato toccato. Nessuno scenario `.json` creato o modificato.

**Branch**: `test/dir-c-qa-scenario-bot-autobattle`, pushato — **PR [#1547](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1547)** verso `main`. I commit che portano codice:

| SHA | messaggio |
|---|---|
| `9280b16f` | `test(autobattle): la configurazione spedita non aveva oracolo per l'oscillazione, e il commento lo diceva` |
| `738408c9` | `test(autobattle): la mutazione dice che l'oracolo nuovo falsifica sulla mappa d'autore e non su questa arena` |

Gli altri sono aggiornamenti di questo handoff: `git log --oneline ad7f212b..HEAD` li elenca tutti, e
un'elenco scritto a mano qui si invaliderebbe a ogni correzione — che è il difetto che questo documento
segnala altrove.

⚠️ **`Source/RefactorTactics/Bot/` è byte-identico a `ad7f212b`**: verificato con
`git diff ad7f212b -- Source/RefactorTactics/Bot/`, vuoto. Le mutazioni di §6.4 non sono sopravvissute.

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

🔵 **Spiegato il 2026-08-28, e la ricerca NON è chiusa**
([#1550](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1550)). Il ciclo di #1287 può
chiudersi solo se il filtro sul dominio, dalla cella **cieca**, fa **arretrare**: manda su una cella che
vede ma più *lontana* dal bersaglio. Se manda avanti, tornare indietro significa allontanarsi, e qualunque
termine di avvicinamento lo penalizza. Su `MakeTestArena` il muro di `q=0` blocca la vista e **non il
passo**, e `HasLineOfSight` esclude gli estremi dalla regola per-cella: la cella del muro è insieme la più
vicina al bersaglio **e** una cella che vede. Sulla mappa d'autore l'ostacolo centrale blocca vista **e**
passo, e lì la cella che vede sta dietro.

Passi indietro su coppie `(cella cieca, bersaglio)` esaminate, **per budget di movimento** — non sommati:

| board | 2 MP | 3 MP | 4 MP | **5 MP (`Move`)** | 6 MP | 7 MP | **8 MP (`Sprint`)** | primo budget pulito |
|---|---|---|---|---|---|---|---|---|
| `MakeTestArena` — 0 su 5 muri bloccano anche il passo | 48 | 19 | 0 | **0** | 0 | 0 | **0** | **4 MP** |
| `DA_HexMap_Arena` — 9 su 11 | 154 | 162 | 170 | **104** | 38 | 9 | 0 | **8 MP** |

∴ le due board si distinguono per **dove cade la soglia**, e il profilo neutro (`MovementMode.Move`, 5 MP)
cade sui due lati opposti. ⚠️ **A 2 e 3 MP il passo indietro esiste anche sull'arena generata**, ed è
dichiarato nel test: il 2 è il ripiegamento che *«non si sceglie: lo impone l'Overwatch»* (`D-070`), e
un'orbita sostenuta chiede lo stesso budget a ogni turno — ma è un argomento, non una misura.

🔴 **E il controfattuale ritrova l'orbita STORICA.** Fra le coppie distinte della mappa d'autore c'è
`(1,-1,L0) ↔ (3,-3,L1)` — esattamente quella che #1287 ha misurato in partita, trovata da un predicato che
di quella misura non sa nulla. È ciò che distingue una misura da un modello plausibile, ed è asserito.

Le due misure sono `Bot.StalemateProbeGeneratedArenaFilterNeverStepsBack` e
`Bot.StalemateProbeAuthoredMapFilterStepsBack`, in `RTBotStalemateProbeTests.cpp`.

🔴 **Ciò che questa spiegazione NON è: una dimostrazione di impossibilità — e una stesura precedente di
questa sezione lo sosteneva.** Diceva che `ScorePlan` non legge `Context.Origin`, quindi `A → B` e `B → A`
chiedono disuguaglianze opposte e il 2-ciclo non sarebbe formabile su nessuna board. **È falso**, ed è caduto
in code review: il punteggio dipende dalla provenienza attraverso il **facing d'arrivo** —
`RTHexBotLibrary.cpp` sottrae `WDamage × max(0, CoperturaQui − CoperturaTenuta)`, e con
`WDamage == WApproach == 10` un punto di copertura vale esattamente una cella di avvicinamento. La tesi è
ritirata, e con lei la parola «chiuso».

⚠️ **Quindi la riga della tabella resta aperta, e vale ancora**: chi troverà una mutazione del **bot** che fa
cadere quell'asserzione la scriva lì — e con lei cade questa spiegazione. Ciò che è cambiato non è che la
ricerca sia finita: è che ora si sa **dove** guardare — a un termine che paghi l'arretramento da solo, o a un
budget di movimento sotto i 4 MP.

⚠️ **Due stesure sbagliate del predicato, prima di questa, e vanno dette.** La prima contava una condizione
*necessaria* diversa e dava un numero positivo su entrambe le board — un controfattuale che non discriminava.
La seconda modellava il punteggio con la sola distanza, dichiarava «lo stesso tie-break di `ChooseBestPlan`»
mentre risolveva le parità di quota **al contrario**, e pubblicava come «coppie» delle **somme sui sette
budget**: 9510 su una board che ne ha al massimo 62×61 = 3782. Entrambe trovate in code review.

### 6.5 Mutazione del RILEVATORE — ✅ e corregge in meglio la riga qui sopra

Togliendo `*Prev != Cell` da `FRTOrbitProbe::Observe` — cioè facendo contare anche lo stare fermo — cadono
**tre** test insieme:

```
Meta.OrbitProbeIgnoresStandingStill        <- il test unitario, come previsto
Match.Autobattle.NobodyOscillatesOnTheAuthoredMap
Match.Autobattle.EngagesOnTheGeneratedTestArena   <- la configurazione SPEDITA
```

∴ **L'asserzione sull'arena generata NON è vacua.** La sua soglia è esercitata da dati reali con margine
reale: su quella board ci sono unità abbastanza ferme da superare il limite, se le si contasse male.

✅ **E il percorso di comportamento che le mancava non manca per difetto di ricerca: non esiste su quella
board** — misurato il 2026-08-28, §6.4, [#1550](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1550).
Il limite resta, ed è più stretto di come questa riga lo dichiarava: un verde dice *«il ciclo **nella forma
di #1287** non ha su questa board dove chiudersi»*, non *«il bot non può orbitare»* — un termine di
punteggio che dipenda dall'origine in un altro modo riaprirebbe la domanda.

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

### 6.9 La code review, e cosa ha trovato

Un giro a effort alto sulla PR. **Dodici rilievi: undici accolti, uno declinato.** Due erano gravi, ed
erano miei.

| # | rilievo | esito |
|---|---|---|
| 1 | il nuovo punto di campionamento non verifica `IsResolving()` dopo il tetto di 400 tick — viola la precondizione che la PR stessa scrive sulla sonda, e che il test gemello applica | ✅ guardia aggiunta |
| 2 | **due commenti aggiunti dalla PR ripetono la falsità che la PR dichiara di aver corretto** sulle chiavi condivise, e il suo stesso nuovo test li smentisce | ✅ riscritti |
| 3 | `MinTurnsToFalsify = 6` ha una giustificazione **aritmeticamente sbagliata**: con `N` campioni il massimo è `N-2` contro un limite `max(1, N/3)`, quindi è falsificabile **da 4** | ✅ portato a 4, con la minimalità pinnata da un test |
| 4 | i due chiamanti divergono sulla politica della premessa (asserisce/avverte) | ✅ documentata come scelta, non come deriva |
| 5 | correggere l'`else` ha reso **saltabile** l'anti-parcheggio: con l'altra rinuncia il test può passare verde senza misurare lo stato assorbente in nessuna forma | ✅ asserzione «almeno una forma è stata misurata» |
| 6 | l'intestazione del nuovo file contraddice la conclusione corretta in §6.5 | ✅ riscritta |
| 7 | l'handoff dichiarava «nessuna automation è stata eseguita» mentre §6.3 la riporta | ✅ |
| 8 | frammento di tabella orfano con SHA in conflitto — e `doc-tables` **non può vederlo**, perché guarda solo le righe che iniziano con una pipe | ✅ rimosso |
| 9 | l'elenco dei file omette `RTOrbitProbeTests.cpp`, e «non pushato» era falso | ✅ |
| 10 | due sezioni numerate `### 6.5` | ✅ rinumerate |
| 11 | tre `TMap` paralleli invece di una `TMap<int32, FEntry>` | ⛔ **declinato** |
| 12 | il warning sul margine sparava anche a oracolo già rosso, stampando un margine negativo | ✅ guardia `>= 0` |

⛔ **Perché il rilievo 11 è declinato.** È corretto che una `TMap` sola renderebbe strutturale l'invariante
«`Penultima` vale solo se `Ultima` esiste», oggi tenuto da un `if (Prev)`. Ma la sonda ha appena acquisito
cinque test che pinnano il comportamento e due verifiche di mutazione: riscriverne la forma interna adesso
scambia una prova appena costruita con un'eleganza, e il guadagno — un `Find` invece di cinque, su quattro
unità per turno — non è misurabile in una suite che gira in novanta secondi. Resta un buon lavoro per chi
toccherà la sonda per il periodo tre, quando la forma dovrà cambiare comunque.

💡 **E un errore di processo, registrato perché non si ripeta.** Per rimuovere la mutazione di verifica ho
usato `git checkout -- <file>` su un file che portava **anche** correzioni non committate, e le ho perse
insieme alla mutazione. La regola che ne esce: **committare le correzioni prima di mutare**, oppure mutare
un file diverso da quello che si sta modificando.

### 6.8 Cosa resta misura statica

Inventario di scenari, test e golden; registro delle capability; verifica che il fix di #1088 e quello di
`D-185` siano su `HEAD`; ancestralità di `f3d0ffa3`.

⛔ **Nessun gate di RELEASE è dichiarato verde da DIR-C.** L'automation headless è stata eseguita (§6.3–§6.5),
e questa riga diceva il contrario: era la stesura pre-deroga, rimasta dopo che §6.3 la superava. Ciò che
resta vero è il resto — una suite verde non è una partita guardata da qualcuno, e `PIE-HEXPLAY-*`, `G10`,
`G13`, `PIE-V01-PLAYSPEED` restano verifiche umane di DIR-A.

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

✅ **Istruita e proposta il 2026-08-28**, e non lasciata come osservazione:
[`BOT-STALL-1`](../../OPEN_DECISIONS.md) in `docs/OPEN_DECISIONS.md` porta la domanda, le due evidenze
misurate e **quattro** uscite col loro costo — (a) esenzione ovunque, che acceca l'oracolo di #1088
sull'unica board su cui gira · (b) esenzione in nessuno dei due, che rende rosso un kiter legittimo ·
(c) esenzione condizionata all'**avanzamento** (danno inflitto *e* stato che avanza), che è la distinzione
che `D-184` fa già a parole ma **non è stata misurata** e porta con sé una soglia nuova · (d) restare
divergenti, dichiarato. Raccomandata (c) se qualcuno la misura, altrimenti (d). Issue
[#1551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1551).

⚠️ **E la divergenza è ora scritta in ENTRAMBI i file**, ciascuno che nomina l'altro oracolo e il costo
dell'allineamento: allinearli «per coerenza» senza leggere l'altro non è più possibile per distrazione.
🔴 **Nessuna delle due asserzioni è stata cambiata**: cambiarne una sarebbe stato prendere la decisione
qui, invece che da chi la possiede.

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

---

## 11. Seconda passata — 2026-08-28, `v0.2`

> **Cosa è**: due delle domande che la prima passata ha lasciato aperte, chiuse con una misura. **Cosa non
> è**: un secondo audit della lane — quello sta sopra e non è stato rifatto.

| | |
|---|---|
| **HEAD di partenza** | `e9e45381` (`origin/main`, un merge oltre il `38e7dcfc` di #1547) |
| **Worktree** | `D:/Repositories/wt-dir-c-v02` — path scelto per non collidere con lo schema di pulizia altrui |
| **Branch** | `test/1550-orbita-geometria-e-parcheggio` |
| **Issue** | [#1550](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1550) (C-1) · [#1551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1551) (C-2) |

🔴 **Il clone condiviso ha morso di nuovo, e in due modi nuovi.** Fra il primo e il secondo comando del
pre-flight un'altra sessione ha cambiato **branch** al checkout condiviso (`main` →
`fix/1548-simboli-duplicati-unity`): due `git branch --show-current` a un minuto di distanza danno risposte
diverse. E il motore è un **mutex globale**, quindi il worktree isola il checkout ma **non** la misura:
`rt-suite.ps1` ha risposto **cinque volte `NON AVVIATA`** (tre per un Editor interattivo altrui, due per una
run di automation altrui) e **una volta `NON VALIDA`** — quella era partita e un'altra run le è comparsa
accanto, quindi i suoi numeri non sono registrabili e non sono stati usati. In più una build è stata respinta
con `Unable to build while Live Coding is active`, dal `LiveCodingConsole` figlio di quell'Editor. **Nessun
processo altrui è stato terminato.** Il costo è tempo d'attesa, e va messo in conto da chi pianifica una lane
che misura: il pattern che funziona è un comando in background che attende il motore e **poi** lancia la
misura nello stesso comando, così una mutazione di verifica non resta sul disco nella finestra fra «motore
libero» e «run partita».

### 11.1 C-1 — perché quella mutazione non si trova, e perché la ricerca resta aperta

La domanda di §6.4/§6.5 ha una **spiegazione misurata**, e le due sezioni sono state corrette dove stanno
invece che smentite qui. Il risultato in breve:

- Il ciclo di #1287 si chiude solo se il filtro, dalla cella **cieca**, fa **arretrare**. Se manda avanti,
  tornare indietro costa avvicinamento e nessun termine lo compensa da solo.
- Su `MakeTestArena` il filtro non arretra **dai 4 MP in su**, e il profilo neutro ne porta 5; sulla mappa
  d'autore arretra fino ai 7. La tabella per budget è in §6.4.
- La causa strutturale: lì i muri bloccano la vista e **non il passo** (0 su 5), qui bloccano entrambi
  (9 su 11).

Le due misure sono in `RTBotStalemateProbeTests.cpp`, e non simulano partite.

🔴 **E la prima conclusione di questa lane era troppo forte.** Sosteneva che un 2-ciclo non fosse formabile
su *nessuna* board perché `ScorePlan` non legge `Context.Origin`. È falso — il facing d'arrivo lo rende
dipendente dalla provenienza, con lo stesso peso di una cella di avvicinamento — ed è stato trovato in code
review. La tesi è ritirata, la riga *«chi troverà una mutazione del bot la scriva in questa tabella»* è
tornata dov'era, e ciò che resta è una spiegazione con il proprio limite scritto.

⚠️ **Tre stesure del predicato, e le prime due sbagliate in modo diverso.** La prima contava una condizione
necessaria che non discriminava fra le due board. La seconda modellava il punteggio con la sola distanza,
dichiarava un tie-break che non aveva, e pubblicava come «coppie» delle somme sui sette budget — numeri più
grandi delle coppie che la board possiede. La terza non modella il punteggio affatto: chiede solo se il
filtro avvicini o allontani, che è l'unica cosa di cui il ciclo ha bisogno.

### 11.2 C-2 — la divergenza è istruita, non risolta

`BOT-STALL-1` in [`docs/OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), con quattro uscite e il costo di
ciascuna; issue [#1551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1551). **Nessuna delle
due asserzioni è stata toccata**: cambiarne una sarebbe stato prendere la decisione qui invece che da chi la
possiede. Ciò che è cambiato sono i **commenti**, ora incrociati: da ciascuno dei due oracoli si arriva
all'altro e all'istruttoria, e allinearli «per coerenza» non è più possibile per distrazione.

⚠️ **Scritto in `docs/OPEN_DECISIONS.md`, che è fuori dal write-set dichiarato della lane.** È l'inbox che
quel file dichiara di essere — *«l'elenco di ciò che aspetta una persona»* — e una voce **aperta** non
chiude nulla; il Decision Log non è stato toccato. Chi possiede PDR-00 la sposti o la respinga.

### 11.3 Verifica di mutazione — due livelli, e questa volta entrambi cadono

**Soggetto (la board).** Reso il muro di `q=0` di `MakeTestArena` anche ostacolo al movimento
(`bBlocksMovement = true`), con build e run complete. Passi indietro sull'arena generata, per budget:

| | 2 | 3 | 4 | **5 (`Move`)** | 6 | 7 | **8** | esito |
|---|---|---|---|---|---|---|---|---|
| baseline — 0 su 5 muri bloccano il passo | 48 | 19 | 0 | **0** | 0 | 0 | **0** | ✅ |
| muro anche ostacolo — 5 su 5 | 103 | 157 | 102 | **21** | 4 | 0 | **0** | 🔴 `Fail`, `EXIT CODE: -1` |

Cadono **entrambe** le asserzioni che dovevano cadere: quella strutturale (*«nessuna di quelle celle blocca
il passo»*, 5 invece di 0) e quella portante (*«col profilo neutro il filtro non fa mai arretrare»*, 21
invece di 0). Il controfattuale sulla mappa d'autore **non si muove**, che è giusto: è un'altra board.
⚠️ Cadono anche `StalemateProbeContendersAreNamed` e `StalemateProbeHeadlessMatchLosesContact`: collaterale
atteso — cinque celle in meno cambiano la partita che quei due misurano.

**Rilevatore (il predicato).** Disattivato il filtro, cioè facendo scegliere la cella più vicina *senza*
chiedere che veda:

| | arena generata (5 MP) | mappa d'autore (5 MP) | esito |
|---|---|---|---|
| predicato intero | **0** su 1470 | 104 su 1375 | ✅ ✅ |
| filtro disattivato | 0 su 1470 | 28 su 1501 | 🔴 il **controfattuale** cade |

🔴 **E cade sull'asserzione giusta: la coppia storica.** Senza il filtro la cella scelta è la più vicina in
assoluto, e `(1,-1,L0) ↔ (3,-3,L1)` — l'orbita che #1287 ha misurato in partita — sparisce dalle coppie
trovate. Il messaggio è letteralmente *«fra le coppie c'e' quella MISURATA da #1287 in partita»*. È
l'asserzione che distingue «il predicato trova qualcosa» da «il predicato trova **quel** difetto».

∴ **i due test si falsificano a vicenda i modi degeneri opposti**: un predicato che dicesse sempre «sì» fa
cadere il gemello sull'arena generata (mutazione del soggetto), uno che perde il difetto vero fa cadere il
controfattuale (mutazione del rilevatore). È la ragione per cui sono due e non uno.

⚠️ **Mutazioni rimosse e binario RICOSTRUITO**, non solo il sorgente: `rt-suite.ps1` verifica che il binario
non **cambi** durante la run, non che **corrisponda** al sorgente, e un `.dll` stantio dichiara `VALIDA`
misurando codice che non esiste in nessun commit. `git status` pulito e binario riscritto dopo l'ultimo
ripristino, entrambi verificati.

### 11.4 Gate eseguiti, e come vanno letti

Tutto misurato sul merge con `origin/main` a `dace4a50` (#1553), che nel frattempo era avanzato.

| gate | esito |
|---|---|
| build `RefactorTacticsEditor` | ✅ `Result: Succeeded`, 0 errori |
| suite intera | ✅ **`1354/1354 completati, 0 fallimenti`**, dichiarata `VALIDA` da `scripts/rt-suite.ps1` (`Found 1354 automation tests` · `TEST COMPLETE. EXIT CODE: 0`) |
| `radar/generate.ts --check` | ✅ 4/4 eroi · 20 abilità |
| `radar/catalog-code.ts` | ✅ le quattro fonti concordano |
| `radar/doc-tables.ts --check` | ✅ tutte le righe hanno la larghezza delle sorelle |
| `node --test` (in `tools/radar/`) | ✅ 82/82 |
| `radar/doc-links.ts --check` | 🔴 **exit 1 — preesistente**: 1 link su 3769, `docs/research/design/hud/mock-elementi-hud-correzioni.md`. È lo stesso di §6.2, non è stato toccato |

⚠️ **In una run precedente il conteggio era `1353 su 1354`, e va spiegato invece che dimenticato.** Il
test mancante era `Vision.VisibleCellsRespectsSight`, l'**ultimo avviato**: la sua riga di conclusione era
tagliata dalla coda di shutdown del `Quit`. `rt-suite.ps1` distingue quel caso da una troncatura e dichiara
comunque `VALIDA`. Nella run finale, sulla stessa build, tutte e 1354 le righe di conclusione ci sono —
quindi il `1353` era la coda del log, non un test che non gira.

**«N eseguiti su M dichiarati», riconciliato a mano** (`D-181`): **1356** nomi `RefactorTactics.*` estratti da
`Source/` e `Plugins/`, **1354** avviati e **1354** conclusi `Success`. La differenza è nominata ed è la stessa di #1547 —
`RefactorTactics.h` (una stringa di `#include`) e `RefactorTactics.Probe.LatestRunDirectory` (uno
**scenario ID** dentro `FRTScenarioLatestRunIsTheMostRecentTest`, non un test). ∴ **1354 su 1354.**

🔴 **Una premessa della direttiva v0.2 non regge più, e va corretta dove sta.** `ScreenHud.ActionSlotHasIconSurface`
è dato per «rosso preesistente, non è tuo». Su `e9e45381` e sul merge con `dace4a50` è **`Result={Success}`**:
non c'è nessun rosso preesistente da scontare, e la suite è verde per intero.

### 11.5 Cosa NON è stato fatto, e perché

- **Non è stata trovata una quarta mutazione del bot, e la ricerca resta APERTA.** Questa riga diceva che
  continuare a cercarla «sarebbe stato cercare qualcosa di cui si è appena misurata l'assenza»: è la stessa
  tesi di impossibilità che §6.4 ha ritirato, sopravvissuta qui dopo la correzione. Ciò che è misurato è
  *dove* la board rende difficile il ciclo — il filtro non arretra dai 4 MP in su — non che la mutazione non
  esista. [#1550](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1550) **resta aperta** per
  questo, e il suo criterio d'accettazione poggiava su quella premessa falsa.
- **Non è stata presa la decisione di `BOT-STALL-1`.** L'uscita raccomandata `(c)` — esenzione condizionata
  all'*avanzamento* — non è stata nemmeno prototipata: porta con sé una soglia nuova, e le soglie sono
  materia di `D-184`.
- **Il margine 4 su 4 resta un `AddWarning`**, per la ragione già scritta in §6.7.
