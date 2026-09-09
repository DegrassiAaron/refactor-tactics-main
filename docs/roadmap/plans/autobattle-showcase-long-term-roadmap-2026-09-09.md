# Autobattle Showcase — roadmap lunga, dalla partita che gira alla partita che si mostra

> `CURRENT` · **Data**: 2026-09-09 · **HEAD misurato**: `origin/main` `c3151afd`
> **Panel**: Wiegers (lead) · Cockburn · Adzic · Fowler · Nygard · Crispin · **Modo**: critique
> **Focus**: requirements + architecture
>
> **Cosa è**: la riconciliazione fra una visione d'esperienza in nove tappe (`A … I`) e gli owner che il
> repository **ha già**, più il minimo di lavoro nuovo che quella riconciliazione ha lasciato scoperto.
> **Cosa non è**: una seconda roadmap di release, un owner di feature, o una fonte di stato. Lo stato vive
> nelle issue e nelle milestone GitHub. Quando questo file e GitHub non concordano, **vince GitHub**.

---

## 0. Il risultato in una riga

**Otto tappe su nove avevano già un owner.** La riconciliazione ha prodotto **due** issue nuove e **una**
decisione aperta registrata; tutto il resto è linkatura fra owner che esistevano già.

| | |
|---|--:|
| Tappe d'esperienza esaminate | **9** (`A`–`I`) |
| Tappe con owner già esistente | **9** |
| Epic nuove create | **0** |
| Milestone GitHub nuove create | **0** |
| Issue nuove create | **2** (#2744 · #2745) |
| Decisioni aperte registrate | **1** (`OBS-1`) |
| Issue chiuse riaperte | **0** |

---

## 1. La visione, e il vincolo che la governa

> Avvio RefactorTactics in una configurazione autobattle, non comando nessuna unità, e guardo una partita
> completa capendo pianificazione, intenti, movimento, combattimento, obiettivi, punteggio e vittoria.

Il vincolo che tiene questa visione fuori dai guai è uno solo, ed è già scritto nel corpo di #952:

```text
#952 Autobattle NON possiede il gioco: lo CONSUMA.

#952
 ├── consuma HUD           → #613 (§4.1) · #25 (E11) · #1937/#1936 (feed)
 ├── consuma Bot           → #2556 · #2629 · #2477 · #326 (E26)
 ├── consuma Camera        → #1769 (E49) · #1781 (CAM-12)
 ├── consuma Presentation  → #286 (E21) · #217 (E20) · #2453 (Combat Feedback)
 ├── consuma Objectives    → #2281 · #331 (E30) · #332 (E31)
 └── consuma Playback      → #1881 · #472 (chiusa)
```

⛔ **Nessuna mega-epic «Autobattle Showcase».** Sarebbe un secondo owner sopra owner esistenti — esattamente
ciò che [`../capability-roadmaps.md`](../capability-roadmaps.md) §3 respinge con *«otto epic nuove sarebbero
otto secondi owner»*.

### 1.1 Perché nessuna Milestone GitHub nuova

Le lettere `A`–`I` di questo documento sono **tappe d'esperienza**, non release. Il repository ha già:

- **release** → milestone GitHub `v0.1 … v1.0` più `PIA` (11 milestone aperte, misurate il 2026-09-09);
- **fette di lavoro** → *checkpoint* (`CP <n>.<m>`, label `checkpoint`);
- **viste trasversali** → sezioni `CR-*` di [`../capability-roadmaps.md`](../capability-roadmaps.md).

E ha già deciso questo caso, per questa stessa epic:
[`D-145`](../../decisions/RT_PDR_00_Decision_Log.md) — *«una sola epic (E47), **nessuna milestone GitHub
nuova**, e nessuna riduzione di scope [...] i nomi non usano mai una seconda numerazione, perché due spazi
collidono e GitHub non sa disambiguare»*. Le sette «release intermedie» proposte allora diventarono i
checkpoint `E47.1 … E47.7`.

∴ `A`–`I` restano **lettere di questo documento**. Non entrano in GitHub come milestone, come label, né come
un `E<n>` nuovo.

---

## 2. FASE 0 — riconciliazione

### 2.1 Fonti lette

`AGENTS.md` · `CLAUDE.md` · [`../roadmap-v0.1.md`](../roadmap-v0.1.md) ·
[`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) · [`../capability-roadmaps.md`](../capability-roadmaps.md) ·
[`../roadmap-checkpoint.md`](../roadmap-checkpoint.md) ·
[`../../decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) ·
[`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) ·
[`../../technical/systems/progettazione-hud.md`](../../technical/systems/progettazione-hud.md) ·
[`../../technical/systems/spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md) ·
[`../../product/showcase-v0.1.md`](../../product/showcase-v0.1.md) · [`showcase-v01-audit.md`](showcase-v01-audit.md) ·
[`mini-roadmap-autobattle-spec-panel-2026-08-16.md`](mini-roadmap-autobattle-spec-panel-2026-08-16.md) ·
`Source/RefactorTactics/{UI,Match,Turn,ScenarioHarness,Bot}/` · `Content/RT/UI/` · `Scenarios/`.

Issue esaminate: **OPEN e CLOSED**, per tutte le famiglie nominate dal mandato più spectator, observer,
objective, score, roster, playback, seek, rewind, scenario, packaged, capture, match setup, multilayer,
presentation.

### 2.2 La tabella

Legenda azione: **KEEP** = l'owner basta così · **LINK** = manca solo il cross-link · **UPDATE** = il corpo
va chiarito · **CREATE** = lavoro reale senza owner.

| Capability | Owner esistente | Stato | Gap | Azione |
|---|---|---|---|---|
| Autobattle: entrambe le squadre al bot | #952 → #954 | CLOSED | — `bAutobattle` · `rt.Match.Autobattle` · `-RTAutobattle` | KEEP |
| Ritmo osservabile / velocità di playback | #955 · #1015 | CLOSED | — la manopola esiste e ha un bottone | KEEP |
| Grammatica visiva board (colore **e** forma) | #956 | CLOSED | — | KEEP |
| Scenario autobattle free-run | #957 | CLOSED | — dal seam di `D-101` | KEEP |
| Corpus di determinismo dell'autobattle | #958 | CLOSED | — `Seed` escluso per `RNG-1` | KEEP |
| Partita registrata, PIE **e** packaged | #959 | CLOSED | — | KEEP |
| Finestra di preparazione osservabile | #2386 | CLOSED | — e decide *chi guarda* in autobattle | KEEP |
| Screen HUD §4.1 in UMG | #613 | OPEN | i 9 `WBP_RT_*` esistono; DoD non chiusa | KEEP |
| **Roster e feed §4.1 in autobattle** | *nessuno* | — | **il §4.1 non legge `IsUnattendedSession()`** | **CREATE → #2744** |
| Player Event Log (pipeline + asset) | #1937 → #1936 · #2697 | OPEN | `WBP_RT_EventLog` **non esiste** in `Content/` | LINK |
| Rimozione pannelli Canvas legacy | #1936 §A | OPEN | — | KEEP |
| Punteggio/obiettivo: la condizione nel tipo | #2281 | OPEN | binding BP non protetto | LINK |
| Bot stall patologico | #2556 · #2629 | OPEN | `BOT-STALL-1` **riaperta** il 2026-09-09 | KEEP |
| Banchi PIE-AI e breakdown decisionale | #2477 | OPEN | 2 banchi su 5 da scrivere | KEEP |
| Competenza del bot oltre la v0.1 | #326 (`E26`) → #328 (`E28`) | OPEN | post-v0.1 | KEEP |
| Camera tattica e presentazione mappa | #1769 (`E49`) + 13 `CAM-*` | OPEN | 5 promosse a v0.1 da `D-286` | KEEP |
| Camera Director della Resolution | #1781 (`CAM-12`) | OPEN | backlog dichiarato, `P3` | KEEP |
| Observer / POV esplicito, omniscient in partita | *nessuno* | — | `OmniscientTeamId` esiste **solo** nell'harness | **DECISIONE → `OBS-1`** |
| Replay: viewer player-facing | #472 | **CLOSED** | ⛔ non ricreare | KEEP |
| Replay: playback core, seek, micro-step | #1881 → #1878 · #1879 · #1880 · #2260 · #2272 | CLOSED | — | KEEP |
| Replay: caso asimmetrico | #2411 | OPEN | — | KEEP |
| Replay: pubblico sanitizzato vs audit privata | #1805 · #2098 · #2156 | OPEN | il filtro non ha produttore | LINK |
| Presentazione dei personaggi | #286 (`E21`) → #288 · #289 · #1758 | OPEN | lavoro in editor | KEEP |
| Combat feedback (cue, danno, status) | #2453 → #2454 · #2456 · #2457 | OPEN | #2455 chiusa | KEEP |
| Icon language | #217 (`E20`) · #265 (`E25`) | OPEN | — | KEEP |
| Shell, Result e ritorno al menu | #934 (`E46`) → #940 | OPEN | **#940 è l'unico CP `E46` aperto** | LINK |
| **Configurazione ripetibile di uno showcase** | *nessuno* | — | sei famiglie di seam, nessun manifesto | **CREATE → #2745** |
| Formato 3v3 | #325 (`E24`) | OPEN | v0.2 | KEEP |
| Stress 4v4 | #221 (`E17`) · #333 (`E32`) | OPEN | v0.1 `P3` / v0.4 | KEEP |
| Mappe Operations, obiettivi multipli | #331 (`E30`) · #332 (`E31`) | OPEN | v0.4 | KEEP |
| Verticalità e multilayer reale | #2388 · #324 (`E23`) | OPEN | `D-332` porta `CR-VERT` in v0.1 | KEEP |
| Vista longitudinale delle capability | #2325 + [`../capability-roadmaps.md`](../capability-roadmaps.md) | OPEN | manca la vista «guardare la partita» | UPDATE |

### 2.3 Come si legge una riga di questo documento

| Etichetta | Significa |
|---|---|
| **FATTO** | misurato su `c3151afd` — file, riga, o esito di un comando riportato |
| **INFERENZA** | conseguenza del sistema corrente, non misurata direttamente |
| **PROPOSTA** | lavoro desiderato, senza owner né DoD finché qualcuno non lo scrive |
| **DECISIONE RICHIESTA** | non diventa implementazione senza una scelta di prodotto |

⛔ Una **PROPOSTA** non diventa un **FATTO** perché è scritta qui.

---

## 3. Le nove tappe

### A — WATCHABLE AUTOBATTLE

**Outcome**: avvio una partita bot-contro-bot e la guardo senza input umano.

**Owner**: #952 (`E47`) · consuma #613.
**Release**: v0.1.

**FATTO — quasi tutto esiste, e i checkpoint sono chiusi.** Tutti e sette i `CP 47.x` (#954 … #959, #1015)
e #2386 sono `CLOSED`. In concreto, su `c3151afd`:

- `FRTMatchBootstrapConfig::bAutobattle` mette entrambe le squadre al bot
  (`Match/RTMatchBootstrapper.cpp:322`), risolto da `ResolveAutobattle()` con precedenza
  console > riga di comando > proprietà;
- il turno avanza da solo: `StartPlanningTimer` → `PlanBots`, `OnPlanningTimeout` → `LockInAndResolve`;
- la finestra di preparazione è un **terzo orologio** dichiarato nel view model
  (`FRTMatchHeaderView::PrepWindowSecondsRemaining`, #2386);
- round, `RoundLimit`, fase e timer sono nel view model e hanno un widget (`WBP_RT_TurnHeader`);
- il §4.2 Tactical World Overlay mostra i piani di **entrambe** le squadre in autobattle
  (`UI/RTHUD.cpp:715`);
- la partita è già stata registrata in PIE e packaged (#959).

**FATTO — il buco.** Il §4.1 in UMG **non ha il ramo autobattle**:

```
git grep -n "IsUnattendedSession" -- Source/ | grep -v "/Tests/"
→ un solo LETTORE: UI/RTHUD.cpp:715   (cioè il §4.2, in Canvas)
```

mentre `UI/RTScreenHudWidgets.cpp` passa `GetPlayerTeamId()` a `BuildPlayerEventFeed` (`:195`),
`BuildTeamRoster` (`:256`) e `BuildUnitCard` (`:270`). ∴ in autobattle roster e feed raccontano **una
squadra sola**.

**Azione**: #2744.

**INFERENZA**: chiusa #2744, la tappa `A` è raggiunta senza altro lavoro di codice — resta la **verifica
umana**, che è `PIE-HEXPLAY-7` e il gate `G10`.

---

### B — READABLE AUTOBATTLE

**Outcome**: non vedo solo *cosa* succede, capisco *perché*.

**Owner**: #1937 → #1936 · #2697 · #613 · #2281.
**Release**: v0.1.

**FATTO — la pipeline esiste, e finisce a un centimetro dallo schermo.**

```text
Resolver → TurnLog canonico → URTPlayerEventProjector::Project → FRTPlayerEvent[]
        → URTHudViewModel::BuildPlayerEventFeed(TurnLog, ObserverTeamId)
        → URTPlayerEventLogWidget::GetFeed()          ← esiste, RTScreenHudWidgets.cpp:195
        → WBP_RT_EventLog                             ← NON ESISTE
```

```
git ls-tree -r --name-only c3151afd -- Content/ | grep -ci eventlog
→ 0
```

`Content/RT/UI/Match/` porta nove `WBP_RT_*` e nessun event log. ⚠️ **Questo aggiorna la misura di #2697**,
che il 2026-09-09 contava *«zero chiamanti fuori dai test»* per `URTPlayerEventProjector::Project`: il
chiamante C++ **c'è ora**, ed è la classe base del widget. Ciò che manca è il `.uasset` — cioè #1936 §F.

**FATTO — il punteggio viaggia senza la sua condizione.** `FRTMatchHeaderView` porta `Team0Score`,
`Team1Score` e `ScoreToWin`, e il suo commento dichiara: *«significa qualcosa solo se la MAPPA dichiara un
obiettivo, e il tipo non lo dice»*. Il percorso Canvas la rispetta e ha un test
(`HUD.MatchStatusShowsObjectiveOnlyWhenMapDeclaresOne`); **un binding Blueprint no**. È #2281.

**DECISIONE RICHIESTA — il roster simmetrico.** `BuildTeamRoster` risponde per contratto a *«chi comando
io»*. Un roster Team A / Team B in autobattle **non si ottiene ribaltando quel significato**: si ottiene o
con una seconda query autorizzata, o componendo due chiamate. #2744 lo scrive come vincolo di *out of
scope*, e il caso generale è `OBS-1`.

**Azione**: LINK (#1936 ← #2744) · KEEP.

---

### C — USEFUL AUTOBATTLE PLAYTEST

**Outcome**: la partita è abbastanza sensata da poterci giudicare il gameplay.

**Owner**: #2556 · #2629 · #2477 · #326 (`E26`).
**Release**: v0.1 per i rossi e i banchi; v0.2 → v0.3 per la competenza.

**FATTO — lo stallo è aperto e ha una decisione riaperta.** `BOT-STALL-1` in
[`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) è **RIAPERTA il 2026-09-09**: *«lo stallo di un'unità è
l'immobilità, o l'immobilità STERILE?»*. Due esenzioni convivono nel corpus
(`Match.Autobattle.NobodyParksOnTheAuthoredMap` chiede *inerte*, `…EngagesOnTheGeneratedTestArena` chiede
*ferma*), ed è il perché #2556 e #2629 sono rossi. Istruttoria:
[`2556-stallo-innesco-scattato-istruttoria-2026-09-09.md`](2556-stallo-innesco-scattato-istruttoria-2026-09-09.md).

**FATTO — i banchi.** #2477 misura che di cinque voci `PIE-AI-*`, tre hanno un banco candidato
(`AutoBattle.OpenField` · `Spec.Bot.SeeksUncontestedObjective` + `AutoBattle.Objective` · `AutoBattle.Hazard`)
e **due non ne hanno**: `PIE-AI-03` (lethal contro posizione) e `PIE-AI-04` (cover contro esposizione).

**FATTO — il corpus deterministico esiste già.** `E47.5` (#958) è chiuso; `Scenarios/AutoBattle/` contiene
cinque scenari.

**PROPOSTA — le metriche** (turni · danno · KO · tempo fermo · pressione sull'obiettivo · durata).
⚠️ **Non diventano issue qui**, e il motivo è misurato: `CR-BALANCE` ha già `BAL-METRICS`
(`tools/radar/{rubric,power,precision,profile,balance}.ts`, `D-108`) e `BAL-BATCH` (#776, `E43`), e queste
ultime sono **bloccate a valle del competence gate `D-102` (#543)**. Una metrica di autobattle prima di quel
gate misurerebbe un bot che non sa ancora giocare — che è la riga che `capability-roadmaps.md` §4 scrive
già: *«un risultato bot-contro-bot **non è ancora** una misura di bilanciamento»*.

**Azione**: KEEP.

---

### D — SPECTATOR & CAMERA

**Outcome**: guardo la partita da spettatore, non da giocatore a cui hanno tolto i controlli.

**Owner**: #1769 (`E49`) per tutto il lavoro camera · #1781 (`CAM-12`) per il Camera Director.
**Release**: cinque `CAM-*` promosse a **v0.1** da `D-286` (#1834 · #1835 · #1836 · #1837 · #1838); il
resto **post-v0.1**.

⛔ **Nessuna epic camera nuova.** #1769 possiede stato camera, gesti, zoom e Strategic View, multilayer,
picking, i quattro domini spaziali, occlusione e il Feature Lab. L'autobattle è un **consumer**.

**FATTO — la camera già distingue presentazione da autorità.** Principio 1 dell'epic, da `D-143`:
*«Camera / Render Geometry ≠ Gameplay Authority»*; e `D-253`: *«Camera Occlusion ≠ Gameplay LOS»*. Un
Camera Director che cambiasse ciò che un'unità vede violerebbe il principio, non aggiungerebbe una feature.

**FATTO — `FrameOwnTeam` esiste, è testata, e nessun tasto la raggiunge** (#1837, `CAM-16`). È il mattone
di «inquadra la squadra» che uno spettatore vorrebbe per due squadre invece che per una.

**DECISIONE RICHIESTA — `ObserverMode`.** Vedi §5: è `OBS-1`.

**Azione**: KEEP · LINK.

---

### E — MATCH STORY & OBJECTIVES

**Outcome**: guardando la partita capisco chi sta vincendo, e perché.

**Owner**: #2281 (il contratto) · #331 (`E30`) e #332 (`E31`) per la scala · #1937 per gli eventi.
**Release**: v0.1 il contratto; **v0.4** obiettivi multipli e logistica.

**FATTO — il giudice c'è, e la fonte in v0.1 è una sola.** `ERTMatchOutcome::Objective` esiste con il suo
test (`Match.EndsOnObjective`); il resolver **tace** senza `HasObjectiveCell()`
(`Objectives.SilentWithoutObjectiveCell`); `ScoreToWin = 0` significa *«via per obiettivo disattivata»*, ed
è il valore della v0.1.

**INFERENZA — la trappola che #2281 chiude.** Mostrare `0-0` su una mappa senza obiettivo non è un pareggio:
è un punteggio inventato per una gara che nessuno sta correndo. Il tipo non porta la condizione, quindi il
primo binding Blueprint distratto la reintroduce.

**PROPOSTA** — feedback world-space sugli obiettivi, più obiettivi contemporanei, map control leggibile.
Hanno già una casa: `E30`/`E31` in v0.4. ⚠️ **Non diventano issue v0.1.**

**Azione**: LINK.

---

### F — REPLAY & INSPECTION

**Outcome**: smetto di guardare live e comincio a studiare la partita.

**Owner**: #1881 (playback core cross-release) · #472 (superficie player-facing, **CLOSED** il 2026-09-03).
**Release**: v0.1 il grosso; il confine pubblico/audit attraversa fino alla v1.0.

⛔ **Non si crea un secondo Replay Viewer**, e non si ricrea #472.

**FATTO — la tappa `F` è la più avanti di tutte.** Chiuse: pacing (#1878), Pause al boundary e
StepMicroStep (#1879), micro-step indirizzabile (#1880 · #2260), seek a tre coordinate (#2272), checksum di
boundary (#2189), corpus (#2190 · #2271), indice partite (#416), seek turno/fase (#415), player che non
ricalcola (#470), ponte Blueprint (#999), e la navigazione dalla shell (#2326).
Aperte: #2411 (caso asimmetrico), #1805 · #2098 · #2156 (pubblico sanitizzato vs audit privata).

**FATTO — l'invariante che non si negozia**, da `capability-roadmaps.md` §CR-REPLAY:

```text
Resolver autorevole → TurnLog / Resolved Timeline canonica → Playback core
                          → Replay Viewer · Autobattle · TD / Debug
```

⛔ **Mai** `Playback → modifica/ricalcola Resolver`.

**Azione**: KEEP · LINK.

---

### G — PRESENTATION PASS

**Outcome**: la partita comincia a sembrare un gioco.

**Owner**: #286 (`E21`, personaggi in scena) · #217 (`E20`, icon language) · #2453 (combat feedback).
**Release**: v0.1 per `E21`/`E20`/#2453; #265 (`E25`) in v0.2.

⛔ **Non si raccoglie dentro #952.** Il corpo di #952 lo scrive già come rischio: *«"Watchable" attira HUD,
VFX e animazioni, che sono E11, E20 ed E21 e hanno i propri checkpoint»*.

**FATTO — il perimetro è già ripartito.** #2453 possiede evento → cue (#2454), ciclo di vita degli status
(#2456), diagnostica separata dal layer player-facing (#2457); i numeri di danno guidati dal `ResolvedEvent`
sono chiusi (#2455). `E21` possiede animazioni di locomozione e impatto (#288) e leggibilità tattica
(#289 · #1758).

**INFERENZA**: l'hit pause chiesto dalla visione è compatibile **solo** come presentation-only. Un hit pause
che entrasse nel pacing della risoluzione toccherebbe il budget di playback, che è #1878 — chiusa, e con un
contratto: il playback cambia *quanto velocemente* si guarda un risultato **già deciso**.

**Azione**: KEEP.

---

### H — AUTOBATTLE SHOWCASE / VIDEO MODE

**Outcome**: configuro una demo **prima** di avviarla, premo Play, e la guardo.

**Owner**: #2745 (discovery) sotto #952 · consuma #934 (`E46`) e #1105/#1625 (authoring scenari).
**Release**: post-v0.1.

**FATTO — i seam esistono, e sono sei famiglie.** Nessuno va costruito:

| cosa | dove | precedenza |
|---|---|---|
| modalità non presidiata | `bAutobattle` · `rt.Match.Autobattle` · `-RTAutobattle` | console > cmdline > proprietà |
| compagni al bot | `BotAllyCount` · `rt.Match.BotAllies` · `-RTBotAllies=N` | idem |
| ritmo | `MatchPlanningSeconds` (negativo = non intervenire) | proprietà |
| formato | `MatchFormat` → `ShippedFormatId` → ripiego | asset > id spedito > ripiego |
| mappa | `MapFixtureId` → `MapSource` → `DemoArenaRadius` | il più specifico vince |
| scenario | `rt.Test.Scenario` + `Scenarios/AutoBattle/*.json` (5 file) | `ResolveScenarioToRun` |

più la velocità di playback in HUD (#1015), la prep window (#2386) e la registrazione PIE + packaged (#959).

**FATTO — cosa manca**: un artefatto unico, versionabile, che produca lo **stesso** allestimento in PIE,
Standalone e packaged, e che si possa nominare in un handoff.

**DECISIONE RICHIESTA — la forma.** `UDataAsset` · file di config · riga di comando · manifesto di scenario ·
config di `GameInstance`/`GameMode` · una combinazione. ⛔ **Non si sceglie qui**, ed è il motivo per cui
#2745 è una issue di *discovery* e non di implementazione.

⛔ **Vincolo duro: nessun secondo Scenario Harness.** `E47.4` (#957) ha già evitato quel debito passando dal
seam di `D-101`; un secondo formato lo reintrodurrebbe per accumulo.

**Azione**: CREATE (#2745).

---

### I — REPRESENTATIVE MATCH

**Outcome**: l'autobattle non mostra solo l'arena v0.1, ma una forma che rappresenta il gioco futuro.

**Owner**: #325 (`E24` 3v3, v0.2) · #221 (`E17` stress 4v4, v0.1 `P3`) · #333 (`E32` 4v4 competitivo, v0.4) ·
#331 (`E30` mappe Operations, v0.4) · #332 (`E31` obiettivi multipli, v0.4) · #2388 · #324 (`E23`) per il
multilayer reale · #322 (`E35` roster 8, v0.2) per la varietà di ruoli.
**Release**: **post-v0.1**, salvo `E17` che è già dentro come validazione con `P3`.

⛔ **Nessuna di queste voci diventa una issue v0.1.** `OD-5` in
[`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) lo dichiara già: *«scenario 4v4 di stress in roadmap
dopo E15, come validazione e non come produzione»*.

**PROPOSTA** — «match abbastanza lungo da produrre una storia tattica» e «spettatore che capisce comunque la
situazione» sono **criteri di prodotto**, non lavoro: diventeranno DoD dentro `E24`/`E30` quando quelle
epic si aprono. Restano registrate qui e nel backlog di #952, non come issue premature.

**Azione**: KEEP.

---

## 4. Il grafo, e cosa NON dipende da cosa

```text
                 ┌──────────────────────────────────────────────┐
                 │  A  WATCHABLE   #952 · #2744                 │  ← v0.1, quasi chiusa
                 └───────────────────┬──────────────────────────┘
                                     │  (il roster/feed di A abilita B)
                 ┌───────────────────▼──────────────────────────┐
                 │  B  READABLE    #1936 · #2697 · #2281 · #613 │  ← v0.1
                 └───────────────────┬──────────────────────────┘
                                     │  (per giudicare serve capire)
                 ┌───────────────────▼──────────────────────────┐
                 │  C  USEFUL PLAYTEST   #2556 · #2629 · #2477  │  ← v0.1
                 └───────────────────┬──────────────────────────┘
                                     │
   ┌─────────────────────────────────┼─────────────────────────────────┐
   │                                 │                                 │
┌──▼───────────────────┐  ┌──────────▼────────────┐  ┌─────────────────▼──┐
│ D SPECTATOR/CAMERA   │  │ E MATCH STORY         │  │ G PRESENTATION     │
│ #1769 · #1781        │  │ #2281 · E30 · E31     │  │ E21 · E20 · #2453  │
└──┬───────────────────┘  └──────────┬────────────┘  └─────────────────┬──┘
   │                                 │                                 │
   └─────────────────────────────────┼─────────────────────────────────┘
                                     │
                 ┌───────────────────▼──────────────────────────┐
                 │  H  SHOWCASE / VIDEO MODE   #2745            │  ← post-v0.1
                 └───────────────────┬──────────────────────────┘
                                     │
                 ┌───────────────────▼──────────────────────────┐
                 │  I  REPRESENTATIVE MATCH  E24 · E30 · E31    │  ← post-v0.1
                 └──────────────────────────────────────────────┘

        F  REPLAY / INSPECTION   #1881 · #472(chiusa) · #2411 · #1805
        ────────────────────────────────────────────────────────────────
        ⚠️ NON è a valle di nessuna delle altre. Cammina in parallelo dalla
           v0.1, ed è la tappa più avanzata di tutte: 12 issue chiuse, 4 aperte.
```

### 4.1 Le dipendenze vere, e le preferenze d'ordine

⚠️ **Non si finge una dipendenza dove c'è solo una preferenza.**

| Coppia | È una dipendenza? | Perché |
|---|---|---|
| `A → B` | **sì, parziale** | il feed di `B` in autobattle sarà della squadra sbagliata finché #2744 non passa |
| `B → C` | **no, preferenza** | i rossi di #2556 si tolgono senza toccare l'HUD |
| `C → D` | **no** | le cinque `CAM-*` v0.1 sono indipendenti dal bot |
| `D`, `E`, `G` fra loro | **no** | tre owner distinti, tre verifiche distinte — procedono in parallelo |
| `F` verso chiunque | **no** | `F` non dipende da `A`–`E`: consuma il TurnLog, che esiste |
| `H → A`,`D`,`E` | **sì** | un manifesto che configuri observer e camera policy presuppone che esistano |
| `I → E`,`G` | **sì** | «capisco la situazione su una mappa grande» presuppone objective e leggibilità |
| `C → BAL-*` | **sì, e a valle di un gate** | `D-102` (#543) precede ogni lotto di misura |

---

## 5. Decision gate

| ID | Domanda | Stato | Dove vive |
|---|---|---|---|
| **`OBS-1`** | L'osservatore in partita è una **posizione nominata** (`Omniscient` \| `Team N`), o resta la squadra del `PlayerController` con un ramo per la sessione non presidiata? | **APERTA — registrata il 2026-09-09** | [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| `BOT-STALL-1` | Lo stallo è l'immobilità, o l'immobilità **sterile**? | **RIAPERTA il 2026-09-09** | [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) · #2556 |
| forma dello showcase | `UDataAsset` \| config \| cmdline \| manifesto \| composizione | **APERTA** | #2745 |
| Camera Director | quando si apre `CAM-12` | **DIFFERITA per decisione** | #1781 — *«non prima che il core camera sia chiuso»* |
| replay omniscient vs POV | il replay pubblico è sanitizzato; l'audit privata è un secondo prodotto | **decisa nella forma, aperta nel produttore** | #1805 · #2098 · #2156 |
| objective/score contract | la condizione entra nel tipo | **aperta, con owner** | #2281 |
| roster spectator | seconda query autorizzata vs due chiamate | **vincolata** | #2744 *out of scope* + `OBS-1` |
| **RNG / seed** | serve varietà fra partite a parità di stato? | ✅ **CHIUSA il 2026-08-30** | `RNG-1`/`RNG-2`: **no** RNG in gameplay, `Seed` riservato e non consumato |

### 5.1 `OBS-1` — perché non è una domanda vergine

**FATTO**: `RTScenarioKnowledge::OmniscientTeamId = INDEX_NONE` esiste già
(`Source/RefactorTactics/ScenarioHarness/RTScenarioKnowledge.h:50`), ed è documentato come *«una posizione
NOMINATA, non "il filtro spento"»*. Il Tactical Designer la consuma da #1754 (chiusa): il selettore di
prospettiva offre `Omniscient` più una posizione per squadra schierata.

**FATTO**: in **partita** quella porta non c'è. `ARTPlayerState::TeamIdOf` è l'unica risposta alla domanda
«di chi è la vista?», e lo è **per decisione**: `D-242` / #1730 ha centralizzato quattro filtri di privacy
che prima avevano copie divergenti, una delle quali era un letterale `PlayerTeamId = 0`.

**FATTO**: l'unica autorizzazione d'osservatore in partita oggi è `IsUnattendedSession()`, con **un** sito
di lettura.

∴ `OBS-1` non chiede «inventiamo un observer mode»: chiede **se e quando** la posizione nominata che
l'harness ha già attraversa il confine verso la partita — e con quale garanzia in rete, dove `#784`
(*canary anti-leak*) dice che un client non deve ricevere *un solo byte* del piano avversario.

---

## 6. Cosa è v0.1, cosa è post-v0.1, cosa è solo proposta

### v0.1

#2744 · #1936 · #2697 · #2281 · #613 · #2556 · #2629 · #2477 · #940 · #2411 · #1805 · #2098 · #2156 ·
#286 → #288 · #289 · #1758 · #217 · #2453 → #2454 · #2456 · #2457 · le cinque `CAM-*` di `D-286`
(#1834 · #1835 · #1836 · #1837 · #1838) · #221 (`P3`, tagliabile).

### post-v0.1

#2745 · #1769 e le `CAM-*` non promosse · #1781 (`CAM-12`) · #326 (`E26`, v0.2) · #325 (`E24`, v0.2) ·
#322 (`E35`, v0.2) · #265 (`E25`, v0.2) · #327 · #328 (v0.3) · #331 · #332 · #333 (v0.4) ·
#773 (`E40`, v0.5) · #776 (`E43`, v0.8).

### Solo proposta — nessuna issue, e il perché

| Proposta | Perché non diventa issue oggi |
|---|---|
| metriche di autobattle (turni, danno, KO, tempo fermo, pressione, durata) | `BAL-METRICS` esiste già in `tools/radar/`; `BAL-BATCH` (#776) è a valle del competence gate `D-102` (#543) |
| playlist di match | dipende dalla forma scelta in #2745 |
| cambio POV, focus su unità, frame team come funzioni di spettatore | dipendono da `OBS-1`; il mattone `FrameOwnTeam` è già #1837 |
| «match lungo abbastanza da produrre una storia tattica» | criterio di prodotto, non lavoro: diventa DoD dentro `E24`/`E30` |
| livello/mappa d'ingresso dedicato allo showcase | conseguenza di #2745, non premessa |
| hit pause | ammesso **solo** presentation-only; entra in `E21`/#2453 se e quando |

---

## 7. Controllo finale

| Verifica | Esito |
|---|---|
| Epic duplicate | **0** — nessuna epic creata |
| Issue duplicate | **0** — #2744 e #2745 verificate contro OPEN **e** CLOSED |
| Lavoro CLOSED ricreato | **0** — #472, #954–#959, #1015, #2386 riusate, non riscritte |
| Issue CLOSED riaperte | **0** |
| Milestone GitHub nuove | **0** — `D-145`, e le release restano `v0.1 … v1.0` + `PIA` |
| Label nuove | **0** — usate `v0.1`, `P1`, `post-v0.1`, `P3`, `question` |
| Decisioni trasformate in requisito | **0** — `OBS-1` registrata come decisione, non come issue |
| Presentation dentro il resolver | **no** — `G` resta consumer di `ResolvedEvent` |
| Replay come secondo simulatore | **no** — invariante `CR-REPLAY` ribadita in §F |
| Spectator che allenta la privacy | **no** — #2744 usa `IsUnattendedSession()` come **dato**, vieta `bIsSpectator`, e riporta l'avvertenza di rete |
| #952 resta centrata sull'autobattle | **sì** — due figlie, entrambe di consumo |
| #613 resta owner dello Screen HUD | **sì** — dichiarato nel corpo di #2744 |
| #1769 resta owner camera | **sì** — nessuna epic camera nuova |
| #1881 / #472 restano owner playback | **sì** — nessun secondo viewer |

---

## 8. Prossimo target d'esecuzione

**#2744** — *«Lo Screen HUD §4.1 non sa di essere in autobattle»*.

Tre ragioni, in ordine di forza:

1. **È il più piccolo lavoro che chiude la tappa `A`.** Tutti e sette i checkpoint di `E47` sono già chiusi:
   ciò che separa «la partita gira» da «la partita si guarda» è **un ramo in tre siti**, non un sistema.
2. **La decisione è già presa.** #2386 ha scelto il dato (`IsUnattendedSession()`) e l'ha applicato al §4.2.
   Estenderlo al §4.1 non chiede una scelta di prodotto — chiede coerenza.
3. **Sblocca `B` senza precederla.** Il `.uasset` dell'event log (#1936 §F) verrebbe altrimenti disegnato
   sopra un feed filtrato per una squadra che non gioca: si scoprirebbe il difetto **dopo** aver fatto il
   lavoro d'editor, che è il momento più caro per scoprirlo.

⛔ **Non implementato in questo passaggio**, come da mandato.

---

## 9. Provenienza

- Mandato d'autore del 2026-09-09, consumato integralmente.
- Documento precedente sullo stesso oggetto:
  [`mini-roadmap-autobattle-spec-panel-2026-08-16.md`](mini-roadmap-autobattle-spec-panel-2026-08-16.md) —
  che produsse `D-145`. Questo documento **non lo sostituisce**: quello riconciliò un prompt, questo
  riconcilia una visione d'esperienza contro gli owner che nel frattempo sono maturati.
- Vista longitudinale di navigazione: [`../capability-roadmaps.md`](../capability-roadmaps.md) §`CR-WATCH`,
  aggiunta insieme a questo file. Issue indice: #2325.
