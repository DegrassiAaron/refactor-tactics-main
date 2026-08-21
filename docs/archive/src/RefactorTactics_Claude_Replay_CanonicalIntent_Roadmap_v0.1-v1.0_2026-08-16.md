# RefactorTactics — Handoff Claude
## Replay, Canonical Intent, Decisioni Runtime, Privacy Server/Client e Roadmap v0.1 → v1.0

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

> `HISTORICAL` · **Materiale NON autorevole**, archiviato il **2026-08-17** dopo il consolidamento.
> **Non si applica**: si legge per sapere da dove viene una decisione. Le fonti autorevoli restano
> [`adr-0009-replay-logico-canonico.md`](../../decisions/adr-0009-replay-logico-canonico.md),
> [`adr-0004-finestre-di-reazione.md`](../../decisions/adr-0004-finestre-di-reazione.md) e
> `feature-registry.yaml`.
>
> **Triage**: [`replay-canonical-intent-triage-2026-08-17.md`](../../roadmap/plans/replay-canonical-intent-triage-2026-08-17.md)
> — matrice delle otto proposte: `PROPOSED 1 · ALREADY DECIDED 2 · OPEN ISSUE 1 · DEFERRED 3 · REJECTED 1`.
>
> **Recepito**: [#1118](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1118) (`P4`, la
> risposta di reazione separata dalla sua ragione, sub-issue di `E14`) ·
> [#1119](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1119) (`RCI-1`…`RCI-4` sul
> Canonical Intent, sub-issue di `E40`) · commento di consolidamento su
> [#780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/780), che il §26 chiedeva
> esplicitamente. **Nessun `D-nnn` nuovo**: le due decisioni che proponeva come canoniche erano già prese.
>
> 🔴 **Più della metà della sua roadmap `R0`–`R6` descriveva lavoro già fatto o già pianificato.** `#165`
> è `CLOSED`; `#886` ha un DoD riscritto da uno spec panel del 2026-08-17; `#542` ha un design approvato
> in [`cp153b-decision-provider-design-2026-08-16.md`](../../roadmap/plans/cp153b-decision-provider-design-2026-08-16.md).
> Il documento apre chiedendo di non fidarsi del proprio HEAD (`557fdb88`), e aveva ragione.
>
> ✅ **La sua parte più accurata è il §40**, ed è quella che nessuno aveva ancora eseguito: i nove test che
> propone sono assenti **tutti e nove**, mentre i quattro che dà per esistenti ci sono **tutti e quattro**
> — e non sono soli, il repository ne dichiara **48** con prefisso `Replay.`.
>
> ⚠️ **Ciò che non è entrato**: il §35 (roadmap `v0.2`→`v1.0`) era una seconda copia della roadmap per
> release, e gli owner sono [`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) e le epic
> `E36`–`E45`, tutte già aperte; il §24 (tabella di distribuzione server/client) non contraddice niente e
> non è azionabile finché la rete è `E40`; il §10 assegna i Reaction Profile per eroe, che è contenuto di
> `#314` e sarebbe scope creep sulla `v0.1`.

**Data handoff:** 2026-08-16  
**Repo di riferimento:** `DegrassiAaron/refactor-tactics-main`  
**HEAD osservato durante questa chat:** `557fdb88eabc60587b7bef66d51a06a13f01ff42`

> ⚠️ **Regola operativa per Claude:** prima di modificare qualunque file, issue, roadmap o wiki, rieseguire un audit di `main`, delle issue aperte/chiuse e del Feature Registry. Questo handoff consolida le decisioni e le proposte emerse nella chat, ma **non sostituisce lo stato reale del repository** se nel frattempo è cambiato.

---

# 1. Obiettivo di questo handoff

Consolidare in un unico documento le decisioni e le proposte emerse durante il focus su:

- Replay logico;
- TurnLog canonico;
- decisioni live durante Resolution;
- Overwatch / Fast Reaction / Brace / Reaction Clash;
- Predictive Action;
- `OpportunityId`;
- Replay Verifier;
- persistenza degli input canonici;
- `FRTCanonicalIntent`;
- confine fra proposal, canonical intent, snapshot, TurnLog e presentation;
- responsabilità server/client;
- privacy degli intenti;
- relazione con Scenario Harness;
- relazione con `DecisionProvider`;
- futura architettura rete;
- roadmap dettagliata fino alla **v0.1**;
- roadmap più generale fino alla **v1.0**.

Claude deve usare questo documento per:

1. verificare lo stato reale del repository;
2. riconciliare documentazione, ADR/Decision Log, roadmap, Feature Registry, Scenario Map e Wiki;
3. evitare sistemi paralleli;
4. creare o aggiornare Epic/issue **solo dopo aver cercato quelle esistenti**;
5. collegare issue, feature, scenari e documentazione;
6. proporre una roadmap coerente fino alla v1.0.

---

# 2. Principi di progetto da NON violare

1. Il client propone; il server valida e applica.
2. Il server è l'autorità della simulazione.
3. Snapshot, resolver e stato logico non dipendono da animazioni, frame rate o timing UI.
4. Gli intenti nemici non devono mai essere inviati ai client avversari.
5. Nascondere un dato graficamente **non** è privacy.
6. Stesso snapshot + stesse regole + stessa versione + stesso seed + stessi input canonici ⇒ stesso risultato.
7. C++ governa simulazione, rete, serializzazione, pathfinding e validazione.
8. Blueprint/UMG governa configurazione, UI, animazioni, VFX e presentazione.
9. Il TurnLog è una traccia autoritativa degli esiti, non un secondo simulatore.
10. Replay Player e Replay Verifier sono prodotti diversi.
11. Non creare sistemi paralleli per Scenario, Replay, Bot Simulation o Test.
12. Stable ID, ordinamenti deterministici e hash espliciti sono obbligatori.
13. Il formato persistente va versionato senza rompere i replay storici.
14. Le scelte del giocatore e gli esiti del resolver sono concetti distinti.
15. Il wall-clock non deve entrare negli hash deterministici.

---

# 3. Stato Replay canonico già deciso nel progetto

Owner principale: `docs/decisions/adr-0009-replay-logico-canonico.md`.

## Replay Player

Autorità: **traccia archiviata**.

Non deve ricalcolare collisioni, danni, path legality, reaction, KO o targeting.

Garanzia:

> «vedi quello che è successo».

## Replay Verifier

Autorità: **resolver**.

Ri-simula, confronta con la traccia e produce un verdetto; non presenta.

Garanzia:

> «quello che è successo è riproducibile dalle regole».

Test/guardrail già previsti o esistenti:

- `Replay.Player.RunsWithoutResolver`
- `Replay.Player.RejectsIncompatibleArchive`
- `Replay.Verifier.ReportsFirstDivergence`
- `Replay.Verifier.ResimulationIsDeterministic`

---

# 4. Replay Archive — stato attuale

Forma implementata:

```text
Replays/
  <MatchId>/
    match.rtmanifest
    turn-001.rtlog
    turn-002.rtlog
```

Il manifest include concetti come:

- `MatchId`
- `FormatId`
- topologia
- hash ordinati per turno
- checksum finale dello stato
- esito
- durata wall-clock
- `bClosed`
- `TurnCount`

Decisioni già prese:

- `MatchId` è un `FGuid`;
- è fuori dagli hash;
- wall-clock è metadata, non input deterministico;
- il recorder scrive durante il match;
- un crash può lasciare un archivio parziale riconoscibile;
- il recorder riusa il serializer TurnLog;
- `ContentManifestHash` / `RulesVersion` sono stati rinviati a una release successiva.

---

# 5. TurnLog: ruolo corretto

Il TurnLog deve contenere:

1. **decisioni live** che entrano durante la Resolution;
2. **esiti** prodotti dal resolver.

Non deve diventare:

- dump dello stato;
- duplicato dello snapshot;
- log UI;
- copia del client proposal;
- contenitore di ogni click;
- secondo input file generalista.

Separazione:

```text
A. INPUT DI PLANNING
   cosa era stato accettato prima della Resolution

B. DECISIONE LIVE
   cosa è stato scelto a un Decision Boundary

C. ESITO
   cosa il resolver ha realmente prodotto
```

Regola:

```text
A → Canonical Intent
B → TurnLog / ReactionDecision
C → TurnLog
```

---

# 6. TurnLog attuale — ReactionDecision

Il codice ha già categorie tra cui:

```text
Move
Combat
Fallback
Reaction
Environment
Facing
Predictive
ReactionDecision
```

`ERTReactionDecisionOutcome` attuale comprende:

```text
FireChosen
HoldChosen
HoldTimeout
HoldNoDecider
HoldRejected
HoldImmediate
```

Il runtime possiede anche:

```cpp
FRTReactionDecision
{
    FString Response;
    ERTReactionDecisionOutcome Outcome;
}
```

Distinzione:

```text
Response = COSA si applica
Outcome  = COME/PERCHÉ si è arrivati a quella risposta
```

Esempio:

```text
Response = HOLD
Outcome  = HoldChosen
```

vs:

```text
Response = HOLD
Outcome  = HoldTimeout
```

---

# 7. Issue replay-critical #886

**#886 — Le decisioni di reazione tornano dal TurnLog: il Verifier non le richiede**

Problema:

```text
partita reale:
Opportunity X
→ giocatore sceglie FIRE
→ movimento troncato

Verifier senza recorded decision:
Opportunity X
→ nessun human decider
→ HOLD
→ movimento continua
→ divergenza
```

Regola da implementare:

```text
OpportunityId
    ↓
cerca decisione registrata nel TurnLog
    ↓
se esiste:
    NON interrogare il live decider
    applica la decisione registrata
```

La recorded decision deve vincere sul decisore corrente.

Matching per `OpportunityId`, **non per ordine**.

Test richiesti/indicati:

- `Replay.Verifier.ReactionDecisionsComeFromTheTrace`
- `Replay.Verifier.RecordedResponseBeatsLiveDecider`
- `Replay.Verifier.ResimulationIsDeterministic` esteso a reaction decision reali

Non confondere #886 con `DecisionProvider` D-101/#542.

---

# 8. DecisionProvider #542

**#542 — Il seam dei DecisionProvider: chi decide restituisce decisioni, mai esiti**

Obiettivo:

unificare produttori di decisioni:

- scenario scriptato;
- bot;
- umano;
- replay;
- future policy di test.

Vincolo:

```text
DecisionProvider
    ↓
DECISIONE
```

mai:

```text
DecisionProvider
    ↓
ESITO PRECOTTO
```

Il resolver deve restare l'unico che produce gli esiti.

---

# 9. Predictive Action

Predictive Action è diversa da una reaction live.

```text
Planning:
"prevedo che passerai nella cella X"

Resolution:
boundary
→ TriggerMatched
oppure
→ PredictionWhiffed
```

Non apre input live.

Non serve `ReactionDecision`.

Il TurnLog ha già `ERTPredictiveOutcome`.

Direzione:

```text
Accepted Intent
  ActionId
  TargetCell
        ↓
Resolver
        ↓
Predictive outcome nel TurnLog
```

Wraith `InterceptShot` è già stato riclassificato in questo modello.

---

# 10. Brace e Reaction Profile

Decisione consolidata:

> `Brace` arma il Reaction Profile del personaggio.

Risposta universale:

```text
Hold Ground
```

Profilo base:

```text
AllowedResponses = [HOLD_GROUND]
```

→ nessun Decision Boundary.

Profili più ricchi possono produrre:

```text
HOLD_GROUND
GROUND
SIDESTEP:<Cell>
GLANCE:LEFT
GLANCE:RIGHT
```

Roster specificato:

- Gadget → `Profile.Grounding`
- Phase → `Profile.Sidestep`
- Wraith → `Profile.Glance`
- Riktor → solo profilo base

Reaction Profile deve essere data-driven, non ramo speciale nel resolver.

---

# 11. Limite del ReactionDecision attuale

`ERTReactionDecisionOutcome` è sufficiente per Overwatch ma è Overwatch-centric.

Evitare:

```cpp
enum
{
    OverwatchFire,
    HoldGround,
    Ground,
    Sidestep,
    GlanceLeft,
    GlanceRight,
    ...
}
```

Direzione proposta in chat:

```text
WHAT
CommittedResponse

WHY
DecisionOutcome
```

Esempio:

```text
CommittedResponse = SIDESTEP:(4,7,0)
DecisionOutcome   = Chosen
```

Questa è una **proposta architetturale**, non va dichiarata canonica senza audit/Decision.

Per replay v8 è possibile usare un adapter:

```text
FireChosen + Target 17
→ Response "FIRE:17"

Hold*
→ Response "HOLD"
```

Il Verifier dovrebbe lavorare in termini di `FRTReactionDecision`, non di special case Overwatch.

---

# 12. Reaction Clash

Issue:

**#314 — CP 14.7 Reaction Profile e Reaction Clash**

Definizione:

```text
due partecipanti
+
entrambi hanno >= 2 risposte legali
=
contested
```

`Contested` è **derivato**, non un campo `Type = Clash`.

Blind choice:

```text
A locka READ
    ↓
server-only pending

B locka SHIFT
    ↓
server-only pending

deadline fissa
    ↓
REVEAL
    ↓
TurnLog canonico:
A = READ
B = SHIFT
Result
```

Il reveal non deve anticipare quando entrambi lockano subito: il timing del lock è un potenziale canale informativo.

Possibile proposta futura:

```text
DecisionGroupId
```

per correlare decisioni multi-responder, senza usarlo per decidere se il confronto è contested.

---

# 13. Wall-clock vs simulation time

Distinguere:

```text
ReactionDecisionSeconds
```

da:

```text
CommittedResponse
```

Il primo è telemetry/pacing.

Il secondo è input canonico/replay.

Non mettere negli hash:

```text
LockedAt = 2.187 s
```

Il replay deve conoscere il risultato semantico:

```text
Timeout → HOLD
```

---

# 14. Canonical Intent — problema attuale

`FRTPlannedIntent` esiste già nel sistema privacy ma contiene anche:

```text
OwnerCell
TeamId
bAlive
bRevealed
FText ActionName
FText ReactionName
PlannedPath
PlannedWaypoints
...
```

Problemi per uso canonico/persistente:

- `OwnerCell` non è StableUnitId;
- `FText ActionName` non è Stable Action ID;
- `FText ReactionName` non è Stable Reaction ID;
- mescola intent, stato, privacy e presentation.

Nel runtime il piano è ancora distribuito su `ARTUnit`:

```text
PlannedAbilityIndex
PlannedAttackTarget pointer
PlannedAttackCell
PlannedCoverEdge
PlannedPath
PlannedWaypoints
PlannedFacing
PlannedDashAbility
PlannedDashCell
PlannedReactionAbility
PlannedReactionCondition
PlannedCleansePriority
...
```

Indici e pointer non sono adatti al replay persistente.

---

# 15. Proposta: `FRTCanonicalIntent`

Forma concettuale:

```cpp
struct FRTCanonicalIntent
{
    int32 UnitId;

    TArray<FRTCellId> MovePath;

    FName DashActionId;
    FRTCellId DashTarget;

    FName ActionId;

    ERTIntentTargetKind TargetKind;
    int32 TargetUnitId;
    FRTCellId TargetCell;
    ERTHexDirection TargetEdge;

    FName ReactionActionId;
    FRTDeclaredCondition ReactionCondition;

    bool bDeclaresFacing;
    ERTHexDirection DeclaredFacing;

    TArray<FGameplayTag> CleansePriority;
};
```

**Non è una firma definitiva.**

Claude deve verificare tipi reali, enum esistenti, UHT, serializer, versioning e Build.cs.

Regola:

> Dentro il Canonical Intent entrano solo le scelte competitive che il resolver consuma.

Non entrano:

- HP;
- Shield;
- Team;
- `bAlive`;
- `bRevealed`;
- current cell;
- current facing;
- Ready;
- countdown;
- ping;
- label;
- localizzazione;
- selection/hover;
- animation state.

---

# 16. Stable ID al boundary

Canonicalization deve trasformare:

```text
PlannedAbilityIndex
→ ActionId
```

```text
ARTUnit*
→ StableUnitId
```

```text
FText ReactionName
→ ReactionActionId
```

Regola:

```text
runtime pointers/indices
      ↓ canonicalization
stable ids
```

---

# 17. Waypoints vs Accepted Path

Distinguere:

```text
Waypoints
```

da:

```text
Accepted MovePath
```

Waypoints = autoria/UI.

Path = percorso accettato e consumato dal resolver.

Proposta replay:

```text
Local UI
   ↓
waypoints
   ↓
pathfinding
   ↓
server validation
   ↓
accepted MovePath
   ↓
Canonical Intent
```

Persistenza del replay: path accettato, non click del mouse.

---

# 18. Proposal vs Canonical Intent

Confine consigliato:

```text
Client Proposal
      ↓
Server Validation
      ↓
Canonical Intent
      ↓
Replay boundary
      ↓
Resolver
```

Il replay non dovrebbe partire dal pacchetto grezzo del client.

Networking, anti-cheat, input validation, pathfinding e resolver devono restare strati verificabili separatamente.

---

# 19. Relazione con `FRTScenarioIntent`

Lo Scenario Harness ha già una forma più vicina al dominio:

```text
FName Ability
FName Dash
FName Reaction
Move[]
Target
TargetCell
CoverEdge
Facing
```

Usa ID invece di indici.

Ma non riutilizzare direttamente `FRTScenarioIntent` come canonical runtime type perché usa ID locali allo scenario (`A1`, `B2`).

Direzione:

```text
FRTScenarioIntent
       ↓ normalize
FRTCanonicalIntent
       ↓
Resolver
```

In futuro:

```text
Human
Bot
Scenario
Replay
   ↓
DecisionProvider / validation
   ↓
FRTCanonicalIntent
   ↓
Resolver
```

---

# 20. Persistenza degli intenti per replay reale

L'archivio attuale salva manifest + TurnLog.

Per ri-simulare una partita reale in futuro serve anche l'input accettato.

Proposta:

```text
Replays/<MatchId>/
│
├── match.rtmanifest
│
├── turn-001.rtintent
├── turn-001.rtlog
│
├── turn-002.rtintent
├── turn-002.rtlog
└── ...
```

Semantica:

```text
rtintent = cosa era stato deciso
rtlog    = cosa è successo
```

Scrittura:

```text
Planning
   ↓
Commit
   ↓
Server validation
   ↓
Canonical Intent
   ↓
WRITE turn-N.rtintent
   ↓
Resolution
   ↓
WRITE turn-N.rtlog
```

Questa è una **proposta**, non ancora necessariamente canonica.

---

# 21. Non mettere gli intenti nel TurnLog

Evitare:

```text
TurnLog
- PlayerPlannedMove
- PlayerSelectedAction
- PlayerArmedReaction
- MoveStarted
- MoveBlocked
- Damage
```

Regola:

```text
RTIntent   = comando/input accettato
RTTurnLog  = decisioni runtime + risultato
```

ReactionDecision è diversa perché nasce durante Resolution.

---

# 22. Match Setup per replay verificabile

Anche `rtintent + rtlog` non bastano senza stato iniziale.

Proposta futura:

```text
Replays/<MatchId>/
│
├── match.rtmanifest
├── match.rtsetup
├── turn-001.rtintent
├── turn-001.rtlog
└── ...
```

`match.rtsetup` logico, non Actor serialization.

Possibili contenuti:

```text
FormatId
MapId / map identity
Map revision
Roster
StableUnitIds
HeroIds
Loadout
Initial positions
Initial facing
Initial logical state
Seed
RulesVersion
ContentManifestHash
```

Non anticipare `RulesVersion` / `ContentManifestHash` in v0.1 se il repository li mantiene deferiti.

---

# 23. Formula replay verificabile

North-star:

```text
MATCH SETUP
    +
CANONICAL TURN INTENTS
    +
RUNTIME DECISIONS
    +
TURNLOG
    +
HASHES
    =
VERIFIABLE LOGICAL MATCH
```

Non dichiarare che tutto questo è già implementato.

---

# 24. Distribuzione server/client

Principio:

> Il server possiede sempre la versione canonica. Il client possiede solo ciò che serve per scegliere e presentare.

| Dato | Server | Client proprietario | Alleati | Nemici |
|---|---:|---:|---:|---:|
| Piano locale in editing | no autorità | sì completo | no | no |
| Proposal | sì riceve | sì origine | no | no |
| Canonical Intent | sì autorità | opzionale propria copia | no raw | mai |
| Team Intent View | produce | sì | sì | no |
| Ready/Commit | sì autorità | sì consentito | sì consentito | pubblico solo |
| Snapshot | sì | no | no | no |
| Full simulation state | sì | no | no | no |
| Reaction Opportunity | sì | solo responder autorizzato | view autorizzata | no |
| Pending Reaction Decision | sì | propria | no | no |
| Canonical Reaction Decision | sì | secondo policy | secondo policy | dopo reveal se consentito |
| Resolver | sì | no | no | no |
| Full TurnLog | sì | no raw | no raw | no raw |
| Resolved public events | proietta | sì | sì | sì filtrati |
| Replay Archive completo | sì | no normalmente | no | no |
| Perspective Replay futuro | genera | sì | per prospettiva | per prospettiva |

---

# 25. Regola privacy fondamentale

Sbagliato:

```text
Server
  full enemy intent
      ↓
replica al client
      ↓
UI lo nasconde
```

Corretto:

```text
Server
  CanonicalIntentStore
      ↓
authorized projection
      ↓
Client
```

Il client non deve poter estrarre dati che non gli sono mai stati inviati.

---

# 26. CanonicalIntentStore

Issue futura esistente:

**#780 — Store canonico degli intenti: un solo posto, e sta sul server**

Oggi parla di `FRTPlannedIntent`.

Questa chat propone di rivalutare la forma:

```text
CanonicalIntentStore
      │
      ├── FRTCanonicalIntent
      │
      └── BuildIntentView(...)
                ↓
          FRTIntentView
```

`FRTPlannedIntent` potrebbe restare struttura privacy/presentation/intermedia.

Non applicare senza riconciliare #780 e documenti owner.

---

# 27. Snapshot

Snapshot completo della Resolution: server-only.

Include concettualmente:

```text
MapState
Units
Statuses
Objectives
Team Knowledge
Canonical Intents
Turn phase
Seed
Rules state
```

Il client riceve solo proiezioni/risultati osservabili.

---

# 28. Full TurnLog

TurnLog canonico completo: server-authoritative.

Non replicarlo raw.

Futuro:

```text
Canonical TurnLog
       ↓
Knowledge / Observation Filter
       ↓
Team A Event Stream
Team B Event Stream
Observer/Admin Stream
```

Importante per invisibilità, rumore, trappole, Team Knowledge e unknown source.

---

# 29. Reaction Clash e privacy

Prima del reveal:

```text
Server:
PendingDecision A
PendingDecision B
```

Client A non riceve B.

Client B non riceve A.

Al reveal entrano nella traccia osservabile secondo policy.

Il timing del lock non deve essere un side-channel.

---

# 30. Full Replay vs Perspective Replay

Direzione futura:

```text
Full Replay Archive
      ↓
Projection
      ├── Team A Perspective
      ├── Team B Perspective
      └── Observer/Admin
```

Non ancora necessariamente canonica.

Non inviare replay omnisciente durante una partita competitiva per poi nasconderlo via UI.

---

# 31. Offline v0.1 vs rete futura

In v0.1 offline tutto può stare fisicamente sulla stessa macchina, ma mantenere distinzione concettuale:

```text
Authoritative side
- Canonical Intent
- Snapshot
- Resolver
- TurnLog
- Replay Archive

Presentation side
- Local Planning
- Intent View
- HUD
- Animation
- VFX
- Input
```

Formula:

```text
CLIENT PROPONE
SERVER CANONICALIZZA
SERVER SIMULA
SERVER REGISTRA
SERVER PROIETTA
CLIENT PRESENTA
```

---

# 32. ROADMAP RICHIESTA — v0.1 DETTAGLIATA

Claude deve produrre una roadmap dettagliata **integrata con quella reale**, non parallela.

Prima:

1. leggere `docs/roadmap/roadmap-v0.1.md`;
2. leggere `docs/roadmap/v0.1-issue-plan.md`;
3. leggere `docs/roadmap/feature-registry.yaml`;
4. controllare Epic #14 e lane correnti;
5. controllare issue aperte/chiuse;
6. controllare ownership/workstream (`parallel-batch.yaml` se vigente);
7. evitare conflitti con branch o workstream attivi.

## 32.1 Sequenza replay/reaction prioritaria

### R0 — CP14.5 baseline
Verificare stato reale #165:

- Overwatch universal;
- opportunity;
- `OpportunityId`;
- decision boundary;
- `AskReactionDecision`;
- `ApplyReactionDecision`;
- TurnLog v8;
- reaction decision hash;
- movement truncation;
- privacy DTO.

### R1 — Replay Verifier decision seam
**#886**

- recorded decision dal TurnLog;
- key = OpportunityId;
- recorded > live decider;
- no positional matching;
- reaction scenario deterministico.

### R2 — CP14.6 human decision UI/pacing
**#166**

- FIRE/HOLD UI;
- countdown 3.0s server-authoritative;
- timeout → HOLD;
- invalidazione Overwatch;
- slow motion presentation-only;
- team-only visibility;
- `ReactionDecisionSeconds`;
- p50/p90;
- PIE;
- #886 prima o insieme.

### R3 — Reaction Profile / Clash
**#314**, P3/tagliabile.

- Brace arma Reaction Profile;
- profilo base no window;
- profili data-driven;
- contested derived;
- blind reveal;
- fixed deadline;
- no nested;
- cost on lock;
- scenari Clash.

Se v0.1 si accorcia, preservare baseline single-responder e spostare 14.7/14.8.

### R4 — Decision model generalization
Creare issue solo se non esiste.

- verificare necessità di `CommittedResponse`;
- evitare enum roster-specific;
- adapter v8;
- compatibility/versioning.

Non bloccare #886 se non necessario.

### R5 — Determinism gates

- reaction decision determinism;
- TurnLog hash;
- Replay Verifier scenario reaction;
- divergence zero;
- packaged verification;
- first-divergence diagnostic.

### R6 — Governance alignment

Aggiornare se stale:

- Feature Registry;
- ADR-0009 metadata;
- ADR-0004;
- Decision Log;
- Scenario Map;
- PIE manual tests;
- v0.1 DoD;
- wiki refs;
- dependency graph.

---

# 33. Roadmap v0.1 completa — non solo replay

La roadmap precedente aveva lane simili a:

```text
Lane A — Reactions
#165 → #166
poi #314 → #319

Lane B — Perception
#690 + #686 → #159 → #160

Lane C — UI
#219/#637 → #220 → #77/#613 → #705 → #291

Lane D — Consistency
#625 + #687 + #649 → #512 → #170
```

Claude deve verificare lo stato aggiornato prima di riutilizzarle.

Per ogni issue/checkpoint indicare:

- ID;
- titolo;
- stato;
- dipendenze;
- owner feature;
- milestone;
- priorità;
- file principali;
- test automatici;
- scenario;
- PIE/manual gate;
- packaged gate;
- privacy impact;
- replay impact;
- DoD;
- blocker;
- rischio;
- evidenza richiesta.

Obiettivi v0.1 da preservare:

- 2v2 offline;
- hex multilivello;
- 4 eroi;
- azioni base complete;
- movement;
- dash;
- attack/skill;
- reaction baseline;
- terrain;
- cover/structure;
- dynamic objective;
- HUD/log;
- Scenario/Test separato da Training;
- Bot Simulation visuale;
- deterministic TurnLog;
- Replay Verifier;
- zero divergence;
- packaged regression;
- showcase riproducibile.

---

# 34. Cosa NON forzare nella v0.1

Salvo decisioni già canoniche:

- dedicated server;
- matchmaking;
- public modding;
- full Replay UI;
- Perspective Replay completo;
- ContentManifestHash implementation se deferita;
- RulesVersion implementation se deferita;
- mass batch simulation;
- GAS authority;
- crypto tamper-evidence;
- nested reaction stack;
- interactive LIFO stack;
- full Reaction Clash se tagliato dalla milestone.

---

# 35. ROADMAP GENERICA v0.2 → v1.0

## v0.2 — Canonical execution seams

Focus:

- `DecisionProvider` #542;
- unificazione Scenario/Bot/Replay input;
- `FRTCanonicalIntent` o equivalente;
- serializer/versioning intenti;
- eventuale `turn-N.rtintent`;
- compatibility metadata;
- `ContentManifestHash`;
- `RulesVersion`;
- Reaction Profile/Clash se tagliati dalla v0.1;
- Replay Verifier più vicino ai real-match inputs.

Deliverable:

```text
same canonical intent type
→ visual
→ headless
→ replay verifier
```

## v0.3 — Information Game / Team Knowledge

- vista;
- rumore;
- detected/hidden;
- conoscenza condivisa;
- observation projection;
- TurnLog projection;
- replay perspective groundwork;
- unknown source/reveal semantics.

Gate:

> nessun sistema gameplay legge informazione che la squadra non possiede.

## v0.4 — Map Operations

- mappa multilivello robusta;
- porte;
- ponti;
- tunnel;
- ascensori;
- acqua/fuoco/elettricità;
- hazard propagation;
- topology revision;
- cache invalidation;
- LOS/target/path separation;
- map interactions data-driven.

Replay: ogni mutazione mappa spiegabile nel TurnLog.

## v0.5 — Network Authority / Private Planning

- listen server authoritative;
- `CanonicalIntentStore`;
- client proposal;
- server validation;
- ally preview 8–12 Hz unreliable sequenced;
- ready/commit reliable;
- no enemy intent bytes;
- team-only ping/drawing;
- sanitized reaction opportunities;
- anti-leak canary.

Issue rilevanti già note:

- #780 Canonical Intent Store;
- #784 privacy canary.

Gate:

```text
intent leak = 0
```

## v0.6 — GAS Integration

GAS per lifecycle/cost/cooldown/attributes/effects.

NON per simulation authority.

Replay gate: stessa semantica ⇒ stesso StateHash/LogHash.

## v0.7 — Dedicated Server / 3v3

- dedicated server;
- 3v3;
- lobby;
- reconnect;
- session lifecycle;
- soak;
- match recovery.

Replay:

- dedicated TurnLog equivalente al percorso headless;
- reconnect non riscrive la storia canonica.

## v0.8 — Batch Simulation / Bot Competence / Performance

- batch runner;
- bot scoring;
- seeds;
- performance;
- regression corpus;
- balance metrics.

Vincolo:

```text
Batch Runner
riusa:
Intent → Planning → Snapshot → Resolver → TurnLog
```

Nessun secondo simulatore.

## v0.9 — RC Hardening

- feature/content freeze;
- migration;
- save/replay compatibility;
- security hardening;
- packaged soak;
- upgrade tests;
- observability;
- deployment rehearsal.

Replay:

- aprire artefatti build precedente;
- migrazione esplicita;
- fail-closed.

## v1.0 — Production

- production dedicated server;
- matchmaking;
- telemetry/observability;
- rollback;
- security;
- player-facing flows;
- deployment;
- perf budgets;
- smoke tests;
- content certification.

Replay:

- audit partite reali;
- divergence = 0;
- first-divergence diagnostics;
- retention policy;
- privacy policy;
- observer/full replay;
- perspective replay se approvato;
- tamper-evidence solo se richiesto.

---

# 36. Progressione architetturale

```text
v0.1
TurnLog
OpportunityId
ReactionDecision
Verifier seam

v0.2
Canonical Intent persistence

v0.5
CanonicalIntentStore network

v1.0
real-match replay audit
```

La v0.1 costruisce seam, non tutte le feature finali.

---

# 37. Docs/Wiki da consolidare

Verificare owner e path reali prima di modificare.

Target possibili:

```text
docs/decisions/adr-0009-replay-logico-canonico.md
docs/decisions/adr-0004-finestre-di-reazione.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/technical/spec-turnlog.md
docs/technical/architettura-codice.md
docs/technical/test-e-diagnosi.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/feature-registry.yaml
docs/technical/scenario-map.md
docs/OPEN_DECISIONS.md
```

Feature Registry è source of truth dello stato feature.

Non modificare output generati a mano.

Se workflow vigente:

```text
python scripts/feature_registry.py generate
```

e relativi `--check`.

Se Wiki è separata:

- non fingere di averla modificata;
- preparare testo;
- aggiornare `wiki_refs` se consentito.

---

# 38. Feature da cercare prima di crearne

- Replay Archive
- Replay Verifier
- Reaction Decision Replay
- Decision Boundary
- Reaction Profile
- Reaction Clash
- Scenario Harness
- Canonical Intent
- Private Planning
- Network Authority
- TurnLog
- Team Knowledge

Non creare duplicati semantici.

---

# 39. Scenari utili

## Replay / Reaction

```text
Replay.Reaction.FireRecorded
Replay.Reaction.RecordedResponseBeatsLiveDecider
Replay.Reaction.OpportunityIdentityMismatch
Replay.Reaction.TimeoutIsDeterministic
```

## Canonical Intent — futuro

```text
Intent.StableIdsSurviveRoundTrip
Intent.PathIsAcceptedPathNotWaypoints
Intent.ActionIdIndependentFromKitIndex
Intent.TargetUnitUsesStableId
```

## Privacy — rete futura

```text
Network.NoEnemyIntentBytes
Network.ReactionOpportunityNoFutureKnowledge
Network.ClashChoiceHiddenUntilReveal
```

## Clash

```text
Spec.Clash.HiddenUntilReveal
Spec.Clash.RevealIsFixedDeadline
Spec.Clash.Determinism
```

Verificare quelli già esistenti.

---

# 40. Automation tests suggeriti

Verificare esistenza prima di crearli:

```text
Replay.Verifier.ReactionDecisionsComeFromTheTrace
Replay.Verifier.RecordedResponseBeatsLiveDecider
Replay.Verifier.ResimulationIsDeterministic

CanonicalIntent.RoundTrip
CanonicalIntent.NoRuntimePointers
CanonicalIntent.NoPresentationFields
CanonicalIntent.UsesStableActionIds

Privacy.CanonicalIntentNeverReplicatedToEnemy
Privacy.FullTurnLogNeverReplicatedRaw
Privacy.ClashPendingDecisionIsServerOnly
```

Gli ultimi appartengono alle milestone rete.

---

# 41. Errori da evitare

1. Creare un `DecisionLog` parallelo.
2. Mettere tutto nel TurnLog.
3. Serializzare Actor pointer.
4. Serializzare ability indices come identity persistenti.
5. Usare `FText` come Action ID.
6. Inviare full TurnLog ai client.
7. Inviare enemy intent e nasconderlo in UI.
8. Usare wall-clock nel determinismo.
9. Matching reaction decision per ordine anziché identity.
10. Introdurre `bIsClash` se derivabile.
11. Hard-codificare Brace/Overwatch nel Replay Verifier.
12. Aggiungere response enum roster-specific.
13. Fare del Replay Player un secondo resolver.
14. Fare dello Scenario Harness un secondo simulatore.
15. Fare del Bot Simulator un secondo percorso.
16. Dare a GAS autorità del simulatore.
17. Modificare generated registry output a mano.
18. Duplicare issue esistenti.
19. Dichiarare canonica una proposta di questo handoff senza Decision/ADR.
20. Anticipare in v0.1 feature deferite.

---

# 42. Proposte da formalizzare solo dopo conferma

## P1 — Canonical Intent separato da Planned Intent

```text
FRTPlannedIntent != necessariamente FRTCanonicalIntent
```

## P2 — Canonical Intent server-authoritative

Client = proposal/local planning.

## P3 — Persistenza Accepted Intents

Possibile:

```text
turn-N.rtintent
```

## P4 — ReactionDecision generalizzata

`CommittedResponse` separata dal reason/outcome.

## P5 — Full TurnLog server-only

Client = projection/event stream.

## P6 — Match Setup persistente

Possibile:

```text
match.rtsetup
```

## P7 — Full vs Perspective Replay

Full archive server-side, projection per observer/team.

## P8 — DecisionGroupId

Possibile correlazione stabile multi-responder.

Per ognuna:

1. cercare decisione equivalente;
2. evitare duplicati;
3. proporre ADR/Decision solo se realmente nuova.

---

# 43. Definition of Done del lavoro Claude

Claude deve consegnare:

## A. Audit

- HEAD;
- issue states;
- feature states;
- docs owner;
- conflitti;
- stale metadata;
- duplicati concettuali.

## B. Decision matrix

Per ogni punto:

```text
CANONICAL
ALREADY IMPLEMENTED
OPEN ISSUE
PROPOSED
DEFERRED
REJECTED
```

## C. Roadmap v0.1 dettagliata

Con issue concrete, dipendenze, priorità, milestone, test, scenario, packaged, privacy, replay, risk, evidence.

## D. Roadmap v0.2 → v1.0

Più generica, per Epic/capability.

## E. Docs

Aggiornare/proporre patch ad ADR, Decision Log, roadmap, Feature Registry, Scenario Map, Wiki, Open Decisions.

## F. GitHub tracking

- riusare issue;
- nuove solo se necessarie;
- linkare feature/scenario/wiki;
- non duplicare #886/#542/#780/#314/#166.

## G. Test

Almeno:

- Replay Verifier con reaction decision;
- precedence negative test;
- deterministic replay scenario;
- privacy checks dove la release lo consente.

## H. Commit plan

Commit piccoli:

```text
docs(...)
test(...)
refactor(...)
feat(...)
chore(registry...)
```

---

# 44. Sequenza operativa consigliata

1. Audit `main`.
2. Audit #886, #166, #314, #542, #780 e replay issues.
3. Audit `RTTurnLog`.
4. Audit `RTReactionOpportunityTypes`.
5. Audit `RTTurnManager::AskReactionDecision`.
6. Audit `ApplyReactionDecision`.
7. Audit Replay Verifier.
8. Audit `FRTPlannedIntent`.
9. Audit `ARTUnit` planning fields.
10. Audit `FRTScenarioIntent`.
11. Audit Feature Registry.
12. Audit roadmap v0.1.
13. Audit roadmap post-v0.1.
14. Produrre decision matrix.
15. Chiudere design minimo #886.
16. Non allargare #886.
17. Definire futura canonical intent normalization.
18. Definire issue future per intent persistence solo se mancanti.
19. Aggiornare tracking.
20. Roadmap dettagliata v0.1.
21. Roadmap capability v0.2-v1.0.
22. Rigenerare viste generated.
23. Eseguire test/docs checks.
24. Preparare PR/commit plan.

---

# 45. Sintesi architetturale

```text
LOCAL PLAN
    ↓
CLIENT PROPOSAL
    ↓
SERVER VALIDATION
    ↓
CANONICAL INTENT
    ↓
IMMUTABLE SNAPSHOT
    ↓
RESOLVER
    │
    ├── DECISION BOUNDARY
    │       ↓
    │   RECORDED RUNTIME DECISION
    │
    ↓
TURNLOG
    ↓
AUTHORIZED PROJECTION
    ↓
CLIENT PRESENTATION
```

Replay Verifier:

```text
MATCH SETUP
    +
CANONICAL INTENTS
    +
RECORDED RUNTIME DECISIONS
    +
RESOLVER
    ↓
NEW TURNLOG
    ↓
COMPARE WITH ARCHIVED TURNLOG
```

Replay Player:

```text
ARCHIVED TURNLOG
    ↓
PRESENTATION
```

senza resolver.

Privacy:

```text
SERVER KNOWS FULL
CLIENT KNOWS AUTHORIZED VIEW
```

Determinismo:

```text
SAME CANONICAL INPUTS
+ SAME RULES
+ SAME SNAPSHOT
+ SAME SEED
= SAME RESULT
```

---

# 46. Output finale richiesto a Claude

Il report finale deve contenere:

1. Stato reale prima delle modifiche
2. Decisioni consolidate
3. Proposte accettate/scartate
4. File modificati
5. Issue create/aggiornate
6. Feature Registry aggiornato
7. Scenario Map aggiornata
8. Wiki aggiornata o testo pronto
9. Roadmap dettagliata v0.1
10. Roadmap sintetica v0.2–v1.0
11. Test eseguiti
12. Gate packaged
13. Rischi residui
14. Next issue consigliata
15. Commit/PR plan

Il risultato deve lasciare RefactorTactics con **una sola architettura canonica**, senza duplicare sistemi di replay, intent, scenario, bot o networking.
