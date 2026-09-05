# RefactorTactics — Handoff per Claude
## Roadmap P0/P1 — Reaction Hardening, Decision Boundaries, Determinismo, Privacy e Pacing

**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Data handoff:** 2026-09-04  
**Obiettivo:** aggiornare/creare le issue GitHub necessarie a rendere ad alta priorità le sistemazioni emerse dalla revisione critica del Reaction System, senza duplicare responsabilità già presenti.

---

# ⚠️ AUDIT 2026-09-05 — questo handoff è superato in quattro punti

Verificato sul repository e su GitHub il **2026-09-05**, non a memoria. Le sezioni che seguono restano leggibili come cronaca; dove contraddicono questo blocco, **vince questo blocco**.

Regola applicata: la §11.8 di questo stesso documento — «se il codice o issue corrente contraddicono questo handoff, NON inventare la correzione: segnala il conflitto nella issue tracker principale». Il tracker è **#2462**.

## A. Tre issue del critical path erano già chiuse

| Issue | §1 e §3 di questo doc | Stato misurato 2026-09-05 |
|---|---|---|
| #1879 | promuovere a P0 | **CLOSED / COMPLETED** — 2026-09-04 12:11 |
| #1880 | «mantenere il blocco duro» | **CLOSED / COMPLETED** — 2026-09-04 06:47 |
| #2189 | promuovere a P0 | **CLOSED / COMPLETED** — 2026-09-04 07:58, via #2261 |

**R4, R5 e R6 di §5 sono consegnati.** Questo documento è stato scritto lo stesso giorno in cui quelle tre si chiudevano, e ne ha fotografato lo stato precedente.

## B. Metà delle «nuove issue» di §4 descriveva codice già in produzione

| Proposta | Realtà misurata |
|---|---|
| NUOVA 2 — Canonical Event → Opportunity → Boundary | `FRTReactionOpportunityKey` esiste (CP 14.3): id **funzione dello stato**, non GUID, replay-stabile. `AllowedResponses` ha già la semantica `<= 1` auto / `>= 2` boundary — `RTReactionOpportunityTypes.cpp:64`. Marcato `RTServerOnly`, fonte [D-021] |
| NUOVA 3 — indexed trigger registry | Il costo `unità × reaction × target × micro-step` **non è stato misurato**. Prima serve uno spike su `BuildOverwatchTriggers` (`RTReactionOpportunityTypes.h:669`, chiamato da `RTTurnManager.cpp:5093`) |
| NUOVA 4 — «non usare `RevalidateEverything()`» | Quella funzione **non esiste**. La premessa non regge come scritta |
| §7 — test da creare | Molti esistono già con altri nomi: `Reactions.MultipleTriggersStillOnce`, `Overwatch.SimultaneousTargetsSingleOpportunity`, `Overwatch.OrderIsDeterministic`, `Reactions.OpportunityIdIsDerivedNotRandom`, `Overwatch.OpportunityLeaksNoFuture` — 57 in tutto |

**NUOVA 2, 3 e 4 non sono state create.** Crearle avrebbe duplicato sottosistemi già spediti, contro `SEARCH → REUSE → CREATE`.

## C. I nomi delle metriche di §6 non sono quelli del repository

§6 usa nomi puntati (`Reaction.DecisionSeconds`, `Resolution.PlaybackSeconds`). Il repository usa `ReactionDecisionSeconds` e `ResolutionPlaybackSeconds`, e la §7 di questo stesso documento chiede di adattarsi alle convenzioni reali.

Cercate con i nomi nativi: **1 metrica su 8 esiste** — `ReactionDecisionSeconds`, 7 occorrenze in `Source/`. Le altre sette a zero.

E il pacing **è già stato misurato**: il Deliverable A di #166 è atterrato con PR #1554 il 2026-08-28 — 9,0 / 18,0 / 24,0 s con 1, 2 e 3 unità armate dello stesso giocatore. Ciò che manca è il **costo** della reaction, non il suo ritmo.

## D. Reaction Clash è P3 ma ha già otto test

Esistono 8 `RefactorTactics.Clash.*` mentre #314 è `P3` e questo documento la vuole fuori dalla baseline. Da riconciliare: o i test coprono una baseline che non sapevamo di avere, o #314 è più avanti della sua label.

## Cosa è stato eseguito

| Azione di questo documento | Esito |
|---|---|
| §3.A — #152 P2 → **P0** | fatto, + sezione guardrail nel corpo |
| §3.B — #166 P2 → **P0** | fatto, + AC di aggregazione UI e `ResolutionPlaybackSeconds` |
| §3.C e §3.D — #1879 e #2189 a P0 | **non applicabile**: chiuse come COMPLETED |
| §3.E — #759 collegata come guardrail | fatto nel tracker, owner **non** spostato |
| §3.F — #314 e #319 restano P3 | già vero, nessuna azione |
| §4 NUOVA 1 — tracker | creato: **#2462**, con critical path corretto |
| §4 NUOVA 2, 3, 4 | **non create** — vedi B |
| gap misurati e senza owner | **#2459** (costo) e **#2461** (invarianti 3 e 8) |

## Ordine di implementazione corretto — sostituisce §5

```text
✅ FATTO   R4 boundary identity            #1880
✅ FATTO   R5 boundary determinism         #2189
✅ FATTO   R6 runtime stepping             #1879
✅ FATTO   canonical opportunity contract  CP 14.3

🔴 APERTO  invarianti 3 e 8                #2461
🔴 APERTO  costo della reaction            #2459
🔴 APERTO  UI, aggregazione e pacing       #166
🟡 SPIKE   esiste davvero un global scan?  misurare prima di indicizzare
⏭️ POST    privacy temporale multiplayer   #759, owner separato

P3, NON BLOCCANTI
     #314 Reaction Clash · #319 Decision Time Bank
```

---

## 0. Mandato operativo

Agisci direttamente sul repository GitHub.

Prima di creare qualsiasi issue:

1. cerca issue aperte e chiuse con termini correlati;
2. riusa issue esistenti quando possiedono già la responsabilità;
3. non creare duplicati;
4. conserva la storia e le decisioni già presenti nei body;
5. aggiungi sezioni additive oppure aggiorna in modo chirurgico ciò che è chiaramente stale;
6. non cambiare regole di gameplay non richieste;
7. non usare nomi o comportamenti delle skill/personaggi attuali come fondamento del sistema: i kit sono in revisione;
8. considera Reaction Clash e Decision Time Bank come estensioni P3, non baseline MVP.

Il lavoro è **alta priorità**. La baseline del Reaction System e i suoi invarianti tecnici devono essere trattati come **P0**.

---

# 1. Stato già verificato

> ⛔ **Stale su tre righe.** #1879, #1880 e #2189 sono CLOSED / COMPLETED dal 2026-09-04. Vedi l'AUDIT 2026-09-05 in testa.

## Issue esistenti da RIUSARE

### #152 — `[EPIC v0.1] E14 — Overwatch e reazioni interattive`

Stato verificato:
- OPEN
- attualmente `P2`
- milestone `v0.1 · Percezione e reazioni`
- baseline aperta: CP 14.6
- estensioni:
  - #314 — Reaction Clash — P3
  - #319 — Decision Time Bank — P3

### #166 — `CP 14.6 — Counterplay, UI della finestra e misura del pacing`

È il proprietario naturale di:
- decision UI;
- countdown;
- pacing reale;
- metriche di tempo;
- DTO sanitizzato;
- nessuna logica gameplay nel widget;
- nessuna informazione della finestra inviata all'avversario.

### #1879 — `v0.1 · Resolution controls Vs Bot/Debug — Pause al boundary e StepMicroStep`

Possiede già il contratto:
- pause solo a safe boundary;
- un micro-step è una barriera logica atomica;
- `StepMicroStep` esegue un intero boundary;
- eventi simultanei non possono essere spezzati;
- niente metà stato visibile;
- StableId non deve diventare priorità gameplay.

### #1880

È il prerequisito indicato da #2189 per rendere i boundary della traccia **indirizzabili**.

Verificare titolo/body aggiornato prima di modificarla.

### #2189 — `Determinismo: lo StateHash esiste solo a fine partita — nessun boundary ha un checksum...`

Possiede già:
- checksum per boundary;
- riuso di `URTMatchStateHashLibrary`;
- identificazione del primo boundary divergente;
- anti-vacuity test;
- nessun checksum per singola voce;
- dipendenza dura da #1880.

Attualmente la priorità è ereditata come P2 e il body stesso dice che va ritriagiata se il gate appartiene al determinismo core.

### #759 — `Privacy temporale: la finestra non deve essere deducibile dal ritmo osservato`

È owner della privacy temporale multiplayer:
- payload nemico non contiene dati della finestra;
- ritmo/pause non devono rivelarne l'esistenza;
- timeout e risposta devono restare server-authoritative;
- il test multiplayer reale è post-v0.1.

NON duplicare questa issue.

---

# 2. Decisioni architetturali da trasformare in roadmap

La baseline da proteggere è:

```text
Canonical Event
    ↓
Indexed Trigger Lookup
    ↓
Reaction Candidates
    ↓
Validation
    ↓
Reaction Opportunity
    ↓
AllowedResponses
    ├─ 0/1 → canonical auto-resolution
    └─ 2+  → one Decision Boundary
                  ↓
            canonical decision
                  ↓
                 apply
                  ↓
       targeted revalidation
                  ↓
                resume
```

## Invarianti

1. Non ogni state change genera una Reaction Opportunity.
2. Il trigger evaluation point è distinto dal player decision point.
3. Una reaction non modifica retroattivamente un evento già committed.
4. Tutti i trigger dello stesso micro-step/boundary vengono raccolti prima della risoluzione.
5. Più target nello stesso micro-step devono produrre una singola opportunity multi-target quando semanticamente appartengono allo stesso evento.
6. Nessun ordine gameplay può dipendere da Tick, frame rate, animazioni, TMap/TSet iteration order, Actor order, packet arrival o UI timing.
7. StableId può essere soltanto il tie-break tecnico finale, non una priorità gameplay implicita.
8. Nessun nested Decision Boundary nell'MVP.
9. Nessun reaction stack LIFO interattivo nell'MVP.
10. Reaction Clash resta P3.
11. Decision Time Bank resta P3 finché CP 14.6 non produce misure reali.
12. La revalidation deve essere mirata, non globale.
13. Nessun global scan `unit × reaction × target` a ogni micro-step.
14. Il sistema deve essere event-driven, non Tick-driven.
15. La presentazione può interpolare; lo stato logico è discreto e canonico.

---

# 3. Modifiche richieste alle issue esistenti

## A. #152 — promuovere baseline Reaction System a P0

Cambiare la priorità dell'epic da `P2` a **`P0`** oppure, se la policy del repository impedisce di rendere tutta E14 P0, esplicitare chiaramente che:

- **CP 14.1–14.6 = baseline P0**
- **CP 14.7 e 14.8 = P3 extension**

Preferenza: se la label deve essere unica, usa `P0` sull'epic e mantieni #314/#319 P3.

Aggiungere una sezione:

```md
## 🔴 Reaction hardening — critical path

La baseline CP 14.1–14.6 è parte del core deterministico della resolution.

Guardrail:
- no nested reaction boundary;
- no Reaction Clash nella baseline;
- no Decision Time Bank nella baseline;
- no global reaction scan;
- no Tick-driven reaction logic;
- no dipendenza dai kit correnti;
- StableId solo come tie-break tecnico finale;
- stessa boundary state per tutti i candidate dello stesso boundary;
- targeted revalidation dopo state-changing reaction.
```

Collegare #166, #1879, #1880, #2189, #759 e le nuove issue descritte sotto.

## B. #166 — promuovere a P0 e trasformarla nel gate UX/pacing della baseline

Cambiare `P2` → **`P0`**.

Aggiungere Acceptance Criteria:

```md
### Reaction UX / pacing hardening

- [ ] nessun prompt interattivo quando `AllowedResponses <= 1`;
- [ ] trigger multipli semanticamente appartenenti allo stesso boundary vengono aggregati in una sola decisione;
- [ ] nessuna sequenza di popup generata dall'ordine di iterazione dei candidate;
- [ ] misurare `ReactionOpportunitiesPerTurn`;
- [ ] misurare `DecisionBoundariesPerTurn`;
- [ ] misurare `ReactionDecisionSeconds` p50/p90;
- [ ] misurare il wall-clock totale speso in interruption durante una resolution;
- [ ] registrare almeno un caso stress con più candidate nello stesso movement micro-step;
- [ ] se più choice possono essere mostrate nella stessa finestra senza cambiare semantica, la UI deve aggregarle.
```

Non implementare Time Bank qui.

## C. #1879 — promuovere a P0

Cambiare `P2` → **`P0`**.

Aggiungere esplicitamente che il boundary di Pause/Step deve essere lo **stesso primitive boundary canonico** usato dal Reaction System.

Acceptance Criteria aggiuntivi:

```md
- [ ] Reaction evaluation non introduce un secondo movement stepper;
- [ ] un Decision Boundary può esistere solo tra due stati canonici completi;
- [ ] nessuna reaction può osservare o modificare una metà di micro-step;
- [ ] tutti i candidate dello stesso micro-step vedono la stessa boundary state;
- [ ] una response applicata può sporcare solo i revision domain dichiarati;
- [ ] StepMicroStep e normale playback producono gli stessi boundary hash.
```

## D. #2189 — promuovere a P0

Cambiare priorità a **P0** e mantenere il blocco duro #1880.

Aggiungere:

```md
- [ ] stessa simulazione con candidate enumeration intenzionalmente permutata produce gli stessi boundary hash;
- [ ] stesso snapshot/regole/versione/seed/decision log produce la stessa sequenza di boundary hash;
- [ ] una divergence introdotta dopo una reaction identifica il primo boundary errato;
- [ ] gli eventi simultanei dello stesso boundary restano un gruppo anche nel confronto.
```

## E. #759 — NON cambiare owner, ma collegarla come guardrail

Non duplicare il lavoro.

La baseline v0.1 deve evitare API che obblighino il multiplayer futuro a rivelare trigger privati, opportunity, responder, AllowedResponses, decision timing o timeout timing. La prova completa del timing side-channel resta #759.

## F. #314 e #319

Mantenerle **P3**. Non promuoverle.

---

# 4. Nuove issue da creare

Crearle solo dopo ricerca duplicati.

## NUOVA 1 — Tracker principale

### Titolo
`[P0][ROADMAP] Reaction hardening — boundary, determinismo, privacy, performance e pacing`

### Label
- `P0`
- `v0.1`
- `epic` oppure label equivalente

### Body essenziale

```md
## Obiettivo

Rendere la baseline delle reazioni parte del core deterministico della Resolution, eliminando dipendenze da frame/tick/order, candidate explosion, revalidation globale e interruption spam.

## Critical path

1. Canonical reaction event contract
2. Indexed candidate collection
3. BoundaryContext + targeted revalidation
4. #1879 — safe boundary / StepMicroStep
5. #1880 — boundary indirizzabile
6. #2189 — checksum per boundary
7. #166 — UI/pacing/metriche
8. #759 — privacy temporale multiplayer, owner separato

## Guardrail MVP

- no Reaction Clash;
- no Decision Time Bank;
- no nested Decision Boundary;
- no dynamic dependency graph generale;
- no global reaction scan;
- no gameplay multithreading;
- no Tick-driven reaction evaluation;
- nessuna dipendenza da skill/personaggi correnti.

## Gate

- [ ] tutti i boundary sono atomici;
- [ ] candidate collection deterministica;
- [ ] enumeration-order independence test verde;
- [ ] boundary hash sequence deterministica;
- [ ] targeted revalidation;
- [ ] candidate/event e CPU/boundary misurati;
- [ ] opportunities/turn e boundaries/turn misurati;
- [ ] prompt aggregati;
- [ ] nessun leak attraverso DTO;
- [ ] architettura compatibile con #759 senza replica di planning/reaction data privata.
```

## NUOVA 2 — Canonical Event / Opportunity contract

### Titolo
`[P0][Reactions] Canonical Event → Opportunity → Decision Boundary contract`

Definire il contratto C++ unico fra simulatore e Reaction System.

Evaluation point canonici suggeriti:

```text
ActionBoundary
MovementStepResolved
ImpactResolved
StateTransitionResolved
TurnBoundary
```

Eventi derivati come `CellEntered`, `SegmentCrossed`, ecc. possono essere trigger facts raccolti dentro il boundary, non necessariamente Decision Boundary autonomi.

### Acceptance Criteria

```md
- [ ] esiste un tipo/contratto canonico per il boundary event;
- [ ] trigger fact e Decision Boundary sono concetti distinti;
- [ ] un evento committed non può essere modificato retroattivamente;
- [ ] tutti i trigger facts dello stesso boundary sono raccolti prima della candidate resolution;
- [ ] più target nello stesso boundary possono produrre una sola multi-target opportunity;
- [ ] `AllowedResponses <= 1` non apre UI;
- [ ] `AllowedResponses >= 2` può aprire al massimo un Decision Boundary per opportunity;
- [ ] nessun Decision Boundary annidato;
- [ ] TurnLog identifica event/boundary/opportunity in modo stabile;
- [ ] Automation Test per aggregazione e ordering independence.
```

Out of scope: skill specifiche, Reaction Clash, Time Bank, networking completo, UI finale.

## NUOVA 3 — Indexed trigger registry

### Titolo
`[P0][Reactions] Indexed trigger registry e deterministic candidate collection`

### Problema

Evitare il costo:

```text
unità × reaction × target × micro-step
```

### Soluzione

```text
EventType
  → registered reaction sources
      → spatial/team/state filters
          → deterministic candidate vector
```

### Acceptance Criteria

```md
- [ ] nessun global scan di tutte le reaction a ogni micro-step;
- [ ] candidate collection usa indice per event/trigger category;
- [ ] candidate vector canonicalizzato prima della validation;
- [ ] ordine TMap/TSet non influenza il risultato;
- [ ] StableId usato soltanto come tie-break finale;
- [ ] test con insertion order permutato;
- [ ] metriche `CandidatesPerEvent` p50/p90;
- [ ] metriche CPU per candidate collection;
- [ ] scenario stress con più unità e trigger nello stesso step;
- [ ] nessuna allocazione incontrollata per candidate in hot path.
```

## NUOVA 4 — BoundaryContext e targeted revalidation

### Titolo
`[P0][Resolver] BoundaryContext e targeted revalidation per revision domains`

### Problema

Non usare `RevalidateEverything()` né validator concorrenti su snapshot diversi.

### BoundaryContext suggerito

- snapshot/revision identity;
- occupancy revision;
- movement/path revision;
- LOS/visibility revision;
- cover/geometry revision;
- unit-state revision;
- objective revision, se rilevante.

Una reaction applicata restituisce i domain dirty e il resolver invalida solo cache/azioni dipendenti.

### Acceptance Criteria

```md
- [ ] tutti i candidate dello stesso boundary vengono validati contro la stessa boundary state;
- [ ] non esistono validator che leggono stato intermedio del boundary;
- [ ] una reaction restituisce esplicitamente i domain modificati;
- [ ] revalidation solo dei consumer dipendenti;
- [ ] nessun `RevalidateEverything()` nel path normale;
- [ ] test: reaction che cambia posizione invalida movement/LOS necessari ma non consumer indipendenti;
- [ ] test: reaction che modifica solo status non forza pathfinding se non necessario;
- [ ] contatore `RevalidationCountPerBoundary`;
- [ ] metriche CPU della revalidation.
```

Out of scope: dependency graph dinamico generale, ECS, multithreading gameplay.

---

# 5. Ordine di implementazione raccomandato

> ⛔ **Superato.** R4, R5 e R6 sono consegnati. L'ordine valido è quello nell'AUDIT 2026-09-05 in testa.

```text
R0 — Triage / labels
     #152 #166 #1879 #2189 → P0

R1 — Canonical Event Contract
     nuova issue Canonical Event → Opportunity → Boundary

R2 — Indexed Candidate Collection
     nuova issue trigger registry

R3 — BoundaryContext / Dirty Domains
     nuova issue targeted revalidation

R4 — Boundary identity
     #1880

R5 — Boundary determinism
     #2189

R6 — Runtime stepping integration
     #1879

R7 — UI + pacing
     #166

R8 — PIE + stress metrics

POST
     #759 multiplayer temporal privacy

P3 / NON BLOCCANTI
     #314 Reaction Clash
     #319 Decision Time Bank
```

---

# 6. Metriche obbligatorie

```text
Reaction.CandidatesPerEvent
Reaction.OpportunitiesPerTurn
Reaction.DecisionBoundariesPerTurn
Reaction.RevalidationsPerBoundary
Reaction.CandidateCollectionCpuMs
Reaction.BoundaryCpuMs
Reaction.DecisionSeconds
Resolution.PlaybackSeconds
```

Per pacing: p50, p90 e campione dichiarato.

Non sommare tempo CPU resolver, playback e attesa decisione umana: sono budget differenti.

---

# 7. Test automatici richiesti

Adattare i nomi alle convenzioni reali del repository.

```text
Reaction.SameBoundaryAggregatesTriggers
Reaction.MultiTargetProducesSingleOpportunity
Reaction.SingleResponseDoesNotPrompt
Reaction.CandidateEnumerationOrderDoesNotChangeOutcome
Reaction.NoNestedDecisionBoundary
Reaction.ResponseCannotRetroactivelyChangeCommittedEvent
Reaction.TargetedRevalidationOnlyTouchesDirtyDomains

Resolution.BoundaryIsAtomic
Resolution.StepAndNormalPlaybackHaveSameBoundaryHashes

Determinism.SameInputsProduceSameBoundaryHashSequence
Determinism.ReactionDivergenceReportsFirstBoundary
```

Anti-vacuity: ogni gate importante deve essere dimostrato anche rosso tramite mutation/injected divergence quando sensato.

---

# 8. Test PIE / scenario

Aggiungere o estendere una sessione già presente nel repository, senza creare una seconda tassonomia di sessioni.

Scenario minimo:

```text
- almeno 2 unità in movimento simultaneo;
- almeno 2 trigger reaction nello stesso movement step;
- almeno una opportunity multi-target;
- una reaction con scelta interattiva;
- una reaction auto-resolved (`AllowedResponses <= 1`);
- state change che forza targeted revalidation;
- StepMicroStep attivo in modalità debug;
- confronto finale StateHash / boundary hashes.
```

Verificare visivamente:
- nessuno stato a metà;
- nessun popup duplicato;
- path/animazione possono interpolare senza cambiare outcome;
- facing visuale può interpolare, il facing logico resta quello del boundary.

---

# 9. Privacy

Questa roadmap NON sposta il multiplayer nella v0.1, ma vieta di costruire API che rendano impossibile rispettare la privacy in seguito.

Non mettere mai dati reaction/planning privati su Actor globali replicati.

Nel futuro networked:

```text
Canonical server state
       ↓
authorized team relay / sanitized DTO
       ↓
client
```

Mai:

```text
replicate everything
       ↓
hide it in UI
```

#759 resta owner del timing side-channel.

---

# 10. Definition of Done del tracker

```md
- [ ] #152 baseline Reaction System marcata P0;
- [ ] #166 P0 con pacing metrics;
- [ ] #1879 P0;
- [ ] #1880 boundary identity completata;
- [ ] #2189 boundary checksum completata;
- [ ] Canonical Event contract completato;
- [ ] indexed trigger registry completato;
- [ ] targeted revalidation completata;
- [ ] candidate-order independence verde;
- [ ] same-boundary aggregation verde;
- [ ] no nested boundary verde;
- [ ] boundary hashes deterministici;
- [ ] metriche CPU registrate;
- [ ] metriche UX/pacing registrate;
- [ ] PIE registrata;
- [ ] packaged/headless gate pertinente verde;
- [ ] #314 e #319 restano P3;
- [ ] nessuna dipendenza da skill/personaggi in revisione.
```

---

# 11. Regole per Claude durante l'editing GitHub

Prima di ogni modifica:

1. fetch dell'issue;
2. leggere commenti recenti;
3. controllare label/milestone;
4. cercare duplicate issue;
5. preservare riferimenti e decisioni vive;
6. non usare documenti `archive/` come autorità corrente;
7. repository/source truth vince su handoff;
8. se il codice o issue corrente contraddicono questo handoff, NON inventare la correzione: segnala il conflitto nella issue tracker principale;
9. non introdurre nomi di skill/eroi come requisito;
10. al termine produrre un riepilogo di issue create, issue aggiornate, label cambiate, dipendenze ed eventuali conflitti/blocchi.

---

# 12. Risultato atteso

Alla fine GitHub deve mostrare chiaramente una **critical path P0** che protegge:

- atomicità dei micro-step;
- determinismo delle reaction;
- candidate collection scalabile;
- revalidation mirata;
- localizzazione delle divergenze;
- interruption UX controllata;
- metriche reali;
- privacy-by-architecture.

Reaction Clash e Decision Time Bank devono rimanere estensioni e non bloccare la baseline.
