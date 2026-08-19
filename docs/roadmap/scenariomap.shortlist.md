# Scenario map — shortlist del corpus

> `GENERATA` · il blocco §1 lo riscrive `python scripts/feature_registry.py shortlist`, misurando
> `Scenarios/` e le capability dichiarate in `RTScenarioSession.cpp`.
> **Cosa è**: l'elenco corto di cosa si verifica da solo e cosa richiede una persona.
> **Cosa non è**: l'owner. Il documento normativo — con la motivazione voce per voce e il subset
> `RELEASE-V01` del gate `G9` — è [`../technical/tooling/scenario-map.md`](../technical/tooling/scenario-map.md).
> Le regole di identità e tag stanno in
> [`../technical/tooling/scenario-index-e-tag.md`](../technical/tooling/scenario-index-e-tag.md).

⚠️ **La cartella non è la classe.** Uno scenario si classifica leggendo il suo `requires` e la disponibilità
della capability, **mai** il percorso: le cartelle sono storage e non promettono nulla.

---

## 1. Il corpus, misurato

<!-- RT_SHORTLIST_SCENARIOS:BEGIN -->

**78 scenari versionati** — misurati su `Scenarios/`: **67** eseguibili · **11** `BLOCKED` per una capability assente · **66** dichiarati `planned` nel registry e non ancora scritti.

**Capability disponibili oggi**, lette da `RTScenarioSession.cpp` (stanno nel codice, non nei dati: un JSON che se le dichiarasse da sé produrrebbe il primo verde bugiardo): `BotPlanning` · `Cover` · `CreateCover` · `DecisionBoundary` · `DeclaredRotation` · `Environment` · `EnvironmentalActionOwner` · `FixtureReference` · `PredictiveAction` · `Reaction` · `ReactionPlanning` · `Structures`.

| Scenario `BLOCKED` | Capability che manca |
|---|---|
| `RT_Showcase_Relay_v01` | `InterceptRevalidation` · `Objective` |
| `Spec.Brace.ProfileChangesResponse` | `ReactionProfile` |
| `Spec.Clash.ReadBeatsStand` | `ReactionClash` |
| `Spec.Clash.ShiftBeatsRead` | `ReactionClash` |
| `Spec.Clash.StandBeatsShift` | `ReactionClash` |
| `Spec.Clash.TieAppliesOnce` | `ReactionClash` |
| `Spec.Movement.AntiDashTriggerIgnoresMove` | `SemanticTrigger` |
| `Spec.Movement.TeleportSkipsIntermediateCells` | `Teleport` |
| `Spec.Movement.TripwireOnCrossEdge` | `SpatialTrigger` · `Teleport` |
| `Spec.Objective.PointSurvivesKO` | `Objective` |
| `Spec.Perception.HeardNotSeen` | `Perception` |

| Pianificato, non scritto | Feature che lo chiede |
|---|---|
| `AutoBattle.Hazard` | `RT-FEAT-MATCH-AUTOBATTLE` |
| `AutoBattle.Objective` | `RT-FEAT-MATCH-AUTOBATTLE` |
| `AutoBattle.Obstacles` | `RT-FEAT-MATCH-AUTOBATTLE` |
| `AutoBattle.OpenField` | `RT-FEAT-MATCH-AUTOBATTLE` |
| `Spec.ActionEconomy.MoveImpairsPrecision` | `RT-FEAT-ACTION-MOVEMENT-COMPAT` |
| `Spec.ActionEconomy.OverwatchReservesMovementSlot` | `RT-FEAT-ACTION-PLAN-VALIDATION` |
| `Spec.ActionEconomy.PathLengthChangesEffect` | `RT-FEAT-ACTION-MOVEMENT-COMPAT` |
| `Spec.ActionEconomy.SprintBlocksPrecision` | `RT-FEAT-ACTION-MOVEMENT-COMPAT` |
| `Spec.ActionEconomy.SprintEnhancesMomentum` | `RT-FEAT-ACTION-MOVEMENT-COMPAT` |
| `Spec.Bot.BeliefDoesNotBecomeKnowledge` | `RT-FEAT-BOT-BELIEF` |
| `Spec.Bot.CandidateDiversityKeepsControl` | `RT-FEAT-BOT-TACTICAL` |
| `Spec.Bot.DecoyNoiseIndistinguishable` | `RT-FEAT-BOT-FAIRNESS` |
| `Spec.Bot.GraphRevisionInvalidatesBelief` | `RT-FEAT-BOT-BELIEF` |
| `Spec.Bot.HoldOnUnaccountedThreat` | `RT-FEAT-BOT-PREDICTIVE` |
| `Spec.Bot.InformationGainIgnoresHiddenContent` | `RT-FEAT-BOT-BELIEF` |
| `Spec.Bot.OverkillMovesSecondUnit` | `RT-FEAT-BOT-TACTICAL` |
| `Spec.Bot.PlanHysteresisIgnoresSmallDelta` | `RT-FEAT-BOT-TACTICAL` |
| `Spec.Bot.RobustPlanSurvivesFlank` | `RT-FEAT-BOT-PREDICTIVE` |
| `Spec.Bot.TeamPlanRejectsHardConflict` | `RT-FEAT-BOT-TACTICAL` |
| `Spec.Bot.TemporalSynergyRequiresPhaseOrder` | `RT-FEAT-BOT-TACTICAL` |
| `Spec.Brace.GlanceOffersOnlyLegalSides` | `RT-FEAT-REACTION-PROFILE` |
| `Spec.Brace.GroundingChargesOnDeclaredTerrain` | `RT-FEAT-REACTION-PROFILE` |
| `Spec.Brace.SidestepRedirectsToLegalHexOnly` | `RT-FEAT-REACTION-PROFILE` |
| `Spec.Clash.Determinism` | `RT-FEAT-REACTION-CLASH` |
| `Spec.Clash.HiddenUntilReveal` | `RT-FEAT-REACTION-CLASH` |
| `Spec.Clash.RevealIsFixedDeadline` | `RT-FEAT-REACTION-CLASH` |
| `Spec.Map.DoorOpensTransition` | `RT-FEAT-MAP-TRANSITION-CLEARANCE` |
| `Spec.Map.FootprintCollisionBlocksCell` | `RT-FEAT-MAP-STANDABILITY` |
| `Spec.Map.Interaction.OpenFailsDependentMoveBlocks` | `RT-FEAT-MAP-INTERACTION-GRAPH` |
| `Spec.Map.Interaction.SwitchControlsMultipleDoors` | `RT-FEAT-MAP-INTERACTION-GRAPH` |
| `Spec.Map.Interaction.SwitchOpensDoor` | `RT-FEAT-MAP-INTERACTION-GRAPH` |
| `Spec.Map.NinetyDegreeCornerBakesCorrectly` | `RT-FEAT-MAP-STANDABILITY` |
| `Spec.Map.ValidCellsBlockedTransition` | `RT-FEAT-MAP-TRANSITION-CLEARANCE` |
| `Spec.Map.WallCrossesCellStillStandable` | `RT-FEAT-MAP-STANDABILITY` |
| `Spec.Overwatch.ConductiveDischargeUsesStandardConduction` | `RT-FEAT-REACTION-OVERWATCH` |
| `Spec.Overwatch.FrontlineFollowsFacing` | `RT-FEAT-REACTION-OVERWATCH` |
| `Spec.Overwatch.PressurePushChangesResolvedPath` | `RT-FEAT-REACTION-OVERWATCH` |
| `Spec.Privacy.HiddenEnemyHoverNoLeak` | `RT-FEAT-UI-POINTER-INTERACTION` |
| `Spec.TimeBank.BotDrainsLikePlayer` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.ClashCostsFullWindow` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.ControlLoadNeverExtendsWindow` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.ControlLoadScalesInitialBank` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.DrainsAfterGrace` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.ExhaustionKeepsResponsesLegal` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.GraceDoesNotDrain` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.NeverBelowZero` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.PacketOrderInvariant` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.ReplayReadsRecordedBank` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.TimeoutCostsFullWindow` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.TimeoutIgnoresPreferredResponse` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `Spec.TimeBank.TimeoutSpendsNoCharge` | `RT-FEAT-CORE-DECISION-TIME-BANK` |
| `State.Hero.Gadget.Charged` | `RT-FEAT-CHARACTER-STATE` |
| `State.Hero.Phase.Flow` | `RT-FEAT-CHARACTER-STATE` |
| `State.Hero.Riktor.Bulwark` | `RT-FEAT-CHARACTER-STATE` |
| `State.Howitzer.Siege` | `RT-FEAT-CHARACTER-STATE` |
| `State.MultiState.Stress` | `RT-FEAT-CHARACTER-STATE` |
| `Stress.4v4.CoreRoster` | `RT-FEAT-STRESS-4V4` |
| `Team.Conflux.GadgetPhase.ConductiveFlood` | `RT-FEAT-FACTION-SCENARIOS` |
| `Team.Constrine.RiktorWraith.OnlyExit` | `RT-FEAT-FACTION-SCENARIOS` |
| `Team.Resonance.AuroraKwang.FrozenAnchor` | `RT-FEAT-FACTION-SCENARIOS` |
| `Team.Sentinel.SteelMurdock.HoldTheLine` | `RT-FEAT-FACTION-SCENARIOS` |
| `Visual.UI.AllyIntentInspectReadOnly` | `RT-FEAT-UI-POINTER-INTERACTION` |
| `Visual.UI.DoorHoverAndInteract` | `RT-FEAT-UI-POINTER-INTERACTION` |
| `Visual.UI.ReactionWindowPreemptsWorldInput` | `RT-FEAT-UI-POINTER-INTERACTION` |
| `Visual.UI.SelectMoveCancel` | `RT-FEAT-UI-POINTER-INTERACTION` |
| `Visual.UI.TargetEnemyConfirmCancel` | `RT-FEAT-UI-POINTER-INTERACTION` |

<!-- RT_SHORTLIST_SCENARIOS:END -->

## 2. Le quattro classi in una riga

Quanti stiano in **A** e quanti in **B** non è derivabile dai file — è **dove sta l'oracolo**, e lo decide
una persona. I conteggi qui sotto sono quelli di
[`../technical/tooling/scenario-map.md`](../technical/tooling/scenario-map.md), misurati il **2026-08-09**; il totale del
corpus e i bloccati sono invece generati in §1 e vincono sempre.

| Classe | Chi esegue | Chi giudica | Quanti *(owner; `C` e fuori classe rimisurati il 2026-08-13)* |
|:--:|---|---|---|
| **A** | la macchina | l'assertion | **26** scenari |
| **B** | la macchina | **una persona** che guarda | **21** scenari ↔ 21 voci `PIE-VIS-*` |
| **C** | una persona | una persona | **112** voci PIE *(diceva `95`)* |
| **D** | — | — | vedi §1: i `BLOCKED` e i `planned` sono misurati |
| *fuori classe* | la macchina | una persona sceglie **quando** | **2** voci — `PIE-MUT-BASTION-SLOW` e `PIE-MUT-ACTIONS-ZERO` *(diceva 1)* |

In B l'assertion esiste comunque: garantisce che ciò che stai guardando sia lo **stato giusto**, perché uno
scenario visivo senza assertion può mostrarti una bellissima animazione di un colpo che ha mancato.

> ✅ **I due scarti che la misura di §1 aveva trovato sono stati corretti nell'owner** il 2026-08-09:
> la classe A passa da 21 a **24** (i tre `EnvironmentalActionOwner` si sono accesi con `#282`) e i
> `planned` da 13 a **21** — erano ventuno elencati sotto un totale di tredici. Nello stesso pomeriggio
> `#346` ne ha aggiunti due in `Scenarios/Combat/`, e la classe A è arrivata a **26**: le due correzioni
> sono state calcolate su rami diversi e riconciliate al merge. Restano scritti a mano solo i conteggi
> **A/B/C**, che dipendono da dove sta l'oracolo e non dai file.

---

## 3. Classe A — automatico, nessun umano · 27

Eseguiti in blocco da `RefactorTactics.Scenario.EveryShippedScenarioRuns`, che **scopre il corpus dall'indice**:
aggiungere un file basta perché venga eseguito. Nessuno di questi compare nel registro PIE, ed è corretto.

**`Scenarios/Combat/` · 9** — il colpo diretto arriva (`BasicAttack`) · il muro ferma la **vista**, non il
passaggio (`BlockedByWall`) · lo scudo assorbe **e** restituisce (`CounterStrikesBack`) · l'AoE prende anche
l'alleato (`FriendlyFire`) · la linea prende chi sta sulla traiettoria (`LineHitsThrough`) · il contrattacco
richiede un'arma (`NoCounterWhenUnarmed`) · l'area prende gli alleati ma **non** chi la lancia
(`SplashHitsAlliesNotSelf`) · lo `Slow` del Blast accorcia il **Move dello stesso turno**
(`RiktorImpactShotSlows`) e il suo gemello di controllo lo dimostra per assenza (`MoveIsFullWithoutSlow`).

**`Scenarios/Movement/` · 6** — il passo arriva sulla cella pianificata (`Basic`) · **`BasicFailsOnPurpose`** è
l'unica prova che l'harness sappia dire «rosso» · una destinazione bloccata non produce percorso (`Blocked`) ·
chi cede la cella contesa e con quale reason (`Collision`) · due unità attraversano l'arena su due turni
(`LongWalk`) · lo scambio A↔B è rifiutato **in pianificazione** (`SwapRejectedByPlanning`).

**`Spec.Facing.*` · 6** — accesi da soli alla chiusura di E16: il Move fissa il facing e **persiste**
(`DerivesFromMove`) · il Dash riscrive prima del Blast (`DashReorients`) · il bersaglio orienta prima di
risolvere (`TargetingReorients`) · `Guard` riduce di fronte (`FrontAttackKeepsGuard`) e **non** da dietro
(`BackAttackIgnoresGuard`) · `Brace` invece tiene da ogni lato (`BraceHoldsFromBehind`).

**`EnvironmentalActionOwner` · 3** — accesi da `#282` il 2026-08-09, quando le azioni ambientali hanno avuto
un **possessore**: la scarica corre sul grafo dell'acqua perché un eroe la innesca
(`Spec.Environment.ElectricPropagation`) · l'acqua spegne le fiamme (`…WaterQuenchesFire`) · rompere un arco
**annulla** il percorso invece di allungarlo (`Spec.Map.BridgeBreaksThePath`).

**+3** — `Spec.Cover.TemporaryCoverExpires` (acceso da E9.5: una copertura temporanea **scade**) ·
`Spec.Predictive.WhiffOnEmptyCell` (acceso da E18: la previsione **sbagliata** trova il vuoto, ed è il
turno in cui *non* succede niente a dimostrare che si sta scommettendo) · `RT_Showcase_Relay_v01`
(gli 8 turni della showcase, oggi **BLOCKED** su 4 capability, arrivata a **3 turni giocati**).

## 4. Classe B — automatico + occhio · 21

Tutti in `Scenarios/Visual/`, uno per voce `PIE-VIS-*`. Si eseguono scegliendo lo scenario e premendo Play;
l'oracolo è chi guarda.

| Gruppo | Scenari | Cosa deve **vedersi** |
|---|--:|---|
| `Visual.Combat.*` | 8 | `Guard` toglie 15 al **primo** colpo, `Brace` 10 a **ogni** colpo · il KO mai prima del colpo · il piano rivalidato invece di un colpo a caso · Riktor incassa e non arretra · il fumo lascia **vedere** e non colpire · il bonus elettrico viene dal **terreno** |
| `Visual.Environment.*` | 3 | Fuoco in **due** momenti distinti (ingresso e Cleanup) · il terzo passo su ghiaccio è **subìto** · `Wet` è un'**assenza**: i danni del Cleanup che non arrivano |
| `Visual.Map.*` | 5 | La copertura è di un **bordo** e si vede quale · la barriera alta **nega**, la bassa **riduce** · due colpi identici dall'altura (nessun bonus, D-024) · la porta chiusa allunga il giro · l'**unica** transizione fra layer |
| `Visual.Movement.*` | 2 | La carica si legge diversa dal passo · un rifiuto è **muto**: non deve accadere niente |
| `Visual.Reaction.*` | 2 | 22 diventano 2 (`Deflection`) · il colpo **cambia destinatario** a mezz'aria (`Interposition`) |
| `Visual.Core.PhaseOrder` | 1 | `Dash → Blast → Move` in tre momenti separati |

## 5. Classe C — solo input umano · 112 voci PIE

Nessuno scenario può sostituirle: richiedono mouse, editor, giudizio o cronometro.

| Sezione del registro | Voci | Perché serve una persona |
|---|--:|---|
| Checklist principale (materiali, editor mode, bot) | 43 | **Gesto e asset**: drag, gizmo, Undo, materiali da creare in editor |
| Partita su griglia esagonale (M6) | 16 | **Ciò che solo l'occhio vede**: unità centrate, fluidità del playback |
| Contenuto della v0.1 | 22 | **Leggibilità**: che il giocatore *capisca* dal log |
| Verifiche di mutazione — **solo `PIE-V01-ARENA`** | 1 | Le altre due della sezione sono `PIE-MUT-*`, *fuori classe* |
| Stati del personaggio (E34) | 10 | Nessuna: **verificano un sistema che non esiste** — classe D travestita da C |
| Scenario Test Harness | 6 | «Premo Play e parte da solo», con esito leggibile senza aprire l'Output Log |
| Durata, ritmo e scala | 5 | **Cronometro**: producono numeri di playtest, non superano gate |
| Bot — leggibilità delle decisioni | 5 | **Il perché, non il cosa** |
| Strumenti di leggibilità | 2 | Giudizio a schermo puro |
| Formato e icone | 2 | Riconoscibilità alla dimensione reale dell'HUD |

Registro: [`../technical/test-manuali-pie.md`](../technical/test-manuali-pie.md).

> 🔁 **Rimisurata il 2026-08-13.** Diceva **95** su nove sezioni; le sezioni di classe C sono **dieci** —
> mancava *«Verifiche di mutazione»* per intero — e due erano indietro (Checklist `31`→`43`, Contenuto
> `18`→`22`). ⚠️ Queste righe stanno **fuori** dal blocco `RT_SHORTLIST_SCENARIOS` generato: sono prosa a
> mano in un file generato, e nessun `--check` le confronta con l'owner. Si rileggono da
> [`../technical/tooling/scenario-map.md`](../technical/tooling/scenario-map.md) §5, con l'`awk` per sezione che sta lì.

## 6. Classe D — dichiarato, non eseguibile

### 6.1 Scritti e `BLOCKED` — **l'elenco è in §1**

Descrivono una feature che **non esiste**, dichiarano la capability in `requires`, escono `BLOCKED`
nominandola e **si accendono da soli** quando atterra. `BLOCKED` è trattato verde: trattarlo come rosso
renderebbe irrazionale scriverne in anticipo.

Chi li accende: `DecisionBoundary`/`ReactionClash` → **E14** (CP 14.7) · `Perception` → **E13** (`#151`) ·
`Objective` → **E10** (CP 10.2, `#75`).

> ✅ **`PredictiveAction` è atterrata il 2026-08-10** (E18, issue `#225`): `Spec.Predictive.WhiffOnEmptyCell`
> si è acceso da solo ed è **`PASS`**, 4/4 assertion su 2 turni. È la terza volta che il meccanismo funziona
> dopo i sei `Spec.Facing.*` e `Spec.Cover.TemporaryCoverExpires`, e la prima in cui uno scenario-specifica
> arriva col proprio *«completare»* già scritto dentro: il campo `_nota_da_completare` diceva a chi
> implementava quale assertion mancava e come dichiarare la previsione. Ha risparmiato la domanda.
>
> Nello stesso movimento `RT_Showcase_Relay_v01` è passato da **1 a 3 turni giocati** — restava `BLOCKED`
> sul T2, che chiedeva proprio questa capability, e il T3 non chiedeva nulla. Resta `BLOCKED`, ora sul T4.

> ✅ **`EnvironmentalActionOwner` è atterrata il 2026-08-09** (`75b8264`, issue `#282`): non era un'epic da
> costruire ma una issue di **cablaggio** — il sistema era chiuso, mancava *chi possiede* le azioni. I tre
> scenari che la chiedevano si sono accesi da soli, e §1 lo misura. Il commento nel codice dichiara il
> confine: la capability **non** copre `Action.Ignite` né `Action.ModifyArc`, che per D-046 restano senza
> owner in v0.1.

### 6.2 Pianificati, non ancora scritti — **l'elenco è in §1**

Dichiarati in `feature-registry.yaml` sotto `scenarios: {planned: […]}`, dove compaiono come **warning** del
validator — un piano che non diventa un file resta visibile invece di sparire.

> ✅ **I 13 scenari `Spec.Clash.*` / `Spec.TimeBank.*` sono «da scrivere», e nient'altro** — rimisurato il
> 2026-08-13. Questa nota li dichiarava *impossibili* e ne contava **11**: erano tre `Spec.Clash.*` più otto
> `Spec.TimeBank.*`, che la riconciliazione di `#361` ha portato a **dieci**. `ERTAssertionKind` ha oggi
> **otto** assertion: alle cinque di stato finale si sono aggiunte `LogEventCount` e `LogEventOrder` (`#318`)
> e `LogEventAmount` (`#361`, `a7e4677b` del 2026-08-10). Ordine degli eventi, contatore e valore del TurnLog
> — ciò che mancava — si leggono tutti. Li scrivono CP 14.7 e CP 14.8.

### 6.3 Dichiarati e mai scritti · 4

La *fascia D* di [`../technical/runbooks/scenari-validazione-visiva.md`](../technical/runbooks/scenari-validazione-visiva.md)
elenca 8 scenari `Visual.*` «scritti adesso»: **nessuno degli otto file esiste**. Quattro temi sono stati poi
scritti come `Spec.*` — la forma migliore, una specifica eseguibile invece di una vetrina cieca. Restano
scoperti `Visual.Facing.Cone`, `Visual.Intercept.Revalidation`, `Visual.CoverWindow.OpenFireSeal` e il quarto.

---

## 7. Le due cose da ricordare

1. **«Non più bloccato» non vuol dire «verde».** Che le assertion tengano lo dice la suite, non una tabella.
2. **Il meccanismo funziona**: i sei `Spec.Facing.*` e `Spec.Cover.TemporaryCoverExpires` si sono accesi da
   soli il 2026-08-09, alla chiusura di E16 ed E9.5, senza che nessuno dovesse ricordarsi di promuoverli.
   È la differenza fra uno scenario-spec e una nota in un documento.
