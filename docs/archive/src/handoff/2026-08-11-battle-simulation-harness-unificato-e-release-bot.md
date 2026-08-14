> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO IL 2026-08-11
>
> **Materiale NON autorevole.** Il testo qui sotto è l'originale, invariato. Vale per **provenienza**: la
> regola vive negli owner che l'hanno recepito.
>
> **Recepito da**: [`../../../gameplay/spec-bot-tattico.md`](../../../gameplay/spec-bot-tattico.md) ·
> [`../../../technical/test-e-diagnosi.md`](../../../technical/test-e-diagnosi.md) ·
> [`../../../decisions/RT_PDR_00_Decision_Log.md`](../../../decisions/RT_PDR_00_Decision_Log.md) `D-101`,
> `D-102` · referto
> [`../../roadmap-plans/bot-ai-consolidamento-2026-08-11.md`](../../roadmap-plans/bot-ai-consolidamento-2026-08-11.md) §9.
>
> ✅ **È il secondo handoff Bot/AI dello stesso giorno, ed è calibrato molto meglio del primo.** Il §1.1
> nomina i **due** Feature ID reali invece di undici inventati, il §1.2 nomina `#326` e `#328` con i numeri
> giusti, il §33.1 dice di aggiornarli invece di moltiplicarli, e il §3 avverte da sé di non reintrodurre il
> roster storico. Le premesse false del primo handoff qui non ci sono.
>
> ✅ **Ciò che era davvero nuovo**: il §7 (**Decision Provider** — un provider restituisce *decisioni*, mai
> *esiti*) e il §18 (**competence gate prima del balance**). Nessuno dei due aveva occorrenze nel repository,
> e il secondo aveva già un'istanza viva non riconosciuta come tale — vedi il referto §9.
>
> ⚠️ **Il §1.1 chiede di non creare nuovi `RT-FEAT-BOT-*`, e il consolidamento della mattina ne aveva appena
> creati tre.** La riconciliazione è nel referto §9.1: non è stata una moltiplicazione per capability — che è
> ciò che il §1.1 vieta — ma una conseguenza del fatto che `epic` e `release` sono **valori singoli** nello
> schema del registry, cioè l'eccezione che il §33.1 prevede esplicitamente («solo se la granularità corrente
> del registry lo richiede»).
>
> ⚠️ **Il §1 elenca `docs/wiki/feature-status.md` fra le viste generate: non esiste più.** [D-076](../../../decisions/RT_PDR_00_Decision_Log.md)
> ha reso il clone della Wiki la fonte unica, e la vista si chiama ora `Stato-delle-feature` **nel clone**.
> Stessa sorte per `docs/roadmap/roadmap-editor.md`, che è `HISTORICAL` dal 2026-08-08.
>
> ---
>
> ## 🔴 PRIMA DI IMPLEMENTARE IL BATTLE LAB — due prescrizioni farebbero REGREDIRE il progetto
>
> Revisione del panel, 2026-08-11, misurata su `main`. Referto completo:
> [`…/bot-ai-consolidamento-2026-08-11.md`](../../roadmap-plans/bot-ai-consolidamento-2026-08-11.md) §10.
>
> **1. Il §16 («seed corpus», run appaiate «stesso seed») e il §38 («Repeat 1000») presuppongono un RNG che
> in questo progetto NON esiste.** Misurato: `FMath::Rand`, `FRandomStream` e `RandRange` hanno **zero**
> occorrenze fuori da `Tests/`, e `Scenario.Seed` è caricato, copiato nel result e scritto nel report — mai
> consumato come casualità. Due partite con lo stesso allestimento danno lo stesso esito, **sempre**.
> Conseguenza: quello che il §16 chiama *corpus di seed* è un **corpus di allestimenti** (posizioni, roster,
> mappa), e implementarlo alla lettera spinge verso l'unica cosa che renderebbe vero il nome — aggiungere un
> generatore casuale, cioè distruggere la proprietà da cui dipendono l'hash del TurnLog, il corpus golden e
> la riproducibilità dei replay. E ripetere mille volte un calcolo deterministico non verifica la logica: il
> test giusto è la **permutazione**, che esiste già.
>
> **2. Il §12 definisce il Golden Scenario con `ContentManifestHash` e `RulesVersion`, che
> [D-083](../../../decisions/RT_PDR_00_Decision_Log.md) ha deciso di costruire alla v0.2** con perimetro
> fissato e innesco dichiarato. La stessa decisione vieta la scorciatoia che il §12 induce: *«un campo
> scritto a zero in attesa di un consumatore sarebbe un dato che nessuno legge e un sentinella che somiglia a
> un valore valido»*. Il §12 ha ragione sul principio e sbaglia la lista dei campi.
>
> Più tre rilievi minori nel referto §10: il §11 mette la **performance fra le assertion** (e il §19 dice il
> contrario); il §36 punto 9 rende una feature Done solo **dopo** la release, che è circolare; il §13
> introduce un **secondo schema d'identità** accanto a `D-077`.
>
> ✅ Ciò che il documento ha di **forte** è già entrato: un solo harness e provider che restituiscono
> decisioni (`D-101`, issue [#542](https://github.com/DegrassiAaron/refactor-tactics-main/issues/542)) ·
> competence gate prima del bilanciamento (`D-102`, issue [#543](https://github.com/DegrassiAaron/refactor-tactics-main/issues/543)).

# RefactorTactics — Battle Simulation, Unified Scenario Harness e Bot Release Roadmap
## Handoff operativo per Claude Code — integrazione nello stato corrente, consolidamento, Project Control Center, Wiki, Roadmap, Epic/Issue e checkpoint

**Data:** 2026-08-11  
**Progetto:** RefactorTactics  
**Destinatario:** Claude Code / agente che opera sul repository  
**Baseline documentale nota:** Unreal Engine 5.8; **verificare la patch/toolchain realmente bloccata in HEAD prima di modificare codice o API**.  
**Scopo:** integrare nello stato reale del progetto:
1. il design del **Battle Simulation / Balance Lab**;
2. un **Scenario Harness unico** per scenario visuale, test golden, bot, replay, headless e batch;
3. tutte le decisioni Bot AI consolidate nella chat fino a questo punto;
4. una roadmap progressiva collegata alle **versioni del gioco** (`v0.1`, `v0.2`, `v0.3`, successive) per rendere verificabile a ogni checkpoint che cosa i bot sanno effettivamente fare;
5. Epic/Issue/checkpoint, Feature Registry, Scenario Map, Editor Map, Wiki, PDR/owner spec, milestone e viste del Project Control Center.

> **Questo file NON è una nuova source of truth.** È un handoff di consolidamento. Il repository, il Decision Log/ADR, il codice as-built e i registri canonici correnti prevalgono. Claude deve aggiornare/consolidare gli owner esistenti, non creare un secondo sistema parallelo.

---

# 0. Mandato operativo obbligatorio per Claude

Non limitarti a produrre un report. Devi **integrare e consolidare** queste informazioni nello stato reale del progetto.

Prima di qualunque modifica:

1. leggere `CLAUDE.md`, `AGENTS.md`, `README.md` e istruzioni locali;
2. verificare repository, branch, HEAD, versione UE e toolchain;
3. verificare Decision Log / ADR;
4. ispezionare il bot as-built (`URTHexBotLibrary` o equivalente corrente);
5. ispezionare il Scenario Registry / Scenario Harness / TestDirector / GameMode reali;
6. ispezionare il resolver, snapshot e TurnLog reali;
7. ispezionare il Project Control Center e la sua source of truth;
8. cercare Epic/Issue già esistenti prima di crearne di nuove;
9. cercare Wiki/PDR/spec già owner del dominio;
10. classificare ogni elemento del presente handoff come:

```text
ALREADY_PRESENT
UPDATE
NEW
CONFLICT
DEFERRED
SUPERSEDED
```

11. aggiornare gli owner canonici;
12. rigenerare le viste derivate con il tooling reale;
13. eseguire validator, test e link checker;
14. produrre report finale con numeri reali di issue/epic e file realmente modificati.

## 0.1 Regola di prevalenza

```text
Decisioni esplicite più recenti approvate
>
Decision Log / ADR correnti
>
Codice e dati as-built verificati
>
Feature Registry / Scenario Registry canonici
>
Owner spec / Wiki correnti
>
questo handoff
>
PDR o roadmap storiche
>
brainstorming / documenti superati
```

Se trovi un conflitto, **non scegliere in silenzio**. Riconcilia tramite il workflow corrente e aggiorna `docs/DOC_CONFLICT_MATRIX.md` o l'equivalente reale.

---

# 1. Stato noto da riverificare contro HEAD

Da audit precedenti risultavano presenti almeno:

```text
Repository: DegrassiAaron/refactor-tactics-main
Branch: main

docs/gameplay/spec-bot-hex.md
Source/RefactorTactics/Bot/RTHexBotLibrary.*

docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.json      # generated, NON modificare a mano
scripts/feature_registry.py

docs/roadmap/roadmap.shortlist.md
docs/roadmap/featuremap.shortlist.md
docs/roadmap/scenariomap.shortlist.md
docs/roadmap/milestonemap.shortlist.md
docs/wiki/feature-status.md

docs/roadmap/editor-sessions.yaml
docs/roadmap/roadmap-editor.md

docs/control-center/README.md
docs/roadmap/plans/project-control-center-spec.md
docs/technical/scenario-map.md
docs/roadmap/plans/wiki-consolidamento-2026-08-10.md
docs/CHANGELOG_DOCUMENTATION.md
docs/DOC_CONFLICT_MATRIX.md
```

**Riverificare tutto.** Non creare copie con path differenti se gli owner sono stati rinominati.

## 1.1 Feature Bot già note

Vincolo da consolidamento precedente:

```text
RT-FEAT-BOT-BASE
RT-FEAT-BOT-TACTICAL
```

Non creare una costellazione di nuovi `RT-FEAT-BOT-*` solo perché questo handoff contiene molte capability. La granularità fine va nelle Issue, nei checkpoint, negli scenari e nei test.

## 1.2 Epic Bot reali già individuate

Da audit precedente:

- **#326 — `[EPIC v0.2] E26 · Tactical Bot v1`**
- **#328 — `[EPIC v0.3] E28 · Expert Bot v2`**

Dipendenze/issue correlate già note da riverificare:

- #151 — conoscenza parziale vista/udito;
- #160 — bot/HUD su conoscenza parziale;
- #165 — Reaction/Overwatch bot senza future knowledge;
- #319 — Decision Time Bank / comportamento bot;
- #149 — bilanciamento utility/stallo;
- #291 — facing bot;
- #464 — ramo difensivo/self-heal non raggiunto;
- storiche #36, #202, #213, #337.

**Non duplicare #326/#328.** Questo handoff deve ampliare e collegare il loro scope se coerente con HEAD.

---

# 2. Decisioni correnti da preservare

## 2.1 Bot = producer di Intent

Il bot non controlla direttamente Actor/Pawn e non modifica il simulatore.

```text
Team Knowledge
      ↓
Bot Planner
      ↓
FRTIntent / comando normale
      ↓
Planning / Validation / Commit
      ↓
Snapshot
      ↓
Action Resolver
      ↓
TurnLog
```

Vietato:

```text
Bot -> SetActorLocation
Bot -> ApplyDamage
Bot -> modifica MapState
Bot -> risolve ability
Bot -> bypass validator
Bot -> usa un simulatore gameplay alternativo
```

## 2.2 Fair knowledge / anti-cheat

Anche se il bot gira server-side, usa esclusivamente **player-equivalent TeamKnowledge**.

Può conoscere:
- stato pubblico;
- nemici visibili;
- Last Known;
- rumori e contact autorizzati;
- objective state autorizzato;
- geometria/regole pubbliche;
- intenti della propria squadra;
- azioni nemiche già osservate.

Non può conoscere:
- intenti nemici privati;
- posizione reale di unità invisibili;
- target/path/destinazione futuri;
- future Reaction Opportunity;
- reaction armata non osservata;
- identità reale di un decoy se la squadra non può saperlo.

Invariant:

```text
Same TeamKnowledge
+ same BotConfig
+ same BotSeed
=
same candidates
+ same selected Intent
```

a prescindere da hidden canonical state differente.

## 2.3 Determinismo

Mai usare elapsed CPU time come budget di qualità.

Usare:

```text
MaxGoals
MaxCandidates
TopK
MaxTeamPlans
MaxEnemyScenarios
MaxForwardSimulations
```

e tie-break stabili.

## 2.4 AI target

Core:
- Utility AI custom C++;
- candidate generation;
- team planner;
- reaction planner;
- belief / enemy hypotheses;
- Strategic Director;
- opponent model;
- debug trace.

StateTree: eventuale orchestration/lifecycle.  
Behavior Tree/EQS: solo se hanno un ruolo chiaro, non autorità competitiva.  
Learning Agents/ML: R&D offline futuro, non blocker shipping.

---

# 3. Chiarimento importante su documenti storici

Esistono PDR/demo storici che riportano:
- roster `Aegis / Nyx / Drift / Vex`;
- interrupt da 5 secondi;
- vecchi modelli di demo.

Il consolidamento più recente noto usa invece:

```text
Roster v0.1:
Flux
Riva
Bastion
Vektor

Fast Reaction:
3.0 s
timeout HOLD
```

e turn pipeline corrente da verificare contro ADR/HEAD.

**Non reintrodurre automaticamente roster o timer storici.** Quando un PDR è utile come rationale ma non più current, marcarlo/collegarlo come historical baseline e puntare agli owner correnti.

---

# 4. Obiettivo: un solo Scenario Harness per tutto

Il progetto ha già consolidato il principio:

```text
Human UI --------+
Scenario --------+
Bot -------------+--> Intent
Replay ----------+
                    ↓
                 Planning
                    ↓
                  Commit
                    ↓
                 Snapshot
                    ↓
                 Resolver
                    ↓
                 TurnLog
```

Il nuovo Battle Simulation **NON deve introdurre un secondo harness**.

Deve essere una modalità di esecuzione dello stesso sistema.

## 4.1 Cosa deve poter eseguire lo stesso harness

1. scenario visuale in `L_DevSandbox`;
2. scenario scripted con intent predefiniti;
3. scenario con bot;
4. human + bot;
5. Automation Test golden;
6. replay/audit di una fixture;
7. partita Bot vs Bot completa;
8. fast simulation senza playback;
9. headless match;
10. batch di centinaia/migliaia di match;
11. riproduzione visuale di uno specifico seed fallito;
12. packaged/dedicated soak;
13. future balance tournament.

---

# 5. Architettura proposta dello Unified Scenario Harness

Usare i nomi reali del repository. I nomi sotto sono **contratti concettuali**, non un ordine di creare classi parallele.

```text
Scenario Definition / Run Request
              │
              ▼
        Scenario Registry
              │
              ▼
       Unified Scenario Harness
              │
        ┌─────┼──────────┐
        ▼     ▼          ▼
   Setup   Agents     Assertions
        │     │          │
        └─────┼──────────┘
              ▼
          Planning
              ▼
        Ready / Commit
              ▼
           Snapshot
              ▼
           Resolver
              ▼
          TurnLog
              ▼
       Observation Stream
              │
      ┌───────┼─────────┐
      ▼       ▼         ▼
 Assertions  Agents   Telemetry
              │
              ▼
      next turn / end match
```

## 5.1 Harness responsibilities

Il Harness può:
- caricare/validare ScenarioId;
- costruire il setup iniziale tramite API autorizzate;
- creare gli agent producer di decisione;
- orchestrare planning/commit;
- scegliere modalità di presentation;
- raccogliere TurnLog;
- produrre observation autorizzate;
- eseguire assertion;
- raccogliere telemetry;
- ripetere run;
- salvare un report.

Il Harness **non può**:
- risolvere movement/combat;
- applicare danni;
- mutare direttamente terreno durante il match;
- inventare outcome;
- bypassare reaction framework;
- bypassare validator;
- usare hidden state per far “vincere” un test.

---

# 6. Scenario Definition

Riutilizzare lo schema reale. Concettualmente una definition deve poter esprimere:

```text
ScenarioId
SchemaVersion
ScenarioVersion
MapId / fixture
RulesetId
Content version/hash requirements

Initial state / setup
Teams
Units
Stable UnitIds
Character/Loadout references
Spawn cells
Facing
Public/knowledge setup se supportato

Agent specification per slot/team
Reaction policy per test scripted
Turn limit / match limit

Seed policy
Assertions
Expected hashes quando golden
Metadata
Feature references
Milestone/release references
Tags/categories
```

Non duplicare numeri delle ability: referenziare Stable ID/versioni reali.

## 6.1 Agent specification

Un team/unit slot deve poter usare, secondo lo scenario:

```text
Human
Scripted
Bot
Replay
NoOp/TestPolicy
```

Non è necessario che questi siano `UENUM` se l'as-built usa altro.

Obiettivo:

```text
Scenario A:
Team 0 = Scripted
Team 1 = Scripted

Scenario B:
Team 0 = Bot
Team 1 = Bot

Scenario C:
Team 0 = Human
Team 1 = Bot
```

senza cambiare resolver.

---

# 7. Decision Provider / Agent boundary

Se il codice attuale non possiede già un contratto equivalente, introdurre il minimo seam necessario.

Contratto logico:

```text
Decision Context (sanitizzato/autorizzato)
        ↓
Decision Provider
        ↓
Intent proposal / Reaction response
```

Provider possibili:

### Scripted Provider
Legge intent specificati dallo scenario.

### Bot Provider
Invoca la pipeline Bot reale usando TeamKnowledge.

### Human Provider
Usa il normale PlayerController/UI; il Harness non finge input umano.

### Replay Provider
Rigioca intent registrati/fixture.

### Test Reaction Provider
Serve per casi golden di Fast Decision quando non si vuole input umano; deve passare dallo stesso Reaction Opportunity/validation flow.

**Non creare un provider che restituisce outcome. Deve restituire decisioni.**

---

# 8. Modalità di esecuzione

Preservare le `ExecutionMode` reali se già esistono. Il target logico è:

## Visual
- Editor/PIE;
- playback completo;
- debug overlay;
- utile per design e riproduzione.

## Fast
- usa gameplay reale;
- presentation abbreviata/skip;
- stesso risultato logico.

## Headless
- nessuna dipendenza da UI, LocalPlayer, animation, VFX;
- stato logico puro;
- stesso resolver.

## Batch
- N run derivati da una Experiment Definition;
- output machine-readable.

## ReplayAudit
- riproduce input/snapshot;
- confronta StateHash/LogHash.

Queste modalità **non devono cambiare le regole competitive**.

---

# 9. Lifecycle di una run

Pipeline target:

```text
1. Resolve ScenarioId
2. Validate schema / references / content
3. Resolve RulesVersion / ContentManifestHash / BotConfigVersion
4. Resolve Seed
5. Build initial logical state
6. Build TeamKnowledge iniziale autorizzato
7. Create decision providers
8. Start Turn
9. Ask providers for normal Intent
10. Pass Intent through normal validation
11. Ready / Commit
12. Build immutable Snapshot
13. Run authoritative Resolver
14. Handle Decision Boundaries / Reaction Opportunity
15. Append TurnLog
16. Generate authorized observation stream
17. Update Bot memory/belief from observations only
18. Execute assertions
19. Emit telemetry
20. Repeat until scenario/match end
21. Compute StateHash / LogHash
22. Produce result
```

---

# 10. Reaction / Overwatch nel Harness

Fast Reaction corrente:

```text
3.0 s
FIRE / HOLD
timeout HOLD
```

Il Harness deve supportare:

```text
Scripted reaction response
Bot Reaction Planner
Human real-time response
```

ma sempre passando da:

```text
Reaction Trigger
→ sanitized Reaction Opportunity
→ response
→ server/logic validation
→ resolution
```

Per un test headless non serve aspettare 3 secondi reali: si può applicare la **decision policy** immediatamente, purché il boundary logico e la scelta registrata siano gli stessi.

La modalità Fast/Headless non deve conoscere future opportunities.

---

# 11. Assertions unificate

Le assertion devono poter coprire almeno:

## Setup
- definition valida;
- stable IDs;
- spawn validi;
- regole/versioni corrette.

## Per-turn / per-event
- evento presente/assente;
- reason code;
- unità in cella;
- HP/status;
- graph revision;
- objective state;
- reaction opportunity/response;
- combo/event sequence;
- privacy/knowledge invariant.

## Finale
- winner / score;
- StateHash;
- LogHash;
- turno finale;
- KO/obiettivo;
- nessuna divergenza.

## Bot-specific
- selected Intent/TeamPlanStableId;
- candidate legality;
- no hidden-state leak;
- belief update;
- TeamPlan conflict/synergy;
- decision trace **solo dev/test**.

## Performance
- path queries;
- candidate count;
- team plans;
- resolver time;
- simulation time.

---

# 12. Golden Scenario

Un Golden Scenario deve essere:

```text
ScenarioDefinition
+ RulesVersion
+ ContentManifestHash
+ ResolverConfigHash
+ Agent/Policy version
+ Seed
+ expected canonical result
```

Non bloccare hash golden mentre contenuti/regole sono ancora volutamente instabili.

Quando viene bloccato:

```text
same inputs
→ same StateHash
→ same LogHash
```

Repeat e permutation test devono essere automatizzati.

---

# 13. Structured Result

Riutilizzare `result.json` o lo schema reale.

Campi minimi desiderati:

```text
runId
experimentId optional
scenarioId
scenarioVersion
executionMode

gameRelease
rulesVersion
contentManifestHash
resolverConfigHash
botConfigVersion(s)

mapId
teams/compositions
side assignment

matchSeed
botSeed(s)
pairId optional

result
winner
score
turns

stateHash
logHash

assertionSummary
performanceSummary
telemetryArtifactRefs
failureReason
```

Non inserire metadata cosmetici nell'hash competitivo se non ne fanno già parte.

---

# 14. Riproduzione di un seed problematico in L_DevSandbox

Requisito esplicito del nuovo sistema.

Ogni batch failure deve produrre abbastanza dati per:

```text
Batch/CI failure
    ↓
Run Manifest
    ↓
ScenarioId + versions + seeds + teams + side
    ↓
Replay from CLI/Editor
    ↓
L_DevSandbox Visual Mode
    ↓
same logical result + debug overlays
```

Claude deve integrare con il meccanismo reale di Scenario Selector/GameMode; non inventare una seconda UI.

Il workflow ideale:

```text
1. copio RunId / result artifact
2. seleziono "Replay Run" o imposto ScenarioId + SeedOverride
3. Play
4. vedo lo stesso TurnLog
5. posso attivare Bot DecisionTrace / belief / threat / candidate overlay
```

---

# 15. Battle Simulation Runner

Il Battle Simulation è un **orchestratore di run del Scenario Harness**.

```text
Experiment Definition
        ↓
Generate Run Manifests
        ↓
Unified Scenario Harness
        ↓
Result per match
        ↓
Aggregator
        ↓
Balance / QA Report
```

Mai:

```text
Experiment -> mini-simulatore alternativo
```

## 15.1 Experiment Definition

Concettualmente:

```text
ExperimentId
ExperimentVersion

Scenario/Map pool
RulesVersion
ContentManifestHash

Team A composition(s)
Team B composition(s)

Bot baseline/profile/version
Difficulty/personality matrix optional

Match seed corpus
Bot seed corpus

Side rotation
Mirror policy
Pairing policy

Repeat count
Stop-on-failure policy
Telemetry level

Output format
```

---

# 16. Seed corpus e paired runs

Non generare un corpus completamente casuale ad ogni build.

Usare corpus versionati:

```text
BalanceSeedCorpus.v1
BotSeedCorpus.v1
```

Per confronto A/B:

```text
PairId P001

Run A
  Rules/Balance Candidate A
  Seed 123

Run B
  Rules/Balance Candidate B
  Seed 123
```

Per side bias:

```text
Run 1
Team A West
Team B East

Run 2
Team A East
Team B West

same seed
```

Mirror match obbligatori per diagnosticare:
- side bias;
- tie-break bias;
- map asymmetry;
- bot-order bias;
- initiative issues.

---

# 17. Telemetry Battle Lab

## 17.1 Match
- winner;
- score;
- turns;
- objective control;
- match/side/map/composition;
- StateHash/LogHash.

## 17.2 Unit
- damage dealt/taken;
- KO;
- objective contribution;
- control contribution;
- distance;
- hazard exposure;
- ability/reaction use.

## 17.3 Ability pipeline
Per distinguere `ability weak` da `bot non sa usarla`:

```text
LegalOpportunity
CandidateGenerated
Evaluated
ReachedTopK
Selected
Committed
Resolved
Fizzled
OutcomeValue
```

## 17.4 Capability coverage
Esempio:

```text
Barricade:
SelfCover           PASS
AllyProtection      PASS
ObjectiveControl    PASS
DenyEscape          PARTIAL
TrajectoryControl   UNTESTED
```

## 17.5 Combo
- opportunity;
- attempt;
- success;
- counter available;
- counter attempted;
- counter succeeded;
- value.

## 17.6 Reaction
- armed;
- opportunity generated;
- FIRE;
- HOLD;
- timeout;
- expired;
- prompts before commit;
- outcome.

**HOLD privati sono telemetry server/offline, non automaticamente public TurnLog.**

## 17.7 Map/Sector
- visit;
- end-turn occupancy;
- damage/KO;
- control contribution;
- transition use;
- door/bridge/tunnel interaction;
- environmental combo;
- strategic sector heatmap.

---

# 18. Competence gate prima del balance

Regola da inserire nella documentazione QA:

> **Un risultato Bot-v-Bot non è evidenza forte di balance finché il bot non è certificato sulle capability che determinano quel risultato.**

Esempio:

```text
Riva WR 42%

Bot competence:
Water setup       PASS
Displacement      PASS
Steam             FAIL
Noise             UNTESTED
```

Conclusione:

```text
NON nerfare/buffare automaticamente Riva.
Prima correggere o certificare la competence AI.
```

## 18.1 Bot Competence Certification

Per personaggio/capability:

```text
Movement
Basic attack
Dash
Defense
Reaction
Environment
Core combo
Map control
Information
Advanced counter
```

Stati possibili, secondo schema corrente:

```text
PASS
PARTIAL
FAIL
UNTESTED
```

Collegare ogni riga a Scenario/Test reali.

---

# 19. CI e scale di esecuzione

## Per commit / PR
- core automation;
- golden determinism;
- bot competence smoke;
- 16–32 seed mini batch;
- privacy canary.

## Nightly
- 256–512 paired seeds;
- side swap;
- mirror;
- replay divergence audit;
- performance regression.

## Milestone / deep investigation
- migliaia di match;
- matchup/composition matrix;
- personality population;
- map/sector analytics;
- ability/counter coverage.

Non far fallire la CI perché il win rate varia di 1%.

Hard fail per:
- crash;
- invalid intent;
- determinism mismatch;
- privacy leak;
- candidate generation collapse;
- competence scenario regression;
- schema/registry failure.

Balance anomalies = warning/report finché un gate esplicito non dice diversamente.

---

# 20. Roadmap: due assi da NON confondere

Il progetto possiede già una progressione tooling/harness proposta in un master precedente:

```text
Harness v0.1  Scenario -> Play -> TurnLog / Result
Harness v0.2  Scenario Registry + Assertions + Fast Mode
Harness v0.3  Skill Balance Lab
Harness v0.4  Tactical Map Editor
Harness v0.5  Headless / Batch / CI
Harness v0.6+ Balance Analytics / Network / Stress
```

**Non distruggere o rinumerare automaticamente questo asse.**

La richiesta attuale aggiunge un secondo asse:

```text
GAME RELEASE v0.1
GAME RELEASE v0.2
GAME RELEASE v0.3
...
```

che serve per vedere **quale livello di Bot AI è verificabile nel gioco a ciascun checkpoint**.

Claude deve collegare i due assi nel Feature Registry/Roadmap senza confonderli.

---

# 21. GAME RELEASE v0.1 — Playable Bot Baseline

## Obiettivo player-visible

Il vertical slice deve poter mostrare una partita 2v2/offline-vs-bot coerente con la roadmap corrente.

Baseline corrente da riverificare:

```text
Roster:
Flux
Riva
Bastion
Vektor
```

Il bot `v0.1` deve **saper giocare legalmente e deterministicamente**, non essere ancora un Expert.

## Capacità Bot visibili

Minimo:

- produce normali Intent;
- stesso validator umano;
- movimento valido su hex;
- raggiunge/contesta objective basilare;
- sceglie fra attacco/ability legalmente utili;
- evita collisioni/hazard ovvi;
- usa cover/posizione in modo basilare;
- non usa hidden enemy state;
- reaction policy base quando necessaria alla demo;
- decisione deterministica;
- TurnLog/debug spiegabile.

Da non richiedere come gate v0.1:
- opponent learning;
- multi-turn Strategic Director completo;
- deep enemy hypotheses;
- forward simulation;
- personality population;
- large-scale balance analytics.

## Scenario Harness visibile a v0.1

Deve permettere almeno:

```text
Scenario -> Play -> real Intent -> Commit -> Snapshot -> Resolver -> TurnLog -> Result
```

e:
- scenario scripted;
- basic bot scenario;
- golden repeat;
- visual replay/debug;
- run by Stable ScenarioId.

## Epic / checkpoint da consolidare

Non inventare numero se non esiste già.

**Candidate Epic title:**
`[EPIC v0.1] Bot Baseline & Unified Scenario Execution`

### Issue titles candidate v0.1

```text
[v0.1][Bot] Route bot decisions through normal FRTIntent validation
[v0.1][Bot] Add deterministic legal move/objective candidate selection
[v0.1][Bot] Add basic position cover and hazard utility
[v0.1][Bot] Add basic ability candidate scoring
[v0.1][Bot] Add base reaction policy required by vertical slice
[v0.1][Bot QA] Add hidden-state anti-omniscience canary
[v0.1][Bot QA] Add deterministic repeat and permutation tests
[v0.1][Scenario] Consolidate Stable ScenarioId execution path
[v0.1][Scenario] Run scripted intents through normal Planning/Commit pipeline
[v0.1][Scenario] Add visual scenario run and TurnLog/result output
[v0.1][Scenario] Add full 2v2 autonomous smoke scenario
[v0.1][Scenario] Add failing-run reproduction in L_DevSandbox
[v0.1][Release] Validate bot/scenario flow in packaged Development build
```

## v0.1 exit gate

```text
A 2v2 autonomous match can complete without direct state shortcuts.
Same config/seed repeats with same canonical hashes.
Hidden canonical changes not present in TeamKnowledge do not change bot choice.
A failed run can be reproduced visually.
```

---

# 22. GAME RELEASE v0.2 — Tactical Bot v1

## Epic reale da aggiornare

**#326 — `[EPIC v0.2] E26 · Tactical Bot v1`**

Non crearne una duplicata.

## Obiettivo player-visible

Il bot deve iniziare a **capire RefactorTactics**, non solo eseguire mosse valide.

## Capacità Bot

- Situation Analysis;
- Tactical Goals;
- candidate diversity;
- score breakdown;
- richer objective/offense/survival/position/control scoring;
- Team Planner;
- ally conflict detection;
- overkill/redundancy;
- capability-driven team synergy;
- water -> electric setup/payoff;
- temporal compatibility delle combo;
- human+bot coordination via ally intent;
- plan hysteresis / Ready leggibile;
- partial knowledge foundation;
- noise/Last Known usati solo tramite TeamKnowledge;
- Overwatch/Fast Reaction FIRE/HOLD senza future knowledge;
- basic threat/information map se dipendenze E13/E14 sono pronte.

## Harness checkpoint

Deve supportare almeno:
- Scenario Registry;
- assertions;
- Fast Mode;
- bot decision assertions;
- human+bot fixture;
- reaction fixtures;
- bot competence scenarios.

## Issue titles candidate per #326

```text
[v0.2][AI] Add Situation Analysis from TeamKnowledge
[v0.2][AI] Add Tactical Goal generation and stable candidate model
[v0.2][AI] Add candidate diversity buckets
[v0.2][AI] Add explainable utility score breakdown
[v0.2][AI] Add team Top-K combination planner
[v0.2][AI] Add ally hard/soft conflict evaluation
[v0.2][AI] Add overkill and redundant-control penalties
[v0.2][AI] Add capability-driven setup/payoff synergy
[v0.2][AI] Validate temporal compatibility using resolver/rules metadata
[v0.2][AI] Coordinate bots around human ally draft/commit intents
[v0.2][AI] Add bot plan hysteresis and Ready stability
[v0.2][AI Reaction] Add Overwatch FIRE/HOLD utility without future knowledge
[v0.2][AI Knowledge] Consume Last Known and acoustic TeamKnowledge safely
[v0.2][AI QA] Add water-electric team combo scenario
[v0.2][AI QA] Add human+bot coordination scenarios
[v0.2][AI QA] Add Overwatch HOLD-then-FIRE scenario
[v0.2][AI Debug] Add candidate/team-plan/score debug views
```

## v0.2 exit gate

```text
Bot team performs at least one non-hardcoded setup/payoff combo.
Bot resolves ally conflicts before commit.
Bot can coordinate with a human ally intent.
Bot reactions use current opportunity only.
All decisions remain deterministic and fair-knowledge constrained.
```

---

# 23. GAME RELEASE v0.3 — Expert Bot v2

## Epic reale da aggiornare

**#328 — `[EPIC v0.3] E28 · Expert Bot v2`**

## Obiettivo player-visible

Il bot deve **ragionare sull'incertezza e sulla strategia**, senza diventare onnisciente.

## Capacità Bot

### Belief / uncertainty
- known state separato da belief;
- plausible cells da graph/reachability;
- evidence acustica;
- decoy indistinguibile se non osservabile;
- knowledge decay;
- unaccounted threats;
- threat projection.

### Enemy scenarios
- abstract enemy hypotheses;
- pochi team scenarios coerenti;
- robust score;
- worst-case/risk weighting;
- no hypothesis -> knowledge self-confirmation.

### Strategic Director
- goal persistente;
- objective/score/turn awareness;
- strategic urgency;
- risk budget;
- Hold / Contest / Abandon;
- Regroup vs Disengage;
- strategic sector/map control;
- transition/choke value;
- map revision invalidation;
- multi-turn intent come persistenza di goal, NON deep search completo.

### Difficulty / Personality
- Easy / Normal / Hard / Expert search budget;
- near-optimal deterministic selection;
- Aggression / Risk / Objective / Coordination / Patience / InformationSeeking / Opportunism;
- personality come preferenza, non handicap;
- strategic context può overrideare preferenza quando serve per non perdere.

## Harness checkpoint

- skill/bot competence lab;
- enemy-belief fixtures;
- strategic map fixtures;
- difficulty/personality comparison;
- robust plan scenarios;
- no-omniscience regression corpus.

## Issue titles candidate per #328

```text
[v0.3][AI Belief] Add enemy belief state separated from TeamKnowledge
[v0.3][AI Belief] Generate plausible cells from graph reachability
[v0.3][AI Belief] Fuse acoustic evidence without revealing decoys
[v0.3][AI Belief] Add knowledge decay and unaccounted threats
[v0.3][AI] Add perceived threat projection
[v0.3][AI Prediction] Add deterministic enemy scenario generation
[v0.3][AI Prediction] Add robust team-plan scoring across scenarios
[v0.3][AI Strategy] Add multi-turn Strategic Director goal persistence
[v0.3][AI Strategy] Add score/turn-aware urgency and risk budget
[v0.3][AI Strategy] Add Hold Contest Abandon objective evaluation
[v0.3][AI Strategy] Add Regroup and Disengage strategic goals
[v0.3][AI Strategy] Add authored strategic sectors and perceived map control
[v0.3][AI Strategy] Evaluate doors bridges tunnels and choke transitions
[v0.3][AI Difficulty] Add deterministic search-budget difficulty profiles
[v0.3][AI Personality] Add personality utility modifiers
[v0.3][AI QA] Add difficulty/personality comparison fixtures
[v0.3][AI QA] Add belief/decoy/no-omniscience golden corpus
[v0.3][AI Debug] Add belief threat sector and strategic-goal overlays
```

## v0.3 exit gate

```text
Expert can choose a robust plan across multiple plausible enemy scenarios.
Hidden real enemy state still cannot influence the decision.
Strategic goals persist sensibly across turns and react to score/objective changes.
Map control is based on perceived knowledge, not canonical omniscience.
```

---

# 24. GAME RELEASE v0.4 — Unified Battle Simulation & Balance Lab

Questa versione è una **proposta di release checkpoint** se il roadmap reale non possiede già un nome/scopo v0.4 differente. Claude deve integrare, non sovrascrivere.

## Obiettivo

Trasformare Scenario Harness + Bot AI in strumento di QA/balance riproducibile.

## Player-visible

Non necessariamente una grossa feature utente. Il vantaggio è:
- bot più testati;
- balance meno regressivo;
- riproduzione rapida dei bug;
- maggior stabilità delle mappe/ability.

## Tooling

- headless Bot-v-Bot match runner;
- `ExperimentDefinition`;
- deterministic seed corpus;
- paired run;
- side swap;
- mirror match;
- run manifest;
- JSON/JSONL/CSV export;
- batch aggregation;
- ability pipeline metrics;
- combo/counter telemetry;
- objective/map/sector telemetry;
- determinism rerun;
- replay problematic seed in Editor.

## Candidate Epic

`[EPIC v0.4] Unified Battle Simulation & Balance Lab`

### Issue titles candidate

```text
[v0.4][Harness] Add common RunManifest for visual fast headless and batch execution
[v0.4][Harness] Add headless match execution through the existing Scenario Harness
[v0.4][Harness] Add Battle Experiment definition and run expansion
[v0.4][Harness] Add versioned match and bot seed corpora
[v0.4][Harness] Add paired seed and side-swap execution
[v0.4][Harness] Add mirror-match diagnostics
[v0.4][Telemetry] Export match unit and action telemetry
[v0.4][Telemetry] Add ability opportunity-to-resolution pipeline metrics
[v0.4][Telemetry] Add combo opportunity attempt success counter metrics
[v0.4][Telemetry] Add reaction FIRE HOLD timeout telemetry
[v0.4][Telemetry] Add strategic-sector and transition metrics
[v0.4][QA] Add automatic determinism rerun audit
[v0.4][QA] Add replay-from-RunId workflow in L_DevSandbox
[v0.4][Balance] Introduce versioned BalanceBaseline bot profile
[v0.4][Balance] Produce machine-readable experiment summary and anomalies
```

## v0.4 exit gate

```text
One command/config can run a deterministic batch of autonomous matches.
Every failed run is reproducible from its RunManifest.
Side swap and mirror diagnostics are available.
Balance data records bot/content/rules versions.
```

---

# 25. GAME RELEASE v0.5 — Opponent Adaptation & Production Battle Analytics

Proposta da riconciliare col roadmap reale.

## Bot capabilities

- observation-only Opponent Model;
- route preference;
- objective preference;
- aggression/retreat tendency;
- target preference;
- observed combo patterns;
- evidence confidence;
- decay;
- prediction mismatch;
- model reliability;
- exploit budget;
- robust fallback against unpredictable opponents;
- limited counter-planning.

Regola:

```text
Opponent model modifies hypothesis weights.
It never reveals hidden current intent/state.
```

## Battle Lab capabilities

- Bot Competence Certification;
- character capability coverage;
- counterplay rate;
- side-adjusted reports;
- matchup/composition matrix;
- population tests across personalities;
- sector/cell heatmap export;
- nightly paired regression;
- anomaly warnings.

## Candidate Epics

```text
[EPIC v0.5] Opponent Observation Model & Adaptive Expert
[EPIC v0.5] Bot Competence Certification & Battle Analytics
```

### Issue titles candidate

```text
[v0.5][AI Opponent] Add sanitized team observation stream for bot memory
[v0.5][AI Opponent] Add route preference evidence normalized by alternatives
[v0.5][AI Opponent] Add objective/aggression/retreat tendency evidence
[v0.5][AI Opponent] Add target and observed-combo tendencies
[v0.5][AI Opponent] Add deterministic evidence decay
[v0.5][AI Opponent] Add prediction-mismatch and model-reliability tracking
[v0.5][AI Opponent] Bound exploit modifiers with an ExploitBudget
[v0.5][AI QA] Add hidden HOLD reaction privacy regression
[v0.5][Balance] Add Bot Competence certification schema
[v0.5][Balance] Link competence capabilities to Scenario/Test evidence
[v0.5][Balance] Add side-adjusted composition matchup reporting
[v0.5][Balance] Add map/sector heatmap aggregation
[v0.5][Balance] Add counter-availability attempt success metrics
[v0.5][Balance] Add personality population tournaments
[v0.5][CI] Add nightly paired battle simulation suite
```

## v0.5 exit gate

```text
Opponent adaptation uses only observed evidence.
A player can intentionally mislead the model through legal gameplay.
Balance reports expose competence confidence before suggesting balance conclusions.
Nightly batch produces reproducible regression artifacts.
```

---

# 26. GAME RELEASE v0.6 — Forward Simulation & Production Hardening

Proposta da riconciliare con la release roadmap.

## Bot capabilities

- limited resolver-based forward simulation;
- shortlist first, simulate only best plans;
- multiple enemy scenarios;
- deterministic simulation budget;
- no deep minimax requirement;
- performance budgets;
- multi-match scheduling that cannot change decisions.

Pipeline:

```text
Many candidates
→ cheap heuristic
→ shortlist
→ few Team Plans × few Enemy Scenarios
→ SAME logical Resolver on synthetic copies
→ state evaluation
```

## Production QA

- packaged headless/dedicated soak;
- hundreds of simultaneous bot instances scheduling;
- replay divergence corpus;
- performance regression;
- privacy audit in bot matches;
- CI retention policy for artifacts;
- release checklist.

## Candidate Epic

`[EPIC v0.6] Resolver Forward Planning & Bot/Simulation Production Hardening`

### Issue titles candidate

```text
[v0.6][AI] Add synthetic logical snapshot for forward candidate simulation
[v0.6][AI] Reuse authoritative resolver for limited forward evaluation
[v0.6][AI] Add deterministic shortlist and forward-simulation budgets
[v0.6][AI] Add state-value evaluator for simulated outcomes
[v0.6][AI QA] Add forward-simulation determinism fixtures
[v0.6][Perf] Instrument planner forward-simulation cost
[v0.6][Perf] Add deterministic multi-match AI scheduling
[v0.6][Harness] Add packaged dedicated headless soak runner
[v0.6][Harness] Add replay-divergence audit corpus
[v0.6][Privacy] Add packaged bot knowledge/intent canary audit
[v0.6][CI] Add simulation artifact retention and failure replay metadata
[v0.6][Release] Add Bot/Scenario/BattleLab gates to release checklist
```

---

# 27. GAME RELEASE v0.7+ — Research / Meta / Learning Agents

Questa release non deve diventare blocker delle precedenti.

Possibili direzioni:

- Unreal Learning Agents evaluation;
- reinforcement/imitation learning offline;
- scorer tuning suggestions;
- exploit discovery;
- draft/meta simulation;
- roster selection AI;
- large-scale strategy research;
- behavior diversity research.

**No shipping dependency finché non viene presa una nuova decisione esplicita.**

Candidate Epic:

`[R&D] Learning Agents, Offline Training & Exploit Discovery`

Issue titles:

```text
[R&D][AI] Evaluate Unreal Learning Agents for offline tactical training
[R&D][AI] Evaluate imitation learning from deterministic bot/human logs
[R&D][AI] Prototype offline scorer-weight tuning
[R&D][Balance] Evaluate exploit discovery from large-scale self play
[R&D][AI] Compare learned policy recommendations against deterministic Utility AI
[R&D][Architecture] Document shipping and experimental-plugin boundary
```

---

# 28. Tabella checkpoint sintetica

| Game release | Cosa deve essere osservabile nei bot | Harness/Battle capability | Epic primaria |
|---|---|---|---|
| **v0.1** | Bot legale, deterministico, objective/attack/position base | Scenario -> real pipeline -> TurnLog/Result, visual repro | Bot Baseline / Unified Scenario Execution |
| **v0.2** | Utility tattica, Team Planner, combo, human+bot, Overwatch reasoning | Registry, assertions, Fast Mode, competence fixtures | **#326 E26 Tactical Bot v1** |
| **v0.3** | Belief, enemy scenarios, Strategic Director, map control, difficulty/personality | Skill/AI Lab, belief/strategy regressions | **#328 E28 Expert Bot v2** |
| **v0.4** | AI stabile per benchmark controllati | Headless/Batch, paired seeds, telemetry, BalanceBaseline | Unified Battle Simulation & Balance Lab |
| **v0.5** | Opponent model osservativo/adattivo | Competence certification, matchup/heatmap/nightly analytics | Adaptive Expert + Battle Analytics |
| **v0.6** | Limited forward simulation / production expert | packaged soak, replay audit, multi-match perf/privacy | Production Hardening |
| **v0.7+** | R&D | self-play / learning research | Learning Agents R&D |

---

# 29. Scenario Map da creare/consolidare

Non inventare ScenarioId definitivi: allocare ID seguendo il registry reale.

## v0.1 Baseline
- bot legal move/objective;
- cover vs exposure;
- hazard detour;
- ability choice;
- deterministic tie;
- hidden-state fairness;
- 2v2 autonomous smoke;
- full v0.1 showcase.

## v0.2 Tactical
- ally collision conflict;
- friendly fire conflict;
- overkill redundancy;
- water-electric setup/payoff;
- setup/payoff/deny;
- temporal compatibility;
- human draft coordination;
- human commit coordination;
- plan hysteresis;
- Overwatch HOLD -> FIRE;
- simultaneous reaction targets.

## v0.3 Expert
- acoustic area not exact;
- decoy indistinguishable;
- belief update;
- unaccounted threat;
- information gain;
- robust plan vs objective push;
- robust plan vs flank;
- hypothesis does not become knowledge;
- objective hold vs abandon;
- score-aware endgame risk;
- strategic sector/choke control.

## v0.4 Battle Lab
- mirror matchup;
- side swap paired seed;
- determinism rerun;
- batch failure reproduction;
- ability never generated diagnostic;
- combo/counter telemetry fixture.

## v0.5 Adaptation
- repeated-route learning;
- forced-route does not imply preference;
- evidence decay;
- surprise recovery;
- target preference with alternatives;
- hidden reaction HOLD privacy;
- misleading noise/pattern scenario.

## v0.6 Forward/Production
- forward shortlist determinism;
- forward scenario result consistency;
- packaged soak;
- dedicated replay audit;
- high-volume bot scheduling same decisions.

Ogni scenario deve collegare:
- Feature ID;
- release/milestone;
- Epic/Issue;
- owner spec/Wiki;
- automation status;
- visual status;
- packaged status;
- expected evidence.

---

# 30. Editor Map

Inserire **solo task realmente manuali**.

Esempi:
- verificare visualmente Bot debug overlay in `L_DevSandbox`;
- configurare/validare Scenario Selector se richiede Editor;
- playtest human+bot;
- verificare visuale belief/threat/strategic sector overlay;
- validare Data Asset Difficulty/Personality quando introdotti;
- verificare ghost intent dei bot come normali intent alleati;
- riprodurre run fallita e confrontare TurnLog/overlay;
- visual review delle heatmap se esiste una UI Editor.

NON Editor Tasks:
- scrivere C++;
- modificare YAML;
- creare issue;
- creare Automation Test;
- eseguire validator;
- generare report;
- export CSV/JSON.

---

# 31. Wiki / documentazione da consolidare

Non creare pagine duplicate se esistono owner equivalenti.

Copertura logica necessaria:

```text
Bot AI Overview
Bot Decision Pipeline
Bot Utility Scoring
Bot Team Planning
Bot Tactical Roles / Capabilities
Bot Knowledge / Anti-Cheat
Bot Belief / Threat / Information
Bot Reaction / Overwatch AI
Bot Strategic Director / Map Control
Bot Difficulty / Personality
Bot Opponent Model
Unified Scenario Harness
Battle Simulation / Balance Lab
Bot Competence Certification
Bot Debugging / QA
Bot Release Roadmap
```

Cross-link obbligatori:

```text
Bot Knowledge <-> Networking Privacy
Bot Knowledge <-> Rumore / Perception
Bot Reaction <-> Overwatch / Fast Reaction
Bot Planner <-> Map/Path/LOS/Targeting
Bot Planner <-> Ability definitions
Scenario Harness <-> Snapshot/Resolver/TurnLog
Battle Lab <-> Replay/Hash/Determinism
Battle Lab <-> Balance matrices / telemetry
Roadmap <-> Feature Registry <-> Scenario Map <-> Epic/Issue
```

---

# 32. PDR / owner spec da aggiornare

Verificare numerazione e owner reali.

Almeno integrare:

## PDR-03 Architecture
- Scenario Harness come adapter/orchestrator, non simulatore;
- bot come Intent producer;
- headless path privo di presentation dependency.

## PDR-04 Networking/Privacy
- fair BotKnowledge;
- bot server-side non implica accesso hidden;
- test canary.

## PDR-05 Deterministic Simulation
- Battle Simulation usa stesso resolver;
- RunManifest/version/seed/hash;
- forward simulation futuro riusa resolver.

## PDR-06 Map
- bot tactical/strategic queries sul grafo;
- revision invalidation;
- sector view non secondo pathfinding.

## PDR-07 Abilities
- capability metadata per bot;
- ability opportunity/candidate telemetry;
- competence coverage.

## PDR-08 UI
- bot ally intents come normali team intent;
- debug overlays Development-only;
- visual replay di run failure.

## PDR-09 Data/Validation
- stable Scenario/Experiment/BotConfig IDs/versioni;
- validator;
- manifest/hash provenance.

## PDR-10 Roadmap/QA
- game-version checkpoints sopra;
- Scenario Harness/Battle Lab gates;
- CI tiers;
- Definition of Done Bot.

## Demo / v0.1 owner
- exact v0.1 bot competence gate;
- eliminare/annotare conflitti storici di roster/timing.

## Nuova spec AI se owner mancante
Un precedente handoff suggeriva una PDR Bot dedicata. Verificare il prossimo ID libero e NON sovrascrivere PDR esistente.

---

# 33. Project Control Center

La source of truth nota:

```text
docs/roadmap/feature-registry.yaml
```

Viste generate:

```text
feature-registry.json
roadmap.shortlist.md
featuremap.shortlist.md
scenariomap.shortlist.md
milestonemap.shortlist.md
docs/wiki/feature-status.md
```

Regola:

```text
Feature Registry YAML = stato canonico
Generated views       = derivate
Project Control Center = UI
```

Aggiornare il grafo:

```text
Feature
  ↓ implemented by
Epic / Issue / Release checkpoint
  ↓ validated by
Scenario / Automation / Batch experiment
  ↓ may require
Editor Task
  ↓ documented by
Wiki / owner spec / ADR
```

## 33.1 Feature IDs

Preferire aggiornare:

```text
RT-FEAT-BOT-BASE
RT-FEAT-BOT-TACTICAL
```

e collegare capability/release ai checkpoint/issue.

Per lo Scenario Harness/Battle Lab:
- cercare feature owner esistente;
- riusarla se copre Scenarios/QA/Bots;
- creare un Feature ID nuovo **solo se la granularità corrente del registry lo richiede** e dopo verifica di #337/convenzioni.

---

# 34. GitHub Epic / Issue policy

Prima di creare:
1. search per titolo/contenuto;
2. controllare open/closed;
3. riaprire/aggiornare se corretto;
4. rispettare parent/child/label/milestone reali;
5. inserire Feature IDs reali;
6. inserire Scenario IDs reali dopo allocazione;
7. inserire backlink a Wiki/spec;
8. non inventare issue number nel testo prima della creazione.

Consolidare sicuramente:

```text
#326 v0.2 E26 Tactical Bot v1
#328 v0.3 E28 Expert Bot v2
```

Per v0.1/v0.4+ cercare Epic equivalenti prima di creare le candidate di questo documento.

---

# 35. Definition of Done — Unified Scenario Harness

Done solo se:

- [ ] esiste un solo path di scenario execution;
- [ ] scripted/bot/replay producono normali decisioni, non outcome;
- [ ] visual/fast/headless condividono gameplay logic;
- [ ] ScenarioId stabile;
- [ ] validator intercetta reference/schema invalidi;
- [ ] Planning/Commit/Snapshot/Resolver reali;
- [ ] Reaction Opportunity reale;
- [ ] assertions strutturate;
- [ ] result machine-readable;
- [ ] StateHash/LogHash disponibili dove supportati;
- [ ] failing run riproducibile visualmente;
- [ ] Automation Test;
- [ ] packaged/headless test quando previsto;
- [ ] docs/wiki/registry aggiornati.

---

# 36. Definition of Done — Bot feature

Una capability Bot è Done solo se:

1. produce Intent legale;
2. usa TeamKnowledge autorizzato;
3. è deterministica;
4. score/decisione è debug-friendly;
5. ha scenario dedicato;
6. ha Automation/Functional test;
7. privacy canary pertinente;
8. non duplica regole del resolver;
9. passa packaged gate della release;
10. è collegata a Feature/Epic/Issue/Wiki.

---

# 37. Definition of Done — Battle Simulation

Done solo se:

- [ ] usa Unified Scenario Harness;
- [ ] usa stesso resolver;
- [ ] RunManifest contiene versioni/hash/seed necessari;
- [ ] seed corpus versionato;
- [ ] side swap;
- [ ] mirror;
- [ ] paired experiments;
- [ ] telemetry con schema versionato;
- [ ] determinism audit;
- [ ] riproduzione da RunId;
- [ ] competence gate disponibile;
- [ ] output machine-readable;
- [ ] CI integration;
- [ ] performance misurata;
- [ ] nessuna telemetry privata viene accidentalmente resa client-public.

---

# 38. Test obbligatori trasversali

Almeno:

```text
Scenario StableId lookup
Scenario schema/reference validation
Visual vs Headless same logical result
Fast vs Headless same logical result
Repeat 1000 where appropriate
Permutation of unordered inputs
Same TeamKnowledge / different hidden canonical state => same bot decision
Decoy/non-decoy indistinguishable => same belief update
Hidden Overwatch HOLD not leaked
Bot intent passes same validator
Human+bot synergy fixture
Water-electric temporal compatibility fixture
Side-swap paired run
Mirror run
RunManifest replay
Batch failure visual reproduction
Content/Rules/BotConfig version recorded
```

---

# 39. Performance / instrumentation

Preservare i target di progetto correnti e misurare, non inventare hard gate senza profiling.

Metriche:

```text
Path query count/time/cache hit
Candidate count
TopK
Team plans evaluated
Enemy scenarios
Forward simulations
Bot decision CPU
Resolver CPU
Total logical match CPU
Batch throughput
Memory/run
Artifact size
```

Scheduling multi-match può distribuire lavoro nel tempo/threading ma **non deve cambiare candidate budget o esito**.

---

# 40. Commit plan suggerito per Claude

Adattare al workflow reale e separare docs/code.

```text
docs(ai): consolidate bot release capability roadmap
docs(test): define unified scenario and battle simulation contract
roadmap(ai): link v0.1-v0.6 bot checkpoints to feature registry
docs(wiki): consolidate scenario harness battle lab and bot QA
chore(roadmap): regenerate control-center derived views
chore(github): consolidate bot epics and add missing release checkpoints
```

Se implementazione runtime viene richiesta separatamente:

```text
refactor(scenario): unify decision providers behind existing scenario harness
feat(test): add reproducible run manifest and result schema
feat(sim): add headless battle experiment runner
test(sim): add paired seed side swap and determinism audit
```

---

# 41. Procedura operativa Claude

## Fase A — Audit
- repo/branch/HEAD/UE;
- bot as-built;
- scenario harness as-built;
- resolver/log;
- registry/schema;
- roadmap/release;
- Epic/Issue;
- Wiki/PDR;
- CI/tooling.

## Fase B — Conflict matrix
Cercare almeno:
- old roster vs Flux/Riva/Bastion/Vektor;
- old 5s interrupt vs 3s Fast Reaction;
- duplicated Scenario Harness;
- old square-grid/NavMesh bot;
- duplicated Feature Registry paths;
- harness version vs game release version;
- v0.2/v0.3 scope vs #326/#328.

## Fase C — Consolidation
- owner spec;
- Wiki;
- PDR cross-link;
- Feature Registry;
- roadmap;
- scenario map;
- editor map.

## Fase D — GitHub
- update #326;
- update #328;
- search/create v0.1 baseline epic only if missing;
- search/create v0.4+ epics only if the release roadmap supports them;
- create only missing child issues;
- write real numbers back to registry/docs.

## Fase E — Generated views / validation
- run real `feature_registry.py` commands;
- regenerate derived views;
- validate YAML/schema/IDs;
- link check;
- docs tests.

## Fase F — Report
Restituire:

```text
BATTLE/SIM/BOT CONSOLIDATION REPORT

Repo / HEAD / UE:
...

Current Scenario Harness found:
...

Current Bot implementation found:
...

Conflicts resolved:
...

Docs/PDR updated:
...

Wiki updated:
...

Feature Registry:
...

Game release roadmap:
v0.1 ...
v0.2 #326 ...
v0.3 #328 ...
v0.4 ...
...

Epics updated:
#...

Epics created:
#...

Issues updated:
#...

Issues created:
#...

Scenario IDs:
...

Editor tasks:
...

Generated views:
...

Validation:
PASS/FAIL

Deferred:
...

Next executable checkpoint:
...
```

---

# 42. Non fare

- non creare un secondo Scenario Harness;
- non creare un secondo simulator;
- non bypassare Planning/Commit;
- non far decidere outcome ai bot;
- non dare hidden state ai bot;
- non trasformare hypothesis in knowledge;
- non usare future reaction opportunities;
- non usare elapsed CPU time per difficulty;
- non hardcodare combo per character name;
- non usare win rate da solo come balance verdict;
- non creare nuovi Feature ID granulari senza necessità;
- non modificare generated JSON a mano;
- non inventare Issue/Scenario number;
- non rinumerare #326/#328;
- non trattare v0.4+ proposta come canonica se la release roadmap reale ha già altre decisioni;
- non rendere Learning Agents dipendenza shipping;
- non mettere telemetry privata/DecisionTrace nel client avversario.

---

# 43. Risultato desiderato

Dopo il consolidamento, il Project Control Center deve consentire di rispondere rapidamente a:

```text
Che cosa sa fare il bot nella v0.1?
Quale Epic porta al Tactical Bot v0.2?
Quale scenario dimostra la combo acqua/elettricità?
Quando entra il belief/noise reasoning?
Quando arriva l'Expert Bot?
Quando possiamo lanciare 500 match headless?
Quale versione introduce opponent adaptation?
Quale test prova che il bot non bara?
Quale issue blocca il prossimo checkpoint?
Quale run di CI posso riprodurre in L_DevSandbox?
Quanto è affidabile una conclusione di balance per un personaggio?
```

La risposta deve derivare dal grafo canonico:

```text
Game Release
   ↓
Feature
   ↓
Epic / Issue
   ↓
Scenario / Test / Battle Experiment
   ↓
Evidence
   ↓
Wiki / Owner Spec
```

Questo è il checkpoint finale del presente handoff.
