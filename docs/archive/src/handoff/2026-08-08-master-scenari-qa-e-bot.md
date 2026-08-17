> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked.
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md) §«cluster Scenarios / QA / Bots».
>
> **Il piu' assorbito dei master**: il sorgente del bot
> ([`2026-08-08-bot-ai-roadmap-e-test-pie.md`](2026-08-08-bot-ai-roadmap-e-test-pie.md)) porta in testa
> «✅ RECEPITO il 2026-08-08» — questo master ne e' il riassunto, arrivato dopo.
>
> ⚠️ **Non applicare** la `PrimaryCategory` obbligatoria di §5: l'identita' degli scenari ha **un asse solo**
> — `scenarioId` puntato piu' tag liberi, ID staccato dal percorso
> ([`scenario-index-e-tag.md`](../../../technical/scenario-index-e-tag.md), `#209`). Delle 11 `RT-FEAT-BOT-*`
> di §30 ne esistono due.

# RefactorTactics — Scenarios / QA / Bots Master Consolidation v0.1

**Data:** 2026-08-09  
**Scope:** Scenario Harness, Scenario Browser/BP_GameMode, Developer Testing Toolkit, Automation/Functional/Packaged tests, PIE, structured reports, Bot/AI, deterministic stress.  
**Stato:** master di consolidamento. La repository corrente e gli ADR reali prevalgono.

---

# 0. Principio

Tutti i producer di decisioni devono entrare nello stesso gameplay path:

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

Vietato creare scorciatoie del tipo:
- SetActorLocation diretto;
- ApplyDamage diretto;
- ability risolta fuori dal resolver;
- secondo simulatore solo per test/bot.

# 1. Developer / Testing Toolkit

Roadmap consolidata:

```text
v0.1  Scenario -> Play -> TurnLog / Result
v0.2  Scenario Registry + Assertions + Fast Mode
v0.3  Skill Balance Lab
v0.4  Tactical Map Editor
v0.5  Headless / Batch / CI
v0.6+ Balance Analytics / Network / Stress
```

Priorità assoluta: v0.1.

# 2. v0.1 — Scenario Play & Logging

Workflow target:

```text
1. apro L_DevSandbox/test map
2. scelgo Scenario
3. Play
4. nessun input manuale
5. scenario inizializza stato/unità
6. carica normali Intent
7. Ready/Commit automatici
8. Snapshot/Resolver reali
9. Reaction tramite test policy
10. playback visuale
11. TurnLog
12. output strutturato
13. stesso seed -> stessi hash
```

Deliverable:
- Scenario Definition testuale/versionabile;
- loader/validation;
- orchestrazione;
- auto planning;
- auto Ready/Commit;
- visual playback;
- reaction policies;
- TurnLog debug;
- structured output;
- golden scenario corpus.

Lo scenario non duplica i numeri delle ability: referenzia Stable ID reali.

# 3. Scenario authoring

Preferire il formato già usato dal repository.

Requisiti minimi:
- ScenarioId stabile;
- SchemaVersion;
- Map/fixture;
- unit/team setup;
- scripted intents o agent policy;
- reaction test policy;
- expected assertions quando supportate;
- metadata.

Errori invalidanti:
- CharacterId sconosciuto;
- AbilityId sconosciuto;
- CellId invalido;
- UnitId duplicato;
- Team invalido;
- target invalido;
- SchemaVersion non supportata;
- ReactionPolicy invalida.

# 4. Scenario Browser / BP_GameMode

`BP_GameMode` non è il database degli scenari.

Deve contenere solo configurazione run, ad esempio:

```text
Enable Scenario
Category
ScenarioId
ExecutionMode
AutoRun
AutoReady
SeedOverride
RepeatCount
ShowDebug
StopOnFailure
```

I dati veri vivono nel Registry/Scenario Definition.

Flusso:

```text
PrimaryCategory
      ↓
ScenarioId filtrato
      ↓
Scenario Registry
      ↓
Scenario Definition
      ↓
Scenario Harness
      ↓
Real Gameplay Pipeline
```

La categoria serve a trovare; lo ScenarioId identifica; il resolver gioca.

# 5. Categorie primarie approvate

Una sola `PrimaryCategory` per scenario:

```text
Debug
Core Systems
Actions
Reactions
Characters
Factions
Team Combos
Environment
Map Interactions
Perception
Objectives
Bots / AI
Networking & Privacy
UI / Presentation
Regression
Performance / Stress
Showcase / Demo
```

Milestone/versione NON è una categoria.

Classificazione secondaria tramite metadata:
- CharacterIds;
- FactionIds;
- FeatureTags;
- PurposeTags;
- MilestoneId;
- AutomatedTest;
- ExpectedTurnCount.

Uno scenario non va duplicato per apparire in più viste.

# 6. Execution Mode

Separato dalla categoria:

```text
Visual
Fast
Headless
```

Lo stesso ScenarioId deve poter girare in più modalità senza cambiare il risultato logico.

# 7. Scenario Registry

Responsabilità minime:
- resolve ScenarioId;
- list by PrimaryCategory.

Future query:
- character;
- faction;
- milestone;
- feature;
- purpose.

Stable ID, non array index:

```text
ScenarioId -> Registry -> Definition
```

# 8. Scenario levels

```text
LEVEL 1 — Micro
Movement / Ability / Surface / Reaction

LEVEL 2 — Interaction
Water+Electric
Open->Fire->Seal
Overwatch Hold->Fire

LEVEL 3 — Character
Gadget kit
Phase kit
Riktor kit
Wraith kit

LEVEL 4 — Team
Gadget+Phase
Riktor+Wraith
Faction/team coordination

LEVEL 5 — Showcase
Full vertical slice

LEVEL 6 — Stress
2v2 autonomous
4v4 stress
network/privacy/performance
```

# 9. Scenario Map

Ogni scenario deve essere collegabile bidirezionalmente:

```text
Feature <-> Scenario
Issue   <-> Scenario
Scenario <-> Test
Wiki    <-> Scenario
Roadmap <-> Scenario
```

Niente scenari senza owner feature.

# 10. Assertions — v0.2

Tipi iniziali:
- UnitAtCell;
- UnitHpEquals;
- UnitHasStatus;
- SurfaceHasStatus;
- CoverExists;
- AbilityResolved;
- AbilityFizzled;
- EventExists;
- EventCount;
- StateHashEquals;
- LogHashEquals.

Failure:

```text
Expected
Actual
Turn
Phase
MicroStep
ReasonCode
Stable IDs
```

# 11. Structured output

Output indicativo:

```text
Saved/RTTests/<ScenarioId>/<RunId>/
  result.json
  turnlog.jsonl
```

`result.json` minimo:

```text
schemaVersion
scenarioId
result
seed
rulesVersion
stateHash
logHash
executionMode
```

# 12. Golden corpus v0.1

Corpus piccolo, non tutto insieme.

Categorie minime:
- movement;
- ability;
- environment;
- reaction;
- integration/showcase.

Esempi da riconciliare col registry esistente:

```text
Debug.Empty
Movement.Basic
Movement.Collision
Movement.Blocked
Character.Hero.Gadget.Basic
Character.Hero.Phase.Water
Character.Hero.Riktor.Cover
Character.Hero.Wraith.Intercept
Environment.WaterElectric.Basic
Reaction.Overwatch.HoldThenFire
Map.DynamicCover.OpenFireSeal
Showcase.V01.Core
```

Non rinominare scenari reali solo per uniformità estetica.

# 13. Testing pyramid

```text
Core Automation
-> Feature Tests
-> Scenario Harness
-> Functional / PIE
-> Network tests
-> Packaged tests
-> Headless / CI
-> Stress / soak
```

# 14. Determinismo

Scenario fixture deve registrare almeno:

```text
RulesVersion
ContentManifestHash
ResolverConfigHash
Seed
InitialStateHash
FinalStateHash
LogHash
```

Progressione:

```text
Repeat 10
Repeat 100
Repeat 1000
Permutation
Visual vs Fast equivalence
Fast vs Headless equivalence
```

# 15. Reaction test policies

Il test risponde alle vere `ReactionOpportunity`.

Policy iniziali:

```text
CommitFirstValid
Hold
Timeout
HoldFirstThenCommit
TargetHighestPriority
TargetSpecificUnit
```

Mai bypassare la Decision Window con codice test-only.

# 16. As-built Scenario Harness

Handoff recenti indicano che il progetto usa:

```text
CVar + GameMode + same runner
```

e non richiede un `ARTTestDirector` obbligatorio.

Regola:
- verificare il codice reale;
- non reintrodurre un Actor test solo perché compare in documenti storici;
- un eventuale director è orchestratore, mai gameplay authority.

# 17. Bot — ruolo

Il bot è un producer di normali Intent.

```text
Team Knowledge
    ↓
Bot Planner
    ↓
Intent
    ↓
Planning / Commit / Snapshot / Resolver
```

Il bot non:
- legge CanonicalIntentStore nemico;
- legge future enemy path;
- legge posizioni reali nascoste;
- usa SetActorLocation;
- usa ApplyDamage;
- usa un secondo resolver.

Difficoltà più alta = ragionamento migliore, non più informazione.

# 18. Bot architecture

Strati:

```text
Knowledge
Belief
Tactical Maps
Candidate Generation
Utility
Team Coordination
Prediction
Reaction Policy
```

v0.1 implementa solo il necessario.

Possibile current entry point:

```text
URTHexBotLibrary
```

o equivalente reale.

Non creare una seconda AI se esiste già.

# 19. Bot v0.1

Deve:
- usare hex graph;
- produrre Intent legali;
- muoversi verso objective;
- scegliere celle utili;
- considerare cover/hazard;
- attaccare target legali;
- usare ability v0.1;
- usare Dash;
- usare Overwatch se green;
- rispondere a Fast Reaction;
- considerare HP/costi/cooldown;
- evitare friendly fire ovvio;
- evitare conflitti alleati;
- coordinazione 2v2 basilare;
- decision trace;
- determinismo.

Non serve ancora:
- Monte Carlo;
- RL;
- neural network;
- look-ahead profondo;
- opponent model completo.

# 20. Candidate generation

Generare solo candidati legalmente validi usando i sistemi reali:
- path/query;
- LOS/targeting;
- ability legality;
- GraphRevision;
- occupancy;
- Team Knowledge.

# 21. Utility scoring

Formula concettuale:

```text
Utility =
 Objective
+ Damage
+ Kill
+ Control
+ Position
+ Cover
+ Survival
+ AllySynergy
+ Environment
+ Information
- Threat
- KnownReactionRisk
- FriendlyFire
- AllyConflict
- ResourceCost
- Uncertainty
```

Interi, data-driven.

I range proposti nei brief sono baseline di tuning, non canone.

# 22. Character bot profile

Un solo algoritmo, pesi diversi.

Non:

```text
if Hero == Gadget
```

Preferire:

```text
BotProfile
CharacterDefinition
Ability tags
-> weights
```

# 23. Team coordination

v0.1:
- avoid ally collision;
- avoid friendly fire;
- setup/payoff semplice;
- bonus se sfrutta setup alleato;
- penalità incompatibilità.

Future:

```text
top candidates per unit
-> prune
-> synergy/conflict
-> Top-K team plans
-> team utility
-> plan
```

# 24. Reaction policy AI

Il bot riceve la stessa sanitized ReactionOpportunity.

Overwatch:

```text
FIRE / HOLD
```

Non conosce trigger futuri.

`ReactionPatience` modifica la soglia di FIRE, non la knowledge.

# 25. Bot determinism

```text
same state
+ same TeamKnowledge
+ same bot profile
+ same content/rules
+ same seed
=
same plan
```

Tie-break stabile:

```text
Utility desc
ActionPriority
AbilityId
TargetUnitId
TargetCell canonical order
CandidateStableId
```

# 26. Bot explainability

Decision trace separato dal TurnLog canonico:

```text
Unit
SelectedCandidate
Score
ScoreComponents
TopRejected
KnowledgeRevision
BotProfileId/Version
DecisionSeed
```

Trace server-only/debug durante planning.

# 27. Bot roadmap

```text
BOT v0.1
"sa giocare"

TACTICAL BOT v1
TeamKnowledge
last known/noise
threat/opportunity maps
environment/team coordination
belief weights
4v4 stress

EXPERT BOT v2
counterfactual
opponent model
multi-hypothesis
advanced profiles
Coach/QA
```

Expert non blocca v0.1.

# 28. Bot test scenarios

Core:

```text
AI.Basic.ObjectiveNoContact
AI.Basic.CoverVsExposure
AI.Basic.HazardDetour
AI.Basic.FriendlyFire
AI.Basic.AllyConflict
```

Reaction:

```text
AI.Reaction.OverwatchHoldThenFire
AI.Reaction.SimultaneousTargets
```

Fairness:

```text
AI.Knowledge.HiddenEnemyFairness
AI.Knowledge.AcousticAreaNotExact
```

Team:

```text
AI.Team.WaterElectric
AI.Team.DoorChoke
```

Stress:

```text
AI.Match.2v2Autonomous
AI.Stress.4v4
```

# 29. PIE registry

PIE manuale non sostituisce l'automation.

Ogni test manuale deve avere ID, prerequisiti, scenario, steps, expected e status.

# 30. Feature Registry status recuperato

Dalle fonti correnti:

```text
RT-FEAT-BOT-BASE                  IMPLEMENTED_PARTIAL
RT-FEAT-BOT-LEGAL-CANDIDATES      PARTIAL/FUTURE
RT-FEAT-BOT-SCORING               PARTIAL
RT-FEAT-BOT-REACTIONS             DESIGNED
RT-FEAT-BOT-TEAMKNOWLEDGE         SPECIFIED
RT-FEAT-BOT-TEAM-COORDINATION     DESIGNED
RT-FEAT-BOT-PERCEPTION            DESIGNED
RT-FEAT-BOT-BELIEF                FUTURE
RT-FEAT-BOT-COUNTERFACTUAL        FUTURE
RT-FEAT-BOT-DIFFICULTY            SPECIFIED
RT-FEAT-BOT-STRESS                DESIGNED

RT-FEAT-TEST-SCENARIO-HARNESS     IMPLEMENTED_PARTIAL
RT-FEAT-UI-SCENARIO-BROWSER       PARTIAL
RT-FEAT-UI-SCENARIO-CATEGORIES    DESIGNED
```

Non promuovere a DONE senza gate.

# 31. Definition of Done

Feature Done solo se applicabile:

```text
spec
data
runtime
log/debug
automation
scenario
ui/wiki
packaged
network/privacy
```

# 32. Wiki

Pagine pratiche:

```text
Developer Testing Toolkit
Running a Scenario
Scenario Categories
Scenario Authoring
TurnLog and Test Reports
Golden Scenarios v0.1
Bot / AI
Bot Testing
Skill Balance Workflow
Map Editing Workflow
Automated Testing Roadmap
```

# 33. Cleanup documentale

Candidate storico/archive:
- `spec-bot-utility.md` square-grid storico;
- vecchi tutorial AI NavMesh/Behavior Tree se descritti come gameplay authority;
- vecchi TestDirector spec se as-built CVar+GameMode+runner;
- roadmap AI isolata;
- scenario list flat non categorizzata.

# 34. Chat cleanup

Dopo integrazione canonica:

Candidate ad Archive/Delete:

```text
Developer Toolkit
Modifica BP Game Mode
Bot e intelligenza artificiale
```

# 35. Epic suggerite

## Developer / Testing Toolkit
- Scenario Definition
- Loader
- Harness
- Auto Planning/Commit
- Reaction test policy
- TurnLog/report
- golden corpus

## Scenario Registry & Browser
- PrimaryCategory
- Stable ScenarioId
- metadata
- BP_GameMode filter
- read-only info
- assertions
- Fast mode

## AI Foundation
- current hex entry point
- candidates
- utility
- profile
- deterministic tie-break
- trace

## Fair Knowledge AI
- TeamKnowledge input
- no-hidden-state
- last known/noise when green
- privacy tests

## Tactical Team AI
- synergy
- conflict pruning
- Overwatch/door/environment
- 2v2 team scoring

## Headless / CI
- Visual/Fast/Headless equivalence
- batch
- repeat
- report aggregation
- CI

# 36. Exit criteria

Il cluster Scenarios/QA/Bots è consolidato quando:

1. esiste una sola architettura Scenario Harness;
2. BP_GameMode configura ma non contiene gli scenari;
3. Stable ScenarioId è l'identità runtime;
4. PrimaryCategory è solo navigazione;
5. Visual/Fast/Headless condividono il simulatore;
6. Reaction test policy usa vere opportunities;
7. scenario->feature->test->roadmap è tracciabile;
8. bot produce normali Intent;
9. bot non legge hidden state;
10. bot deterministico;
11. scenario e bot status nel Feature Registry sono aggiornati;
12. vecchi TestDirector/bot square docs sono historical;
13. chat Developer Toolkit/BP_GameMode/Bot possono uscire dal CORE.
