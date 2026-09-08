# Spec panel — #2679 «La finestra di reazione non esiste come oggetto», 2026-09-08

> `REFERTO` · **Oggetto**: [#2679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2679)
> **Modalità**: `critique` · **Focus**: requirements · architecture · testing
> **Misure**: su `main` = **`5327d514`**, rimisurate su `origin/main` = **`a83ea7d9`** dopo il
> fast-forward che ha portato `D-354`.
> **Esito**: 1 criterio **già soddisfatto** invertito, 1 costo strutturale non dichiarato portato in
> superficie, 1 decisione ([`D-355`](../../decisions/RT_PDR_00_Decision_Log.md)), 1 drift
> contratto/implementazione, 2 correzioni minori.
> **Contesto**: la issue nasce da `/feature-behavior` su CP 14.6 e scorpora
> [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166).

**Le citazioni della issue sono accurate.** `RTTurnManager.h:627`, `:1400`, `:1473`, la fixture
`Scenarios/Spec/Overwatch/ThreeArmedWatchersOnOneEntry.json` e i cinque test nominati esistono tutti, e il
rischio replay su `ArmRecordedReactionDecisions` è posto correttamente. Ciò che segue riguarda **due premesse
portanti** che non reggono alla misura, e la più grave è il primo criterio della sua stessa DoD.

---

## 1. C1 — il primo criterio della DoD è già vero, e non è falsificabile

La DoD chiede *«una finestra aperta **sospende ogni unità** finché non si chiude — falsificato da: un'unità
avanza mentre la finestra è aperta»*. Non esiste modo di falsificarlo: `Turn/RTTurnManager.cpp:7318` lo
dichiara già, accanto al codice che lo produce.

> *«IL DECISION BOUNDARY. La «sospensione globale» di ADR-0004 §5 è il fatto che questa chiamata stia fra due
> micro-step e debba ritornare prima del successivo: nessuna unità avanza mentre una finestra è aperta, e non
> perché qualcuno le fermi — **perché il ciclo non gira**.»*

**KARL WIEGERS**: *«La sospensione globale è gratis oggi, ed è gratis precisamente perché
`AskReactionDecision` è sincrona — la proprietà che questa issue esiste per rimuovere. Il criterio non misura
lavoro da fare: misura una proprietà che l'architettura regala e che questa issue mette a rischio.»*

📝 **Riformulato per inversione**: da *«una finestra aperta sospende ogni unità»* a *«la sospensione globale
**sopravvive** all'attesa asincrona»* — falsificato da: `ResolveNextHexMicroStep` avanza mentre una finestra
attende una risposta umana.

---

## 2. C2 — il costo non è la finestra: è rendere riprendibile un ciclo che vive sullo stack

La issue dice che *«manca una cosa sola: che la finestra sia un oggetto che dura»*. Manca ben altro.

| Fatto | Sede |
|---|---|
| Il ciclo dei micro-step vive dentro `ResolveMovement` | `RTTurnManager.cpp:7305` |
| `State`, `Paths`, `EnteredBefore`, `bStoppedByTopology`, `bDeniedByOccupant`, `DeniedDestination`, `PlannedMoves` sono **locali di stack** | `RTTurnManager.cpp:7137-7305` |
| `ON_SCOPE_EXIT` ripristina `CurrentMicroStepIndex` all'uscita di funzione | `RTTurnManager.cpp:7304` |
| I siti di `AskReactionDecision` sono **due**: Overwatch e `Brace` di [D-047] | `RTTurnManager.cpp:7131` · `RTTurnManager_Blast.cpp:2264` |

Per attendere `FastReactionDuration` un umano bisogna **o** bloccare il game thread — inaccettabile: senza
tick non gira il countdown, non arriva l'input, la finestra non si disegna, e il rimedio si impedisce da sé —
**o** portare quello stack in stato persistente e renderlo riavviabile a metà.

**MARTIN FOWLER**: *«E qui la issue si contraddice. Avverte di non diventare la tredicesima responsabilità di
`ARTTurnManager`, poi chiede la sola modifica che vi aggiunge — necessariamente — una `FRTHexMoveState`
sospesa, un indice di micro-step vivo fra due frame e sette array paralleli che oggi muoiono a fine funzione.
Estrarre la finestra non evita niente: la finestra è la parte piccola.»*

⚠️ **Il numero di [#1818](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1818) è già stale**:
la issue cita 10.412 righe, la misura su `5327d514` dà **10.817** (`RTTurnManager.cpp` 8.427 +
`RTTurnManager.h` 2.390). L'argomento si rafforza; la voce di DoD che lo esprime, no — vedi §6.

---

## 3. C3 — `BLOCKED — DECISION REQUIRED`, deciso `(A)` dall'autore → [`D-355`](../../decisions/RT_PDR_00_Decision_Log.md)

**ALISTAIR COCKBURN**: *«Chiedo sempre chi è l'attore primario e cosa sta guardando quando agisce. Qui la
risposta era: non sta guardando niente.»*

`LockInAndResolve` (`RTTurnManager.cpp:1854`) risolve **tutte** le fasi in un `do…while` sincrono
(`:1920-1943`), ordina il TurnLog, valuta la fine partita — e **solo a `:2260`** chiama `BeginPlayback()`.
Simulazione e playback sono sequenziali e disgiunti, nello stesso frame. ∴ nell'istante in cui la finestra si
aprirebbe, il giocatore **non ha visto il nemico entrare nella zona**.

E l'alternativa non era neutra: [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166)
riga 59 dice *«la slow-motion **durante la finestra** è sola presentazione»*, e `D-350` la realizza scrivendo
`ViewerPlaybackSpeed < 1`, cioè **l'orologio del playback** (`TickPlayback`, `:8015`). Una finestra che
precede il playback non ha alcun orologio da rallentare.

Tre uscite poste all'autore, deciso **(A)**:

| | Esito |
|---|---|
| **(A)** durante il playback, interlacciando | ✅ **scelta** — coerente con `#166:59`, `D-350`, ADR-0004 §5 |
| **(B)** prima del playback, decisione al buio | ⛔ richiede di riaprire `D-350` e `#166` riga 59 |
| **(C)** nessuna finestra runtime in v0.1, pre-dichiarazione via `CollapsedByCondition` | ⛔ svuota il widget `FIRE`/`HOLD`, cuore di CP 14.6 (`P0`) |

🔑 **La barriera esiste già**: `StepMicroStep` (`:7967`) calcola il confine con
`URTPlaybackLibrary::NextMicroStepBoundary` (`RTPlaybackLibrary.cpp:176`) e ferma `TickPlayback` esattamente
lì, correggendo lo scarto di frame (`:8045-8052`). Ciò che manca non è **dove** fermarsi: è che la
simulazione sappia ripartire dall'altra parte.

---

## 4. C4 — un drift fra un commento normativo e il ramo che lo smentisce

`RTTurnManager.cpp:6622` promette una strada:

> *«una UI umana (CP 14.6) risponderà invece con una **stringa vuota** fino a che il giocatore non ha scelto,
> e sarà **l'orchestratore a richiamare**»*

Nove righe sotto (`:6631`), `Response.IsEmpty()` applica `DecisionOnTimeout`. Vuota significa **scaduta**, non
«non ancora»: una UI che rispondesse `""` mentre il giocatore pensa otterrebbe `HoldTimeout` immediato,
decisione applicata, finestra chiusa. Nessun orchestratore richiamerà — non c'è più niente da richiamare.

**GOJKO ADZIC**: *«La issue tratta la forma del decisore umano come aperta. Non lo è: qualcuno l'ha già
scritta, e il codice sotto la smentisce. Un contratto che il proprio ramo contraddice non è un'alternativa
disponibile, è un difetto da chiudere.»*

📝 `D-355` sceglie la sospensione; il commento va corretto o rimosso. Insieme a lui, altri due che la
decisione rende falsi: `RTTurnManager.h:552` (*«la risoluzione è già avvenuta per intero quando il playback
comincia: non esiste uno stato logico a metà barriera da proteggere»*) e `RTTurnManager.h:627` (*«nessun
chiamante di produzione»*).

---

## 5. C5 — due consumatori a valle che nessuna voce di DoD nomina

**MICHAEL NYGARD**: *«Cerco sempre chi si accorge del cambiamento senza essere stato avvertito.»*

`ERTReactionDecisionOutcome::NoDecider` assorbe oggi *«un'unità umana senza UI: la finestra esiste e nessuno
può rispondere»* (`RTTurnManager.cpp:6615`), e `URTPacingLibrary::CountOpenedWindows` lo **esclude**
deliberatamente dal conteggio (`RTPacingLibrary.cpp:165-175`), con una previsione esplicita:

> *«➕ **Rientrerà da solo** quando la UI di DIR-A atterrerà: quelle finestre diventeranno `Chosen` o
> `Timeout`, che stanno nel ramo sopra. Nessuno dovrà ricordarsi di aggiornare questa funzione.»*

La previsione è corretta e la **conseguenza** non è verificata da nessuno: quando questa issue atterra,
`ReactionDecisionSecondsUpperBound` comincia a contare finestre che prima valevano zero, e il tetto di
`D-348` cambia valore **in silenzio**. Nessun test lo guarda.

---

## 6. Minori

**LISA CRISPIN — un gate che non può fallire.** La DoD elenca `Reactions.NoResolverWait` fra i test da non far
regredire. `Tests/RTReactionTests.cpp:275` verifica `URTReactionLibrary::EvaluateReactionTrigger` — funzione
**pura**, sulle **Slot Reaction** (Counter/Deflect, invariante #3): non attraversa `AskReactionDecision`, non
attraversa il decision boundary, e resterà verde qualunque cosa accada alla finestra. Non è sbagliato: è
**vuoto**. I gate che contano sono `Overwatch.DecisionIsReplayable`, `Overwatch.TimeoutIsHold` e
`Replay.Verifier.ResimulationIsDeterministic`.

**MARTIN FOWLER — una casella che si spunta da sé.** *«La scelta fra dentro ed estratta è dichiarata, con la
ragione»* è soddisfatta da qualunque scelta, inclusa quella di aggiungere 400 righe a un file che ne ha
10.817. Un criterio che non può respingere niente è un promemoria, non un criterio: o si prescrive, o si dà
un bound falsificabile.

**GOJKO ADZIC — il contratto scrive l'apertura, non la chiusura.** Manca il `Given/When/Then` della risposta
che arriva, e quello delle due finestre nello stesso micro-step — che è esattamente ciò che
`ThreeArmedWatchersOnOneEntry.json` esiste per esercitare. La fixture è nominata bene; lo scenario che
dovrebbe attraversarla no.

---

## 7. Punteggi e cosa cambia nella issue

| | Prima |
|---|---|
| Chiarezza | 8,5/10 |
| Completezza | **5/10** — riprendibilità, ordine sim→playback, drift e consumatori a valle non nominati |
| Falsificabilità | **4/10** — 2 criteri su 6 non falsificabili |
| Consistenza col contratto | **4/10** — conflitto aperto con `D-350` |

Il corpo di #2679 è riscritto con: il criterio C1 invertito, il costo di C2 dichiarato come deliverable
primario, `D-355` registrata come decisione che governa, il drift di C4 fra le voci di chiusura, la
conseguenza di C5 come gate, il gate vuoto sostituito e le due caselle non falsificabili rese tali.

### #166 informata, e due suoi numeri corretti

`D-355` tocca [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) senza portarvi lavoro:
`D-350` e la riga *«la slow-motion **durante la finestra** è sola presentazione»* **presupponevano già** questa
risposta, e il presupposto non era dichiarato da nessuna parte. Registrata anche la **sequenza** — tre caselle
di CP 14.6 (tasto `V` inerte, `Overwatch.SlowMotionDoesNotChangeOutcome`, `TimeoutReason = Abandoned`) non
sono spuntabili prima che #2679 atterri, e chi le trovasse aperte non deve concluderne che siano state saltate.

Nel farlo sono emersi due numeri sbagliati nel suo corpo, corretti in linea:

| | |
|---|---|
| *«derivato dagli **undici** `continue` reali»* | 🔴 → **tredici**, di cui dieci skip di un watcher. La correzione era in `D-349` **dal 2026-09-08 e non era mai arrivata su #166** — il Decision Log la registrava, la issue no |
| #1818 a *«10.412 righe»* | citazione corretta, file già oltre: **10.817** su `5327d514` |

⚠️ **Il primo è lo stesso difetto che `D-349` esiste per prevenire**, ricomparso sul documento che quella
decisione ha prodotto: una correzione registrata in un posto e non propagata all'altro. Il vocabolario non
aveva buchi — a sbagliare era solo il numero che lo giustificava.
