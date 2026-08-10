> ## 🗄️ `HISTORICAL` — SORGENTE RECEPITO
>
> **Archiviato il 2026-08-10.** Era in `todo/consolidazione-chat-openai/`, untracked.
>
> **Recepito da** [`spec-decision-time-bank.md`](../../../gameplay/spec-decision-time-bank.md) (CP 14.8) e da
> `D-050`…`D-057`. I punti in cui il kit contraddiceva il canone sono elencati in
> [`decision-time-bank-conflict-report-2026-08-09.md`](../../../roadmap/plans/decision-time-bank-conflict-report-2026-08-09.md).
> Triage: [`consolidamento-chat-openai-triage-2026-08-09.md`](../../../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md).

# RefactorTactics — Decision Time Bank / Reaction Time Bank
## Prompt di consolidamento per Claude Code

**Data:** 2026-08-09  
**Scopo:** consolidare nel progetto RefactorTactics l'idea del **Time Bank** applicato alle Decision Window durante la Resolution, allineando documentazione, Wiki, Roadmap, Feature Map / Feature Registry, Scenario Map, test ed Epic/Issue GitHub.

> Questo documento contiene una proposta di design discussa il 2026-08-09.  
> **Non trasformare automaticamente tutti i valori numerici in CANONICAL.**  
> Prima fai audit delle source of truth reali del repository e classifica ogni punto come:
>
> - `CANONICAL`
> - `PROPOSED FOR PLAYTEST`
> - `OPEN / TBD`
>
> L'esistenza di un sistema di Time Bank per limitare lo stalling durante le reaction è la direzione da consolidare.  
> I valori iniziali sotto sono baseline di playtest, non numeri definitivi di bilanciamento.

---

# 1. Missione

Integrare un sistema generale di **Decision Time Bank** che limiti quanto tempo reale un giocatore può consumare nelle decisioni live durante la Resolution.

Il problema che deve risolvere:

```text
Reaction Opportunity
-> HOLD
-> altra Opportunity
-> HOLD
-> altra Opportunity
-> ...
```

Anche con Fast Reaction corte, una catena di prompt può allungare troppo la partita.

Il Time Bank deve:

- limitare lo stalling complessivo del giocatore;
- mantenere le singole Fast Reaction molto brevi;
- creare pressione strategica;
- funzionare per tutte le Decision Window, non solo Overwatch;
- restare server-authoritative;
- essere compatibile con determinismo, replay e TurnLog;
- non introdurre leak di informazioni;
- funzionare con Decision Window singole e simultanee;
- essere misurabile tramite telemetry/playtest.

---

# 2. Contesto canonico da preservare

Prima di modificare qualcosa, verificare HEAD e le source of truth reali.

Baseline nota da confrontare col repository:

```text
Turn:
Planning
-> Prep
-> Dash
-> Blast
-> Move
-> Cleanup
```

Il normale `Move` resta l'ultima azione volontaria del turno.

Fast Reaction:

```text
FastReactionDuration = 3.0 s
Timeout Overwatch = HOLD
Overwatch charge = 1
HOLD mantiene la reaction armata
same-microstep targets = una singola multi-target opportunity
no nested interactive reaction stack nel MVP
```

Reaction model:

```text
Reaction armed
-> trigger evaluation
-> Reaction Opportunity
-> Decision Boundary se servono 2+ risposte legali
-> Fast Reaction
-> risposta canonica
-> resume resolution
```

Una Fast Reaction NON è una seconda fase di Planning.

La simulazione autorevole si ferma solo a un **Decision Boundary** deterministico.  
La presentazione può rallentare, ma non decide gli esiti.

Formula di determinismo da preservare:

```text
same snapshot
+
same accepted intents
+
same live decisions
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

---

# 3. Source of truth / documenti da auditare

Cercare almeno:

```text
docs/roadmap/feature-registry.yaml
docs/gameplay/*
docs/decisions/*
docs/technical/*
docs/testing/*
docs/scenarios/*
roadmap*
FEATURE_MAP*
SCENARIO_MAP*
PRODUCT_MAP*
```

Verificare anche documenti equivalenti a:

```text
brief-overwatch-reazioni
spec-reazioni-componibili
Decision Window / Fast Reaction ADR
Reaction Clash
Brace / Reaction
Match Timing / Pacing
Networking / Privacy
Deterministic Simulation / TurnLog
UI / UX
Scenario/Test Harness
```

Handoff recenti da cercare, se presenti:

```text
RefactorTactics_Overwatch_FastReaction_Claude.md
RefactorTactics_ReactionSystem_ReactionClash_Claude_Consolidation_2026-08-09.md
RefactorTactics_Brace_Reaction_Brainstorm_Claude_2026-08-09.md
RefactorTactics_MatchTiming_MapScale_Claude.md
RefactorTactics_FEATURE_MAP_2026-08-08.md
RefactorTactics_FeatureRegistry_Roadmap_Wiki_Claude_2026-08-08.md
Testing automatico Cloud.txt
```

Wiki nota da verificare:

```text
refactor-tactics-main.wiki
```

NON creare una seconda roadmap, un secondo Feature Registry o una seconda Wiki.

---

# 4. Concetto principale: Decision Time Bank

Il sistema non deve essere specifico di Overwatch.

Nome concettuale preferito:

```text
Decision Time Bank
```

oppure, se la tassonomia reale del progetto usa un termine differente:

```text
Fast Decision Time Bank
Reaction Time Bank
```

Scegliere il nome finale dopo audit del naming corrente.

Il bank è una risorsa temporale del **giocatore**, condivisa fra le Decision Window live della Resolution.

Consumer previsti:

```text
Overwatch
Brace
Guard
Counter
Dodge
Intercept
Fast Action
Fast Reaction
Reaction Clash / hidden simultaneous choice
Noise-triggered Reaction
Environmental Reaction
future Opportunity Attack
future Counterspell
future Hack Reaction
```

NON implementare un Time Bank separato per ogni abilità.

---

# 5. Baseline di playtest proposta

Prima baseline semplice:

```text
InitialDecisionTimeBank = 30.0 s per player / match

GracePerDecisionWindow = 1.0 s

MaxDecisionWindowDuration = 3.0 s

BankRefill = 0 per MVP playtest

BankFloor = 0
```

Questi valori sono:

```text
PROPOSED FOR PLAYTEST
```

NON considerarli definitivi senza scenario/playtest/telemetry.

---

# 6. Regola fondamentale: il Time Bank NON allunga la singola Fast Reaction

Il bank non significa:

```text
"hai fino a 30 secondi per decidere"
```

La singola finestra resta bounded.

Esempio:

```text
MaxDecisionWindowDuration = 3.0 s
GracePerDecisionWindow = 1.0 s
```

Timeline:

```text
0.0 ---------------- 1.0 ---------------- 3.0
       FREE            BANK CONSUMPTION
```

Quindi:

```text
decisione a 0.7 s
-> bank consumato = 0.0

decisione a 2.4 s
-> bank consumato = 1.4

decisione a 3.0 s
-> bank consumato = 2.0
```

Obiettivo:

> limitare quante volte un giocatore può usare tutta la finestra, non trasformare ogni prompt in una pausa lunga.

---

# 7. Grace Window

La Grace Window serve a non penalizzare il giocatore per:

- percepire che si è aperta la reaction;
- spostare lo sguardo;
- leggere 2–3 opzioni;
- fare un click immediato.

Non deve diventare una seconda riserva infinita.

Per questo il numero di prompt resta limitato anche dalle policy della reaction.

Esempio Overwatch già discusso:

```text
MaxPromptsPerReaction = data-driven
baseline storica proposta = 3
```

La protezione anti-stalling diventa:

```text
MaxPromptsPerReaction
        +
Decision Time Bank
```

Il primo limita lo spam di una reaction.

Il secondo limita il tempo complessivo consumato dal giocatore.

---

# 8. Esempio Overwatch

Stato iniziale:

```text
Decision Bank = 30.0 s
Grace = 1.0 s
Max Window = 3.0 s
```

## Trigger 1

Tank entra nella killzone.

```text
OVERWATCH
[FIRE]
[HOLD]
```

Decisione dopo 2.5 s:

```text
bank cost = 1.5 s
bank remaining = 28.5 s
```

Il giocatore sceglie:

```text
HOLD
```

Overwatch resta armata.

## Trigger 2

Scout entra.

Decisione dopo 0.6 s:

```text
bank cost = 0
bank remaining = 28.5 s
```

HOLD.

## Trigger 3

Carry entra.

Decisione dopo 3.0 s:

```text
bank cost = 2.0 s
bank remaining = 26.5 s
```

FIRE.

La meccanica di baiting resta intatta:

```text
HOLD rifiuta solo l'opportunità corrente
```

Il giocatore NON deve sapere se arriverà un trigger futuro.

---

# 9. Bank exhaustion

Quando:

```text
RemainingBank == 0
```

la Decision Window non deve continuare a concedere tempo esteso come se il bank esistesse ancora.

Baseline da playtestare:

```text
Bank exhausted:
- solo una brevissima response grace
- nessun tempo esteso
- poi timeout policy
```

Valore discusso:

```text
ExhaustedResponseGrace ~ 0.75 s
```

Questo valore è `PROPOSED FOR PLAYTEST`.

Alternativa da mantenere aperta:

```text
ExhaustedResponseGrace = 0
-> fallback immediato
```

Confrontare UX e pacing tramite scenario.

---

# 10. Timeout policy

Il timeout deve essere:

```text
server-authoritative
```

Ogni Decision Definition / Reaction Definition deve avere una policy esplicita.

Esempi concettuali:

```text
Overwatch       -> HOLD
Counterspell    -> PASS
Dodge           -> NO_DODGE
Brace           -> DEFAULT_BRACE
Fast Reposition -> STAY
```

Per Overwatch resta:

```text
Timeout -> HOLD
```

Motivo:

```text
FIRE consuma una risorsa irreversibile.
Un timeout non deve spendere automaticamente la reaction.
```

Non creare fallback impliciti lato client.

---

# 11. Server authority del timer

Il clock autorevole è del server.

Il client:

```text
- riceve deadline / remaining permitted time;
- mostra UI e countdown;
- invia response;
```

Il server:

```text
- apre il Decision Boundary;
- calcola grace / bank usage;
- valida la deadline;
- determina il tempo realmente consumato;
- applica timeout;
- aggiorna il bank;
- registra il risultato nel TurnLog;
```

Il client NON può inviare:

```text
"I clicked after 0.4 s"
```

come valore fidato.

Usare:

```text
server receive time
+
server-established window start/deadline
```

con la policy reale adatta al networking del progetto.

---

# 12. Determinismo e tempo reale

Il wall-clock non deve contaminare il resolver deterministico.

Separare:

```text
Simulation Time
Presentation Time
Decision Time
Wall-clock Match Time
```

Il Decision Time produce un input canonico:

```text
Response
or
TimeoutResponse
```

Il replay deterministico deve usare la decisione canonica registrata, NON rieseguire il countdown reale.

Golden test:

```text
OpportunityId -> canned canonical response
```

Nessun timer reale nei golden test.

UI/Functional test separato per countdown e timeout.

---

# 13. TurnLog / replay

Il TurnLog deve poter spiegare almeno:

```text
DecisionWindowOpened
DecisionGraceEnded
DecisionCommitted
DecisionTimedOut
DecisionBankConsumed
DecisionBankExhausted
DecisionWindowClosed
```

NON creare necessariamente questi nomi letterali se esiste già una taxonomy.

Riutilizzare:

```text
FastDecisionCommitted
FastDecisionTimedOut
ReactionHeld
ReactionCommitted
```

se sono già canonici.

Ogni evento rilevante dovrebbe poter registrare:

```text
DecisionId
OpportunityId
PlayerId / owning side
DecisionType
AllowedResponses
CanonicalResponse
OpenedBoundary
DecisionDurationMs
GraceConsumedMs
BankConsumedMs
BankBeforeMs
BankAfterMs
TimeoutReason
```

Non inserire nel log pubblico dati che violano knowledge/privacy.

---

# 14. Decision Window simultanee

Se più giocatori devono decidere nello stesso logical boundary e le scelte sono indipendenti:

NON:

```text
Player A -> 3s
poi
Player B -> 3s
poi
Player C -> 3s
```

Preferire:

```text
Decision Boundary
    |
    +-- A window
    +-- B window
    +-- C window
        |
   run concurrently
        |
collect canonical responses
        |
stable deterministic resolution
```

Wall-clock massimo:

```text
max(window durations)
```

non:

```text
sum(window durations)
```

Il resolver non deve dipendere da:

```text
packet arrival order
client frame rate
TMap/TSet iteration
```

Dopo la chiusura del boundary, applicare le risposte secondo ordine deterministico stabile.

---

# 15. Reaction Clash / "morra cinese"

Il Time Bank deve riusare anche la futura grammar di Reaction Clash.

Esempio:

```text
ATTACKER:
LEFT / CENTER / RIGHT

DEFENDER:
LEFT / CENTER / RIGHT
```

Le due scelte:

```text
- si aprono nello stesso boundary;
- sono segrete durante la selezione;
- consumano il bank personale dei due giocatori;
- vengono validate dal server;
- vengono revealate solo quando previsto;
- poi vengono risolte deterministicamente.
```

Non esporre la choice avversaria prima del reveal.

Il Time Bank non deve diventare un canale per inferire la scelta avversaria.

---

# 16. Privacy: punto CRITICO da auditare

Durante la discussione è stata proposta la possibilità di mostrare il **bank residuo agli avversari** per renderlo parte della strategia.

Esempio:

```text
Player A: 24.3 s
Player B:  9.8 s
Player C:  1.4 s
```

Questo è interessante perché consente tattiche di pressione:

```text
"quel giocatore ha poco tempo residuo, forziamogli decisioni difficili"
```

MA:

```text
PUBLIC BANK VISIBILITY = PROPOSED / REQUIRES PRIVACY REVIEW
```

Problema da verificare:

> una variazione pubblica del bank potrebbe rivelare che un giocatore ha ricevuto una Decision Window privata o legata a informazione nascosta.

Quindi Claude deve:

1. verificare il threat model esistente;
2. identificare se il delta del bank crea timing/information leak;
3. creare scenario/test privacy;
4. NON consolidare automaticamente `bank pubblico live` come canone se crea leak.

Opzioni da confrontare:

```text
A. Owner-only bank
B. Team-only bank
C. Public bank live
D. Public bank aggiornato solo a boundary pubblici
E. Public rounded / bucketed value
```

La scelta finale resta `OPEN` finché non passa privacy review.

---

# 17. Networking / anti-cheat

Il sistema deve resistere a:

```text
forged response timestamp
late packet
duplicate response
stale DecisionId
response for another player
response after timeout
disconnect during window
reconnect
packet reordering
spectator
late join
```

Server validation minima:

```text
DecisionId valid
Owner valid
Boundary still open
Response allowed
Opportunity still valid
Not already committed
Deadline valid
Bank state authoritative
```

Decision responses critiche:

```text
Reliable
```

oppure secondo il pattern di rete già canonico del progetto.

Non inventare un trasporto parallelo se esiste già un Fast Decision RPC path.

---

# 18. Latenza

La feature deve essere testata con network latency.

Non trasformare il bank in una misura della qualità della connessione.

Claude deve verificare la policy esistente per:

```text
server-authoritative deadline
client display
latency allowance
late packet handling
```

Se non esiste una policy, registrare una decisione aperta e creare issue dedicata.

NON risolverla introducendo prediction client come autorità.

---

# 19. Match pacing

Il Time Bank va collegato a:

```text
RT-FEAT-MATCH-PACING
```

o FeatureId equivalente reale.

Con baseline proposta:

```text
30 s * 6 players = 180 s
```

In un ipotetico 3v3, il massimo bank aggregato sarebbe 3 minuti di decision time esteso sull'intero match, oltre alle grace window.

NON usare questo calcolo per dichiarare automaticamente la durata finale del match.

Misurare:

```text
reaction opportunities / match
manual prompts / match
average response ms
p50 / p90 response ms
grace-only response %
bank spent / player
bank exhaustion turn
timeouts
resolution wall-clock inflation
```

---

# 20. Possibile refill futuro

Non implementare nell'MVP senza playtest.

Variante futura discussa:

```text
Start Bank = 20 s
Refill per turn = +2 s
Cap = 30 s
```

Questo modello potrebbe:

- premiare decisioni rapide;
- evitare che un errore iniziale rovini tutte le reaction future;
- mantenere pressione senza eliminare il giocatore dal sistema.

Stato:

```text
FUTURE PLAYTEST VARIANT
```

Baseline iniziale:

```text
30 s per match
no refill
```

---

# 21. UI / UX

La UI della Decision Window deve mostrare chiaramente:

```text
- opzioni legali;
- countdown della singola finestra;
- grace vs bank consumption;
- bank residuo;
- timeout fallback;
```

Esempio:

```text
OVERWATCH

[FIRE] [HOLD]

Window: 2.3 s
Decision Bank: 18.4 s
Timeout: HOLD
```

Non sovraccaricare la UI con numeri se i playtest mostrano che peggiora la lettura.

Valutare visuale:

```text
Grace:
neutral / no drain

Bank drain:
bar visibly decreasing

Bank exhausted:
warning state
```

Non affidarsi solo al colore.

Rispettare accessibilità e le convenzioni `Confermato / Previsto / Incerto` dove pertinenti.

---

# 22. Bot

Il bot non deve attendere wall-clock reale.

Per test e AI:

```text
DecisionProvider
Opportunity -> canonical response
```

Possibili policy:

```text
CommitFirstValid
Hold
Timeout
HoldFirstThenCommit
TargetSpecificUnit
TargetHighestPriority
```

Per bot gameplay reale, il Time Bank può essere:

```text
- ignorato come attesa reale;
- simulato come resource/pacing state;
```

ma la policy finale va definita dal sistema bot reale.

Non introdurre sleep/delay artificiali nei test deterministici.

---

# 23. Feature Registry / Feature Map

Prima cercare FeatureId esistenti.

Feature già note da verificare:

```text
RT-FEAT-CORE-DECISION-BOUNDARY
RT-FEAT-REACTION-OPPORTUNITY
RT-FEAT-REACTION-FAST
RT-FEAT-REACTION-OVERWATCH
RT-FEAT-REACTION-MULTI-TRIGGER
RT-FEAT-REACTION-SIMULTANEOUS
RT-FEAT-REACTION-PRIVACY
RT-FEAT-REACTION-FAST-ACTION
RT-FEAT-MATCH-PACING
```

NON duplicarle.

Se manca un owner specifico per il bank, valutare un nuovo FeatureId coerente con il naming reale, ad esempio:

```text
RT-FEAT-REACTION-TIME-BANK
```

oppure:

```text
RT-FEAT-CORE-DECISION-TIME-BANK
```

NON scegliere l'ID definitivo senza audit.

Possibili sub-feature:

```text
Decision Bank Core
Grace Window
Bank Exhaustion
Timeout Policy Integration
Simultaneous Decision Timing
Reaction Clash Timing
UI Bank Presentation
Telemetry
Network Authority
Privacy / Timing Leak
```

Se il registry usa feature aggregate:

> non frammentare artificialmente tutto in dieci FeatureId.

Usare gate/sub-feature/child issue secondo il modello reale.

Per ogni feature aggiornata:

```text
FeatureId
Title
Area
Kind
Release
Priority
Status
Gates
RoadmapRef
Dependencies
OwnerSpecs
Issues
Tests
Scenarios
WikiRefs
LastVerified
```

---

# 24. Roadmap

NON creare una roadmap parallela.

Auditare:

```text
E14
```

Baseline nota dal corpus precedente:

```text
E14 = Decision Window / Fast Reaction / Overwatch
```

È stato anche citato in handoff precedenti come:

```text
E14 / GitHub #152
```

MA il numero reale deve essere verificato a HEAD/GitHub.

Il Time Bank appartiene naturalmente al workstream:

```text
E14
```

salvo audit che mostri un epic diverso già owner del match pacing / timing.

Possibile struttura:

```text
E14 — Decision Window / Fast Reaction / Overwatch
|
+-- Decision Boundary baseline
+-- Reaction Opportunity
+-- Fast Reaction
+-- Overwatch
+-- Decision Time Bank
|   +-- server clock
|   +-- grace
|   +-- drain
|   +-- exhaustion
|   +-- timeout
|   +-- TurnLog/replay
|   +-- UI
|   +-- telemetry
|   +-- privacy/network
|
+-- Reaction Clash extension
```

Collegare anche:

```text
RT-FEAT-MATCH-PACING
```

Non creare nuova Epic se E14 è un contenitore sufficiente.

Creare una Epic separata solo se l'audit dimostra che:

- il Time Bank è un sistema trasversale troppo ampio;
- esiste un workstream pacing/clock già separato;
- tenerlo in E14 crea ownership sbagliata.

---

# 25. Epic / Issue GitHub

Claude DEVE usare GitHub CLI / integrazione disponibile per verificare le issue reali.

Prima:

```bash
gh issue list --state all --limit 500
gh issue list --search "E14"
gh issue list --search "decision window"
gh issue list --search "fast reaction"
gh issue list --search "overwatch"
gh issue list --search "time bank"
gh issue list --search "pacing"
```

Non inventare numeri issue.

Aggiornare issue esistenti quando possibile.

## Candidate issue cluster

Adattare ai titoli/naming reali.

### Issue A — Decision Time Bank core

Obiettivo:

```text
Aggiungere una risorsa temporale per-player condivisa da tutte le Decision Window live.
```

Acceptance:

```text
[ ] bank server-authoritative
[ ] initial bank configurable
[ ] bank floor = 0
[ ] bank consumo solo dopo grace
[ ] max individual window resta bounded
[ ] bank non allunga la Fast Reaction
[ ] deterministic replay usa canonical response, non real timer
[ ] TurnLog registra consumption
```

---

### Issue B — Grace / Exhaustion / Timeout

Acceptance:

```text
[ ] grace configurable
[ ] exhausted behavior explicit
[ ] per-decision timeout policy
[ ] Overwatch timeout = HOLD
[ ] no implicit irreversible commit on timeout
[ ] Functional test per timeout
```

---

### Issue C — Simultaneous Decision Windows

Acceptance:

```text
[ ] independent player windows can run in parallel
[ ] wall-clock is not sum of sequential prompts
[ ] responses collected at same boundary
[ ] stable resolution order
[ ] packet order cannot change result
[ ] permutation test
```

---

### Issue D — Decision Time Bank UI

Acceptance:

```text
[ ] current window timer visible
[ ] bank remaining visible according to approved privacy policy
[ ] grace/drain readable
[ ] timeout fallback visible
[ ] bank exhaustion warning
[ ] keyboard/controller path works
[ ] accessibility: not color-only
```

---

### Issue E — Time Bank telemetry / pacing validation

Metrics:

```text
DecisionOpened
ResponseMs
GraceOnly
BankSpentMs
BankRemaining
Timeout
Exhaustion
OpportunityCount
PromptsPerTurn
PromptsPerMatch
ResolutionWallClock
```

Acceptance:

```text
[ ] scenario report exports metrics
[ ] p50/p90 response time measurable
[ ] bank usage per player measurable
[ ] no sensitive hidden info in public telemetry
```

---

### Issue F — Privacy / timing side-channel audit

Acceptance:

```text
[ ] evaluate public vs team vs owner-only bank
[ ] canary privacy test
[ ] hidden reaction does not leak through public bank update
[ ] spectator / late join checked
[ ] packaged network test when networking milestone applies
```

---

### Issue G — Reaction Clash + Time Bank integration

Solo se Reaction Clash è nella roadmap corrente.

Acceptance:

```text
[ ] simultaneous hidden choices use each player's bank
[ ] no early reveal
[ ] timeout policy explicit for both sides
[ ] commit/reveal deterministic
[ ] replay records canonical choices
```

---

# 26. Scenario Map

La Scenario Map deve avere relazioni bidirezionali:

```text
Feature -> Scenario
Scenario -> Feature
Issue -> Scenario
Scenario -> Test
Wiki -> Scenario
Roadmap -> Scenario
```

Non creare ID in conflitto col sistema reale.

## Scenario candidate 1 — Grace does not drain

```text
DecisionTimeBank.Grace.NoDrain
```

Setup:

```text
bank = 30s
grace = 1s
response = 0.7s
```

Expected:

```text
bank remains 30s
```

---

## Scenario candidate 2 — Drain after grace

```text
DecisionTimeBank.Drain.AfterGrace
```

Setup:

```text
response = 2.4s
grace = 1.0s
```

Expected:

```text
bank consumed = 1.4s
```

---

## Scenario candidate 3 — Overwatch Hold / Hold / Fire

```text
Reaction.Overwatch.TimeBank.HoldHoldFire
```

Sequence:

```text
Tank -> HOLD
Scout -> HOLD
Carry -> FIRE
```

Assert:

```text
HOLD keeps reaction armed
bank decreases only for decision time beyond grace
FIRE consumes charge
future trigger count never exposed
```

---

## Scenario candidate 4 — Bank exhaustion

```text
DecisionTimeBank.Exhaustion
```

Expected:

```text
remaining bank = 0
next decision only gets exhausted-response policy
timeout fallback applied
bank never negative
```

---

## Scenario candidate 5 — Timeout maps to Overwatch HOLD

```text
DecisionTimeBank.Overwatch.TimeoutHold
```

Assert:

```text
no FIRE
no charge consumed
reaction stays armed if policy allows
canonical timeout decision logged
```

---

## Scenario candidate 6 — Simultaneous players

```text
DecisionTimeBank.SimultaneousPlayers
```

Setup:

```text
A and B receive independent windows at same boundary
```

Assert:

```text
both clocks run concurrently
resolution waits for both or deadlines
wall-clock ~= max, not sum
stable result independent of response packet order
```

---

## Scenario candidate 7 — Same micro-step Overwatch targets

Reuse existing reaction scenario if present.

```text
Reaction.Overwatch.SameStepMultiTarget
```

Assert:

```text
one opportunity
FIRE A / FIRE B / HOLD
one owner Decision Window
one Time Bank drain
```

---

## Scenario candidate 8 — Reaction Clash

```text
Reaction.Clash.TimeBank.HiddenSimultaneous
```

Assert:

```text
two banks drain independently
choices hidden
reveal at correct boundary
same choices -> same result/hash
```

---

## Scenario candidate 9 — Forged timestamp

```text
DecisionTimeBank.Network.RejectForgedTiming
```

Client claims false response time.

Assert:

```text
server ignores client timing claim
server-authoritative duration used
```

---

## Scenario candidate 10 — Late response

```text
DecisionTimeBank.Network.LateResponse
```

Assert:

```text
response after deadline rejected
timeout response remains canonical
duplicate/late packet cannot mutate result
```

---

## Scenario candidate 11 — Disconnect during Decision Window

```text
DecisionTimeBank.Network.Disconnect
```

Policy may still be open.

Test must at least validate:

```text
no deadlock
deterministic fallback
TurnLog reason
```

---

## Scenario candidate 12 — Public bank privacy

```text
DecisionTimeBank.Privacy.HiddenReaction
```

If an owner receives a private/hidden-info Decision Window:

Assert Team B must NOT infer forbidden information through:

```text
bank delta
RPC payload
property replication
packet-specific public events
log
spectator state
```

Use canary technique.

---

## Scenario candidate 13 — Long match pacing

```text
DecisionTimeBank.Pacing.ScriptedMatch
```

Script many opportunities.

Collect:

```text
total prompts
total wall-clock decision time
bank spent
timeouts
bank exhausted turn
```

Non usare come golden determinism test con real sleep; simulare response timestamps o usare dedicated Functional/Timing test.

---

# 27. Test automatici

Aggiungere o pianificare test equivalenti a:

```text
RefactorTactics.DecisionTimeBank.GraceDoesNotConsume
RefactorTactics.DecisionTimeBank.ConsumesAfterGrace
RefactorTactics.DecisionTimeBank.NeverBelowZero
RefactorTactics.DecisionTimeBank.ExhaustionUsesFallback
RefactorTactics.DecisionTimeBank.TimeoutCanonical
RefactorTactics.DecisionTimeBank.OverwatchTimeoutIsHold
RefactorTactics.DecisionTimeBank.HoldKeepsReactionArmed
RefactorTactics.DecisionTimeBank.SimultaneousWindowsOrderIndependent
RefactorTactics.DecisionTimeBank.PacketOrderInvariant
RefactorTactics.DecisionTimeBank.ReplayDoesNotUseWallClock
RefactorTactics.DecisionTimeBank.RejectsLateResponse
RefactorTactics.DecisionTimeBank.RejectsWrongOwner
RefactorTactics.DecisionTimeBank.PrivacyNoHiddenWindowLeak
```

Adattare al naming reale.

Attenzione: evitare test Unreal con nome eseguibile che sia prefisso gerarchico di un altro test.

Golden tests:

```text
NO real sleep
NO wall-clock dependency
```

Functional/UI/network tests:

```text
possono usare il timer reale quando il timer stesso è ciò che si verifica.
```

---

# 28. Debug tooling

Verificare console/logging esistente.

Possibili comandi, solo se coerenti col naming reale:

```text
rt.Decision.Dump
rt.Decision.DumpBank
rt.Decision.DebugWindows
rt.Decision.ForceTimeout
rt.Decision.SetBank
```

Solo per dev/test.

Overlay debug possibile:

```text
DecisionId
Owner
Type
OpenedAt
Deadline
GraceRemaining
BankBefore
BankRemaining
AllowedResponses
CanonicalResponse
```

Non mostrare dati server-only ai client shipping.

---

# 29. Data / Ruleset

I numeri devono diventare data-driven nel posto corretto, non hard-coded sparsi.

Possibile ruleset:

```text
DecisionTimingPolicy
{
    InitialBankMs
    GraceMs
    MaxWindowMs
    ExhaustedGraceMs
    RefillPerTurnMs
    MaxBankMs
}
```

Questo è concettuale.

Prima verificare come il progetto rappresenta:

```text
RulesetDefinition
ResolverConfig
Data Assets
match rules
```

Non introdurre un secondo sistema di configurazione.

I parametri competitivi devono contribuire a:

```text
RulesVersion
ResolverConfigHash
ContentManifestHash
```

secondo il modello reale.

---

# 30. Wiki

Aggiornare la Wiki esistente.

Pagine/sezioni candidate:

```text
Reactions
Fast Reaction
Overwatch
Brace
Decision Windows
Match Timing / Pacing
Competitive Rules
```

La Wiki deve spiegare player-facing:

```text
- le decisioni live sono brevi;
- ogni giocatore possiede una riserva temporale;
- decidere rapidamente conserva la riserva;
- la singola reaction resta comunque limitata;
- quando la riserva termina si applica la policy di timeout/risposta rapida;
- Overwatch timeout = HOLD;
```

NON duplicare nella Wiki i dettagli server-only.

NON copiare manualmente lo status roadmap.

Ogni pagina feature deve referenziare:

```text
FeatureId
Roadmap
Scenario
Issue/Epic
```

secondo la convenzione reale.

Se `public bank visibility` è ancora OPEN, la Wiki non deve presentarla come regola definitiva.

---

# 31. Documentazione tecnica da aggiornare

Aggiornare solo sezioni rilevanti dei documenti reali.

Aree:

```text
Architecture
Deterministic Simulation
Networking / Privacy
UI / UX
Reaction System
Overwatch
Brace / Reaction
Match Timing / Pacing
TurnLog / Replay
Scenario/Test Harness
Roadmap / QA / Risk Register
```

Non riscrivere interi PDR senza motivo.

Documentare esplicitamente:

```text
Decision Time != Simulation Time
```

e:

```text
real-time deadline -> canonical decision -> deterministic replay
```

---

# 32. Risk Register

Aggiungere o consolidare rischi equivalenti.

## TIMEBANK-01 — Reaction spam still inflates wall-clock

Mitigazione:

```text
MaxPromptsPerReaction
+
Time Bank
+
parallel independent windows
+
telemetry
```

---

## TIMEBANK-02 — Bank punishes UI/perception latency

Mitigazione:

```text
Grace Window
UX profiling
accessibility
```

---

## TIMEBANK-03 — Network latency consumes player resource

Mitigazione:

```text
server timing policy
latency tests
no client-authored elapsed time
```

---

## TIMEBANK-04 — Public bank leaks hidden reaction activity

Mitigazione:

```text
privacy review
canary tests
owner/team/public policy
```

---

## TIMEBANK-05 — Too little bank removes meaningful reactions late match

Mitigazione:

```text
playtest
possible refill variant
telemetry
```

---

## TIMEBANK-06 — Too much bank fails anti-stall goal

Mitigazione:

```text
pacing scenario
p90 metrics
match duration measurement
```

---

## TIMEBANK-07 — Sequential simultaneous prompts multiply pause time

Mitigazione:

```text
concurrent windows at same logical boundary
```

---

# 33. Definition of Done

Il Time Bank NON è Done perché il countdown appare in UMG.

Gate:

```text
[ ] design/spec consolidated
[ ] feature registry linked
[ ] roadmap linked
[ ] server-authoritative bank runtime
[ ] Decision Window integration
[ ] timeout/fallback integration
[ ] deterministic replay
[ ] TurnLog/debug
[ ] automation tests
[ ] scenario coverage
[ ] UI
[ ] telemetry
[ ] privacy/network validation when applicable
[ ] packaged validation when applicable
[ ] Wiki updated
[ ] Epic/issues linked
```

Usare i gate reali del Feature Registry se già esistono.

---

# 34. Procedura operativa richiesta a Claude

Eseguire nell'ordine.

## Step 1 — Audit

Identificare:

```text
Repository
Branch
HEAD
UE version
Feature Registry owner
Roadmap owner
Reaction owner spec
Decision Window ADR
Pacing owner spec
Scenario Registry
Wiki repository
GitHub Epic/Issues
Runtime Decision Window implementation
Tests
```

---

## Step 2 — Conflict report

Cercare conflitti come:

```text
5s interrupt
3s Fast Reaction
per-window timer only
future per-player bank
reaction-specific timers
public/private timing
```

Classificare:

```text
CURRENT
STALE
PROPOSED
CONFLICT
```

Non cancellare history utile senza redirect/provenance.

---

## Step 3 — Decision log / ADR

Registrare l'idea usando il prossimo ID reale.

Separare le decisioni:

```text
A. Esiste un per-player Decision Time Bank condiviso fra Decision Window.
B. La singola Decision Window resta bounded e non usa tutto il bank.
C. Grace prima del consumo.
D. Timeout è server-authoritative e definition-specific.
E. Simultaneous independent windows possono scorrere in parallelo.
F. Public visibility del bank resta OPEN finché privacy review non è chiusa.
G. Valori 30s / 1s / 3s sono playtest baseline.
```

---

## Step 4 — Feature Registry / Feature Map

Aggiornare feature esistenti e aggiungere eventuale bank owner.

Collegare:

```text
Decision Boundary
Reaction Opportunity
Fast Reaction
Overwatch
Reaction Clash
Match Pacing
Networking Privacy
UI
Telemetry
Scenario Harness
```

---

## Step 5 — Roadmap

Integrare nella roadmap esistente.

Prima verificare:

```text
E14
```

e l'eventuale issue GitHub reale.

Non inventare una nuova milestone se basta un checkpoint/sub-feature.

---

## Step 6 — Epic / Issues

Cercare e aggiornare quelle esistenti.

Creare solo quelle mancanti.

Dopo la creazione:

```text
- aggiornare Feature Registry con numeri/URL reali;
- aggiornare roadmap;
- aggiornare Scenario Map;
- aggiungere backlink Wiki.
```

---

## Step 7 — Scenario Map

Creare/consolidare gli scenari del §26.

Riutilizzare scenari Reaction / Overwatch esistenti quando coprono già il caso.

Non duplicare lo stesso scenario con un nuovo nome.

---

## Step 8 — Tests

Aggiungere test automatici solo se il runtime corrispondente esiste nella tranche corrente.

Per lavoro solo documentale:

```text
creare test plan + issue
```

NON creare fake test verdi che verificano solo una costante.

---

## Step 9 — Wiki

Aggiornare player-facing e cross-reference.

Non esporre server internals o numeri `PROPOSED` come definitivi.

---

## Step 10 — Validation

Eseguire i validator reali del repo.

Controllare:

```text
FeatureId duplicati
link rotti
ScenarioId duplicati
issue mancanti
Wiki refs mancanti
roadmap refs stale
feature DONE senza gate
numeric values duplicati in più source of truth
```

---

# 35. Output finale obbligatorio di Claude

Restituire un report con questa struttura.

## A. Audit

```text
Repository
Branch
HEAD
UE version
Canonical roadmap
Feature Registry
Scenario Registry
Reaction spec
Decision Window ADR
Pacing spec
Wiki repo
GitHub state
```

## B. Decisioni

Tabella:

```text
Decision
Status: CANONICAL | PROPOSED FOR PLAYTEST | OPEN
Owner
Conflict found
Action taken
```

## C. File modificati

Per ogni file:

```text
Path
Why
What changed
Source of truth / generated view
```

## D. Feature Registry

```text
FeatureId
Title
Status
Gates
RoadmapRef
Issues
Tests
Scenarios
WikiRefs
Dependencies
```

## E. Roadmap

```text
Epic/Checkpoint
Scope added
Dependencies
Exit gate
Deferred items
```

## F. Epic / Issues

```text
Existing updated
New created
Duplicates avoided
Actual GitHub numbers/URLs
```

## G. Scenario Map

```text
ScenarioId
FeatureIds
Purpose
Automated
Issue
Roadmap
Status
```

## H. Tests

```text
Test
Level
Feature
Expected
Status
```

## I. Wiki

```text
Pages updated
Links added
Open rules not published as canonical
```

## J. Privacy review

Rispondere esplicitamente:

```text
Can remaining bank be public live without leaking hidden decision activity?
YES / NO / CONDITIONAL
Evidence/tests
Recommended policy
```

## K. Pacing / telemetry

```text
Metrics added/planned
Baseline values
Playtest questions
```

## L. Open decisions

Minimo:

```text
- Initial bank definitivo?
- Grace definitiva?
- Exhausted grace definitiva?
- Refill sì/no?
- Public/team/owner visibility?
- Network latency policy?
- Disconnect timeout policy?
```

## M. Suggested commits

Commit focalizzati, per esempio:

```text
docs(reactions): define decision time bank playtest model
docs(roadmap): integrate decision bank into E14 and match pacing
docs(scenarios): add decision timing validation matrix
docs(wiki): document fast decision time bank
test(reactions): add decision bank timing scenarios
chore(project): link decision bank issues and feature registry
```

Adattare ai file realmente modificati.

---

# 36. Guardrail

NON:

```text
- creare un secondo Reaction System;
- creare un Time Bank per ogni abilità;
- estendere una singola Fast Reaction a 30 secondi;
- usare timer client come autorità;
- usare real-time clock nel golden replay;
- consumare automaticamente FIRE su timeout Overwatch;
- sequenzializzare artificialmente decisioni indipendenti simultanee;
- rivelare hidden Reaction Opportunity tramite bank/UI/network;
- usare packet arrival order come tie-break;
- creare feature/epic/issue duplicate;
- dichiarare 30s/1s/0.75s definitivi senza playtest;
- promuovere public bank visibility a canon senza privacy review;
- cambiare il formato match definitivo;
- reintrodurre interrupt storici da 5 secondi come baseline;
- modificare roster o skill non necessarie a questo task;
- anticipare modding, matchmaking o progression.
```

---

# 37. Criterio finale

La feature deve ottenere questo risultato:

```text
Decision Window
     |
     +--> immediate player response
     |       -> no / low bank cost
     |
     +--> player thinks longer
             -> bank drains
                    |
                    v
             finite match-wide resource
```

senza rompere:

```text
determinism
server authority
privacy
Overwatch HOLD mindgame
Reaction Clash secrecy
match pacing
replay
TurnLog
scenario automation
```

Il Time Bank non deve essere solo un anti-AFK timer.

Deve diventare un **budget di decisione competitivo**, controllato, misurabile e integrato nel sistema generale di Decision Window.

---

# 38. Primo risultato atteso

Dopo l'audit, il primo consolidamento dovrebbe produrre una vista simile:

```text
RT-FEAT-CORE-DECISION-BOUNDARY
        |
        v
RT-FEAT-REACTION-FAST
        |
        +----------------------+
        |                      |
        v                      v
Decision Time Bank       Overwatch
        |                      |
        +----------+-----------+
                   |
                   v
             Match Pacing
                   |
        +----------+-----------+
        |                      |
        v                      v
    UI / UX             Telemetry / QA
        |
        v
Network / Privacy
```

La struttura finale deve però seguire Feature Registry e Roadmap reali del repository, non questo diagramma se il progetto usa ownership differenti.
