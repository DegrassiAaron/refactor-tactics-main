# Mouse ↔ mondo ↔ HUD — contratto semantico di interazione

> `CURRENT` · **Owner proposto**: questo documento · **Target**: v0.1, PC-first · **Engine**: Unreal Engine 5.8  
> **Path repository previsto**: `docs/technical/spec-mouse-world-ui-interaction.md`  
> **Ambito**: significato di Hover / LMB / RMB in funzione dello stato di interazione e del bersaglio semantico sotto il cursore.
>
> Non è owner di pathfinding, targeting, porte, interaction verbs, facing, privacy, Ghost Timeline o Decision Window. Compone quelle regole in un contratto di input unico.

## 1. Stato reale misurato

`ARTPlayerController` ha già cursore, hover cella, selezione, click su cella/unità, ability hotkey, waypoint, RMB undo, path/AoE preview e targeting validato. Oggi però `OnSelect` può trasformare direttamente `propria unità selezionata + click nemico` in pianificazione dell'abilità attiva.

`ARTUnit` possiede già:

- `PlannedAttackCell` + `bAttackTargetsCell`;
- `PlannedCoverEdge` + `bHasPlannedCoverEdge`;
- `PlannedFacing` + `bDeclaresPlannedFacing`.

I commenti del codice dichiarano il lato mouse/HUD come buco di E11.

**Obiettivo:** prima del click il giocatore deve sapere cosa farà quel click.

## 2. Invarianti

1. **Hover è read-only**: non modifica mai piano, target, reaction, facing o Ready.
2. **LMB esegue solo l'affordance mostrata prima del click.**
3. **RMB = Back contestuale.**
4. La mesh non governa la UX: il raycast produce candidati, un resolver semantico sceglie il target.
5. La UI non ricalcola pathfinding, targeting, facing o reason code.
6. Hover/tooltip/inspector usano solo dati autorizzati dalla Team Knowledge.
7. Ghost è focus UI, mai gameplay target.
8. Hazard resta proprietà della cella: niente Actor di comodo.
9. Decision Window: scelta da opzioni esplicite, non precision aiming nel mondo.
10. HUD e mondo condividono focus/cross-highlight, non autorità.

## 3. Stati di interazione

### `PointerMode`

| Mode | Significato |
|---|---|
| `Inspect` | stato neutro/default |
| `Move` | costruzione/modifica path |
| `TargetUnit` | target unità |
| `TargetCell` | target cella/area |
| `TargetEdge` | target uno dei sei bordi |
| `TargetObject` | target elemento logico |
| `Interact` | `Action.Interact` + verbo |
| `Facing` | facing finale legale |
| `DecisionWindow` | scelta live in Resolution |

### `PhaseFocus`

`Auto | Prep | Dash | Blast | Move`

È separato dal `PointerMode`. Reaction resta un ramo, non una quinta fase.

## 4. Bersagli semantici

`ControllableUnit · AllyUnit · KnownEnemyUnit · Cell · CoverEdge · MapElement · Objective · OwnGhost · AllyGhost · HUDInteractive · HUDDecoration · None`.

Una porta multi-segmento deve risolversi all'intero oggetto logico, non al pezzo di mesh colpito. In v0.1 non si introduce un generico `MapElementId` se cella/bordo/`DoorId`/arco esistenti bastano.

## 5. RMB — Back canonico

RMB applica la prima voce valida:

```text
DecisionWindow       -> fallback esplicito, se consentito
Inspector pinned     -> chiudi inspector
Target*/Interact     -> annulla targeting e torna Inspect
Facing               -> annulla facing e torna Inspect
Move + waypoint      -> rimuovi ultimo waypoint
Move senza waypoint  -> torna Inspect
PhaseFocus pinned    -> PhaseFocus = Auto
altrimenti           -> no-op
```

RMB non deseleziona implicitamente l'unità. `Esc` segue lo stesso concetto salvo Decision Window.

## 6. Matrice `Inspect`

| Oggetto | Hover | LMB | RMB |
|---|---|---|---|
| propria unità | outline + `Select` | seleziona/cambia unità | Back |
| alleato | outline + intent summary | pin inspector ally | Back |
| nemico conosciuto | outline + `Inspect` | **Inspect; non attacca** | Back |
| cella libera, unità selezionata | destination + path preview | entra `Move` e prova waypoint | Back |
| cella senza unità selezionata | superficie/quota | inspect cella | Back |
| cover edge | bordo + relazione difensiva | inspect cover | Back |
| porta | intero oggetto logico + stato | Interaction Inspector | Back |
| ponte/arco | transizione + stato | inspect struttura | Back |
| hazard | pattern cella + chip | inspect cella/hazard | Back |
| objective | highlight zona/marker | Objective Inspector/focus | Back |
| own ghost | fase/certainty | pin `PhaseFocus` | Back |
| ally ghost | Team Intent summary | focus intent | Back |
| HUD interactive | hover widget | widget | Back/widget |
| HUD decoration | no capture | passa al mondo | passa al mondo |

### Cambio intenzionale

```text
oggi:    unità selezionata + click nemico -> può pianificare ability attiva
nuovo:   Inspect + nemico -> Inspect
         ability shortcut/slot -> TargetMode
         TargetMode + target -> pianifica
```

Il percorso rapido resta `2 -> click`.

## 7. Matrice `Move`

| Oggetto | Hover | LMB |
|---|---|---|
| altra propria unità | Select secondario | seleziona e termina editing Move |
| unità selezionata | origin | no-op |
| alleato/nemico | cella occupata + reason | rifiuto |
| cella raggiungibile | path + costo + destination | aggiunge waypoint |
| cella non raggiungibile | preview fino al limite + reason | no-op |
| cover | annotazione | **non cattura**: prevale cella |
| porta | stato/blocco | **non cattura**: prevale cella |
| ponte | transizione | prevale cella/layer |
| hazard | rischio/costo | cella selezionabile + warning |
| objective | preview | prevale cella |
| ghost | non hittabile | prevale ciò che sta sotto |

Una mesh di porta non deve impedire di indicare una cella oltre la porta. È topologia/pathfinding a rifiutare.

## 8. Matrice `TargetUnit`

| Oggetto | Hover | LMB |
|---|---|---|
| legal target | reticle + preview | assegna target |
| unità invalida | forbidden + reason sanitizzato | no-op |
| cella | contesto | no-op |
| cover/porta/ponte/objective/hazard | annotazione | no-op |
| ghost | non-target | no-op |

`enemy != legal` per definizione: la legalità arriva dal targeting domain.

## 9. Matrice `TargetCell`

Produttore UI di `PlannedAttackCell` / `bAttackTargetsCell`.

| Oggetto | Hover | LMB |
|---|---|---|
| unità sopra cella | unità leggibile ma non cattura | seleziona **cella sottostante** |
| cella valida | target anchor + shape/AoE | imposta target cella |
| cella invalida | forbidden + reason | no-op |
| cover/porta | annotazione | prevale cella |
| ponte | quota/transizione | prevale cella del layer corretto |
| hazard | interazione prevista | prevale cella |
| objective | preview area | prevale cella |
| ghost | non hittabile | prevale cella |

Un'unità non rende impossibile lanciare un AoE sulla cella che occupa.

## 10. Matrice `TargetEdge`

Produttore UI di `PlannedCoverEdge` / `bHasPlannedCoverEdge`.

| Oggetto | Hover | LMB |
|---|---|---|
| edge legale | segmento + preview | imposta cella + direzione |
| edge illegale | forbidden + reason | no-op |
| unità | non cattura | prevale edge |
| cella | mostra i sei settori | nessun commit senza edge |
| porta sul bordo | annotazione | prevale semantica dell'azione |
| ghost/hazard | trasparenti | prevale edge |

## 11. Matrice `TargetObject`

| Oggetto | Hover | LMB |
|---|---|---|
| unità/cella | non-target | no-op |
| cover targetable | highlight struttura | target object |
| porta | highlight intero `DoorId` | target porta |
| ponte | highlight arco/ponte logico | target ponte |
| hazard | non-oggetto | no-op |
| objective targetable | highlight owner logico | target objective |
| ghost | non-target | no-op |

## 12. Matrice `Interact`

Owner gameplay: `spec-interazioni-mappa-cp101.md`.

```text
Action.Interact + elemento + verbo + capability/requisiti
```

| Oggetto | Hover | LMB |
|---|---|---|
| unità | non-interagibile | no-op |
| cella | non-interagibile salvo elemento associato | no-op |
| cover con verbi | highlight + summary | Interaction Inspector |
| porta | stato noto + verbi | Interaction Inspector |
| ponte | stato noto + verbi | Interaction Inspector |
| hazard | non-interagibile in quanto hazard | no-op |
| sorgente logica hazard | verbi se MapElement | Inspector |
| objective interagibile | verbi | Inspector |
| ghost | non-interagibile | no-op |

Forma minima:

```text
D1 — PORTA LABORATORIO
Stato: CHIUSA

[ APRI ]          disponibile
[ FORZA ]         MissingCapability
[ OVERRIDE ]      Blocked
```

Hover sul verbo mostra cosa cambierebbe senza applicarlo. Reason privati vengono degradati prima del widget.

## 13. Matrice `Facing`

| Oggetto | Hover | LMB |
|---|---|---|
| sector legale | anchor pieno + preview | imposta `PlannedFacing` |
| sector illegale | anchor barrato + reason | no-op |
| unità/ghost | trasparenti | prevale sector |
| cella | supporta direzione | conferma sector |
| cover | preview relazione futura | prevale sector |
| porta/ponte/hazard/objective | annotazione | prevale sector |

Overwatch deriva dal facing: nessun secondo `OverwatchDirection`.

## 14. Ghost

- **Own Ghost:** hover informativo; LMB pinna `PhaseFocus`; mai edit diretto.
- **Ally Ghost:** focus e Team Intent sanitizzato; mai edit.
- **Enemy Ghost:** non esiste lato client.

## 15. HUD ↔ world cross-highlight

| HUD | Hover | LMB |
|---|---|---|
| Action Slot | range/shape/costi/reason world-space | apre PointerMode richiesto |
| Ghost phase | evidenzia ghost fase | pin `PhaseFocus` |
| roster own | evidenzia unità | seleziona |
| roster ally | evidenzia ally + intent | focus |
| warning chip | evidenzia sorgenti/celle | pin dettaglio |
| Interaction Verb | preview conseguenze | pianifica verbo |
| Objective panel | evidenzia objective | focus |
| Combat Log row | source/target/cella | focus evento |
| Confirm Plan | summary | commit o reason |
| decorazione | no capture | passa al mondo |

## 16. Decision Window

| Target | Hover | LMB | RMB |
|---|---|---|---|
| world | read-only | **non committa** | fallback se previsto |
| FIRE A/B | evidenzia target | commit scelta | — |
| HOLD | evidenzia fallback | HOLD | — |

Percorso equivalente mouse/tastiera/controller.

## 17. Priorità hit semantica

```text
Inspect:        controllable unit > ally/enemy > map element/objective > ghost > edge > cell
Move:           controllable unit > cell > annotazioni
TargetUnit:     legal unit > invalid unit > nothing
TargetCell:     cell > annotazioni
TargetEdge:     legal edge > invalid edge > nothing
TargetObject:   legal map element > invalid map element > nothing
Interact:       interactable element > non-interactable element > nothing
Facing:         facing sector > everything else
DecisionWindow: UI option > world read-only
```

## 18. Cursore e timing

Cursori minimi: `Inspect · Select · Move · TargetUnit · TargetCell · TargetEdge · TargetObject · Interact · Facing · Forbidden`.

Secondo canale obbligatorio: marker/shape/testo, mai solo colore.

Baseline playtest:

- frame corrente: highlight + cursore + legalità;
- ~200 ms: compact label;
- ~500 ms: tooltip/inspector esteso.

## 19. Architettura proposta

```text
Mouse -> raw hit candidates
          |
          v
Semantic Interaction Resolver
  + PointerMode
  + stato autorizzato
          |
          v
Interaction View
  HoverTarget / Affordance / Cursor / Reason / Highlights
       |                 |
       v                 v
World Overlay        UMG ViewModel
       \                 /
        -> LMB/RMB routing
```

Nomi candidati, non API già esistenti:

- `ERTPointerMode`;
- `FRTSemanticPointerTarget`;
- `FRTInteractionView`;
- `URTPlayerInteractionLibrary` o piccolo model del controller.

## 20. Test derivati

```text
RefactorTactics.UI.Mouse.HoverNeverChangesPlan
RefactorTactics.UI.Mouse.EnemyClickInInspectDoesNotAttack
RefactorTactics.UI.Mouse.AbilityThenEnemyClickPlansAttack
RefactorTactics.UI.Mouse.MoveCellWinsOverDoorMesh
RefactorTactics.UI.Mouse.HazardDoesNotStealMove
RefactorTactics.UI.Mouse.TargetUnitRejectsCell
RefactorTactics.UI.Mouse.TargetCellIgnoresUnitHit
RefactorTactics.UI.Mouse.TargetEdgeResolvesDeclaredEdge
RefactorTactics.UI.Mouse.TargetObjectResolvesWholeDoor
RefactorTactics.UI.Mouse.InteractReasonIsSanitized
RefactorTactics.UI.Mouse.OwnGhostFocusesPhaseWithoutEditing
RefactorTactics.UI.Mouse.AllyGhostCannotBeEdited
RefactorTactics.UI.Mouse.RightClickBackIsDeterministic
RefactorTactics.UI.Mouse.DecisionWindowWorldClickCannotCommit
```

Mutation checks: hover scrivente, enemy-click che torna ad attaccare, porta che ruba Move, reason privato esposto, world click che committa Decision Window.

## 21. Scenario Harness

**Non creare JSON UI finti adesso.** Il formato scenario non possiede `hover/lmb/rmb/pointerMode`; aggiungerli prima del runtime renderebbe l'harness il primo produttore della capability.

Ordine: pure automation → controller integration → PIE.

Candidati futuri, solo dopo una capability reale `PointerInteraction`:

```text
Spec.UI.Mouse.EnemyInspectIsReadOnly
Spec.UI.Mouse.TargetCellUnderUnit
Spec.UI.Mouse.InteractionReasonPrivacy
Spec.UI.Mouse.GhostFocusDoesNotEdit
```

## 22. PIE

Voce prevista: **`PIE-V01-MOUSE-INTERACTION`**.

1. seleziona Flux;
2. hover cella → LMB chiaramente Move;
3. LMB aggiunge path;
4. RMB pop ultimo waypoint;
5. hover nemico in Inspect → Inspect, non Attack;
6. Ability 2;
7. target hover/preview;
8. RMB cancel senza perdere selezione;
9. porta + Interaction Inspector quando CP 10.1 è interagibile;
10. hover verbo senza mutazione;
11. RMB chiude inspector;
12. click Ghost Blast → pin fase senza edit;
13. RMB → `PhaseFocus = Auto`.

**Gate UX:** il tester non deve mai cliccare “per vedere cosa succede”.

## 23. Owner map

| Superficie | Owner |
|---|---|
| semantic resolver + LMB/RMB | nuova issue ausiliaria E11 |
| UMG/Inspector | #613 |
| Ghost | #172 |
| Scrubbing | #173 |
| facing input | #291 |
| Interact | #74 |
| interaction graph futuro | #324 |
| Decision Window | E14 |
| privacy | Team Knowledge / intent filtering |

## 24. Definition of Done

- ogni PointerMode implementato coperto da test;
- Hover provato read-only;
- RMB Back deterministico;
- raycast fisico non decide la priorità semantica;
- `TargetCell` produce `PlannedAttackCell`;
- `TargetEdge` produce `PlannedCoverEdge`;
- enemy click neutro non pianifica azioni;
- Ghost/HUD cross-highlight senza edit impliciti;
- reason code rispettano Team Knowledge;
- Decision Window senza precision aiming;
- `PIE-V01-MOUSE-INTERACTION` eseguito;
- packaged build coerente;
- nessuna regola gameplay duplicata nel pointer layer.
