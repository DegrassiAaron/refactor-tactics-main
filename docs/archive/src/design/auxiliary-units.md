# REFACTORTACTICS — AUXILIARY UNITS
## Pet, evocazioni, droni, torrette, gadget e unità aggiuntive controllate
### Specifica per Claude Code — consolidamento documentazione, codice, issue e roadmap

## Contesto

Stai lavorando nella repository **RefactorTactics**, tattico competitivo PC-first in **Unreal Engine 5** con turni simultanei, simulazione deterministica, planning privato per squadra, Fog of War e mappa tattica esagonale.

Prima di modificare codice o documentazione:

1. analizza la repository;
2. leggi `AGENTS.md`, `CLAUDE.md` e le istruzioni locali se presenti;
3. usa i documenti sotto `Docs/` come fonte di verità;
4. verifica la versione UE realmente bloccata nella repository;
5. individua classi, Data Asset, Gameplay Tags, resolver, intent e test già esistenti;
6. segnala conflitti tra questa specifica e l'implementazione corrente;
7. non creare sistemi paralleli se esiste già un'astrazione adatta;
8. non inventare API Unreal.

Baseline documentale corrente: **UE 5.8**, salvo diversa versione realmente bloccata nel repository.

---

# 1. OBIETTIVO

Introdurre una famiglia semplice e scalabile di **unità ausiliarie** create, evocate, deployate o controllate dai personaggi.

Esempi:

- pet;
- drone;
- torretta;
- summon temporanea;
- gadget deployabile;
- mini-robot/crawler;
- construct;
- proiezione/esca.

Queste entità devono aumentare:

- controllo dello spazio;
- visibilità;
- percezione;
- interazione con la mappa;
- combo;
- minacce previste;
- bluff;
- controllo di choke point;

senza trasformare RefactorTactics in:

- RTS;
- gioco con micro-management pesante;
- sistema dove un eroe con pet equivale automaticamente a due eroi completi.

Principio di design:

> Le Auxiliary Unit devono essere soprattutto strumenti di informazione, spazio, controllo e interazione ambientale. Il danno diretto è secondario e fa parte del power budget del personaggio proprietario.

---

# 2. CONCETTO UNIFICATO: AUXILIARY UNIT

Usare un concetto runtime unico, provvisoriamente:

`AuxiliaryUnit`

Non creare sistemi indipendenti per:

- Pet;
- Drone;
- Turret;
- Summon;
- Gadget;
- Construct.

La differenza deve essere prevalentemente **data-driven**.

Architettura concettuale:

```text
Character
   |
   +---- owns ----> AuxiliaryUnit
   |
   +---- issues ---> AuxiliaryIntent / AuxiliaryCommand
                          |
                          v
                     Turn Snapshot
                          |
                          v
                     Action Resolver
                          |
                          v
                       TurnLog
```

Una Auxiliary Unit deve poter partecipare alla simulazione logica senza diventare necessariamente un personaggio completo.

---

# 3. PRINCIPIO DI ACTION ECONOMY

Regola fondamentale:

> Un'Auxiliary Unit NON genera normalmente un secondo turno completo per il proprietario.

Evitare:

```text
Hero:
- Move
- Attack
- Ability

Pet:
- Move
- Attack
- Ability

Turret:
- Attack
```

come baseline.

Questo produrrebbe:

- moltiplicazione dell'action economy;
- aumento enorme della complessità del planning;
- vantaggio intrinseco ai personaggi con summon;
- maggiore durata dei turni;
- UI congestionata;
- difficoltà di bilanciamento.

Preferire invece:

```text
Hero
 |
 +-- Main Intent
 |
 +-- 0..1 Auxiliary Command
```

oppure Auxiliary Unit con comportamento già configurato.

Il suo output offensivo deve essere incluso nel **power budget complessivo del kit**.

Esempio:

```text
Engineer
Main attack: relativamente debole

+
Turret:
danno automatico limitato
```

NON:

```text
Engineer forte
+
Drone forte
+
Turret forte
```

---

# 4. ARCHETIPI SUPPORTATI

## 4.1 Pet

Esempio:

- animale;
- bestia cybernetica;
- compagno robotico.

Caratteristiche:

- segue il proprietario;
- può essere mandato verso una zona;
- può percepire;
- può fare una semplice azione automatica;
- eventualmente può essere targetable.

Uso ideale:

- scouting;
- detection;
- pressione ravvicinata;
- inseguimento;
- guardia.

---

## 4.2 Drone

Caratteristiche:

- movimento autonomo semplice;
- poca o nessuna capacità offensiva;
- estensione di LOS/perception;
- scan;
- interazione con dispositivi;
- spotting.

Uso ideale:

- reconnaissance;
- supporto;
- Fog of War;
- percezione acustica.

---

## 4.3 Turret

Caratteristiche:

- deploy;
- generalmente immobile;
- facing/arc;
- range;
- comportamento automatico;
- reaction-based fire.

Uso ideale:

- controllo choke point;
- area denial;
- Overwatch automatica;
- forcing di percorsi.

---

## 4.4 Summon

Caratteristiche:

- durata limitata;
- HP limitati;
- comportamento deterministico;
- 0 abilità proprie nell'MVP;
- un solo Command Target / Command Mode.

Uso ideale:

- body pressure;
- blocco area;
- inseguimento;
- sacrificio;
- forcing.

---

## 4.5 Gadget

Esempi:

- mina;
- sonar;
- noise maker;
- beacon;
- sensore;
- trappola.

Caratteristiche:

- generalmente immobile;
- può non avere HP;
- trigger-based;
- spesso nessuna occupancy;
- può generare eventi.

---

## 4.6 Construct / Crawler

Esempio:

`Spider Bot`

Caratteristiche:

- movimento semplice;
- nessun attacco;
- interazione con:

  - porte;
  - generatori;
  - relay;
  - ascensori;
  - trappole;
  - valvole;
  - console.

Questo archetipo deve sfruttare il fatto che porte, ponti, tunnel e dispositivi della mappa sono parte del grafo tattico e delle transizioni logiche.

---

## 4.7 Projection / Decoy

Possibile estensione futura.

Esempi:

- clone olografico;
- falsa firma acustica;
- esca visiva.

Può:

- non occupare realmente una cella;
- non essere targetable come una unità normale;
- generare informazioni ingannevoli legittime.

NON implementare nella prima iterazione se aumenta lo scope.

---

# 5. CONTROLLO SEMPLICE

Per la prima versione supportare massimo tre comportamenti generali:

```text
FOLLOW
GUARD
COMMAND
```

## FOLLOW

L'Auxiliary Unit segue il proprietario.

Il comportamento deve essere deterministico.

Non usare steering real-time come autorità competitiva.

---

## GUARD

L'Auxiliary mantiene:

- una cella;
- una piccola zona;
- una direzione;
- un arco;

e applica il comportamento configurato.

Esempi:

- osserva;
- intercetta;
- spara;
- ascolta;
- segnala.

---

## COMMAND

Durante il Planning il giocatore fornisce un ordine semplice.

Possibili target:

- Cell;
- Unit;
- Direction;
- Interaction.

Esempio:

```text
Drone -> Scout H7
Pet -> Hunt Enemy_03
Crawler -> Hack Door_02
Turret -> Face NorthEast
```

---

# 6. NON CREARE UNA SECONDA ABILITY BAR

Default UX:

l'Auxiliary Unit NON deve presentarsi come un quinto/sesto personaggio completo.

Preferire che il personaggio proprietario controlli la propria Auxiliary tramite:

- ability;
- command mode;
- piccolo pannello companion;
- target contestuale.

Esempio:

```text
ENGINEER

Move
Attack
Ability 1
Ability 2

Companion:
[ Follow ]
[ Guard ]
[ Command ]
```

oppure:

```text
Deploy Drone
Scout Command
Recall
```

Queste azioni devono essere parte del kit e del suo bilanciamento.

---

# 7. BASELINE AUXILIARY UNIT v0.1

Usare come baseline iniziale:

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

Targetable:                  configurabile

HP:                          configurabile

Movement:                    configurabile

Occupancy:                   configurabile

Duration:                    configurabile

Visible:                     SI secondo perception rules

Audible:                     SI secondo noise rules

Intent privacy:              team-only / server-only come gli intenti normali
```

Default consigliato:

```text
Occupancy = NonBlocking
```

salvo archetipi che dichiarano esplicitamente il contrario.

---

# 8. PRIMO PROTOTIPO CONSIGLIATO: SCOUT DRONE

Creare come primo caso di test un drone semplice.

Nome provvisorio:

`Mite`

Valori esclusivamente iniziali:

```text
HP:        30
Move:      3 hex
Attack:    none
Duration:  persistent until destroyed/removed
Occupancy: non-blocking
```

Comandi:

```text
FOLLOW
SCOUT <Cell>
WATCH <Direction>
```

Funzioni:

- estende la capacità di osservazione;
- può migliorare detection;
- può rilevare rumori;
- può fornire un punto di visione;
- può eventualmente interagire con dispositivi;
- può essere distrutto se il design lo richiede.

NON dare al drone attacco nell'MVP.

Obiettivo del prototipo:

validare Auxiliary Unit senza introdurre contemporaneamente un problema di bilanciamento offensivo.

---

# 9. SECONDO PROTOTIPO CONSIGLIATO: TURRET

Esempio iniziale:

```text
Deploy Turret

Move:      0
HP:        30
Range:     4
Arc:       directional
Charges:   1 / turn
```

Durante Planning:

```text
Deploy
Cell = H7
Facing = NE
```

Durante Resolution:

```text
Enemy enters valid arc
        |
        v
Reaction Opportunity
        |
        v
Turret policy
        |
        v
Commit
        |
        v
Shot
```

La Turret NON deve avere un sistema Overwatch speciale.

Riutilizzare il sistema generale già previsto:

```text
Reaction
+
Trigger
+
Reaction Opportunity
+
Policy
+
Resolution
```

Policy iniziale possibile:

`FireFirstValid`

Nessuna Fast Reaction manuale richiesta per l'MVP della Turret.

---

# 10. SUMMON SEMPLICE

Non creare creature con ability bar propria.

Esempio:

```text
Summon Hound

Lifetime: 3 turns
HP:       35
Move:     2
Attack:   12 melee
```

Comandi:

```text
HUNT <UnitId>
GUARD <Cell>
FOLLOW
```

Il comportamento autonomo deve usare scoring/ordinamento deterministico.

Esempio:

```text
LegalTargets
    |
sort by
    |
CommandPriority
Distance
TargetPriority
StableUnitId
    |
    v
SelectedTarget
```

Non dipendere da:

- TMap iteration order;
- frame rate;
- animation timing;
- random globale.

---

# 11. INTERAZIONE CON FOG OF WAR E PERCEPTION

Un'Auxiliary Unit deve poter contribuire alla conoscenza della squadra.

Esempi:

## Scout Drone

```text
Drone vede Enemy
    |
    v
Team Knowledge
```

## Pet sensoriale

```text
Pet rileva Noise
    |
    v
Team Knowledge
```

IMPORTANTE:

L'Auxiliary NON deve bypassare il sistema di perception.

Deve essere trattata come una nuova sorgente/sensore autorizzato.

Separare:

- geometria LOS;
- Visibility;
- Detection;
- Awareness;
- Hearing.

---

# 12. INTERAZIONE CON IL SISTEMA RUMORE

Le Auxiliary Unit possono essere sia:

- sorgenti di rumore;
- sensori acustici;
- creatori di falsi rumori.

Esempi:

```text
Pet:
HearingThreshold basso
```

```text
Drone:
NoiseGeneration basso
```

```text
Crawler:
Noise.Movement.Mechanical
```

```text
Noise Maker Gadget:
genera Noise.Decoy
```

Esempio tattico:

```text
Drone entra nel tunnel

Noise detected:
Intensity 7
Direction NE

Enemy not visible
```

La squadra può ottenere:

```text
Possible Enemy Contact
```

NON posizione o intento nemico non autorizzato.

---

# 13. INTERAZIONE CON LA MAPPA

Le Auxiliary Unit devono usare gli stessi servizi spaziali del gioco.

Movement:

```text
FRTCellId
+
Graph
+
Unit Movement Profile
+
A*
```

NON NavMesh come autorità.

Possibili profili:

```text
MovementProfile.Human
MovementProfile.Drone
MovementProfile.Crawler
MovementProfile.Beast
```

Un drone potrebbe in futuro:

- attraversare alcune transizioni non disponibili agli umani;
- ignorare terrain movement cost;
- non attraversare roof/ceiling blocker;
- usare layer dedicati.

NON introdurre subito movement 3D speciale se il vertical slice non è pronto.

Prima iterazione:

usa lo stesso Layer e le stesse celle normali.

---

# 14. INTERAZIONE CON PORTE, PONTI E DISPOSITIVI

Construct e Crawler devono poter produrre normali interaction intent.

Esempio:

```text
SpiderBot
  |
  v
Interact Door_04
  |
  v
Resolver
  |
  v
Graph Edge Enabled = false
```

La modifica deve:

- aggiornare lo stato logico;
- produrre TurnEvent;
- aggiornare GraphRevision;
- invalidare cache path se necessario;
- essere visibile nel TurnLog.

NON modificare direttamente mesh/Actor come autorità.

---

# 15. DETERMINISMO

Le Auxiliary Unit fanno parte dello stato logico.

Devono avere Stable ID.

Esempio concettuale:

```text
OwnerId
SpawnSequence
AuxiliaryDefinitionId
```

NON usare solamente Actor pointer o runtime memory address come identità.

Lo snapshot deve includere gli Auxiliary state necessari alla Resolution.

Esempio concettuale:

```cpp
FRTAuxiliaryUnitState
{
    StableUnitId;
    DefinitionId;
    OwnerUnitId;
    TeamId;
    CellId;
    Facing;
    Hp;
    LifetimeRemaining;
    CommandMode;
    CommandTarget;
    Status;
}
```

Il nome e la struttura reali devono aderire ai tipi già presenti nella repository.

Non creare questa struct se `FRTUnitState` può già rappresentare correttamente l'entità tramite categoria/profile.

Valutare prima la soluzione più semplice:

## Preferenza

Se possibile:

```text
FRTUnitState
+
UnitKind = Character / Auxiliary
```

oppure equivalente.

Evitare gerarchie duplicate di stato logico.

---

# 16. SPAWN E DESPAWN

Spawn e despawn devono essere eventi logici.

Esempi:

```text
AuxiliarySpawned
AuxiliaryDestroyed
AuxiliaryExpired
AuxiliaryRecalled
```

Lo spawn deve dichiarare:

- Stable ID;
- Definition ID;
- owner;
- Team;
- Cell;
- source ActionId.

Lo spawn NON deve essere deciso da una Animation Notify.

L'Actor visuale viene creato/rimosso come conseguenza della simulazione.

---

# 17. PRIVACY NETWORKING

Gli intenti delle Auxiliary Unit seguono esattamente le regole di privacy del planning.

Esempio:

```text
Owner plans:

Drone -> H7
```

Il server può conoscere il piano.

Gli alleati autorizzati possono ricevere:

- path;
- destination;
- command;
- target;
- facing;

tramite DTO team-only sanitizzati.

Gli avversari NON devono ricevere:

- future drone path;
- destination;
- command;
- target;
- future turret facing;
- summon target.

NON replicare intenti Auxiliary su:

- GameState globale;
- PlayerState globale;
- AlwaysRelevant Actor;
- replicated auxiliary Actor accessibile agli avversari.

Il fatto che un Auxiliary Actor sia visibile durante la Resolution NON autorizza a replicare il suo planning futuro.

---

# 18. AUXILIARY E REACTION SYSTEM

Le Auxiliary Unit possono utilizzare il sistema Reaction, ma senza introdurre una seconda pipeline.

Esempi:

```text
Turret
Trigger = EnemyEnterArea
Policy = FireFirstValid
```

```text
Guard Drone
Trigger = EnemyNoiseDetected
Policy = MarkContact
```

```text
Pet
Trigger = EnemyEnterAdjacent
Policy = AlertOwner
```

Utilizzare:

```text
Reaction Definition
+
Reaction Opportunity
+
Deterministic Policy
+
Resolver
```

Le Auxiliary automatiche NON devono normalmente aprire una Fast Reaction Window al giocatore.

Una eventuale Auxiliary manuale con Fast Reaction è estensione successiva.

---

# 19. SIMULTANEITÀ

Caso:

due nemici entrano nello stesso arco della Turret durante lo stesso micro-step.

NON scegliere in base all'ordine di iterazione.

Applicare la stessa regola delle Reaction Opportunity simultanee:

```text
Collect simultaneous valid targets
        |
        v
Stable ordering
        |
        v
Policy selection
```

Esempio:

```text
TargetPriority
Distance
StableUnitId
```

La policy esatta deve essere data-driven e bloccata nel ruleset competitivo.

---

# 20. UI / UX

Le Auxiliary non devono saturare la mappa.

Usare:

- piccolo badge sul roster del proprietario;
- iconografia diversa dalle unità principali;
- ghost path solo quando owner/Aux è selezionato;
- filtri;
- focus;
- opacity ridotta per preview secondarie.

Possibile HUD:

```text
NYX
HP 82

Companion:
MITE [ACTIVE]

Mode:
WATCH

[Recall]
[Command]
```

Intenti alleati:

```text
Drone path      -> team-only ghost
Turret facing   -> directional ghost
Pet guard zone  -> area ghost
```

Applicare sempre:

- Confermato;
- Previsto;
- Incerto.

I warning non devono derivare dal planning nemico.

---

# 21. DATA-DRIVEN

Le Auxiliary Definition devono seguire gli stessi principi dei contenuti competitivi.

Valutare un Primary Data Asset:

`URTAuxiliaryUnitDefinition`

solo se non esiste già una definizione di unità generalizzabile.

Campi concettuali:

```text
AuxiliaryId
Version
Tags
UnitKind
MovementProfileId
MaxHP
Lifetime
OccupancyPolicy
Targetable
VisionProfile
HearingProfile
NoiseProfile
CommandModes
ReactionProfile
BehaviorPolicyId
```

NON implementare campi non utilizzati.

Prima iterazione deve essere minima.

---

# 22. GAMEPLAY TAGS SUGGERITI

Integrare il vocabolario governato, senza creare tag duplicati.

Esempi concettuali:

```text
Unit.Auxiliary
Unit.Auxiliary.Pet
Unit.Auxiliary.Drone
Unit.Auxiliary.Turret
Unit.Auxiliary.Summon
Unit.Auxiliary.Gadget
Unit.Auxiliary.Construct

Command.Follow
Command.Guard
Command.Move
Command.Hunt
Command.Interact

Event.Auxiliary.Spawned
Event.Auxiliary.Destroyed
Event.Auxiliary.Expired
Event.Auxiliary.Commanded

Ability.Deploy
Ability.Summon
Ability.Command
Ability.Recall
```

Prima controllare i Gameplay Tags esistenti.

---

# 23. VALIDATOR

Aggiungere validation rules quando il framework viene implementato.

Possibili errori:

```text
Auxiliary Definition senza ID
Owner limit < 0
Unknown MovementProfile
Unknown BehaviorPolicy
Unknown ReactionProfile
Lifetime < 0
Invalid command mode
Spawn su cella invalida
Blocking Auxiliary senza occupancy policy
Reaction con policy inesistente
```

Possibili regole competitive:

```text
MaxActiveAuxiliaryPerOwner <= Ruleset limit
```

---

# 24. TURNLOG

Non creare log parallelo.

Estendere il normale TurnLog.

Eventi minimi:

```text
AuxiliarySpawned
AuxiliaryCommanded
AuxiliaryMoveStep
AuxiliaryReactionTriggered
AuxiliaryInteraction
AuxiliaryDestroyed
AuxiliaryExpired
AuxiliaryRecalled
```

Se gli eventi generali già esistenti supportano `UnitId`, preferire:

```text
MoveStep
DamageApplied
InteractionResolved
UnitKO
```

con `UnitKind=Auxiliary`.

Aggiungere eventi Auxiliary-specific solo quando portano semantica reale.

---

# 25. TEST AUTOMATICI

Questa feature è Done solo se include test.

## Core deterministic test

Scenario:

```text
Owner spawns Drone
Drone receives Scout command
Drone moves 3 cells
```

Assert:

- Stable ID;
- final Cell;
- same TurnLog;
- same StateHash;
- same LogHash.

Ripetere molte volte con stesso input.

---

## Action economy test

Assert:

```text
Owner cannot submit:
full normal action
+
multiple independent Auxiliary actions
```

oltre il budget previsto dal Ruleset.

---

## Turret reaction test

Scenario:

```text
Turret guarding arc
Enemy A enters
```

Assert:

```text
ReactionOpportunity created
policy commits
charge consumed
damage/event correct
```

---

## Simultaneous target test

Scenario:

```text
Enemy A
Enemy B

enter same arc
same micro-step
```

Assert:

selection independent from container iteration order.

---

## Privacy test

Inserire canary nel planning Auxiliary:

```text
AuxPathCanary_12345
```

Assert:

non appare sui client avversari durante Planning.

---

## FoW test

Enemy Drone fuori LOS:

- non mostrare posizione;
- non mostrare command;
- non mostrare destination.

Dopo evento percepibile:

mostrare solo informazione autorizzata.

---

## Lifetime test

Summon:

```text
Lifetime = 3 turns
```

Assert despawn deterministicamente al boundary corretto.

---

## Graph interaction test

Crawler:

```text
Interact Door
```

Assert:

- edge changes state;
- GraphRevision increments;
- path cache invalidated;
- TurnLog event generated.

---

# 26. PERFORMANCE BUDGET

Non assumere roster enormi.

Target iniziale:

```text
MaxAuxiliaryPerOwner = 1
```

Per 4v4 worst-case iniziale:

```text
8 Characters
+
8 Auxiliary
=
16 logical moving entities
```

Il resolver deve rimanere nei budget esistenti.

Non creare:

- Tick costosi;
- Behavior Tree per ogni Auxiliary come autorità;
- NavMesh agent per simulazione;
- Actor-heavy simulation.

La presentazione può usare Actor.

La simulazione deve usare stato compatto.

---

# 27. ISSUE DA CREARE

Analizza prima issue e milestone esistenti e non duplicare ticket.

Se non esistono, creare issue equivalenti alle seguenti.

## ISSUE A — Auxiliary Unit design/data model

Titolo indicativo:

`feat(aux): add shared auxiliary unit model`

Scope:

- identificare riuso di `FRTUnitState`;
- definire UnitKind;
- owner;
- stable ID;
- lifetime;
- Data Definition minima;
- serialization;
- validator.

Acceptance:

- unit state rappresenta Character e Auxiliary;
- snapshot deterministico;
- zero Actor pointer nel resolver;
- automation test.

---

## ISSUE B — Auxiliary Command Intent

Titolo:

`feat(planning): add auxiliary command intents`

Scope:

- Follow;
- Guard;
- Command;
- owner validation;
- max commands;
- team preview DTO.

Acceptance:

- command passa nella pipeline Planning -> Commit -> Snapshot;
- team-only preview;
- no enemy leak.

---

## ISSUE C — Auxiliary spawn/despawn

Titolo:

`feat(resolver): resolve auxiliary spawn and lifetime`

Scope:

- spawn;
- destroy;
- recall;
- expire;
- TurnLog.

Acceptance:

- deterministic Stable ID;
- lifetime test;
- replay-safe.

---

## ISSUE D — Scout Drone prototype

Titolo:

`feat(aux): prototype scout drone`

Scope:

- no attack;
- movement;
- Follow;
- Scout;
- Watch;
- LOS/perception integration minima.

Acceptance:

- test automatico;
- playable in DevSandbox;
- visible ghost path;
- no enemy intent leak.

---

## ISSUE E — Turret prototype

Titolo:

`feat(aux): prototype reaction turret`

Scope:

- deploy;
- facing;
- arc;
- Reaction Opportunity;
- automatic policy;
- 1 charge.

Acceptance:

- no custom reaction pipeline;
- simultaneous target test;
- TurnLog explanation.

---

## ISSUE F — Auxiliary UI

Titolo:

`feat(ui): add auxiliary planning controls`

Scope:

- owner companion panel;
- command selector;
- ghost path/guard arc;
- filter/focus.

Acceptance:

- no second full roster entry by default;
- team intent readable;
- Confirmed/Predicted/Uncertain respected.

---

## ISSUE G — Auxiliary privacy test

Titolo:

`test(net): prevent auxiliary intent leaks`

Scope:

- path;
- destination;
- command;
- target;
- facing;
- canary capture.

Acceptance:

- zero canary on enemy connection in packaged network test.

---

## ISSUE H — Auxiliary functional scenarios

Titolo:

`test(aux): add automated auxiliary gameplay scenarios`

Scenarios:

```text
Aux.Drone.BasicMove
Aux.Drone.Follow
Aux.Turret.SingleTrigger
Aux.Turret.SimultaneousTargets
Aux.Summon.Lifetime
Aux.Crawler.DoorInteraction
```

Implementare soltanto quelli supportati dallo scope raggiunto.

---

# 28. ROADMAP — INTEGRAZIONE

Non anticipare questa feature nelle Fondazioni F0.

La dipendenza minima è:

```text
F0 Fondazioni
    |
    v
F1 Networking/privacy foundation
    |
    v
F2 Ability / resolver framework
    |
    v
F2.A Auxiliary Foundation
    |
    v
F2.B Scout Drone prototype
    |
    v
F2.C Turret prototype
    |
    v
F3 Map multilayer / advanced interaction
    |
    v
F4 Vertical Slice integration
```

Se la roadmap corrente non permette sub-milestone, integrare i task dentro F2/F3/F4.

## F2 — Abilities

Aggiungere:

- Auxiliary logical state;
- deploy/summon/command;
- Stable ID;
- snapshot;
- resolver;
- TurnLog;
- action economy;
- Scout Drone;
- Turret Reaction.

Exit gate:

```text
Drone + Turret funzionano deterministicamente
in scripted scenario
```

---

## F3 — Map multilayer

Aggiungere:

- movement profile Auxiliary;
- porte;
- dispositivi;
- crawler interaction;
- eventuale drone layer/transitions solo se necessario;
- graph revision interaction.

Exit gate:

```text
Auxiliary uses authoritative graph
and correctly invalidates graph state
```

---

## F4 — Vertical Slice

Aggiungere solo se il gameplay lo giustifica:

- almeno un personaggio con Auxiliary;
- UX completa;
- FoW/perception;
- balance pass;
- bot support;
- automated scenario;
- packaged network privacy test.

NON rendere obbligatorio un personaggio summoner nel vertical slice se introduce troppo scope.

---

# 29. PRIORITÀ IMPLEMENTATIVA

Ordine consigliato:

```text
1. Shared Unit model
2. Owner relation
3. Auxiliary Command Intent
4. Snapshot / Resolver
5. Spawn / Despawn
6. Scout Drone
7. Privacy
8. UI
9. Turret
10. Map interaction
11. Summon
```

Il primo obiettivo NON è creare molti pet.

Il primo obiettivo è dimostrare che il modello architetturale funziona.

---

# 30. PERSONAGGI E KIT FUTURI

Brainstorm da documentare, non implementare subito.

## Engineer / Handler

Companion:

`Mite Drone`

Gameplay:

- scouting;
- Watch;
- interaction;
- detection.

---

## Beast Handler

Pet:

- segue;
- fiuta;
- marca;
- intercetta adiacenza.

Power budget spostato dal danno personale al controllo del pet.

---

## Engineer

Turret:

- deploy;
- orientamento;
- Overwatch automatico.

Counterplay:

- flank;
- distruzione;
- smoke;
- stealth;
- LOS break.

---

## Hacker

Spider Bot:

- porte;
- relay;
- generatori;
- ascensori;
- noise decoy.

---

## Summoner

Creature temporanea:

- Follow;
- Guard;
- Hunt;
- lifetime breve.

Evitare multiple creature simultanee nella prima versione.

---

# 31. DESIGN RULES DA CONSOLIDARE

Aggiungere alla documentazione come decisioni di progetto, se compatibili con la repo:

1. Auxiliary Unit è il concetto condiviso per pet, drone, summon, turret e construct.
2. Default max 1 Auxiliary attiva per owner.
3. Nessuna ability bar indipendente nell'MVP.
4. Nessun Ready indipendente.
5. Nessun planning timer indipendente.
6. Nessuna capture objective di default.
7. Auxiliary commands consumano budget definito dal personaggio/ruleset.
8. Stable ID e snapshot obbligatori.
9. Resolver autoritativo.
10. Actor solo presentazione.
11. Intenti Auxiliary sono privati come gli intenti Character.
12. Reactions riusano il sistema generale Reaction Opportunity.
13. Auxiliary perception passa dal Team Knowledge.
14. Path usa grafo/FRTCellId.
15. Auxiliary non bypassa LOS/FoW.
16. Stesso snapshot + seed + rules + commands = stesso risultato.
17. Nessun comportamento competitivo deciso da Tick/animation/BehaviorTree.
18. Feature Done solo con test automatico, TurnLog, privacy e packaged verification.

---

# 32. DOCUMENTAZIONE DA CONSOLIDARE

Individuare i documenti reali della repository e aggiornare quelli appropriati.

Come minimo verificare impatti su:

## Architecture

Aggiungere:

- Auxiliary unit ownership;
- runtime state;
- command flow;
- dependency boundaries.

## Simulation / Determinism

Aggiungere:

- Auxiliary state;
- spawn/despawn;
- stable ordering;
- automatic behavior policy.

## Networking / Privacy

Aggiungere:

- Auxiliary intents;
- team-only DTO;
- enemy leak tests.

## Map / Pathfinding

Aggiungere:

- movement profiles;
- occupancy;
- interaction with edges/devices.

## Characters / Abilities / GAS

Aggiungere:

- deploy/summon/command abilities;
- power budget;
- no second action economy.

## UI/UX

Aggiungere:

- companion panel;
- ghost path;
- guard arc;
- visibility states.

## Data / Validation

Aggiungere:

- Auxiliary definition;
- Stable ID;
- ruleset max count;
- validator.

## Roadmap / QA

Aggiungere:

- milestone placement;
- automated tests;
- performance budget;
- Definition of Done.

Valutare inoltre un nuovo documento dedicato:

```text
Docs/Design/AuxiliaryUnits.md
```

o percorso coerente con la repository.

La documentazione principale deve però essere consolidata: non lasciare decisioni importanti soltanto nel nuovo file.

---

# 33. CODICE — PRINCIPIO DI IMPLEMENTAZIONE

NON implementare automaticamente tutte le classi seguenti.

Prima verificare cosa esiste.

Possibili concetti:

```text
ER TUnitKind
ER TAuxiliaryType
ER TAuxiliaryCommandMode

FRTAuxiliaryCommand
URTAuxiliaryUnitDefinition
FRTAuxiliaryBehaviorPolicy
```

Preferire estensione dei tipi esistenti rispetto a duplicazione.

Esempio:

se esiste:

```cpp
FRTUnitState
```

valutare:

```cpp
UnitKind
OwnerUnitId
```

anziché:

```cpp
FRTAuxiliaryUnitState
```

completamente separato.

Lo stesso vale per:

- Move Intent;
- Targeting;
- Damage;
- Status;
- KO;
- TurnEvent;
- LOS;
- Pathfinding.

Auxiliary deve riusare il più possibile il gameplay framework.

---

# 34. GIT / COMMIT PLAN

Proporre commit piccoli e focalizzati.

Esempio:

```text
docs(aux): define auxiliary unit design and roadmap
feat(unit): support auxiliary unit ownership
feat(planning): add auxiliary command intents
feat(resolver): add auxiliary spawn and lifetime
test(aux): add deterministic auxiliary scenarios
feat(aux): add scout drone prototype
test(net): cover auxiliary intent privacy
feat(aux): add reaction turret prototype
feat(ui): add auxiliary planning controls
```

Non mischiare tutto in un unico mega-commit.

---

# 35. OUTPUT RICHIESTO A CLAUDE

Dopo l'analisi della repository, produrre prima un piano sintetico con:

```text
Current state
Conflicts found
Docs affected
Code affected
Tests affected
Issues affected
Roadmap placement
Implementation order
```

Poi procedere alle modifiche.

Alla fine riportare:

```text
Files created
Files modified
Issues created/updated
Milestones updated
Tests added
Tests executed
Build result
Known limitations
Next recommended issue
```

---

# 36. NON OBIETTIVI

NON introdurre ora:

- squadre di summon;
- inventario di pet;
- breeding;
- pet progression;
- skill tree pet;
- pet equipment;
- Behaviour Tree complessi;
- EQS come autorità;
- NavMesh authority;
- possession diretta del pet;
- multi-selection RTS;
- 4 abilità per summon;
- objective capture da summon;
- chain di summon che evocano altre summon;
- replication globale dei loro intenti;
- Fast Reaction manuale indipendente per ogni minion.

Mantenere lo scope rigoroso.

---

# 37. CRITERIO DI SUCCESSO

La feature foundation è riuscita quando è possibile dimostrare:

```text
Character
   |
Deploy Scout Drone
   |
Command Drone
   |
Planning
   |
Commit
   |
Snapshot
   |
Deterministic Resolution
   |
TurnLog
```

con:

- stesso output a parità di input;
- nessun leak dell'intento al team nemico;
- UI leggibile;
- stesso grafo/pathfinding;
- test automatico;
- packaged verification.

Solo dopo questa prova aggiungere:

- Turret;
- Crawler;
- Summon;
- Pet più complessi.

---

# DECISIONE DI DESIGN CENTRALE

Le Auxiliary Unit NON devono essere semplicemente:

> "un altro personaggio che spara".

Devono permettere ai character di controllare indirettamente:

- spazio;
- informazione;
- percezione;
- choke point;
- dispositivi;
- rumore;
- percorsi;
- pressione.

Questa è la direzione che meglio si integra con i pilastri di RefactorTactics:

```text
MAPPA
+
INFORMAZIONE
+
PREDIZIONE
+
SIMULTANEITÀ
+
COORDINAZIONE
```

mantenendo la complessità sotto controllo.
