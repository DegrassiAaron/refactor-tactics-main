# Referto — teletrasporto e movimenti istantanei

> `HISTORICAL` · **Referto di triage**, non una fonte. · **Data**: 2026-08-12 · **Base**: `c0eee5b8`
> **Sorgente esaminato**: `RefactorTactics_Teleport_InstantMovement_Claude.md`, archiviato in
> [`../../archive/src/handoff/2026-08-12-teleport-instant-movement.md`](../../archive/src/handoff/2026-08-12-teleport-instant-movement.md).
>
> **Cosa possiede**: il verdetto sezione per sezione e le misure che lo sostengono.
> **Cosa non possiede**: nessuna regola. La regola vive in
> [`../../gameplay/spec-tassonomia-movimento.md`](../../gameplay/spec-tassonomia-movimento.md); le domande
> aperte in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) `MOV-1`, `MOV-2`.

## 1. Il verdetto in una riga

Il kit ha ragione sulla tesi e torto sulla premessa. La tesi — *«un movimento molto veloce non è un
teletrasporto»* — è già canone. La premessa — che nel repository esista solo il primo dei due — è **falsa**:
`ERTMovementStyle::LinearLeap` è già un trasferimento, archiviato sotto Dash.

Il valore del kit non è quindi ciò che propone di costruire. È che, applicando la sua stessa distinzione al
codice, fa emergere **due righe sbagliate nella matrice dell'owner** e **una quarta capacità del motore
irraggiungibile dal roster**.

## 2. Le cinque misure che decidono il triage

Nessuna viene dal kit; tutte prese sul branch.

| # | Misura | Comando / riferimento | Conseguenza |
|---|---|---|---|
| **M1** | Il Teleport **non esiste** in produzione | `git grep -i teleport -- Source/` → solo `Structures.Bridge.NoTeleportOnRemoval`, un test che verifica il **contrario** | la §2 del kit è accurata su questo |
| **M2** | 🔴 Ma la **semantica** esiste: `LinearLeap` fa `Result.Entered = { destinazione }` e basta | `RTMovementActionLibrary.cpp` §`LinearLeap` | e poiché `ApplyTerrainOnEnterEffects(Map, Unit, Entered)` legge **esattamente** `Entered` (`RTTurnManager.cpp`), `Action.Leap` **non prende gli hazard intermedi** |
| **M3** | 🔴 Quindi due righe della colonna **Dash** sono false di uno dei suoi quattro stili | matrice §2 dell'owner: `micro-step: sì`, `hazard intermedi: sì` | corrette in `policy`, come già era `attraversa le celle intermedie` |
| **M4** | Lo scenario che il kit propone **esiste già**, scritto prima dell'implementazione | `Scenarios/Spec/Movement/TeleportSkipsIntermediateCells.json` | `requires: ["Teleport"]` lo tiene `BLOCKED`; i 90 HP attesi sono dentro, con le istruzioni per chi lo completerà |
| **M5** | 🔴 `Action.Leap` **non è nel kit di nessun eroe** | `git grep Leap` → catalogo core, test, zero kit | è la **quarta** capacità irraggiungibile dal roster dopo le tre di [#425](https://github.com/DegrassiAaron/refactor-tactics-main/issues/425), e l'unica con semantica di trasferimento |

## 3. Audit GitHub, come richiesto dal §1

```text
Search performed (repo-scoped, open + closed):
  teleport · blink · teletrasporto · spatial transfer · instant movement   -> 0 risultati
  Leap (in:title)                                                          -> 0 risultati

Closest existing issues:
  #425 CLOSED  Tre capacita' del motore non sono raggiungibili dal roster
  #307 CLOSED  La causa di uno spostamento non e' leggibile nel TurnLog
  #308 CLOSED  Una spinta attraverso il fuoco genera gli eventi di ogni cella
  #436 OPEN    CP 36.1 — tassonomia delle capability (possiede la BLOCCABILITA')
  #605 OPEN    CP 38.2 — validazione del piano (possiede il punto unico LEGALE/ILLEGALE)

Why none owns the delta:
  #425 ha censito TRE capacita' irraggiungibili — kiting, LinearDash, self-target — e `LinearLeap`
  non e' fra quelle. Stessa classe, quarto caso, scoperto da questo consolidamento. La issue e'
  chiusa e non si riapre per aggiungere scope.
```

⚠️ **La fotografia del kit sulle issue è accurata**: le nove che cita (`#307 #308 #165 #159 #605 #606 #609 #641 #436`) esistono tutte con lo stato dichiarato. È raro e va detto.

## 4. Verdetto sezione per sezione

| § del kit | Verdetto | Dove vive davvero |
|---|---|---|
| §0–§2 regola operativa, audit | **meta**, ed è corretta | eseguita |
| §3.1 `Move / Dash / Phase Move / Teleport` | ⚠️ **parzialmente nuovo, e mal partizionato** | «Phase Move» è `LinearPass` (attraversa e colpisce) — esiste. Ma la partizione manca il caso vero: `LinearLeap`, che non è nessuna delle quattro |
| §4 flusso del trasferimento | **prematuro** | nessun runtime lo consuma. Le sue proprietà osservabili sono già righe della matrice |
| §5 Overwatch e trigger spaziali | **già canone** | matrice: `trigger spaziali → solo all'arrivo` |
| §6 Teleport ≠ Portal | ✅ **nuovo e utile**, non recepito | un portal come **arco del grafo** tocca `GraphRevision` e il pathfinding. Fuori scope qui, e senza consumatore |
| §7 destinazione visibile, no blind teleport | **prematuro** | è una policy di un'azione che non esiste |
| §8 `ConflictPolicy = FailAll` | ✅ **buon argomento**, non recepito | *«non dipende dall'ordine di iterazione»* è l'invariante giusta. Ma non c'è conflitto da risolvere finché non c'è un trasferimento volontario |
| §9 Swap atomico · §10 Recall/Anchor | **futuro** | ⚠️ `Action.Anchor` **esiste già** e significa un'altra cosa (resistenza allo spostamento): un `Recall/Anchor` erediterebbe una collisione di nome |
| §11 Forced Teleport | **futuro** | il contrasto con #308 è corretto |
| §12 rumore di partenza/arrivo | ✅ **utile**, rinviato | riusa il dominio di #159, come il kit stesso chiede |
| §13 multilivello | **già deciso in piccolo** | `LinearLeap` **non cambia layer** (`RTMovementActionLibrary.h`): la policy `SameLayerOnly` che il kit consiglia è già quella in vigore |
| §14 LOS e traiettoria | **già canone** | servizi già distinti |
| §15 non creare una seconda tassonomia | ✅ **rispettato** | nessun tipo nuovo introdotto. È `MOV-1` a decidere se servirà |
| §16 eventi e TurnLog | **già canone** | #307 ha reso ricostruibile causa e famiglia; nessun campo nuovo |
| §17 sei scenari | ✂️ **uno esiste, cinque prematuri** | vedi M4 |
| §18 UI/UX | **prematuro** | «non disegnare un path che sembri attraversato» è giusto e si scrive quando c'è cosa disegnare |
| §19 scope `Short Blink` | ⚠️ **è `MOV-2`**, non una decisione | |
| §20 la contraddizione | ✅ **risolta, e la risposta è più netta di come la pone** | **non esiste nessuna `D-nnn`** che fissi l'assenza del Teleport dalla v0.1: è un fatto misurato, non una decisione. Quindi portare un Blink non contraddirebbe niente — richiede una decisione che non c'è |
| §21–§26 deliverable e gate | **meta** | eseguiti |

## 5. Cosa è cambiato

| File | Cosa |
|---|---|
| `docs/gameplay/spec-tassonomia-movimento.md` | matrice: `micro-step` e `hazard intermedi` della colonna Dash da «sì» a **policy** · nota che spiega perché · le due frasi «Teleport non esiste» precisate |
| `docs/OPEN_DECISIONS.md` | `MOV-1` (famiglia o policy?) e `MOV-2` (Blink in v0.1?) |
| `docs/roadmap/feature-registry.yaml` | nota su `RT-FEAT-ACTION-MOVE-PROFILES`: `Action.Leap` senza kit |

**Una issue aperta, e una sola**:
[#645](https://github.com/DegrassiAaron/refactor-tactics-main/issues/645) — `Action.Leap` è la quarta
capacità irraggiungibile dal roster. Non dipende da `MOV-1`: il salto è irraggiungibile qualunque sia la
famiglia a cui appartiene. Delle sei che il kit lasciava intravedere (Blink, Swap, Recall, Portal, Forced
Teleport, Blind Teleport) **nessuna è stata aperta**: nessuna ha un delta finché `MOV-2` è aperta.

**Non toccati, e la ragione**: il workbook (vietato da `balance/README.md`) · `CLAUDE.md`/`AGENTS.md` (nessuna regola globale nuova) · le viste generate (rigenerate, non editate) · gli scenari (quello che serve esiste già) · il codice (il kit lo esclude, e `MOV-1` va decisa prima).

## 6. Cosa questo referto non ha fatto

- **Non ha creato un'epic né un subsystem.** Nessun `ERTSpatialTransferType`, nessun `URTSpatialTransferService`: il §15 del kit lo vieta e la misura M2 lo rende inutile.
- **Non ha assegnato una `D-nnn`.** `MOV-1` e `MOV-2` sono domande, e `AGENTS.md` vieta di sceglierle per plausibilità.
- **Non ha completato lo scenario `TeleportSkipsIntermediateCells`**: si completa quando la capability esiste, e il file dice già come.
- **Non ha aperto una issue per il Blink.** Non esiste un delta implementativo finché `MOV-2` è aperta: sarebbe una issue la cui prima riga è «decidere se farla».
