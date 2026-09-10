# Debug Simulation — referto di spec panel

> `AUDIT` · **Data**: 2026-09-10 · **Base di misura**: `origin/main` = `18065c28`, dopo `git fetch --prune`
> e `git pull --ff-only`.
> **Nato da**: una richiesta di creare una *«Debug Simulation Mode developer-only»* con dieci epic di
> release (`v0.1`…`v1.0`) e dieci issue operative. **La roadmap non è stata creata** — vedi §2.
>
> ⚠️ **Questo file non dichiara stati di avanzamento.** Ogni riga marcata 🔎 è una misura del 2026-09-10
> con accanto il comando che la produce. Rimisurala prima di fidartene.
>
> **Cosa non è**: una fonte. Se questa pagina e un owner divergono, ha ragione l'owner.

---

**Indice** · [1. La richiesta](#1-la-richiesta) · [2. Perché la roadmap non è stata creata](#2-perché-la-roadmap-non-è-stata-creata) · [3. Il call flow reale](#3-il-call-flow-reale) · [4. Dove l'avanzamento è accoppiato al tempo](#4-dove-lavanzamento-è-accoppiato-al-tempo) · [5. Cosa esiste già](#5-cosa-esiste-già) · [6. Il buco vero](#6-il-buco-vero) · [7. Le cinque issue aperte](#7-le-cinque-issue-aperte) · [8. Deduplicazione](#8-deduplicazione) · [9. Domande aperte](#9-domande-aperte) · [10. Cosa è stato fatto in questa sessione](#10-cosa-è-stato-fatto-in-questa-sessione)

---

## 1. La richiesta

Poter avviare il gioco o uno scenario in una modalità di debug e controllare **semanticamente**
l'esecuzione: `Play`, `Pause`, `Step`, `Next Action`, `Next Phase`, velocità, stato corrente, event log,
overlay tattici, e gli stessi comandi sugli scenari esistenti. Con un principio dichiarato:

```text
semantic simulation stepping != frame stepping
```

e un invariante architetturale: il debugger controlla **quando** la simulazione avanza, mai **come** si
risolve.

🔑 **L'invariante è corretto, ed è già scritto nel repository** — non da questa sessione. È il pavimento
di [`#1881`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881), aperta il 2026-08-30:

```text
Resolver autorevole
  -> TurnLog / Resolved Timeline canonica
       -> Playback / Inspection
            -> Speed · Play/Pause · Step Micro-step · Seek/Replay

MAI: Playback -> modifica Resolver
```

---

## 2. Perché la roadmap non è stata creata

La richiesta chiedeva `EPIC v0.1` … `EPIC v1.0`, una per release, con dieci milestone corrispondenti.

🔎 **Le milestone esistono già, e significano altro** (`gh api repos/DegrassiAaron/refactor-tactics-main/milestones?state=all`):

| Milestone | Tema del repository | Tema chiesto dal prompt |
|---|---|---|
| `v0.2 · Struttura e finestre` | il campo diventa manipolabile | Observability, Watches & Breakpoints |
| `v0.3 · Informazione` | conoscenza parziale | Replay & Reproducibility |
| `v0.4 · Operations` | partite lunghe, mappe grandi | Debug State Manipulation |
| `v0.7 · Competitive Alpha` | dedicated server | Multiplayer Debug Control |

Le due scale non sono compatibili: sono due significati diversi dello stesso nome.

🔴 **E la forma è già stata respinta.** [`roadmap-balance.md`](../roadmap-balance.md) §11 elenca come
**primo** rischio aperto:

> **Backlog parallelo** — *«quarta ricomparsa della stessa proposta: milestone 2026-08-13 (superata),
> `TD 0.x` rinumerata ⛔, `SW-E1…SW-E9` ⛔»*

La quinta è stata `CLAUDE_CODE_CREATE_SCENARIO_LAB_ISSUES.md`, respinta il 2026-09-09
([referto](match-lab-discovery-spec-panel-2026-09-09.md) §2 e §8, prompt archiviato in
[`docs/archive/src/handoff/`](../../archive/src/handoff/2026-09-09-prompt-scenario-lab-non-eseguito.md)).
Questa sarebbe stata la sesta.

⚠️ **Il prompt conteneva già la regola che lo ferma**, e applicarla è stato eseguirlo, non disobbedirgli:

> *«Se esiste già un'Issue che copre sostanzialmente una delle nuove attività: riusala […] NON crearne
> una copia»* · *«Non creare un secondo sistema parallelo di tracking»* · *«Se NON le usa, non introdurre
> dieci milestone inutilmente»* · *«Se l'architettura reale differisce dalle ipotesi di questo prompt,
> adatta le Issue alla realtà del codice e spiega la deviazione»*.

∴ **Owner scelto: `#1881`**, che è letteralmente *«bot, replay e debug fino alla v1.0»*. Le issue nuove
sono sue sub-issue.

---

## 3. Il call flow reale

Ricostruito leggendo `Turn/`, `ScenarioHarness/`, `Replay/`, `Player/`, `UI/`.

```text
                       tre orologi                          l'harness
   ┌───────────────────────┴───────────────────────┐            │
   │ PlanningTimerHandle   ReadyCountdownTimer     │            │ RTScenarioRunner.cpp
   │ PrepWindowTimerHandle                         │            │ RTScenarioSession.cpp
   └───────────────────────┬───────────────────────┘            │
                           ▼                                    ▼
                    OnPlanningTimeout()  ─────────►  LockInAndResolve()   RTTurnManager.cpp:2113
                                                            │
                          EnsureMatchRoster · Pacing.NoteLockIn · clear dei tre timer
                          ResolvedTimeline.Reset · TurnLog.Reset · ValidatePlansAtLockIn
                                                            │
                                                            ▼
                                                   RunPhaseLoop()          :2082
                                                            │
                        ┌───────────────────────────────────┴──────────────────────────┐
                        │  do {  Phase = NextPhase(Phase)                               │
                        │        Prep  -> ResolvePrep()                                 │
                        │        Dash  -> ResolveDash()                                 │
                        │        Blast -> ResolveCombat()   ──► Brace  ► PendingBlast   │
                        │        Move  -> ResolveMovement() ──► Overwatch ► PendingMove │
                        │        if (IsResolutionSuspended()) return;   ◄── UNICA uscita│
                        │  } while (Phase != Planning)                                  │
                        └───────────────────────────────────┬──────────────────────────┘
                                                            ▼
                                              ConcludeResolution() ──► BeginPlayback()
                                                            │
                                                            ▼
                                              TickPlayback(DeltaSeconds)     ◄── Tick, ViewerPlaybackSpeed
                                                            │
                                                      FinishPlayback() ──► ConcludeTurn()
```

E la ripresa, quando una fase si è sospesa:

```text
SubmitReactionResponse / ExpireReactionWindow / CloseBraceWindow
        │
        ▼
ResumeSuspendedResolution()          RTTurnManager_Movement.cpp:777
        │   ├─ PendingBlast sospeso? ─► ResumeBlastResolution()
        │   └─ altrimenti            ─► while (AdvanceMovementResolution() == Advanced)
        │                               FinishMovementResolution()
        ▼
RunPhaseLoop()      ← le fasi rimaste, e NON riesegue quella risolta ([D-356])
        ▼
ConcludeResolution()
```

**Il movimento, dentro `ResolveMovement`** (`RTTurnManager_Movement.cpp:312`):

```text
BeginMovementResolution()
    │
    └─► while (Step == Advanced && Guard < 256)
             Step = AdvanceMovementResolution()      ──► Advanced | Finished | Suspended
    │
    ├─ Suspended ─► return SENZA concludere: il contesto resta vivo
    └─ altrimenti ─► FinishMovementResolution()
```

---

## 4. Dove l'avanzamento è accoppiato al tempo

| Accoppiamento | Dove | Tocca la logica? |
|---|---|---|
| `Tick` → `TickPlayback` | `RTTurnManager.cpp:7659`, `Dt = DeltaSeconds * EffectivePlaybackSpeed(ViewerPlaybackSpeed)` | **no** — riproduce eventi già decisi |
| `Tick` → `TickReactionWindow` | `RTTurnManager.h:1340`, `DeltaSeconds` **non** scalato | **sì**, ma solo per la scadenza della finestra |
| Tre `FTimerHandle` | `PlanningTimerHandle`, `ReadyCountdownTimerHandle`, `PrepWindowTimerHandle` | portano al commit, non risolvono |
| `FRTScenarioSession::Tick` | un passo per **frame** renderizzato (`bPumpTurnManager = false`) | no — pompa, non decide |
| `URTScenarioRunner::RunSingle` | passo fisso di `0,05 s` | no |
| Animazioni | ⛔ nessuna: `URTPlaybackLibrary` è matematica pura, e la locomozione è interpolazione | **no** |

🔑 **La risoluzione non dipende dal frame rate, e non è un merito da conquistare: è già così.**
L'harness chiama `LockInAndResolve()` **diretto** e risolve un turno intero in una chiamata, senza mondo
ticcante. È ciò che rende il livello headless della piramide sufficiente per quasi tutto — e ciò che
rende falsa la premessa del prompt secondo cui *«l'esecuzione gameplay è oggi accoppiata a Tick,
animazioni, presentation»*.

⚠️ **L'unica eccezione dichiarata è recente e voluta**: dal 2026-09-08 ([D-355]) simulazione e
presentazione **si interlacciano** al confine di micro-step, perché una finestra di reazione umana deve
aprirsi su un movimento che lo schermo ha già mostrato. Il turno può quindi esistere risolto a metà, e lo
stato logico a metà barriera vive in `PendingMovement` / `PendingBlast`.

---

## 5. Cosa esiste già

Otto delle dieci issue chieste dal prompt hanno un proprietario. Misurato il 2026-09-10.

### 5.1 I confini semantici — **esistono e sono tre**

`(TurnNumber, ERTMatchPhase, MicroStepIndex)`. `FRTTurnLogEntry` porta tutte e tre più `ActionId`,
`BaseActionId`, `UnitId`, `SrcCell`, `TgtCell`, `Amount`, `Category`, `Outcome`, `GraphRevision`. Il
formato è versionato: `ERTTurnLogFormatVersion::WithMicroStep = 12` (**#1880**). `FRTBoundaryChecksum`
(**#2189**, **#2374**) indirizza la stessa terna e sa dire **dove** due esecuzioni divergono.

⛔ **Non serve una tassonomia nuova.** Il prompt chiedeva di definire `Phase` / `Action` / `Step`
*«basandosi sul codice esistente»*: il codice esistente li ha già, e l'unico dei tre che non attraversa
tutti i confini è `Action` — vedi §6.

### 5.2 `Play` / `Pause` / `Step` — **consegnati da #1879**

```text
RTTurnManager.h:637   SetPlaybackControlsEnabled(bool)   // fail-closed: default NEGATO
RTTurnManager.h:661   PausePlayback()                    // si ferma al prossimo confine di micro-step
RTTurnManager.h:665   ResumePlayback()
RTTurnManager.h:681   StepMicroStep()                    // un intero micro-step, poi torna in pausa
RTTurnManager.h:696   MicroStepsInCurrentPlaybackPhase()
```

E il confine si calcola in **secondi**, non in tick, con una ragione scritta: *«un "avanza per N frame"
dipenderebbe dal frame rate, e la stessa pressione fermerebbe il playback in punti diversi su macchine
diverse»*. ∴ `semantic stepping != frame stepping` **è già la regola implementata**.

### 5.3 La velocità — **consegnata, con il suo gate**

`ERTPlaybackSpeed { Quarter, Half, Normal, Double, Quadruple, Instant }` — esattamente la scala chiesta,
più `Instant`, che *«si riconosce per NOME: non ha un moltiplicatore»*. Il criterio *«la velocità non
cambia il risultato logico»* è il gate di **#955**, chiuso.

### 5.4 Gli overlay tattici — **esistono, owner #80**

🔎 `grep -rhno 'TEXT("rt\.[A-Za-z0-9_.]*"' Source/RefactorTactics --include=*.cpp | sort -u`

```text
rt.Debug.DrawCells      rt.Debug.DrawCover     rt.Debug.DrawIntent
rt.Debug.DrawPaths      rt.Debug.DrawResolution
rt.Debug.Los            rt.Debug.Knowledge     rt.Debug.HeroProfile
rt.Debug.DumpTurnLog    rt.Debug.DumpSnapshot  rt.Debug.DumpCellPlacement
rt.Debug.Pacing         rt.Debug.VerifyReplay
rt.Camera.TopDown       rt.Camera.TopDownShot
rt.Match.Autobattle     rt.Match.BotAllies     rt.Match.PlanningSeconds
rt.Test.Run             rt.Test.Scenario       rt.Test.List
```

Più `RTOverlayPalette`, `RTOverlayArea`, `RTSightLines`, `RTHexLabelLibrary` (cell ID), e le linee di
tiro appena entrate con **#2742**. Un secondo sistema di visualizzazione sarebbe quello che il prompt
stesso vieta.

### 5.5 Lo stepping degli scenari — **è la forma dell'harness**

`FRTScenarioSession` si descrive da sé: *«esecuzione di uno scenario come macchina a stati, avanzabile un
passo alla volta»*, con `Step(DeltaSeconds, bPumpTurnManager)`. Le due strade — chi guarda e chi verifica
— *«guidano la stessa sessione, non due copie della stessa logica»*.

### 5.6 Event log e HUD — **hanno epic proprie**

**#1937** `Player Event Log & Explainability`, **#2697** (il feed e i suoi consumatori), **#1936**,
**#2453** per il combat feedback; **#25** `E11 — HUD, log e debug` e **#2757** per la riga di stato.

---

## 6. Il buco vero

Tre fatti, ciascuno con la propria misura.

### 6.1 Ciò che si ferma è l'immagine, non la simulazione

`RunPhaseLoop()` risolve `Prep → Dash → Blast → Move → Cleanup` in un `do…while` sincrono con **una**
uscita anticipata: `IsResolutionSuspended()`, vera solo quando una finestra di reazione (**#2679**) o un
`Brace` (**#2692**) hanno sospeso. **Nessun chiamante può chiedere di fermarsi.**

∴ un `Next Phase` applicato al playback non fermerebbe il resolver, che ha già finito; e applicato al
resolver non ha oggi dove agganciarsi.

🔑 **Il meccanismo però esiste ed è già stato pagato.** [D-355] descrive per esteso il costo di portare lo
stack di `ResolveMovement` in stato persistente, e [D-356] ha dichiarato la **forma della continuazione**:
`Phase` resta dov'era, `RunPhaseLoop` esce, `ResumeSuspendedResolution` riprende le fasi rimaste. Ciò che
manca non è il meccanismo — è un **richiedente**.

⚠️ [D-355] lascia esplicitamente aperto *«se il playback delle fasi **senza** boundary conservi la forma
attuale — oggi non ha ragione di fermarsi, e non deve acquisirne una per simmetria»*. Il confine è
sottile e va rispettato: si tocca il resolver, non `TickPlayback`.

### 6.2 La primitiva del passo esiste e nessuno può chiederne uno

```cpp
enum class ERTMovementAdvanceResult : uint8 { Advanced, Finished, Suspended };
ERTMovementAdvanceResult AdvanceMovementResolution();     // RTTurnManager.h:1318, public
```

🔎 I due chiamanti — `ResolveMovement` (`:312`) e `ResumeSuspendedResolution` (`:777`) — hanno lo **stesso**
`while (Step == Advanced && Guard < 256)`. La granularità giusta c'è; l'ingresso per consumarne una
unità no.

### 6.3 I comandi consegnati non sono raggiungibili in partita

🔎 `grep -rn "SetPlaybackControlsEnabled" Source/ | grep -v Tests/`

```text
Source/RefactorTactics/Turn/RTTurnManager.cpp:7568:void ARTTurnManager::SetPlaybackControlsEnabled(bool bEnabled)
Source/RefactorTactics/Turn/RTTurnManager.h:637:	void SetPlaybackControlsEnabled(bool bEnabled);
```

Solo la propria definizione e la propria dichiarazione. Stessa misura per `PausePlayback()`,
`ResumePlayback()` e `StepMicroStep(`.

⚠️ **Il default fail-closed è giusto e non si tocca** — è il modo in cui il divieto sul PvP live esiste
per costruzione. Ciò che manca è l'altra metà: chi lo accende in sviluppo.

🔑 **Il contrasto con la velocità è la prova che è un pezzo mancante e non una scelta**: la manopola della
velocità un ingresso ce l'ha, `Player/RTPlayerController.cpp:1939`. Delle tre voci della matrice di
`#1881` — Speed, Pause, Step — una è premibile e due no.

### 6.4 E `Action` non attraversa il confine

🔎 `grep -c "ActionId" Source/RefactorTactics/Turn/RTResolvedEvent.h` → **1**, ed è un **commento**
(`:227`) che dichiara che `StatusTag` è *«copiato da `FRTTurnLogEntry::ActionId` […] nello stesso punto in
cui la voce viene scritta»*.

∴ il sito di scrittura ha l'`ActionId` in mano, il precedente della copia esiste, e la timeline che guida
il playback non lo porta. Un `Next Action` non ha su cosa fermarsi.

🔑 Il costo è basso e misurato: `RTTurnManager.cpp:3347` e `:3428` dichiarano che *«`ResolvedTimeline` è
playback e non entra né in `StateHash` né nel formato di replay»*.

---

## 7. Le cinque issue aperte

Tutte sub-issue di [`#1881`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881).

| # | Titolo | Priorità | Milestone | Dipende da |
|---|---|---|---|---|
| [#2855](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2855) | `RunPhaseLoop` ha una sola uscita, e nessuno può chiederne una | P2 | — | — |
| [#2856](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2856) | `AdvanceMovementResolution` e nessuno può chiederne **uno** | P2 | — | #2855 |
| [#2857](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2857) | `Next Action` non ha un confine | P3 | — | #2855, #2856 |
| [#2858](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2858) | I controlli di #1879 non hanno un chiamante di produzione | P2 | `v0.1` | — |
| [#2859](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2859) | **Gate**: nessun test dimostra `stepped ≡ continuous` | P2 | — | #2855, #2856 |

```text
#2855 ──┬── #2856 ──── #2857
        │      │
        └──────┴──────► #2859   (gate)

#2858  indipendente — completa il gate v0.1 già dichiarato da #1879
```

🔑 **`#2858` è l'unica in `v0.1`**, e la ragione è che non aggiunge scope: rende **dimostrabile** un gate
che `#1881` dichiara già per la v0.1 — *«Pause e Step si fermano su un safe boundary, in VsBot e Debug»* —
e che oggi è verde nei test e non premibile in partita.

---

## 8. Deduplicazione

### 8.1 Issue riusate invece di crearne una copia

| Attività chiesta | Owner riusato | Nuova issue creata? |
|---|---|---|
| Epic ombrello `v0.1`…`v1.0` | **#1881** — stesso invariante, stessa matrice, stessi gate | ⛔ **no** |
| `0.1-A` characterization | corpus golden, `RTSimulationDeterminismTests`, `RTScenarioCorpusTests`, `RTPlaybackControlsTests` | ⛔ no |
| `0.1-B` decouple advancement ↔ presentation | **#2679** + [D-355] + [D-356]; `URTPlaybackLibrary` estratta da **#1817**/**#1818** | ⛔ no |
| `0.1-C` semantic boundaries | **#1880**, **#2260**, **#2272**, **#2374** | ⛔ no |
| `0.1-D` controller `Play/Pause/Step/Speed` | **#1879** (Pause/Step) + **#955**/**#1015** (velocità) | ➕ solo il pezzo mancante: **#2855**, **#2856**, **#2857** |
| `0.1-E` startup mode | — nessun owner | ➕ **#2858** |
| `0.1-F` debug HUD | **#25**, **#2757** | ⛔ no |
| `0.1-G` event log | **#1937**, **#2697**, **#1936**, **#2453** | ⛔ no |
| `0.1-H` overlay tattici | **#80** | ⛔ no |
| `0.1-I` scenario integration | `FRTScenarioSession` + **#1117** (chiusa), **#2788** (chiusa) | ⛔ no |
| `0.1-J` equivalence | **#955** copre la velocità, non lo stepping | ➕ **#2859** |
| Epic `v0.2` Observability/Breakpoints | **#1881** §*v0.2+*, **#811** `CP 45.4 Osservabilità` | ⛔ no |
| Epic `v0.3` Replay & Reproducibility | `Replay/` completo, **#472** (chiusa), **#415**, ADR-0009 | ⛔ no |
| Epic `v0.5` Scenario regression | `ScenarioHarness/` + `RTScenarioAutoRunTests` + corpus | ⛔ no |
| Epic `v0.6` Advanced tactical diagnostics | **#80**, **#2742**, `RTHexLosConsole`, `RTKnowledgeDebugConsole` | ⛔ no |
| Epic `v0.7` Multiplayer debug | **#759** (privacy temporale, M10), milestone `v0.5`/`v0.7` esistenti | ⛔ no |
| Epic `v0.8` Desync & forensics | **#2189** — `ChecksumsAlongTrace` / `FirstDivergence` / `DescribeDivergence` | ⛔ no |
| Epic `v0.4` State manipulation | ⛔ **fuori scope dichiarato**, e nessuna issue va aperta finché non serve | ⛔ no |

### 8.2 Cose che il prompt chiedeva e che **non vanno costruite**

- ⛔ **Una modalità di gioco per negare i controlli in PvP.** `RTPlaybackControlsTests.cpp:6` lo dichiara:
  `ERTMatchMode`, `bCompetitive`, `bIsPvP` non esistono, e *«inventarla per poterla negare sarebbe
  fabbricare una dipendenza»*. Il divieto si ottiene per costruzione dal default fail-closed.
- ⛔ **Un RNG per il test di equivalenza.** `…Simulation.SeedDeclaredUnconsumed` misura che il seme è
  dichiarato e non consumato: il progetto non ha un generatore da seminare.
- ⛔ **Una fermata al confine di fase nel playback.** [D-355] la vieta per nome.
- ⛔ **Un secondo framework di scenari.** `FRTScenarioSession` è già una macchina a stati steppabile.

---

## 9. Domande aperte

Solo ciò che il repository non risolve da solo.

1. **Il tetto `Guard < 256` sotto avanzamento richiesto.** Oggi conta i passi di *un pump*. Con un
   richiedente esterno, o si azzera a ogni richiesta — e allora smette di proteggere da un resolver che
   non termina — o accumula, e allora una sessione di debug lunga scatta l'`ensureMsgf` senza che nulla
   sia rotto. Registrata in **#2856**, va decisa nella issue.
2. **Il tasto della pausa.** `P` è già `PrepWindowPauseAction` (`RTPlayerController.cpp:449`). Le due
   pause sono mutuamente esclusive per costruzione, ma un tasto che a volte fa una cosa e a volte l'altra
   resta ambiguo per chi preme. Registrata in **#2858**.
3. **Chi legge lo stato quando la simulazione è ferma.** [D-356] dichiara come residuo che *«un test che
   interroghi l'HUD a resolution sospesa non esiste»*, e una sospensione **richiesta** dura quanto vuole
   chi l'ha chiesta, non quanto una finestra. La lettura appartiene a **#25** / **#2757**; il rischio è
   registrato in **#2855**.
4. **Se `Next Phase` sia davvero ciò che serve.** Il prompt lo dà per necessario. Il repository ha già
   `Step` al micro-step e `Seek` per turno e fase nel replay (**#415**): può darsi che la granularità
   mancante sia solo `Action`, e che `#2855` risolva un problema che nessuno ha davvero. **È una domanda
   per il committente, non per un referto** — la stessa forma della domanda 3 di
   [`match-lab-discovery-spec-panel-2026-09-09.md`](match-lab-discovery-spec-panel-2026-09-09.md) §8.5,
   che è ancora senza risposta.

---

## 10. Cosa è stato fatto in questa sessione

- audit read-only di `Source/RefactorTactics/{Turn,Replay,ScenarioHarness,Debug,Map,UI,Player}`, del
  Decision Log, delle roadmap, delle label e delle milestone;
- ricostruzione del call flow di §3 e delle misure di §4–§6;
- deduplicazione contro le issue OPEN e CLOSED, per intento e scope e non per titolo (§8);
- **cinque issue aperte** — **#2855**, **#2856**, **#2857**, **#2858**, **#2859** — tutte sub-issue di
  **#1881** tramite l'API `sub_issues`, cioè il meccanismo che il repository usa già;
- **#1881** aggiornata con una sezione datata che dichiara cosa manca e cosa non è stato aperto.

**Non è stato fatto**: nessuna epic nuova, nessuna milestone nuova, nessuna label nuova; nessun file di
`Source/` modificato; nessuna build, nessun test eseguito, nessun avvio dell'Editor.

### Verifiche

| Gate | Esito |
|---|---|
| Compile | `N/A` — nessun file di `Source/` toccato |
| Tests | `N/A` |
| Determinism · Replay · Privacy | `N/A` |
| PIE · Packaged | `N/A` |
| Misure `grep` di §5 e §6 | eseguite su `origin/main` = `18065c28`, comando accanto a ogni riga 🔎 |
