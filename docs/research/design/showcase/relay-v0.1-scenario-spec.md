# RefactorTactics — RT_Showcase_Relay_v01
## Scenario completo 2v2, coordinate assiali, turni, risultati attesi e handoff per Claude Code

**Stato:** DESIGN FIXTURE / scenario target della v0.1  
**Baseline documentale:** Unreal Engine 5.8.1 salvo diversa patch bloccata nel repository  
**Roster:** Team Blue = Gadget + Phase; Team Red = Riktor + Wraith  
**Scopo:** trasformare la showcase in una specifica riproducibile, utile come demo, Scenario Harness fixture, golden replay e test di integrazione.

> IMPORTANTE: questo file definisce il **risultato di design desiderato**. Claude deve verificare il repository corrente prima di implementare o rinominare API, action ID, range, priorità, costi, cooldown e schema dello Scenario Harness.  
> Non introdurre `if (Showcase)` nel resolver. Ogni feature deve passare dai sistemi generali.

---

# 1. Regole canoniche assunte

Ordine macro-fasi:

```text
Planning
→ Commit
→ Prep
→ Dash
→ Blast
→ Move
→ Cleanup
```

Vincoli:

- il normale `Move` è l'ultima azione volontaria standard;
- Dash/Charge/Leap/Reposition non sono `Move`;
- gli intenti di entrambi i team sono scelti prima della Resolution;
- Overwatch arma una reaction e usa il Facing;
- Fast Reaction baseline: 3 s, timeout = HOLD;
- HOLD non consuma la charge;
- niente stack interattivo annidato nella v0.1;
- stessa fixture + rules/config/content + seed ⇒ stesso TurnLog e stesso stato finale;
- la presentazione non decide mai gli esiti.

---

# 2. Coordinate della mappa

## 2.1 Sistema assiale

Usare:

```text
FRTCellId { X=q, Y=r, Layer=0 }
```

Direzioni convenzionali usate da questa specifica:

```text
E  = (+1,  0)
NE = (+1, -1)
NW = ( 0, -1)
W  = (-1,  0)
SW = (-1, +1)
SE = ( 0, +1)
```

La precedente immagine concettuale della mappa è **visuale, non normativa** per le coordinate.  
Questa sezione è la normalizzazione logica da usare per lo scenario.

## 2.2 Shape valida — 45 celle

```text
r=-3: q=-1..+1
r=-2: q=-2..+2
r=-1: q=-3..+3
r= 0: q=-4..+4
r=+1: q=-4..+4
r=+2: q=-3..+3
r=+3: q=-2..+2
```

Tutte le celle non elencate come speciali sono `Floor`.

## 2.3 Celle e zone speciali

| Zona | Celle | Stato iniziale / uso |
|---|---|---|
| Relay | `(0,0,0)` | Objective centrale |
| Team Blue spawn | `(-4,0,0)`, `(-4,1,0)` | Gadget, Phase |
| Team Red spawn | `(4,0,0)`, `(4,1,0)` | Riktor, Wraith |
| Smoke flank | `(-3,-1,0)`, `(-3,0,0)`, `(-3,1,0)` | Smoke |
| High Ground ridge | `(-1,-3,0)`, `(0,-3,0)`, `(1,-3,0)`, `(-2,-2,0)`, `(-1,-2,0)`, `(0,-2,0)`, `(1,-2,0)`, `(2,-2,0)`, `(2,-1,0)` | HighGround |
| Fire pocket | `(0,-1,0)`, `(1,-1,0)` | Fire |
| Rough choke | `(2,0,0)`, `(2,1,0)`, `(3,1,0)`, `(2,2,0)` | Rough |
| Water lane | `(-3,2,0)`, `(-2,2,0)`, `(-1,2,0)` | ShallowWater |
| Conductive lane | `(1,2,0)`, `(2,2,0)` | Conductive |
| Ice flank | `(-2,3,0)`, `(-1,3,0)`, `(0,3,0)`, `(1,3,0)`, `(2,3,0)` | Ice |

## 2.4 Archi/cover/interazioni

### Bridge

```text
BridgeEdge:
(-1,2,0) <-> (0,2,0)
```

Tipo: `Bridge`.

### Gate / topology test

```text
GateEdge:
(1,0,0) <-> (2,0,0)
InitialState = Open
ControlCell = (3,0,0)
```

`Interact` su `ControlCell` deve modificare lo stato del GateEdge tramite il sistema generale di struttura/interazione e incrementare la GraphRevision quando tale sistema esiste.

### Directional cover target

Tre cover di design:

```text
Cover_A:
Cell = (-1,-1,0)
ProtectedFacing = E

Cover_B:
Cell = (2,-1,0)
ProtectedFacing = W

Cover_C:
Cell = (3,0,0)
ProtectedFacing = W
```

La rappresentazione concreta deve seguire il modello E9 corrente, non questa pseudo-struttura se il codice usa dati diversi.

---

# 3. Snapshot iniziale

| Unit | Team | Start | HP/MP | Facing iniziale |
|---|---:|---|---|---|
| Gadget | Blue | `(-4,0,0)` | usare catalogo corrente | E |
| Phase | Blue | `(-4,1,0)` | usare catalogo corrente | E |
| Riktor | Red | `(4,0,0)` | usare catalogo corrente | W |
| Wraith | Red | `(4,1,0)` | usare catalogo corrente | W |

Objective:

```text
Objective.Relay
Cell = (0,0,0)
InitialOwner = Neutral
InitialScore = 0 / usare ruleset corrente
```

Seed design fixture:

```text
20260808
```

Se il progetto ha uno schema seed diverso, mantenerne solo la semantica: seed esplicito, stabile e registrato.

---

# 4. Sequenza completa della partita

## Turno 1 — Apertura, terreno, cover e quota

### Gadget

```text
Start: (-4,0,0)
Main: nessuna / Wait
MovePath:
  (-4,0,0)
  (-3,0,0)   # attraversa Smoke
  (-2,0,0)
End: (-2,0,0)
FacingFinal: E
```

Atteso:

- path valido;
- passaggio su Smoke senza cambiare autorità del movimento;
- Ghost Path leggibile;
- `MoveStep` nel TurnLog.

### Phase

```text
Start: (-4,1,0)
Action: Hero.Phase.FluidTrail
DashPath:
  (-4,1,0)
  (-3,2,0)
  (-2,2,0)
  (-1,2,0)
End: (-1,2,0)
FacingAfterDash: E/NE derivato dal movimento reale
```

Atteso:

- Linear Dash / movimento speciale;
- quando la mutazione terreno è disponibile: acqua lasciata sul percorso secondo la definizione corrente;
- nessuna logica showcase-only.

### Riktor

```text
Start: (4,0,0)
Prep: Hero.Riktor.KineticPanel
PanelTarget: settore centrale verso W
MovePath:
  (4,0,0)
  (3,0,0)
End: (3,0,0)
FacingFinal: W
```

Atteso:

- KineticPanel crea cover solo se E9/runtime structure è disponibile;
- altrimenti lo scenario resta marcato `RequiresFeature`.

### Wraith

```text
Start: (4,1,0)
MovePath:
  (4,1,0)
  (3,1,0)
  (2,1,0)
  (2,0,0)
  (2,-1,0)
End: (2,-1,0)
TerrainEnd: HighGround
FacingFinal: W/SW secondo ultimo passo ammesso
```

Atteso:

- path attraversa Rough con costo corrente;
- arrivo su HighGround;
- nessun bonus generico al danno inventato.

### Checkpoint T1

Dimostra:

```text
Simultaneous Planning
Ready/Commit
Hex path
Smoke terrain
Dash
Water lane
HighGround
Facing
Directional cover / KineticPanel
TurnLog
```

---

## Turno 2 — Setup Wet, modifica cover e prima Predictive Action

### Gadget

```text
Start: (-2,0,0)
Main: Hero.Gadget.ConductiveNode
TargetCell: (0,2,0)   # bridge approach
MovePath:
  (-2,0,0)
  (-1,1,0)
End: (-1,1,0)
FacingFinal: E/SE
```

Atteso:

- se il range corrente non consente `(0,2,0)`, Claude deve scegliere la cella valida più vicina alla lane senza cambiare lo scopo;
- quando implementato, la cella/nodo diventa conduttiva per la durata prevista dal catalogo.

### Phase

```text
Start: (-1,2,0)
Blast: Hero.Phase.PressureJet
Target: Wraith
ExpectedEffects:
  Damage
  Wet
  Push 1 if destination valid
Move:
  scegliere una rotta che NON attraversi PredictCell
```

### Riktor

```text
Start: (3,0,0)
Prep/Main: Hero.Riktor.Reconfigure
Target: KineticPanel
Expected: orientare la cover per chiudere una linea verso il Relay
MoveEnd: (2,0,0) se legalmente raggiungibile
```

### Wraith

```text
Start: (2,-1,0)
PredictiveAction: Hero.Wraith.InterceptShot
PredictedCell: (0,1,0)
Window: boundary/phase previsto dalla definizione corrente
Expected: Phase non attraversa (0,1,0)
Result: WHIFF
```

Atteso nel log:

```text
PredictiveActionDeclared
PredictionEvaluated
PredictionWhiffed / reason equivalente
```

Non aprire una Fast Reaction: questa è una Predictive Action interamente decisa in Planning.

### Checkpoint T2

Dimostra:

```text
Wet
Push
Conductive setup
Reconfigure
Predictive Action
Intent may fail because prediction was wrong
Reason code
```

---

## Turno 3 — Moving target, Fire/Burning e fallback

### Gadget

```text
Start: (-1,1,0)
Blast: Hero.Gadget.LinearDischarge
DeclaredTarget: Wraith @ posizione di Planning
MovingTargetPolicy for showcase variant: AttackCell OR il fallback realmente definito nel catalogo
```

### Wraith

```text
Start: posizione risultante da T2
Dash: Hero.Wraith.PassingBlade
DesiredPath:
  passare per almeno una cella Fire fra:
  (1,-1,0)
  (0,-1,0)
EndTarget: (1,0,0)
```

Atteso:

- Dash risolve prima del Blast;
- `Fire` applica on-enter;
- `Burning` viene applicato;
- Gadget rivalida il target al Blast;
- se il target si è mosso, applica il fallback data-driven;
- nessun tracking inventato.

### Phase

```text
Blast: Hero.Phase.CircularTide
Center: (1,0,0) o cella valida più vicina che mostri AoE senza friendly-fire involontario
```

Atteso:

- AoE;
- routing ally/enemy secondo implementazione reale;
- Wet/Push solo se definiti dalla variante scelta.

### Riktor

```text
Action: Brace / postura difensiva disponibile nel catalogo
End: mantenere accesso lato Est del Relay
```

### Checkpoint T3

Dimostra:

```text
Dash before Blast
Fire
Burning
Moving target
Fallback
AoE
Defense posture
```

---

## Turno 4 — Overwatch, HOLD e FIRE

Posizionare Wraith in modo che il suo cono, derivato dal Facing, controlli l'accesso centrale/bridge.

### Wraith

```text
Main: Overwatch
Facing: W/SW coerente con il cono verso Blue
Charges: 1
Timeout: HOLD
```

### Gadget

```text
MoveProfile: Sprint
Path: entra per primo nel cono di Wraith
ExpectedOpportunity #1:
  Target = Gadget
  Responses = FIRE / HOLD
TestPolicyResponse = HOLD
```

Atteso:

- Decision Boundary;
- simulazione logica globale ferma;
- presentation slow-motion ammessa;
- HOLD non consuma la charge.

### Phase

```text
Move: entra nel cono in un micro-step successivo
ExpectedOpportunity #2:
  Target = Phase
  Responses = FIRE / HOLD
TestPolicyResponse = FIRE
```

Atteso:

- seconda opportunity reale, non preannunciata;
- FIRE consuma la charge;
- applicare effetto Overwatch del profilo Wraith definito dal catalogo;
- nessun terzo prompt dopo il consumo.

### Riktor

```text
Move/Wait: mantiene controllo del lato Est
```

### Checkpoint T4

Dimostra:

```text
Overwatch universal framework
Facing-driven cone
ReactionOpportunity
Decision Boundary
3 s window
HOLD -> still armed
FIRE -> consume charge
Movement micro-step trigger
```

---

## Turno 5 — Smoke, targeting, Interact e Deflect

### Phase

```text
Action: Hero.Phase.MistVeil
Center: (1,1,0) o cella valida che includa Wraith ma non renda impossibile tutto il test
ExpectedTerrainMutation: Smoke radius 1 quando supportata
```

### Gadget

Prima del Commit, validare un draft volutamente borderline:

```text
Draft:
  Hero.Gadget.LinearDischarge -> Wraith
```

La UI/validator deve mostrare l'effetto reale di Smoke (es. cap targeting corrente).  
Poi committare un attacco legalmente valido:

```text
Commit:
  Hero.Gadget.ArcPulse / BasicAttack -> Wraith
```

### Wraith

```text
PreparedReaction: Hero.Wraith.Deflection
Expected: riduzione del danno secondo semantica Action.Deflect
```

### Riktor

```text
Interact:
  ControlCell = (3,0,0)
  Target = GateEdge (1,0,0)<->(2,0,0)
Expected:
  Open -> Closed oppure stato equivalente
  GraphRevision increments
```

Se il sistema runtime di strutture/interact non è ancora disponibile, marcare `RequiresFeature=E9/E10`; non simulare con un flag showcase-only.

### Checkpoint T5

Dimostra:

```text
Smoke
Target validation
Corrected draft before Ready
Deflect
Interact
Topology change
GraphRevision
```

---

## Turno 6 — Interposition, redirect, Wet e Push

### Gadget

```text
Blast: Hero.Gadget.ArcPulse / BasicAttack
Target: Wraith
```

### Riktor

```text
PreparedReaction: Hero.Riktor.Interposition
Trigger: direct attack toward ally
Expected:
  original target = Wraith
  effective target = Riktor
```

Dopo redirect:

```text
Revalidate:
  LOS
  trajectory
  cover
against Riktor as effective target
```

Non aprire una reaction interattiva annidata per la rivalidazione.

### Phase

```text
Blast: Hero.Phase.PressureJet
Target: Riktor
Expected:
  Damage
  Wet
  Push 1 if destination valid
```

### Wraith

```text
Blast: Hero.Wraith.PulseShot
Target: Gadget
```

### Checkpoint T6

Dimostra:

```text
Intercept
Target redirection
Cover/LOS revalidation
No nested reaction stack
Wet
Push
Two teams resolving simultaneous offensive intents
```

---

## Turno 7 — Payoff ambientale

Questo è il turno più denso della showcase.

### Phase

Preferenza:

```text
Hero.Phase.FluidTrail
```

con path che raggiunga almeno una delle celle Fire:

```text
(1,-1,0)
(0,-1,0)
```

senza attraversare una cella occupata illegalmente.

Quando le mutazioni ambiente sono attive:

```text
Water enters Fire
→ Fire removed
→ EnvironmentChanged logged
```

Se il catalogo corrente non consente questa combinazione tramite FluidTrail, Claude deve mantenere lo scopo e usare l'azione Phase realmente prevista per creare acqua, senza inventare una action showcase.

### Gadget

Usare la versione dell'attacco elettrico che massimizza il test della propagazione:

```text
Hero.Gadget.LinearDischarge or Hero.Gadget.Overload
```

Target primario: un nemico Wet o su rete Water/Conductive.

Expected chain rules se CP8.3 è ancora quella vigente:

```text
initial hit
→ propagation on Water/Conductive
→ max propagation limit from catalog/rules
→ each unit hit at most once
→ stable order: distance -> CellId -> UnitId
```

Non hard-codificare numeri in questo scenario se il catalogo corrente li ha cambiati.

### Wraith

```text
Normal Move through Ice
```

Atteso:

- se restano i requisiti correnti, slide deterministico +1;
- il Dash NON usa lo stesso slide se la regola corrente lo vieta.

### Riktor

Creare durante il Planning un draft non valido:

```text
Hero.Riktor.Ram through Rough
```

Expected validation:

```text
INVALID: Rough blocks Charge
```

Poi correggere prima del Commit verso una legal action/path.

### Checkpoint T7

Dimostra:

```text
Water + Fire
Wet/Burning interaction
Electric chain
Stable propagation order
Ice slide
Rough blocks Charge
Planning validation
EnvironmentChanged events
```

---

## Turno 8 — Objective > KO e seconda prediction WHIFF

Situazione desiderata:

- Phase è a una Move breve dal Relay `(0,0,0)`;
- Gadget è a HP basso;
- Wraith prova a prevedere l'accesso più ovvio;
- Riktor è separato dal Relay da Rough/cover/gate/topology.

### Wraith

```text
PredictiveAction: Hero.Wraith.InterceptShot
PredictedCell: accesso ovvio al Relay, es. (1,0,0)
```

### Phase

```text
Move:
  usare una rotta alternativa legale
  End = (0,0,0)
Expected:
  non attraversa PredictedCell
  InterceptShot = WHIFF
```

### Riktor

```text
Move/Action:
  tenta di contestare il Relay
Expected:
  non raggiunge (0,0,0) entro il budget corrente
```

La causa deve essere reale: posizione, costo Rough, GateEdge, occupazione o budget. Non forzare un fallimento artificiale.

### Gadget

```text
Blast: attacco finale su Wraith
Expected:
  Gadget resta esposto
```

### Wraith / Red payoff

```text
Hero.Wraith.PulseShot or valid attack -> Gadget
Expected:
  Gadget KO
```

### Cleanup

```text
KO resolution
Objective resolution
Cooldown/status cleanup
TurnLog finalize
```

Finale desiderato:

```text
Gadget = KO
Phase = alive on Relay
Riktor/Wraith = not contesting Relay
Team Blue scores/wins by objective according to current ruleset
```

### Checkpoint T8

Dimostra:

```text
Prediction WHIFF
Alternative path
KO
Objective update
Objective matters more than deathmatch
Cleanup
MatchEnded
```

---

# 5. Matrice di copertura feature

| Feature | Turni |
|---|---|
| Planning simultaneo | 1–8 |
| Ready / Commit | 1–8 |
| Hex + A* + micro-step | 1–8 |
| Facing | 1, 4, 6 |
| HighGround | 1 |
| Smoke | 1, 5 |
| Fire / Burning | 3, 7 |
| Water / Wet | 1, 2, 6, 7 |
| Conductive / electricity | 2, 7 |
| Rough | 1, 7, 8 |
| Ice | 7 |
| Cover | 1, 2, 6 |
| Structure topology / GraphRevision | 5 |
| Push / forced movement | 2, 6 |
| Moving target / fallback | 3 |
| Predictive Action hit/miss model | 2, 8 |
| Overwatch Fast Reaction | 4 |
| HOLD then FIRE | 4 |
| Deflect | 5 |
| Intercept / Interposition | 6 |
| Water + Fire | 7 |
| Electric propagation | 7 |
| Objective | 8 |
| KO | 8 |
| TurnLog / reason code | 1–8 |
| Deterministic replay | intera fixture |

---

# 6. Assertions consigliate

Non serve implementarle tutte in una PR. Usare solo quelle già supportate o necessarie alla tranche corrente.

## T1

```text
TurnCompleted(1)
UnitAtCell(Gadget, -2,0,0)
UnitAtCell(Phase, -1,2,0)
UnitAtCell(Riktor, 3,0,0)
UnitAtCell(Wraith, 2,-1,0)
```

## T2

```text
EventExists(PredictiveActionDeclared, Wraith)
EventExists(PredictionWhiffed, Wraith)
UnitHasStatus(Wraith, Wet)              # se PressureJet già lo supporta
```

## T3

```text
UnitHasStatus(Wraith, Burning)
EventExists(TargetMoved or fallback reason, Hero.Gadget.LinearDischarge)
```

## T4

```text
ReactionOpportunity(Owner=Wraith, Target=Gadget)
ReactionResponse(HOLD)
ReactionStillArmed(Wraith)
ReactionOpportunity(Owner=Wraith, Target=Phase)
ReactionResponse(FIRE)
ReactionConsumed(Wraith)
```

## T5

```text
SurfaceHasStatus/CellSurface(Smoke)
GraphRevisionChanged
EventExists(Deflect)
```

## T6

```text
EventExists(Intercept)
OriginalTarget=Wraith
EffectiveTarget=Riktor
NoNestedInteractiveReaction
```

## T7

```text
FireRemovedOnWaterContact
ElectricPropagationDeterministic
NoUnitHitTwiceBySameElectricEvent
IceSlideOccurred
InvalidDraft(Ram,Rough)
```

## T8

```text
PredictionWhiffed(Wraith)
UnitKO(Gadget)
UnitAtCell(Phase,0,0,0)
ObjectiveUpdated(Relay, TeamBlue)
MatchEnded
```

## Full run

```text
StateHashEquals(golden)
LogHashEquals(golden)
Repeat N times => zero divergence
```

Bloccare gli hash golden solo quando la fixture e le regole sono abbastanza stabili.

---

# 7. Feature status da verificare prima dell'implementazione

Claude deve ricontrollare il repository e classificare ogni voce:

```text
READY
PARTIAL
MISSING
SUPERSEDED
```

Almeno:

```text
Hero.Phase.FluidTrail terrain mutation
Hero.Phase.MistVeil terrain mutation
Hero.Gadget.ConductiveNode mutation/duration
Electric propagation
Water + Fire
KineticPanel
Reconfigure
Interact + GateEdge + GraphRevision
Hero.Wraith.InterceptShot predictive thin slice
Universal Overwatch + Fast Decision
Hero.Riktor.Interposition using generic Intercept
Hero.Wraith.Deflection using generic Deflect
Objective Relay
Scenario Harness assertions
```

Non implementare tutto in una singola PR.

---

# 8. Sequenza di implementazione consigliata per Claude

```text
1. Audit repo + cataloghi + scenario harness
2. Portare la mappa logica Relay Basin in fixture dati
3. Far passare Turn 1 con soli sistemi già READY
4. Aggiungere Turn 2 e predictive thin slice
5. Aggiungere Turn 3 fallback/moving target
6. Aggiungere Turn 4 universal Overwatch/Fast Reaction
7. Aggiungere Turn 5 structures/Smoke
8. Aggiungere Turn 6 Interposition
9. Aggiungere Turn 7 environment combo
10. Aggiungere Turn 8 Objective/KO
11. Golden replay + repeat + packaged smoke
```

Ogni passaggio deve restare una slice giocabile e testabile.

---

# 9. Regole anti-shortcut

Vietato:

```text
Scenario -> SetActorLocation
Scenario -> ApplyDamage diretto
Scenario -> muta terreno direttamente bypassando action/effect/resolver
Scenario -> special case su HeroId/ActionId dentro TurnManager
Scenario -> prevede intenti avversari usando dati non autorizzati
```

Richiesto:

```text
Scenario
→ normal Intent/Command
→ Planning validation
→ Commit
→ Snapshot/segment
→ Resolver
→ TurnLog
→ state
→ assertions
```

La stessa fixture deve poter alimentare Visual, Fast e Headless mode.

---

# 10. Definition of Done della showcase

La showcase `RT_Showcase_Relay_v01` è Done quando:

1. la mappa usa coordinate assiali e nessun residuo square-grid;
2. i quattro eroi usano il catalogo corrente;
3. gli 8 turni completano senza input manuale in Scenario Harness;
4. le Fast Reaction usano vere Reaction Opportunities;
5. la predictive action non usa informazioni future;
6. gli effetti ambiente passano dal resolver generale;
7. il Relay decide il finale secondo ruleset;
8. il TurnLog spiega hit, miss, redirect, status, environment e objective;
9. repeat test produce zero divergenze;
10. la stessa fixture è osservabile in Visual mode;
11. nessun dato/feature v0.1 viene hard-coded esclusivamente per questa demo;
12. packaged smoke test passa.
