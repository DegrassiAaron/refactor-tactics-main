# Roadmap Codice / Automation v0.1 — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Consuma il work order *«REFACTORTACTICS v0.1: ROADMAP
> CODICE / AUTOMATION»* (blocchi `B01`–`B14`), arrivato come **argomento di `/sc:spec-panel`** e non come
> file: non c'è un sorgente da archiviare, e questo documento è la sua unica traccia.
>
> **Data**: 2026-08-29 · **Base**: `origin/main` @ `855672bb` · **Modo**: critique · **Focus**: requirements + testing
>
> **Cosa è**: il verdetto su una **specifica di lavoro** che ordina di creare/aggiornare issue su GitHub.
> `/sc:spec-panel` è task documentale ([`CLAUDE.md`](../../../CLAUDE.md) §6): **nessuna issue è stata creata,
> chiusa o modificata**, e il §6.1-5 del [referto gemello](roadmap-issues-v01-v10-spec-panel-2026-08-29.md)
> pretende esattamente questo — il dry-run precede la scrittura.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge da [`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md)
> o dal [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md), **ha ragione l'owner**.

---

## 1. Il verdetto in una riga

> **La specifica descrive un sistema che in gran parte esiste già, e il suo blocco d'apertura chiede di
> smontare un presidio deliberato: `B01` vuole che «MaxTurns raggiunto» diventi `PASS`, ma quel `Fail` è
> scritto apposta per non rendere verde lo stallo del bot di #1088 — la stessa regressione che `B08`, tre
> pagine dopo, chiede di coprire.**

Su quattordici blocchi, **due** descrivono lavoro non ancora fatto e non ancora posseduto da nessuno; **otto**
hanno un owner vivo su GitHub o nel DoD; **quattro** sono già implementati e verdi in suite. Il contributo che
vale il consumo è l'idea di `B14` — *poche fixture, molti checkpoint, più execution mode* — che è già la
strategia del corpus esistente ma non è scritta da nessuna parte come regola.

---

## 2. Base di misura

Misurato lato server e su albero, non ricordato.

```text
Repo      : DegrassiAaron/refactor-tactics-main
Base      : origin/main @ 855672bb  (fetch --prune eseguito)
Sessione  : branch diag/1665-istanze-board, albero NON pulito (lavoro di un'altra sessione)
Issue open: 278 · milestone v0.1: sei, 65 aperte / 112 chiuse
Suite     : 1368 nomi RefactorTactics.* dichiarati in Source/ · 143 file di test runtime
```

| Affermazione della specifica | Misura | Esito |
|---|---|---|
| Esiste `bPassed = bHasWinner && …` o `Winner==None ⇒ Fail` | `grep` su `ScenarioHarness/` | 🔴 **non esiste**. Verdict ed esito sono già due tipi: `ERTTestOutcome{Pass,Fail,Error,Blocked}` (`RTTestScenario.h:37`) contro `ERTMatchOutcome{InProgress,Team0Wins,Team1Wins,Draw}` (`Turn/RTTurnRules.h:23`) |
| «`Match EndReason` separato dal Winner» va costruito (`B02`) | `RTTurnRules.h:37-45` | 🔴 **esiste dal 2026-08**: `ERTMatchEndReason{None,Elimination,Objective,RoundLimit}`, con docstring che dichiara proprio quella separazione |
| «scenario corto con MaxTurns e nessun winner può PASS» | `Scenarios/Movement/*.json` | ✅ **già vero, e per un'altra via**: quegli scenari **non hanno** `maxTurns` — hanno turni scriptati ed `expect`, e passano senza winner. `Movement.Basic`, `Blocked`, `Collision` sono esattamente il caso |
| `MaxTurns` è un cap neutro (`B03`) | `RTTestScenario.h:737-753` | 🔴 **falso, ed è il punto**: è tetto del **solo free-run**, e *«raggiungerlo è un `Fail`, non un `Pass`»*, perché *«un tetto che producesse `Pass` renderebbe verde esattamente lo stallo che questo scenario esiste per cogliere»* (#1088) |
| Playback speed invariance da costruire (`B04`) | nomi test in `RTMatchAutobattleTests.cpp` | 🔴 **già coperto**: `Match.Autobattle.DeterminismIsIndependentOfPlayback`, più `SameSeedGivesSameResult` e `DeterminismSurvivesUnitPermutation` |
| Repeat determinism da costruire (`B10`) | `RTTestScenario.h:767`, `RTScenarioRunner.h:85` | 🔴 **esiste**: campo `repeatCount` con cap `MaxRepeatCount = 100`, confronto **byte per byte** delle tracce. `G4` chiede già 100 ripetizioni |
| Replay parity da costruire (`B13`) | `RTSimulationDeterminismTests.cpp` | 🟡 **sei test già verdi**: `Replay.Verifier.ResimulationIsDeterministic`, `ReactionDecisionsComeFromTheTrace`, `RecordedResponseBeatsLiveDecider`, `OrphanRecordedResponseIsReported`, `AmbiguousTraceIsReported`, `CollapsedWindowIgnoresTheTrace` |
| Bot combat regression senza owner (`B08`) | `gh issue view 1088` | 🔴 **#1088 è CLOSED**. La catena viva è `#1655` `#1551` `#1550`, più il probe `RTBotStalemateProbeTests` (11 asserzioni) |
| `RT_Showcase_Relay_v01` esiste col nome citato | `Scenarios/RT_Showcase_Relay_v01.json` | ✅ **nome esatto**, 8 turni, `expect` **solo a livello root** (`TurnsCompleted: 5`) |
| I checkpoint `T1`–`T8` sono tutti giocabili | `requires` per turno + `RTScenarioSession.cpp:258-259` | 🔴 **T6 e T8 sono `Blocked` per costruzione**: `InterceptRevalidation` (owner **#1060**) e `Objective` (owner **#75**) sono in `KnownUnavailableCapabilities` |
| Objective system da costruire (`B12`) | `gh issue view 75` | 🟡 **owner esistente e aperto**: `#75` *CP 10.2 — Obiettivo contestabile*, milestone `v0.1 · Mondo giocabile`. Epic **E10** = `#24` |
| Serve una suite `RefactorTactics.v01.CompleteMatch` (`B14`) | regola 6.1 dei prefissi, `G3` | 🔴 **il nome violerebbe la gerarchia**: `v01` non è una famiglia di dominio, e `G3` verifica staticamente **0 violazioni su 1364 stringhe** |

⛔ **Non misurato**: nessuna suite eseguita, nessun build lanciato in questa sessione. Il referto non dichiara
verde né rosso alcun gate — legge quelli che il DoD dichiara, rimisurati dall'owner il **2026-08-29**.

---

## 3. Punteggi

| Dimensione | Voto | Ragione in una riga |
|---|---|---|
| **Chiarezza** (Doumont) | **7.5** / 10 | blocchi numerati, critical path esplicito, «cosa non deve bloccare» dichiarato — raro e prezioso |
| **Completezza** | **7.0** / 10 | copre pipeline, invarianti e DoD; **manca** il passo di misura *prima* di prescrivere il rimedio |
| **Testabilità** (Wiegers · Adzic) | **8.0** / 10 | quasi ogni blocco nomina il proprio automation test — è il punto più forte della specifica |
| **Consistenza** | **4.0** / 10 | `B01` e `B08` si contraddicono: il primo rende `PASS` il tetto, il secondo vuole cogliere lo stallo che quel tetto misura |
| **Fedeltà misurata** | **3.5** / 10 | **sette premesse su tredici** verificabili sono scadute o false; quattro blocchi ordinano lavoro già in suite |
| **Complessivo** | **6.0** / 10 | ottimo ordinamento, premesse da rifare, due blocchi da ritirare |

---

## 4. Findings — critique

Severità: 🔴 critico (produce regressione) · 🟠 maggiore (produce lavoro sprecato) · 🟡 minore.

### 🔴 F-01 · `B01` chiede di rendere verde lo stallo che `B08` vuole cogliere — WIEGERS, CRISPIN

> «scenario corto con `MaxTurns` e nessun winner **può PASS**» (B01, acceptance)

Il codice ha già la separazione che `B01` chiede — `ERTTestOutcome` non nomina nemmeno il vincitore. Ciò che
`B01` cambierebbe davvero è l'**unico** punto in cui il tetto produce `Fail`, e quel punto ha una motivazione
scritta nel campo stesso (`RTTestScenario.h:740-747`):

> *«Raggiungerlo è un `Fail`, non un `Pass`: il tetto esiste perché un test appeso somiglia a un test lento, e
> una partita che non finisce è un difetto del gioco — quello misurato da #1088, dove dodici round di soli
> spostamenti finivano in pareggio.»*

**CRISPIN**: «La specifica chiede due cose incompatibili a undici pagine di distanza. `B01` vuole che il tetto
sia un esito neutro; `B08` vuole che il bot che non attacca sia colto da un test. Il tetto **è** quel test.
Applicare `B01` alla lettera cancella il rilevatore e poi apre una issue per ricostruirlo.»

**WIEGERS**: «Il requisito reale non è *«MaxTurns può passare»*. È: *«un free-run che esaurisce il tetto è un
`Fail`; uno scenario a turni scriptati che li esaurisce senza vincitore è un `PASS`»* — e la seconda metà è
**già vera oggi**, perché quegli scenari non hanno un tetto, hanno una lista di turni.»

📝 **Riscrittura**: `nessuna nuova semantica. Il verdict è già separato dall'outcome; il solo Fail legato al tetto è il presidio di #1088 e resta.`
🎯 **Priorità**: alta — è l'unico blocco che, eseguito, **peggiora** il repository.

### 🔴 F-02 · Quattro blocchi ordinano lavoro già in suite — COCKBURN

`B02` (EndReason), `B04` (speed invariance), `B10` (repeat determinism) e metà di `B13` (replay parity) sono
implementati e verdi. Non «parzialmente»: con i nomi esatti che la specifica chiede.

| Blocco | Chiede | Esiste già come |
|---|---|---|
| `B02` | `EndReason` separato dal Winner | `ERTMatchEndReason` — `RTTurnRules.h:37-45`, con la separazione nel docstring |
| `B04` | stesso hash a 1x/2x/4x/headless | `Match.Autobattle.DeterminismIsIndependentOfPlayback` (+ 7 varianti da E47.2/#955) |
| `B10` | ≥16 repeat, stesso `TurnLogHash` | `repeatCount` (cap 100) + `G4` a 100 ripetizioni + `Simulation.ChecksumStableAcrossPermutations` |
| `B13` | decisione FIRE/HOLD identica al replay | `Replay.Verifier.ReactionDecisionsComeFromTheTrace` + `RecordedResponseBeatsLiveDecider` |

**COCKBURN**: «Il goal del primary actor è raggiunto in quattro casi su quattordici. Una specifica che ordina
un goal raggiunto produce lavoro nullo oppure — peggio — un secondo test che fa la stessa domanda con un nome
diverso, e da quel momento due oracoli possono divergere senza che nessuno se ne accorga.»

### 🟠 F-03 · `B14` propone un nome che il gate `G3` rifiuta — FOWLER

> «Creare una suite canonica, ad esempio: `RefactorTactics.v01.CompleteMatch`»

La regola **6.1** di `roadmap-v0.1.md` impone il prefisso gerarchico di dominio, e `G3` la verifica
staticamente: **0 violazioni su 1364 stringhe**. `v01` è una release, non un dominio. Un nome così introduce
una seconda tassonomia proprio dove il progetto ne ha una sola presidiata.

**FOWLER**: «La suite di release non è un prefisso: è un **filtro**. `rt-suite.ps1 -Filter` esiste già e
accetta qualunque sottoalbero. Il gate v0.1 si esprime come elenco di nomi nel DoD — che è ciò che `G3` fa
oggi con i dieci nomi vincolanti — non come radice nuova.»

📝 **Riscrittura**: `il gate v0.1 è un elenco di nomi esistenti nel DoD, eseguito con -Filter; nessun prefisso nuovo.`

### 🟠 F-04 · Il vero blocco della v0.1 non è nella lista dei quattordici — NYGARD

Il critical path proposto mette `B12 Objective` in settima posizione. Ma `Objective` è la capability che
**blocca il T8 del Golden Scenario** (`RTScenarioSession.cpp:259`, owner `#75`) ed è una delle tre vie di
`ERTMatchEndReason` che `E10` pretende con un test per ciascuna. Finché `#75` è aperta:

- `B09` non può chiudere oltre `T7` — e infatti `expect` root dichiara `TurnsCompleted: 5`;
- `B12` non è un blocco nuovo: è `#75`, aperta, con milestone;
- `G10` può chiudere **solo** per `Elimination` o `RoundLimit`, mai per `Objective`.

**NYGARD**: «La specifica ordina di *costruire un objective system generale*. Ne esiste già il contratto —
`ERTMatchEndReason::Objective`, `Team0Score`/`Team1Score` interi in `FRTMatchState`, la soglia nel formato.
Ciò che manca è **il produttore del progresso**, che è precisamente lo scope di `#75`. Aprire una issue nuova
qui creerebbe il manager parallelo che la specifica stessa vieta al §REGOLE.»

### 🟡 F-05 · `B03`/`B11` sono l'unico gap reale, e sono più piccoli di come sono scritti

Misurato: `FRTTestResult` **non ha** `WallClockDuration`, `SimulationDuration`, né `PlaybackSpeedMultiplier`;
`FRTTestScenario` **non ha** `MaxWallClockSeconds`. `ARTTurnManager::ViewerPlaybackSpeed` e
`URTPlaybackLibrary::EffectivePlaybackSpeed` esistono, ma vivono nella presentazione — che è dove devono
stare, ed è ciò che `B03` chiede di garantire.

È lavoro legittimo e non posseduto da nessuna issue. Ma è **un campo di safety-timeout nel runner e tre
metriche nel report**, non un sistema: i quattro test nominati da `B11` (`MaxTurnsStopsRun`,
`FastModeSkipsPresentationWaits`, `HeadlessDoesNotSleepForScriptedReaction`, `TimeoutPreservesCanonicalOutcome`)
hanno già i primi due coperti in sostanza da `FreeRun.*` e da `DeterminismIsIndependentOfPlayback`.

---

## 5. Mapping `B01`–`B14` → stato reale

| # | Blocco | Stato misurato | Owner esistente | Azione |
|---|---|---|---|---|
| `B01` | Verdict ≠ outcome | ✅ **già separato** | — | ⛔ **non aprire** — vedi F-01 |
| `B02` | Outcome taxonomy + EndReason | ✅ **implementato** | `E10` #24 | ⛔ non aprire |
| `B03` | Execution settings + timing metrics | 🟠 **gap parziale reale** | nessuno | ✅ **issue nuova** (piccola) |
| `B04` | Playback speed invariance | ✅ **verde in suite** | `E47` #952 | ⛔ non aprire |
| `B05` | Reaction FIRE/HOLD baseline | ✅ **canonica e replayable** | `E14`, #1118 | ⛔ non aprire; `#1118` copre il residuo |
| `B06` | Capability closure | 🟡 **due mancanti, entrambe con owner** | #1060, #75 | ⛔ non aprire |
| `B07` | CompleteMatch.Minimal | 🟡 **esiste come `AutoBattle.ArenaV01` free-run** | `G10`, `G13` | ⛔ non aprire |
| `B08` | Bot combat regression | 🔴 **#1088 CLOSED, catena viva** | #1655 · #1551 · #1550 | 🔄 **aggiornare**, non creare |
| `B09` | Golden Relay full | 🟡 **T1–T5 giocano, T6/T8 bloccati** | `E15` #153 · #170 | 🔄 aggiornare con le dipendenze |
| `B10` | Determinism corpus | ✅ **`repeatCount` + `G4`** | `E12` #26 | ⛔ non aprire |
| `B11` | Duration / timing tests | 🟠 **gap reale, sovrapposto a `B03`** | nessuno | ✅ **fondere in `B03`** |
| `B12` | Objective / Match End | 🔴 **è il vero blocco** | **#75** CP 10.2 | 🔄 **aggiornare #75**, non creare |
| `B13` | Replay parity | ✅ **sei test verdi** | #170 · #813 | ⛔ non aprire |
| `B14` | Automation gate v0.1 | 🟡 **è il DoD `G1`–`G14`** | DoD | 🔄 aggiungere la regola «poche fixture» |

**Bilancio: 1 issue nuova su 14 blocchi.** Le altre tredici sono coperte, verdi o già possedute.

---

## 6. Issue duplicate evitate

Nel formato che il referto gemello §6.2 conserva — rendere osservabile il *non-lavoro*:

| Proposta della specifica | Coperta da | Prova |
|---|---|---|
| `[Scenario] Separare verdict dal match outcome` | — | `ERTTestOutcome` ≠ `ERTMatchOutcome`, due enum in due header |
| `[Match] Outcome taxonomy e EndReason` | `E10` #24 | `ERTMatchEndReason` con quattro vie |
| `[Scenario] Playback speed invariance` | #952 (E47.2/#955) | `Match.Autobattle.DeterminismIsIndependentOfPlayback` |
| `[Reaction] Decision baseline FIRE/HOLD` | `E14`, #1118 | 6 test `Replay.Verifier.*` + capability `DecisionBoundary` disponibile |
| `[Scenario] CompleteMatch.Minimal` | `G10` · `G13` | `AutoBattle.ArenaV01` free-run su mappa d'autore |
| `[QA] Determinism corpus` | #26 · `G4` | `repeatCount` cap 100, confronto byte-per-byte |
| `[Replay] Parity` | #170 · #813 | `Replay.Verifier.ResimulationIsDeterministic` = il test di `G4` |
| `[Objective] Match end system` | **#75** | capability `Objective`, owner dichiarato nel codice |
| `[Bot] Combat regression` | #1655 · #1551 · #1550 | #1088 chiusa; `RTBotStalemateProbeTests`, 11 asserzioni |
| `[QA] Suite RefactorTactics.v01.*` | DoD `G1`–`G14` | il nome violerebbe la regola 6.1 che `G3` presidia |

---

## 7. L'unica issue nuova realmente necessaria

**Titolo**: `Il report di uno scenario non distingue tempo di simulazione, di presentazione e di parete: un test appeso e un test lento hanno lo stesso referto`

- **Existing ID**: nessuno (ricerche eseguite: `velocita`, `timeout`, `scenario harness`, `wall clock` — zero corrispondenze in 278 aperte)
- **Parent**: `E47` [#952](https://github.com/DegrassiAaron/refactor-tactics-main/issues/952) — Mini v0.1 Autobattle
- **Milestone**: `v0.1 · Prova integrata`
- **Problem statement**: `FRTTestResult` porta `TurnsPlayed` e `StateHash` ma **nessuna durata**. Un free-run che si impianta e uno che gira lento producono lo stesso `result.json`, e il runner non ha un tetto di parete: il solo tetto è in turni, che non scatta se un turno non termina.
- **Scope IN**: `MaxWallClockSeconds` in `FRTTestScenario` (safety del runner, **mai** regola di gioco); `WallClockDuration` e `SimulationDuration` in `FRTTestResult`; il timeout produce `Error`, **non** `Fail` — non è un difetto del gioco.
- **Scope OUT**: `PlaybackSpeedMultiplier` come campo di scenario (la velocità è del *viewer*, e metterla nel dato la renderebbe un ingresso della simulazione — l'opposto dell'invariante); qualunque modifica a `MaxTurns`.
- **File**: `ScenarioHarness/RTTestScenario.h`, `RTTestResult.h`, `RTScenarioRunner.cpp`, `RTScenarioLoader.cpp` (**bump di versione schema**, oggi `4`), `RTTestReportWriter.cpp`.
- **Acceptance**: uno scenario con `maxWallClockSeconds: 1` e un turno che non termina esce `Error` con la ragione; le due durate compaiono in `result.json`; `TurnLogHash` e `StateHash` **non cambiano** con o senza il campo.
- **Automation test**: `Scenario.WallClockTimeoutIsErrorNotFail` · `Scenario.DurationsAreReportedSeparately` · `Scenario.TimeoutDoesNotChangeTheHash`.
- **Rischio determinismo**: 🔴 **alto se sbagliata** — una durata che entrasse nell'hash renderebbe la suite non riproducibile. L'ultimo test dell'elenco esiste per questo.
- **Commit**: `feat(scenario): il referto separa il tempo di parete dalla simulazione, e un test appeso non è più un test lento`

---

## 8. Critical path v0.1, rimisurato

Il percorso della specifica è ordinato bene ma parte da lavoro fatto. Questo parte da ciò che è **rosso o
aperto** oggi:

```
#75 Objective (CP 10.2)          ──┬──> T8 del Relay              ──> G10 via Objective
                                   └──> E10 chiusa (3 vie testate)
#1655 · #1551 · #1550 BOT-STALL  ─────> G10 «non degenere»        ──> G13
#1060 vocabolario T6             ─────> T6 del Relay              ──> E15 · #170 golden replay
[nuova] durate e timeout         ─────> referti leggibili         ──> G2 packaged ripetibile
G9 PIE (2 aperte, 5 parziali)    ─────> G13
G11 KPI · G14 docs               ─────> chiusura E12
```

**Prossimi 5 task di codice, in ordine di dipendenza**

1. **#75 — il produttore del progresso obiettivo.** Sblocca la capability `Objective`, il T8 del Relay e la terza via di `E10`. È il blocco con più valle a valle.
2. **#1655 → #1551 → #1550 — la catena BOT-STALL.** `G10` non chiude con un pareggio degenere, e il probe misura già dove guardare: candidato generato e scorato, ma non selezionato.
3. **#1060 — `OriginalTargetEquals` / `EffectiveTargetEquals`.** Vocabolario di assertion, non gameplay: il T6 esiste ma non è esprimibile.
4. **[nuova] durate e `maxWallClockSeconds`.** Piccola, indipendente dalle altre tre, e rende diagnosticabili le run delle prime tre.
5. **`G2` metà packaged, ripetibile.** Gli 11 test `ClientContext` passano ma la run muore in `exit 3` allo shutdown GPU: il verdetto non è leggibile dal codice di uscita.

---

## 9. Cosa questo referto NON ha fatto

- ⛔ **Nessuna issue creata, aggiornata o chiusa.** Il dry-run precede la scrittura.
- ⛔ **Nessuna suite eseguita, nessun build.** I gate citati sono quelli che il DoD dichiara, rimisurati dal suo owner il 2026-08-29 su `bbf0d780` — non da questa sessione.
- ⛔ **Nessun commit.** L'albero di lavoro appartiene a un'altra sessione (`diag/1665-istanze-board`, due `.uasset` e un `.cpp` modificati): questo file nasce **untracked** e il suo commit vuole un branch dedicato da `origin/main`.
- 🟡 **Non verificato**: che i sei test `Replay.Verifier.*` coprano *tutte* le domande di `B13` — è stato letto il loro nome e il loro scopo dichiarato, non il corpo di ciascuno.
- ✅ **`doc-links.ts --check` è VERDE su questo branch**: **4046** link in 283 documenti, tutti risolvono.
  ⚠️ Alla stesura era **rosso**, e la ragione era la base: il branch di allora era 3 commit dietro
  `origin/main`, dove il referto gemello è arrivato con il merge di
  [#1669](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1669). Rimisurato dopo il rebase, come
  la nota precedente prescriveva di fare.
