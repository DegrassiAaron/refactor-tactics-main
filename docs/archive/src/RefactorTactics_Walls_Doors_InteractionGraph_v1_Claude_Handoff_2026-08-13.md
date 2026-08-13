> 🗄️ **ARCHIVIATO il 2026-08-14 — consumato.** Questo è un **sorgente**, non un owner: si legge per la
> provenienza, mai per la regola. Il contenuto recepito vive ora nelle fonti canoniche. Referto del filtro:
> [`walls-doors-interaction-spec-panel-2026-08-13.md`](../../roadmap/plans/walls-doors-interaction-spec-panel-2026-08-13.md).
>
> **Cosa è entrato** — [D-138](../../decisions/RT_PDR_00_Decision_Log.md). L'epic **E23** ([#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324))
> resta l'unico owner del dominio e ha ora **sette sub-issue** collegate; sono nate
> [#832](https://github.com/DegrassiAaron/refactor-tactics-main/issues/832) (identità stabile della struttura),
> [#833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/833) (interaction graph `1→1` e `1→N`) e
> [#834](https://github.com/DegrassiAaron/refactor-tactics-main/issues/834) (leggibilità `S1`/`D1`), con le tre
> feature che le tracciano. L'orizzonte del dominio oltre la v0.2 è **innestato** sulle epic esistenti in
> [`roadmap-post-v0.1.md`](../../roadmap/roadmap-post-v0.1.md) §E23. Le due domande che restano aperte sono
> `INT-5` e `INT-6` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).
>
> **Cosa NON è entrato, e perché.** 🔴 **Le §7.B, §7.D, §7.G e §25 danno per mancante ciò che ha test verdi
> dalla v0.1.** La porta multi-transition **esiste**: `FRTHexDoor::DoorId` raggruppa i bordi, `SetDoorState`
> li muta insieme incrementando la revisione una sola volta. Due test lo dimostrano:
> `Structures.Door.GroupClosesTogether` prova il **raggruppamento** (tre bordi di una cella, `DoorId 3`) e
> `Structures.Door.StateChangeBumpsRevision` la **porta larga** — tre **celle** sul bordo E con `DoorId 7`, un
> comando e una revisione — cioè quella che la §4.3 propone come obiettivo. Con
> essa: invalidazione della path cache, ordine indipendente e determinismo dell'hash. Applicare quelle sezioni
> avrebbe **rotto** i test che le dimostrano già. Il delta reale è l'**identità** (`23.3`), non il gruppo.
>
> ⚠️ **La §5 e la §18 erano superate di sette ore.** `RELEASE_ORDER` non era fermo a `v0.4`:
> [D-136](../../decisions/RT_PDR_00_Decision_Log.md) lo aveva portato a `v1.0` il 2026-08-13, e le sei epic
> `E40`–`E45` esistevano già. I sette placeholder `E<ALLOCATE>` **non sono stati allocati**: sarebbero stati
> una seconda tassonomia sopra una appena canonizzata.
>
> ⚠️ **La ladder §12–§16 non è la taxonomy reale, e il documento stesso lo prevede** (*«rimappare se la
> taxonomy reale è diversa»*). `v0.6 Authoring` e `v0.7 Graph v2` non hanno owner: quelle release
> appartengono a `E41` (GAS) ed `E42` (dedicated server). Restano proposte — non si creano epic per simmetria,
> e la §31 lo vieta essa stessa.
>
> ⚠️ **Tre delle cinque decisioni della §27 non sono state aperte.** La distruzione era **già decisa**
> (`Destroyed` è terminale, tre punti nel codice più un test); `WidthCm` poggia su un modello a centimetri che
> non esiste, mentre il gameplay legge bordi — cioè già la «baseline raccomandata» dal documento; il power
> network non è posseduto da **nessuna release**, e una decisione aperta su un sistema senza owner resta
> aperta per sempre.
>
> ⚠️ **Undici dei quattordici scenari proposti non sono entrati nella Scenario Map**, e il criterio è
> l'oracolo: i cinque di privacy passerebbero **per assenza di rete** (il loro oracolo nasce con `E27` e
> diventa verificabile con `E40`), i tre di identità e validazione non sono scenari ma automation — nessuna
> delle otto `ERTAssertionKind` legge l'identità di una struttura — e i tre di scala misurano mappe che `E30`
> non ha ancora prodotto.
>
> ⚠️ **La §3 dichiara che il corpo di #324 contiene `E23.1`–`E23.7`**: era falso del corpo GitHub, che ne
> portava cinque, e vero dell'owner documentale. La verifica di quella riga ha trovato la deriva — e con essa
> due issue dichiarate aperte che erano chiuse.

---

# REFACTORTACTICS — MURI, PORTE E INTERACTION GRAPH FINO ALLA v1.0
## Handoff operativo per Claude Code / Claude Cloud

**Data:** 2026-08-13  
**Repository verificata:** `DegrassiAaron/refactor-tactics-main`  
**Branch canonico:** `main`  
**Baseline verificata:** `d40ccf63938cc39fc2d737680123436758c54867`

---

# 0. Missione

Devi lavorare direttamente sulla repository RefactorTactics e trasformare questo handoff in un piano canonico realmente tracciato per il dominio **muri / porte / strutture / interaction graph / leggibilità delle relazioni**, dall'attuale v0.1 fino alla v1.0.

Non limitarti a creare un documento. Devi:

1. fare audit di `main` e verificare che questa baseline non sia già stata superata;
2. leggere issue open/closed collegate a geometria, muri, porte, cover, interaction graph, editor, scenario, replay e rete;
3. aggiornare issue esistenti quando già possiedono il lavoro;
4. creare nuove issue solo per gap reali;
5. **non creare una seconda Epic parallela a E23 / #324**;
6. pianificare l'evoluzione del dominio fino alla v1.0;
7. estendere la roadmap canonica oltre v0.4 se una roadmap più recente non lo ha già fatto;
8. aggiornare il Feature Registry;
9. aggiornare Scenario Map, Editor Map, execution tracking, test manuali e Wiki dove applicabile;
10. rigenerare tutte le viste derivate tramite tooling;
11. eseguire validator e gate del repository;
12. produrre un report finale con issue create/modificate, roadmap diff, scenari e file toccati.

La repository corrente vince sempre su questo documento in caso di conflitto.

Non inventare API Unreal. Non usare gli handoff archiviati come source of truth.

---

# 1. Fonti di verità da leggere prima di modificare

Leggere `CLAUDE.md`, `AGENTS.md` e istruzioni equivalenti se presenti.

Poi almeno:

```text
docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
```

Owner tecnici/gameplay:

```text
docs/technical/spec-hex-geometry-authoring.md
docs/technical/spec-mappa-multilivello.md
docs/technical/scenario-map.md
docs/technical/scenari-validazione-visiva.md
docs/technical/spec-pointer-interaction.md
docs/gameplay/spec-interazioni-mappa-cp101.md
```

Se un path è stato rinominato, trovare il successore canonico.

Roadmap/tracking:

```text
docs/roadmap/roadmap-v0.1.md
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/feature-registry.yaml
docs/roadmap/feature-registry.md
docs/roadmap/execution-graph.yaml
docs/roadmap/execution-map.md
docs/roadmap/roadmap-editor.md
```

Tooling:

```text
scripts/feature_registry.py
```

Codice attuale, in particolare:

```text
Source/RefactorTactics/Map/RTGeometryGrammar.*
```

e i file reali che contengono:

```text
FRTGeometrySegment
URTGeometryGrammarLibrary::ValidateSegment
URTGeometryBakeLibrary::BakeCell
FRTHexCover
Graph/Transition state
MapState
TurnLog / ERTLogCategory
```

---

# 2. Baseline corrente da riconfermare

## 2.1 Griglia tattica e architettura sono separate

Il modello vecchio:

```text
1 muro = 1 lato dell'esagono
```

non è più valido.

Il modello corrente è:

```text
TACTICAL HEX GRID
        |
        v
posizioni / relazioni discrete

ARCHITECTURAL GEOMETRY
        |
        v
muri / muretti / aperture / porte

VALIDATION + BAKE / BINDING
        |
        v
dati canonici competitivi
```

La griglia non obbliga il mondo a essere un alveare.

## 2.2 Authority geometrica discreta

Verificare `FRTGeometrySegment` e la grammatica runtime corrente. La world geometry / polilinea deve restare derivata, non una seconda authority float.

## 2.3 Pipeline authoring

Baseline:

```text
geometria quantizzata
    |
    v
URTGeometryGrammarLibrary::ValidateSegment
    |
    v
URTGeometryBakeLibrary::BakeCell
    |
    v
FRTHexCover / bBlocksMovement / altri dati canonici
```

Dopo il bake il gameplay non deve chiedere alla mesh se una transizione è valida.

## 2.4 Rebake

`FRTHexCover::bGenerated` distingue i dati rigenerabili dal bake da quelli authored manualmente. Conservare idempotenza e provenance semantica senza trasformarla in gameplay.

## 2.5 Thin slice v0.1

Baseline verificata:

```text
Wall
LowWall
```

Il vecchio `VoidFootprint` è uscito dal thin slice dei bordi. Verificare decisioni più recenti prima di cambiare.

---

# 3. Epic e issue già esistenti — non duplicare

## Epic canonica

```text
#324
[EPIC v0.2] E23 · Muri, porte e interaction graph
```

E23 è l'owner di prodotto del sistema.

Non creare una nuova Epic “Walls”, “Doors” o “Interaction Graph” salvo prova documentata che E23 non possa possederla.

## E23 corrente

La baseline del corpo di #324 contiene:

```text
E23.1 Separazione geometria/logica
E23.2 Porta come oggetto logico unico
E23.3 Stable ID e binding
E23.4 Interaction graph
E23.5 Leggibilità
E23.6 Standability cotta da geometria
E23.7 La transizione è un dato
```

Verificare il corpo live prima di modificarlo.

## Authoring anticipato

Baseline verificata:

```text
#619 chiusa
#620 chiusa
#621 chiusa
#712 aperta/in corso
```

#712 possiede il gesto autore: draw, ghost, snap, transaction / Ctrl+Z. Non spostare porte runtime dentro #712.

Verificare inoltre lo stato attuale di:

```text
#622
#711
#687
```

oltre a ogni issue che oggi tratta:

- movement probe;
- workspace grid;
- geometry editor;
- standability;
- transition blocking;
- cover authoring;
- structure Stable ID.

---

# 4. Decisioni di design da conservare

## 4.1 Hex = posizione tattica, non piastrella architettonica

Un lato nominale dell'esagono può rappresentare circa 1,5 m, ma non significa che un muro fisico debba essere lungo esattamente un lato o seguirne il perimetro.

## 4.2 Muri

I muri possono essere rettilinei e attraversare la struttura della griglia. L'authoring usa le famiglie di direttrici ammesse dalla grammatica corrente; configurazioni a 90° sono possibili quando la grammatica le ammette. Non reintrodurre lo zig-zag dei bordi hex.

## 4.3 Porta larga ~3 m

Una porta larga non va modellata visivamente come due lati consecutivi di hex, perché diventerebbe angolare.

Deve essere:

```text
UN oggetto logico
UN Stable ID
UN solo stato atomico
1..N transition governate
```

Esempio concettuale:

```text
Door.D3
Width ≈ 300 cm
State = Open | Closed
AffectedTransitions = [T1, T2]
```

Il player vede `D3`, non `D3A` + `D3B`, salvo che siano davvero due porte indipendenti.

## 4.4 Runtime legge stato logico

`MapState / TransitionState` è l'autorità competitiva; mesh/collisione sono rappresentazione, supporto editor e validation.

## 4.5 Interaction Graph

Il data model deve poter supportare almeno:

```text
1 Source -> 1 Target
1 Source -> N Targets
N Sources -> 1 Target
```

senza implementare subito circuiti booleani complessi.

## 4.6 UI

Il player deve poter capire:

```text
che cosa controlla S1?
come si apre D1?
```

Pattern:

```text
focus S1 -> evidenzia D1
focus D1 -> mostra controller noto S1
```

Usare tactical label, icon, connection overlay, testo, pattern e highlight. Mai solo colore.

## 4.7 Team planning

Caso:

```text
Ally A -> Interact S1 -> Open D1
Ally B -> Move/Attack through D1
```

La dipendenza di B deve usare la grammar corrente `Confermato / Previsto / Incerto` o i nomi canonici più recenti. Il preview non concede autorità.

## 4.8 Privacy / Knowledge

Non replicare tutto l'Interaction Graph ai client per poi nasconderlo. Il server può possedere il graph canonico; il team riceve il sottoinsieme autorizzato/known quando FoW/Knowledge rende le relazioni non pubbliche.

---

# 5. Roadmap proposta fino alla v1.0

## Nota fondamentale

Alla baseline verificata il repository pianifica esplicitamente:

```text
v0.1
v0.2
v0.3
v0.4
future
```

E `scripts/feature_registry.py` contiene:

```text
RELEASE_ORDER = ("v0.1", "v0.2", "v0.3", "v0.4", "future")
```

La parte v0.5 -> v1.0 di questo documento è quindi una **proposta da canonizzare**, non una verità già presente.

Prima di applicarla:

1. verificare se una roadmap più nuova ha già definito le release;
2. se sì, mappare il piano sulla taxonomy reale;
3. se no, adottare questa ladder o documentare una variante migliore;
4. aggiornare `RELEASE_ORDER`, validator, generator e relativi test nello stesso lavoro.

---

# 6. v0.1 — Fondazioni authoring, senza aprire E23

## Obiettivo

Consegnare l'authoring geometrico necessario affinché E23 possa arrivare senza rifare il sistema.

Non implementare l'intero Door/Interaction Graph in v0.1.

## Scope

Consolidare/chiudere:

- geometry grammar;
- authoring quantizzato;
- bake deterministico;
- generated vs manual cover;
- ghost valid/invalid;
- snap;
- transaction / Undo;
- workspace grid;
- Movement Probe / standability debug;
- visual validation;
- automation bake/grammar.

## Gate

```text
author gesture
-> valid discrete geometry
-> deterministic bake
-> canonical gameplay data
```

## Scenari da riconfermare

```text
Spec.Map.WallCrossesCellStillStandable
Spec.Map.FootprintCollisionBlocksCell
Spec.Map.NinetyDegreeCornerBakesCorrectly
```

Usare gli ID reali correnti.

---

# 7. v0.2 — E23: Core muri, porte e interaction graph

Questa è la release principale del dominio.

## Owner

```text
E23 / #324
```

## Risultato

Il battlefield passa da sola geometria cotta a strutture logiche dinamiche.

## Lavoro

### A. Logical Structure Model

Definire il modello canonico minimo per strutture necessarie: Wall, LowWall, Door, Gate, Barrier solo dove serve. Niente mega-taxonomy.

### B. Multi-transition Door

Una porta può governare `1..N` transition. Lo stato è atomico. Test esplicito della porta larga ~3 m.

### C. Stable IDs

Stable ID persistente attraverso cook, map serialization, scenario refs, TurnLog e replay. Binding duplicati/conflittuali = validation error.

### D. Transition State

Deve essere esprimibile:

```text
Cell A valid
Cell B valid
A -> B blocked
```

senza inventare una fake cover.

### E. Interaction Graph v1

Minimo:

```text
Switch -> Door
Switch -> N Doors
```

con ordine deterministico.

### F. Interact

L'intento dichiara `Interact SourceId`; il server/runtime risolve il mapping canonico. Il client non sceglie target interni non autorizzati.

### G. Revision / cache

```text
Structure state change
-> graph/structure revision
-> path cache invalidation
-> LOS/target revalidation
-> TurnLog
-> presentation
```

### H. UX v1

Implementare `S1`, `D1`, source->target, target->controller, stato Open/Closed e accessibilità.

### I. Explainability / replay

TurnLog canonico per interaction attempt, resolution/failure, structure state change, move blocked e reason code. Il Player non deve ricalcolare l'esito.

---

# 8. v0.2 — Issue da creare o aggiornare sotto E23

Prima verificare #324 e issue collegate. Creare solo gap mancanti.

Titoli descrittivi, senza assegnare numeri condivisi a memoria:

### Logical Structure Runtime Model

Acceptance:
- Stable ID;
- runtime non legge mesh;
- stato serializzabile;
- snapshot/replay-safe;
- test core.

### Door Multi-Transition Atomic State

Acceptance:
- standard door;
- wide/double door ~3 m;
- 1 logical DoorId;
- N transition;
- Open/Close atomico;
- deterministic hash.

### Transition State Independent from Cell Validity

Acceptance:
- A valid;
- B valid;
- A->B blocked;
- reason code;
- path test.

### Interaction Graph v1

Acceptance:
- 1->1;
- 1->N;
- deterministic target order;
- target validation;
- no Blueprint hard-reference as competitive authority.

### Interaction UX v1

Acceptance:
- S1/D1;
- source->target;
- target->controller;
- state;
- no color-only encoding.

### Structure / Interaction TurnLog & Replay

Acceptance:
- canonical event;
- replay reconstructs state;
- no client recalculation;
- permutation/repeat test.

### E23 Scenario Pack

Acceptance:
- core scenarios exist;
- Scenario Test Harness;
- visual scenario;
- machine-readable output.

Se questi scope esistono già, modificare le issue esistenti.

---

# 9. v0.2 — Scenari core

Riconfermare gli scenari già presenti:

```text
Spec.Map.ValidCellsBlockedTransition
Spec.Map.DoorOpensTransition
```

Aggiungere solo se mancanti e con naming reale:

```text
Spec.Map.Door.StandardOpenClose
Spec.Map.Door.WideMultiTransition
Spec.Map.Interaction.SwitchOpensDoor
Spec.Map.Interaction.SwitchControlsMultipleDoors
Spec.Map.Interaction.OpenFailsDependentMoveBlocks
Spec.Map.Interaction.OpenFireSeal
Spec.Map.Structure.BreachOpensRoute
Spec.Map.Structure.DeterminismRepeat
Spec.Map.Structure.PermutationStable
```

---

# 10. v0.3 — Knowledge, Fog of War e relazioni non pubbliche

La v0.3 canonica è già orientata all'informazione: integrare E23 con quel dominio.

## Obiettivo

Una relazione `S2 -> D7` può essere Public, Known to Team A, Unknown to Team B, Discovered later.

## Lavoro

- integrazione con TeamKnowledge corrente;
- authorized relationship DTO;
- `Controller: ???` quando appropriato;
- discovery Unknown -> Known;
- eventuale ghost/stale knowledge usando il sistema corrente;
- nessun hidden link replicato globalmente.

## Scenari

```text
Spec.Map.Interaction.HiddenControllerRelation
Spec.Map.Interaction.ControllerDiscovered
Spec.Map.Interaction.KnownToOneTeamOnly
Spec.Map.Interaction.NoHiddenRelationLeak
Spec.Map.Interaction.StaleKnowledgeDoesNotUpdate
```

Canary test appena il path network è disponibile.

---

# 11. v0.4 — Operations scale e multilayer

La v0.4 canonica porta mappe più grandi / Operations.

## Lavoro

- large gates con molte transition;
- bridge/elevator/lift/tunnel interaction usando lo stesso modello source->target;
- binding layer-safe;
- revisione locale/chunk dove possibile;
- performance di path cache churn, LOS revalidation, graph lookup, serialization e replay size;
- UX di focus/lookup su mappe grandi.

## Scenari

```text
Spec.Map.Gate.MultiTransitionLarge
Spec.Map.Bridge.RemoteControl
Spec.Map.Elevator.Interaction
Spec.Map.LayeredDoor.ValidTransition
Spec.Map.Structure.LocalRevisionInvalidatesOnlyAffectedQueries
Spec.Map.Operations.InteractionScale
```

---

# 12. v0.5 — Network Alpha / authoritative structures

**Proposta post-v0.4.** Rimappare se la taxonomy reale è diversa.

## Obiettivo

Portare Structure/Interaction sul server authoritative reale.

## Lavoro

- server canonical structure state;
- client request via owned RPC path;
- validation ownership/phase/range/source/operation;
- public vs team-only state;
- late join;
- reconnect;
- authoritative correction;
- dedicated server;
- replay dal risultato server.

## Privacy

Non replicare undiscovered SourceId, hidden target list, private control relation o future enemy interaction intent.

## Scenari

```text
Network.Structure.AuthoritativeDoor
Network.Interaction.InvalidSourceRejected
Network.Interaction.HiddenBindingCanary
Network.Structure.LateJoinState
Network.Structure.ReconnectState
Network.Structure.ServerClientReplayAgreement
```

---

# 13. v0.6 — Authoring / Content Alpha

**Proposta.**

## Obiettivo

Passare da “funziona in una mappa” a “si possono produrre mappe in modo affidabile”.

## Lavoro

- Structure Definition / data riutilizzabili;
- editor authoring per struttura, graph binding, aperture, door, label e controller;
- Validate / Preview / Bake;
- batch validators per duplicate Stable ID, missing target, cross-layer binding, conflicting controller, invalid door transition, generated/manual provenance, uncooked refs, hash instability;
- schema/version migration solo quando necessaria;
- cook stability Editor -> Cook -> Packaged.

## Test

```text
Editor.Structure.AuthorBakeRoundTrip
Editor.Structure.RebakeIdempotent
Editor.Structure.ManualCoverPreserved
Editor.Door.StableIdSurvivesCook
Editor.Interaction.InvalidTargetFailsValidation
Editor.Interaction.DuplicateBindingFailsValidation
```

---

# 14. v0.7 — Interaction Graph v2 / systemic map control

**Proposta.** Non trasformare il gioco in un circuit simulator senza una decisione.

## Obiettivo

Estendere il graph solo dove crea gameplay sistemico utile.

Capacità possibili, da approvare prima:

```text
N Sources -> Target
Source -> N Targets
operation-specific target
lock / unlock
enable / disable
power dependency
conditional interaction
```

Prima di introdurre AND/OR, power graph o circuiti:
- cercare Decision Log;
- se assente aprire OPEN DECISION;
- decidere schema minimo;
- evitare boolean logic arbitraria nei Blueprint.

Possibili source/target solo se richiesti dal gameplay: wall switch, pressure plate, terminal, generator, remote control, ability/hack -> door/gate/bridge/elevator/hazard/light controller.

## Scenari

```text
Spec.Map.Interaction.MultipleControllersOneDoor
Spec.Map.Interaction.OperationSpecificTargets
Spec.Map.Interaction.PowerDependency
Spec.Map.Interaction.ControllerDisabled
Spec.Map.Interaction.AtomicMultiTargetOperation
```

Creare solo scenari supportati da decisioni adottate.

---

# 15. v0.8 — Beta: gameplay, bot, UX, accessibilità

**Proposta.**

## Gameplay

Playtestare choke point, door wide vs narrow, OPEN -> FIRE -> SEAL, enemy exploit, friendly dependency, Overwatch, breach e remote control.

## Bot

Il bot deve conoscere affordance e interaction graph consentiti, valutare open/close, rispettare TeamKnowledge e non usare canonical enemy-only info. Nessun `if Door` speciale: usare dati/affordance.

## UX

Source->target, target->source, focus, tactical labels, state icons, ally dependency, uncertainty, accessibility, clutter filtering.

## Scenari

```text
Bot.Interaction.OpenRoute
Bot.Interaction.CloseChoke
Bot.Interaction.DoesNotUseHiddenController
UI.Interaction.MultiTargetReadable
UI.Interaction.ColorBlindReadable
Planning.Interaction.DependsOnAlly
```

---

# 16. v0.9 — Release Candidate / hardening

**Proposta.** Nessun nuovo grosso sistema.

## Gate

Determinismo:
- repeat;
- permutation;
- replay comparison;
- hash stability.

Network:
- dedicated soak;
- reconnect;
- late join;
- privacy canary.

Performance:
- Operations map;
- many structures;
- multiple transitions;
- path/LOS cache churn;
- UI overlays;
- TurnLog volume.

Content:
- shipping maps pass validators;
- no duplicate structure ID;
- no invalid graph binding;
- no stale schema.

UX:
- source/target overlay leggibile;
- unknown remains unknown;
- interactive affordance discoverabile dove previsto.

## Scenari

```text
RC.Structure.AllDoorStates
RC.Structure.OperationsStress
RC.Interaction.NetworkPrivacySoak
RC.Interaction.ReplayCorpus
RC.Interaction.UIReadability
```

---

# 17. v1.0 — Ship gate

Il dominio è v1.0 quando:

## Geometry
- world/grid separation stabile;
- runtime non dipende da mesh collision come authority.

## Walls
- static/dynamic walls coerenti tra movement, LOS e cover.

## Doors
- standard e wide door = un logical structure;
- multi-transition atomic state;
- path/LOS/reactions aggiornano correttamente.

## Interaction graph
- stable source/target IDs;
- deterministic operations;
- validator dei bad bindings;
- nessuna pair map-specific hard-coded.

## Planning
- ally dependency leggibile;
- predicted != confirmed;
- stale preview non concede autorità.

## Knowledge
- unknown relation non leakata;
- discovered relation replicata correttamente.

## Network
- dedicated authoritative;
- late join/reconnect;
- zero canary leaks.

## Replay
- structure changes rappresentabili canonicamente;
- Player non ricalcola;
- zero divergenze inspiegate.

## Editor
- shipping maps authorable, validabili, bakeabili e cookabili;
- invalid data fail early.

## Accessibility
- nessuno stato dipende solo dal colore.

## QA
- automation;
- functional;
- visual/manual;
- packaged;
- network;
- replay;
- performance.

---

# 18. Epic strategy fino alla v1.0

Non assegnare numeri Epic a caso. Il repository ha già sofferto collisioni di contatori condivisi.

E23 resta owner v0.2.

Per v0.3+ creare nuove Epic solo se la release introduce un dominio nuovo e il modello corrente usa Epic release-scoped.

Usare placeholder in fase di audit:

```text
E<ALLOCATE>-Interaction Knowledge
E<ALLOCATE>-Operations Structures
E<ALLOCATE>-Network Structure Authority
E<ALLOCATE>-Structure Authoring Production
E<ALLOCATE>-Interaction Graph v2
E<ALLOCATE>-Interaction Beta Hardening
E<ALLOCATE>-Structure RC
```

Poi allocare col meccanismo canonico. Non riempire buchi storici.

---

# 19. Feature Registry

Il Feature Registry è owner dello stato feature. Non scrivere lo stesso status a mano in altre viste.

## Audit

Cercare feature esistenti collegate a:

```text
MAP
STANDABILITY
TRANSITION
COVER
DOOR
STRUCTURE
INTERACTION
KNOWLEDGE
EDITOR
```

Non creare feature ID paralleli se uno esistente copre il concetto.

Solo dopo audit, se mancano capability stabili, valutare ID semantici coerenti col naming reale. Non usare a priori nuovi ID solo perché questo handoff li immagina.

## Release order

Se la roadmap adotta v0.5…v1.0, aggiornare `scripts/feature_registry.py` e i test del tooling in modo che tutte le viste conoscano le nuove release. Nessuna release deve sparire silenziosamente.

---

# 20. Tutti i file di tracking da aggiornare

Il numero non è importante: scoprirli dal tooling corrente.

## 20.1 Owner/manuali

Verificare e aggiornare dove pertinente:

```text
docs/roadmap/feature-registry.yaml
docs/roadmap/roadmap-post-v0.1.md
docs/roadmap/roadmap-checkpoint.md
docs/roadmap/execution-graph.yaml
docs/roadmap/roadmap-editor.md
docs/technical/scenario-map.md
docs/technical/scenari-validazione-visiva.md
docs/technical/test-manuali-pie.md
docs/decisions/RT_PDR_00_Decision_Log.md
docs/OPEN_DECISIONS.md
docs/technical/spec-hex-geometry-authoring.md
docs/gameplay/spec-interazioni-mappa-cp101.md
```

Aggiornare `roadmap-v0.1.md` solo se cambia davvero il lavoro v0.1. Non inserire il piano v1.0 nel documento owner della sola v0.1.

## 20.2 Editor session data

Individuare il file canonico attuale per `editor-sessions.yaml` o successore. Aggiornarlo solo per task realmente manuali.

## 20.3 Derivati GENERATI

Non editarli a mano. Rigenerare tramite script:

```text
docs/roadmap/feature-registry.json
docs/roadmap/project-graph.json
docs/roadmap/roadmap.shortlist.md
docs/roadmap/featuremap.shortlist.md
docs/roadmap/scenariomap.shortlist.md
docs/roadmap/milestonemap.shortlist.md
docs/roadmap/editormap.shortlist.md
```

Se il generatore corrente produce cinque shortlist con nomi differenti, usare quelli reali.

## 20.4 Wiki

Aggiornare/deployare blocchi di stato e pagine di gioco tramite workflow canonico. Non duplicare status manuali che il registry genera.

## 20.5 Control Center

Se cambia `project-graph.json` o la taxonomy release, verificare `docs/control-center/` e Project Control Center. La dashboard non deve hard-codificare v0.1-v0.4.

## 20.6 Snapshot/lane

I file `roadmap_lane_*` sono snapshot, non source of truth. Non aggiornarli meccanicamente salvo che il workflow corrente richieda un nuovo snapshot esplicito.

---

# 21. Tooling / validazione

Usare i comandi correnti del repository. Baseline:

```bash
python scripts/feature_registry.py validate
python scripts/feature_registry.py generate
python scripts/feature_registry.py shortlist
python scripts/feature_registry.py report
python scripts/feature_registry.py generate --check
python scripts/feature_registry.py shortlist --check
```

Eseguire anche i gate reali per links, naming, shared IDs, scenario validation e tests. Se esistono `rt_shared_id check`, `audit-refs` o equivalenti, usarli secondo README/CI. Non inventare comandi.

---

# 22. Issue audit — procedura obbligatoria

Prima di creare issue:

```bash
gh issue list --state open --limit 200
gh issue list --state closed --limit 200
gh issue view 324
gh issue view 712
```

Cercare parole chiave:

```text
wall
door
structure
interaction
geometry
transition
standability
cover
switch
controller
gate
bridge
elevator
knowledge
```

Per ogni proposta classificare:

```text
NEW
UPDATE EXISTING
MERGE SCOPE
NO ACTION
```

Se 70–80% del lavoro è già descritto da una issue, aggiornare quella.

---

# 23. Standard body delle issue

Ogni issue nuova deve includere almeno:

```text
Why
Owner spec
Release
Epic
Feature ID
Dependencies
Scope
Out of scope
Definition of Done
Automation
Scenario
PIE / visual validation
Packaged requirement
Network/privacy requirement
Replay requirement
Debug/TurnLog
```

Per task Editor includere manual evidence, cosa sblocca e sessione.

Non mettere in Editor Queue attività automatizzabili.

---

# 24. Scenario Map

Gli scenari devono collegare:

```text
Scenario
-> Features
-> Issues
-> Tests
-> Editor Tasks
-> Release
```

Aggiornare gli owner e poi rigenerare le viste. Gli scenari non sono una lista decorativa: devono rispondere a “cosa manca perché questo scenario sia realmente eseguibile?”.

---

# 25. Test pyramid del dominio

## Core
- Stable IDs;
- deterministic sort;
- transition binding;
- multi-transition door atomic update;
- validator;
- hash normalization.

## Feature
- Interact -> state change;
- path invalidation;
- LOS revalidation;
- TurnLog.

## Functional
- switch;
- door;
- dependent move;
- open-fire-seal;
- breach.

## Network
- authority;
- hidden relationship;
- reconnect;
- late join.

## Replay
- structure changed survives serialization;
- same log/state hash.

## Visual
- tactical labels;
- source-target line;
- wide door readability;
- ghost/snap/editor.

## Packaged
Release gates applicabili.

---

# 26. Editor Map

Creare Editor Task solo per ciò che Claude non può verificare affidabilmente da CLI.

Esempi validi:

```text
verify 3 m door looks straight and readable
verify S1/D1 labels at tactical zoom
verify controller line does not clutter map
verify ghost invalid feedback
verify doorway traversal animation alignment
```

Non sono Editor Task: aggiungere USTRUCT, scrivere validator, creare JSON, eseguire automation, aggiornare YAML.

---

# 27. Decisioni aperte da non risolvere di straforo

Se non già decise, aprire/aggiornare OPEN DECISION prima di implementare:

## Door width semantic
`WidthCm` è solo editor/presentation o influenza gameplay? Baseline raccomandata: gameplay = explicit `AffectedTransitions`, non auto-derivazione dai centimetri.

## N-source logic
Se una porta ha più controller: AND, OR, priority, sequence? Non inventare.

## Power network
Se introdotto, appartiene all'Interaction Graph o a un sistema separato? Decidere prima.

## Destruction
Una structure `Destroyed` può tornare `Closed`? Baseline: no, senza repair/rebuild esplicito.

## Unknown controller
Quando la relazione controller->target è pubblica vs TeamKnowledge? Usare owner FoW corrente.

---

# 28. Definition of Done della pianificazione

Il lavoro di pianificazione è finito quando:

1. E23/#324 è aggiornato e non duplicato;
2. tutte le issue necessarie v0.2 sono tracciate;
3. le evoluzioni v0.3-v1.0 hanno Epic/issue solo dove serve;
4. la release taxonomy può rappresentare fino a v1.0;
5. `feature-registry.yaml` è coerente;
6. tutte le viste generate sono rigenerate;
7. Scenario Map contiene gli scenari del dominio;
8. Editor Map contiene solo verifiche manuali;
9. execution graph ha dipendenze reali;
10. Decision Log/Open Decisions sono coerenti;
11. `roadmap-post-v0.1.md` è estesa al nuovo horizon;
12. validator/generator passano;
13. link/naming/shared-id gate passano;
14. nessun file generato è stato editato manualmente;
15. il report finale elenca tutto ciò che è cambiato.

---

# 29. Output finale richiesto a Claude

Creare un report canonico, ad esempio:

```text
docs/roadmap/plans/walls-doors-interaction-v1-plan-2026-08-13.md
```

con naming conforme alla repo.

Il report deve contenere:

## A. HEAD audit

```text
HEAD before
HEAD after
baseline differences
```

## B. Existing work

```text
Issue | Status | Release | Epic | Feature | Action taken
```

## C. New issues

```text
Issue # | Title | Release | Epic | Dependencies | Scenarios
```

## D. Epic map

```text
v0.1
v0.2
v0.3
v0.4
v0.5
v0.6
v0.7
v0.8
v0.9
v1.0
```

con i veri ID allocati.

## E. Feature Registry diff

Nuove/modificate feature e gate.

## F. Scenario diff

Scenari aggiunti/modificati.

## G. Tracking files

Elenco esatto dei file aggiornati separando:

```text
OWNER
GENERATED
SNAPSHOT
ARCHIVE
```

## H. Validator output

Comandi e risultati.

## I. Remaining decisions

Solo decisioni reali non risolte.

---

# 30. Commit strategy

Preferire commit focalizzati, per esempio:

```text
docs(map): consolidate walls doors interaction model through v1.0
roadmap(map): extend structure interaction release plan
chore(tracking): update feature and execution graph owners
docs(scenario): add structure interaction validation matrix
chore(registry): extend release taxonomy and regenerate views
docs(editor): map manual structure interaction validation
```

Se issue GitHub vengono create/modificate prima dei commit, citare gli ID reali.

---

# 31. Guardrail finali

NON:

- creare una nuova E23;
- aprire E23 prima dei gate v0.1 se la roadmap ancora lo vieta;
- duplicare issue esistenti;
- usare mesh collision come authority competitiva;
- reintrodurre wall = hex side;
- modellare la porta da 3 m come due lati angolari;
- dividere una porta logica in due stati indipendenti senza design;
- hard-codificare `S1 -> D1`;
- inviare hidden interaction links ai client;
- creare una seconda TeamKnowledge;
- creare una seconda Scenario/Test pipeline;
- modificare file `GENERATED` a mano;
- usare lane snapshot come source of truth;
- assegnare numeri Epic/Decision condivisi a memoria;
- riempire vecchi buchi nei contatori;
- scrivere v1.0 come done/committed se è solo planning;
- trasformare le idee v0.7+ in scope approvato senza decisione.

---

# 32. Risultato atteso

A lavoro concluso il progetto deve poter rispondere dal proprio grafo di tracking alle domande:

```text
Quando arrivano le porte?
Quale Epic le possiede?
Quali issue mancano?
Quale Feature ID le traccia?
Quali scenari le validano?
Come viene testata la porta da 3 m?
Quale switch apre D1?
La relazione è nota al mio team?
Cosa succede se l'apertura fallisce?
Quale TurnLog lo spiega?
Come scala su Operations?
Come funziona in dedicated server?
Quali test mancano per v1.0?
Quali task richiedono intervento manuale nell'Editor?
```

La risposta deve venire dal repository, non da questo handoff.

---

# 33. Sequenza riassuntiva

```text
v0.1
Discrete geometry authoring + bake
        |
        v
v0.2
E23 — logical structures + doors + interaction graph
        |
        v
v0.3
knowledge / FoW / hidden relationship privacy
        |
        v
v0.4
multilayer + Operations scale
        |
        v
v0.5
network authoritative / dedicated
        |
        v
v0.6
production authoring / validation / cook
        |
        v
v0.7
systemic interaction graph expansion
        |
        v
v0.8
beta gameplay / bot / UX / accessibility
        |
        v
v0.9
RC hardening / soak / replay / performance
        |
        v
v1.0
shipping quality gate
```

Questa sequenza è una proposta da confrontare con la roadmap corrente e poi rendere canonica solo dopo l'audit.
