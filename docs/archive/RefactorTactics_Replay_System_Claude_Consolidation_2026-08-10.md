# RefactorTactics — Replay System, Match History e Deterministic Audit
> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

## Prompt operativo per Claude Code: consolidamento documentazione, Wiki, Roadmap, Feature Map, Scenario Map, Epic e Issue

**Data handoff:** 2026-08-10  
**Progetto:** RefactorTactics  
**Target:** Unreal Engine 5, PC-first, simulazione tattica deterministica a turni simultanei  
**Scopo di questo handoff:** trasformare il focus sul sistema di replay in una specifica canonica collegata a codice, documentazione, Wiki, roadmap, Feature Registry/Feature Map, Scenario Map, test, Epic e Issue GitHub.

---

# 0. ISTRUZIONE PRINCIPALE

Devi **auditare il repository reale prima di modificare qualsiasi cosa**.

Non assumere che nomi file, stati, milestone, Feature ID, Epic ID o issue suggeriti qui siano ancora esatti.

Prima:

1. leggi `AGENTS.md`, `CLAUDE.md`, `README.md`;
2. individua la baseline UE effettivamente bloccata nel repository;
3. individua:
   - Decision Log / ADR;
   - documentazione simulatore deterministico;
   - TurnLog;
   - snapshot;
   - hash;
   - Fast Decision / Decision Boundary;
   - Overwatch / Reaction;
   - Scenario Harness;
   - roadmap;
   - Feature Registry / Feature Map;
   - Scenario Map;
   - Wiki;
   - issue plan;
   - issue/epic GitHub esistenti;
4. confronta questo handoff con HEAD;
5. crea un breve conflict report;
6. consolida, non duplicare.

Le decisioni esplicite più recenti del progetto prevalgono sui PDR storici e sui documenti vecchi.

**Non dichiarare `DONE` una capability solo perché è documentata.**
Verifica codice, test, UI, packaged build e network/privacy gate dove applicabili.

---

# 1. CONTESTO CONSOLIDATO ESISTENTE DA RISPETTARE

Il progetto possiede già le fondamenta concettuali del replay:

- simulatore deterministico;
- snapshot logici immutabili;
- TurnLog canonico;
- `StateHash`;
- `LogHash`;
- `RulesVersion`;
- `ContentManifestHash`;
- `ResolverConfigHash`;
- seed/stream deterministici;
- reason code;
- playback guidato dal TurnLog;
- Scenario Harness / golden test;
- target di **zero replay divergence**.

Fonti storiche/documentali da cercare nel repository o in `docs/PDR`:

- PDR simulazione deterministica / snapshot / TurnLog;
- PDR UI/UX / playback / combat log;
- PDR networking/privacy;
- PDR content manifest/hash;
- PDR roadmap/QA;
- documenti Fast Reaction / Overwatch;
- documenti Time Bank;
- Feature Registry / Feature Map recente;
- Scenario Map;
- Roadmap consolidata.

Documenti recenti indicano già una feature equivalente a:

```text
RT-FEAT-CORE-HASH-REPLAY
StateHash / LogHash / replay deterministico
```

con fondamenta presenti ma replay persistente/audit ancora parziale.

**Non creare automaticamente una seconda feature equivalente.**
Decidi dopo l'audit se:
- estendere questa feature;
- trasformarla in umbrella;
- aggiungere sub-feature coerenti con lo schema reale.

---

# 2. DECISIONE ARCHITETTURALE DA CONSOLIDARE

Il replay canonico di RefactorTactics **NON è una registrazione video** e **NON deve dipendere dal network replay di Unreal come fonte di verità**.

Regola da documentare:

> Il replay canonico di RefactorTactics è una registrazione versionata degli input autorevoli, delle decisioni runtime, degli snapshot/checkpoint e del TurnLog deterministico. Animazioni, VFX e replication Unreal sono presentazione/trasporto e non costituiscono l'autorità del replay.

Seconda regola:

> Ogni replay deve poter essere usato sia per il playback della partita sia, quando il ruleset/build è compatibile, per verificare deterministicamente che la simulazione produca gli stessi `StateHash` e `LogHash`.

Questo genera **due use case distinti** che non vanno confusi.

---

# 3. DUE MODALITÀ TECNICHE DISTINTE

## 3.1 Playback Replay

Scopo:

- guardare la partita;
- pausa/play;
- cambiare velocità;
- step evento;
- step micro-step;
- step fase;
- step turno;
- seek;
- seguire una unità;
- aprire Combat Log;
- spiegare l'esito.

Pipeline:

```text
Replay Archive
    |
    v
TurnLog canonico + checkpoint
    |
    v
Replay Player
    |
    v
Presentation
Animations / VFX / Camera / UI
```

Il Playback Player **non decide**:
- collisioni;
- danni;
- reaction;
- KO;
- objective;
- path legality;
- targeting.

Riproduce gli eventi già risolti.

---

## 3.2 Deterministic Re-Simulation / Replay Verification

Scopo:

- QA;
- regression;
- audit competitivo;
- debugging;
- verifica della compatibilità del resolver;
- individuazione della prima divergenza.

Pipeline:

```text
Initial Snapshot
+
Accepted Intents
+
Canonical Runtime Decisions
+
Turn Seeds
+
RulesVersion
+
ContentManifestHash
+
ResolverConfigHash
        |
        v
    Resolver
        |
        v
New State + TurnLog
        |
        v
Compare hashes
```

Formula concettuale:

```text
same snapshot
+
same accepted intents
+
same runtime decisions
+
same RulesVersion
+
same ContentManifestHash
+
same ResolverConfigHash
+
same seed
=
same StateHash
+
same LogHash
```

Il verifier deve distinguere chiaramente:

```text
Replay playback succeeded
```

da:

```text
Replay deterministic verification succeeded
```

Un replay vecchio potrebbe essere ancora visualizzabile tramite eventi archiviati anche se non è più re-simulabile dalla build corrente.

---

# 4. DECISIONE IMPORTANTE: GLI INTENT NON BASTANO PIÙ

Con Fast Reaction, Overwatch, Brace/reaction e future Decision Window, durante la Resolution possono comparire nuove decisioni umane.

Esempio:

```text
Planning:
Wraith -> Overwatch NE

Resolution:
MicroStep 2
Enemy A enters cone
-> HOLD

MicroStep 5
Enemy B enters cone
-> FIRE B
```

Registrare solo l'intento iniziale:

```text
Overwatch NE
```

non basta a ricostruire deterministicamente il turno.

Serve un concetto generale equivalente a:

```text
FRTDecisionRecord
```

Il nome è suggerito: riusa tipi esistenti se il progetto possiede già un record canonico delle Fast Decision.

---

# 5. DECISION RECORD CANONICO

Una decisione runtime deve registrare almeno i dati logici necessari alla riproduzione.

Esempio concettuale:

```text
DecisionId
Turn
Phase
MicroStep / DecisionBoundaryId
DecisionType
SourceActionId / ReactionInstanceId
OwningPlayerId / OwningUnitId
OpportunityId
AllowedResponses canonical IDs
SelectedResponse
SelectedTargetIds se applicabili
Resolution = Accepted / Rejected / TimeoutFallback
ReasonCode
```

Esempio:

```text
DecisionId: D847
Turn: 6
Phase: Move
MicroStep: 4
Type: FastReaction
ReactionInstance: R52
Allowed:
- FIRE Unit_A
- FIRE Unit_B
- HOLD
Selected:
- FIRE Unit_B
Outcome:
- Accepted
```

Timeout:

```text
Selected:
- HOLD
Reason:
- Timeout
```

## Regola temporale

Il wall-clock **non deve entrare nel determinismo**.

Può esistere telemetry separata:

```text
ResponseLatencyMs
WindowShownAt
WindowClosedAt
```

ma la parte canonica deve essere:

```text
SelectedResponse = HOLD
Reason = Timeout
```

Questo mantiene:

```text
real-time deadline
    ->
canonical decision
    ->
deterministic replay
```

---

# 6. DECISION LOG VS TURN LOG

Non fonderli semanticamente.

## Decision Log / Decision Record

Risponde:

> Cosa ha deciso il giocatore o la policy di timeout?

## TurnLog

Risponde:

> Cosa è successo nella simulazione?

Esempio:

```text
Decision:
Overwatch -> FIRE Unit_B
```

produce potenzialmente:

```text
ReactionCommitted
AbilityDeclared
AttackResolved
DamageApplied
StatusChanged
MovementInterrupted
```

Pipeline:

```text
Intent
   |
   v
Decision Boundary
   |
   v
Decision Record
   |
   v
Resolver
   |
   v
Turn Events
```

Il replay archive può memorizzare entrambi, ma devono restare concetti distinti.

---

# 7. MATCH REPLAY ARCHIVE

L'unità persistente raccomandata è la partita completa.

Struttura concettuale:

```text
RT Match Replay
|
+-- ReplayHeader
|
+-- InitialMatchSnapshot
|
+-- Turn 01
|   +-- TurnStartSnapshot / Checkpoint
|   +-- AcceptedIntents
|   +-- Decisions
|   +-- TurnLog
|   +-- StartStateHash
|   +-- EndStateHash
|   +-- LogHash
|
+-- Turn 02
|   +-- ...
|
+-- ...
|
+-- MatchResult
+-- FinalStateHash
```

Per RefactorTactics non ottimizzare prematuramente lo storage.

Con un numero limitato di turni, **un checkpoint per turno è un default ragionevole**.

Non introdurre checkpoint intra-turno finché un test reale non dimostra che servono.

---

# 8. REPLAY HEADER

Definire o consolidare un header versionato equivalente a:

```cpp
struct FRTReplayHeader
{
    ReplayFormatVersion;

    MatchId;
    CreatedAt;

    GameBuildId;

    RulesVersion;
    ContentManifestHash;
    ResolverConfigHash;

    MapId;
    MapVersion;

    MatchSeed;

    Teams;
    Players;
    Characters;

    TurnCount;

    ReplayPolicy;
    ReplayFlags;
};
```

Nomi/campi finali dipendono dai tipi reali del progetto.

## Campi fondamentali

```text
ReplayFormatVersion
RulesVersion
ContentManifestHash
ResolverConfigHash
GameBuildId
```

Servono a impedire che un replay venga silenziosamente interpretato usando regole diverse.

Esempio:

```text
Ability X v1 = 20 damage
Ability X v2 = 18 damage
```

Un replay v1 non deve essere ricalcolato con v2 senza rilevare incompatibilità.

---

# 9. TURN RECORD

Struttura concettuale:

```cpp
struct FRTReplayTurn
{
    int32 TurnNumber;

    FRTTurnSnapshot StartSnapshot;

    TArray<FRTIntent> AcceptedIntents;

    TArray<FRTDecisionRecord> Decisions;

    FRTTurnLog TurnLog;

    uint64 StartStateHash;
    uint64 EndStateHash;
    uint64 LogHash;
};
```

Adatta a tipi/hash reali.

Il formato runtime/persistente può essere più compatto.

Prima stabilizzare il **modello canonico**, poi ottimizzare la serializzazione.

---

# 10. COSA REGISTRARE E COSA NON REGISTRARE

## Registrare nel replay canonico

- header/versioni;
- initial snapshot;
- checkpoint per turno;
- accepted intents;
- Fast Decisions / Reaction decisions;
- policy timeout risultante;
- TurnLog canonico;
- stable IDs;
- seed/stream data necessari;
- StateHash / LogHash;
- match result;
- reason code;
- manifest/config hashes.

## NON registrare come input canonico

- anteprime non committate;
- mouse hover;
- path ghost temporanei;
- UI state;
- interpolazioni;
- frame time;
- montage time;
- packet arrival order;
- ordine TMap/TSet;
- camera;
- VFX random non competitivi.

Le preview di planning non devono diventare la fonte del replay.

Il dato canonico è ciò che il server/resolver ha realmente accettato.

---

# 11. SEEK E CHECKPOINT

Con un checkpoint all'inizio di ogni turno:

```text
Seek Turn 8 / Move / MicroStep 3
```

può fare:

```text
Load Turn08.StartSnapshot
        |
        v
Replay/apply canonical events fino al punto richiesto
```

Non serve riprodurre Turn 1–7.

Prima versione:

- seek per turno;
- seek per fase;
- seek per evento;
- micro-step se il TurnLog lo espone già in modo stabile.

In futuro valutare checkpoint intra-turno solo se necessario.

---

# 12. REPLAY PLAYER UI

Consolidare la UI con l'architettura TurnLog/explainability già presente.

Prima versione utile:

```text
[<< Turn] [< Event] [Play/Pause] [Event >] [Turn >>]

Speed:
0.25x 0.5x 1x 2x 4x
```

Timeline concettuale:

```text
TURN 07
---------------------------------------------------------
| PREP | DASH | BLAST |        MOVE       | CLEANUP |
                         ^
                      current

Events:
  * Barrier deployed
       * Dash
             * Shot
                   ! Reaction Opportunity
                   ! FIRE
                         * Damage
                              * Move blocked
```

Filtri candidati:

```text
All
My Team
Enemy
Movement
Damage
Reactions
Environment
Objectives
```

Quando si seleziona un evento, il Combat Log deve usare dati canonici/reason code, non ricalcolare lato UI.

Esempio:

```text
Arc Lance -> 18 damage

Base                 20
Cover                 -4
Wet Chain             +2
------------------------
Final                 18
```

---

# 13. FULL REPLAY VS PERSPECTIVE REPLAY

Questa distinzione è obbligatoria per privacy, Fog of War, noise e multiplayer.

## Full Replay

Uso:

- fine partita;
- admin;
- QA;
- tournament review;
- debugging autorizzato.

Può mostrare:

- tutte le unità;
- intenti di entrambe le squadre;
- informazioni complete;
- reaction;
- hidden state consentito dalla policy post-match.

## Perspective Replay

Riproduce:

> ciò che Team A / Player A poteva legittimamente conoscere in quel momento.

Mostra solo:

- propria squadra;
- enemy visibili;
- acoustic contacts autorizzati;
- last known;
- uncertainty;
- Team Knowledge effettiva;
- nessun planning avversario non osservato.

## Regola privacy

Durante il match:

```text
Full replay archive = server-only
```

Non trasformare il replay in un canale laterale di leakage.

Il client avversario non deve ricevere anticipatamente:
- accepted intent nemici;
- future paths;
- future targets;
- future Reaction Opportunities;
- hidden state;
- full replay payload.

Dopo il match la disponibilità del Full Replay deve dipendere da una `ReplayPolicy` esplicita.

---

# 14. REPLAY + TEAM KNOWLEDGE / FOG / NOISE

Il Perspective Replay non deve "ricostruire a posteriori" informazioni che il giocatore non aveva.

Quando una squadra percepisce:

```text
sound contact
last-known position
uncertain area
```

il replay di quella prospettiva deve usare gli eventi/knowledge records autorizzati dell'epoca.

Non fare:

```text
hidden enemy state
    ->
client replay filter
```

se questo comporta l'invio del dato segreto al client.

Applicare lo stesso modello di privacy del gameplay:

```text
authoritative hidden state
    |
    v
team knowledge / sanitized replay view
    |
    v
client
```

---

# 15. REPLAY COME DEBUGGER / BLACK BOX

Il replay deve essere progettato anche per bug report.

Use case:

```text
"Al turno 9 una unità ha attraversato un muro."
```

Con un ReplayId il developer deve poter:

```text
open replay
-> jump Turn 9
-> inspect MoveStep
```

e trovare almeno:

```text
UnitId
FromCell
ToCell
Edge/TransitionId
GraphRevision
MovementProfile
Cost
ReasonCode
Relevant before/after
```

Non inserire tutto in ogni evento se il dato è già recuperabile deterministicamente dal checkpoint.

Ma i reason code essenziali devono rendere il bug spiegabile senza reverse-engineering della UI.

---

# 16. REPLAY COME GOLDEN / REGRESSION CORPUS

Il Replay Verifier deve integrarsi con il test system, non creare un simulatore parallelo.

Corpus candidato:

```text
Tests/Replays/
```

oppure la directory canonica reale del progetto.

Fixture candidate:

```text
Movement.Basic
Movement.Collision
Reaction.Overwatch.HoldThenFire
Reaction.Overwatch.TimeoutHold
Decision.FastReaction.RoundTrip
Environment.WaterElectric
Replay.Seek.Turn
Replay.Determinism.Repeat
Replay.Determinism.Permutation
Replay.Version.ContentMismatch
Replay.Version.RulesMismatch
Replay.Privacy.TeamPerspective
```

CI:

```text
Load replay
-> verify compatibility
-> re-simulate
-> compare hashes
```

Output utile:

```text
Turn 1 PASS
Turn 2 PASS
Turn 3 PASS
Turn 4 FAIL

Expected StateHash: ...
Actual   StateHash: ...

Expected LogHash: ...
Actual   LogHash: ...

First divergent event:
Turn 4 / Move / MicroStep 3 / Event 12
```

Se fattibile, produrre una diagnostica della **prima divergenza**, non solo "hash mismatch".

---

# 17. NETWORK REPLAY UNREAL

Non usare il sistema di replay/demo recording Unreal come formato canonico del gameplay di RefactorTactics.

Motivo:

RefactorTactics ha uno stato logico discreto e deterministico.

La source of truth deve essere:

```text
logical snapshots
+ accepted intents
+ canonical runtime decisions
+ canonical events
+ hashes
```

Il sistema Unreal può eventualmente essere valutato in futuro per:
- supporto spectator;
- cattura supplementare;
- strumenti di presentazione;

ma non deve sostituire il replay logico.

Documentare questa scelta in ADR se il repository usa ADR per decisioni architetturali.

---

# 18. ARCHITETTURA C++ CANDIDATA

Prima cerca classi/tipi equivalenti già esistenti.

Se mancanti, una separazione plausibile è:

```text
Replay/
  RTReplayTypes.h
  RTReplayRecorder.h/.cpp
  RTReplayReader.h/.cpp
  RTReplayPlayer.h/.cpp
  RTReplayVerifier.h/.cpp
  RTReplaySerializer.h/.cpp
```

Responsabilità:

```text
RTReplayRecorder
    raccoglie header/checkpoint/intents/decisions/log/hashes

RTReplayReader
    apre e valida metadata/versioni

RTReplayPlayer
    guida playback della presentazione

RTReplayVerifier
    richiama il resolver reale e confronta hash

RTReplaySerializer
    persistence/versioning/compatibility
```

Dipendenze desiderate:

```text
Planning ----------\
Decision System ----\
SnapshotBuilder ----- > ReplayRecorder -> ReplayArchive
ActionResolver ------/
TurnLog ------------/

ReplayArchive -> ReplayPlayer -> Presentation

ReplayArchive -> ReplayVerifier -> REAL ActionResolver
```

Regola:

```text
ReplayPlayer != ActionResolver
ReplayVerifier -> ActionResolver
```

Niente dipendenze inverse del core logico verso UMG.

---

# 19. SERIALIZZAZIONE

Prima iterazione developer-friendly:

```text
.rtreplay.json
```

oppure formato JSON/debug già coerente col progetto.

Shipping/futuro:

```text
.rtreplay
```

contenitore binario versionato e potenzialmente compresso.

**Non bloccare prematuramente il formato binario definitivo.**

Bloccare invece presto:

```text
ReplayFormatVersion
Stable IDs
RulesVersion
ContentManifestHash
ResolverConfigHash
canonical field ordering
canonical event ordering
```

Se esiste già un framework di serialization/versioning, riusalo.

---

# 20. MATCH HISTORY

Separare:

```text
Match History Metadata
```

da:

```text
Full Replay Payload
```

La schermata storico non deve caricare l'intero replay.

Metadata candidati:

```text
MatchId
Date
Mode
Map
Teams
Characters
Result
Turns
Duration wall-clock
RulesVersion
Replay availability
Replay verification state
```

Il wall-clock può stare nei metadata, non nel determinismo.

Funzioni future:

```text
Open Replay
Copy ReplayId
Export
Delete local
Filter
Search
```

Cloud/account sync solo se esiste una roadmap reale che lo giustifica; non introdurlo come requisito della prima versione.

---

# 21. ROADMAP — CONSOLIDAMENTO PROPOSTO

Non creare una mega-milestone parallela.

Integrare nella roadmap reale.

La direzione consigliata, da adattare a HEAD, è:

## Fondazioni / v0.1

Riusa quanto già esiste:

- snapshot;
- TurnLog canonico;
- stable event schema;
- `StateHash`;
- `LogHash`;
- deterministic repeat/golden test;
- event reason codes.

Non serve un replay viewer completo.

## Tooling / circa v0.5

**First Playable Replay**

- `ReplayFormatVersion`;
- `ReplayArchive`;
- per-turn checkpoint;
- Accepted Intents;
- DecisionRecord;
- JSON/debug serializer;
- local import/export;
- Replay Player;
- basic seek;
- deterministic Replay Verifier;
- golden replay corpus;
- first divergence diagnostics.

Questa collocazione è coerente con il fatto che il replay è anche tooling/QA.

## Network Alpha / circa v0.6

- replay DTO/view privacy model;
- Perspective Replay;
- Team Knowledge integration;
- Fast Decision network decision record;
- canary tests;
- nessun full archive inviato durante match.

## Dedicated / Production Beta / circa v0.7

**Production Ready Replay**

- persistent replay storage;
- ReplayId;
- server-side archive;
- reconnect/spectator/replay audit;
- Full vs Perspective Replay policy;
- packaged client/server tests;
- telemetry;
- retention policy;
- compatibility/reporting;
- zero replay divergence corpus.

Se il repository oggi usa versioni/milestone diverse, mappa queste capability alle milestone correnti senza inventare una roadmap parallela.

---

# 22. FEATURE MAP / FEATURE REGISTRY

Prima verifica se l'umbrella esistente è:

```text
RT-FEAT-CORE-HASH-REPLAY
```

Possibile strategia:

```text
RT-FEAT-CORE-HASH-REPLAY
    umbrella: deterministic replay foundation
```

e, solo se il Feature Registry supporta feature più granulari, aggiungere entry equivalenti a:

```text
Replay Archive
Runtime Decision Record
Replay Player
Replay Seek
Replay Verifier
Replay UI / Timeline
Perspective Replay
Replay Persistence / ReplayId
Match History
```

Non imporre questi ID se la naming convention reale è diversa.

Ogni entry deve avere i campi reali del registry, inclusi dove previsti:

```text
Feature ID
Status
First playable
Production ready
Epic
Issues
Dependencies
Scenario coverage
Wiki
Tests
Owner/module
DoD gates
```

Gate importanti:

```text
spec
data
runtime
log_debug
automation
scenario
ui_wiki
packaged
network_privacy
```

Una replay feature online non è `DONE` senza `network_privacy`.

---

# 23. SCENARIO MAP

Non duplicare scenari esistenti.

Aggiungere/collegare scenari equivalenti almeno a:

## Core / local

```text
Replay.Basic.RoundTrip
```

Setup:
- scenario breve deterministico;
- registra replay;
- ricarica;
- playback completo.

Assert:
- stesso numero turni;
- stessi eventi canonici;
- checkpoint caricabili.

---

```text
Replay.Seek.Turn
```

Assert:
- load checkpoint Turn N;
- playback da Turn N produce lo stesso stato/event stream finale.

---

```text
Replay.Decision.FastReaction
```

Caso:
- Decision Boundary;
- HOLD;
- successivo FIRE.

Assert:
- entrambe le decisioni presenti nel record;
- re-simulation identica.

---

```text
Replay.Decision.TimeoutFallback
```

Caso:
- Fast Reaction timeout;
- fallback canonico HOLD.

Assert:
- wall-clock non entra nell'hash;
- decision record contiene timeout/fallback;
- replay deterministico.

---

```text
Replay.Determinism.Repeat
```

- N run stesso replay;
- zero divergence.

---

```text
Replay.Version.ContentMismatch
```

- manifest differente;
- verifier rifiuta o segnala incompatibilità esplicita;
- non ricalcola silenziosamente.

---

## Network/privacy future

```text
Replay.Privacy.FullArchiveServerOnlyDuringMatch
Replay.Privacy.TeamPerspective
Replay.Privacy.EnemyIntentCanary
Replay.Network.Reconnect
Replay.Network.LateJoinPolicy
Replay.Network.SpectatorPolicy
```

Gli ultimi non devono bloccare versioni offline precedenti se la roadmap li colloca dopo.

---

# 24. WIKI DA AGGIORNARE / CONSOLIDARE

Cerca pagine esistenti prima di crearne nuove.

Struttura candidata:

```text
Replay System
|- Overview
|- Match History
|- Replay Architecture
|- Replay Player
|- Replay Timeline & Seeking
|- Deterministic Replay Verification
|- Replay File / Version Compatibility
|- Replay Privacy & Perspective
|- Replay Debugging
|- Replay Tests & Scenarios
```

Ogni pagina feature deve linkare, secondo le convenzioni reali:

```text
Feature ID
Roadmap milestone
Epic
Issues
Scenario IDs
Tests
Related ADR/PDR
Dependencies
```

La Wiki player-facing non deve esporre dettagli server-only inutili.

La pagina privacy deve spiegare chiaramente la differenza fra:
- Full Replay;
- Team/Player Perspective Replay;
- disponibilità durante il match;
- disponibilità post-match.

---

# 25. DOCUMENTAZIONE TECNICA DA AGGIORNARE

Non riscrivere interi PDR se basta una sezione/cross-reference.

Aggiornare/consolidare almeno le aree equivalenti a:

```text
Deterministic Simulation / TurnLog
Architecture
UI / UX / Resolution Playback
Networking / Privacy
Content Manifest / Versioning
Fast Decision / Decision Boundary
Overwatch / Reaction
Time Bank
Scenario/Test Harness
Roadmap / QA / Risk Register
```

Punti obbligatori da rendere canonici:

```text
Replay Player != Replay Verifier
DecisionRecord != TurnLog
Uncommitted preview != replay input
Wall-clock != simulation time
Network Replay UE != gameplay replay authority
Full replay data server-only while match active
Replay compatibility uses explicit versions/hashes
```

---

# 26. ADR / DECISION LOG

Se il repository usa ADR, creare o aggiornare una decisione equivalente a:

```text
ADR: Canonical Logical Replay Architecture
```

Decisione:

```text
RefactorTactics uses a logical deterministic replay archive
based on snapshots, accepted intents, canonical runtime decisions,
TurnLog and hashes.

Unreal network demo replay is not authoritative gameplay history.
```

Alternative considerate:

1. Unreal network replay only;
2. TurnLog only;
3. video capture;
4. snapshot + intents only.

Motivare perché sono insufficienti come fonte canonica.

---

# 27. EPIC GITHUB

Prima:

```bash
gh issue list
gh api ...
```

o strumenti equivalenti, per individuare Epic/Issue già esistenti.

**NON creare duplicati.**

Se esiste un Epic Core Determinism / Tooling / Replay:
- aggiornarlo;
- collegare le nuove issue;
- evitare una nuova Epic.

Se manca, candidato:

```text
[EPIC] Replay, Match History & Deterministic Audit
```

Scopo:

```text
Dal TurnLog/hash già esistente a un replay persistente,
navigabile, verificabile e privacy-safe.
```

L'Epic deve contenere:

- Why;
- goals;
- scope;
- out of scope;
- architecture decision;
- feature IDs;
- milestone mapping;
- child issues;
- dependencies;
- scenario IDs;
- DoD;
- risks;
- privacy gates;
- packaged gates.

---

# 28. ISSUE CANDIDATE

Adatta naming e quantità allo stato reale del repo.

## Issue 1 — Replay architecture + ADR

```text
Define canonical replay model and compatibility contract
```

Deliverable:
- ADR;
- domain model;
- version/hash policy;
- ownership/dependency diagram.

---

## Issue 2 — Canonical runtime DecisionRecord

```text
Persist Fast Decision / Reaction choices as deterministic replay inputs
```

Dipende da Decision Boundary/Fast Decision se non ancora disponibile.

Include:
- Accepted;
- Timeout fallback;
- selected target;
- stable IDs;
- reason code;
- tests.

---

## Issue 3 — Replay Archive / Recorder

```text
Record per-turn checkpoints, accepted intents, decisions, TurnLog and hashes
```

No UI.

---

## Issue 4 — Replay serialization + compatibility

```text
Add versioned debug serializer and explicit compatibility validation
```

Prima JSON/debug, binary later.

---

## Issue 5 — Replay Player

```text
Play canonical TurnLog from archived checkpoints without invoking gameplay resolution
```

---

## Issue 6 — Replay seek / timeline

```text
Seek by turn / phase / event and expose current event to UI/debug
```

---

## Issue 7 — Deterministic Replay Verifier

```text
Re-simulate archived input through the real resolver and compare hashes
```

Include first-divergence diagnostics.

---

## Issue 8 — Replay golden corpus / CI

```text
Add replay regression corpus and zero-divergence CI checks
```

---

## Issue 9 — Match History

```text
Add lightweight match metadata index and replay discovery/open flow
```

Non caricare payload completo nella lista.

---

## Issue 10 — Replay UI / Combat Explainability

```text
Add replay controls, event timeline, focus and Combat Log integration
```

---

## Issue 11 — Replay perspective/privacy

Future network milestone:

```text
Implement Full vs Perspective Replay policy using sanitized Team Knowledge
```

Include canary.

---

## Issue 12 — Persistent server replay / ReplayId

Dedicated/production milestone:

```text
Persist authoritative replay archives and expose ReplayId with retention policy
```

Non fare prima se storage/backend non è ancora scope.

---

# 29. FORMATO OBBLIGATORIO DELLE ISSUE

Per ogni Issue implementativa usare la convenzione reale del repo.

Se non esiste una convenzione più forte, includere:

```text
Why
Scope
Out of scope
Technical approach
Existing code to reuse
Data/API impact
Networking/privacy impact
Editor/UX setup
Acceptance criteria
Automation tests
Scenario coverage
Debug/telemetry
Performance
Dependencies
Documentation/Wiki updates
Definition of Done
```

Ogni issue deve linkare:
- Epic;
- Feature ID;
- milestone/version;
- Scenario IDs;
- dipendenze.

---

# 30. DEFINITION OF DONE DEL REPLAY

Una capability replay non è Done perché riesce a mostrare una sequenza di animazioni.

Gate complessivi:

```text
[ ] replay model versioned
[ ] stable IDs
[ ] compatibility checks
[ ] accepted intents archived
[ ] runtime decisions archived
[ ] TurnLog archived
[ ] checkpoint loading
[ ] StateHash / LogHash
[ ] playback independent from resolver
[ ] verifier uses the real resolver
[ ] repeat determinism
[ ] first divergence diagnostic
[ ] scenario coverage
[ ] Automation Test
[ ] Combat Log explainability
[ ] seek
[ ] Wiki updated
[ ] Feature Registry linked
[ ] Roadmap linked
[ ] Epic/issues linked
[ ] packaged validation at target maturity
[ ] network privacy validation when online
[ ] zero canary leak when online
[ ] zero replay divergence for supported corpus
```

---

# 31. TEST AUTOMATICI MINIMI

Aggiungere o pianificare test equivalenti a:

## Serialization

```text
ReplayHeader round-trip
ReplayTurn round-trip
DecisionRecord round-trip
Unknown/new optional field policy
FormatVersion mismatch
```

## Determinism

```text
same archive -> same hashes
repeat 100/1000 where practical
permuted insertion order -> same canonical output
different VFX/frame rate -> no gameplay divergence
```

## Decisions

```text
FastDecision FIRE
FastDecision HOLD
FastDecision timeout -> HOLD
simultaneous target selection
stale/invalid response not archived as accepted decision
```

## Seek

```text
full playback final state
==
checkpoint seek final state
```

## Versioning

```text
RulesVersion mismatch
ContentManifestHash mismatch
ResolverConfigHash mismatch
```

## Network/privacy future

```text
enemy planning canary absent
full replay archive absent from enemy client during match
perspective replay has only authorized knowledge
reconnect/late join policy
```

---

# 32. RISK REGISTER

Aggiungere/consolidare rischi equivalenti.

## REPLAY-01 — Replay format becomes coupled to UObject/Actor layout

Mitigazione:

```text
logical structs
stable IDs
explicit versions
serializer boundary
```

## REPLAY-02 — Old replay silently uses new ability data

Mitigazione:

```text
ContentManifestHash
definition versions
compatibility reject/report
```

## REPLAY-03 — Fast Decision not recorded

Mitigazione:

```text
canonical DecisionRecord
golden reaction replay
timeout test
```

## REPLAY-04 — Replay player accidentally re-resolves gameplay

Mitigazione:

```text
Player consumes TurnLog
Verifier alone invokes resolver
architecture test/review
```

## REPLAY-05 — Hidden enemy planning leaks through replay

Mitigazione:

```text
server-only full archive during match
Perspective Replay
sanitized DTO
canary tests
```

## REPLAY-06 — Replay depends on frame/montage timing

Mitigazione:

```text
logical event ordering
presentation-only timing
frame-rate playback tests
```

## REPLAY-07 — Replay storage grows unnecessarily

Mitigazione:

```text
metadata index separate from payload
per-turn checkpoint initially
measure before compression/complex delta encoding
```

## REPLAY-08 — Hash mismatch gives no actionable debug info

Mitigazione:

```text
first divergent turn/event diagnostic
reason codes
checkpoint hashes
```

## REPLAY-09 — Unreal replay system becomes accidental second authority

Mitigazione:

```text
ADR
one canonical logical format
optional Unreal capture clearly supplemental
```

---

# 33. PERFORMANCE / STORAGE

Non fissare numeri arbitrari senza misurazione.

Aggiungere telemetry:

```text
Replay archive size
Bytes per turn
Serialization time
Load time
Seek time
Verifier time
Replay Player event throughput
```

Target iniziale qualitativo:

- nessun impatto percepibile sulla Resolution;
- recording non deve cambiare esito/order;
- seek a un turno deve essere rapido;
- verifier headless deve essere adatto a batch/CI.

Ottimizzare solo dopo aver misurato fixture rappresentative.

---

# 34. DEBUG / CONSOLE

Integrare con comandi debug esistenti.

Candidati, da adattare:

```text
rt.Replay.Record
rt.Replay.Stop
rt.Replay.Save
rt.Replay.Load
rt.Replay.Verify
rt.Replay.SeekTurn
rt.Replay.DumpHeader
rt.Replay.DumpDecision
rt.Replay.DumpEvent
rt.Replay.Compare
```

Non aggiungere una foresta di comandi se gli stessi flussi sono già coperti da Scenario Harness/console tooling.

Debug UI utile:

```text
ReplayFormatVersion
Turn
Phase
MicroStep
Current Event
Current Decision
StateHash
LogHash
Checkpoint Hash
Compatibility State
Perspective
```

---

# 35. OUT OF SCOPE INIZIALE

Non introdurre subito:

- cloud replay hosting;
- social feed;
- clip automatiche;
- cinematic director AI;
- esportazione video;
- spectator live completo;
- tournament backend;
- delta-compression sofisticata;
- cross-version migration universale;
- replay editing;
- mod replay compatibility avanzata;
- ML training pipeline.

Possono essere roadmap future, non devono bloccare il replay locale verificabile.

---

# 36. MATCH HISTORY FUTURO

Roadmap futura possibile, solo se coerente con il prodotto:

```text
Match History
-> Replay
-> Share ReplayId
-> Tournament Review
-> Debug Report Attachment
-> Highlights
-> Spectator
-> Analytics
```

La prima utilità deve restare:

```text
QA + debug + player replay
```

---

# 37. FILE / SORGENTI DA CERCARE NEL REPOSITORY

Non assumere le path, ma cerca equivalenti a:

```text
docs/design/
docs/roadmap/
docs/PDR/
docs/wiki/
docs/scenarios/
docs/features/
Config/
Source/RefactorTactics/
Tests/
```

Cerca termini:

```text
Replay
TurnLog
StateHash
LogHash
DecisionRecord
FastDecision
DecisionBoundary
Reaction
Overwatch
Scenario Harness
Golden
Determinism
ContentManifestHash
ResolverConfigHash
TeamKnowledge
Perspective
Spectator
Reconnect
```

---

# 38. STRATEGIA DI LAVORO

Ordine obbligatorio:

```text
1. AUDIT REPOSITORY
2. AUDIT EXISTING GITHUB ISSUES/EPICS
3. CONFLICT REPORT
4. CANONICAL DECISIONS
5. ADR / TECH DOC CONSOLIDATION
6. FEATURE REGISTRY UPDATE
7. SCENARIO MAP UPDATE
8. ROADMAP CONSOLIDATION
9. WIKI UPDATE
10. ISSUE/EPIC PLAN
11. CREATE/UPDATE GITHUB EPIC + ISSUES
12. VALIDATE LINKS
13. FINAL REPORT
```

Non iniziare dal creare 12 issue se 8 esistono già.

---

# 39. GITHUB: CREAZIONE REALE DI EPIC E ISSUE

Se il repository e i permessi GitHub consentono write:

1. individua repo corrente;
2. leggi milestone, labels, assignee e convenzioni;
3. cerca issue duplicate;
4. crea o aggiorna l'Epic;
5. crea solo le child issue mancanti;
6. assegna milestone corretta;
7. aggiungi labels reali;
8. assegna owner se la convenzione lo prevede;
9. collega dipendenze;
10. aggiungi link reciproci a docs/feature/scenario;
11. aggiorna roadmap con gli URL/numero reali;
12. aggiorna Feature Registry con issue reali, non placeholder.

Se il repo usa GitHub sub-issues, usa il meccanismo esistente.

Se usa tasklist Epic:

```markdown
- [ ] #123 Replay Archive
- [ ] #124 Replay Verifier
```

usa quello.

Se **non hai permessi write**:

- NON fingere di aver creato issue;
- genera/aggiorna il file issue-plan canonico;
- includi title/body/labels/milestone/dependencies;
- riporta esattamente il permesso mancante.

---

# 40. ROADMAP: EVITARE DUPLICAZIONI

Il progetto distingue:

```text
Feature Registry
    = capability / prodotto

Roadmap
    = vista temporale / esecutiva

Scenario Map
    = validazione dimostrabile

Wiki
    = conoscenza navigabile

GitHub Issue
    = unità di lavoro
```

Non trasformare uno nell'altro.

Collegamenti:

```text
Feature
  -> Epic/Milestone
  -> Issues
  -> Scenarios
  -> Tests
  -> Wiki
```

Una feature può attraversare più milestone.

Una Epic può chiudere gate di più feature.

---

# 41. STATO / MATURITÀ PROPOSTA

Dopo l'audit, la classificazione attesa dovrebbe essere concettualmente simile a:

```text
Replay Determinism Foundation
Status: PARTIAL / IMPLEMENTED FOUNDATION

Replay Archive
Status: DESIGNED / SPECIFIED / PARTIAL

Replay Player
Status: DESIGNED

Replay Verifier
Status: DESIGNED/PARTIAL se Scenario Harness già copre parte

Perspective Replay
Status: FUTURE / NETWORK MILESTONE

Persistent Replay / ReplayId
Status: FUTURE / DEDICATED MILESTONE
```

Ma **usa la verità del codice/HEAD**, non questa previsione.

---

# 42. COMMIT SUGGERITI

Adattare allo stato reale.

Possibile sequenza documentale:

```text
docs: define canonical replay and deterministic audit model
docs: link replay features scenarios and roadmap
docs: add replay wiki architecture and privacy pages
```

Se vengono create issue via GitHub, non serve un commit per le issue.

Se viene anche implementato codice, separare:

```text
feat(replay): add canonical replay archive types
feat(replay): record runtime decision inputs
feat(replay): add turn checkpoint recorder
feat(replay): add replay player
test(replay): add deterministic replay verifier
test(replay): add golden replay corpus
feat(ui): add replay timeline and seek controls
feat(net): add perspective replay privacy policy
```

Non creare un mega-commit.

---

# 43. REPORT FINALE OBBLIGATORIO

Alla fine restituisci:

## A. Audit

- baseline UE verificata;
- branch/commit HEAD;
- file canonici trovati;
- codice replay/hash/TurnLog già esistente;
- feature/scenario/issue già esistenti;
- conflitti trovati.

## B. Decisioni consolidate

Elenca:
- replay authority;
- playback vs verification;
- DecisionRecord;
- checkpoint;
- compatibility;
- privacy;
- Full vs Perspective.

## C. File modificati/creati

Tabella:

```text
Path | Azione | Motivo
```

## D. Feature Registry

Tabella:

```text
Feature ID | Status | First Playable | Production Ready | Epic | Issues
```

## E. Scenario Map

```text
Scenario ID | Feature | Test | Milestone | Status
```

## F. Roadmap

- milestone toccate;
- dipendenze;
- exit gate;
- cosa NON è stato anticipato.

## G. GitHub

- Epic creato/aggiornato;
- numero/url;
- issue create/aggiornate;
- issue riusate;
- duplicate evitate;
- milestone/labels;
- eventuali permessi mancanti.

## H. Test

- test eseguiti;
- test non eseguiti;
- risultati;
- packaged/network test rinviati con motivazione.

## I. Prossimo passo consigliato

Scegli **un solo checkpoint immediato**.

Raccomandazione generale:

```text
consolidare ReplayFormatVersion + DecisionRecord + ReplayArchive minimale
su uno scenario deterministico già esistente,
prima di costruire la UI replay completa.
```

---

# 44. CRITERIO FINALE DI SUCCESSO

Il lavoro è corretto quando il repository racconta una storia unica e tracciabile:

```text
Turn / Snapshot / Resolver
        |
        +--> TurnLog
        |
        +--> Accepted Intents
        |
        +--> Runtime Decisions
        |
        v
    Replay Archive
       /      \
      v        v
 Player       Verifier
 Playback     Re-Simulation
      |          |
      v          v
 UI/UX        Hash Audit
```

e ogni parte è collegata a:

```text
Feature Registry
Roadmap
Scenario Map
Tests
Wiki
Epic
Issues
```

Il replay non è una feature decorativa.

Deve diventare contemporaneamente:

- cronologia della partita;
- strumento player-facing;
- black box per bug report;
- golden corpus;
- regression test;
- audit deterministico;
- base futura per spectator/tournament review;
- sistema compatibile con privacy e Fog of War.

**Non fingere implementazioni mancanti.**
Quando una parte è solo progettata, segnala `SPECIFIED/DESIGNED/FUTURE` secondo lo schema reale e crea il work item necessario.
