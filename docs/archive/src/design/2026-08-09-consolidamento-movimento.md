# REFACTORTACTICS — MOVE SYSTEM CONSOLIDATION PACK FOR CLAUDE

## Scopo

Questo documento deve essere usato da Claude come **brief operativo di consolidamento** per integrare nel repository di RefactorTactics le decisioni emerse durante il focus sulla skill/azione **Move** e sulla tassonomia generale del movimento.

Claude non deve limitarsi a copiare questo file nella documentazione. Deve:

1. analizzare la documentazione corrente del progetto;
2. individuare conflitti, duplicazioni e documenti superati;
3. consolidare le decisioni qui sotto nel posto canonico corretto;
4. aggiornare **Product Map**;
5. aggiornare **Feature Map**;
6. aggiornare **Scenario Map**;
7. aggiornare la **Roadmap**;
8. creare o aggiornare **Epic** e **Issue** necessarie;
9. aggiornare Wiki / feature pages / cross-link;
10. aggiornare test, acceptance criteria, debug e Definition of Done;
11. mantenere riferimenti incrociati tra feature, roadmap, scenari e test.

Non introdurre feature aggiuntive fuori dallo scope descritto, salvo eventuali issue esplicitamente marcate come **Future / Backlog**.

---

# 1. CONTESTO PROGETTO DA PRESERVARE

RefactorTactics è un tattico competitivo PC-first in Unreal Engine 5 con:

- turni simultanei;
- simulatore autorevole;
- snapshot immutabile;
- resolution deterministica;
- stato logico separato dalla presentazione;
- client che propone, server che valida e applica;
- mappa come grafo tattico 3D;
- informazioni di planning private per squadra;
- `TurnLog` canonico;
- gameplay data-driven;
- Gameplay Tags governati;
- C++ per regole competitive, resolver, rete e validazione;
- Blueprint/Data Assets per configurazione, UI, animazioni, VFX e varianti.

Baseline documentale corrente: **Unreal Engine 5.8**, salvo diversa patch già bloccata nel repository.

---

# 2. ORDINE CANONICO DEL TURNO

Preservare come vincolo di design:

```text
Decision / Planning
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

Regola fondamentale:

> **Move è sempre l'ultima fase di movimento volontario standard del turno.**

Non progettare sequenze arbitrarie come:

```text
Move → Attack
```

Una unità può invece fare:

```text
Dash → Blast/Attack → Move
```

se il proprio kit e il ruleset lo consentono.

Dash non è Move più veloce.

---

# 3. DEFINIZIONE CANONICA DI MOVE

Definizione consigliata per Wiki / feature registry:

> **Move è il riposizionamento volontario standard di un'unità. Viene pianificato insieme alle altre azioni ma risolto sempre nell'ultima fase del turno. Il giocatore sceglie percorso, destinazione e orientamento finale. Il movimento viene eseguito per micro-step e può interagire con terreno, occupazione, visibilità, rumore, hazard e reaction.**

Move non deve essere modellato come una normale abilità che compete necessariamente con lo slot principale dell'azione.

Move rappresenta il movimento volontario standard disponibile alla unità salvo:

- status;
- Brace;
- regole di scenario;
- skill;
- immobilizzazione;
- effetti espliciti.

---

# 4. INPUT DEL MOVE

Durante Planning il Move deve contenere almeno:

```text
Move
- Path
- Destination
- FinalFacing / FacingPolicy
- MovementProfile
```

Esempio:

```text
MOVE

Start:       H4
Path:        H4 → H5 → I5 → J5
Destination: J5
Facing:      NE

Cost:        3 / 4
Profile:     Normal
```

Il sistema non deve ridurre Move a:

```text
Start + Destination
```

perché il percorso attraversato è gameplay.

---

# 5. PATH: A* PROPONE, IL GIOCATORE DECIDE

L'A* può suggerire il percorso migliore secondo la query policy attiva.

Il giocatore deve però poter modificare il path.

Motivazione:

due percorsi verso la stessa destinazione possono differire per:

- cover;
- acqua;
- ghiaccio;
- fuoco;
- elettricità;
- rumore;
- choke point;
- Overwatch;
- trappole;
- LOS;
- facing finale;
- porte;
- ponti;
- quota;
- tunnel;
- rischio.

UX suggerita:

```text
Click destination
→ auto-path

Drag / waypoint
→ manual path

Reset
→ recompute recommended path
```

Principio:

> A* propone. Il giocatore decide.

---

# 6. MOVE VIENE RISOLTO PER MICRO-STEP

Non trattare:

```text
A7 → D7
```

come teleport logico.

Risoluzione:

```text
Micro-step 1
A7 → B7

Micro-step 2
B7 → C7

Micro-step 3
C7 → D7
```

Ogni transizione può generare eventi.

Possibili eventi:

```text
MoveStarted
MoveStep
EdgeCrossed
UnitEnteredCell
UnitLeftCell
SurfaceEntered
SurfaceExited
NoiseGenerated
VisibilityChanged
DetectionChanged
HazardTriggered
ReactionOpportunity
MoveBlocked
MovementInterrupted
MoveEnded
```

I nomi finali devono seguire le convenzioni del repository.

---

# 7. PROFILI DI MOVE

Move può avere profili senza creare sistemi separati.

Baseline:

```text
Move
├── Sneak
├── Normal
└── Sprint
```

Sono varianti del Move standard.

Esempio qualitativo:

| Profilo | Range / Budget | Rumore | Stabilità | Pivot |
|---|---|---|---|---|
| Sneak | ridotto | molto basso | alta | normale |
| Normal | standard | standard | standard | standard |
| Sprint | maggiore | alto | ridotta | potenzialmente ridotto |

I numeri reali sono data-driven e devono essere bilanciati con scenario/playtest.

IMPORTANTE:

> **Sprint non è Dash.**

Sprint è un profilo della fase Move.

---

# 8. MODELLO COSTI DI MOVE

Il budget non equivale necessariamente al numero di celle.

Esempio:

```text
MoveBudget = 5
```

significa:

> può spendere fino a 5 unità di costo fisico.

Esempio indicativo:

| Terreno | Physical Move Cost |
|---|---:|
| floor | 1 |
| grass | 1 |
| shallow water | 2 |
| rubble | 2 |
| mud | 2 |
| ice | 1 + surface effect |
| stairs | transition-specific |
| open door | transition-specific |
| closed door | blocked / interaction |
| allowed drop | transition-specific |

Separare:

```text
PhysicalCost
TacticalPreference
RiskCost
Validity
```

Il resolver competitivo usa le regole fisiche/validità autorevoli.

UI e bot possono usare preferenze e rischio senza cambiare la legalità.

---

# 9. TASSOMIA CANONICA DEL MOVIMENTO

Non usare un unico concetto generico indistinguibile.

Famiglie minime:

```text
Move
Dash
Forced Movement
Teleport / Blink
Reaction Movement
```

`Reaction Movement` può tecnicamente usare policy di una delle famiglie sopra, ma la causa deve restare identificabile.

---

# 10. MATRICE CANONICA MOVE / DASH / FORCED / TELEPORT

Usare questa matrice come riferimento iniziale.

| Regola | Move | Dash | Forced Movement | Teleport |
|---|---|---|---|---|
| volontario | sì | sì | no | normalmente sì |
| fase | Move | Dash | fase dell'effetto | fase dell'abilità |
| micro-step | sì | normalmente sì | sì | no |
| attraversa celle intermedie | sì | policy | sì | no |
| attraversa archi | sì | policy | sì | no |
| usa MoveBudget | sì | no | no | no |
| usa costi terreno | sì | DashPolicy | no come budget | no intermedi |
| collisioni | sì | DashPolicy | sì | landing |
| hazard intermedi | sì | normalmente sì | sì | no |
| spatial triggers | sì | sì | sì | landing |
| `EnemyUsesMove` | sì | no | no | no |
| `EnemyUsesDash` | no | sì | no | ability/profile |
| facing | Move policy | Dash policy | effect policy | effect policy |
| rumore | profile | profile | event-based | ability-based |
| auto-reroute | no | no | no | n/a |
| consuma movimento vittima | n/a | n/a | mai | n/a |

Questa matrice deve diventare documentazione canonica nel Movement System.

---

# 11. DASH

Dash è movimento volontario speciale nella fase Dash.

NON:

> Move ma più lungo.

Dash può essere definito mediante un profilo:

```text
DashProfile
- Shape
- Range
- AllowedTransitions
- CollisionPolicy
- TerrainPolicy
- FacingPolicy
- ReactionProfile
- StopPolicy
```

Esempi di famiglie future:

```text
Dash
Charge
Leap
Slide
Phase Dash
Grapple Movement
```

Esempio:

```text
Dash
→ Attack
→ Move
```

può essere legale.

---

# 12. DASH E CHARGE NON SONO IDENTICI

Esempio Heavy Charge:

```text
- linea retta;
- collisione significativa;
- rumore alto;
- facing bloccato o limitato;
- possibile push;
- stop on impact.
```

Esempio Agile Dash:

```text
- può curvare;
- può attraversare alleati;
- ampia libertà di facing;
- può evitare determinate zone;
- nessun impact automatico.
```

Stessa macrofase.

Policy differenti.

---

# 13. FORCED MOVEMENT

Forced Movement comprende:

```text
Push
Pull
Knockback
Knockaside
Drag
Throw
Current
Conveyor
Explosion displacement
Reaction displacement
```

Regola fondamentale:

> **Forced Movement non è un'azione della unità spostata.**

Non consuma:

- Move;
- Dash;
- l'azione della vittima;
- il MoveBudget della vittima.

La causa deve essere registrata.

---

# 14. TELEPORT / BLINK

Teleport non attraversa logicamente le celle intermedie.

Esempio:

```text
A
↓
Teleport
↓
D
```

Non:

```text
A → B → C → D
```

Quindi normalmente:

```text
CrossEdge(B,C)       no
EnterCell(B)         no
EnterCell(C)         no
Hazard(B)            no
Hazard(C)            no
Tripwire(B,C)        no
```

ma:

```text
ArriveCell(D)        sì
Occupancy(D)         sì
HazardOnArrival(D)   sì
VisibilityUpdate(D)  sì
EnemyEnterArea(D)    sì
```

Le ability possono esplicitamente modificare questa policy.

---

# 15. TRIGGER GEOGRAFICI VS SEMANTICI

Formalizzare questa distinzione.

## Trigger geografici

Descrivono cosa è successo nello spazio:

```text
EnemyEnterArea
EnemyEnterCell
EnemyLeaveArea
EnemyCrossEdge
LeaveAdjacency
OccupancyChanged
```

## Trigger semantici

Descrivono come è avvenuto:

```text
EnemyUsesMove
EnemyUsesSprint
EnemyUsesDash
EnemyIsForced
EnemyTeleports
```

Questi due insiemi devono poter essere combinati.

Esempio:

```text
WHEN EnemyCrossEdge(E12 → E13)
AND MovementKind == Dash
```

→ Anti-Dash Trap

Esempio:

```text
WHEN EnemyEnterArea
MovementKind = ANY
```

→ zona generica di controllo.

Questo modello deve essere integrato nel Reaction System e nella feature map.

---

# 16. OVERWATCH E MOVEMENT

Overwatch standard deve reagire principalmente a una condizione spaziale valida, non solo a `EnemyUsesMove`.

Baseline:

```text
Enemy enters controlled area
AND
LOS valid
AND
Detected
AND
Reaction armed
```

Possibili sorgenti dello spostamento:

```text
Move            → può attivare
Dash            → può attivare
Forced Movement → può attivare
Teleport        → può attivare all'arrivo
```

Questo consente combo:

```text
Riva Push
→ enemy enters ally controlled area
→ Overwatch Reaction Opportunity
```

Preservare il modello già definito:

```text
Reaction Definition
+
Player Intent
+
Snapshot
+
Reaction Opportunities
+
Commit opzionale
=
Reaction Resolution
```

---

# 17. RUMORE E MOVEMENT

Rumore e movement devono restare integrati.

Move:

```text
MovementNoise
+
SurfaceModifier
```

Sprint:

```text
High MovementNoise
+
SurfaceModifier
```

Dash:

```text
DashNoise
+
SurfaceModifier
+
PossibleImpactNoise
```

Forced Movement:

```text
Noise.Displacement
Noise.Surface.*
Noise.Impact
```

Teleport:

```text
NoFootsteps
+
Optional Ability Noise
```

Gli eventi di rumore sono prodotti dalla simulazione e poi passano al sistema Perception / Team Knowledge.

Non usare l'intento nemico per warning acustici.

---

# 18. FACING

Facing è stato gameplay autorevole.

Non è solo animazione.

Il Movement System deve supportare almeno:

```text
MoveFacingPolicy
DashFacingPolicy
ForcedMovementFacingPolicy
TeleportFacingPolicy
```

Per Forced Movement, possibili policy:

```text
PreserveFacing
FaceMovementDirection
FaceSource
FaceAwayFromSource
RotateSteps
SetFacing
```

Per Move è emersa una proposta importante:

> il facing naturale può essere derivato dall'ultima transizione del path, poi modificato entro un limite di pivot del personaggio.

Esempio su esagoni:

```text
Final incoming direction = SE
PostMoveTurnSteps = 1
```

consente:

```text
E
SE
S
```

ma non un 180° gratuito.

QUESTO NON È ANCORA DA BLOCCARE COME REGOLA UNIVERSALE.

Deve essere marcato come:

```text
Design candidate
Needs scenario + playtest
```

---

# 19. COLLISIONI — BASELINE V0.1

## Move contro unità ferma

```text
A → [B occupied]
```

→ stop / `MoveBlocked.Occupied`

## Due unità verso stessa cella

```text
A → [X] ← B
```

Baseline:

```text
A blocked
B blocked
```

Motivo:

- nessuna iniziativa nascosta;
- leggibilità;
- determinismo;
- semplicità.

## Swap simultaneo

```text
[A][B]

A → B
B → A
```

Baseline:

```text
both blocked
```

Varianti future possibili:

```text
AllySwap
Phase
AgilePass
```

come capability esplicite.

---

# 20. DASH COLLISION

Default:

```text
Dash hits occupied cell
→ Dash stops
```

Policy data-driven possibili:

```text
Stop
Push
Damage
Bounce
PassThrough
Swap
```

Non implementare un resolver custom per ogni personaggio se può essere espresso tramite policy.

---

# 21. FORCED MOVEMENT COLLISION

Baseline:

```text
Push A → occupied cell
```

→ A si ferma prima della cella occupata.

Non introdurre ancora push-chain automatico come comportamento globale.

Feature futura:

```text
CanChainPush
```

solo dopo test specifici.

---

# 22. HAZARD E FORCED MOVEMENT

Se una unità viene spinta attraverso:

```text
Dry → Fire → Fire → Dry
```

ha realmente attraversato quelle celle.

Generare eventi:

```text
ForcedMoveStep
SurfaceEntered.Fire
HazardTriggered
```

Lo stesso vale per:

- Water;
- Ice;
- Electrified Water;
- Trap;
- Mine;
- Door;
- Bridge;
- environmental volume logico.

Forced Movement ignora il **costo volontario** del terreno, non la geometria logica né gli hazard.

---

# 23. PATH INVALIDATO DURANTE MOVE

Decisione consolidata:

> **Mai auto-reroute durante la Resolution.**

Piano:

```text
A → B → C → D
```

Se dopo B:

```text
B → C invalid
```

risultato:

```text
A → B
MoveBlocked
Reason = TransitionInvalidated
```

NON:

```text
B → E → F → D
```

Motivazione:

un reroute automatico potrebbe cambiare:

- Overwatch exposure;
- rumore;
- hazard;
- LOS;
- facing;
- prediction avversaria;
- traiettoria;
- rischio.

Il computer non deve inventare un nuovo piano del giocatore.

---

# 24. FORCED DISPLACEMENT DURANTE MOVE

Se durante un Move già in corso una Reaction o un effetto sposta la unità:

```text
A → B → C → D

at B:
Reaction
→ Push B → X
```

baseline proposta:

```text
MovementInterrupted
Reason = ForcedDisplacement
```

Non tentare automaticamente di tornare verso C.

Questa regola deve entrare negli scenari di validazione.

---

# 25. OPEN DESIGN QUESTION — DISPLACEMENT PRIMA DEL MOVE

Caso:

```text
Planning:
Move H5 → H6 → H7

Blast:
Push H5 → G5

Move Phase begins
```

Sono emersi tre modelli.

## Model A — Cancel Move

```text
ExpectedOrigin != ActualOrigin
→ Move invalidated
```

PRO:
- semplice;
- leggibile;
- deterministico;
- forte valore ai displacement.

CONTRO:
- anche un push minimo può annullare un Move intero.

## Model B — Repath to destination

```text
Current = G5
Goal = H7
→ recompute
```

PRO:
- preserva il goal.

CONTRO:
- il computer crea un nuovo percorso;
- può attraversare pericoli non scelti;
- rompe il principio "A* propone, player decides".

NON consigliato come default.

## Model C — Preserve movement vector/steps

Esempio planned:

```text
NE
NE
E
```

dopo displacement:

```text
execute NE
NE
E from new origin
```

PRO:
- preserva la sequenza;
- interessante nei turni simultanei.

CONTRO:
- complesso con:
  - porte;
  - layer;
  - scale;
  - ascensori;
  - archi speciali.

### Decisione per prototipo

Testare prima **Model A**.

Se troppo punitivo, testare **Model C** prima di Model B.

NON marcare ancora come regola finale senza scenario dedicato.

Creare issue / experiment esplicito.

---

# 26. MOVE E BRACE

Move e Brace devono essere compatibili con il Reaction System e con il concetto di commitment.

Interpretazione:

```text
Move
→ rinuncio alla posizione attuale per riposizionarmi

Brace
→ privilegio stabilità / preparazione / reazione
```

Possibili regole:

```text
Brace → Move disabled
```

oppure:

```text
Brace → MoveBudget reduced
```

oppure:

```text
Move → Brace state removed
```

Personaggi futuri possono avere:

```text
Mobile Brace
```

ma deve essere una eccezione esplicita del kit.

Non bloccare ancora i numeri.

---

# 27. DIFFERENZIAZIONE DEI PERSONAGGI V0.1

Roster corrente da considerare per il progetto:

```text
v0.1
- Flux
- Riva
- Bastion
- Vektor
```

Move deve essere una azione comune, ma non necessariamente identica nel risultato tattico.

## Flux

Possibile identità:

```text
MoveBudget: medio
Pivot: medio
Movement priority: setup linee / nodi / elettricità
```

## Riva

Possibile specializzazione:

```text
movement advantage on water
```

L'acqua creata può diventare anche una rete di mobilità.

## Bastion

Possibile identità:

```text
MoveBudget: basso/medio
Pivot: limitato
High displacement resistance
```

Il facing finale può essere molto importante.

## Vektor

Possibile identità:

```text
MoveBudget: alto
Pivot: alto
Strong trajectory / interception interaction
```

Questi valori sono proposte di direzione design, NON numeri definitivi.

---

# 28. PIPELINE DI MICRO-STEP PROPOSTA

Durante Move:

```text
1. Collect intended transitions
2. Validate edges
3. Resolve simultaneous occupancy conflicts
4. Apply accepted transitions
5. Update occupancy
6. Resolve surface / hazard
7. Update facing
8. Update visibility / detection
9. Generate noise
10. Emit movement events
11. Evaluate triggers
12. Create Reaction Opportunities
13. Resolve committed reactions
14. Apply reactions
15. Revalidate surviving movers
16. Next micro-step
```

Verificare questa sequenza contro l'Action Resolver e la pipeline canonica già esistente.

Se la documentazione corrente usa ordine differente, non sovrascriverlo alla cieca:
- identificare il conflitto;
- scegliere una sola fonte canonica;
- aggiornare riferimenti;
- documentare la migrazione.

---

# 29. EVENT MODEL

Evitare resolver completamente isolati.

Concetto comune:

```text
MovementTransition
{
    UnitId
    From
    To

    MovementKind
    MovementProfile

    CauseActionId
    SourceUnitId

    Phase
    MicroStep

    IsVoluntary
}
```

`MovementKind` almeno:

```text
Move
Dash
Forced
Teleport
ReactionMovement
```

Eventi derivati possibili:

```text
MoveStarted
MoveStep
DashStep
ForcedMoveStep
TeleportArrived

EdgeCrossed
UnitEnteredCell
UnitLeftCell

MovementBlocked
MovementInterrupted
MovementCompleted
```

I nomi devono essere adattati alle convenzioni C++ / Gameplay Tag correnti.

---

# 30. GAMEPLAY TAGS DA VALUTARE

Integrare solo dopo verifica del dizionario governato.

Possibile tassonomia:

```text
Movement.Move
Movement.Move.Sneak
Movement.Move.Normal
Movement.Move.Sprint

Movement.Dash
Movement.Forced
Movement.Teleport
Movement.Reaction

Event.Movement.Started
Event.Movement.Step
Event.Movement.Blocked
Event.Movement.Interrupted
Event.Movement.Completed

Event.Spatial.EnterCell
Event.Spatial.LeaveCell
Event.Spatial.CrossEdge
Event.Spatial.EnterArea
Event.Spatial.LeaveArea
```

Non creare tag duplicati se il repository ne contiene già equivalenti.

---

# 31. UI / UX

Move Planning deve mostrare:

- reachable cells;
- selected path;
- cost;
- destination;
- final facing;
- surface / transition warnings;
- hazard known;
- allied path conflicts;
- known/public threats;
- own team intents.

Usare la classificazione:

```text
Confermato
Previsto
Incerto
```

Esempi:

```text
Path legalmente percorribile
→ Confermato

Possible collision with ally plan
→ Previsto

Possible enemy interference
→ Incerto
```

IMPORTANTISSIMO:

> I warning non devono mai essere derivati dagli intenti privati degli avversari.

---

# 32. INTEGRAZIONE CON NETWORKING / PRIVACY

Durante Planning:

- path e destinazione del giocatore possono essere condivisi con la propria squadra;
- preview team-only a 8–12 Hz;
- sequence number;
- DTO sanitizzato;
- no global replicated actor contenente intenti.

Durante Resolution:

- il risultato autorevole è pubblico secondo le regole di visibilità/perception;
- Reaction Opportunity deve essere sanitizzata;
- non inviare future posizioni / futuri trigger / path nemici.

Il Movement System deve essere compatibile con:

```text
CanonicalIntentStore
Team Relay
Snapshot Builder
Action Resolver
TurnLog
```

---

# 33. FEATURE INTERACTIONS DA REGISTRARE NELLA FEATURE MAP

Creare dipendenze esplicite tra Movement e:

```text
Map Graph
Pathfinding
Facing
Cover
LOS
Targeting
Noise
Perception
Fog of War
Reaction System
Overwatch
Brace
Hazards
Water
Fire
Electricity
Ice
Doors
Bridges
Tunnels
Elevators
Collision
Turn Resolver
TurnLog
Team Intent Preview
UI Planning
Bots
Scenario System
Replay
```

Non trattare Movement come feature isolata.

---

# 34. SCENARIO MAP — NUOVI SCENARI DA CREARE

Creare scenari separati, minimali e riproducibili.

Naming adattabile alle convenzioni correnti.

## MOV-001 — Basic Move Path

Verifica:

- path valido;
- costo;
- destination;
- TurnLog;
- final position.

## MOV-002 — Manual Path Override

Due route con stesso goal.

Verifica:

- A* propone route A;
- player imposta route B;
- resolver usa route B.

## MOV-003 — Terrain Cost

Percorsi con:

```text
floor
water
rubble
```

Verifica cost budget.

## MOV-004 — Contested Destination

```text
A → X ← B
```

Atteso:

```text
both blocked
```

## MOV-005 — Swap Blocked

```text
A ↔ B
```

Atteso:

```text
both blocked
```

## MOV-006 — Path Invalidated

```text
A → B → C
```

C diventa irraggiungibile.

Atteso:

```text
stop at B
no reroute
```

## MOV-007 — Forced Displacement Mid-Move

```text
Move A → B → C
Reaction at B
Push → X
```

Atteso:

```text
MovementInterrupted.ForcedDisplacement
```

## MOV-008 — Forced Displacement Before Move

Test A/B/C policy.

Per v0.1:

```text
Model A baseline
```

ma scenario deve supportire confronto futuro con Model C.

## MOV-009 — Move Through Hazard

Verifica ogni micro-step.

## MOV-010 — Push Through Hazard

Verifica ForcedMoveStep + hazard.

## MOV-011 — Move Generates Noise

Superfici differenti.

## MOV-012 — Teleport Does Not Cross Intermediate Edges

Verifica:

- no tripwire intermedi;
- landing trigger sì.

## MOV-013 — Overwatch Triggered by Move

Movement enters controlled area.

## MOV-014 — Overwatch Triggered by Dash

Dash enters controlled area.

## MOV-015 — Overwatch Triggered by Forced Movement

Push enters controlled area.

## MOV-016 — Anti-Dash Trigger

Move non triggera.
Dash triggera.

## MOV-017 — Tripwire CrossEdge

Move / Dash / Forced trigger.
Teleport non triggera.

## MOV-018 — Facing After Move

Scenario di prototipo per:

```text
incoming direction + pivot allowance
```

NON usarlo ancora come golden rule finché il design non è approvato.

## MOV-019 — Sprint vs Dash

Mostrare chiaramente:

```text
Sprint = Move profile
Dash = Dash phase action
```

## MOV-020 — Move + Brace Interaction

Scenario per confrontare policy candidate.

---

# 35. AUTOMATION TEST DA AGGIUNGERE

Creare o aggiornare test per:

```text
Movement path exact execution
No automatic reroute
Deterministic simultaneous collision
Swap blocked
Terrain cost
Forced movement ignores MoveBudget
Forced movement triggers hazards
Teleport skips intermediate cells
MovementKind preserved in events
Movement event ordering deterministic
Move → reaction → interruption
Same snapshot + intents → same log/hash
Path adjacency permutation → same result
```

Se il repository usa golden fixtures:

```text
Snapshot
+
AcceptedIntents
+
RulesVersion
+
ResolverConfig
=
Expected TurnLog
+
Expected StateHash
```

---

# 36. DEBUG / TELEMETRY

Aggiungere strumenti coerenti con la diagnostica esistente.

Esempi:

```text
rt.Move.DebugPath
rt.Move.DebugTransitions
rt.Move.DebugConflicts
rt.Move.DumpIntent
rt.Move.DumpEvents
```

Usare nomi già presenti se esistono.

TurnLog reason codes minimi:

```text
Occupied
ContestedDestination
SwapNotAllowed
TransitionInvalidated
ForcedDisplacement
InsufficientBudget
BlockedByTerrain
BlockedByEdge
InvalidDestination
InvalidOrigin
```

---

# 37. PRODUCT MAP — AGGIORNAMENTO RICHIESTO

Individuare il file canonico Product Map e aggiungere / consolidare:

```text
Core Tactical Loop
└── Movement System
    ├── Move
    │   ├── Path planning
    │   ├── Manual route
    │   ├── Move profiles
    │   │   ├── Sneak
    │   │   ├── Normal
    │   │   └── Sprint
    │   ├── Terrain cost
    │   ├── Facing
    │   └── Micro-step resolution
    │
    ├── Dash
    ├── Forced Movement
    ├── Teleport
    ├── Collision
    ├── Spatial Triggers
    └── Movement / Reaction integration
```

Mostrare chiaramente cosa è:

```text
v0.1
v0.2
future
```

Non spostare nello scope v0.1 feature non approvate.

---

# 38. FEATURE MAP — AGGIORNAMENTO RICHIESTO

Creare un nodo principale:

```text
FEAT-MOVEMENT
```

con sotto-feature o equivalenti:

```text
FEAT-MOVE-STANDARD
FEAT-MOVE-PROFILES
FEAT-MOVE-PATH-OVERRIDE
FEAT-MOVE-TERRAIN-COST
FEAT-MOVE-FACING
FEAT-MOVE-COLLISION
FEAT-DASH
FEAT-FORCED-MOVEMENT
FEAT-TELEPORT
FEAT-SPATIAL-TRIGGERS
FEAT-MOVEMENT-REACTIONS
FEAT-MOVEMENT-NOISE
```

Collegare ogni feature a:

```text
Status
Milestone
Epic
Issues
Scenarios
Tests
Wiki page
Dependencies
```

---

# 39. SCENARIO MAP — AGGIORNAMENTO RICHIESTO

Ogni scenario Movement deve essere indicizzato per:

```text
Feature
Mechanic
Character
Environment
Phase
Expected Event
Automation status
Playable status
```

Esempio:

```text
MOV-013
Feature: Movement + Overwatch
Phase: Move
Character: any
Environment: open / choke
Expected: ReactionOpportunity
Automation: required
Playable: required
```

Creare cross-link dalle Wiki page alle scene di dimostrazione.

Obiettivo:

> cliccare una feature nella Wiki e poter sapere quale scenario la dimostra e in quale milestone viene completata.

---

# 40. ROADMAP — AGGIORNAMENTO

Preservare la macro-roadmap:

```text
F0 Fondazioni
F1 Rete privata
F2 Abilities
F3 Mappa multilivello
F4 Vertical Slice
F5 Dedicated Server
F6 Beta Systems
```

Movimento deve essere distribuito senza anticipare scope.

## F0 — Fondazioni

Deve includere almeno:

```text
FRTCellId
Map graph
A*
Move intent
Move path
Manual route override if feasible
MoveBudget
Micro-step movement
Collision baseline
No auto-reroute
Snapshot
TurnLog
Automation tests
```

## F1 — Network

```text
team-only Move preview
sequence
server validation
privacy tests
```

## F2 — Abilities

```text
Dash
Forced Movement caused by abilities
Teleport
Reaction integration
Brace interaction
```

## F3 — Multilayer

```text
stairs
ramps
bridges
doors
tunnels
elevators
special transition policies
```

## F4 — Vertical Slice

```text
full character differences
noise/perception integration
Overwatch / Reaction
scenario suite
UI explainability
```

---

# 41. EPIC DA CREARE O CONSOLIDARE

Prima cercare se esiste già un Epic Movement.

Se esiste:
- consolidare;
- non duplicare.

Se non esiste creare:

```text
EPIC — Tactical Movement System
```

## Goal

Implementare il sistema unificato di movement autorevole per:

```text
Move
Dash
Forced Movement
Teleport
Spatial Triggers
Collision
Facing
Terrain
Reaction integration
```

## Success criteria

- deterministic;
- data-driven;
- TurnLog explainable;
- no auto-reroute;
- network-safe;
- scenario coverage;
- automation tests;
- packaged verification.

---

# 42. ISSUE PROPOSTE

Non duplicare issue esistenti.

Creare / aggiornare almeno:

## MOV-CORE-01 — Define canonical MovementKind model

Acceptance:
- Move/Dash/Forced/Teleport distinguishable;
- semantic cause preserved in events.

## MOV-CORE-02 — Implement MoveIntent path + destination

Acceptance:
- exact path stored;
- validated against graph/revision;
- destination derived/validated.

## MOV-CORE-03 — Add manual path override

Acceptance:
- suggested A* path can be replaced by legal player path.

## MOV-CORE-04 — Implement MoveBudget and physical costs

Acceptance:
- integer/fixed costs;
- terrain aware;
- deterministic.

## MOV-CORE-05 — Implement micro-step Move resolver

Acceptance:
- one logical transition per micro-step;
- TurnLog events;
- no frame dependency.

## MOV-CORE-06 — Implement simultaneous collision baseline

Acceptance:
- contested destination blocks both;
- swap blocks both;
- deterministic permutation tests.

## MOV-CORE-07 — Prevent auto-reroute during resolution

Acceptance:
- invalid edge stops move;
- reason code logged.

## MOV-CORE-08 — Implement Forced Movement core

Acceptance:
- no victim MoveBudget consumption;
- traversed hazards trigger;
- movement source recorded.

## MOV-CORE-09 — Implement Teleport movement semantics

Acceptance:
- no intermediate edge/cell events;
- landing events valid.

## MOV-CORE-10 — Spatial movement trigger model

Acceptance:
- geographic vs semantic triggers;
- Move/Dash/Forced/Teleport filters.

## MOV-CORE-11 — Movement / Reaction integration

Acceptance:
- Overwatch can react at micro-step boundary;
- forced movement may generate opportunity.

## MOV-CORE-12 — Movement / Noise integration

Acceptance:
- movement profile + surface modifier;
- no planning leak.

## MOV-CORE-13 — Facing movement policy prototype

Acceptance:
- prototype final incoming direction + pivot;
- scenario;
- no ranked rule lock yet.

## MOV-CORE-14 — Pre-Move displacement policy experiment

Acceptance:
- implement scenario supporting Model A;
- compare Model A vs Model C;
- record playtest decision.

## MOV-NET-01 — Team-only movement preview

Acceptance:
- 8–12 Hz;
- sanitized DTO;
- no enemy leak;
- stale sequence rejection.

## MOV-TEST-01 — Movement golden test suite

Acceptance:
- deterministic log/hash;
- collision;
- hazards;
- teleport;
- reactions;
- invalidation.

## MOV-UI-01 — Move planning visualization

Acceptance:
- reachable cells;
- path;
- cost;
- final facing;
- Confirmed / Predicted / Uncertain;
- ally conflict warning.

---

# 43. ISSUE PRIORITY

Suggerimento:

```text
P0
MOV-CORE-01
MOV-CORE-02
MOV-CORE-04
MOV-CORE-05
MOV-CORE-06
MOV-CORE-07
MOV-TEST-01

P1
MOV-CORE-03
MOV-UI-01
MOV-NET-01

P2
MOV-CORE-08
MOV-CORE-09
MOV-CORE-10
MOV-CORE-11
MOV-CORE-12

Experiment
MOV-CORE-13
MOV-CORE-14
```

Adattare alle etichette reali del repository.

---

# 44. FEATURE STATUS

Per ogni feature usare lo status model già presente nel progetto.

Se manca, non inventare un nuovo sistema parallelo.

Possibili stati concettuali:

```text
Proposed
Defined
In Roadmap
In Progress
Implemented
Validated
Done
Deferred
```

Una feature è **Done** soltanto se:

- funziona nel modello server/client previsto;
- non espone informazioni non autorizzate;
- produce log/debug;
- possiede test automatico;
- è verificata in packaged build;
- Wiki / Feature Map / Roadmap / Scenario Map sono allineati.

---

# 45. DOCUMENTI DA CONTROLLARE / CONSOLIDARE

Claude deve cercare nel repository i documenti canonici e aggiornare almeno le aree corrispondenti a:

```text
PDR-03 — Architettura UE5
PDR-04 — Networking / privacy
PDR-05 — Simulazione deterministica
PDR-06 — Mappa / pathfinding
PDR-07 — Abilità / personaggi / GAS
PDR-08 — UI / UX / coordinazione
PDR-09 — Dati / validazione
PDR-10 — Roadmap / QA / rischi

Reaction System
Overwatch / Fast Reaction
Noise / Perception
Brace
Basic Actions
Facing
Character definitions
Scenario framework
Feature Registry
Product Map
Feature Map
Scenario Map
Wiki
Roadmap
GitHub epics/issues
```

NON presumere che i nomi fisici siano esattamente questi.

Prima fare discovery.

---

# 46. CONSOLIDAMENTO DOCUMENTALE

Procedura richiesta:

```text
1. Discover canonical docs
2. Detect duplicates
3. Detect contradictions
4. Mark newest explicit project decisions
5. Merge into canonical docs
6. Remove / redirect stale duplicates if project conventions allow
7. Update cross-links
8. Update feature registry
9. Update roadmap
10. Update scenarios
11. Update tests
12. Create/update GitHub epic/issues
13. Produce change report
```

Non creare una nuova "Movement spec" isolata se esiste già un documento canonico adatto.

È accettabile creare un Movement document canonico se la struttura corrente ne ha chiaramente bisogno, purché venga registrato nella navigazione/docs index.

---

# 47. CONTRADDIZIONI DA CERCARE

Cercare esplicitamente documentazione vecchia che afferma:

```text
Move before Attack
Move and Dash synonymous
movement resolved only at destination
auto-reroute during resolution
all movement triggers identical
Teleport crosses intermediate cells
Forced movement consumes victim movement
enemy planning used for movement warnings
```

Se trovate:

- aggiornare;
- documentare la modifica;
- evitare due versioni concorrenti della regola.

---

# 48. DATI / DATA ASSET

Non hardcodare numeri per character o terrain.

La struttura dati deve poter esprimere almeno:

```text
MovementProfileId
MoveBudget
MovementKind
TerrainCostPolicy
CollisionPolicy
FacingPolicy
ReactionPolicy
NoiseProfile
AllowedTransitionTags
```

Non introdurre tutto subito nel runtime se non necessario.

Per F0 implementare solo i campi minimi richiesti dal vertical slice.

---

# 49. VALIDATOR

Aggiungere o pianificare validator per:

```text
negative movement budget
invalid movement profile ID
unknown movement tag
path containing non-adjacent cells
path through invalid edge
path cost over budget
illegal teleport destination
Dash definition missing policy
Forced movement invalid policy
unknown collision policy
invalid facing policy
```

---

# 50. BOT

Non implementare AI complessa ora.

Documentare però che il bot usa:

```text
legal path graph
+
tactical preference
+
risk cost
+
public/team knowledge
```

e non cheat information.

---

# 51. WIKI

Creare / aggiornare pagina:

```text
Movement System
```

con sezioni:

```text
Move
Move Profiles
Dash
Forced Movement
Teleport
Collision
Facing
Terrain
Noise
Reactions
Examples
Scenarios
Roadmap status
```

Ogni feature deve linkare:

```text
Feature ID
Roadmap milestone
Scenario IDs
Tests
```

---

# 52. RISCHI DA AGGIUNGERE AL RISK REGISTER

## RISK-MOV-01 — Movement rule explosion

Mitigazione:
- MovementKind + policy;
- evitare resolver custom per personaggio.

## RISK-MOV-02 — Auto-reroute violates player intent

Mitigazione:
- no auto-reroute.

## RISK-MOV-03 — Simultaneous collision ambiguity

Mitigazione:
- explicit baseline;
- golden tests.

## RISK-MOV-04 — Reaction / movement event ordering

Mitigazione:
- deterministic boundaries;
- canonical event ordering.

## RISK-MOV-05 — Facing complexity

Mitigazione:
- prototype separately;
- do not lock before scenario tests.

## RISK-MOV-06 — Displacement invalidates whole Move too often

Mitigazione:
- A/B/C scenario experiment;
- compare Model A vs C.

---

# 53. DEFINITION OF DONE SPECIFICA MOVEMENT

Movement feature Done soltanto se:

```text
[ ] authoritative resolver
[ ] deterministic
[ ] exact path respected
[ ] no implicit reroute
[ ] TurnLog reason codes
[ ] automatic tests
[ ] scenario
[ ] UI debug visualization
[ ] team preview privacy
[ ] packaged validation
[ ] docs updated
[ ] feature map updated
[ ] scenario map updated
[ ] roadmap updated
[ ] epic/issues linked
```

---

# 54. OUTPUT RICHIESTO A CLAUDE

Alla fine del lavoro Claude deve produrre un report:

```text
# Movement Consolidation Report

## Files inspected

## Files modified

## Files created

## Rules consolidated

## Rules deprecated

## Open design questions

## Product Map changes

## Feature Map changes

## Scenario Map changes

## Roadmap changes

## Epic created/updated

## Issues created/updated

## Tests added/planned

## Wiki changes

## Conflicts discovered

## Decisions requiring user approval

## Recommended next implementation step

## Suggested Git commits
```

---

# 55. COMMIT STRATEGY

Preferire commit focalizzati.

Esempio:

```text
docs(movement): consolidate move and movement taxonomy
docs(features): link movement features to scenarios and roadmap
docs(scenarios): add movement validation scenario matrix
chore(roadmap): add tactical movement epic and milestones
test(movement): define deterministic movement scenario coverage
```

Se vengono create issue GitHub:

```text
chore(project): link movement epic and issues
```

---

# 56. NON FARE

Non:

```text
- cambiare roster v0.1 senza richiesta;
- anticipare modding pubblico;
- introdurre matchmaking;
- introdurre progression;
- usare NavMesh come autorità;
- usare CharacterMovement per decidere l'esito competitivo;
- usare Tick/frame rate nel resolver;
- usare intenti avversari per warning;
- introdurre auto-reroute;
- trasformare Dash in Sprint;
- trasformare Teleport in path traversal;
- far consumare MoveBudget alla vittima di Forced Movement;
- duplicare feature / epic / issue esistenti.
```

---

# 57. PRIORITÀ IMMEDIATA

Prima implementazione / consolidamento da portare avanti:

```text
1. canonical MovementKind
2. MoveIntent con exact path
3. MoveBudget
4. micro-step resolver
5. deterministic collision baseline
6. no auto-reroute
7. TurnLog
8. movement golden tests
9. planning visualization
10. scenario suite
```

Poi:

```text
Dash
Forced Movement
Teleport
Reaction integration
Facing prototype
Network preview
```

---

# 58. DOMANDA DI DESIGN ANCORA APERTA

NON chiudere automaticamente questa decisione:

> Cosa accade al Move pianificato se la posizione della unità cambia durante Dash/Prep/Blast/Reaction prima della Move phase?

Baseline da testare:

```text
Model A — Cancel Move
```

Alternativa prioritaria:

```text
Model C — Preserve planned direction sequence
```

Non scegliere Model B — auto-repath — come default.

Il risultato deve essere deciso tramite:

```text
Scenario
+
Playtest
+
TurnLog readability
+
Counterplay impact
```

---

# 59. PRINCIPIO FINALE

Il Movement System non è solo locomozione.

È il punto di intersezione tra:

```text
position
terrain
facing
cover
LOS
noise
perception
hazards
collision
prediction
reaction
Overwatch
team coordination
simultaneous resolution
```

La documentazione, Product Map, Feature Map, Scenario Map e Roadmap devono riflettere questa centralità senza gonfiare lo scope del vertical slice.
