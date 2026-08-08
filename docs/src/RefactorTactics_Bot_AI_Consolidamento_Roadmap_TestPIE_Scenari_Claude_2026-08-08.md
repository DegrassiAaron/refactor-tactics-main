# RefactorTactics — Bot AI tattica, roadmap, test PIE e scenari
## Handoff esecutivo per Claude Code — consolidamento repository

**Data:** 2026-08-08  
**Scopo:** consolidare nella repository le decisioni emerse sul sistema Bot/AI di RefactorTactics, riallineare la documentazione corrente, integrare il lavoro nella roadmap, aggiornare il registro dei test manuali PIE e creare scenari di test automatici/visuali coerenti con lo Scenario Harness reale del progetto.

> Questo documento è un handoff di design/engineering. Non sostituisce la source of truth della repository. Claude deve prima verificare branch, HEAD, versione UE bloccata, codice as-built, Decision Log/ADR e documenti canonici, quindi applicare solo ciò che è compatibile con lo stato reale.

---

# 0. Modalità di lavoro obbligatoria

Prima di modificare qualunque file:

1. leggere `CLAUDE.md`, `AGENTS.md`, `README.md` e le istruzioni locali presenti nella repository;
2. identificare la **versione/patch Unreal Engine realmente bloccata** nel repository; non assumere UE 5.8 se il branch reale dice altro;
3. leggere la documentazione corrente relativa a:
   - architettura;
   - griglia hex / `FRTCellId` / pathfinding;
   - simulatore deterministico, snapshot, resolver e `TurnLog`;
   - planning e action economy;
   - reaction / Overwatch / Decision Boundary;
   - TeamKnowledge, percezione, facing, Fog of War e rumore;
   - ambiente, cover, porte, ponti e interazioni;
   - Scenario Harness / test automatici;
   - roadmap corrente;
   - Definition of Done v0.1;
   - registro test manuali PIE;
   - roster/ability catalog corrente;
4. ispezionare il codice AI esistente, in particolare l'eventuale `URTHexBotLibrary` o equivalente;
5. verificare lo Scenario Harness reale. Gli audit recenti indicano che l'implementazione corrente usa una soluzione **CVar + GameMode + stesso runner**, e non richiede un `ARTTestDirector`. Non reintrodurre un Actor test obbligatorio se l'as-built conferma questa scelta;
6. confrontare questo documento con la source of truth e segnalare conflitti prima di riscrivere la storia del progetto;
7. non creare sistemi paralleli solo per il bot o per i test.

Regola di prevalenza:

```text
Decisioni esplicite correnti / ADR
    > codice as-built verificato
    > documentazione canonica corrente
    > questo handoff
    > documenti storici / PDR vecchi / ricerca
```

---

# 1. Obiettivo di design

Il bot di RefactorTactics non deve essere un semplice agente del tipo:

```text
vede nemico -> corre verso nemico -> spara
```

Il gioco è basato su:

- turni simultanei;
- planning;
- informazione incompleta;
- Fog of War;
- facing e percezione;
- rumore;
- Overwatch e Fast Reaction;
- predictive/delayed actions;
- ambiente interattivo;
- combo di squadra;
- obiettivi oltre al puro deathmatch.

Il bot deve quindi **ragionare come un giocatore che prepara un piano**, utilizzando soltanto le informazioni che la propria squadra è autorizzata a conoscere.

Principio principale:

```text
Bot
 ↓
Intent / Command normale
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

Il bot non deve avere una strada privilegiata verso lo stato di gioco.

Vietato:

```text
Bot -> SetActorLocation
Bot -> ApplyDamage
Bot -> modifica diretta MapState
Bot -> esegue Ability senza Intent
Bot -> legge CanonicalIntentStore nemico
Bot -> legge path/destinazioni future nemiche
Bot -> usa un secondo resolver
```

---

# 2. Principio di fairness: il bot non bara

Anche se il bot gira sul server e tecnicamente il server conosce tutto, il planner AI deve ricevere una vista **sanitizzata** costruita secondo le stesse regole informative della squadra.

Architettura target:

```text
AUTHORITATIVE MATCH STATE
          |
          v
      PERCEPTION
          |
          v
    TEAM KNOWLEDGE
          |
          v
     BOT PLANNER
          |
          v
        INTENT
```

Non:

```text
AUTHORITATIVE MATCH STATE
          |
          v
      BOT PLANNER
```

Il bot non può leggere:

- intenti privati nemici;
- path pianificati nemici;
- destinazioni future;
- target pianificati;
- future Reaction Opportunities;
- posizione reale di unità non percepite;
- stato interno completo di unità nascoste.

Le difficoltà superiori devono **ragionare meglio**, non conoscere più informazioni.

---

# 3. Architettura concettuale dell'intelligenza

Modello target evolutivo:

```text
                   TEAM KNOWLEDGE
              vision / noise / memory
                        |
                        v
                Enemy Hypotheses
                        |
                        v
              Tactical Situation
                        |
          +-------------+-------------+
          |             |             |
          v             v             v
       Threat        Opportunity    Objective
        Map              Map          Map
          \             |             /
           +------------+------------+
                        |
                        v
               Candidate Generator
                        |
                        v
                  Team Planner
                        |
              +---------+---------+
              |                   |
              v                   v
        Plan Bot Unit A      Plan Bot Unit B
              |                   |
              +---------+---------+
                        |
                        v
              Counterfactual Tests
                        |
                        v
                  Utility Score
                        |
                        v
                BEST TEAM PLAN
                        |
                        v
                    INTENTS
```

Non tutto entra nella v0.1. La struttura serve per evitare di chiudersi in un bot usa-e-getta.

---

# 4. Strati dell'AI

Separare concettualmente almeno questi livelli:

| Strato | Responsabilità | Esempio |
|---|---|---|
| Knowledge | ciò che il team sa legalmente | nemico visto al turno precedente |
| Belief | ciò che potrebbe essere vero | probabile posizione dietro il ponte |
| Tactical Maps | valore/rischio delle celle | threat, cover, objective, info |
| Candidate Generation | azioni/piani legalmente validi | Move, Attack, Overwatch, Dash |
| Utility | valore del candidato | danno, sicurezza, obiettivo |
| Team Coordination | combo e conflitti alleati | acqua -> elettricità |
| Prediction | comportamento plausibile nemico | avanza / hold / flank |
| Reaction Policy | scelta a Decision Boundary | FIRE / HOLD |

Nella v0.1 implementare solo gli strati necessari, ma documentare il confine.

---

# 5. Roadmap AI proposta

Usare la numerazione **reale** della roadmap. Non rinumerare Epics/checkpoint solo per adattarli a questo documento.

La progressione concettuale è:

```text
BOT v0.1
"sa giocare"
    ↓
TACTICAL BOT v1
"capisce RefactorTactics"
    ↓
EXPERT BOT v2
"ragiona su ipotesi e contromosse"
```

## 5.1 BOT v0.1 — vertical slice

Deve sapere:

- usare il substrato hex corrente;
- produrre Intent normali;
- muoversi verso obiettivi;
- scegliere celle utili;
- cercare cover;
- evitare hazard conosciuti;
- attaccare target legalmente validi;
- usare le abilità del roster v0.1;
- usare Dash quando utile;
- usare Overwatch;
- rispondere alle Fast Reaction tramite policy deterministica;
- considerare HP, costi, cooldown e risorse;
- evitare friendly fire evidente;
- evitare collisioni/conflitti alleati palesi;
- fare una coordinazione 2v2 basilare;
- produrre un breakdown leggibile delle decisioni;
- essere deterministico a parità di input e seed.

Non richiede ancora:

- belief map sofisticata;
- opponent model;
- Monte Carlo;
- look-ahead profondo;
- machine learning;
- reinforcement learning;
- neural network.

## 5.2 Tactical Bot v1

Aggiunge:

- TeamKnowledge pienamente integrato;
- last known / acoustic contacts;
- threat map;
- opportunity map;
- information value;
- coordinazione di squadra vera;
- sinergie ambientali;
- belief weights sulle possibili posizioni nemiche;
- predictive action scoring;
- migliore reaction policy;
- stress 4v4.

## 5.3 Expert Bot v2

Aggiunge, solo dopo stabilizzazione del resolver:

- simulazioni counterfactual;
- più enemy hypotheses;
- opponent model basato su eventi osservati;
- pianificazione robusta contro più risposte plausibili;
- personalità tattiche evolute;
- possibile riuso del planner come Coach/QA tool.

---

# 6. Componenti C++: direzione preferita

Prima verificare le classi reali. Non creare file vuoti o duplicati se esiste già una AI hex.

Target concettuale minimo v0.1:

```text
AI/
  RTBotBrainComponent             oppure servizio/library corrente
  RTBotCandidate
  RTBotScoreBreakdown
  RTBotProfile / Data Asset
  RTBotDecisionTrace
```

Possibili componenti successivi:

```text
RTBotThreatMap
RTBotBeliefModel
RTBotTeamPlanner
RTBotOpponentModel
RTBotSimulationEvaluator
```

Se il progetto usa già `URTHexBotLibrary`, consolidare attorno a quella soluzione invece di introdurre un secondo cervello.

Behavior Tree/StateTree non devono diventare l'autorità della pianificazione tattica simultanea. Possono essere utili per stati macro/presentazione, ma il planner competitivo deve restare deterministico e produrre normali Intent.

---

# 7. TeamKnowledge e mente incompleta

Il bot deve possedere una memoria tattica distinta dallo stato reale.

Esempio concettuale:

```text
Enemy Unit
LastSeenCell = H12
LastSeenTurn = 4
LastFacing = East
PossibleRegion = {...}
Confidence = 63
```

Un contatto sonoro non deve rivelare automaticamente la cella reale:

```text
Noise detected
Direction = NorthEast
EstimatedDistance = 3..5
NoiseClass = Movement.Heavy
```

Possibile interpretazione futura:

```text
Hypothesis A: enemy around H16-H18   weight 55
Hypothesis B: another enemy          weight 30
Hypothesis C: decoy                  weight 15
```

Questi pesi sono belief weights deterministici, non necessariamente probabilità statistiche.

Per la v0.1, se TeamKnowledge non è ancora completamente pronto, introdurre un'interfaccia/DTO corretta e limitare le feature AI alle informazioni realmente disponibili. Non creare un fake FoW solo per il bot.

---

# 8. Candidate Generator

Il bot non inventa mosse arbitrarie: interroga i sistemi gameplay reali e genera solo candidati legalmente possibili.

Famiglie candidate:

```text
Wait
Move
BasicAttack
Interact
Brace/Guard se corrente
Dash
Overwatch
Ability 1..N
Prepared/Predictive Action se già implementata
```

Per Move non enumerare ingenuamente tutta la mappa. Generare/prunare punti tatticamente interessanti, ad esempio:

```text
BestObjectiveCells      3
BestCoverCells          3
BestAttackPositions     3
BestEscapeCells         2
BestInfoCells           2     // quando perception è disponibile
CurrentCell             1
```

I numeri devono diventare data-driven e profilabili.

Il Candidate Generator deve usare:

- path/query reale;
- LOS/targeting reale;
- ability legality reale;
- GraphRevision corrente;
- occupancy reale;
- team knowledge autorizzata.

---

# 9. Matrice Utility Bot v0.1

Questa è una **baseline di bilanciamento iniziale**, non un canone numerico. Claude deve inserirla come proposta/data-driven e non come verità definitiva se i cataloghi correnti hanno già pesi migliori.

## 9.1 Regola generale

Un candidato illegale viene scartato prima dello scoring.

```text
if !Legal:
    REJECT
```

Per i candidati legali:

```text
Utility =
    ObjectiveValue
  + DamageValue
  + KillValue
  + ControlValue
  + PositionValue
  + CoverValue
  + SurvivalValue
  + AllySynergy
  + EnvironmentValue
  + InformationValue
  - IncomingThreat
  - KnownReactionRisk
  - FriendlyFireRisk
  - AllyConflict
  - ResourceCost
  - UncertaintyPenalty
```

Usare valori interi.

## 9.2 Range iniziali suggeriti

| Fattore | Range iniziale | Note |
|---|---:|---|
| ObjectiveValue | 0..250 | cattura, contest, deny, progress |
| DamageValue | 0..180 | danno deterministico/atteso legalmente stimabile |
| KillValue | 0..300 | bonus forte ma non sempre superiore all'obiettivo |
| ControlValue | 0..140 | slow, displacement, deny, funnel |
| PositionValue | -100..+140 | flank, linee future, uscita, quota |
| CoverValue | 0..100 | low/high cover secondo regole correnti |
| SurvivalValue | 0..180 | aumenta a HP bassi o sotto pressione |
| AllySynergy | 0..200 | combo di team e setup/payoff |
| EnvironmentValue | -180..+200 | acqua, elettrico, fuoco, porte, cover, hazard |
| InformationValue | 0..120 | scouting/reveal, quando il sistema esiste |
| IncomingThreat | 0..220 | esposizione a minacce conosciute |
| KnownReactionRisk | 0..180 | solo Overwatch/reaction conosciute o plausibili da knowledge |
| FriendlyFireRisk | 0..350 | forte penalità, salvo abilità/policy esplicita |
| AllyConflict | 0..300 | collisione, stesso spazio, piano incompatibile |
| ResourceCost | 0..100 | cooldown/charge/costo opportunità |
| UncertaintyPenalty | 0..120 | dipende da profilo e confidence |

Questi range devono stare in un Data Asset/config o equivalente, non hard-coded per sempre.

---

# 10. Matrice per tipo di decisione

## 10.1 Movimento

Valutare almeno:

| Fattore | Direzione |
|---|---|
| avvicinamento obiettivo | positivo |
| raggiungimento cover | positivo |
| uscita da threat/hazard | positivo |
| apertura LOS utile | positivo |
| flank | positivo |
| setup combo ambientale | positivo |
| mantenere via di fuga | positivo |
| entrare in hazard noto | negativo |
| esposizione a linee note | negativo |
| passare in Overwatch conosciuta | negativo |
| collisione alleata prevista | molto negativo |
| allontanarsi inutilmente dall'obiettivo | negativo |

## 10.2 Attacco

Valutare almeno:

| Fattore | Direzione |
|---|---|
| danno confermato | positivo |
| KO | molto positivo |
| interrompe combo/obiettivo | positivo |
| target ad alto valore strategico | positivo |
| setup per alleato | positivo |
| friendly fire | molto negativo |
| basso valore per costo elevato | negativo |
| target altamente incerto | negativo/profilato |

## 10.3 Overwatch

Valutare:

- valore del cono/area controllata;
- choke point;
- copertura dell'obiettivo;
- protezione di un alleato;
- probabilità/belief lecita di transito futuro, solo quando il belief system esiste;
- perdita di un attacco immediato;
- rischio di essere flanked;
- charge/costo opportunità.

## 10.4 Ambiente

Il bot non deve vedere solo `HazardCost`. Deve cercare opportunità:

```text
Water -> electric opportunity
Water + fire -> steam/visibility interaction
Door -> funnel / deny / choke
Cover -> defense / destroy / reconfigure
Ice -> movement/control opportunity
Bridge/tunnel -> route control
Noise source -> masking / detection / deception
```

Usare soltanto interazioni realmente implementate e canoniche.

---

# 11. Profili del roster / specializzazione AI

Non creare un algoritmo separato per ogni eroe. Usare lo stesso scorer con pesi/profili diversi.

Esempio concettuale per il roster corrente atteso dagli ultimi documenti (verificare i nomi canonici prima di applicare):

| Profilo | Bias principali |
|---|---|
| Flux / tecnico elettrico | danno elettrico, propagazione, setup acqua/elettrico |
| Riva / controller acqua | control, environment, ally synergy, displacement |
| Bastion / difesa-cover | cover, team safety, objective hold, route control |
| Vektor / predittivo | flank, interception, prediction, high payoff gambit |

Se il roster canonico ha nomi diversi, usare i ruoli/ability ID reali e aggiornare questa tabella.

Mai:

```text
if Hero == Flux -> hard-coded branch
```

Preferire:

```text
BotProfile / CharacterDefinition / Ability tags -> weights
```

---

# 12. Coordinazione di squadra

## 12.1 v0.1 — coordinazione semplice

Prima versione:

- evitare collisioni alleate;
- evitare friendly fire;
- riconoscere setup/payoff ovvi;
- dare bonus se un candidato sfrutta un setup alleato già scelto;
- penalizzare piani reciprocamente incompatibili.

Esempio:

```text
Riva -> crea Wet
Flux -> Electric payoff
```

La coppia può avere:

```text
IndividualScoreA + IndividualScoreB + SynergyBonus
```

## 12.2 Tactical Bot — team planner

Successivamente, non scegliere semplicemente il best candidate di ciascuna unità indipendentemente.

Pipeline:

```text
Top candidates per unit
      ↓
pruning
      ↓
synergy pairs / conflict detection
      ↓
Top-K team plans
      ↓
team utility
      ↓
selected team plan
```

Evitare esplosione combinatoria.

Esempio target futuro:

```text
8-15 candidati utili per unità
Top-K team plans limitato/configurabile
```

Misurare sempre il costo CPU.

---

# 13. Prediction e Belief Model

Questa parte è post-v0.1 salvo thin slice già prevista in roadmap.

Il bot deve chiedersi:

> qual è il piano migliore contro ciò che credo che il nemico possa fare?

Non:

> qual è il piano migliore conoscendo ciò che farà davvero?

Esempio:

```text
Enemy hypotheses
35 -> raggiunge H8
25 -> prende cover H7
20 -> resta
15 -> Dash H10
 5 -> altro
```

Una Predictive Action può essere valutata tramite belief weights.

La precisione del bot cresce perché costruisce ipotesi migliori, non perché legge gli intenti.

---

# 14. Reaction Policy / Overwatch

Il sistema di reaction è un punto chiave dell'intelligenza.

Il bot riceve la stessa `ReactionOpportunity` sanitizzata prevista per il gameplay.

Per Overwatch standard:

```text
FIRE / HOLD
```

Il bot non deve conoscere trigger futuri.

Valutazione Commit iniziale:

```text
CommitUtility =
    TargetStrategicValue
  + ExpectedDamage
  + KillPotential
  + ObjectiveThreat
  + AllyProtection
  - ResourceOpportunityCost
```

`ReactionPatience` modifica la soglia di FIRE, non la conoscenza del futuro.

Esempio:

```text
Conservative / Patient:
FIRE threshold alto

Aggressive:
FIRE threshold basso
```

Caso simultaneo nello stesso micro-step:

```text
Target A
Target B
HOLD
```

Il bot sceglie il target con utility più alta usando tie-break stabile.

Timeout policy automatica e policy di test devono essere distinte dalla decisione AI normale.

---

# 15. Determinismo

Requisito:

```text
same logical state
+ same TeamKnowledge
+ same bot profile
+ same rules/content versions
+ same seed
=
same Intent / same bot plan
```

Non dipendere da:

- ordine di `TMap` / `TSet`;
- frame rate;
- timing animazioni;
- tick client;
- wall clock;
- ordine non definito dei candidati.

Tie-break suggerito, da adattare ai tipi reali:

```text
1. Utility desc
2. ActionPriority stabile
3. AbilityId stabile
4. TargetUnitId stabile
5. TargetCell canonical order
6. CandidateStableId
```

Se si introduce errore intenzionale per difficoltà bassa, deve essere anch'esso deterministico rispetto a seed/stream dedicato e non spostare RNG competitivo.

---

# 16. Explainability e debug

Ogni decisione AI deve poter essere spiegata.

Esempio debug server-side:

```text
BOT PLAN selected
Unit = Vektor
Action = Overwatch East
Score = 842

+250 ObjectiveControl
+180 ChokeControl
+125 ExpectedDamage
+110 CoverAdvantage
 +90 AllySupport
 -30 KnownReactionRisk
```

Conservare almeno:

```text
Selected candidate
Top rejected candidates
Score total
Score components
Knowledge revision
Bot profile ID/version
Decision seed/stream se usato
```

Privacy:

- il trace completo del bot durante Planning deve essere **server-only/debug**;
- non replicare agli avversari il reasoning che rivela il piano;
- il TurnLog canonico continua a descrivere gli eventi di simulazione, non deve essere gonfiato con tutti i candidati AI;
- se serve, creare/riusare un `BotDecisionTrace` separato, machine-readable, collegato al Turn/Unit/Intent.

---

# 17. Difficoltà

Non usare cheat informativi.

Esempio di progressione:

| Capacità | Easy | Normal | Hard | Expert |
|---|---:|---:|---:|---:|
| candidati utili/unità | 4 | 8 | 12 | 16 |
| team planning | no | base | sì | avanzato |
| ambiente | base | medio | alto | alto |
| enemy belief | no | semplice | sì | sì |
| counterfactual | no | no | limitato | sì |
| opponent model | no | no | no | sì |
| reaction policy | semplice | normale | avanzata | avanzata |
| suboptimalità intenzionale | alta | media | bassa | minima |

I valori sono iniziali e devono essere configurabili.

---

# 18. Opponent Model — futuro

Non machine learning: memoria deterministica di pattern osservati.

Possibili statistiche:

```text
Aggression
ObjectiveFocus
ReactionPatience
CoverPreference
DashFrequency
OverwatchFrequency
FlankPreference
RiskTolerance
```

Esempio:

```text
Player tends to:
- HOLD Overwatch for high-value targets
- Sprint toward objectives
- avoid electrified water
- favor left flank
```

Il modello deve aggiornarsi solo da eventi percepiti/legittimamente conosciuti.

---

# 19. Counterfactual simulation — futuro

Quando il resolver sarà abbastanza stabile e veloce, il planner Expert può riusarlo per simulare:

```text
OurPlan
x
EnemyHypothesis
-> outcome utility
```

Requisiti:

- nessuna mutazione dello stato match;
- snapshot/copy pura;
- budget CPU esplicito;
- ordine stabile;
- stesse regole del resolver reale;
- nessun dato segreto nella generazione delle enemy hypotheses.

Non implementare ora se aumenta il rischio della v0.1.

---

# 20. Possibile riuso: Bot Coach / QA

Direzione futura interessante:

- analizzare un piano umano dopo il turno;
- mostrare un'alternativa trovata dal planner;
- usare il planner per cercare regressioni o linee fortemente dominanti;
- usare il bot nello Scenario Harness per soak/stress;
- usare i trace per balancing.

Non introdurre coaching live in modalità competitiva se può rivelare informazioni non autorizzate.

---

# 21. Consolidamento documentale richiesto

Claude deve trovare i path reali e aggiornare la documentazione senza duplicare fonti di verità.

## 21.1 `spec-bot-utility.md` storico

Gli audit recenti indicano che `docs/gameplay/spec-bot-utility.md` descrive il vecchio bot su substrato quadrato.

Azione preferita:

```text
ARCHIVE / HISTORICAL
```

Preservare provenance, ma impedirne la lettura come spec corrente.

Creare o aggiornare una **spec corrente bot hex**, con naming coerente alla repository, che includa almeno:

- utility scoring;
- candidate generation;
- deterministic tie-break;
- TeamKnowledge / partial knowledge;
- facing/perception dependency;
- reaction policy;
- team coordination;
- scenario harness integration;
- privacy/no hidden intent;
- 2v2 vertical slice;
- 4v4 stress;
- debug/explainability;
- future belief/counterfactual roadmap.

## 21.2 Architettura tecnica

Aggiornare il documento owner dell'architettura per includere il bot come **producer di Intent**, non come secondo gameplay path.

Diagramma consigliato:

```text
Human UI ---------+
Bot --------------+--> Intent --> Planning --> Commit --> Snapshot --> Resolver
Scenario Harness -+
Replay -----------+
```

## 21.3 TeamKnowledge / perception

Cross-linkare la spec AI ai documenti current su:

- TeamKnowledge;
- facing;
- Fog of War;
- rumore;
- last known;
- acoustic contacts;
- privacy.

## 21.4 Scenario Harness

Aggiornare la spec del test automatico per indicare che gli **Agent Scenarios** possono usare il bot per produrre Intent, sempre attraversando il gameplay reale.

Non reintrodurre `ARTTestDirector` se l'as-built corrente è CVar + GameMode + runner.

---

# 22. Integrazione nella roadmap

Claude deve rielaborare la roadmap **corrente**, non creare una roadmap AI isolata che poi diverge.

Vincoli:

1. mantenere Epics/checkpoint esistenti se sensati;
2. aggiungere dipendenze AI ai checkpoint corretti;
3. non mettere TeamKnowledge nel bot prima che esista il relativo dominio;
4. non mettere counterfactual prima di un resolver stabile;
5. non bloccare la v0.1 con feature Expert;
6. il bot base deve arrivare in tempo per il vertical slice/scenario showcase;
7. il 4v4 è stress validation, non necessariamente formato finale;
8. se esistono già E13 TeamKnowledge, E14 Decision Window/Overwatch, E15 showcase, E16 facing, E17 stress validation o equivalenti, integrare lì senza inventare numerazione concorrente.

## 22.1 Inserimenti minimi suggeriti

### Prima del/nel Vertical Slice

```text
AI Foundation
- current hex bot entry point
- legal candidate generation
- utility scoring
- deterministic tie-break
- bot profile data
- objective movement
- attack/ability use
- basic cover/hazard
- basic reaction policy
- decision trace
- automated tests
```

### Con TeamKnowledge / Perception

```text
Fair Knowledge AI
- bot consumes TeamKnowledge
- last known
- acoustic contacts
- facing/perception
- no authoritative hidden-state reads
- privacy/canary test
```

### Con Environment/Reactions stabili

```text
Tactical Coordination
- water/electric synergy
- dynamic cover / door choke
- Overwatch positioning
- ally conflict pruning
- 2v2 team plan scoring
```

### Stress validation

```text
4v4 AI Stress
- 8 units
- planning time
- candidate count
- bot decision CPU
- resolver CPU
- reaction opportunities
- TurnLog size
- zero divergence
```

### Post-v0.1

```text
Belief Model
Opponent Model
Counterfactual Planner
Expert Profiles
Bot Coach / balancing support
```

---

# 23. Definition of Done AI v0.1

La AI base è Done solo se:

```text
[ ] usa il substrato hex corrente
[ ] produce normali Intent
[ ] non bypassa validation/snapshot/resolver
[ ] non legge hidden enemy intents
[ ] decisione deterministica a stesso input
[ ] tie-break stabile
[ ] sa raggiungere/contestare un objective
[ ] sa usare attacco e ability legali
[ ] sa considerare cover/hazard basilari
[ ] sa usare Overwatch/Fast Reaction se la feature è green
[ ] evita friendly fire ovvio
[ ] evita conflitti alleati ovvi
[ ] espone score breakdown/debug
[ ] ha Automation Test
[ ] ha Scenario Harness test
[ ] ha almeno un test visuale PIE
[ ] passa packaged smoke se il sistema coinvolge runtime asset/network
[ ] documentazione e roadmap sono aggiornate
```

---

# 24. Aggiornare il registro test manuali PIE

Il repository sembra usare un registro tipo `docs/design/test-manuali-pie.md`. Verificare il path reale.

Se l'utente ha scritto "PE", interpretare come **PIE / Play In Editor** salvo terminologia diversa nel repo.

Non far crescere una lista senza governance. Aggiungere:

- ID stabile;
- area `AI/Bot`;
- prerequisiti;
- scenario/fixture;
- passi minimi;
- risultato atteso;
- tag `RELEASE-V01` se deve far parte del gate release;
- eventuale `AUTOMATED` / `MANUAL` / `PACKAGED`;
- ultimo esito/data se il registro già lo prevede.

## 24.1 Nuovi test PIE proposti

Usare gli ID reali liberi.

### AI-PIE-01 — Bot produce solo Intent legali

- avvia scenario base;
- nessun input umano;
- bot pianifica;
- assert visuale/log: nessun bypass, nessun invalid intent accettato.

### AI-PIE-02 — Objective awareness senza contatto

- nessun nemico visibile;
- objective disponibile;
- il bot avanza/contesta secondo profilo invece di restare inattivo o cercare stato nascosto.

### AI-PIE-03 — Lethal vs posizione

- bersaglio legalmente eliminabile;
- alternativa di movimento mediocre;
- il bot preferisce il KO se l'objective non domina la scelta.

### AI-PIE-04 — Cover vs esposizione

- due celle raggiungibili con costo simile;
- una offre cover valida;
- il bot preferisce la posizione tatticamente migliore secondo score breakdown.

### AI-PIE-05 — Hazard avoidance

- path breve attraversa hazard noto;
- path leggermente più lungo è sicuro;
- il bot evita il hazard se il profilo/regola lo giustifica.

### AI-PIE-06 — Overwatch: HOLD poi FIRE

- prima opportunity target di basso valore;
- seconda opportunity target ad alto valore;
- il bot HOLD sulla prima e FIRE sulla seconda;
- nessuna conoscenza della seconda opportunity usata durante la prima.

### AI-PIE-07 — Overwatch simultaneo

- due target entrano nello stesso micro-step;
- singola opportunity multi-target;
- bot sceglie target con utility maggiore e tie-break stabile.

### AI-PIE-08 — Team combo acqua/elettrico

- controller crea setup Wet;
- tecnico elettrico può sfruttarlo;
- team AI preferisce o valorizza la combo quando superiore alle alternative.

### AI-PIE-09 — Friendly fire avoidance

- AoE efficace ma include alleato;
- alternativa sicura disponibile;
- bot evita il piano salvo profilo/regola esplicita che renda il sacrificio razionale.

### AI-PIE-10 — Ally collision/conflict

- due bot vorrebbero la stessa destinazione/choke;
- planner/pruning evita piano incompatibile quando esiste alternativa valida.

### AI-PIE-11 — Hidden enemy fairness

- nemico fuori knowledge ma presente nello stato autorevole;
- il bot non devia il piano come se ne conoscesse la cella reale.

### AI-PIE-12 — Noise/last-known

Da abilitare solo quando TeamKnowledge/rumore è green:

- nemico non visibile;
- evento acustico autorizzato;
- bot reagisce all'area/direzione nota, non alla posizione segreta esatta.

### AI-PIE-13 — Decoy sonoro

Post-substrate rumore:

- decoy genera evento legittimo;
- il bot può essere tratto in inganno coerentemente con la propria knowledge;
- nessun accesso a flag segreto "isDecoy" se non identificato dalle regole.

### AI-PIE-14 — Door/choke + Overwatch

- porta/choke controllabile;
- il bot difensivo riconosce valore di chiudere/canalizzare e presidiare la rotta.

### AI-PIE-15 — 2v2 autonomous match smoke

- roster vertical slice corrente;
- entrambe le squadre bot o una squadra bot;
- N turni senza input umano;
- nessun crash, invalid intent o stall;
- objective/KO/cleanup completano correttamente.

### AI-PIE-16 — 4v4 stress

- 8 unità;
- registrare planning CPU, candidate count, reaction count, resolver duration, TurnLog size;
- verificare leggibilità e assenza di prompt storm se ci sono bot+human reaction boundaries.

---

# 25. Test automatici richiesti

Creare/aggiornare Automation Tests e Scenario Harness tests. Non trasformare tutto in PIE manuale.

## 25.1 Core AI

```text
Bot.NoIllegalIntent
Bot.SameKnowledgeSameDecision
Bot.DeterministicTieBreak
Bot.DoesNotUseHiddenEnemyIntent
Bot.PrefersObjectiveWithoutContact
Bot.PrefersLegalLethalWhenAppropriate
Bot.AvoidsKnownLethalHazard
Bot.PrefersUsefulCover
Bot.AvoidsFriendlyFire
Bot.AvoidsAllyConflict
```

## 25.2 Reactions

```text
Bot.Overwatch.HoldLowValue
Bot.Overwatch.FireHighValue
Bot.Overwatch.HoldThenFire
Bot.Overwatch.SimultaneousTargetsStableChoice
Bot.Reaction.NoFutureOpportunityKnowledge
Bot.Reaction.TimeoutPolicyScenario
```

## 25.3 Team/environment

```text
Bot.Team.WaterElectricSynergy
Bot.Team.DynamicCoverSynergy
Bot.Team.DoorChokeControl
Bot.Environment.DoesNotTreatAllHazardsAsPureCost
```

Implementare solo ciò che le feature reali permettono. Gli altri diventano backlog/test plan.

## 25.4 Perception/fairness

Quando TeamKnowledge è disponibile:

```text
Bot.Knowledge.VisibleEnemy
Bot.Knowledge.LastKnownOnly
Bot.Knowledge.AcousticAreaNotExactCell
Bot.Knowledge.DecoyCanMislead
Bot.Knowledge.HiddenEnemyPositionCanary
```

## 25.5 Determinism / stress

```text
same scenario x N runs
-> same BotDecisionHash
-> same StateHash
-> same LogHash
```

Aggiungere permutation test dove l'ordine dei container può cambiare.

---

# 26. Scenario Harness: scenari da creare

Usare il formato testuale **già reale** dello Scenario Harness. Non inventare JSON alternativo se il repo usa un altro schema.

Gli scenari devono poter girare, dove supportato, in:

- Visual / PIE;
- Fast;
- Headless/Automation.

Il bot deve produrre normali Intent e il runner deve attraversare il gameplay reale.

## Scenario S1 — `AI.Basic.ObjectiveNoContact`

**Scopo:** validare objective awareness senza informazioni nemiche.

Setup:

- 1 bot;
- objective raggiungibile;
- nessun nemico percepito;
- due route legali.

Assert:

- intent legale;
- progress verso objective;
- decision trace contiene `ObjectiveValue`;
- nessuna consultazione a hidden enemy state.

## Scenario S2 — `AI.Basic.CoverVsExposure`

**Scopo:** validare scoring posizione/cover.

Setup:

- due destinazioni con costo simile;
- una con cover valida;
- minaccia conosciuta.

Assert:

- preferenza coerente alla cover/threat;
- score breakdown leggibile.

## Scenario S3 — `AI.Basic.HazardDetour`

**Scopo:** validare path/candidate risk.

Setup:

- route corta per hazard noto;
- route sicura leggermente più lunga.

Assert:

- scelta coerente col profilo;
- path reale valido;
- nessun bypass A*.

## Scenario S4 — `AI.Reaction.OverwatchHoldThenFire`

**Scopo:** firma del sistema reaction AI.

Sequenza:

```text
TargetLow enters
-> Opportunity
-> HOLD

TargetHigh enters later
-> Opportunity
-> FIRE
```

Assert:

- HOLD non consuma charge;
- FIRE consuma charge;
- prima decisione non contiene dati sul secondo trigger;
- reaction trace deterministico.

## Scenario S5 — `AI.Reaction.SimultaneousTargets`

**Scopo:** evitare dipendenza dall'ordine di iterazione.

Setup:

- due target triggerano nello stesso micro-step.

Assert:

- una singola opportunity multi-target;
- scelta del target con utility/tie-break stabile;
- N repeat identici.

## Scenario S6 — `AI.Team.WaterElectric`

**Scopo:** prima vera combo coordinata.

Setup:

- unità acqua/controller;
- unità elettrica;
- alternative individuali meno efficienti;
- area adatta al setup.

Assert:

- combo riceve `AllySynergy`/`EnvironmentValue`;
- entrambi generano Intent normali;
- resolver produce propagazione canonica;
- zero hard-code hero-specific.

## Scenario S7 — `AI.Team.DynamicCoverChoke`

**Scopo:** validare ambiente + coordinazione.

Setup:

- cover/porta modificabile;
- objective/choke;
- unità difensiva + ranged/predictive.

Piano desiderabile:

```text
unit A modifica/crea cover o chiude rotta
unit B sfrutta la nuova geometria/Overwatch
```

Assert:

- GraphRevision/cover state aggiornati dal gameplay reale;
- piano team non usa informazioni future illegali.

## Scenario S8 — `AI.Fairness.HiddenIntentCanary`

**Scopo:** test critico anti-cheat AI.

Setup:

- inserire canary riconoscibile nell'intento nemico server-only;
- creare due intenti nemici diversi che producono **stessa TeamKnowledge corrente**.

Assert:

```text
BotDecision(A) == BotDecision(B)
```

finché la knowledge autorizzata è identica.

Questo è uno dei test più importanti dell'intero sistema.

## Scenario S9 — `AI.Knowledge.LastKnownSearch`

Abilitare con TeamKnowledge.

Setup:

- nemico visto e poi perso;
- last known + movement bounds.

Assert:

- il bot ragiona sulla zona possibile;
- non segue la posizione vera fuori visione.

## Scenario S10 — `AI.Knowledge.NoiseDecoy`

Abilitare col rumore.

Setup:

- enemy hidden;
- decoy produce rumore reale;
- fonte reale rimane silenziosa o mascherata.

Assert:

- bot reagisce al contatto sonoro secondo policy;
- può essere ingannato;
- non legge `IsDecoy` se non identificato.

## Scenario S11 — `AI.Match.VerticalSlice2v2`

Usare il roster **canonico corrente**. Se confermato dagli ultimi documenti:

```text
Flux + Riva
vs
Bastion + Vektor
```

Altrimenti sostituire coi nomi reali.

Obiettivi:

- full loop;
- movement;
- ability;
- environment;
- cover;
- reaction;
- objective;
- cleanup;
- TurnLog;
- bot decision trace;
- zero input umano.

## Scenario S12 — `AI.Stress.4v4`

Usare il roster/core stress corrente.

Misurare:

```text
bot planning duration
candidate generation duration
candidates/unit
team plans evaluated
reaction opportunities/turn
resolver duration
TurnLog size
StateHash / LogHash
```

Exit gate:

```text
N repeat
0 divergence
0 invalid intents
0 deadlock/stall
metrics recorded
```

---

# 27. Metriche AI

Aggiungere telemetry/debug locale dove appropriato:

```text
BotPlanningMs
CandidateGenerationMs
ScoringMs
TeamPlanningMs
CandidatesGenerated
CandidatesPruned
TeamPlansEvaluated
SelectedUtility
KnowledgeRevision
BotDecisionHash
```

Non fissare budget di produzione arbitrari prima delle misure.

Per la v0.1 registrare almeno baseline e regressioni.

---

# 28. Privacy e networking

Quando entra networking/dedicated:

- il bot server-side continua a consumare TeamKnowledge sanitizzata;
- nessun client avversario riceve BotDecisionTrace privato durante Planning;
- i bot non diventano un canale indiretto di leak;
- i test privacy devono verificare anche il percorso bot;
- spectator/replay devono avere policy esplicita sul reasoning AI.

Aggiungere canary test packaged se la rete è nel milestone corrente.

---

# 29. Scenario Harness e AI: no bypass

Il test automatico deve esercitare il vero bot e il vero gameplay.

Target:

```text
Scenario
  ↓
Bot Policy / Scripted Intent
  ↓
Planning
  ↓
Ready / Commit
  ↓
Snapshot
  ↓
Resolver
  ↓
TurnLog
  ↓
Assertions
```

Vietato:

```text
if Test && Bot -> teleport
if Test && Bot -> ApplyDamage
if Test -> skip reaction validation
if Test -> set objective state directly per ottenere PASS
```

Il Test Harness è un orchestratore, non una seconda simulazione.

---

# 30. File/documenti da verificare e probabilmente modificare

Usare i path reali trovati nel repository. Candidati attesi:

```text
docs/gameplay/spec-bot-utility.md              -> archive/historical
docs/gameplay/<new-current-bot-spec>.md        -> create/update
docs/technical/architettura-codice.md          -> AI producer-of-intent diagram
docs/technical/test-automatico-unreal.md       -> Agent Scenario integration
docs/design/roadmap-checkpoint.md               -> AI checkpoints/dependencies
docs/design/roadmap-v0.1.md                    -> bot scope in v0.1
docs/design/v0.1-definition-of-done.md         -> AI gates/tests
docs/design/test-manuali-pie.md                -> new PIE registry entries
docs/design/v0.1-issue-plan.md                 -> issues if repo uses issue plan
Decision Log / ADR index                       -> only if new decisions need IDs
Scenarios/...                                   -> create scenario fixtures using real schema
```

Non creare file duplicati se esistono owner canonici con nomi diversi.

---

# 31. Issue / backlog da inserire o rielaborare

Prima cercare issue esistenti e riusarle.

Possibili work item, da mappare alla numerazione corrente:

```text
AI Foundation — deterministic hex utility bot
AI Candidate Generator — legal action/path/target enumeration
AI Scoring — data-driven utility + breakdown
AI Profiles — role/personality/difficulty data
AI Reactions — Overwatch/Fast Reaction policy
AI Team Coordination — conflict pruning + synergy
AI Fair Knowledge — TeamKnowledge-only planner input
AI Perception — last-known/noise integration
AI Stress — 2v2/4v4 autonomous scenarios
AI Expert Future — belief/counterfactual/opponent model
```

Ogni issue deve avere:

- dipendenze;
- scope;
- out-of-scope;
- file toccati;
- DoD;
- Automation Test;
- PIE test se necessario;
- scenario fixture;
- debug/log;
- performance metric se rilevante.

---

# 32. Decisioni da registrare se non già presenti

Non assegnare ID a caso: usare i prossimi liberi nel Decision Log.

Decisioni da rendere esplicite:

1. **Bot fairness** — il planner AI non legge hidden intents o hidden authoritative enemy state; usa TeamKnowledge sanitizzata.
2. **Bot authority boundary** — il bot produce Intent normali; resolver/validation restano autorità.
3. **AI core** — v0.1 usa Utility AI deterministica/candidate scoring; Behavior Tree non è il planner competitivo centrale.
4. **Difficulty fairness** — difficoltà aumenta capacità di ricerca/scoring, non accesso informativo.
5. **AI explainability** — decision trace server-side separato dal TurnLog canonico quando contiene reasoning privato.
6. **AI roadmap** — belief/counterfactual/opponent model sono post-v0.1 salvo thin slice esplicitamente approvata.

Se queste decisioni sono già implicitamente coperte da ADR/Decision esistenti, cross-linkare invece di duplicare.

---

# 33. Errori da evitare

Non:

- trasformare `spec-bot-utility.md` quadrata in current senza preservare la storia;
- creare una seconda AI parallela a `URTHexBotLibrary` se quella è current;
- usare stato autorevole completo perché "tanto il bot è server-side";
- usare Behavior Tree come scorciatoia per la simultaneità;
- usare RNG non deterministico per rendere Easy più stupido;
- fare scoring dipendente da `TMap` iteration order;
- hard-codare eroi nei valutatori;
- hard-codare lo showcase nel bot;
- fare migliaia di simulazioni counterfactual nella v0.1;
- duplicare Scenario Harness;
- rendere tutti i nuovi test manuali PIE se possono essere Automation/Scenario tests;
- aggiungere un nuovo gate release senza motivarlo.

---

# 34. Acceptance criteria del consolidamento

Il task documentale/roadmap/scenari è completato quando:

```text
[ ] stato AI as-built verificato
[ ] vecchia spec bot quadrata non è più source of truth
[ ] esiste una spec bot hex corrente
[ ] bot -> Intent -> Resolver è documentato
[ ] fairness/TeamKnowledge è esplicita
[ ] difficulty non bara
[ ] Utility v0.1 e score matrix sono documentati/data-driven
[ ] reaction policy è documentata
[ ] roadmap corrente contiene AI foundation + dipendenze
[ ] Expert AI è chiaramente post-v0.1
[ ] DoD v0.1 include gate AI appropriati
[ ] registro PIE include test AI con subset RELEASE-V01 ragionato
[ ] Automation tests AI sono pianificati/creati dove possibile
[ ] Scenario Harness contiene scenari AI usando il formato reale
[ ] esiste un test HiddenIntentCanary / same knowledge same decision
[ ] esiste un test determinismo repeated AI
[ ] 2v2 autonomous scenario è definito
[ ] 4v4 AI stress scenario è definito o schedulato
[ ] decision trace/debug è specificato senza leak
[ ] nessun secondo resolver/pathfinding/test gameplay è stato introdotto
[ ] issue esistenti sono riusate invece di duplicate
```

---

# 35. Output richiesto a Claude Code

Al termine, produrre un report chiaro.

## A. Audit iniziale

```text
Branch
HEAD iniziale
UE version/patch
Documenti canonici letti
Classi AI reali trovate
Scenario Harness reale trovato
Roadmap/PIE registry owner
Conflitti con questo handoff
```

## B. Documentazione

Per ogni file:

```text
path
azione: CREATE / UPDATE / ARCHIVE / NO CHANGE
perché
decisioni consolidate
```

## C. Roadmap

Mostrare:

- checkpoint modificati;
- dipendenze AI;
- cosa entra nella v0.1;
- cosa è post-v0.1;
- exit gate AI;
- 2v2 e 4v4 stress placement.

## D. Test

Mostrare:

```text
Automation tests aggiunti/pianificati
PIE tests aggiunti
PIE RELEASE-V01 subset
Packaged tests richiesti
Determinism/repeat tests
Privacy/fairness tests
```

## E. Scenari

Per ogni scenario:

```text
ScenarioId
file
mode supportato: Visual/Fast/Headless
feature coperte
assert principali
stato: implemented / planned / blocked by dependency
```

## F. Issue

Per ogni issue riusata/creata/proposta:

```text
number/id
title
milestone/checkpoint
dependency
DoD
test/scenario associato
```

## G. Verifica

Eseguire i test/documentation checks disponibili e riportare:

```text
command
result
failures
```

Non dichiarare PASS se un test non è stato eseguito.

## H. Git

Proporre commit focalizzati, ad esempio:

```text
docs(ai): consolidate deterministic tactical bot design
docs(roadmap): integrate bot milestones and gates
test(ai): add bot scenario fixtures and deterministic coverage
docs(test): extend PIE registry for bot validation
```

Non fare un unico commit enorme se il repository preferisce commit separati.

---

# 36. Priorità consigliata

Ordine suggerito, adattandolo allo stato reale:

```text
P0  Audit AI corrente + fairness boundary
P0  Archive vecchia spec bot square
P0  Spec bot hex current
P0  Roadmap + DoD + PIE registry
P0  HiddenIntentCanary + deterministic decision test
P1  Candidate scoring / score breakdown docs/data
P1  Objective/cover/hazard scenarios
P1  Overwatch HOLD/FIRE scenario
P1  2v2 autonomous smoke
P2  Team synergy water/electric
P2  dynamic cover/choke scenario
P2  TeamKnowledge/noise scenarios quando dipendenze verdi
P2  4v4 stress
P3  belief model / counterfactual / opponent model
```

---

# 37. Principio finale

Il risultato desiderato non è "un bot che vince".

È un bot che:

- usa le stesse regole del giocatore;
- sa solo ciò che la squadra può sapere;
- costruisce piani coerenti con RefactorTactics;
- usa obiettivi, ambiente, cover, Overwatch e combo;
- può essere ingannato da informazione incompleta;
- è deterministico e testabile;
- spiega perché ha scelto un piano;
- cresce per strati senza richiedere una riscrittura completa.

La v0.1 deve privilegiare **intelligenza prevedibile, fair, leggibile e diagnosticabile** rispetto a sofisticazione prematura.
