# RefactorTactics — Consolidamento PRD, Source of Truth e integrazione delle decisioni recenti

## Prompt operativo per Claude Code

> **Scopo**  
> Usare questo documento come input per consolidare la documentazione, l'architettura, il backlog tecnico e la roadmap di **RefactorTactics** alla luce delle decisioni più recenti.  
> La repository deve diventare la fonte di verità; i vecchi PDR/PDF devono restare come storico quando superati.

---

# 1. Ruolo

Agisci come:

- Senior Unreal Engine 5 Developer;
- Gameplay/Network Engineer;
- Technical Game Designer;
- Software Architect;
- Technical Writer;
- QA/Automation Engineer;
- maintainer della documentazione e della roadmap.

Lavora direttamente nella repository **RefactorTactics**.

Prima di modificare file:

1. leggi `CLAUDE.md`, `AGENTS.md`, `README.md` e istruzioni locali, se presenti;
2. inventaria tutta la documentazione sotto `Docs/`, `docs/` e directory equivalenti;
3. analizza codice, test, issue/backlog e roadmap già presenti;
4. identifica quali documenti sono PDR storici, quali sono specifiche correnti e quali sono brainstorming;
5. non assumere che un PDF più vecchio sia ancora normativo;
6. segnala esplicitamente i conflitti prima di risolverli.

---

# 2. Obiettivo

La documentazione del progetto è cambiata rapidamente.

I PDR iniziali contengono ancora molte fondamenta valide, ma alcune specifiche sono state sostituite o ampliate.

Devi:

1. consolidare i PDR esistenti;
2. integrare le decisioni recenti;
3. eliminare duplicazioni;
4. marcare chiaramente ciò che è superato;
5. distinguere decisioni approvate da proposte;
6. rendere la documentazione Markdown della repository la **source of truth**;
7. allineare roadmap, acceptance criteria, test e backlog;
8. identificare il codice che dovrà essere adattato alle nuove specifiche;
9. proporre issue implementative granulari;
10. non fare feature creep nella v0.1.

---

# 3. Regola di precedenza documentale

Applicare questa gerarchia:

1. decisioni esplicite e recenti del progetto;
2. ADR approvati;
3. specifiche correnti del vertical slice;
4. documentazione Markdown corrente;
5. PDR precedenti;
6. brainstorming / research / proposte;
7. fonti esterne.

Quando due fonti sono in conflitto:

- non scegliere silenziosamente;
- registra il conflitto;
- usa la decisione più recente solo se realmente determinabile;
- altrimenti crea una `OPEN DECISION`;
- conserva la specifica precedente come storico.

---

# 4. Fondamenta che restano valide

Le seguenti decisioni architetturali restano centrali.

## 4.1 Simulazione autorevole e deterministica

Pipeline concettuale:

```text
Canonical Intents
      |
      v
Immutable Snapshot
      |
      v
Deterministic Resolver
      |
      v
Canonical TurnLog
      |
      v
New Logical State
      |
      v
Presentation / UI / VFX / Replay
```

Le animazioni non decidono:

- collisioni;
- danni;
- movement outcome;
- reaction;
- objective;
- status;
- propagazione ambientale.

Stesso:

```text
Snapshot
+ RulesVersion
+ ContentManifestHash
+ ResolverConfigHash
+ Seed
```

deve produrre lo stesso:

```text
Logical State
TurnLog
StateHash
LogHash
```

Non dipendere da:

- frame rate;
- `Tick`;
- timing montage;
- ordine implicito di `TMap` / `TSet`;
- packet timing;
- VFX;
- random globale.

---

## 4.2 Confine C++ / Blueprint / GAS

### C++

Autorità per:

- simulazione;
- networking;
- serializzazione;
- pathfinding;
- validation;
- regole competitive;
- snapshot;
- resolver;
- TurnLog;
- privacy;
- determinismo.

### Blueprint / Data

Per:

- configurazione;
- UI;
- animazioni;
- VFX;
- presentazione;
- prototipi;
- varianti di contenuto.

### GAS

GAS gestisce:

- ownership delle ability;
- costi;
- cooldown;
- attributi;
- Gameplay Tags;
- Gameplay Effects / Cue.

GAS **non è l'autorità del simulatore**.

Il resolver determina l'esito competitivo.

---

## 4.3 Dati e contenuti

Continuare con:

- Primary Data Assets;
- stable IDs;
- Definition Version;
- RulesVersion;
- ContentManifestHash;
- Gameplay Tags governati;
- validator;
- soft references;
- manifest di match;
- hash riproducibili.

Il modding pubblico resta fuori scope della v0.1.

---

# 5. Turn structure aggiornata

La struttura principale ispirata ai principi di Atlas Reactor è:

```text
Decision / Planning
        |
        v
Prep
        |
        v
Dash
        |
        v
Blast
        |
        v
Move
```

## Regola fondamentale

Il normale `Move` è sempre l'ultima fase volontaria del turno.

Non progettare sequenze arbitrarie:

```text
Move -> Attack
```

Dash, blink, leap e spostamenti speciali possono accadere prima solo perché appartengono a una fase specifica.

La documentazione che descrive genericamente:

```text
Movement + Action
```

senza questa struttura deve essere aggiornata o marcata come precedente.

---

# 6. Planning

Il Planning è simultaneo e privato tra squadre.

Il giocatore prepara il piano prima della Resolution.

Gli alleati possono condividere:

- path;
- destinazione;
- action/ability;
- target;
- AoE;
- facing/direction;
- label;
- Ready;
- eventuali warning derivati esclusivamente da dati leciti.

Il client avversario non deve ricevere questi dati.

La UI del planning non può utilizzare gli intenti nemici nascosti anche se il server li conosce.

---

# 7. Action Ghosts

Introdurre formalmente il concetto di **Action Ghost**.

Durante Planning la UI deve poter rappresentare lo stato previsto dell'unità nei diversi momenti/fasi.

Un Action Ghost può mostrare:

- posizione prevista;
- orientamento;
- Dash;
- posizione dopo Dash;
- attacco;
- AoE;
- direzione;
- stance;
- Move finale;
- effetto sulla mappa se deterministico rispetto al piano alleato.

Gli Action Ghost devono:

- mostrare chiaramente la fase cui appartengono;
- distinguere Confermato / Previsto / Incerto;
- non mostrare futuro nemico segreto;
- aiutare a leggere combo e collisioni;
- essere presentation-only.

---

# 8. Fast Action e Fast Reaction

Le decision window durante Resolution non sono una seconda fase di Planning.

Usare un sistema tecnico comune di Decision Window, ma distinguere semanticamente:

## Fast Action

Decisione rapida nata come continuazione di una propria azione.

Esempio:

```text
Ability resolves
   |
   v
Fast Action
   |
   +--> Option A
   +--> Option B
```

## Fast Reaction

Decisione rapida causata da un trigger esterno.

Esempio:

```text
Enemy enters controlled area
   |
   v
Reaction Opportunity
   |
   v
Fast Reaction
```

Baseline corrente:

```text
FastReactionDuration = 3.0 s
```

La finestra deve avere poche opzioni e non trasformarsi in un micro-planning complesso.

Durante una decision window:

- la simulazione autorevole è ferma su un decision boundary;
- la presentazione può continuare in slow motion;
- il countdown è real-time;
- il risultato logico non dipende dalla velocità della presentazione.

---

# 9. Overwatch come azione generale

Overwatch non deve essere una skill hard-coded di un singolo eroe.

È un caso concreto di un sistema generale di reaction.

Modello:

```text
Reaction Definition
+ Player Intent
+ Snapshot
+ Trigger
= Reaction Opportunity
        |
        v
Fast Reaction
        |
   +----+----+
   |         |
 FIRE      HOLD
```

## Overwatch MVP

Baseline:

- scelta durante Planning;
- area/cone direzionale;
- facing rilevante;
- range configurabile;
- richiede LOS;
- richiede Detection;
- trigger valutato per micro-step;
- 1 charge;
- `FIRE` consuma la charge;
- `HOLD` perde solo quella opportunity;
- timeout = `HOLD`;
- più opportunity possibili finché la reaction resta armata;
- prompt limit configurabile;
- nessun interrupt annidato nell'MVP;
- può essere cancellata da KO/Stun/disarm/etc.;
- ordine deterministico.

## Trigger simultanei

Se più target triggerano nello stesso micro-step:

NON:

```text
Prompt A
Prompt B
```

basandosi su ordine di iterazione.

Creare una singola opportunity:

```text
[FIRE A]
[FIRE B]
[HOLD]
```

## Anti-information leak

Non rivelare:

- trigger futuri;
- percorsi futuri;
- destinazioni future;
- quantità di future opportunity;
- intenti nemici.

Il client vede solo informazioni valide al decision boundary corrente.

---

# 10. Delayed / predictive actions e tactical bets

Formalizzare le azioni che rappresentano una scommessa sul comportamento nemico.

Possibili trigger/previsioni:

- attraversamento di una cella;
- ingresso in area;
- uscita da area;
- uso di Move;
- uso di Dash;
- attacco;
- uso ability;
- fine movimento;
- permanenza in una posizione;
- apertura di una porta;
- interazione con un oggetto;
- cambio di fase.

Esempi:

- trap;
- mine;
- interception shot;
- ambush;
- delayed AoE;
- opportunity attack;
- counter;
- guard;
- conditional hack;
- environmental trigger.

Il piano viene scelto durante Planning.

Il giocatore non può rivederlo dopo aver osservato il comportamento avversario, salvo Decision Window esplicite.

Questa famiglia di meccaniche deve essere trattata come una delle caratteristiche distintive di RefactorTactics.

---

# 11. Griglia esagonale

La griglia corrente è esagonale.

`FRTCellId` mantiene:

```text
X
Y
Layer
```

ma:

```text
X / Y = coordinate assiali esagonali
```

`Layer` distingue:

- ground;
- bridge;
- roof;
- tunnel;
- livelli sovrapposti;
- altri piani logici.

Qualunque documentazione che descriva adiacenze `4-way` deve essere marcata `SUPERSEDED`.

Il pathfinding autorevole usa A* sul grafo esagonale/multilivello.

---

# 12. Mappa come grafo tattico 3D

La mappa è un grafo.

```text
Node = valid tactical position
Edge = valid tactical transition
```

Le celle sono dati compatti centralizzati.

Non creare un Actor per ogni cella.

Gli Actor restano principalmente per:

- rendering;
- collisioni;
- elementi visibili/interattivi;
- bridge/door/elevator presentation;
- unità.

Gli archi sono dati di prima classe.

Una porta, un ponte o un ascensore modifica una transizione del grafo, non soltanto una mesh.

---

# 13. Visibility, Detection e Team Knowledge

La squadra non vede automaticamente tutta la mappa.

Separare:

```text
LOS
Detection
Visibility
Awareness / Team Knowledge
```

## LOS

Geometria:

> esiste una linea visiva?

## Detection

Percezione:

> la squadra è in grado di rilevare la presenza?

## Visibility

Informazione visuale effettivamente conosciuta.

## Team Knowledge

Modello canonico dell'informazione disponibile alla squadra.

Pipeline:

```text
Authoritative Simulation
        |
        v
Perception Systems
        |
        v
Team Knowledge
        |
        v
Sanitized Replication
        |
        v
Client ViewModel
        |
        v
UI
```

Team Knowledge dovrà integrare almeno:

- visual sighting;
- stealth/detection;
- acoustic information;
- last known position;
- observed reactions;
- informazioni ambientali osservate.

---

# 14. Rumore come sistema informativo

Il rumore è una seconda forma di percezione, non un semplice debuff.

Principio:

> Non vedere un nemico non significa non avere informazioni sulla sua presenza.

Ogni evento può produrre `NoiseEvent`.

Esempi:

- Walk;
- Sprint;
- Dash;
- sparo;
- esplosione;
- porta;
- crollo;
- acqua;
- ghiaccio;
- elettricità;
- macchinari;
- allarmi.

La propagazione deve usare il grafo tattico.

Non usare una `SphereOverlap` come autorità competitiva.

Formula concettuale:

```text
ReceivedNoise =
SourceIntensity
- DistanceCost
- AcousticOcclusion
- AmbientMask
+ SurfaceModifiers
```

Usare valori interi/data-driven.

## Livelli di informazione

Possibili risultati:

1. direzione;
2. area larga;
3. area stretta;
4. cella precisa;
5. identificazione.

Non usare RNG nascosto come meccanica base.

Preferire:

```text
ReceivedNoise >= HearingThreshold
```

## Acoustic masking

Una sorgente rumorosa può mascherarne una debole.

Questo consente tattiche come:

```text
Explosion
+
Assassin Sprint
```

## Decoy

Supportare in futuro rumori falsi realmente generati dalla simulazione.

Il giocatore riceve un evento sonoro vero, ma può interpretarne erroneamente la causa.

---

# 15. Ambiente sistemico

La mappa deve essere un sistema strategico.

Elementi principali:

- acqua;
- fuoco;
- elettricità;
- ghiaccio;
- vapore;
- fumo;
- metallo;
- cover;
- porte;
- ponti;
- tunnel;
- ascensori;
- hazard;
- superfici;
- quota.

Interazioni desiderate:

```text
Water + Electricity -> Electrified Water

Ice + Fire -> Water / Steam

Water + Fire -> Steam / extinguish

Smoke / Steam -> visibility change

Ice + fast movement -> altered movement / slip rule

Metal + Electricity -> propagation modifier
```

Ogni effetto:

- deterministico;
- data-driven;
- spiegabile;
- registrato nel TurnLog;
- testabile.

---

# 16. UI: Confermato, Previsto, Incerto

Questa distinzione resta obbligatoria.

## Confermato

Deriva da:

- stato pubblico;
- informazione realmente osservata;
- regola deterministica già nota.

## Previsto

Deriva da:

- piano proprio;
- intenti alleati;
- regole deterministiche;
- condizioni note.

## Incerto

Dipende da:

- azione avversaria nascosta;
- movimento nemico futuro;
- target futuro;
- detection incompleta;
- conflitto non ancora risolto.

La UI non deve mai trasformare dati server-only in informazione visuale avversaria.

---

# 17. Networking e privacy

Il principio resta:

```text
Client proposes
Server validates
Server applies
```

## Canonical intent

`CanonicalIntentStore` completo:

```text
SERVER ONLY
```

## Team preview

```text
TEAM ONLY
```

Preview:

- sequenziata;
- preferibilmente unreliable;
- target 8–12 Hz;
- sanitized DTO;
- ownership/rate-limit validation.

## Commit

- reliable;
- idempotente;
- validato;
- versionato.

## Reaction responses

- owner/team authorized only;
- reliable;
- server validated.

## Divieti

Non mettere planning privato in:

- global GameState replicated;
- global PlayerState fields;
- AlwaysRelevant Actors;
- globally replicated arrays;
- public logs prematuri.

Il fatto che una UI non mostri un dato NON significa che il dato sia sicuro.

---

# 18. Personaggi e roster

Esistono roster differenti nei PDR storici.

NON scegliere automaticamente il roster di un vecchio documento.

Procedura:

1. trova il roster più recente esplicitamente approvato nella repository;
2. confrontalo con:
   - Aegis;
   - Nyx;
   - Drift;
   - Vex;
   - eventuali roster successivi;
3. registra conflitti;
4. non inventare una scelta se manca una decisione umana;
5. marca come `OPEN DECISION` quando necessario.

Il vertical slice deve comunque coprire almeno:

- attacco lineare;
- AoE;
- Dash;
- defense/counter;
- acqua;
- elettricità;
- modifica cover/arco;
- interazione con environment.

Varianti e progressione devono essere orizzontali, non pure upgrade.

---

# 19. Unità ausiliarie

Il sistema deve essere predisposto a unità aggiuntive semplici generate o controllate da personaggi.

Esempi:

- pet;
- summon;
- drone;
- turret;
- gadget mobile;
- decoy;
- deployable.

Principi:

- `StableUnitId`;
- owner/source esplicito;
- team esplicito;
- ciclo di vita deterministico;
- AI/policy semplice;
- stessi sistemi di cella/occupancy/visibility/noise quando applicabili;
- no simulatore parallelo.

La v0.1 deve usare questi sistemi solo se necessari al roster definitivo.

Non ampliare lo scope solo perché l'architettura li supporta.

---

# 20. Testing automatico — requisito aggiornato

Integrare nella roadmap un **RT Automated Scenario Test Harness**.

Obiettivo:

```text
Scenario testuale
      |
      v
Unreal
      |
      v
Planning automatico
      |
      v
Ready
      |
      v
Snapshot
      |
      v
Real Resolver
      |
      v
TurnLog
      |
      v
Assertions
      |
      v
result.json
```

Il test deve usare il percorso gameplay reale.

NON:

```text
Test -> SetActorLocation
Test -> ApplyDamage
```

se il gameplay reale usa Intent/Snapshot/Resolver.

## Modalità previste

### Visual

Premo Play in `L_DevSandbox` e vedo il test svolgersi senza input umano.

### Fast

Stessa simulazione, presentazione ridotta e Decision Window auto-risolte.

### Headless

Command line / Automation / CI.

## Primo scope

Implementare per gradi:

1. scenario testuale;
2. Test Director;
3. AutoRun;
4. due unità;
5. movement intent;
6. AutoReady;
7. snapshot reale;
8. movement resolver reale;
9. TurnLog reale;
10. assertion base;
11. `result.json`;
12. PASS/FAIL.

## Output strutturato

Preferire:

```text
Saved/RTTests/<Scenario>/<RunId>/
    result.json
    turnlog.jsonl
    state_initial.json
    state_final.json
```

almeno:

```text
result.json
turnlog.jsonl
```

Il report deve permettere a Claude Code di diagnosticare i fail senza leggere migliaia di righe di log.

---

# 21. Durata match e formati

I vecchi numeri relativi a:

- 8 turni;
- 30 secondi;
- interrupt 5 secondi;
- durata 20–30 minuti;

non devono essere considerati automaticamente definitivi.

Il target generale corrente è:

- match principali entro circa 30–45 minuti max;
- mappe iniziali compatte;
- possibilità futura di mappe/modalità più ampie e più lunghe.

Devono essere riesaminati:

- numero massimo turni;
- planning duration;
- resolution duration;
- end-turn duration;
- numero medio di Fast Reaction;
- impatto del formato 2v2 / 3v3 / 4v4.

Registrare questi parametri nel modello di bilanciamento, non duplicarli come hard rule in vari documenti.

---

# 22. Formati squadra

Distinguere chiaramente:

## Vertical slice

Scenario principalmente 2v2 per contenere lo scope e testare i sistemi.

## Formato principale target

3v3.

## Scenario 4v4

Deve essere supportabile dall'architettura e usato per stress/design test, ma non deve automaticamente aumentare lo scope della v0.1.

Annotare chiaramente dove un documento confonde:

- numero di giocatori;
- numero di unità;
- numero di personaggi controllati da ogni giocatore.

---

# 23. Matrici di bilanciamento

Conservare una fonte strutturata per i valori configurabili.

Categorie:

- global match;
- movement;
- visibility;
- noise;
- surfaces;
- environment;
- abilities;
- characters;
- equipment;
- reactions;
- objectives;
- auxiliary units;
- map transitions.

Separare:

```text
DESIGN RULE
```

da:

```text
BALANCE VALUE
```

Esempio:

Design rule:

> Fast Reaction deve essere breve.

Balance value:

```text
FastReactionDuration = 3.0
```

Ogni parametro dovrebbe avere:

- ID;
- valore;
- unità;
- scope;
- status;
- source;
- min/max;
- note;
- versione.

---

# 24. Struttura documentale target

Portare gradualmente la repository verso una struttura simile:

```text
docs/
|
+-- README.md
+-- DOC_CONFLICT_MATRIX.md
+-- OPEN_DECISIONS.md
+-- CHANGELOG_DOCUMENTATION.md
|
+-- product/
|   +-- GAME_VISION.md
|   +-- MATCH_STRUCTURE.md
|   +-- VERTICAL_SLICE_V0_1.md
|
+-- gameplay/
|   +-- TURN_STRUCTURE.md
|   +-- ACTION_SYSTEM.md
|   +-- REACTIONS.md
|   +-- OVERWATCH.md
|   +-- DELAYED_ACTIONS.md
|   +-- MOVEMENT.md
|   +-- VISIBILITY_FOW.md
|   +-- NOISE_PERCEPTION.md
|   +-- ENVIRONMENT.md
|   +-- AUXILIARY_UNITS.md
|   +-- CHARACTERS.md
|
+-- technical/
|   +-- ARCHITECTURE.md
|   +-- SIMULATION.md
|   +-- NETWORKING_PRIVACY.md
|   +-- MAP_GRAPH.md
|   +-- PATHFINDING.md
|   +-- ABILITIES_GAS.md
|   +-- DATA_CONTENT.md
|   +-- UI_UX.md
|   +-- AUTOMATED_TESTING.md
|
+-- balance/
|   +-- BALANCE_MODEL.md
|   +-- PARAMETERS.md
|
+-- roadmap/
|   +-- MILESTONES.md
|   +-- V0_1_SCOPE.md
|
+-- decisions/
|
+-- archive/
    +-- pdr-v0.1/
```

Non creare file vuoti solo per soddisfare questa struttura.

Riutilizzare file esistenti quando possibile.

---

# 25. DOC_CONFLICT_MATRIX

Creare o aggiornare:

```text
docs/DOC_CONFLICT_MATRIX.md
```

Formato:

| Area | Specifica precedente | Specifica corrente | Fonte | Stato | Azione |
|---|---|---|---|---|---|

Stati:

- `CONFIRMED`
- `SUPERSEDED`
- `CONFLICT`
- `OPEN`
- `DUPLICATE`

Controllare obbligatoriamente:

- square/4-way vs hex;
- vecchio movement+action vs Prep/Dash/Blast/Move;
- Move come ultima fase;
- interrupt 5s vs Fast Reaction 3s;
- reaction generiche;
- Overwatch generica;
- Action Ghosts;
- delayed/predictive actions;
- trap / tactical bets;
- roster differenti;
- Fog of War;
- Team Knowledge;
- noise;
- auxiliary units;
- durata match;
- turn cap;
- planning duration;
- 2v2 / 3v3 / 4v4;
- GAS authority;
- multilivello;
- testing automatico;
- roadmap.

---

# 26. ADR

Creare ADR soltanto per decisioni davvero consolidate.

Candidati:

```text
ADR-001-HEX-GRID.md
ADR-002-TURN-PHASE-ORDER.md
ADR-003-DETERMINISTIC-SNAPSHOT.md
ADR-004-TEAM-ONLY-INTENTS.md
ADR-005-FAST-DECISION-WINDOWS.md
ADR-006-GENERIC-OVERWATCH.md
ADR-007-TEAM-KNOWLEDGE.md
ADR-008-AUTOMATED-SCENARIO-TESTING.md
```

Formato:

```text
# ADR-XXX — Title

Status:
Date:

## Context

## Decision

## Consequences

## Alternatives considered

## Supersedes
```

Non inventare una decisione per riempire un ADR.

---

# 27. Roadmap impact analysis

Prima di modificare la roadmap, creare una sezione di analisi.

Formato:

| Decisione | Milestone interessata | Impatto | Cambio raccomandato |
|---|---|---|---|

Analizzare almeno:

- hex grid;
- turn phase order;
- Action Ghosts;
- Fast Reaction;
- Overwatch;
- delayed/predictive actions;
- Fog of War;
- Team Knowledge;
- noise;
- auxiliary units;
- automated testing;
- privacy test;
- multiplayer scaling.

---

# 28. Roadmap proposta da preservare come base

La roadmap storica F0–F6 resta un utile punto di partenza:

```text
F0 Fondazioni
F1 Rete privata
F2 Abilities
F3 Mappa multilivello
F4 Vertical Slice
F5 Dedicated Server
F6 Beta Systems
```

Non sostituirla automaticamente.

Aggiornarla sulla base dei nuovi impatti.

Possibile principio:

### F0

- hex grid corretta;
- deterministic local loop;
- movement;
- snapshot;
- TurnLog;
- Automated Scenario Test Harness minimo.

### F1

- team-only intents;
- privacy/canary tests;
- sequence/rate limit;
- listen server.

### F2

- ability intent model;
- Prep/Dash/Blast/Move;
- Fast Decision infrastructure;
- Overwatch base;
- GAS mirror.

### F3

- multilivello;
- visibility/LOS/detection;
- Team Knowledge;
- environment;
- noise baseline.

### F4

- vertical slice completo;
- Action Ghosts;
- reactions;
- environment interactions;
- bots/test agents;
- objective;
- UI leggibile.

Questa è una proposta di analisi, non una decisione da imporre senza confronto con la roadmap reale della repository.

---

# 29. Issue/backlog da produrre

Dopo il consolidamento, generare una proposta di issue granulari.

Ogni issue deve includere:

- titolo;
- milestone;
- obiettivo;
- motivazione;
- scope;
- out-of-scope;
- acceptance criteria;
- test;
- privacy impact;
- determinism impact;
- documentation impact;
- dipendenze.

Categorie candidate:

```text
docs
architecture
turn
planning
map
pathfinding
visibility
noise
reactions
overwatch
environment
testing
network
ui
data
balance
```

Non creare automaticamente decine di issue duplicate se esistono già.

Confrontare prima il backlog esistente.

---

# 30. Definition of Done

Una feature competitiva è Done solo se:

1. funziona nel modello server/client previsto;
2. non espone dati non autorizzati;
3. produce TurnLog/reason codes sufficienti;
4. ha test automatico;
5. ha debug/log adeguato;
6. rispetta o misura i performance budget;
7. funziona in packaged build;
8. documentazione aggiornata;
9. non introduce divergenza replay;
10. non duplica la logica del simulatore.

---

# 31. Procedura operativa per Claude

## Fase A — Inventory

Produrre elenco di:

- PDR;
- docs correnti;
- ADR;
- test;
- roadmap;
- issue/backlog;
- file di balance;
- implementazione correlata.

Classificare:

- CURRENT;
- PDR;
- PROPOSAL;
- RESEARCH;
- ARCHIVE;
- UNKNOWN.

Non modificare ancora.

---

## Fase B — Conflict matrix

Creare/aggiornare:

```text
DOC_CONFLICT_MATRIX.md
```

Prima di fare grandi modifiche, mostrare un summary dei conflitti.

---

## Fase C — Canonical model

Definire quali documenti diventano source of truth.

Evitare che la stessa regola sia definita in più file.

Un documento deve essere owner del concetto.

Gli altri devono linkarlo.

---

## Fase D — Migration

Aggiornare la documentazione.

I vecchi PDR:

- non cancellarli;
- archiviarli o marcarli chiaramente;
- mantenerli disponibili come storico.

Markdown corrente = source of truth.

PDF = export/storico.

---

## Fase E — Code impact

Senza fare refactor non richiesti, identificare:

- classi obsolete;
- naming incoerente;
- TODO;
- sistemi da estendere;
- test mancanti;
- codice che assume square grid;
- codice che assume vecchio phase order;
- codice che tratta Overwatch come special case;
- codice senza Team Knowledge;
- codice senza structured test output.

Produrre un report.

---

## Fase F — Roadmap e issues

Aggiornare/integrare:

- milestone;
- backlog;
- dipendenze;
- acceptance criteria;
- test gates.

---

## Fase G — Validation

Controllare:

- broken Markdown links;
- file duplicati;
- link a documenti archiviati usati come canonical;
- vecchie regole 4-way;
- vecchi interrupt 5s;
- vecchio phase order;
- privacy regressions;
- naming incoerente;
- balance value duplicati;
- test che bypassano il resolver.

---

# 32. Output finale richiesto

Al termine mostrare:

1. documenti analizzati;
2. documenti canonici;
3. file creati;
4. file modificati;
5. file archiviati;
6. conflitti risolti;
7. `OPEN DECISIONS`;
8. impatti sul codice;
9. issue nuove proposte;
10. issue esistenti da modificare;
11. roadmap aggiornata/proposta;
12. test mancanti;
13. rischi principali;
14. prossimi tre passi consigliati.

---

# 33. Regola critica

Non trasformare una proposta in una decisione canonica solo perché sembra buona.

Quando l'informazione non è sufficiente:

```text
OPEN DECISION
```

Quando una vecchia specifica è stata chiaramente sostituita:

```text
SUPERSEDED
```

Quando più documenti dicono la stessa cosa:

```text
DUPLICATE -> consolidate
```

---

# 34. Risultato desiderato

Dopo questa attività, un nuovo sviluppatore deve poter aprire:

```text
docs/README.md
```

e capire rapidamente:

- cos'è RefactorTactics;
- qual è il gameplay loop;
- come funziona il turno;
- quali dati sono privati;
- come funziona il resolver;
- qual è la mappa;
- come funzionano visibility e noise;
- come funzionano reaction e Overwatch;
- qual è lo scope v0.1;
- quali decisioni sono definitive;
- quali sono ancora aperte;
- quali documenti sono solo storico;
- quali test verificano le regole.

**La repository deve diventare la fonte di verità del progetto.**
