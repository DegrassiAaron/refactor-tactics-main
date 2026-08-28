# DIR-C HANDOFF — QA / Scenario / Bot / Autobattle

> `CURRENT` · **Data**: 2026-08-28 · **Lane**: DIR-C (QA, Scenario Harness, Bot v0.1, Autobattle,
> corpus deterministico) · **Vincolo rispettato**: nessun `UnrealEditor.exe` / `UnrealEditor-Cmd` / PIE
> avviato, nessun `.uasset` / `.umap` / `Content/**` toccato.
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
| **HEAD finale DIR-C** | `9280b16f` su `test/dir-c-qa-scenario-bot-autobattle` |
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
| File di test | **141** in `Source/RefactorTactics/Tests/` | |
| Test dichiarati | **1286** nomi `RefactorTactics.*` unici | ⚠️ *dichiarati*, non *eseguiti* — vedi §5 |
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
contro i **1286 in 141** misurati oggi, e dà `Structures` PARTIAL / `Overwatch` MISSING quando entrambe sono
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

**Commit**: `9280b16f` — `test(autobattle): la configurazione spedita non aveva oracolo per l'oscillazione, e il commento lo diceva`

---

## 6. Test eseguiti senza Editor

**Nessuno, e va detto così.** §1 vieta a DIR-C `UnrealEditor.exe` e `UnrealEditor-Cmd`, e l'Unreal Automation
Framework non ha un runner fuori dall'Editor. Ciò che è stato fatto senza Editor è **misura statica**:
inventario di scenari, test e golden; lettura del registro delle capability; verifica che il fix di #1088 e
quello di `D-185` siano su `HEAD`; verifica dell'ancestralità di `f3d0ffa3`.

⛔ **Nessun gate è dichiarato verde da DIR-C.**

---

## 7. Da eseguire in DIR-A — con esito atteso e interpretazione

### 7.1 Compilazione

```
"<UE>/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development ^
  -Project="<worktree>/RefactorTactics.uproject" -WaitMutex
```

**Atteso**: compila. Il nuovo header sta sotto `Source/RefactorTactics/Tests/`, e
`PublicIncludePaths.Add(ModuleDirectory)` risolve `Map/RTCellId.h`; UBT raccoglie i file nuovi da sé.
**Se fallisce**: è il primo consumatore di `FRTOrbitProbe`, quindi l'errore è locale al probe o all'include —
non tocca nessuna API di gioco.

### 7.2 I tre oracoli dello stato assorbente

```
Automation RunTests RefactorTactics.Match.Autobattle
```

| test | atteso | se rosso |
|---|---|---|
| `EngagesOnTheGeneratedTestArena` | verde | 🔴 **leggere quale delle due asserzioni cade.** Se cade *«nessuna unità oscilla fra due celle»*, **non è un difetto del test**: è lo stato assorbente di periodo due che si manifesta sulla configurazione spedita, cioè il motivo per cui questo oracolo è stato aggiunto. Il numero da riportare è nell'`AddInfo`: «più ritorni di periodo due su una unità: N (limite L, su T turni)» |
| `NobodyParksOnTheAuthoredMap` | verde, **invariato** | carica `DA_HexMap_Arena` da `.uasset`: senza asset non è eseguibile |
| `NobodyOscillatesOnTheAuthoredMap` | verde, **invariato** — solo migrato al probe condiviso | una divergenza qui significa che l'estrazione ha cambiato semantica, non che il bot è cambiato |

### 7.3 Verifica di mutazione — **obbligatoria prima del merge**

Il file `RTMatchAutobattleTests.cpp` porta scritta la lezione che rende questo passo non negoziabile:
*«dopo aver toccato un oracolo la mutazione va rifatta prima del commit — una suite verde non distingue un
test che passa da uno che non può più cadere»*. DIR-C ha toccato due oracoli e **non ha potuto eseguirla**.

| mutazione | dove | atteso |
|---|---|---|
| rimettere il filtro sul dominio di #1287 | `RTHexBotLibrary.cpp`, candidate ristrette alle celle da cui si vede quando non si può colpire e non si vede nessuno | 🔴 **`EngagesOnTheGeneratedTestArena` deve cadere sull'asserzione di oscillazione.** Se resta verde, il nuovo oracolo non misura nulla sulla configurazione spedita e va riletto, non tenuto |
| `WEngage` → `0` | `RTHexBotLibrary.h:171` | i due oracoli di parcheggio devono cadere |
| `WElevation` → `20` | `RTHexBotLibrary.h:156` | tre test devono cadere («sequenza ferma 9 turni») |

### 7.4 Suite completa

```
Automation RunTests RefactorTactics
```

⚠️ **Una console variable in testa a `-ExecCmds` fa saltare l'intera coda** (#1300): usare `-dpcvars=`.
Verificare nel log `Found <n> automation tests based on '<filtro>'` **e** `**** TEST COMPLETE. EXIT CODE ****`:
senza la prima riga la run non ha misurato niente. Il confronto «N eseguiti su M dichiarati» va fatto a mano —
lo script che lo faceva è uscito con `D-181`. **M dichiarati su questo HEAD: 1286.**

---

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

---

## 10. Gate v0.1 — stato per la lane DIR-C

⛔ **Nessuna riga è dichiarata verde da DIR-C**: le colonne dicono cosa esiste come oracolo, non cosa è stato
eseguito oggi.

| criterio §17 | oracolo | eseguito da DIR-C |
|---|---|---|
| bot produce intenti legali | candidate da `ReachableCells`; `Plan.MovementMainAndReactionAreLegal` | no |
| bot non usa hidden state | `HexBotPlay.HiddenEnemyFairness` + `Spec/Bot/*` | no |
| autobattle non stalla per un bug noto | 3 oracoli, di cui **1 aggiunto oggi** | no — **§7.2/§7.3** |
| complete match termina | `FreeRun.ArenaV01ReachesAWinner` (vittoria al turno 19) | no |
| reaction FIRE/HOLD riproducibili | `Overwatch.DecisionIsReplayable`, golden `HoldThenFire` | no |
| objective conclude la partita | 🔴 **BLOCKED: Objective** (#75) | — |
| showcase usa pipeline reale | ✅ per costruzione — 5/8 turni, BLOCKED dichiarato | no |
| same seed → same hashes | `SameSeedGivesSameResult` — vero **per costruzione**, nessun RNG | no |
| replay non richiede nuove decisioni | `Overwatch.DecisionIsReplayable` | no |
