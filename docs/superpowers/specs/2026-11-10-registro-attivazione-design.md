# Il Registro nomina ogni attivazione che il playback mostra — design

> Issue: [#4033](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4033). Decisione: `D-600` (1), che compie la seconda metà di `D-582` (3). Prima fetta (il fumetto): [#4025](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4025), spec [`2026-11-08-fumetto-skill-design.md`](2026-11-08-fumetto-skill-design.md).
> Misure sul codice: lettura di `origin/main` = `53b5fc634` il 2026-11-10, nulla eseguito.
>
> **Statuto**: implementata in `feat/4033-registro-attivazione` (`712ccecbf`: la riga d'attivazione e la sua fusione nel feed, pure e testate; `c223bbeba`: `ShowActivation` scrive la riga, il salto recupera le rimaste, il feed le fonde; `71d91d692`: il giro di correzioni della review finale, con l'ordine delle fasi dopo un salto di R8 e la diagnostica del feed). Suite `RefactorTactics` intera verde a `71d91d692`: 3746/3746 `Result={Success}`, `EXIT CODE: 0`. PIE ⏳: la voce `PIE-HUD-REGISTRO-ATTIVA` di [`test-manuali-pie.md`](../../technical/test-manuali-pie.md) esiste e nessuna sessione l'ha misurata a schermo. Packaged `NOT RUN` (lo misura `G17`).

## 1. Intento

In vista strategica il fumetto della skill non si vede (`D-582` (3)): il giocatore deve poter leggere nel Registro a destra **quale azione è partita e di chi**. L'autore ha deciso (`D-600` (1)) che la riga compare **sempre** (vista tattica e strategica), **per tutte le azioni** che il playback mostra come attivazione, gadget compresi, come **riga a sé** fuori dalla dominanza, **nell'istante in cui il playback la mostra**. Le altre righe del Registro restano come oggi.

## 2. Il punto di partenza (misurato)

- Il Registro (`URTPlayerEventLogWidget`, `RTScreenHudWidgets.cpp`) rilegge a 5 Hz `URTHudViewModel::BuildPlayerEventFeed(TurnManager, …)`, che proietta il **TurnLog** dell'ultimo turno (`URTPlayerEventProjector::Project`), tiene le ultime `MaxFeedLines` righe e compone il testo con `ComposePlayerEventText`. `PickEventLogLines` sceglie fra feed vivo e righe del turno precedente.
- Un'attivazione **non è nel TurnLog**: è un `FRTResolvedEvent` di tipo `AbilityActivated` in `ResolvedTimeline` (`RTServerOnly`, fuori dagli hash), emesso da `EmitAbilityActivated`.
- Il playback la mostra in `ARTTurnManager::ShowActivation`, chiamata da `RevealPlaybackActivations` (Prep, Dash) e da `AdvanceBlastSequence` (Blast). A monte, `BeginPlayback` e `BuildBlastSequence` scartano le sorgenti che chi guarda non osserva (`SourceVerdict.AllowsTeam`): a `ShowActivation` arriva solo ciò che chi guarda può vedere.
- Il proiettore tiene **una riga per unità** (dominanza, `D-299` (2), #1936 §E). Le righe senza unità (`UnitId == INDEX_NONE`) ne sono esenti.
- Il Registro è visibile nelle due viste: nessun widget dello schermo legge `IsStrategicView()`.
- `SkipPlayback` chiama `FinishPlayback`, che **non** mostra le attivazioni rimaste.

## 3. Regole

- **R1 — Una riga per attivazione mostrata.** Ogni chiamata di `ShowActivation` produce una riga del Registro, qualunque sia l'azione: skill del kit, attacco base, azione generica, gadget o modulo del loadout. Non dipende da `ActionShouts`, che resta la regola del solo fumetto. *Lettura dell'assistente* (`D-600` (4)(a)): `Move` e `Wait`, azioni generiche per `D-025`, non sono attivazioni del playback (`EmitAttackIntentActivation` le salta), quindi non hanno riga; il movimento resta silenzioso come vuole #1936 §D.
- **R2 — Privacy ereditata, nessun secondo giudizio.** La riga nasce solo in `ShowActivation`, quindi vede solo ciò che il playback mostra a chi guarda. Non si rilegge la conoscenza e non si filtra di nuovo (`D-223`, `D-600` (4)(g)).
- **R3 — Fuori dalla dominanza.** La riga d'attivazione non passa dal proiettore: non sostituisce nessuna riga e nessuna riga la sostituisce. La catena di `D-299` (2) fra le righe d'esito non cambia.
- **R4 — Il momento.** La riga entra nel feed nell'istante in cui `ShowActivation` gira, non all'inizio del playback. In pausa non ne arrivano; al passo avanti arrivano con le azioni.
- **R5 — Il testo.** «`<unità>` — `<azione>`», con il trattino lungo come le righe di oggi. L'unità è il nome che l'HUD usa già per l'eroe (`ARTUnit::DisplayLabel` sui campi dell'unità). L'azione è il nome del kit (`ARTUnit::KitDisplayNameFor`); senza nome leggibile, l'ultimo segmento dell'id, come il ripiego di `BuildSkillCallout`, ma senza maiuscolo. Il testo si compone **quando la riga nasce**, perché lì c'è l'unità; dopo, la riga non legge più il mondo. `Importance = Important` (#1936 §D: «uso di abilità»); fase e unità sorgente valorizzate.
- **R6 — Ordine e budget** *(lettura dell'assistente, ribaltabile)*. L'autore ha letto due frasi: «il Registro ne tiene 12 e le più vecchie escono» (`D-600` (1), riga a sé) e «Le altre righe restano come oggi» (`D-600` (1), il momento). Le teniamo entrambe:
  - le righe del TurnLog si calcolano e si tagliano **come oggi**, e nessuna riga d'attivazione le spinge fuori;
  - le righe d'attivazione occupano lo spazio che resta fino a `MaxFeedLines`, e quando non basta escono **le attivazioni più vecchie**;
  - l'ordine a schermo segue l'esempio dell'opzione scelta, «Kallari — Lama d'ombra» e poi «Colpita — 13»: una riga d'attivazione si inserisce **subito prima della riga d'esito della stessa unità sorgente** (`PrimaryStableUnitId`), se c'è; più attivazioni della stessa unità vanno lì nell'ordine in cui sono nate. Un'attivazione la cui unità non ha una riga d'esito va in fondo. `Sequence` numera la lista unita.
  Effetto voluto: i segni degli ostacoli di `ARTHUD`, che leggono lo stesso feed, non cambiano. Con il formato di default (due unità per squadra, una riga d'esito per unità) lo spazio per le attivazioni c'è; in un turno con dodici righe d'esito non ne resterebbe, ed è il limite dichiarato di questa lettura.
- **R7 — Vita.** Le righe d'attivazione appartengono al turno. Si azzerano **insieme al TurnLog**, al commit del turno (`LockInAndResolve`), così un turno senza playback (`bEnablePlayback` falso o `ResolvedTimeline` vuota: `ConcludeResolution` passa a `ConcludeTurn` senza `BeginPlayback`) non eredita le righe del turno prima. Non si azzerano alla ripresa dopo una finestra di reazione (`BeginPlayback(true)`), non in `ConcludeResolution` e non alla fine del playback: restano visibili nel Planning successivo insieme alle righe d'esito di quel turno, attraverso lo stesso `PickEventLogLines`.
- **R7b — A chi appartengono** *(scelta dell'assistente)*. Ogni riga porta la squadra per cui il playback l'ha mostrata (quella di `TeamIdOf` del giocatore locale in `BeginPlayback`). Il feed accoda una riga solo se quella squadra è fra gli osservatori della chiamata. Non è un secondo giudizio di conoscenza: è la chiave del pubblico, come la memoria del Registro, che «vale solo per gli occhi con cui è stata fatta» e si butta quando la chiave degli osservatori cambia ([`D-548`](../../decisions/RT_PDR_00_Decision_Log.md) (4)). Limite dichiarato: in una sessione non presidiata il Registro mostra gli esiti di tutte le squadre, ma le attivazioni solo per la squadra che il playback segue, come il fumetto.
- **R8 — Salto del playback** *(scelta dell'assistente, ribaltabile)*. Alla fine **vera** del playback, anche quando si salta, ogni attivazione che il playback avrebbe mostrato a chi guarda e che non ha ancora la sua riga la riceve, nell'ordine del playback e senza doppioni. Senza questa regola, chi salta in vista strategica non saprebbe quali skill sono partite. Il ramo sospeso di `FinishPlayback` (finestra di reazione) **non** è una fine vera.
  - ✅ **Corretto** *(trovato dalla review finale; prima l'aveva segnalato, leggendo il codice, la review di `c223bbeba`)*: un salto durante Prep o Dash di un playback parziale, con la finestra di reazione nel Blast, dava le righe nell'ordine Blast → Prep/Dash. Ora alla fine vera, dopo il recupero, `FinishPlayback` ordina la lista per fase del playback, in modo stabile (`71d91d692`); lo pinna `RefactorTactics.Playback.ActivationLine.SkipBeforeABraceWindowKeepsPlaybackOrder`, rosso senza l'ordinamento.
- **R9 — Solo presentazione.** Nessun campo in TurnLog, snapshot, replay, `StateHash` o replicazione. La riga di diagnostica `Attiva: …` in `AddLogEvent` resta com'è.
- **R10 — Le due viste.** Nessun ramo sulla vista: la riga c'è sia in vista tattica sia in vista strategica.

## 4. Architettura

- In `ARTTurnManager`: una lista di presentazione delle righe d'attivazione del turno, con accesso in sola lettura per la HUD; l'aggiunta in `ShowActivation` (R1, R5, R7b); l'azzeramento accanto a `TurnLog.Reset()` in `LockInAndResolve` (R7); il recupero di R8 nel ramo di fine vera di `FinishPlayback`. Per R8 servono le stesse sorgenti che il playback filtra già per chi guarda (le attivazioni Prep e Dash ammesse e la sequenza del Blast), e una chiave d'identità dell'attivazione che eviti i doppioni.
- In `URTHudViewModel::BuildPlayerEventFeed(const ARTTurnManager*, …)`: le righe del TurnLog come oggi, più le righe d'attivazione accodate prima del taglio a `MaxFeedLines` (R6). Gli overload su `TArray<FRTTurnLogEntry>` non cambiano.
- `ARTHUD` chiama lo stesso feed per i segni degli ostacoli: una riga d'attivazione non ha ostacolo (`bHasBlocker = false`) e non ne disegna.
- Nessun asset cambia: la riga è un `FRTPlayerEventLineView` con il `Text` già composto, e il widget lo scrive come le altre.

## 5. Test (nomi indicativi, nelle famiglie esistenti)

- `Playback.ActivationLine.AppearsWhenShownNotBefore`: all'inizio del playback nessuna riga d'attivazione; dopo la `ShowActivation` dell'attivazione, una.
- `Playback.ActivationLine.EveryActionKindHasALine`: skill del kit, attacco base, azione generica e un gadget (`Gadget.PortableCover`) producono ciascuno una riga; il contrasto con il fumetto, che grida solo per la skill del kit, sta nello stesso test.
- `Playback.ActivationLine.HiddenSourceHasNoLine`: un'attivazione nemica non osservata non produce riga (e nemmeno il fumetto).
- `Playback.ActivationLine.OutsideDominance`: un'unità che colpisce ha sia la riga «Colpita — N» sia la riga d'attivazione, e la riga d'attivazione sta subito sopra; nessuna delle due sostituisce l'altra.
- `Playback.ActivationLine.SurvivesTheReactionWindow`: le righe nate prima di una finestra di reazione ci sono ancora alla ripresa; un turno nuovo le azzera.
- `Playback.ActivationLine.SkipRevealsTheRest`: saltare il playback a metà lascia una riga per ogni attivazione visibile del turno, senza doppioni (R8).
- `ScreenHud.ActivationLine.TextNamesUnitAndAction` e il ripiego senza nome leggibile.
- `ScreenHud.ActivationLine.BudgetDropsOldestActivationsFirst`: con più di `MaxFeedLines` righe in totale escono le attivazioni più vecchie, e le righe d'esito restano quelle di oggi (R6).
- `Playback.ActivationLine.TurnWithoutPlaybackStartsEmpty`: un turno senza playback non mostra le righe d'attivazione del turno prima (R7).
- `ScreenHud.ActivationLine.BelongsToTheTeamItWasShownFor`: un feed chiesto per osservatori che non comprendono quella squadra non contiene la riga (R7b).
- `ScreenHud.ActivationLine.SameInBothViews`: il feed non cambia con `IsStrategicView()`.
- La suite intera resta verde: è un percorso condiviso del playback (`ShowActivation`, `BeginPlayback`, `FinishPlayback`).
- ➕ **I nomi reali**: `RefactorTactics.ActivationLine.*` per i test puri della riga e della fusione (quelli che qui portano `ScreenHud.ActivationLine.`) e `RefactorTactics.Playback.ActivationLine.*` per quelli del playback, compreso `SameInBothViews`. Il caso di R8 corretto dalla review finale è `Playback.ActivationLine.SkipBeforeABraceWindowKeepsPlaybackOrder`.

## 6. Gate

Compile; Automation (test nuovi e suite intera); Determinism/Replay **N/A** (solo presentazione, nessun golden cambia); Privacy con il test della sorgente nascosta; PIE: una voce nuova in `docs/technical/test-manuali-pie.md` (vista strategica: il Registro nomina la skill partita; salto del playback); Packaged NOT RUN (lo misura `G17`).

## 7. Fuori dallo scope

- Il Registro intero in sincronia con il playback: l'autore ha scelto di no in `D-600` (1).
- Il fumetto dei blocchi e le righe dei blocchi: #4034. La cura interrotta: #4042.
- Le reazioni: oggi non sono attivazioni del playback e non hanno riga.
