# Spec — Pointer Interaction Contract (CP 11.8): Hover · LMB · RMB

> **Owner documentale** del contratto del puntatore. `CURRENT` · normativo.
> Checkpoint **CP 11.8** ([#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705)),
> epic **E11** ([#25](https://github.com/DegrassiAaron/refactor-tactics-main/issues/25)) —
> [`roadmap-v0.1.md`](../../roadmap/roadmap-v0.1.md) §5 → E11.
> Feature: `RT-FEAT-UI-POINTER-INTERACTION` in `feature-registry.yaml` —
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

**Non è** un redesign: la grande maggioranza delle voci di questa matrice descrive ciò che il codice fa già.
Il valore è renderle **enunciabili e testabili**, e rendere visibili le **cinque** che oggi non reggono (§6).

*(La stesura del 2026-08-12 diceva «nove decisioni su dieci» e «le tre che non reggono». La revisione del
2026-08-13 ne ha misurate altre due — §6.4 e §6.5 — e una di esse, [D-128](../../decisions/RT_PDR_00_Decision_Log.md),
**cambia** deliberatamente un comportamento invece di descriverlo. La frase «nove su dieci» è stata tolta e
non corretta con un altro numero: era una stima, e sostituirne una con un'altra non la rende una misura.)*

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
> [ADR-0002](../../decisions/adr-0002-griglia-esagonale.md)). La domanda che poneva — *cosa succede sotto il
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

### 2.1 Due assenze misurate il 2026-08-13, commit `3cec1d57`

Le righe qui sopra descrivono ciò che il puntatore **fa**. Queste due descrivono ciò che **non può fare**, e
non erano nella misura del 2026-08-12.

| Fatto | Evidenza |
|---|---|
| **Non esiste uno stato «nessuna abilità armata».** `SelectedAbilityIndex` nasce a `0` e nessuno lo riporta a un valore non valido: le sole scritture sono `ARTUnit::SelectAbility`, chiamata dagli hotkey, e l'harness | `RTUnit.h:170` · `ARTUnit::SelectAbility` (`RTUnit.cpp:525-531`) · `RTPlayerController.cpp:785` · `RTScenarioSession.cpp:473` |
| **Tre campi di piano sono consumati dal resolver e non hanno produttore nel gioco.** `PlannedAttackCell`/`bAttackTargetsCell`, `PlannedCoverEdge`/`bHasPlannedCoverEdge`, `PlannedFacing`/`bDeclaresPlannedFacing` sono letti dal `TurnManager`, ma le sole assegnazioni fuori dai test stanno nello Scenario Harness | letture: `RTTurnManager.cpp:2036-2045,2329-2333,3239-3241,4972-4980` · scritture: `RTScenarioSession.cpp:666-675` — **nessuna** in `Player/` o `UI/` |

La seconda riga si verifica in un comando, e conviene rifarla invece di crederci:

```sh
grep -rn "PlannedAttackCell *=\|bAttackTargetsCell *=\|PlannedCoverEdge *=\|bHasPlannedCoverEdge *=\|PlannedFacing *=\|bDeclaresPlannedFacing *=" Source/ \
  | grep -v "RTScenarioSession\|RTTurnManager\|RTUnit.h\|Tests/"
```

~~Oggi non stampa nulla.~~ ✅ **Superata il 2026-08-13 sera.** Il comando stampa ora otto righe in
`Player/RTPlayerController.cpp` — `HandleTargetCell`, `HandleTargetEdge`, `HandleFacingSector`. Le due
assenze di questa sezione **non descrivono più il codice**, e la tabella qui sopra resta come misura
d'origine, non come stato.

### 2.2 Stato dopo la prima consegna — 2026-08-13 sera

| Delta | Stato |
|---|---|
| 1. Nessuno stato esplicito | ✅ **chiuso** — `ARTPlayerController::GetPointerContext()`, derivato e non memorizzato |
| 2. Nessuna consapevolezza di fase nell'input | ✅ **chiuso** — `ResolutionPlayback` esce dalla fase del `TurnManager` |
| 3. Nessuna precedenza HUD → mondo | ⏳ **aperto** — dipende dai widget UMG di [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613): senza hitbox non c'è nulla che consumi il puntatore |
| 4. Nessuno stato neutro | ✅ **chiuso** — [D-128](../../decisions/RT_PDR_00_Decision_Log.md): `SelectedAbilityIndex` nasce a `INDEX_NONE` |
| 5. Nessun produttore UI | ✅ **chiuso** — i tre produttori esistono e sono coperti |

> ⚠️ **`ReactionWindow` e `Modal` esistono nell'enum e nessuno li produce.** Sono ordinati correttamente in
> `URTPointerLibrary::ResolveBack` — e testati lì — ma `GetPointerContext()` non li restituisce mai: la
> finestra di reazione è **E14** e il modale è **#613**. È deliberato, e la ragione è quella di §2.1: un flag
> che nessuno scrive sarebbe un campo senza produttore, cioè il difetto che questo checkpoint ha appena
> finito di documentare. Quando quegli owner arrivano, aggiungono il proprio ramo in `GetPointerContext()`.

> ⚠️ **La legalità del facing dichiarato in Planning è una PREVISIONE, non il verdetto.** `HandleFacingSector`
> valida su `PlannedPath` con stile `Budget`/`None`; il resolver rivalida a fine Move su
> `MovementStyleThisTurn` e `WalkedThisTurn`, cioè su quel che è successo davvero. Un percorso interrotto fa
> cadere la dichiarazione con `DeclarationRejected`, ed è corretto: la UI propone, il servizio decide (§3).

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
| `IdleSelection` | Planning, **nessuna unità selezionata** | selezione di un'unità |
| `Planning` | Planning, unità propria selezionata, **nessuna abilità armata** | scelta abilità o `LockIn` |
| `Pathing` | Planning, si stanno posando waypoint di movimento | `RMB`/`BackSpace` fino a svuotare, o `LockIn` |
| `Targeting` | Planning, un'abilità è **armata esplicitamente** e attende bersaglio | conferma, `RMB`, o cambio abilità |
| `Facing` | Planning, si dichiara la rotazione finale | conferma di un settore, `RMB`, o `LockIn` |
| `ResolutionPlayback` | dal primo segmento risolto al `Cleanup` | fine risoluzione |
| `ReactionWindow` | una `DecisionWindow` è aperta (E14) | commit o timeout → `HOLD` |
| `Modal` | tooltip bloccante / dialogo / conferma | chiusura del modale |

> ⚠️ **`Targeting` e `Pathing` sono contesti distinti anche se oggi convivono** in `OnSelect`: un click su
> cella significa *waypoint* nel primo e *bersaglio a terra* nel secondo. È esattamente la voce che il DoD
> chiama «`LMB` non deve avere due significati concorrenti nello stesso stato»: qui non ce l'ha, perché gli
> stati sono due.

> 🔴 **`Planning` non è rappresentabile oggi**, ed è la voce 4 di §2.1: `SelectedAbilityIndex` nasce a `0` e
> nessuno lo riporta a un valore non valido, quindi un'abilità è **sempre** armata e il contesto neutro non
> esiste. La riga della tabella dice «nessuna abilità armata» perché è ciò che CP 11.8 deve costruire, non
> ciò che si misura. Senza quello stato, [D-128](../../decisions/RT_PDR_00_Decision_Log.md) non è implementabile:
> non c'è un «neutro» in cui il click possa ispezionare.

### `Targeting` ha un `TargetKind`

`Targeting` non è un contesto solo. **L'azione armata dichiara che forma di bersaglio accetta**, e da quella
dipende sia cosa vince sotto il cursore (§4.1) sia quale campo di piano viene scritto:

| `TargetKind` | Bersaglio | Campo prodotto | Stato |
|---|---|---|---|
| `Unit` | un'unità legale | `PlannedAttackTarget` | ✅ `RTPlayerController.cpp:540` |
| `Cell` | una cella, anche occupata, anche vuota | `PlannedAttackCell` + `bAttackTargetsCell` | ⛔ nessun produttore — [#737](https://github.com/DegrassiAaron/refactor-tactics-main/issues/737) |
| `Edge` | uno dei sei bordi di una cella | `PlannedCoverEdge` + `bHasPlannedCoverEdge` | ⛔ nessun produttore — [#737](https://github.com/DegrassiAaron/refactor-tactics-main/issues/737) |
| `Object` | l'oggetto logico intero (`DoorId`, arco, objective) | interazione, §5.1 | ⛔ risolve la mesh, non l'oggetto |

Il `TargetKind` **non è una modalità che il giocatore sceglie**: la dichiara l'azione. Armare `Hero.Riktor.Reconfigure`
apre `Targeting`/`Edge`; armare un AoE apre `Targeting`/`Cell`. Il giocatore sceglie l'azione, e la forma del
bersaglio segue — così l'affordance è decisa prima del click, che è l'invariante di questo documento.

`Facing` è un contesto separato e **non** un quarto `TargetKind`: non dichiara un bersaglio, dichiara come
finisce girata la propria unità. Il suo produttore manca per la stessa ragione degli altri due, ed è la riga
ancora aperta di [#291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/291).

### Precedenza degli input

```text
Modal / ReactionWindow  >  HUD  >  world tactical hit
```

Regola: un input consumato da un livello **non** raggiunge quello sotto. Il click su HUD non deve
attraversare fino alla cella (§6.3).

### 4.1 La mesh non governa la UX

Quella precedenza dice quale **livello** vince. Non dice nulla su cosa vince *dentro* il mondo, e lì il
raycast da solo dà la risposta sbagliata: restituisce **ciò che sta più vicino alla camera**, che è una
proprietà della geometria di collisione, non del significato.

> Il raycast produce **candidati**. Quale candidato diventa il target lo decide il contesto.

| Contesto | Ordine di precedenza sotto il cursore |
|---|---|
| `IdleSelection` / `Planning` | unità controllabile → altra unità nota → oggetto logico / objective → ghost proprio → bordo → cella |
| `Pathing` | unità controllabile → **cella** → tutto il resto è annotazione |
| `Targeting` / `Unit` | unità legale → unità illegale (con reason) → nulla |
| `Targeting` / `Cell` | **cella** → tutto il resto è annotazione |
| `Targeting` / `Edge` | bordo legale → bordo illegale (con reason) → nulla |
| `Targeting` / `Object` | oggetto logico legale → oggetto illegale (con reason) → nulla |
| `Facing` | settore di rotazione → tutto il resto |
| `ReactionWindow` | opzione dichiarata dall'opportunity → il mondo è sola lettura |

Tre conseguenze che oggi non valgono, e che sono le più facili da sbagliare implementando:

1. **Una porta non deve impedire di indicare la cella oltre la porta.** In `Pathing` la mesh della porta è
   un'annotazione: vince la cella. Se quel percorso non si può fare lo rifiuta la topologia, con un reason —
   non il fatto che un collider stava davanti.
2. **Un'unità sopra una cella non deve impedire di bersagliare quella cella.** In `Targeting`/`Cell` vince la
   cella *sotto* l'unità. È il caso che rende un'area utilizzabile: si centra un AoE su chi lo occupa.
3. **Un ghost non è mai un bersaglio di gioco.** È un fuoco della UI. Dove un ghost copre qualcosa,
   vince ciò che sta sotto.

Un oggetto composto da più mesh — una porta a due segmenti, un ponte ad archi — risolve all'**oggetto logico
intero**, non al pezzo colpito. In v0.1 non si introduce un `MapElementId` generico: `FRTCellId`, il bordo,
`DoorId` e l'arco esistono già e bastano. L'identità stabile e il grafo di interazione restano di E23
([#324](https://github.com/DegrassiAaron/refactor-tactics-main/issues/324)).

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
| `Cell` | `Targeting` / `Cell` | `Preview` celle colpite e alleati in area | `Confirm` bersaglio a terra — **anche se occupata** (§4.1) | `Cancel` → torna a `Planning` |
| `Cell` | `Targeting` / `Unit` | `Inspect` contesto | `NoOp` — l'azione vuole un'unità | `Cancel` |
| `FriendlyUnit` | `IdleSelection` / `Planning` | `Inspect` | `Select` | `NoOp` |
| `FriendlyUnit` | `Targeting` / `Unit` | `Preview` con alleato marcato in area | `Confirm` **solo** se l'azione ammette bersagli alleati, altrimenti `Blocked` | `Cancel` |
| `EnemyUnit` **rilevata** | `IdleSelection` / `Planning` | `Inspect` pubblico | `Inspect` — **non pianifica** ([D-128](../../decisions/RT_PDR_00_Decision_Log.md)) | `Cancel` |
| `EnemyUnit` **rilevata** | `Targeting` / `Unit` | `Preview` dell'attacco: portata, copertura sul lato, esito atteso | `Confirm` attacco/carica · `Blocked(reason)` se illegale | `Cancel` → `Planning` |
| `EnemyUnit` **non rilevata** | tutti | `NoOp` — §6.1 | `NoOp` | `NoOp` |
| `CoverEdge` | `Planning` / `Targeting` / `Cell` | `Inspect` lato e valore | `NoOp` (non è un bersaglio in questi contesti) | `NoOp` |
| `CoverEdge` | `Targeting` / `Edge` | `Preview` del bordo dichiarato e della relazione difensiva | `Confirm` cella + direzione | `Cancel` → `Planning` |
| `FacingSector` | `Facing` | `Preview` del settore: pieno se legale, barrato con reason se no | `Confirm` rotazione dichiarata | `Cancel` → `Planning` |
| `Door` | `Planning` | `Inspect` stato e *chi può aprirla* — sull'oggetto **intero**, non sul segmento colpito | `OpenContext` se i verbi sono più d'uno · `Confirm` se ce n'è uno solo legale · `Blocked(reason)` | `Cancel` |
| `Door` | `Pathing` | `Inspect` stato e blocco | **`NoOp` sulla porta**: vince la cella (§4.1) | come `Cell` |
| `Bridge` / `Transition` | `Planning` / `Pathing` | `Preview` transizione di livello | `Confirm` waypoint sulla cella del **ponte** (quota, non pianta) | `Cancel` |
| `Hazard` / `Surface` | tutti | `Inspect` effetto dichiarato | come `Cell` nello stesso contesto (§4.1: non ruba mai il bersaglio) | come `Cell` |
| `Objective` | `Planning` | `Inspect` stato di contesa | `OpenContext` se i verbi sono più d'uno · `Confirm` se ce n'è uno solo legale · `Blocked` | `Cancel` |
| `InteractionObject` | `Planning` | `Inspect` | `OpenContext` se i verbi sono più d'uno · `Confirm` se ce n'è uno solo legale · `Blocked` | `Cancel` |
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
> l'opportunity dichiara, e sono già sanificate ([ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md)).

### 5.4 `OpenContext`: la forma minima

`OpenContext` è l'unico degli otto esiti che nessuna riga usava prima di questa revisione, e serviva:
un elemento con **più verbi** non si risolve con un click che ne sceglie uno di nascosto. Il puntatore apre
un elenco esplicito, e ogni voce porta con sé la ragione per cui è o non è disponibile.

```text
D1 — PORTA LABORATORIO
Stato: CHIUSA

[ APRI ]          disponibile
[ FORZA ]         MissingCapability
[ OVERRIDE ]      Blocked
```

Hover su un verbo mostra **cosa cambierebbe** senza applicarlo. I reason code passano dallo stesso filtro
di §6.1 prima di raggiungere il widget: un motivo che rivela stato privato viene degradato, non mostrato.
I verbi e i requisiti restano di [`../../gameplay/spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md)
([#74](https://github.com/DegrassiAaron/refactor-tactics-main/issues/74)): qui si dichiara solo che il
puntatore li **elenca** invece di sceglierli.

### 5.5 `RMB` è un Back, e l'ordine è totale

`Cancel` compare in molte righe di §5.1, e finché resta una colonna della tabella non dice cosa succede
quando due cose sarebbero annullabili insieme. `RMB` applica **la prima voce valida** di questo elenco, e
l'elenco è ordinato:

```text
1. ReactionWindow aperta   -> fallback esplicito, se l'opportunity ne dichiara uno
2. Modal aperto            -> chiudi il modale
3. Inspector pinnato       -> chiudi l'inspector
4. Targeting / Facing      -> annulla la dichiarazione, torna a Planning
5. Pathing con waypoint    -> rimuovi l'ultimo waypoint
6. Pathing senza waypoint  -> torna a Planning
7. PhaseFocus pinnato      -> PhaseFocus = Auto
8. altrimenti              -> NoOp
```

Due regole che l'ordine da solo non dice:

- **`RMB` non deseleziona mai implicitamente l'unità.** Uscire da un targeting non deve costare la selezione:
  è l'errore che costringe a ricliccare la propria unità dopo ogni ripensamento.
- **`RMB` non tocca un piano già in `LockIn`.** Il Back agisce sulla dichiarazione in corso, non su ciò che
  è stato consegnato.

`BackSpace` segue lo stesso elenco (è già legato a `UndoAction`, `RTPlayerController.cpp:246-247`). `Esc`
pure, con la sola eccezione della `ReactionWindow`: lì non chiude, perché non scegliere è già `HOLD`.

### 5.6 `PhaseFocus` non è un contesto

La voce 7 dell'elenco nomina uno stato che **non** appartiene a §4, e la distinzione conta:

`PhaseFocus ∈ {Auto, Prep, Dash, Blast, Move}` è l'asse dello **scrubbing** — quale fase del proprio piano si
sta guardando. È ortogonale al contesto del puntatore: si può ispezionare la fase `Blast` mentre si posano
waypoint. Non è una quinta fase del turno, non è una modalità di input, e non cambia cosa fa `LMB`.

Owner: [`brief-planning-visuale.md`](brief-planning-visuale.md) e CP 11.6
([#173](https://github.com/DegrassiAaron/refactor-tactics-main/issues/173)). Qui compare per una ragione
sola: è l'ultimo livello che `RMB` smonta prima di diventare `NoOp`, e senza dichiararlo l'elenco di §5.5
sarebbe incompleto.

---

## 6. Le regole che oggi non reggono

**Cinque** voci della matrice **non** descrivono il codice attuale. Sono il lavoro di CP 11.8.

*(Erano tre alla stesura del 2026-08-12. Le due aggiunte il 2026-08-13 — §6.4 e §6.5 — vengono dalla misura
di §2.1 e non da un cambio di scopo: erano già vere, e nessuno le aveva contate.)*

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

### 6.4 ✅ Chiusa — un'abilità era sempre armata, quindi non esisteva un neutro

`SelectedAbilityIndex` nasce a `0` (`RTUnit.h:170`) e le sole scritture lo portano su un altro indice valido
(`ARTUnit::SelectAbility`, chiamata dagli hotkey `1`–`4`). Non c'è alcun percorso che lo riporti a
`INDEX_NONE`. Conseguenza diretta, e verificabile in PIE: **cliccare un nemico pianifica sempre qualcosa** —
lo slot `0` di quell'eroe — e il giocatore non l'ha armato, l'ha trovato armato.

È il motivo per cui l'incoerenza che questo documento portava nella propria §5.1 era reale: l'hover su un
nemico diceva `Inspect`, il click faceva `Confirm`. [D-128](../../decisions/RT_PDR_00_Decision_Log.md) la chiude
scegliendo il lato dell'affordance — in `Planning` il click **ispeziona**, e per bersagliare bisogna armare.

✅ **Consegnata il 2026-08-13 sera.** `SelectedAbilityIndex` nasce a `INDEX_NONE`, `ARTUnit::SelectAbility`
accetta `INDEX_NONE` per disarmare, e `RMB` ci torna dal livello `Declaration` di §5.5. Coperta da
`PlayerInput.NeutralEnemyClickDoesNotPlan`, che porta **nello stesso test** la controprova del percorso
rapido: armata l'azione, lo stesso click pianifica. Senza quella metà il test passerebbe anche con un
controller che non sa più bersagliare nulla.

⚠️ Lo stato neutro è **portante in tre punti**, e la verifica di mutazione lo mostra: riportare il default a
`0` fa cadere tre test, non uno — il click neutro, la dichiarazione di facing e l'ordine del Back.

### 6.5 ✅ Chiusa — tre dichiarazioni che il resolver sapeva eseguire e il giocatore non sapeva chiedere

Bersaglio a cella, bordo di copertura e rotazione dichiarata avevano regole, consumo nel `TurnManager` e
test, e nessun produttore nel gioco (§2.1): tre pezzi di gameplay già pagati, inerti in partita e verdi
negli scenari.

✅ **Consegnati il 2026-08-13 sera** ([#737](https://github.com/DegrassiAaron/refactor-tactics-main/issues/737)):

| Modo | Produttore | Copertura |
|---|---|---|
| `Targeting`/`Cell` | `ARTPlayerController::HandleTargetCell` | `PlayerInput.TargetCellProducesPlannedAttackCell` |
| `Targeting`/`Edge` | `ARTPlayerController::HandleTargetEdge` | `PlayerInput.TargetEdgeProducesPlannedCoverEdge` |
| `Facing` | `ARTPlayerController::HandleFacingSector` | `PlayerInput.FacingSectorProducesPlannedFacing` · `PlayerInput.IllegalFacingIsRejectedNotCorrected` |

Il `TargetKind` è **derivato dal catalogo** (`URTPointerLibrary::TargetKindForAction`), non un campo nuovo:
`StructureOp != None` → `Edge`, `Shape == Area` → `Cell`, altrimenti `Unit`. Aggiungere un campo al dato
avrebbe creato una seconda fonte di verità accanto a due che lo dicevano già.

> ✅ **E l'asimmetria delle capability si è chiusa da sé.** `RTScenarioSession.cpp` dichiarava
> `PredictiveAction` disponibile con la motivazione *«un canale che il gioco ha già»*, **falsa** alla misura
> della mattina. Ora il canale esiste, il verde di quei quattro scenari dice qualcosa di vero sul giocatore,
> e il commento è stato riscritto con la storia intera invece che con la sola conclusione.
>
> ✅ **E `DeclaredRotation` è entrata fra le capability la sera stessa**, con la chiave `facing` in
> `FRTScenarioIntent` e i due scenari che dimostrano la rotazione dichiarata sul percorso reale
> (`Spec.Facing.IllegalDeclaredRotationIsRejected` e `Spec.Facing.StationaryDeclaredRotationApplies`).
> Era l'ultima delle tre asimmetrie storiche di quell'elenco: non ne resta nessuna aperta.
>
> 🔴 **Un buco trovato misurando, non supponendo**: togliendo la capability i due scenari passano a
> `BLOCKED` e la suite resta **verde**. `RefactorTactics.Scenario.DeclaredRotationScenariosPass` pinna
> l'esito a `Pass` — la stessa disciplina di #601 per la reazione. Quando una capability atterra, l'ancora
> va scritta **nello stesso commit**.

---

## 7. Definition of Done

*(Spuntate il 2026-08-13 sera. Le caselle aperte portano **chi** le blocca: nessuna è aperta per mancanza di
tempo.)*

- [ ] La matrice §5 è completa e ogni combinazione rilevante produce uno degli otto esiti dell'elenco chiuso — ⏳ manca la parte HUD (§5.2) e `OpenContext`
- [ ] Le precedenze `Modal/Reaction UI > HUD > world tactical hit` sono esplicite nel codice, non implicite — ⏳ **#613**: senza hitbox UMG non c'è nulla che consumi il puntatore
- [x] Il resolver di hit restituisce un **target logico** (`FRTCellId` · `UnitId` · StableObjectId), mai una decisione di gameplay — `URTPointerLibrary::ResolveTarget`
- [x] Il `PlayerController` usa un **contesto esplicito** (§4); niente cascata di `if` sul tipo di Actor colpito — `GetPointerContext()`, derivato
- [x] Esiste uno stato **neutro**: entrando in Planning nessuna abilità è armata, e il click su un nemico ispeziona ([D-128](../../decisions/RT_PDR_00_Decision_Log.md), §6.4)
- [x] `Targeting` porta un `TargetKind` dichiarato dall'azione (§4), non una modalità che il giocatore sceglie a parte
- [x] La precedenza **intra-mondo** di §4.1 è esplicita: in `Pathing` la cella vince sulla mesh di porta/ponte/hazard, e in `Targeting`/`Cell` vince sull'unità che la occupa
- [ ] Un oggetto multi-mesh (porta a segmenti, ponte ad archi) risolve all'**oggetto logico intero** — 🟡 il resolver ha il ramo `Object`, ma nessun produttore di candidati lo popola: serve il raycast di `OnSelect` che riconosca gli elementi logici (**#74** per le porte)
- [ ] Un elemento con **più verbi legali** produce `OpenContext`, non un `Confirm` che ne sceglie uno implicitamente — ⏳ **#74** (i verbi) + **#613** (il contenitore)
- [ ] Click-through HUD → mondo coperto da test — ⏳ **#613**
- [x] `RMB` segue l'**ordine totale** di §5.5, non deseleziona mai l'unità e non tocca un piano già `LockIn`
- [ ] Hover su nemico **non rilevato** non produce target né tooltip che riveli dato privato
- [ ] Hover e click su `AllyIntentGhost` non consentono modifica dell'intento alleato
- [ ] Door, Cover, Bridge e Objective risolvono l'**oggetto logico** e chiedono la legalità ai servizi di §3
- [ ] Durante `ResolutionPlayback` le sole scelte di gameplay possibili sono le Decision Boundary autorizzate da E14
- [ ] Ogni rifiuto porta un **reason code**: `Blocked` senza motivo è un difetto, non un esito — 🟡 i tre produttori nuovi lo dicono (`UE_LOG` con la ragione); i rami preesistenti di `OnSelect` no, e il reason **a schermo** è **#613**
- [x] `PIE-V01-POINTER` registrata in [`test-manuali-pie.md`](../test-manuali-pie.md) — registrata; ⏳ **non ancora eseguita**

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

✅ **Scritti il 2026-08-13 sera** — dieci, ognuno per una regola nuova, tutti verdi e tutti passati per la
verifica di mutazione:

| Test | Regola |
|---|---|
| `NeutralEnemyClickDoesNotPlan` | D-128, §6.4 — e nello stesso test la **controprova**: armata l'azione, lo stesso click pianifica |
| `PathingCellWinsOverDoorMesh` | §4.1 conseguenza 1, con la controprova che in `Planning` la stessa porta è invece l'oggetto giusto |
| `TargetCellIgnoresOccupyingUnit` | §4.1 conseguenza 2, con la controprova su `TargetKind::Unit` |
| `GhostIsNeverAGameplayTarget` | §4.1 conseguenza 3 |
| `RightClickBackFollowsTotalOrder` | §5.5 — tutti i livelli montati insieme, si verifica che cada il più prioritario |
| `RightClickNeverDeselects` | §5.5 — la regola che si nota solo usando il gioco |
| `TargetCellProducesPlannedAttackCell` | §6.5 — il produttore, più il rifiuto fuori portata che **non** distrugge il piano precedente |
| `TargetEdgeProducesPlannedCoverEdge` | §6.5 — il produttore |
| `FacingSectorProducesPlannedFacing` | §6.5 — il produttore |
| `IllegalFacingIsRejectedNotCorrected` | §6.5 — **nessuna correzione silenziosa**; la direzione illegale si **cerca** interrogando `LegalFacings`, non si hardcoda |

> ⚠️ **`ArmedAbilityThenEnemyClickPlans` non esiste come test a sé**, ed è deliberato: vive come seconda metà
> di `NeutralEnemyClickDoesNotPlan`, dove condivide il setup e dice la cosa che conta — che il neutro **non**
> ha allungato il percorso rapido. Un nome in meno nell'elenco, la stessa proprietà verificata. È scritto qui
> perché un elenco di nomi attesi che non corrisponde alla suite è esattamente il difetto che il gate `G3` ha
> già pagato due volte.

⏳ **Restano da scrivere, e ognuno porta chi lo blocca**: gli otto originali qui sopra (HUD, privacy,
playback, `ReactionWindow`) più `MultiVerbElementOpensContext` (§5.4, dipende da **#74** e **#613**).

> ⚠️ Il campo `tests` di `RT-FEAT-UI-POINTER-INTERACTION` nel Feature Registry elenca ora **solo i dieci che
> esistono**: `validate` tratta un pattern senza corrispondenza come errore, non come avviso, e ha ragione.
> I nomi entrano nel registro quando entrano i test — mai prima.

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
**Consumer**: `#172` (CP 11.5, ghost e facing) · `#173` (CP 11.6, `PhaseFocus` di §5.6) ·
`#291` (produttore dell'input di rotazione) · `#737` (i tre produttori di §6.5) ·
`#74` (CP 10.1, i verbi che `OpenContext` elenca in §5.4).
**Vincolato da**: E13 per §6.1 · E14 per `ReactionWindow` · E23 (`#324`) per l'identità stabile degli
oggetti multi-mesh di §4.1.

File coinvolti: `Player/RTPlayerController.*` · eventuale `Player/RTPointerInteraction*` ·
view model e UMG di CP 11.7 · servizi `Map/`, `Pathfinding/` esistenti · `ScenarioHarness/` ·
questo documento.

## 9. Rapporto con gli altri documenti

- [`progettazione-hud.md`](progettazione-hud.md) — **cosa mostra** l'HUD (§4.1 Screen, §4.2 Tactical Overlay,
  §18 Target/Hover Inspector). Questo file dice **cosa fa il puntatore**: i due non si sovrappongono.
- [`brief-planning-visuale.md`](brief-planning-visuale.md) — ghost e timeline, consumer di §5.1.
- [ADR-0004](../../decisions/adr-0004-finestre-di-reazione.md) — la `ReactionWindow` di §5.3.
- [`spec-hover-cella.md`](spec-hover-cella.md) — `HISTORICAL`, provenienza dell'hover (§1).
- [`scenario-map.md`](../tooling/scenario-map.md) — classe dei sei scenari di §7.
- [`../../gameplay/spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md) — i **verbi**
  che `OpenContext` elenca (§5.4). Il puntatore li mostra; non li definisce e non ne giudica i requisiti.

### Provenienza della revisione 2026-08-13

Due sorgenti proponevano un secondo owner per questa stessa superficie, a un percorso diverso e con una
nomenclatura parallela (`PointerMode`, `RefactorTactics.UI.Mouse.*`, `PIE-V01-MOUSE-INTERACTION`). Non sono
stati adottati come documenti: **una superficie ha un owner**, e questo file lo era già dal 2026-08-12. È
stato recepito il loro contenuto — §2.1, §4.1, §5.4, §5.5, §5.6, §6.4, §6.5 e le otto righe nuove della
matrice — dentro la nomenclatura che il repository usa già.

Gli originali sono in [`../../archive/src/design`](../../archive/src/design/) per provenienza:
`2026-08-13-mouse-world-ui-interaction.md` e `2026-08-13-mouse-interaction-integration-plan.md`. Servono a
ricostruire *da dove* nasce una regola, non a deciderla.
