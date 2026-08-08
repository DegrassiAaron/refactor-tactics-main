# Brief — Delayed Actions e boundary di fase

> ✅ **[D-016](../decisions/RT_PDR_00_Decision_Log.md) (2026-08-08): un thin slice entra nella v0.1.**
> Il brief lasciava l'epic fuori roadmap. La decisione è di includere **una sola** Predictive Action reale,
> per rendere percepibile il pilastro della predizione senza aprire il framework di trap.
>
> **Bersaglio preferito**: `Vektor.InterceptShot`.
>
> ```text
> Planning: previsione dichiarata per intero
>   → trigger/boundary deterministico
>   → previsione corretta  → risoluzione automatica
>   → previsione errata    → whiff/fallback dichiarato
>   → NESSUN input umano durante la Resolution
> ```
>
> **Non va trasformata in Fast Reaction.** Resta fuori dalla v0.1: framework completo di trap/mine/gambit
> persistenti, editor visuale di trigger, catene di predictive action, interrupt annidati.
>
> 🔎 **Nota di stato verificata il 2026-08-08.** Oggi `Vektor.InterceptShot` è a catalogo con
> `ERTActionSlot::None` e **nessun trigger**, e il rinvio a **E14** è dichiarato *nei dati*
> (`RTHeroCatalogLibrary.cpp`) perché il suo trigger è d'ingresso su movimento. Trattarla come Predictive
> Action **la sgancia da E14**: non le serve una finestra interattiva, le serve un boundary deterministico.
> È una semplificazione — ma resta una **migrazione di classificazione** da tracciare, non da fare in una PR
> documentale.

> **Fonte**: `docs/src/RefactorTactics_DelayedActions_PhaseWindows_Claude.md` (1546 righe) ·
> **Data**: 2026-08-07 · **Stato**: brief di scope, nessuna implementazione
> **Rapporto con le decisioni vigenti**: [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) (accettato) copre le
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
iniziata, e il rischio di scope della v0.1 (§8 di [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md)) è già segnalato come
**alto**.

## 6. Rischi e domande aperte

1. ~~**Il documento dà per esistente più di quanto esista.** §23 elenca `RTReactionLibrary` fra i sistemi
   presenti: nel repo **non c'è** un file con quel nome.~~
   > ⚠️ **Rettifica del 2026-08-07**: questa osservazione era **sbagliata già quando è stata scritta**.
   > `Turn/RTReactionLibrary.{h,cpp}` **esiste**, è l'epic **E5** (27 test) ed è dichiarato ✅ in
   > [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) §2 nella stessa giornata. Il documento sorgente aveva
   > ragione. La lezione resta ma si rovescia: **verificare i nomi reali vale in entrambe le direzioni** —
   > negare l'esistenza di un sistema porta a ricostruirlo, che è il doppio del danno.
2. **`EndPrep` non ha oggi un uso dichiarato**: nessuna azione del catalogo v0.1 ne avrebbe bisogno. Aprire
   quattro boundary quando ne servono due è superficie non richiesta.
3. **Interazione con gli stati temporanei (CP 8.2)**: una Delayed Action che risolve a `EndMove` incontra uno
   stato che nel frattempo può essere stato **revocato** (`Wet`/`Obscured` legati alla cella) o **speso**
   (`Marked`). La regola «si usa lo stato logico al boundary» (test 5) va estesa esplicitamente agli stati, non
   solo a copertura e LOS.
4. **Un solo parametro con due nomi**: `FastDecisionDuration` (documento) e `FastReactionDuration` (ADR-0004)
   sono lo stesso 3.0 s. Non introdurre il secondo nome nel codice.

## 6-bis. Trigger su transizione — [D-013](../decisions/RT_PDR_00_Decision_Log.md), 2026-08-07

Il sorgente `RefactorTactics_Predictive_Actions_Traps_Claude.md` §8.2 chiede un `Tripwire` che scatti
sull'**attraversamento di un arco** `CrossEdge(H6, H7)`, distinto dall'ingresso in H7 da un altro lato. La
domanda registrata era «gli archi del grafo devono portare trigger?», e sembrava un gate da chiudere prima
di **E9**.

**La domanda era mal posta, e la verifica sul codice l'ha riformulata.** Gli archi degli adiacenti **non
esistono**: `URTHexPathLibrary::GraphNeighbors` li calcola a ogni interrogazione da
`URTHexLibrary::Neighbors(Cell)`, e `FRTHexEdge{From, To, Cost, Kind}` è memorizzato **solo** per le
transizioni fra layer. Fra due celle di layer 0 non c'è alcun oggetto a cui attaccare un trigger.

Le due vie, e perché una vince nettamente:

| Via | Costo | Esito |
|---|---|---|
| **La definizione predittiva possiede la coppia** `(From → To)` + i tipi di transizione validi; il resolver la confronta col micro-step | nessuna modifica alla mappa | ✅ **adottata** |
| Gli adiacenti diventano `FRTHexEdge` memorizzati | ~1400 archi su una mappa r=12 (oggi 0); formato e `ComputeHash` di `URTHexMapAsset` da versionare; `HexMap.HashIncludesTransitions` da rivedere; migrazione di `DA_HexMap_Sandbox` | ❌ scartata |

**Conseguenze operative:**

- `FRTHexEdge` resta riservato ai **salti di layer**. Non diventa il posto dove vive lo stato di gioco.
- **OD-4 esce dai bloccanti**: nessun vincolo di precedenza rispetto a E9. Porte e ponti continuano a
  modificare le transizioni fra layer via `Revision`, come già fanno.
- Il trigger deve dichiarare i **tipi di transizione** che considera validi (sorgente §16): `Move` e `Dash`
  attraversano l'arco, mentre teletrasporto, displacement e spinta **entrano nella cella senza percorrerlo**.
  È la distinzione che rende un tripwire aggirabile — cioè giocabile.
- Un trigger su transizione è quindi **dato dell'azione**, non dato della mappa: la stessa disciplina di
  §3, dove boundary e targeting policy sono campi di `FRTActionDef`.

## 7. Rapporto con gli altri documenti

| Documento | Relazione |
|---|---|
| [ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) | **Prevale** su finestre, timeout, privacy e trigger |
| [`brief-overwatch-reazioni.md`](brief-overwatch-reazioni.md) | Copre Overwatch (E14); questo brief non lo ripete |
| [`brief-conoscenza-parziale.md`](brief-conoscenza-parziale.md) | Copre detection e livelli di certezza (E13) |
| [`adr-0003-modello-azioni-v01.md`](../decisions/adr-0003-modello-azioni-v01.md) | Le macro-fasi restano `Prep → Dash → Blast → Move`: i boundary sono le loro uscite, non un nuovo ordine |
