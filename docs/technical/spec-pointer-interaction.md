# Spec — Pointer Interaction Contract (CP 11.8): Hover · LMB · RMB

> **Owner documentale** del contratto del puntatore. `CURRENT` · normativo.
> Checkpoint **CP 11.8** ([#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705)),
> epic **E11** ([#25](https://github.com/DegrassiAaron/refactor-tactics-main/issues/25)) —
> [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §5 → E11.
> Feature: `RT-FEAT-UI-POINTER-INTERACTION` in [`feature-registry.yaml`](../roadmap/feature-registry.yaml) —
> **lo stato vive lì**, non in questo file.

Questo documento fissa **prima del codice** una sola tabella:

```text
oggetto sotto il puntatore × contesto corrente × Hover | LMB | RMB  →  esito
```

Non aggiunge regole di gioco. Il mouse non è un secondo ruleset: input e HUD **propongono** operazioni;
legalità, percorso, bersagli, interazioni e commit restano decisi dai servizi già autorevoli
(§3). Chi legge questo file deve poter prevedere cosa fa un click **senza aprire**
`RTPlayerController.cpp`.

---

## 1. Perché esiste, e cosa NON è

L'interazione col puntatore esiste già ed è **cresciuta per accumulo**: ogni checkpoint che aveva bisogno
di un click ne ha aggiunto un ramo dentro `OnSelect`. Il risultato non è rotto, ma non è dichiarato da
nessuna parte — e una regola che vive solo in una cascata di `if` non è verificabile, è archeologia.

**Non è** un redesign: nove decisioni su dieci di questo contratto descrivono ciò che il codice fa già.
Il valore è renderle **enunciabili e testabili**, e rendere visibili le tre che oggi non reggono (§6).

**Fuori perimetro** — nessuna di queste voci è oggetto di CP 11.8:

- redesign della camera (pan/zoom/rotate/focus restano come sono);
- input dell'**Editor Mode** (`ARTHexMapActor` in editor sceglie il layer da `ActiveLayer`, non dalla quota
  colpita: è un percorso diverso e resta suo);
- gamepad completo e CommonUI — resta però vincolante il requisito già scritto in
  [`progettazione-hud.md`](progettazione-hud.md) §47-bis.2: *percorso tastiera e controller equivalente a
  quello del mouse* per ogni Decision Window;
- logica di targeting o pathfinding **dentro** la UI;
- nuove regole di Door / Cover / Objective.

> ↩️ **Eredità.** [`spec-hover-cella.md`](spec-hover-cella.md) è `HISTORICAL` (substrato quadrato, superato da
> [ADR-0002](../decisions/adr-0002-griglia-esagonale.md)). La domanda che poneva — *cosa succede sotto il
> cursore* — non era stata riassegnata a nessun owner dal pivot esagonale: **la riprende questo file**.

---

## 2. Stato misurato — 2026-08-12, commit `ee0da4b3`

Il contratto parte da ciò che esiste, non da un foglio bianco.

| Fatto | Evidenza |
|---|---|
| `LMB` → `SelectAction` | `RTPlayerController.cpp:231` |
| `RMB` → `UndoAction`, **insieme a** `BackSpace` | `RTPlayerController.cpp:246-247` |
| Hover: `PlayerTick` → `GetHitResultUnderCursor` → `SetHoveredCell`, **solo presentazione** | `RTPlayerController.cpp:298-321` |
| La cella dell'hover viene dalla **quota** del punto colpito (il ponte evidenzia la cella del ponte) | `RTPlayerController.cpp:312-314` |
| `OnSelect` decide con una **cascata di `if` sul tipo di Actor colpito** | `RTPlayerController.cpp:395-447` |
| La decisione è separata dal raycast: `HandleClickOnCell` / `HandleClickOnUnit` sono verificabili headless | `RTPlayerController.h:116,123` |
| Guardia di autorità: si pianifica solo per le proprie unità | `URTCombatLibrary::CanPlayerControlUnit`, `RTPlayerController.cpp:401,421` |
| La modalità di targeting **non è uno stato del controller**: è `SelectedAbilityIndex` **sull'unità** | `RTPlayerController.cpp:504,606` |
| Il controller **non conosce la fase**: l'unica lettura è in `OnRestart`, e serve al fine partita | `ARTPlayerController::OnRestart` — `GetPhase() == ERTMatchPhase::MatchEnded` |
| Il Canvas HUD **non registra alcuna hitbox** (`AddHitBox` non compare in `Source/`) | `RTHUD.cpp` — assenza verificata |
| `bShowMouseCursor = true`; nessun `SetInputMode` esplicito | `RTPlayerController.cpp:257` |

Le ultime tre righe sono i tre delta reali: **nessuno stato esplicito**, **nessuna consapevolezza di fase
nell'input**, **nessuna precedenza HUD → mondo**.

---

## 3. Chi decide cosa — la regola di autorità

Il puntatore produce **al massimo** un *target logico*. Non produce mai un verdetto.

```text
raycast  →  target logico (FRTCellId | UnitId | StableObjectId)
                 ↓
         contesto corrente (§4)
                 ↓
         richiesta di operazione
                 ↓
    servizio autorevole  →  esito + reason code
```

I servizi autorevoli esistono già e **non si duplicano**:

| Domanda | Chi risponde |
|---|---|
| la cella è nella mappa? | `URTHexMapAsset::ContainsCell` |
| è raggiungibile, e con che percorso? | `URTHexPathLibrary` · `URTHexSimLibrary::ReachableCells` |
| chi la occupa? | `URTHexOccupancyLibrary` |
| è visibile / rilevata? | `URTHexVisionLibrary` |
| la copertura vale su quel lato? | `URTHexCoverLibrary` |
| quali celle colpisce l'azione? | `URTHexCombatLibrary::HexHitCells` |
| la porta si può aprire, e da chi? | `URTHexDoorLibrary` |
| questo giocatore comanda questa unità? | `URTCombatLibrary::CanPlayerControlUnit` |
| l'azione è legale adesso? | snapshot del `ARTTurnManager` |

**Regola vincolante**: se una risposta serve al puntatore e non è in questa tabella, si aggiunge un
consumatore del servizio — mai un calcolo nel controller o nel widget.

---

## 4. I contesti

Il controller acquisisce uno **stato esplicito**. Non è una fase del turno: è il modo corrente
dell'interazione, e vale insieme alla fase.

| Contesto | Quando | Chi lo termina |
|---|---|---|
| `IdleSelection` | Planning, nessuna azione armata | selezione di un'unità |
| `Planning` | Planning, unità propria selezionata, nessuna abilità attiva | scelta abilità o `LockIn` |
| `Pathing` | Planning, si stanno posando waypoint di movimento | `RMB`/`BackSpace` fino a svuotare, o `LockIn` |
| `Targeting` | Planning, `SelectedAbilityIndex` valido e in attesa di bersaglio | conferma, `RMB`, o cambio abilità |
| `ResolutionPlayback` | dal primo segmento risolto al `Cleanup` | fine risoluzione |
| `ReactionWindow` | una `DecisionWindow` è aperta (E14) | commit o timeout → `HOLD` |
| `Modal` | tooltip bloccante / dialogo / conferma | chiusura del modale |

> ⚠️ **`Targeting` e `Pathing` sono contesti distinti anche se oggi convivono** in `OnSelect`: un click su
> cella significa *waypoint* nel primo e *bersaglio a terra* nel secondo. È esattamente la voce che il DoD
> chiama «`LMB` non deve avere due significati concorrenti nello stesso stato»: qui non ce l'ha, perché gli
> stati sono due.

### Precedenza degli input

```text
Modal / ReactionWindow  >  HUD  >  world tactical hit
```

Regola: un input consumato da un livello **non** raggiunge quello sotto. Il click su HUD non deve
attraversare fino alla cella (§6.3).

---

## 5. La matrice

Esiti ammessi — l'elenco è **chiuso**:

| Esito | Significato |
|---|---|
| `NoOp` | nulla accade, nemmeno visivamente |
| `Inspect` | mostra informazione già pubblica, non cambia stato |
| `Select` | cambia il soggetto delle operazioni successive |
| `Preview` | mostra un esito **non** committato, marcato `Previsto` o `Incerto` |
| `Confirm` | registra un'intenzione nel piano (revocabile fino a `LockIn`) |
| `Cancel` | rimuove l'ultima intenzione o esce dal contesto |
| `OpenContext` | apre un elenco esplicito di operazioni |
| `Blocked(reason)` | rifiuto **con reason code**, mai silenzio |

### 5.1 Mondo

| Oggetto | Contesto | Hover | LMB | RMB |
|---|---|---|---|---|
| `EmptyWorld` (nessun hit) | tutti | `NoOp` — hover pulito | `NoOp` | `Cancel` del contesto corrente |
| `Cell` | `IdleSelection` | `Inspect` highlight | `NoOp` | `NoOp` |
| `Cell` | `Planning` / `Pathing` | `Preview` percorso e costo | `Confirm` waypoint · `Blocked(reason)` se irraggiungibile | `Cancel` ultimo waypoint |
| `Cell` | `Targeting` | `Preview` celle colpite e alleati in area | `Confirm` bersaglio a terra | `Cancel` → torna a `Planning` |
| `FriendlyUnit` | `IdleSelection` / `Planning` | `Inspect` | `Select` | `NoOp` |
| `FriendlyUnit` | `Targeting` | `Preview` con alleato marcato in area | `Confirm` **solo** se l'azione ammette bersagli alleati, altrimenti `Blocked` | `Cancel` |
| `EnemyUnit` **rilevata** | `Planning` | `Inspect` pubblico | `Blocked(reason)` se nulla è selezionato · `Confirm` attacco/carica se c'è una propria unità selezionata | `Cancel` |
| `EnemyUnit` **non rilevata** | tutti | `NoOp` — §6.1 | `NoOp` | `NoOp` |
| `CoverEdge` | `Planning` / `Targeting` | `Inspect` lato e valore | `NoOp` (non è un bersaglio) | `NoOp` |
| `Door` | `Planning` | `Inspect` stato e *chi può aprirla* | `Confirm` interazione se legale · `Blocked(reason)` altrimenti | `Cancel` |
| `Bridge` / `Transition` | `Planning` / `Pathing` | `Preview` transizione di livello | `Confirm` waypoint sulla cella del **ponte** (quota, non pianta) | `Cancel` |
| `Hazard` / `Surface` | tutti | `Inspect` effetto dichiarato | come `Cell` nello stesso contesto | come `Cell` |
| `Objective` | `Planning` | `Inspect` stato di contesa | `Confirm` interazione se legale · `Blocked` altrimenti | `Cancel` |
| `InteractionObject` | `Planning` | `Inspect` | `Confirm` se legale · `Blocked` altrimenti | `Cancel` |
| `OwnGhost` | `Planning` / `Pathing` / `Targeting` | `Inspect` fase e origine | `Select` della fase (scrubbing, CP 11.6) | `Cancel` di quella fase |
| `AllyIntentGhost` | tutti | `Inspect` **sola lettura** | `Inspect` — mai `Confirm` — §6.2 | `NoOp` |

### 5.2 HUD

| Oggetto | Contesto | Hover | LMB | RMB |
|---|---|---|---|---|
| `HUDActionSlot` | `Planning` | `Inspect` tooltip: costo, cooldown, portata | `Select` abilità → contesto `Targeting` | `Cancel` se l'abilità è quella attiva |
| `HUDUnit` / `TeamRoster` | `IdleSelection` / `Planning` | `Inspect` | `Select` unità (equivale al click nel mondo) | `NoOp` |
| `HUDReady` (`LockIn`) | `Planning` | `Inspect` cosa manca al lock | `Confirm` ready | `Cancel` ready se non ancora risolto |
| `HUDCombatLog` / `Tooltip` | tutti | `Inspect` | `Inspect` — nessun effetto di gioco | `NoOp` |

In **ogni** riga di questa tabella il puntatore è **sopra l'HUD**: l'input è consumato e non raggiunge il
mondo, qualunque cosa ci sia sotto.

### 5.3 Contesti che sovrascrivono tutto

| Contesto | Hover | LMB | RMB |
|---|---|---|---|
| `ResolutionPlayback` | `Inspect` e camera consentiti | `Inspect` — ogni input che cambierebbe il piano è `Blocked(reason)` | `NoOp` |
| `ReactionWindow` | `Inspect` delle **sole** risposte sanificate dell'opportunity | `Confirm` di una risposta legale; nient'altro è raggiungibile | `Cancel` → equivale a non scegliere, quindi `HOLD` al timeout |
| `Modal` | `Inspect` | `Confirm` del modale | `Cancel` del modale |

> Durante `ReactionWindow` il targeting normale **non si riapre**: le opzioni sono quelle che
> l'opportunity dichiara, e sono già sanificate ([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md)).

---

## 6. Le regole che oggi non reggono

Tre voci della matrice **non** descrivono il codice attuale. Sono il lavoro di CP 11.8.

### 6.1 Privacy — un nemico non rilevato non può diventare bersaglio dell'hover

`PlayerTick` fa `GetHitResultUnderCursor` e non filtra per rilevamento; `OnSelect` distingue le unità per
`TeamId`, non per livello di percezione. Finché **E13** non decide, l'unità avversaria è visibile perché è
un Actor nel mondo — e un tooltip su di essa rivelerebbe stato privato.

**Regola**: il resolver di hit **filtra** con `URTHexVisionLibrary` prima di produrre un target. Un nemico
non rilevato si comporta come `EmptyWorld`: nessun highlight, nessun tooltip, nessun bersaglio.
Nessun hover, warning, ghost o Decision Window usa mai intenti avversari privati: i warning si costruiscono
su stato **pubblico** più intenti della **propria** squadra.

### 6.2 Il ghost di un alleato è sola lettura

Non esiste oggi, perché la Ghost Timeline (CP 11.5/11.6) non ha ancora un view model. La regola va fissata
**adesso** perché è più facile scriverla che toglierla dopo: un `AllyIntentGhost` si ispeziona e non si
modifica da chi non ne è owner, salvo comandi di coordinazione **esplicitamente previsti** — che in v0.1
non esistono.

### 6.3 L'HUD deve consumare il puntatore

Oggi il Canvas HUD non registra hitbox (`AddHitBox` non compare in `Source/`), quindi **ogni** click passa
al mondo. Con lo Screen HUD di CP 11.7 (`#613`) i widget UMG intercetteranno l'input da soli, e il problema
diventa il suo opposto: garantire che l'input consumato **non** raggiunga anche il mondo. La precedenza di
§4 va resa esplicita e coperta da test, non lasciata al comportamento di default di Slate.

---

## 7. Definition of Done

- [ ] La matrice §5 è completa e ogni combinazione rilevante produce uno degli otto esiti dell'elenco chiuso
- [ ] Le precedenze `Modal/Reaction UI > HUD > world tactical hit` sono esplicite nel codice, non implicite
- [ ] Il resolver di hit restituisce un **target logico** (`FRTCellId` · `UnitId` · StableObjectId), mai una decisione di gameplay
- [ ] Il `PlayerController` usa un **contesto esplicito** (§4); niente cascata di `if` sul tipo di Actor colpito
- [ ] Click-through HUD → mondo coperto da test
- [ ] `RMB` annulla targeting e preview di percorso senza toccare un piano già `LockIn`
- [ ] Hover su nemico **non rilevato** non produce target né tooltip che riveli dato privato
- [ ] Hover e click su `AllyIntentGhost` non consentono modifica dell'intento alleato
- [ ] Door, Cover, Bridge e Objective risolvono l'**oggetto logico** e chiedono la legalità ai servizi di §3
- [ ] Durante `ResolutionPlayback` le sole scelte di gameplay possibili sono le Decision Boundary autorizzate da E14
- [ ] Ogni rifiuto porta un **reason code**: `Blocked` senza motivo è un difetto, non un esito
- [ ] `PIE-V01-POINTER` registrata in [`test-manuali-pie.md`](test-manuali-pie.md)

### Test automatici minimi

`RefactorTactics.PlayerInput.*`:

- `HUDConsumesPointerBeforeWorld`
- `HoverNeverCommits`
- `RightClickCancelsPreviewOnly`
- `HiddenEnemyCannotBecomeHoverTarget`
- `AllyGhostIsReadOnly`
- `PlaybackRejectsPlanningInput`
- `ReactionWindowOwnsInputPriority`
- `LogicalMapObjectResolvedFromStableId`

> I test vivono sul percorso **headless**: `HandleClickOnCell` / `HandleClickOnUnit` sono già separate dal
> raycast proprio perché la decisione sia verificabile senza viewport (`RTPlayerController.h:110-123`). Il
> contesto di §4 va sullo stesso lato del confine.

### Scenari

`Visual.UI.SelectMoveCancel` · `Visual.UI.TargetEnemyConfirmCancel` · `Visual.UI.DoorHoverAndInteract` ·
`Visual.UI.AllyIntentInspectReadOnly` · `Visual.UI.ReactionWindowPreemptsWorldInput` ·
`Spec.Privacy.HiddenEnemyHoverNoLeak`

Sono `planned` nel Feature Registry: nessuno dei sei si può scrivere prima che il contesto esista, perché
l'oracolo è l'esito della matrice e oggi l'esito non è dichiarato in nessun formato leggibile da uno
scenario.

---

## 8. Dipendenze e file

**Dipende da**: `#77` (CP 11.1, contenuto HUD) · `#613` (CP 11.7, Screen HUD e view model).
**Consumer**: `#172` (CP 11.5, ghost e facing) · `#291` (produttore dell'input di rotazione).
**Vincolato da**: E13 per §6.1 · E14 per `ReactionWindow`.

File coinvolti: `Player/RTPlayerController.*` · eventuale `Player/RTPointerInteraction*` ·
view model e UMG di CP 11.7 · servizi `Map/`, `Pathfinding/` esistenti · `ScenarioHarness/` ·
questo documento.

## 9. Rapporto con gli altri documenti

- [`progettazione-hud.md`](progettazione-hud.md) — **cosa mostra** l'HUD (§4.1 Screen, §4.2 Tactical Overlay,
  §18 Target/Hover Inspector). Questo file dice **cosa fa il puntatore**: i due non si sovrappongono.
- [`brief-planning-visuale.md`](brief-planning-visuale.md) — ghost e timeline, consumer di §5.1.
- [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) — la `ReactionWindow` di §5.3.
- [`spec-hover-cella.md`](spec-hover-cella.md) — `HISTORICAL`, provenienza dell'hover (§1).
- [`scenario-map.md`](scenario-map.md) — classe dei sei scenari di §7.
