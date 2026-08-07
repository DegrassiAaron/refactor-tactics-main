# ADR-0004 — Finestre di reazione: composizione dell'invariante #3 e modello unificato

> **Stato**: Accettato — da implementare · **Data**: 2026-08-07 · **Decisore**: utente (dev singolo)
> **Contesto sorgente**: `docs/src/RefactorTactics_Overwatch_FastReaction_Claude.md` (19 sezioni)
> **Brief**: [`brief-overwatch-reazioni.md`](../gameplay/brief-overwatch-reazioni.md) (decisioni D16–D22)
> **Supera**: [ADR-0003](adr-0003-modello-azioni-v01.md) §4 riga «Stack di reazioni LIFO interattivo → scartato»
> **limitatamente alla finestra singola non annidata**; lo stack LIFO resta scartato.

## Contesto

Il canone vieta le attese nel resolver (invariante **#3**, «raccogli poi applica»: snapshot a inizio fase,
nessun `Delay`/timeline/montage, l'ordine dell'array non cambia l'esito). Su questa base:

- `spec-sequenza-turno.md` §2 classifica le **finestre di reazione live** come conflitto **C1**, «north-star,
  gated», e propone **due** riconciliazioni: (a) politiche *pre-committed*, (b) «ogni finestra apre un **nuovo
  round di sotto-risoluzione deterministico**»;
- ADR-0003 §4 scarta lo **stack di reazioni LIFO interattivo** per lo stesso motivo;
- l'epic **E5** è stata chiusa con la via (a): 24 test verdi, reazioni dichiarate in planning e valutate come
  funzioni pure sullo snapshot di fase.

Il documento sorgente sull'Overwatch, redatto dopo, porta a maturità la via **(b)** e aggiunge l'argomento di
design che la via (a) non può soddisfare: **bait, bluff e commitment** (§18). Con reazioni dichiarative il
mindgame collassa — se dichiaro «spara al primo che entra», il tank brucia sempre l'Overwatch e il portatore
di danno passa. Il valore della meccanica sta nel *non sapere se arriverà un bersaglio migliore*.

La decisione **D7** del brief sulla conoscenza parziale era passata in giornata da «nessuna finestra» a
«finestra come presentazione»; entrambe le formulazioni sono ora superate da questo ADR.

## Decisione

### 1. L'invariante #3 si **compone**, non si deroga

Il turno cessa di essere una singola risoluzione e diventa una **sequenza di sotto-risoluzioni**:

```
Turno = [ snapshot → risolvi → boundary ] · [ snapshot → risolvi → boundary ] · … · Cleanup
        └───────  «raccogli poi applica»  ──┘   ripetuto, non violato
```

Un **decision boundary** è un punto in cui la simulazione autorevole si arresta, raccoglie decisioni dai
giocatori e riparte con un nuovo snapshot. Dentro ciascun segmento valgono tutte le regole di oggi: nessun
`Delay`, nessuna timeline, nessun montage, e l'ordine dell'array non cambia l'esito.

**Riformulazione dell'invariante #3** (da recepire in `piano-canonico-mvp.md §5`):

> Resolver «raccogli poi applica»: snapshot a inizio **segmento di risoluzione**, nessun `Delay`/timeline/
> montage dentro il segmento, l'ordine dell'array non deve cambiare l'esito. Un segmento è delimitato
> dall'inizio di una macro-fase **oppure** da un decision boundary.

Il resolver non attende **mai** dentro un segmento: termina il segmento e restituisce il controllo.

### 2. Modello unificato: `opportunity → commit`

Tutte le reazioni — difensive esistenti e Overwatch — usano un solo modello:

```
Reaction armata
  └─ trigger valutato            ← funzione PURA sullo snapshot, come oggi
       └─ FRTReactionOpportunity { AllowedResponses[] }
            ├─ AllowedResponses ≤ 1 → commit immediato, NESSUN boundary   ← caso degenere: E5 di oggi
            └─ AllowedResponses ≥ 2 → decision boundary + finestra         ← Overwatch
```

`Counter`, `Deflect`, `Brace`, `Shield`, `Cleanse` hanno **una sola risposta legale**: scattano o non scattano.
Restano deterministiche e senza finestre. **I 24 test di E5 restano verdi senza cambiare comportamento atteso.**

`Reactions.NoResolverWait` conserva il suo significato, precisato: **il trigger resta puro** — è il *commit*
che può richiedere input, ed è un passo distinto e successivo.

### 3. Determinismo

- La decisione del giocatore entra nel **TurnLog come dato** (`OpportunityId → Response`), quindi il replay la
  riproduce senza reinterrogare nessuno.
- Il **timeout è una funzione pura dello stato**: `Timeout → HOLD`. Mai `FIRE`, perché consuma una risorsa
  irreversibile e un mancato input non deve spenderla.
- L'esito dipende **solo** da *quale* risposta arriva, **mai** da *quando* arriva dentro la finestra.
- La slow-motion durante la finestra è **presentazione**: non influenza esiti, seed, ordine, collisioni, path
  né timing logico.

### 4. Trigger simultanei

Se più unità soddisfano il trigger nello **stesso micro-step**, si genera **una sola** opportunity con più
bersagli (`FIRE A` / `FIRE B` / `HOLD`), **mai** prompt in sequenza: prompt sequenziali darebbero un vantaggio
all'ordine di iterazione, che l'invariante **#4** vieta.

Quando più reazioni distinte scattano nello stesso micro-step, l'ordine è totale e stabile:
`ReactionPriority → AbilityPriority → UnitInitiative → StableUnitId → ReactionInstanceId`.

### 5. La sospensione è **globale** *(risolve la domanda aperta §8.2 del brief)*

Durante una finestra la simulazione si ferma **per tutte le unità**, non solo per quella che decide.

**Derivata, non preferita**: se il resto della resolution proseguisse, l'avanzamento delle altre unità
dipenderebbe dal tempo di risposta umano — il *quando* tornerebbe a influenzare l'esito, contro §3 di questo
ADR e l'invariante #4. La sospensione globale è anche la più leggibile: chi guarda vede il mondo fermarsi su
un momento di tensione, non alcune unità muoversi e altre no.

### 6. Il trigger richiede il livello **`Rilevato`** *(risolve §8.4)*

La condizione del trigger è `TargetInsideArea ∧ HasLineOfSight ∧ TargetDetected ∧ ReactionStillArmed`
(§14 sorgente). Con i tre livelli di conoscenza di **E13**, `TargetDetected` significa **`Rilevato`**.

Conseguenze operative, tutte derivate senza regole nuove:

| Situazione | Livello | Trigger |
|---|---|---|
| Bersaglio nel cono, a vista | `Rilevato` | ✅ scatta |
| Bersaglio nel fumo entro 2 celle (cap `Max_Contact_Range`) | `Rilevato` | ✅ scatta |
| Bersaglio nel fumo oltre 2 celle | `Incerto` | ❌ non scatta |
| Solo rumore, nessun contatto visivo | `Incerto` | ❌ non scatta |
| Fuori vista, ultimo contatto noto | `UltimoContatto` | ❌ non scatta |

Sparare su un contatto **incerto** è una meccanica diversa (`Resonance Shot`, §17 sorgente): resta north-star.
L'Overwatch base non spara a una posizione dedotta.

**Dipendenza dichiarata**: E14 non parte prima di **E13**. Senza livelli di conoscenza, `TargetDetected` non
ha una definizione e l'Overwatch sparerebbe a unità che la squadra non percepisce.

### 7. Visibilità della finestra *(risolve §8.1)*

- **Decide** solo il proprietario della reaction.
- **Vede** l'intera squadra, in sola lettura, coerentemente con la privacy di squadra dell'invariante #6
  (gli intenti alleati sono già condivisi).
- L'avversario **non riceve nulla**: né l'esistenza della finestra, né la sua durata, né l'esito prima che
  sia applicato.

È l'unica delle quattro domande che non discende dagli invarianti: in tre secondi la coordinazione vocale non
è realistica, quindi la visione dell'alleato serve alla **leggibilità**, non alla decisione. Se al playtest
risultasse rumore inutile, si degrada a «vede solo il proprietario» senza toccare il modello.

### 8. Parametri iniziali *(risolve §8.3)*

| Parametro | Valore | Origine |
|---|---|---|
| `FastReactionDuration` | **3.0 s** — **baseline di sistema per ogni Fast Reaction**, non solo per l'Overwatch | §3 sorgente · confermato da [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md) §8 |
| `MaxPromptsPerReaction` | **3** | §5 sorgente; data-driven |
| `DefaultTimeoutBehavior` | **Hold** | §3 di questo ADR |
| `Charges` (Overwatch v0.1) | **1** | §5 sorgente |
| Cap aggregato di finestre per turno | **nessuno** | decisione **D20**: si misura al playtest |

`MaxPromptsPerReaction` limita le opportunity di **una** reaction; D20 riguarda il budget **aggregato**, che
resta volutamente non limitato. I due non sono in contraddizione.

### 9. Cosa **non** cambia

Restano scartati o north-star: **stack di reazioni LIFO interattivo** · **interrupt annidati** (una finestra
non può aprirne un'altra) · reveal progressivo · 5 categorie di velocità · timeline di esecuzione 45–60 s ·
probabilità di qualunque tipo. Le macro-fasi `Prep → Dash → Blast → Move → Cleanup` sono **invariate**, Move
dopo Blast (ADR-0003 §1).

## Alternative considerate

| Alternativa | Esito |
|---|---|
| Confermare D7: nessuna finestra, tutto dichiarativo | **Scartata dall'utente**: rinuncia definitiva a bait, bluff e commitment |
| Finestra come sola *presentazione*, condizioni dichiarate in planning | **Scartata**: il primo bersaglio che entra brucia sempre la reaction — il bait diventa banale |
| Filtro di bersaglio dichiarato (soglia di minaccia, «ignora il primo») | **Scartata**: neutralizza il bait ma sposta tutto il mindgame in planning, rendendolo statico |
| Finestre solo per Overwatch, difensive invariate | **Scartata dall'utente**: due modelli di reazione da mantenere |
| **Modello unificato con caso degenere** ✅ | **Scelta**: un solo modello, E5 conservata come `AllowedResponses ≤ 1` |

## Conseguenze

**Positive**: un solo modello di reazione, estendibile a Guard, Counter, Dodge, Intercept, Ambush, Trap,
Opportunity Attack senza toccare la pipeline · bait e bluff diventano gameplay reale · l'aggancio esiste già
(`Action.SuppressiveLine` in fase Prep e `Vektor.InterceptShot`, entrambi a catalogo e testati) · E5 resta
chiusa e verde.

**Negative / costi**:
- la **forma del turno** cambia: da una risoluzione a una sequenza di segmenti. Tocca `ARTTurnManager`, il
  playback e il TurnLog (che deve registrare i boundary e le risposte);
- **netcode a N round-trip per turno** invece di uno (rilevante da **M10**, non prima);
- **durata della resolution non limitata**: `MaxPromptsPerReaction 3` × 3 s = **9 s per una sola unità
  armata**, contro i 12 s di `Resolution_sec` del workbook. Rischio accettato con D20, da misurare;
- in turni **simultanei**, chi non ha reazioni armate attende senza agire;
- il TurnLog cresce di una dimensione (decisioni), e la sua serializzazione va **versionata**.

**Invarianti**: **#3 riformulato** (§1) — non indebolito: nessuna attesa entra in un segmento. #1, #2, #5, #7
invariati. **#4 preservato**: l'input è un dato del log, il timeout è una funzione pura, i trigger simultanei
non dipendono dall'ordine di iterazione. **#6 preservato e rafforzato**: la opportunity inviata al client
contiene **solo il presente** — mai trigger futuri, percorsi futuri, opportunity future o intenti avversari.

## Verifica

| Test | Cosa dimostra |
|---|---|
| `Reactions.SingleResponseCommitsWithoutWindow` | `AllowedResponses ≤ 1` non apre boundary: E5 invariata |
| suite E5 (24 test) **invariata** | l'unificazione non cambia il comportamento delle difensive |
| `Reactions.NoResolverWait` **invariato** | il trigger resta una funzione pura senza `UWorld` |
| `Overwatch.TimeoutIsHold` | il default allo scadere è puro e non consuma la charge |
| `Overwatch.DecisionIsReplayable` | stesso snapshot + stesse risposte registrate ⇒ stesso TurnLog e stesso checksum |
| `Overwatch.SimultaneousTargetsSingleOpportunity` | trigger nello stesso micro-step ⇒ una opportunity, non prompt in sequenza |
| `Overwatch.OrderIsDeterministic` | permutare l'input non cambia l'ordine delle opportunity |
| `Overwatch.RequiresDetection` | contatto `Incerto` (fumo oltre 2, o solo rumore) **non** arma il trigger |
| `Overwatch.OpportunityLeaksNoFuture` | la opportunity non contiene trigger, percorsi o posizioni future |
| `Overwatch.HoldKeepsArmed` | `HOLD` perde l'opportunità, non la reaction |
| `Overwatch.CancelledByStun` · `…ByForcedMovement` | l'overwatch armato non è garantito fino a fine turno |

Checkpoint in [`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md) epic **E14** (CP 14.1–14.5); questo ADR **è** il CP 14.1.

## Revisione

Rivedere alla chiusura di **CP 14.5**, che misura la durata reale della resolution con 1, 2 e 3 unità armate.

**Soglia di allarme**: resolution stabilmente sopra i **20 secondi**. In quel caso rientrano le due opzioni già
valutate — *cap aggregato per turno condiviso fra le unità* oppure `MaxPromptsPerReaction = 1` — senza toccare
il modello, perché sono entrambe parametri.

> **La soglia dei 20 s è coerente con le bande di formato** ([`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md)
> §9): playback tipico **8–15 s** in 2v2 e **12–20 s** in 3v3 Standard. Attenzione a **cosa** si confronta: le
> bande misurano il **playback** (Presentation Time), la soglia misura la resolution **comprese le finestre**
> (Presentation + Decision Time). Sommarle senza distinguerle è l'errore che §11 di quella spec esiste per
> evitare — e il motivo per cui `ReactionDecisionSeconds` è una metrica separata da `ResolutionPlaybackSeconds`.

**Seconda revisione** a **M10** (rete e privacy): il modello a N round-trip va verificato contro latenza,
riconnessione e timeout di rete, dove il «timeout → HOLD» diventa anche la risposta al giocatore disconnesso.
