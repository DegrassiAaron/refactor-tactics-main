# HUD v0.1 — Lane A · CODE / ARCHITECTURE

> `EPHEMERAL` · **Vista esecutiva temporanea, non una roadmap canonica.** Non sostituisce
> [`roadmap-v0.1.md`](../roadmap-v0.1.md), [`roadmap-checkpoint.md`](../roadmap-checkpoint.md), le issue
> owner, il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) né
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md).
>
> **Regola di questo file**: ogni riga nomina il proprio owner e **non inventa stato**. Se una riga qui e il
> suo owner divergono, ha ragione l'owner e questa riga è scaduta.
>
> **Misurata**: 2026-09-05 su `origin/main` `8a530c6e`, modalità **DEV**. La baseline completa —
> con ciò che è stato misurato e ciò che è `NOT RUN` — è in
> [`hud-v01-three-terminals-audit-2026-09-05.md`](hud-v01-three-terminals-audit-2026-09-05.md).
> **Nessun test è stato eseguito in questa passata**: gli stati `DONE` qui sotto sono *codice presente e
> test dichiarati*, non un verde rimisurato.

## Come si legge

`Lane A produce contratti · Lane B li falsifica.` Ogni passo dichiara la verifica Editor che lo chiude
(`Editor handoff` → un `HUD-EDITOR-Ex` di
[`hud-v01-editor-verification-roadmap.md`](hud-v01-editor-verification-roadmap.md)).

Stati usati: `DONE` · `PARTIAL` · `TODO` · `BLOCKED` · `VERIFY` (codice presente, resta solo l'oracolo).

Architettura, invariata e non negoziabile in questa lane:

```text
Simulator / authoritative runtime  decide
        ↓
ViewModel / DTO / semantic ID      espone
        ↓
UMG / HUD / overlay                mostra
```

Nessun widget calcola formule, legality o reason code. Nessun secondo resolver, pathfinder o privacy filter.

---

## Riepilogo

| ID | Owner | Stato live | Resta |
|---|---|---|---|
| `HUD-CODE-C0` | — | `DONE` | — (è l'audit) |
| `HUD-CODE-C1` Icon language | #217 · #219 · #220 · #637 | `VERIFY` | `PIE-ICON-01` |
| `HUD-CODE-C2` HUD ViewModels | #77 · #613 | `DONE` | commento scaduto su #77 |
| `HUD-CODE-C3` Screen HUD UMG | #613 | `VERIFY` | `PIE-V01-SCREENHUD` |
| `HUD-CODE-C4` Player Event Log | #1936 · #1937 · #79 | `PARTIAL` | parti A e F |
| `HUD-CODE-C5` Ghost Timeline | #172 · #173 | `PARTIAL` + decisione aperta | 4 test dichiarati, contrasto |
| `HUD-CODE-C6` Pointer | #705 · #1614 · #1402 | `BLOCKED` | hitbox UMG |
| `HUD-CODE-C7` Ready / Unready | #2193 (CLOSED) · #2390 | `VERIFY` | `PIE-V01-READY` |
| `HUD-CODE-C8` Unit overlay / status | #2288 · #2347 (CLOSED) · #2378 | `VERIFY` | `PIE-ICON-02` |
| `HUD-CODE-C9` `rt.Debug.*` | #80 | `PARTIAL` | tre `DrawX` che stampano |
| `HUD-CODE-C10` batch gate | — | `TODO` | l'elenco dei comandi |

---

## C0 — Audit

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C0` |
| **Owner** | nessuno: è la passata stessa |
| **Stato live** | `DONE` — 2026-09-05 |
| **Intent** | avere una baseline misurata invece che ricordata |
| **Output** | [`hud-v01-three-terminals-audit-2026-09-05.md`](hud-v01-three-terminals-audit-2026-09-05.md) |
| **Anti-vacuità** | ogni riga dell'audit porta il comando che l'ha prodotta |
| **Exit** | ✅ raggiunto |

Quattro cose che l'audit ha cambiato rispetto all'handoff che lo ha chiesto:

1. `#78`, `#2193`, `#2288`, `#2347` sono **chiuse**: l'handoff le elencava come residui;
2. `#1937` è l'**epic owner** del Player Event Log, e l'handoff nominava solo `#1936`;
3. `#1614` è la **seconda** issue di CP 11.8, non nominata;
4. l'ultimo commento di `#77` (2026-08-14) descrive un albero che non esiste più.

---

## C1 — Icon language / contratto semantico

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C1` |
| **Owner** | `#217` (E20) · `#219` (CP 20.2) · `#220` (CP 20.3) · `#637` (tassonomia, `question`) |
| **Stato live** | `VERIFY` — il codice di CP 20.3 è su `main`; `#220` resta aperta per **una** voce |
| **Intent** | un'icona è una **chiave semantica**, non un percorso di texture: cambiare l'arte non tocca i widget |
| **Producer** | `URTIconLibrary` (`RequiredIconIds`, `FindMissingRequiredIcons`, `ValidateIconCatalog`, `ResolveIcon`) |
| **Contract** | `FRTIconDef{IconId, Category, Asset}` · `FRTIconResolution{Asset, bResolved}` · `URTIconCatalogData` (`UPrimaryDataAsset`, `PrimaryAssetType RTIconCatalog`) |
| **Consumer** | `URTActionSlotWidget::GetResolvedIcon()` · `URTUnitOverlayWidget` · i `WBP_RT_*` via la classe base |
| **Dependencies** | *hard*: nessuna. *sequencing*: C2/C3 consumano il catalogo |
| **Implementation minimum** | già raggiunto: `RequiredIconIds()` **deriva dai dati di gioco** (fasi volontarie, `GetCoreActionCatalog()`, tag sotto `Status.`), quindi un tag nuovo senza icona fa cadere la copertura il giorno in cui viene definito |
| **Tests** | 6 `IconCatalog.*` · `ScreenHud.ActionSlotResolvesFromCatalog` · `ScreenHud.IconCatalogReachesTheRoot` · `ScreenHud.WidgetApiExposesNoTexture` · **`ScreenHud.BlueprintPropertiesExposeNoTexture`** (PR #2415: chiude la metà Blueprint di D-031) |
| **Anti-vacuità** | il filtro `RefactorTactics.IconCatalog` deve dare `performed > 0`. Controllo positivo disponibile: aggiungere una variabile `Texture2D` dentro un `WBP_RT_*` **deve** far cadere `BlueprintPropertiesExposeNoTexture` |
| **Determinism** | nessun impatto: presentazione |
| **Privacy** | nessun impatto |
| **Replay/TurnLog** | nessun impatto |
| **Editor handoff** | `HUD-EDITOR-E1` (catalogo) · `HUD-EDITOR-E2` (binding) |
| **Exit — `CODE READY`** | ✅ **già raggiunto.** Contratto stabile, catalogo con test reale, nessun widget conosce un percorso di texture. Il residuo di `#220` non è codice: è `PIE-ICON-01` |

⛔ **Non aprire lavoro di codice qui.** `#637` è una domanda di tassonomia (17 categorie del manifest assenti
dal codice) e resta `DEFER`: allargare il catalogo prima che la v0.1 lo chieda è debito senza soggetto.

---

## C2 — HUD ViewModels / stato base

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C2` |
| **Owner** | `#77` (CP 11.1) · `#613` (CP 11.7) |
| **Stato live** | `DONE` nel codice |
| **Intent** | il giocatore capisce round, fase, timer, la propria squadra, l'unità selezionata e cosa può ancora fare |
| **Producer** | `URTHudViewModel` (`UBlueprintFunctionLibrary`) su `ARTTurnManager` / `ARTUnit` |
| **Contract** | `FRTMatchHeaderView` (round, `RoundLimit`, `Phase`, `PlanningSecondsRemaining`, `ReadyCountdownSecondsRemaining`, `SecondsUntilCommit`, `bResolving`, punteggi) · `FRTUnitCardView` (HP/MaxHP/Shield/`bIsAlly`/`bAlive`) · `FRTUnitSlotsView{Movement, Main, Reaction}` · `FRTAbilityCooldownView` (slot, turni, `ChargeFraction`, `bUsableNow`) · `FRTStatusBadgeView` · `FRTUnitOverlayView` |
| **Consumer** | `URTScreenHudWidgetBase` e le sei sottoclassi · `ARTHUD` (Canvas §4.2) · `URTUnitOverlayWidget` |
| **Dependencies** | *hard*: `ARTTurnManager`. *sequencing*: C3 |
| **Implementation minimum** | raggiunto. La regola dei **due orologi** vive in `ComputeSecondsUntilCommit` — una sede sola — e `ARTHUD::ComposeMatchStatusLine` la consuma invece di riscriverla (PR #2424) |
| **Tests** | 19 `HudViewModel.*` · 14 `ScreenHud.*` |
| **Anti-vacuità** | controllo di mutazione **già eseguito** dall'owner: rimosso il filtro di squadra di `BuildTeamRoster`, il test cade (`expected 2, was 3`), ripristinato **e ricompilato** torna verde |
| **Determinism** | nessuno: il view model **legge**, non decide |
| **Privacy** | `BuildTeamRoster` filtra per squadra; `BuildAuthoritativeIntents` passa da `RTIntentPrivacyLibrary` |
| **Replay/TurnLog** | nessun impatto: nessun campo del view model entra nello `StateHash` |
| **Editor handoff** | `HUD-EDITOR-E2` · `E3` · `E4` |
| **Exit — `CODE READY`** | ✅ raggiunto. **Regola dura confermata dal codice**: il widget non calcola formule né legality |

⚠️ **Il DoD di `#77` è ancora 0/10, e il suo ultimo commento è del 2026-08-14.** Quel commento conclude
*«manca l'anello letto: `RTHUD.cpp` non include il view model, e `Content/RT/` non ha una cartella `UI/`»*.
L'anello letto **esiste dal 2026-08-26/27** (otto `WBP_RT_*`). Chi riprende `#77` rimisuri prima di
concludere che manchi qualcosa.

---

## C3 — Screen HUD UMG (§4.1)

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C3` |
| **Owner** | `#613` · piano [`screen-hud-umg-2026-08-26.md`](screen-hud-umg-2026-08-26.md) |
| **Stato live** | `VERIFY` — DoD del 2026-09-05: **7 voci ✅**, una 🟡 (zone/centro libero), una ⏳ (`PIE-V01-SCREENHUD`) |
| **Intent** | lo Screen HUD esiste, si monta e legge dati sanitizzati |
| **Producer** | `URTScreenHudWidgetBase` (contesto di partita, `GetIconCatalog`, `bShowDebug = false`) + `URTFrontendNavigator` (layer HUD) |
| **Contract** | i `UFUNCTION(BlueprintPure)` delle sei sottoclassi: `GetHeader`, `GetRoundCounterText`, `GetRoster`, `GetCard`, `GetSlots`, `GetActions`, `GetArmedActionIndex`, `GetResolvedIcon` |
| **Consumer** | `WBP_RT_TacticalHUD` · `TurnHeader` · `TeamRoster` · `SelectedUnitPanel` · `ActionDock` · `ActionSlot` |
| **Dependencies** | *hard*: C1, C2. *sequencing*: C4 (parte F), C6 (hitbox) |
| **Separazione da non violare** | **Screen HUD §4.1** = header · roster · selected · dock · warning · feed eventi · conferma del piano. **Tactical World Overlay §4.2** = path · waypoint · destinazione · AoE · fuoco amico · facing · coni. Il §4.2 resta in Canvas (`ARTHUD`) **dove la spec lo vuole**: non migrarlo per uniformità |
| **Tests** | `ScreenHud.MatchWidgetsLoad` · `MatchWidgetsDeriveFromCppBase` · `DockArmsOnlyTheSelectedAction` (prova il **comportamento** del grafo, non la sua forma) · `RosterShowsOnlyOwnTeamAndKeepsTheFallen` · `SlotsAreReadNotDeduced` · `RoundLimitComesFromTheFormat` |
| **Anti-vacuità** | filtro `RefactorTactics.ScreenHud` con `performed > 0`; l'owner ha misurato 52/52 su `1535e70f` |
| **Determinism / Privacy / Replay** | nessun impatto |
| **Editor handoff** | `HUD-EDITOR-E2` · `E3` · `E4` |
| **Exit — `CODE READY`** | ✅ raggiunto. Ogni consumer ha un dato stabile, nessuna regola di gioco nei widget, debug **spento di default** |

⛔ **`#613` non si chiude su un test.** Quello che resta è ingombro, leggibilità a risoluzione di gioco,
centro libero e coerenza durante il playback — `PIE-V01-SCREENHUD`, che dal ponte MCP **non è eseguibile**:
in play mode `SlateInspector.Snapshot` risponde vuoto e `CaptureEditorImage` fallisce.

---

## C4 — Player Event Log

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C4` |
| **Owner** | `#1936` (fette A–F) · **`#1937`** (epic, asse semantico di `CR-PRESENT`) · `#79` (CP 11.3, reason code) |
| **Stato live** | `PARTIAL` — C, D, E in `main` (PR #2104); **A** e **F** aperte |
| **Intent** | il giocatore legge cosa è successo senza leggere il TurnLog diagnostico |
| **Producer** | `URTPlayerEventProjector::Project(Entries, ObserverTeamId)` + `IsAuthorized` |
| **Contract** | `FRTPlayerEvent{Type, Importance, PrimaryUnitId, ActionId, …}` · `ERTPlayerEventType` · `ERTPlayerEventImportance` |
| **Consumer** | `WBP_RT_EventLog` — **non esiste**: è la fetta **F**, un `.uasset` dentro `WBP_RT_TacticalHUD` |
| **Dependencies** | *hard*: C3 (il contenitore). *ordine obbligato*: **A non può precedere F** — togliere i pannelli Canvas prima che il feed esista lascia il giocatore senza log |
| **Pipeline, e non si accorcia** | `Resolver → TurnLog canonico → predicato di autorizzazione → Projector → FRTPlayerEvent[] → UI`. ⛔ Mai proiettare prima di sanificare, mai filtrare la privacy **dopo** aver generato il testo, mai fare parsing di stringhe diagnostiche, mai semplificare il TurnLog di debug per rendere leggibile il log del giocatore |
| **Implementation minimum** | **F**: un widget che consuma `FRTPlayerEvent[]` con argomenti semantici — nessuna frase precomposta, nessuna cella. **A**: rimuovere i pannelli Canvas legacy, con `rt.HUD.CanvasPanels` già presente come interruttore |
| **Tests** | 11 `UI.PlayerEventLog.*` + `UI.Log*` (ordine, dominanza, omissione del nemico ricordato, morte pubblica, fallback che nomina l'azione) |
| **Anti-vacuità** | `RefactorTactics.UI` con `performed > 0`; per **A** serve un controllo negativo: con `rt.HUD.CanvasPanels 0` il feed deve restare l'unico log visibile |
| **Determinism** | nessuno: il proiettore è una **vista**, e non entra nell'hash |
| **Privacy** | è il punto: `FRTKnowledgeVerdict::AllowsTeam` è il predicato, isolato e fail-closed. La fetta C-bis **cade**: il predicato esiste già |
| **Replay/TurnLog** | il TurnLog canonico **non si tocca** |
| **Editor handoff** | `HUD-EDITOR-E8` |
| **Exit — `CODE READY`** | quando **F** ha un consumer che legge `FRTPlayerEvent[]` e **A** è eseguibile senza lasciare il giocatore senza log |

⚠️ **Due limiti noti, da decidere prima di scrivere il feed** (commento #1936 del 2026-09-02):
`SecondaryUnitId` è sempre `INDEX_NONE` per le voci di combattimento — il TurnLog ha un solo `UnitId` per
voce e lo assegna a **chi subisce**; e la **dominanza non aggrega**: due colpi sulla stessa unità restano una
riga e non sommano i danni.

🔴 **Residuo di `#79` che il feed eredita**: misurato in PIE il 2026-09-04, *«ho provato e me l'hanno
negato»* e *«non ho provato»* producono **la stessa riga**. Il reason code esiste nel runner degli scenari,
non in partita.

---

## C5 — Ghost Timeline / contratto di preview

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C5` |
| **Owner** | `#172` (CP 11.5) · `#173` (CP 11.6) |
| **Stato live** | `PARTIAL` + **decisione aperta** |
| **Intent** | il giocatore capisce **dove finirà** e **da dove agirà**, non solo la destinazione |
| **Producer** | le primitive canoniche del resolver: stesso A\*, stesso snapshot, stessa forma di combattimento |
| **Contract** | una entry per fase con `Phase`, `UnitId`, `ActionId`, `PreviewOrigin`, `PreviewDestination`, `Facing`, `PoseId`, `TargetCells`, `AffectedCells`, `Certainty`; **`ReactionPreview` separata dalla lista delle fasi** — la reaction non è una quinta fase |
| **Consumer** | Tactical World Overlay §4.2 (`ARTHUD`), non lo Screen HUD |
| **Dependencies** | *hard*: nessuna sul codice HUD. *UX*: C3. ⚠️ **Non** è una dipendenza canonica della catena UI `#219/#637 → #220 → #77/#613 → #705 → #291` |
| **Implementation minimum** | i **quattro test dichiarati e mai scritti**, che sono il modo in cui il contratto diventa falsificabile |
| **Tests presenti (9 `Preview.*`)** | `OriginIsCurrentCellWithoutDash` · `OriginIsPlannedDashCell` · `HitCellsMatchCombatShape` · `CellTargetProducesFootprint` · `DeadTargetHasNoFootprint` · `ReachableCellsArePassedThrough` · `AllyInAreaIsFlagged` · `AllyInCellTargetAreaIsFlagged` · `ClearedWhenPlanIsCancelled` |
| **Tests assenti (`grep -rl` → 0 file)** | `Preview.GhostMatchesResolverPath` · `Preview.ReactionIsNotAPhaseEntry` · `Preview.WarningsComeFromResolverReasons` · `Preview.ArmedReactionRendersAsBranch` |
| **Anti-vacuità** | `RefactorTactics.Preview` con `performed > 0`; per `GhostMatchesResolverPath` serve una **fixture negativa**: un percorso che il resolver rifiuta non deve comparire come ghost valido |
| **Determinism** | ⛔ **il rischio principale di questa riga**: mai un secondo pathfinder, un secondo targeting, una seconda regola di copertura |
| **Privacy** | la preview è degli **intenti propri e alleati**; nessun canale laterale verso il piano avversario |
| **Replay/TurnLog** | nessun impatto |
| **Editor handoff** | `HUD-EDITOR-E7` |
| **Exit — `CODE READY`** | i quattro test scritti e verdi, **e** la decisione qui sotto presa |

🔴 **`DECISION_REQUIRED` — contrasto linea↔superficie (commento #172, 2026-08-29).** Misurato: la linea di
scatto `#009E73` sta a `dE 2.1` da `ShallowWater` e a `14.3` da `Floor`, la linea `Move` propria a `dE 1.9`
da `Ice`. Due vie, **nessuna scelta**: (a) estendere il gate `T9` alla coppia linea↔superficie leggendo
`URTHexLibrary::SurfaceColor`; (b) cambiare canale — contorno scuro o spessore indipendente dalla tinta.
Con nove superfici e tre linee, cercare una tinta buona su tutte è probabilmente il problema sbagliato.
**Non risolverla nel widget**: è un contratto di presentazione, e l'owner è `#172`.

---

## C6 — Pointer Interaction Contract

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C6` |
| **Owner** | `#705` (CP 11.8) · `#1614` (overlay dell'hover) · `#1402` (Back sullo scatto) |
| **Stato live** | `BLOCKED` — non per tempo, per **produttori assenti** |
| **Intent** | l'input **propone**; il runtime decide la legalità, e ogni rifiuto porta un motivo |
| **Producer** | `ARTPlayerController::GetPointerContext()` (derivato, non memorizzato) + `RTPointerInteraction.{h,cpp}` |
| **Contract** | otto esiti chiusi: `NoOp` · `Inspect` · `Select` · `Preview` · `Confirm` · `Cancel` · `OpenContext` · `Blocked(reason)`; contesti `IdleSelection · Planning · Pathing · Targeting · ResolutionPlayback · ReactionWindow · Modal`; precedenza `Modal/Reaction UI > HUD > world tactical hit` |
| **Consumer** | i `WBP_RT_*` (quando registreranno hitbox) e il mondo |
| **Dependencies** | *hard*: **C3** — `AddHitBox` ha **0 occorrenze** in `Source/`, rimisurato oggi: finché il Canvas non registra hitbox **ogni** click passa al mondo e la precedenza HUD→mondo non ha nulla da consumare. Poi `E13` (nemico non rilevato) ed `E14` (finestra di reazione) |
| **Implementation minimum** | nessuno oggi: tre dei quattro produttori mancanti non appartengono a questo checkpoint |
| **Tests presenti (10 `PlayerInput.*` del perimetro)** | `NeutralEnemyClickDoesNotPlan` · `PathingCellWinsOverDoorMesh` · `TargetCellIgnoresOccupyingUnit` · `GhostIsNeverAGameplayTarget` · `RightClickBackFollowsTotalOrder` · `RightClickNeverDeselects` · `HoverNeverCommits` · `RightClickCancelsPreviewOnly` (gli ultimi due da #1766, entrambi passati per la verifica di mutazione) |
| **Tests dichiarati e non scritti (8)** | `HUDConsumesPointerBeforeWorld` (attende #613) · `AllyGhostIsReadOnly` (CP 11.5/11.6) · `ReactionWindowOwnsInputPriority` (E14) · `LogicalMapObjectResolvedFromStableId` (#74) · `HiddenEnemyCannotBecomeHoverTarget` (E13, privacy) · `PlaybackRejectsPlanningInput` (è una **feature**, non un test) · `ArmedAbilityThenEnemyClickPlans` e `MultiVerbElementOpensContext` (la spec owner li dichiara già così) |
| **Anti-vacuità** | `RefactorTactics.PlayerInput` + `RefactorTactics.Pointer` con `performed > 0` (34 test fra i due file) |
| **Determinism** | nessuno |
| **Privacy** | due regole **nuove** e non ancora esigibili: un nemico non rilevato non può diventare bersaglio dell'hover; il ghost di un alleato è **sola lettura** |
| **Replay/TurnLog** | `PlaybackRejectsPlanningInput` è una restrizione di input durante il playback |
| **Editor handoff** | `HUD-EDITOR-E5` |
| **Exit — `CODE READY`** | non raggiungibile prima che C3 produca hitbox. **Non aprire lavoro qui**: la fetta implementabile è già chiusa |

---

## C7 — Ready / Unready

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C7` |
| **Owner** | `#2193` — **CLOSED** 2026-09-04 · rischio aperto in `#2390` |
| **Stato live** | `VERIFY` — il codice c'è, la voce PIE no |
| **Intent** | dichiarare Ready non è committare: c'è un countdown, ed è annullabile |
| **Producer** | `ARTTurnManager` + `URTHudViewModel::BuildMatchHeader` / `ComputeSecondsUntilCommit` |
| **Contract** | `FRTMatchHeaderView::ReadyCountdownSecondsRemaining` e `SecondsUntilCommit`, con `-1` come «non si applica» |
| **Consumer** | `WBP_RT_TurnHeader` — **ricablato oggi**: `Get_TimerText_Text` legge `SecondsUntilCommit`, non `PlanningSecondsRemaining` |
| **Dependencies** | *hard*: C2, C3 |
| **Macchina a stati** | `Planning → ReadyCountdown → (Cancel → Planning \| scadenza → LockInAndResolve)` |
| **Vincoli** | UX a **wall-clock**: nessun campo nello snapshot, nessuno stato nel TurnLog, nessuno nello `StateHash`; il cap di planning vince; il piano sopravvive al Cancel |
| **Tests** | 19 `HudViewModel.*` (i due nuovi sulla regola dei due orologi) · 5 `HUD.MatchStatus*` |
| **Anti-vacuità** | baseline a countdown **zero**: con countdown disattivato il comportamento deve restare quello di prima |
| **Determinism** | ⛔ nessun secondo temporale entra nell'hash |
| **Privacy** | nessun impatto |
| **Editor handoff** | `HUD-EDITOR-E10` |
| **Exit — `CODE READY`** | ✅ raggiunto |

⚠️ **`#2390` è aperta e tocca questa riga**: l'anteprima potrebbe restare accesa se il primo turno si chiude
per timeout — il delegate ha un solo iscritto, e si iscrive col Ready.

---

## C8 — Unit overlay / status

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C8` |
| **Owner** | `#2288` e `#2347` — **CLOSED** 2026-09-04 · `#2378` mergiata 2026-09-05 |
| **Stato live** | `VERIFY` |
| **Intent** | guardando un'unità si capisce come sta e in che stato è |
| **Producer** | `URTHudViewModel::BuildUnitCard` / `BuildStatusBadges` / `BuildUnitOverlay` · `ARTHUD::ShouldDrawUnitOverlay` · `ClampOverlayAnchor` |
| **Contract** | `FRTUnitOverlayView{Card, TeamColor, bFriendlyFire, …}` · `FRTStatusBadgeView{IconId, RemainingTurns, bIsControl}` |
| **Consumer** | `URTUnitOverlayWidget` (`WidgetComponent`) + `WBP_RT_UnitOverlay`, con `IconCatalog` come `EditDefaultsOnly` |
| **Dependencies** | *hard*: C1 (le chiavi), C2 (i dati) |
| **Tests** | `HudViewModel.*` sui badge · `UI.HudOverlayClamp` · `HUD.Marks` |
| **Anti-vacuità** | fixture minima: uno stato **legato alla cella** (senza contatore) e uno **a durata** (con contatore) |
| **Determinism** | nessuno |
| **Privacy** | `ShouldDrawUnitOverlay(Entry, bIsOwnTeam)` decide sulla conoscenza: un nemico non visto non ha overlay |
| **Editor handoff** | `HUD-EDITOR-E9` |
| **Exit — `CODE READY`** | ✅ raggiunto. Resta `PIE-ICON-02` (seduta **U48**): le due icone di movimento devono distinguersi **fra loro, alla dimensione in cui si giocano** |

---

## C9 — `rt.Debug.*`

| Campo | Contenuto |
|---|---|
| **ID** | `HUD-CODE-C9` |
| **Owner** | `#80` (CP 11.4) |
| **Stato live** | `PARTIAL` |
| **Intent** | osservabilità per chi sviluppa, **mai** primaria nella vista del giocatore |
| **Producer, misurato oggi — 12 comandi, 4 file** | `RTDebugConsole.cpp` (8): `DumpCellPlacement` · `DumpSnapshot` · `DumpTurnLog` · `VerifyReplay` · `DrawIntent` · `DrawCover` · `DrawPaths` · `DrawResolution`; `RTHexLosConsole.cpp`: `Los`; `RTHexOverlayConsole.cpp`: `DrawCells`; `RTKnowledgeDebugConsole.cpp`: `Knowledge`; `RTPacingConsole.cpp`: `Pacing` |
| **Contract** | il namespace è il contratto: `Debug.NamespaceDeclaresAllCommands` fissa la regola *«il DoD elenca ciò che deve esserci, non tutto ciò che c'è»* |
| **Dependencies** | nessuna |
| **Tests** | `Debug.VerifyReplayDetectsDivergence` (introduce una divergenza e pretende che il comando la rilevi) · `Debug.NamespaceDeclaresAllCommands` |
| **Anti-vacuità** | il conteggio **va rimisurato, non copiato**: l'audit di oggi dice 12 registrati e 8 nel DoD, e i due numeri rispondono a domande diverse |
| **Privacy** | ⛔ `rt.Debug.DrawIntent` **non deve** rivelare gli intenti avversari — oggi banale perché offline, ed è ora che non si crea l'abitudine sbagliata (seduta U15) |
| **Editor handoff** | `HUD-EDITOR-E11` |
| **Exit — `CODE READY`** | quando i tre `DrawX` disegnano, o quando il DoD dichiara che stampano |

🔴 **Il debito di questa riga ha un nome**: `DrawPaths`, `DrawCover` e `DrawResolution` **stampano, non
disegnano** (`RTDebugConsole.cpp:163`). `rt.Debug.Los` stampa **per scelta dichiarata** e si chiama così
per non promettere un disegno. Se un comando dice `Draw`, la verifica è a schermo: `HUD-EDITOR-E11`.

---

## C10 — `CODE READY` batch gate

Prima di cedere Unreal alla lane VALIDATION, e poi alla Lane B:

- [ ] modifiche di codice complete e committate
- [ ] nessun Editor concorrente (`Get-Process *Unreal*` vuoto)
- [ ] nessun binario sporco (`git status --short` su `Content/`)
- [ ] documenti owner aggiornati **solo** se un contratto è cambiato
- [ ] elenco esplicito dei comandi di validazione, qui sotto
- [ ] elenco esplicito dei check Editor abilitati, in Lane B

### I comandi, mirati prima e la suite intera **una volta sola**

```powershell
# mirati, uno per fetta toccata
./scripts/rt-suite.ps1 -Filter "RefactorTactics.IconCatalog"                     # C1
./scripts/rt-suite.ps1 -Filter "RefactorTactics.HudViewModel"                    # C2
./scripts/rt-suite.ps1 -Filter "RefactorTactics.ScreenHud"                       # C3
./scripts/rt-suite.ps1 -Filter "RefactorTactics.UI"                              # C4
./scripts/rt-suite.ps1 -Filter "RefactorTactics.Preview"                         # C5
./scripts/rt-suite.ps1 -Filter "RefactorTactics.PlayerInput+RefactorTactics.Pointer"  # C6
./scripts/rt-suite.ps1 -Filter "RefactorTactics.HUD"                             # C7/C8
./scripts/rt-suite.ps1 -Filter "RefactorTactics.Debug"                           # C9

# una volta, sul batch
./scripts/rt-suite.ps1
```

Per ogni run si riporta: `command · HEAD · found N · performed N · passed N · failed N · exit code`.
**Regola dura**: su un filtro critico `performed > 0`. Una suite verde senza `performed` non è evidenza.

⚠️ **`exit 2` non è un fallimento**: la suite non è partita affatto (motore occupato o lock perso), e va
rilanciata. E una misura che vede cambiare `HEAD`, working tree, binario o processi Unreal è **NON VALIDA**,
non rossa.

**Stato che questo gate produce**: `CODE READY`. **Non** `VALIDATED`, e mai `EDITOR ACCEPTED`.
