# RefactorTactics — Overwatch Runtime Lifecycle, Watch Stage e Reposition
> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

## Handoff operativo per Claude Code

> 📦 **Archiviato il 2026-08-10 — recepito, non applicato.** L'esito del triage vive in
> [`roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md`](../../../roadmap/plans/overwatch-runtime-lifecycle-triage-2026-08-10.md),
> e ciò che resta aperto in [`OPEN_DECISIONS.md`](../../../OPEN_DECISIONS.md) come `OW-1`…`OW-4`.
> **Questo file non è autorità**, ma a differenza del gemello sulle Base Action **non è stato superato**: il
> suo modello `Watch → EndWatchStage → Reposition` chiude `BAS-5` e prevale sul «Move a budget ridotto».
> Quattro delle sue regole sono già canone-compatibili — cadence *once-per-target*, `MaxPrompts` che conta
> opportunity distinte, eligibility post-transition, hard cancel ≠ soft block.
>
> ⚠️ **Tre cose non vanno prese alla lettera.** `Reposition` (§12) è un nome **già occupato**:
> `Action.Reposition` è uno scatto di 2 celle in fase **Dash** (`RTCatalogLibrary.cpp:365`). Gli
> ScenarioId `REACT-001…011` (§40) sono uno schema **già respinto** il 2026-08-08 perché era un terzo
> formato. E **sette** degli undici feature ID del §36 non esistono nel registry. Leggilo per la
> **provenienza** e per il modello, mai per gli identificatori.

**Data:** 2026-08-10  
**Scope:** consolidamento dell'ultimo focus su Overwatch, lifecycle runtime, timing nella Move Phase, trigger per micro-step, HOLD/FIRE, simultaneità, counterplay pre-Watch, Reposition post-Overwatch, privacy, test, Wiki, Feature Map, Scenario Map, Roadmap, Epic/Issue GitHub.  
**Baseline tecnica documentale:** Unreal Engine 5.8; verificare e bloccare la patch/toolchain effettive del repository prima di implementare.  
**Importante:** questo handoff contiene decisioni più recenti rispetto ad alcuni handoff Overwatch precedenti. Non fare search/replace cieco: eseguire audit e conflict report prima di modificare il canone.

---

# 0. OBIETTIVO

Consolidare nel repository il modello Overwatch discusso il 10 agosto 2026.

La direzione corrente è:

> Overwatch è una Universal Action distinta da Brace. Il personaggio dedica il turno a una contromossa di controllo spazio. Durante la prima parte della Move Phase resta fermo e osserva il Move avversario. Terminata la Watch Window, Overwatch è finita e il personaggio può eseguire soltanto un Reposition limitato, già pianificato.

Il modello NON deve diventare:

- una stance mobile che segue il personaggio;
- una skill hard-coded di Wraith o di un singolo eroe;
- un sistema che ricalcola automaticamente percorsi dopo aver visto la Resolution;
- una reservation nascosta di celle future;
- una catena arbitraria di reaction annidate;
- un trigger a ogni singolo passo dello stesso bersaglio.

---

# 1. AUDIT OBBLIGATORIO PRIMA DI MODIFICARE

Eseguire almeno:

```bash
git status
git branch --show-current
git rev-parse HEAD
```

Individuare:

```text
CLAUDE.md
AGENTS.md
README.md
CONTEXT_INDEX.md

docs/product/*
docs/decisions/*
docs/gameplay/*
docs/technical/*
docs/balance/*
docs/roadmap/*
docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.md
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/technical/scenario-map.md

Scenarios/
Source/RefactorTactics/Turn/
Source/RefactorTactics/Tests/
Source/RefactorTactics/ScenarioHarness/
Wiki / wiki source reale
```

Cercare repository-wide:

```text
Overwatch
Brace
ReactionOpportunity
ReactionInstance
Fast Reaction
FIRE
HOLD
MaxPrompts
ControlledArea
SuppressiveZone
Watch
Reposition
Sprint
Move
Dash
Forced Movement
Teleport
Facing
Decision Boundary
Reaction Batch
E14
E16
feature-registry
scenario-map
```

Leggere in particolare, se ancora presenti:

```text
RefactorTactics_Overwatch_FastReaction_Claude.md
RT_Reaction_System_Master_Consolidation_v0.1.md
CLAUDE_Consolidamento_BaseAction_Signatures_Brace_Overwatch_2026-08-10.md
RefactorTactics_AzioniGeneriche_Overwatch_Universale_v0.1.md
RefactorTactics_ActionGhosts_Phases_FastReactions_Claude.md
RefactorTactics_Decision_Time_Bank_Claude_Consolidation_2026-08-09.md
CLAUDE_Showcase_v0.1_Integration_CurrentCode.md
```

Verificare anche codice esistente vicino a:

```text
FRTSuppressiveZone
FRTSuppressionMover
FRTSuppressionHit
MakeSuppressiveZone
ResolveSuppression
ResolveHexPaths
```

Non creare una seconda geometria Overwatch se la primitive di controllo celle/edge esistente può essere generalizzata.

---

# 2. ORDINE DI PREVALENZA

Usare:

```text
Decision Log / ADR più recenti
> codice + cataloghi correnti
> decisioni esplicite più recenti di progetto
> Feature Registry / Scenario Registry / Roadmap correnti
> questo handoff
> handoff precedenti
> PDR / workbook / ricerca storica
```

Se il repository contiene una decisione più recente in conflitto:

1. non sovrascrivere silenziosamente;
2. classificare il conflitto;
3. preservare il canone più recente;
4. aprire/aggiornare una decision issue;
5. riportare il conflitto nel report finale.

Classificazioni:

```text
CURRENT
SUPERSEDED
PROPOSED
OPEN_BALANCE
CONFLICT
HISTORICAL
```

---

# 3. DECISIONE: OVERWATCH È DISTINTA DA BRACE

Preservare:

```text
Brace != Overwatch
```

Entrambe possono usare il Reaction Framework, ma sono azioni universali distinte.

```text
Reaction Framework
├── Brace
└── Overwatch
```

Overwatch non deve essere trasformata in una sotto-opzione di Brace senza una nuova decisione esplicita.

---

# 4. COSTO E SIGNIFICATO DI OVERWATCH

Direzione corrente:

```text
Attack OR Ability OR Overwatch
```

Overwatch rappresenta:

> "Per questo turno mi concentro su una contromossa di controllo spazio."

Il costo non è solo rinunciare all'azione offensiva. Il Move normale viene sostituito da un Reposition limitato eseguito dopo la Watch Window.

Nessun refund se:

- Overwatch non viene triggerata;
- il giocatore fa HOLD;
- Overwatch viene cancellata;
- FIRE non produce il risultato sperato.

---

# 5. MACRO-FASI: NON CAMBIARLE

Preservare:

```text
Planning
-> Commit
-> Prep
-> Dash
-> Blast
-> Move
-> Cleanup
```

`Watch` e `Reposition` NON sono nuove macro-fasi.

Sono stage logici interni alla `Move Phase`.

Proposta consolidata:

```text
MOVE PHASE

Stage A — WATCH / STANDARD MOVE
    - unità Overwatch: ferme
    - altre unità: Move normale per micro-step
    - Overwatch attiva e può generare Reaction Opportunity

End Watch Stage
    - ogni Overwatch residua termina / Expired

Stage B — REPOSITION
    - ex-unità Overwatch eseguono Move limitato pre-pianificato
    - Overwatch non esiste più
    - Reposition simultaneo fra tutte le unità interessate
```

---

# 6. PLANNING DI OVERWATCH

Durante Planning il giocatore prepara almeno:

```text
Overwatch Profile
Watch Origin
Watch Facing
Controlled Area / geometry
Trigger policy
Reaction policy
Reposition path opzionale
Reposition final facing
```

Il Reposition DEVE essere pianificato prima della Resolution.

Vietato:

```text
vedo dove sono finiti gli avversari
-> poi scelgo il Reposition
```

Gli alleati possono vedere Watch + Reposition per coordinarsi.

Gli avversari non devono ricevere il planning.

---

# 7. WATCH FACING

La direzione della contromossa è preparata nel Planning.

Usare concettualmente:

```text
PreparedWatchFacing
```

distinto dal:

```text
FinalFacing
```

del Reposition.

Durante Stage A:

```text
WatchFacing = PreparedWatchFacing
```

Durante Stage B Overwatch è già terminata; il normale sistema di facing può applicare il `FinalFacing` del Reposition.

Non implementare una killzone che ruota automaticamente seguendo il pivot del personaggio durante Stage A.

Per v0.1:

```text
forced orientation change prima del Watch
-> cancella Overwatch
```

salvo nuova policy esplicita.

---

# 8. LIFECYCLE RUNTIME

Tenere separati:

```text
Planning Intent
!=
Reaction Runtime Instance
```

Stati runtime consigliati:

```text
Inactive
Armed
OpportunityPending
Committed
Resolved
Expired
Cancelled
```

Terminali distinti:

```text
Resolved  = reaction consumata/completata via commit
Expired   = finestra finita senza commit utile
Cancelled = invalidazione esterna
```

Overwatch può essere materializzata/armata all'inizio della Resolution ma non essere trigger-eligible finché non inizia il Watch Stage.

`Armed != TriggerEligible`.

---

# 9. COUNTERPLAY PRIMA DEL WATCH

Prep, Dash e Blast devono poter modificare la condizione dell'Overwatch prima della sua finestra.

Baseline corrente:

| Evento prima del Watch | Risultato |
|---|---|
| semplice Damage | preserve |
| KO | cancel |
| Stun | cancel |
| Knockdown | cancel |
| Disarm / required weapon lost | cancel |
| Forced Movement dell'owner | cancel |
| forced facing/orientation | cancel v0.1 |
| Suppression | soft block |
| Smoke | eligibility only |
| LOS persa | eligibility only |
| Detection persa | eligibility only |
| Door open/close | world state, non lifecycle |
| Cover creata/distrutta | world state, non lifecycle |
| origin mismatch al Watch | fail/cancel |

Distinguere:

```text
Hard Cancellation
vs
Soft Eligibility Block
```

Una Overwatch cancellata NON si riarma nello stesso turno solo perché la condizione torna valida.

Una soft-blocked può tornare trigger-eligible se la condizione temporanea termina prima di EndWatchStage.

---

# 10. DASH E OVERWATCH BASE

Overwatch universale v0.1 controlla lo Stage A del `Move`.

Il Dash avviene prima.

Quindi:

```text
Enemy Dash attraversa la futura killzone
-> nessun trigger della Overwatch standard
```

Questo è counterplay temporale naturale derivato dall'ordine delle fasi.

Non introdurre una finta immunità speciale "Dash immune to Overwatch": semplicemente la Overwatch base non è ancora nella propria fase valida.

Profili futuri `AntiDash` possono usare lo stesso framework ma sono fuori dalla baseline universale.

Per la Universal Overwatch v0.1:

```text
Choose Overwatch
-> no voluntary Dash nello stesso piano
```

salvo nuova eccezione character-specific approvata in futuro.

---

# 11. MODELLO WATCH + REPOSITION

Rifiutare il vecchio modello "Mobile Overwatch".

NON fare:

```text
owner A -> B -> C
cone A -> cone B -> cone C
```

Durante Stage A l'owner Overwatch resta fermo.

FIRE presto NON sblocca il movimento prima.

HOLD NON ritarda il movimento oltre EndWatchStage.

Tutti gli owner Overwatch attendono la chiusura dello Stage A.

Poi eseguono il Reposition nello Stage B.

---

# 12. REPOSITION

`Reposition` è un profilo di Move conseguente alla scelta Overwatch.

Non confonderlo con:

```text
Sneak
Dash
ReactionMovement
Teleport
```

Modello preferito:

```text
MovementKind = Move
MovementProfile = OverwatchReposition
```

o equivalente già esistente nel codice.

Il Reposition:

- ha budget ridotto;
- usa normali costi, occupancy, hazard, edge e collision rules;
- è pre-pianificato;
- è risolto per micro-step;
- può essere parzialmente completato;
- non è un teleport;
- non è automaticamente silenzioso.

Il valore esatto del budget resta `OPEN_BALANCE`.

NON fissare percentuali come canone.

Preferire valore intero/data-driven:

```text
NormalMoveBudget
OverwatchRepositionBudget
```

---

# 13. NESSUNA RESERVATION AUTOREVOLE

Decisione corrente:

> il Reposition non prenota segretamente la destinazione.

Planning:

```text
Reposition A -> B -> C
```

Se durante Stage A un'altra unità occupa C, C resta occupata.

Nessun diritto speciale perché C era una destinazione pianificata.

La late spatial priority è un costo reale dell'Overwatch.

NON creare:

```text
HiddenReservedCell
ReservedDestinationBlocksEnemy
```

Motivi:

- planning futuro non deve occupare fisicamente il mondo;
- sarebbe un leak dell'intento;
- renderebbe Overwatch troppo sicura.

È ammesso solo un `soft claim` UI/team-only per warning fra alleati.

---

# 14. REPOSITION INVALIDATION

Se il path:

```text
A -> B -> C
```

trova:

```text
B free
C occupied
```

risultato:

```text
A -> B
B -> C blocked
Final = B
```

Non cancellare l'intero Reposition per un blocco tardivo.

NON auto-reroute.

Se invece:

```text
ExpectedStart = A
ActualStart = X
```

per displacement precedente:

```text
RepositionInvalidated
Reason.StartCellMismatch
```

Nessun nuovo A* automatico da X.

---

# 15. NESSUN REFUND

Questa regola deve essere esplicita.

Se Overwatch:

```text
FIRE
oppure
HOLD fino a fine finestra
oppure
non viene mai triggerata
oppure
viene cancellata
```

il personaggio mantiene comunque soltanto il profilo `OverwatchReposition`.

Mai convertire a Move completo.

Mai restituire l'azione offensiva.

La decisione di Planning è già stata spesa.

---

# 16. STAGE B — SIMULTANEITÀ

Tutte le unità che devono fare Reposition lo risolvono simultaneamente fra loro per micro-step.

Niente ordine artificiale:

```text
Wraith reposition
poi Riktor reposition
```

Se due unità tentano la stessa cella nello stesso micro-step, applicare la collision policy corrente del Movement Resolver.

Se la baseline corrente è:

```text
A -> X <- B
=> both blocked
```

riutilizzarla, non creare una policy Overwatch parallela.

---

# 17. OBJECTIVE TIMING

Direzione proposta da verificare contro il canone corrente:

```text
Stage A
-> Stage B
-> movement-complete objective evaluation
```

Gli objective `OnEnter` possono reagire normalmente quando una cella viene attraversata.

Gli objective che dipendono dalla posizione finale dovrebbero essere valutati dopo Stage B.

Se il repository ha una decisione diversa, aprire conflict/decision issue; non modificare silenziosamente.

---

# 18. OVERWATCH STANDARD: TRIGGER RUNTIME

Durante Stage A, Overwatch osserva il Move volontario per micro-step.

Non usare soltanto la semantica stretta:

```text
EnemyEnterArea
```

perché un bersaglio può iniziare Stage A già dentro la zona.

La semantica da consolidare è:

> una unità genera un contatto Overwatch quando compie una transizione di Move volontario che interessa la geometria controllata.

Una transizione è candidata se, secondo il modello Controlled Area scelto:

```text
FromCell controlled
OR
ToCell controlled
OR
CrossedEdge controlled
```

e tutti gli altri requisiti risultano validi.

Il target fermo nella zona NON genera trigger.

Target già dentro che si muove: può generare trigger.

---

# 19. ORDINE DEL MICRO-STEP

Non valutare Overwatch mentre si itera un'unità alla volta.

Per ogni micro-step:

```text
1. collect/resolve simultaneous Move transitions
2. resolve collision rules
3. apply authoritative positions
4. update occupancy
5. apply immediate transition/surface effects appropriati
6. update LOS / Detection / visibility
7. emit movement events
8. evaluate armed Overwatch
9. aggregate valid opportunities
10. open Reaction Batch / Decision Window se necessario
11. apply committed reactions
12. finalize interruption / KO / displacement per il boundary
13. proceed to next micro-step
```

L'ordine preciso deve essere allineato al resolver esistente e versionato.

Niente dipendenza da:

- Tick;
- FPS;
- animation timing;
- TMap/TSet order;
- packet arrival.

---

# 20. LOS E DETECTION AL BOUNDARY

Baseline v0.1:

> eligibility valutata sullo stato POST-transition.

Esempio:

```text
A -> B
B è nella smoke
LOS post-step = false
=> no Opportunity
```

Se il target esce dalla smoke in B:

```text
LOS post-step = true
Detected = true
=> può generare Opportunity
```

Non introdurre geometria continua sub-edge nel vertical slice senza necessità.

---

# 21. UNA SOLA OPPORTUNITY PER TARGET

Decisione corrente molto importante:

> la Overwatch base può offrire al watcher al massimo una Reaction Opportunity per target per Reaction Instance.

Quindi:

```text
Tank entra
-> FIRE / HOLD

HOLD

Tank continua dentro la killzone
-> niente altro prompt da quella Overwatch
```

Anche:

```text
Tank entra
-> HOLD
Tank esce
Tank rientra
-> niente nuovo prompt
```

La cadence base è:

```text
OncePerTargetPerReactionInstance
```

Non `PerStep`.

Non `PerEntry`.

Profili futuri possono estendere la cadence, ma non la baseline v0.1.

Runtime può mantenere concettualmente:

```text
ConsumedTargetOpportunities
```

o un tipo equivalente con StableUnitId.

Non usare il nome come API definitiva senza adattarlo al codice.

---

# 22. PERCHÉ ONCE-PER-TARGET

La semantica di HOLD deve restare:

> "Lascio passare questo bersaglio e scommetto che arriverà un'occasione migliore."

Se lo stesso target riapre FIRE/HOLD a ogni cella:

- HOLD perde significato;
- MaxPrompts può essere consumato da un solo bersaglio;
- il baiting Tank -> Scout -> Carry viene distrutto;
- la Resolution genera prompt storm.

Questa decisione va riflessa in Wiki/player-facing explanation e Scenario Map.

---

# 23. TARGET SIMULTANEI

Se due o più target diventano validi nello stesso micro-step logico:

```text
Tank
Scout
```

creare UNA sola opportunity:

```text
[FIRE Tank]
[FIRE Scout]
[HOLD]
```

Non prompt sequenziali.

Una opportunity multi-target conta come UN prompt.

Se il giocatore sceglie HOLD:

```text
tutti i target della opportunity
-> consumed/declined per quella Reaction Instance
```

Se sceglie FIRE su uno:

- con 1 charge la Overwatch termina;
- gli altri target non generano un prompt artificiale subito dopo.

Pensare già a future multiple charges, ma non espandere scope v0.1.

---

# 24. MAX PROMPTS

Baseline storica:

```text
MaxPromptsPerReaction = 3
```

Resta data-driven e da playtest.

Interpretazione nuova da consolidare:

> MaxPrompts conta Reaction Opportunities distinte, non passi, non unità individuali dentro una opportunity aggregata.

Esempio:

```text
Opportunity #1: Tank -> HOLD
Opportunity #2: Scout -> HOLD
Opportunity #3: Carry -> HOLD
=> Expired.MaxPromptsReached
```

Valore `3` = `OPEN_BALANCE / PLAYTEST`, non hard-code di design.

---

# 25. FIRE E MOVIMENTO

La transizione che ha generato l'Opportunity è già completata.

Esempio:

```text
A -> B
     ^
     Overwatch fires here
```

FIRE NON riporta il target ad A.

L'effetto Overwatch può:

```text
damage only
```

e lasciare continuare:

```text
B -> C
```

oppure avere payload:

```text
MovementInterrupted
```

e fermare i micro-step successivi.

La regola universale Overwatch NON deve implicare automaticamente stop movement.

È il payload/profilo a determinarlo.

---

# 26. FIRE / HOLD BASELINE

Preservare:

```text
FastReactionDuration = 3.0 s
Responses = FIRE / HOLD
Timeout = HOLD
Charges = 1
```

`HOLD`:

- rifiuta l'opportunity corrente;
- non consuma la charge;
- marca come consumate le target opportunity incluse;
- mantiene Overwatch armata se non scaduta.

`FIRE`:

- committa;
- consuma la charge;
- risolve il payload;
- con 1 charge porta normalmente a `Resolved`.

Il giocatore non deve sapere se arriverà un'altra opportunity.

---

# 27. END WATCH STAGE

EndWatchStage avviene quando non restano normali Standard Move da risolvere nello Stage A, secondo le regole del resolver.

A quel boundary:

```text
remaining Armed Overwatch
-> Expired.EndWatchStage
```

Da quel momento:

```text
Overwatch = finita
```

Lo Stage B Reposition NON può generare trigger dalla vecchia Overwatch.

---

# 28. REACTION BATCH / MULTIPLE OVERWATCH

Se più Overwatch diventano valide nello stesso boundary:

```text
Reaction Batch
├── Watcher A Opportunity
├── Watcher B Opportunity
└── ...
```

Non sequenziare artificialmente prompt in base all'ordine di iterazione.

Se più unità appartengono allo stesso player, usare una singola Decision Batch/deadline se l'infrastruttura Decision Window lo consente.

Player differenti possono decidere in parallelo.

Non sommare:

```text
3s + 3s + 3s
```

se le decisioni sono indipendenti e simultanee.

Rispettare il Decision Time Bank esistente senza crearne uno nuovo specifico per Overwatch.

---

# 29. SAME-BOUNDARY COMMIT

Una response validata e accettata sul boundary è `boundary-locked`.

Non cancellare retroattivamente un commit solo perché un altro effetto dello stesso Reaction Batch viene applicato prima per tie-break interno.

Obiettivo:

```text
valid at boundary
+ accepted commit
=> commit esiste
```

Gli outcome possono comunque essere:

- negati dal Reaction Clash;
- modificati dalla BindingPolicy;
- ridotti/annullati dalle regole specifiche.

Ma il semplice ordine di iterazione non deve decidere chi "ha fatto in tempo".

---

# 30. REACTION-GENERATED MOVEMENT E NO NESTING

Se una reaction produce:

```text
Push
Knockback
ReactionMovement
```

gli eventi spaziali normali vengono prodotti.

Possono applicarsi:

- hazard automatici;
- occupancy;
- LOS/Detection;
- environment;
- log.

Ma nell'MVP NON aprire una nuova Fast Reaction interattiva da quell'evento.

No:

```text
Overwatch
-> Push
-> Overwatch
-> Dodge
-> Overwatch
```

Preservare `NoNestedInteractive` per MVP.

---

# 31. PRIVACY

Non replicare planning Overwatch avversario.

Gli alleati possono ricevere:

```text
Watch
Reposition path
Final facing
labels
ready
```

Gli avversari no.

Hidden Overwatch + HOLD:

```text
target receives ZERO information
```

Una committed FIRE può generare informazioni legittime secondo Perception/TeamKnowledge, ma non deve rivelare automaticamente più di quanto il gameplay consente.

Una Reaction Opportunity sanitizzata non contiene:

- future enemy path;
- future trigger;
- future opportunities;
- canonical enemy intents;
- numero di trigger futuri.

Mantenere il threat-model del timing leak: una Decision Window privata non deve rivelarsi semplicemente tramite freeze globale o timing osservabile.

---

# 32. UI / ACTION GHOST

Durante Planning, per il team autorizzato, mostrare chiaramente due elementi distinti:

```text
WATCH
- area
- origin
- prepared facing
- trigger/profile
- certainty

REPOSITION
- path ridotto
- destination ghost
- final facing
```

Non usare lo stesso stile visuale.

Suggerimento:

```text
Watch = area/sector + stance ghost
Reposition = path ghost secondario / post-watch
```

Il Reposition è un intento legalmente valido al Planning, ma la sua riuscita finale è più incerta perché viene eseguito dopo gli Standard Move.

Usare `Confermato / Previsto / Incerto` senza derivare warning da intenti nemici nascosti.

---

# 33. CONTROLLED AREA

Riutilizzare/generalizzare primitive già esistenti.

Non creare una geometria parallela solo per Overwatch se `FRTSuppressiveZone` o servizi equivalenti possono essere estesi in modo pulito.

La primitive comune dovrebbe poter rappresentare almeno, se necessario:

```text
ControlledCells
ControlledEdges
Origin
PreparedFacing
Shape/Profile
Range
SpatialRevision
```

Separare:

```text
Controlled Geometry
!=
Target Eligibility
```

LOS, Detection, reaction state e charge non devono essere incorporati dentro la geometria.

---

# 34. SUPERSEDED / DA NON REINTRODURRE

Marcare o correggere materiale che presenta come corrente:

## Overwatch come parte di Brace

```text
Brace -> Overwatch
```

SUPERSEDED.

Brace e Overwatch restano entry point distinti sul Reaction Framework.

## Mobile Gunner / Overwatch durante Sprint

```text
Overwatch follows owner movement
Mobile Gunner preserves Overwatch during Sprint
```

REJECTED per baseline.

## Mobile Overwatch

```text
Move owner
-> killzone follows owner
```

REJECTED.

## Overwatch = immobilità totale fino a fine turno

SUPERSEDED dalla soluzione:

```text
Watch stationary
-> Reposition limitato
```

## FIRE sblocca subito il Move

REJECTED.

Stage B inizia solo quando Stage A è terminato.

## Reposition scelto live dopo aver visto gli avversari

REJECTED.

Reposition è Planning data.

## Destination reservation

REJECTED.

Nessuna reservation autorevole/hidden.

## Auto-reroute Reposition

REJECTED.

## Same target prompt every step

REJECTED.

Baseline = once per target per Reaction Instance.

## Standard Overwatch reagisce a Dash

REJECTED per v0.1.

Il framework generale può supportare profili futuri Anti-Dash.

---

# 35. OPEN BALANCE / OPEN DESIGN

Non inventare valori.

Restano aperti:

```text
OverwatchRepositionBudget esatto
MaxPrompts definitivo (3 = playtest baseline)
range/shape per character profile
payload specifici dei character Overwatch
priorità definitiva tra tipi di reaction diversi
Reaction Clash definitivo
Decision Time Bank numeri definitivi
objective timing se il repo ha canone incompatibile
eventuali profili futuri PerEntry / PerTransition
eventuali AntiDash / AntiTeleport / AntiForced profiles
```

Non usare questi OPEN per bloccare il consolidamento delle regole già decise.

---

# 36. FEATURE REGISTRY / FEATURE MAP

Prima cercare feature esistenti.

Sono noti almeno concetti/ID del tipo:

```text
RT-FEAT-REACTION-OPPORTUNITY
RT-FEAT-REACTION-FAST
RT-FEAT-REACTION-OVERWATCH
RT-FEAT-REACTION-MULTI-TRIGGER
RT-FEAT-REACTION-SIMULTANEOUS
RT-FEAT-REACTION-NO-NESTED
RT-FEAT-REACTION-FACING
RT-FEAT-REACTION-PRIVACY
RT-FEAT-CORE-DECISION-BOUNDARY
RT-FEAT-CORE-MICROSTEPS
RT-FEAT-ACTION-MOVE
```

NON creare feature duplicate se queste possono assorbire lo scope.

Se il registry richiede feature owner più granulari e non esistono equivalenti, valutare ID coerenti per:

```text
Overwatch Lifecycle
Watch Stage
Post-Overwatch Reposition
Per-Target Opportunity Cadence
Reaction Batch / Same-Boundary Commit
```

Ma creare nuovi ID solo dopo audit del registry machine-readable.

Ogni feature deve collegare:

```text
Roadmap
Epic/Issue
Scenario
Test
Wiki
Dependencies
Status/Gates
```

---

# 37. ROADMAP

NON creare una roadmap parallela.

Verificare prima `E14`.

La Feature Map disponibile indica E14 come workstream naturale per:

```text
ReactionOpportunity
Fast Reaction
Overwatch
Multi-trigger
Simultaneous targets
No nested reaction
```

Usare E14 o Epic reale equivalente se confermato nel repository/GitHub.

Dipendenze possibili:

```text
E4 / movement resolver
E16 / Facing
network/privacy workstream
UI / planning ghost
scenario harness
```

Il `Reposition` post-Overwatch deve essere consolidato come dipendenza/estensione del Movement Resolver, non come secondo sistema di movimento.

Non spostare l'intera feature in una milestone nuova senza necessità.

---

# 38. EPIC GITHUB

Prima:

1. elencare Epic/Issue esistenti;
2. cercare Overwatch / Reaction / E14;
3. verificare se E14 corrisponde a un Epic GitHub reale oppure a un checkpoint interno;
4. riutilizzare il contenitore esistente.

Preferenza:

> integrare questo scope nell'Epic Reaction/Decision Window/Overwatch esistente.

Creare un nuovo Epic `Overwatch Runtime Lifecycle v0.1` SOLO se non esiste alcun Epic equivalente e la governance del repo lo richiede.

Non duplicare Epic.

---

# 39. ISSUE PLAN

Cercare prima equivalenti. Creare/aggiornare solo quelle mancanti.

Titoli suggeriti, da adattare alla naming convention reale:

```text
feat(turn): segment Move resolution into Watch and Reposition stages
feat(overwatch): implement runtime lifecycle and EndWatchStage expiration
feat(overwatch): evaluate voluntary Move triggers at deterministic micro-step boundaries
feat(overwatch): enforce once-per-target opportunity cadence
feat(overwatch): aggregate simultaneous targets into one opportunity
feat(overwatch): support same-boundary Reaction Batch commits
feat(overwatch): implement pre-Watch cancellation and soft-block rules
feat(move): add data-driven Overwatch Reposition movement profile
feat(move): resolve preplanned Reposition with no reservation or auto-reroute
feat(ui): preview Watch area and post-Watch Reposition separately
feat(net): sanitize Overwatch opportunities and prevent hidden HOLD/timing leaks
feat(log): add Overwatch lifecycle/reposition reason codes and TurnLog events

test(overwatch): add per-target HOLD and re-entry regression coverage
test(overwatch): add simultaneous target opportunity coverage
test(overwatch): add pre-Watch disruption lifecycle coverage
test(overwatch): add Watch-to-Reposition transition scenarios
test(overwatch): add Reposition collision and start-mismatch scenarios
test(net): add hidden Overwatch HOLD privacy canary coverage

docs(reaction): consolidate Overwatch Watch/Reposition lifecycle
docs(move): document Overwatch Reposition movement profile
docs(feature): link Overwatch lifecycle to registry roadmap and issues
docs(scenario): consolidate Overwatch runtime scenario coverage
docs(wiki): update Overwatch player-facing rules and lifecycle
```

Ogni issue deve avere acceptance criteria reali.

---

# 40. SCENARIO MAP

Prima riutilizzare gli scenari esistenti:

```text
REACT-001 — Overwatch Fire
REACT-002 — Hold Then Fire
REACT-003 — Simultaneous Targets
REACT-004 — Timeout
REACT-007 — Reaction Cancelled
REACT-009 — Privacy Canary
```

Non duplicare questi casi con nuovi ID se basta estenderli.

Aggiungere/consolidare copertura per:

## Trigger / cadence

```text
target starts outside -> enters -> Opportunity
target starts inside -> first voluntary Move -> Opportunity
target HOLD -> continues inside -> NO second Opportunity
target HOLD -> exits -> re-enters -> NO second Opportunity
stationary target inside area -> NO Opportunity
```

## Simultaneous

```text
two targets same micro-step
-> one opportunity
-> FIRE A / FIRE B / HOLD

HOLD
-> both consumed for that Reaction Instance
```

## Movement timing

```text
transition A -> B completes
-> FIRE
-> B remains current cell
-> interrupt, if any, affects only later micro-steps
```

## LOS / smoke

```text
post-transition LOS false
-> no Opportunity

post-transition LOS true
-> Opportunity
```

## Pre-Watch disruption

```text
Dash crosses future killzone -> no standard Overwatch trigger
Damage only -> preserve
Stun -> cancel
Disarm -> cancel
Forced Movement owner -> cancel
Suppression -> soft block
Smoke -> soft eligibility block
```

## Watch -> Reposition

```text
no trigger -> EndWatchStage -> Expired -> Reposition
FIRE early -> owner remains stationary until EndWatchStage -> Reposition
HOLD -> same
Overwatch cancelled -> no refund -> only planned Reposition
```

## Reposition

```text
destination occupied by Standard Move -> partial/blocked
two Reposition units same destination -> standard simultaneous collision policy
start cell mismatch -> Reposition invalidated
no auto-reroute
no hidden reservation
hazard traversal uses normal Move rules
```

## Standoff

```text
two opposing Overwatch users
no Standard Move trigger between them
EndWatchStage
both expire
both Reposition simultaneously
```

## Multiple Overwatch

```text
same target triggers two watchers same boundary
-> Reaction Batch
-> decisions not serialized artificially
```

## No nesting

```text
Overwatch effect causes displacement into another watched area
-> spatial events/log occur
-> no nested interactive Fast Reaction in MVP
```

Creare nuovi Scenario ID solo con lo standard reale del registry e dopo aver verificato quelli esistenti.

---

# 41. AUTOMATION / GOLDEN TEST

Minimo richiesto:

```text
same snapshot + same responses -> same StateHash / LogHash
permutation of unit storage order -> same result
one edge transition per micro-step
same target -> max one Opportunity per ReactionInstance
exit/re-entry -> no retrigger baseline
simultaneous targets -> one opportunity
multi-target HOLD -> all consumed
MaxPrompts counts opportunity sets
FIRE consumes one charge
timeout -> HOLD
post-transition LOS policy deterministic
Watch user remains stationary during Stage A
FIRE timing does not unlock Reposition early
all remaining Overwatch expire at EndWatchStage
Reposition starts only Stage B
Reposition partial path stops at last valid cell
start mismatch invalidates
no auto-reroute
no reservation
same-stage Reposition collision deterministic
no nested interactive reaction
hidden HOLD no canary leak
30/60/144 FPS playback does not change logical result
```

Se runtime non esiste ancora:

- creare test plan;
- creare issue;
- non creare fake tests che passano controllando solo costanti.

---

# 42. TURNLOG / REASON CODES

Consolidare eventi/reason code equivalenti a:

```text
OverwatchPrepared
ReactionArmed
OverwatchWindowOpened
OverwatchOpportunityCreated
ReactionHold
ReactionCommitted
ReactionResolved
ReactionExpired
ReactionCancelled
OverwatchWindowClosed

MoveStep
MovementInterrupted

OverwatchRepositionPlanned
OverwatchRepositionStarted
OverwatchRepositionStep
OverwatchRepositionBlocked
OverwatchRepositionInvalidated
OverwatchRepositionCompleted
```

Reason candidate:

```text
EndWatchStage
MaxPromptsReached
OwnerKO
OwnerStunned
OwnerKnockedDown
OwnerDisarmed
OwnerForcedMovement
OwnerForcedFacing
OriginMismatch
Suppressed
NoLOS
NotDetected
Occupied
TransitionInvalidated
StartCellMismatch
```

ATTENZIONE:

`NoLOS`, `NotDetected`, `Suppressed` possono essere eligibility/debug reason, non necessariamente terminal `ReactionCancelled` reason.

Non confondere soft block e cancel.

---

# 43. TELEMETRY

Aggiungere al piano telemetry:

```text
OverwatchSelected
OverwatchTriggered
OverwatchNeverTriggered
OverwatchCancelledBeforeWatch
OpportunitiesGenerated
UniqueTargetsOffered
HoldsBeforeCommit
SimultaneousTargetOpportunities
MaxPromptsExpired
OverwatchFired
OverwatchExpiredAtEndWatch

RepositionPlannedCells
RepositionActualCells
RepositionCompleted
RepositionPartial
RepositionBlockedBeforeFirstStep
RepositionStartMismatch
```

Metriche di playtest importanti:

```text
% Overwatch che non produce Opportunity
avg HOLD prima di FIRE
% target bait Tank/low priority lasciati passare
% Reposition completati
% Reposition bloccati/invalidati
tempo totale in Decision Window
prompt per turno / match
```

Usare i dati per decidere il budget Reposition e `MaxPrompts`.

---

# 44. WIKI

Aggiornare la Wiki player-facing.

Pagine/sezioni equivalenti:

```text
Generic Actions
Overwatch
Reactions / Fast Reactions
Move
Turn Phases
Facing
Planning / Action Ghost
```

La pagina Overwatch deve spiegare in modo semplice:

```text
1. scegli Overwatch nel Planning;
2. prepari settore/facing;
3. durante Watch resti fermo;
4. nemico in Move può creare FIRE/HOLD;
5. HOLD lascia passare quel bersaglio;
6. lo stesso bersaglio non riapre la decisione per la stessa Overwatch;
7. a fine Watch Overwatch termina;
8. esegui il Reposition limitato già pianificato;
9. il Reposition può essere bloccato perché avviene dopo i Move normali.
```

Non esporre internals server/privacy non utili al giocatore.

Valori `OPEN_BALANCE` non vanno presentati come definitivi.

---

# 45. DOCUMENTAZIONE TECNICA

Aggiornare soltanto le sezioni rilevanti di:

```text
Reaction System Master
Overwatch universal action spec
Turn sequence
Movement resolver / Move spec
Planning visual / Action Ghost
Deterministic simulation
Networking / privacy
UI/UX
Feature Registry
Scenario Map
Roadmap / checkpoint
Open Decisions
Conflict Matrix / Decision Log / ADR, se previsto
```

Non riscrivere tutti i PDR.

I vecchi documenti che dicono solo:

```text
Overwatch disarma prima del proprio normale Move
```

devono essere aggiornati alla nuova semantica:

```text
Watch Stage stationary
-> EndWatchStage
-> Overwatch termina
-> preplanned limited Reposition Stage
```

Se un documento storico va preservato, marcarlo `SUPERSEDED/HISTORICAL` o aggiungere redirect/provenance secondo la governance del repo.

---

# 46. FEATURE ↔ SCENARIO ↔ ISSUE ↔ WIKI

Dopo le modifiche verificare relazioni bidirezionali.

Ogni feature Overwatch rilevante deve poter rispondere a:

```text
Quale Epic/Issue la implementa?
Quale milestone/checkpoint la contiene?
Quale scenario la dimostra?
Quale Automation/Functional Test la verifica?
Quale pagina Wiki la spiega?
Qual è il suo gate/status corrente?
```

E ogni Scenario deve avere almeno un Feature owner.

---

# 47. DEFINITION OF DONE

Non segnare Overwatch `DONE` solo perché funziona in PIE.

Applicare i gate reali del progetto:

```text
spec
data
runtime
log/debug
automation
scenario
ui/wiki
packaged
network/privacy quando applicabile
```

Per la feature online:

```text
zero leak di intenti / hidden HOLD
```

Per determinismo:

```text
zero divergence StateHash / LogHash
```

---

# 48. VALIDAZIONE

Eseguire i validator reali del repository.

Controllare almeno:

```text
FeatureId duplicati
ScenarioId duplicati
link rotti
Issue/Epic mancanti
roadmap refs stale
Wiki backlinks
feature DONE senza gate
valori numerici duplicati in più source of truth
vecchi 5s interrupt ancora trattati come current
vecchio Mobile Overwatch ancora trattato come current
vecchio post-Overwatch Normal/Sneak Move non aggiornato
```

Se esistono generatori:

```text
feature-registry.yaml -> generated markdown/json
```

modificare la source of truth, non soltanto gli output generati.

---

# 49. COMMIT PLAN SUGGERITO

Adattare al workflow reale:

```text
docs(overwatch): consolidate watch and reposition lifecycle
docs(reaction): define per-target opportunity cadence and batch semantics
docs(move): define post-overwatch reposition profile and late priority

feat(turn): add watch and reposition stages to move resolution
feat(overwatch): add deterministic opportunity cadence and lifecycle
feat(move): add preplanned limited overwatch reposition

feat(ui): preview overwatch watch and reposition intents
feat(net): harden overwatch opportunity privacy boundaries

test(overwatch): add watch lifecycle and opportunity golden scenarios
test(move): add overwatch reposition collision and invalidation coverage

docs(project): link overwatch feature scenario roadmap wiki and issues
```

Preferire commit piccoli e focalizzati.

---

# 50. OUTPUT FINALE OBBLIGATORIO DI CLAUDE

Restituire un report con:

## A. Audit

```text
branch
HEAD
UE version
source-of-truth files
existing Overwatch/Reaction Epic
existing related issues
existing Feature IDs
existing Scenario IDs
```

## B. Conflict report

Tabella:

```text
Topic | Old state | New/current state | Action taken | Source
```

Includere almeno:

```text
Brace vs Overwatch
Mobile Overwatch
post-Overwatch Move policy
Dash trigger semantics
once-per-target cadence
reservation
auto-reroute
Watch/Reposition staging
```

## C. Files modified

Elenco e motivo.

## D. Feature Registry changes

```text
Feature ID
status
roadmap
issue/epic
scenario
wiki
dependencies
```

## E. Scenario Map changes

Elenco scenari riutilizzati, aggiornati e nuovi.

## F. GitHub

```text
Epic reused/created
Issues updated/created
real issue numbers
real URLs
milestones
relations
```

Non riportare numeri inventati.

## G. Tests

```text
command
result
pass/fail
```

## H. Validation

Validator eseguiti e risultati.

## I. Remaining OPEN decisions

Soltanto quelle ancora realmente aperte dopo l'audit.

## J. Suggested next step

Preferenza:

> chiudere runtime generico Overwatch + Watch/Reposition prima di implementare profili character-specific.

---

# 51. SINTESI DEL CANONE DA CONSOLIDARE

```text
OVERWATCH v0.1

Planning
--------
Choose Overwatch
Prepare Watch origin/facing/area
Prepare limited Reposition path
Prepare final facing


Resolution
----------
Prep / Dash / Blast
    can disrupt or alter eligibility

Move Stage A — WATCH
    Overwatch owner stationary
    Standard movers move by deterministic micro-steps

    voluntary Move transition intersects controlled geometry
    + post-transition LOS/detection valid
    + target not previously offered
        ->
    one Reaction Opportunity

    simultaneous targets
        ->
    one multi-target Opportunity

    FIRE
        -> commit / charge consumed

    HOLD
        -> current target set consumed
        -> wait for different target

    same target
        -> never re-prompts in same Reaction Instance


EndWatchStage
-------------
remaining Overwatch -> Expired
Overwatch no longer exists


Move Stage B — REPOSITION
-------------------------
preplanned limited Move
no reservation
no auto-reroute
late occupancy matters
simultaneous with other Reposition units
normal movement/hazard/collision rules


Core trade-off
--------------
Overwatch sacrifices:
1. offensive action;
2. full normal Move;
3. spatial priority.

In exchange it gains:
controlled reactive threat during enemy Standard Move.
```

---

# 52. ULTIMA REGOLA OPERATIVA

Non passare ancora ai profili specifici di Gadget/Phase/Riktor/Wraith finché:

```text
Watch Stage
Reposition Stage
Opportunity cadence
simultaneous target aggregation
lifecycle cancellation/soft block
privacy boundary
scenario coverage
```

non sono consolidati nella documentazione e nella roadmap.

Dopo il consolidamento, i profili personaggio devono essere payload/data sopra questo framework, non eccezioni strutturali al resolver.
