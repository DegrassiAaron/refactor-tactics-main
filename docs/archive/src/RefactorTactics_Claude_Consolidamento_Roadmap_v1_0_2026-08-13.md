> 🗄️ **ARCHIVIATO il 2026-08-13 — consumato.** Questo è un **sorgente**, non un owner: si legge per la
> provenienza, mai per la regola. Il contenuto recepito vive ora nelle fonti canoniche.
>
> **Cosa è entrato** — [D-136](../../decisions/RT_PDR_00_Decision_Log.md): `RELEASE_ORDER` arriva a
> `v1.0`, e le sei release nuove sono descritte in
> [`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) come **E40**–**E45**. Con esse: 19 feature
> post-v0.1 hanno preso l'epic che il loro owner già dichiarava, le due feature di rete sono uscite da
> `future` verso `v0.5` e `v0.7`, `RT-FEAT-ABILITY-RUNTIME` e `RT-FEAT-BOT-COMPETENCE` sono nate perché il
> registry non aveva **nessun** owner per GAS né per la competenza del bot. Le due domande che l'audit non
> chiude sono `REL-1` e `REL-2` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).
>
> **Cosa NON è entrato, e perché.** ⚠️ **La §12 chiedeva di riconciliare «feature v0.2 senza owner/epic
> chiara», e la diagnosi era sbagliata**: non era ownership mancante, era un **campo non scrivibile**.
> `known_roadmap_refs()` leggeva le epic dal solo `roadmap-v0.1.md`, quindi `epic: E38` su una feature v0.2
> era un *errore del validator* e le 30 `epic: null` erano una conseguenza meccanica. Otto feature lo
> dicevano già, dentro `out_of_release_scope`: *«l'epic esiste e si chiama E38 […] ma `roadmap.epic` resta
> `null`»*. Rimosso il vincolo, 19 assegnazioni sono state **derivate** e nessuna scelta.
>
> ⚠️ **`scenario-decision-provider` non è stato creato**: la §13 lo propone come candidato `P1` della v0.5,
> ma il seam è deciso da [D-101](../../decisions/RT_PDR_00_Decision_Log.md) e tracciato da
> [`#542`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/542), **milestone v0.2**. Portarlo
> in v0.5 avrebbe spostato indietro un lavoro assegnato e ne avrebbe creato una seconda copia.
>
> ⚠️ **Le §3–§9 descrivono come già vero ciò che chiedono di preservare, e infatti non c'era nulla da
> fare** — il che è il loro scopo, ma va detto perché la lettura frettolosa le farebbe eseguire:
> `Source/RefactorTactics/ScenarioHarness/` contiene già `RTScenarioIndex`, `RTScenarioLoader`,
> `RTScenarioRunner`, `RTScenarioSession`, `RTTestResult`, `RTTestReportWriter` e `RTTestScenario`;
> `RT-FEAT-TOOL-MAP-EDITOR` è **`INTEGRATED` in v0.1**, non «futura v0.4»; `RT-FEAT-TOOL-BALANCE-GROUND`
> è **v0.1 `IMPLEMENTING`**, non «Skill Balance Lab v0.3». Le §8 e §9 dichiarano superate due collocazioni
> che il repository aveva già superato per conto suo.
>
> ⚠️ **Il `DecisionProvider` della §4 non esiste ancora**, ed è l'unico punto dove il sorgente descrive
> come «emerso» qualcosa che è solo **deciso**: `RTTestScenario.h:196` lo nomina per dire *«cosa questo NON
> è»*, cioè come lavoro futuro. Resta di `#542`.
>
> **Le ~60 issue candidate delle §13–§18** sono state create **limitatamente ai `P0`**, per scelta esplicita
> dell'autore: i `P1`/`P2` restano elencati nel corpo delle epic, come già fa
> [`#704`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/704) con la sua tabella «Non ancora
> aperti». La §26 — *«non scrivere E40/E41 finché non stai realmente creando epic»* — è stata rispettata: il
> contatore è stato misurato su `main` (**E39** il massimo) prima di assegnare.

# REFACTORTACTICS — HANDOFF PER CLAUDE CODE
## Consolidamento roadmap, documentazione, Wiki e issue fino alla v1.0

**Data:** 2026-08-13  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Branch da verificare:** `main`

# 0. Obiettivo

Consolida lo stato corrente di RefactorTactics e porta la roadmap di prodotto fino alla **v1.0**, senza creare fonti parallele e senza riscrivere ciò che il repository ha già deciso.

Questa attività è di audit, riconciliazione, roadmap, documentazione, Wiki, Feature Registry, Scenario Map, Execution Map e pianificazione issue. Non implementare tutte le feature.

Regola principale:

> Misura prima `main`; poi modifica. Repository corrente + Decision Log/ADR + Feature Registry + issue live > handoff/chat storici.

# 1. Audit obbligatorio

Prima di toccare file, leggi almeno:

- `AGENTS.md`
- `CLAUDE.md`
- `README.md`
- `docs/README.md`
- `docs/product/piano-canonico-mvp.md`
- `docs/decisions/RT_PDR_00_Decision_Log.md`
- ADR correnti
- `docs/OPEN_DECISIONS.md`
- `docs/roadmap/feature-registry.yaml`
- `docs/roadmap/feature-registry.md`
- `docs/roadmap/roadmap-v0.1.md`
- `docs/roadmap/v0.1-definition-of-done.md`
- `docs/roadmap/roadmap-post-v0.1.md`
- `docs/roadmap/roadmap-checkpoint.md`
- `docs/roadmap/milestonemap.shortlist.md`
- `docs/roadmap/featuremap.shortlist.md`
- `docs/roadmap/execution-graph.yaml`
- `docs/technical/scenario-map.md`
- `docs/technical/test-automatico-unreal.md`
- `docs/technical/spec-turnlog.md`
- `docs/technical/test-manuali-pie.md`
- `docs/gameplay/spec-bot-tattico.md`
- `docs/technical/spec-hex-geometry-authoring.md`
- documenti balance correnti
- Wiki corrente
- issue GitHub open/closed relative a epic/checkpoint.

Verifica inoltre:

- versione Unreal realmente bloccata;
- HEAD di `main`;
- ultime PR merge su roadmap/registry/execution map;
- milestone GitHub;
- scenario corpus reale in `Scenarios/`;
- codice reale in `Source/RefactorTactics/ScenarioHarness/`;
- tooling reale in `Source/RefactorTacticsEditor/`.

Non fidarti di vecchi conteggi o stati se sono rimisurabili.

# 2. Baseline da preservare

Salvo decisioni più recenti trovate su `main`:

- UE **5.8.1**;
- core competitivo in C++;
- Blueprint per presentazione/configurazione/editor UX;
- griglia esagonale pointy-top;
- `FRTCellId { X, Y, Layer }`;
- costi, priorità e danni deterministici in interi;
- `Planning → Prep → Dash → Blast → Move → Cleanup`;
- Move volontario come ultima fase volontaria;
- snapshot/resolver/TurnLog deterministici;
- Stable ID e versioni esplicite;
- no GAS come autorità del resolver;
- client propone, autorità valida/applica;
- privacy intenti come invariante;
- nessuna simulazione parallela per test, bot, replay o balance.

Se qualcosa è stato superato, segnala il conflitto e applica la fonte canonica più recente.

# 3. Scenario Harness: non ricrearlo

Verifica `Source/RefactorTactics/ScenarioHarness/`.

Il sistema corrente include già o ha incluso recentemente concetti come:

- TestScenario;
- ScenarioLoader;
- ScenarioRunner;
- ScenarioSession;
- ScenarioIndex;
- TestResult;
- TestReportWriter;
- console/test commands.

Pipeline obbligatoria:

```text
Decision Provider
      ↓
Intent / Reaction response
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
      ↓
Assertions / Report / Playback
```

Nessun test deve precalcolare l'esito.

Vietate scorciatoie competitive tipo:

```cpp
SetActorLocation(...)
ApplyDamage(...)
SetHealth(...)
ForceOutcome(...)
```

se il gameplay reale passa dalla pipeline canonica.

# 4. DecisionProvider

Integra come direzione corrente il seam unico già emerso nelle decisioni/issue recenti:

```text
Scripted Scenario
Bot
Replay
Human
Test Reaction Policy
        ↓
DecisionProvider
        ↓
Intent / Response
```

Un provider:

- sceglie una decisione;
- NON restituisce un risultato;
- NON conosce la soluzione del resolver;
- NON bypassa il gameplay.

Visual/Fast/Headless cambiano solo playback, attese, rendering e output, mai il risultato logico.

# 5. Regola fondamentale dell'harness

Lo Scenario Harness **non deve essere più capace del gioco reale**.

Se una feature non ha un vero produttore gameplay o non è raggiungibile nel turno reale:

```text
scenario exists
requires capability
capability unavailable
→ BLOCKED
```

Non falso PASS.

Applicare questa disciplina a reaction planning, rotation dichiarata, target cell/edge, conditional reaction, teleport/spatial transfer e future systems.

# 6. ScenarioId e classificazione

Non reintrodurre `PrimaryCategory` se `main` ha già adottato il modello corrente:

```text
scenarioId = identità stabile dichiarata
tags       = faccette libere/incrociabili
path       = organizzazione fisica, non identità
redirects  = compatibilità di rinomina
```

Uno scenario può avere contemporaneamente tag tipo:

```text
movement
reaction
character
visual
regression
balance
showcase
stress
```

Non creare una seconda tassonomia.

# 7. TurnLog e assertion

TurnLog è struttura canonica per:

- replay;
- playback;
- debug;
- assertions;
- report;
- analisi Claude;
- determinismo;
- diagnosis.

Preserva assertion già implementate, incluse quelle sul log, ad esempio:

```text
LogEventCount
LogEventOrder
```

Non usare `expectedLogHash` letterale in ogni scenario.

Il determinismo si verifica rieseguendo:

```text
Run A
Run B
→ StateHash uguale
→ LogHash uguale
```

# 8. Map Editor — stato corrente

La vecchia idea “Map Editor = futura v0.4” è superata.

Verifica e preserva:

- `RT-FEAT-TOOL-MAP-EDITOR`
- `RT-FEAT-TOOL-MAP-GEOMETRY`
- execution lane/milestone editor corrente;
- owner tecnico `docs/technical/spec-hex-geometry-authoring.md`.

Verifica lo stato live di:

- occupancy 12 settori;
- grammatica geometrica quantizzata + validator;
- bake geometria → dati canonici;
- gesture authoring;
- ghost;
- snap;
- Undo/Redo.

Regola:

```text
Editor UX
   ↓ calls
Pure Runtime Rule
   ↓
Canonical Map Data
```

Il modulo Editor può possedere gesture/ghost/snap/viewport/transaction/UX, ma non una seconda authority su grammatica, cover, occupancy, path, LOS, bake o movement legality.

Preserva le decisioni correnti sul bake. In particolare verifica il modello corrente equivalente a:

```text
segmento aperto → border / FRTHexCover
```

e non inventare footprint chiusi o `Surface.Void` nella stessa pipeline se il Decision Log li ha esclusi.

# 9. Balance Ground

La vecchia milestone fissa “Skill Balance Lab v0.3” è superata.

Verifica `RT-FEAT-TOOL-BALANCE-GROUND` e le decisioni relative alla Battle Simulation.

Principio:

> Un dato bot-vs-bot NON è evidenza di bilanciamento finché non sappiamo che il bot sa usare la capability misurata.

Ogni capability del bot deve poter avere:

```text
PASS
PARTIAL
FAIL
UNTESTED
```

sostenuto da test/scenario/evidenza riproducibile.

Win-rate e metriche di balance informano il design ma NON diventano gate CI. Crash, divergence, leak e invalid state sì.

# 10. Roadmap già canonica

## v0.1 — Vertical Slice

Tema:

```text
Il turno simultaneo funziona e si vede.
```

Formato:

```text
2v2 offline vs bot
```

Chiude con i gate canonici della Definition of Done. Non aggiungere feature speculative.

## v0.2 — Struttura e finestre

Preserva, se confermate su `main`:

- E22 — Cover Window;
- E23 — muri, porte, interaction graph;
- E24 — Standard 3v3;
- E25 — Icon Language;
- E26 — Tactical Bot v1;
- E35 — roster 8;
- E36 — status framework;
- E38 — economy/movement compatibility/plan validation;
- E39 — spatial transfer.

Gate concettuale:

```text
3v3 Standard end-to-end
+ roster 8
+ map interaction
+ Cover Window
+ Tactical Bot
+ suite verde
+ replay deterministico
```

## v0.3 — Informazione

Preserva, se confermate:

- E27 — perception completa;
- E28 — Expert Bot;
- E29 — predictive avanzato / traps;
- E33 — Conditional Intent.

Invariante:

```text
belief ≠ knowledge
```

## v0.4 — Operations

Preserva, se confermate:

- E30 — Operations maps;
- E31 — multi-objective/logistics;
- E32 — 4v4 competitivo solo se il playtest lo giustifica;
- E34 — character state/configuration/transformation.

# 11. Estensione proposta v0.5 → v1.0

La repository potrebbe non avere ancora release `v0.5...v1.0` nello schema.

Quindi:

1. non dichiararle canoniche prima dell'audit;
2. prepara proposta e governance;
3. estendi schema/validator/registry solo quando accettate;
4. NON assegnare E40/E41/... prima di verificare il contatore condiviso su `main`.

Usa titoli/slug e lascia che GitHub assegni i numeri.

# 12. Governance prima di v0.5+

## `roadmap-release-schema-v05-v10` — P0

Estendere, se approvato:

- `RELEASE_ORDER`;
- Feature Registry schema;
- generatori;
- shortlist;
- Control Center;
- Execution Map;
- Wiki;
- validator;

da:

```text
v0.1
v0.2
v0.3
v0.4
future
```

a:

```text
v0.1
v0.2
v0.3
v0.4
v0.5
v0.6
v0.7
v0.8
v0.9
v1.0
future
```

con test di mutazione.

## `roadmap-v02-feature-ownership-reconcile` — P1

Riconciliare feature v0.2 senza owner/epic chiara, per esempio Supers, Auxiliary Units, Replay Archive, Ice Engine, faction system/scenarios e qualsiasi altro caso reale trovato.

Ogni feature deve avere owner, release, issue/epic o defer esplicito, test/scenario strategy.

## `roadmap-character-state-release-conflict` — P1

Risolvi eventuali conflitti fra feature CHARACTER-STATE / CHAR-TRANSFORMATION / E34 e release v0.2/v0.4/future. Non decidere a intuito.

# 13. v0.5 — Online Foundation

Tema: portare il gioco reale in rete senza matchmaking prematuro.

Issue candidate:

- `net-authoritative-match-lifecycle` — P0
- `net-canonical-intent-store` — P0
- `net-team-preview-relay` — P0
- `net-ready-commit-protocol` — P0
- `net-authoritative-resolution` — P0
- `net-intent-privacy-canary` — P0
- `net-plan-validation-reason-codes` — P1
- `net-preview-latency-budget` — P1
- `scenario-decision-provider` — P1
- `net-two-team-packaged-scenario` — P0

Regole chiave:

```text
preview ally-only = 8–12 Hz, unreliable, sequenced
ready/commit = reliable, idempotent
full intent = server-only
resolution = server authoritative
```

Gate:

```text
partita completa
+ state authority stabile
+ zero intent leak
```

# 14. v0.6 — Ability Runtime / GAS

GAS entra solo come supporto, mai come authority competitiva.

Issue candidate:

- `gas-architecture-boundary` — P0
- `gas-unit-asc-foundation` — P0
- `gas-stable-action-binding` — P0
- `gas-cost-resource-bridge` — P1
- `gas-cooldown-bridge` — P1
- `gas-status-effect-bridge` — P1
- `gas-resolver-event-application` — P0
- `gas-network-privacy-audit` — P0
- `gas-replay-determinism-regression` — P0
- `gas-roster-migration` — P1

Regola:

```text
Resolver decides
GAS applies/lifecycles/presents
```

Non:

```text
Montage/AbilityTask decides hit
```

# 15. v0.7 — Competitive Alpha

Tema: Dedicated Server + online loop reale.

Issue candidate:

- `server-dedicated-target` — P0
- `server-match-lifecycle` — P0
- `online-custom-lobby` — P0
- `online-party-team-assignment` — P1
- `online-matchmaking-baseline` — P1
- `online-reconnect-resync` — P0
- `online-latejoin-spectator-policy` — P1
- `server-replay-audit` — P1
- `server-telemetry-foundation` — P1
- `server-soak-3v3` — P0

Gate:

```text
launch client
→ lobby
→ join/create
→ play
→ disconnect
→ reconnect
→ finish
```

su dedicated server packaged.

# 16. v0.8 — Beta / Balance

Issue candidate:

- `batch-match-runner` — P0
- `bot-competence-schema` — P0
- `balance-ground-report` — P1
- `balance-metric-provenance` — P0
- `balance-no-winrate-ci-gate` — P0
- `performance-budget-suite` — P0
- `accessibility-pass` — P1
- `input-controller-parity` — P1
- `onboarding-training-scenarios` — P1
- `packaged-long-soak` — P0

Ogni report balance deve includere almeno:

```text
build
rules version
content hash
bot profile
competence status
seed policy
match/scenario format
```

# 17. v0.9 — Release Candidate

Feature freeze.

Issue candidate:

- `launch-content-freeze` — P0
- `ranked-ruleset` — P1
- `matchmaking-rating-v1` — P1
- `afk-disconnect-forfeit-policy` — P1
- `horizontal-progression-minimum` — P2, cuttable
- `data-only-modding-beta` — P2, cuttable
- `localization-and-settings-audit` — P1
- `security-abuse-hardening` — P0
- `save-replay-version-migration` — P0
- `release-candidate-soak` — P0

Progressione e modding non devono bloccare 1.0 se il core competitivo è pronto.

# 18. v1.0 — Launch

v1.0 è un gate di produzione, non una nuova feature release.

Issue candidate:

- `release-v10-master` — P0
- `production-dedicated-deployment` — P0
- `production-matchmaking-rollout` — P0
- `production-observability` — P0
- `launch-security-privacy-audit` — P0
- `launch-replay-audit` — P0
- `launch-performance-certification` — P0
- `launch-content-validation` — P0
- `launch-packaged-smoke-matrix` — P0
- `launch-rollback-plan` — P0

Gate finale:

> Una partita competitiva completa può essere trovata, giocata, risolta, spiegata, registrata e riprodotta su infrastruttura production senza replay divergence, intent leak o dipendenze dall'Editor.

# 19. Dipendenza macro

```text
v0.1 Vertical Slice
   ↓
v0.2 3v3 + roster 8 + struttura
   ↓
v0.3 Information / Prediction
   ↓
v0.4 Operations / Scale
   ↓
v0.5 Online Foundation
   ↓
v0.6 GAS / Ability Runtime
   ↓
v0.7 Dedicated Competitive Alpha
   ↓
v0.8 Beta / Batch / Balance
   ↓
v0.9 Release Candidate
   ↓
v1.0 Launch
```

Le release sono ordinate; le execution lane possono sovrapporsi quando le dipendenze reali sono soddisfatte.

# 20. Priorità

Usa:

```text
P0 = senza questo la release mente
P1 = serve al prodotto della release
P2 = importante ma tagliabile
P3 = esperimento / espansione
```

# 21. Se lo scope esplode

NON tagliare:

- determinismo;
- TurnLog/replay;
- privacy;
- server authority;
- Plan Validation;
- 3v3 giocabile;
- UI leggibile;
- packaged testing;
- dedicated server prima del multiplayer pubblico;
- reconnect;
- content validation;
- performance measurement.

Tagliabile/posticipabile:

- 4v4 come formato principale;
- trasformazioni complesse;
- progressione;
- modding pubblico;
- ranked sofisticato;
- extra roster;
- extra maps;
- bot super-avanzato;
- analytics decorative.

# 22. Stable ID e nomi personaggi

Non fare rename ciechi.

Prima verifica Stable Hero ID, display name, Wiki, scenario JSON, replay, cataloghi e Data Assets.

Uno Stable ID è identità persistente; il display name può cambiare senza migrare lo Stable ID.

Qualunque migrazione deve essere esplicita, versionata/redirect se serve, replay-safe e testata.

# 23. Documentazione

Aggiorna solo gli owner reali, probabilmente:

```text
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/execution-graph.yaml
docs/roadmap/milestonemap.shortlist.md
docs/roadmap/featuremap.shortlist.md
docs/technical/scenario-map.md
docs/technical/test-automatico-unreal.md
docs/product/piano-canonico-mvp.md
docs/OPEN_DECISIONS.md
```

Non modificare a mano file generati. Usa gli script owner.

# 24. Wiki

Aggiorna o crea pagine equivalenti a:

- Roadmap;
- Stato del progetto;
- Come vengono testate le meccaniche;
- Scenario Testing;
- Map Authoring;
- Bot e difficoltà;
- Balance e playtest;
- Multiplayer roadmap;
- Release path.

Per ogni release mostra:

```text
Cosa cambia per il giocatore
Cosa dimostra
Cosa NON include
```

# 25. Issue GitHub

Prima di creare issue:

1. cerca per keyword;
2. cerca per Feature ID;
3. cerca epic esistenti;
4. cerca closed equivalenti;
5. verifica milestone;
6. verifica OPEN_DECISIONS.

Non creare doppioni.

Template:

```text
Title
Owner documentale
Feature ID
Release
Priority
Why
Scope
Out of scope
Dependencies
Definition of Done
Automation
Scenario
TurnLog / Replay impact
Privacy impact
Packaged gate
```

# 26. Non assegnare numeri epic in anticipo

Non scrivere E40/E41/E42 finché non stai realmente creando epic contro `main`.

Usa `Proposed Epic — ...`, poi assegna E-n solo nel commit/merge che registra l'epic dopo aver verificato il contatore condiviso.

# 27. Scenari come specifiche eseguibili

Ogni feature rilevante dovrebbe arrivare a:

```text
unit test
→ integration test
→ scenario
→ packaged verification
```

Le fixture geometriche pure non sono scenari.

Uno scenario è match state + unità + intent + turno + resolver + risultato osservabile.

# 28. Map Editor + Harness

Target futuro:

```text
Edit canonical map
      ↓
Run scenario
      ↓
Observe TurnLog
      ↓
Validate result
```

Il Map Editor produce dati; lo Scenario Harness esercita il gioco; il Resolver decide.

# 29. Balance Ground + Harness

Target futuro:

```text
Scenario / Match Format
      ↓
Decision Providers
      ↓
Real Gameplay
      ↓
TurnLog + Metrics
      ↓
Competence-qualified report
```

Mai uno special balance simulator con regole diverse.

# 30. Output richiesto

Alla fine produci:

## A. Audit

```text
Area | Owner | Stato misurato | Commit | Issue | Conflitti
```

## B. Conflitti

```text
Fonte A | Fonte B | Perché divergono | Fonte vincente | Modifica
```

## C. Roadmap finale

Mostra v0.1 → v1.0 e distingue chiaramente:

- già canonico;
- consolidato in questa attività;
- ancora proposta.

## D. Issue matrix

```text
Issue | Existing/New | Priority | Owner | Feature ID | Dependencies | Gate
```

## E. Feature Registry changes

```text
FeatureId | Old release/status | New release/status | Reason
```

Non avanzare status senza gate misurabili.

## F. Scenario impact

```text
ScenarioId | Existing/Planned | Feature | Capability | Runnable/Blocked | Why
```

## G. Wiki changes

```text
Page | Created/Updated | Audience | Purpose
```

## H. Docs changed

```text
Path | Owner role | Why changed | Generated/manual
```

## I. Open decisions

Qualunque punto non decidibile va in `docs/OPEN_DECISIONS.md`.

## J. Validation

Esegui i comandi reali della repo, incluso se esistono ancora:

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate --check
python scripts/feature_registry.py shortlist --check
```

più link check, symbol/naming check, graph tests e consistency checks pertinenti.

Riporta risultati veri.

# 31. Commit proposal

Commit piccoli, per esempio:

```text
docs(roadmap): extend release model toward v1.0
docs(roadmap): reconcile post-v0.1 feature ownership
docs(testing): align scenario harness and decision provider roadmap
docs(tooling): align map authoring and balance ground roadmap
docs(wiki): publish player-facing roadmap through v1.0
chore(registry): regenerate roadmap and feature views
```

Non eseguire commit se non richiesto esplicitamente.

# 32. Definition of Success

Il repository deve dare una sola risposta verificabile a:

```text
Qual è la prossima release?
Quali feature ne fanno parte?
Quali issue la chiudono?
Cosa è già fatto?
Cosa è bloccato?
Quali scenari lo dimostrano?
Quale feature possiede il dato?
Quale documento possiede la regola?
Quando entra il multiplayer?
Quando entra GAS?
Quando diventa utile il Balance Ground?
Quando il Map Editor è release-critical?
Cosa manca davvero alla 1.0?
```

# 33. Regola finale

Non allineare copiando lo stesso stato in dieci posti.

Allinea:

```text
OWNER
   ↓
GENERATED VIEWS / REFERENCES
```

Esempi:

```text
Feature state            → feature-registry.yaml
Roadmap release          → roadmap owner
Execution dependencies   → execution-graph.yaml
Scenario truth           → scenario files + scenario-map
Decision                 → Decision Log / ADR
Open unresolved question → OPEN_DECISIONS.md
```

Le altre pagine referenziano. Non diventano nuove fonti.

La priorità è mantenere **una sola verità per concetto**.
