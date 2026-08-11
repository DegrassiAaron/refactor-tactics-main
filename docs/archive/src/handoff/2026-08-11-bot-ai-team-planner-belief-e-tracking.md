> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO IL 2026-08-11
>
> **Materiale NON autorevole.** Il testo qui sotto è l'originale, invariato. Le sue affermazioni valgono
> per **provenienza**, non come regola: la regola vive negli owner che l'hanno recepito.
>
> **Recepito da**: [`../../../gameplay/spec-bot-tattico.md`](../../../gameplay/spec-bot-tattico.md) (owner
> nuovo) · [`../../../decisions/RT_PDR_00_Decision_Log.md`](../../../decisions/RT_PDR_00_Decision_Log.md)
> `D-095`–`D-099` · [`../../../roadmap/feature-registry.yaml`](../../../roadmap/feature-registry.yaml) ·
> referto di consolidamento
> [`../../../roadmap/plans/bot-ai-consolidamento-2026-08-11.md`](../../../roadmap/plans/bot-ai-consolidamento-2026-08-11.md).
>
> ⚠️ **Il §42 elenca undici `RT-FEAT-BOT-*` come «stato noto da verificare». Nove non esistono, e non sono
> mai esistiti.** Il registry ne aveva **due** — `RT-FEAT-BOT-BASE` e `RT-FEAT-BOT-TACTICAL` — e il documento
> stesso ordina di verificarlo, cosa che il referto ha fatto. Gli status citati (`IMPLEMENTED_PARTIAL`,
> `SPECIFIED`, `DESIGNED`, `FUTURE`) appartengono per metà a un vocabolario che il repository non usa.
>
> ⚠️ **Le sei Epic del §45 esistevano già, con altri nomi e altri numeri**: `E26` ([#326](https://github.com/DegrassiAaron/refactor-tactics-main/issues/326)),
> `E27` ([#327](https://github.com/DegrassiAaron/refactor-tactics-main/issues/327)), `E28` ([#328](https://github.com/DegrassiAaron/refactor-tactics-main/issues/328)),
> `E29` ([#329](https://github.com/DegrassiAaron/refactor-tactics-main/issues/329)). Non ne è stata creata
> nessuna nuova: sono state estese.
>
> ⚠️ **Metà delle sue «decisioni da consolidare» (§57) erano già decise** — l'invariante di difficoltà, il
> divieto di NavMesh come autorità, il determinismo senza `TMap` order, il divieto di ramo per eroe. Il
> referto §3 elenca quali, e con quale documento.
>
> ✅ **Ciò che era davvero nuovo, e che nessun documento del repository possedeva**: il ruolo dei framework AI
> di Unreal (§5 — StateTree, Behavior Tree, EQS, Learning Agents, Mass AI). Zero occorrenze in `docs/` e
> `Source/` prima di questo handoff.

# RefactorTactics — Bot AI / Team Planner / Belief / Project Tracking
## Handoff operativo per Claude Code — consolidamento repository, Wiki, Project Control Center, Epic/Issue e milestone

**Data:** 2026-08-11  
**Progetto:** RefactorTactics  
**Scopo:** consolidare tutto il focus Bot/AI discusso finora, integrandolo nelle fonti canoniche di progetto e nel sistema di tracking: documentazione, Wiki, Feature Registry, Roadmap, Scenario Map, Editor Map, Project Control Center, Decision Log/ADR, Epic, Issue e milestone.

> Questo file è un handoff operativo. NON è una nuova source of truth. Claude deve ispezionare il repository reale, cercare owner spec e work item esistenti, aggiornare/consolidare invece di duplicare e usare gli ID/numerazioni realmente correnti.

---

# 0. Mandato operativo obbligatorio

Claude non deve limitarsi a produrre un report. Deve applicare il consolidamento nel repository.

Prima di modificare:

1. leggere `CLAUDE.md`, `AGENTS.md`, `README.md` e istruzioni locali;
2. verificare repository, branch, HEAD, versione/patch Unreal Engine bloccata e toolchain;
3. verificare il Decision Log/ADR corrente;
4. ispezionare il codice Bot/AI esistente, in particolare `URTHexBotLibrary` o equivalente reale;
5. ispezionare il sistema Scenario Harness reale;
6. ispezionare le fonti canoniche del Project Control Center;
7. cercare documentazione/Wiki AI già esistente;
8. cercare Epic/Issue esistenti prima di crearne di nuove;
9. aggiornare le fonti canoniche;
10. rigenerare le viste derivate;
11. eseguire validator/test/lint disponibili;
12. produrre un report finale con file, Feature ID, Scenario ID, Epic/Issue, ADR e gap rimasti.

## Regola di prevalenza

```text
decisioni esplicite più recenti approvate nel progetto
>
Decision Log / ADR correnti
>
codice as-built verificato
>
documentazione canonica corrente
>
questo handoff
>
documentazione storica / PDR superati / brainstorming
```

Se una decisione di questo handoff confligge con un ADR corrente, NON sovrascrivere silenziosamente: riconciliare, aggiornare/supersedere l'ADR secondo il workflow reale del repository.

---

# 1. Fonti di tracking da integrare

Da un precedente handoff verificato, il repository era:

```text
DegrassiAaron/refactor-tactics-main
```

branch canonico:

```text
main
```

Claude DEVE riverificare questi dati prima di operare.

La source of truth nota per le feature è:

```text
docs/roadmap/feature-registry.yaml
```

con tooling noto:

```text
scripts/feature_registry.py
```

Viste/artefatti derivati noti:

```text
docs/roadmap/feature-registry.json
docs/roadmap/roadmap.shortlist.md
docs/roadmap/featuremap.shortlist.md
docs/roadmap/scenariomap.shortlist.md
docs/roadmap/milestonemap.shortlist.md
docs/wiki/feature-status.md
```

Documenti di tracking noti:

```text
docs/roadmap/feature-registry.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/technical/scenario-map.md
```

Regola:

```text
Feature Registry YAML = stato canonico
Generated views       = derivate
Project Control Center = UI del grafo, non nuovo database
```

Non mantenere lo stesso stato manualmente in cinque posti.

---

# 2. Grafo di progetto da mantenere coerente

Tutte le mappe operative devono restare viste dello stesso grafo:

```text
Feature
  ↓ implemented by
Epic / Issue / Roadmap / Milestone
  ↓ validated by
Scenario / Automation / Packaged Test
  ↓ may require
Editor Task manuale
  ↓ documented by
Wiki / Owner Spec / ADR
```

Relazioni inverse devono essere navigabili nel Project Control Center.

Il cluster Bot/AI deve quindi risultare collegato bidirezionalmente fra:

- Feature Registry;
- Roadmap;
- Epic/Issue;
- milestone;
- Scenario Map;
- Automation/Functional/Packaged tests;
- Editor Map solo per task veramente manuali;
- Wiki;
- owner spec tecnica/gameplay;
- Decision Log/ADR;
- eventuali dashboard/shortlist generate.

---

# 3. Decisione architetturale principale: il bot è un producer di Intent

Il bot non controlla direttamente il mondo.

Pipeline obbligatoria:

```text
Team Knowledge
      ↓
Bot Planner
      ↓
FRTIntent / comando normale
      ↓
Planning
      ↓
Validation / Commit
      ↓
Snapshot
      ↓
Authoritative Resolver
      ↓
TurnLog
```

Vietato:

```text
Bot -> SetActorLocation
Bot -> ApplyDamage
Bot -> modifica diretta MapState
Bot -> ability risolta fuori dal resolver
Bot -> secondo simulatore gameplay
Bot -> CanonicalIntentStore nemico
Bot -> future enemy path / target / destination
```

Bot e umano devono giocare lo stesso gioco.

---

# 4. Fair Knowledge: il bot non bara

Anche se gira server-side, il planner non deve leggere lo stato completo autorevole del nemico.

Architettura:

```text
AUTHORITATIVE MATCH STATE
          ↓
      PERCEPTION
          ↓
    TEAM KNOWLEDGE
       ↙       ↘
 Human UI     Bot AI
```

Non:

```text
Authoritative hidden state
          ↓
         Bot
```

Il bot può conoscere:

- geometria competitiva nota;
- regole e definizioni pubbliche dei personaggi/ability;
- stato pubblico;
- nemici visibili;
- Last Known autorizzato;
- rumori/acoustic contact autorizzati;
- hazard e cover conosciuti;
- objective state pubblico;
- intenti della propria squadra;
- reaction/ability nemiche solo se pubblicamente osservate o deducibili secondo TeamKnowledge.

Il bot NON può conoscere:

- posizione reale di unità nascoste;
- hidden enemy intent;
- hidden reaction armed state;
- future Reaction Opportunity;
- futuro path/destinazione/target;
- conoscenza interna del profilo AI di un bot avversario;
- che un evento sonoro è un decoy se la squadra non può saperlo.

Regola di difficoltà:

> difficoltà superiore = ricerca/scoring migliori, non accesso informativo migliore.

---

# 5. Tecnologie Unreal: ruolo approvato

Consolidare questa direzione senza trasformarla in dipendenze premature.

## 5.1 Utility AI custom — core competitivo

Scelta principale per:

- candidate generation;
- tactical scoring;
- team plan evaluation;
- reaction decision;
- deterministic ranking.

## 5.2 StateTree — orchestrazione opzionale

Può coordinare macro-stati:

```text
Planning
Assess
Generate
Coordinate
Commit
Observe Resolution
Fast Reaction
End Turn
```

Non deve essere l'autorità che calcola il miglior piano tattico competitivo.

## 5.3 Behavior Tree

Può restare utile per prototipi o comportamenti non competitivi/locali, ma NON è il planner centrale dei turni simultanei.

## 5.4 EQS

Può essere usato come laboratorio/prototipo per query tattiche, ma il runtime competitivo deve preferire query custom sulla mappa tattica discreta (`FRTCellId`, grafo, Layer, costi, hazard, GraphRevision) dove serve controllo deterministico.

## 5.5 A* / tactical graph

Il bot usa lo stesso path/query system reale del gioco. NavMesh/Recast non diventa autorità del movimento competitivo.

## 5.6 Learning Agents / ML

Solo ricerca futura/offline:

- training;
- bilanciamento;
- exploit discovery;
- tuning di scorer;
- bot sperimentali.

Non introdurre Learning Agents/RL/neural network come dipendenza del vertical slice o del bot competitivo corrente.

## 5.7 Mass AI

Fuori scope corrente.

---

# 6. Architettura logica Bot target

Strati da documentare:

```text
Knowledge
   ↓
Belief
   ↓
Situation Analysis
   ↓
Tactical Goals
   ↓
Candidate Generation
   ↓
Utility Evaluation
   ↓
Top-K / Candidate Diversity
   ↓
Team Coordination
   ↓
Enemy Scenarios / Prediction
   ↓
Robust Scoring
   ↓
Reaction Policy
   ↓
Intent
```

Non tutto viene implementato subito. Lo scopo è avere confini stabili e impedire un bot usa-e-getta.

---

# 7. Situation Analysis

Il planner estrae feature tattiche da `TeamKnowledge` / DTO equivalente:

- objective pressure;
- kill opportunity;
- own survival;
- ally vulnerability;
- combo opportunity;
- formation quality;
- information quality;
- available cover;
- known hazard;
- known choke;
- unaccounted enemy threats.

Non deve decidere ancora l'azione.

Output suggerito: dati puri/interi, debug-friendly.

---

# 8. Tactical Goals

Il bot genera pochi goal rilevanti, ad esempio:

```text
SecureObjective
KillEnemy
ProtectAlly
CreateCombo
BreakEnemyFormation
GainPosition
DenyArea
Recover
GatherInformation
Disengage
```

I goal non sono hard-coded per personaggio. Il personaggio e il BotProfile modificano il loro peso.

---

# 9. Action Skeleton e Candidate Generation

Evitare enumerazione combinatoria ingenua di:

```text
all paths × all cells × all ability × all targets × all facing × all reactions
```

Pipeline preferita:

```text
Tactical Goal
   ↓
Action Skeleton
   ↓
interesting cells / targets
   ↓
legal validation
   ↓
Candidate Plan
```

Action skeleton possibili:

```text
Move -> Attack
Move -> Ability
Ability -> Move
Move -> Overwatch
Defend -> Move
Move -> Wait
Interact -> Move
```

Generare solo candidati legalmente validi tramite sistemi gameplay reali:

- path/query;
- LOS;
- targeting;
- ability legality;
- cooldown/resource;
- GraphRevision;
- occupancy;
- TeamKnowledge.

---

# 10. Candidate Cell Pruning e Diversity

Da tutte le celle raggiungibili mantenere solo quelle tatticamente rilevanti.

Bucket concettuali:

```text
Objective
Offense
Cover
Control
Escape
Setup
Information
CurrentCell
```

Non prendere soltanto Top-N overall: potrebbe eliminare opzioni tattiche diverse.

Preferire Top-K per bucket/categoria.

Esempio di budget iniziale da rendere data-driven e profilabile, NON canonico:

```text
MaxRawReachableCells     = query reale
MaxInterestingCells      = ~8-16
MaxCandidatesPerUnit     = ~32-64
KeepTopCandidates        = ~4-8
```

Usare budget di conteggio, non elapsed CPU time, per preservare determinismo.

---

# 11. Utility AI — categorie di scoring

Un candidato illegale viene scartato prima dello scoring.

Categorie consolidate:

```text
Objective
Offense / Damage / Kill
Survival
Position
Control
TeamSynergy
Information
Risk / Threat
FriendlyFire
ResourceCost
Uncertainty
```

Scoring con interi/fixed-point.

Ogni candidato deve produrre un breakdown spiegabile, non solo `Score=827`.

Esempio:

```text
Objective       +120
Damage          +290
KillPotential   +210
Position         +80
TeamSynergy     +260
Exposure        -120
Hazard           -40
Uncertainty      -50
--------------------
TOTAL            750
```

I valori sono tuning data-driven, non hard-code di personaggi.

---

# 12. Damage / Kill / Overkill

Non usare solo danno grezzo.

Considerare:

- expected deterministic damage;
- kill opportunity;
- strategic target value;
- overkill;
- resource spent;
- follow-up value.

Team Planner deve penalizzare ridondanza:

```text
OverkillPenalty
RedundantControlPenalty
DuplicateSetupPenalty
```

---

# 13. Positional Value e Tactical Maps

Valore cella contestuale:

- cover dalle minacce note;
- attack LOS;
- objective distance/control;
- escape routes;
- controlled edges/chokes;
- hazard exposure;
- enemy reachability stimata;
- ally spacing;
- facing quality;
- environmental synergy;
- information gain.

Tactical maps previste come dati, non Actor:

```text
Threat Map
Opportunity Map
Information / Knowledge Map
Objective Map
```

Devono usare solo informazioni autorizzate.

---

# 14. Bot Profile, Character Profile e Difficulty

Separare:

```text
Base Bot Rules
Character Tactical Preferences
Bot Personality
Difficulty Search Profile
```

Non:

```cpp
if (Character == Flux) { ... }
```

Preferire:

```text
CharacterDefinition
Ability tags / capabilities
BotProfile
-> weights
```

Esempi di personalità:

```text
Aggressive
Methodical
Objective
Protective
Opportunist
Controller
```

Difficoltà:

```text
Easy   -> meno candidati / meno coordinazione / near-top choice
Normal -> utility completo
Hard   -> team planner + uncertainty
Expert -> più scenari + forward evaluation futura
```

Mai HP/danno bonus come default della difficoltà AI.

---

# 15. Determinismo Bot

Requisito forte:

```text
same TeamKnowledge snapshot
+ same bot profile/version
+ same content/rules
+ same bot seed
=
same Intent / same TeamPlanStableId
```

Vietato:

- `FMath::Rand()` globale;
- iteration order di `TMap/TSet` come tie-break;
- `think for 500 ms` come budget decisionale competitivo.

Usare budget fissi:

```text
MaxGoals
MaxSkeletons
MaxCandidateCells
MaxCandidates
TopK
MaxTeamPlans
MaxEnemyScenarios
```

Tie-break stabile con ID canonici.

---

# 16. Team Planner — decisione principale

Modello approvato:

```text
Unit A -> Top-K candidates
Unit B -> Top-K candidates
Unit C -> Top-K candidates
            ↓
      RTBotTeamPlanner
            ↓
synergy + conflicts + resources
            ↓
      Best Team Plan
```

Ogni unità mantiene preferenze proprie; il Team Planner sceglie la miglior combinazione.

Per 3 unità e 6 candidate ciascuna:

```text
6^3 = 216 team combinations
```

Brute-force deterministico è accettabile inizialmente; beam search solo quando necessario e con budget fisso.

---

# 17. Dynamic Tactical Roles

Il ruolo del personaggio non è il ruolo tattico del turno.

Ruoli dinamici concettuali:

```text
Initiator
Setup
Payoff
Finisher
Protector
Anchor
Controller
Denier
Flanker
Scout
ObjectiveRunner
Disengage
```

Esempio:

```text
Turn N:
Riva    -> Setup
Flux    -> Payoff
Bastion -> Denier

Turn N+1:
Riva    -> ObjectiveRunner
Flux    -> Controller
Bastion -> Protector
```

Non imporre un role assignment rigido prima della valutazione: deve emergere dal piano di squadra migliore.

---

# 18. Capability Tags e Job model

Evitare combo hard-coded per eroe.

Usare capability semantiche, idealmente Gameplay Tags governati o metadati equivalenti già previsti dal progetto.

Esempi:

```text
BotCapability.Setup.Surface.Water
BotCapability.Payoff.Surface.Water
BotCapability.Control.Displace
BotCapability.Control.BlockEdge
BotCapability.Defense.ProtectAlly
BotCapability.Information.Reveal
BotCapability.Objective.Capture
```

Una Team Opportunity può richiedere Job:

```text
SETUP
PAYOFF
DENY
PROTECT
CAPTURE
SCOUT
```

Le unità "bid" tramite il valore dei candidate plan, non tramite class-name branching.

---

# 19. Team Synergy

Team score concettuale:

```text
TeamScore =
  Σ IndividualScores
+ Synergies
+ OpportunityCompletion
+ FormationValue
- Conflicts
- SharedRisk
- Redundancy
```

Sinergie da riconoscere semanticamente:

```text
Water -> Electric
Push -> Hazard
Reveal -> Sniper/LongRange
Cover removal -> Line attack
Immobilize -> AoE
Smoke/Steam -> Infiltrator movement
Noise decoy -> Flank
Door close -> Overwatch / denial
Setup -> Payoff
```

---

# 20. Temporal Compatibility

Una combo intra-turno è valida soltanto se il resolver permette al setup di influenzare il payoff.

Il planner NON deve duplicare a mano l'ordine delle fasi.

Richiedere un contratto/query condiviso con Rules/Resolver metadata, ad esempio concettualmente:

```text
CanSetupAffectPayoff(ProducerAction, ConsumerAction, Ruleset)
```

La stessa informazione dovrebbe alimentare:

- bot planner;
- UI preview;
- validator;
- explainability.

Scenario/test obbligatorio per impedire "combo immaginarie" con ordine di resolution incompatibile.

---

# 21. Team Conflict

Distinguere:

## Hard conflict

- destinazione incompatibile;
- risorsa condivisa insufficiente;
- interazione mutually exclusive;
- intent illegalmente incompatibili.

=> scartare il Team Plan.

## Soft conflict

- crossing paths;
- crowding;
- stesso cover dependency;
- friendly fire risk;
- ridondanza;
- shared exposure.

=> penalità.

Riutilizzare quando possibile lo stesso conflict/warning service della UI ally planning.

---

# 22. Shared Resource e Marginal Utility

Se più unità competono per una risorsa condivisa, non assegnarla semplicemente per UnitId.

Usare valore marginale:

```text
MarginalUtility = ScoreWithResource - ScoreWithoutResource
```

La risorsa va all'uso che aumenta di più il valore del team plan, con StableId come tie-break finale.

Lo stesso concetto può assegnare Job dinamici.

---

# 23. Human + Bot Coordination

La squadra mista deve essere supportata.

Esempio:

```text
Human -> Flux
Bot   -> Riva
Bot   -> Bastion
```

L'intento umano è un vincolo/input del Team Planner.

Distinguere:

```text
Human Draft   -> Preview confidence
Human Commit  -> Locked/fixed plan
```

Il bot ottimizza intorno al piano umano, non lo sovrascrive.

Possibile UI:

```text
Riva intends Flood E8
Potential synergy: Electric
```

Non usare il bot per fare backseat gaming aggressivo.

---

# 24. Replanning, Hysteresis e Planning Lock

Con preview alleate 8-12 Hz, i bot possono ricalcolare quando cambia `TeamPlanningRevision`.

Evitare bot nervosi:

```text
switch plan only if
NewScore > CurrentScore + SwitchThreshold
```

Policy temporale suggerita da validare UX:

```text
inizio planning -> normal replanning
ultimi secondi -> higher switch threshold
quasi fine     -> replan only if current intent invalid
```

Non introdurre dipendenza del risultato competitivo da wall-clock; questa è policy di planning/presentation.

---

# 25. Ready dei bot

Il bot può calcolare velocemente, ma il comportamento visuale deve restare leggibile.

Target:

```text
valid stable plan
  ↓
show ally intent
  ↓
short grace / stability
  ↓
Ready
```

Se un ally human cambia piano in modo significativo:

```text
Unready -> recompute -> stable -> Ready
```

La semantica Ready/Commit resta quella reale del gioco.

---

# 26. Fast Reaction / Overwatch AI

Il bot riceve la stessa sanitized `ReactionOpportunity` del giocatore/team autorizzato.

Overwatch standard:

```text
FIRE
HOLD
```

Il bot NON conosce trigger futuri.

Valuta soltanto:

- strategic target value;
- expected damage/kill;
- objective threat;
- ally protection;
- reaction resource cost;
- known unaccounted threats;
- current opportunity cost;
- ReactionPatience profile.

`ReactionPatience` modifica la soglia FIRE/HOLD, non la conoscenza del futuro.

Caso target simultanei nello stesso micro-step: scegliere fra target validi usando utility + tie-break stabile.

---

# 27. Known State vs Belief State

Non mescolare conoscenza e ipotesi.

Known State:

```text
Enemy visible @ E7
HP/status pubblici
```

Belief State:

```text
last seen @ G9
possible cells / region
acoustic evidence
confidence
```

Una belief non diventa conoscenza soltanto perché il bot l'ha scelta come scenario più plausibile.

---

# 28. Belief Confidence

Per MVP evitare falsa precisione probabilistica.

Categorie suggerite:

```text
Confirmed
Strong
Plausible
Weak
```

Internamente possono avere pesi interi, ma NON presentarle come percentuali statistiche se non sono realmente calibrate.

---

# 29. Belief Cell Generation

Da `LastKnownCell` usare il grafo reale e il movimento noto:

```text
LastKnownCell
   ↓
Reachability / movement profile
   ↓
possible legal cells
   ↓
tactical representative cells
```

Considerare:

- muri;
- porte note;
- layer;
- terrain cost;
- movement capability nota;
- GraphRevision;
- pubblico stato di ponti/tunnel/transizioni.

Non mantenere tutte le celle: raggruppare/prunare per tactical destination.

---

# 30. Evidence Fusion: Vision + Noise + Reachability

Aggiornamento belief concettuale:

```text
Reachability
+ recent visual evidence
+ acoustic evidence
+ tactical plausibility
- impossible routes
= belief ranking
```

Preferire somme/interi facilmente debugabili prima di formule probabilistiche sofisticate.

Un rumore è evidence, non identità certa.

Se il team non sa che è un decoy, il bot non lo sa.

---

# 31. Knowledge Decay

L'incertezza cresce con:

- turni trascorsi;
- movement capability;
- branching della mappa;
- accesso a Dash/teleport noto;
- stealth;
- cambi GraphRevision.

Una posizione vecchia non deve restare precisa indefinitamente.

---

# 32. Threat Projection

La Threat Map futura deve combinare:

```text
visible threats
+ plausible enemy positions
+ possible attack envelopes
+ known hazards
```

Sempre da TeamKnowledge/Belief autorizzati.

Un nemico non localizzato può diventare `UnaccountedThreat`, senza inventarne la posizione reale.

---

# 33. Enemy Hypotheses / Team Scenarios

Non fare minimax esaustivo inizialmente.

Generare pochi scenari coerenti, ad esempio:

```text
Objective Push
Aggressive Focus
Defensive Hold
Flank
Disengage
```

Ogni scenario usa tactical role/capability note del nemico e posizioni plausibili, non hidden intent.

Non combinare ingenuamente tutte le hypothesis per tutte le unità.

---

# 34. Robust Scoring

Un nostro Team Plan deve essere valutato contro più scenari plausibili.

Concetto:

```text
WeightedScenarioScore
- WorstCasePenalty
= RobustScore
```

Personalità/difficoltà possono cambiare la tolleranza al worst case, non le informazioni disponibili.

Questo permette piani meno fragili senza conoscere il futuro.

---

# 35. Bait Detection e Unaccounted Threat

Il bot può dedurre rischio senza sapere il futuro.

Esempio Overwatch:

```text
Tank visibile entra
Striker nemico non localizzato
```

Il bot può HOLD perché il valore immediato del Tank è basso e resta una minaccia non contabilizzata.

Non può HOLD perché "sa" che lo Striker passerà dopo.

Questa differenza va testata esplicitamente.

---

# 36. Information Gain

Scouting/reveal devono avere valore anche senza sapere cosa verrà trovato.

InformationGain dipende da:

- quantità/importanza di area incerta ridotta;
- objective routes osservate;
- choke osservati;
- unaccounted threat area coperta;
- valore tattico dell'informazione.

Vietato usare contenuto hidden reale per valutare un reveal.

---

# 37. Opponent Profile — futuro

Un modello futuro può inferire da eventi osservati:

```text
Aggression
ObjectiveFocus
RiskTolerance
ReactionPatience
```

Non deve leggere il BotProfile reale dell'avversario anche se l'avversario è un bot server-side.

Questo è post vertical slice salvo thin slice esplicitamente approvata.

---

# 38. Forward Simulation / Counterfactual — futuro

Quando il resolver logico è abbastanza puro:

```text
Own Team Candidate
+ Enemy Scenario
      ↓
copy logical state
      ↓
Authoritative logical resolver / reusable deterministic core
      ↓
result state
      ↓
evaluate
```

Non spawnare Actor, non attivare montage, non usare GAS/presentation come autorità.

Non introdurre look-ahead profondo/Monte Carlo nella v0.1.

---

# 39. Multi-turn Strategy — futuro

Strato futuro sopra il Team Planner:

```text
Strategic Director
      ↓
Hold Relay / Control Bridge / Regroup / Hunt / Deny Tunnel
      ↓
per-turn Tactical Opportunities
      ↓
Dynamic Roles
      ↓
Team Planner
```

Non blocca il vertical slice.

---

# 40. Explainability e Bot Decision Trace

Creare/mantenere un trace server-side/dev-only separato dal TurnLog canonico quando contiene reasoning privato.

Contenuto utile:

```text
Turn
KnowledgeRevision
BotProfileId + Version
DecisionSeed
Goals considered
Candidate count
Top candidates
Selected candidate
Score breakdown
Team synergy/conflict
Enemy scenarios considered
Robust score
Reason for FIRE/HOLD
```

Non replicare il trace agli avversari durante Planning.

TurnLog registra gli eventi gameplay, non necessariamente il reasoning privato completo.

---

# 41. Debug tooling

Comandi/strumenti candidati, da riconciliare con naming reale:

```text
rt.Bot.Debug 1
rt.Bot.DumpCandidates <UnitId>
rt.Bot.DumpTeamPlan
rt.Bot.DrawScores 1
rt.Bot.ShowRoles 1
rt.Bot.ShowBelief <UnitId>
rt.Bot.ShowThreatProjection 1
rt.Bot.DumpEnemyScenarios
```

Overlay editor/dev:

- candidate cell score;
- score breakdown;
- selected Tactical Goal;
- dynamic role;
- team synergy/conflict;
- belief cells/confidence;
- projected threat;
- current TeamPlanStableId.

---

# 42. Feature Registry — stato noto da verificare

Un consolidamento precedente riportava:

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
```

Claude deve verificare il registry corrente.

Questa chat aumenta il livello di specifica di molte feature ma NON implica implementazione.

Non promuovere a DONE/IMPLEMENTED solo perché il design è ora più dettagliato.

---

# 43. Feature Registry — consolidamento raccomandato

Preferire l'estensione dei Feature ID esistenti anziché creare micro-feature inutili.

## 43.1 Mappare dentro feature esistenti

`RT-FEAT-BOT-BASE`
- bot produce normali Intent;
- deterministic fixed-budget planning;
- stable tie-break;
- decision trace base.

`RT-FEAT-BOT-LEGAL-CANDIDATES`
- goal/action skeleton;
- legal query;
- interesting-cell pruning;
- candidate diversity buckets.

`RT-FEAT-BOT-SCORING`
- utility breakdown;
- positional value;
- overkill/redundancy;
- data-driven profiles.

`RT-FEAT-BOT-TEAMKNOWLEDGE`
- sanitized TeamKnowledge DTO;
- human UI / bot same knowledge contract;
- hidden-state canary invariants.

`RT-FEAT-BOT-TEAM-COORDINATION`
- Top-K Team Planner;
- dynamic roles;
- capabilities/jobs;
- synergy;
- hard/soft conflict;
- temporal compatibility;
- shared resources/marginal utility;
- human+bot coordination;
- replanning/hysteresis.

`RT-FEAT-BOT-REACTIONS`
- sanitized ReactionOpportunity;
- FIRE/HOLD utility;
- simultaneous targets;
- ReactionPatience;
- bait/unaccounted-threat reasoning.

`RT-FEAT-BOT-PERCEPTION`
- Last Known;
- acoustic evidence;
- Knowledge decay;
- graph-aware perception/belief input.

`RT-FEAT-BOT-BELIEF`
- Known vs Belief;
- confidence levels;
- representative belief cells;
- evidence fusion;
- threat projection;
- information gain;
- unaccounted threats.

`RT-FEAT-BOT-COUNTERFACTUAL`
- enemy team scenarios;
- robust scoring;
- forward simulation;
- opponent model;
- multi-turn strategy, if scope remains grouped.

`RT-FEAT-BOT-DIFFICULTY`
- search depth/budget differences;
- near-optimal diversity;
- no knowledge/stat cheats.

`RT-FEAT-BOT-STRESS`
- 2v2 autonomous;
- 4v4 stress;
- balance telemetry;
- determinism/performance corpus.

## 43.2 Creare nuovi Feature ID solo se il registry richiede gate separati

Candidate nuove feature, NON crearle alla cieca:

```text
RT-FEAT-BOT-THREAT-PROJECTION
RT-FEAT-BOT-ENEMY-SCENARIOS
RT-FEAT-BOT-HUMAN-COORDINATION
RT-FEAT-BOT-DEBUG-TRACE
RT-FEAT-BOT-MULTITURN-STRATEGY
```

Prima verificare se è meglio rappresentarle come gate/acceptance criteria delle feature sopra.

---

# 44. Roadmap / milestone placement

Usare ID/numerazione reale del repository. NON inventare una nuova numerazione se esiste già.

Mappatura concettuale:

## Vertical slice / Bot v0.1

Scope:

- Intent producer reale;
- legal candidate generation;
- objective/cover/hazard/ability scoring;
- deterministic utility breakdown;
- basic conflict avoidance;
- basic setup/payoff synergy 2v2;
- Overwatch/Fast Reaction policy;
- DecisionTrace;
- fairness contract/test;
- autonomous 2v2 scenario.

## Tactical Bot v1 / milestone successiva

Scope:

- TeamKnowledge completo;
- dynamic Team Planner;
- human+bot planning;
- capability/job system;
- temporal synergy;
- threat/information maps;
- last-known/noise;
- belief model;
- enemy scenarios;
- robust scoring;
- 4v4 stress.

## Expert Bot v2 / future

Scope:

- counterfactual/forward simulation;
- opponent behavior inference;
- multi-turn Strategic Director;
- advanced profiles;
- Coach/QA reuse;
- Learning Agents experiments/offline research only if approved.

---

# 45. Epic da creare/consolidare

Cercare prima equivalenti esistenti. Se assenti, candidate Epic:

## EPIC — AI Foundation / Deterministic Intent Planner

Milestone: vertical slice / v0.1.

Copre:
- bot entry point;
- candidate generation;
- utility;
- deterministic budget/tie-break;
- profiles;
- trace.

## EPIC — Reaction Intelligence / Overwatch & Fast Reaction

Milestone: vertical slice / v0.1 se reaction è nello showcase.

Copre:
- ReactionOpportunity AI;
- FIRE/HOLD;
- simultaneous targets;
- patience;
- reaction tests.

## EPIC — Tactical Team AI / Coordination & Synergy

Milestone: basic slice in v0.1, full in Tactical Bot v1.

Copre:
- Top-K team plan;
- dynamic roles;
- capability/job model;
- synergy/conflict;
- temporal compatibility;
- shared resource/marginal utility;
- human+bot coordination.

## EPIC — Fair Knowledge AI / Perception, Belief & Threat

Milestone: fairness thin slice v0.1; full Tactical Bot v1.

Copre:
- TeamKnowledge-only input;
- hidden-state tests;
- Last Known;
- noise evidence;
- belief;
- threat projection;
- information gain.

## EPIC — Predictive AI / Enemy Scenarios & Robust Planning

Milestone: Tactical Bot v1 -> Expert Bot v2.

Copre:
- enemy hypotheses;
- team scenarios;
- robust score;
- bait/unaccounted threat;
- future counterfactual.

## EPIC — Bot QA / Debug / Autonomous Match Analytics

Milestone: cross-cutting, start v0.1.

Copre:
- decision trace;
- debug overlays;
- golden fixtures;
- autonomous 2v2;
- stress 4v4;
- balance telemetry.

## EPIC — Expert AI / Counterfactual & Multi-turn Strategy

Milestone: future, non gate v0.1.

Copre:
- forward simulation;
- opponent profile;
- Strategic Director;
- Coach/research;
- optional ML research.

---

# 46. Issue titles — AI Foundation

Cercare e consolidare prima di creare.

```text
[AI] Define bot planner authority boundary: Intent producer only
[AI] Add sanitized BotKnowledgeSnapshot / TeamKnowledge input contract
[AI] Consolidate existing hex bot entry point and remove parallel AI paths
[AI] Implement deterministic bot candidate model and stable IDs
[AI] Generate legal move/action/target candidates using real gameplay queries
[AI] Add tactical goal and action skeleton candidate pruning
[AI] Add candidate diversity buckets for offense/control/defense/objective/setup
[AI] Implement integer utility score breakdown
[AI] Add positional scoring for cover, objective, LOS, hazard and escape routes
[AI] Add data-driven Character/Bot/Difficulty profiles
[AI] Add fixed search budgets and deterministic tie-break rules
[AI] Add server-side BotDecisionTrace and candidate dump
```

---

# 47. Issue titles — Tactical Team AI

```text
[AI Team] Implement Top-K deterministic team plan search
[AI Team] Add hard and soft ally intent conflict evaluation
[AI Team] Add friendly-fire and ally collision conflict scoring
[AI Team] Add setup/payoff/environment synergy scoring
[AI Team] Add capability tags and dynamic tactical job matching
[AI Team] Add dynamic tactical roles per turn
[AI Team] Validate temporal compatibility of setup/payoff against resolver order
[AI Team] Add redundancy and overkill penalties to team utility
[AI Team] Add shared-resource allocation via marginal utility
[AI Team] Coordinate bots around human ally draft and committed intents
[AI Team] Add plan hysteresis and stable replanning during Planning
[AI Team] Add bot Ready/unready integration with real planning lifecycle
[AI Team] Generate short localized intent labels from selected tactical goals
```

---

# 48. Issue titles — Reaction Intelligence

```text
[AI Reaction] Consume sanitized ReactionOpportunity in bot planner
[AI Reaction] Implement deterministic Overwatch FIRE/HOLD utility policy
[AI Reaction] Add ReactionPatience profile without future-information access
[AI Reaction] Handle simultaneous valid reaction targets with stable ranking
[AI Reaction] Add bait and unaccounted-threat reasoning without future trigger knowledge
[AI Reaction] Add reaction decision trace and automation fixtures
```

---

# 49. Issue titles — Fair Knowledge / Belief / Threat

```text
[AI Knowledge] Enforce TeamKnowledge-only bot input with hidden-state canary tests
[AI Knowledge] Add Last Known enemy memory to bot knowledge
[AI Knowledge] Integrate authorized acoustic evidence into bot knowledge
[AI Belief] Separate Known State from Enemy Belief State
[AI Belief] Generate graph-aware plausible enemy cells from Last Known state
[AI Belief] Add discrete belief confidence and deterministic evidence scoring
[AI Belief] Invalidate belief reachability on GraphRevision changes
[AI Belief] Treat unknown decoy noise identically to real noise evidence
[AI Threat] Build threat projection from visible and plausible enemy states
[AI Threat] Add unaccounted-enemy threat handling
[AI Information] Score information gain without reading hidden enemy positions
```

---

# 50. Issue titles — Predictive / Expert AI

```text
[AI Prediction] Generate bounded coherent enemy team scenarios
[AI Prediction] Evaluate team plans across multiple plausible enemy scenarios
[AI Prediction] Add robust score with worst-case risk penalty
[AI Prediction] Add tactical bait/feint scenario evaluation
[AI Expert] Add deterministic counterfactual evaluation using logical resolver copies
[AI Expert] Infer opponent tendencies only from observed public events
[AI Expert] Add multi-turn Strategic Director above tactical Team Planner
[AI Research] Evaluate Learning Agents for offline training and balance analysis
```

---

# 51. Issue titles — QA / Debug / Balance

```text
[AI QA] Add bot determinism repeat and permutation golden tests
[AI QA] Add hidden-enemy omniscience canary test
[AI QA] Add noise-decoy knowledge equivalence test
[AI QA] Add human+bot synergy planning fixture
[AI QA] Add temporal synergy compatibility fixture
[AI QA] Add Overwatch HOLD-then-FIRE fixture
[AI QA] Add candidate diversity regression fixture
[AI QA] Add plan hysteresis regression fixture
[AI QA] Add 2v2 autonomous bot scenario
[AI QA] Add 4v4 autonomous stress scenario
[AI QA] Add bot decision metrics to structured scenario reports
[AI Debug] Add belief/threat/team-plan visualization in Development builds
```

---

# 52. Scenario Map — scenari da creare/consolidare

Usare ScenarioId reali/naming corrente; NON duplicare scenari equivalenti.

Candidate:

## Basic

```text
AI.Basic.ObjectiveNoContact
AI.Basic.CoverVsExposure
AI.Basic.HazardDetour
AI.Basic.LegalCandidateOnly
AI.Basic.CandidateDiversity
AI.Basic.DeterministicTieBreak
```

## Team

```text
AI.Team.AllyCollisionConflict
AI.Team.FriendlyFireConflict
AI.Team.WaterElectric
AI.Team.SetupPayoffDeny
AI.Team.OverkillRedundancy
AI.Team.TemporalCompatibility
AI.Team.SharedResourceMarginalUtility
AI.Team.HumanDraftCoordination
AI.Team.HumanCommitCoordination
AI.Team.PlanHysteresis
```

## Reaction

```text
AI.Reaction.OverwatchHoldThenFire
AI.Reaction.SimultaneousTargets
AI.Reaction.UnaccountedThreatHold
```

## Knowledge / Belief

```text
AI.Knowledge.HiddenEnemyFairness
AI.Knowledge.AcousticAreaNotExact
AI.Knowledge.NoiseBeliefUpdate
AI.Knowledge.DecoyIndistinguishable
AI.Knowledge.GraphRevisionBelief
AI.Knowledge.UnaccountedThreat
AI.Information.RevealUnknownSector
```

## Prediction

```text
AI.Prediction.RobustPlanVsObjectivePush
AI.Prediction.RobustPlanVsFlank
AI.Prediction.NoHypothesisBecomesKnowledge
```

## Determinism / Stress

```text
AI.Determinism.Repeat
AI.Determinism.Permutation
AI.Determinism.FixedBudget
AI.Match.2v2Autonomous
AI.Stress.4v4
```

Ogni Scenario deve avere:

- Feature IDs validate;
- deterministic setup;
- BotProfile/version;
- seed;
- expected Intent/TeamPlanStableId dove appropriato;
- TurnLog/StateHash/LogHash se applicabile;
- privacy assertions;
- expected decision trace assertions solo dev/test;
- Automation/Visual/Headless status.

---

# 53. Automation Test richiesti

Almeno:

1. same TeamKnowledge + same profile + same seed => same plan;
2. permutation di candidate/unit array => same plan;
3. hidden canonical enemy position A/B con TeamKnowledge identica => same bot result;
4. decoy/non-decoy indistinguibili se TeamKnowledge li presenta uguali => same belief update;
5. illegal candidate non entra nello scoring;
6. candidate diversity conserva opzione control/objective anche con offense dominante;
7. team planner scarta hard conflict;
8. soft conflict produce penalty;
9. water->electric riceve synergy solo se temporalmente compatibile;
10. overkill/redundancy sposta una unità su target/goal alternativo;
11. human draft influenza i bot senza diventare locked;
12. human commit viene trattato come constraint fisso;
13. hysteresis evita switch per delta minimo;
14. ReactionOpportunity FIRE/HOLD non usa future trigger;
15. belief non si auto-promuove a knowledge;
16. information gain non dipende dal contenuto hidden reale;
17. fixed-budget search produce output identico su ripetizioni;
18. scenario autonomous 2v2 completa planning->commit->snapshot->resolver->TurnLog.

---

# 54. Editor Map — SOLO task realmente manuali

L'AI è prevalentemente C++/data/test. Non riempire l'Editor Map di task automatizzabili.

Candidate Editor Tasks manuali reali:

```text
Validate bot candidate-score overlay readability on hex map
Validate belief-confidence overlay readability at 1080p
Validate threat-projection overlay readability across layers
Validate bot ally intent labels and ghost plans in mixed human+bot team
Validate bot Ready/unready visual feedback during replanning
Validate Overwatch bot FIRE/HOLD debug presentation in Visual scenarios
Validate simultaneous-reaction target highlighting for bot debug scenarios
Validate debug visualization does not obscure cover/facing/hazard overlays
Validate bot tactical role/debug icons for developer readability
```

NON Editor Task:

- creare classi C++;
- implementare scorer;
- modificare YAML;
- scrivere test;
- creare Issue;
- generare Scenario Definition;
- aggiungere validator;
- aggiornare Wiki.

Ogni Editor Task deve linkare Feature/Scenario/Issue che sblocca.

---

# 55. Wiki da aggiornare/consolidare

Non creare duplicati se esistono pagine equivalenti.

Pagine/owner topic che devono essere coperti:

```text
Bot / AI Overview
Bot Fair Knowledge and Anti-Cheating
Bot Candidate Generation and Utility Scoring
Bot Team Planner and Tactical Coordination
Bot Reactions / Overwatch FIRE-HOLD
Bot Belief, Last Known and Threat Projection
Bot Difficulty and Profiles
Bot Debugging and Decision Trace
Bot Testing and Autonomous Scenarios
Human + Bot Team Coordination
```

Cross-link obbligatori:

```text
Bot AI <-> Team Knowledge / Fog of War
Bot AI <-> Noise / Acoustic Perception
Bot AI <-> Overwatch / Fast Reaction
Bot AI <-> Map / Pathfinding / LOS / Targeting
Bot AI <-> Deterministic Resolver
Bot AI <-> Scenario Harness
Bot AI <-> Feature Registry / Roadmap
Bot AI <-> Environment / Water / Electric / Cover / Doors
Bot AI <-> UI Ally Intent / Ready
Bot AI <-> Networking Privacy
```

---

# 56. Documentazione tecnica da consolidare

Cercare gli owner reali. Candidate path noti/storici da verificare:

```text
docs/gameplay/<current-bot-spec>.md
docs/gameplay/spec-bot-utility.md            # possibile storico/archive
docs/technical/architettura-codice.md
docs/technical/scenario-map.md
docs/technical/test-automatico-unreal.md
docs/design/roadmap-v0.1.md                  # se path reale
/docs/roadmap/roadmap-v0.1.md                 # noto dal Project Control Center handoff
docs/roadmap/roadmap-checkpoint.md
docs/design/v0.1-definition-of-done.md        # se esiste
Decision Log / ADR index
```

Aggiornare l'architettura con il diagramma:

```text
TeamKnowledge
    ↓
Bot Planner
    ↓
Intent
    ↓
Planning/Validation/Commit
    ↓
Snapshot
    ↓
Resolver
    ↓
TurnLog
```

Documentare esplicitamente che StateTree/BT/EQS non diventano authority del planning competitivo.

---

# 57. Decision Log / ADR candidate

NON assegnare ID a caso. Cercare decisioni esistenti e usare i prossimi ID reali soltanto se servono nuove entries.

Decisioni da consolidare:

1. **Bot Authority Boundary** — il bot produce normali Intent; validator/resolver restano autorità.
2. **Fair Knowledge AI** — il bot usa TeamKnowledge sanitizzata, non hidden authoritative enemy state.
3. **Deterministic Utility Planner** — core AI tramite candidate generation + integer utility + stable fixed-budget search.
4. **Team Planner Architecture** — per-unit Top-K + central team combination scoring.
5. **Dynamic Roles / Capabilities** — team synergy data-driven, niente hero-name hard-code.
6. **Temporal Synergy** — setup/payoff valido solo se compatibile con resolver rules/order.
7. **Difficulty Fairness** — difficoltà modifica search/scoring, non knowledge/HP/damage cheats.
8. **Reaction Fairness** — FIRE/HOLD usa solo current sanitized ReactionOpportunity; niente future triggers.
9. **Belief Is Not Knowledge** — hypothesis non aggiorna lo stato noto senza nuova evidence autorizzata.
10. **Fixed Search Budget** — conteggi deterministici, non wall-clock think time.
11. **Bot Decision Trace Privacy** — reasoning debug server-only separato dal TurnLog pubblico quando necessario.
12. **Human+Bot Coordination** — human draft come preview constraint, human commit come locked constraint.
13. **Unreal AI Framework Role** — StateTree orchestrator opzionale, Utility planner core; BT/EQS non authority.
14. **ML Scope** — Learning Agents/ML solo ricerca futura/offline salvo nuova decisione.

---

# 58. Data Assets / schema da valutare

Non inventare nuove asset class se il repository ha già un modello.

Concetti dati da rappresentare in modo data-driven:

```text
BotProfileId + Version
DifficultyProfile
CharacterTacticalProfile
UtilityWeights
CandidateBudget
CandidateBucketQuota
ReactionPatience
PlanSwitchThreshold
RobustRiskWeight
MaxEnemyScenarios
CapabilityTags
```

Tutti con Stable ID/version/hash secondo la pipeline contenuti del progetto.

---

# 59. Project Control Center — integrazione richiesta

La dashboard deve rendere il cluster AI navigabile.

Esempio Feature card:

```text
RT-FEAT-BOT-TEAM-COORDINATION
  Epic: Tactical Team AI
  Milestone: <actual milestone>
  Issues: #...
  Scenarios:
    AI.Team.WaterElectric
    AI.Team.TemporalCompatibility
    AI.Team.HumanCommitCoordination
  Wiki:
    Bot Team Planner
  Owner Spec:
    <current bot spec>
  Editor Tasks:
    <manual debug readability only>
```

Esempio Scenario:

```text
AI.Knowledge.HiddenEnemyFairness
  validates:
    RT-FEAT-BOT-TEAMKNOWLEDGE
    RT-FEAT-BOT-BELIEF
  issue:
    [AI Knowledge] Enforce TeamKnowledge-only bot input...
  automated: true
  privacy gate: true
```

Rigenerare shortlists/JSON/feature-status tramite tooling esistente.

---

# 60. Status / gate discipline

La chat ha prodotto design, non implementazione.

Quindi:

- non promuovere `FUTURE` a `IMPLEMENTED`;
- non segnare `DONE` senza test/gate;
- aggiornare `owner_specs`, `notes`, `acceptance`, `dependencies`, `scenarios`, `tests`, `issues` dove lo schema lo consente;
- se esiste uno status `SPECIFIED`/`DESIGNED`, usarlo solo secondo le regole del validator reale;
- non creare percentuali manuali;
- mantenere il modello `N/M gates` se quello corrente.

---

# 61. Definition of Done AI

Una feature Bot/AI è Done solo se, quando applicabile:

```text
spec aggiornata
+ data/versioning
+ runtime reale
+ normali Intent
+ real validator/resolver path
+ no hidden-state leak
+ deterministic result
+ decision trace/debug
+ Automation Test
+ Scenario fixture
+ UI/dev visualization se necessaria
+ packaged test
+ network/privacy test quando entra networking
+ Wiki/Project Control Center links
```

---

# 62. Performance targets AI

Non fissare numeri nuovi senza profiling, ma aggiungere metriche.

Misurare almeno:

```text
candidate generation count/time
candidate prune ratio
Top-K count
team combination count
team planner time
belief cell count
scenario count
forward simulation count (future)
cache hit
replan count
plan switch count
```

Usare Unreal Insights/scopes o telemetry coerente con il progetto.

Il comportamento decisionale deve restare deterministico anche se la misura di performance varia.

---

# 63. Analytics / Balance reuse

Il Bot/Scenario Harness deve poter supportare futuro batch play:

```text
2v2 / 3v3 / 4v4 autonomous matches
```

Metriche candidate:

- win rate;
- objective control;
- damage/KO;
- ability usage;
- unused abilities;
- combo frequency;
- Overwatch FIRE/HOLD;
- average route;
- death/KO heatmap;
- hazard interaction;
- plan switch count;
- reaction frequency;
- match turns/duration;
- StateHash/LogHash divergence;
- bot decision distribution.

Non rendere balance analytics un gate del Bot v0.1 oltre alle metriche minime necessarie.

---

# 64. Errori da evitare

Non:

- creare una seconda AI parallela a `URTHexBotLibrary`/entry point reale;
- trasformare StateTree o Behavior Tree nel resolver tattico;
- usare NavMesh come autorità del path;
- leggere hidden state "perché il bot è sul server";
- hard-codare Flux/Riva/Bastion nei combo scorer;
- usare percentuali belief false come se fossero statistiche calibrate;
- sapere che un noise decoy è falso senza evidence;
- convertire una hypothesis in knowledge;
- duplicare resolver order nell'AI;
- calcolare synergy non temporalmente valida;
- usare time budget non deterministico;
- usare TMap/TSet order;
- creare tutte le candidate feature ID se possono essere gate di feature esistenti;
- mettere implementazione C++ nell'Editor Map;
- creare nuove Wiki duplicate;
- creare Issue duplicate;
- segnare feature Done solo perché la spec è completa;
- far finire BotDecisionTrace privato nel client avversario.

---

# 65. Sequenza operativa Claude

## Fase A — Audit

1. verifica repo/branch/UE version;
2. leggi instruction files;
3. individua AI current implementation;
4. individua TeamKnowledge/Perception current implementation;
5. individua Feature Registry e schema/validator;
6. individua roadmap/milestone/Epic/Issue correnti;
7. individua Scenario Registry/Harness;
8. individua Wiki/owner spec AI;
9. individua Decision Log/ADR;
10. segnala conflitti/duplicati.

## Fase B — Consolidamento design/docs

1. aggiorna owner spec Bot/AI corrente;
2. aggiorna architettura tecnica;
3. aggiorna TeamKnowledge/Noise/Reaction cross-link;
4. aggiorna Wiki;
5. archivia/supersedi vecchia AI square-grid/NavMesh/BT authority se storica;
6. aggiorna ADR/Decision Log solo dove necessario.

## Fase C — Tracking

1. aggiorna `feature-registry.yaml`;
2. collega owner spec/Wiki;
3. collega Epic/Issue;
4. collega Scenario;
5. collega tests;
6. aggiungi Editor Task solo manuali;
7. aggiorna roadmap/checkpoint/milestone mapping;
8. rigenera viste derivate;
9. valida riferimenti.

## Fase D — GitHub

1. cerca Epic/Issue equivalenti;
2. aggiorna quelle esistenti;
3. crea solo gap reali;
4. assegna milestone reale;
5. collega Feature ID/Scenario ID nei body;
6. non inventare issue number nel documento prima della creazione.

## Fase E — Test/validation

1. esegui feature registry validator/generator;
2. verifica link broken;
3. esegui test docs/tooling;
4. se viene toccato codice, build/test secondo repository;
5. non implementare Expert Bot per effetto collaterale del consolidamento.

---

# 66. Deliverable finale richiesto a Claude

Alla fine riportare:

```text
A. Repo/branch/UE version verificati
B. Decisioni consolidate / ADR creati o aggiornati
C. File documentazione modificati
D. Pagine Wiki create/aggiornate
E. Feature Registry: feature/gate/status modificati
F. Roadmap/milestone mapping modificato
G. Epic create/aggiornate
H. Issue create/aggiornate con numero e milestone
I. Scenario Map: ScenarioId aggiunti/aggiornati
J. Editor Map: task manuali aggiunti/aggiornati
K. Generated views rigenerate
L. Validator/test eseguiti e risultati
M. Gap/open questions
N. Commit consigliati/eseguiti
```

Mostrare sempre i numeri reali delle Issue create/aggiornate nel report finale.

---

# 67. Open questions — non inventare decisioni

Restano da confermare contro repository/playtest:

1. entry point/classi AI reali dopo audit;
2. exact BotProfile/DataAsset schema;
3. quote Top-K / candidate budget reali dopo profiling;
4. exact StateTree adoption timing;
5. se EQS verrà mantenuto come prototipo interno o non usato;
6. exact Tactical Role enum/tag taxonomy;
7. exact Capability Tag taxonomy;
8. exact shared team resource model, se esiste nello scope corrente;
9. exact Ready grace/hysteresis UX values;
10. exact belief confidence weights;
11. exact knowledge decay;
12. numero enemy scenarios per difficulty;
13. formula robust score finale;
14. quando Threat Projection diventa feature separata;
15. quando parte counterfactual evaluation;
16. se/come usare opponent tendency inference;
17. se Learning Agents viene mai introdotto nel repository;
18. scope reale multi-turn Strategic Director;
19. eventuale bot usage in ranked e relative policy;
20. spectator/replay visibility del BotDecisionTrace.

Segnalare come `OPEN`/`FUTURE`; non bloccare v0.1 se non necessario.

---

# 68. Commit suggeriti

Adattare alla struttura reale e separare docs/tracking da implementazioni non richieste.

```text
docs(ai): consolidate deterministic bot planner architecture
docs(ai): define fair knowledge team planning and belief model
docs(wiki): consolidate bot AI team coordination and testing pages
docs(roadmap): integrate bot AI epics features and milestones
feat(registry): link bot AI features scenarios issues and editor tasks
test(registry): validate bot AI project graph references
chore(roadmap): regenerate project control center derived views
```

Se il task corrente è solo consolidamento, NON implementare automaticamente tutto il runtime AI.

---

# 69. Risultato atteso

Dopo questo handoff il progetto deve avere una storia unica e coerente:

```text
BOT FOUNDATION
"produce Intent legali e deterministici"
        ↓
TACTICAL TEAM AI
"coordina setup/payoff, conflitti e umano+bot"
        ↓
FAIR KNOWLEDGE AI
"ragiona solo su ciò che la squadra sa"
        ↓
BELIEF / PREDICTION
"considera scenari plausibili senza barare"
        ↓
EXPERT AI
"usa counterfactual e strategia multi-turn solo quando il core è stabile"
```

E tutto deve essere tracciabile nel Project Control Center tramite:

```text
Feature <-> Epic/Issue <-> Milestone <-> Scenario <-> Test <-> Wiki/Spec <-> Editor Task
```

senza duplicare le source of truth.
