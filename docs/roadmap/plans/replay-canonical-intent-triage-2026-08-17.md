# Replay, Canonical Intent e privacy server/client — triage del sorgente

> `CURRENT` · **Stato**: sorgente consumato, decisioni prese · **Data**: 2026-08-17
> **HEAD misurato**: `9a1bd1d4` (`origin/main`, dopo il merge di [#1108](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1108))
> **Sorgente**: `RefactorTactics_Claude_Replay_CanonicalIntent_Roadmap_v0.1-v1.0_2026-08-16.md`
> — 46 sezioni, 8 proposte esplicite (`P1`–`P8`), archiviato in
> [`../../archive/src/`](../../archive/src/RefactorTactics_Claude_Replay_CanonicalIntent_Roadmap_v0.1-v1.0_2026-08-16.md).
> **Owner delle regole**: [`adr-0009-replay-logico-canonico.md`](../../decisions/adr-0009-replay-logico-canonico.md) ·
> [`adr-0004-finestre-di-reazione.md`](../../decisions/adr-0004-finestre-di-reazione.md) ·
> `../feature-registry.yaml`. Questo referto non è owner di niente.
> **Particolarità**: il sorgente si dichiara *«HEAD osservato `557fdb88`»* e chiede di non fidarsene.
> Applicato alla lettera — e la verifica ha spostato **più della metà** del documento dalla colonna
> «da fare» a «già fatto o già in corso».

---

## 1. Il verdetto in una riga

Il sorgente ha ragione sui **principi** e è arretrato sui **fatti**: la sua roadmap `R0`–`R6` descrive
lavoro che in tre casi su sei è chiuso o ha già un piano scritto, mentre le sue **otto proposte
architetturali** — quelle che dichiara da non canonizzare senza audit — sono l'unica parte con **zero
occorrenze nel repository** e nessuna issue che le possieda. Il gap reale non è il replay: è il
**Canonical Intent**.

---

## 2. La regola numero zero, applicata

Il sorgente elenca sette issue come vive. Misurate il 2026-08-17 con `gh issue view`:

| Issue | Il sorgente dice | Misurato |
|---|---|---|
| #165 | «verificare stato reale» (`R0`) | ✅ **CLOSED** — CP 14.5 consegnato, TurnLog **v8** |
| #886 | «`R1`, da implementare» | 🔵 **OPEN**, ma il DoD è stato **riscritto oggi** da uno spec panel, e la track `simulation` è `ACTIVE` su di essa |
| #166 | «`R2`, UI e pacing» | ✅ OPEN, P2 — invariato |
| #314 | «`R3`, P3/tagliabile» | ✅ OPEN, P3 — invariato |
| #542 | «`R4`, DecisionProvider» | 🔵 OPEN, ma il **design è scritto e approvato** — [`cp153b-decision-provider-design-2026-08-16.md`](cp153b-decision-provider-design-2026-08-16.md) |
| #780 | «Store canonico, futura» | ✅ OPEN, P0, `post-v0.1` — invariato |
| #209 | non citata | ✅ CLOSED |

**Tre sezioni su sei della roadmap `R0`–`R6` progettano lavoro che è già in `main` o già pianificato.**

---

## 3. Le misure che hanno spostato il verdetto

Nessuna di queste è ripresa dal sorgente: sono state eseguite su `9a1bd1d4`.

### 3.1 I quattro test che il sorgente dà per «previsti o esistenti» esistono tutti

`Replay.Player.RunsWithoutResolver` · `Replay.Player.RejectsIncompatibleArchive` ·
`Replay.Verifier.ReportsFirstDivergence` · `Replay.Verifier.ResimulationIsDeterministic` — **4 su 4**
presenti. E non sono soli: `git grep -oh -E "Replay\.[A-Za-z.]+" Source/RefactorTactics/Tests/`
restituisce **48** nomi distinti, contro i quattro che il §3 nomina. Il sottosistema replay è
**molto** più coperto di quanto il sorgente supponga.

### 3.2 I nove test che il sorgente *propone* sono assenti tutti e nove

`Replay.Verifier.ReactionDecisionsComeFromTheTrace` · `…RecordedResponseBeatsLiveDecider` ·
`CanonicalIntent.RoundTrip` · `…NoRuntimePointers` · `…NoPresentationFields` · `…UsesStableActionIds` ·
`Privacy.CanonicalIntentNeverReplicatedToEnemy` · `…FullTurnLogNeverReplicatedRaw` ·
`…ClashPendingDecisionIsServerOnly` — **0 su 9**.

Il §40 è quindi la parte **più accurata** del sorgente, ed è quella che nessuno ha ancora eseguito.

### 3.3 🔴 `Spec.Clash.HiddenUntilReveal` sembra esistere e non esiste

Il primo grep lo dava presente in `Scenarios/Spec/Harness/LogAssertionsReadTheTurnLog.json`. Aprendo
il file, la stringa vive dentro il valore di `"_nota_assenza"` — **prosa che cita lo scenario futuro**,
non lo scenario. Nessuno dei sei scenari proposti ai §39 esiste.

È la forma d'errore che costa di più in questo repository: una citazione trovata da `grep` che *sembra*
una prova. Il conteggio corretto degli scenari proposti dal sorgente è **0 su 6**.

### 3.4 Le otto proposte non hanno un solo byte nel repository

| Simbolo cercato | `Source/` |
|---|---|
| `FRTCanonicalIntent` | **0** |
| `CommittedResponse` | **0** |
| `rtintent` · `rtsetup` | **0** |
| `DecisionGroupId` | **0** |

Mentre esistono, e sono ciò da cui le proposte partono: `FRTPlannedIntent`
(`Turn/RTIntentPrivacyLibrary.h`), `FRTReactionDecision` (`Turn/RTReactionOpportunityTypes.h:250`),
`OpportunityId` (prodotto, ordinato, serializzato).

### 3.5 `ERTReactionDecisionOutcome` ha esattamente i sei valori dichiarati

`FireChosen` · `HoldChosen` · `HoldTimeout` · `HoldNoDecider` · `HoldRejected` · `HoldImmediate`.
Il §6 del sorgente è **esatto**, ed è il solo punto in cui riporta un enum senza errori. La sua critica
del §11 — *«sufficiente per Overwatch ma Overwatch-centric»* — regge sulla misura: cinque dei sei
valori nominano `Hold`, e il sesto `Fire`.

---

## 4. Decision matrix delle otto proposte

Vocabolario richiesto dal sorgente stesso (§43.B).

| # | Proposta | Verdetto | Perché, e dove atterra |
|---|---|---|---|
| **P1** | `FRTCanonicalIntent` separato da `FRTPlannedIntent` | **PROPOSED** | Il difetto è misurato: `FRTPlannedIntent` mescola intent, stato e presentation (`FText ActionName`, `OwnerCell`, `bRevealed`). Ma la forma proposta è concettuale e il sorgente lo dichiara. → domande aperte `RCI-1`/`RCI-2` |
| **P2** | Canonical Intent server-authoritative | **ALREADY DECIDED** | Non è nuova: è il principio 1–2 del sorgente stesso e la premessa di **#780** (`CP 40.2`, P0, `post-v0.1`). Nessuna decisione da aprire |
| **P3** | Persistenza `turn-N.rtintent` | **DEFERRED** | Dipende da P1: non si serializza un tipo che non esiste. E la milestone è `v0.2` per ammissione del §35. → `RCI-3` |
| **P4** | `CommittedResponse` separata dall'outcome | **OPEN ISSUE** *(nuova)* | È l'unico gap **v0.1-adiacente**: senza di essa il Verifier di **#886** legge `ERTReactionDecisionOutcome`, che è Overwatch-centric, e `#314` aggiungerebbe valori roster-specific. Il sorgente chiede di **non bloccare #886** — rispettato: l'issue è separata |
| **P5** | Full TurnLog server-only | **ALREADY DECIDED** | `RTIntentPrivacyLibrary` esiste e `RTIntentPrivacyTests` ha **19** occorrenze di `FRTPlannedIntent`. Il principio è già implementato per gli intenti; la proiezione del TurnLog è **#780**/E40 |
| **P6** | `match.rtsetup` | **DEFERRED** | Il §22 lo lega a `RulesVersion`/`ContentManifestHash`, che il repository tiene **deferiti** — e il sorgente stesso vieta di anticiparli (§34) |
| **P7** | Full vs Perspective Replay | **DEFERRED** | v1.0 per ammissione del §35. Nessuna azione oggi |
| **P8** | `DecisionGroupId` | **REJECTED per ora** | Il sorgente lo propone e nella stessa riga ne vieta l'uso decisionale (*«senza usarlo per decidere se il confronto è contested»*). Un identificatore che non decide niente e non ha consumatore è **dato senza consumatore**: si apre quando `#314` produce il primo caso multi-responder reale |

**Conteggio**: `PROPOSED 1 · ALREADY DECIDED 2 · OPEN ISSUE 1 · DEFERRED 3 · REJECTED 1`.

---

## 5. Cosa NON è entrato, e perché

| Sezione | Perché |
|---|---|
| §3, §4, §5, §6 | descrivono lo stato **corrente** e sono corretti: non c'è niente da applicare |
| §7 (`R1`/#886) | il DoD è stato riscritto il 2026-08-17 da uno spec panel più preciso di questa descrizione |
| §8 (`#542`) | il design esiste già in [`cp153b-decision-provider-design-2026-08-16.md`](cp153b-decision-provider-design-2026-08-16.md) |
| §10 (Reaction Profile per eroe) | assegna `Profile.Grounding`/`Sidestep`/`Glance` a Gadget/Phase/Wraith. È **contenuto di #314**, che è `P3` e dichiaratamente tagliabile: anticiparlo qui sarebbe scope creep sulla v0.1 |
| §24 (tabella server/client) | 14 righe di distribuzione. Nessuna contraddice il repository, e nessuna è azionabile finché la rete è **E40/v0.5**. Resta nel sorgente archiviato come provenienza |
| §33 (lane A–D) | il sorgente stesso dice «verificare lo stato aggiornato»: le lane sono superate da `parallel-batch.yaml`, che governa il write-set |
| §35 (roadmap v0.2→v1.0) | è una **seconda copia** della roadmap per release. Gli owner sono [`../roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) e le epic `E36`–`E45`, tutte già aperte. Duplicarla sarebbe stato che nessun `--check` vede invecchiare |

---

## 6. Cosa entra

1. **[#1118](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1118)** per `P4` — la
   generalizzazione della risposta di reazione, con l'adapter v8 e il vincolo esplicito di **non
   bloccare #886**. Sub-issue di **#152** (`E14`).
2. **[#1119](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1119)** — le quattro domande
   `RCI-1`…`RCI-4` sul Canonical Intent, portate da una issue `question` invece che da righe in
   [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md): quel file è nel write-set di
   [#1104](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1104), aperta. Vanno
   riconciliate lì quando #1104 atterra. Sub-issue di **#773** (`E40`).
3. **Un commento di consolidamento su [#780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/780)**,
   che il sorgente §26 chiede esplicitamente (*«non applicare senza riconciliare #780»*). Il corpo di
   #780 **non è stato modificato**: niente di ciò che dichiara è diventato falso.

**Nessun `D-nnn` nuovo.** Le due decisioni che il sorgente propone come canoniche (`P2`, `P5`) sono già
prese altrove, e le altre sei sono proposte o differite: aprire una decisione per una proposta è
precisamente ciò che il §41.19 del sorgente vieta.

---

## 7. Guardrail confermati dal repository

I venti errori del §41 sono stati confrontati con il codice. **Nessuno è attualmente commesso.** Tre
meritano di restare visibili perché il lavoro in corso li sfiora:

- **§41.9** *«matching reaction decision per ordine anziché identity»* — è esattamente il rischio di
  **#886**, il cui DoD riscritto lo nomina.
- **§41.12** *«response enum roster-specific»* — è il rischio di **#314**, e la ragione per cui `P4`
  diventa una issue invece di una nota.
- **§41.17** *«modificare generated registry output a mano»* — è il difetto che #1108 ha trovato
  **rosso su `origin/main`** il 2026-08-17 e corretto.

---

## 8. Limiti di questo referto

- **Non ho eseguito build né test Unreal**: il write-set è interamente `docs/` e GitHub. I conteggi di
  test sono `git grep` sui nomi dichiarati, non esecuzioni.
- **Le proposte `P1`/`P3` non hanno una firma verificata**: `FRTCanonicalIntent` come scritto nel
  sorgente cita `FRTDeclaredCondition` e `ERTIntentTargetKind`, che **non ho verificato esistere**.
  Chi implementa `RCI-1` lo misuri prima.
- **`OPEN_DECISIONS.md`, il Decision Log e il Feature Registry non sono stati toccati** perché contesi
  da PR aperte. Le quattro domande vivono in una issue finché quel terreno non si libera: è una
  lacuna **dichiarata**, non chiusa.
- 🔴 **Nessun gate protegge i simboli C++ citati in questo file, e la misura che lo dice va conservata.**
  Verificato con una mutazione: sostituendo un identificatore di questa pagina con
  `URTFakeSpatialLibrary`, `check-docs-symbols` resta **verde**. La ragione è nel gate stesso —
  `EXEMPT_DIRS = ("docs/archive/", "docs/src/", "docs/roadmap/plans/")` — e non è un difetto: i piani
  non sono normativi. Ma significa che `FRTPlannedIntent`, `FRTReactionDecision`,
  `ERTReactionDecisionOutcome`, `URTScenarioLoader` e gli altri sono stati verificati **a mano** con
  `git grep`, e chi rilegge fra un mese non ha un `--check` che glielo confermi.
  *(La stessa mutazione su un **link** fa invece cadere `check-docs-links`: quel gate copre questa
  cartella, e la sua copertura è stata dimostrata invece che supposta.)*
