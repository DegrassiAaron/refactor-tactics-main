# Brief — Planning visuale: Action Ghosts e Ghost Timeline

> **Stato**: analizzato, checkpoint proposti · **Data**: 2026-08-07
> **Sorgente**: [`../archive/src/design/action-ghosts-fasi-fast-reactions.md`](../archive/src/design/action-ghosts-fasi-fast-reactions.md) (22 sezioni)
> **Destinazione**: epic **E11** di [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) — CP 11.5 e CP 11.6, più un'estensione della DoD di CP 11.2

Il documento sorgente descrive **come si guarda un turno prima di confermarlo**: copie semitrasparenti del
personaggio (*Action Ghost*) disposte per fase (*Ghost Timeline*), scrubbing delle fasi, grammatica visiva
della certezza, e la reaction rappresentata come **ramo condizionale** invece che come quinta fase.

È materiale di **presentazione**. Nessuna delle sue proposte cambia una regola — ed è esattamente ciò che
lo rende sicuro da adottare: il renderer non decide nulla.

---

## 1. Cosa conferma — nessuna azione richiesta

Sette punti su venti del §20 sono **già canone**. Vanno letti come conferma esterna, non come richiesta:

| Punto della sorgente | Già deciso in |
|---|---|
| Ordine `Planning → Prep → Dash → Blast → Move`; il Move normale è l'**ultima** azione volontaria | ADR-0003 §1 · `ERTMatchPhase` invariato |
| Dash, Blink, Charge, Leap e displacement **non** sono la Move Phase | ADR-0003 §3 · `ERTMovementStyle` |
| Nessun Action Ghost nemico derivato dal Planning: gli intenti privati **non arrivano** al client | invariante **#6** · `FRTPlannedIntent → FilterForTeam → FRTIntentView` |
| Slow-motion è presentazione: non tocca ordine, seed, collisioni, path, esito | ADR-0004 §3 |
| Fast Reaction **decisa in Resolution**, non pre-programmata in Planning | ADR-0004 · brief overwatch |
| Niente macro `If Tank → HOLD / If Carry → FIRE` | ADR-0004, *Alternative considerate*: «filtro di bersaglio dichiarato» — **scartata** |
| Ghost e animazioni non sono autorità; il playback segue il TurnLog | invariante **#1** |

> La §12 della sorgente riscopre e motiva una decisione già presa. È un buon segno di coerenza del design,
> ma **non riapre** la scelta.

---

## 2. Cosa aggiunge — il lavoro vero

| # | Aggiunta | Perché conta |
|---|---|---|
| A1 | **Action Ghost per fase**: non solo la cella di destinazione, ma posizione **e posa significativa** per Prep / Dash / Blast / Move | Oggi l'anteprima è una linea di debug (CP 2.3). Il giocatore non vede *da dove* sparerà né *dove* finirà il dash |
| A2 | **Ghost Timeline a 4 slot** con **scrubbing**: seleziono una fase, il suo ghost si evidenzia e gli altri si attenuano; compaiono origine, target, linea, AoE, facing, cover, warning | È il modo per rendere leggibile un turno simultaneo in pochi secondi |
| A3 | **Grammatica visiva della certezza**: confermato = linea piena · previsto = tratteggiata + icona team · incerto = ghost dissolto + `?` | CP 11.2 definisce le tre **classi** ma non la loro **resa**: senza grammatica, tre stati diventano tre etichette |
| A4 | **Reaction come ramo condizionale**, mai quinta fase: `⚡ Reaction Armed` accanto alla timeline, con `?` | Impedisce che la UI insegni una sequenza sbagliata al giocatore |
| A5 | **Modello dati del view model** con `ReactionPreview` **separata** dalla lista lineare delle fasi | Un solo array «fasi + reaction» produrrebbe proprio la quinta fase che il design vieta |
| A6 | **Budget di presentazione**: pooling di mesh/decal, nessun Actor persistente per preview, aggiornamento a frequenza limitata (**non** ogni Tick) | Una preview che gira a Tick su 4 unità × 4 fasi è un costo silenzioso che si scopre al playtest |
| A7 | **Displacement reattivo ≠ Move Phase**: il movimento prodotto da una reaction è speciale e **non consuma né sostituisce** la Move | Vincolo per `Hero.Phase.FlowReaction` (E14) e per il troncamento del movimento in CP 14.5 |

---

## 3. Conflitti e trappole

**C1 — Il roster degli esempi è superato.** §7 usa *Aegis* e *Drift*. Roster vigente: **Gadget · Phase ·
Riktor · Wraith** (`showcase-v0.1.md` §0). Gli esempi restano validi come forma, non come contenuto.

**C2 — «Fog of War» non è il modello di questo progetto.** §6 elenca la FoW fra le cause d'incertezza. Il
canone non ha fog of war: la mappa statica resta nota, e la **conoscenza parziale** (tre livelli
`Nascosto / Incerto / Rilevato`) è **E13**. Conseguenza pratica: prima di E13 la classe *incerto* può
coprire solo ciò che dipende dall'avversario — bersaglio che può spostarsi, collisione possibile, reaction
avversaria — **non** la visibilità. Dichiararlo, invece di mostrare un `?` che finge un sistema assente.

**C3 — Il rischio maggiore: una seconda verità sul percorso.** Un renderer che calcola da sé dove finirà
l'unità diventa una seconda implementazione delle regole, e il giorno in cui diverge dal resolver il
giocatore vede una promessa che il gioco non mantiene. Vincolo: **la preview consuma lo stesso A\* e lo
stesso snapshot dell'autorità** — è già la regola acquisita in CP 2.3 (`MakeCurrentSnapshot`,
`BuildCompositeHexPath`), e la sorgente non la enuncia. Va scritta nella DoD.

**C4 — I warning non si ricalcolano nella UI.** Lo scrubbing (§5) promette avvisi: «un alleato attraversa la
traiettoria», «sarai esposto». I motivi devono arrivare dallo stesso strato che li produrrà nel TurnLog
(`URTTurnLogLibrary::DescribeEntry` è il precedente), non da una logica parallela nel widget. Dove il
warning è una **predizione** e non un esito, va marcato *previsto* o *incerto* — mai *confermato*.

**C5 — Il facing non esiste come dato di gioco.** La sorgente lo usa in due punti (`Facing` nel view model,
«in quale direzione sarà orientato»). Oggi il facing **non decide nulla**: `Actions.Wait.AllowsFacingAndReaction`
esiste proprio come promessa di non impedirlo, e nessuna regola di cover o LOS lo consuma.

> ✅ **Deciso il 2026-08-07** — [ADR-0005](../decisions/adr-0005-orientamento.md), epic **E16**. Il facing **diventa stato
> di gioco**, e la raccomandazione «derivato, finché nessuna regola lo consuma» è superata: i consumatori ora
> sono tre. Deriva dall'**ultima azione di movimento** (una direzione per i movimenti lineari, tre per il Move
> a budget — ultimo passo ±1 — sei da fermo) e decide **difesa** (fuori dall'arco frontale copertura e Guard
> non valgono), **percezione** (vista a cono + consapevolezza a 360° entro 2 celle, E13) e **reazioni** (il
> cono dell'Overwatch nasce dal facing, E14).
>
> Conseguenza per **CP 11.5**: il ghost deve mostrare il facing pianificato **e le direzioni legali**. Il campo
> `Facing` del view model smette di essere una posa decorativa: è una scelta, e sceglierla senza vederla
> significa sceglierla alla cieca.
>
> ⚠️ **Aggiornato il 2026-08-08 — [D-020](../decisions/RT_PDR_00_Decision_Log.md).** La frase «l'orientamento si
> fissa a fine Move» **non vale più**: un'azione con bersaglio orienta l'unità **prima di risolvere**, quindi
> il facing cambia **più volte dentro il round**. Per la preview la conseguenza è concreta: il ghost non può
> mostrare *un* facing per unità, deve mostrare **quello della fase selezionata** dallo scrubbing — il facing
> del Blast può differire da quello del Dash e da quello finale. Resta vero che il facing **finale** (dopo il
> `Move`) è quello che si eredita nel round successivo, ed è la scommessa che il giocatore deve poter vedere.

**C6 — Il lavoro era già cominciato altrove, ed è stato incorporato.** `Tests/RTPlanningPreviewTests.cpp` è in
`main` con la famiglia `Preview.*` (`HitCellsMatchCombatShape`, `AllyInAreaIsFlagged`,
`ClearedWhenPlanIsCancelled`, `ReachableCellsArePassedThrough`), insieme alle modifiche di `ARTHexMapActor`. È
lo **stesso asse** dei CP qui sotto — celle colpite, alleato nell'area, pulizia su annullamento — e va esteso,
non duplicato. *(La nota parlava di «working tree» e di un branch non ancora mergiato: mergiato.)*

---

## 4. Checkpoint proposti — epic **E11** (HUD, log e debug)

Non serve un'epic nuova: è presentazione dell'HUD e vive accanto a intenti, certezza e combat log.

| CP | Obiettivo | DoD misurabile | Test / verifica |
|---|---|---|---|
| **11.2** *(esteso)* | Intenti alleati e certezza — **con grammatica visiva** | Alle tre classi già previste si aggiunge la resa: confermato = linea piena · previsto = tratteggiata + icona di squadra · incerto = dissolto + `?`. La classificazione arriva dal resolver, la UI **non** ricalcola il perché. Finché E13 non esiste, l'incertezza da visibilità **non** viene mostrata | `UI.IntentCertaintyClassification`; `PIE-V01-INTENT` |
| **11.5** | **Ghost Timeline**: preview per fase | View model con una entry per fase (`Phase`, `UnitId`, `ActionId`, `PreviewOrigin`, `PreviewDestination`, `Facing`, `PoseId`, `TargetCells`, `AffectedCells`, `Certainty`) e **`ReactionPreview` separata**; ghost per Prep/Dash/Blast/Move sulla mappa; origine, destinazione, celle bersaglio e area coincidono con ciò che il resolver userebbe (**stesso A\***, stesso snapshot); pooling, nessun Actor persistente per preview, aggiornamento a frequenza limitata | `Preview.GhostMatchesResolverPath`, `Preview.HitCellsMatchCombatShape` *(esistente)*, `Preview.ReactionIsNotAPhaseEntry`, `Preview.ClearedWhenPlanIsCancelled` *(esistente)* |
| **11.6** | **Scrubbing** e ramo condizionale della reaction | Selezionando una fase il suo ghost si evidenzia e gli altri si attenuano, con origine/target/linea/AoE/cover in evidenza; i **warning** (alleato nell'area, esposizione, collisione possibile) arrivano dallo stesso strato dei reason code e sono marcati *previsto*/*incerto*; la reaction armata compare come **ramo con `?`**, mai come quinta colonna della timeline | `Preview.AllyInAreaIsFlagged` *(esistente)*, `Preview.WarningsComeFromResolverReasons`, `Preview.ArmedReactionRendersAsBranch`; `PIE-V01-GHOSTS` |

**Ordine consigliato** (dalla §22 della sorgente): PoC su **una** unità con i quattro slot — ghost di
posizione corrente, endpoint del Dash, posa d'attacco con linea/AoE, posizione finale del Move, selezione
della fase, tre stili di certezza, un arco di reaction condizionale. **Solo dopo**: ghost degli alleati,
warning di collisione, Fast Reaction reale, conoscenza parziale, multilivello, rifinitura delle animazioni.

---

## 5. Cosa resta fuori

- **Pose e animazioni d'autore** (anticipation, landing, aiming, shield, cast): appartengono alla
  presentazione di **M8**; i CP qui sopra chiedono che un `PoseId` esista e sia selezionabile, non che le
  animazioni siano prodotte.
- **Ghost su più livelli** con occlusione fra layer: rinviato, come indica la sorgente.
- **Facing autorevole**: **non più fuori scope** — è l'epic **E16** ([ADR-0005](../decisions/adr-0005-orientamento.md)).
  Resta fuori di qui la sua *implementazione*: CP 11.5 lo **mostra**, CP 16.1 lo **decide**.
