# Brief — Delayed Actions e boundary di fase

> **Fonte**: `docs/src/RefactorTactics_DelayedActions_PhaseWindows_Claude.md` (1546 righe) ·
> **Data**: 2026-08-07 · **Stato**: brief di scope, nessuna implementazione
> **Rapporto con le decisioni vigenti**: [ADR-0004](adr-0004-finestre-di-reazione.md) (accettato) copre le
> **finestre di reazione**; questo brief isola ciò che il documento aggiunge e che **nessun documento del repo
> copriva**: le **Delayed Actions** e i **boundary di fase** come punti di risoluzione nominati.

## 1. Cosa aggiunge davvero

Il documento sorgente tratta quattro temi. Tre sono già decisi e **non si riaprono**:

| Tema | Dove è già deciso | Verdetto |
|---|---|---|
| Fast Reaction, finestra 3 s, timeout → HOLD | ADR-0004 §8 (`FastReactionDuration = 3.0 s`, `DefaultTimeoutBehavior = Hold`) | ✅ **coincide**: il documento chiama lo stesso valore `FastDecisionDuration`. Un solo parametro, due nomi: prevale quello dell'ADR |
| Overwatch (HOLD/FIRE, charges, trigger simultanei) | [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md) §6, CP 14.3–14.5 | ✅ già pianificato |
| Confermato / Previsto / Incerto, privacy, fog of war | ADR-0004 §6-§7 · [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md), epic E13 | ✅ già pianificato |
| **Delayed Actions e boundary nominati** | — | 🆕 **nuovo**: nessun documento del repo lo copre |

**Delayed Action** = azione dichiarata interamente in Planning che **risolve a un boundary di fase successivo**,
scommettendo su uno stato futuro. Non è una reazione: non riceve informazione nuova e non apre alcuna finestra
interattiva (documento §5). È l'unico modo, oggi assente, di esprimere «sparo dove *penso* che arriverai».

## 2. Boundary — e come si mappano sul codice reale

Il documento (§3.2) chiede quattro boundary nominati: `EndPrep · EndDash · EndBlast · EndMove`. Sono punti
**logici**, non tempi di animazione.

Corrispondenza con il codice attuale (verificata sul branch, non ipotizzata):

- `ERTMatchPhase` (`Turn/RTTurnRules.h:9`) dichiara già `Planning · Prep · Dash · Blast · Move · Cleanup ·
  MatchEnded`: i quattro boundary sono le **uscite** delle prime quattro fasi attive, non un enum nuovo.
- Il resolver applica già le fasi in sequenza in `ARTTurnManager` con un `do/while` sulle macro-fasi: un
  boundary è il punto fra un `Resolve*` e il successivo.
- `Turn/RTActionQueue.h` + `RTActionQueueLibrary` esistono e ordinano le azioni per fase e priorità: la coda
  delle Delayed Actions **estende** questa, non ne affianca una seconda (vincolo §23 del documento).

## 3. Dati e policy da adottare così come sono

| Elemento | Valore dal documento | Nota di adozione |
|---|---|---|
| Campi obbligatori in Planning (§3.3) | ActionId, unità, **boundary**, targeting policy, bersaglio, costi, cooldown, facing, AoE, condizioni, **fallback**, **friendly fire policy**, priorità | Tutti già presenti in `FRTActionDef` tranne **boundary** e **targeting policy** |
| Targeting predittivo (§3.4) | `LockCell · LockLine · LockArea · LockDirection`; `TrackUnit` solo come trade-off dichiarato | Nuovo: oggi il targeting è implicito nella `Shape` |
| Fallback se la previsione sbaglia (§3.5) | `Fizzle · AttackCell · AttackTarget · RetargetStable · PartialEffect`; baseline consigliata **`LockCell + Fizzle/AttackCell`** | `ERTActionFallback` (`Ability/RTActionDef.h:48`) ha già `AttackCell`, `AttackTarget`, `Cancel` (≡ Fizzle): mancano solo `RetargetStable` e `PartialEffect` |
| Bilanciamento (§20) | danno ridotto · cooldown maggiore · range minore · area più stretta · telegraph · 1 delayed per turno · fizzle · friendly fire | «Non imporre tutti questi costi insieme» |

> **Osservazione utile al catalogo**: la *friendly fire policy* che il documento elenca fra i campi obbligatori
> (§3.3) esisteva già come dato (`FRTActionDef::bFriendlyFire`) ma **non veniva letta dal resolver in partita**.
> Difetto trovato e corretto durante il CP 8.2, nello stesso branch di questo brief.

## 4. Gli otto test minimi (§25)

Vincolanti se l'epic viene aperta; il documento li nomina per comportamento, i nomi in stile repo sono qui:

| # | Comportamento | Nome proposto |
|---|---|---|
| 1 | Il nemico termina il Dash nella cella prevista → colpo a segno | `Delayed.EndDashHit` |
| 2 | Termina altrove → fizzle o `AttackCell` secondo policy | `Delayed.EndDashMiss` |
| 3 | Delayed@EndMove risolve **solo** dopo il completamento del Move | `Delayed.ResolvesAfterMoveCompletes` |
| 4 | Il bersaglio attraversa la cella durante il Dash ma non c'è a EndMove → **non** colpisce | `Delayed.CrossingIsNotPresence` |
| 5 | La copertura cambia prima del boundary → si usa lo stato logico **al boundary** | `Delayed.UsesStateAtBoundary` |
| 6 | Owner in KO prima del boundary → cancel/fizzle secondo definizione | `Delayed.OwnerDefeatedPolicy` |
| 7 | Due delayed allo stesso boundary, ordine di inserimento permutato → **TurnLog e hash identici** | `Delayed.PermutationInvariant` |
| 8 | Nessuna fuga di pianificazione avversaria | `Delayed.NoEnemyPlanningLeak` |

Il test 7 è quello che conta di più: è la stessa proprietà che regge già `Actions.PermutationInvariant` e
`HexSim.ReplayDivergenceZero`.

## 5. Checkpoint proposti — **non aperti**

Se il tema entra in roadmap, la forma minima è di 4 checkpoint dentro una **epic propria** — numero da
assegnare, **E15 (showcase) ed E16 (orientamento) sono già occupate** — da collocare *dopo* E14:
`.1` modello dei boundary e coda · `.2` targeting predittivo e fallback · `.3` risoluzione al boundary con i
test 1–7 · `.4` ghost timeline e privacy (test 8). **La decisione di aprirla è tua**: E14 non è
iniziata, e il rischio di scope della v0.1 (§8 di [`roadmap-v0.1.md`](roadmap-v0.1.md)) è già segnalato come
**alto**.

## 6. Rischi e domande aperte

1. **Il documento dà per esistente più di quanto esista.** §23 elenca `RTReactionLibrary` fra i sistemi
   presenti: nel repo **non c'è** un file con quel nome (le reazioni vivono in `ARTTurnManager` e nel catalogo
   azioni). Il documento stesso prescrive di verificare i nomi reali prima di implementare: qui è stato fatto.
2. **`EndPrep` non ha oggi un uso dichiarato**: nessuna azione del catalogo v0.1 ne avrebbe bisogno. Aprire
   quattro boundary quando ne servono due è superficie non richiesta.
3. **Interazione con gli stati temporanei (CP 8.2)**: una Delayed Action che risolve a `EndMove` incontra uno
   stato che nel frattempo può essere stato **revocato** (`Wet`/`Obscured` legati alla cella) o **speso**
   (`Marked`). La regola «si usa lo stato logico al boundary» (test 5) va estesa esplicitamente agli stati, non
   solo a copertura e LOS.
4. **Un solo parametro con due nomi**: `FastDecisionDuration` (documento) e `FastReactionDuration` (ADR-0004)
   sono lo stesso 3.0 s. Non introdurre il secondo nome nel codice.

## 7. Rapporto con gli altri documenti

| Documento | Relazione |
|---|---|
| [ADR-0004](adr-0004-finestre-di-reazione.md) | **Prevale** su finestre, timeout, privacy e trigger |
| [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md) | Copre Overwatch (E14); questo brief non lo ripete |
| [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md) | Copre detection e livelli di certezza (E13) |
| [`adr-0003-modello-azioni-v01.md`](adr-0003-modello-azioni-v01.md) | Le macro-fasi restano `Prep → Dash → Blast → Move`: i boundary sono le loro uscite, non un nuovo ordine |
