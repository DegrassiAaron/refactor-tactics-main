# RefactorTactics — Implementazione terreno Ghiaccio v0.1 in Unreal Engine 5

## Contesto progetto

Stai lavorando su **RefactorTactics**, tattico competitivo PC-first sviluppato in **Unreal Engine 5**, basato su turni simultanei.

Caratteristiche principali:

- formato principale 3v3;
- vertical slice 2v2;
- planning simultaneo;
- resolution deterministica;
- mappa tattica esagonale multilivello;
- `FRTCellId` con coordinate `X`, `Y`, `Layer`;
- server authoritative;
- il client propone, il server valida;
- stesso snapshot + seed + versione + regole = stesso risultato;
- C++ per simulazione, rete, regole, serializzazione e pathfinding;
- Blueprint/Data Assets per configurazione, UI, VFX e contenuti;
- Gameplay Tags governati;
- niente logica competitiva dipendente da animazioni o frame rate;
- costi e valori della simulazione preferibilmente integer/fixed-point;
- terreni e abilità completamente data-driven;
- niente riferimenti hard-coded tra singola abilità e singolo terreno.

La mappa non è uno sfondo: è un sistema strategico attivo.

Il terreno deve poter cambiare durante una partita e modificare:

- movimento;
- pathfinding;
- visibilità;
- targeting;
- forced movement;
- abilità;
- copertura;
- hazard;
- propagazioni ambientali;
- collegamenti del grafo tattico.

---

# Obiettivo

Progettare e implementare il sistema iniziale del **terreno Ghiaccio v0.1**.

Il ghiaccio NON deve essere semplicemente:

> terreno con movement cost più alto.

La sua identità principale deve derivare da:

- traction;
- momentum;
- scivolamento;
- direzione;
- forced movement;
- integrità;
- trasformazioni fisiche;
- interazione con caldo, freddo, acqua, elettricità, vento e impatti;
- modifica temporanea del grafo della mappa.

Il sistema deve essere abbastanza generale da diventare successivamente la base del sistema ambientale completo.

---

# Principio fondamentale

Evitare RNG nascosto per:

- cadute;
- scivolamento;
- rottura del ghiaccio.

In un gioco competitivo il risultato deve essere prevalentemente deterministico.

La UI deve poter classificare gli esiti come:

- **Confermato**
- **Previsto**
- **Incerto**

`Incerto` deve derivare soprattutto da informazioni non disponibili durante il planning, per esempio un'azione avversaria simultanea, non da un tiro casuale invisibile.

---

# 1. Stati ambientali da implementare

Implementare inizialmente questi stati:

```text
Ice
CrackedIce
Water
Steam
ElectrifiedWater
```

Possibile estensione futura:

```text
ThinIce
Snow
DeepSnow
FrozenMud
BlackIce
```

Non implementarli ora salvo predisposizione data-driven.

---

# 2. State machine ambientale

La catena fondamentale deve essere:

```text
                 COLD
                  │
                  ▼
            ┌──────────┐
            │   ICE    │
            └──────────┘
              │      │
       IMPACT │      │ HEAT
              ▼      ▼
       ┌──────────┐ ┌─────────┐
       │CRACKEDICE│ │  WATER  │
       └──────────┘ └─────────┘
              │        │   │   │
       IMPACT │     COLD  ⚡  HEAT
              ▼        │   │   │
           BROKEN      │   │   ▼
                       │   │ STEAM
                       │   │
                       └───┘
```

Transizioni minime:

```text
Ice + Heat -> Water
Ice + Impact -> CrackedIce
CrackedIce + Impact -> Broken
Water + Cold -> Ice
Water + Electricity -> ElectrifiedWater
Water + Heat -> Steam
Steam + Cold -> Water
Steam + Wind -> dissipazione
```

La transizione deve dipendere da quantità configurabili, per esempio:

```text
HeatAmount
ColdAmount
ForceAmount
ElectricityAmount
WindAmount
IntegrityDamage
```

Non usare condizioni hard-coded come:

```cpp
if (Ability == Fireball && Terrain == Ice)
```

Le abilità devono emettere proprietà ambientali generiche.

Esempio:

```text
Damage.Fire
Environment.Heat = 40
Environment.Force = 20
```

Il terreno reagisce a `Heat`, non a `Fireball`.

---

# 3. Proprietà del ghiaccio

Ogni cella di ghiaccio deve poter esporre almeno:

```text
Traction
MovementCost
PushMultiplier
TurnInstability
SprintInstability
HeatResistance
ColdResistance
Integrity
Opacity
SurfaceType
HazardFlags
Revision
```

Valori iniziali graybox suggeriti:

| Proprietà | Ground | Ice | Wet Ice equivalente futuro | Cracked Ice |
|---|---:|---:|---:|---:|
| Traction | 100 | 35 | 20 | 40 |
| MovementCost | 100 | 100 | 110 | 110 |
| PushMultiplier | 100 | 150 | 175 | 150 |
| TurnInstability | 0 | 20 | 35 | 25 |
| SprintInstability | 0 | 40 | 60 | 50 |
| HeatResistance | — | 100 | 40 | 70 |
| Integrity | — | 100 | — | 50 |

Sono valori iniziali per test, non bilanciamento definitivo.

---

# 4. Movimento sul ghiaccio

Il movimento normale deve rimanere relativamente sicuro.

## Move normale

Caratteristiche:

- niente caduta casuale;
- movement cost quasi normale;
- capacità di cambiare direzione;
- maggiore vulnerabilità a spinte e momentum.

## Run/Sprint

Più velocità deve significare maggiore momentum.

Lo sprint deve rendere:

- più difficile fermarsi;
- più difficile cambiare direzione;
- più probabile uno slide deterministico;
- possibile terminare in stato `Unbalanced` o `Prone`.

---

# 5. Momentum

Creare un concetto logico di `Momentum`.

Possibile modello iniziale:

```text
Momentum =
BaseMovementMomentum
+ MovementSpeedContribution
+ PreviousMomentum
+ ExternalForce
+ SlopeContribution
```

Applicare poi la traction:

```text
EffectiveMomentum =
Momentum * SurfaceSlipFactor
```

Usare integer/fixed-point.

Niente float non deterministici se evitabili nella simulazione competitiva.

---

# 6. Cambio di direzione

La griglia è esagonale.

Le direzioni differiscono di multipli di 60°.

Un possibile sistema iniziale:

```text
0°   -> instability +0
60°  -> instability +10
120° -> instability +25
180° -> instability +50
```

Questi valori devono essere configurabili.

Un personaggio che corre diritto sul ghiaccio può essere relativamente sicuro.

Un personaggio che tenta:

```text
Sprint
-> curva 120°
-> Attack
```

può generare uno stato di forte instabilità.

---

# 7. Stability e Traction del personaggio

Predisporre attributi logici come:

```text
TractionModifier
Stability
Mass
ForcedMovementResistance
MomentumControl
```

Non devono necessariamente essere GAS Attribute nel primo prototipo.

Possibile formula:

```text
Instability =
SurfaceInstability
+ MovementSpeed
+ DirectionChange
+ Slope
+ ExternalForce
- Traction
- Stability
```

Il risultato può generare soglie deterministicamente.

Esempio:

```text
Instability < 30
-> Stable

30-59
-> Slide 1

60-89
-> Slide 2 + Unbalanced

>= 90
-> Slide + Prone
```

I valori sono da considerare graybox.

---

# 8. Slide

Lo slide è forced movement generato dalla superficie.

Deve avere:

```text
Direction
RemainingMomentum
Source
MaxDistance
StopReason
```

Possibili stop:

```text
HigherTractionSurface
Wall
UnitCollision
Obstacle
SlopeUp
MomentumZero
Reaction
MapBoundary
```

Lo slide deve passare dallo stesso sistema di forced movement usato da:

- Push;
- Knockback;
- Pull;
- Explosion;
- Dash overshoot.

Non creare sistemi separati non necessari.

---

# 9. Forced movement e Push

Il ghiaccio deve amplificare le spinte.

Esempio:

```text
Push 1
```

su terreno normale:

```text
A -> B
```

su ghiaccio:

```text
A -> B -> C -> D
```

a seconda di:

```text
PushForce
Mass
Traction
Momentum
Slope
Collision
```

La skill non deve avere una regola speciale "push extra on ice".

È il terreno a trasformare il movimento.

---

# 10. Collisioni tra unità

Lo slide può causare collisioni.

Esempio:

```text
A ---> B
```

Generare:

```text
CollisionEvent
```

Il resolver può trasferire parte del momentum:

```text
MomentumTransfer =
IncomingMomentum
* TransferFactor
/ TargetMass
```

Possibile risultato:

```text
A -> B -> C
```

se B si trova anch'esso su ghiaccio.

Per evitare catene infinite, introdurre limite configurabile:

```text
MaxForcedMovementChain = 4
```

o un sistema equivalente basato su event budget.

Garantire determinismo e ordinamento stabile.

---

# 11. Stati Unbalanced e Prone

Non rendere ogni errore una perdita completa del turno.

## Unbalanced

Possibili effetti:

```text
- Stability
No Sprint
IncreasedPushReceived
ReducedTurnControl
```

## Prone

Possibili effetti:

```text
movement interrupted
reduced defense
melee vulnerability
requires StandUp transition
```

Implementare soltanto lo scheletro necessario per il vertical slice, senza espandere eccessivamente il sistema status.

---

# 12. Scivolamento volontario

Il ghiaccio deve offrire anche opportunità.

Un giocatore deve poter usare il ghiaccio per:

- estendere un movimento;
- raggiungere una posizione altrimenti fuori range;
- attraversare velocemente un corridoio;
- entrare in melee;
- allontanarsi da una minaccia.

Questo significa che uno slide non deve essere sempre considerato errore o debuff.

Prevedere in futuro abilità come:

```text
IceSkater
ControlledSlide
MomentumRedirect
Brake
Anchor
Brace
```

Non implementarle tutte adesso.

---

# 13. Pendenze

Predisporre il supporto a:

```text
SlopeContribution
```

Una discesa deve aumentare il momentum.

Una salita deve ridurlo.

Esempio:

```text
Ice + downhill -> longer slide
Ice + uphill -> faster momentum decay
```

Il sistema dovrà funzionare in futuro sulla mappa multilivello.

---

# 14. Integrità del ghiaccio

Il ghiaccio può avere:

```text
Integrity = 100
```

Azioni differenti infliggono danni diversi.

Esempio graybox:

```text
LightUnitStep      10-15
NormalUnitStep     20-30
HeavyUnitStep      40-50
Sprint             +20
Dash               +30
Explosion          +60
GroundSlam         +80
```

Preferire valori deterministici.

Quando l'integrità scende sotto una soglia:

```text
Ice -> CrackedIce
```

e successivamente:

```text
CrackedIce -> Broken
```

---

# 15. Thin Ice futuro

Predisporre il sistema in modo che in futuro si possa creare:

```text
Terrain.Ice.Thin
```

senza modificare il resolver.

Un personaggio pesante potrebbe rompere il ghiaccio molto più rapidamente di uno leggero.

---

# 16. Rottura del ghiaccio

Il risultato dipende dal layer sottostante.

Esempio:

```text
Layer 2: Ice
Layer 1: Water
```

rottura:

```text
Layer 2 node unavailable
Unit transitions to Layer 1
```

possibili effetti:

```text
Wet
Slowed
Cold
```

Oppure:

```text
Layer 2: Ice bridge
Layer 1: lower ground
```

rottura:

```text
Fall transition
FallDamage
```

Usare `FRTCellId.Layer`.

---

# 17. Acqua

`Water` deve diventare uno stato ambientale attivo.

Proprietà minime:

```text
Conductivity
Depth
MovementModifier
WetApplication
FreezeThreshold
SteamThreshold
```

Nel vertical slice si può assumere `Depth = Shallow`.

---

# 18. Elettricità + acqua

La combo deve essere:

```text
Ice
-> Heat
-> Water
-> Electricity
-> ElectrifiedWater
```

L'elettricità deve propagarsi attraverso celle d'acqua connesse.

La propagazione può essere una ricerca sul grafo:

```text
BFS / flood fill
```

con limiti:

```text
MaxPropagationCells
ElectricityFalloff
WaterConnectivity
```

La propagazione deve essere deterministica.

Non trattare necessariamente il ghiaccio puro come conduttore forte.

---

# 19. Freddo + acqua

Il freddo deve permettere:

```text
Water + Cold -> Ice
```

Questo può:

- rimuovere ElectrifiedWater;
- creare nuova superficie attraversabile;
- creare ghiaccio scivoloso;
- modificare il grafo.

---

# 20. Ponti di ghiaccio

Questa è una feature importante da predisporre.

Scenario:

```text
Land
~~~~ water ~~~~
Land
```

Una abilità Freeze può trasformare alcune celle d'acqua in Ice.

Questo deve poter creare temporaneamente nuovi archi nel grafo tattico.

Esempio:

```text
Before:
A      B

NoConnection
```

dopo Freeze:

```text
A-Ice-Ice-Ice-B
```

Il pathfinding deve rilevare la revisione del grafo.

Aggiornare:

```text
GraphRevision
ChunkRevision
```

e invalidare le cache path relative.

Non è necessario implementare una infrastruttura di caching complessa in questa feature se non esiste ancora, ma predisporre l'API correttamente.

---

# 21. Fuoco + ghiaccio

La sequenza preferita è:

```text
Ice
-> Heat
-> Water
```

Non generare sempre Steam immediatamente.

Steam richiede ulteriore energia:

```text
Water + Heat -> Steam
```

Questo crea gameplay su più turni.

Esempio:

```text
Turn N:
Fireball -> Ice -> Water

Turn N+1:
FlameJet -> Water -> Steam
```

---

# 22. Steam

Steam non deve essere trattato come cover fisica.

Distinguere:

```text
Cover
```

da:

```text
Opacity / Obscurant
```

Una parete può:

```text
BlockProjectile = true
BlockLOS = true
```

Steam può:

```text
BlockProjectile = false
Opacity > 0
```

La LOS deve poter accumulare opacità.

Esempio:

```text
SteamOpacity = 35
```

due celle:

```text
AccumulatedOpacity = 70
```

Il sistema visibilità decide quindi se il bersaglio rimane identificabile.

---

# 23. Steam e stealth

Predisporre gameplay come:

```text
Steam
-> reduced detection
-> stealth advantage
```

Un personaggio può avere in futuro:

```text
ThermalVision
SteamVisionResistance
DetectionBonus
ObscuredStealthBonus
```

Non hard-codare questi effetti nell'Ice system.

---

# 24. Vento + Steam

Un effetto con proprietà:

```text
Environment.Wind
```

può:

- ridurre durata Steam;
- spostare Steam;
- dissiparlo.

Per v0.1 è sufficiente supportare:

```text
Wind -> reduce SteamDuration
```

Lo spostamento fisico delle cloud può essere futuro.

---

# 25. Abilità compatibili automaticamente

Le abilità devono dichiarare proprietà ambientali generiche.

Esempio:

```text
Environment.Heat
Environment.Cold
Environment.Force
Environment.Electricity
Environment.Wind
Environment.IntegrityDamage
Movement.Dash
Movement.Teleport
```

Questo deve produrre naturalmente:

| Abilità | Effetto |
|---|---|
| Fire | scioglie ghiaccio |
| Cold | congela acqua |
| Explosion | incrina ghiaccio |
| Push | aumenta forced movement |
| Pull | trascina su superficie |
| Dash | aumenta momentum |
| Teleport | evita traversing surface |
| Ground Slam | danneggia Integrity |
| Wind | dissipa Steam |
| Electricity | elettrifica acqua |
| Barrier | ferma slide |
| Grapple | può interrompere slide |

---

# 26. Planning UI

La preview deve poter spiegare cosa succederà.

Esempio:

```text
Movement:
Confirmed: A3 -> B3 -> C3

Predicted:
Slide C3 -> D3

Uncertain:
Possible enemy collision at D3
```

Oppure:

```text
Confirmed:
Push target 1 cell

Predicted:
Ice slide +2 cells

Uncertain:
Enemy simultaneous movement may block final cell
```

Niente UI che mostri informazioni private nemiche.

---

# 27. Resolution order

Integrare le interazioni ambientali nel ciclo di resolution.

Ordine generale progetto:

```text
1. effects / reactions
2. transitions and movement micro-steps
3. control / defense / interrupts
4. attacks / abilities
5. environmental propagation
6. KO / objectives / cooldown / cleanup
```

Definire chiaramente in quale fase avvengono:

```text
Momentum
Slide
Collision
IceIntegrityDamage
IceBreak
Heat
Melt
ElectricPropagation
SteamCreation
SteamDecay
```

Proporre un ordine deterministico e motivarlo.

Preferenza iniziale:

```text
Movement microstep
-> forced movement
-> collision
-> integrity damage
-> terrain transitions
```

Le propagazioni ambientali estese possono avvenire nella relativa fase ambientale.

---

# 28. Gameplay Tags suggeriti

Proporre una tassonomia coerente, per esempio:

```text
Terrain.Surface.Ice
Terrain.Surface.Ice.Cracked
Terrain.Surface.Water

Environment.State.Steam
Environment.State.Electrified

Environment.Interaction.Heat
Environment.Interaction.Cold
Environment.Interaction.Force
Environment.Interaction.Electricity
Environment.Interaction.Wind

Status.Movement.Unbalanced
Status.Movement.Prone
Status.Movement.Sliding

Movement.Forced.Push
Movement.Forced.Pull
Movement.Forced.Slide
```

Non aggiungere decine di tag inutili.

---

# 29. Architettura Unreal desiderata

Seguire le convenzioni esistenti del progetto.

Se non esistono ancora classi equivalenti, proporre qualcosa vicino a:

```text
FRTCellId

FRTCellState
FRTSurfaceState
FRTEnvironmentState

FRTEnvironmentInteraction
FRTEnvironmentInteractionResult

URTEnvironmentDefinition
URTSurfaceDefinition

URTEnvironmentSubsystem

FRTForcedMovementRequest
FRTForcedMovementResult

FRTMovementSimulationContext

FRTHexDirection
```

Non creare Actor per ogni cella.

Le celle devono essere dati compatti centralizzati.

Gli Actor servono solo per:

- rendering;
- collisioni;
- interazioni visibili.

---

# 30. Data Asset

Progettare una definizione data-driven per Ice.

Esempio concettuale:

```text
DA_Surface_Ice

Id
Version
GameplayTags

Movement:
  Cost
  Traction
  PushMultiplier
  TurnInstability
  SprintInstability

Integrity:
  MaxIntegrity
  CrackThreshold
  BreakThreshold

Environment:
  HeatResistance
  FreezeResistance
  Conductivity
  Opacity

Transitions:
  Heat -> Water
  Impact -> CrackedIce
  ...
```

Le transizioni devono poter essere validate dal Data Validator.

---

# 31. Debug

Creare debug visualization opzionale.

Per ogni cella deve essere possibile visualizzare:

```text
Surface
Traction
Momentum
Integrity
Heat
Cold
Electricity
Opacity
GraphRevision
```

Possibili console/debug flag:

```text
rt.Debug.Environment
rt.Debug.Movement
rt.Debug.Ice
```

Usare nomi coerenti con Unreal.

Non inventare API engine inesistenti.

---

# 32. TurnLog

Ogni cambiamento importante deve generare eventi logici.

Esempio:

```text
MovementStarted
MomentumChanged
SlideStarted
ForcedMovement
Collision
IceIntegrityChanged
IceCracked
IceBroken
WaterCreated
WaterElectrified
SteamCreated
SteamDissipated
GraphChanged
UnitProne
```

Gli eventi devono utilizzare ID stabili.

Devono poter servire in futuro per:

- replay;
- combat log;
- UI;
- debug;
- test deterministici.

---

# 33. Test automatici

Creare Automation Tests almeno per:

## Test 1 — movimento normale

```text
Unit walks on Ice
-> no slide
```

## Test 2 — sprint

```text
Unit sprints straight
-> predictable momentum
```

## Test 3 — curva

```text
Sprint + 120° direction change
-> increased instability
```

## Test 4 — Push

```text
Push on Ground = 1
Push on Ice > 1
```

## Test 5 — collisione

```text
Sliding unit hits another unit
-> deterministic collision result
```

## Test 6 — Heat

```text
Ice + sufficient Heat
-> Water
```

## Test 7 — Impact

```text
Ice + IntegrityDamage
-> CrackedIce
```

## Test 8 — Electricity

```text
Connected Water cells
+ Electricity
-> deterministic propagation
```

## Test 9 — Freeze

```text
Water + Cold
-> Ice
```

## Test 10 — Steam

```text
Water + sufficient Heat
-> Steam
```

## Test 11 — Graph revision

```text
Water cells become Ice
-> graph connection changes
-> GraphRevision increments
```

## Test 12 — determinismo

Eseguire due volte:

```text
same snapshot
same command list
same seed
```

e verificare:

```text
same final state
same TurnLog
```

---

# 34. Casi limite

Gestire o almeno documentare:

```text
slide fuori mappa
slide contro muro
slide contro porta
slide contro unità
due unità che entrano simultaneamente nella stessa cella
due slide contrapposti
ghiaccio che si rompe sotto due unità
freeze durante electricity propagation
melt durante movement
steam generation durante LOS
ice bridge distrutto mentre è attraversato
forced movement su cambio Layer
```

Indicare quali vengono implementati ora e quali vengono rinviati.

---

# 35. Scope v0.1

Implementare SOLO:

```text
Ice
CrackedIce
Water
Steam
ElectrifiedWater

Traction
Momentum
Slide
Push interaction
Collision basic
Integrity
Heat
Cold
Force
Electricity
Steam opacity
Graph revision
```

Non implementare ancora:

```text
Snow
DeepSnow
BlackIce
complex weather
full fluid simulation
temperature diffusion simulation
advanced steam advection
procedural freezing
realistic thermodynamics
complex fracture simulation
```

Semplicità e determinismo prima di realismo.

---

# 36. Deliverable richiesto

Prima analizza il codice esistente di RefactorTactics.

NON modificare architettura esistente senza necessità.

Poi fornisci:

## A. Analisi

- classi già esistenti riutilizzabili;
- dipendenze;
- problemi architetturali rilevanti;
- assunzioni;
- versione UE utilizzata.

## B. Architettura

Diagramma testuale:

```text
Ability
  ↓
EnvironmentInteraction
  ↓
EnvironmentResolver
  ↓
CellState
  ↓
TerrainTransition
  ↓
GraphRevision
  ↓
TurnLog
  ↓
Presentation
```

Espandilo secondo l'architettura reale del repository.

## C. File

Lista precisa:

```text
CREATE:
...

MODIFY:
...
```

## D. C++

Fornire codice compilabile essenziale.

Evitare pseudocodice quando può essere fornito C++ reale.

Non inventare API Unreal Engine.

Se una parte dipende dalla versione UE, segnalarlo.

Non introdurre più di due nuove macro Unreal concettualmente rilevanti per step didattico.

## E. Data Assets

Spiegare:

- classi;
- proprietà;
- asset da creare;
- valori iniziali.

## F. Editor setup

Passo-passo:

```text
1.
2.
3.
...
```

## G. Graybox test

Creare una zona nella `L_DevSandbox` tipo:

```text
Ground Ground Ground
Ground Ice   Ice
Ground Ice   Ice
Ground Water Water
```

con almeno:

- una unità;
- percorso;
- Sprint;
- Push;
- Heat;
- Freeze;
- Electricity.

## H. Debug

Mostrare in viewport:

```text
CellId
Surface
Traction
Integrity
Momentum
```

## I. Automation Test

Creare test compilabili.

## J. Build/test

Indicare:

```text
compile command
Automation Test command
PIE setup
listen server test se applicabile
packaged build verification
```

## K. Errori comuni

Per esempio:

- usare Actor per ogni cella;
- mettere la logica nelle Blueprint;
- usare RNG per slide;
- aggiornare la NavMesh come autorità;
- dipendere dagli FPS;
- usare `TMap` iteration order per resolution;
- collegare direttamente Fireball a Ice;
- modificare il grafo senza incrementare Revision;
- usare VFX come causa del risultato;
- replicare dati ambientali non necessari.

## L. Commit Git

Proporre commit piccoli.

Esempio:

```text
feat(environment): add data-driven surface definitions

feat(movement): add deterministic traction and slide

feat(environment): add ice state transitions

feat(environment): add water electricity propagation

test(environment): cover ice simulation determinism
```

## M. Passo successivo

Dopo v0.1 proporre soltanto il prossimo step più utile.

---

# 37. Regola architetturale più importante

Il sistema deve produrre emergent gameplay.

Non vogliamo programmare:

```text
Fireball + Ice = X
LightningBolt + Water = Y
Dash + Ice = Z
```

Vogliamo programmare:

```text
Fireball
-> Heat + Force

LightningBolt
-> Electricity

Dash
-> Momentum

Ice
-> reacts to Heat + Force + Momentum

Water
-> reacts to Cold + Electricity + Heat
```

Da queste proprietà devono emergere automaticamente:

```text
Ice + Heat -> Water

Water + Electricity -> ElectrifiedWater

Water + Heat -> Steam

Steam -> reduced visibility

Ice + Dash -> Slide

Ice + Push -> increased forced movement

Ice + Impact -> CrackedIce

Water + Cold -> Ice

Frozen Water -> new graph path
```

Questo principio deve guidare tutta l'implementazione.

---

# 38. Risultato di gameplay desiderato

Alla fine il sistema deve permettere scenari come:

### Combo 1

```text
ICE
↓ Fire
WATER
↓ Electricity
ELECTRIFIED WATER
```

### Combo 2

```text
ICE
↓ Heat
WATER
↓ Heat
STEAM
↓
VISIBILITY REDUCTION
↓
STEALTH ADVANTAGE
```

### Combo 3

```text
SPRINT
↓
ICE
↓
SLIDE
↓
PUSH
↓
EXTRA SLIDE
↓
LEDGE
↓
FALL
```

### Combo 4

```text
WATER
↓ Cold
ICE BRIDGE
↓
NEW GRAPH PATH
↓
HEAVY UNIT
↓
INTEGRITY DAMAGE
↓
CRACK
↓
BREAK
```

### Combo 5

```text
Enemy on Ice
↓
Force ability
↓
Slide
↓
Collision
↓
Second unit receives momentum
```

Tutti questi risultati devono derivare dalle stesse regole generiche.

---

Procedi analizzando prima il repository e implementa il sistema incrementale, evitando refactor non necessari e mantenendo il progetto compilabile dopo ogni step.