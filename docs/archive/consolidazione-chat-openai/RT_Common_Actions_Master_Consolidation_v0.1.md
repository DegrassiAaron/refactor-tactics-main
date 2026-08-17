# RefactorTactics — Common Actions Master Consolidation v0.1

> 🔄 **Nomi del roster sostituiti il 2026-08-17** per [D-130] (`Flux`→`Gadget`, `Riva`→`Phase`, `Bastion`→`Riktor`, `Vektor`→`Wraith`). Le issue e le PR che questo documento cita usano ancora i nomi precedenti: è il costo dichiarato dalla decisione, non una svista.

**Data:** 2026-08-09  
**Scope:** Wait, Move, BasicAttack, Guard, Brace, Activate, Interact, Overwatch, Move Profiles, Special Movement e Facing.

## 1. Ordine canonico

```text
Planning -> Commit -> Prep -> Dash -> Blast -> Move -> Cleanup
```

- Il normale `Move` è sempre l'ultima fase/azione volontaria standard.
- `Dash`, `Charge`, `Leap`, `Blink`, displacement e movement reattivo non sono il normale Move.
- Fast Action / Fast Reaction sono Decision Boundary, non macro-fasi.

## 2. Conflitto documentale trovato

Esistono due decisioni recenti incompatibili.

### D-014 gameplay
```text
Wait
BasicAttack
Interact
Brace
Move
Overwatch
```

con `Activate -> Interact` e `Guard` non più universale.

### D-AUDIT-01 successiva
Conferma invece per v0.1:
```text
Wait
Move
BasicAttack
Guard
Brace
Activate
Interact
Overwatch
```

e specifica:
- Guard = protezione generale;
- Brace = stance/anti-displacement;
- Activate = attivazione dispositivo/mappa;
- Interact = interazione generica/obiettivo.

### Baseline di cleanup
Usare la tassonomia a **8 azioni** come baseline più recente, ma prima di cambiare codice/cataloghi verificare il Decision Log/ADR corrente del repository. Il conflitto va registrato, non corretto silenziosamente.

## 3. Tassonomia master

```text
UNIVERSAL ACTIONS
├─ Wait
├─ BasicAttack
├─ Guard
├─ Brace
├─ Activate
├─ Interact
├─ Overwatch
└─ Move
   ├─ Sneak
   ├─ Normal
   └─ Sprint

SPECIAL / PRE-BLAST MOVEMENT
├─ Dash
├─ Charge
├─ Leap
├─ Blink
└─ Special Reposition

FORCED MOVEMENT
├─ Push
├─ Pull
├─ Knockback
├─ Drag
├─ Throw
├─ Current
└─ Conveyor
```

`Sprint != Dash`.

## 4. Wait

Wait è una rinuncia volontaria a una possibilità d'azione.

Regole:
- nessun effetto nascosto;
- nessun bonus automatico;
- nessuna risorsa gratuita salvo regola esplicita;
- eventuali effetti character-specific devono essere dichiarati nel kit/data.

## 5. Basic Attack

`BasicAttack` è una categoria universale, non “la skill debole senza cooldown”.

Pattern di design:
```text
PRIMARY WEAPON
ENGINE ATTACK
SETUP ATTACK
UTILITY / EMERGENCY ATTACK
```

Il payload è character-specific e data-driven.

### No false choice
Per ogni personaggio deve esistere almeno una situazione ripetibile in cui il Basic Attack è una scelta sensata.

Direzioni v0.1 da validare:
- Wraith: Primary Weapon;
- Gadget: Engine Attack / setup;
- Phase: Setup Attack / Wet;
- Riktor: Utility/Emergency.

Non canonizzare numeri o effetti finché i cataloghi non li approvano.

## 6. Guard

`Guard` è protezione generale universale, distinta da Brace.

Non deve diventare automaticamente:
- Intercept;
- Counter;
- Parry;
- Overwatch;
- difesa signature.

Costo e payload numerico restano data-driven.

## 7. Brace

`Brace` è preparazione difensiva specifica, soprattutto per stabilità/displacement/geometry defense.

Si integra con il Reaction Framework:

```text
Planning -> Brace armed -> Trigger -> Reaction Opportunity -> Response / HOLD
```

Non confondere:
- Guard = protezione generale;
- Brace = stance/reaction anti-displacement più specifica.

## 8. Activate

`Activate` è l'azione universale per attivare dispositivi o elementi di mappa.

Esempi:
- console;
- generator;
- valve;
- switch;
- bridge control.

Flusso:
```text
Query legal actions -> Activate -> Validate -> Resolver -> Map/Graph mutation -> TurnLog
```

Niente hard-code per Actor type.

## 9. Interact

`Interact` è l'interazione generica con mappa, obiettivi o entità.

Esempi:
- pick up/drop objective;
- contextual interaction;
- objective operation.

Activate e Interact possono condividere primitive tecniche ma restano player-facing distinti finché il Decision Log mantiene questa scelta.

## 10. Move

Move è il riposizionamento volontario standard nella fase finale `Move`.

Input minimo:
```text
Path
Destination
MovementProfile
Facing / FacingPolicy
```

Il path è gameplay.

### A* propone, il giocatore decide
```text
Click destination -> recommended path
Waypoint/drag -> manual path
Reset -> recompute
```

Due path verso la stessa cella possono differire per cover, hazard, rumore, Overwatch, porte, quota, tunnel e facing.

## 11. Move Profiles

```text
Move
├─ Sneak
├─ Normal
└─ Sprint
```

- Sneak: meno distanza / meno rumore.
- Normal: baseline.
- Sprint: più distanza / più rumore ed esposizione.

Valori e costi sono tunable data-driven.

## 12. Micro-step

Move non è teleport logico.

```text
A -> B -> C -> D
```

deve produrre eventi per transizione, ad esempio:
`MoveStep`, `EdgeCrossed`, `EnteredCell`, `NoiseGenerated`, `HazardTriggered`, `ReactionOpportunity`, `MoveBlocked`.

## 13. Special Movement

Dash è movimento volontario speciale nella fase Dash.

Possibili famiglie:
```text
Dash
Charge
Leap
Blink
Special Reposition
```

Policy candidate:
```text
Shape
Range
AllowedTransitions
CollisionPolicy
TerrainPolicy
FacingPolicy
ReactionProfile
StopPolicy
```

## 14. Forced Movement

Forced Movement non è un'azione della vittima e non consuma Move/Dash/MoveBudget della vittima.

La causa/source deve essere registrata.

## 15. Teleport / Blink

Teleport non attraversa logicamente le celle intermedie.

Normalmente:
- no edge trigger intermedio;
- no hazard intermedio;
- no occupancy intermedia;
- sì occupancy/hazard/visibility/trigger di landing.

## 16. Trigger geografici vs semantici

Geografici:
```text
EnemyEnterArea
EnemyEnterCell
EnemyLeaveArea
EnemyCrossEdge
LeaveAdjacency
OccupancyChanged
```

Semantici:
```text
EnemyUsesMove
EnemyUsesSprint
EnemyUsesDash
EnemyIsForced
EnemyTeleports
```

Le reaction possono combinarli.

## 17. Overwatch

Overwatch è Universal Action che prepara una reaction.

Costo-opportunità corrente:
```text
Attack OR Ability OR Overwatch
```

salvo eccezione dichiarata.

Il dettaglio runtime appartiene al `Reaction System Master`.

## 18. Facing

Facing è stato logico autorevole su griglia hex.

Baseline corrente emersa dall'audit:
- `Linear*`: facing dalla direzione del movimento;
- `Budget Move`: scelta fra ultimo passo e due direzioni adiacenti;
- da fermo: rotazione libera fra 6 direzioni, senza slot;
- forced movement con source: facing verso la source dell'ultimo displacement;
- Overwatch usa lo stesso facing, non una Direction parallela.

Le precedenti matrici `MoveEndPivotMaxSteps` / `DashEndPivotMaxSteps` per eroe restano **proposal/playtest**, non canone finché non viene approvata una nuova decisione.

## 19. Character Base Action Signature

L'identità del personaggio non vive solo nelle quattro signature ability.

Può differenziarsi in:
```text
Move
Move Profiles
Special Movement
Basic Attack
Guard
Brace
Overwatch
Activate/Interact affinity
Wait behavior se esplicito
Facing behavior entro il canone
```

Gerarchia:
```text
Universal Action
 -> Role tendency
 -> Character profile
 -> Build/Talent/Gadget
 -> Context/Status/Terrain
```

Data-driven, niente sottoclasse C++ per eroe.

## 20. Action Economy

Punti chiusi:
- `Attack OR Ability OR Overwatch`;
- Sprint è Move profile.

Non inventare:
- costi MP/AP;
- Guard/Brace cost;
- Activate/Interact cost;
- Sprint extra slot;
- cooldown/charges del Basic Attack.

## 21. Scenario Registry

- `ACTION-001` Wait Is Explicit
- `ACTION-002` Basic Attack Primary
- `ACTION-003` Basic Attack Setup
- `ACTION-004` Guard vs Brace
- `ACTION-005` Activate Device
- `ACTION-006` Interact Objective
- `ACTION-007` Overwatch Opportunity Cost
- `MOVE-001` Normal Microsteps
- `MOVE-002` Sneak vs Sprint Noise
- `MOVE-003` Sprint Is Not Dash
- `MOVE-004` Manual Path Choice
- `MOVE-005` Forced Movement
- `MOVE-006` Teleport Semantics
- `FACE-001` Linear Facing
- `FACE-002` Budget Move Facing Choice
- `FACE-003` Stationary Free Rotation
- `FACE-004` Forced Facing

## 22. Test

Core:
- stable IDs;
- deterministic TurnLog;
- Move micro-step;
- Sprint in Move phase;
- Dash in Dash phase;
- Forced Movement non consuma Move della vittima;
- Teleport senza step intermedi;
- Facing nello snapshot;
- deterministic Facing updates;
- Overwatch cone dal Facing;
- BasicAttack character-specific senza branch per eroe nel TurnManager.

Quando entra la rete:
- intenti team-only;
- nessun enemy path/target/action leak.

## 23. Cleanup chat

Dopo integrazione nel canone diventano candidate ad Archive/Delete:
- Skill Move in RefactorTactics
- Focus attacco base
- Focus abilità Wait
- Skill comuni per personaggi
- Azioni base e varianti

Brace/Overwatch restano collegati al Reaction System Master.

## 24. Documenti da emendare

1. D-014 a 6 azioni -> riallineare se Decision Log conferma D-AUDIT-01.
2. Vecchie spec `Guard non universale` -> superseded.
3. Vecchie spec `Activate = Interact` -> superseded.
4. Sprint trattato come Dash -> correggere.
5. Basic Attack sempre debole -> superseded dal modello Primary/Engine/Setup/Utility.
6. Pivot per eroe -> proposal, non canone se ADR Facing dice altro.
7. Vecchio workbook balance -> non source of truth finché non riallineato.

## 25. Epic suggerite

### Generic Action Canonicalization
- reconcile D-014 vs D-AUDIT-01
- Action Catalog
- stable ID migration
- Wait/Guard/Brace/Activate/Interact semantics
- wiki + scenario coverage

### Movement Profiles
- Sneak/Normal/Sprint
- micro-step
- terrain/noise
- manual path UX

### Special Movement
- Dash/Charge/Leap/Blink
- teleport semantics
- forced movement taxonomy

### Facing v0.1
- snapshot
- move rules
- stationary rotation
- forced facing
- Overwatch cone
- tests

### Character Base Action Signature
- per-character Basic Attack
- Move/Guard/Brace/Overwatch identity
- interaction affinity
- false-choice playtest

## 26. Exit criteria

Il cluster è consolidato quando:
1. esiste una sola tassonomia current;
2. D-014 vs D-AUDIT-01 è risolto nel Decision Log;
3. Action Catalog e Wiki coincidono;
4. Sprint è ovunque Move profile;
5. Dash/special movement è separato;
6. Basic Attack è character-specific;
7. Guard e Brace sono distinti;
8. Activate e Interact sono distinti oppure una nuova decisione li unifica;
9. Facing è coerente con ADR;
10. gli scenari ACTION/MOVE/FACE sono registrati;
11. le vecchie chat possono essere eliminate senza perdita.
