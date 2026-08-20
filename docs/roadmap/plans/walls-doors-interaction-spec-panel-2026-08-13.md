# Muri, porte e interaction graph fino alla v1.0 — referto spec-panel

> `CURRENT` · **Stato**: revisione chiusa, **applicata** · **Data**: 2026-08-13, concluso il 2026-08-14
> **HEAD della revisione**: `f4cae9f5`
> **Sorgente revisionato**: `RefactorTactics_Walls_Doors_InteractionGraph_v1_Claude_Handoff_2026-08-13.md`
> (1383 righe, untracked), archiviato a fine sessione in
> [`../../archive/src/RefactorTactics_Walls_Doors_InteractionGraph_v1_Claude_Handoff_2026-08-13.md`](../../archive/src/RefactorTactics_Walls_Doors_InteractionGraph_v1_Claude_Handoff_2026-08-13.md)
> **Scopo**: classificare ogni voce dell'handoff contro il repository **misurato**, prima che qualcuno la
> implementi — quattro delle sue richieste hanno già dei test verdi
> **Decisione**: [D-138](../../decisions/RT_PDR_00_Decision_Log.md)

> **Che cosa possiede questo documento**: la *provenienza* del piano e il **filtro** applicato all'handoff.
> Le regole restano nei loro owner — `roadmap-post-v0.1.md` §E23 per il piano, `feature-registry.yaml` per lo
> stato, `scenario-map.md` per gli scenari, `OPEN_DECISIONS.md` per ciò che aspetta una persona. Se cerchi la
> regola, non è qui.

---

## A. Audit di HEAD

| | |
|---|---|
| Baseline dichiarata dall'handoff | `d40ccf63` — 2026-08-13 20:15 CEST |
| `HEAD` all'inizio del lavoro | `f4cae9f5` — **7 commit avanti**, stessa giornata |

La fotografia era vecchia di poche ore, e quelle poche ore contengono esattamente il lavoro che l'handoff
chiede in §5 e §18. Due commit la superano:

- **`ebf316a5`** — `RELEASE_ORDER` arriva a `v1.0` ([D-136](../../decisions/RT_PDR_00_Decision_Log.md)). L'handoff
  §5 dichiara `("v0.1","v0.2","v0.3","v0.4","future")` e chiede di canonizzare la ladder: **già fatto**, con un
  gate (`check_release_order()`) che rifiuta una release esprimibile e non descritta.
- **`551185bd`** — le sei epic `v0.5`…`v1.0` esistono su GitHub: **E40**–**E45** (`#773`–`#778`).

∴ §5 e §18 dell'handoff sono **NO ACTION**. I placeholder `E<ALLOCATE>` non vanno allocati: sarebbero una
seconda taxonomy sopra una appena canonizzata.

---

## B. Il filtro — che cosa l'handoff dà per mancante e invece esiste

L'handoff descrive il dominio come se le porte fossero da costruire. **Non lo sono.** La v0.1 ha già la porta
come *stato del bordo*, `INTEGRATED`, con dodici test.

| L'handoff chiede | Realtà misurata su `f4cae9f5` | Esito |
|---|---|---|
| §7.B «porta larga ~3 m: 1 DoorId, N transition, stato atomico» | `FRTHexDoor::DoorId` esiste e `SetDoorState` muta **il gruppo**. Due test, e fanno cose diverse: `Structures.Door.GroupClosesTogether` prova il **raggruppamento** (tre bordi di **una** cella, `DoorId 3`, più una quarta senza gruppo che resta ferma); `Structures.Door.StateChangeBumpsRevision` prova la **porta larga** — «Portone largo tre bordi: un comando, una revisione», tre **celle** sul bordo E con `DoorId 7` e `Revision == Before + 1` | ✅ **esiste** |
| §7.G «structure change → revision → path cache invalidation» | `Structures.Door.StateChangeBumpsRevision`, `…InvalidatesPathCache`, `…TruncatesPlannedPath` | ✅ **esiste** |
| §7.D «A valida, B valida, A→B bloccata» | `BlocksBetween` legge il bordo, non la cella; `TruncatePathToTopology` ferma il path con `BlockedByTopology` | ✅ **esiste** per la porta; ⬜ manca per la **geometria cotta** (CP 23.7) |
| §25 «deterministic sort, hash normalization» | `Structures.Door.OpsOrderIndependent`, `HexMap.DoorHashDeterminism`; `DoorId` entra in `RTMatchStateHash` | ✅ **esiste** |
| §27 «Destruction: `Destroyed` può tornare `Closed`?» | Già deciso: `Destroyed` è **TERMINALE**, in tre punti del codice, e `Structures.Door.DestroyedStaysOpen` lo pinna | ✅ **già deciso** |
| §6 scenari `WallCrossesCellStillStandable`, `FootprintCollisionBlocksCell`, `NinetyDegreeCornerBakesCorrectly` | Già in [`scenario-map.md`](../../technical/tooling/scenario-map.md) §pianificati, su `RT-FEAT-MAP-STANDABILITY`, CP 23.6 | ✅ **già mappati** |
| §9 scenari `ValidCellsBlockedTransition`, `DoorOpensTransition` | Idem, su `RT-FEAT-MAP-TRANSITION-CLEARANCE`, CP 23.7 | ✅ **già mappati** |

> 🔴 **La lezione, e vale oltre questo handoff.** Un pacchetto di consolidamento è una **fotografia datata**, e
> si filtra invece di applicarlo. Qui la fotografia aveva **sette ore** e già quattro dei suoi «da fare» erano
> fatti. Il costo di non filtrare non è il lavoro doppio: è che riscrivere `SetDoorState` per «introdurre» il
> gruppo atomico avrebbe **rotto** i tre test che lo dimostrano già.

### Che cosa manca davvero

| Gap | Perché è un gap | Dove atterra |
|---|---|---|
| **Stable ID che sopravvive al cook** | `DoorId` è un `int32` locale all'asset: identifica il gruppo *dentro* una mappa, non attraverso `cook`/scenari/replay. Il codice lo sa già — [`RTPointerInteraction.h:124`](../../../Source/RefactorTactics/Player/RTPointerInteraction.h): *«`DoorId` e arco esistono già e bastano. L'identità stabile è E23 (#324)»* | **CP 23.3**, v0.2 |
| **Interaction graph source→target** | Zero occorrenze di switch/lever/terminal/controller nel dominio mappa. Non esiste | **CP 23.4**, v0.2 |
| **Leggibilità `S1`/`D1`** | Nessuna etichetta tattica di struttura, nessun hover sorgente↔bersaglio | **CP 23.5**, v0.2 |
| **Standability e transizione cotte da geometria** | `DESIGNED`, non implementate | **CP 23.6/23.7**, v0.2 |

---

## C. La rimappatura — l'handoff propone una ladder che il repository non usa

L'handoff §12–§16 propone `v0.5 Network → v0.6 Authoring → v0.7 Graph v2 → v0.8 Beta → v0.9 RC`, e dichiara
esso stesso: *«Rimappare se la taxonomy reale è diversa»* (§12). **È diversa.**

| Handoff | Taxonomy reale (`roadmap-post-v0.1.md` §Le release) | Esito |
|---|---|---|
| §10 v0.3 — knowledge / FoW / relazioni non pubbliche | **E27** `#327` · Percezione completa: vista, udito, memoria | ✅ rimappato |
| §11 v0.4 — Operations scale e multilayer | **E30** `#331` · Classe di mappa Operations | ✅ rimappato |
| §12 v0.5 — network authoritative | **E40** `#773` · Il turno simultaneo in rete | ✅ rimappato |
| §13 v0.6 — production authoring / cook | **E41** · GAS come runtime delle abilità | ❌ **nessun owner** |
| §14 v0.7 — interaction graph v2 (AND/OR, power) | **E42** · Dedicated server e loop online reale | ❌ **nessun owner** |
| §15 v0.8 — beta gameplay / bot / UX | **E43** · Misura a lotti, e il bot che sa cosa misura | 🟡 parziale (il bot sì, la UX no) |
| §16 v0.9 — RC hardening | **E44** · Feature freeze, e ciò che regge | ✅ rimappato |
| §17 v1.0 — ship gate | **E45** · Un gate di produzione | ✅ rimappato |

**Le due righe rosse non diventano epic.** L'handoff stesso vieta di *«trasformare le idee v0.7+ in scope
approvato senza decisione»* (§31), e nessuna decisione le ha approvate. Restano idee registrate qui, che è il
posto delle idee; se il gameplay le chiederà, la decisione precede l'epic.

### Il vincolo che l'handoff non vede

La spec owner delle interazioni — [`spec-interazioni-mappa-cp101.md`](../../gameplay/spec-interazioni-mappa-cp101.md)
§11 — dichiara **fuori scope** il controllo remoto sorgente → bersaglio, e dice **perché**:

> *«il controllo **remoto** sorgente → bersaglio, che richiede la privacy dei collegamenti (§8) e quindi la rete»*

Non è una difficoltà: è una **dipendenza**. Ne segue la sola parte della ladder che vale la pena scrivere,
perché è verificabile:

```text
v0.2  E23.4   il graph è un DATO           ← cardinalità, ordine deterministico, validator dei binding
v0.3  E27     la relazione ha un PUBBLICO  ← Known/Unknown per squadra: prima non è esprimibile
v0.5  E40     la privacy è VERIFICABILE    ← canary di leak: prima non c'è un client a cui non dirlo
```

∴ un `Spec.Map.Interaction.NoHiddenRelationLeak` scritto in v0.2 **non fallirebbe mai**, e passerebbe per
assenza di rete, non per correttezza. Gli scenari di privacy dell'handoff §10 e §12 vanno dove il loro oracolo
esiste — ed è la ragione per cui questo referto non li aggiunge alla Scenario Map come `planned` in v0.2.

---

## D. Issue — classificazione delle proposte

Procedura §22 eseguita: `gh issue list` open+closed, ricerca per `wall|door|structure|interaction|geometry|transition|standability|cover|switch|controller|gate|bridge|elevator|knowledge`.

| Proposta handoff §8 | Azione | Motivo |
|---|---|---|
| Logical Structure Runtime Model | **NO ACTION** | `FRTHexDoor` + `FRTHexArc` + `ERTStructureOp` esistono e sono `INTEGRATED`. Un «modello runtime» nuovo sarebbe il secondo |
| Door Multi-Transition Atomic State | **NO ACTION** | `SetDoorState` lo fa; `GroupClosesTogether` prova il gruppo e `StateChangeBumpsRevision` la porta larga |
| Transition State Independent from Cell Validity | **UPDATE** `#324` CP 23.7 | Esiste per la porta, manca per la geometria cotta: è il delta, non una issue nuova |
| Interaction Graph v1 | **NEW** | Gap reale |
| Interaction UX v1 | **NEW** | Gap reale |
| Structure / Interaction TurnLog & Replay | **NO ACTION** | `FRTDoorChange` porta già l'attore e alimenta il TurnLog; l'hash è testato |
| E23 Scenario Pack | **NEW** | Gli scenari v0.2 del dominio non esistono |
| Stable ID e binding | **NEW** | Gap reale, ed è il **prerequisito** degli altri tre |

Le issue effettivamente aperte, con numeri reali, sono in §F.

### Correzioni all'epic `#324`

Due difetti misurati nel corpo live, entrambi da *derivato non aggiornato*:

1. la tabella dell'anticipazione dichiara **`#620` e `#621` «⬜ aperta»**: sono **CLOSED**. L'handoff aveva la
   riga giusta e l'epic quella sbagliata;
2. il corpo porta **`E23.1`–`E23.5`**, mentre l'owner documentale
   [`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) §E23 ha **23.1–23.7** da [D-065](../../decisions/RT_PDR_00_Decision_Log.md).
   `23.6` e `23.7` — standability cotta e transizione come dato — non erano su GitHub.

> ⚠️ L'handoff §3 dichiara che *«la baseline del corpo di #324 contiene E23.1…E23.7»*. È **falso** del corpo
> GitHub e **vero** dell'owner documentale. Un handoff non è autorità: la riga si verifica, e verificarla è ciò
> che ha trovato la deriva.

---

## E. Decisioni — filtrate, non trascritte

L'handoff §27 propone cinque decisioni aperte. Cercate **per tema**, non per prefisso:

| Proposta | Esito | Perché |
|---|---|---|
| Door width semantic: `WidthCm` è gameplay? | **NO ACTION** | La domanda presuppone un modello a centimetri che non esiste. Il gameplay legge `DoorId` + bordi, cioè già la *«baseline raccomandata»* dell'handoff. Aprirla registrerebbe come dubbia una scelta già implementata e testata |
| Destruction: `Destroyed` → `Closed`? | **NO ACTION** | Già deciso: terminale. Tre punti nel codice, un test |
| N-source logic: AND / OR / priority? | **APERTA** → `INT-5` | Reale, e non decidibile leggendo |
| Unknown controller: pubblico o TeamKnowledge? | **APERTA** → `INT-6` | Reale. Owner FoW = E27 |
| Power network: dentro o fuori il graph? | **NO ACTION** | Nessun power network è previsto da alcuna epic. Una decisione aperta su un sistema che nessuna release possiede resta aperta per sempre |

Già presenti e **non duplicate**: `INT-2` (fase del verbo), `INT-4` (costo del verbo), `MAP-2` (LOS che sfiora
l'angolo), `MAP-3` (cottura non invertibile). `MAP-3` in particolare è il rischio che l'handoff §2.4 descrive
come «conservare idempotenza e provenance»: è **aperto**, non risolto.

---

## F. Che cosa è cambiato

### Issue

| # | Titolo | Azione | CP | Feature |
|---|---|---|---|---|
| `#324` | `[EPIC v0.2] E23 · Muri, porte e interaction graph` | **aggiornata** — `23.6`/`23.7` aggiunti al corpo, `#620`/`#621` corrette da «aperte» a chiuse, sette **sub-issue** collegate | — | — |
| `#832` | L'identità di una struttura non sopravvive al cook | **nuova** | 23.3 | `RT-FEAT-MAP-STRUCTURE-IDENTITY` |
| `#833` | Chi apre `D1`? Il grafo sorgente → bersaglio non esiste | **nuova** | 23.4 | `RT-FEAT-MAP-INTERACTION-GRAPH` |
| `#834` | `S1` e `D1`: il giocatore non può sapere cosa controlla | **nuova** | 23.5 | `RT-FEAT-UI-STRUCTURE-READABILITY` |

`#619` `#620` `#621` `#712` sono state **collegate** a `#324` senza toccarne il corpo: erano già dichiarate
come anticipazione di `23.1`, mancava la relazione.

### Feature Registry

| Feature | Azione |
|---|---|
| `RT-FEAT-MAP-STRUCTURE-IDENTITY` | **nuova** · `v0.2` · `P1` · `DESIGNED` · `scenario: na` |
| `RT-FEAT-MAP-INTERACTION-GRAPH` | **nuova** · `v0.2` · `P1` · `DESIGNED` · tre scenari `planned` |
| `RT-FEAT-UI-STRUCTURE-READABILITY` | **nuova** · `v0.2` · `P2` · `DESIGNED` · `scenario: na` |
| `RT-FEAT-MAP-STANDABILITY` | `checkpoints: []` → `["23.6"]` |
| `RT-FEAT-MAP-TRANSITION-CLEARANCE` | `checkpoints: []` → `["23.7"]` |

### Scenari

Tre aggiunti alla Scenario Map, tutti su `RT-FEAT-MAP-INTERACTION-GRAPH`, tutti con oracolo `UnitAtCell`
disponibile oggi: `Spec.Map.Interaction.SwitchOpensDoor`, `…SwitchControlsMultipleDoors`,
`…OpenFailsDependentMoveBlocks`. Undici esclusi, con la ragione scritta accanto alla tabella.

### Decisioni

`D-138` nel Decision Log; `INT-5` e `INT-6` in `OPEN_DECISIONS.md`. Tre proposte dell'handoff **non** aperte.

### Un difetto trovato usandolo

`known_roadmap_refs()` leggeva i **checkpoint** dal solo `roadmap-v0.1.md`. Il 2026-08-13 `D-136` aveva
insegnato alla stessa funzione a leggere le **epic** anche dall'owner post-v0.1, ma non i checkpoint: scrivere
`checkpoints: ["23.3"]` produceva *«checkpoint inesistente»*. È il **terzo gemello** dello stesso difetto — i
primi due li aveva trovati la code review di `D-136` — e spiega perché le due feature `DESIGNED` di `E23`
portavano `checkpoints: []` pur avendo `23.6` e `23.7` nell'owner da `D-065`.

Corretto in `scripts/feature_registry.py`, con tre test in `scripts/test_feature_registry_releases.py`.
Verifica di mutazione: disattivata la lettura dell'owner post-v0.1 cade **esattamente**
`test_e23_checkpoints_from_the_post_v01_owner_are_known`, gli altri 29 restano verdi.

---

## H. Che cosa ha trovato la code review

Undici findings, e **due erano difetti introdotti da questo stesso lavoro**. Vale la pena elencarli perché
uno dei due è la forma di difetto che questo referto passa il tempo a denunciare negli altri.

### 🔴 Il gate reso muto da una tabella di prosa

La tabella §*L'orizzonte del dominio oltre la v0.2* usa `| **v0.2** |` come prima colonna — nello stesso file
che `release_table_rows()` parsa per sapere quali release l'owner dichiara. Il regex usava `[^|]*` per le
celle, e `[^|]` **include `\n`**: non trovando la terza colonna sulla stessa riga, la cercava oltre il ritorno
a capo e agganciava la riga successiva.

Misurato: `release_table_rows()` passava da **10** a **13** righe, con tre coppie fantasma `('v0.2','\n')`,
`('v0.4','\n')`, `('v0.9','\n')`. `check_release_order()` le contava come *«release dichiarate dall'owner»* —
quindi il gate che `D-136` aveva creato per impedire una release esprimibile e non descritta **taceva su tre
release**, e nulla diventava rosso: le release c'erano, due volte.

Corretto con `[^|\n]*`. Tre test lo pinnano, e sono **nati rossi** sul difetto presente: contano le righe,
rifiutano le celle epic vuote, rifiutano i duplicati. Verifica di mutazione: tolta la riga canonica di `v0.9`
dall'owner, il gate torna a dire *«è esprimibile e non descritta»*.

> Che sia successo **qui** è la parte istruttiva. Il referto apre dicendo che un pacchetto si filtra e si
> misura; e un gate è stato disattivato da una tabella di prosa scritta per **spiegare** la misura.

### 🔴 La citazione trascritta invece che misurata

`D-138` attribuiva a `Structures.Door.GroupClosesTogether` il `DoorId 7`, le tre celle e l'incremento singolo
della revisione. Nessuno dei tre gli appartiene: quel test usa `DoorId 3` su tre bordi di **una sola** cella —
e il suo commento avverte che *«il gruppo non deve essere rettilineo»*, cioè non è la porta larga. Il caso della
porta larga è `Structures.Door.StateChangeBumpsRevision`, che apre con *«Portone largo tre bordi: un comando,
una revisione»*.

La tesi regge — il gruppo atomico esiste ed è testato — ma **la prova citata era quella sbagliata**, e chi
l'avesse verificata aprendo `GroupClosesTogether` avrebbe trovato altri numeri. Il dato veniva dall'handoff,
non dal file: esattamente ciò che questo referto rimprovera all'handoff. Corretto in cinque punti.

### Applicati

`network_privacy` di `RT-FEAT-UI-STRUCTURE-READABILITY` passa da `na` a `todo` — `na` conta come **soddisfatto**
in `SATISFIED`, e la feature sarebbe potuta salire a `DONE` mentre `INT-6`, aperta dalla stessa decisione,
dice che la risposta serve *prima* che `#834` scelga dove filtrare. Le due feature che avevano il solo referto
come `owner_specs` prendono anche `roadmap-post-v0.1.md`: il referto dichiara in intestazione di non possedere
regole, quindi non poteva essere l'unico owner di nulla. E `known_roadmap_refs()` ora **itera** sui due owner
invece di duplicare i tre regex — con due forme che mancavano davvero: le righe barrate `| ~~**38.1**~~ |`
(un checkpoint *chiuso* resta *dichiarato*) e gli intervalli `` `CP 34.1`–`34.11` ``, che valgono 17 checkpoint
fra `E34` ed `E37`.

### Non applicati, e perché

| Finding | Decisione |
|---|---|
| La validazione dei checkpoint non è *scoped* per epic: `E23` accetta `39.13` | **Registrato, non corretto.** Il buco è **preesistente** (`if str(cp) not in checkpoints` ignora l'epic da sempre); questo lavoro lo allarga da 103 a 167 valori accettabili, il che lo rende più rilevante ma non lo introduce. Misurato l'impatto della correzione: **2 coppie** già incoerenti — `RT-FEAT-CORE-TURN` e `RT-FEAT-ACTION-DASH-DISPLACEMENT`, entrambe `epic: E2` con checkpoint di `E4`. Renderle rosse richiede decidere **quale dei due campi sia giusto**, che è una decisione sui dati e non sul parser. Issue [#841](https://github.com/DegrassiAaron/refactor-tactics-main/issues/841) |
| La tabella «Dove sta il lavoro» è una seconda copia di stato senza generatore | **Accettato in parte.** Le colonne *Issue* e *Feature* sono navigazione — l'owner dello stato resta il registry, e il referto lo dice. La colonna *Stato* invecchia davvero: è stata lasciata perché rimuoverla toglierebbe la risposta a «a che punto è `23.x`», ma non è generata e va riletta contro il registry, non creduta |

---

## G. Perimetro dei file

| Classe | Regola |
|---|---|
| `OWNER` | Editati a mano, sono la fonte |
| `GENERATED` | `feature-registry.json`, `project-graph.json`, le cinque `*.shortlist.md` — **rigenerati**, mai editati |
| `SNAPSHOT` | `roadmap_lane_*` — non toccati: sono fotografie, e aggiornarli meccanicamente ne distrugge il senso |
| `ARCHIVE` | L'handoff consumato, con la riga di provenienza in `docs/archive/src/README.md` |
