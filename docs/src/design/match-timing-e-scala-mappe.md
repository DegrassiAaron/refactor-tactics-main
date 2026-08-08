> ✅ **RECEPITO il 2026-08-08.** Owner normativo: [`../../gameplay/spec-durata-partita-e-scala-mappe.md`](../../gameplay/spec-durata-partita-e-scala-mappe.md)
> (classi di mappa, durata, round, Planning/Ready, Fast Reaction, budget del round) e la wiki
> [`../../wiki/game/obiettivi-e-fine-partita.md`](../../wiki/game/obiettivi-e-fine-partita.md).
> Decisione: [D-030](../../decisions/RT_PDR_00_Decision_Log.md). Le **due sole** parti non ancora nel codice —
> classe di mappa sul dato mappa e `UnitsPerTeam` nel formato — sono **E19** di
> [`../../roadmap/roadmap-v0.1.md`](../../roadmap/roadmap-v0.1.md); Standard 3v3 e Operations sono E24/E30 in
> [`../../roadmap/roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md).
>
> ⚠️ Attenzione a §4: `URTMatchFormatData` **esiste già** nel codice. I timer di parete restano fuori dal dato
> **per decisione motivata** (ADR-0005 §4c), non per dimenticanza: non riproporre di spostarli.

# RefactorTactics — Consolidamento Match Timing, Round Budget e Scala Mappe
## Prompt operativo per Claude Code

> Scopo: consolidare nella repository RefactorTactics le decisioni più recenti su durata partita, struttura del round, timer, Ready, Fast Reaction, scala delle mappe, obiettivi e metriche di playtest, aggiornando **documentazione, codice/configurazione, test, issue e roadmap** senza duplicare fonti di verità.

---

# 0. Modalità di lavoro obbligatoria

Stai lavorando direttamente nella repository **RefactorTactics**.

Prima di modificare qualsiasi file:

1. leggi `AGENTS.md`, `CLAUDE.md`, `README.md` e documenti equivalenti se presenti;
2. individua la documentazione canonica sotto `Docs/`, soprattutto:
   - gameplay loop / match structure;
   - vertical slice / demo;
   - roadmap e QA;
   - mappa / pathfinding / level design;
   - simulazione deterministica / snapshot / TurnLog;
   - UI/UX;
   - networking/privacy;
   - Overwatch / Fast Action / Fast Reaction;
   - Fog of War / percezione / rumore;
   - Ruleset / Data Assets / balance;
3. cerca nel codice le strutture esistenti che governano:
   - fase del match;
   - planning timer;
   - Ready / Unready;
   - countdown;
   - round limit;
   - objective / victory;
   - Fast Reaction;
   - playback della resolution;
   - Ruleset / Match configuration;
   - telemetry / test harness;
4. cerca issue, roadmap, TODO e milestone già esistenti;
5. **non creare sistemi paralleli** se esiste già un ownership corretto;
6. prima di editare, produci una breve audit table con:
   - file/area;
   - valore attuale;
   - conflitto;
   - decisione nuova;
   - azione proposta.

## Regola di prevalenza

Quando trovi conflitti, applica questo ordine:

1. decisioni esplicite di questo documento;
2. decisioni recenti già consolidate nella repository;
3. PDR/documentazione canonica;
4. vecchie proposte, demo legacy, placeholder e valori di tuning.

Non mantenere due baseline incompatibili senza marcarne chiaramente lo stato.

---

# 1. Decisione strategica principale

RefactorTactics **NON deve essere vincolato a mappe piccole** solo perché Atlas Reactor usava mappe relativamente compatte.

Atlas Reactor resta riferimento soprattutto per:

- simultaneità del planning;
- commitment;
- leggibilità della resolution;
- macro-fasi del turno;
- tensione predittiva.

Non è un vincolo per:

- dimensione fisica della mappa;
- numero di celle;
- distanza tra spawn;
- numero totale di round;
- durata esatta della partita.

## Principio da consolidare

> **RefactorTactics deve essere compatto nel tempo, non necessariamente piccolo nello spazio.**

La scala della mappa deve derivare dalle esigenze sistemiche di RefactorTactics.

---

# 2. Perché serve più spazio tattico

Le meccaniche ormai previste richiedono spazio per avere vero valore:

- Fog of War;
- visibilità limitata;
- stealth;
- rumore e percezione acustica;
- memoria dell'ultima posizione nota;
- Overwatch direzionale;
- Fast Reaction;
- flank;
- choke point;
- rotte alternative;
- quota;
- coperture;
- porte;
- ponti;
- tunnel;
- ascensori;
- acqua;
- fuoco;
- elettricità;
- ghiaccio;
- vapore/fumo;
- hazard;
- obiettivi dinamici;
- controllo di territorio.

Una mappa troppo piccola rischia di:

- rendere la visione quasi globale;
- rendere il rumore quasi globale;
- ridurre stealth e Fog of War a dettagli marginali;
- ridurre i flank;
- permettere a Overwatch di coprire troppe rotte;
- eliminare la scelta fra percorso rapido, sicuro, silenzioso o coperto;
- comprimere troppo il positional gameplay.

Questa motivazione deve comparire nella documentazione di level design / map scale e avere cross-reference verso Fog of War, Noise e Reaction/Overwatch.

---

# 3. Temporal Map Size

Non dimensionare le mappe principalmente in:

- metri;
- numero assoluto di esagoni;
- confronto visivo con Atlas Reactor.

La metrica primaria deve essere **temporale**:

> Quanti round / Move servono per raggiungere zone tatticamente rilevanti?

Definire nella documentazione il concetto di **Temporal Map Size**.

Una buona mappa deve permettere:

- decisioni tattiche già dal round 1;
- primo contatto o prima contestazione significativa entro circa 1–2 round nella modalità Standard;
- attraversamento completo sensibilmente più costoso;
- almeno 2–3 macro-rotte realmente differenti;
- trade-off tra velocità, rumore, copertura, visibilità, rischio e controllo obiettivo.

Evitare design:

```text
Spawn
-> Move
-> Move
-> Move
-> finalmente succede qualcosa
```

Preferire:

```text
Spawn
-> scelta immediata della rotta
-> pressione / intel / contest già nel primo round
-> contatto significativo entro 1–2 round
-> attraversamento totale molto più impegnativo
```

---

# 4. Classi di mappa

La classificazione deve essere **data-driven** e associabile al Match Format / Ruleset, non hard-coded nella logica.

## 4.1 Skirmish

Uso:

- tutorial;
- test;
- vertical slice;
- 2v2;
- match rapidi;
- sandbox funzionali.

Baseline da playtestare:

- attraversamento completo: circa **3–4 Move normali**;
- primo contatto significativo: circa **1 round**;
- round attesi: circa **10–14**;
- eventuale hard cap: **14–16**.

La mappa del vertical slice può essere Skirmish e **non deve definire la scala delle mappe competitive finali**.

## 4.2 Standard

Formato competitivo principale.

Uso:

- 3v3;
- principale modalità competitiva.

Baseline da playtestare:

- attraversamento completo: circa **5–7 Move normali**;
- primo contatto / prima contestazione: **1–2 round**;
- almeno **2–3 macro-rotte**;
- choke point con counter-route;
- vera possibilità di perdere/riacquisire contatto visivo;
- rumore non automaticamente globale.

La quantità assoluta di celle NON è ancora bloccata.

Ordine di grandezza esplorabile:

- oltre 100 celle;
- anche circa **150–200 celle percorribili** se temporalmente la mappa resta corretta.

**150–200 non è un requisito. È solo una dimensione plausibile da prototipare e misurare.**

## 4.3 Operations — futuro

Fuori scope vertical slice.

Direzione futura possibile:

- mappe più grandi;
- più objective;
- più esplorazione;
- Fog of War più importante;
- repositioning strategico;
- maggiore valore di rumore e logistica;
- partite potenzialmente **45–60+ minuti**.

L'architettura deve poterlo supportare, ma **non implementarlo ora**.

---

# 5. Durata della partita

## Standard 3v3

Target di game design:

- partita veloce: circa **20–25 min**;
- partita tipica: circa **25–30 min**;
- partita combattuta: circa **30–40 min**;
- hard ceiling / eccezione: circa **45 min**.

### Decisione importante

**45 minuti NON è il target.**

È il limite superiore che la modalità Standard dovrebbe raggiungere raramente.

Obiettivo di playtest iniziale:

```text
P50 Match Duration ~ 25–30 min
P90 Match Duration < ~40–45 min
```

Sono target di tuning/playtest, non SLA tecnici.

---

# 6. Numero di round

Vecchie assunzioni come:

- demo fissa a 8 turni;
- massimo universale 12 turni;

non devono più essere considerate regole del gioco finale.

## Standard 3v3 — baseline da playtestare

- round attesi: circa **16–20**;
- hard cap indicativo: circa **20–22**.

## Skirmish / Vertical Slice 2v2 — baseline da playtestare

- round attesi: circa **10–14**;
- hard cap indicativo: circa **14–16**.

Il numero deve essere governato dal **Match Format / Ruleset**.

Non trasformare questi numeri in costanti C++ globali.

---

# 7. Planning

Il planning deve usare:

```text
MAX TIMER
+
READY ANTICIPATO
```

Non assumere che ogni round consumi tutto il timer.

## Standard 3v3

Baseline da testare:

```text
PlanningMax = 40–45 s
```

Motivazione:

il giocatore può dover valutare:

- più unità;
- path;
- ghost delle azioni;
- target;
- AoE;
- Fast Reaction preparate;
- Overwatch;
- azioni ritardate;
- terreno;
- Fog of War;
- intenti alleati;
- collisioni;
- friendly fire;
- obiettivi;
- risorse/cooldown.

Un timer fisso da 30 s può essere troppo stretto nelle situazioni complesse.

Il design deve però premiare rapidità e coordinazione con Ready anticipato.

---

# 8. Ready / Unready / countdown

Decisione consolidata:

- Ready anticipato;
- Unready consentito finché il commit non è avvenuto;
- countdown breve quando tutti sono Ready;
- countdown annullabile.

Baseline:

```text
All Ready
-> 3 s countdown
-> Commit
```

Se durante il countdown un giocatore usa Unready:

```text
Countdown cancelled
-> ritorno al Planning
```

Questo meccanismo deve essere coerente tra:

- UI;
- game state;
- networking;
- test automatici;
- bot/test harness.

---

# 9. Fast Reaction

Vecchie specifiche con:

```text
Reaction Charge = 5 s
```

sono obsolete come baseline.

Nuova baseline consolidata:

```text
FastReactionDuration = 3.0 s
```

Fast Reaction:

- NON è un secondo planning;
- deve essere immediata;
- poche opzioni;
- input rapido;
- nessun menu profondo;
- decision boundary autorevole;
- timeout definito dalla policy.

Per Overwatch standard:

```text
FIRE
HOLD
```

Timeout:

```text
HOLD
```

Una reaction specifica futura potrà avere durata differente se data-driven, ma **3 s è la baseline di sistema**.

---

# 10. Resolution

La resolution deve essere:

- leggibile;
- spettacolare;
- rapida;
- separata dal tempo logico del resolver.

Baseline visuale da playtestare:

## Skirmish / 2v2

```text
~8–15 s di playback tipico
```

## Standard / 3v3

```text
~12–20 s di playback tipico
```

Le Fast Reaction possono aumentare il wall-clock reale.

Non rallentare artificialmente la resolution per far durare di più la partita.

Preferire:

> più cicli decisionali significativi

rispetto a:

> resolution molto lunghe.

---

# 11. Distinzione dei tempi

Consolidare questa terminologia:

## Simulation Time

Tempo logico del resolver.

Può essere <100 ms lato server per il match/round MVP secondo i budget tecnici.

## Presentation Time

Durata con cui TurnLog/eventi vengono mostrati.

Può durare secondi.

## Decision Time

Tempo reale concesso al giocatore:

- Planning;
- Fast Reaction;
- altre future Decision Window.

## Wall-clock Match Time

Durata reale della partita.

Queste quattro grandezze NON devono essere confuse in documentazione, telemetry o test.

---

# 12. Budget temporale di un round

Struttura concettuale:

```text
Planning
-> eventuale Ready Countdown
-> Commit
-> Snapshot
-> Resolution
-> eventuali Fast Action / Fast Reaction
-> Cleanup / Objective / Score
-> next round
```

Baseline Standard 3v3:

- Planning max: **40–45 s**;
- Ready countdown: **3 s**;
- Resolution playback: **12–20 s** tipica;
- Fast Reaction: **3 s per opportunity** quando generata;
- cleanup/presentation: circa **3–5 s**, preferibilmente integrato nel playback.

**Non sommare tutti i massimi per stimare la durata media.**

Usare telemetry reale.

---

# 13. Condizioni di fine partita

Il match non deve essere governato solo da:

```text
RoundLimit reached
```

Il Ruleset deve poter supportare:

1. Victory Condition;
2. Score Threshold;
3. Round Limit;
4. eventuale Overtime.

Flusso concettuale:

```text
VictoryCondition reached
    -> Win

else if ScoreThreshold reached
    -> Win according to Ruleset

else if RoundLimit reached
    -> Compare score

if tied
    -> OvertimePolicy
```

Non implementare overtime complesso nel vertical slice se non necessario.

---

# 14. Objective come anti-stallo

Mappe più ampie non devono diventare camping simulator.

Gli Objective devono aiutare a comprimere progressivamente il conflitto.

Direzione di design:

## Early game

- scelta delle rotte;
- raccolta intel;
- controllo territorio;
- primo positioning.

## Mid game

- contest objective;
- controllo choke/route;
- pressione sulle risorse.

## Late game

- objective più prezioso;
- escalation;
- incentivo crescente al contatto.

Esempio puramente concettuale:

```text
Round 1–5:
Objective A

Round 6–10:
Objective B active

Late game:
high-value objective / escalation
```

NON rendere questi round una regola.

Principio da consolidare:

> Gli obiettivi devono comprimere il conflitto nel tempo senza obbligare il level design a usare mappe artificialmente piccole.

---

# 15. Relazione con Fog of War

La mappa Standard deve permettere che:

- non tutti gli avversari siano sempre visibili;
- si possa perdere contatto;
- l'ultima posizione nota abbia valore;
- il flank sia reale;
- il giocatore possa muoversi per evitare detection;
- la scelta tra route sicura e route veloce abbia significato.

Cross-reference al sistema Team Knowledge / Fog of War.

---

# 16. Relazione con rumore e percezione acustica

Il sistema Noise deve beneficiare della scala della mappa.

La mappa deve poter creare:

- zone acusticamente separate;
- superfici che cambiano Noise;
- tunnel con eco/propagazione diversa;
- zone con Ambient Noise;
- possibilità di Sprint rumoroso;
- decoy;
- Acoustic Mask;
- informazioni di direzione/area senza posizione esatta.

Il rumore non deve essere automaticamente percepito da tutto il team avversario.

Cross-reference alla specifica Noise/Perception.

---

# 17. Relazione con Overwatch

Overwatch direzionale funziona meglio con:

- choke point;
- porte;
- ponti;
- corridoi;
- tunnel;
- alternative routes;
- flank.

Level-design requirement:

> Evitare posizioni da cui una sola Overwatch possa controllare sistematicamente tutte le principali rotte senza counterplay.

Questo deve diventare almeno una regola di review/playtest della mappa.

---

# 18. Movimento e mappe più grandi

NON risolvere il problema della scala aumentando semplicemente il Move base.

La distanza deve creare decisioni.

Esempi:

## Move

- standard;
- costo/rischio normale.

## Sprint

- maggiore distanza;
- più rumore;
- possibile maggiore rischio ambientale;
- maggiore prevedibilità.

## Dash

- spostamento speciale nella relativa fase.

## Sneak — futuro

- meno rumore;
- minore velocità / maggior costo.

## Shortcut / transition

- porte;
- ponti;
- ascensori;
- tunnel;
- abilità;
- archi dinamici.

Obiettivo:

> mappa più ampia senza trasformare il gioco in spostamenti vuoti.

---

# 19. Ordine macro delle fasi da preservare

Non modificare la decisione già consolidata:

```text
Decision / Planning
-> Prep
-> Dash
-> Blast
-> Move
```

Il Move normale resta l'ultima fase di spostamento volontario.

Non introdurre sequenze arbitrarie:

```text
Move -> Attack
```

Fast Action e Fast Reaction sono Decision Window contestuali della resolution e NON una nuova fase completa di Planning.

---

# 20. Configurazione data-driven da consolidare nel codice

Prima verifica le strutture reali della repository.

Se esiste `URTRuleset`, `MatchFormat`, `RulesDefinition` o equivalente, **estendilo invece di creare un sistema parallelo**.

I parametri che devono avere ownership data-driven sono almeno concettualmente:

```text
MatchFormatId
TeamSize

PlanningMaxSeconds
ReadyCountdownSeconds
FastReactionDefaultSeconds

ExpectedRoundCount
RoundLimit

VictoryPolicy
ScoreThreshold
OvertimePolicy

ExpectedMatchDurationMinutes
SoftMaxMatchDurationMinutes

MapScaleClass
TargetTraversalMoves
TargetFirstContactRoundMin
TargetFirstContactRoundMax
```

Non usare per forza questi nomi: aderire a naming e tipi reali del progetto.

## Regola importante

Separare:

### Competitive rules

- RoundLimit;
- PlanningMax;
- Ready countdown;
- Fast Reaction duration;
- victory/score policy.

### Design targets / telemetry targets

- ExpectedMatchDuration;
- P50/P90;
- first contact target;
- traversal target.

I secondi non devono necessariamente entrare nello snapshot competitivo se non servono alla simulazione.

---

# 21. Possibili modifiche codice

Dopo l'audit, valuta modifiche minime e scalabili nelle aree corrette.

NON implementare tutto per forza in questo task se la milestone non è pronta.

## 21.1 Ruleset / Match Format

Possibili responsabilità:

- timer;
- round limit;
- team size;
- victory policy;
- reaction default;
- map scale class.

## 21.2 Turn Manager

Deve leggere i valori dal Ruleset, non da magic numbers.

Verificare:

- Planning start/end;
- Ready;
- Unready;
- countdown;
- commit;
- round increment;
- round-limit termination.

## 21.3 Decision Window / Reaction subsystem

Verificare che la durata default di 3 s sia data-driven.

Non hard-code 3 s in più punti.

## 21.4 UI

`WBP_TurnTimer` o equivalente deve mostrare valori runtime.

La UI non deve assumere `30 s` o `5 s`.

## 21.5 Objective System

Preparare ownership per:

- score threshold;
- victory condition;
- round-limit resolution;
- futura escalation.

Non aggiungere un sistema enorme se F4 non è ancora in scope.

## 21.6 Map Definition

Valuta metadata non competitivi / design-time per:

- MapScaleClass;
- traversal target;
- first contact target;
- route count target.

Servono per validator, testing e telemetry, non per dettare rigidamente il level design.

---

# 22. Validator da aggiungere/proporre

Integrare nel sistema Data Validation quando la relativa struttura esiste.

Esempi:

- `PlanningMaxSeconds <= 0` -> Error;
- `FastReactionDefaultSeconds <= 0` -> Error;
- `RoundLimit <= 0` -> Error;
- `ExpectedRoundCount > RoundLimit` -> Warning/Error secondo semantica;
- `ExpectedMatchDuration > SoftMaxMatchDuration` -> Warning;
- Map Standard con meno di 2 route principali dichiarate -> Warning di design, non necessariamente errore build;
- invalid MapScaleClass -> Error;
- Ruleset senza VictoryPolicy -> Error se richiesto.

Non introdurre validator basati su valori ancora puramente sperimentali come hard error.

---

# 23. Telemetry da integrare

La roadmap deve includere raccolta strutturata di almeno:

```text
MatchDurationSeconds
RoundsPlayed

RoundDurationSeconds
PlanningDurationSeconds
ReadyAtSeconds

ResolutionPlaybackSeconds

ReactionWindowCount
ReactionDecisionSeconds

FirstEnemyContactRound
FirstObjectiveContestRound

CellsTraversedPerUnit
MapTraversalRounds

TimeWithNoEnemyContact
TimeWithNoMeaningfulDecision

VictoryReason
RoundLimitReached
OvertimeEntered
```

Per analisi:

```text
P50 / P90 MatchDuration
P50 / P90 PlanningDuration
P50 FirstContactRound
P90 FirstContactRound
```

Target Standard iniziali:

```text
P50 MatchDuration ~ 25–30 min
P90 MatchDuration < ~40–45 min
First meaningful contact ~ round 1–2
```

---

# 24. Automated Scenario Test Harness

Se nella repository esiste già il test harness automatico, integra metriche e scenari senza creare un secondo framework.

Aggiungere/proporre scenari automatici tipo:

## MatchTiming.ReadyEarly

- PlanningMax configurato;
- tutti Ready prima del timeout;
- countdown 3 s;
- commit anticipato;
- assert che non venga atteso il PlanningMax completo.

## MatchTiming.UnreadyCancelsCountdown

- tutti Ready;
- countdown avviato;
- un player Unready;
- countdown cancellato;
- nessun commit prematuro.

## MatchTiming.ReactionDefault

- trigger Fast Reaction;
- assert durata configurata = 3 s in Visual;
- in Fast/Headless risposta immediata senza attesa wall-clock;
- esito logico identico.

## MatchTiming.RoundLimit

- scenario fino al limite;
- assert match termination reason.

## MapTemporalSize.FirstContact

- scripted movement;
- misura round del primo contatto;
- produce metrica, non necessariamente FAIL mentre i target sono ancora in tuning.

## MapTemporalSize.Traversal

- misura Move/round necessari per attraversamento;
- report machine-readable.

---

# 25. Test automatici richiesti/proposti

## Core Automation

- Ruleset timing serialization;
- default/override values;
- RoundLimit;
- Ready state transitions;
- countdown state transitions.

## Feature Tests

- Planning -> Ready -> Countdown -> Commit -> Snapshot;
- Unready durante countdown;
- Fast Reaction default;
- victory via RoundLimit;
- victory via Objective/Score quando disponibile.

## Functional Tests

- Skirmish map temporal traversal;
- first-contact scenario;
- objective contest timing.

## Network Tests

Quando F1 è disponibile:

- Ready/Unready reliable;
- countdown coerente;
- nessun leak di intent;
- Fast Reaction opportunity solo al client/team autorizzato;
- timeout autorevole.

## Packaged Tests

- timer/config Ruleset coerenti;
- zero magic-number divergence;
- telemetry prodotta;
- privacy invariata.

---

# 26. Documentazione da aggiornare

Dopo l'audit, aggiorna i documenti canonici esistenti.

Probabili aree:

## Vertical Slice / Demo

Correggere vecchie affermazioni come:

- "8 turni" come modello generale;
- "30 s" come planning definitivo;
- "5 s" come reaction baseline.

Se un valore appartiene a un vecchio scripted demo scenario, marcarlo come **legacy scenario parameter**, non regola universale.

## Roadmap / QA

Aggiungere:

- match timing validation;
- map temporal-size validation;
- telemetry;
- first-contact metric;
- duration P50/P90;
- reaction duration consistency;
- Ready/Unready tests.

## UI/UX

Aggiornare:

- planning timer runtime;
- Ready countdown;
- Fast Reaction 3 s;
- no hard-coded demo values.

## Map / Pathfinding / Level Design

Aggiungere:

- Temporal Map Size;
- Skirmish / Standard / Operations;
- traversal target;
- first-contact target;
- route diversity;
- relazione con FOW/Noise/Overwatch.

## Simulation / TurnLog

Aggiungere distinzione:

- Simulation Time;
- Presentation Time;
- Decision Time;
- Wall-clock Match Time.

## Data / Validation

Documentare nuovi campi di Ruleset/MatchFormat solo se realmente introdotti.

---

# 27. Issue da creare o aggiornare

Prima cerca issue equivalenti ed evita duplicati.

Usa label/milestone esistenti.

Se mancano, proponi le issue ma non inventare taxonomy senza verificarla.

## ISSUE A — Consolidate Match Format timing configuration

**Goal**

Portare timer, round limit e policy principali sotto Ruleset/MatchFormat data-driven.

**Scope**

- PlanningMax;
- ReadyCountdown;
- FastReactionDefault;
- RoundLimit;
- victory policy ownership;
- rimozione magic numbers.

**Acceptance**

- nessun 30/5/12 hard-coded come baseline universale;
- valori leggibili da config/data;
- automation test.

**Roadmap**

F0/F1 se il Turn Manager esiste già, altrimenti prima milestone compatibile.

---

## ISSUE B — Ready / Unready cancellable countdown

**Goal**

Implementare state machine completa:

```text
Planning
-> AllReady
-> Countdown
-> Commit
```

con:

```text
Unready -> Planning
```

**Acceptance**

- 3 s data-driven;
- cancellabile;
- TurnLog/debug event;
- automation test;
- network-ready design.

**Roadmap**

F0 locale, hardening F1 network.

---

## ISSUE C — Standardize Fast Reaction to 3 seconds

**Goal**

Consolidare baseline Fast Reaction = 3 s.

**Acceptance**

- una sola fonte dati;
- UI runtime;
- Visual mode usa countdown reale;
- Fast/Headless non attende;
- timeout policy separata dalla durata;
- Overwatch default HOLD.

**Roadmap**

Milestone Reaction / Abilities, ma documentazione subito.

---

## ISSUE D — Temporal Map Size metrics

**Goal**

Misurare la mappa in round/Move oltre che in celle.

**Deliverable**

Tool/test/debug report con:

- traversal Move count;
- first contact round;
- objective contest round;
- route count metadata.

**Acceptance**

- output machine-readable;
- nessun fail rigido su target sperimentali senza flag.

**Roadmap**

F3/F4.

---

## ISSUE E — Match duration telemetry

**Goal**

Misurare durata reale del match e dei suoi componenti.

**Metrics**

- match;
- round;
- planning;
- resolution;
- reactions;
- Ready;
- contact;
- objective.

**Acceptance**

- report locale/playtest;
- P50/P90 calcolabili;
- nessun dato privato leakato.

**Roadmap**

inizio F4, hardening F5.

---

## ISSUE F — Map scale classification

**Goal**

Introdurre metadata / Data Asset per:

- Skirmish;
- Standard;
- futura Operations.

**Acceptance**

- no behavior hard-coded;
- validator base;
- docs;
- almeno una Skirmish map classificata.

**Roadmap**

F3/F4.

---

## ISSUE G — Objective anti-stall / escalation design spike

**Goal**

Definire come gli objective comprimono il conflitto su mappe Standard.

**Scope**

- early/mid/late pressure;
- score threshold;
- round-limit interaction;
- eventuale overtime.

**Non-goal**

Implementare subito Operations o sistemi competitivi complessi.

**Roadmap**

F4 design/implementation.

---

## ISSUE H — Standard-map graybox prototype

**Goal**

Creare in futuro un graybox separato dalla Skirmish demo per validare:

- 5–7 Move traversal;
- 1–2 round first contact;
- 2–3 macro-route;
- Fog of War;
- Noise;
- Overwatch counter-routes.

**Roadmap**

post vertical-slice foundations / F3-F4.

---

# 28. Integrazione nella roadmap

Mantieni la roadmap esistente e inserisci il lavoro nelle milestone corrette, senza riscriverla da zero.

Baseline suggerita:

| Milestone | Integrazione |
|---|---|
| F0 Fondazioni | Ruleset timing ownership minimo; Ready/Unready locale; TurnLog timing events; niente sistemi grandi |
| F1 Rete privata | Ready/Unready reliable; countdown autorevole; timer sync; privacy invariata |
| F2 Abilities | Fast Reaction 3 s data-driven; Decision Window; timeout policy; reaction tests |
| F3 Mappa multilivello | MapScale metadata; temporal-size debug/test; route/traversal metrics |
| F4 Vertical Slice | Skirmish tuning; match-duration telemetry; first-contact/objective metrics; playtest 20–30 min circa |
| F5 Dedicated | telemetry hardening; P50/P90; soak; network timer/reaction reliability |
| F6 Beta | Standard 3v3 tuning; target 25–30 min; map-scale validation; competitive limits |
| Future | Operations; 45–60+ min; objective/match formats più ampi |

Non spostare arbitrariamente milestone già approvate: aggiorna la tabella reale della repository.

---

# 29. Stato delle decisioni

Nel documento canonico usa esplicitamente tre categorie.

## CONSOLIDATED / LOCKED

- RefactorTactics non deve copiare la scala mappa di Atlas Reactor.
- Principio: **compact in time, not necessarily in space**.
- Fast Reaction baseline = **3 s**.
- Ready anticipato.
- Ready countdown annullabile.
- Vertical slice Skirmish non determina la scala Standard.
- Obiettivi devono prevenire stallo sulle mappe più ampie.
- Map scale e timing devono essere data-driven dove appropriato.
- Il Move normale resta nell'ultima macro-fase volontaria prevista.

## BASELINE DA PLAYTESTARE

- Standard PlanningMax = **40–45 s**.
- Standard resolution playback = **12–20 s** tipica.
- Standard round attesi = **16–20**.
- Standard hard cap = **20–22**.
- Standard traversal = **5–7 Move**.
- Standard first contact = **1–2 round**.
- Skirmish round attesi = **10–14**.
- Skirmish hard cap = **14–16**.
- Skirmish traversal = **3–4 Move**.
- Standard P50 match = **25–30 min**.
- Standard P90 < **40–45 min**.
- circa 150–200 celle percorribili è solo un possibile ordine di grandezza.

## FUTURE / NON SCOPE

- Operations.
- 45–60+ min come formato dedicato.
- overtime sofisticato.
- escalation objective definitiva.
- large-map production implementation nel vertical slice.

---

# 30. Cerca esplicitamente questi valori/conflitti

Usa grep/ripgrep e cerca almeno:

```text
8 turni
8 turns
8 rounds

12 turni
12 turns
12 rounds

30s
30 s
30 seconds

5s
5 s
5 seconds

Reaction Charge
Interrupt
Fast Reaction

20-30
20–30
25-30
25–30

RoundLimit
PlanningTime
PlanningDuration
Ready
Unready

MatchDuration
TurnDuration
ResolutionDuration

Atlas Reactor
map size
small map
mappa piccola
```

Cerca anche magic numbers `3`, `5`, `8`, `12`, `30`, `45` **solo nei file già identificati come timing-related**, per evitare falsi positivi inutili.

---

# 31. Non fare questi errori

- Non cambiare il resolver per far durare di più la partita.
- Non legare Presentation Time a Simulation Time.
- Non aggiungere Wait/sleep al server-authoritative resolver.
- Non aumentare Move base solo perché la mappa Standard è più grande.
- Non mettere design-target telemetry nello snapshot se non servono al risultato competitivo.
- Non duplicare `URTRuleset` con un nuovo config system.
- Non hard-code 3 s in UI, server e ability separatamente.
- Non trasformare valori sperimentali in validator Error.
- Non implementare Operations ora.
- Non creare nuove issue se ne esiste già una equivalente: aggiorna l'esistente.
- Non modificare la privacy degli intenti.
- Non spostare il Move dalla sua macro-fase consolidata.

---

# 32. Acceptance criteria del consolidamento

Il lavoro è Done quando:

1. la documentazione canonica distingue chiaramente Skirmish, Standard e Future Operations;
2. non rimane `5 s` come Fast Reaction baseline corrente;
3. eventuali `8 turni` sono marcati come vecchio/demo scenario oppure aggiornati;
4. `12 turni` non è più presentato come limite universale;
5. Standard 3v3 ha target documentale **25–30 min medio**;
6. **40–45 min** è documentato come ceiling raro, non target;
7. Planning 40–45 s è marcato come baseline da playtestare;
8. Ready anticipato e Unready cancellano il countdown;
9. Temporal Map Size è documentato;
10. Standard targetta 5–7 Move traversal e first contact 1–2 round come baseline;
11. Fog of War, Noise e Overwatch sono cross-referenziati;
12. Ruleset/MatchFormat ha ownership chiara dei parametri competitivi;
13. nessun magic number incompatibile rimane nei sistemi toccati;
14. esistono test o issue esplicite per Ready, Reaction, RoundLimit e timing;
15. telemetry di match timing è integrata nella roadmap;
16. issue nuove sono deduplicate rispetto al tracker esistente;
17. roadmap è aggiornata senza espandere lo scope del vertical slice;
18. documentazione, codice e issue usano terminologia coerente.

---

# 33. Output finale richiesto a Claude

Alla fine del lavoro rispondi con:

## Repository audit

Tabella:

```text
Area | File | Old value/assumption | New decision | Action
```

## Files changed

Elenco dei file modificati con motivo.

## Code changes

Per ogni file:

- cosa è cambiato;
- ownership del valore;
- eventuali compatibility implications.

## Documentation changes

Elenco delle decisioni consolidate e dei cross-reference aggiunti.

## Issues created/updated

Per ciascuna:

- numero;
- titolo;
- milestone;
- labels;
- acceptance criteria.

## Roadmap integration

Mostra come il lavoro è stato inserito in F0–F6/Future.

## Tests

Indica:

- test aggiunti;
- test aggiornati;
- test non ancora implementabili e relativa issue.

## Telemetry

Elenco delle metriche introdotte/proposte.

## Superseded assumptions

Elenco esplicito di ciò che non è più baseline:

- 5 s reaction;
- 8 turni come modello generale;
- 12 turni come hard cap universale;
- 30 s Planning come valore finale obbligatorio;
- mappa piccola per imitare Atlas Reactor.

## Remaining open decisions

Solo decisioni realmente non risolte, ad esempio:

- numero definitivo round Standard;
- dimensione reale in celle;
- valori definitivi objective/score;
- overtime;
- soglie finali P50/P90 dopo playtest.

## Git diff summary

Sintesi focalizzata.

## Suggested commits

Preferire commit separati quando il repository lo giustifica, per esempio:

```text
docs(match): consolidate timing and map-scale targets
feat(rules): move match timing into ruleset configuration
test(match): cover ready countdown and round limits
chore(roadmap): add timing telemetry and map-scale validation
```

Non effettuare commit automaticamente salvo richiesta esplicita.

---

# 34. Principio finale da preservare

La metrica più importante non è:

> "Quanto è piccola la mappa?"

ma:

> **"Quanto presto iniziano decisioni tattiche significative, e quanto spazio rimane per informazione, flank, rischio e controllo?"**

RefactorTactics deve puntare a:

```text
decisioni interessanti dal round 1
+
contatto/contest significativo entro 1–2 round
+
mappa abbastanza ampia da rendere Fog of War, Noise, Stealth e Overwatch realmente strategici
+
partita Standard 3v3 tipicamente 25–30 minuti
+
raramente oltre 40–45 minuti
```

La mappa deve essere **più libera di Atlas Reactor nello spazio**, ma la partita deve restare **disciplinata nel tempo**.
