# Il fumetto per ogni azione e per le reazioni che scattano — design

> Issue: [#4059](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4059). Decisioni: [`D-604`](../../decisions/RT_PDR_00_Decision_Log.md) (chi grida, le reazioni, la riga) e [`D-605`](../../decisions/RT_PDR_00_Decision_Log.md) (quando grida una reazione, il perimetro, la fuga del Cleanup). Seguito della spec del fumetto, [`2026-11-08-fumetto-skill-design.md`](2026-11-08-fumetto-skill-design.md), e di quella del Registro, [`2026-11-10-registro-attivazione-design.md`](2026-11-10-registro-attivazione-design.md).
> Misure sul codice: lettura di `origin/main` = `297c1454c` il 2026-11-11, nulla eseguito.
> ⚠️ **I percorsi di `Source/` sono quelli di prima della riorganizzazione delle cartelle**, in corso in un'altra sessione mentre questa spec si scrive. Dopo il suo merge i file cambiano cartella; i simboli no, e la spec li nomina per simbolo.
>
> **Statuto**: non implementata.

## 1. Intento

`D-604` fa gridare il fumetto per ogni azione tranne `Action.Move`, `Action.Sprint`, `Action.Withdraw` e `Action.Interact`, attacco base compreso. Fa gridare anche le reazioni che scattano, e dà a una reazione la sua riga d'attivazione nel Registro. `D-605` dice quando grida una reazione innescata da un colpo («All'arrivo del colpo»). Mette nel perimetro di #4059 anche il colpo dell'Overwatch e la risposta del Brace, e fa gridare la fuga dal pericolo dopo il movimento. Dove una regola è una lettura dell'assistente, lo dice.

## 2. Il punto di partenza (misurato)

- **Chi grida.** `URTHudViewModel::ActionShouts` (`UI/HUD/RTHudViewModel.cpp`) risponde `BaseActionId.IsNone() && !bIsGenericOrEquipment`. `FRTPlaybackDirector::ShowActivation` (`Turn/Playback/RTPlaybackDirector.cpp`) calcola il bool con `FindCoreAction` e `FindEquipment`. A `ShowActivation` arrivano solo gli eventi `AbilityActivated`.
- **Le reazioni che hanno un evento.** `ReactionResolved` nasce solo in `ARTTurnManager::RunReactionPass` e in `ARTTurnManager::ResolveInterceptions`, sempre con `Phase = Blast`. Nessuno dei due punti scrive `SourceVerdict`, e un verdetto vuoto vale «nessuno». Il playback lo scarta: `RTRangoNellAtto` (`Turn/Playback/RTPlaybackLibrary.cpp`) gli dà −1, e `BuildBlastSequence` lascia fuori i ranghi negativi. In `RTPresentationBinding.cpp` è `PendingPresentation` (#2454).
- **I punti del passo delle reazioni**, nell'ordine della timeline:
  - interposizione (`ResolveInterceptions`) e `BlastHits` (Counter, Deflect, i moduli `Reaction.CounterShot`, `ReactiveShield`, `EmergencyDash`, le reazioni del kit) escono **prima** dell'attivazione e del colpo che le innescano;
  - `BlastDisplacement` (Anchor) e `BlastStatus` (Cleanse) escono dopo i colpi;
  - `CleanupSurfaceBirth` (`Reaction.HazardEscape`) esce dopo il movimento, dentro `ConcludeResolution`, ma con `Phase = Blast`. Anche il `Move` della sua fuga ha `Phase = Blast` fisso (`ApplyForcedDisplacement`), quindi oggi la fuga si anima durante il Blast, prima del movimento.
- **Il colpo dell'Overwatch** (`ARTTurnManager::ApplyReactionDecision`, ramo FIRE, fase Move) non scrive niente nella `ResolvedTimeline`: niente `Attack`, niente `AbilityActivated`, niente `ReactionResolved`. A schermo restano la rotta troncata del bersaglio e la sua barra della vita, già scesa (l'overlay legge la vita dell'unità, e la risoluzione applica il danno prima del playback), senza un momento loro. Il micro-passo del colpo sta solo nella voce di TurnLog (`MicroStepIndex`).
- **La risposta del Brace** nella sua finestra (`SubmitReactionResponse` → `CloseBraceWindow` → `ResumeSuspendedResolution` → `ResumeBlastResolution` → `ApplyDisplacements`, decisione in `RTTurnManager_Blast.cpp`) scrive una voce `ReactionDecision` e, per lo scarto, un `Move` con l'`ActionId` della spinta, che il playback anima. Nessun evento di presentazione suo. Il Brace ha un `AbilityActivated` solo quando si arma, in Prep.
- **Il colpo di ritorno del Contrattacco** non ha un evento `Attack`: l'unico produttore di `Attack` è il ciclo su `Plan.Hits`.
- **Il playback** ha le fasi Prep, Dash, Blast e Move. Il Cleanup non è mai una fase del playback («gia' applicato, nessun beat», nel commento di `BeginPlayback`), e `PhaseTime(Cleanup)` vale un solo beat. L'header ha quattro celle fisse (Prep, Dash, Blast, Move). Il Registro sa già mostrare la sigla «CLEANUP».
- **Il TurnLog.** La fase è il primo campo che entra nell'hash del TurnLog (`MixEntryFields`, `HashTurnLog`), nel formato binario e nell'ordinamento (`EntryLess`). Il passo `CleanupSurfaceBirth` scrive una voce per ogni unità che ha pianificato la fuga, anche quando non scatta.
- **`FRTResolvedEvent`** non è serializzato da nessuna parte (né replay, né hash, né golden). Un campo nuovo non tocca il formato versionato.

## 3. Regole

- **R1 — Chi grida** (`D-604` (1), (4), (5)). `ActionShouts` risponde falso solo se l'`ActionId` è `Action.Move`, `Action.Sprint`, `Action.Withdraw` o `Action.Interact`. `Action.Wait` resta senza fumetto a monte, perché non è un'attivazione del playback (`EmitAttackIntentActivation` lo salta), anche se `ActionShouts` per lui risponderebbe vero. Il criterio è l'`ActionId`, non la base: una skill del kit costruita su `Sprint` è una skill e grida. Non serve più sapere se un'azione è generica o d'equipaggiamento.
- **R2 — Il testo** (*lettura dell'assistente*, `D-604` (7)(b)). Il nome che il kit dà all'azione (`ARTUnit::KitDisplayNameFor`), come nella riga del Registro. Senza nome, il ripiego di R3 della spec del fumetto. ⚠️ Nei test le azioni del catalogo comune armate con `RTAbilityFixtures::AddCoreAbility` non hanno un `DisplayName`, quindi gridano col ripiego e scrivono un `Warning` («il kit non nomina la skill …»). In produzione il nome c'è.
- **R3 — Il verdetto delle reazioni** (*lettura dell'assistente*, `D-604` (7)(d)). Ogni evento di reazione porta il verdetto di chi reagisce, congelato quando nasce, con lo stesso soggetto della sua voce di TurnLog: `FreezeVerdictFor(Unit(chi reagisce))` nel passo delle reazioni, nell'interposizione e nel Brace, e `UnitAt(watcher, la sua cella)` per l'Overwatch. Il playback mostra una reazione solo se `SourceVerdict.AllowsTeam(chi guarda)`: gli stessi filtri delle attivazioni (`D-223`, `D-582` (3)(b)).
- **R4 — Una reazione del Blast grida all'arrivo del colpo** (`D-605` (1), «All'arrivo del colpo»). Nella sequenza del Blast l'evento si mette subito dopo l'`Attack` il cui bersaglio è chi reagisce e la cui sorgente è l'attaccante che l'ha innescata. È lo stesso atto del colpo: il fumetto si accende al battito dopo l'arrivo del colpo.
  - **L'attaccante che ha innescato** lo porta un campo nuovo di `FRTResolvedEvent`, uno `StableUnitId` con `0` come «nessuno» (`D-063`). Le sorgenti sono indici in `Ctx.Units`, convertiti con `Units[i]->StableUnitId` come già `TargetStableUnitId`. Nel `BlastHits` è chi ha innescato (`TriggeredBy`, già in `TargetStableUnitId`). Nell'interposizione è l'attaccante del colpo deviato (`Plan.Hits[HitIdx].AttackerId`): il `TargetStableUnitId` di quell'evento è l'alleato protetto, e resta com'è. Per il Brace e per l'Ancoraggio è chi spinge (`Ctx.KnockAttackerId`), se c'è. Per Cleanse e la fuga vale `0`.
  - **Se lo stesso attaccante colpisce il reattore più volte**, l'ancora è il primo di quei colpi in ordine di timeline.
  - **Con l'attaccante `0`**, l'ancora è il primo `Attack` sul reattore in ordine di timeline, che può non essere quello che ha portato lo stato: è un limite dichiarato. Senza nessun `Attack` sul reattore, la reazione resta un atto a sé, in ordine di timeline.
  - **Se il colpo d'ancora è già nel prefisso congelato** di una finestra di reazione (già mostrato), la reazione sta fra i primi elementi dopo il prefisso, nell'ordine degli atti, e si rivela subito dopo la ripresa. Fa eccezione uno Skip prima della finestra: il colpo non è stato mostrato, e la riga arriva dal recupero di R9.
  - *Lettura dell'assistente* (`D-605` (4)(a)): vale anche per Anchor e Cleanse, che reagiscono alla spinta e allo stato portati da un colpo, e per il Brace.
- **R5 — La risposta del Brace** (perimetro `D-605` (2); il momento è la *lettura dell'assistente* `D-605` (4)(d)). Quando la risposta si applica (Hold Ground o SIDESTEP, con o senza finestra), nasce un `ReactionResolved` con chi reagisce `T`, `ActionId = Action.Brace`, l'attaccante di R4 (chi spinge) e il verdetto di R3. Con la finestra la spinta è di norma già nel prefisso congelato, quindi l'evento sta fra i primi elementi oltre il prefisso (R4) e si rivela subito dopo la ripresa.
- **R6 — Il colpo dell'Overwatch** (perimetro `D-605` (2); il momento è la *lettura dell'assistente* `D-605` (4)(c)). Nel ramo FIRE nasce un `ReactionResolved` con fase `Move`, chi reagisce il watcher, bersaglio il bersaglio colpito, `ActionId` e `BaseActionId` dell'azione armata, e il verdetto di R3. Un campo nuovo di `FRTResolvedEvent` porta l'indice di cella del bersaglio al momento del colpo (`Entered.Num()` del suo stato di movimento), con `INDEX_NONE` come «nessuno». Il playback copia queste reazioni in una coda in `BeginPlayback` e le rivela nella fase Move quando l'animazione del bersaglio raggiunge quella cella (`MicroStepAtAlpha`). Di solito è l'ultima cella della sua rotta, perché `StopUnitInPlace` lo ferma lì. Il campo serve quando l'animazione è troncata al prefisso visibile prima di quella cella (`ObservedPrefixLength`), o quando il bersaglio non ha un'animazione visibile: è il caso comune di un nemico che entra nel cono da una zona non vista. In quei casi la reazione si rivela all'istante `indice / PlaybackCellsPerSecond`. **La fase Move si apre anche per una reazione visibile**, come R7 fa per il Cleanup, e la sua durata comprende l'istante della reazione più tarda: oggi la fase nasce solo da animazioni visibili, e dura quanto la rotta visibile più lunga. ⚠️ Non va indicizzato per posizione nella timeline: `EmitMoveEvents` toglie e riaccoda gli eventi `Move` della fase Move a ogni ripresa.
- **R7 — La fuga del Cleanup** (`D-605` (3)). Gli eventi di presentazione della fuga, il `ReactionResolved` e il `Move`, escono con `Phase = Cleanup`: `RunReactionPass` scrive la fase dell'evento secondo il punto del passo, non più `Blast` per tutti. Il playback ha una fase Cleanup dopo Move, che si apre solo se c'è una reazione o un'animazione del Cleanup visibile a chi guarda. La sua durata segue il modello del Dash: prima le reazioni, a un intervallo di `AttackShowSeconds` l'una, poi il movimento. *Lettura dell'assistente* (`D-605` (4)(b)): le voci di TurnLog della fuga restano in fase Blast, perché la fase entra nell'hash del TurnLog. Correggerle cambierebbe l'hash in ogni turno in cui la fuga è pianificata, anche senza fuga.
- **R8 — La scena.** Una reazione mostrata scrive la riga del log `Reagisce: <unità> -> <ActionId>` e la riga d'attivazione del Registro (`AddActivationLine`, stessa chiave anti-doppione `(fase, sorgente, azione)`), e grida per R1. Un'unità che ha già un fumetto lo vede sostituito, come oggi.
- **R9 — Il salto del playback** (R8 della spec del Registro). Il recupero di fine playback scrive le righe anche delle reazioni non ancora mostrate: quelle della sequenza del Blast, della coda del Move e della coda del Cleanup.
- **R10 — Il confine di un'azione.** Una reazione dentro l'atto di un colpo (R4) non è un confine nuovo per `Next Action`; una reazione in un atto a sé sì.
- **R11 — La presentazione.** `ReactionResolved` non è più `PendingPresentation`: lo presentano il fumetto e la riga. La tabella di `RTPresentationBinding.cpp` e il censimento dei test si aggiornano, e #2454 va avvisata.

## 4. Limiti dichiarati

- **Il colpo di ritorno del Contrattacco** non ha un evento `Attack`, e resta invisibile. `D-604` chiede il fumetto della reazione, non il colpo.
- **L'header** non ha una cella per il Cleanup: durante la fase Cleanup nessuna cella si accende. Una quinta cella vuole l'asset `WBP_RT_TurnHeader` e un colore di fase che la palette non ha. Resta un seguito.
- **La riga d'esito della fuga** (`ReactionFired`, dal TurnLog) resta marcata BLAST per R7; la sua riga d'attivazione è CLEANUP.
- **L'istante del colpo dell'Overwatch** rivela, a chi vede il watcher, anche quando il bersaglio è arrivato su quella cella. Non è un'informazione nuova: la voce di TurnLog del watcher porta già la cella del bersaglio e il micro-passo, sotto lo stesso verdetto.
- **La fuga di `EmergencyDash`** passa da `ApplyForcedDisplacement` con fase Dash ma scrive Blast. R7 cambia solo la fuga del Cleanup.

## 5. Architettura

- **`UI/HUD/RTHudViewModel`.** `ActionShouts(const FName& ActionId)`, pura, con l'elenco dei quattro id.
- **`Turn/Playback/RTPlaybackDirector`.**
  - `ShowActivation` perde il calcolo di `bIsGenericOrEquipment`.
  - `ShowReaction(const FRTResolvedEvent&)`, gemella di `ShowActivation` per R8.
  - Il ramo `ReactionResolved` in `AdvanceBlastSequence`, e `NotePlaybackActShown`/`IsActBoundary` per R10: oggi la coppia (chi reagisce, reazione) sarebbe un confine rispetto alla coppia (attaccante, attacco).
  - La coda e la rivelazione del Move (R6).
  - La fase Cleanup (R7): classificazione delle animazioni, apertura, `EnterPlaybackPhase`, rami di movimento, rete di fine fase.
  - Il recupero di R9 in `FinishPlayback`.
- **`Turn/Playback/RTPlaybackLibrary`.**
  - `RTRangoNellAtto` e `BuildBlastSequence`: il rango e l'ancoraggio di R4, il filtro di R3, il prefisso congelato.
  - `PhaseTime(Cleanup)`.
- **`Turn/Resolution/RTResolvedEvent.h`.** Due campi, con le sentinelle documentate: l'attaccante che ha innescato una reazione (R4) e l'indice di cella del bersaglio dell'Overwatch (R6).
- **`Turn/RTTurnManager*`.**
  - Il verdetto nei due punti che emettono `ReactionResolved` (R3).
  - La fase Cleanup degli eventi della fuga (R7), con un parametro di fase per l'evento in `ApplyForcedDisplacement` che non cambia la voce di TurnLog.
  - L'evento del Brace (R5).
  - L'evento dell'Overwatch (R6).
- **`Turn/Playback/RTPresentationBinding.cpp`.** R11.

## 6. Test (nomi indicativi, nelle famiglie esistenti)

- **Chi grida.** `HudViewModel.SkillCallout.ShoutsUnlessMoveOrInteract`, al posto di `ShoutsOnlyForASignatureSkill`:
  - sì per l'attacco base e le sue varianti, le azioni generiche (tranne `Interact`; `Wait` non arriva mai al fumetto), l'equipaggiamento, `Action.Dodge`, `Action.Leap`, `Action.Reposition` e le skill del kit;
  - no per i quattro id muti.
- **I test che fissano la regola vecchia** si riscrivono sulla nuova, senza indebolire gli asserti:
  - `Playback.SkillCallout.SignatureSkillShoutsAndBaseAttackDoesNot`;
  - `Playback.SkillCallout.EquipmentActionDoesNotShout`;
  - gli asserti sul fumetto di `Playback.ActivationLine.EveryActionKindHasALine`.
- **Le reazioni del Blast.** `Playback.SkillCallout.ReactionShoutsWhenTheHitArrives`: un Counter della squadra di chi guarda. Il fumetto e la riga nascono nel battito dopo l'arrivo del colpo sul reattore, non prima. Il controllo è un'interposizione.
- **Privacy.** `Playback.SkillCallout.HiddenReactorDoesNotShout`: la reazione di un nemico non osservato (dietro un muro alto, come `Playback.ActivationLine.HiddenSourceHasNoLine`) non mostra né fumetto né riga.
- **Il Brace.** `Playback.SkillCallout.BraceResponseShoutsAfterTheWindow`: con la finestra, la risposta grida al primo tick dopo la ripresa.
- **L'Overwatch.** `Playback.SkillCallout.OverwatchShotShoutsAtTheTargetCell`: il fumetto del watcher nasce quando il bersaglio raggiunge la cella del colpo. E `Playback.SkillCallout.OverwatchShotShoutsForAnUnseenTarget`: il bersaglio arriva da una zona non vista, non ha un'animazione visibile, e il fumetto del watcher nasce lo stesso.
- **La fuga.** `Playback.SkillCallout.HazardEscapeShoutsAfterTheMove`: il fumetto e l'animazione della fuga stanno nella fase Cleanup, dopo il movimento. Più un test puro su `PhaseTime(Cleanup)`.
- **Il salto.** `Playback.ActivationLine.SkipRevealsTheReactions`: il salto lascia la riga di ogni reazione scattata, senza doppioni.
- **La presentazione.** Il censimento `Presentation.AbsenceCensusIsPinned` si aggiorna (R11).
- **Da rivedere:** `Playback.PhaseTimeSplitsShownFromSlack` e `Playback.PhaseDurationIsOneBeatWithoutMotion`, che fissano il Cleanup a un beat, e `HudViewModel.MatchHeaderCarriesThePlayedPhase` («il campo non vale mai Cleanup»).
- **Suite intera `RefactorTactics`:** il percorso del playback è condiviso.

## 7. Gate

- **Compile.**
- **Automation:** i test nuovi e la suite intera.
- **Determinism e Replay:** **N/A** per gli eventi di presentazione, ma da misurare. L'hash del TurnLog non deve cambiare (R7), e un golden del TurnLog lo conferma.
- **Privacy:** i test di R3.
- **PIE:** le voci `PIE-HUD-FUMETTO` e `PIE-HUD-REGISTRO-ATTIVA` si rimisurano. Diventano giudicabili anche `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE`, che aspettavano un momento per la reazione (`RTPresentationBinding.cpp`).
- **Packaged:** `NOT RUN`, lo misura `G17`.
