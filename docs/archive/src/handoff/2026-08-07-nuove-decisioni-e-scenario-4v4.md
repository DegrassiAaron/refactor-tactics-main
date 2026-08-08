> ✅ **RECEPITO in parte** il 2026-08-08. Le decisioni di §3 (roster, Fast Reaction 3,0 s, ordine delle fasi,
> griglia, no-GAS) sono **già canone** — vedi [`../../../product/piano-canonico-mvp.md`](../../../product/piano-canonico-mvp.md)
> e il [Decision Log](../../../decisions/RT_PDR_00_Decision_Log.md): questo documento le registra, non le
> introduce. Lo **scenario 4v4 a 8 turni** resta materiale di stress: **E17** in v0.1 (validazione, non
> produzione) e **E32** in [`../../../roadmap/roadmap-post-v0.1.md`](../../../roadmap/roadmap-post-v0.1.md) se mai
> diventasse un formato. Il formato competitivo finale **non è deciso**: 3v3 è baseline, 4v4 stress test.

# REFACTORTACTICS — CONSOLIDAMENTO NUOVE DECISIONI, SCENARIO 4V4 E ROADMAP
## Handoff operativo per Claude Code

**Data consolidamento:** 2026-08-07  
**Progetto:** RefactorTactics  
**Repository di riferimento:** verificare quella realmente aperta prima di operare  
**Baseline documentale:** Unreal Engine 5.8 / canone repo 5.8.1 dove presente  
**Scopo:** consolidare le decisioni più recenti del progetto, correggere il materiale storico, aggiornare documentazione, codice, test, issue e roadmap senza creare implementazioni parallele.

---

# 0. Obiettivo del task

Questo documento NON chiede di implementare tutto in una singola PR.

Claude Code deve:

1. analizzare il repository reale;
2. identificare il canone corrente;
3. confrontarlo con PDR, cataloghi, matrici e handoff recenti;
4. aggiornare la documentazione affinché non esistano più contraddizioni operative;
5. proporre o implementare solo il codice coerente con la milestone corrente;
6. creare/aggiornare issue senza duplicati;
7. integrare le nuove decisioni nella roadmap;
8. mantenere separati:
   - canone già implementato;
   - target della showcase;
   - decisioni nuove approvate ma non ancora implementate;
   - idee future / experimental.

La priorità è evitare che vecchi PDF o matrici reintroducano sistemi superati.

---

# 1. Prima di modificare qualsiasi file

Eseguire prima:

```text
git status
git branch --show-current
git rev-parse HEAD
```

Poi leggere, se presenti:

```text
AGENTS.md
CLAUDE.md
README.md

docs/design/piano-canonico-mvp.md
docs/design/roadmap-checkpoint.md
docs/design/roadmap-v0.1.md
docs/design/adr-0003-modello-azioni-v01.md

docs/design/balance/RT_ActionCatalog_v0.1.md
docs/design/balance/RT_HeroCatalog_v0.1.md
docs/design/balance/RT_TerrainCatalog_v0.1.md

docs/design/showcase-v0.1.md
docs/design/spec-terreni-e8.md
```

Cercare inoltre:

```text
Source/RefactorTactics/
Config/
Content/
Tests/
Docs/
.github/
```

Regola:

> Il repository corrente e il piano canonico corrente prevalgono sui vecchi PDF.

Se un documento recente e il codice divergono:

- non correggere silenziosamente;
- descrivere il conflitto;
- indicare quale dei due sembra obsoleto;
- aggiornare decision log/changelog;
- creare issue se la scelta non è già stata approvata.

---

# 2. Ordine di prevalenza delle fonti

Usare questo ordine:

```text
1. Decisioni esplicite più recenti del progetto
2. Codice corrente + test verdi
3. docs/design/piano-canonico-mvp.md
4. ADR e cataloghi correnti
5. Handoff operativi del 7 agosto 2026
6. Roadmap corrente
7. PDR consolidati
8. Matrici di bilanciamento storiche
9. Vecchi PRD/demo/ricerche
```

Non rinominare arbitrariamente personaggi o sistemi se il repository li ha già consolidati.

---

# 3. Conflitti noti da risolvere esplicitamente

## 3.1 Roster

Materiale storico contiene almeno:

```text
Aegis / Nyx / Drift / Vex
Mara / Ivo / Nyx / Sol
Steel / Aurora / Murdock / Kwang
```

Il canone operativo recente della showcase/codice è:

```text
Flux
Riva
Bastion
Vektor
```

Formazione v0.1 corrente:

```text
Team 0:
Flux
Riva

Team 1:
Bastion
Vektor
```

Decisione:

> Non reintrodurre roster storici nei documenti operativi.

I vecchi nomi possono restare solo in:

- archivio;
- research;
- migration note;
- riferimento storico.

Le matrici di bilanciamento che usano ancora Steel/Aurora/Murdock/Kwang devono essere marcate come **da migrare / historical**, non trattate come fonte di verità runtime.

---

## 3.2 Fast Reaction

Vecchi documenti contengono:

```text
5 secondi
7-8 secondi
Reaction Charge / Interrupt Window
```

Baseline aggiornata:

```text
FastDecisionDuration = 3.0 seconds
```

Per Overwatch:

```text
FIRE
HOLD
Timeout -> HOLD
```

Decisione:

> 3 secondi prevale come baseline v0.1 per Fast Action / Fast Reaction, salvo ability-specific exception esplicitamente approvata.

---

## 3.3 Ordine fasi

Canone corrente:

```text
Planning / Decision
    ↓
Prep
    ↓
Dash
    ↓
Blast
    ↓
Move
    ↓
Cleanup
```

Regola vincolante:

> Il normale Move è sempre l'ultima fase volontaria standard del turno.

NON introdurre:

```text
Move -> Attack
Attack -> Move -> Attack
timeline libera
```

Dash, Blink, Charge, Leap, displacement e reaction movement sono movimenti speciali e NON equivalgono al normale Move.

---

## 3.4 Griglia

Materiale vecchio può contenere:

- griglia quadrata;
- 4-way adjacency;
- mappe puramente 2D.

Canone:

```text
FRTCellId
X
Y
Layer
```

X/Y sono coordinate assiali di una **griglia esagonale**.

Baseline:

```text
6 vicini sullo stesso Layer
+
archi speciali tra Layer
```

La mappa resta un grafo tattico 3D.

Aggiornare qualsiasi documento che descriva ancora il graybox autorevole come griglia quadrata 4-way.

---

## 3.5 GAS

North-star:

- GAS per costi;
- cooldown;
- attributi;
- Gameplay Effects/Cues.

Canone operativo corrente:

> NON introdurre GAS come prerequisito della showcase o del resolver.

Il resolver C++ corrente resta l'autorità.

Aggiungere moduli GAS solo quando una issue/milestone reale ne richiede le API.

---

## 3.6 Networking

North-star prodotto:

- server authoritative;
- planning privato;
- team-only preview;
- zero leak.

v0.1 corrente può essere ancora offline/local-first.

Decisione:

> Non introdurre networking prematuramente nella showcase locale, ma non progettare sistemi che rendano impossibile la privacy futura.

I dati devono già essere classificabili come:

```text
Server-only
Team-only
Owner-only
Public
Derived local
```

---

# 4. Formato partita: aggiornamento da consolidare

Esiste materiale storico in conflitto:

```text
3v3 principale
4v4 finale
2v2 vertical slice
20-30 minuti
```

Decisione di prodotto più recente:

> Una partita standard non deve superare circa 30–45 minuti.

Interpretazione operativa:

```text
Target desiderato:
~30 minuti quando la partita è fluida

Hard upper target:
~45 minuti

Vertical slice / showcase:
può essere più corta
```

Non mantenere `20–30 minuti` come invariante assoluto se il documento pretende di descrivere il prodotto completo.

Può restare come:

```text
short-match target
vertical slice target
internal showcase target
```

ma va distinto dal target massimo del prodotto.

---

# 5. Modalità e controllo squadra

Mantenere separati i formati.

## Vertical slice v0.1

```text
2v2
4 personaggi totali
Flux + Riva vs Bastion + Vektor
```

Scopo:

- integrazione;
- golden scenario;
- leggibilità;
- ambiente;
- reazioni;
- objective;
- determinismo.

## Formato principale / competitivo

La documentazione storica contiene sia 3v3 sia 4v4.

La discussione recente introduce esplicitamente un **4v4 con tutti i quattro personaggi della vertical slice per squadra**.

Non sovrascrivere in modo cieco il formato competitivo prima di verificare il decision log corrente.

Se il repository non contiene ancora una decisione finale unica:

creare una issue di design:

```text
[DESIGN] Lock primary match format and control model
```

con almeno:

- 3v3;
- 4v4;
- durata target;
- numero giocatori umani;
- chi controlla quante unità;
- map size;
- reaction frequency;
- UX load;
- server resolver cost.

Assunzione di playtest già discussa:

```text
2v2 / 3v3:
un giocatore può controllare una squadra

4v4:
un giocatore per personaggio è il modello da testare
```

Non trattarla come regola di rete definitiva finché non è registrata nel canone.

---

# 6. Nuovo scenario di validazione: 4v4 mirror

## 6.1 Obiettivo

Aggiungere alla roadmap uno **scenario 4v4 di stress e design validation**.

NON sostituisce subito la showcase 2v2.

Serve a verificare:

- scalabilità del resolver;
- leggibilità con 8 unità;
- action economy;
- durata planning;
- numero Reaction Opportunity;
- quantità di Action Ghost;
- overlay FoW/perception;
- objective play;
- map size;
- split fight;
- prestazioni;
- TurnLog;
- privacy futura;
- bilanciamento delle combo quando tutti i ruoli sono presenti.

---

## 6.2 Composizione

Mirror match intenzionale:

```text
TEAM BLUE
Flux
Riva
Bastion
Vektor

TEAM RED
Flux
Riva
Bastion
Vektor
```

Perché mirror:

- isola il valore del piano dal valore della composizione;
- rende più semplice confrontare comportamento e balance;
- stressa gli stessi sistemi in entrambe le direzioni;
- evita di introdurre altri personaggi prima di aver validato i quattro core.

---

# 7. Arena 4v4 di riferimento

Nome provvisorio:

```text
RT_Stress_4v4_CoreRoster
```

Non hard-codificare il nome nel resolver.

La mappa deve essere sensibilmente più ampia della showcase 2v2.

Obiettivo concettuale:

```text
                  HIGH / SECONDARY ROUTE
              [cover] ---- [Relay B]
                    \      /
                     \    /
BLUE ----->        CENTRAL RELAY        <----- RED
                     /    \
                    /      \
          WATER / CONDUCTIVE     ROUGH / TUNNEL
                 LANE                 LANE
```

Tre macro-direttrici:

1. **Centro**
   - objective;
   - cover;
   - linee di tiro;
   - Bastion pressure.

2. **Water / Conductive**
   - Riva setup;
   - Flux payoff;
   - friendly-fire risk;
   - terrain denial.

3. **Rough / Tunnel / alternate route**
   - Vektor prediction;
   - traps;
   - Fast Reaction;
   - noise/perception;
   - flank.

La versione più semplice può restare sul Layer 0.

La versione successiva può aggiungere:

```text
Layer 1:
bridge / platform / roof

Layer -1:
tunnel
```

Non bloccare il prototipo 4v4 sul multilivello se il multilivello non è ancora stabile.

---

# 8. Cosa deve dimostrare il 4v4

In 2v2 spesso esiste un singolo centro di attenzione.

Nel 4v4 devono poter avvenire contemporaneamente:

```text
CENTRO
Bastion vs Bastion
contest objective

LATO ACQUA
Riva + Flux
setup ambientale

FLANK
Vektor
predictive control

REACTION BOUNDARY
Overwatch / Intercept / Hold / Fire
```

La profondità desiderata è:

> più micro-conflitti simultanei che si influenzano a vicenda.

Non trasformare però il gioco in micro-management RTS.

---

# 9. Scenario narrativo 4v4 da usare come target di design

Questa sequenza è una **showcase target**, non una promessa che tutte le feature esistano già.

## Turno 1 — Apertura e split

Blue:

- Bastion prende il centro;
- Riva/Flux prendono la lane ambientale;
- Vektor minaccia flank.

Red:

- Bastion prepara controllo del choke;
- Riva sceglie un setup più difensivo;
- Flux cerca linea lunga;
- Vektor controlla la rotta alternativa.

Mostrare:

- planning simultaneo;
- ally intents;
- Ghost Timeline;
- separazione delle lane.

---

## Turno 2 — Setup Wet

Una Riva usa un'azione coerente con il catalogo corrente per:

```text
damage / Wet / Push
```

oppure crea superficie Wet quando la feature esiste.

Flux prepara il payoff.

Il nemico vede soltanto le informazioni lecite.

Obiettivo:

> far capire che l'ambiente è una risorsa condivisa e non un semplice VFX.

---

## Turno 3 — Predizione e whiff

Flux dichiara un attacco su una posizione/target secondo la moving-target policy.

Vektor usa Dash prima del Blast.

L'attacco viene rivalidato.

Possibili esiti:

```text
AttackCell
Fizzle
Fallback
```

secondo la definition reale.

TurnLog deve spiegare:

```text
TargetMoved
FallbackPolicy
ImpactCell
```

---

## Turno 4 — Fast Reaction firma

Vektor ha preparato Overwatch / Intercept compatibile col sistema corrente.

Flux entra per primo.

Prompt:

```text
FIRE
HOLD
```

Vektor sceglie:

```text
HOLD
```

Riva entra dopo.

Nuova opportunity:

```text
FIRE
HOLD
```

Vektor:

```text
FIRE
```

Proprietà obbligatorie:

- HOLD non consuma la charge;
- il client non sa se arriverà un futuro trigger;
- trigger simultanei nello stesso micro-step vengono aggregati;
- no nested reaction nell'MVP;
- timeout = HOLD;
- 3 secondi baseline.

---

## Turno 5 — Informazione incompleta

Introdurre Smoke / Visibility / FoW quando il relativo dominio è reale.

Separare:

```text
LOS
Detection
Visibility
Awareness
```

La UI usa:

```text
Confermato
Previsto
Incerto
```

Non simulare fake FoW in UI se TeamKnowledge non esiste ancora.

Prima:

- cap targeting;
- stato Obscured;
- debug visibility.

Dopo:

- team knowledge;
- last known;
- acoustic contacts.

---

## Turno 6 — Interposition

Bastion protegge un alleato.

Riutilizzare il sistema generale di:

```text
Intercept
Defensive Reaction
```

Non inserire branch hero-specific nel TurnManager.

Obiettivo:

> una difesa modifica la geometria/logica dello scontro, non solo il numero di HP.

---

## Turno 7 — Acqua + elettricità

Riva prepara rete Wet/Water/Conductive.

Flux usa propagazione elettrica.

Mostrare:

```text
source
propagation
ordered cells
ordered targets
each target once
friendly-fire if allowed
TurnLog reason
```

Ordine stabile.

---

## Turno 8 — Objective > deathmatch

La partita deve poter essere decisa dall'obiettivo anche con un personaggio KO.

Objective update avviene nella fase canonica stabilita dal ruleset.

Non scrivere regole scenario-specific nel TurnManager.

---

# 10. Azioni generiche: canone da integrare

Baseline:

```text
Wait
Basic Attack
Interact
Brace
Move
Overwatch
```

---

## 10.1 Move come famiglia

Non creare tre abilità separate se non necessario.

Usare profili:

```text
Sneak
Move
Sprint
```

Esempio iniziale di bilanciamento:

```text
Sneak:
range ridotto
noise molto basso

Move:
range standard
noise standard

Sprint:
range alto
noise alto
reaction exposure alta
```

I numeri restano data-driven.

---

## 10.2 Overwatch universale

Decisione:

> Overwatch è universale come grammatica/azione di Planning, ma l'effetto dipende dal profilo del personaggio/equipaggiamento.

Esempi:

```text
Marksman:
Fire / Hold

Tank:
Intercept / Brace / Hold

Controller:
Push left / Push right / Hold

Assassin:
Ambush / Hold

Engineer:
Hack / Hold
```

Non rendere tutti i personaggi equivalenti.

---

# 11. Reaction framework

Overwatch NON deve essere un sottosistema separato.

Modello:

```text
Reaction Definition
+
Planning Intent
+
Current Working Resolution State
+
Trigger
=
Reaction Opportunity
```

Poi:

```text
Opportunity
-> Automatic
-> Conditional
-> Fast Select
```

Tre policy utili:

```text
Automatic
Conditional
Fast Select
```

Questo è necessario perché Overwatch universale non deve aprire 20 prompt per turno.

---

# 12. Reaction Opportunity

Una opportunity contiene solo informazione valida al boundary corrente.

NON contiene:

- future path;
- future positions;
- future opportunity count;
- enemy canonical intent;
- future target list.

Trigger simultanei:

```text
Enemy A enters
Enemy B enters
```

nello stesso micro-step:

```text
ONE opportunity

FIRE A
FIRE B
HOLD
```

NON due prompt sequenziali derivati dall'ordine di iterazione.

---

# 13. Fast Action vs Fast Reaction

Condividono infrastruttura, non semantica.

```text
Fast Action:
continuazione limitata di una propria azione

Fast Reaction:
decisione provocata da un evento esterno
```

Esempi validi:

```text
LEFT / RIGHT
FIRE / HOLD
INTERPOSE / BRACE
TARGET A / TARGET B / HOLD
```

Non consentire:

```text
scegli una nuova ability completa
scegli una cella arbitraria su tutta la mappa
ricostruisci il piano
```

---

# 14. Delayed Actions

Decisione:

> una Delayed Action è completamente configurata nel Planning ma risolve a un boundary futuro.

Boundary baseline:

```text
EndPrep
EndDash
EndBlast
EndMove
```

Esempio:

```text
EndMove
TargetCell = H12
```

Il giocatore prevede dove sarà il nemico.

Nessuna nuova scelta live.

---

# 15. Predictive Actions / Tactical Gambits

Famiglia concettuale:

```text
Position
Path
Movement
Dash
Attack
Ability
Target
Direction
Interaction
Surface
EndPosition
Noise
Visibility
Projectile
```

Regola di balance:

> più precisa è la previsione, maggiore può essere il payoff.

Non trasformare ogni predictive action in Fast Reaction.

---

# 16. Trap system

Distinguere:

```text
Predictive Action
Trap / Persistent Triggered Effect
Fast Reaction
```

Trap MVP future:

```text
Intercept Cell
Tripwire / Edge Trap
Punish Action
```

Una trap può avere valore anche senza attivarsi:

- devia un path;
- forza Scan;
- rallenta;
- consuma una risorsa;
- crea bluff.

Evitare:

```text
invisible huge explosion
zero telegraph
zero counterplay
```

---

# 17. Auxiliary Units

Introdurre un concetto unico data-driven:

```text
AuxiliaryUnit
```

Non creare sistemi separati per:

```text
Pet
Drone
Turret
Summon
Gadget
Construct
Decoy
```

Baseline:

```text
Max active per owner:        1
Abilities proprie:           0
Auxiliary Command / turno:   0..1
Independent Ready:           NO
Independent Planning timer:  NO
Objective capture:           NO
Fast Reaction manuale:       NO
Stable ID:                   SI
Snapshot:                    SI
TurnLog:                     SI
```

Regola di action economy:

> un'Auxiliary Unit non concede normalmente un secondo turno completo.

---

## 17.1 Primo prototipo consigliato

Scout Drone:

```text
HP: 30
Move: 3
Attack: none
Occupancy: non-blocking
```

Comandi:

```text
FOLLOW
SCOUT Cell
WATCH Direction
```

Serve per validare:

- perception;
- path;
- TeamKnowledge;
- destruction;
- ownership;
- TurnLog;
- noise.

---

## 17.2 Turret

Secondo prototipo.

Riusa Reaction/Overwatch.

Policy baseline:

```text
FireFirstValid
```

Nessuna Fast Reaction manuale necessaria.

---

# 18. Fog of War, visibilità e Team Knowledge

Principio:

> Non vedere non significa non sapere nulla.

Distinguere almeno:

```text
Visible
Detected
Identified
Last Known
Acoustic Contact
Unknown
```

Il client deve ricevere solo la conoscenza autorizzata per la propria squadra.

Architettura:

```text
Authoritative state
    ↓
Perception
    ↓
Team Knowledge
    ↓
Sanitized UI
```

Mai:

```text
Hidden enemy state
    ↓
client
    ↓
widget hidden
```

---

# 19. Rumore come risorsa informativa

Il rumore non è un semplice debuff.

Ogni evento può produrre:

```text
FRTNoiseEvent
Source
OriginCell
Type
Intensity
Turn
MicroStep
```

Propagazione sul grafo tattico.

Non usare `SphereOverlap` come autorità competitiva.

Esempi di fonti:

```text
Sneak
Move
Sprint
Dash
Door
Rifle
Explosion
Fire
Electricity
Generator
Tunnel echo
Auxiliary unit
Decoy
```

---

## 19.1 Nessun RNG nascosto per la percezione base

Preferire:

```text
ReceivedNoise >= HearingThreshold
```

Non:

```text
65% chance to hear
```

---

## 19.2 Rumore + Fast Reaction

Possibile futuro:

```text
NoiseDetected
>= threshold
-> Reaction Opportunity
```

Il client può ricevere:

```text
Movement detected in NE sector
```

senza ricevere:

- enemy path;
- enemy destination;
- exact unit se non identificata.

---

# 20. Action Ghosts / Ghost Timeline

Planning UI:

```text
PREP | DASH | BLAST | MOVE
```

Ghost:

- posizione;
- facing;
- endpoint Dash;
- attack origin;
- line/AoE;
- Move finale;
- reaction arc;
- delayed boundary.

Gli Action Ghost sono presentation-only.

Non devono decidere l'esito.

---

## 20.1 Certainty grammar

Usare sempre:

```text
Confermato
Previsto
Incerto
```

### Confermato

Stato pubblico + regola deterministica.

### Previsto

Include intenti propri/alleati.

### Incerto

Dipende da:

- nemico;
- Fog of War;
- collisioni future;
- Reaction;
- target moved;
- future environment.

---

# 21. HUD con 4v4

Il 4v4 è un test critico di leggibilità.

Obiettivi UI:

- centro libero;
- focus su unità selezionata;
- collapse degli intenti non rilevanti;
- ally intent filter;
- reaction prompt compatto;
- Ghost Timeline per unità, non 8 timeline sempre espanse;
- event focus nel TurnLog;
- overlay modes:
  - Movement;
  - Vision;
  - Sound;
  - Threat;
  - Terrain.

Regola:

> Non mostrare contemporaneamente tutto ciò che il sistema conosce.

---

# 22. Automated Scenario Test Harness

Il Test Harness deve essere promosso a infrastruttura trasversale.

Workflow:

```text
Claude Code
-> scenario testuale
-> Unreal
-> AutoRun
-> intents
-> Ready
-> Snapshot
-> Resolver
-> TurnLog
-> assertions
-> result.json
-> Claude analysis
```

Non creare una seconda simulazione.

---

## 22.1 Modalità

```text
Visual
Fast
Headless
```

Lo stato finale deve essere logicamente equivalente.

---

## 22.2 Scenari minimi

Aggiungere progressivamente:

```text
Movement.Basic
Movement.Collision
Terrain.Ice.Slide
Terrain.Fire.OnEnter
Terrain.WetElectric.Basic
Reaction.Overwatch.HoldThenFire
Reaction.Overwatch.SimultaneousTargets
Delayed.EndMove.Whiff
Predictive.InterceptCell.Hit
Predictive.InterceptCell.Miss
Trap.Edge.Cross
Auxiliary.Drone.Scout
Visibility.LastKnown
Noise.BasicPropagation
Noise.Tunnel
Objective.BasicContest
Showcase.Relay.2v2
Stress.CoreRoster.4v4
```

Non implementarli prima dei sistemi corrispondenti.

---

# 23. Determinismo

Formula:

```text
same snapshot
+
same accepted intents
+
same decisions
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

Le Fast Decision introducono nuovi input canonici.

Quindi:

- devono essere loggate;
- devono entrare nel replay;
- timeout deve produrre decisione canonica;
- il resolver deve fermarsi solo su decision boundary deterministici.

---

# 24. TurnLog

Il TurnLog deve spiegare:

```text
what
who
when
why
before
after
reason
```

Nuovi event type probabili:

```text
ReactionArmed
ReactionOpportunityCreated
FastDecisionCommitted
FastDecisionTimedOut
ReactionHeld
ReactionCommitted
DelayedActionScheduled
DelayedActionResolved
PredictionWhiffed
TrapArmed
TrapTriggered
NoiseEmitted
NoiseDetected
KnowledgeUpdated
AuxiliarySpawned
AuxiliaryCommanded
AuxiliaryDestroyed
```

Non aggiungerli tutti se non servono alla milestone.

---

# 25. Stato corrente del codice da rispettare

L'handoff recente della showcase indica già:

```text
GameMode con:
GeneratedDemoArena
GeneratedTestArena

Team0Heroes:
Hero.Flux
Hero.Riva

Team1Heroes:
Hero.Bastion
Hero.Vektor
```

Il motore azioni possiede già concetti come:

```text
ActionId
ResolutionPhase
Priority
RangeCells
CostMP
CooldownTurns
Fallback
Slot
MovementStyle
PropagationLimit
Effects
bAllowsReaction
ReactionTrigger
bCanBeInterrupted
bFriendlyFire
```

Esistono già sistemi per:

```text
Reaction
Defensive Reaction
Intercept
Suppression
movement micro-step
```

Prima di creare nuovi tipi:

> cercare se una generalizzazione del codice corrente basta.

---

# 26. Fast Reaction: dipendenza tecnica critica

Il movimento corrente può ancora risolvere un path completo in una singola chiamata.

Un Fast Reaction interattivo richiede:

```text
step
-> inspect triggers
-> decision boundary
-> optional input
-> resume
```

NON rompere il resolver esistente in modo prematuro.

Ordine raccomandato:

1. ADR Fast Decision;
2. estrarre API step-able;
3. garantire equivalenza con vecchio resolver;
4. permutation tests;
5. repeat tests;
6. introdurre decision boundary;
7. Vektor InterceptShot come primo caso;
8. Overwatch universale dopo il proof.

---

# 27. Roadmap consolidata proposta

Non creare una mega-milestone per ogni nuova idea.

Integrare nelle milestone esistenti.

---

## F0 — Fondazioni / Core deterministico

Conservare scope stretto.

Aggiungere solo le fondamenta necessarie:

- hex axial stabile;
- 6-neighbor graph;
- Stable IDs;
- TurnLog estendibile;
- snapshot;
- deterministic movement;
- scenario test harness minimo;
- `Movement.Basic`;
- `result.json`.

NON implementare:

- FoW completo;
- noise completo;
- traps;
- auxiliary combat;
- Fast Reaction live.

Exit gate:

- golden movement;
- repeat;
- packaged/local smoke;
- no square-grid regression.

---

## F1 — Scenario / planning / privacy-ready boundary

Se la roadmap reale usa F1 per networking, mantenerla.

Aggiungere design/DTO readiness:

- classify intent fields;
- team-only planning model;
- Action Ghost consumes sanitized model;
- no enemy future data;
- scenario fixture loader;
- canary privacy test quando entra networking.

Non anticipare dedicated server.

---

## F2 — Abilities, Reactions, Predictive Core

Integrare:

- current 4-hero kit;
- generic actions;
- Overwatch profile data model;
- Reaction Opportunity;
- Delayed Action boundaries;
- Intercept Cell;
- Punish Action;
- Fast Decision ADR;
- movement step-able refactor;
- Vektor first interactive reaction;
- golden tests.

Exit gate:

```text
2 predictive abilities
hit
miss
1 Fast Reaction
hold then fire
simultaneous targets
repeat deterministic
```

---

## F3 — Environment, structures, multilayer, perception substrate

Integrare:

- Wet;
- Burning;
- Electrified;
- Smoke;
- water/electric;
- water/fire;
- cover;
- door;
- bridge;
- tunnel;
- GraphRevision;
- edge traps;
- acoustic propagation core;
- auxiliary movement compatibility;
- LOS/Detection separation.

Exit gate:

- water/electric golden test;
- edge trap;
- bridge/door revision;
- tunnel path;
- acoustic propagation pure test.

---

## F4 — Vertical Slice 2v2

Target:

```text
RT_Showcase_Relay_v01
Flux + Riva
vs
Bastion + Vektor
```

Include solo feature realmente verdi.

Obiettivi:

- complete loop;
- objective;
- UI;
- Fast Reaction se stabile;
- Ghost Timeline;
- TurnLog explainability;
- automation fixture;
- 20–30 minute short-play target se ancora utile.

---

## F4.5 — 4v4 Core Roster Stress Validation

Aggiungere checkpoint, non nuova produzione completa.

Scenario:

```text
Flux/Riva/Bastion/Vektor
vs
Flux/Riva/Bastion/Vektor
```

Validare:

- planning UX;
- 8 unità;
- reaction prompt count;
- resolver budget;
- TurnLog size;
- Ghost rendering;
- objective lanes;
- map size;
- match target <=45 min;
- split fight;
- performance.

Exit gate:

```text
4v4 scripted scenario completes
0 divergence
no reaction prompt storm
planning understandable
resolver metrics recorded
UI still readable
```

---

## F5 — Team Knowledge, Fog of War, Noise, Auxiliary Prototype

Se il progetto preferisce introdurre perception prima, spostare in base alle dipendenze reali.

Integrare:

- TeamKnowledge;
- visible/detected/last known;
- acoustic contacts;
- Sound overlay;
- Scout Drone prototype;
- drone as sensor;
- noise-driven reaction test.

Non dare al drone un kit completo.

---

## F6 — Multiplayer / Dedicated / Production hardening

Integrare secondo roadmap reale:

- authoritative server;
- team relay;
- zero-leak;
- reconnect;
- replay audit;
- 4v4 network stress;
- spectator policy;
- packaged soak.

Se il repository usa già un'altra numerazione F0–F6:

> non rinumerare tutto solo per questo documento. Inserire i checkpoint nei punti equivalenti.

---

# 28. Issue/backlog da creare o aggiornare

Prima:

```text
gh issue list
```

oppure usare la UI/API disponibile.

Non creare duplicati.

Proposta:

## Governance / docs

```text
[DOCS] Consolidate current canon and archive obsolete roster/timing rules
[DOCS] Lock phase order Planning -> Prep -> Dash -> Blast -> Move
[DOCS] Mark historical balance matrices and migrate current roster references
[DESIGN] Lock primary match format: 3v3 vs 4v4 and control model
[DESIGN] Lock match duration targets and map-size implications
```

## Generic actions

```text
[GAMEPLAY] Consolidate universal actions: Wait / BasicAttack / Interact / Brace / Move / Overwatch
[GAMEPLAY] Model Sneak/Move/Sprint as movement profiles
[GAMEPLAY] Add data-driven Overwatch profiles
```

## Fast decisions

```text
[ADR] Define deterministic Fast Action / Fast Reaction decision boundaries
[TURN] Refactor movement resolver to step-able session without output regression
[REACTION] Add ReactionOpportunity canonical model
[REACTION] Implement Hold / Commit / Timeout canonical decisions
[REACTION] Aggregate simultaneous triggers into one opportunity
[REACTION] Implement Vektor InterceptShot as first live Fast Reaction
```

## Predictive / traps

```text
[EPIC] Predictive Actions & Trap System
[TURN] Add named phase boundaries
[GAMEPLAY] Add Delayed Action scheduling
[GAMEPLAY] Prototype Intercept Cell hit/miss
[MAP] Add edge-trigger support for Tripwire
[GAMEPLAY] Prototype Punish Action event trigger
```

## Perception / noise

```text
[EPIC] Team Knowledge, Visibility & Acoustic Perception
[PERCEPTION] Separate LOS / Detection / Visibility / Awareness
[PERCEPTION] Add TeamKnowledge sanitized state
[NOISE] Add deterministic NoiseEvent model
[NOISE] Add graph acoustic propagation
[UI] Add Sound overlay and acoustic contacts
```

## Auxiliary

```text
[EPIC] Auxiliary Units
[UNIT] Add minimal AuxiliaryUnit data/runtime model
[UNIT] Add Follow / Guard / Command behavior
[TEST] Add Scout Drone scenario
[REACTION] Reuse Overwatch framework for automatic turret
```

## 4v4

```text
[DESIGN] Define 4v4 mirror stress scenario using core roster
[MAP] Graybox 4v4 three-lane arena
[TEST] Add deterministic 4v4 scripted fixture
[UI] Validate planning readability with 8 units
[PERF] Benchmark resolver and overlays in 4v4
[REACTION] Measure Fast Reaction prompt frequency in 4v4
```

---

# 29. Codice: cosa NON fare

Non fare:

```text
if Hero == Flux ...
if Hero == Vektor ...
if Turn == 4 move relay ...
if IsShowcase skip validation ...
if IsTest SetActorLocation ...
if IsTest ApplyDamage ...
```

Non creare:

- un secondo resolver;
- un secondo pathfinding;
- un sistema Overwatch parallelo;
- un sistema Turret Overwatch separato;
- un pet system separato dal generic auxiliary model;
- un fake TeamKnowledge solo UI;
- un fake FoW basato sul nascondere Actor che il client conosce già.

---

# 30. Codice: direzione preferita

Preferire:

```text
Data definition
    ↓
Intent
    ↓
Validation
    ↓
Snapshot
    ↓
Resolver
    ↓
TurnEvent
    ↓
TurnLog
    ↓
Presentation
```

Per input live:

```text
Resolver
    ↓
Decision Boundary
    ↓
Sanitized Opportunity
    ↓
Canonical Decision
    ↓
Resume Resolver
```

---

# 31. Test richiesti

Ogni feature significativa deve avere almeno:

1. happy path;
2. failure/whiff;
3. permutation test quando l'ordine può cambiare;
4. repeat deterministic test;
5. TurnLog reason assertion;
6. packaged smoke se coinvolge rete/UI/runtime asset.

Per 4v4:

```text
same scenario x N runs
same StateHash
same LogHash
```

Aggiungere metriche:

```text
resolver duration
events per turn
reaction opportunities per turn
manual prompts per turn
TurnLog size
ghost primitive count
```

Non fissare budget nuovi senza misura.

---

# 32. Acceptance criteria del consolidamento

Questo task documentale/operativo è completato quando:

```text
[ ] il roster operativo è unico nei docs correnti;
[ ] vecchi roster sono marcati historical;
[ ] Fast Reaction baseline è 3s;
[ ] phase order è unico;
[ ] Move è sempre ultimo;
[ ] hex axial è esplicito;
[ ] eventuale 4-way legacy è rimosso dai docs operativi;
[ ] generic actions sono documentate;
[ ] Overwatch universale è separato dall'effetto del singolo eroe;
[ ] Delayed / Predictive / Trap / Fast Reaction sono semanticamente distinti;
[ ] Auxiliary Unit è un concetto unico;
[ ] noise e TeamKnowledge hanno ownership/privacy chiare;
[ ] Test Harness è una capability trasversale;
[ ] scenario 4v4 è in roadmap come stress validation;
[ ] durata target 30–45 max è registrata;
[ ] issue esistenti sono riusate invece di duplicate;
[ ] nessun codice prematuro è stato aggiunto.
```

---

# 33. Output richiesto a Claude Code

Alla fine riportare:

## Audit

- branch;
- HEAD iniziale/finale;
- UE version reale;
- documenti canonici letti;
- test iniziali;
- conflitti trovati.

## File modificati

Per ogni file:

```text
path
perché
decisione consolidata
```

## Codice

Separare:

```text
implemented now
refactored
not implemented
future issue
```

## Issue

Per ogni issue:

```text
number
title
status
dependency
milestone
```

## Roadmap

Mostrare:

- checkpoint aggiornati;
- dipendenze;
- exit gate;
- cosa è stato esplicitamente rinviato.

## Test

Riportare comandi ed esito reale.

## Git

Proporre commit focalizzati, per esempio:

```text
docs(design): consolidate current gameplay canon
docs(roadmap): integrate reactions perception and 4v4 stress checkpoint
feat(test): add deterministic 4v4 scenario fixture
feat(reaction): add canonical reaction opportunity
```

Non accorpare tutto in un unico commit se tocca domini diversi.

---

# 34. Ordine di esecuzione consigliato per Claude

## Passo 1 — Audit

Nessuna modifica.

Confrontare:

- canone;
- codice;
- roadmap;
- issue;
- PDR;
- matrici.

## Passo 2 — Decision log

Aggiornare prima il canone documentale.

## Passo 3 — Roadmap

Integrare:

- Generic Actions;
- Fast Decisions;
- Predictive/Traps;
- Perception/Noise;
- Auxiliary;
- 4v4 Stress.

## Passo 4 — Issue

Creare solo le mancanti.

## Passo 5 — Codice minimo coerente con milestone

Non saltare dipendenze.

## Passo 6 — Test

Automation + scenario fixtures.

## Passo 7 — Report finale

Con delta chiaro.

---

# 35. Regola finale

RefactorTactics deve convergere verso un unico linguaggio sistemico:

```text
ACTION
REACTION
TRIGGER
PREDICTION
BOUNDARY
SURFACE
TRANSITION
PERCEPTION
KNOWLEDGE
AUXILIARY
EVENT
```

Le feature future devono comporsi da questi mattoni.

Evitare di creare una nuova eccezione ogni volta che nasce una nuova idea.

Il 4v4 non è soltanto “più personaggi”.

Deve diventare il test che verifica se:

- il planning resta leggibile;
- la simultaneità resta comprensibile;
- le reaction non bloccano il ritmo;
- la mappa produce più fronti;
- il TurnLog resta spiegabile;
- l'ambiente genera strategia;
- l'informazione incompleta genera deduzione;
- il resolver scala;
- le unità ausiliarie non rompono l'action economy;
- la partita resta entro il target massimo di durata.

Se queste proprietà reggono con otto unità, l'architettura core sta andando nella direzione giusta.
