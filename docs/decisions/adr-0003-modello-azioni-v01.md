# ADR-0003 — Modello azioni/priorità del catalogo v0.1 sulle macro-fasi di Atlas

> **Stato**: Accettato — da implementare · **Data**: 2026-08-05 · **Decisore**: utente (dev singolo)
> **Contesto sorgente**: `docs/archive/pdr-v0.1/RT_PDR_12_Catalog_v0.1.pdf` + `docs/src/RefactorTactics — Catalogo e bilanciamento v0.1.pdf`,
> su richiesta di realizzare la roadmap v0.1 (`docs/road-map_info.md`)
>
> ⚠️ **Emendamento 2026-08-07** — [`spec-durata-partita-e-scala-mappe.md`](../gameplay/spec-durata-partita-e-scala-mappe.md):
> il **«limite di 12 turni»** citato qui sotto non è più una regola universale. Resta la decisione strutturale —
> **la partita finisce per più vie, non solo per eliminazione** — mentre il numero diventa `RoundLimit`,
> **parametro del formato di partita**: **10–14** per il 2v2 della v0.1 (12 resta il valore iniziale del
> catalogo), **16–20** per il 3v3 Standard. Nient'altro di questo ADR cambia: le macro-fasi
> `Prep → Dash → Blast → Move` e il Move dopo il Blast restano invariati.

## Contesto

Il PDR 12 e il catalogo di bilanciamento v0.1 definiscono un vertical slice 2v2 molto più ricco di quello
canonico: **4 eroi** (Flux, Riva, Bastion, Vektor) con 4 abilità ciascuno, ~35 azioni con ID stabili,
**reazioni**, 8 terreni con effetti attivi, coperture direzionali e strutture (porte/ponti/pannelli),
**obiettivi dinamici** e partita a **12 turni** massimi.

Questi documenti sono, nella gerarchia delle fonti, **requisiti di lungo periodo** (`piano-canonico-mvp.md §1`):
direzione, non scope. Divergono dal canone su sette punti *load-bearing*:

| # | Divergenza | Canone (prima di questo ADR) | Catalogo v0.1 |
|---|---|---|---|
| 1 | Ordine delle fasi | `Prep → Dash → Blast → Move` | `Prep(10) → Movimento(20) → Controllo(30) → Attacco(40) → Ambiente(50) → Cleanup(60)` |
| 2 | Budget movimento | 4 celle (Dash 3) | 5 MP, costo intero per cella (1 normale, 2 difficile/rampa) |
| 3 | Reazioni | north-star, escluse (`§8.2`) | slot Reazione con trigger, 1 attivazione/turno |
| 4 | Roster | 2 archetipi (Ranger/Guardian) | 4 eroi con identità, statistiche e varianti |
| 5 | Vittoria | eliminazione della squadra | eliminazione **+** obiettivi **+** limite di round *(12 nel catalogo; oggi `RoundLimit` di formato)* |
| 6 | Versione engine | UE **5.8.1** (bloccata) | UE 5.6.x |
| 7 | Rete | rimandata a M10 | client-server autoritativo con rollback limitati |

L'utente decide di **adottare il catalogo v0.1 come scope della release v0.1**, con una correzione esplicita
sulla divergenza #1: **le macro-fasi restano quelle di Atlas Reactor**.

## Decisione

### 1. Le macro-fasi non cambiano

`ERTMatchPhase{ Planning, Prep, Dash, Blast, Move, Cleanup, MatchEnded }` resta **invariato**.
In particolare il **Move resta dopo il Blast**: l'attacco vale da fermo, e muoversi è un impegno che si paga.
È l'identità tattica di Atlas Reactor e la premessa di tutta la logica esistente (bot compreso:
«l'attacco vale solo da fermo perché il Blast precede il Move»).

Il modello a 7 fasi del catalogo **non sostituisce** le macro-fasi: i suoi codici numerici diventano un
**attributo delle azioni**, rimappato sulle macro-fasi secondo la tabella §3.

### 2. Del catalogo si adotta tutto il resto

- **Modello dati azione**: `ActionId` (FName, ID stabile tipo `Action.Move`), codice di fase, `Priority`
  intera, `Range`, `Cost` (MP), `Cooldown` in turni, `Fallback`, `bCanBeInterrupted`.
- **Ordinamento intra-fase per priorità intera**: priorità minore risolve prima. Estende — non contraddice —
  la regola APNAP di `piano-canonico-mvp.md §5.1`, che già prescriveva
  «velocità → priorità (intera) → tie-break assoluto». Ordine totale:
  `MacroFase → Priority → ActionDefinitionId → SourceUnitId → EventSequence`. Mai l'ordine di una `TMap`.
- **Budget movimento**: **5 MP**, costo intero per cella (1 normale, 2 difficile, 2 salita via rampa);
  Sprint 8 MP; Dash/Charge/Leap distanza fissa. Sostituisce «4 celle / Dash 3» (canone §6).
- **Slot per eroe**: 1 movimento + 1 azione principale + 1 reazione + facing finale + fallback dichiarato.
- **Reazioni**: dichiarate in planning, trigger valutato deterministicamente, **massimo 1 attivazione per turno**.
- **Terreni** (8), **stati** (Wet, Burning, Electrified, Obscured, Rooted, Exposed, Marked, Slow),
  **coperture/strutture** con integrità, **obiettivi dinamici**, **fallback** espliciti per azione.
- **Cataloghi come documenti versionati** in `docs/balance/` (§13 del catalogo) e data asset
  `PDA_*` sotto `Content/RT/` **feature-first** (non `Content/RefactorTactics/Data/` come scrive il catalogo:
  prevalgono le `convenzioni-contenuti-ue.md`).

### 3. Rimappatura dei codici del catalogo sulle macro-fasi

| Codice catalogo | Contenuto | Macro-fase canonica | Nota |
|---|---|---|---|
| **0** Snapshot | congelamento stato, intenti, seed | fine di `Planning` | esiste già (`FRTHexSnapshot`) |
| **10** Preparazione | Guard, Brace, Shield, stance, trappole, `SuppressiveLine`, prep reazioni | **Prep** | 1:1 |
| **20** Movimento *rapido* | Dash, Charge, Leap, Reposition, Sprint | **Dash** | la mobilità rapida precede il Blast |
| **30** Controllo | Push, Pull, Root, Interrupt, Slow, reazioni (Counter/Intercept/Deflect/Cleanse) | **Blast** (sotto-ordinamento per priorità, prima del danno) | il controllo non è una macro-fase separata |
| **40** Attacco | attacchi, abilità offensive, cure, `Activate`, `Interact` | **Blast** | 1:1 |
| **20** Movimento *normale* | `Action.Move` | **Move** | ⚠️ **qui il catalogo divergeva**: lo metteva prima dell'attacco |
| **50** Ambiente | hazard, propagazione elettrica, fuoco/acqua | **Cleanup**, prima dei KO | dopo il Move: colpisce anche chi è appena entrato nella cella |
| **60** Cleanup | KO, verifica obiettivi, decremento cooldown, TurnLog | **Cleanup** | 1:1 |

Conseguenza operativa: la fase 20 del catalogo **si sdoppia**. Ogni azione di movimento dichiara
esplicitamente se risolve in `Dash` (mobilità rapida) o in `Move` (percorso normale). Non esistono azioni
che risolvono «in mezzo».

### 4. Divergenze scartate (restano north-star o decisioni precedenti)

| Divergenza | Esito |
|---|---|
| Ordine `Movimento → Attacco` | **Scartata**: prevalgono le macro-fasi Atlas (decisione utente) |
| UE 5.6.x | **Scartata**: UE **5.8.1** bloccata dal canone §3 |
| Client-server con rollback | **Rimandata** a M10 (rete e privacy); v0.1 resta offline, architettura server-authority-ready |
| 4v4, GAS, progressione | Fuori scope (north-star §8) |
| Stack di reazioni LIFO interattivo | **Scartato**: viola l'invariante #3 (nessuna attesa nel resolver). Le reazioni v0.1 sono preparate in planning e valutate nello snapshot della fase — vedi §8.2 del canone |
| Ghiaccio scivoloso | Adottato come **opzionale**: il catalogo stesso lo dichiara rimandabile |

## Alternative considerate

| Alternativa | Esito |
|---|---|
| Canone prevale, catalogo resta north-star (2 archetipi, nessuna reazione) | Scartata dall'utente: v0.1 deve essere lo slice completo |
| Adottare il modello a 7 fasi del catalogo **incluso** l'ordine Movimento→Attacco | Scartata dall'utente: le macro-fasi sono quelle di Atlas |
| **Macro-fasi Atlas + modello azioni del catalogo** ✅ | **Scelta** |

## Conseguenze

**Positive**: l'identità tattica (attacco da fermo, movimento rischioso) è preservata; il resolver, il bot e i
test di ordine-indipendenza esistenti non vanno reimpostati; il campo `Priority` fornisce l'ordinamento fine
che `§5.1` già richiedeva ma che il combat «danni sommati» non esercitava; il contenuto (azioni, eroi,
terreni) diventa **dati**, non codice.

**Negative / costi**:
- ogni azione di movimento va classificata `Dash` o `Move` — la tabella del catalogo non lo dice e va decisa
  voce per voce (fatto in `roadmap-v0.1.md`, epic **E4**);
- il budget passa da 4 celle a **5 MP**: i test di movimento e i pesi del bot vanno riparametrati;
- le reazioni aggiungono una superficie nuova (trigger, una-attivazione, interazione con l'invariante #3);
- ~35 azioni × fase/priorità/fallback = molte combinazioni da coprire con test;
- `piano-canonico-mvp.md §6` (movimento 4 celle, Dash 3) è **superato** da questo ADR.

**Invarianti**: #1–#7 **preservati**. Le reazioni non introducono attese nel resolver; nessun float entra in
costi, priorità o danni (il catalogo è già tutto intero); la privacy dell'intento (#6) si estende alle
reazioni — visibili agli alleati, **mai** replicate ai nemici.

## Verifica

- Test di rimappatura: per ogni codice del catalogo, un'azione di esempio risolve nella macro-fase attesa
  (in particolare `Action.Move` **dopo** `Action.BasicAttack`).
- Test di ordinamento: permutare l'array di input non cambia il TurnLog (estende
  `FRTResolvePathsOrderIndependenceTest` alle azioni con priorità).
- Test di budget: 5 MP con costi 1/2 e rampa 2; Sprint 8 MP applica `Status.Exposed`.
- Test «nessun float»: costi, priorità e danni sono interi in tutti i data asset (validator, CP 1.4).

DoD completa in [`v0.1-definition-of-done.md`](../roadmap/v0.1-definition-of-done.md); checkpoint in
[`roadmap-v0.1.md`](../roadmap/roadmap-v0.1.md).

## Revisione

Rivedere alla chiusura dell'epic **E5 (Reazioni)** — è il punto dove il costo può sfondare. Se sfonda:
degradare a difensive di sola fase Prep (`Guard`/`Brace`/`Shield`) e rimandare `Counter`/`Intercept`/`Deflect`
a dopo la v0.1, senza toccare il resto del modello.

### Esito della revisione — E5 chiusa (CP 5.4, issue `#53`)

**Il costo non ha sfondato: la via di degrado NON è stata applicata.** Le tre reazioni che il piano B avrebbe
rimandato fuori dalla v0.1 — `Counter`, `Deflect`, `Intercept` — sono tutte costruite e collaudate, insieme alle
difensive di Prep. Nessun elemento di E5 è stato tagliato.

**Il modello ha retto senza modifiche strutturali.** I quattro checkpoint hanno aggiunto dati e un punto di
valutazione, non eccezioni al motore:

- `ERTActionSlot::Reaction` e `ERTReactionTrigger` sono **campi di `FRTActionDef`**, cioè catalogo. Aggiungere
  una reazione resta "aggiungere una riga di dati", che è la promessa dell'ADR.
- `Action.Shield` è arrivata **senza una riga di codice**: `ResolvePrep` traduceva già `ERTActionEffect::Shield`
  in scudo temporaneo. È la prova più netta che il motore azioni sta facendo il suo lavoro.
- Il trigger si valuta su `Plan.Hits` **già raccolti** — nessun `Delay`, nessuna timeline, nessuna attesa nel
  resolver. L'invariante #3 non è stata toccata, e lo stack LIFO interattivo resta north-star (canone §8.2).

**Le due correzioni al modello, entrambe additive:**

1. **L'ordine intra-fase delle reazioni conta davvero.** `Action.Intercept` (priorità 10) cambia il *bersaglio*
   dei colpi, quindi deve risolvere prima che `Deflect` (15) e `Counter` (20) valutino chi è stato colpito.
   L'ordine `macro-fase → priorità → ActionId → UnitId → EventSequence` che l'ADR già prescriveva si è rivelato
   sufficiente: è servito applicarlo, non cambiarlo.
2. **Servono due meccanismi di delta sul danno, non uno.** `ApplyFirstHitDelta` (una volta per bersaglio) non
   può esprimere `Brace` ("-10 a *ogni* danno diretto"): è nata `ApplyDamageDelta`. Sono regole diverse, e
   tenerle in due funzioni le rende distinguibili invece di nascondere un flag dentro una.

**Il limite che resta dichiarato**: `Cleanse` opera sui tre stati che esistono oggi; `Burning` ed `Electrified`
sono ambiente (epic E8/CP 8.2). Il meccanismo scorre una lista di tag, quindi l'estensione non toccherà il
resolver. Analogamente, le clausole "non su danno ambientale" di `Counter`/`Deflect`/`Intercept` sono verificate
**per costruzione** del trigger, non simulando hazard inesistenti.

**Privacy (CP 5.4)**: l'invariante #6 è stata estesa alle reazioni introducendo un **DTO filtrato per squadra**
(`FRTIntentView`, prodotto da `URTIntentPrivacyLibrary::FilterForTeam`). Prima la UI leggeva il piano completo di
tutte le unità e decideva di non disegnare quelle avversarie: un occultamento *grafico*, cioè un dato presente
sul client e nascosto a schermo. Ora un piano avversario non rivelato non esiste proprio nella vista, e la
reazione non viene mai copiata in una vista avversaria — **neppure sotto `Status.Reveal`**: `Reveal` mostra cosa
un'unità sta per *fare*, non cosa è pronta a *parare*. È la lettura letterale della DoD di CP 5.4 ("mai visibile
ai nemici") ed è una scelta di design da riconfermare quando la rete arriverà a **M10**, che è il momento in cui
questo DTO smette di essere una precauzione e diventa il payload replicato.

*Revisione eseguita alla chiusura di E5, con i quattro checkpoint (`#50`–`#53`) mergiati in `main`.*
